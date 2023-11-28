//******************************************************************************
//
//	File:		Slider.cpp
//
//	Written by:	Robert Chinn
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************

#include <Debug.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Font.h>
#include <Window.h>

#include "Slider.h"

#include <AutoLock.h>
#include <archive_defs.h>
#include <interface_misc.h>

/*------------------------------------------------------------*/

namespace BPrivate
{

const int32 kMinBarThickness = 10;
const int32 kMinBarLength = 32;
const int32 kHashLength = 4;
const int32 kOldHashLength = 6; 		// used in places for backward compatibility
const int32 kOldMinBarThickness = 6;	// ditto
const int32 kTriThumbWidth = 15;
const int32 kRectThumbWidth = 11;	// 17
const int32	kGap = 4;
const int32 kThicknessUndefined = -1;

static rgb_color kWhiteColor;
static rgb_color kWhiteGrayColor;
static rgb_color kBGGrayColor;
static rgb_color kDarkGrayColor;
static rgb_color kBlackColor;
static rgb_color kUnusedColor;
static rgb_color kDefaultBarColor;
static rgb_color kBlockThumbFillColor;

// use these colors to draw shadows with B_OP_ALPHA drawing mode
const rgb_color kDarken1Color = { 0, 0, 0, 29 };
const rgb_color kDarken2Color = { 0, 0, 0, 70 };

// snooze amounts in mousedown loop
const int32 kMinSnoozeAmount = 5000;
const int32 kMaxSnoozeAmount = 1000000;
const int32 kDefaultSnoozeAmount = 20000;

static float
RoundFloat(const float val) {
	const float c = (float)ceil(val);
	const float f = (float)floor(val);
	return (val < (f + 0.50)) ? f : c;
}

} using namespace BPrivate;


/*------------------------------------------------------------*/

BSlider::BSlider(BRect frame, const char *name, const char *label, BMessage *message,
	int32 minValue, int32 maxValue, thumb_style thumbType,
	uint32 resizingMode, uint32 flags)
	: BControl(frame, name, label, message, resizingMode, flags | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	_InitColors();

	fOrientation = B_HORIZONTAL;
	fBarThickness = kThicknessUndefined;
	fModificationMessage = NULL;
	fSnoozeAmount = kDefaultSnoozeAmount;
	
	fMinLimitStr = NULL;
	fMaxLimitStr = NULL;

	fMinValue = minValue;
	fMaxValue = maxValue;
	SetValue(0);
	fKeyIncrementValue = 1;
			
	fHashMarkCount = 0;
	fHashMarks = B_HASH_MARKS_NONE;
	
	fStyle = thumbType;
		
//	if (Style() == B_BLOCK_THUMB)
//		SetBarColor(kDarkGrayColor);
//	else
//		SetBarColor(kUnusedColor);
	SetBarColor(kDefaultBarColor);
	
	UseFillColor(false);

	_InitObject();
}

BSlider::BSlider(BRect frame, const char *name, const char *label, BMessage *message,
	int32 minValue, int32 maxValue, orientation posture, thumb_style thumbType,
	uint32 resizingMode, uint32 flags)
	: BControl(frame, name, label, message, resizingMode, flags | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	_InitColors();

	fOrientation = posture;
	fBarThickness = kMinBarThickness;
	fModificationMessage = NULL;
	fSnoozeAmount = kDefaultSnoozeAmount;
	
	fMinLimitStr = NULL;
	fMaxLimitStr = NULL;

	fMinValue = minValue;
	fMaxValue = maxValue;
	SetValue(0);
	fKeyIncrementValue = 1;
			
	fHashMarkCount = 0;
	fHashMarks = B_HASH_MARKS_NONE;
	
	fStyle = thumbType;
		
//	if (Style() == B_BLOCK_THUMB)
//		SetBarColor(kDarkGrayColor);
//	else
//		SetBarColor(kUnusedColor);
	SetBarColor(kDefaultBarColor);

	UseFillColor(false);

	_InitObject();
}


BSlider::~BSlider()
{
	delete fModificationMessage;	
	free(fMinLimitStr); 	// allocated by strdup
	free(fMaxLimitStr); 	// allocated by strdup	
}

BSlider::BSlider(BMessage *data)
	: BControl(data)
{
	_InitColors();

	fModificationMessage = NULL;
	BMessage *message = NULL;
	if (data->HasData(S_MOD_MESSAGE, B_RAW_TYPE)) {
		long		length;
		const void	*ptr;
		message = new BMessage();
		if (message) {
			data->FindData(S_MOD_MESSAGE, B_RAW_TYPE, &ptr, &length);
			message->Unflatten((const char *) ptr);
		}
	}
	SetModificationMessage(message);

	if ( data->FindInt32(S_DELAY, (int32*)&fSnoozeAmount) != B_OK)
		SetSnoozeAmount(kDefaultSnoozeAmount);

	int32 color;
	if ( data->FindInt32(S_FILL_COLOR, 1, (int32*)&color) != B_OK)
		UseFillColor(false);
	else {
		rgb_color c = _long_to_color_(color);
		UseFillColor(true,(rgb_color*)&c);
	}

	orientation msgOrientation;
	if ( data->FindInt32(S_ORIENTATION, (int32 *)&msgOrientation) != B_OK)
		fOrientation = B_HORIZONTAL;
	else
		fOrientation = msgOrientation;
	
	fMinLimitStr = NULL;
	fMaxLimitStr = NULL;

	const char *s1=NULL, *s2=NULL;
	data->FindString(S_MINLABEL,&s1);
	data->FindString(S_MAXLABEL,&s2);
	SetLimitLabels(s1,s2);
		
	if ( data->FindInt32(S_MIN,&fMinValue) != B_OK)
		fMinValue = 0;
	if ( data->FindInt32(S_MAX,&fMaxValue) != B_OK)
		fMaxValue = 100;

	if ( data->FindInt32(S_INCVALUE,&fKeyIncrementValue) != B_OK)
		fKeyIncrementValue = 1;
		
	if ( data->FindInt32(S_HASHMARKCOUNT,&fHashMarkCount) != B_OK)
		fHashMarkCount = 11;	// segments + 1
	int16 hashmarkloc;
	if ( data->FindInt16(S_HASHMARKS,&hashmarkloc) != B_OK)	
		fHashMarks = B_HASH_MARKS_NONE;
	else
		fHashMarks = (hash_mark_location)hashmarkloc;

	int16 thumbstyle;
	if ( data->FindInt16(S_THUMBSTYLE,&thumbstyle) != B_OK)
		fStyle = B_BLOCK_THUMB;
	else
		fStyle = (thumb_style)thumbstyle;
	
	if ( data->FindInt32(S_BAR_COLOR,(int32*)&color) != B_OK) {
//		if (Style() == B_BLOCK_THUMB)
//			SetBarColor(kDarkGrayColor);
//		else
//			SetBarColor(kUnusedColor);
		SetBarColor(kDefaultBarColor);
	} else
		SetBarColor(_long_to_color_(color));

	float thickness;
	if ( data->FindFloat(S_BAR_THICKNESS, &thickness) != B_OK)
		fBarThickness = kThicknessUndefined;
	else
		fBarThickness = thickness;

	SetFlags(Flags() | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);

	_InitObject();	
}

void
BSlider::_InitObject()
{
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	fLocation.x = 0; fLocation.y = 0;
	fInitialLocation = fLocation;
	fOffScreenView = this;
	SetInvalidate(true);
}

