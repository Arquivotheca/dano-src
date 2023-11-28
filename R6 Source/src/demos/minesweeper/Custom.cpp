//--------------------------------------------------------------------
//	
//	Custom.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Minesweeper.h"
#include "Custom.h"

extern	TCustomWindow*		custom;


//====================================================================

TCustomWindow::TCustomWindow(BRect rect, int32 x, int32 y, int32 b)
			  :BWindow(rect, "Custom", B_TITLED_WINDOW, B_NOT_RESIZABLE |
			  											B_NOT_ZOOMABLE)
{
	char		str[256];
	BBox		*box;
	BButton		*button;
	BRect		r;

	fX = x;
	fY = y;
	fB = b;

	r = rect;
	r.OffsetTo(0, 0);
	r.InsetBy(-1, -1);
	box = new BBox(r, "");
	AddChild(box);

	r.Set(CUSTOM_OK_X1, CUSTOM_OK_Y1, CUSTOM_OK_X2, CUSTOM_OK_Y2);
	fOK = new BButton(r, "button", CUSTOM_OK_TEXT, new BMessage(M_OK));
	fOK->MakeDefault(true);
	box->AddChild(fOK);

	r.Set(CUSTOM_CANCEL_X1, CUSTOM_CANCEL_Y1, CUSTOM_CANCEL_X2, CUSTOM_CANCEL_Y2);
	button = new BButton(r, "button", CUSTOM_CANCEL_TEXT, new BMessage(M_CANCEL));
	box->AddChild(button);

	r.Set(WIDTH_X1, WIDTH_Y1, WIDTH_X2, WIDTH_Y2);
	sprintf(str, "%d", x);
	fWidth = new BTextControl(r, "", WIDTH_TEXT, str, new BMessage(M_WIDTH));
	fWidth->SetModificationMessage(new BMessage(M_WIDTH));
	box->AddChild(fWidth);

	r.Set(HEIGHT_X1, HEIGHT_Y1, HEIGHT_X2, HEIGHT_Y2);
	sprintf(str, "%d", y);
	fHeight = new BTextControl(r, "", HEIGHT_TEXT, str, new BMessage(M_HEIGHT));
	fHeight->SetModificationMessage(new BMessage(M_HEIGHT));
	box->AddChild(fHeight);

	r.Set(BOMBS_X1, BOMBS_Y1, BOMBS_X2, BOMBS_Y2);
	sprintf(str, "%d", b);
	fBombs = new BTextControl(r, "", BOMBS_TEXT, str, new BMessage(M_BOMBS));
	fBombs->SetModificationMessage(new BMessage(M_BOMBS));
	box->AddChild(fBombs);
	fWidth->MakeFocus(true);
}

//--------------------------------------------------------------------

void TCustomWindow::MessageReceived(BMessage* theMessage)
{
	char		str[256];
	int32		x, y, b;
	BMessage	msg(M_CUSTOM);

	x = strtol(fWidth->Text(), NULL, 10);
	y = strtol(fHeight->Text(), NULL, 10);
	b = strtol(fBombs->Text(), NULL, 10);

	switch(theMessage->what) {
		case M_OK:
			if ((x < 8) || (x > 1000)) {
				(new BAlert("", "Width must be between 8 and 1000", "OK"))->Go();
				break;
			}
			else if ((y < 8) || (y > 1000)) {
				(new BAlert("", "Height must be between 8 and 1000.", "OK"))->Go();
				break;
			}
			else if ((x * y > 0) && ((b >= x * y) || (b > 999))) {
				sprintf(str, "Bombs must be between 1 and %d.", min_c(999, (x * y) - 1));
				(new BAlert("", str, "OK"))->Go();
				break;
			}

			msg.AddInt32("width", x);
			msg.AddInt32("height", y);
			msg.AddInt32("bombs", b);
			be_app->PostMessage(&msg);

		case M_CANCEL:
			if (Lock()) {
				custom = NULL;
				Close();
			}
			break;

		case M_WIDTH:
		case M_HEIGHT:
		case M_BOMBS:
			break;

		default:
			break;
	}
}
