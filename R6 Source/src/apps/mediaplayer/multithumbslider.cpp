#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Font.h>
#include <Region.h>
#include <Screen.h>

#include "multithumbslider.h"
#include "DrawingTidbits.h"

/*------------------------------------------------------------*/



rgb_color
DisabledColor(rgb_color c)
{
	return ShiftColor(c, 0.5);
}

color_space
BitsPerPixel()
{
	BScreen b(B_MAIN_SCREEN_ID);
	color_space c = b.ColorSpace();
	return c;
}

float
FontHeight(const BView* v, bool full)
{
	font_height finfo;
	v->GetFontHeight(&finfo);
	float height = finfo.ascent + finfo.descent;
	if (full)
		height += finfo.leading;
		
	return height;
}

/*------------------------------------------------------------*/

TThumb::TThumb(TMultiThumbSlider *owner, BMessage *mdMsg,
	BMessage *stdMsg, BMessage *modMsg)
	:	fOwner(owner),
		fValue(0),
		fMouseDownMsg(mdMsg),
		fStandardMsg(stdMsg),
		fModificationMsg(modMsg)
{
	fInitialLocation.Set(0,0);
	fLocation.Set(0,0);
}


TThumb::~TThumb()
{
	delete fMouseDownMsg;
	delete fStandardMsg;
	delete fModificationMsg;
}

void 
TThumb::Draw(BView *osv, BRect barRect)
{
	float loc = Location().x;

	// 	fill triangle , white/gray
	rgb_color color = (fOwner->IsEnabled()) ? kWhiteGrayColor : DisabledColor(kWhiteGrayColor);
	osv->SetHighColor(color);
	osv->FillTriangle(BPoint(loc,barRect.bottom-3),
		BPoint(loc-6,barRect.bottom+3),
		BPoint(loc+6,barRect.bottom+3) );

	osv->BeginLineArray(6);
	
	color = kBlackColor;
	if (!fOwner->IsEnabled()) color = DisabledColor(color);
	// black right edge
	osv->AddLine(BPoint(loc,barRect.bottom-3),
		BPoint(loc+6,barRect.bottom+3), color);
	// black base line
	osv->AddLine(BPoint(loc-6,barRect.bottom+4),
		BPoint(loc+6,barRect.bottom+4), color);
	// left edge, med gray
	color.red = 120; color.green = 120; color.blue = 120;
	if (!fOwner->IsEnabled()) color = DisabledColor(color);
	osv->AddLine(BPoint(loc-6,barRect.bottom+3),
		BPoint(loc-1,barRect.bottom-2), color);
	// bottom med gray
	if (!fOwner->IsEnabled()) color = DisabledColor(color);
	osv->AddLine(BPoint(loc-6,barRect.bottom+3),
		BPoint(loc+5,barRect.bottom+3), color);
	// right edge shade, lt gray
	color.red = 200; color.green = 200; color.blue = 200;
	if (!fOwner->IsEnabled()) color = DisabledColor(color);
	osv->AddLine(BPoint(loc,barRect.bottom-2),
		BPoint(loc+4,barRect.bottom+2), color);
	// bottom edge, lt gray
	if (!fOwner->IsEnabled()) color = DisabledColor(color);
	osv->AddLine(BPoint(loc-3,barRect.bottom+2),
		BPoint(loc+4,barRect.bottom+2), color);
	
	osv->EndLineArray();
}

int32 
TThumb::Value() const
{
	return fValue;
}

void 
TThumb::SetValue(int32 v)
{
	if (v == fValue)
		return;
		
	fValue = v;
}

BPoint 
TThumb::InitialLocation() const
{
	return fInitialLocation;
}

void 
TThumb::SetInitialLocation(BPoint point)
{
 	if (point == fInitialLocation)
		return;
		
	fInitialLocation = point;
}

BPoint 
TThumb::Location() const
{
	return fLocation;
}

void 
TThumb::SetLocation(BPoint point)
{
 	if (point == fLocation)
		return;
		
	fLocation = point;
}

BRect
TThumb::Frame() const
{
	BPoint loc(Location());
	BRect barFrame(const_cast<TThumb*>(this)->Owner()->BarFrame());
	BRect frame(loc.x-6, barFrame.top, loc.x+6, barFrame.bottom);
	
	return frame;
}

bool
TThumb::HitTest(BPoint point) const
{
	BRect frame(Frame());
	return point.x >= frame.left && point.x <= frame.right;
}

