#include "bm_view.h"
#include <stdio.h>
#include <Region.h>
#include "picker_view.h"
#include "sample_view.h"
#include "select_view.h"
#include "twindow.h"
#include "main.h"
#include <string.h>
#include <malloc.h>
#include <Debug.h>
#include <ByteOrder.h>

#define	K_C	179

#define OFFSET_H  4
#define OFFSET_V  18

#define	H_DELTA		(8*NN+20)

BMView::BMView(BRect rect, char *name)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	int      i, j;
	BRect	 r;

	r.top = 0;
	r.left = 0;
	r.bottom = (NN*8)-1;
	r.right = (NN*8)-1;
	
	the_off = new BBitmap(r, B_COLOR_8_BIT, TRUE);
	drawer = new BView(r, "off_view", B_FOLLOW_ALL, B_WILL_DRAW);
	the_off->Lock();
	the_off->AddChild(drawer);
	the_off->Unlock();
	
	witness = new BBitmap(r, B_COLOR_8_BIT, TRUE);
	witness_drawer = new BView(r, "witness", B_FOLLOW_ALL, B_WILL_DRAW);
	witness->Lock();
	witness->AddChild(witness_drawer);
	witness->Unlock();
	
	fm = new FontMachine();

	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			select_copy[i][j] = 0xff;

	Reset();
}

BMView::~BMView() {
	delete fm;
}

void BMView::ResetSelection() {
	int      i, j;
	BRect    r;

	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			select[i][j] = 0xff;
	r.top = 0;
	r.left = 0;	
	r.bottom = 1000;
	r.right = 1000;
	Draw(r);
}

void BMView::PasteSelection() {
	int      i, j;

	fm->fdirty = true;

	memcpy(undo_block, data, sizeof(data));
	memcpy(&undo_header, &char_header, CHAR_HEADER_SIZE);
	memcpy(undo_select, select, NN*NN);

	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			if (select[i][j] != 0xff)
				data[i][j] = select[i][j]&0x0f;
	ResetSelection();
}

void BMView::Copy() {
	memcpy(select_copy, select, NN*NN);
	Draw(BRect(0,0,1000,1000));
}

void BMView::Cut() {
	int      i, j;

	fm->fdirty = true;

	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			if (select[i][j] != 0xff)
				data[i][j] = 0;
	memcpy(select_copy, select, NN*NN);
	ResetSelection();
}

void BMView::Paste() {
	fm->fdirty = true;
	memcpy(select, select_copy, NN*NN);
	Draw(BRect(0,0,1000,1000));
}

void BMView::Reset() {
	cur_char = 0;
	ConvertToBitmap(fm->GetCharBits(0), (uchar*)data, &char_header);
	ConvertToBitmap(fm->GetCharBits(0), (uchar*)undo_block, &undo_header);
	ResetSelection();
}

//---------------------------------------------------------

void BMView::ConvertToBitmap(fc_char *ch, uchar *data, fc_char *header) {
	int       i, j, dx, val;
	
	for (i=0; i<NN*NN; i++)
		data[i] = 0;
	if (ch != 0) {
		memcpy(header, ch, CHAR_HEADER_SIZE);
		header->bbox.right++;
		header->bbox.bottom++;
		dx = (ch->bbox.right-ch->bbox.left+2)>>1;
		for (i=ch->bbox.left; i<=ch->bbox.right; i++)
			for (j=ch->bbox.top; j<=ch->bbox.bottom; j++) {
				val = ch->bitmap[(j-ch->bbox.top)*dx+((i-ch->bbox.left)>>1)];
				val = (val>>(((i-ch->bbox.left+1)&1)<<2))&7;
				data[i+OFFSET_H+NN*(OFFSET_V+j)] = val;
			}
	}
	else {
		header->bbox.left = 0;
		header->bbox.right = 1;
		header->bbox.top = -1;
		header->bbox.bottom = 0;
	}
}

