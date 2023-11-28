//--------------------------------------------------------------------
//	
//	Content.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "DiskProbe.h"
#include "ProbeWindow.h"
#include "Content.h"


//====================================================================

TContentView::TContentView(BRect rect, uchar *block, int32 length,
							int32 base, bool read_only, int32 font_size)
			 :BView(rect, "d_content", B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE |
			 										  B_FRAME_EVENTS)
{
	BFont font = *be_fixed_font;
	fBase = base;
	fBlock = block;
	fLastOffset = fLength = length;
	fReadOnly = read_only;
	fFontSize = font_size;
	fField = F_OTHER;
	fOldField = F_HEX;
	fOffset = 0;
	fRange = 0;
	fDirty = false;
	fOnes = false;

	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetFont(&font);
	SetFontSize(fFontSize);
}

//--------------------------------------------------------------------

void TContentView::Draw(BRect where)
{
	char		ascii[BYTES_WIDE + 1];
	char		string[256];
	uchar		*offset;
	float		x;
	int32		columns;
	int32		rows;
	int32		y;
	BFont		font;
	BRect		update;
	font_height	height;

	GetFont(&font);
	x = font.StringWidth("M");
	font.GetHeight(&height);
	y = (int32) (height.ascent + height.descent + height.leading);
	offset = fBlock;

	SetHighColor(ui_color(B_UI_DOCUMENT_TEXT_COLOR));
	for (rows = 0; rows < (fLength + BYTES_WIDE - 1) / BYTES_WIDE; rows++) {
		update.Set(GAP, y * rows, GAP + 4 * x, y * (rows + 1));
		if (update.Intersects(where)) {
			if (fBase == B_HEX)
				sprintf(string, "%.4x", (unsigned int) (rows * BYTES_WIDE));
			else
				sprintf(string, "%.4li", rows * BYTES_WIDE);
			MovePenTo(GAP, y * (rows + 1));
			DrawString(string);
		}
		if (fBlock) {
			for (columns = 0; columns < BYTES_WIDE; columns++) {
				if ((rows * BYTES_WIDE + columns) < fLength) {
					update.Set((GAP * 3) + ((4 + columns * 3) * x),
								y * rows,
								(GAP * 3) + ((4 + columns * 3) * x) + (3 * x),
								y * (rows + 1));
					if (update.Intersects(where)) {
						MovePenTo((GAP * 3) + ((4 + columns * 3) * x),
									y * (rows + 1));
						sprintf(string, "%.2x", *offset);
						DrawString(string);
					}
					if ((*offset >= ' ') && (*offset <= '~'))
						ascii[columns] = *offset;
					else
						ascii[columns] = '.';
					offset++;
				}
				else
					break;
			}
			update.Set((GAP * 5) + ((4 + BYTES_WIDE * 3) * x),
						y * rows,
						(GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + (BYTES_WIDE * x),
						y * (rows + 1));
			if (update.Intersects(where)) {
				MovePenTo((GAP * 5) + ((4 + BYTES_WIDE * 3) * x),
								y * (rows + 1));
				ascii[columns] = 0;
				DrawString(ascii);
			}
		}
	}
	DrawSelection(where, true);
}

//--------------------------------------------------------------------

void TContentView::KeyDown(const char *str, int32 len)
{
	char		key;
	uchar		number;
	int32		mods = 0;
	int32		new_field = F_OTHER;

	if (Window()->CurrentMessage())
		Window()->CurrentMessage()->FindInt32("modifiers", &mods);

	key = str[0];	
	switch (key) {
		case B_TAB:
			if (fField == F_HEX) {
				if (!(mods & B_SHIFT_KEY))
					new_field = F_ASCII;
			}
			else {
				if (mods & B_SHIFT_KEY)
					new_field = F_HEX;
			}
			if (new_field != F_OTHER) {
				DrawSelection(Bounds(), false);
				fField = new_field;
				DrawSelection(Bounds(), true);
			}
			else
				Window()->PostMessage(Window()->CurrentMessage());
			break;

		case B_LEFT_ARROW:
			if (fOffset != 0) {
				DrawSelection(Bounds(), false);
				fOffset--;
				if (fOffset < 0)
					fOffset = 0;
				if (mods & B_SHIFT_KEY)
					fRange++;
				else
					fRange = 0;
				DrawSelection(Bounds(), true);
				Scroll(fOffset);
				fOnes = false;
			}
			else if ((fRange) && (!(mods & B_SHIFT_KEY))) {
				DrawSelection(Bounds(), false);
				fOffset = 0;
				fRange = 0;
				DrawSelection(Bounds(), true);
				Scroll(fOffset);
				fOnes = false;
			}
			break;

		case B_RIGHT_ARROW:
			if ((fOffset + fRange + 1) < fLength) {
				DrawSelection(Bounds(), false);
				if (mods & B_SHIFT_KEY)
					fRange++;
				else {
					fOffset += fRange + 1;
					fRange = 0;
				}
				DrawSelection(Bounds(), true);
				Scroll(fOffset + fRange);
				fOnes = false;
			}
			else if ((fRange) && (!(mods & B_SHIFT_KEY))) {
				DrawSelection(Bounds(), false);
				fOffset = fLength - 1;
				fRange = 0;
				DrawSelection(Bounds(), true);
				Scroll(fOffset + fRange);
				fOnes = false;
			}
			break;

		case B_UP_ARROW:
			if (fOffset > (BYTES_WIDE - 1)) {
				DrawSelection(Bounds(), false);
				fOffset -= BYTES_WIDE;
				if (mods & B_SHIFT_KEY)
					fRange += BYTES_WIDE;
				else
					fRange = 0;
				DrawSelection(Bounds(), true);
				Scroll(fOffset);
				fOnes = false;
			}
			break;

		case B_DOWN_ARROW:
			if ((fOffset + fRange + BYTES_WIDE) < fLength) {
				DrawSelection(Bounds(), false);
				if (mods & B_SHIFT_KEY)
					fRange += BYTES_WIDE;
				else {
					fOffset += fRange + BYTES_WIDE;
					fRange = 0;
				}
				DrawSelection(Bounds(), true);
				Scroll(fOffset + fRange);
				fOnes = false;
			}
			break;

		default:
			if ((fReadOnly) || (!fLength))
				break;
			if (fField == F_ASCII) {
				if (fRange) {
					DrawSelection(Bounds(), false);
					fRange = false;
				}
				fBlock[fOffset] = key;
				Update(fOffset);
				if (fOffset + 1 < fLength) {
					fOffset++;
					Scroll(fOffset);
					fDirty = true;
					fOnes = false;
					Update(fOffset);
				}
			}
			else if (fField == F_HEX) {
				if ((key >= '0') && (key <= '9'))
					number = key - '0';
				else if ((key >= 'A') && (key <= 'F'))
					number = key - 'A' + 10;
				else if ((key >= 'a') && (key <= 'f'))
					number = key -'a' + 10;
				else
					break;
				if (fRange) {
					DrawSelection(Bounds(), false);
					fRange = false;
				}
				if (!fOnes) {
					fBlock[fOffset] = (fBlock[fOffset] & 0xf) | (number << 4);
					fOnes = true;
					Update(fOffset);
				}
				else {
					fBlock[fOffset] = (fBlock[fOffset] & 0xf0) | number;
					fOnes = false;
					Update(fOffset);
					if (fOffset + 1 < fLength) {
						DrawSelection(Bounds(), false);
						fOffset++;
						Scroll(fOffset);
						Update(fOffset);
					}
				}
				fDirty = true;
			}		
	}
}

//--------------------------------------------------------------------

void TContentView::FrameResized(float w, float h)
{
	if (!fFontSize)
		SetFontSize(0);
}

//--------------------------------------------------------------------

void TContentView::MakeFocus(bool focus)
{
	int32		field;
	int32		mods;

	if (!focus) {
		field = fField;
		DrawSelection(Bounds(), false, field);
		fField = F_OTHER;
		DrawSelection(Bounds(), true, field);
	}
	else if (Window()->IsActive()) {
		field = F_HEX;
		if (Window()->CurrentMessage()) {
			Window()->CurrentMessage()->FindInt32("modifiers", &mods);
			if (mods & B_SHIFT_KEY)
				field = F_ASCII;
		}
		DrawSelection(Bounds(), false, field);
		fField = field;
		DrawSelection(Bounds(), true, fField);
	}
	BView::MakeFocus(focus);
}

//--------------------------------------------------------------------

void TContentView::MessageReceived(BMessage *msg)
{
	const char	*text;
	int32		len;
	int32		loop;
	BMessage	*message;

	switch (msg->what) {
		case M_SELECT_ALL:
			if (fRange != (fLength - 1)) {
				DrawSelection(Bounds(), false);
				fOffset = 0;
				fRange = fLength - 1;
				DrawSelection(Bounds(), true);
				fOnes = false;
			}
			break;

		case B_COPY:
			be_clipboard->Lock();
			be_clipboard->Clear();
			message = be_clipboard->Data();
			message->AddData("text/plain", B_MIME_TYPE, &fBlock[fOffset], fRange + 1);
			be_clipboard->Commit();
			be_clipboard->Unlock();
			break;

		case B_PASTE:
			if (!fReadOnly) {
				DrawSelection(Bounds(), false);
				be_clipboard->Lock();
				message = be_clipboard->Data();
				message->FindData("text/plain", B_MIME_TYPE, 
				  (const void**) &text, &len);
				be_clipboard->Unlock();
				if (fOffset + len > fLength - 1)
					len = fLength - fOffset;
				fRange = len - 1;
				for (loop = fOffset; loop < fOffset + len; loop++) {
					fBlock[loop] = text[loop - fOffset];
					Update(loop);
					fDirty = true;
				}
			}
			break;

		default:
			BView::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

void TContentView::MouseDown(BPoint where)
{
	float		x;
	int32		field;
	int32		line_height;
	int32		index;
	int32		last;
	int32		mods = 0;
	int32		start;
	int32		y;
	uint32		buttons;
	BFont		font;
	BPoint		point;
	BRect		r;
	font_height	height;

	if (!fLength)
		return;

	if (!IsFocus())
		MakeFocus(true);
	GetFont(&font);
	x = font.StringWidth("M");
	font.GetHeight(&height);
	y = (int32) (height.ascent + height.descent + height.leading);

	if (Window()->CurrentMessage())
		Window()->CurrentMessage()->FindInt32("modifiers", &mods);

	r.Set((GAP * 3) + (4 * x), 0, (GAP * 3) + ((4 + BYTES_WIDE * 3) * x) - x,
				y * ((fLength + BYTES_WIDE - 1) / BYTES_WIDE + 1));
	if (r.Contains(where)) {
		field = F_HEX;
		x *= 3;
	}
	else {
		r.left = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x);
		r.right = r.left + (BYTES_WIDE * x);
		if (r.Contains(where))
			field = F_ASCII;
		else
			return;
	}

	line_height = LineHeight();
	index = (int32) (((int)((where.y / line_height)) * BYTES_WIDE) + ((where.x 
	  - r.left) / x));
	start = index;
	fOnes = false;
	if ((index == fOffset) && (fField != field)) {
		DrawSelection(Bounds(), false);
		fField = field;
		DrawSelection(Bounds(), true);
	}
	else if (index != fOffset) {
		DrawSelection(Bounds(), false);
		if (mods & B_SHIFT_KEY) {
			if (index < fOffset) {
				fRange += fOffset - index;
				fOffset = index;
			}
			else {
				fRange = index - fOffset;
				start = fOffset;
			}
		}
		else {
			fOffset = index;
			fRange = 0;
		}
		fField = field;
		DrawSelection(Bounds(), true);
	}
	else if (fRange) {
		DrawSelection(Bounds(), false);
		fRange = 0;
		DrawSelection(Bounds(), true);
	}

	last = index;
	for (;;) {
		snooze(100000);
		GetMouse(&point, &buttons);
		if (!buttons)
			break;
		if (point.x < r.left)
			point.x = r.left;
		else if (point.x > r.right)
			point.x = r.right;
		index = (int32) (((int)((point.y / line_height)) * BYTES_WIDE) + 
		  ((point.x - r.left) / x));
		if (index < 0)
			index = 0;
		if (index >= fLength)
			index = fLength - 1;
		if (last != index) {
			DrawSelection(Bounds(), false);
			if (index < start) {
				fRange = start - index;
				fOffset = index;
			}
			else {
				fRange = index - start;
				fOffset = start;
			}
			DrawSelection(Bounds(), true);
			if (index < start)
				Scroll(fOffset);
			else
				Scroll(fOffset + fRange);
			last = index;
		}
	}
}

//--------------------------------------------------------------------

void TContentView::Activated(bool active)
{
	if (active) {
		if (fOldField != F_OTHER) {
			DrawSelection(Bounds(), false, fOldField);
			fField = fOldField;
			DrawSelection(Bounds(), true, fOldField);
		}
		else
			fField = fOldField;
	}
	else {
		fOldField = fField;
		if (fField != F_OTHER) {
			DrawSelection(Bounds(), false, fOldField);
			fField = F_OTHER;
			DrawSelection(Bounds(), true, fOldField);
		}
	}
}

//--------------------------------------------------------------------

bool TContentView::Dirty(void)
{
	return fDirty;
}

//--------------------------------------------------------------------

void TContentView::DrawSelection(BRect where, bool select, int32 field)
{
	float		x;
	int32		e_col;
	int32		e_row;
	int32		s_col;
	int32		s_row;
	int32		y;
	BFont		font;
	BRect		a_r;
	BRect		h_r;
	BRegion		*ascii;
	BRegion		*hex;
	font_height	height;

	if (!fLength)
		return;

	hex = new BRegion();
	ascii = new BRegion();
	GetFont(&font);
	x = (float) font.StringWidth("M");
	font.GetHeight(&height);
	y = (int32) (height.ascent + height.descent + height.leading);
	s_row = fOffset / BYTES_WIDE;
	s_col = fOffset % BYTES_WIDE;
	e_row = (fOffset + fRange) / BYTES_WIDE;
	e_col = (fOffset + fRange) % BYTES_WIDE;

	h_r.left = (GAP * 3) + ((4 + s_col * 3) * x) - (x / 2);
	a_r.left = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + (s_col * x);
	h_r.top = y * s_row + (height.descent / 2) + 2;
	a_r.top = h_r.top;
	if (s_row == e_row) {
		h_r.right = (GAP * 3) + ((4 + e_col * 3) * x) + (3 * x) - (x / 2);
		a_r.right = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + ((e_col + 1) * x);
	}
	else {
		h_r.right = (GAP * 3) + ((4 + (BYTES_WIDE - 1) * 3) * x) + (3 * x) - (x / 2);
		a_r.right = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + (BYTES_WIDE * x);
	}
	h_r.bottom = y * (s_row + 1) + (height.descent / 2) + 1;
	a_r.bottom = h_r.bottom;
	hex->Include(h_r);
	ascii->Include(a_r);
	s_row++;

	if (s_row < e_row) {
		h_r.left = (GAP * 3) + (4 * x) - (x / 2);
		a_r.left = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x);
		h_r.top = y * s_row + (height.descent / 2) + 2;
		a_r.top = h_r.top;
		h_r.right = (GAP * 3) + ((4 + (BYTES_WIDE - 1) * 3) * x) + (3 * x) - (x / 2);
		a_r.right = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + (BYTES_WIDE * x);
		h_r.bottom = y * e_row + (height.descent / 2) + 1;
		a_r.bottom = h_r.bottom;
		hex->Include(h_r);
		ascii->Include(a_r);
		s_row = e_row;
	}

	if (s_row == e_row) {
		h_r.left = (GAP * 3) + (4 * x) - (x / 2);
		a_r.left = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x);
		h_r.top = y * s_row + (height.descent / 2) + 2;
		a_r.top = h_r.top;
		h_r.right = (GAP * 3) + ((4 + e_col * 3) * x) + (3 * x) - (x / 2);
		a_r.right = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + ((e_col + 1) * x);
		h_r.bottom = y * (e_row + 1) + (height.descent / 2) + 1;
		a_r.bottom = h_r.bottom;
		hex->Include(h_r);
		ascii->Include(a_r);
	}

	if (select)		SetHighColor(ui_color(B_UI_DOCUMENT_TEXT_COLOR));
	else			SetHighColor(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR));

	if (fField == F_HEX) {
		if ((field == F_HEX) || (field == F_ALL))
			FillRegion(hex);
		if ((field == F_ASCII) || (field == F_ALL))
			StrokeRegion(ascii);
	}
	else if (fField == F_ASCII) {
		if ((field == F_HEX) || (field == F_ALL))
			StrokeRegion(hex);
		if ((field == F_ASCII) || (field == F_ALL))
			FillRegion(ascii);
	}
	else {
		if ((field == F_HEX) || (field == F_ALL))
			StrokeRegion(hex);
		if ((field == F_ASCII) || (field == F_ALL))
			StrokeRegion(ascii);
	}
	SetHighColor(ui_color(B_UI_DOCUMENT_TEXT_COLOR));
	delete hex;
	delete ascii;

	if ((select) && (fLastOffset != fOffset)) {
		fLastOffset = fOffset;
		((TProbeWindow *)Window())->SetOffset(fOffset);
	}
}

