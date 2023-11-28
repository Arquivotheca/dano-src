#include <Screen.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <Application.h>
#include <Roster.h>
#include <Entry.h>
#include <Path.h>
#include <Screen.h>
#include <File.h>
#include <Menu.h>
#include <MenuBar.h>
#include <ScrollBar.h>
#include <MenuItem.h>
#include "track_obj.h"
#include <SoundFile.h>
#include <math.h>

#include "track_view.h"
#include "edit_window.h"

#define	EDIT_MAX_H	1024
#define	EDIT_MAX_V	400

//-------------------------------------------------------------------------
extern	long	do_ask_volume(BWindow *dad, char *name, float *f1);
extern	long	do_ask_filter1(BWindow *dad, char *name, float *f1, float *f2);
extern	long	do_ask_strength(BWindow *dad, char *name, float *f1);
extern	long	do_ask_beat(float v);
//-------------------------------------------------------------------------
#define		UNDO			0x201
#define		CLEAR			0x202
#define		CUT				0x203
#define		COPY			0x204
#define		PASTE			0x205
//-------------------------------------------------------------------------
#define		ZERO			0x400
#define		VOLUME			0x401
#define		MAX_VOLUME		0x402
#define		REVERBE			0x403
#define		ECHO			0x404
#define		CHORUS			0x405
#define		METAL			0x406
#define		FADE			0x407
#define		COMPRESS		0x408
//-------------------------------------------------------------------------
extern	void	main_refresh();
//-------------------------------------------------------------------------

class	PrivateClip {
public:
					PrivateClip();
					~PrivateClip();
		void		SetClip(short *pt, long vcnt);
		short		*GetClip(long *vcnt);
private:
		short	*p;
		long	cnt;
};

//-------------------------------------------------------------------------
PrivateClip		the_clip;
//-------------------------------------------------------------------------


	PrivateClip::PrivateClip()
{
	p = 0;
	cnt = 0;
}


//-------------------------------------------------------------------------

	PrivateClip::~PrivateClip()
{	
	free((char *)p);
}

//-------------------------------------------------------------------------

void PrivateClip::SetClip(short *pt, long vcnt)
{
	cnt = vcnt;		
	
	free((char *)p);

	//printf("set clp %ld\n", vcnt);
	p = (short *)malloc(vcnt * sizeof(short));
	memcpy((char *)p, pt, vcnt * sizeof(short));
}

//-------------------------------------------------------------------------

short	*PrivateClip::GetClip(long *vcnt)
{
	*vcnt = cnt;
	return p;
}

//-------------------------------------------------------------------------

class eTimeView : public TimeView {
public:
				eTimeView(BRect frame, char *name); 
virtual	void	eChangeSelection(float new_a, float new_b, char draw);
		void	drag_time();
virtual	void	MouseDown(BPoint where);
};


//-------------------------------------------------------------------------

	eTimeView::eTimeView(BRect frame, char *name) : TimeView(frame, name) {}

//-------------------------------------------------------------------------


void	eTimeView::drag_time()
{
	BPoint	w;
	BPoint	w1;
	ulong	button;
	float	dt;

	GetMouse(&w, &button);

	do {
		w1 = w;
		GetMouse(&w, &button);
		if (w1.x != w.x) {
			dt = w1.x - w.x;
			dt *= tpp;
			move_sb(dt * 32.0);
		}
		else
			snooze(32000);
	} while(button);
}

//-------------------------------------------------------------------------

