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
	ResizeTo(r.Width() + 16, r.Height() + 24);
	AddChild(preview);
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

#if 0
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
#endif