//--------------------------------------------------------------------

void TContentView::FillRegion(BRegion *region)
{
	BRect		r;

	r = region->RectAt(0);
	InvertRect(r);
	if (region->CountRects() == 1)
		return;

	r = region->RectAt(1);
	InvertRect(r);
	if (region->CountRects() == 2)
		return;

	r = region->RectAt(2);
	InvertRect(r);
}

//--------------------------------------------------------------------

void TContentView::GetSelection(int32 *start, int32 *finish)
{
	*start = fOffset;
	*finish = fOffset + fRange;
}

//--------------------------------------------------------------------

void TContentView::GetSize(int32 *x, int32 *y)
{
	float		width;
	int32		length;
	BFont		font;

	GetFont(&font);
	width = (float) font.StringWidth("M");
	*x = (int32)((GAP * 6) + ((4 + (BYTES_WIDE * 3) + BYTES_WIDE) * width));

	length = max_c(512, fLength);
	*y = (int32)(LineHeight() * ((length + BYTES_WIDE - 1) / BYTES_WIDE) + 7);
}

//--------------------------------------------------------------------

int32 TContentView::LineHeight(void)
{
	BFont		font;
	font_height	height;

	GetFont(&font);
	font.GetHeight(&height);
	return((int)(height.ascent + height.descent + height.leading));
}

