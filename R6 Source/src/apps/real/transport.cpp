//******************************************************************************
//
//	File:		tsb.cpp
//
//	Copyright 1993-95, Be Incorporated
//
//
//******************************************************************************


#include <Debug.h>

#include <string.h>
#include <math.h>
#include <OS.h>
#include <Screen.h>
#include <stdio.h>
#include <stdlib.h>
#include "transport.h"
#include "top_view.h"

#ifndef _INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif


/*------------------------------------------------------------*/

#define	Q1			1
#define	Q2			2
#define	Q3			3
#define	Q4			4
#define	Q5			5
#define	Q6			6
#define	Q7			7
#define	Q8			8
#define	CENTER		9
#define	YELLOW		10

#define	CX			150
#define	CY			150

#define	pi2			(3.1415926*2)
/*------------------------------------------------------------*/

long	Transport::find_object(BPoint where)
{
	float	dist;
	float	x,y;


	x = where.x - CX;
	y = where.y - CY;

	dist = sqrt(x * x + y * y);
	
	if (dist < 32)
		return CENTER;

	if (dist > 90)
		return YELLOW;

	if (x > 0) {
		if (y < 0) {
			if (fabs(y) > fabs(x)) return Q1;
			return Q2;
		}
		if (fabs(y) > fabs(x))
			return Q4;
		return Q3;	
	}	
	
	if (x < 0) {
		if (y < 0) {
			if (fabs(y) > fabs(x)) return Q8;
			return Q7;
		}
		if (fabs(y) > fabs(x))
			return Q5;
		return Q6;	
	}		

	return -1;
}

/*------------------------------------------------------------*/

BPicture	*Transport::make_mask(long code)
{
	BPicture	*p;

	p = new BPicture();

	junk->Window()->Lock();

	junk->BeginPicture(p);

	junk->SetHighColor(0,0,0);

	switch(code) {
		case CENTER :
			junk->FillEllipse(BRect(CX-30, CY-30, CX+30, CY+30));break;
		case Q1 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), 45, 45);break;
		case Q2 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), 0, 45);break;
		case Q3 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), -45, 45);break;
		case Q4 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), -90, 45);break;
		case Q5 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), -135, 45);break;
		case Q6 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), -180, 45);break;
		case Q7 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), -225, 45);break;
		case Q8 :
			junk->FillArc(BRect(CX-87, CY-87, CX+87, CY+87), -270, 45);break;
	}

	junk->EndPicture();
	junk->Window()->Unlock();

	return p;
}

/*------------------------------------------------------------*/

void	Transport::copy_to_main(BBitmap *src, long obj_code)
{
	BPicture	*p;

	BuildGraphics();
	p = make_mask(obj_code);

	mv->Window()->Lock();
	mv->ClipToPicture(p, BPoint(0,0), 1);
	mv->SetDrawingMode(B_OP_ALPHA);
	mv->DrawBitmap(src, BPoint(0,0));
	mv->ConstrainClippingRegion(NULL);
	mv->SetDrawingMode(B_OP_COPY);
	mv->Window()->Unlock();
	delete p;
	
	if (obj_code != CENTER) {
		p = make_mask(CENTER);
		mv->Window()->Lock();
		mv->SetDrawingMode(B_OP_ALPHA);
		mv->ClipToPicture(p, BPoint(0,0), 1);
		mv->DrawBitmap(up_on, BPoint(0,0));
		mv->ConstrainClippingRegion(NULL);
		mv->SetDrawingMode(B_OP_COPY);
		mv->Window()->Unlock();
		delete p;
	}
}

/*------------------------------------------------------------*/

float	Transport::PosToAngle(BPoint p)
{
	float	x;
	float	y;
	float	d;
	float	alpha;

	x = p.x - CX;
	y = p.y - CY;

	d = sqrt(x * x + y * y);

	alpha = asin(x/d);

	if (y < 0)
		alpha = (3.1415926) - alpha;

	if (alpha < 0)
		alpha = alpha + (pi2);
	return alpha;	
}

/*------------------------------------------------------------*/

void	Transport::MoveYellow(float a0)
{
	yellow_pos = a0;
	BuildGraphics();
	dad->UpdateTransport();
}

/*------------------------------------------------------------*/

void	Transport::norm(float *p1, float *p2)
{
	float	delta;
	float	delta1;
	float	v0,v1;

	while(*p1 > pi2)
		*p1 -= pi2;

	while(*p2 > pi2)
		*p2 -= pi2;


	v0 = *p1;
	v1 = *p2;

	delta = v0 - v1;

	v0 += (pi2);

	delta1 = v0 - v1;

	if (fabs(delta1) < fabs(delta)) {
		*p1 = v0;
		return;
	}

	v0 = *p1;
	v1 = *p2;
	v1 += (pi2);
	delta1 = v0 - v1;

	if (fabs(delta1) < fabs(delta)) {
		*p2 = v1;
		return;
	}
}

