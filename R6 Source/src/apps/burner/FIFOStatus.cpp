#include "FIFOStatus.h"
#include <stdio.h>

const rgb_color kBufferLowColor		= { 240, 0, 0, 255};
const rgb_color kBufferMedColor		= { 240, 240, 0, 255};
const rgb_color kBufferHighColor	= { 0, 240, 0, 255};
const rgb_color kBorderColor		= { 0, 0, 0, 255};
const rgb_color kTextColor			= { 0, 0, 0, 255};

FIFOStatus::FIFOStatus(BRect frame, uint32 resizingMode)
	: BView(frame, "FIFOStatus", resizingMode, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	fPercentFull = 0.0f;
}


FIFOStatus::~FIFOStatus()
{
}

void FIFOStatus::SetPercentFull(float percent)
{
	// enforce range from 0.0 to 1.0
	if (percent < 0.0f) {
		percent = 0.0f;
	} else if (percent > 1.0f) {
		percent = 1.0f;
	}
	
	if (percent != fPercentFull) {
		fPercentFull = percent;
		if (Parent() != NULL) {
			Draw(Bounds());
		}
	}
}

void FIFOStatus::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	BView *parent = Parent();
	if (parent != NULL) {
		SetLowColor(parent->LowColor());	
	}
	BFont font;
	GetFont(&font);
	font.SetFace(B_REGULAR_FACE);
	font.SetSize(10);
	SetFont(&font);
	GetFontHeight(&fFontHeight);
}

void FIFOStatus::Draw(BRect update)
{
	BRect bounds(Bounds());
	BRect rect(bounds);
	rect.bottom -= (fFontHeight.descent + fFontHeight.ascent + fFontHeight.leading);
	PushState();
	SetHighColor(kBorderColor);
	StrokeRect(rect);
	rect.InsetBy(1, 1);
	
	// fill used portion of bar with the appropriate color
	BRect fillRect(rect);
	fillRect.top = fillRect.bottom - floor(fPercentFull * fillRect.Height());
	rgb_color barCol;
	if (fPercentFull > 0.4f) {
		barCol = kBufferHighColor;
	} else if (fPercentFull > 0.2f) {
		barCol = kBufferMedColor;
	} else {
		barCol = kBufferLowColor;
	}
	SetHighColor(barCol);
	FillRect(fillRect);

	// fill empty space in bar
	fillRect.bottom = fillRect.top - 1;
	fillRect.top = rect.top;
	if (fillRect.top <= fillRect.bottom) {
		FillRect(fillRect, B_SOLID_LOW);
	}
	// draw text into bar (75.2%)
	char buf[8];
	sprintf(buf, "%0.0f%%", fPercentFull * 100);
	SetDrawingMode(B_OP_OVER);
	SetHighColor(kTextColor);
	DrawString(buf, BPoint(rect.left + (rect.Width() - StringWidth(buf)) / 2,
						   rect.top + (rect.Height() - fFontHeight.ascent) / 2));
	SetDrawingMode(B_OP_COPY);

	// draw label
	rect.top = rect.bottom + 2;
	rect.bottom = bounds.bottom;
	rect.left = bounds.left;
	rect.right = bounds.right;
	FillRect(rect, B_SOLID_LOW);
	const char *label = "Buffer";
	DrawString(label, BPoint(rect.left + (rect.Width() - StringWidth(label)) / 2,
						     rect.bottom - fFontHeight.descent));
	PopState();
}