void	eTimeView::MouseDown(BPoint where)
{
	float	v1;
	float	v2;
	ulong	button;
	BRect	wb;
	long	last_x;
	char	moving;
	float	dmove;
	

	GetMouse(&where, &button);
	if (modifiers() & B_OPTION_KEY || (button > 1)) {
		drag_time();
		return;
	}

	wb = this->Bounds();
	v1 = h_to_time(where.x);
	v2 = -1000;
	
	last_x = -32000;

	do {
		snooze(20000);
		GetMouse(&where, &button);

		moving = 0;
		if (where.x > wb.right) {
			dmove = (where.x - wb.right) / 60.0;
			moving = 1;
		}
		else
		if (where.x < 0) {
			moving = 1;
			dmove = where.x / 60;
		}
		
		if (v2 != h_to_time(where.x)) {
			v2 = h_to_time(where.x);
			if (v1 < 0) v1 = 0;
			if (v2 < 0) v2 = 0;
			eChangeSelection(v1, v2, !moving);
			if (!moving)
				Draw(Bounds());
		}
		
		if (moving) {
			move_sb(dmove);
		}

		if (where.x == last_x) {
			snooze(15000);
			goto skip;
		}			
		last_x = (int)where.x;
skip:;
	} while(button);
}

//-------------------------------------------------------------------------

void	eTimeView::eChangeSelection(float new_a, float new_b, char draw)
{
	BRect			r;
	EditView		*v;

	r = Bounds();

	if (draw) {
		the_off->Lock();
	}

	r.bottom -= 2;

	r.left = time_to_h(select_start);
	r.right = time_to_h(select_end);
	
	if (draw)
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
	
	if (draw) {
		fInvertRect(the_off, r);
		off_view->SetDrawingMode(B_OP_COPY);
		off_view->Sync();
		the_off->Unlock();
	}

	v = (EditView *)Window()->FindView("edit view");
	if (v) {
		v->NewSelection(select_start, select_end, draw);
	}
}

//-------------------------------------------------------------------------


class eZoomControl : public ZoomControl {
public:
				eZoomControl(BRect r);
virtual	void	HandleValue(float v);
};


//-------------------------------------------------------------------------

	eZoomControl::eZoomControl(BRect r)
	: ZoomControl(r)
{
}

//-------------------------------------------------------------------------

void	eZoomControl::HandleValue(float v)
{
	EditWindow	*w;

	w = (EditWindow *)Window();
	w->HandleZoom(v);
}

//-------------------------------------------------------------------------

class	eTHSB : public BScrollBar {
public:
		long	min;
		long	max;
public:
					eTHSB(char *name, BRect view_bound, float max);
virtual		void	ValueChanged(float v);
			void	UpdateSpeed(float s);
};

/*------------------------------------------------------------*/

void	eTHSB::ValueChanged(float v)
{
	EditWindow	*w;

	w = (EditWindow *)Window();
	w->new_h(v);
}


/*------------------------------------------------------------*/

eTHSB::eTHSB(char *name, BRect view_bound, float max)
	: BScrollBar(view_bound, name, 0, 0, max*100.0, B_HORIZONTAL)
{
}

/*------------------------------------------------------------*/

void	eTHSB::UpdateSpeed(float zoom)
{
	float	small;
	
	
	small = zoom * 100.0 * 8.0;
	if (small < 1)
		small = 1;

	SetSteps(small*1.0, small * 16.0);
}

//-------------------------------------------------------------------------

void	EditView::NewSelection(float s, float e, char draw)
{
	select_left = s;
	select_right = e;
	if (draw) {
		Render();
		Draw(BRect(0,0,32000,32000));
	}
}

//-------------------------------------------------------------------------

long	EditView::time_to_h(float t)
{
	float	h;

	h = (t - cur_time)/zoom;

	return((int)(h+0.5));
}

//-------------------------------------------------------------------------