//--------------------------------------------------------------------

void TContentView::Scroll(int32 offset)
{
	int32		height;
	int32		row;
	float		top, bottom;
	BScrollBar	*scroll;

	scroll = ((BScrollView *)Parent())->ScrollBar(B_VERTICAL);
	height = LineHeight();
	row = (offset / BYTES_WIDE) * height;
	top = scroll->Value();
	bottom = top + Bounds().Height();
	if (row < top)
		scroll->SetValue(row);
	else if ((row + height) > bottom)
		scroll->SetValue(scroll->Value() + (row + height) - bottom + 4);
}

//--------------------------------------------------------------------

void TContentView::SetBase(int32 base)
{
	BFont		font;
	BRect		r;
	BRegion		new_clip;

	if (fBase != base) {
		fBase = base;
		GetFont(&font);
		r = Bounds();
		r.left += GAP;
		r.right = r.left + (4 * font.StringWidth("M"));
		SetHighColor(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR));
		FillRect(r);
		SetHighColor(ui_color(B_UI_DOCUMENT_TEXT_COLOR));

		new_clip.Include(r);
		PushState();
		ConstrainClippingRegion(&new_clip);
		Draw(r);
		PopState();
		((TProbeWindow *)Window())->SetOffset(fOffset);
	}
}

