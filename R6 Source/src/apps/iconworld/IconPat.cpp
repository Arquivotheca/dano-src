//--------------------------------------------------------------------
//	
//	IconPat.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_PAT_H
#include "IconPat.h"
#endif
#ifndef ICON_WORLD_H
#include "IconWorld.h"
#endif
#ifndef ICON_VIEW_H
#include "IconView.h"
#endif
#ifndef ICON_COLOR_H
#include "IconColor.h"
#endif

#include <Screen.h>


pattern pattern00[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
pattern pattern01[] = {0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55};
pattern pattern02[] = {0xdd,0x77,0xdd,0x77,0xdd,0x77,0xdd,0x77};
pattern pattern03[] = {0xff,0xdd,0xff,0x77,0xff,0xdd,0xff,0x77};
pattern pattern04[] = {0xff,0xdf,0xff,0xff,0xff,0xfd,0xff,0xff};
pattern pattern05[] = {0xff,0xdf,0xff,0xff,0xff,0xff,0xff,0xff};
pattern pattern06[] = {0xff,0xaa,0xff,0xaa,0xff,0xaa,0xff,0xaa};
pattern pattern07[] = {0x7f,0xef,0xfd,0xdf,0xfe,0xf7,0xbf,0xfb};
pattern pattern08[] = {0xee,0xdd,0xbb,0x77,0xee,0xdd,0xbb,0x77};
pattern pattern09[] = {0xff,0x22,0xff,0x88,0xff,0x22,0xff,0x88};
pattern pattern10[] = {0x7f,0xbf,0xdf,0xff,0xfd,0xfb,0xf7,0xff};
pattern pattern11[] = {0x77,0xbb,0xdd,0xff,0x77,0xee,0xdd,0xff};
pattern pattern12[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
pattern pattern13[] = {0x55,0xaa,0x55,0xaa,0x55,0xaa,0x55,0xaa};
pattern pattern14[] = {0x88,0x22,0x88,0x22,0x88,0x22,0x88,0x22};
pattern pattern15[] = {0x00,0x22,0x00,0x88,0x00,0x22,0x00,0x88};
pattern pattern16[] = {0x00,0x20,0x00,0x00,0x00,0x02,0x00,0x00};
pattern pattern17[] = {0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00};
pattern pattern18[] = {0x00,0x55,0x00,0x55,0x00,0x55,0x00,0x55};
pattern pattern19[] = {0x80,0x10,0x02,0x20,0x01,0x08,0x40,0x04};
pattern pattern20[] = {0x11,0x22,0x44,0x88,0x11,0x22,0x44,0x88};
pattern pattern21[] = {0x00,0xdd,0x00,0x77,0x00,0xbd,0x00,0xed};
pattern pattern22[] = {0x80,0x40,0x20,0x00,0x02,0x04,0x08,0x00};
pattern pattern23[] = {0x88,0x44,0x22,0x00,0x88,0x11,0x22,0x00};


//====================================================================

TPatView::TPatView(BRect rect)
		 :BView(rect, "", B_NOT_RESIZABLE, B_WILL_DRAW)
{
}

//--------------------------------------------------------------------

void TPatView::AttachedToWindow(void)
{
	fPats[0] = pattern00;
	fPats[1] = pattern01;
	fPats[2] = pattern02;
	fPats[3] = pattern03;
	fPats[4] = pattern04;
	fPats[5] = pattern05;
	fPats[6] = pattern06;
	fPats[7] = pattern07;
	fPats[8] = pattern08;
	fPats[9] = pattern09;
	fPats[10] = pattern10;
	fPats[11] = pattern11;
	fPats[12] = pattern12;
	fPats[13] = pattern13;
	fPats[14] = pattern14;
	fPats[15] = pattern15;
	fPats[16] = pattern16;
	fPats[17] = pattern17;
	fPats[18] = pattern18;
	fPats[19] = pattern19;
	fPats[20] = pattern20;
	fPats[21] = pattern21;
	fPats[22] = pattern22;
	fPats[23] = pattern23;

	fPat = 0;
}

//--------------------------------------------------------------------

void TPatView::Draw(BRect where)
{
	short		pat = 0;
	long		i;
	long		x = 0, y = 0;
	BRect		r;
	rgb_color	c;
	TColorWindow	*wind;

	wind = ((TIconApp *)be_app)->fColorWind;
	c.red = 0;
	c.green = 0;
	c.blue = 0;
	BeginLineArray(PATWIDTH + PATHEIGHT + 2);
	for (i = 0; i <= PATWIDTH; i += 1)
		AddLine(BPoint(i * PATSIZE + WINDBORDER, WINDBORDER),
			BPoint(i * PATSIZE + WINDBORDER, PATHEIGHT * PATSIZE + WINDBORDER),c);
	for (i = 0; i <= PATHEIGHT; i += 1)
		AddLine(BPoint(WINDBORDER, i * PATSIZE + WINDBORDER),
		    	BPoint(PATWIDTH * PATSIZE + WINDBORDER, i * PATSIZE + WINDBORDER),c);
	EndLineArray();

	BScreen screen( Window() );
	const color_map *cmap = screen.ColorMap();
	SetHighColor(cmap->color_list[wind->fView->GetTheHighColor()]);
	SetLowColor(cmap->color_list[wind->fView->GetTheLowColor()]);
	
	for (y = 0; y < PATHEIGHT; y += 1) {
		for (x = 0; x < PATWIDTH; x += 1) {
			r.Set(WINDBORDER + 3 + x * PATSIZE,
				  WINDBORDER + 3 + y * PATSIZE,
				  WINDBORDER - 3 + PATSIZE + x * PATSIZE,
				  WINDBORDER - 3 + PATSIZE + y * PATSIZE);
			if (where.Intersects(r)) 
				FillRect(r,*GetPatMap(pat));
			pat += 1;
		}
	}

	SetHighColor(0, 0, 0);
	SetLowColor(255, 255, 255);

	pat = GetPat();
	x = pat % PATWIDTH;
	y = pat / PATWIDTH;
	r.Set(x * PATSIZE + WINDBORDER + 1,
		  y * PATSIZE + WINDBORDER + 1,
		  x * PATSIZE + WINDBORDER - 1 + PATSIZE,
		  y * PATSIZE + WINDBORDER - 1 + PATSIZE);
	StrokeRect(r);
	Sync();
}

//--------------------------------------------------------------------

void TPatView::MouseDown(BPoint /* point */)
{
	short	current, new_sel,last_sel;
	long	x, y;
	ulong	buttons;
	BPoint	where;
	BRect	r;

	current = last_sel = GetPat();
	do {
		GetMouse(&where, &buttons);
		x = where.x;
		y = where.y;
		x = (x - WINDBORDER) / PATSIZE;
		y = (y - WINDBORDER) / PATSIZE;
		if (((x >= 0) && (x < PATWIDTH)) && ((y >= 0) && (y < PATHEIGHT)) &&
			((where.x > WINDBORDER) && (where.y > WINDBORDER)))
			new_sel = (y * PATWIDTH) + x;
		else
			new_sel = current;
		if (new_sel != last_sel) {
			x = last_sel % PATWIDTH;
			y = last_sel / PATWIDTH;
			SetHighColor(255, 255, 255);
			r.Set(x * PATSIZE + WINDBORDER + 1,
				  y * PATSIZE + WINDBORDER + 1,
				  x * PATSIZE + WINDBORDER - 1 + PATSIZE,
				  y * PATSIZE + WINDBORDER - 1 + PATSIZE);
			StrokeRect(r);
			x = new_sel % PATWIDTH;
			y = new_sel / PATWIDTH;
			SetHighColor(0, 0, 0);
			r.Set(x * PATSIZE + WINDBORDER + 1,
				  y * PATSIZE + WINDBORDER + 1,
				  x * PATSIZE + WINDBORDER - 1 + PATSIZE,
				  y * PATSIZE + WINDBORDER - 1 + PATSIZE);
			StrokeRect(r);
			last_sel = new_sel;
		}
		snooze(50000);
	} while(buttons);
	SetPat(new_sel);
}

//--------------------------------------------------------------------

short TPatView::GetPat(void)
{
	return	fPat;
}

//--------------------------------------------------------------------

pattern* TPatView::GetPatMap(short patID)
{
	return fPats[patID];
}

//--------------------------------------------------------------------

void TPatView::SetPat(short patID)
{
	fPat = patID;
}