void	EditView::Render()
{
	long	i;
	float	t;
	short	min,max;
	float	v_mult;
	BRect	r;
	long	it;
	float	time_length;

	if ((last_ct == cur_time)    &&
	    (last_sl == select_left) &&
		(last_sr == select_right)&&
		(last_z == zoom)		 &&
		(dirty == 0)			 &&
		(last_v_size == last_vs)) {
		return;
	}

	dirty = 0;
	last_ct = cur_time;
	last_sl = select_left;
	last_sr = select_right;
	last_z = zoom;
	last_vs = last_v_size;

	b->Lock();
	t = 0;

	view->SetHighColor(200, 200, 200);
	view->FillRect(BRect(0,0,1023,last_v_size));
	view->Sync();
	v_mult = last_v_size / 65536.0;

	t = cur_time;
	it = (int)(t / zoom);	
	t = it * zoom;

	time_length = cache->Length();

	for (i = 0; i < 1024; i++) {
		if (t < time_length) {
			cache->GetSample(t, zoom, &min, &max);
			t += zoom;
			min = (int)(min*v_mult);
			max = (int)(max*v_mult);
			if (min!=0 || max!=0) {
				vl(i, (last_v_size/2) + min, (last_v_size/2), 45); 
				vl(i, (last_v_size/2), (last_v_size/2) + max, 72); 
			}
		}
		else
			goto out;
	}
out:;

	view->SetHighColor(0,0,255);
	view->MovePenTo(BPoint(0, last_v_size/2));
	view->StrokeLine(BPoint(1024, last_v_size/2));
	view->Sync();

	r.top = 0;
	r.bottom = last_v_size;
	r.left = time_to_h(select_left);
	r.right = time_to_h(select_right);
	fDimRect(b, r);
	b->Unlock();
}

//-------------------------------------------------------------------------

void	EditView::set_param(float t, float zo)
{
	zoom = zo;
	cur_time = t;
}

//-------------------------------------------------------------------------


	EditView::EditView(BRect frame, char *name, SampleCache *the_cache)
  	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	cache = the_cache;
	b = new BBitmap(BRect(0, 0, EDIT_MAX_H-1, EDIT_MAX_V),
						  	B_COLOR_8_BIT,
						  	TRUE);
	b->AddChild(view = new BView(BRect(0,0,1023,300),
										"",
										B_FOLLOW_ALL,
										B_WILL_DRAW));

	SetViewColor(B_TRANSPARENT_32_BIT);
	Render();
	g_delay = 10.0;
	g_intens = 50.0;
	last_h_size = 0;
	last_v_size = 0;
	select_left = 0;
	select_right = 0;
	zoom = 0.01;
	undo = 0;
	undo_length = 0;
	last_ct = -32000;
}

//-------------------------------------------------------------------------

	EditView::~EditView()
{
	if (undo)
		free((char *)undo);
}

//-------------------------------------------------------------------------

void	EditView::Draw(BRect r)
{
	BRect	rr;
	long	h_size,v_size;

	rr = Bounds();

	h_size = (int)(rr.right - rr.left);
	v_size = (int)(rr.bottom - rr.top);

	if (v_size != last_v_size) {
		last_v_size = v_size;
		Render();
	}
	last_h_size = h_size;
	DrawBitmap(b, BPoint(0,0));
}

//-------------------------------------------------------------------------

void	EditView::vl(long x1,long y1,long y2, uchar c)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = EDIT_MAX_H;
	char 	*base;
	float	k;
	
	if (y1 < 0)
		y1 = 0;
	if (y2 > (EDIT_MAX_V - 1))
		y2 = EDIT_MAX_V - 1;
	if (x1 < 0 || x1 > EDIT_MAX_H)
		return;
	
	
	dy = y2-y1;
	
	base = (char *)b->Bits() + y1*rowbyte+x1;
	
	while(dy > 4) {
		*base = c;
		base += rowbyte;
		dy -= 4;
		*base = c;
		base += rowbyte;
		*base = c;
		base += rowbyte;
		*base = c;
		base += rowbyte;
	}
	
	while(dy>0) {
		*base = c;
		base += rowbyte;
		dy--;
	}
}

//-------------------------------------------------------------------------

void	EditView::MouseDown(BPoint where)
{
	eTimeView	*time;

	time = (eTimeView *)Window()->FindView("edit_time");
	time->MouseDown(where);
}

