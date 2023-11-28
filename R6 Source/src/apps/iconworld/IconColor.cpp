//--------------------------------------------------------------------
//	
//	IconColor.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>

#ifndef ICON_COLOR_H
#include "IconColor.h"
#endif
#ifndef ICON_WORLD_H
#include "IconWorld.h"
#endif
#ifndef ICON_VIEW_H
#include "IconView.h"
#endif
#ifndef ICON_PAT_H
#include "IconPat.h"
#endif
#ifndef	REGION_H
#include <Region.h>
#endif

#include <Screen.h>


//====================================================================

TColorView::TColorView(BRect rect)
		   :BView(rect, "", B_NOT_RESIZABLE, B_WILL_DRAW)
{
}

//--------------------------------------------------------------------

void TColorView::AttachedToWindow(void)
{
	fHighColor = 0;
	fLowColor = 255;
	SetFont(be_plain_font);
	SetFontSize(9);
}

//--------------------------------------------------------------------

void TColorView::Draw(BRect where)
{
	long		i, j, colorIndex = 0;
	BRect		r;
	long		textSize;
	BRegion		*clip;
	rgb_color	c;

	clip = new BRegion();
	GetClippingRegion(clip);

	BScreen screen( Window() );
	const color_map *cmap = screen.ColorMap();

	c.red = 0;
	c.green = 0;
	c.blue = 0;
	BeginLineArray(COLORWIDTH+COLORHEIGHT+2);
	for (i = 0; i <= COLORWIDTH; i += 1)
		AddLine(BPoint(i * COLORSIZE + WINDBORDER, WINDBORDER),
			    BPoint(i * COLORSIZE + WINDBORDER, COLORHEIGHT * COLORSIZE + WINDBORDER),c);
	for (i = 0; i <= COLORHEIGHT; i += 1)
		AddLine(BPoint(WINDBORDER, i * COLORSIZE + WINDBORDER),
			    BPoint(COLORWIDTH * COLORSIZE + WINDBORDER, i * COLORSIZE + WINDBORDER),c);
	EndLineArray();
	
	for (i = 0; i < COLORHEIGHT; i += 1)
		for (j = 0; j < COLORWIDTH; j += 1) {
			r.Set(WINDBORDER + 1 + j * COLORSIZE,
				  WINDBORDER + 1 + i * COLORSIZE,
				  WINDBORDER - 1 + COLORSIZE + j * COLORSIZE,
				  WINDBORDER - 1 + COLORSIZE + i * COLORSIZE);
			if (clip->Intersects(r)) {
				BView::SetHighColor(cmap->color_list[colorIndex]);
				FillRect(r);
			}
			colorIndex += 1;
		}

	BView::SetHighColor(0, 0, 0);
	MovePenTo(BPoint(WINDBORDER, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	DrawString("Front:");
	textSize = StringWidth("Front: 888  ") + WINDBORDER;
	MovePenTo(BPoint(textSize, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	DrawString("Red:");
	MovePenTo(BPoint(textSize, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 2)));
	DrawString("Green:");
	MovePenTo(BPoint(textSize, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 3)));
	DrawString("Blue:");

	textSize = StringWidth("Front: 888  Green: 888    ") + WINDBORDER + 24;
	MovePenTo(BPoint(textSize, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	DrawString("Back:");
	textSize += StringWidth("Back: 888  ");
	MovePenTo(BPoint(textSize, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	DrawString("Red:");
	MovePenTo(BPoint(textSize, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 2)));
	DrawString("Green:");
	MovePenTo(BPoint(textSize, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 3)));
	DrawString("Blue:");
	
	SetTheHighColor(GetTheHighColor());
	SetTheLowColor(GetTheLowColor());
	delete clip;
}

//--------------------------------------------------------------------

void TColorView::MouseDown(BPoint /* point */)
{
	long	x, y;
	short	current, new_sel, last_sel;
	ulong	buttons;
	BPoint	where;
	bool	doBack;

	doBack = modifiers() & B_COMMAND_KEY;
	GetMouse(&where, &buttons);
	if (buttons & 6)
		doBack = TRUE;
	if (doBack)
		last_sel = current = GetTheLowColor();
	else
		last_sel = current = GetTheHighColor();

	do {
		GetMouse(&where, &buttons);
		x = where.x;
		y = where.y;
		x = (x - WINDBORDER) / COLORSIZE;
		y = (y - WINDBORDER) / COLORSIZE;
		if (((x >= 0) && (x < COLORWIDTH)) && ((y >= 0) && (y < COLORHEIGHT)) &&
			((where.x > WINDBORDER) && (where.y > WINDBORDER)))
			new_sel = (y * COLORWIDTH) + x;
		else
			new_sel = current;

		if (new_sel != last_sel) {
			if (doBack)
				SetTheLowColor(new_sel);
			else
				SetTheHighColor(new_sel);
			last_sel = new_sel;
		}
		snooze(50000);
	} while(buttons);
}

//--------------------------------------------------------------------

short TColorView::GetTheHighColor(void)
{
	return fHighColor;
}

//--------------------------------------------------------------------

short TColorView::GetTheLowColor(void)
{
	return fLowColor;
}

//--------------------------------------------------------------------

void TColorView::SetTheHighColor(short color_id)
{
	char		color_str[256];
	long		x, y;
	long		offset;
	long		width;
	BRect		r;
	rgb_color	color;
	TPatWindow	*pat_wind;

	x = fHighColor % COLORWIDTH;
	y = fHighColor / COLORWIDTH;
	r.Set(x * COLORSIZE + WINDBORDER,
		  y * COLORSIZE + WINDBORDER,
		  x * COLORSIZE + WINDBORDER + COLORSIZE,
		  y * COLORSIZE + WINDBORDER + COLORSIZE);
	BView::SetHighColor(0, 0, 0);
	StrokeRect(r);

	x = color_id % COLORWIDTH;
	y = color_id / COLORWIDTH;
	r.Set(x * COLORSIZE + WINDBORDER,
		  y * COLORSIZE + WINDBORDER,
		  x * COLORSIZE + WINDBORDER + COLORSIZE,
		  y * COLORSIZE + WINDBORDER + COLORSIZE);
	BView::SetHighColor(255, 255, 255);
	StrokeRect(r);
	BView::SetHighColor(0, 0, 0);

	x = fLowColor % COLORWIDTH;
	y = fLowColor / COLORWIDTH;
	r.Set(x * COLORSIZE + WINDBORDER,
		  y * COLORSIZE + WINDBORDER,
		  x * COLORSIZE + WINDBORDER + COLORSIZE,
		  y * COLORSIZE + WINDBORDER + COLORSIZE);
	StrokeRect(r, B_MIXED_COLORS);

	fHighColor = color_id;
	pat_wind = ((TIconApp *)be_app)->fPatWind;
	if (pat_wind->Lock()) {
		pat_wind->fView->Draw(pat_wind->fView->Bounds());
		pat_wind->Unlock();
	}

	BScreen screen( Window() );
	const color_map *cmap = screen.ColorMap();

	color = cmap->color_list[color_id];
	offset = StringWidth("Front: ") + WINDBORDER;
	width = StringWidth("888");
	r.Set(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + 1,
		  offset + width, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 3));
	FillRect(r, B_SOLID_LOW);

	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	sprintf(color_str, "%d", color_id);
	DrawString(color_str);
	offset += width + StringWidth("  Green: ");
	r.left = offset;
	r.right = offset + width;
	FillRect(r, B_SOLID_LOW);
	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	sprintf(color_str, "%d", color.red);
	DrawString(color_str);
	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 2)));
	sprintf(color_str, "%d", color.green);
	DrawString(color_str);
	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 3)));
	sprintf(color_str, "%d", color.blue);
	DrawString(color_str);
}

