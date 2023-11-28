#define	TIME_MAX_H	1024

#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include "wave_window.h"
#include "util.h"
//-------------------------------------------------------------------------

rgb_color	highc = {160,160,160};
//-------------------------------------------------------------------------

uchar	inv_table[256];
char	inv_table_inited = 0;
uchar	dim_table[256];
char	dim_table_inited = 0;
extern	TimeView	*the_time;

//-------------------------------------------------------------------------
void	init_dim_table();
void	set_cur_time(float new_one);
void	init_inv_table();
void	fDimRect(BBitmap *the_off, BRect r);
//-------------------------------------------------------------------------

void	set_cur_time(float new_one)
{
	BRect	bnd;

	if (the_time == 0)
		return;

	if (the_time->cur_t == new_one)
		return;

	the_time->Window()->Lock();
	the_time->cur_t = new_one;

	bnd = the_time->Bounds();
	bnd.right = TIME_MAX_H;
	the_time->private_draw(bnd);

	the_time->Draw(bnd);
	the_time->Window()->Unlock();
}

//-------------------------------------------------------------------------

void	init_inv_table()
{
	ushort		v;
	uchar		ov;
	rgb_color	c;
	BScreen		s;

	for (v = 0; v < 256; v++) {
		c = s.ColorForIndex(v);	
		if ((c.red == c.green) && (c.green == c.blue) & (c.green != 0)) {
			c.red = 255-c.red;
			c.green = 255-c.green;
			c.blue = 255-c.blue;
			ov = s.IndexForColor(c);
		}
		else {
			ov = v;
		}
		
		ov = s.IndexForColor(c);
		inv_table[v] = ov;
	}
	inv_table_inited = 1;
}

//-------------------------------------------------------------------------

void	init_dim_table()
{
	ushort		v;
	uchar		ov;
	rgb_color	c;
	BScreen		s;

	for (v = 0; v < 256; v++) {
		c = s.ColorForIndex(v);	
		c.red = (int)(c.red*0.8);
		c.green = (int)(c.green*0.8);
		c.blue = (int)(c.blue*0.8);

		ov = s.IndexForColor(c);
		dim_table[v] = ov;
	}
	dim_table_inited = 1;
}

//-------------------------------------------------------------------------

void	fDimRect(BBitmap *the_off, BRect r)
{
	long	rowbyte = the_off->BytesPerRow();
	char 	*base;
	long	y1,y2;	
	long	x1,x2;
	long	dx,dy;
	BRect	bnd;
	
	bnd = the_off->Bounds();
	

	if (dim_table_inited == 0) {
		init_dim_table();
	}

	y1 = (int)r.top;
	y2 = (int)r.bottom;
	if (r.right < r.left)
		return;

	if (y1 >= bnd.bottom)
		return;

	if (y2 < 0)
		return;

	
	
	if (y1 < 0) {
		y1 = 0;
	}

	//if (y2 > (40 - 1)) {
		//y2 = 40 - 1;
	//}
	if (y2 > (bnd.bottom - 1)) {
		y2 = (int)bnd.bottom - 1;
	}


	dy = (y2-y1);

	x1 = (int)(r.left);
	x2 = (int)(r.right);

	if (x1 > (bnd.right))
		return;
	if (x2 < 0)
		return;

	if (x1 < 0)
		x1 = 0;
	if (x2 > (bnd.right))
		x2 = (int)bnd.right;

	if (r.top >= 0) {
		while(y1 <= y2) {
			dx = x2 - x1;
			base = (char *)the_off->Bits() + y1*rowbyte+x1;
			y1++;

			while(dx>=0) {
				*base = dim_table[*base];
				base++;
				dx--;
			}
		}
	}
}

//-------------------------------------------------------------------------