void BMView::PutBackChar() {
	int       i, j, val, dx, dy, size;
	fc_char   *ch;

	dx = char_header.bbox.right-char_header.bbox.left;
	dy = char_header.bbox.bottom-char_header.bbox.top;
	size = ((dx+1)>>1)*dy;
	if (size < 0) size = 0;
	ch = (fc_char*)malloc(size+CHAR_HEADER_SIZE);
	memset(ch, 0, size+CHAR_HEADER_SIZE);
	memcpy(ch, &char_header, CHAR_HEADER_SIZE);
	ch->bbox.right--;
	ch->bbox.bottom--;
	dx = (dx+1)>>1;
	for (i=ch->bbox.left; i<=ch->bbox.right; i++)
		for (j=ch->bbox.top; j<=ch->bbox.bottom; j++) {
			val = data[OFFSET_V+j][i+OFFSET_H];
			val <<= (((i-ch->bbox.left+1)&1)<<2);
			ch->bitmap[(j-ch->bbox.top)*dx+((i-ch->bbox.left)>>1)] |= val;
		}	
	fm->SetCharBits(cur_char, ch);
	free(ch);
}

//---------------------------------------------------------

void BMView::AttachedToWindow() {
}

//---------------------------------------------------------

static uchar color_table[8] = { 63, 27, 22, 18, 13, 9, 4, 0 };

void	BMView::RenderWitness(int32 index_char)
{
	BFont		font(&((TApplication*)be_app)->aWindow->SelectFont->font);
	uint8		str_code[7];
	uint8		*str;
	uint16		*uni_ptr;
	uint16		uni_str;
	uint32		*list_code;

	uni_str = (((TApplication*)be_app)->bm_view->fm->list_code)[index_char];
	uni_ptr = &uni_str;
	str = (uchar*)&str_code;
	convert_to_utf8(str, uni_ptr);
	str[0] = 0;
	render(BRect(0,0,8*NN-1,8*NN-1), witness, true);
	witness->Lock();
	font.SetSize(font.Size() * 8.0);
	witness_drawer->SetFont(&font);
	witness_drawer->DrawString((char*)str_code, BPoint(4*8, (NN-6)*8));
	witness_drawer->Sync();
	witness->Unlock();
}

void	BMView::render(BRect r, BBitmap *the_off, bool background_only)
{
	int     i;
	long    xmin, xmax,ymin, ymax;
	long	x,y;
	long	dy;
	char	*p;
	uchar	color;
	ulong	tmp_long[6];

	ymin = r.top/8;
	ymax = (r.bottom+7)/8;
	xmin = r.left/8;
	xmax = (r.right+7)/8;

	if (xmin < 0) xmin = 0;
	if (xmax > NN) xmax = NN;
	if (ymin < 0) ymin = 0;
	if (ymax > NN) ymax = NN;	
	
	for (y = ymin; y < ymax; y++)
	for (x = xmin; x < xmax; x++) {
		if (background_only)
			color = color_table[0];
		else if (select[y][x] != 0xff)
			color = color_table[select[y][x]&0x0f];
		else
			color = color_table[data[y][x]];
		tmp_long[0] = (color<<24) | (color << 16) | (color << 8) | color;
		tmp_long[1] = (tmp_long[0] & B_BENDIAN_TO_HOST_INT32(0xffffff00))
						| B_BENDIAN_TO_HOST_INT32(0x16);
		tmp_long[2] = tmp_long[0];
		tmp_long[3] = tmp_long[1];
		tmp_long[4] = tmp_long[0];
		tmp_long[5] = tmp_long[1];
		
		if ((select[y][x] != 0xff) && !background_only) {
			if ((x == 0) || (select[y][x-1] == 0xff)) {
				tmp_long[0] = (tmp_long[0] & B_BENDIAN_TO_HOST_INT32(0x00ffffff))
								| B_BENDIAN_TO_HOST_INT32(0x36000000);
				tmp_long[2] = (tmp_long[2] & B_BENDIAN_TO_HOST_INT32(0x00ffffff))
								| B_BENDIAN_TO_HOST_INT32(0x36000000);
				tmp_long[4] = (tmp_long[4] & B_BENDIAN_TO_HOST_INT32(0x00ffffff))
								| B_BENDIAN_TO_HOST_INT32(0x36000000);
			}
			if ((y == 0) || (select[y-1][x] == 0xff)) {
				tmp_long[0] = 0x36363636;
				tmp_long[1] = B_BENDIAN_TO_HOST_INT32(0x36363616);
			}
			if ((x == (NN-1)) || (select[y][x+1] == 0xff)) {
				tmp_long[1] = (tmp_long[1] & B_BENDIAN_TO_HOST_INT32(0xffff00ff))
								| B_BENDIAN_TO_HOST_INT32(0x00003600);
				tmp_long[3] = (tmp_long[3] & B_BENDIAN_TO_HOST_INT32(0xffff00ff))
								| B_BENDIAN_TO_HOST_INT32(0x00003600);
				tmp_long[5] = (tmp_long[5] & B_BENDIAN_TO_HOST_INT32(0xffff00ff))
								| B_BENDIAN_TO_HOST_INT32(0x00003600);
			}
			if ((y == (NN-1)) || (select[y+1][x] == 0xff)) {
				tmp_long[4] = 0x36363636;
				tmp_long[5] = B_BENDIAN_TO_HOST_INT32(0x36363616);
			}
		}

		p = (char *)the_off->Bits();
		p = p+(x*8)+(y*NN*8*8);
		
		*((ulong *)p) = tmp_long[0];
		*((ulong *)(p+4)) = tmp_long[1];
		p += NN*8;

		for (i=0; i<5; i++) {
			*((ulong *)p) = tmp_long[2];
			*((ulong *)(p+4)) = tmp_long[3];
			p += NN*8;
		}

		*((ulong *)p) = tmp_long[4];
		*((ulong *)(p+4)) = tmp_long[5];
		p += NN*8;

		*((ulong *)p) = 0x16161616;
		*((ulong *)(p+4)) = 0x16161616;
	}
}

