#include <Window.h>
#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <Debug.h>
#include <Application.h>
#include <MessageFilter.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollBar.h>
#include <Bitmap.h>
#include <Entry.h>
#include <OS.h>
#include <PlaySound.h> 
#include <Screen.h>
#include "global.h"
//----------------------------------------------------------------

#define	HS	480
#define	VS	350

//-----------------------------------------------------------
extern		void	do_about_sound();
extern		void	set_channel_f(long c, float f);
extern		char	ab_mode;
BWindow		*		pdad;
//-----------------------------------------------------------

typedef	struct {
	long	x;
	long	y;
	short	dx;
	short	dy;
	uchar	v;
} pix;

//-----------------------------------------------------------

class	TAboutView	: public BView {
		long		v;
		BBitmap		*poff;
		BBitmap		*off;
		BView		*off_view;
		pix			*parray;
		pix			*parray_cp;
		long		pcount;
		uchar		fade[256];
		char		gm;
		char		valid;
		long		mode;
		long		map;
		long		phase;
public:
				TAboutView(BRect frame, char *name);
				~TAboutView();
	
virtual	void	Draw(BRect r);
virtual	void	MouseDown(BPoint p);
		void	pre_render();
		void	do_anim();
		void	do_scroll();
		void	do_standby();
		void	do_anim0(float k);
		long	count_pixels();
		void	fill_parray();
		void	reverse();
		void	do_fade(uchar *p1);
virtual	void	KeyDown(const char *p, long n);
		void	upd_map(long n);
};

//----------------------------------------------------------------

class TAboutWindow : public BWindow {

public:
				TAboutWindow(BRect, const char*);
				~TAboutWindow();
		void	do_anim();
		void	do_scroll();
		void	do_standby();

		TAboutView		*main_view;
};

//-----------------------------------------------------------
TAboutWindow	*a_w = 0;
//-----------------------------------------------------------

void	TAboutView::pre_render()
{
	long	i;

	off->Lock();
	off_view->SetHighColor(0,0,0);
	off_view->FillRect(BRect(0,0,32000,32000));
	off_view->SetHighColor(255,0,0);
	off_view->SetFontSize(128);
	off_view->SetDrawingMode(B_OP_OVER);
	off_view->MovePenTo(BPoint(30, VS/2 + 15));
	off_view->DrawString("3DmiX");

	for (i = 20; i < 230*2; i++) {
		off_view->SetHighColor(0,i/20,i/2);
		off_view->MovePenTo(BPoint(i, VS/2 + 25));
		off_view->StrokeLine(BPoint(i, VS/2 + 55));
	}

	off_view->Sync();
	off->Unlock();
}


//-----------------------------------------------------------

short	mod(float v)
{
	v = v * 0.85;
	v -= 1;
	if (v < 0) v = 0;

	return v;
}

//-----------------------------------------------------------

short	mod1(float v)
{
	v = v * 0.92;
	v -= 1;
	if (v < 0) v = 0;

	return v;
}

//-----------------------------------------------------------

	TAboutView::TAboutView(BRect frame, char *name) :
	BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	BScreen	s;
	long	i;

	off = new BBitmap(BRect(0, 0, HS-1, VS-1),
						  	B_COLOR_8_BIT,
						  	TRUE);
	
	off->AddChild(off_view = new BView(BRect(0,0,HS,VS),
									  "",
									  B_FOLLOW_ALL,
									  B_WILL_DRAW));
	
	poff = new BBitmap(BRect(0, 0, HS-1, VS-1),
						  	B_COLOR_8_BIT,
						  	TRUE);
	
	gm = 0;
	SetViewColor(0,0,0);
	pre_render();
	pcount = count_pixels();
	parray = (pix *)malloc(sizeof(pix) * pcount);
	parray_cp = (pix *)malloc(sizeof(pix) * pcount);
	fill_parray();


	mode = 0;
	map = 0;

rgb_color	c;
rgb_color	c1;
long		j;
long		l1, l2;

	for (i = 0; i < 256; i++) {
		c = s.ColorForIndex(i);
		c1.red = mod(c.red);
		c1.green = mod(c.green);
		c1.blue = mod(c.blue);
		j = s.IndexForColor(c1);
		c1 = s.ColorForIndex(j);
		l1 = c.red + c.green + c.blue;
		l2 = c1.red + c1.green + c1.blue;

		if (l2 >= l1) {
			j = i / 24.0;
		}
		fade[i] = j;
	}
	valid = 1;
}