//--------------------------------------------------------------------

void TContentView::SetBlock(uchar *block, int32 len, bool read_only,
				bool dirty)
{
	fBlock = block;
	fLastOffset = fLength = len;
	fReadOnly = read_only;
	fOffset = 0;
	fRange = 0;
	fDirty = dirty;
	fOnes = false;
	SetHighColor(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR));
	FillRect(Bounds());
	Draw(Bounds());
}

//--------------------------------------------------------------------

void TContentView::SetDirty(bool dirty)
{
	fDirty = dirty;
}


//--------------------------------------------------------------------

void TContentView::SetFontSize(int32 size)
{
	int32		bytes;
	int32		loop;
	int32		width;
	int32		x;
	float		old;
	BFont		font;

	GetFont(&font);
	fFontSize = size;
	if (!fFontSize) {
		old = (float) font.Size();
		width = (int32) Bounds().Width();
		bytes = 4 + (BYTES_WIDE * 3) + BYTES_WIDE;
		for (loop = (int32)(width / bytes); ; loop++) {
			font.SetSize(loop);
			x = (int32)((GAP * 6) + (bytes * font.StringWidth("M")));
			if (x > width) {
				if (old == loop - 1)
					return;
				font.SetSize(loop - 1);
				SetFont(&font);
				break;
			}
		}
	}
	else {
		font.SetSize(fFontSize);
		SetFont(&font);
	}
	Invalidate(Bounds());
}

