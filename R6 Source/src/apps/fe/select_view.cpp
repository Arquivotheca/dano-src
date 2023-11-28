#include "select_view.h"
#include <stdio.h>
#include <Region.h>
#include "bm_view.h"
#include "twindow.h"
#include "main.h"
#include <Debug.h>

//---------------------------------------------------------

SelectView::SelectView(BRect rect, char *name)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect	r;
	long	i,j;

	r.top = 0;
	r.left = 0;
	r.bottom = (16*16)+48+150;
	r.right = (16*16);

	the_off = new BBitmap(r, B_COLOR_8_BIT, TRUE);
	drawer = new BView(r, "off_view", B_FOLLOW_ALL, B_WILL_DRAW);
	drawer->SetFont(be_fixed_font);
	if (be_fixed_font->Size() > 10.0)
		drawer->SetFontSize(10.0);
	the_off->Lock();
 	the_off->AddChild(drawer);
	the_off->Unlock();
	Reset(0);
}

void SelectView::Reset(bool all) {
	int      count;
	BRect    r;
	
	select = 0;
	
	if (all) {
		ChangeSelect(0);
		do_new_select(0);
		count = ((TApplication*)be_app)->bm_view->fm->count_char;
		count = 24*((count+3)>>2);
		if (count < (256+150))
			count = (256+150);
		((TApplication*)be_app)->scr_view->ScrollBar(B_VERTICAL)->SetRange(0, count-(256+150));
		((TApplication*)be_app)->scr_view->ScrollBar(B_VERTICAL)->SetValue(0);
		r.top = 0;
		r.bottom = 500000;
		r.left = 0;
		r.right = 256;
		Draw(r);
	}
}

//---------------------------------------------------------

void SelectView::render(BRect r)
{
	char	      str[2];
	int 	      i, imin, imax, index, index_min, index_max;	
	float	      width;
	uint32        count_char;
	uint32        *list_code;
	fc_char       **list_char;
	FontMachine   *fm;
	
	/* redraw the good lines */
	imin = r.top/24;
	imax = (r.bottom+24)/24;
	if (imin < 0)
		imin = 0;
	if (imax > imin+17)
		imax = imin+17;
		
	index_min = r.left/64;
	if (index_min < 0)
		index_min = 0;
	if (index_min > 3)
		return;
	index_max = (r.right+64)/64;
	if (index_max <= index_min)
		return;
	if (index_max > 3)
		index_max = 4;

	/* get font description */
	fm = ((TApplication*)be_app)->bm_view->fm;
	list_code = fm->list_code;
	list_char = fm->list_char;
	count_char = fm->count_char;

	/* erase the background */
	drawer->Window()->Lock();
	drawer->SetHighColor(255,255,255);
	drawer->FillRect(BRect(index_min*64, imin*24-r.top+24, index_max*64-1, imax*24-r.top+23));
	drawer->Sync();
	drawer->SetHighColor(0,0,0);

	for (i=imin; i<imax; i++)
		for (index=index_min; index<index_max; index++)
			if ((index+i*4) < count_char)
				drawCell(index*64, 24+(i*24-r.top),
						 list_code[i*4+index], list_char[i*4+index]);
	drawer->Sync();
	drawer->Window()->Unlock();
}	

static uchar color_table[8] = { 63, 27, 22, 18, 13, 9, 4, 0 };

void SelectView::drawCell(int h, int v, uint32 code, fc_char *ch) {
	int       i, j, val, dx, row;
	char      *base;
	uchar     *bitmap;
	uchar     str_code[7];
	uchar     *str;
	uint16    *uni_ptr;
	uint16    uni_str;

	drawer->StrokeRect(BRect(h, v, h+64, v+24));

	uni_str = code;
	uni_ptr = &uni_str;
	str = (uchar*)&str_code;
	convert_to_utf8(str, uni_ptr);
	str[0] = 0;
	val = 0;
	for (i=0; i<3; i++) {
		if (str_code[i] != 0) {
			val = (val<<8)+str_code[i];
		}
		else break;
	}
	sprintf((char*)str_code, "%x\x00", val);
	drawer->DrawString((char*)str_code, BPoint(h+3, v+11));
				   
	sprintf((char*)str_code, "%04x\x00", code);
	drawer->DrawString((char*)str_code, BPoint(h+3, v+21));
	
	base = (char*)the_off->Bits();
	row = the_off->BytesPerRow();

	bitmap = ch->bitmap;
	base += row*(v+18+ch->bbox.top) + (h+44);
	dx = (ch->bbox.right-ch->bbox.left+2)>>1;
	for (j=ch->bbox.top; j<=ch->bbox.bottom; j++) {
		for (i=ch->bbox.left; i<=ch->bbox.right; i+=2) {
			val = bitmap[(i-ch->bbox.left)>>1];
			base[i] = color_table[val>>4];
			base[i+1] = color_table[val&7];
		}
		bitmap += dx;
		base += row;
	}
}	

//---------------------------------------------------------

void SelectView::Draw(BRect r)
{
	long	h1, v1, v_offset;
	BRect   r2 = r;

	render(r);
	r2.OffsetBy(0.0, -r.top+24);
	DrawBitmap(the_off, r2, r);

	map_select(select, &h1, &v1);
	r.Set(h1+1,v1+1, h1+63, v1+23);
	SetDrawingMode(B_OP_BLEND); 
	SetHighColor(180,180,180);
	FillRect(r);
	SetHighColor(255,255,255);
	SetDrawingMode(B_OP_COPY);
}

//---------------------------------------------------------

void	SelectView::do_new_select(long select)
{
	BMView	*a_view;

	a_view = (BMView *)Window()->FindView("BMView");
	if (a_view) {
		a_view->change_select(select);
	}
}

//---------------------------------------------------------

void SelectView::ChangeSelect(long new_one) {
	long	px, py;
	BRect	r;
	long	old;

	if (new_one == -1)
		new_one = select;
	
	old = select;
	select = new_one;

	map_select(select, &px, &py);
	r.top = py;
	r.bottom = r.top + 23;
	r.left = px;
	r.right = r.left + 63;
	Draw(r);

	if (old != new_one) {
		map_select(old, &px, &py);
		r.top = py;
		r.bottom = r.top + 23;
		r.left = px;
		r.right = r.left + 63;
		Draw(r);

		((TApplication*)be_app)->samp_view->add_test_char(select);		
	}
}

//---------------------------------------------------------

void SelectView::MouseDown(BPoint where) {
	long	old;
	ulong	buttons;
	long	px,py;
	long	new_select;
	BRect	r;

	/* get font description */

	old = select;

	do {
		px = where.x/64;
		py = where.y/24;
		if (px < 0) px = 0;
		if (px > 3) px = 3;
		if (py < 0) py = 0;

		new_select = px + (py*4);
		if (new_select >= ((TApplication*)be_app)->bm_view->fm->count_char)
			goto skip;
		
		if (new_select == old)
			goto skip;

		ChangeSelect(new_select);
		old = new_select;
		do_new_select(new_select);
skip:;
		snooze(15000);
		GetMouse(&where, &buttons);
	} while(buttons);
}

//---------------------------------------------------------

void	SelectView::map_select(long in_select, long *px, long *py) {
	*px = (in_select % 4)*64;
	*py = (in_select / 4)*24;
}
