//---------------------------------------------------------

void BMView::drawBottomRule(BRect r) {
	long	h1;
	long	v1;
	BRect	r1;

	h1 = (OFFSET_H+char_header.bbox.left)*8 + H_DELTA;
	v1 = (NN*8)+3;

	if (r.bottom >= v1) {
		r1.top = NN*8+1;
		r1.bottom = NN*8 + 15;

		r1.left = 0+H_DELTA;
		r1.right = 8*OFFSET_H-1+H_DELTA;
		SetHighColor(255,255,255);
		FillRect(r1);
		
		r1.left = 8*OFFSET_H+H_DELTA;
		r1.right = (int)(8.0*(OFFSET_H+char_header.escape.x_escape-0.06))+H_DELTA;
		SetHighColor(170,170,170);
		FillRect(r1);		
		
		r1.left = r1.right+1;
		r1.right = NN*8+H_DELTA;
		SetHighColor(255,255,255);
		FillRect(r1);

		SetHighColor(0,0,0);
		FillTriangle(BPoint(h1,v1), BPoint(h1+4, v1+4), BPoint(h1-4,v1+4));
	}
	if (r.right >= (h1-1) && r.left <= (h1+1)) {
		SetHighColor(130,0,0);
		MovePenTo(BPoint(h1-1,NN*8-1));
		StrokeLine(BPoint(h1-1, 1));
	}

	h1 = (OFFSET_H+char_header.bbox.right)*8 + H_DELTA;
	v1 = (NN*8)+3;

	if (r.bottom >= v1) {
		SetHighColor(0,0,0);
		FillTriangle(BPoint(h1,v1), BPoint(h1+4, v1+4), BPoint(h1-4,v1+4));
	}
	if (r.right >= (h1-1) && r.left <= (h1+1)) {
		SetHighColor(130,0,0);
		MovePenTo(BPoint(h1-1,NN*8-1));
		StrokeLine(BPoint(h1-1, 1));
	}
}

//---------------------------------------------------------

void BMView::drawRightRule(BRect r) {
	long	h1;
	long	v1;
	BRect	r1;

	h1 = (NN*8)+3 + H_DELTA;
	v1 = OFFSET_V*8;
	
	if (r.bottom >= (v1-1) && r.top <= (v1+1)) {
		SetHighColor(100,100,100);
		MovePenTo(BPoint(NN*8-1+H_DELTA,v1-1));
		StrokeLine(BPoint(1+H_DELTA, v1-1));
	}

	h1 = (NN*8)+3 + H_DELTA;
	v1 = (OFFSET_V+char_header.bbox.bottom)*8;
	
	if (r.right >= h1) {
		r1.left = NN*8+1+H_DELTA;
		r1.right = NN*8 + 15+H_DELTA;
		r1.top = 0;
		r1.bottom = NN*8;
		SetHighColor(255,255,255);
		FillRect(r1);
		SetHighColor(0,0,0);
		FillTriangle(BPoint(h1,v1), BPoint(h1+4, v1-4), BPoint(h1+4,v1+4));
	}
	if (r.bottom >= (v1-1) && r.top <= (v1+1)) {
		SetHighColor(0,0,130);
		MovePenTo(BPoint(NN*8-1+H_DELTA,v1-1));
		StrokeLine(BPoint(1+H_DELTA, v1-1));
	}

	h1 = (NN*8)+3 + H_DELTA;
	v1 = (OFFSET_V+char_header.bbox.top)*8;
	
	if (r.right >= h1) {
		SetHighColor(0,0,0);
		FillTriangle(BPoint(h1,v1), BPoint(h1+4, v1-4), BPoint(h1+4,v1+4));
	}
	if (r.bottom >= (v1-1) && r.top <= (v1+1)) {
		SetHighColor(0,0,130);
		MovePenTo(BPoint(NN*8-1+H_DELTA,v1-1));
		StrokeLine(BPoint(1+H_DELTA, v1-1));
	}
}