//-------------------------------------------------------------------------


	EditWindow::EditWindow(BRect r, const char* t, SampleCache *cache)
			 :BWindow(r, t, B_DOCUMENT_WINDOW, B_NOT_ZOOMABLE) 
{
	eTimeView	*ti;
	EditView	*ed;
	BRect		a_rect;
	long 		vp = (int)(r.bottom - r.top);
	BMenuBar	*menubar;
	BMenuItem	*item;
	float 		mb_height;
	BMenu		*a_menu;

	cur_zoom = 0.01;
	cur_time = 0.00;

eZoomControl	*ez;

	a_rect.right = 109;
	a_rect.bottom = vp +  1;
	a_rect.top = vp + 1 - B_H_SCROLL_BAR_HEIGHT;
	a_rect.left = 0;
	AddChild(ez = new eZoomControl(a_rect));
	
	menubar = new BMenuBar(BRect(0,0,2000,15), "MB");
	
	AddChild(menubar);

	a_menu = new BMenu("Edit");
	menubar->AddItem(a_menu);
	a_menu->AddItem(undo_itm = new BMenuItem("Undo", new BMessage(UNDO), 'Z'));
	undo_itm->SetEnabled(false);
	a_menu->AddSeparatorItem();
	a_menu->AddItem(new BMenuItem("Clear", new BMessage(CLEAR)));
	a_menu->AddItem(new BMenuItem("Cut", new BMessage(CUT), 'X'));
	a_menu->AddItem(new BMenuItem("Copy", new BMessage(COPY), 'C'));
	a_menu->AddItem(new BMenuItem("Paste", new BMessage(PASTE), 'V'));

	a_menu = new BMenu("Filter");
	menubar->AddItem(a_menu);
	a_menu->AddItem(new BMenuItem("Zero", new BMessage(ZERO)));
	a_menu->AddItem(new BMenuItem("Change Volume...", new BMessage(VOLUME)));
	a_menu->AddItem(new BMenuItem("Max Volume", new BMessage(MAX_VOLUME)));
	//a_menu->AddItem(new BMenuItem("Reverbe...", new BMessage(REVERBE)));
	a_menu->AddItem(new BMenuItem("Echo...", new BMessage(ECHO)));
	a_menu->AddItem(new BMenuItem("Chorus...", new BMessage(CHORUS)));
	a_menu->AddItem(new BMenuItem("Metal...", new BMessage(METAL)));
	a_menu->AddItem(new BMenuItem("Fade...", new BMessage(FADE)));
	a_menu->AddItem(new BMenuItem("Compress", new BMessage(COMPRESS)));

	mb_height = menubar->Bounds().Height();

	ti = new eTimeView(BRect(0,mb_height+1, (r.right-r.left), 40+mb_height+1), "edit_time");
	AddChild(ti);
	SetSizeLimits(200, 1024, 100, 355);
	ed = new EditView(BRect(0,41 + mb_height + 1,1024, vp - B_H_SCROLL_BAR_HEIGHT), "edit view", cache);
	AddChild(ed);

	AddChild(new eTHSB("_HSB_", 		   BRect(110,
											    (vp-B_H_SCROLL_BAR_HEIGHT) + 1,
												(r.right-r.left)-14+1,
												(vp) + 1
											   ),
										  cache->Length()
		
					  )
			);
	
	


	ti->eChangeSelection(0, cache->Length(), true);

	snooze(30000);

	ez->MouseDown(BPoint(44,0));
}

//-------------------------------------------------------------------------

void	EditWindow::new_h(float t)
{
	EditView	*ed;
	eTimeView	*tv;

	cur_time = t/32.0;

	ed = (EditView *)FindView("edit view");
	ed->set_param(cur_time, cur_zoom);
	ed->Render();
	
	tv = (eTimeView *)FindView("edit_time");
	
	tv->set_parm(cur_time, cur_zoom);
	ed->Draw(BRect(0,0,32000,32000));
	tv->Draw(BRect(0,0,32000,32000));
}

//-------------------------------------------------------------------------