void
BSlider::_InitColors()
{
	kWhiteColor.set_to(255,255,255,255);
	kBlackColor.set_to(0,0,0,255);
	kBGGrayColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	kWhiteGrayColor = tint_color(kBGGrayColor, B_LIGHTEN_1_TINT);		// was 235,235,235
	kUnusedColor = tint_color(kBGGrayColor, B_DARKEN_1_TINT);
	kDarkGrayColor = tint_color(kBGGrayColor, B_DARKEN_4_TINT);			// It's actually not used anymore (was: 80,80,80)
	kDefaultBarColor = tint_color(kBGGrayColor, B_DARKEN_2_TINT);		// was 156,156,156
	kBlockThumbFillColor= tint_color(kBGGrayColor, B_LIGHTEN_2_TINT);	// was 245,245,245
}

BArchivable*
BSlider::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BSlider"))
		return NULL;
	return new BSlider(data);
}

status_t
BSlider::Archive(BMessage *data, bool deep) const
{
	BControl::Archive(data, deep);
	
	BMessage *m = ModificationMessage();
	if (m) {
		BMallocIO	stream;
		m->Flatten(&stream);
		data->AddData(S_MOD_MESSAGE, B_RAW_TYPE, stream.Buffer(), stream.BufferLength());
	}		
	data->AddInt32(S_DELAY, fSnoozeAmount);

	data->AddInt32(S_BAR_COLOR, _color_to_long_(fBarColor));
	if (FillColor(NULL))
		data->AddInt32(S_FILL_COLOR, _color_to_long_(fFillColor));
	
	if (fMinLimitStr != NULL)
		data->AddString(S_MINLABEL,fMinLimitStr);
	if (fMaxLimitStr != NULL)
		data->AddString(S_MAXLABEL,fMaxLimitStr);
		
	data->AddInt32(S_MIN,fMinValue);
	data->AddInt32(S_MAX,fMaxValue);

	data->AddInt32(S_INCVALUE,fKeyIncrementValue);

	data->AddInt32(S_HASHMARKCOUNT,fHashMarkCount);
	data->AddInt16(S_HASHMARKS,fHashMarks);	
	
	data->AddInt16(S_THUMBSTYLE,fStyle);	
	data->AddInt32(S_ORIENTATION, fOrientation);
	data->AddFloat(S_BAR_THICKNESS, fBarThickness);
	
	return B_OK;
}

status_t
BSlider::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}

void
BSlider::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}

void
BSlider::AttachedToWindow()
{
	ResizeToPreferred();

	SetColorsFromParent();

	// set the start position relative to the bar, not the bounds !@#$
	SetValue(Value());

	BControl::AttachedToWindow();
}

void
BSlider::AllAttached()
{
	BControl::AllAttached();
}

void
BSlider::AllDetached()
{
	BControl::AllDetached();
}

void
BSlider::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

void
BSlider::MessageReceived(BMessage *message)
{
	BControl::MessageReceived(message);
}

void
BSlider::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}

void
BSlider::FrameResized(float w, float h)
{
	BControl::FrameResized(w,h);
	if (Bounds().IsValid())
		SetValue(Value());
}

void 
BSlider::KeyDown(const char *bytes, int32 n)
{
	if (!IsEnabled() || IsHidden())
		return;
	switch (bytes[0]) {
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
			SetValue(Value() - KeyIncrementValue());
			Invoke();
			break;
		case B_UP_ARROW:
		case B_RIGHT_ARROW:
			SetValue(Value() + KeyIncrementValue());
			Invoke();
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

void
BSlider::MouseDown(BPoint thePoint)
{
	// only do something if we're enabled
	if (!IsEnabled())
		return;

	// see if the click was in a valid rect for the current settings
	if (!(ThumbFrame().Contains(thePoint)))
		if (!(BarFrame().Contains(thePoint)))
			if (!(HashMarksFrame().Contains(thePoint)))
				return;
	
	if ((Flags() & B_NAVIGABLE) != 0 && !IsFocus())
		MakeFocus(true);
	
	SetExplicitFocus();
	
	// the mouse btn is held down, track it and update
	ulong buttons;
	BPoint currPosition = fInitialLocation = _Location();
	BPoint where;
	bool changed;
	while(true) {
		GetMouse(&where, &buttons);
		if (!buttons)
			break;

		changed = false;
		if (fOrientation == B_HORIZONTAL) {		
			if (where.x != currPosition.x) {
				changed = true;
				if (where.x <= _MinPosition())
					where.x = _MinPosition();
				else if (where.x >= _MaxPosition())
					where.x = _MaxPosition();
			}
		} else { // fOrientation == B_VERTICAL
			if (where.y != currPosition.y) {
				changed = true;
				if (where.y <= _MinPosition())
					where.y = _MinPosition();
				else if (where.y >= _MaxPosition())
					where.y = _MaxPosition();
			}
		}
	
		if (changed) {	
			SetValue(ValueForPoint(where));

			//	send the mod message not the stnd invoke msg
			InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
		}

		// 	if this is in an asynch window then
		//	set up for tracking and exit this loop
		//	mouse tracking is then handled in MouseMoved
		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
			SetTracking(true);
			SetMouseEventMask(B_POINTER_EVENTS,
				B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
			break;
		}

		snooze(SnoozeAmount());
		currPosition = where;		
	}
	
	// only invoke if not asynch
	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) == 0) {
		if (fInitialLocation != _Location()) {
			Invoke();
		}
	}
}

void
BSlider::MouseUp(BPoint pt)
{
	if (IsTracking()) {
		// stnd msg is only sent on single click/release of mouse btn
		if (fInitialLocation != _Location()) {
			Invoke();
		}
			
		//	turn off tracking
		SetTracking(false);
	} else
		BControl::MouseUp(pt);
}

void
BSlider::MouseMoved(BPoint pt, uint32 code, const BMessage *message)
{
	if (IsTracking()) {

		/*
		This can replace the bounds checking that follows
		probably need an API change to allow setting of the frame or the
		offset though.
		
		Should also add a tracking variable for the mousedown position
		for the snap back feature
			
		BRect trackingRect = ThumbFrame();
		trackingRect.InsetBy(-10, -10);		
		if (!(trackingRect.Contains(pt))) {
			trackingRect = BarFrame();
			trackingRect.InsetBy(-10, -10);		
			if (!(trackingRect.Contains(pt))) {
				trackingRect = HashMarksFrame();
				trackingRect.InsetBy(-10, -10);		
				if (!(trackingRect.Contains(pt))) {
					return;
				}
			}
		}*/

		BPoint	newLoc(pt);
		BPoint currPosition(_Location());
		bool changed(false);
		
		if (fOrientation == B_HORIZONTAL) {
			if (newLoc.x != currPosition.x) {
				changed = true;
				if (newLoc.x <= _MinPosition())
					newLoc.x = _MinPosition();
				else if (newLoc.x >= _MaxPosition())
					newLoc.x = _MaxPosition();
			}
		} else { // fOrientation == B_VERTICAL
			if (newLoc.y != currPosition.y) {
				changed = true;
				if (newLoc.y <= _MinPosition())
					newLoc.y = _MinPosition();
				else if (newLoc.y >= _MaxPosition())
					newLoc.y = _MaxPosition();
			}
		}

		if (changed) {
			SetValue(ValueForPoint(newLoc));
		}
		
		if (currPosition != _Location()) {
			//	send the mod message not the stnd invoke msg					
			InvokeNotify(ModificationMessage(), B_CONTROL_MODIFIED);
		}

	} else
		BControl::MouseMoved(pt, code, message);
}

void
BSlider::Pulse()
{
	BControl::Pulse();
}

void 
BSlider::SetLabel(const char *label)
{
	BControl::SetLabel(label);
}

