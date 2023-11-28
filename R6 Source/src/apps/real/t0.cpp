#include <Debug.h>

#include <string.h>
#include <math.h>
#include <OS.h>
#include <Screen.h>
#include <stdio.h>
#include <stdlib.h>
#include "top_view.h"

#ifndef _INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif

/*------------------------------------------------------------*/

#define	XS			800	
#define	YS			600

#define	SMALL_R		151
#define	BIG_R		438	

#define	SMALL_DR	(BIG_R + 40)

/*------------------------------------------------------------*/


float	ellipse_x1[BIG_R * 2];


/*------------------------------------------------------------*/

void	TopView::hline(long x1, long x2, long y1, ulong c, float d)
{
	ulong	*b;
	uchar	dd;

	if (y1 < 0) return;
	if (y1 >= (YS)) return;

	if (x1 < 1) x1 = 1;
	if (x2 >= (XS-1)) x2 = (XS-1);

	b = (ulong *)the_bitmap->Bits();
	b += (y1 * XS);
	b += x1;
	
	b--;

	dd = 140-(140 * d);

	*b++ = (dd<<16) | (dd<<8) | (dd<<0);
	while(x1 < x2) {
		*b++ =  c;
		x1++;
	}
	*b++ = (dd<<16) | (dd<<8) | (dd<<0);
}

/*------------------------------------------------------------*/


void	TopView::do_ellipse(long cx, long cy, ulong c)
{
	long	dy;
	float	xx;
	int		ixx;

	for (dy = 0; dy < BIG_R * 2; dy++) {
		xx = ellipse_x1[dy];
		ixx = xx;

		hline(cx - ixx,
			  cx + ixx,
			  dy + cy - BIG_R,
			  c, (xx-ixx));
	}
}

/*------------------------------------------------------------*/


void	init_ellipse()
{
	float	v;
	long	i;

	for (i = 0; i < BIG_R * 2; i++) {
		v = sqrt(BIG_R * BIG_R - (i-BIG_R) * (i-BIG_R));
		ellipse_x1[i] = v;
	}
}


/*------------------------------------------------------------*/

long	xtop_p(void *p)
{
	TopView	*pp;

	pp = (TopView *)p;

	pp->thread();

	return 0;
}

/*------------------------------------------------------------*/

TopView::TopView(BRect r, long flags) :
	BView(r, "", flags, B_WILL_DRAW | B_PULSE_NEEDED)
{
	BRect		a_rect;
	SndSrc		*src;

	init_ellipse();
	the_bitmap = new BBitmap(BRect(0, 0, XS - 1, YS-1),
					   		 B_RGB32, TRUE);

	mv = new BView(BRect(0, 0, XS, YS-1),
				   "", 0, B_WILL_DRAW);

	the_bitmap->AddChild(mv);
	
	a_rect.Set(5000+TRANSPORT_H_POS,
			   5000+TRANSPORT_V_POS,
			   5000+TRANSPORT_H_POS + TRANSPORT_H_SIZE,
			   5000+TRANSPORT_V_POS + TRANSPORT_V_SIZE);

	v_transport = new Transport(a_rect, B_FOLLOW_NONE | B_WILL_DRAW);
	
	src = new SndSrc("/boot/home/Desktop/sounds/castanets.wav");
	v_transport->SetSource(src);
	
	this->AddChild(v_transport);
	
	mode = NORMAL_ANIM;
	big_dh = 0;
	big_dv = 0;
	small_alpha = 4.1;
	
	DrawBits();
	resume_thread(spawn_thread(xtop_p,"top_p",B_NORMAL_PRIORITY,this));
}

/*------------------------------------------------------------*/

TopView::~TopView()
{
	delete the_bitmap;
}

/*------------------------------------------------------------*/


void	TopView::MouseDown(BPoint where)
{
	if (big_dh)
		mode = MOVE_DOWN;
	else
		mode = MOVE_UP;
}

/*------------------------------------------------------------*/

void	TopView::Draw(BRect upd)
{
	DrawBitmap(the_bitmap, BPoint(0,0));
}


/*------------------------------------------------------------*/


