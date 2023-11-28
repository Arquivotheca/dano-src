// TextStatus.cpp

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <www/util.h> // for find_color() in libwww
#include <Debug.h>

#include "PulseContentInstance.h"

#define DEFAULT_UPDATE_FREQUENCY	1.0
#define SECTOMICROSEC(s)		((long)(1000000l * s))

// ---------------------- PulseContentInstance ----------------------

// TextDisplayView constructor
// Main purpose and side-effect: disassemble all parameters in the BMessage
// payload.  The idea here is that the parameter names can be run straight
// in from a ContentInstance, which in turn got them from the HTML EMBED
// tag.
// Secondary side-effect: Call a SetText to prime the value of fDisplayText.
PulseContentInstance::PulseContentInstance(Content *content,
													 GHandler *handler,
													 const BMessage& params)
	: // superclasses
	  ContentInstance(content, handler),
	  // default values
	  fNumBars(0), // this is some pretty bad hardware design right here
	  fDisplayPoint(0.0f, 0.0f),
	  fView(NULL),
	  fDisplayMode(pdCPU),
	  fDrawBackground(false)
{
	const char *value;
	
	if (params.FindString("mode", &value) == B_OK) {
		if (strcasecmp(value, "cpu") == 0)
			fDisplayMode = pdCPU;
		else if (strcasecmp(value, "mem") == 0)
			fDisplayMode = pdMem;
		else
			fDisplayMode = pdCPU;
	} else {
		fDisplayMode = pdCPU;
	}
	
	// set color, if specified
	if (params.FindString("framecolor", &value) == B_OK) {
		framecolor = decode_color(value);
	} else {
		framecolor = B_TRANSPARENT_32_BIT;
	}
	
	if (params.FindString("activecolor", &value) == B_OK) {
		if (strcasecmp(value, "transparent") == 0) {
			activecolor = B_TRANSPARENT_32_BIT;
		} else {
			activecolor = decode_color(value);
		}
	} else {
		activecolor = decode_color("#00cc00");
	}
	
	if (params.FindString("idlecolor", &value) == B_OK) {
		if (strcasecmp(value, "transparent") == 0) {
			idlecolor = B_TRANSPARENT_32_BIT;
		} else {
			idlecolor = decode_color(value);
		}
	} else {
		idlecolor = decode_color("#000000");
	}

//	if (params.FindString("bgcolor", &value) == B_OK) {
//		if (strcasecmp(value, "transparent") == 0) {
//			fBackgroundColor = B_TRANSPARENT_32_BIT;
//		} else {
//			fBackgroundColor = decode_color(value);
//		}
//	} else {
//		fBackgroundColor = B_TRANSPARENT_32_BIT;
//	}

	if (params.FindString("update_period", &value) == B_OK) {
		errno = 0;
		fUpdatePeriodSeconds = (float)strtod(value, NULL);
		if (errno != 0 || fUpdatePeriodSeconds == 0)
			fUpdatePeriodSeconds = DEFAULT_UPDATE_FREQUENCY;
	} else {
		fUpdatePeriodSeconds = DEFAULT_UPDATE_FREQUENCY;
	}

	PostDelayedMessage(new BMessage(bmsgUpdatePulse),
		SECTOMICROSEC(fUpdatePeriodSeconds));
}

// TextDisplayView destructor
// empty (no heap objects in class).
PulseContentInstance::~PulseContentInstance()
{

}

status_t
PulseContentInstance::AttachedToView(BView *view, uint32 *contentFlags)
{
	ContentInstance::AttachedToView(view, contentFlags);
	fView = view;
	*contentFlags |= cifHasTransparency;
	return B_OK;
}

status_t
PulseContentInstance::DetachedFromView()
{
	fView = NULL;
	return ContentInstance::DetachedFromView();
}

status_t
PulseContentInstance::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	return ContentInstance::GetSize(width, height, flags);
}

status_t
PulseContentInstance::FrameChanged(BRect newFrame, int32 fullWidth, int32 fullHeight)
{
	status_t r = ContentInstance::FrameChanged(newFrame, fullWidth, fullHeight);
	InvalidateCachedData();
	return r;
}