void 
BSlider::SetLimitLabels(const char *minLabel, const char *maxLabel)
{
	if (minLabel) {
		if (fMinLimitStr)
			free(fMinLimitStr);
			
		fMinLimitStr = strdup(minLabel);
	}
	
	if (maxLabel) {
		if (fMaxLimitStr)
			free(fMaxLimitStr);
			
		fMaxLimitStr = strdup(maxLabel);
	}
	
	ResizeToPreferred();
	if (LockLooper()) {
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

const char*
BSlider::MinLimitLabel() const
{
	return fMinLimitStr;
}

const char*
BSlider::MaxLimitLabel() const
{
	return fMaxLimitStr;
}

//
//	set the actual value (between minvalue and maxvalue)
//	

void
BSlider::SetValue(int32 v)
{
	// check the bounds
	if (v < fMinValue) v = fMinValue;
	if (v > fMaxValue) v = fMaxValue;
	
	// offset to actual min and max
	float min = v - fMinValue;
	float max = fMaxValue - fMinValue;
	float val = ((min / max) * (_MaxPosition() - _MinPosition()));

	//	get the new location
	BPoint p;
	if (fOrientation == B_HORIZONTAL) {
		p.x = (float)ceil(val + _MinPosition());
		p.y = 0;
	} else { // fOrientation == B_VERTICAL
		p.x = 0;
		p.y = (float)floor(_MaxPosition() - val);
	}

	if (fLocation != p)
		Invalidate();
	_SetLocation(p);

	BControl::SetValue(v);
}

//
//	get a value (between min and max) for the coordinate
//

int32
BSlider::ValueForPoint(BPoint p) const
{
	float position = (fOrientation == B_HORIZONTAL) ? p.x : p.y;
	float min = _MinPosition();
	float max = _MaxPosition();
	
	// check the bounds
	if (position < min) position = min;
	if (position > max) position = max;
	
	//	offset to 0 based
	if (fOrientation == B_HORIZONTAL) {
		position = position - min;
	} else {
		position = max - position;
	}
	max = max - min;
	
	//	get a value and compensate
	float v = RoundFloat((position / max) * (fMaxValue - fMinValue));
	int32 val = (int32)v + fMinValue;

	return val;
}

void
BSlider::SetPosition(float p)
{
	if (p < 0) p = 0;
	if (p > 1.0) p = 1.0;
	
	int32 v = (int32)((fMaxValue - fMinValue) * p + .5);
	
	SetValue(v);
}

float
BSlider::Position() const
{
	float v = Value();
	return v / ( fMaxValue - fMinValue );
}

void
BSlider::SetEnabled(bool on)
{
	BControl::SetEnabled(on);
}

void
BSlider::GetLimits(int32 * minimum, int32 * maximum)
{
	*minimum = fMinValue;
	*maximum = fMaxValue;
}

void 
BSlider::Draw(BRect)
{
	DrawSlider();
	if (IsFocus() && Window()->IsActive() && !IsInvalidatePending())
		InvalidateAtTime(NextFocusTime());
}

void
BSlider::DrawSlider()
{
	if (!Bounds().IsValid())
		return;

	if (LockLooper())
	{
		fFocusColor = NextFocusColor();
		DrawBar();
		DrawHashMarks();
		DrawText();
		DrawThumb();
		DrawFocusMark();
		Sync();
		UnlockLooper();
	}
}

void 
BSlider::DrawBar()
{
	BRect barRect(BarFrame());
	rgb_color lineCol = kBlackColor;
	rgb_color backCol = BarColor();
	rgb_color fillCol = backCol;
	rgb_color tempCol;
	bool filled = FillColor(&fillCol);
	bool focused = IsFocus() && Window()->IsActive();
	bool enabled = IsEnabled();
	
	if (focused) lineCol = fFocusColor;

	if (!enabled) {
		backCol = ViewColor();
		lineCol.disable(backCol);
		fillCol = lineCol;
	}
	// fill entire barRect with bar color
	fOffScreenView->SetHighColor(backCol);
	fOffScreenView->FillRect(barRect);
	
	fOffScreenView->BeginLineArray(38);
	if (fOrientation == B_HORIZONTAL) {
		// draw top & bottom outline of bar, possibly with shadow
		fOffScreenView->AddLine(barRect.LeftTop(), barRect.RightTop(), lineCol);
		fOffScreenView->AddLine(barRect.LeftBottom(), barRect.RightBottom(), lineCol);
		if (enabled) {
			tempCol = tint_color(backCol, B_DARKEN_1_TINT);
			fOffScreenView->AddLine(BPoint(barRect.left, barRect.top + 2),
									BPoint(barRect.right, barRect.top + 2), tempCol);
			tempCol = tint_color(tempCol, B_DARKEN_1_TINT);
			fOffScreenView->AddLine(BPoint(barRect.left, barRect.top + 1),
									BPoint(barRect.right, barRect.top + 1), tempCol);
			if (focused) {
				fOffScreenView->AddLine(BPoint(barRect.left, barRect.top + 1),
										BPoint(barRect.right, barRect.top + 1), lineCol);	
				fOffScreenView->AddLine(BPoint(barRect.left, barRect.bottom - 1),
										BPoint(barRect.right, barRect.bottom - 1), lineCol);	
			}
		}
	} else { // fOrientation == B_VERTICAL
		// draw left & right outline of bar
		fOffScreenView->AddLine(barRect.LeftTop(), barRect.LeftBottom(), lineCol);
		fOffScreenView->AddLine(barRect.RightTop(), barRect.RightBottom(), lineCol);
		if (enabled) {
			tempCol = tint_color(backCol, B_DARKEN_1_TINT);
			fOffScreenView->AddLine(BPoint(barRect.left + 2, barRect.top),
									BPoint(barRect.left + 2, barRect.bottom), tempCol);	
			tempCol = tint_color(tempCol, B_DARKEN_1_TINT);
			fOffScreenView->AddLine(BPoint(barRect.left + 1, barRect.top),
									BPoint(barRect.left + 1, barRect.bottom), tempCol);	
			if (focused) {
				fOffScreenView->AddLine(BPoint(barRect.left + 1, barRect.top),
										BPoint(barRect.left + 1, barRect.bottom), lineCol);	
				fOffScreenView->AddLine(BPoint(barRect.right - 1, barRect.top),
										BPoint(barRect.right - 1, barRect.bottom), lineCol);	
			}
		}
	}

	// fill barRect to the left (or bottom) of the thumb with fill color
	if (filled) {
		BRect fillRect(barRect);
		if (fOrientation == B_HORIZONTAL) {
			fillRect.top += (enabled) ? 3 : 1;
			fillRect.bottom -= (focused && enabled) ? 2 : 1;
			fillRect.right = _Location().x;
		} else {	// fOrientation == B_VERTICAL
			fillRect.left += (enabled) ? 3 : 1;
			fillRect.right -= (focused && enabled) ? 2 : 1;
			fillRect.top = _Location().y;
		}
		fOffScreenView->SetHighColor(fillCol);
		fOffScreenView->FillRect(fillRect);

		if (enabled) {
			tempCol = tint_color(fillCol, B_DARKEN_1_TINT);
			// draw shadows on the fill color
			if (fOrientation == B_HORIZONTAL) {
				fOffScreenView->AddLine(BPoint(fillRect.left, fillRect.top - 1),
										BPoint(fillRect.right, fillRect.top - 1), tempCol);
				if (!focused) {
					tempCol = tint_color(tempCol, B_DARKEN_1_TINT);
					fOffScreenView->AddLine(BPoint(fillRect.left, fillRect.top - 2),
											BPoint(fillRect.right, fillRect.top - 2), tempCol);
				}
			} else {	// fOrientation == B_VERTICAL
				fOffScreenView->AddLine(BPoint(fillRect.left - 1, fillRect.top),
										BPoint(fillRect.left - 1, fillRect.bottom), tempCol);
				if (!focused) {
					tempCol = tint_color(tempCol, B_DARKEN_1_TINT);
					fOffScreenView->AddLine(BPoint(fillRect.left - 2, fillRect.top),
											BPoint(fillRect.left - 2, fillRect.bottom), tempCol);
				}
			}
		}
	}
	
	rgb_color lightShadowCol = tint_color((filled) ? fillCol : backCol, B_DARKEN_1_TINT);
	rgb_color darkShadowCol = (focused) ? lineCol : tint_color(lightShadowCol, B_DARKEN_1_TINT);
	tempCol = tint_color(ViewColor(), B_DARKEN_1_TINT);

	// draw curved bar ends on either side of the bar rect
	BRect endRect1(barRect), endRect2(barRect);
	// bar rect is shortened on each end by half the amount of the thumb,
	// so we need to calculate that in order to figure out where to draw
	int32 halfThumb = 0;
	if (Style() == B_BLOCK_THUMB) {
		halfThumb = kRectThumbWidth / 2;
	} else if (Style() == B_TRIANGLE_THUMB) {
		halfThumb = kTriThumbWidth / 2;
	}
	
	if (fOrientation == B_HORIZONTAL) {
		endRect1.right = endRect1.left - 1;
		endRect1.left -= halfThumb - 1;
		endRect2.left = endRect2.right + 1;
		endRect2.right += halfThumb;
		// draw rounded outer left edge
		fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top),
								BPoint(endRect1.right, endRect1.top), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.bottom),
								BPoint(endRect1.right, endRect1.bottom), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top),
								BPoint(endRect1.left, endRect1.top + 2), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.left, endRect1.top + 2),
								BPoint(endRect1.left, endRect1.bottom - 2), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.left, endRect1.bottom - 2),
								BPoint(endRect1.left + 2, endRect1.bottom), lineCol);

		
		// draw rounded outer right edge
		fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.top),
								BPoint(endRect2.left, endRect2.top), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.bottom),
								BPoint(endRect2.left, endRect2.bottom), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.top),
								BPoint(endRect2.right, endRect2.top + 2), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.right, endRect2.top + 2),
								BPoint(endRect2.right, endRect2.bottom - 2), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.right, endRect2.bottom - 2),
								BPoint(endRect2.right - 2, endRect2.bottom), lineCol);

		// fill leftover bar space with background or fill color
		fOffScreenView->SetHighColor((filled) ? fillCol : backCol);
		fOffScreenView->FillRect(BRect(endRect1.left + 3, endRect1.top + 3,
									   endRect1.right, endRect1.bottom - 1));
		fOffScreenView->SetHighColor(backCol);
		fOffScreenView->FillRect(BRect(endRect2.left, endRect2.top + 3,
									   endRect2.right - 2, endRect2.bottom - 1));
		if (enabled) {
			// draw left inner shadows
			fOffScreenView->AddLine(BPoint(endRect1.left + 3, endRect1.top + 2),
									BPoint(endRect1.right, endRect1.top + 2), lightShadowCol);
			fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top + 3),
									BPoint(endRect1.left + 2, endRect1.bottom - 1), lightShadowCol);

			fOffScreenView->AddLine(BPoint(endRect1.left + 1, endRect1.top + 2),
									BPoint(endRect1.left + 1, endRect1.bottom - 2), darkShadowCol);
			fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top + 1),
									BPoint(endRect1.right, endRect1.top + 1), darkShadowCol);
			fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top + 2),
									BPoint(endRect1.left + 2, endRect1.top + 2), darkShadowCol);		

			// draw right inner shadows
			lightShadowCol = tint_color(backCol, B_DARKEN_1_TINT);
			darkShadowCol = (focused) ? lineCol : tint_color(lightShadowCol, B_DARKEN_1_TINT);
			fOffScreenView->AddLine(BPoint(endRect2.right - 1, endRect2.top + 2),
									BPoint(endRect2.left, endRect2.top + 2), lightShadowCol);
			fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.top + 1),
									BPoint(endRect2.left, endRect2.top + 1), darkShadowCol);

			if (focused) {
				fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.bottom - 1),
										BPoint(endRect1.right, endRect1.bottom - 1), lineCol);	
				fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.bottom - 2),
										BPoint(endRect1.left + 2, endRect1.bottom - 2), lineCol);	

				fOffScreenView->AddLine(BPoint(endRect2.right - 1, endRect2.top + 2),
										BPoint(endRect2.right - 1, endRect2.bottom - 2), lineCol);
				fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.bottom - 1),
										BPoint(endRect2.left, endRect2.bottom - 1), lineCol);	
				fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.bottom - 2),
										BPoint(endRect2.right - 2, endRect2.bottom - 2), lineCol);	
				fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.top + 2),
										BPoint(endRect2.right - 2, endRect2.top + 2), lineCol);		
			} else {
				fOffScreenView->AddLine(BPoint(endRect2.right - 1, endRect2.top + 3),
										BPoint(endRect2.right - 1, endRect2.bottom - 2), backCol);			
			}
		} else if (filled) { // disabled && filled
			fOffScreenView->AddLine(BPoint(endRect1.left + 1, endRect1.top + 2),
									BPoint(endRect1.left + 1, endRect1.bottom - 2), fillCol);			
			fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top + 1),
									BPoint(endRect1.left + 2, endRect1.bottom - 1), fillCol);
			fOffScreenView->AddLine(BPoint(endRect1.left + 3, endRect1.top + 1),
									BPoint(endRect1.right, endRect1.top + 1), fillCol);
			fOffScreenView->AddLine(BPoint(endRect1.left + 3, endRect1.top + 2),
									BPoint(endRect1.right, endRect1.top + 2), fillCol);
			
		}
	} else { // fOrientation == B_VERTICAL
		endRect1.top = endRect1.bottom + 1; // endRect1 is on bottom
		endRect1.bottom += halfThumb - 1;
		endRect2.bottom = endRect2.top - 1; // endRect2 is on top
		endRect2.top -= halfThumb - 1;
		// draw rounded outer bottom edge
		fOffScreenView->AddLine(BPoint(endRect1.left, endRect1.bottom - 2),
								BPoint(endRect1.left, endRect1.top), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.right, endRect1.bottom - 2),
								BPoint(endRect1.right, endRect1.top), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.left, endRect1.bottom - 2),
								BPoint(endRect1.left + 2, endRect1.bottom), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.right, endRect1.bottom - 2),
								BPoint(endRect1.right - 2, endRect1.bottom), lineCol);
		fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.bottom),
								BPoint(endRect1.right - 2, endRect1.bottom), lineCol);
		
		// draw rounded outer top edge
		fOffScreenView->AddLine(BPoint(endRect2.left, endRect2.top + 2),
								BPoint(endRect2.left, endRect2.bottom), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.right, endRect2.top + 2),
								BPoint(endRect2.right, endRect2.bottom), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.left, endRect2.top + 2),
								BPoint(endRect2.left + 2, endRect2.top), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.right, endRect2.top + 2),
								BPoint(endRect2.right - 2, endRect2.top), lineCol);
		fOffScreenView->AddLine(BPoint(endRect2.left + 2, endRect2.top),
								BPoint(endRect2.right - 2, endRect2.top), lineCol);

		// fill leftover bar space with background or fill color
		fOffScreenView->SetHighColor((filled) ? fillCol : backCol);
		fOffScreenView->FillRect(BRect(endRect1.left + 3, endRect1.top,
									   endRect1.right - 1, endRect1.bottom - 2));
		fOffScreenView->SetHighColor(backCol);
		fOffScreenView->FillRect(BRect(endRect2.left + 3, endRect2.top + 3,
									   endRect2.right - 1, endRect2.bottom));
		if (enabled) {
			// draw bottom inner shadows
			fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top),
									BPoint(endRect1.left + 2, endRect1.bottom - 1), lightShadowCol);
			fOffScreenView->AddLine(BPoint(endRect1.left + 1, endRect1.top),
									BPoint(endRect1.left + 1, endRect1.bottom - 2), darkShadowCol);

			// draw top inner shadows
			lightShadowCol = tint_color(backCol, B_DARKEN_1_TINT);
			darkShadowCol = (focused) ? lineCol : tint_color(lightShadowCol, B_DARKEN_1_TINT);
			fOffScreenView->AddLine(BPoint(endRect2.left + 3, endRect2.top + 2),
									BPoint(endRect2.right - 1, endRect2.top + 2), lightShadowCol);
			fOffScreenView->AddLine(BPoint(endRect2.left + 2, endRect2.top + 3),
									BPoint(endRect2.left + 2, endRect2.bottom), lightShadowCol);

			fOffScreenView->AddLine(BPoint(endRect2.left + 1, endRect2.top + 2),
									BPoint(endRect2.left + 1, endRect2.bottom), darkShadowCol);
			fOffScreenView->AddLine(BPoint(endRect2.left + 2, endRect2.top + 1),
									BPoint(endRect2.right - 2, endRect2.top + 1), darkShadowCol);
			fOffScreenView->AddLine(BPoint(endRect2.left + 2, endRect2.top + 2),
									BPoint(endRect2.left + 2, endRect2.top + 2), darkShadowCol);		

			if (focused) {
				fOffScreenView->AddLine(BPoint(endRect2.right - 1, endRect2.top + 2),
										BPoint(endRect2.right - 1, endRect2.bottom), lineCol);	
				fOffScreenView->AddLine(BPoint(endRect2.right - 2, endRect2.top + 2),
										BPoint(endRect2.right - 2, endRect2.top + 2), lineCol);	

				fOffScreenView->AddLine(BPoint(endRect1.right - 1, endRect1.top),
										BPoint(endRect1.right - 1, endRect1.bottom - 2), lineCol);
				fOffScreenView->AddLine(BPoint(endRect1.right - 2, endRect1.bottom - 1),
										BPoint(endRect1.left + 2, endRect1.bottom - 1), lineCol);
										
				fOffScreenView->AddLine(BPoint(endRect1.right - 2, endRect1.bottom - 2),
										BPoint(endRect1.right - 2, endRect1.bottom - 2), lineCol);	
				fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.bottom - 2),
										BPoint(endRect1.left + 2, endRect1.bottom - 2), lineCol);		
			} else {
				fOffScreenView->AddLine(BPoint(endRect1.left + 3, endRect1.bottom - 1),
										BPoint(endRect1.right - 2, endRect1.bottom - 1), fillCol);			
			}
		} else if (filled) { // disabled && filled
			fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.bottom - 1),
									BPoint(endRect1.right - 2, endRect1.bottom - 1), fillCol);			
			fOffScreenView->AddLine(BPoint(endRect1.left + 1, endRect1.top),
									BPoint(endRect1.left + 1, endRect1.bottom - 2), fillCol);
			fOffScreenView->AddLine(BPoint(endRect1.left + 2, endRect1.top),
									BPoint(endRect1.left + 2, endRect1.bottom - 2), fillCol);
		}
	}
	fOffScreenView->EndLineArray();

	// draw antialiased corners to help blend into the background
	fOffScreenView->SetDrawingMode(B_OP_ALPHA);
	fOffScreenView->BeginLineArray(4);
	BRect wholeRect(endRect1 | endRect2);
	fOffScreenView->AddLine(BPoint(wholeRect.left, wholeRect.top + 1),
							BPoint(wholeRect.left + 1, wholeRect.top), kDarken1Color);
	fOffScreenView->AddLine(BPoint(wholeRect.left, wholeRect.bottom - 1),
							BPoint(wholeRect.left + 1, wholeRect.bottom), kDarken1Color);
	fOffScreenView->AddLine(BPoint(wholeRect.right, wholeRect.top + 1),
							BPoint(wholeRect.right - 1, wholeRect.top), kDarken1Color);
	fOffScreenView->AddLine(BPoint(wholeRect.right, wholeRect.bottom - 1),
							BPoint(wholeRect.right - 1, wholeRect.bottom), kDarken1Color);
	fOffScreenView->EndLineArray();
	fOffScreenView->SetDrawingMode(B_OP_COPY);
}