void	fInvertRect(BBitmap *the_off, BRect r)
{
	long	rowbyte = the_off->BytesPerRow();
	char 	*base;
	long	y1,y2;	
	long	x1,x2;
	long	dx,dy;
	BRect	bnd;
	
	bnd = the_off->Bounds();

	if (inv_table_inited == 0) {
		init_inv_table();
	}

	y1 = (int)r.top;
	y2 = (int)r.bottom;
	if (r.right < r.left)
		return;

	if (y1 >= bnd.bottom)
		return;

	if (y2 < 0)
		return;

	
	
	if (y1 < 0) {
		y1 = 0;
	}

	//if (y2 > (40 - 1)) {
		//y2 = 40 - 1;
	//}
	if (y2 > (bnd.bottom - 1)) {
		y2 = (int)bnd.bottom - 1;
	}


	dy = (y2-y1);

	x1 = (int)(r.left);
	x2 = (int)(r.right);

	if (x1 > (bnd.right))
		return;
	if (x2 < 0)
		return;

	if (x1 < 0)
		x1 = 0;
	if (x2 > (bnd.right))
		x2 = (int)bnd.right;

	if (r.top >= 0) {
		while(y1 <= y2) {
			dx = x2 - x1;
			base = (char *)the_off->Bits() + y1*rowbyte+x1;
			y1++;

			while(dx>=0) {
				*base = inv_table[*base];
				base++;
				dx--;
			}
		}
	}
}

//-------------------------------------------------------------------------


float	get_start_time()
{
	if (the_time == 0)
		return 0;

	
	return the_time->select_start;
}

//-------------------------------------------------------------------------

float	get_end_time()
{
	if (the_time == 0)
		return 0;
	return the_time->select_end;
}

//-------------------------------------------------------------------------

double	TimeView::ToGrid(double v)
{
	double	ib;
	double	bpm;
	
	bpm = Beat();
	ib = 60.0/bpm;

	
	if (ib != 0) {

		double	ipos;
		long	int_v;

		ipos = v / ib;
		ipos += 0.5;
		int_v = (int)ipos;	
		v = int_v * ib;
	}

	return v;
}