//----------------------------------------------------------------

	TAboutView::~TAboutView()
{
	valid = 0;
	a_w = 0;
	snooze(500000);
	delete off;
	delete poff;
	delete parray;
	delete parray_cp;
}

//----------------------------------------------------------------

void	TAboutView::Draw(BRect r)
{
	DrawBitmap(poff, BPoint(0,0));
}

//----------------------------------------------------------------

void	TAboutView::upd_map(long n)
{
	BScreen		s;
	rgb_color	c;
	rgb_color	c1;
	long		j;
	long		i;
	long		l1, l2;

	if (n == 0) {
		for (i = 0; i < 256; i++) {
			c = s.ColorForIndex(i);
			c1.red = mod(c.red);
			c1.green = mod(c.green);
			c1.blue = mod(c.blue);
			j = s.IndexForColor(c1);
			c1 = s.ColorForIndex(j);
			l1 = c.red + c.green + c.blue;
			l2 = c1.red + c1.green + c1.blue;

			if (l2 >= l1) {
				j = i / 24.0;
			}
			fade[i] = j;
		}
	}
/*
	if (n == 1) {
		for (i = 0; i < 256; i++) {
			c = s.ColorForIndex(i);
			c1.red = mod1(c.red);
			c1.green = mod1(c.green);
			c1.blue = mod1(c.blue);
			j = s.IndexForColor(c1);
			c1 = s.ColorForIndex(j);
			l1 = c.red + c.green + c.blue;
			l2 = c1.red + c1.green + c1.blue;

			if (l2 >= l1) {
				j = i / 24.0;
			}
			fade[i] = i;
			//fade[i] = j;
		}
	}
*/
}

//----------------------------------------------------------------

void	TAboutView::KeyDown(const char *p, long n)
{
	if (*p == 0x09)
		mode++;

	mode %= 3;

	if (*p == 0x20) {
		map++;
		map %= 2;
		//upd_map(map);
	}
}

//----------------------------------------------------------------

void	TAboutView::MouseDown(BPoint where)
{
	float	nx,ny;
	float	dx,dy;
	long	i;
	long	start;
	long	step;
	long	d;

	start = rand() % 5;
	step  = 2 + rand() % 4;
	nx = where.x;
	ny = where.y;
	

	Window()->Unlock();
	
	for (i = start; i < pcount; i+=step) {
		dx = -((parray[i].x>>8) - nx);
		dy = -((parray[i].y>>8) - ny);

		d = 50 + rand() % 50;
		dx /= d;
		dy /= d;

		dx += (rand()%1000 - 500)/6500.0;
		dy += (rand()%1000 - 500)/6500.0;
		parray[i].dx = dx*256;
		parray[i].dy = dy*256;
	}

	gm = 1;

ulong	but;

	
	do {
		snooze(15000);

		Window()->Lock();
		GetMouse(&where, &but);
		Window()->Unlock();
long	n;

		for (i = 0; i < 80; i++) {
			n = ((rand() + rand()) % pcount);
			n = abs(n);
			parray[n].x = where.x*256;
			parray[n].y = where.y*256;
			parray[n].dx *= 0.2;
			parray[n].dy *= 0.2;
		}
	} while(but);
	Window()->Lock();
}

//----------------------------------------------------------------
long	u = 0;
short	vr[0x1000];
long	mapx[HS];
long	mapy[VS];

//----------------------------------------------------------------

void	TAboutView::fill_parray()
{
	long	x,y;
	long	i;
	uchar	*p;
	uchar	v;
	long	cnt = 0;
	float	d;

	for (y = 0; y < VS; y++) {
		p = (uchar *)off->Bits();
		p += (y * HS);
		for (x = 0; x < HS; x++) {
			v = *p++;
			if (v) {
				parray_cp[cnt].x = parray[cnt].x = x*256;
				parray_cp[cnt].y = parray[cnt].y = y*256;
				d = sin(x/10.0) + ((rand()%1000)-500.0)/3000.0;
				parray_cp[cnt].dx = parray[cnt].dx = d*256;
				d = cos(x/10.0) + ((rand()%1000)-500.0)/3000.0;
				parray_cp[cnt].dy = parray[cnt].dy = d*256;
				parray_cp[cnt].v = parray[cnt].v = v;
				cnt++;
			}
		}
	}
	reverse();

	p = (uchar *)poff->Bits();
	memset(p, 0, HS*VS);
}


