//******************************************************************************
//
//	File:			IconMenuItem.cpp
//
//	Description:	Class for menu items with small icons.
//	
//	Written by:		Steve Horowitz
//
//	Copyright 1994-95, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef ICON_MENU_ITEM_H
#include "IconMenuItem.h"
#endif

#define DEBUG 1
#include <Debug.h>

// --------------------------------------------------------------


TIconMenuItem::TIconMenuItem(BBitmap *bm, const char* title, BMessage* msg,
							 bool drawText)
			:BMenuItem(title, msg)
{
	fDrawText = drawText;
	fBitmap = bm;
}

// --------------------------------------------------------------

TIconMenuItem::TIconMenuItem(BBitmap *bm, BMenu* menu, bool drawText)
			  :BMenuItem(menu)
{
	fDrawText = drawText;
	fBitmap = bm;
}

// --------------------------------------------------------------
TIconMenuItem::~TIconMenuItem()
{
	delete(fBitmap);
}

// --------------------------------------------------------------
void TIconMenuItem::SetBitmap(BBitmap *bm)
{
	if (fBitmap)
		delete fBitmap;
	fBitmap = bm;
}

// --------------------------------------------------------------
void TIconMenuItem::DrawContent()
{
	BPoint	loc;


	if (fDrawText) {
		loc = ContentLocation();
		loc.x += fBitmap ? fBitmap->Bounds().Width() + 7 : 0;
		Menu()->MovePenTo(loc);
		inherited::DrawContent();
	}

	DrawBitmap();
}

// --------------------------------------------------------------
void TIconMenuItem::Highlight(bool hilited)
{
	inherited::Highlight(hilited);
	DrawBitmap();
}

// --------------------------------------------------------------
void TIconMenuItem::DrawBitmap()
{
	BPoint	loc;

	if (!fBitmap)
		return;

	BRect	r = fBitmap->Bounds();
//+	PRINT(("cached=%d, h=%d\n", (int) fHeight, (int) r.Height()));

	loc = ContentLocation();
	loc.y += ((fHeight - r.Height()) / 2);

//	PRINT(("Icon: ")); PRINT_OBJ(loc);
	Menu()->SetDrawingMode(B_OP_OVER);
	Menu()->DrawBitmap(fBitmap, loc);
	Menu()->SetDrawingMode(B_OP_COPY);
}

// --------------------------------------------------------------
void TIconMenuItem::GetContentSize(float* width, float* height)
{
	inherited::GetContentSize(width, height);
	if (fBitmap) {
		BRect r = fBitmap->Bounds();
		*width += 7 + r.Width();
		if (r.Height() > *height)
			*height = r.Height();
	}
	fHeight = *height;
}