//-------------------------------------------------------------------------

	TimeView::TimeView(BRect frame, char *name)
  	: BView(frame, name, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{

	the_off = new BBitmap(BRect(0,0,TIME_MAX_H-1,frame.bottom - frame.top),
						  B_COLOR_8_BIT,
						  TRUE);

	the_off->AddChild(off_view = new BView(BRect(0,0,2048,frame.bottom - frame.top),
										   "",
										   B_FOLLOW_ALL,
										    B_WILL_DRAW));

	SetViewColor(B_TRANSPARENT_32_BIT);
	start = 0;
	tpp = 0.01;
	select_start = 0.0;
	select_end = 5.6;
	cur_t = -1000.0;
	vsize = 40;
	beat_pm = 40;
	beat_per_measure = 4;
}

//-------------------------------------------------------------------------

void	TimeView::Save(long ref)
{
	writef(ref, select_start);
	writef(ref, select_end);
	write32(ref, vsize);
	writef(ref, beat_pm);
	write32(ref, beat_per_measure);
}

//-------------------------------------------------------------------------

void	TimeView::Load(long ref)
{
	BRect	r;

	select_start = readf(ref);
	select_end = readf(ref);
	vsize = read32(ref);
	beat_pm = readf(ref);
	beat_per_measure = read32(ref);
	
	r = Bounds();
	r.right = TIME_MAX_H;
	private_draw(r);
	Draw(Bounds());
}


//-------------------------------------------------------------------------

	TimeView::~TimeView()
{
	delete the_off;
}

//-------------------------------------------------------------------------

char	TimeView::ShowBeat()
{
	return (vsize != 40);
		
}

//-------------------------------------------------------------------------

double	TimeView::Beat()
{
	return beat_pm;
}

//-------------------------------------------------------------------------


void	TimeView::SetBPM(long v)
{
	BRect	r;

	beat_per_measure = v;

	r = Bounds();
	r.right = TIME_MAX_H;

	private_draw(r);
	Draw(BRect(0,0,32000,32000));
}


//-------------------------------------------------------------------------


void	TimeView::SetBeat(double v)
{
	BRect	r;
	
	beat_pm = v;

	r = Bounds();
	r.right = TIME_MAX_H;
	private_draw(r);
	Draw(BRect(0,0,32000,32000));
}

//-------------------------------------------------------------------------

void	TimeView::draw_cur_time(double t)
{
	long	h;
	double	v;
	BRect	r;
	BRect	r1;
	char	buf[256];
	long	vp;

	r = Bounds();

	h = time_to_h(t);

	off_view->SetHighColor(200,30,30);

	off_view->FillTriangle(BPoint(h, r.bottom-4),
				 		   BPoint(h-4, r.bottom-8),
				 		   BPoint(h+4, r.bottom-8));

	off_view->MovePenTo(BPoint(h, r.bottom-12));
	off_view->StrokeLine(BPoint(h, r.bottom - 9));
	
	r1.bottom = r.bottom - 12;
	r1.left = h - 20;
	r1.right = h + 20;
	
	off_view->SetHighColor(30,30,30);

	time_to_string(t, buf);
	
	float	w;

	off_view->SetFontSize(9);
	w = off_view->StringWidth(buf);
	
	w = h - w/2;

	vp = 25;

	if (ShowBeat()) 
		vp = 23;

	off_view->MovePenTo(BPoint(w, vp));	
	off_view->SetDrawingMode(B_OP_OVER);
	off_view->DrawString(buf);
	off_view->SetFontSize(11);

	off_view->Sync();
}

//-------------------------------------------------------------------------

void	TimeView::Toggle()
{
	BRect	bnd = Bounds();

	bnd.right = TIME_MAX_H;

	if (vsize == 40) {
		do {
			vsize--;
			private_draw(bnd);
			Draw(bnd);
		} while(vsize != 23);
		Sync();
		return;
	}

	if (vsize == 23) {
		do {
			vsize++;
			private_draw(bnd);
			Draw(bnd);
		} while(vsize != 40);
		Sync();
		return;
	}
}

//-------------------------------------------------------------------------

void	TimeView::Draw(BRect r)
{
	DrawBitmap(the_off, BPoint(0,0));
}

//-------------------------------------------------------------------------

double	TimeView::h_to_time(double h)
{
	double	t;

	t = (h*tpp) + start;

	return(t);
}

//-------------------------------------------------------------------------

long	TimeView::time_to_h(double t)
{
	double	h;

	h = (t - start)/tpp;

	return((int)(h+0.5));
}

//-------------------------------------------------------------------------

void	TimeView::set_parm(float p_start, float p_tpp)
{
	BRect		bnd;
	
	start = p_start;
	tpp = p_tpp;

	bnd = Bounds();

	bnd.right = TIME_MAX_H;

	private_draw(bnd);
}

//-------------------------------------------------------------------------

void	TimeView::time_to_string(float t, char *buf)
{
	char	*p1;
	long	intv;
	float	t0 = t;
	
	intv = (int)t;

	p1 = buf;

	if (intv > 3600) {
		*p1++ = 0x30+(intv/3600);			//hours
		*p1++ = ':';
	}

	intv = intv % 3600;						//number from 0 to 3599
	*p1++ = 0x30+(intv/600);				//minutes
	intv = intv % 600;						//number from 0 to 599
	*p1++ = 0x30+(intv/60);					//minutes digit
	*p1++ = ':';
	intv = intv % 60;

	*p1++ = 0x30+(intv/10);
	intv %= 10;
	*p1++ = 0x30+(intv);

	*p1++ = ':';

	intv = (int)t;
	t -= intv;
	t *= 100;

	intv = (int)t;

	*p1++ = 0x30 + (intv/10);
	intv %= 10; 
	*p1++ = 0x30 + (intv/10);
	*p1++ = 0;

}

//-------------------------------------------------------------------------

void	TimeView::private_draw(BRect r)
{
	long		i;
	long		cur_int;
	long		h;
	rgb_color	c1;
	char		buf[256];
	long		last;
	float		w;
	long		step;
	long		select_left;
	long		select_right;
	BRect		tmp_r;

	the_off->Lock();
	off_view->SetHighColor(backgr);
	off_view->FillRect(r);

	select_left = time_to_h(select_start);
	select_right = time_to_h(select_end);

	tmp_r.left = select_left;
	tmp_r.right = select_right;
	tmp_r.top = r.top;
	tmp_r.bottom = r.bottom;
	off_view->FillRect(tmp_r);

	off_view->Sync();



	step = 10;
	if (tpp > 0.02) {
		step = 5;
	}
	
	if (tpp > 0.04) {
		step = 2;
	}

	cur_int = (int)(start*(step*1.0));
	c1.red = 0;c1.green = 0;c1.blue = 255;
	do {
		h = time_to_h(cur_int/(step*1.0));
		if (h > -10) {
			vl(h, (vsize-11), (vsize - 4), 18);
			vl(h+1, (vsize-10), (vsize - 3), 29);
		} 	
		cur_int+=1;
	} while(h < (TIME_MAX_H + 50));

	
	cur_int = (int)start;

	cur_int -= 3;
	step = 1;
	if (tpp >= (0.019*2)) {
		step = 2;
		cur_int &= ~1;
	}
	if (tpp >= (0.019*4)) {
		step = 4;
		cur_int &= ~3;
	}
	if (tpp >= (0.019*8)) {
		step = 8;
		cur_int &= ~7;
	}
	if (tpp >= (0.019*16)) {
		step = 16;
		cur_int &= ~15;
	}


	last = -1000;

	c1.red = 255;c1.green = 0;c1.blue = 0;
	do {
		h = time_to_h(cur_int);
		if (h > -10) {
			vl(h, (vsize-22), (vsize - 1), 18);
			vl(h+1, (vsize-21), vsize, 29);
		} 	
		cur_int+=step;
	} while(h < (TIME_MAX_H + 50));


	
	cur_int = (int)start;

	last = -1000;

	c1.red = 255;c1.green = 0;c1.blue = 0;
	
	cur_int -= 3;
	step = 1;
	if (tpp >= 0.019) {
		step = 2;
		cur_int &= ~1;
	}
	if (tpp >= 0.019*2) {
		step = 4;
		cur_int &= ~3;
	}
	if (tpp >= 0.019*4) {
		step = 8;
		cur_int &= ~7;
	}
	if (tpp >= 0.019*8) {
		step = 16;
		cur_int &= ~15;
	}
	if (tpp >= 0.019*16) {
		step = 32;
		cur_int &= ~31;
	}
	
	off_view->SetDrawingMode(B_OP_OVER);
	off_view->SetHighColor(40,40,40);
	do {
		h = time_to_h(cur_int);
		if (h > -10) {
			if ((h-last) > 90) {
				BRect	r;
				long	ww;

				time_to_string(cur_int, buf);
				ww  = (int)off_view->StringWidth(buf);
				w = ww;
				w = h - w/2;
				if (w < 3)
					w = 3;

				r.left = w-1;
				r.right = w + ww;
				r.top = 0;
				r.bottom = vsize/2.5 + 1;
				off_view->SetHighColor(backgr);
				off_view->FillRect(r);
				off_view->SetHighColor(40,40,40);
				off_view->MovePenTo(BPoint(w, (vsize/2.5)));			//was 15
				off_view->DrawString(buf);
				last = h;
			}
		} 	
		cur_int+=step;
	} while(h < (TIME_MAX_H + 50));
			
	off_view->SetHighColor(130,130,140);
	off_view->MovePenTo(BPoint(0,r.bottom-1));
	off_view->StrokeLine(BPoint(4096, r.bottom-1)); 
	off_view->SetHighColor(160,160,160);
	off_view->MovePenTo(BPoint(0,r.bottom));
	off_view->StrokeLine(BPoint(4096, r.bottom)); 


	select_left = time_to_h(select_start);
	select_right = time_to_h(select_end);

	tmp_r.left = select_left;
	tmp_r.right = select_right;
	tmp_r.top = r.top;
	tmp_r.bottom = r.bottom - 2;

	if (ShowBeat()) {
		off_view->SetHighColor(18*8,18*8,18*8);
		off_view->MovePenTo(BPoint(0, vsize + 1));
		off_view->StrokeLine(BPoint(320000, vsize + 1));
		off_view->SetHighColor(28*8,28*8,28*8);
		off_view->MovePenTo(BPoint(0, vsize + 2));
		off_view->StrokeLine(BPoint(320000, vsize + 2));
		off_view->Sync();
	
		double	ibeat;
		double	cur_t;
		long	beat_num;

		if (beat_pm > 0) {
			ibeat = 60.0/beat_pm;

			beat_num = (int)(start / ibeat);
			beat_num--;
			cur_t = beat_num * ibeat;
			
			do {
				h = time_to_h(cur_t);
				if (h > -10) {
					//vl(h, (vsize + 1), 38, 18);
					//vl(h+1, (vsize + 2), 38, 29);
					if ((beat_num % beat_per_measure) == 0) {
						vl(h, (vsize + 2), 38, 8);				//test
						vl(h+1, (vsize + 3), 38, 29);
					}
					else {
						vl(h, (vsize + 8), 38, 18);
						vl(h+1, (vsize + 9), 38, 29);
					}
				} 	
				beat_num++;
				cur_t += ibeat;
			} while(h < (TIME_MAX_H + 50));


		}
	}

	
	draw_cur_time(cur_t);
	off_view->Sync();
	
	fInvertRect(the_off, tmp_r);

	the_off->Unlock();
}

//-------------------------------------------------------------------------

void	TimeView::ChangeSelection(float new_a, float new_b)
{
	BRect	r;

	r = Bounds();

	the_off->Lock();

	r.bottom -= 2;

	r.left = time_to_h(select_start);
	r.right = time_to_h(select_end);
	
	fInvertRect(the_off, r);

	if (new_a < new_b) {
		select_start = new_a;
		select_end = new_b;
	}
	else {
		select_start = new_b;
		select_end = new_a;
	}

	r.left = time_to_h(select_start);
	r.right = time_to_h(select_end);
	fInvertRect(the_off, r);
	off_view->SetDrawingMode(B_OP_COPY);
	off_view->Sync();
	the_off->Unlock();
}

//-------------------------------------------------------------------------

void	TimeView::AttachedToWindow()
{
	BRect		bnd;

	bnd = Bounds();

	bnd.right = TIME_MAX_H;
	private_draw(bnd);
}

//-------------------------------------------------------------------------

void	TimeView::move_sb(float direction)
{
	BScrollBar	*s;
	float		smallStep;
	float		largeStep;

	s = (BScrollBar *)Window()->FindView("_HSB_");

	if (s) {
		s->GetSteps(&smallStep, &largeStep);
		s->SetValue(s->Value() + (smallStep * direction));
	}
}

//-------------------------------------------------------------------------

void	TimeView::MouseDown(BPoint where)
{
	float	v1;
	float	v2;
	float	d;
	ulong	button;
	BRect	wb;
	float	u_select_start = select_start;
	float	u_select_end = select_end;
	char	extend = 0;
	char	grid = 0;
	float	new_t;

	if (modifiers() & B_SHIFT_KEY)
		extend = 1;

	if (modifiers() & B_CONTROL_KEY)
		grid = 1;

	wb = this->Bounds();
	v1 = h_to_time(where.x);
	v2 = -1000;
	
	do {
		Window()->Unlock();
		snooze(20000);
		Window()->Lock();
		GetMouse(&where, &button);
		if (where.x > wb.right) {
			d = where.x - wb.right;
			d /= 60.0;
			move_sb(d);
		}
		if (where.x < 0) {
			move_sb(where.x / 60.0);
		}

		new_t = h_to_time(where.x);

		if (grid)
			new_t = ToGrid(new_t);

		if (v2 != new_t) {
			v2 = new_t;

			if (extend) {
				float	d1,d2;
	
				d1 = fabs(v2 - select_start);
				d2 = fabs(v2 - select_end);

				if (d1 > d2) {
					v1 = u_select_start;
					v2 = v2;
				}
				else {
					v1 = v2;
					v2 = u_select_end;
				}
			}

			if (v1 < 0) v1 = 0;
			if (v2 < 0) v2 = 0;
			if (grid) {	
				v1 = ToGrid(v1);
				v2 = ToGrid(v2);
			}

			ChangeSelection(v1, v2);
			Draw(Bounds());
		}

	} while(button);
}

//-------------------------------------------------------------------------

void	TimeView::vl(long x1,long y1,long y2, uchar c)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = the_off->BytesPerRow();
	char 	*base;
	
	
	if (y1 < 0)
		y1 = 0;
	if (x1 < 0 || x1 > rowbyte)
		return;
	
	
	dy = y2-y1;
	
	base = (char *)the_off->Bits() + y1*rowbyte+x1;
	/*
	while(dy>4) {
		*base = c;
		base += rowbyte;
		*base = c;
		base += rowbyte;
		*base = c;
		base += rowbyte;
		*base = c;
		base += rowbyte;
		dy-=4;
	}
	*/

	while(dy>0) {
		*base = c;
		base += rowbyte;
		dy--;
	}
}

