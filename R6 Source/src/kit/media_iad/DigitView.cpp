#include <Bitmap.h>
#include <math.h>
#include <stdio.h>
#include "DigitView.h"

const int32 DIGIT_WIDTH = 10;
const int32 DIGIT_HEIGHT = 15;
const int32 MARGIN = 4;

extern uchar DigitData[10][DIGIT_WIDTH * DIGIT_HEIGHT * 3];

DigitView::DigitView(int32 digits)
	: BView(BRect(0, 0, digits * (DIGIT_WIDTH + MARGIN) + MARGIN - 1, DIGIT_HEIGHT + MARGIN + MARGIN - 1), "digits", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	mValue = 0;
	mDigits = digits;
	SetViewColor(0, 0, 0);
	
	for (int x = 0; x < 10; x++)
	{
		mDigitBits[x] = new BBitmap(BRect(0, 0, DIGIT_WIDTH - 1, DIGIT_HEIGHT - 1), B_RGB_32_BIT);
		mDigitBits[x]->SetBits(DigitData[x], DIGIT_WIDTH * DIGIT_HEIGHT * 3, 0, B_RGB32);
	}
}

DigitView::~DigitView()
{
	for (int x = 0; x < 10; x++)
		delete mDigitBits[x];
 }

void DigitView::Draw(BRect r)
{
	int32 value = mValue, d, mult;
	for (int n = 0; n < mDigits; n++)
	{
		mult = (int32) pow(10.0, mDigits - n - 1);
		d = value / mult;
		value -= d * mult;
		DrawBitmapAsync(mDigitBits[d], BPoint(MARGIN + n * (DIGIT_WIDTH + MARGIN), 4));
	}
	Sync();
}