//--------------------------------------------------------------------

void TContentView::SetSelection(int32 start, int32 finish)
{
	if ((start >= 0) && (start < fLength)) {
		DrawSelection(Bounds(), false);
		fOffset = start;
		if (fOffset + (finish - start) < fLength)
			fRange = finish - start;
		else
			fRange = (fLength - start - 1);
		DrawSelection(Bounds(), true);
		fOnes = false;
	}
}

//--------------------------------------------------------------------

void TContentView::StrokeRegion(BRegion *region)
{
	float		tmp1;
	float		tmp2;
	BRect		r;

	r = region->RectAt(0);
	StrokeLine(r.LeftBottom(), r.LeftTop());
	StrokeLine(r.LeftTop(), r.RightTop());
	StrokeLine(r.RightTop(), r.RightBottom());
	if (region->CountRects() == 1) {
		StrokeLine(r.LeftBottom(), r.RightBottom());
		return;
	}
	tmp1 = r.left;
	tmp2 = r.right;

	r = region->RectAt(1);
	if (r.right > tmp1)
		StrokeLine(r.LeftTop(), BPoint(tmp1, r.top));
	else
		StrokeLine(r.LeftTop(), r.RightTop());
	StrokeLine(r.LeftTop(), r.LeftBottom());
	StrokeLine(r.RightTop(), r.RightBottom());
	if (region->CountRects() == 2) {
		StrokeLine(r.LeftBottom(), r.RightBottom());
		if (r.right > tmp1)
			StrokeLine(BPoint(r.right, r.top), BPoint(tmp2, r.top));
		else
			StrokeLine(BPoint(tmp1, r.top), BPoint(tmp2, r.top));
		return;
	}
	tmp1 = r.right;

	r = region->RectAt(2);
	StrokeLine(r.LeftTop(), r.LeftBottom());
	StrokeLine(r.LeftBottom(), r.RightBottom());
	StrokeLine(r.RightBottom(), BPoint(r.right, r.top));
	StrokeLine(BPoint(r.right, r.top), BPoint(tmp1, r.top));
}