//-------------------------------------------------------------------------
	
	TCtrlView::TCtrlView(BRect frame, char *name)
  	: BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	BPopUpMenu		*pop;		
	BMessage		*msg;
	BMenuItem		*mitem;
	BRect			r;
	BMenuField		*btn;

	SetViewColor(backgr);
	
	r.left = 15;
	r.right = 65;
	r.top = 9;
	r.bottom = 22;
	b = new BBitmap(BRect(0,0,31,31),
						  B_COLOR_8_BIT,
						  TRUE);

	b->AddChild(bv = new BView(BRect(0, 0, 31, 31),"",B_FOLLOW_ALL,B_WILL_DRAW));
	rot = 3.1415926;

	cur_beat = new BTextControl(r, "Ticks:", "Ticks:", "40", 0, 0);
	cur_beat->SetModificationMessage(new BMessage('bet!'));
	AddChild(cur_beat);

	pop = new BPopUpMenu("4/4",
						 true,
						 true);
	
	msg = new BMessage(NEW_BPM);msg->AddInt32("v", 3040);mitem = new BMenuItem("3/4", msg);
	pop->AddItem(mitem);
	
	msg = new BMessage(NEW_BPM);msg->AddInt32("v", 4040);mitem = new BMenuItem("4/4", msg);
	mitem->SetMarked(true);
	pop->AddItem(mitem);
	
	msg = new BMessage(NEW_BPM);msg->AddInt32("v", 5040);mitem = new BMenuItem("5/4", msg);
	pop->AddItem(mitem);
	
	msg = new BMessage(NEW_BPM);msg->AddInt32("v", 6040);mitem = new BMenuItem("6/4", msg);
	pop->AddItem(mitem);
	
	msg = new BMessage(NEW_BPM);msg->AddInt32("v", 7040);mitem = new BMenuItem("7/4", msg);
	pop->AddItem(mitem);
	
	r.top = 8; r.bottom = r.top + 18;
	r.left = 68; r.right = r.left + 43;
	btn = new BMenuField(r, "beat/b", "", pop, true);
	btn->SetFontSize(10);
	(btn->MenuBar())->SetFontSize(10);
	(btn->Menu())->SetFontSize(10);
	btn->SetDivider(0);
	AddChild(btn);
	SetRot(rot);
}

