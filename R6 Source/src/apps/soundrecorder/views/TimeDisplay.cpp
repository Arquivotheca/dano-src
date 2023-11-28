#include <Bitmap.h>

#include "Bitmaps.h"
#include "TimeDisplay.h"
#include "DrawingTidbits.h"

const int kTimeDigitSpacing = 5;
const int kTimeInterDigitSpacing = 2;
const int kTimeInterColonSpacing = 1;
const BPoint kInitialOffset(3, 3);

const BRect kTimeDigitRect(0, 0, 4, 8);
const BRect kTimeDisplayRect(0, 0, 6*4 + kTimeDigitSpacing - 1, 8);

TimeDisplay::TimeDisplay(BRect rect, const char *name, bool hours, bool minutes,
	bool seconds, bool frame, uint32 resizeMask)
	:	BView(rect, name, resizeMask, B_WILL_DRAW),
		doHours(hours),
		doMinutes(minutes),
		doSeconds(seconds),
		doFrame(frame)
{
	segments = new BBitmap(BRect(0, 0, 64 - 1, 9 - 1), B_COLOR_8_BIT);
	segments->SetBits(LCDMedium64x9_raw, 64*9, 0, B_COLOR_8_BIT);

	SetViewColor(0, 0, 0);
}

void 
TimeDisplay::SetHours(int32 newHours)
{
	if (!doHours || newHours == hours)
		return;
	
	bool doFirst = ((newHours / 10) != (hours / 10));
	bool doSecond = ((newHours % 10) != (hours % 10));
	hours = newHours;
	
	BRect rect;
	if (doFirst) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset);
		Invalidate(rect);
	}
	if (doSecond) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset
			+ BPoint(kTimeInterDigitSpacing + kTimeDigitRect.Width(), 0));
		Invalidate(rect);
	}
}

void 
TimeDisplay::SetMinutes(int32 newMinutes)
{
	if (!doMinutes || newMinutes == minutes)
		return;
	
	bool doFirst = ((newMinutes / 10) != (minutes / 10));
	bool doSecond = ((newMinutes % 10) != (minutes % 10));
	minutes = newMinutes;
	
	BRect rect;
	if (doFirst) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset + BPoint(
			1 * kTimeInterDigitSpacing
			+ 2 * kTimeInterColonSpacing
			+ 3 * kTimeDigitRect.Width(), 0));
		Invalidate(rect);
	}
	if (doSecond) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset + BPoint(
			2 * kTimeInterDigitSpacing
			+ 2 * kTimeInterColonSpacing
			+ 4 * kTimeDigitRect.Width(), 0));
		Invalidate(rect);
	}
}

void 
TimeDisplay::SetSeconds(int32 newSeconds)
{
	if (!doSeconds || newSeconds == seconds)
		return;
	
	bool doFirst = ((newSeconds / 10) != (seconds / 10));
	bool doSecond = ((newSeconds % 10) != (seconds % 10));
	seconds = newSeconds;
	
	BRect rect;
	if (doFirst) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset + BPoint(
			2 * kTimeInterDigitSpacing
			+ 4 * kTimeInterColonSpacing
			+ 6 * kTimeDigitRect.Width(), 0));
		Invalidate(rect);
	}
	if (doSecond) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset + BPoint(
			3 * kTimeInterDigitSpacing
			+ 4 * kTimeInterColonSpacing
			+ 7 * kTimeDigitRect.Width(), 0));
		Invalidate(rect);
	}
}

void 
TimeDisplay::SetFrame(int32 newFrame)
{
	if (!doFrame || newFrame == frame)
		return;
	
	bool doFirst = ((newFrame / 10) != (frame / 10));
	bool doSecond = ((newFrame % 10) != (frame % 10));
	frame = newFrame;
	
	BRect rect;
	if (doFirst) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset + BPoint(
			3 * kTimeInterDigitSpacing
			+ 6 * kTimeInterColonSpacing
			+ 9 * kTimeDigitRect.Width(), 0));

		Invalidate(rect);
	}
	if (doSecond) {
		rect = kTimeDigitRect;
		rect.OffsetBy(kInitialOffset + BPoint(
			4 * kTimeInterDigitSpacing
			+ 6 * kTimeInterColonSpacing
			+ 10 * kTimeDigitRect.Width(), 0));
		Invalidate(rect);
	}
}

void 
TimeDisplay::Draw(BRect)
{
	BRect bounds(Bounds());
	rgb_color outline = Parent()->ViewColor();
	outline = ShiftColor(outline, 1.2);

	BeginLineArray(4);
	AddLine(BPoint(bounds.left, bounds.top), BPoint(bounds.left, bounds.bottom), outline);
	AddLine(BPoint(bounds.left, bounds.top), BPoint(bounds.right, bounds.top), outline);
	AddLine(BPoint(bounds.left, bounds.bottom), BPoint(bounds.right, bounds.bottom), kWhite);
	AddLine(BPoint(bounds.right, bounds.bottom), BPoint(bounds.right, bounds.top), kWhite);
	EndLineArray();
	
	bounds.InsetBy(1, 1);
	FillRect(bounds);
	
	float offset = DrawDigit(0, hours >= 0 ? hours / 10 + 1 : 0,
		kTimeDigitRect, segments);
	offset = DrawDigit(offset + kTimeInterDigitSpacing, hours >= 0 ? hours % 10 + 1 : 0,
		kTimeDigitRect, segments);
		
	// draw :
	offset = DrawDigit(offset + kTimeInterColonSpacing, 11, kTimeDigitRect, segments);

	offset = DrawDigit(offset + kTimeInterColonSpacing, minutes >= 0 ? minutes / 10 + 1 : 0,
		kTimeDigitRect, segments);
	offset = DrawDigit(offset + kTimeInterDigitSpacing, minutes >= 0 ? minutes % 10 + 1 : 0,
		kTimeDigitRect, segments);
		
	// draw :
	offset = DrawDigit(offset + kTimeInterColonSpacing, 11, kTimeDigitRect, segments);
	
	offset = DrawDigit(offset + kTimeInterColonSpacing, seconds >= 0 ? seconds / 10 + 1 : 0,
		kTimeDigitRect, segments);
	offset = DrawDigit(offset + kTimeInterDigitSpacing, seconds >= 0 ? seconds % 10 + 1 : 0,
		kTimeDigitRect, segments);

	offset = DrawDigit(offset + kTimeInterColonSpacing, 11, kTimeDigitRect, segments);
	
	offset = DrawDigit(offset + kTimeInterColonSpacing, frame >= 0 ? frame / 10 + 1 : 0,
		kTimeDigitRect, segments);
	offset = DrawDigit(offset + kTimeInterDigitSpacing, frame >= 0 ? frame % 10 + 1 : 0,
		kTimeDigitRect, segments);
}

float
TimeDisplay::DrawDigit(float offset, uint32 digitIndex, BRect digitSize,
	const BBitmap *map)
{
	BRect src(digitSize);
	
		// the '-' is first in the digit array
	src.OffsetBy(digitIndex * (digitSize.Width() + 1), 0);
	BRect dst(Bounds());
	dst.OffsetTo(offset, 0);
	dst.right = dst.left + digitSize.Width();
	dst.bottom = dst.top + digitSize.Height();
	SetHighColor(ViewColor());
	dst.OffsetBy(kInitialOffset);
	FillRect(dst);

	// take advantages of parts of the digit being transparent
	SetDrawingMode(B_OP_OVER);
	SetLowColor(B_TRANSPARENT_32_BIT);
	
	DrawBitmap(map, src, dst);
	return offset + dst.Width();
}