BMessage *
TThumb::MouseDownMessage() const
{
	return fMouseDownMsg;
}

void 
TThumb::SetMouseDownMessage(BMessage *message)
{
	if (message == fMouseDownMsg)
		return;
		
	delete message;
	fMouseDownMsg = message;
}

BMessage *
TThumb::Message() const
{
	return fStandardMsg;
}

void 
TThumb::SetMessage(BMessage *message)
{
	if (message == fStandardMsg)
		return;
		
	delete message;
	fStandardMsg = message;
}

BMessage *
TThumb::ModificationMessage() const
{
	return fModificationMsg;
}

void 
TThumb::SetModificationMessage(BMessage *message)
{
	if (message == fModificationMsg)
		return;
		
	delete message;
	fModificationMsg = message;
}

/*------------------------------------------------------------*/

TMultiThumbSlider::TMultiThumbSlider(BRect frame, const char *name, const char *label,
	int32 minValue, int32 maxValue, uint32 resizingMode, uint32 flags)
	:	BControl(frame, name, label, NULL, resizingMode, flags)
{
	_InitObject(minValue, maxValue);
}

void
TMultiThumbSlider::_InitObject(int32 minValue, int32 maxValue)
{
	fThumbList = new BList();
		
	fOffScreenBits = NULL;
	fOffScreenView = NULL;		

	fMinValue = minValue;
	fMaxValue = maxValue;

	fSnoozeAmount = kDefaultSnoozeAmount;
	
	fKeyIncrementValue = 1;
			
	SetBarColor(kUnusedColor);

	UseFillColor(false);
	fCurrentThumb = -1;
	
	fTrackingOffset = 0;
}

TMultiThumbSlider::~TMultiThumbSlider()
{
	if (IsTracking() && fCurrentThumb != -1) 
		// if we are still tracking, make sure we send the done
		// tracking message
		// this is probably not the best thing to do for general use,
		// it is needed for the media player tracking though because
		// not getting the 'done tracking' message screws it up.
		Invoke(MessageFor(fCurrentThumb));


	delete fOffScreenBits;

	for (int32 last = ThumbCount() - 1 ; last >= 0 ; last--) 
		delete (TThumb *)fThumbList->RemoveItem(last);
	
	delete fThumbList;
}

void
TMultiThumbSlider::AttachedToWindow()
{
	float width,height;
	
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
	
	BRect offscreenRect(Bounds());
	
	if (!fOffScreenView)
		fOffScreenView = new BView(offscreenRect, "", B_FOLLOW_ALL, B_WILL_DRAW);
	
	if (!fOffScreenBits)
		fOffScreenBits = new BBitmap(offscreenRect, BitsPerPixel(), true);
	
	if (fOffScreenBits && fOffScreenView)
		fOffScreenBits->AddChild(fOffScreenView);
		
	if (Parent()) {
		rgb_color color = Parent()->ViewColor();
		SetViewColor(color);
		SetLowColor(color);
	}

	BControl::AttachedToWindow();
}

void
TMultiThumbSlider::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
	
	delete fOffScreenBits;			// will delete the off screen view as well
	
	fOffScreenBits = NULL;
	fOffScreenView = NULL;
}

void
TMultiThumbSlider::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}

void
TMultiThumbSlider::FrameMoved(BPoint newPosition)
{
	BControl::FrameMoved(newPosition);
}

void
TMultiThumbSlider::FrameResized(float width, float height)
{
	
	BRect bounds(Bounds());

	if (bounds.IsValid()) {
		if (fOffScreenBits){
			fOffScreenBits->RemoveChild(fOffScreenView);
			delete fOffScreenBits;
	
			fOffScreenView->ResizeTo(bounds.Width(), bounds.Height());
			
			fOffScreenBits = new BBitmap(bounds, BitsPerPixel(), true);
			fOffScreenBits->AddChild(fOffScreenView);
		}
		
		// this is not optimal for now
		// unfortunately the location and value are independent
		int32 count = ThumbCount();
		for (int32 i = 0 ; i < count ; i++) 
			// 	this is similar to SetValueFor, except the message is not sent
			//	the location is simply updated
			_SetLocation(PointForValue(ValueFor(i)), i);
	}
	BControl::FrameResized(width, height);
	Invalidate(Bounds());
}

