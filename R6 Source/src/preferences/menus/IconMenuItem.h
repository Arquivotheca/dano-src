//******************************************************************************
//
//	File:			IconMenuItem.h
//
//	Description:	Header for menu item class with small icons.
//	
//	Written by:		Steve Horowitz
//
//	Copyright 1994-95, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef	ICON_MENU_ITEM_H
#define ICON_MENU_ITEM_H

#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif

//---------------------------------------------------------------
class TIconMenuItem : public BMenuItem {


public:
						TIconMenuItem(BBitmap *bm, const char* title,
									  BMessage*, bool drawText = TRUE);
						TIconMenuItem(BBitmap *bm, BMenu*,
									  bool drawText = TRUE);
						~TIconMenuItem();
virtual	void			DrawContent();
virtual	void			Highlight(bool isHighlighted);
virtual	void			GetContentSize(float* width, float* height);

		void			SetBitmap(BBitmap *bm);

private:
		void			DrawBitmap();

		BBitmap*		fBitmap;
		bool			fDrawText;
		float			fHeight;
		typedef BMenuItem inherited;
};

#endif
