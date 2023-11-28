//--------------------------------------------------------------------
//	
//	Header.cpp
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
#include "Header.h"


//====================================================================

THeaderView::THeaderView(BRect rect, int32 base, int32 type)
			:BView(rect, "d_header", B_FOLLOW_NONE, B_WILL_DRAW)
{
	char		*text;
	BFont		bold_font = *be_bold_font;
	BFont		plain_font = *be_plain_font;
	BRect		r;
	BStringView	*str;

	fBase = base;
	fSize = 0;
	fIcon = NULL;

	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	bold_font.SetSize(FONT_SIZE);
	plain_font.SetSize(FONT_SIZE);

	(type == T_FILE) ? text = FILE_TEXT :
						text = DEVICE_TEXT;
	r.Set(LABEL_FIELD_X1, LABEL_FIELD_Y1,
		  LABEL_FIELD_X1 + bold_font.StringWidth(text), LABEL_FIELD_Y2);
	str = new BStringView(r, "", text);
	str->SetFont(&bold_font);
	AddChild(str);

	r.left = r.right;
	r.right = 32767;
	fName = new BStringView(r, "", "");
	fName->SetFont(&plain_font);
	AddChild(fName);	

	r.Set(BLOCK_FIELD_X1, BLOCK_FIELD_Y1,
		  BLOCK_FIELD_X1 + plain_font.StringWidth(BLOCK_TEXT) + bold_font.StringWidth(" "), BLOCK_FIELD_Y2);
	str = new BStringView(r, "", BLOCK_TEXT);
	str->SetFont(&bold_font);
	AddChild(str);

	r.left = r.right;
	r.top -= 4;
	r.right += plain_font.StringWidth("000000000000");
	r.bottom -= 4;
	fPosition = new BTextControl(r, "", "", "", new BMessage(M_BLOCK));
	fPosition->SetFont(&plain_font);
	fPosition->SetDivider(0.0);
	fPosition->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	AddChild(fPosition);

	r.left = r.right + plain_font.StringWidth(" ");
	r.top += 4;
	r.right = r.left + plain_font.StringWidth("of 000000000000");
	r.bottom += 4;
	fTotal = new BStringView(r, "", "");
	fTotal->SetFont(&plain_font);
	AddChild(fTotal);

	r.left = r.right + plain_font.StringWidth("M");
	r.right = r.left + bold_font.StringWidth(BLOCK_OFFSET_TEXT);
	str = new BStringView(r, "", BLOCK_OFFSET_TEXT);
	str->SetFont(&bold_font);
	AddChild(str);

	r.left = r.right;
	r.right = r.left + StringWidth("000000");
	fOffset1 = new BStringView(r, "", "");
	fOffset1->SetFont(&plain_font);
	AddChild(fOffset1);

	(type == T_FILE) ? text = FILE_OFFSET_TEXT :
						text = DEVICE_OFFSET_TEXT;
	r.left = r.right + plain_font.StringWidth("M");
	r.right = r.left + bold_font.StringWidth(text);
	str = new BStringView(r, "", text);
	str->SetFont(&bold_font);
	AddChild(str);

	r.left = r.right;
	r.right = 32767;
	fOffset2 = new BStringView(r, "", "");
	fOffset2->SetFont(&plain_font);
	AddChild(fOffset2);

	SetBlock(0, true);
}

//--------------------------------------------------------------------

void THeaderView::Draw(BRect where)
{
	BRect sr = Bounds();
	SetHighColor(ui_color(B_SHINE_COLOR));
	StrokeLine(BPoint(sr.left, sr.top), BPoint(sr.left, sr.bottom));
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	if (fIcon) {
		BRect dr;
		sr.Set(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
		dr.Set(ICON_X1, ICON_Y1, ICON_X2, ICON_Y2);
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fIcon, sr, dr);
		SetDrawingMode(B_OP_COPY);
	}
}

//--------------------------------------------------------------------

void THeaderView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_SELECT_ALL:
			((BTextView *)(fPosition->ChildAt(0)))->SelectAll();
			break;

		default:
			BView::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void THeaderView::GetSelection(int32 *start, int32 *finish)
{
	((BTextView *)(fPosition->ChildAt(0)))->GetSelection(start, finish);
}

//--------------------------------------------------------------------

void THeaderView::SetBase(int32 base)
{
	if (fBase != base) {
		fBase = base;
		SetBlock(fBlock, true);
	}
}

//--------------------------------------------------------------------

void THeaderView::SetBlock(off_t block, bool set_total)
{
	char	position[256];
	char	total[256];

	fBlock = block;
	switch (fBase) {
		case B_DECIMAL:
			sprintf(position, "%Ld", fBlock);
			sprintf(total, "of %Ld", fSize);
			break;
		case B_HEX:
			sprintf(position, "0x%Lx", fBlock);
			sprintf(total, "of 0x%Lx", fSize);
			break;
	}
	fPosition->SetText(position);
	((BTextView *)(fPosition->ChildAt(0)))->SelectAll();
	if (set_total)
		fTotal->SetText(total);
}

//--------------------------------------------------------------------

void THeaderView::SetIcon(BBitmap *icon)
{
	fIcon = icon;
	Invalidate();
}

//--------------------------------------------------------------------

void THeaderView::SetInfo(char *name, off_t size, bool reset_block)
{
	fSize = size;
	if (name)
		fName->SetText(name);
	if (reset_block)
		SetBlock(0, true);
}

//--------------------------------------------------------------------

void THeaderView::SetOffset(off_t block, off_t file)
{
	char	offset1[128];
	char	offset2[128];

	switch (fBase) {
		case B_DECIMAL:
			sprintf(offset1, "%Ld", block);
			sprintf(offset2, "%Ld", file);
			break;
		case B_HEX:
			sprintf(offset1, "0x%Lx", block);
			sprintf(offset2, "0x%Lx", file);
			break;
	}
	if (strcmp(offset1, fOffset1->Text()))
		fOffset1->SetText(offset1);
	if (strcmp(offset2, fOffset2->Text()))
		fOffset2->SetText(offset2);
}