void 
TMultiThumbSlider::KeyDown(const char *bytes, int32 n)
{
	if (!IsEnabled() || IsHidden())
		return;

	switch (bytes[0]) {
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
			break;
		case B_UP_ARROW:
		case B_RIGHT_ARROW:
			break;
		case B_ENTER:
		case B_SPACE:
			// I don't want to pass these on, if you think so, what should it do?
			break;

		default:
			BControl::KeyDown(bytes, n);
			break;
	}
}

#define SNAP_TO_CLICK

void
TMultiThumbSlider::MouseDown(BPoint point)
{
	// only do something if we're enabled
	if (!IsEnabled())
		return;

	fCurrentThumb = PointInThumb(point);
#ifdef SNAP_TO_CLICK
	bool snapToClick = false;
	if (fCurrentThumb == -1) {
		snapToClick = true;
		fCurrentThumb = kMainThumb;
	}
#else
	if (fCurrentThumb == -1)
		return;
#endif

	if (MouseDownMessageFor(fCurrentThumb))
		Invoke(MouseDownMessageFor(fCurrentThumb));

	// the mouse btn is held down, track it and update
	BPoint currPosition(_Location(fCurrentThumb));
	float initialLocation = currPosition.x;

	fTrackingOffset = currPosition.x - point.x;
#ifdef SNAP_TO_CLICK
	if (snapToClick)
		// clidked outside of thumb - consider as if we clicked right in the
		// middle
		fTrackingOffset = 0;
#endif

	while (true) {
	
		BPoint where;
		uint32 buttons;
		GetMouse(&where, &buttons);
		if (!buttons) {
			if (ModificationMessageFor(fCurrentThumb))
				Invoke(ModificationMessageFor(fCurrentThumb));
			break;
		}
		// 	if this is in an asynch window then
		//	set up for tracking and exit this loop
		//	mouse tracking is then handled in MouseMoved

		// We can assume that async controls work.
		if ( /* (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0 */ true
		
#ifdef SNAP_TO_CLICK
			&& !snapToClick	// if snap to click, let it fall through the
							// first time to make the thumb jump to the
							// mouse - we would not get that otherwise because
							// MouseMoved would not kick in until we move
#endif
		) {

			SetTracking(true);
			SetMouseEventMask(B_POINTER_EVENTS,
				B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
			//	tell the slider to draw now so that the
			//	state can change if necessary
			DrawSlider(Bounds());
			break;
		}


		if (
#ifdef SNAP_TO_CLICK
			snapToClick ||
#endif
			where.x != currPosition.x) {

#ifdef SNAP_TO_CLICK
			snapToClick = false;
#endif
		
			where.x += fTrackingOffset;
			
			if (where.x <= _MinPosition())
				where.x = _MinPosition();
			else if (where.x >= _MaxPosition())
				where.x = _MaxPosition();
		
			SetValueFor(ValueForPoint(where), fCurrentThumb);

			//	send the mod message not the stnd invoke msg					
			if (ModificationMessageFor(fCurrentThumb))
				Invoke(ModificationMessageFor(fCurrentThumb));
		}

		snooze(SnoozeAmount());
		currPosition = where;		
	}
	
	_SetInitialLocation(currPosition, fCurrentThumb);

	// only invoke if not asynch
	// We can assume that async controls work.
	if ( /*(Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) == 0*/ false) {
		if (initialLocation != _Location(fCurrentThumb).x) {
			Invoke(MessageFor(fCurrentThumb));
			fCurrentThumb = -1;
		}
	}
}

void
TMultiThumbSlider::MouseUp(BPoint point)
{
	if (!IsTracking()) {
		BControl::MouseUp(point);
		return;
	}

	// stnd msg is only sent on single click/release of mouse btn
	if (fCurrentThumb != -1
		// && _InitialLocation(fCurrentThumb) != _Location(fCurrentThumb)
		// always post Invoke message because it is also used
		// as a end-of-tracking message
		) 
		Invoke(MessageFor(fCurrentThumb));

		
	//	turn off tracking
	SetTracking(false);
	fCurrentThumb = -1;
}

void
TMultiThumbSlider::MouseMoved(BPoint point, uint32 code, const BMessage *message)
{
	if (!IsTracking()) {
		BControl::MouseMoved(point, code, message);
		return;
	}

	if (fCurrentThumb == -1)
		return;

	BPoint newLoc(point);
	BPoint currPosition(_Location(fCurrentThumb));
		
	if (newLoc.x != currPosition.x) {

		newLoc.x += fTrackingOffset;
		
		if (newLoc.x <= _MinPosition())
			newLoc.x = _MinPosition();
		else if (newLoc.x >= _MaxPosition())
			newLoc.x = _MaxPosition();

		SetValueFor(ValueForPoint(newLoc), fCurrentThumb);
	}
	
	if (currPosition != _Location(fCurrentThumb)) {
		//	send the mod message not the stnd invoke msg					
		if (ModificationMessageFor(fCurrentThumb))
			Invoke(ModificationMessageFor(fCurrentThumb));
	}
}

//
//	set the actual value (between minvalue and maxvalue)
//	
void
TMultiThumbSlider::SetValue(int32 value)
{
ASSERT_WITH_MESSAGE(0, "use SetValueFor(int32, int32)");
	// check the bounds
	if (value < fMinValue) value = fMinValue;
	if (value > fMaxValue) value = fMaxValue;
	
	// offset to actual min and max
	float min = value - fMinValue;
	float max = fMaxValue - fMinValue;
	float val = (min / max) * (_MaxPosition() - _MinPosition());
	val = ceil(val);
	
	//	get the new location
	BPoint p;
	p.x = val + _MinPosition();
	p.y = 0;

	_SetLocation(p);	

	BControl::SetValue(value);
}


void
TMultiThumbSlider::SetValueFor(int32 value, int32 index, bool,
	bool lazy)
{
	if (index < 0 || index >= ThumbCount())
		return;

	TThumb *thumb = (TThumb *)fThumbList->ItemAt(index);
	if (lazy && thumb->Value() == value)
		return;
		
	BPoint point(PointForValue(value));
	
	//	Only invalidate the old thumb rect, the new thumb rect and the
	// area in between

	BRect oldThumbRect(thumb->Frame());

	_SetLocation(point, index);
	_SetValue(value, index);
	
	BControl::SetValueNoUpdate(value);
		// avoid invalidate by SetValue

	Invalidate(oldThumbRect | thumb->Frame());
}

int32
TMultiThumbSlider::ValueFor(int32 index) const
{
	return _Value(index);
}

void
TMultiThumbSlider::_SetValue(int32 value, int32 index)
{
	if (index < 0 || index >= ThumbCount())
		return;

	ASSERT(fThumbList->ItemAt(index));
	((TThumb*)fThumbList->ItemAt(index))->SetValue(value);
}

int32
TMultiThumbSlider::_Value(int32 index) const
{
	if (index < 0 || index >= ThumbCount())
		return -1;	// assumes positive only
	
	ASSERT(fThumbList->ItemAt(index));
	return ((TThumb*)fThumbList->ItemAt(index))->Value();
}


//
//	get a value (between min and max) for the coordinate
//
int32
TMultiThumbSlider::ValueForPoint(BPoint point) const
{
	float position = point.x;
	float min = _MinPosition();
	float max = _MaxPosition();
	
	// check the bounds	
	if (position < min)
		position = min;
	if (position > max)
		position = max;
	
	//	offset to 0 based	
	position = position - min;
	max = max - min;
	
	//	get a value and compensate
	float value = position/max * (fMaxValue-fMinValue);
	float c = ceil(value);
	float f = floor(value);
	int32 val = (int32)((value < (f + 0.50)) ? f : c);
	val = val + (int32)fMinValue;

	return val;
}

int32
TMultiThumbSlider::ValueForPoint(BPoint point, int32) const
{
	return ValueForPoint(point);
}

BPoint
TMultiThumbSlider::PointForValue(int32 value) const
{
	// check the bounds
	if (value < fMinValue)
		value = fMinValue;
	if (value > fMaxValue)
		value = fMaxValue;
	
	// offset to actual min and max
	float min = value - fMinValue;
	float max = fMaxValue - fMinValue;
	float val = (min / max) * (_MaxPosition() - _MinPosition());
	val = ceil(val);
	
	//	get the new location
	BPoint point;
	point.x = val + _MinPosition();
	point.y = 0;
	
	return point;
}

void
TMultiThumbSlider::SetPosition(float point)
{
ASSERT_WITH_MESSAGE(0, "use SetPosition(float, int32)");
	if (point < 0)
		point = 0;
	if (point > 1.0)
		point = 1.0;
	
	int32 value = (int32)((fMaxValue - fMinValue) * point);
	
	SetValue(value);
}

void
TMultiThumbSlider::SetPosition(float point, int32 index)
{
	if (point < 0)
		point = 0;
	if (point > 1.0)
		point = 1.0;
	
	int32 value = (int32)((fMaxValue - fMinValue) * point);
	
	SetValueFor(value, index);
}

float
TMultiThumbSlider::Position() const
{
ASSERT_WITH_MESSAGE(0, "use Position(int32)");
	float value = Value();
	return value / fMaxValue;
}

float
TMultiThumbSlider::Position(int32 index) const
{
	if (index < 0 || index >= ThumbCount())
		return -1;

	float value = (float)_Value(index);
	return value / fMaxValue;
}

void
TMultiThumbSlider::SetEnabled(bool on)
{
	BControl::SetEnabled(on);
}

void 
TMultiThumbSlider::Draw(BRect updateRect)
{
	DrawSlider(updateRect);
}

void
TMultiThumbSlider::DrawSlider(BRect updateRect)
{
	if (!Bounds().IsValid())
		return;
		
	if (!fOffScreenBits || !fOffScreenBits->Lock())
		return;

	BView *osv = OffscreenView();
	BRect barFrame(BarFrame());

	osv->SetHighColor(ViewColor());
	osv->FillRect(osv->Bounds());
	
	DrawBar(osv, barFrame);
	DrawHashMarks(osv, barFrame, updateRect);
	DrawThumb(osv, barFrame, updateRect);
	
	osv->Sync();

	DrawBitmap(fOffScreenBits, BPoint(0, 0));
	fOffScreenBits->Unlock();
}

void 
TMultiThumbSlider::DrawBar(BView*, BRect)
{
	BRect barRect(BarFrame());
	rgb_color white = (IsEnabled()) ? kWhiteColor : DisabledColor(kWhiteColor);
	rgb_color ltgray = (IsEnabled()) ? kLtGrayColor : DisabledColor(kLtGrayColor);
	rgb_color medgray = (IsEnabled()) ? kMedGrayColor : DisabledColor(kMedGrayColor);
	rgb_color black = (IsEnabled()) ? kBlackColor : DisabledColor(kBlackColor);
	
	// draw the trim
	fOffScreenView->BeginLineArray(11);	

	// trim - top left, bottom left, top right, 3 corners - single pixel
	fOffScreenView->AddLine(BPoint(barRect.left, barRect.top),
		BPoint(barRect.left, barRect.top),ltgray);
	fOffScreenView->AddLine(BPoint(barRect.left, barRect.bottom),
		BPoint(barRect.left, barRect.bottom),ltgray);
	fOffScreenView->AddLine(BPoint(barRect.right, barRect.top),
		BPoint(barRect.right, barRect.top),ltgray);
		
	// gray - left edge, top edge
	fOffScreenView->AddLine(BPoint(barRect.left, barRect.top+1),
		BPoint (barRect.left, barRect.bottom-1),medgray);
	fOffScreenView->AddLine(BPoint(barRect.left+1, barRect.top),
		BPoint(barRect.right - 1, barRect.top),medgray);
		
	// black - left edge, top edge, inset 1 pixel
	fOffScreenView->AddLine(BPoint(barRect.left+1, barRect.top+2),
		BPoint (barRect.left+1, barRect.bottom-1),black);
	fOffScreenView->AddLine(BPoint(barRect.left+1, barRect.top+1),
		BPoint(barRect.right - 1, barRect.top+1),black);
		
	// white - bottom edge, right edge
	fOffScreenView->AddLine(BPoint(barRect.left+1, barRect.bottom),
		BPoint(barRect.right, barRect.bottom),white);
	fOffScreenView->AddLine(BPoint(barRect.right, barRect.top+1),
		BPoint(barRect.right, barRect.bottom-1),white);
		
	fOffScreenView->EndLineArray();
	
	// fill in the bar
	if (IsEnabled())
		fOffScreenView->SetHighColor(BarColor());
	else
		fOffScreenView->SetHighColor(DisabledColor(BarColor()));			
	
	barRect.left += 2; 	barRect.right -= 1;
	barRect.top += 2; 	barRect.bottom -= 1;
	fOffScreenView->FillRect(barRect);

	//	add the fill color, left of thumb
	rgb_color color;
	if (IsEnabled() && FillColor(&color)) {
		BRect usedRect(barRect);
		usedRect.right = _Location().x;
		usedRect.right = (usedRect.right < barRect.right) ? usedRect.right : barRect.right-1;
		fOffScreenView->SetHighColor(color);
		fOffScreenView->FillRect(usedRect);
	}
}

void
TMultiThumbSlider::DrawHashMarks(BView* , BRect , BRect )
{
}

BRect
TMultiThumbSlider::BarFrame() const
{
	BRect barRect(Bounds());
	int32 halfThumb = 0;
	
	//	offset left and right half the amount of the thumb
	halfThumb = kTriThumbWidth / 2;
	barRect.left += halfThumb;
	barRect.right -= halfThumb;
	
	return barRect;
}

void 
TMultiThumbSlider::DrawThumb(BView* osv, BRect barFrame, BRect updateRect)
{
	if (fMinValue >= fMaxValue)
		return;

	int32 count = ThumbCount();
	for (int32 i=0 ; i< count; i++) {
		TThumb *thumb = ThumbAt(i);
		if (thumb->Frame().Intersects(updateRect))
			thumb->Draw(osv, barFrame);
	}
}

BRect
TMultiThumbSlider::ThumbFrame() const
{
ASSERT_WITH_MESSAGE(0, "use pointinthumb");
	float loc = _Location().x;
	BRect frame(loc,BarFrame().top,loc,BarFrame().bottom);
	
	frame.top = frame.bottom - 3;
	frame.bottom = frame.bottom + 3;
	frame.left -= 6;
	frame.right += 6;

	return frame;
}

void
TMultiThumbSlider::GetPreferredSize( float *width, float *height)
{
	*width = 0;
	*height = 0;
	
	*height += kMinBarHeight;

	float currWidth = Bounds().Width();
	*width = (currWidth < kMinBarWidth) ? kMinBarWidth : currWidth;
}

void
TMultiThumbSlider::ResizeToPreferred()
{
	float width,height;
	
	GetPreferredSize( &width, &height);
	ResizeTo(width,height);
}

status_t
TMultiThumbSlider::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}