void	TopView::DrawBits()
{
	BRect	r;
	BBitmap	*b;
	BPoint	p;

	mv->Window()->Lock();
	mv->SetHighColor(153, 153, 153);
	mv->FillRect(mv->Bounds());

	mv->SetHighColor(0,0,0);

	p = GetSmallPos();

	r.Set(p.x - SMALL_R, p.y - SMALL_R, p.x + SMALL_R, p.y + SMALL_R);

	//mv->FillEllipse(r);



	p = GetBigPos();


	r.Set(p.x - BIG_R, p.y - BIG_R, p.x + BIG_R, p.y + BIG_R);

	mv->Sync();
	do_ellipse(p.x, p.y, 0xff000000);

	b = v_transport->get_image();

	p = GetSmallPos();
	
	mv->SetDrawingMode(B_OP_ALPHA);
	mv->DrawBitmap(b, BPoint(p.x - TRANSPORT_H_SIZE / 2 , p.y - TRANSPORT_V_SIZE / 2));
	mv->SetDrawingMode(B_OP_COPY);


	mv->Sync();
	
	mv->Window()->Unlock();
}

/*------------------------------------------------------------*/


void	TopView::UpdateTransport()
{
	BRect	r;
	BBitmap	*b;
	BPoint	p;

	mv->Window()->Lock();

	p = GetSmallPos();

	r.Set(p.x - SMALL_R, p.y - SMALL_R, p.x + SMALL_R, p.y + SMALL_R);

	mv->FillEllipse(r);
	mv->SetDrawingMode(B_OP_ALPHA);
	b = v_transport->get_image();
	mv->DrawBitmap(b, BPoint(p.x - TRANSPORT_H_SIZE / 2 , p.y - TRANSPORT_V_SIZE / 2));
	mv->SetDrawingMode(B_OP_COPY);
	mv->Sync();

	mv->Window()->Unlock();
}

/*------------------------------------------------------------*/

void	TopView::thread()
{
	long	i;
	double	s,e;

	while(1) {
		if (mode == NORMAL_ANIM) {
			if (Window()->Lock()) {
				if (v_transport->NeedUpdate()) {
					UpdateTransport();
					DrawBitmap(the_bitmap, GetSmallRect(), GetSmallRect());
					snooze(30000);
				}
				snooze(15000);
				Window()->Unlock();
			}
			else
				return;
		}

		if (mode == MOVE_UP) {
			s = system_time();
			for (i = 0; i < 15; i++) {
				small_alpha -= 0.063;
				big_dh -= 12;
				big_dv -= 8;
				if (Window()->Lock()) {
					//DrawBits();
					DrawBitmap(the_bitmap, BPoint(0,0));
					Window()->Unlock();
				}
				else
					return;
			}
			e = system_time();
			printf("dt=%f\n", (e-s)/1000.0);
			mode = NORMAL_ANIM;
		}

		if (mode == MOVE_DOWN) {
			for (i = 0; i < 15; i++) {
				small_alpha += 0.063;
				big_dh += 12;
				big_dv += 8;
				if (Window()->Lock()) {
					DrawBits();
					DrawBitmap(the_bitmap, BPoint(0,0));
					Window()->Unlock();
				}
				else
					return;
			}
			mode = NORMAL_ANIM;
		}
	}
}

/*------------------------------------------------------------*/

BPoint	TopView::GetSmallPos()
{
	BPoint	p;
	float	s;
	float	c;
	BPoint	p0;

	p0 = GetBigPos();

	s = sin(small_alpha) * (SMALL_DR) + p0.x;
	c = cos(small_alpha) * (SMALL_DR) + p0.y;

	p.x = s;
	p.y = c;

	return p;
}

/*------------------------------------------------------------*/

BPoint	TopView::GetBigPos()
{
	BPoint	p;

	p.x = 601 + big_dh;
	p.y = 600 - 150 + big_dv;

	return p;
}

/*------------------------------------------------------------*/


BRect	TopView::GetSmallRect()
{
	BRect	r;
	BPoint	p;
	
	p = GetSmallPos();

	r.top = p.y - 150;
	r.bottom = p.y + 150;
	r.left = p.x - 150;
	r.right = p.x + 150;

	return r;
}

/*------------------------------------------------------------*/