//---------------------------------------------------------

void BMView::Draw(BRect r) {
	BRect		r2;

	DrawBitmap(witness, r, r);
	r2 = r;
	r2.OffsetBy(-H_DELTA, 0);
	render(r2, the_off);
	DrawBitmap(the_off, r2, r);
	drawRightRule(r);
	drawBottomRule(r);
}

//---------------------------------------------------------

void	BMView::track_vertical(BPoint where)
{
	BRect		r;
	long		h0;
	long		h1;
	long		v1;
	ulong		buttons;

	h0 = (OFFSET_H+char_header.bbox.left)*8;
	if (where.x > (h0-4) && (where.x < (h0+4))) {
		do {
			h0 = OFFSET_H+char_header.bbox.left;
			h1 = (where.x+4) / 8;
			if (h1 < 0) h1 = 0;
			if (h1 >= (OFFSET_H+char_header.bbox.right))
				h1 = OFFSET_H+char_header.bbox.right-1;
			if (h1 != h0) {
				char_header.bbox.left = h1-OFFSET_H;			
				if (h0 < h1)
					r.Set(8*h0-4+H_DELTA, 0, 8*h1+4+H_DELTA, 8*(NN+2));
				else
					r.Set(8*h1-4+H_DELTA, 0, 8*h0+4+H_DELTA, 8*(NN+2));
				Draw(r);
				RefreshChange();
			}
			snooze(25000);
			GetMouse(&where, &buttons);
			where.x -= H_DELTA;
		} while(buttons);
		return;
	}

	h0 = (OFFSET_H+char_header.bbox.right)*8;
	if (where.x > (h0-4) && (where.x < (h0+4))) {
		do {
			h0 = OFFSET_H+char_header.bbox.right;
			h1 = (where.x+4) / 8;
			if (h1 >= NN) h1 = (NN-1);
			if (h1 <= (OFFSET_H+char_header.bbox.left))
				h1 = OFFSET_H+char_header.bbox.left+1;
			if (h1 != h0) {
				char_header.bbox.right = h1-OFFSET_H;			
				if (h0 < h1)
					r.Set(8*h0-4+H_DELTA, 0, 8*h1+4+H_DELTA, 8*(NN+2));
				else
					r.Set(8*h1-4+H_DELTA, 0, 8*h0+4+H_DELTA, 8*(NN+2));
				Draw(r);
				RefreshChange();
			}
			snooze(25000);
			GetMouse(&where, &buttons);
			where.x -= H_DELTA;
		} while(buttons);
	}
}

//---------------------------------------------------------

void	BMView::track_horizontal(BPoint where)
{
	BRect		r;
	long		h0;
	long		h1;
	long		v1;
	ulong		buttons;

	h0 = (OFFSET_V+char_header.bbox.top)*8;
	if (where.y > (h0-4) && (where.y < (h0+4))) {
		do {
			h0 = OFFSET_V+char_header.bbox.top;
			h1 = (where.y+4) / 8;
			if (h1 < 0) h1 = 0;
			if (h1 >= (OFFSET_V+char_header.bbox.bottom))
				h1 = OFFSET_V+char_header.bbox.bottom-1;
			if (h1 != h0) {
				char_header.bbox.top = h1-OFFSET_V;			
				if (h0 < h1)
					r.Set(0+H_DELTA, 8*h0-4, 8*(NN+2)+H_DELTA, 8*h1+4);
				else
					r.Set(0+H_DELTA, 8*h1-4, 8*(NN+2)+H_DELTA, 8*h0+4);
				Draw(r);
				RefreshChange();
			}
			snooze(25000);
			GetMouse(&where, &buttons);
			where.x -= H_DELTA;
		} while(buttons);
		return;
	}

	h0 = (OFFSET_V+char_header.bbox.bottom)*8;
	if (where.y > (h0-4) && (where.y < (h0+4))) {
		do {
			h0 = OFFSET_V+char_header.bbox.bottom;
			h1 = (where.y+4) / 8;
			if (h1 >= NN) h1 = (NN-1);
			if (h1 <= (OFFSET_V+char_header.bbox.top))
				h1 = OFFSET_V+char_header.bbox.top+1;
			if (h1 != h0) {
				char_header.bbox.bottom = h1-OFFSET_V;			
				if (h0 < h1)
					r.Set(0+H_DELTA, 8*h0-4, 8*(NN+2)+H_DELTA, 8*h1+4);
				else
					r.Set(0+H_DELTA, 8*h1-4, 8*(NN+2)+H_DELTA, 8*h0+4);
				Draw(r);
				RefreshChange();
			}
			snooze(25000);
			GetMouse(&where, &buttons);
			where.x -= H_DELTA;
		} while(buttons);
	}
}