BRect
BSlider::BarFrame() const
{
	BRect barRect(Bounds());
	int32 halfThumb = 0;
	
	//	offset left and right (or top and bottom) by half the amount of the thumb
	if (Style() == B_BLOCK_THUMB)
		halfThumb = kRectThumbWidth / 2;
	else if (Style() == B_TRIANGLE_THUMB)
		halfThumb = kTriThumbWidth / 2;

	if (fOrientation == B_HORIZONTAL) {
		barRect.left += halfThumb;
		barRect.right -= halfThumb;
	} else { // fOrientation == B_VERTICAL
		barRect.top += halfThumb;
		barRect.bottom -= halfThumb;
	}
	
	font_height finfo;
	GetFontHeight(&finfo);
	int32 textHeight = (int32)ceil(finfo.ascent + finfo.descent + finfo.leading);

	if (fOrientation == B_HORIZONTAL) {
		// 	offset top text from height
		if (Label() || UpdateText())
			barRect.top += (textHeight + kGap);

		// 	offset hash marks from height
		barRect.top += kHashLength;
		
		if (fBarThickness == kThicknessUndefined) {
			// bar thickness was never set; legacy behavior was to fill leftover
			// space with the slider bar
			barRect.bottom -= kHashLength + 1;
			//	offset bottom text from height
			if (MinLimitLabel() || MaxLimitLabel())
				barRect.bottom -= (textHeight + kGap);		
		} else {
			barRect.bottom = barRect.top + fBarThickness - 1;
		}
	} else { // fOrientation == B_VERTICAL
		barRect.left = (float)ceil(barRect.left + (barRect.Width() / 2.0) - (fBarThickness / 2.0));
		barRect.right = barRect.left + fBarThickness - 1;

		bool gapUsed = false;

		if (Label()) {
			barRect.top += (textHeight + kGap);
			gapUsed = true;
		}
		
		if (MaxLimitLabel()) {
			barRect.top += textHeight;
			if (!gapUsed) {
				barRect.top += kGap;
			}
		}
		
		gapUsed = false;	// reset gapUsed for the bottom

		if (UpdateText()) {
			barRect.bottom -= (textHeight + kGap);
			gapUsed = true;
		}
		
		if (MinLimitLabel()) {
			barRect.bottom -= textHeight;
			if (!gapUsed) {
				barRect.bottom -= kGap;
			}
		}
	}
	
	return barRect;
}

