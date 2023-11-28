//--------------------------------------------------------------------
//	
//	Slider.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DiskProbe.h"
#include "ProbeWindow.h"
#include "Slider.h"

const rgb_color kWhiteColor = {255,255,255,0};
const rgb_color kWhiteGrayColor = {235,235,235,0};
const rgb_color kLtGrayColor = {176,176,176,0};
const rgb_color kMedGrayColor = {144,144,144,0};
const rgb_color kBlackColor = {0,0,0,0};
const rgb_color kBarColor = {184,184,184,0};
const rgb_color kFillColor = {102, 152, 203, 0};


//====================================================================

TSliderView::TSliderView(BRect rect, off_t value, off_t max)
			:BView(rect, "d_slider", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW |
															B_FRAME_EVENTS)
{
	fValue = value;
	fMax = max;
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	BRect r = rect;
	r.OffsetTo(0, 0);
	fOffScreenView = new BView(r, "", B_FOLLOW_ALL, B_WILL_DRAW);
	fOffScreenBits = new BBitmap(r, B_COLOR_8_BIT, true);
	fOffScreenBits->AddChild(fOffScreenView);
}

//--------------------------------------------------------------------

TSliderView::~TSliderView(void)
{
	delete fOffScreenBits;
}

//--------------------------------------------------------------------

void TSliderView::Draw(BRect where)
{
	BRect		r;
	int32		thumb;
	rgb_color	white = kWhiteColor;
	rgb_color	whitegray = kWhiteGrayColor;
	rgb_color	ltgray = kLtGrayColor;
	rgb_color	medgray = kMedGrayColor;
	rgb_color	black = kBlackColor;
	rgb_color	fill = kFillColor;
	rgb_color	c;

	fOffScreenBits->Lock();
	r = fOffScreenView->Bounds();
	fOffScreenView->SetHighColor(ViewColor());
	fOffScreenView->FillRect(r);
	fOffScreenView->SetHighColor(ui_color(B_SHINE_COLOR));
	fOffScreenView->StrokeLine(BPoint(r.left, r.top), BPoint(r.left, r.bottom));

	r = BarFrame();

	// draw the trim
	fOffScreenView->BeginLineArray(11);	

	// trim - top left, bottom left, top right, 3 corners - single pixel
	fOffScreenView->AddLine(BPoint(r.left, r.top),
		BPoint(r.left, r.top),ltgray);
	fOffScreenView->AddLine(BPoint(r.left, r.bottom),
		BPoint(r.left, r.bottom),ltgray);
	fOffScreenView->AddLine(BPoint(r.right, r.top),
		BPoint(r.right, r.top),ltgray);
		
	// gray - left edge, top edge
	fOffScreenView->AddLine(BPoint(r.left, r.top+1),
		BPoint (r.left, r.bottom-1),medgray);
	fOffScreenView->AddLine(BPoint(r.left+1, r.top),
		BPoint(r.right - 1, r.top),medgray);
		
	// black - left edge, top edge, inset 1 pixel
	fOffScreenView->AddLine(BPoint(r.left+1, r.top+2),
		BPoint (r.left+1, r.bottom-1),black);
	fOffScreenView->AddLine(BPoint(r.left+1, r.top+1),
		BPoint(r.right - 1, r.top+1),black);
		
	// white - bottom edge, right edge
	fOffScreenView->AddLine(BPoint(r.left+1, r.bottom),
		BPoint(r.right, r.bottom),white);
	fOffScreenView->AddLine(BPoint(r.right, r.top+1),
		BPoint(r.right, r.bottom-1),white);
		
	fOffScreenView->EndLineArray();
	
	// fill in the bar
	fOffScreenView->SetHighColor(kBarColor);
	
	r.left += 2;
	r.right -= 1;
	r.top += 2;
	r.bottom -= 1;
	fOffScreenView->FillRect(r);

	//	add the fill color, left of thumb
	if ((fMax > 0) && (fValue < fMax))
		thumb = (int32)(r.left + (r.Width() * (float)fValue / (float)fMax));
	else
		thumb = (int32) r.right;

	r.right = thumb;
	fOffScreenView->SetHighColor(kFillColor);
	fOffScreenView->FillRect(r);

	if (fMax > 0) {
		// 	fill triangle , white/gray
		fOffScreenView->SetHighColor(whitegray);
		fOffScreenView->FillTriangle(BPoint(thumb, r.bottom - 2),
			BPoint(thumb - 6, r.bottom + 4),
			BPoint(thumb + 6, r.bottom + 4));
	
		fOffScreenView->BeginLineArray(6);
		
		// black right edge
		fOffScreenView->AddLine(BPoint(thumb, r.bottom - 2),
			BPoint(thumb + 6, r.bottom + 4), black);
		// black base line
		fOffScreenView->AddLine(BPoint(thumb - 6, r.bottom + 5),
			BPoint(thumb + 6, r.bottom + 5), black);
		// left edge, med gray
		c.red = 120; c.green = 120; c.blue = 120;
		fOffScreenView->AddLine(BPoint(thumb - 6, r.bottom + 4),
			BPoint(thumb - 1,r.bottom - 1), c);
		// bottom med gray
		fOffScreenView->AddLine(BPoint(thumb - 6, r.bottom + 4),
			BPoint(thumb + 5, r.bottom + 4), c);
		// right edge shade, lt gray
		c.red = 200; c.green = 200; c.blue = 200;
		fOffScreenView->AddLine(BPoint(thumb, r.bottom - 1),
			BPoint(thumb + 4, r.bottom + 3), c);
		// bottom edge, lt gray
		fOffScreenView->AddLine(BPoint(thumb - 3, r.bottom + 3),
			BPoint(thumb + 4, r.bottom + 3), c);
		
		fOffScreenView->EndLineArray();
	}

	fOffScreenView->Sync();
	fOffScreenBits->Unlock();

	DrawBitmap(fOffScreenBits, BPoint(Bounds().left, Bounds().top));
}