void
TMultiThumbSlider::SetSnoozeAmount(int32 snooze)
{
	if (snooze < kMinSnoozeAmount)
		snooze = kMinSnoozeAmount;
	if (snooze > kMaxSnoozeAmount)
		snooze = kMaxSnoozeAmount;
	
	fSnoozeAmount = snooze;
}

int32
TMultiThumbSlider::SnoozeAmount() const
{
	return fSnoozeAmount;
}

int32
TMultiThumbSlider::KeyIncrementValue() const
{
	return fKeyIncrementValue;
}
		
void
TMultiThumbSlider::SetKeyIncrementValue(int32 value)
{
	fKeyIncrementValue = value;
}

rgb_color
TMultiThumbSlider::BarColor() const
{
	return fBarColor;
}

void
TMultiThumbSlider::SetBarColor(rgb_color color)
{
	fBarColor = color;
}

bool
TMultiThumbSlider::FillColor(rgb_color* color) const
{
	if (color) {
		if (fUseFillColor)
			*color = fFillColor;
		else
			color = NULL;
	}
	return fUseFillColor;
}

void
TMultiThumbSlider::UseFillColor(bool on, const rgb_color* color)
{
	fUseFillColor = on;
	if (on && color)
		fFillColor = *color;
}

BView *
TMultiThumbSlider::OffscreenView() const
{
	return fOffScreenView;
}