void 
BSlider::DrawHashMarks()
{
	short hashSetting = HashMarks();
	// the new Slider draws hash marks in the middle or nowhere at all
	if (hashSetting != B_HASH_MARKS_NONE && fHashMarkCount > 0) {
		BRect barRect(BarFrame());
		rgb_color backCol = BarColor();
		rgb_color fillCol = backCol;
		bool filled = FillColor(&fillCol);
		rgb_color hashCol = tint_color(backCol, B_DARKEN_3_TINT);
		rgb_color fillHashCol = tint_color(fillCol, B_DARKEN_3_TINT);
		bool enabled = IsEnabled();

		if (!enabled) {
			fillHashCol = ViewColor();
			hashCol.disable(fillHashCol);
			if (!filled) {
				fillHashCol = hashCol;
			}
		}

		float segsize = (_MaxPosition() - _MinPosition()) / (fHashMarkCount - 1);
		float baseLine = _MinPosition();
		float currPos;
		rgb_color col;
		BPoint loc(_Location());
		
		fOffScreenView->BeginLineArray(fHashMarkCount);

		if (fOrientation == B_HORIZONTAL) {
			float y = barRect.top + (barRect.Height() / 2) - 1;
			
			for (int32 i = 0; i < fHashMarkCount; i++) {
				currPos = (float)ceil(baseLine + (i  * segsize));
				if (currPos > _MaxPosition()) {
					currPos = _MaxPosition();
				}
				col = (currPos < loc.x) ? fillHashCol : hashCol;
				fOffScreenView->AddLine(BPoint(currPos, y), BPoint(currPos, y + 1), col);
			}			
		} else {
			float x = barRect.left + (barRect.Width() / 2) - 1;
			
			for (int32 i = 0; i < fHashMarkCount; i++) {
				currPos = (float)ceil(baseLine + (i  * segsize));
				if (currPos > _MaxPosition()) {
					currPos = _MaxPosition();
				}
				col = (currPos > loc.y) ? fillHashCol : hashCol;
				fOffScreenView->AddLine(BPoint(x, currPos), BPoint(x + 1, currPos), col);
			}
		}
		fOffScreenView->EndLineArray();
	}
}