//--------------------------------------------------------------------

void TSliderView::FrameResized(float w, float h)
{
	BRect	r;

	r = Bounds();

	if (r.IsValid()) {
		fOffScreenBits->RemoveChild(fOffScreenView);
		delete fOffScreenBits;
		r.OffsetTo(0, 0);
		fOffScreenView->ResizeTo(r.Width(),r.Height());
		fOffScreenBits = new BBitmap(r, B_COLOR_8_BIT, true);
		fOffScreenBits->AddChild(fOffScreenView);
	}
	Draw(Bounds());
}

//--------------------------------------------------------------------

void TSliderView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			BView::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TSliderView::MouseDown(BPoint where)
{
	uint32		buttons;
	BPoint		point;
	BRect		r;
	off_t		value;

	if (fMax <= 0)
		return;
	r = BarFrame();
	r.InsetBy(2, 2);
	point = where;
	do {
		value = (off_t) ((point.x - r.left) * (((float)fMax + 1) / r.Width()));
		if (value < 0)
			value = 0;
		else if (value > fMax)
			value = fMax;
		if (value != fValue)
			((TProbeWindow *)Window())->Read(value);
		snooze(100000);
		GetMouse(&point, &buttons);
	} while (buttons);
}

//--------------------------------------------------------------------

BRect TSliderView::BarFrame(void)
{
	BRect		r;

	fOffScreenBits->Lock();
	r = fOffScreenView->Bounds();
	r.top += (int)((r.Height() - 7) / 2);
	r.left +=  2+ (THUMB_WIDTH / 2);
	r.right -= 2 + (THUMB_WIDTH / 2);
	r.bottom = r.top + 7;
	fOffScreenBits->Unlock();
	return r;
}

//--------------------------------------------------------------------

void TSliderView::SetMax(off_t max, off_t value)
{
	if (fMax != max) {
		fMax = max;
		fValue = value;
		Draw(Bounds());
	}
	else
		SetValue(value);
}

//--------------------------------------------------------------------

void TSliderView::SetValue(off_t value)
{
	int32		thumb;
	BRect		r;

	if (fValue != value) {
		r = BarFrame();
		r.InsetBy(2, 2);
		thumb = (int32)(r.left + (r.Width() * (float)fValue / (float)fMax));
		fValue = value;
		if (thumb != r.left + (r.Width() * (float)fValue / (float)fMax))
			Draw(Bounds());	
	}
}

//--------------------------------------------------------------------

off_t TSliderView::Value(void)
{
	return fValue;
}