//-------------------------------------------------------------------------

void	TCtrlView::Save(long ref)
{
	char	buf[32];

	strcpy(buf, cur_beat->Text());
	write(ref, &buf, 32);
}

//-------------------------------------------------------------------------

void	TCtrlView::Load(long ref)
{
	char	buf[32];
	long	cnt;

	cnt = read(ref, &buf, 32);

	if (cnt)
		cur_beat->SetText(buf);
}


//-------------------------------------------------------------------------

	TCtrlView::~TCtrlView()
{
	delete b;
}

//-------------------------------------------------------------------------

void	TCtrlView::MouseDown(BPoint where)
{
	
	if (where.x < 15) {
		((TWaveWindow*)Window())->Switch_H();
	}
}

//-------------------------------------------------------------------------

void	TCtrlView::new_bpm(long v)
{
	TimeView		*tv;

	v /= 1000;
	tv = (TimeView *)Window()->FindView("time");
	tv->SetBPM(v);
}

//-------------------------------------------------------------------------

void	TCtrlView::SetBPM(float v)
{
	char	buf[32];

	sprintf(buf, "%2.3f", v);
	cur_beat->SetText(buf);
	txt_msg();
}

//-------------------------------------------------------------------------

void	TCtrlView::txt_msg()
{
	char			buf[512];
	float			b;
	TimeView		*tv;


	strcpy(buf, cur_beat->Text());

	b = atof(buf);

	if (b < 15)
		b = 15;
	if (b > 512)
		b = 512;	

	tv = (TimeView *)Window()->FindView("time");

	tv->SetBeat(b);
}