//--------------------------------------------------------------------

void TContentView::Update(int32 index)
{
	float		x;
	int32		col;
	int32		row;
	int32		y;
	BFont		font;
	BRect		a_r;
	BRect		h_r;
	font_height	height;

	GetFont(&font);
	x = (int32) font.StringWidth("M");
	font.GetHeight(&height);
	y = (int32) (height.ascent + height.descent + height.leading);
	row = index / BYTES_WIDE;
	col = index % BYTES_WIDE;
	
	h_r.left = (GAP * 3) + ((4 + col * 3) * x) - (x / 2);
	a_r.left = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + (col * x);
	h_r.top = y * row + (height.descent / 2) + 2;
	a_r.top = h_r.top;
	h_r.right = (GAP * 3) + ((4 + col * 3) * x) + (3 * x) - (x / 2);
	a_r.right = (GAP * 5) + ((4 + BYTES_WIDE * 3) * x) + ((col + 1) * x);
	h_r.bottom = y * (row + 1) + (height.descent / 2) + 2;
	a_r.bottom = h_r.bottom;

	SetHighColor(ui_color(B_UI_DOCUMENT_BACKGROUND_COLOR));
	FillRect(h_r);
	FillRect(a_r);
	Invalidate(h_r);
	Invalidate(a_r);
	SetHighColor(ui_color(B_UI_DOCUMENT_TEXT_COLOR));
}
