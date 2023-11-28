#include <Bitmap.h>
#include <stdio.h>

#include "Bitmaps.h"
#include "DrawingTidbits.h"
#include "VolumeSlider.h"

const int32 kMaxHeight = 15;

const int32 kVolumeThumbWidth = 11;
const int32 kVolumeThumbHeight = 11;

const unsigned char kVolumeThumb[] = {
	0xff,0xff,0x13,0x18,0x1d,0x3f,0x1d,0x19,0x12,0xff,0xff,0xff,
	0xff,0x15,0x1d,0x1e,0x1e,0x1d,0x1e,0x1b,0x19,0x12,0xff,0xff,
	0xff,0x1e,0x1e,0x1d,0x1d,0x3f,0x1d,0x1d,0x1b,0x13,0xff,0xff,
	0x1a,0x1e,0x1d,0x1d,0x1d,0x13,0x1d,0x1d,0x1d,0x19,0x12,0xff,
	0x1e,0x1e,0x1d,0x1d,0x1d,0x3f,0x1d,0x1d,0x1d,0x19,0x11,0xff,
	0x3f,0x1d,0x1d,0x1d,0x1d,0x13,0x1d,0x1d,0x1d,0x1c,0x0e,0xff,
	0x1e,0x1e,0x1d,0x1d,0x1d,0x3f,0x1d,0x1d,0x1d,0x19,0x11,0xff,
	0x1a,0x1d,0x1d,0x1d,0x1d,0x13,0x1d,0x1d,0x1d,0x15,0xff,0xff,
	0xff,0x1a,0x19,0x1d,0x1d,0x1d,0x1d,0x1d,0x17,0x12,0xff,0xff,
	0xff,0x16,0x13,0x15,0x19,0x1c,0x19,0x15,0x12,0xff,0xff,0xff,
	0xff,0xff,0xff,0x14,0x11,0x0e,0x11,0x14,0xff,0xff,0xff,0xff
};

const int32 kVolumeCapWidth = 6;
const int32 kVolumeCapHeight = 15;

