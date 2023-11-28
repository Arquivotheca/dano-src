
#include "BarberPoleView.h"
#include "Bitmaps.h"

BarberPoleView::BarberPoleView(BRect frame, const char *name,
	const unsigned char *const *barberPoleArray,
	int32 barberPoleCount,
	const unsigned char *dropHereBits)
	:	BView(frame, name, B_FOLLOW_LEFT + B_FOLLOW_TOP,
			B_PULSE_NEEDED | B_WILL_DRAW),
		bitmap((frame.InsetBySelf(kBevel, kBevel)).OffsetToSelf(0, 0), B_COLOR_8_BIT),
		bits(barberPoleArray),
		dropHereBits(dropHereBits),
		count(barberPoleCount), 
		indx(0),
		progress(true),
		paused(false)
{
}

const rgb_color kLightGray = {150, 150, 150, 255};
const rgb_color kGray = {100, 100, 100, 255};
const rgb_color kWhite = {255, 255, 255, 255};

void 
BarberPoleView::Draw(BRect)
{
	BRect bounds(Bounds());

	BeginLineArray(8);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kLightGray);
	AddLine(bounds.LeftTop(), bounds.LeftBottom(), kLightGray);
	AddLine(bounds.LeftBottom(), bounds.RightBottom(), kWhite);
	AddLine(bounds.RightBottom(), bounds.RightTop(), kWhite);
	bounds.InsetBy(1, 1);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kLightGray);
	AddLine(bounds.LeftTop(), bounds.LeftBottom(), kLightGray);
	AddLine(bounds.LeftBottom(), bounds.RightBottom(), kWhite);
	AddLine(bounds.RightBottom(), bounds.RightTop(), kWhite);
	EndLineArray();
	DrawCurrentBarberPole();
}

void 
BarberPoleView::DrawCurrentBarberPole()
{
	if (progress)
		bitmap.SetBits(bits[indx], kBitmapWidth * kBitmapHeight, 0, B_COLOR_8_BIT);
	else
		bitmap.SetBits(dropHereBits, kBitmapWidth * kBitmapHeight, 0, B_COLOR_8_BIT);

	DrawBitmap(&bitmap, BPoint(kBevel, kBevel));
}


void 
BarberPoleView::Pulse()
{
	if (!progress || paused)
		return;

	DrawCurrentBarberPole();
	if (++indx >= count)
		indx = 0;
}

void 
BarberPoleView::SetPaused()
{
	progress = true;
	paused = true;
	Invalidate();
}

void 
BarberPoleView::SetProgressing()
{
	progress = true;
	paused = false;
	Invalidate();
}

void 
BarberPoleView::SetWaitingForDrop()
{
	progress = false;
	paused = true;
	Invalidate();
}

