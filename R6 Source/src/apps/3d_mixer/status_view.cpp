#include <Window.h>
#include "status_view.h"
#include "global.h"
//-----------------------------------------------------------------------

extern	char	in_h_move;
extern	char	sub_size;

StatusView	*vstat =  0;

//-----------------------------------------------------------------------
#define	V_FULL	24000.0
#define	V_TOP	30
//-----------------------------------------------------------------------

void	v_meter(long level1, long level2)
{
	if (vstat == 0)
		return;

	vstat->Window()->Lock();
	vstat->SetLevel(level1, level2);
	vstat->Window()->Unlock();
	
}

//-----------------------------------------------------------------------

	StatusView::StatusView(BRect frame, char *name) :
		  BView(frame, name, B_FOLLOW_NONE,B_WILL_DRAW)
{
	SetViewColor(S_COLOR_R,S_COLOR_G,S_COLOR_B);
	level1 = 0;
	level2 = 0;
	vstat = this;

  	slider = new TSlider(BRect(12, 200, 51, 200+S_VS+35),"name", 50.0);

	AddChild(slider);
	dv = 0;
}


//-----------------------------------------------------------------------

	StatusView::~StatusView()
{
}

//-----------------------------------------------------------------------
extern	rgb_color	pmap(rgb_color);

#define	MAKE_C(r,g,b)	{c.red = r;c.green = g;c.blue = b;c=pmap(c);return c;}
//-----------------------------------------------------------------------

rgb_color	StatusView::calc_color(long i)
{
	rgb_color	c;

	switch(i) {
		case	0:	MAKE_C(0,0,100);		//all blue
		case	1:	MAKE_C(0,0,115);
		case	2:	MAKE_C(0,0,127);
		case	3:	MAKE_C(0,0,139);
		case	4:	MAKE_C(0,0,151);
		case	5:	MAKE_C(0,0,163);
		case	6:	MAKE_C(0,0,175);
		case	7:	MAKE_C(0,0,197);
		case	8:	MAKE_C(0,0,209);
		case	9:	MAKE_C(0,0,221);
		case	10:	MAKE_C(0,0,233);
		case	11:	MAKE_C(0,0,245);
		case	12:	MAKE_C(0,0,255);
		/*
		case	13:	MAKE_C(35,30,220);			//blue to red
		case	14:	MAKE_C(60,30,180);
		case	15:	MAKE_C(85,30,130);
		case	16:	MAKE_C(110,30,90);
		case	17:	MAKE_C(135,30,50);
		*/
		case	13:	MAKE_C(0,0,255);			//blue to red
		case	14:	MAKE_C(0,0,255);
		case	15:	MAKE_C(0,0,255);
		case	16:	MAKE_C(110,30,90);
		case	17:	MAKE_C(135,30,50);
		
		case	18:	MAKE_C(160,30,30);			//all red
		case	19:	MAKE_C(170,10,10);
		case	20:	MAKE_C(190,0,0);
		case	21:	MAKE_C(210,0,0);
		case	22:	MAKE_C(230,0,0);
		case	23:	MAKE_C(255,0,0);
		case	24:	MAKE_C(255,0,0);
	}

}

//-----------------------------------------------------------------------

void	StatusView::SetLevel(float l1, float l2)
{
	long	v0,v1,v2,v3;

	max1 *= 0.95;
	max2 *= 0.95;

	if (l1 > max1)
		max1 = l1;
	if (l2 > max2)
		max2 = l2;

	level1 = (level1 + l1)/2.0;
	level2 = (level2 + l2)/2.0;

	phase++;

	if (max1 < 1000 && max2 < 1000) {
		if (phase & 0x03 != 0)
			return;
	}

	drawer();
}

//-----------------------------------------------------------------------

void	StatusView::drawer()
{
	BRect			r;
	long			i;
	float			h;
	float			m1;
	float			m2;
	

	if (sub_size == 0)
		return;

	m1 = level1/V_FULL;
	m2 = level2/V_FULL;

	m1 *= 24.0;
	m2 *= 24.0;

	for (i = 0; i < 24; i++) {
		r.top = V_TOP + ((24-i) * 5);
		r.bottom = r.top + 3;
		h = (63 * 0.333);
		r.left = h - 8;
		r.right = h + 8;

		if (i < m1) {
			SetHighColor(calc_color(i));
		}
		else
			SetHighColor(0, 80, 0);

		FillRect(r);
	}

	m1 = max1 / V_FULL;
	m1 *= 24.0;
	if (m1 > 23)
		m1 = 23;
	i = (int)m1;
	r.top = V_TOP + ((24-i) * 5);
	r.bottom = r.top + 3;
	SetHighColor(140, 140, 160);
	FillRect(r);

	

	for (i = 0; i < 24; i++) {
		r.top = V_TOP + ((24-i) * 5);
		r.bottom = r.top + 3;
		h = (63 * 0.333);

		if (i < m2) {
			SetHighColor(calc_color(i));
		}
		else
			SetHighColor(0, 80, 0);

	
		h = (63 * 0.666);
		r.left = h - 8;
		r.right = h + 8;
	
		FillRect(r);
	}
	
	m2 = max2 / V_FULL;
	m2 *= 24.0;
	if (m2 > 23)
		m2 = 23;
	i = (int)m2;
	r.top = V_TOP + ((24-i) * 5);
	r.bottom = r.top + 3;
	SetHighColor(140, 140, 160);
	FillRect(r);
	Sync();
}

//-----------------------------------------------------------------------

void	StatusView::Draw(BRect ur)
{
	SetDrawingMode(B_OP_COPY);
	
	drawer();

	SetDrawingMode(B_OP_OVER);
	SetHighColor(20,20,20);
	MovePenTo(BPoint(20, V_TOP + 27*5));
	DrawString("L");
	MovePenTo(BPoint(41, V_TOP + 27*5));
	DrawString("R");
}

//-----------------------------------------------------------------------

void	StatusView::MouseDown(BPoint p)
{
	BPoint	w;
	ulong	b;
	long	cdv;
	long	new_dv;
	long	delta;

	w = p;

	do {
		p = w;
		GetMouse(&w, &b);
		if (w != p) {
			cdv = (int)(-(w.y - p.y));
			if (cdv) {
				cdv *= 2;
				new_dv = dv + cdv;

				if (new_dv < -200)
					new_dv = -200;
				if (new_dv > 0)
					new_dv = 0;

				delta = new_dv - dv;

				dv = new_dv;

				ScrollBy(0,delta);
				GetMouse(&w, &b);
				p = w;
				Window()->UpdateIfNeeded();
			}
		}
		Window()->Unlock();
		snooze(25000);
		Window()->Lock();
	} while(b);
	
}

//-----------------------------------------------------------------------


