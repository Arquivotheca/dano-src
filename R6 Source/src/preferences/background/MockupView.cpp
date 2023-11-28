//*****************************************************************************
//
//	File:		 MockupView.cpp
//
//	Description: Mockup drawing view class for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "MockupView.h"

MockupView::MockupView(const char *name, BView *preview, uint32 resizingMode, uint32 flags)
 : BView(BRect(0, 0, 0, 0), name, resizingMode, flags)
{
	BRect r = preview->Frame();
	MoveTo(r.left - 8, r.top - 12);
	preview->MoveTo(8, 12);
	AddChild(preview);
	SetMode(MOCKUP_MONITOR);
}

void MockupView::AttachedToWindow(void)
{
	if(Parent())
	{
		SetLowColor(Parent()->LowColor());
		SetViewColor(Parent()->LowColor());
	}
}

void MockupView::Draw(BRect /*upd*/)
{
	BRect in = Bounds();
	FillRect(in, B_SOLID_LOW);	// background

	if(mode == MOCKUP_MONITOR)
	{
		// frame
		BRect	r(in.left + 3, in.top + 7, in.right - 3, in.bottom - 7);
		SetHighColor(184, 184, 184);
		FillRect(r);				// gray inside
		r.InsetBy(-1, -1);
		SetHighColor(96, 96, 96);
		StrokeRoundRect(r, 3, 3);	// dark outside frame
		SetHighColor(0, 0, 0);
		r.InsetBy(5, 5);
		StrokeRect(r);				// black inside frame

		// bottom
		BRect bottom(in.left + 25, in.bottom - 7, in.right - 25, in.bottom);
		SetHighColor(96, 96, 96);
		StrokeRoundRect(bottom, 2, 2);	// dark outside frame
		SetHighColor(184, 184, 184);
		bottom.Set(bottom.left + 1, bottom.top, bottom.right - 1, bottom.bottom - 1);
		FillRect(bottom);

		// led + button
		SetHighColor(0, 255, 0);
		FillRect(BRect(in.right - 42, in.bottom - 5, in.right - 40, in.bottom - 4));
		SetHighColor(96, 96, 96);
		FillRect(BRect(in.right - 36, in.bottom - 6, in.right - 32, in.bottom - 4));
	}
	else
	{
		// outside rects
		SetHighColor(152, 152, 152);
		StrokeRect(BRect(in.left + 4, in.top + 8, in.right - 4, in.bottom - 8));
		StrokeRect(BRect(in.left + 4, in.top, in.left + 60, in.top + 8));

		// gray frame
		SetHighColor(240, 240, 240);
		StrokeRect(BRect(in.left + 5, in.top + 9, in.right - 5, in.bottom - 9));
		StrokeRect(BRect(in.left + 6, in.top + 10, in.right - 6, in.bottom - 10));
		SetHighColor(200, 200, 200);
		StrokeRect(BRect(in.left + 6, in.top + 10, in.right - 5, in.bottom - 9));

		// yellow label
		SetHighColor(255, 203, 0);
		FillRect(BRect(in.left + 5, in.top + 1, in.left + 59, in.top + 8));

		// inside rect
		SetHighColor(152, 152, 152);
		StrokeRect(BRect(in.left + 7, in.top + 11, in.right - 7, in.bottom - 11));					// black inside frame
	}
}

void MockupView::SetMode(mockup_t m)
{
	mode = m;
	BView	*preview = ChildAt(0);
	BRect r = preview->Frame();
	ResizeTo(r.Width() + 16, r.Height() + 24);
	if(Window())
	{
		Sync();
		Draw(Bounds());
	}
}