void	EditWindow::HandleZoom(float v)
{
	EditView	*ed;
	eTimeView	*tv;
	

	v /= 6.0;

	cur_zoom = v;

	ed = (EditView *)FindView("edit view");
	ed->set_param(cur_time, cur_zoom);
	ed->Render();
	
	tv = (eTimeView *)FindView("edit_time");
	
	tv->set_parm(cur_time, cur_zoom);
	ed->Draw(BRect(0,0,32000,32000));
	tv->Draw(BRect(0,0,32000,32000));

eTHSB *	e;

	e = (eTHSB*)FindView("_HSB_");
	e->UpdateSpeed(v);
}


//-------------------------------------------------------------------------

	EditWindow::~EditWindow()
{
}

//-------------------------------------------------------------------------

void	EditWindow::MessageReceived(BMessage *b)
{
	EditView *ed = (EditView *)FindView("edit view");
	
	ed->DoMessage(b);
}

//-------------------------------------------------------------------------

void	EditView::do_zero(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	length;

	SaveUndo();
	p = cache->p_samples();
	length = cache->Length();

	if (end > length)
		end = length;

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);

	for (i = s; i < e; i++)
		p[i] = 0;	

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::do_echo(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	sum;
	float	sum1;
	float	tmp;
	float	length;
	float	f1,f2;
	long	dt;
	float	intens;

	f1 = g_intens;
	f2 = g_delay;

	do_ask_filter1(Window(), "Echo", &f1, &f2);

	if (f1 < 0 || f2 < 0)
		return;

	SaveUndo();
	g_intens = f1;
	g_delay = f2;


	intens = f1 / 100.0;

	p = cache->p_samples();

	length = cache->Length();


	if (end > length)
		end = length;


	dt = (int)(g_delay * (44100.0/1000.0));

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);

	if (s < dt)
		s = dt;

	sum = 0;

	for (i = s; i < e; i++) 
		sum += abs(p[i]);

	for (i = s; i < e; i++) {
		p[i] = (int)(((float)p[i] + ((float)p[i - dt]*intens)) * (1.0/(1.0+intens)));	
	}
	
	sum1 = 0;
	for (i = s; i < e; i++) 
		sum1 += abs(p[i]);

	sum1 += 1;

	for (i = s; i < e; i++)  {
		tmp = p[i];
		tmp *= (sum/sum1);	
		if (tmp > 32760) tmp = 32760;
		if (tmp < -32760) tmp = -32760;
		p[i] = (int)tmp;
	}

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::do_metal(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	acc;
	long	d1,d2,d3,d4;
	float	sum;
	float	sum1;
	float	tmp;
	float	length;

	SaveUndo();
	
	p = cache->p_samples();

	length = cache->Length();

	if (end > length)
		end = length;


	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);

	if (s < 1711) s = 1711;

	d1 = 951;
	d2 = 1131;
	d3 = 1337;
	d4 = 1711;

	sum = 0;

	for (i = s; i < e; i++) 
		sum += abs(p[i]);

	
	for (i = s; i < e; i++) {
	/*
		if ((i & 0x1f) == 0) {
			d1 = 7111 + ((i / 32) % 128);
			d2 = 4113 + ((i / 32) % 80);
			d3 = 5833 + ((i / 32) % 47);
			d4 = 5833 + ((i / 32) % 1220);
		}
	*/
		acc = p[i];
		acc += (float)p[i-d1]*0.8;
		acc *= (0.9/(1.0+0.8));
		p[i] = (int)acc;
	}

	sum1 = 0;
	
	for (i = s; i < e; i++) 
		sum1 += abs(p[i]);

	sum1 += 1;

	for (i = s; i < e; i++)  {
		tmp = p[i];
		tmp *= (sum/sum1);	
		if (tmp > 32760) tmp = 32760;
		if (tmp < -32760) tmp = -32760;
		p[i] = (int)tmp;
	}

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::do_chorus(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	acc;
	long	d1,d2,d3,d4;
	float	sum;
	float	sum1;
	float	tmp;
	float	k = 0;
	float	length;
	float	f1,f2;

	f1 = g_intens;
	f2 = g_delay;

	do_ask_filter1(Window(), "Chorus", &f1, &f2);

	if (f1 < 0 || f2 < 0)
		return;

	SaveUndo();
	
	g_intens = f1;
	g_delay = f2;

	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);

	f1 = f1 / 1000.0;
	f1 = f1 * 44100.0;

	k = 0;

	d1 = (int)((0.3 + (sin(k*0.8)*0.02)) * f1);
	d2 = (int)((0.5 + (sin(k*1.1)*0.02)) * f1);
	d3 = (int)((0.7 + (sin(k*0.75)*0.02)) * f1);
	d4 = (int)((0.9 + (sin(k*1.4)*0.03)) * f1);
	
	if (s < d4) s = d4;

	sum = 0;

	for (i = s; i < e; i++) 
		sum += abs(p[i]);

	
	for (i = e; i > s; i--) {
		if ((i & 3) == 0) {
			d1 = (int)((0.3 + (sin(k*0.8)*0.02)) * f1);
			d2 = (int)((0.5 + (sin(k*1.1)*0.02)) * f1);
			d3 = (int)((0.7 + (sin(k*0.75)*0.02)) * f1);
			d4 = (int)((0.9 + (sin(k*1.4)*0.03)) * f1);
			k += 0.003;
		}

		acc = p[i];
		acc += (float)p[i-d1]*0.9;
		acc += (float)p[i-d2]*0.9;
		acc += (float)p[i-d3]*0.9;
		acc += (float)p[i-d4]*0.9;
		acc *= (1.0/(1.0+(0.9*3.8)));
		p[i] = (int)acc;
	}

	sum1 = 0;
	for (i = s; i < e; i++) 
		sum1 += abs(p[i]);

	sum1 += 1;


	for (i = s; i < e; i++)  {
		tmp = p[i];
		tmp *= (sum/sum1);	
		if (tmp > 32760) tmp = 32760;
		if (tmp < -32760) tmp = -32760;
		p[i] = (int)tmp;
	}

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}