BRect
BSlider::HashMarksFrame() const
{
	return BarFrame();
}

void
BSlider::DrawText()
{
	float xoffset(0.0f), yoffset(0.0f);
	font_height finfo;
	
	rgb_color textcolor = kBlackColor;
	if( !IsEnabled() ) textcolor.disable(ViewColor());
	
	fOffScreenView->SetHighColor(textcolor);
	fOffScreenView->SetLowColor(ViewColor());
	fOffScreenView->GetFontHeight(&finfo);
	BRect bounds(fOffScreenView->Bounds());
	BRect barRect(BarFrame());
	
	const char *str;
	if (fOrientation == B_HORIZONTAL) {
		if ((str = Label()) != NULL) {
			yoffset = finfo.ascent + finfo.leading;
			fOffScreenView->MovePenTo(2, yoffset);
			fOffScreenView->DrawString(str);
			yoffset += finfo.descent + kGap;
		}

		if ((str = UpdateText()) != NULL) {
			yoffset = finfo.ascent + finfo.leading;
			xoffset = fOffScreenView->StringWidth(str);
			fOffScreenView->MovePenTo(bounds.Width()-xoffset-2,yoffset);
			fOffScreenView->DrawString(str);
			yoffset += finfo.descent + kGap;
		}

		yoffset += kHashLength;
		yoffset += barRect.Height();
		yoffset += kHashLength;
		
		if (MinLimitLabel() && MaxLimitLabel()) {
			yoffset += ceil(finfo.ascent + finfo.descent);
			fOffScreenView->MovePenTo(2,yoffset);
			fOffScreenView->DrawString(MinLimitLabel());

			xoffset = fOffScreenView->StringWidth(MaxLimitLabel());
			fOffScreenView->MovePenTo(bounds.Width()-xoffset-2,yoffset);
			fOffScreenView->DrawString(MaxLimitLabel());
		}
	} else { // fOrientation == B_VERTICAL
		if ((str = Label()) != NULL) {
			yoffset = finfo.ascent + finfo.leading;
			fOffScreenView->MovePenTo(bounds.left +
				floor((bounds.Width() - fOffScreenView->StringWidth(str)) / 2), yoffset);
			fOffScreenView->DrawString(str);
			yoffset += finfo.descent;
		}
			
		if ((str = MaxLimitLabel()) != NULL) {
			yoffset += finfo.ascent + finfo.leading;
			fOffScreenView->MovePenTo(bounds.left +
				floor((bounds.Width() - fOffScreenView->StringWidth(str)) / 2), yoffset);
			fOffScreenView->DrawString(str);
			yoffset += finfo.descent;
		}
		
		int32 halfThumb = 0;
		if (Style() == B_BLOCK_THUMB) {
			halfThumb = kRectThumbWidth / 2;
		} else {
			halfThumb = kTriThumbWidth / 2;
		}
		
		yoffset = barRect.bottom + kGap + halfThumb;
		
		if ((str = MinLimitLabel()) != NULL) {
			yoffset += finfo.ascent + finfo.leading;
			fOffScreenView->MovePenTo(bounds.left +
				floor((bounds.Width() - fOffScreenView->StringWidth(str)) / 2), yoffset);
			fOffScreenView->DrawString(str);
			yoffset += finfo.descent;
		}

		if ((str = UpdateText()) != NULL) {
			yoffset = finfo.ascent + finfo.leading;
			xoffset = fOffScreenView->StringWidth(str);
			fOffScreenView->MovePenTo(bounds.left +
				floor((bounds.Width() - fOffScreenView->StringWidth(str)) / 2), yoffset);
			fOffScreenView->DrawString(str);
		}
	}
}

// override to update status text
char*
BSlider::UpdateText() const
{
	return NULL;
}

void
BSlider::_DrawBlockThumb()
{
	BRect thumbRect(ThumbFrame());
	rgb_color lineCol = kBlackColor;
	rgb_color fillCol = kBlockThumbFillColor;
	bool enabled = IsEnabled();
	bool focused = IsFocus() && Window()->IsActive();
	
	if (focused) lineCol = fFocusColor;

	if (!enabled) {
		fillCol = ViewColor();
		lineCol.disable(fillCol);
	} 
	
	fOffScreenView->SetHighColor(fillCol);
	fOffScreenView->FillRoundRect(thumbRect, 3, 3);
	fOffScreenView->SetHighColor(lineCol);
	fOffScreenView->StrokeRoundRect(thumbRect, 3, 3);
	if (focused && enabled) {
		BRect rect(thumbRect);
		rect.InsetBy(1, 1);
		fOffScreenView->StrokeRoundRect(rect, 3, 2);		
		fOffScreenView->StrokeRoundRect(rect, 4, 2);		
	}
	
	float x, y;
	if (fOrientation == B_HORIZONTAL) {
		x = _Location().x;
		y = thumbRect.top + (thumbRect.Height() / 2) - 1;
		fOffScreenView->StrokeLine(BPoint(x, y), BPoint(x, y + 1)); 
	} else {
		x = thumbRect.left + (thumbRect.Width() / 2) - 1;
		y = _Location().y;
		fOffScreenView->StrokeLine(BPoint(x, y), BPoint(x + 1, y)); 
	}
	
	if (enabled) {
		// draw shadows && antialiasing around thumb
		fOffScreenView->SetDrawingMode(B_OP_ALPHA);
		fOffScreenView->BeginLineArray(10);
		fOffScreenView->AddLine(BPoint(thumbRect.left, thumbRect.top + 1),
								BPoint(thumbRect.left + 1, thumbRect.top), kDarken1Color);
		fOffScreenView->AddLine(BPoint(thumbRect.right, thumbRect.top + 1),
								BPoint(thumbRect.right - 1, thumbRect.top), kDarken1Color);
		fOffScreenView->AddLine(BPoint(thumbRect.left, thumbRect.bottom - 1),
								BPoint(thumbRect.left + 2, thumbRect.bottom + 1), kDarken1Color);
		fOffScreenView->AddLine(BPoint(thumbRect.left + 3, thumbRect.bottom + 2),
								BPoint(thumbRect.right - 1, thumbRect.bottom + 2), kDarken1Color);
		fOffScreenView->AddLine(BPoint(thumbRect.right + 1, thumbRect.bottom),
								BPoint(thumbRect.right, thumbRect.bottom + 1), kDarken1Color);
		fOffScreenView->AddLine(BPoint(thumbRect.right + 2, thumbRect.top + 3),
								BPoint(thumbRect.right + 2, thumbRect.bottom - 1), kDarken1Color);

		fOffScreenView->AddLine(BPoint(thumbRect.right + 1, thumbRect.top + 2),
								BPoint(thumbRect.right + 1, thumbRect.bottom - 1), kDarken2Color);
		fOffScreenView->AddLine(BPoint(thumbRect.left + 3, thumbRect.bottom + 1),
								BPoint(thumbRect.right - 3, thumbRect.bottom + 1), kDarken2Color);
		fOffScreenView->AddLine(BPoint(thumbRect.right - 2, thumbRect.bottom + 1),
								BPoint(thumbRect.right, thumbRect.bottom - 1), kDarken2Color);
		fOffScreenView->AddLine(BPoint(thumbRect.right - 1, thumbRect.bottom + 1),
								BPoint(thumbRect.right, thumbRect.bottom), kDarken2Color);
		fOffScreenView->EndLineArray();
		fOffScreenView->SetDrawingMode(B_OP_COPY);
	}
}

