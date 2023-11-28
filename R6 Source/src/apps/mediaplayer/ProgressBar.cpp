#include "ProgressBar.h"

const char *kMessageString = "Buffering...";

const rgb_color kNormalBarColor = {171, 221, 161, 255};

ProgressBar::ProgressBar(BRect rect, const char *name, uint32 resizingMode)
	:	BView(rect, name, resizingMode, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW),
		fLowWater(false)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	BFont font;
	GetFont(&font);
	fMessageWidth = font.StringWidth(kMessageString);	
	font.GetHeight(&fFontMetrics);
}

void ProgressBar::SetValue(float value, bool lowWater)
{
	if (fValue != value) {
		fValue = value;
		Invalidate();
	}

	fLowWater = lowWater;
}


void ProgressBar::Draw(BRect)
{
	BRect bounds(Bounds());
	SetHighColor(0, 0, 0);
	StrokeRect(bounds);

	BRect used(bounds);
	used.InsetBy(1, 1);
	BRect unused(bounds);
	unused.InsetBy(1, 1);
	
	used.right *= fValue;
	unused.left = used.right + 1;
	
	if (fLowWater)
		SetHighColor(220, 0, 0);
	else
		SetHighColor(kNormalBarColor);

	FillRect(used);
	SetHighColor(255, 255, 255);
	FillRect(unused);
	
	if (fLowWater) {
		SetHighColor(0, 0, 0);
		MovePenTo(bounds.Width() / 2 - fMessageWidth / 2, bounds.Height() / 2
			- (fFontMetrics.ascent + fFontMetrics.descent) / 2 + fFontMetrics.ascent);
		DrawString(kMessageString);
	}
}