//-------------------------------------------------------------------------


void	EditView::do_maxvolume(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	acc;
	float	sum;
	float	sum1;
	float	tmp;
	float	k = 0;
	float	length;


	SaveUndo();
	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);

	float	max;

	max = 0;

	for (i = s; i < e; i++)  {
		if (abs(p[i])>max)
			max = abs(p[i]);
	}
	k = 32760.0/max;
	
	for (i = s; i < e; i++) {
		p[i] = (int)(p[i] * k);
	}

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::do_compress(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	acc;
	float	sum;
	float	sum1;
	float	tmp;
	float	tmp1;
	float	length;
	float	gain;
	float	avg;
	float	f1;

	f1 = g_intens;

	do_ask_strength(Window(), "Compress", &f1);
	
	if (f1 < 0)
		return;
	
	SaveUndo();
	g_intens = f1;

	f1 /= 100.0;

	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);


	gain = 1.0;
	avg = 8000.0;

	for (i = s; i < e; i++) {
		tmp = (float)p[i];
		
		avg = (avg * 255.0) + fabs(tmp);
		avg /= 256.0;

		gain = 8000.0/avg;
		
		tmp1 = tmp * gain;
		tmp1 = (tmp1 * f1) + tmp;

		tmp = tmp1 / (1.0+f1);
		if (tmp > 32767)
			tmp = 32767;
		if (tmp < -32767)
			tmp = -32767;
		p[i] = (int)tmp;
	}

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::do_volume(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	tmp;
	float	tmp1;
	float	length;
	float	f1;

	f1 = g_intens;

	do_ask_volume(Window(), "Volume", &f1);
	
	if (f1 < 0)
		return;
	
	SaveUndo();
	g_intens = f1;

	f1 /= 100.0;

	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);



	for (i = s; i < e; i++) {
		tmp = (float)p[i];
		
		tmp = tmp * f1;

		if (tmp > 32767)
			tmp = 32767;
		if (tmp < -32767)
			tmp = -32767;
		p[i] = (int)tmp;
	}

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::do_fade(float start, float end)
{
	short	*p;
	long	s,e;
	long	i;
	float	tmp;
	float	tmp1;
	float	length;
	float	f1;
	double	f0;
	double	df;

	f1 = g_intens;

	do_ask_volume(Window(), "Final", &f1);
	
	if (f1 < 0)
		return;
	
	SaveUndo();
	g_intens = f1;

	f1 /= 100.0;

	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);

	f0 = 1.0;
	df = (f0 - f1);
	df /= (float)(e-s);
	for (i = s; i < e; i++) {
		tmp = (float)p[i];
		tmp = tmp * f0;

		if (tmp > 32760) tmp = 32760;
		if (tmp < -32760) tmp = -32760;

		f0 -= df;
		p[i] = (int)tmp;
	}

	dirty = 1;
	Render();Invalidate();
	main_refresh();
}


