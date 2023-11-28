// TextStatus.cpp

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <www/util.h> // for find_color() in libwww
#include <Debug.h>

#include "TextStatusContentInstance.h"

#define DEFAULT_FONT_SIZE	12.0f
#define DEFAULT_ALIGNMENT	B_ALIGN_LEFT

#if 0
rgb_color set_color(rgb_color* col, uint8 r, uint8 g, uint8 b, uint8 a = 128)
{
	col->red = r;
	col->green = g;
	col->blue = b;
	col->alpha = a;
	return *col;
}
#endif

// ---------------------- TextStatusContentInstance ----------------------

// TextDisplayView constructor
// Main purpose and side-effect: disassemble all parameters in the BMessage
// payload.  The idea here is that the parameter names can be run straight
// in from a ContentInstance, which in turn got them from the HTML EMBED
// tag.
// Secondary side-effect: Call a SetText to prime the value of fDisplayText.
TextStatusContentInstance::TextStatusContentInstance(Content *content,
													 GHandler *handler,
													 const BMessage& params)
	: // superclasses
	  ContentInstance(content, handler),
	  // default values
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
	if (params.HasString(S_TEXT_STATUS_PARAM_COLOR)) {
		fTextColor = decode_color(
			params.FindString(S_TEXT_STATUS_PARAM_COLOR));
	}

	// set background color, if specified
	if (params.HasString(S_TEXT_STATUS_PARAM_BGCOLOR)) {
		fBackgroundColor = decode_color(params.FindString(S_TEXT_STATUS_PARAM_BGCOLOR));
		fDrawBackground = true;
	} else {
		fBackgroundColor = B_TRANSPARENT_32_BIT;
	}

	// set font spec using the new decode_font syntax, if so requested
	if (params.HasString(S_TEXT_STATUS_PARAM_FONT)) {
		if (B_OK != decode_font(
				params.FindString(S_TEXT_STATUS_PARAM_FONT),
				&fFont, &fFont)) 
		{
			// fall back to previous behavior
			if (params.HasString(S_TEXT_STATUS_PARAM_SIZE)) {
				const char *sizeStr 
					= params.FindString(S_TEXT_STATUS_PARAM_SIZE);
				errno = 0;
				fFontSize = (float)strtod(sizeStr, NULL);
				if (errno != 0) {
					fFontSize = DEFAULT_FONT_SIZE;
				}
			}
			fFont.SetSize(fFontSize); // set font size, regardless.
		}
	}

	
	// set text alignment, if specified
	if (params.HasString(S_TEXT_STATUS_PARAM_ALIGN)) {
		BString alignStr(params.FindString(S_TEXT_STATUS_PARAM_ALIGN));
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
	if (params.HasString(S_TEXT_STATUS_PARAM_VALUE)) {
		// SetText(params.FindString(S_TEXT_STATUS_PARAM_VALUE));
		fText.SetTo(params.FindString(S_TEXT_STATUS_PARAM_VALUE));
	} 

	PRINT(("[TextDisplayView::TextDisplayView] Just initialized with value: %s\n", fText.String()));
	PRINT(("[TextDisplayView::TextDisplayView] DisplayText: '%s'\n", fDisplayText.String()));
}

// TextDisplayView destructor
// empty (no heap objects in class).
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
TextStatusContentInstance::Draw(BView *into, BRect UNUSED(exposed))
{
	into->PushState();
	// make sure we can't draw outside our area
	BRect r(FrameInParent());
	BRegion clip;
	clip.Set(r);
	into->ConstrainClippingRegion(&clip);
	into->SetDrawingMode(B_OP_COPY);
	// draw background, if necessary
	if (fDrawBackground) {
		if (fBackgroundColor.alpha != 255) {
			into->SetDrawingMode(B_OP_ALPHA);
		}
		into->SetLowColor(fBackgroundColor);
		into->FillRect(r, B_SOLID_LOW);
	}
	// draw text
	into->SetFont(&fFont);
	into->SetHighColor(fTextColor);
	into->SetDrawingMode(B_OP_OVER);
	if (fTextColor.alpha != 255) {
		into->SetDrawingMode(B_OP_ALPHA);	
	} else if (!fDrawBackground || (fDrawBackground && (fBackgroundColor.alpha != 255))) {
		into->SetDrawingMode(B_OP_OVER);
	}
	into->DrawString(fDisplayText.String(), fDisplayPoint); 
	into->PopState();
	PRINT(("[TextDisplayView::Draw] Just drew text: '%s'\n", fDisplayText.String()));
	return B_OK;
}

// TextStatusContentInstance::Notification
// Inherited from ContentInstance.  Allows JS communication with
// the plugin.
void TextStatusContentInstance::Notification(BMessage *msg)
{
	PRINT(("[TextStatusContentInstance::Notification]\n"));
	PRINT_OBJECT(*msg);
	switch (msg->what) {
		case bmsgLegacySetText: {
			if (msg->HasString(S_UPDATE_TEXT_LEGACY_DATA)) {
				SetText(msg->FindString(S_UPDATE_TEXT_LEGACY_DATA));
			}
		} 
		case bmsgSetText: {
			if (msg->HasString(S_UPDATE_TEXT_DATA)) {
				SetText(msg->FindString(S_UPDATE_TEXT_DATA));
			}
		} break;
		default: {
			ContentInstance::Notification(msg);
		} break;
	}
}