//---------------------------------------------------------

void BMView::invert_select(int hmin, int hmax, int vmin, int vmax)
{
	int     i, j, h, v;

	fm->fdirty = true;

	if (hmin > hmax) {
		h = hmin;
		hmin = hmax;
		hmax = h;
	}
	if (vmin > vmax) {
		v = vmin;
		vmin = vmax;
		vmax = v;
	}
	for (i=hmin; i<=hmax; i++)
		for (j=vmin; j<=vmax; j++) {
			if (select[j][i] == 0xff)
				select[j][i] = data[j][i];
			else if (select[j][i] < 0x0f)
				select[j][i] = 0xff;
		}
}

//---------------------------------------------------------

void BMView::do_selection(BPoint where)
{
	int         i, j, h0, v0, h1, v1, h2, v2;
	BRect		r;
	ulong		buttons;
	
	h1 = h0 = where.x / 8;
	v1 = v0 = where.y / 8;
	invert_select(h0, h1, v0, v1);
	
	do {
		h2 = where.x / 8;
		v2 = where.y / 8;
		
		if ((h2 != h1) || (v2 != v1)) {
			invert_select(h0, h1, v0, v1);
			invert_select(h0, h2, v0, v2);

			if ((h0 < h1) && (h0 < h2))
				r.left = h0*8;
			else if (h1 < h2)
				r.left = h1*8;
			else
				r.left = h2*8;
			
			if ((h0 > h1) && (h0 > h2))
				r.right = h0*8;
			else if (h1 > h2)
				r.right = h1*8;
			else
				r.right = h2*8;
			
			if ((v0 < v1) && (v0 < v2))
				r.top = v0*8;
			else if (v1 < v2)
				r.top = v1*8;
			else
				r.top = v2*8;
			
			if ((v0 > v1) && (v0 > v2))
				r.bottom = v0*8;
			else if (v1 > v2)
				r.bottom = v1*8;
			else
				r.bottom = v2*8;

			r.bottom += 7;
			r.right += 7;
			
			r.left += H_DELTA;
			r.right += H_DELTA;
			Draw(r);
			RefreshChange();

			h1 = h2;
			v1 = v2;
		}

		snooze(25000);
		GetMouse(&where, &buttons);
		where.x -= H_DELTA;
	} while(buttons);
	
	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			select[i][j] |= 0x10;
}

//---------------------------------------------------------

void    BMView::InvertH()
{
	int       i, j;
	uchar     copy[NN][NN];
	
	fm->fdirty = true;

	memcpy(copy, select, NN*NN);
	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			select[j][i] = copy[j][NN-1-i];
	Draw(BRect(0,0,1000,1000));
}

//---------------------------------------------------------

void    BMView::InvertV()
{
	int       i, j;
	uchar     copy[NN][NN];

	fm->fdirty = true;

	memcpy(copy, select, NN*NN);
	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			select[j][i] = copy[NN-1-j][i];
	Draw(BRect(0,0,1000,1000));
}

//---------------------------------------------------------

void    BMView::offset_selection(int dh, int dv)
{
	int       i, j;
	uchar     copy[NN][NN];

	fm->fdirty = true;

	memcpy(copy, select, NN*NN);
	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			select[j][i] = copy[(j-dv+NN)%NN][(i-dh+NN)%NN];
	Draw(BRect(0,0,1000,1000));
}