//-------------------------------------------------------------------------

void	EditView::SaveUndo()
{
	float	length = cache->Length();
	long	l;
	short	*p = cache->p_samples();

	undo_state = 1;
	l = (int)(length * 44100.0);

	if (l != undo_length) {
		free((char *)undo);
		undo = 0;
	}

	if (undo == 0) {
		undo = (short *)malloc(l*sizeof(short));
		undo_length = l;

	}

	memcpy(undo, p, l*sizeof(short)); 
	((EditWindow *)Window())->undo_itm->SetLabel("Undo");
	((EditWindow *)Window())->undo_itm->SetEnabled(true);
}

//-------------------------------------------------------------------------

void	EditView::Undo()
{
	float			length = cache->Length();
	long			current_length;
	long			l;
	short			*p = cache->p_samples();
	long			i;
	short			tmp;		
	long			cnt;

	if (undo) {
		current_length = (int)(length * 44100.0);

		undo_state ^= 0x01;

		l = undo_length;

		//printf("undo len = %ld, cur len = %ld\n", undo_length, current_length);

		if (current_length != undo_length) {
			if (!cache->Resize(undo_length / 44100.0)) {
				return;
			}
			p = cache->p_samples();
		}
	
		cnt = undo_length;

		if (undo_length < current_length) {

short	*ptmp;
			//printf("undo too small\n");
			ptmp = (short *)malloc(sizeof(short) * current_length);
			//printf("new undo is %ld\n", current_length);
			//printf("copy old undo %ld\n", undo_length);
			memcpy(ptmp, undo, undo_length * sizeof(short));
			free((char *)undo);
			undo = ptmp;
			undo_length = current_length;
		}

		for (i = 0; i < cnt; i++) {
			tmp = undo[i];
			undo[i] = p[i];
			p[i] = tmp;
		}

		dirty = 1;
		Render();Invalidate();
		main_refresh();
		if (undo_state == 0)
			((EditWindow *)Window())->undo_itm->SetLabel("Redo");
		else
			((EditWindow *)Window())->undo_itm->SetLabel("Undo");
	}
}

//-------------------------------------------------------------------------

void	EditView::do_clear(float start, float end)
{
	long	s,e;
	float	length;
	long	tot_cnt;
	short	*p;
	
	SaveUndo();

	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);
	tot_cnt = (int)(length * 44100.0);

	memmove((char *)(p + s),
			(char *)(p + e),
			sizeof(short) * (tot_cnt - e));

	memset((char *)(p + tot_cnt - (e-s)),
		   0,
		   (e-s) * sizeof(short));

	dirty = 1;