/*------------------------------------------------------------*/


void	Transport::TrackYellow()
{
	BPoint	p0;
	BPoint	p1;
	ulong	button;
	float	a0;
	float	a1;

	dad->tGetMouse(&p0, &button);

	a0 = yellow_pos;
	
	do {
		dad->tGetMouse(&p1, &button);
		a1 = PosToAngle(p1);

		if (a1 != a0) {
			norm(&a0, &a1);
			a1 = (a0 + a1) / 2.0;
			MoveYellow(a1);
			a0 = a1;
		}
		else
			snooze(15000);
	} while(button);

	MoveYellow(a1);
}

/*------------------------------------------------------------*/

void	Transport::MouseDown(BPoint where)
{
	BPoint	where1;
	ulong	button;
	long	obj;
	long	obj1;
	char	state;
	char	state1;

	obj = find_object(where);
	if (obj < 0)
		return;
	
	if (obj == YELLOW) {
		TrackYellow();
		return;
	}
	
	SetDrawingMode(B_OP_OVER);

	
	copy_to_main(down_on, obj);
	dad->UpdateTransport();
	
	state = 1;

	do {
		dad->tGetMouse(&where1, &button);
		obj1 = find_object(where1);
		if (obj1 != obj)
			state1 = 0;
		else
			state1 = 1;

		if (state != state1) {
			if (state1)
				copy_to_main(down_on, obj);
			else
				copy_to_main(up_on, obj);
			dad->UpdateTransport();
			state = state1;
		}

		snooze(15000);
	} while(button);

	BuildGraphics();
	dad->UpdateTransport();
}

/*------------------------------------------------------------*/

long	xtransport_p(void *p)
{
	Transport	*pp;

	pp = (Transport *)p;

	pp->AnimateColor();

	return 0;
}

/*------------------------------------------------------------*/

Transport::Transport(BRect r, long flags, TopView *pdad) :
	BView(r, "", flags, B_WILL_DRAW | B_PULSE_NEEDED)
{
	up_on = load("/boot/home/Desktop/art/transport_up_on");
	down_on = load("/boot/home/Desktop/art/transport_down_on");
	circle = load("/boot/home/Desktop/art/circle");
	yellow = load("/boot/home/Desktop/art/yellow");
	

	dad = pdad;

	main = new BBitmap(BRect(0, 0, TRANSPORT_H_SIZE - 1, TRANSPORT_V_SIZE-1),
					   B_RGB32, TRUE);

	mv = new BView(BRect(0,0, TRANSPORT_H_SIZE-1, TRANSPORT_V_SIZE-1),
				   "", 0, B_WILL_DRAW);

	main->AddChild(mv);
	
	junk = new BView(BRect(1000,1000, 1000+TRANSPORT_H_SIZE-1, 1000+TRANSPORT_V_SIZE-1),
				   "", 0, B_WILL_DRAW);

	main->AddChild(junk);

	wave_color_index = 0;
	yellow_pos = 3.1415926;
	the_source = 0;

	DrawWave(wave_color_index / 200.0 , wave_color_index);
	BuildGraphics();

	resume_thread(spawn_thread(xtransport_p,"transport_p",B_NORMAL_PRIORITY,this));
}

/*------------------------------------------------------------*/

BBitmap	*Transport::get_image()
{
	return main;
}

/*------------------------------------------------------------*/

void	Transport::SetSource(SndSrc *src)
{
	the_source = src;

	DrawWave(wave_color_index / 200.0 , wave_color_index);
	BuildGraphics();

	if (this->Window()->Lock()) {
		dad->UpdateTransport();
		this->Window()->Unlock();
	}
}

/*------------------------------------------------------------*/


Transport::~Transport()
{
	delete up_on;
	delete down_on;
	delete main;
}

/*------------------------------------------------------------*/

void	Transport::AnimateColor()
{
	long	i;
	float	delta;

	while(!Window()->Lock()) {
		snooze(15000);
		printf("wait\n");
	}

	Window()->Unlock();

	while(1) {
		wave_color_index += 1;
		if (mv->Window()->Lock()) {
			DrawWave(wave_color_index / 200.0 , wave_color_index);
			mv->DrawBitmap(circle, BPoint(0,0));
			mv->Sync();	
			DrawYellow();
			mv->Window()->Unlock();
			if (this->Window()->Lock()) {
				need_update = 1;
				this->Window()->Unlock();
			}
			else {
				return;
			}
		}
		else
			return;

		snooze(42000);
	}
}

/*------------------------------------------------------------*/