//status_t
//PulseContentInstance::Draw(BView *into, BRect exposed)
//{
//	into->PushState();
//	// make sure we can't draw outside our area
//	BRect r(FrameInParent());
//	BRegion clip;
//	clip.Set(r);
//	into->ConstrainClippingRegion(&clip);
//	// draw background and text
//	into->SetFont(&fFont);
//	into->SetHighColor(fTextColor);
//	if (fDrawBackground) {
//		into->SetLowColor(fBackgroundColor);
//		into->SetDrawingMode(B_OP_COPY);
//		into->FillRect(r, B_SOLID_LOW);
//	} else {
//		into->SetDrawingMode(B_OP_ALPHA);
//	}
//	into->DrawString(fDisplayText.String(), fDisplayPoint); 
//	into->PopState();
//	PRINT(("[TextDisplayView::Draw] Just drew text: '%s'\n", fDisplayText.String()));
//	return B_OK;
//}

// PulseContentInstance::Notification
// Inherited from ContentInstance.  Allows JS communication with
// the plugin.
void PulseContentInstance::Notification(BMessage *msg)
{
	PRINT(("[PulseContentInstance::Notification]\n"));
	PRINT_OBJECT(*msg);
	if (msg->what == bmsgUpdatePulse) {
		PRINT(("[PulseContentInstance::Notification] will update pulse bar\n"));

		Update();
		InvalidateCachedData();

		PostDelayedMessage(new BMessage(bmsgUpdatePulse),
			SECTOMICROSEC(fUpdatePeriodSeconds));
	} else {
		ContentInstance::Notification(msg);
	}
}



// ---- bindersupport below ----
status_t 
PulseContentInstance::OpenProperties(void **cookie, void *copyCookie)
{
	(void)cookie;
	(void)copyCookie;
	return B_OK;
}

status_t 
PulseContentInstance::NextProperty(void *cookie, char *nameBuf, int32 *len)
{
	(void)cookie;
	(void)nameBuf;
	(void)len;
	return ENOENT;
}

status_t 
PulseContentInstance::CloseProperties(void *cookie)
{
	(void)cookie;
	return B_OK;
}

put_status_t 
PulseContentInstance::WriteProperty(const char *name, const property &prop)
{
	(void)name;
	(void)prop;
	return EPERM;
}

get_status_t 
PulseContentInstance::ReadProperty(const char *name, property &prop, const property_list &args)
{
	(void)name;
	(void)prop;
	(void)args;
	return ENOENT;
}
// ---- bindersupport above ----



// PulseContentInstance::HandleMessage
// Inherited from GHandler.
// We use our Notification handler here.
status_t PulseContentInstance::HandleMessage(BMessage *message)
{
	Notification(message);
	return B_OK;
}

void PulseContentInstance::Cleanup()
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
//void PulseContentInstance::SetText(const char* text)
//{
//	if ((text != NULL) && (0 != fText.Compare(text))) {
//		fText.SetTo(text);
//		InvalidateCachedData();
//// // If we need to be drawn immediately
////		if (fView && fView->LockLooper()) {
////			fView->Invalidate();
////			fView->UnlockLooper();	
////		}
//		// If we need to update ourselves
//	}  
//}
// TextDisplayView::IsTransparent
// returns: boolean, whether or not the background color has been specified
// (and hence will be drawn by the view)
bool PulseContentInstance::IsTransparent() {
	return (!fDrawBackground);
}

// TextDisplayView::PositionDisplayText
// side-effects: Calculate internal state that allows Draw() to put things
// in the right place.
//void PulseContentInstance::PositionDisplayText() {
//
//	BRect f(FrameInParent());
//	PRINT(("[TextDisplayView::TruncateAndPositionText] our Bounds() is:\n\t"));
//	PRINT_OBJECT(f);
//	float w(ceilf(fFont.StringWidth(fText.String())));
//	float fw(ceilf(f.Width()));
//	font_height fh;
//	fFont.GetHeight(&fh);
//
//	fDisplayPoint.y = floorf(f.bottom - fh.descent);
//
//	if (w >= fw) {
//		// string needs to be truncated to fit in display area
//		fFont.TruncateString(&fDisplayText, B_TRUNCATE_END, fw);
//		fDisplayPoint.x = f.left;
//	} else {
//		float diff = fw - w;
//		switch (fAlign) {
//		case B_ALIGN_RIGHT:
//			fDisplayPoint.x = f.left + diff;
//			break;
//		case B_ALIGN_CENTER:
//			fDisplayPoint.x = f.left + floorf(diff / 2.0f);
//			break;
//		case B_ALIGN_LEFT: // fall through
//		default:
//			fDisplayPoint.x = f.left;
//			break;
//		}
//	}
//}