BBitmap *
TMultiThumbSlider::OffscreenBitmap() const
{
	return fOffScreenBits;
}

//
//
//

// 	point withing bounds of view
BPoint
TMultiThumbSlider::_Location() const
{
	ASSERT_WITH_MESSAGE(0, "use _Location(int32)");
	return BPoint(0,0);
}

BPoint
TMultiThumbSlider::_Location(int32 index) const
{
	if (index <0 || index >= ThumbCount())
		return BPoint(0,0);
	
	TThumb* t = (TThumb*)fThumbList->ItemAt(index);
	ASSERT(t);
	return t->Location();
}

void
TMultiThumbSlider::_SetLocation(BPoint)
{
	ASSERT_WITH_MESSAGE(0, "user _SetLocation(BPoint, int32)");
}

void
TMultiThumbSlider::_SetLocation(BPoint point, int32 index)
{
	if (index <0 || index >= ThumbCount())
		return;
	
	TThumb* t = (TThumb*)fThumbList->ItemAt(index);
	ASSERT(t);
	t->SetLocation(point);
}

BPoint
TMultiThumbSlider::_InitialLocation(int32 index) const
{
	if (index <0 || index >= ThumbCount())
		return BPoint(0,0);
	
	TThumb* t = (TThumb*)fThumbList->ItemAt(index);
	ASSERT(t);
	return t->InitialLocation();
}