void	Transport::BuildGraphics()
{
	char	*bits;

	mv->Window()->Lock();
	bits = (char*)main->Bits();
	memset(bits, 0x00, main->BytesPerRow() * 300); 
	
	BRect	r;

	r.top = 0;
	r.left = 0;
	r.right = 300;
	r.bottom = 300;
	mv->SetHighColor(0,0,0);
	mv->FillEllipse(r);

	//mv->SetHighColor(0,0,0);
	//mv->FillRect(Bounds());

	mv->SetDrawingMode(B_OP_ALPHA);
	mv->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	mv->DrawBitmap(up_on, BPoint(0,0));
	mv->DrawBitmap(circle, BPoint(0,0));
	DrawYellow();
	mv->Sync();
	mv->Window()->Unlock();
}

/*------------------------------------------------------------*/

void	Transport::Draw(BRect update_rect)
{
	need_update = 1;
	//DrawBitmap(main, BPoint(0,0));
}


/*------------------------------------------------------------*/

void	Transport::Pulse()
{
}

/*------------------------------------------------------------*/

BBitmap	*load(char *filename)
{
	long	ref;
	long	aref;
	uchar	header[0x20];
	long	cnt;
	ulong	v;
	uchar	*data0;
	uchar	*adata0;
	long	k;
	int		buffer_size_x;
	int		buffer_size_y;
	uchar	*src;
	uchar	*asrc;
	ulong	*bits;
	BBitmap	*im;
	long	y,x;
	char	buf[256];
	uchar	r,g,b, a;


	sprintf(buf, "%s.tga", filename);

	ref = open(buf, O_RDONLY);
	if (ref < 0)
		printf("failed %s\n", filename);
	
	sprintf(buf, "%s.mask", filename);

	aref = open(buf, O_RDONLY);
	if (aref < 0)
		printf("failed %s\n", filename);
	
	read(ref, &header, 0x12);

	buffer_size_x = header[0xc] + header[0xd]*256;	
	buffer_size_y = header[0xe] + header[0xf]*256;	

	read(aref, &header, 0x12);


	im = new BBitmap(BRect(0, 0, buffer_size_x-1, buffer_size_y-1),
					 B_RGB32);


	bits = (ulong*)im->Bits();

	data0 = (uchar *)malloc(buffer_size_x * buffer_size_y * 3);
	adata0 = (uchar *)malloc(buffer_size_x * buffer_size_y * 3);
	
	read(ref, data0, 3 * buffer_size_x * buffer_size_y);
	read(aref, adata0, 3 * buffer_size_x * buffer_size_y);

	k = 0;

	cnt = 0;
	
	if (header[0x11] == 0x20) {
		for (y = 0; y < buffer_size_y; y++) {
			src = &data0[y*buffer_size_x*3];
			asrc = &adata0[y*buffer_size_x*3];
			for (x = 0; x < (buffer_size_x*3); x+=3) {
				b = src[x];
				g = src[x+1];
				r = src[x+2];
				a = asrc[x+2];
				//if (a < 255 && a > 80)  a -= 80;

				v = a<<24 | ((ulong)r<<16) | ((ulong)g << 8) | (ulong)b;
				bits[cnt] = v;
				cnt++;
			}
		}
	}
	else {
		for (y = (buffer_size_y-1); y >= 0; y--) {
			src = &data0[y*buffer_size_x*3];
			asrc = &adata0[y*buffer_size_x*3];
			for (x = 0; x < (buffer_size_x*3); x+=3) {
				b = src[x];
				g = src[x+1];
				r = src[x+2];
				a = asrc[x+2];
				//if (a < 255 && a > 80)  a -= 80;
				v = a<<24 | ((ulong)r<<16) | ((ulong)g << 8) | (ulong)b;
				bits[cnt] = v;
				cnt++;
			}
		}
	}

	free((char *)data0);
	free((char *)adata0);

	close(ref);
	close(aref);
	return im;
}

/*------------------------------------------------------------*/

static ulong	colors[1050];
static char		colors_inited = 0;

/*------------------------------------------------------------*/


void	Transport::init_colors()
{
	uchar	r, g, b;
	long	i;

	for (i = 0; i < 525; i++) {
		r = (uchar)(i / 2.2);
		g = 100;
		b = (uchar)(255 - (i/2.2));

		colors[i] = 0xff << 24 | (r << 16) | (g << 8) | b;
	}
	for (i = 0; i < 525; i++) {
		r = (uchar)((525-i) / 2.2);
		g = 100;
		b = (uchar)(255 - ((525-i)/2.2));

		colors[i + 525] = 0xff << 24 | (r << 16) | (g << 8) | b;
	}
}

/*------------------------------------------------------------*/

float	fnorm(float v)
{
	long	k;

	k = (v + 0.003) / 0.006;

	return (k * 0.006);
}

/*------------------------------------------------------------*/