//----------------------------------------------------------------

long	TAboutView::count_pixels()
{
	long	x,y;
	uchar	*p;
	ulong	v;
	long	cnt = 0;

	for (y = 0; y < VS; y++) {
		p = (uchar *)off->Bits();
		p += (y * HS);
		for (x = 0; x < HS; x++) {
			v = *p++;
			if (v) {
				cnt++;
			}
		}
	}
	return cnt;
}

//----------------------------------------------------------------

void	TAboutView::reverse()
{
	long	i;
	uchar	*p1;
	uchar	*p2;
	short	x,y;

	for (i = 0; i < pcount; i++) {
		parray[i].x -= 120.0 * parray[i].dx;
		parray[i].y -= 120.0 * parray[i].dy;
skip:;	
	}
}


//----------------------------------------------------------------

void	TAboutView::do_fade(uchar *p1)
{
	long	i;
	uchar	pv;
	ulong	v;

	phase++;
	if (map == 1) {
		if ((phase & 0x03) != 0)
			return;
	}


	for (i = 0; i < HS*VS/4; i++) {
		pv = *p1;
		v = *((ulong *)p1);
		if (v) {
#if defined(__POWERPC__) || defined(__ARMEB__)	/* FIXME: This should probably use <endian.h> for the right define */
			*p1++ = fade[(v>>24) & 0xff];
			*p1++ = fade[(v>>16) & 0xff];
			*p1++ = fade[(v>>8) & 0xff];
			*p1++ = fade[v&0xff];
#endif
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
			*p1++ = fade[v&0xff];
			*p1++ = fade[(v>>8) & 0xff];
			*p1++ = fade[(v>>16) & 0xff];
			*p1++ = fade[(v>>24) & 0xff];
#endif
		}
		else
			p1+=4;
	}
}


//----------------------------------------------------------------

void	TAboutView::do_anim0(float k)
{
	long	i;
	uchar	*p1;
	uchar	*p2;
	short	x,y;
	uchar	pv;
	pix		*pp;

	if (off->Lock()) {
		p1 = (uchar *)poff->Bits();
		do_fade(p1);
		
		pp = parray;

		p1 = (uchar *)poff->Bits();
		for (i = 0; i < pcount; i++) {
			x = (int)pp->x>>8;
			if (x < 0) goto skip;
			if (x >= HS) goto skip;
			
			y = (int)pp->y>>8;
			if (y < 0) goto skip;
			if (y >= VS) goto skip;


			p2 = p1 + (y*HS) + x;
			*p2 = pp->v;	
	skip:;	
			pp->x += pp->dx;
			pp->y += pp->dy;
			pp++;
		}
		off->Unlock();
	}
}

//----------------------------------------------------------------

void	TAboutWindow::do_scroll()
{
	main_view->do_scroll();
}

//----------------------------------------------------------------

void	TAboutWindow::do_standby()
{
	main_view->do_standby();
}

//----------------------------------------------------------------
char	text[] = "3DmiX, Copyright 1999 Be Inc. Written by Benoit Schillings...  Many thanks to Doug Wright, Baron Arnold, Pavel Cisler, Ficus Kirkpatrick, Brian Swetland, Jon Watte, and Zippy... Now, try to click in the Window";

void	TAboutView::do_scroll()
{
	long	hp;
	uchar	*p1;
	uchar	*p2;
	long	p;

	p = 0;

	if (!valid) {
		ab_mode = 3;
		return;
	}

	if (Window()->Lock()) {
		p1 = (uchar *)poff->Bits();
		p2 = (uchar *)off->Bits();
		Window()->Unlock();
	}
	else {
		ab_mode = 3;
		return;
	}

	hp = 20;

	while(valid && hp > -1290) {
		p++;
		//if (p % 10 == 0) {
			//pdad->Lock();
			//pdad->UpdateIfNeeded();
			//pdad->Unlock();
		//}

		if (gm)
			return;

		if (hp < -200)
			do_anim0(0);
		else
			snooze(20000);

		if (!valid) {
			ab_mode = 3;
			return;
		}
		if (off_view->Window()->Lock()) {
			memcpy(p2, p1, HS*VS);
			off_view->MovePenTo(BPoint(hp, VS*0.8));
			off_view->SetFontSize(18);
			off_view->SetHighColor(90,95,200);
			off_view->DrawString(text);
			off_view->Sync();
			off_view->Window()->Unlock();
		}
		else
			return;
		hp -= 3;
		if (!valid) {
			ab_mode = 3;
			return;
		}
		if (Window()->Lock()) {
			DrawBitmap(off, BPoint(0,0));
			snooze(10000);
			Window()->Unlock();
		}
		else
			return;
	}
}