const unsigned char kRightVolumeCap [] = {
	0x19,0x1b,0xff,0xff,0xff,0xff,0xff,0xff,
	0x11,0x18,0x1a,0xff,0xff,0xff,0xff,0xff,
	0x08,0x0d,0x16,0x1a,0xff,0xff,0xff,0xff,
	0x11,0x0c,0x0f,0x17,0x1a,0xff,0xff,0xff,
	0x13,0x13,0x10,0x14,0x19,0xff,0xff,0xff,
	0x13,0x13,0x13,0x15,0x19,0x1a,0xff,0xff,
	0x13,0x13,0x13,0x16,0x1a,0x1a,0xff,0xff,
	0x13,0x13,0x13,0x15,0x1c,0x1b,0xff,0xff,
	0x13,0x13,0x13,0x19,0x1e,0x1c,0xff,0xff,
	0x13,0x13,0x15,0x1d,0x1d,0x1c,0xff,0xff,
	0x13,0x14,0x1a,0x1e,0x1c,0xff,0xff,0xff,
	0x15,0x1b,0x3f,0x1d,0xff,0xff,0xff,0xff,
	0x1d,0x1e,0x1d,0x1c,0xff,0xff,0xff,0xff,
	0x1d,0x1c,0xff,0xff,0xff,0xff,0xff,0xff,
	0x1c,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

const unsigned char kLeftVolumeCap [] = {
	0xff,0xff,0xff,0xff,0x19,0x19,0xff,0xff,
	0xff,0xff,0x1a,0x18,0x11,0x0b,0xff,0xff,
	0xff,0x19,0x15,0x0b,0x06,0x0a,0xff,0xff,
	0x1a,0x15,0x09,0x09,0xb6,0x8f,0xff,0xff,
	0x18,0x0b,0x09,0x8f,0x8f,0x8f,0xff,0xff,
	0x11,0x07,0xb6,0x8f,0x8f,0x68,0xff,0xff,
	0x0c,0x0c,0x8f,0x8f,0x68,0x68,0xff,0xff,
	0x07,0x10,0x8f,0x68,0x68,0x68,0xff,0xff,
	0x16,0x12,0x8f,0x68,0x68,0x68,0xff,0xff,
	0x1a,0x16,0x11,0x68,0x68,0x68,0xff,0xff,
	0xff,0x1a,0x18,0x68,0x68,0x68,0xff,0xff,
	0xff,0xff,0x1c,0x19,0x68,0x68,0xff,0xff,
	0xff,0xff,0xff,0x1d,0x1c,0x19,0xff,0xff,
	0xff,0xff,0xff,0xff,0x1d,0x1e,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x1c,0xff,0xff
};

BBitmap *
TVolumeSlider::NewVolumeWidget()
{
	BRect bounds(0, 0, kSpeakerIconBitmapWidth - 1, kSpeakerIconBitmapHeight - 1);
	BBitmap *result = new BBitmap(bounds, B_COLOR_8_BIT);
	result->SetBits(kSpeakerIconBits, (bounds.Width() + 1) * (bounds.Height() + 1),
		0, B_COLOR_8_BIT);
	return result;
}


TVolumeSlider::TVolumeSlider(BRect frame, BMessage *message,
	int32 minValue, int32 maxValue, BBitmap *optionalWidget, uint32 resizingMode, uint32 flags)
	:	BSlider(frame, NULL, NULL, message, minValue, maxValue,
		B_BLOCK_THUMB, resizingMode, flags),
		fMin(minValue),
		fMax(maxValue),
		fOptionalWidget(optionalWidget)
{

	fLeftCapBits = new BBitmap(BRect(0,0,kVolumeCapWidth-1, kVolumeCapHeight-1),
		B_COLOR_8_BIT, false, false);
	if (fLeftCapBits)
		fLeftCapBits->SetBits(kLeftVolumeCap, fLeftCapBits->BitsLength(), 0,
			B_COLOR_8_BIT);
			
	fRightCapBits = new BBitmap(BRect(0,0,kVolumeCapWidth-1, kVolumeCapHeight-1),
		B_COLOR_8_BIT, false, false);
	if (fRightCapBits)
		fRightCapBits->SetBits(kRightVolumeCap, fRightCapBits->BitsLength(), 0,
			B_COLOR_8_BIT);
			
	fThumbBits= new BBitmap(BRect(0,0,kVolumeThumbWidth-1, kVolumeThumbHeight-1),
		B_COLOR_8_BIT, false, false);
	if (fThumbBits)
		fThumbBits->SetBits(kVolumeThumb, fThumbBits->BitsLength(), 0,
			B_COLOR_8_BIT);
}

TVolumeSlider::~TVolumeSlider()
{
	delete fThumbBits;
	delete fLeftCapBits;
	delete fRightCapBits;
	delete fOptionalWidget;
}

void
TVolumeSlider::GetPreferredSize(float* w, float* h)
{
	*w = Bounds().Width();
	*h = kMaxHeight;
}

void 
TVolumeSlider::AttachedToWindow()
{
	BSlider::AttachedToWindow();

	// make resizing redraw flicker less
	SetViewColor(B_TRANSPARENT_32_BIT);
	
	if (fOptionalWidget)
		ReplaceTransparentColor(fOptionalWidget, Parent()->ViewColor());
}


const rgb_color kDarkGreen = {102, 152, 102, 255};
const rgb_color kFillGreen = {171, 221, 161, 255};

const rgb_color kWhiteColor = {255,255,255,255};
const rgb_color kLtLtGrayColor = {224,224,224,255};
const rgb_color kLtGrayColor = {184,184,184,255};
const rgb_color kMedGrayColor = {128,128,128,255};
const rgb_color kBlackColor = {0,0,0,255};

const rgb_color kUnusedColor = {153,153,153,255};

const float kLeftBarOffset = 6;

void 
TVolumeSlider::DrawBar()
{
	BView* osv = OffscreenView();
	if (!osv)
		return;
	
	// workaround for BSlider using ViewColor to initially erase the
	// offscreen bitmap and still allow ViewColor to be transparent for
	// non-glitch updates
	osv->SetHighColor(Parent()->ViewColor());
	osv->FillRect(osv->Bounds());

	rgb_color white = (IsEnabled()) ? kWhiteColor : ShiftColor(kWhiteColor, 0.5);
	rgb_color ltgray = (IsEnabled()) ? kLtGrayColor : ShiftColor(kLtGrayColor, 0.5);
	rgb_color ltltgray = (IsEnabled()) ? kLtLtGrayColor : ShiftColor(kLtLtGrayColor, 0.5);
	rgb_color black = (IsEnabled()) ? kBlackColor : ShiftColor(kBlackColor, 0.5);
	rgb_color dkgreen = (IsEnabled()) ? kDarkGreen : ShiftColor(kDarkGreen, 0.5);
	rgb_color fillgreen = (IsEnabled()) ? kFillGreen : ShiftColor(kFillGreen, 0.5);
	rgb_color unusedgrey = (IsEnabled()) ? kUnusedColor : ShiftColor(kUnusedColor, 0.5);
	
	if (fOptionalWidget) 
		osv->DrawBitmap(fOptionalWidget, BPoint(0, 2));
	
	BPoint thumbPos = ThumbPosition();
	BRect barFrame(BarFrame());
	BRect leftFrame(barFrame);
	leftFrame.top += 2;
	leftFrame.bottom -= 2;
	BRect rightFrame(leftFrame);
	rightFrame.left = thumbPos.x;
	leftFrame.right = thumbPos.x;
	
	osv->SetHighColor(fillgreen);
	osv->FillRect(leftFrame);
	osv->SetHighColor(unusedgrey);
	osv->FillRect(rightFrame);
	
	osv->BeginLineArray(8);
	
	osv->AddLine(BPoint(leftFrame.left, leftFrame.top),
		BPoint(leftFrame.right, leftFrame.top), dkgreen);	
	osv->AddLine(BPoint(leftFrame.left, leftFrame.top+1),
		BPoint(leftFrame.right, leftFrame.top+1), dkgreen);	
	
	osv->AddLine(BPoint(barFrame.left, barFrame.top),
		BPoint(barFrame.right, barFrame.top),ltgray);
	osv->AddLine(BPoint(barFrame.left, barFrame.top+1),
		BPoint(barFrame.right, barFrame.top+1),black);
		
	osv->AddLine(BPoint(barFrame.left, barFrame.bottom-1),
		BPoint(barFrame.right, barFrame.bottom-1),white);
	osv->AddLine(BPoint(barFrame.left, barFrame.bottom),
		BPoint(barFrame.right, barFrame.bottom),ltltgray);

	osv->EndLineArray();

	osv->SetDrawingMode(B_OP_OVER);
	osv->DrawBitmap(fLeftCapBits, BPoint(barFrame.left - 6,0));
	osv->DrawBitmap(fRightCapBits, BPoint(rightFrame.right,0));	
}

void
TVolumeSlider::DrawHashMarks()
{
	BView* osv = OffscreenView();
	if (!osv)
		return;

	BRect r(BarFrame());
	if (r.Width() <= 6)
		return;

	int32 count = (int32)(r.Width() / 4);
	BPoint pt1, pt2;
	
	pt1.x = pt2.x = r.left + 4;
	pt1.y = r.top + 7;
	pt2.y = pt1.y + 1;
	
	BPoint thumbPos = ThumbPosition();
	rgb_color hashColor;

	osv->BeginLineArray(count * 2);
	while(true) {
		if (pt1.x <= thumbPos.x) {
			SetLowColor(kFillGreen);
			hashColor = kDarkGreen;
		} else {
			SetLowColor(kUnusedColor);
			hashColor = kMedGrayColor;
		}
		osv->AddLine(pt1, pt2, hashColor);

		pt1.x += 5;
		pt2.x += 5;
		if (pt1.x > r.right)
			break;
	}
	osv->EndLineArray();
}

void 
TVolumeSlider::DrawThumb()
{
	BView* osv = OffscreenView();
	if (!osv)
		return;
		
	osv->SetDrawingMode(B_OP_OVER);
	osv->DrawBitmap(fThumbBits, ThumbFrame());
}

BRect 
TVolumeSlider::BarFrame() const
{
	
	BRect result(kLeftBarOffset, 0, Bounds().Width() - 7, kMaxHeight-1);
	if (fOptionalWidget)
		result.left += fOptionalWidget->Bounds().Width() + 4;
	
	return result;
}

BRect 
TVolumeSlider::ThumbFrame() const
{
	BPoint p = ThumbPosition();
	return BRect(	p.x - kVolumeThumbWidth/2 - 1, 2,
					p.x + kVolumeThumbWidth/2 - 1, 2 + kVolumeThumbHeight - 1);
}

BPoint
TVolumeSlider::ThumbPosition() const
{
	float min = Value() - fMin;
	float max = fMax - fMin;
	float val = (min / max) * ((BarFrame().right - 1) - (BarFrame().left + 1));
	val = ceil(val);
	
	//	get the new location
	BPoint p;
	p.x = val + (BarFrame().left + 1);
	p.y = 0;
	
	return p;
}

void 
TVolumeSlider::ValueUpdated(float value)
{
	if (!IsTracking())
		SetPosition(value);
}