//-------------------------------------------------------------------------

BPoint	TCtrlView::transform(BPoint p)
{
	float	sina = sin(rot);
	float	cosa = cos(rot);
	float	x0,y0;

	p.x -= 9;
	p.y -= 17;



	x0 = p.x * cosa - p.y * sina;
	y0 = p.x * sina + p.y * cosa;

	x0 += 9;
	y0 += 17;

	
	p.x = x0;
	p.y = y0;

	return p;
}

//-------------------------------------------------------------------------

void	TCtrlView::blure()
{
	uchar	*c;
	long	x,y;
	uchar	tmp[32][32];
	long	acc;

	c = (uchar *)b->Bits();

	for (y = 1; y < 29;y++)
	for (x = 2; x < 20;x++) {
		acc = c[(x+y*32)];
		acc = acc + acc + acc + acc;

		acc += c[(x+y*32)+1];
		acc += c[(x+y*32)-1];
		acc += c[(x+y*32)+32];
		acc += c[(x+y*32)-32];
		acc /= 8;
		tmp[y][x] = acc;
	}
	for (y = 1; y < 29;y++)
	for (x = 2; x < 20;x++) {
		c[x+y*32] = tmp[y][x];
	}
}

//-------------------------------------------------------------------------

void	TCtrlView::SetRot(float r)
{

	rot = r;

	b->Lock();
	bv->SetHighColor(backgr);
	bv->FillRect(BRect(0,0,32000,32000));
	bv->SetHighColor(32,32,32);

	bv->FillTriangle(transform(BPoint(5,10)),
					 transform(BPoint(5,24)),
					 transform(BPoint(12,17)));
	bv->Sync();
	blure();
	b->Unlock();

	DrawBitmap(b, BRect(1, 1, 1 + 14, 1 + 30),BRect(0,3,14,33));
}

//-------------------------------------------------------------------------
	
void	TCtrlView::Draw(BRect ur)
{
	BRect		r;
	long		i;
	rgb_color	c;

	r = Bounds();

	SetHighColor(130,130,140);
	MovePenTo(BPoint(0,r.bottom-1));
	StrokeLine(BPoint(4096, r.bottom-1)); 
	SetHighColor(160,160,160);
	MovePenTo(BPoint(0,r.bottom));
	StrokeLine(BPoint(4096, r.bottom)); 


	c.red = c.green = c.blue = 0;

	DrawBitmap(b, BRect(1, 1, 1 + 14, 1 + 30),BRect(0,3,14,33));

}
		