// NOTE: This does not work properly for vertical sliders.  The shading on the slider
// is incorrect because the vertical thumb draws the same as the horizontal thumb, only
// rotated 90 degrees.  I didn't fix it when I updated the class to support vertical
// sliders because Tim Martin said he was going to change how it looked before the next
// release.

void
BSlider::_DrawTriangleThumb()
{
	BRect barRect(BarFrame());
	rgb_color lineCol = kBlackColor;
	bool enabled = IsEnabled();
	bool focused = IsFocus() && Window()->IsActive();
	
	if (focused) lineCol = fFocusColor;
	
	rgb_color color;
	float loc;
	
	const rgb_color bg = ViewColor();
	
	if (fOrientation == B_HORIZONTAL) {
		loc = _Location().x;

		// 	fill triangle , white/gray
		color = enabled ? kWhiteGrayColor : kWhiteGrayColor.disable_copy(bg);
		fOffScreenView->SetHighColor(color);
		fOffScreenView->FillTriangle(BPoint(loc,barRect.bottom-3),
			BPoint(loc-6,barRect.bottom+3),
			BPoint(loc+6,barRect.bottom+3) );
	
		fOffScreenView->BeginLineArray(6);
	
		color = lineCol;
		if (!enabled) color.disable(bg);
		// black right edge
		fOffScreenView->AddLine(BPoint(loc,barRect.bottom-3),
			BPoint(loc+6,barRect.bottom+3), color);
		// black base line
		fOffScreenView->AddLine(BPoint(loc-6,barRect.bottom+4),
			BPoint(loc+6,barRect.bottom+4), color);
		// left edge, med gray
		color.set_to(120, 120, 120);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(loc-6,barRect.bottom+3),
			BPoint(loc-1,barRect.bottom-2), color);
		// bottom med gray
		color.set_to(100, 100, 100);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(loc-6,barRect.bottom+3),
			BPoint(loc+5,barRect.bottom+3), color);
		// right edge shade, lt gray
		color.set_to(200, 200, 200);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(loc,barRect.bottom-2),
			BPoint(loc+4,barRect.bottom+2), color);
		// bottom edge, lt gray
		color.set_to(180, 180, 180);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(loc-3,barRect.bottom+2),
			BPoint(loc+4,barRect.bottom+2), color);
	
		fOffScreenView->EndLineArray();
	} else { // fOrientation == B_VERTICAL
		loc = _Location().y;

		// 	fill triangle , white/gray
		color = enabled ? kWhiteGrayColor : kWhiteGrayColor.disable_copy(bg);
		fOffScreenView->SetHighColor(color);
		fOffScreenView->FillTriangle(BPoint(barRect.left + 3, loc),
			BPoint(barRect.left - 3, loc - 6),
			BPoint(barRect.left - 3, loc + 6) );
	
		fOffScreenView->BeginLineArray(6);
	
		color = lineCol;
		if (!enabled) color.disable(bg);
		// black right edge
		fOffScreenView->AddLine(BPoint(barRect.left + 3, loc),
			BPoint(barRect.left - 3, loc + 6), color);
		// black base line
		fOffScreenView->AddLine(BPoint(barRect.left - 4, loc - 6),
			BPoint(barRect.left - 4, loc + 6), color);
		// left edge, med gray
		color.set_to(120, 120, 120);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(barRect.left - 3, loc - 6),
			BPoint(barRect.left + 2, loc - 1), color);
		// bottom med gray
		color.set_to(100, 100, 100);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(barRect.left - 3, loc - 6),
			BPoint(barRect.left - 3, loc + 5), color);
		// right edge shade, lt gray
		color.set_to(200, 200, 200);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(barRect.left + 2, loc),
			BPoint(barRect.left - 2, loc + 4), color);
		// bottom edge, lt gray
		color.set_to(180, 180, 180);
		if (!enabled) color.disable(bg);
		fOffScreenView->AddLine(BPoint(barRect.left - 2, loc - 3),
			BPoint(barRect.left - 2, loc + 4), color);
	
		fOffScreenView->EndLineArray();	
	}
}

void 
BSlider::DrawThumb()
{
	if (fMinValue >= fMaxValue)
		return;
		
	if (Style() == B_BLOCK_THUMB)
		_DrawBlockThumb();
	else if (Style() == B_TRIANGLE_THUMB)
		_DrawTriangleThumb();
}

BRect
BSlider::ThumbFrame() const
{
	BRect frame, barRect(BarFrame());
	BPoint loc(_Location());
	if (fOrientation == B_HORIZONTAL) {
		frame.Set(loc.x, barRect.top, loc.x, barRect.bottom);
	} else {
		frame.Set(barRect.left, loc.y, barRect.right, loc.y);
	}
	
	if (Style() == B_BLOCK_THUMB) {
		int32 halfThumb = 0;

		if (fOrientation == B_HORIZONTAL) {
			frame.top -= kHashLength - 1;
			frame.bottom += kHashLength - 1;
			halfThumb = kRectThumbWidth / 2;

			frame.left -= halfThumb;
			frame.right += halfThumb;
		} else { // fOrientation == B_VERTICAL
			frame.left -= kHashLength - 1;
			frame.right += kHashLength - 1;		

			halfThumb = kRectThumbWidth / 2;
			frame.top -= halfThumb;
			frame.bottom += halfThumb;
		}
		
	} else if (Style() == B_TRIANGLE_THUMB) {
		if (fOrientation == B_HORIZONTAL) {
			frame.top = frame.bottom - 3;
			frame.bottom = frame.bottom + 3;
			frame.left -= 6;
			frame.right += 6;		
		} else {  // fOrientation == B_VERTICAL
			frame.top -= 6;
			frame.bottom += 6;
			frame.left = frame.left - 3;
			frame.right = frame.left + 3;
		}
	}

	return frame;
}