void	Transport::DrawWave(float delta, long c_idx)
{
	float	i;
	long	pcolor;
	float	vx0,vy0;
	float	x0,y0;
	float	x1,y1;

	float	d1;
	float	d2;
	float	d3;

	uchar	r, g, b;
	uchar	*bits;


	bits = (uchar *)circle->Bits();

	memset(bits, 0, circle->BytesPerRow() * 300); 
	
	if (the_source == 0)
		return;

	d1 = 88;
 
	pcolor = c_idx * 4;

	if (colors_inited == 0) {
		init_colors();
		colors_inited = 1;
	}

	for (i = 0; i < pi2; i += 0.006) {
		vx0 = sin(i + delta);
		vy0 = cos(i + delta);

		x0 = vx0 * d1 + CX;
		y0 = vy0 * d1 + CY;

		//d2 = 89 + fabs(cos(i * 5.2) * sin(i * 4)) * 45;
		d2 = 89 + 45 * the_source->GetValue(fnorm(i / (pi2)), 0.006);

		x1 = vx0 * d2 + CX;
		y1 = vy0 * d2 + CY;

		pcolor = pcolor % 1024;
		fline(0.5 + x0, 0.5 + y0, 0.5 + x1, 0.5 + y1, colors[pcolor]);


		d3 = 89 + 50;

		x0 = vx0 * d3 + CX;
		y0 = vy0 * d3 + CY;

		fline(0.5 + x1, 0.5 + y1, 0.5 + x0, 0.5 + y0, 0xff000000);
		pcolor++;
	} 

}


/*------------------------------------------------------------*/

void	Transport::DrawYellow()
{
	float	vx0,vy0;
	float	x0,y0;
	float	x1,y1;
	float	x2,y2;
	float	d1, d2;

	vx0 = sin(yellow_pos);
	vy0 = cos(yellow_pos);

	d1 = 88;

	x0 = vx0 * d1 + CX;
	y0 = vy0 * d1 + CY;

	d2 = 89 + 45; 
	x1 = vx0 * d2 + CX;
	y1 = vy0 * d2 + CY;


	if (mv->Window()->Lock()) {
		mv->SetHighColor(252, 250, 100);
		mv->MovePenTo(BPoint(x0, y0));
		mv->StrokeLine(BPoint(x1, y1));
		mv->DrawBitmap(yellow, BPoint(x1-4,y1-4));
		mv->Sync();
		mv->Window()->Unlock();
	}
}


/*------------------------------------------------------------*/

char	Transport::NeedUpdate()
{
	char	v;

	v = need_update;

	need_update = 0;

	return v;
}

/*------------------------------------------------------------*/

void	Transport::fline(long x1,long y1,long x2, long y2, ulong color)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = circle->BytesPerRow() / 4;
	long	error;
	long	cpt;
	ulong 	*base;
	float	k;
	
	
	
	
	dx = x2-x1;
	dy = y2-y1;
	
	base = (ulong*)circle->Bits() + y1*rowbyte+x1;
	
	if (dx==0 && dy==0) {
		*base = color;	
		return;
	}
	
	
	
	if (dy<0) {
		sy = -1;
		dy = -dy;
		rowbyte = -rowbyte;
	}
	else
		sy = 1;
	
	if (dx > 0) {
		if (dx > dy) {
			error = dx >> 1;
			cpt = x2 - x1;
			k = (31*65536)/(dx);
			dy = (int)(dy*k);
			dx = (int)(dx*k);
			
			while(cpt>=0) {
				cpt--; 
				*base = color;
				*(base + rowbyte) = color;
				base++;
				error += dy;
				if (error >= dx) {
					base += rowbyte;
					error -= dx;
				}
			}
		}
		else {
			error = dy>>1;
			cpt = dy;
			k = (31*65536)/(dy);
			dy = (int)(dy*k);
			dx = (int)(dx*k);
			while(cpt >= 0) {
				cpt--;
				*base =  color;
				*(base + 1) =  color;
				base += rowbyte;
				error += dx;
				if (error >= dy) {
					base++; 
					error -= dy;
				}
			}
		}
	}
	else {
		dx = -dx;
		if (dx > dy) {
			error = dx >> 1;
			cpt = dx;
			k = (31*65536)/(dx);
			dy = (int)(dy*k);
			dx = (int)(dx*k);
			
			while(cpt>=0) {
				cpt--; 
				*base = color;
				*(base + rowbyte) = color;
				base--;
				error += dy;
				if (error >= dx) {
					base += rowbyte;
					error -= dx;
				}
			}
		}
		else {
			error = dy>>1;
			cpt = dy;
			k = (31*65536)/(dy);
			dy = (int)(dy*k);
			dx = (int)(dx*k);
			while(cpt >= 0) {
				cpt--;
				*base =  color;
				*(base + 1) =  color;
				base += rowbyte;
				error += dx;
				if (error >= dy) {
					base--; 
					error -= dy;
				}
			}
		}
	}
}

