/* ++++++++++

   FILE:  MenuColor.cpp
   REVS:  $Revision: 1.7 $
   NAME:  peter
   DATE:  Mon May 13 10:50:24 PDT 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <Debug.h>

#ifndef MENU_COLOR_CONTROL_H
#include "MenuColorControl.h"
#endif

#ifndef MAIN_H
#include "main.h"
#endif

/*------------------------------------------------------------*/

TMenuColorControl::TMenuColorControl(BPoint start, color_control_layout layout,
	long size, const char *name, BMessage *msg, bool use_offscreen,
	BWindow *window)
	: BColorControl(start, layout, size, name, msg, use_offscreen)
{
	fParent = window;
	fSampleView = NULL;
}


/*------------------------------------------------------------*/

static void	draw_rect(BView *v, BRect r)
{
	v->SetHighColor(136, 136, 136);
	v->StrokeLine(r.LeftTop(), r.RightTop());
	v->StrokeLine(r.LeftTop(), r.LeftBottom());
	r.OffsetBy(1, 1);
	v->SetHighColor(255, 255, 255);
	v->StrokeLine(r.LeftBottom(), r.RightBottom());
	v->StrokeLine(r.RightTop(), r.RightBottom());
}

/*------------------------------------------------------------*/

static void	draw_color(BView *v, BRect r, rgb_color c)
{
	r.top += 1;
	r.left += 1;
	v->SetHighColor(c);
	v->FillRect(r);
}

/*------------------------------------------------------------*/

static uchar shift_component(uchar val, float percent)
{
	if (percent >= 1.0)
		return (val - (val * ((int)percent - 1)));
	else
		return (val + ((256 - val) * (1 - (int)percent)));
}

/*------------------------------------------------------------*/

static rgb_color my_shift_color(rgb_color c, float percent)
{
	rgb_color n;

	n.red = shift_component(c.red, percent);
	n.green = shift_component(c.green, percent);
	n.blue = shift_component(c.blue, percent);

//+	PRINT(("old(%d, %d, %d) * %.2f = new(%d, %d, %d)\n",
//+		c.red, c.green, c.blue, percent, n.red, n.green, n.blue));

	return n;
}

/*------------------------------------------------------------*/

void TMenuColorControl::SetValue(long color)
{
	BColorControl::SetValue(color);

	if (fParent) {
		BMessage *msg = new BMessage(CMD_TMP_COLOR_CHANGE);
		msg->AddInt32("color", color);
		fParent->PostMessage(msg);
	}

	if (fSampleView) {
		rgb_color rgb = ValueAsColor();
		fSampleView->fColor = rgb;
		fSampleView->DrawColorRamp();
	}
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/

TSampleView::TSampleView(BRect rect)
	:BView(rect, "sample_view", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fColor.red = fColor.green = fColor.blue = 0;
}

/*------------------------------------------------------------*/

void TSampleView::Draw(BRect)
{
	DrawColorRamp();
}

/*------------------------------------------------------------*/

void TSampleView::DrawColorRamp()
{
	rgb_color	rgb = fColor;
	BRect		r(0, 0, 17, 17);

	MovePenTo(5, 19);
	SetHighColor(0,0,0);
	DrawString("Color scheme:");

	r.OffsetTo(PenLocation().x + 5, 5);

	draw_rect(this, r);
	draw_color(this, r, my_shift_color(rgb, 0.70));

	r.OffsetBy(27, 0);
	draw_rect(this, r);
	draw_color(this, r, my_shift_color(rgb, 0.85));

	r.OffsetBy(27, 0);

	SetHighColor(0,0,0);
	SetPenSize(2);
	StrokeLine(BPoint(r.left, r.top - 2), BPoint(r.right, r.top - 2));
	StrokeLine(BPoint(r.left, r.bottom + 5), BPoint(r.right, r.bottom + 5));
	SetPenSize(1);
	draw_rect(this, r);
	draw_color(this, r, rgb);

	r.OffsetBy(27, 0);
	draw_rect(this, r);
	draw_color(this, r, my_shift_color(rgb, 1.1));

	r.OffsetBy(27, 0);
	draw_rect(this, r);
	draw_color(this, r, my_shift_color(rgb, 1.2));

	r.OffsetBy(27, 0);
	draw_rect(this, r);
	draw_color(this, r, my_shift_color(rgb, 1.3));

	r.OffsetBy(27, 0);
	draw_rect(this, r);
	draw_color(this, r, my_shift_color(rgb, 1.4));
	Flush();
}