void 
PulseContentInstance::Update()
{
	switch(fDisplayMode) {
		case pdMem:
		{
			system_info fSysInfo;
			get_system_info(&fSysInfo);
			fNumBars = 1;
			fBarValue[0] = ((float) fSysInfo.used_pages
				/ (float) fSysInfo.max_pages);
		} break;
		
		case pdCPU:
		default:
		{
			system_info fSysInfo;
			get_system_info(&fSysInfo);
			
			bigtime_t now = system_time();
			//
			// Compute time spend in idle threads
			//
			fNumBars = fSysInfo.cpu_count;
			for(int i=0; i<fNumBars; i++ ) {
				double CPUTime = (double)(fSysInfo.cpu_infos[i].active_time -
							  prevactive[i]) / (now - prevtime);
				prevactive[i] = fSysInfo.cpu_infos[i].active_time;
				if(CPUTime < 0) CPUTime = 0;
				if(CPUTime > 1) CPUTime = 1;
				fBarValue[i] = CPUTime;
			}
			prevtime = now;
		} break;
	}
}


status_t 
PulseContentInstance::Draw(BView *into, BRect exposed)
{
	(void)exposed; // ahh, sue me
	
	if(fNumBars > B_MAX_CPU_COUNT) {
		return B_OK;
	}
	if(fNumBars <= 0) return B_OK;

	BRect bounds(FrameInParent());
	
	into->PushState();
	into->SetDrawingMode(B_OP_ALPHA);

	int h = (int)bounds.Height() - 2;
	float barwidth = (bounds.Width())/fNumBars - 1;
	
	BRect barframe;
	barframe.top = bounds.top + 1;
	barframe.bottom = barframe.top + h;
	barframe.left = bounds.left + 1;
	
	for(int i=0; i<fNumBars; i++) {

		// calculate the barvalue based on cached data
		// drawing code
		barframe.right = barframe.left + barwidth - 1;

		int barsize = (int)(fBarValue[i] * (h)); // h+1
		if (barsize > h)
			barsize = h;
		double rem = fBarValue[i] * (h) - barsize;
		PRINT(("PulseContent::Draw : remainder = %f\n", rem));
		//const rgb_color fractioncolor = {32, 64+128*rem, 32, 0};
		rgb_color fractioncolor;
#define FRACTIONCALC(comp) fractioncolor.comp = (uint8)(idlecolor.comp + rem * (activecolor.comp - idlecolor.comp))
		if (idlecolor.alpha == 0) {
			fractioncolor.red = activecolor.red;
			fractioncolor.blue = activecolor.blue;
			fractioncolor.green = activecolor.green;
		} else {
			FRACTIONCALC(red);
			FRACTIONCALC(blue);
			FRACTIONCALC(green);
		}
		FRACTIONCALC(alpha);
#undef FRACTIONCALC
		int idlesize = (int)(h-barsize);
		into->SetHighColor(framecolor);
		into->StrokeRect(bounds);  // BRect(left-1, top-1, right+1, bottom+1)
		if(idlesize > 0) {
			into->SetHighColor(idlecolor);
			into->FillRect(BRect(barframe.left, barframe.top,
				barframe.right, barframe.top+idlesize-1));
		}
		into->SetHighColor(fractioncolor);
		into->FillRect(BRect(barframe.left, barframe.bottom-barsize,
			barframe.right, barframe.bottom-barsize));
		if(barsize > 0) {
			into->SetHighColor(activecolor);
			into->FillRect(BRect(barframe.left, barframe.bottom-barsize+1,
				barframe.right, barframe.bottom));
		}
		barframe.left += (barwidth + 1);
	}
	into->PopState();
	
	return B_OK;
}

void PulseContentInstance::InvalidateCachedData() {
	// Update();
	MarkDirty();
}

// end of PulseContentInstance.cpp