void
BSlider::DrawFocusMark()
{
	if (IsFocus() && IsEnabled() && Window()->IsActive()) {
		fOffScreenView->SetHighColor(fFocusColor);

		if (Style() == B_BLOCK_THUMB) {
			// already drawn by BarFrame()
		} else if (Style() == B_TRIANGLE_THUMB){
			BRect barRect = BarFrame();
			if (fOrientation == B_HORIZONTAL) {
				fOffScreenView->StrokeLine(BPoint(_Location().x-6,barRect.bottom+6),
					BPoint(_Location().x+6,barRect.bottom+6));
			} else { // fOrientation == B_VERTICAL
				fOffScreenView->StrokeLine(BPoint(barRect.left - 6, _Location().y - 6),
					BPoint(barRect.left - 6, _Location().y + 6));			
			}
		}
	} 
}

void
BSlider::SetFlags(uint32 flags)
{
	BControl::SetFlags(flags);
}

void
BSlider::SetResizingMode(uint32 mode)
{
	BControl::SetResizingMode(mode);
}

void
BSlider::GetPreferredSize( float *width, float *height)
{
	BRect bounds(Bounds());
	*width = 0;
	*height = 0;
	
	font_height	finfo;
	GetFontHeight(&finfo);
	
	float textHeight = ceil(finfo.ascent + finfo.descent + finfo.leading);

	if (fOrientation == B_HORIZONTAL) {
		// if the label is not null or if the updatetext is not null account for them
		if (Label() || UpdateText()) {
			*height = textHeight;
			*height += kGap;
		}
	
		// 	the minimum height needs to account for hashmarks top &\| bottom
		*height += kOldHashLength;
		*height += kOldHashLength;
			
		*height += kOldMinBarThickness;
	
		// if the limit strings are not null account for them	
		if (fMinLimitStr || fMaxLimitStr) {
			*height += kGap;
			*height += textHeight;
		}
	
		float currWidth = bounds.Width();
		*width = (currWidth < kMinBarLength) ? kMinBarLength : currWidth;
	} else { // fOrientation == B_VERTICAL
		*height = kMinBarLength;
		bool gapNeeded = false;
		if (Label()) {
			*height += textHeight;
			gapNeeded = true;
		}
		if (MinLimitLabel()) {
			*height += textHeight;
			gapNeeded = true;
		}
		if (gapNeeded) {
			*height += kGap;
		}
		
		gapNeeded = false;
		
		if (MaxLimitLabel()) {
			*height += textHeight;
			gapNeeded = true;
		}
		if (UpdateText()) {
			*height += textHeight;
			gapNeeded = true;
		}
		if (gapNeeded) {
			*height += kGap;
		}			

		float curr = bounds.Height();
		if (*height < curr) {
			*height = curr;
		}
		
		*width = fBarThickness + kHashLength + kHashLength;
		curr = bounds.Width();
		if (*width < curr) {
			*width = curr;
		}
	}
}

void
BSlider::ResizeToPreferred()
{
	float width,height;
	
	GetPreferredSize( &width, &height);
	ResizeTo(width,height);
}

status_t
BSlider::Invoke(BMessage *message)
{
	return BControl::Invoke(message);
}

BHandler*
BSlider::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BControl::ResolveSpecifier(message, index, spec, form, prop);
}

status_t
BSlider::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}

void
BSlider::SetModificationMessage(BMessage *message)
{
	if (fModificationMessage)
		delete fModificationMessage;
	
	fModificationMessage = message;
}

BMessage*
BSlider::ModificationMessage() const
{
	return fModificationMessage;
}

void
BSlider::SetSnoozeAmount(int32 d)
{
	if (d < kMinSnoozeAmount) d = kMinSnoozeAmount;
	if (d > kMaxSnoozeAmount) d = kMaxSnoozeAmount;
	
	fSnoozeAmount = d;
}

int32
BSlider::SnoozeAmount() const
{
	return fSnoozeAmount;
}

int32
BSlider::KeyIncrementValue() const
{
	return fKeyIncrementValue;
}
		
void
BSlider::SetKeyIncrementValue(int32 value)
{
	fKeyIncrementValue = value;
}

int32
BSlider::HashMarkCount() const
{
	return fHashMarkCount;
}

void
BSlider::SetHashMarkCount(int32 count)
{	
	fHashMarkCount = count;
	if (LockLooper()) {
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

hash_mark_location
BSlider::HashMarks() const
{
	return fHashMarks;
}

void
BSlider::SetHashMarks(hash_mark_location where)
{
	fHashMarks = where;
	if (LockLooper()) {
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

thumb_style
BSlider::Style() const
{
	return fStyle;
}

void
BSlider::SetStyle(thumb_style s)
{
	fStyle = s;
}
			
rgb_color
BSlider::BarColor() const
{
	return fBarColor;
}

void
BSlider::SetBarColor(rgb_color c)
{
	fBarColor = c;
}

bool
BSlider::FillColor(rgb_color* c) const
{
	if (c) {
		if (fUseFillColor)
			*c = fFillColor;
		else
			c = NULL;
	}
	return fUseFillColor;
}

void
BSlider::UseFillColor(bool f, const rgb_color* c)
{
	fUseFillColor = f;
	if (f && c)
		fFillColor = *c;
}

BView*
BSlider::OffscreenView() const
{
	return fOffScreenView;
}

//
//
//

//	current coordinate within bounds of slider
BPoint
BSlider::_Location() const
{
	return fLocation;
}

// 	sets the location within the slider
void
BSlider::_SetLocation(BPoint p)
{
	fLocation = p;
}

// _MinPosition() and _MaxPosition refer to screen coordinate positions, so for
// horizontal sliders there is a positive correlation between value and position,
// but for vertical sliders there is a negative correlation.

float
BSlider::_MinPosition() const
{
	return (fOrientation == B_HORIZONTAL) ? BarFrame().left + 1 : BarFrame().top + 1;
}

float
BSlider::_MaxPosition() const
{
	return (fOrientation == B_HORIZONTAL) ? BarFrame().right - 1 : BarFrame().bottom - 1;
}

void 
BSlider::SetOrientation(orientation newOrientation)
{
	// make sure the new orientation is valid
	if (newOrientation == B_HORIZONTAL) {
		fOrientation = B_HORIZONTAL;
	} else {
		fOrientation = B_VERTICAL;
	}
}

orientation 
BSlider::Orientation() const
{
	return fOrientation;
}

float 
BSlider::BarThickness() const
{
	float r = fBarThickness;
	if (fBarThickness ==  kThicknessUndefined) {
		// XXX: this should calculate bar thickness properly when it hasn't
		//      been set, which should only happen with legacy horizontal sliders.
		//		Just returning kMinBarThickness is not correct.
		r = kMinBarThickness;
	}
	return r;
}

void 
BSlider::SetBarThickness(float thickness)
{
	if (thickness > 0) {
		fBarThickness = thickness;
	}
}

void 
BSlider::SetFont(const BFont *font, uint32 properties)
{
	BView::SetFont(font, properties);
}



//void BSlider::_ReservedSlider1() {}
//void BSlider::_ReservedSlider2() {}
//void BSlider::_ReservedSlider3() {}
void BSlider::_ReservedSlider4() {}
void BSlider::_ReservedSlider5() {}
void BSlider::_ReservedSlider6() {}
void BSlider::_ReservedSlider7() {}
void BSlider::_ReservedSlider8() {}
void BSlider::_ReservedSlider9() {}
void BSlider::_ReservedSlider10() {}
void BSlider::_ReservedSlider11() {}
void BSlider::_ReservedSlider12() {}

BSlider &BSlider::operator=(const BSlider &) {return *this;}