//--------------------------------------------------------------------

void TColorView::SetTheLowColor(short color_id)
{
	char		color_str[256];
	long		x, y;
	long		offset;
	long		width;
	BRect		r;
	rgb_color	color;
	TPatWindow	*wind;

	x = fLowColor % COLORWIDTH;
	y = fLowColor / COLORWIDTH;
	r.Set(x * COLORSIZE + WINDBORDER,
		  y * COLORSIZE + WINDBORDER,
		  x * COLORSIZE + WINDBORDER + COLORSIZE,
		  y * COLORSIZE + WINDBORDER + COLORSIZE);
	StrokeRect(r);

	x = fHighColor % COLORWIDTH;
	y = fHighColor / COLORWIDTH;
	r.Set(x * COLORSIZE + WINDBORDER,
		  y * COLORSIZE + WINDBORDER,
		  x * COLORSIZE + WINDBORDER + COLORSIZE,
		  y * COLORSIZE + WINDBORDER + COLORSIZE);
	BView::SetHighColor(255, 255, 255);
	StrokeRect(r);
	BView::SetHighColor(0, 0, 0);

	x = color_id % COLORWIDTH;
	y = color_id / COLORWIDTH;
	r.Set(x * COLORSIZE + WINDBORDER,
		  y * COLORSIZE + WINDBORDER,
		  x * COLORSIZE + WINDBORDER + COLORSIZE,
		  y * COLORSIZE + WINDBORDER + COLORSIZE);
	StrokeRect(r, B_MIXED_COLORS);

	fLowColor = color_id;
	wind = ((TIconApp *)be_app)->fPatWind;
	if (wind->Lock()) {
		wind->fView->Draw(wind->fView->Bounds());
		wind->Unlock();
	}

	BScreen screen( Window() );
	const color_map *cmap = screen.ColorMap();

	color = cmap->color_list[color_id];
	offset = StringWidth("Front: 888  Green: 888    Back: ") + WINDBORDER + 24;
	width = StringWidth("888");
	r.Set(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + 1,
		  offset + width, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 3));
	FillRect(r, B_SOLID_LOW);

	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	sprintf(color_str, "%d", color_id);
	DrawString(color_str);
	offset += width + StringWidth("  Green: ");
	r.left = offset;
	r.right = offset + width;
	FillRect(r, B_SOLID_LOW);
	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 1)));
	sprintf(color_str, "%d", color.red);
	DrawString(color_str);
	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 2)));
	sprintf(color_str, "%d", color.green);
	DrawString(color_str);
	MovePenTo(BPoint(offset, COLORHEIGHT * COLORSIZE + WINDBORDER + (13 * 3)));
	sprintf(color_str, "%d", color.blue);
	DrawString(color_str);
}