void
TMultiThumbSlider::_SetInitialLocation(BPoint point, int32 index)
{
	if (index <0 || index >= ThumbCount())
		return;
	
	TThumb* t = (TThumb*)fThumbList->ItemAt(index);
	ASSERT(t);
	t->SetInitialLocation(point);
		
}

// point within bar frame
float
TMultiThumbSlider::_MinPosition() const
{
	return BarFrame().left + 1;
}

float
TMultiThumbSlider::_MaxPosition() const
{
	return BarFrame().right - 1;
}

//

status_t
TMultiThumbSlider::AddThumb(int32 value, BMessage* mdMsg,
	BMessage* stdMsg, BMessage* modMsg)
{
	if (!fThumbList)
		return B_ERROR;
		
	return AddThumb(new TThumb(this, mdMsg, stdMsg, modMsg), value);
}

status_t
TMultiThumbSlider::AddThumb(TThumb* t, int32 value)
{
	if (!t)
		return B_ERROR;
		
	if (!fThumbList)
		return B_ERROR;
		
	fThumbList->AddItem(t);
	SetValueFor(value, ThumbCount() - 1, false, false);
	
	return B_OK;
}

//	caller needs to handle tossing of thumb* memory
//		and msg memory
TThumb*
TMultiThumbSlider::RemoveThumb(int32 index)
{
	if (index <0 || index >= ThumbCount())
		return NULL;
		
	return (TThumb*)fThumbList->RemoveItem(index);		
}