status_t 
TextStatusContentInstance::OpenProperties(void **UNUSED(cookie), void *UNUSED(copyCookie))
{
	return B_OK;
}

status_t 
TextStatusContentInstance::NextProperty(void *UNUSED(cookie), char *UNUSED(nameBuf), int32 *UNUSED(len))
{
	return ENOENT;
}

status_t 
TextStatusContentInstance::CloseProperties(void *UNUSED(cookie))
{
	return B_OK;
}

put_status_t 
TextStatusContentInstance::WriteProperty(const char *name, const property &cprop)
{
	// XXX FIXME - we need a const version of property::operator []
	property &prop = const_cast<property &>(cprop);
	if (!strcmp(name,"text")) {
		printf("WARNING: start using 'displayText' instead of 'text' for TextStatus plugin!\n");
		SetText(prop.String().String());
		return B_OK;
	}

	if (!strcmp(name,"fgColor")) {
		rgb_color color;
		color.alpha = 255;
		color.red = (int)(255 * prop["r"].Number());
		color.green = (int)(255 * prop["g"].Number());
		color.blue = (int)(255 * prop["b"].Number());
		property alpha = prop["a"];
		if (!alpha.IsUndefined()) color.alpha = (int)(255*alpha.Number());
		fTextColor = color;
		InvalidateCachedData();
		return B_OK;
	}

	if (!strcmp(name,"bgColor")) {
		rgb_color color;
		color.alpha = 255;
		color.red = (int)(255 * prop["r"].Number());
		color.green = (int)(255 * prop["g"].Number());
		color.blue = (int)(255 * prop["b"].Number());
		property alpha = prop["a"];
		if (!alpha.IsUndefined()) color.alpha = (int)(255*alpha.Number());
		fBackgroundColor = color;
		fDrawBackground = true;
		InvalidateCachedData();
		return B_OK;
	}

	if (!strcmp(name,"displayText")) {
		SetText(prop.String().String());
		return B_OK;
	}
	
	return EPERM;
}

get_status_t 
TextStatusContentInstance::ReadProperty(const char *name, property &prop, const property_list &UNUSED(args))
{
	if (!strcmp(name,"text")) {
		printf("WARNING: start using 'displayText' instead of 'text' for TextStatus plugin!\n");
		prop = fText.String();
		return B_OK;
	}

	if (!strcmp(name,"displayText")) {
		prop = fText.String();
		return B_OK;
	}

	return ENOENT;
}

// TextStatusContentInstance::HandleMessage
// Inherited from GHandler.
// We use our Notification handler here.
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

// TextDisplayView::SetText
// input: char const *
// side-effect: mutates fText, calls FreshenText (to perform any post-
// processing) and TruncateAndPositionText (to calculate positions on-screen,
// truncate text with an ellipsis if necessary).
void TextStatusContentInstance::SetText(const char* text)
{
	if ((text != NULL) && (0 != fText.Compare(text))) {
		fText.SetTo(text);
		InvalidateCachedData();
// // If we need to be drawn immediately
//		if (fView && fView->LockLooper()) {
//			fView->Invalidate();
//			fView->UnlockLooper();	
//		}
		// If we need to update ourselves
	}  
}
// TextDisplayView::IsTransparent
// returns: boolean, whether or not the background color has been specified
// (and hence will be drawn by the view)
bool TextStatusContentInstance::IsTransparent() {
	return (!fDrawBackground);
}

// TextDisplayView::FilterText
// side-effects: copies string data from fText to fDisplayText
// This function is provided as a hook for subclasses to do further
// text processing on the string in fText.
void TextStatusContentInstance::FilterText() {
	PRINT(("[TextDisplayView::FreshenText] setting display text to '%s'\n",
		fText.String()));
	fDisplayText.SetTo(fText);
	PRINT(("[TextDisplayView::FreshenText] fDisplayText.String (%p) = '%s'\n",
		fDisplayText.String(), fDisplayText.String()));
}

// TextDisplayView::PositionDisplayText
// side-effects: Calculate internal state that allows Draw() to put things
// in the right place.
void TextStatusContentInstance::PositionDisplayText() {

	BRect f(FrameInParent());
	PRINT(("[TextDisplayView::TruncateAndPositionText] our Bounds() is:\n\t"));
	PRINT_OBJECT(f);
	float w(ceilf(fFont.StringWidth(fText.String())));
	float fw(ceilf(f.Width()));
	font_height fh;
	fFont.GetHeight(&fh);

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
}

void TextStatusContentInstance::InvalidateCachedData() {
	FilterText();
	PositionDisplayText();
	MarkDirty();
}