eTimeView	*ti;

	ti = (eTimeView *)Window()->FindView("edit_time");
	ti->eChangeSelection(s, s, true);
	ti->Draw(BRect(0,0,32000,32000));
	Render();Invalidate();

	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::do_cut(float start, float end)
{
	long	s,e;
	float	length;
	long	tot_cnt;
	short	*p;
	
	SaveUndo();

	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);
	tot_cnt = (int)(length * 44100.0);

	the_clip.SetClip((p + s), (e-s));

	memmove((char *)(p + s),
			(char *)(p + e),
			sizeof(short) * (tot_cnt - e));

	memset((char *)(p + tot_cnt - (e-s)),
		   0,
		   (e-s) * sizeof(short));

	dirty = 1;

eTimeView	*ti;

	ti = (eTimeView *)Window()->FindView("edit_time");
	ti->eChangeSelection(s, s, true);
	ti->Draw(BRect(0,0,32000,32000));
	
	Render();Invalidate();

	main_refresh();

}

//-------------------------------------------------------------------------

void	EditView::do_copy(float start, float end)
{
	long	s,e;
	float	length;
	long	tot_cnt;
	short	*p;
	
	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);
	tot_cnt = (int)(length * 44100.0);

	the_clip.SetClip((p + s), (e-s));
}

//-------------------------------------------------------------------------

void	EditView::do_paste(float start, float end)
{
	long	s,e;
	float	length;
	long	tot_cnt;
	short	*p;
	short	*clip_p;
	long	clip_cnt;

	SaveUndo();

	length = cache->Length();

	if (end > length)
		end = length;

	p = cache->p_samples();

	s = (int)(start * 44100.0);
	e = (int)(end * 44100.0);
	tot_cnt = (int)(length * 44100.0);

	memmove((char *)(p + s),
			(char *)(p + e),
			sizeof(short) * (tot_cnt - e));			//Clear the current selection
													//by moving it left.


	memset((char *)(p + tot_cnt - (e-s)),			//Clear end of buffer.
		   0,
		   (e-s) * sizeof(short));



	tot_cnt -= (e-s);								//Update Length

	//printf("new length= %ld\n", tot_cnt);

	clip_p = the_clip.GetClip(&clip_cnt);

	//printf("clip cnt = %ld\n", clip_cnt);

	if (!cache->Resize((tot_cnt/44100.0) + (clip_cnt / 44100.0)))	//Resize cache to get enough space for the clipboard
		return;

	//printf("resize cache to %ld\n", tot_cnt + clip_cnt);

	p = cache->p_samples();
	length = cache->Length();
	
	//printf("new length of cache = %ld\n", (long)(length * 44100.0));

	//printf("move to space for clip from %ld to %ld with cnt=%ld\n", (s), (s+clip_cnt), tot_cnt-e);
	memmove((char *)(p + s + clip_cnt),
			(char *)(p + s),
			(tot_cnt - e) * sizeof(short));

	//printf("copy clip to %ld with cnt = %ld\n", s, clip_cnt);
	
	memcpy((char *)(p + s),
		   (char *)clip_p,
		   clip_cnt * sizeof(short));

	dirty = 1;
	
	Render();Invalidate();

	main_refresh();
}

//-------------------------------------------------------------------------

void	EditView::DoMessage(BMessage *b)
{
	eTimeView	*tv = (eTimeView *)Window()->FindView("edit_time");

	float	start, end;

	start = tv->select_start;
	end = tv->select_end;

	switch(b->what) {
		case ZERO		:do_zero(start, end);		break;
		case ECHO		:do_echo(start, end);		break;
		case METAL		:do_metal(start, end);		break;
		case CHORUS		:do_chorus(start, end);		break;
		case MAX_VOLUME	:do_maxvolume(start, end);	break;
		case COMPRESS	:do_compress(start, end);	break;
		case VOLUME		:do_volume(start, end);		break;
		case UNDO		:Undo();					break;
		case CLEAR		:do_clear(start, end);		break;
		case CUT		:do_cut(start, end);		break;
		case PASTE		:do_paste(start, end);		break;
		case COPY		:do_copy(start, end);		break;
		case FADE		:do_fade(start, end);		break;
	}
}