//----------------------------------------------------------------

void	TAboutView::do_standby()
{
	long	hp;
	long	i;
	long	p;
	long	p1;
	long	from, to;

	p = 0;
	while(valid) {
		ab_mode = 2;
		p++;
		do_anim0(0);
		//if (p % 10 == 0) {
			//pdad->Lock();
			//pdad->UpdateIfNeeded();
			//pdad->Unlock();
		//}

		if (Window()->Lock()) {
			DrawBitmap(poff, BPoint(0,0));
			Window()->Unlock();
			snooze(10000);
			if (mode == 0) {
				for (i = 0; i < 27; i++) {
					set_channel_f(i, (400 - parray[i*17].y/256)/50.0);
				}
			}
			if (mode == 1) {
				for (i = 0; i < 27; i++) {
					set_channel_f(i, (400 - parray[i*17].x/256)/50.0);
				}
			}
			if (mode == 2) {
				for (i = 0; i < 27; i++) {
					set_channel_f(i, (parray[i*17].x/256 + parray[i*17].y/256)/50.0);
				}
			}
			
			p1 = p % 16;
			from = (pcount / 16)*p1;
			to = from + (pcount / 16);
			
			for (i = from; i < to; i++) {
				parray[i].dx *= (0.992*0.992*0.992);
				parray[i].dy *= (0.992*0.992*0.992);
			}
		}
		else {
			goto out;
		}
	}
out:;
	ab_mode = 3;
}


//----------------------------------------------------------------

void	TAboutView::do_anim()
{
	float	k;
	long	i;
	long	j;
	float	t0,t1;
	float	dt;


	k = 0.1;

	for (i = 0; i < 121; i++) {
		//if (i % 20 == 0) {
			//pdad->Lock();
			//pdad->UpdateIfNeeded();
			//pdad->Unlock();
		//}

		if (i == 120) {
			parray = parray_cp;
			parray_cp = 0;
		}

		t0 = system_time();
		if (Window()->Lock()) {
			do_anim0(k);
			DrawBitmap(poff, BPoint(0,0));
			t1 = system_time();
			dt = (t1-t0);
			dt = ((ABOUT_TIME1*1000000.0)/120.0) - dt;
			if (dt > 50000)
				dt = 50000;	
			if (dt > 0)
				snooze(dt);
			Window()->Unlock();
		}
		if (valid == 0)
			goto out;
	}

out:;
}

//----------------------------------------------------------------


void	TAboutWindow::do_anim()
{
	main_view->do_anim();
}

//----------------------------------------------------------------

TAboutWindow::TAboutWindow(BRect r, const char* t)
			 :BWindow(r, t, B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	main_view = new TAboutView(BRect(0,0,1000,1000), "about_view");
	AddChild(main_view);
	main_view->MakeFocus();
} 

//----------------------------------------------------------------

TAboutWindow::~TAboutWindow()
{
	a_w = 0;
}

//----------------------------------------------------------------

long	xabout_p(void *p)
{
	ab_mode = 1;
	do_about_sound();
	if (a_w)
	a_w->do_anim();
	if (a_w)
		a_w->do_scroll();
	ab_mode = 0;
	if (a_w)
		a_w->do_standby();

	ab_mode = 3;
}

//----------------------------------------------------------------

void	do_about(BWindow *dad)
{
	BEntry		*entry;
	entry_ref	ref;

	pdad = dad;
	if (a_w) {
		a_w->Lock();
		a_w->Activate();
		a_w->Unlock();
	}
	else {
		a_w = new TAboutWindow(BRect(280, 200, 280 + HS, 200 + VS), "About (v1.1)");
		
		a_w->Lock();
		a_w->Show();
		a_w->UpdateIfNeeded();
		a_w->Unlock();
		resume_thread(spawn_thread(xabout_p,"about_p",B_DISPLAY_PRIORITY,a_w));
	}
}

//----------------------------------------------------------------
