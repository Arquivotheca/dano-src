#include <stdio.h>
#include <Bitmap.h>
#include <Window.h>

// KnobSwitch.cpp
//
//   Stolen from the Tracker's DialogPane.cpp, and modified to display bitmaps
//   of a little knob.
//

#include "KnobSwitch.h"
#include "switch_bitmaps.h"	// the graphics for the switch bitmap

static const float kMovementThreshold = (float)2.0;

KnobSwitch::KnobSwitch(BRect frame, const char *name,
	uint32 resizeMask, uint32 flags)
	:	BControl(frame, name, "", 0, resizeMask, flags),
		fPressing(false), fCurrState(kUp)
{
	fUpBmap = new BBitmap(BRect(0, 0, kUpBitmapWidth - 1, kUpBitmapHeight - 1),
		kUpBitmapColorSpace);
	fMiddleBmap = new BBitmap(BRect(0, 0, kMiddleBitmapWidth - 1, kMiddleBitmapHeight - 1),
		kMiddleBitmapColorSpace);
	fDownBmap = new BBitmap(BRect(0, 0, kDownBitmapWidth - 1, kDownBitmapHeight - 1),
		kDownBitmapColorSpace);

	fUpBmap->SetBits(kUpBitmapBits, sizeof(kUpBitmapBits), 0, kUpBitmapColorSpace);
	fMiddleBmap->SetBits(kMiddleBitmapBits, sizeof(kMiddleBitmapBits), 0, kMiddleBitmapColorSpace);
	fDownBmap->SetBits(kDownBitmapBits, sizeof(kDownBitmapBits), 0, kDownBitmapColorSpace);
	SetViewColor(B_TRANSPARENT_COLOR);
}

KnobSwitch::~KnobSwitch()
{
	delete fUpBmap;
	delete fMiddleBmap;
	delete fDownBmap;
}

// make sure that we redraw after the keyboard changes our state
void 
KnobSwitch::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
	Invalidate();
}


void
KnobSwitch::Draw(BRect)
{
	DrawInState(fCurrState);
}

void
KnobSwitch::GetPreferredSize(float *width, float *height)
{
	BRect bounds;
	if (fUpBmap) {
		bounds = fUpBmap->Bounds();
	} else {
		bounds = Bounds();
	}
	*width = bounds.IntegerWidth();
	*height = bounds.IntegerHeight();
}


void 
KnobSwitch::DrawInState(KnobSwitch::State state)
{
	SetDrawingMode(B_OP_COPY);
	BBitmap *bmap = NULL;
	switch (state) {
		case kUp:
			bmap = fUpBmap;
			break;
		case kMiddle:
			bmap = fMiddleBmap;
			break;
		case kDown:
			bmap = fDownBmap;
			break;
	}
	DrawBitmap(bmap, BPoint(0, 0));
}

KnobSwitch::State 
KnobSwitch::StateForPosition(BPoint mousePos)
{
	BRect bounds(Bounds());

	State state;
	if (mousePos.y < (bounds.top + (bounds.Height() / 3))) {
		state = kUp;
	} else if (mousePos.y > (bounds.bottom - (bounds.Height() / 3))) {
		state = kDown;
	} else {
		state = kMiddle;
	}	

	return state;
}

// make sure that fCurrState stays consistent
void 
KnobSwitch::SetValue(int32 value)
{
	fCurrState = (value != 0) ? kDown : kUp;
	BControl::SetValue(value);
}


void KnobSwitch::MouseMoved( BPoint where, uint32 code, const BMessage *msg )
{
	if (!fMovedSinceClick && (fabs(where.x - fClickPoint.x) > kMovementThreshold
		|| fabs(where.y - fClickPoint.y) > kMovementThreshold))
	{
		fMovedSinceClick = true;
	}
	if( fPressing ) {
		State state = StateForPosition(where);
		if (state != fCurrState) {
			fCurrState = state;
			Invalidate();			
		}
	}
}

void KnobSwitch::MouseUp( BPoint where )
{
	fPressing = false;
	SetMouseEventMask( 0, 0 );
	State state = StateForPosition(where);
	if (!fMovedSinceClick || state != fStateWhenPressed) {
		SetValue(!Value());
		Invoke();
	}
}

void 
KnobSwitch::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;
	
	SetMouseEventMask( B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY );
	fPressing = true;
	fMovedSinceClick = false;
	fClickPoint = point;
	fStateWhenPressed = fCurrState;
	fCurrState = kMiddle;
	Invalidate();
}

// override AttachedToWindow so that the BControl version doesn't change the
// viewcolor out from under us.
void
KnobSwitch::AttachedToWindow()
{
	// if target/looper wasn't set then default to window
	if (!Messenger().IsValid()) {
		SetTarget(Window());
	}                                                
}

