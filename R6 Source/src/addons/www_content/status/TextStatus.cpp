// TextStatus.cpp

#include "TextStatus.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <www/util.h> // for find_color() in libwww

#define DEFAULT_FONT_SIZE	12.0f
#define DEFAULT_ALIGNMENT	B_ALIGN_LEFT

rgb_color set_color(rgb_color* col, uint8 r, uint8 g, uint8 b, uint8 a = 128)
{
	col->red = r;
	col->green = g;
	col->blue = b;
	col->alpha = a;
	return *col;
}

// ---------------------- TextStatusContentInstance ----------------------

TextStatusContentInstance::TextStatusContentInstance(Content *content,
													 GHandler *handler,
													 const BMessage& params)
	: ContentInstance(content, handler),
	  BinderNode(),
	  fText(B_EMPTY_STRING),
	  fDisplayText(B_EMPTY_STRING),
	  fFont(be_plain_font),
	  fDisplayPoint(0.0f, 0.0f),
	  fView(NULL),
	  fFontSize(DEFAULT_FONT_SIZE),
	  fAlign(DEFAULT_ALIGNMENT),
	  fDrawBackground(false)
{
	// set text color, if specified
	if (params.HasString(STATUS_TEXT_COLOR)) {
		if (find_color(&params, STATUS_TEXT_COLOR, &fTextColor) != B_OK) {
			set_color(&fTextColor, 0, 0, 0); // defaults to black	
		}
	} else {
		set_color(&fTextColor, 0, 0, 0); // defaults to black	
	}

	// set background color, if specified
	if (params.HasString(STATUS_TEXT_BGCOLOR)) {
		if (find_color(&params, STATUS_TEXT_BGCOLOR, &fBackgroundColor) != B_OK) {
			set_color(&fBackgroundColor, 255, 255, 255); // defaults to white
		} else {
			fDrawBackground = true;
		}
	} else {
		set_color(&fBackgroundColor, 255, 255, 255); // defaults to white
	}

	// set font size, if specified
	if (params.HasString(STATUS_TEXT_SIZE)) {
		const char *sizeStr = params.FindString(STATUS_TEXT_SIZE);
		errno = 0;
		fFontSize = (float)strtod(sizeStr, NULL);
		if (errno != 0) {
			fFontSize = DEFAULT_FONT_SIZE;
		}
	}

	fFont.SetSize(fFontSize);
	
	// set text alignment, if specified
	if (params.HasString(STATUS_TEXT_ALIGN)) {
		BString alignStr(params.FindString(STATUS_TEXT_ALIGN));
		alignStr.ToLower();
		if (!alignStr.Compare("right")) {
			fAlign = B_ALIGN_RIGHT;
		} else if (!alignStr.Compare("center")) {
			fAlign = B_ALIGN_CENTER;
		} else {
			// left alignment is the default
			fAlign = B_ALIGN_LEFT;
		}
	}

	// set initial text value, if specified
	if (params.HasString(STATUS_TEXT_VALUE)) {
		fText.SetTo(params.FindString(STATUS_TEXT_VALUE));
	}
}

TextStatusContentInstance::~TextStatusContentInstance()
{

}

status_t
TextStatusContentInstance::AttachedToView(BView *view, uint32 *contentFlags)
{
	ContentInstance::AttachedToView(view, contentFlags);
	fView = view;
	*contentFlags |= cifHasTransparency;
	return B_OK;
}

status_t
TextStatusContentInstance::DetachedFromView()
{
	fView = NULL;
	return ContentInstance::DetachedFromView();
}

status_t
TextStatusContentInstance::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	return ContentInstance::GetSize(width, height, flags);
}

status_t
TextStatusContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	status_t r = ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
	InvalidateCachedData();
	return r;
}

status_t
TextStatusContentInstance::Draw(BView *into, BRect exposed)
{
	into->PushState();
	// make sure we can't draw outside our area
	BRect r(FrameInParent());
	BRegion clip;
	clip.Set(r);
	into->ConstrainClippingRegion(&clip);
	// draw background and text
	into->SetFont(&fFont);
	into->SetHighColor(fTextColor);
	if (fDrawBackground) {
		into->SetLowColor(fBackgroundColor);
		into->SetDrawingMode(B_OP_COPY);
		into->FillRect(r, B_SOLID_LOW);
	} else {
		into->SetDrawingMode(B_OP_ALPHA);
	}
	into->DrawString(fDisplayText.String(), fDisplayPoint); 
	into->PopState();
	return B_OK;
}

