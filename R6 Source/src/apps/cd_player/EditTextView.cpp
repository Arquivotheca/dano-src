//--------------------------------------------------------------------
//	
//	EditTextView.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef EDIT_TEXT_VIEW_H
#include "EditTextView.h"
#endif

#include <Window.h>


//====================================================================

TEditTextView::TEditTextView(BRect frame, BRect textRect, bool flag)
			  :BTextView(frame, "", textRect, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fFlag = flag;
}

//--------------------------------------------------------------------

void TEditTextView::AttachedToWindow(void)
{
	rgb_color	c;

	BTextView::AttachedToWindow();
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

	if (fFlag)
		SetFontAndColor(be_bold_font);
	else
		SetFontAndColor(be_plain_font);

	MakeFocus();
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetWordWrap(FALSE);
}

//--------------------------------------------------------------------

void TEditTextView::KeyDown(const char* key, int32 len)
{
	uchar		c;
	uint32		mods;
	BMessage	*msg;

	c = key[0];
	msg = Window()->CurrentMessage();
	mods = msg->FindInt32("modifiers");
	if ((c == B_TAB) && (mods & B_SHIFT_KEY))
		c = B_UP_ARROW;

	if (c == B_RETURN)
		Window()->PostMessage(KILL_EDIT_SAVE);
	else if (c == B_ESCAPE)
		Window()->PostMessage(KILL_EDIT_NOSAVE);
	else if ((c == B_TAB) || (c == B_DOWN_ARROW))
		Window()->PostMessage(NEXT_FIELD);
	else if (c == B_UP_ARROW)
		Window()->PostMessage(PREV_FIELD);
	else if ((c >= ' ') || (c == B_BACKSPACE) ||
			 (c == B_DELETE) ||
			 (c == B_RIGHT_ARROW) || (c == B_LEFT_ARROW))
		BTextView::KeyDown(key, len);
}