int32
TMultiThumbSlider::PointInThumb(BPoint point) const
{
	int32 count = ThumbCount();
	for (int32 i=0 ; i<count ; i++) {
		TThumb *t = (TThumb *)fThumbList->ItemAt(i);
		ASSERT(t);
		if (t->HitTest(point))
			return i;
	}
	
	return -1;
}

int32
TMultiThumbSlider::ThumbCount() const
{
	if (!fThumbList)
		return 0;
	else	
		return fThumbList->CountItems();
}

TThumb*
TMultiThumbSlider::ThumbAt(int32 index) const
{
	if (index <0 || index >= ThumbCount())
		return NULL;
	
	TThumb* t = (TThumb*)fThumbList->ItemAt(index);
	
	return t;
}

BMessage *
TMultiThumbSlider::MouseDownMessageFor(int32 index)
{
	TThumb* t = ThumbAt(index);
	if (t)
		return t->MouseDownMessage();
	else
		return NULL;
}

void
TMultiThumbSlider::SetMouseDownMessageFor(BMessage *message, int32 i)
{
	if (i <0 || i >= ThumbCount())
		return;
		
	TThumb* t = (TThumb*)fThumbList->ItemAt(i);
	if (t) {
		t->SetMouseDownMessage(message);
	}		
}

BMessage *
TMultiThumbSlider::MessageFor(int32 index)
{
	TThumb* t = ThumbAt(index);
	if (t)
		return t->Message();
	else
		return NULL;
}

void
TMultiThumbSlider::SetMessageFor(BMessage *message, int32 i)
{
	if (i <0 || i >= ThumbCount())
		return;
		
	TThumb* t = (TThumb*)fThumbList->ItemAt(i);
	if (t) {
		t->SetMessage(message);
	}		
}

BMessage *
TMultiThumbSlider::ModificationMessageFor(int32 index)
{
	TThumb* t = ThumbAt(index);
	if (t)
		return t->ModificationMessage();
	else
		return NULL;
}

void
TMultiThumbSlider::SetModificationMessageFor(BMessage *message, int32 i)
{
	if (i <0 || i >= ThumbCount())
		return;
		
	TThumb* t = (TThumb*)fThumbList->ItemAt(i);
	if (t) {
		t->SetModificationMessage(message);
	}
}

bool
TMultiThumbSlider::TrackingThumb(int32 which)
{
	return fCurrentThumb == which;
}