//---------------------------------------------------------

void	BMView::MouseDown(BPoint where)
{
	long		color;
	char		c;
	PickView	*p;
	ulong		buttons;
	long		x,y;
	BRect		r;
	long		x0,y0;
	key_info    k_info;

	where.x -= H_DELTA;

	if (where.y > (NN*8)) {
		track_vertical(where);
		return;
	}
	if (where.x > (NN*8)) {
		track_horizontal(where);
		return;
	}

	get_key_info(&k_info);
	if (k_info.modifiers & B_SHIFT_KEY) {
		do_selection(where);
		return;
	}

	PasteSelection();
	
	memcpy(undo_block, data, sizeof(data));
	memcpy(&undo_header, &char_header, CHAR_HEADER_SIZE);
	memcpy(undo_select, select, NN*NN);
	x0 = -10;
	y0 = -10;

	p = (PickView *)(Window()->FindView("picker"));
	color = p->cur;
	c = color;

	do {
		x = where.x/8;
		y = where.y/8;
		if (x>(NN-1) && y>(NN-1))
			goto skip;
		if (x < 0 || y < 0)
			goto skip;
	
		if (x == x0 && y == y0)
			goto skip;
	
		x0 = x;
		y0 = y;

		fm->fdirty = true;
		data[y][x] = c;
		r.top = (y*8);
		r.bottom = r.top+9;
		r.left = (x*8);
		r.right = r.left+9;
		Draw(Bounds());
		RefreshChange();
skip:;
		snooze(10000);
		GetMouse(&where, &buttons);
		where.x -= H_DELTA;
	} while(buttons);

	RefreshChange();
}

//---------------------------------------------------------

void	BMView::change_select(long new_one)
{
	BRect	r;
	long	width;
	long	height;
	long	ascent;

	PutBackChar();
	ResetSelection();
	cur_char = new_one;
	ConvertToBitmap(fm->GetCharBits(new_one), (uchar*)data, &char_header);
	ConvertToBitmap(fm->GetCharBits(new_one), (uchar*)undo_block, &undo_header);
	RenderWitness(cur_char);
	r.top = 0;
	r.left = 0;	
	r.bottom = 1000;
	r.right = 1000;
	Draw(r);
}

//---------------------------------------------------------

void BMView::KeyDown(const char *bytes, int32 numBytes)
{
	int         i, j;
	ulong       key;
	SelectView	*s;

	key = bytes[0];

	for (i=0; i<NN; i++)
		for (j=0; j<NN; j++)
			if (select[i][j] != 0xff) {
				if (key == B_LEFT_ARROW)
					offset_selection(-1, 0);
				else if (key == B_RIGHT_ARROW)
					offset_selection(1, 0);
				else if (key == B_DOWN_ARROW)
					offset_selection(0, 1);
				else if (key == B_UP_ARROW)
					offset_selection(0, -1);
				else if ((key == B_DELETE) || (key == B_BACKSPACE)) {
					fm->fdirty = true;

					memcpy(undo_block, data, sizeof(data));
					memcpy(&undo_header, &char_header, CHAR_HEADER_SIZE);
					memcpy(undo_select, select, NN*NN);
					ResetSelection();
				}
				else break;
				return;
			}
}

//---------------------------------------------------------

void BMView::Save() {
	fm->Save();
}

//---------------------------------------------------------

void   BMView::RefreshChange() {
	SampleView	*sample;

	PutBackChar();
	((TApplication*)be_app)->s_view->ChangeSelect(-1);
	sample = (SampleView *)Window()->FindView("sample");
	sample->Draw(BRect(0,0,1000,1000));
}

//---------------------------------------------------------

void	BMView::do_undo()
{
	ResetSelection();
	memcpy(data, undo_block, sizeof(data));
	memcpy(&char_header, &undo_header, CHAR_HEADER_SIZE);
	memcpy(select, undo_select, NN*NN);
	Draw(BRect(0,0,1000,1000));
	RefreshChange();
}



//---------------------------------------------------------

void	BMView::do_reverse()
{
	memcpy(undo_block, data, sizeof(data));
	memcpy(&undo_header, &char_header, CHAR_HEADER_SIZE);

	ResetSelection();
	fm->RestoreBuffer(cur_char);
	ConvertToBitmap(fm->GetCharBits(cur_char), (uchar*)data, &char_header);
	Draw(BRect(0,0,1000,1000));
	RefreshChange();
}

