void TextStatusContentInstance::Notification(BMessage *msg)
{
	switch (msg->what) {
	case UPDATE_STATUS_TEXT:
		if (msg->HasString(STATUS_TEXT_LABEL)) {
			SetText(msg->FindString(STATUS_TEXT_LABEL));
		}
	default:
		ContentInstance::Notification(msg);
	}
}

status_t 
TextStatusContentInstance::OpenProperties(void **cookie, void *copyCookie)
{
	return B_OK;
}

status_t 
TextStatusContentInstance::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	return ENOENT;
}

status_t 
TextStatusContentInstance::CloseProperties(void *cookie)
{
	return B_OK;
}

put_status_t 
TextStatusContentInstance::WriteProperty(const char *name, const property &prop)
{
	if (!strcmp(name,"text")) {
		SetText(prop.String().String());
		return B_OK;
	}
	
	return EPERM;
}

get_status_t 
TextStatusContentInstance::ReadProperty(const char *name, property &prop, const property_list &args)
{
	return ENOENT;
}

status_t TextStatusContentInstance::HandleMessage(BMessage *message)
{
	Notification(message);
	return B_OK;
}

void TextStatusContentInstance::Cleanup()
{
	// must call both parent's Cleanup() methods
	ContentInstance::Cleanup();
	BinderNode::Cleanup();
}

void TextStatusContentInstance::SetText(const char* text)
{
	if ((text != NULL) && fText.Compare(text)) {
		fText.SetTo(text);
		InvalidateCachedData();
//		if (fView && fView->LockLooper()) {
//			fView->Invalidate();
//			fView->UnlockLooper();	
//		}
	}  
}

void TextStatusContentInstance::InvalidateCachedData()
{
	BRect f(FrameInParent());
	float w(ceilf(fFont.StringWidth(fText.String())));
	float fw(ceilf(f.Width()));
	font_height fh;
	fFont.GetHeight(&fh);

	fDisplayText.SetTo(fText);
	fDisplayPoint.y = floorf(f.bottom - fh.descent);

	if (w >= fw) {
		// string needs to be truncated to fit in display area
		fFont.TruncateString(&fDisplayText, B_TRUNCATE_END, fw);
		fDisplayPoint.x = f.left;
	} else {
		float diff = fw - w;
		switch (fAlign) {
		case B_ALIGN_RIGHT:
			fDisplayPoint.x = f.left + diff;
			break;
		case B_ALIGN_CENTER:
			fDisplayPoint.x = f.left + floorf(diff / 2.0f);
			break;
		case B_ALIGN_LEFT: // fall through
		default:
			fDisplayPoint.x = f.left;
			break;
		}
	}
	MarkDirty();	
}

// ---------------------- TextStatusContent ----------------------

TextStatusContent::TextStatusContent(void* handle)
	: Content(handle)
{
}

TextStatusContent::~TextStatusContent()
{
}

ssize_t
TextStatusContent::Feed(const void *buffer, ssize_t bufferLen, bool done)
{
	return bufferLen;
}

size_t
TextStatusContent::GetMemoryUsage()
{
	return (sizeof(*this));
}

bool
TextStatusContent::IsInitialized()
{
	return true;
}

status_t
TextStatusContent::CreateInstance(ContentInstance **outInstance,
								  GHandler *handler, const BMessage &msg)
{
	*outInstance = new TextStatusContentInstance(this, handler, msg);
	return B_OK;
}

// ----------------------- JapaneseIMContentFactory -----------------------


void TextStatusContentFactory::GetIdentifiers(BMessage* into)
{
	 /*
	 ** BE AWARE: Any changes you make to these identifiers should
	 ** also be made in the 'addattr' command in the makefile.
	 */
	into->AddString(S_CONTENT_MIME_TYPES, "application/x-vnd.Be.TextStatus");
}
	
Content* TextStatusContentFactory::CreateContent(void* handle,
												 const char* mime,
												 const char* extension)
{
	(void)mime;
	(void)extension;
	return new TextStatusContent(handle);
}

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if (n == 0) return new TextStatusContentFactory;
	return 0;
}
