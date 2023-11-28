//******************************************************************************
//
//	File:		tsb.cpp
//
//	Copyright 1993-95, Be Incorporated
//
//
//******************************************************************************

 
#define DEBUG 1

#include <Debug.h>

#include <string.h>
#include <math.h>
#include <OS.h>
#include <Screen.h>
#include <stdlib.h>

#include "tsb.h"

#ifndef _INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif

/*------------------------------------------------------------*/

TShowBit	*tsbb;

/*------------------------------------------------------------*/
long	niter = 256; 
/*------------------------------------------------------------*/
double	vvx;
double	vvy;
double	ssx;
char	t1_done;
char	t2_done;
/*------------------------------------------------------------*/

uchar	palette[256];

inline
int iterate1(double a, double b)
{
	double	x;
	double	y;
	double	xsq;
	double	ysq;
	double	cfour = 4.0;
	double	ctwo = 2.0;
	double	saved;
	int		i = 0;
	int		iter = niter;

	
	x = 0.0;
	y = 0.0;
	i = 0;
	while (i < 12) {
		ysq = y * y;
		y = (ctwo * x * y) + b;
		xsq = x * x;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	saved = xsq + ysq;
	
	while (i < 24) {
		ysq = y * y;
		y = (ctwo * x * y) + b;
		xsq = x * x;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	
	
	saved = fabs((xsq + ysq) - saved);
	if (saved < 0.00001) {
		return niter;
	}
	
	iter -= 25;


	while (i < iter) {
		ysq = y * y;
		y = (ctwo * x * y) + b;
		xsq = x * x;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	return(niter);
}

/*------------------------------------------------------------*/
//extern "C" int iterate(float a, float b);
/*------------------------------------------------------------*/

inline
int iterate(float a, float b)
{
	float	x;
	float	y;
	float	xsq;
	float	ysq;
	float	newx;
	float	saved;
// These are variables, because the metaware compiler would reload the
// constants from memory each time through the loop rather than leave them
// in registers.  Lovely.
	float	cfour = 4.0;
	float	ctwo = 2.0;
	long	i;
	int		iter = niter;
	
	
	x = 0.0;
	y = 0.0;
	i = 0;
	while (i < 12) {
		ysq = y * y;
		y = (ctwo * x * y) + b;
		xsq = x * x;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	saved = xsq + ysq;
	
	while (i < 24) {
		ysq = y * y;
		y = (ctwo * x * y) + b;
		xsq = x * x;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	
	
	saved = fabs((xsq + ysq) - saved);
	
	if (saved < 0.001) {
		return niter;
	}
	
	iter -= 24;


	while (i < iter) {
		ysq = y * y;
		y = (ctwo * x * y) + b;
		xsq = x * x;
		i++;
		x = a + (xsq - ysq);
	
		if ((xsq + ysq) > cfour)
			return(i);
	}
	return(niter);
}

/*------------------------------------------------------------*/

void	TShowBit::MouseDown(BPoint where)
{
	if (!this->Window()->IsFront()) {
		this->Window()->Activate(TRUE);
		this->Window()->UpdateIfNeeded();
	}

	if (busy)
		return;

	if ((modifiers() & B_SHIFT_KEY) == 0) {
		change_selection(where.x, where.y);
		if ((selection.bottom - selection.top) < 4)
			return;
	}
	busy = 1;
	redraw_mand();
	busy = 0;
}

/*------------------------------------------------------------*/

void	TShowBit::redraw_mand()
{
	double	px0;
	double	py0;
	double	scale0;

	if (modifiers() & B_SHIFT_KEY) {
		px -= (scale / 2.0);
		py -= (scale / 2.0);
		scale *= 2.0;
	}
	else {
		px0 = px - (scale/2.0) + (scale * (((selection.left + selection.right)/2.0)/ (1.0*size_x)));
		py0 = py - (scale/2.0) + (scale * (((selection.top + selection.bottom)/2.0)/ (1.0*size_y)));
		
		px = px0; py = py0; scale = scale;	
	}
	selection.Set(-1000, -1000, -1000, -1000);
	mand(px, py, scale, scale);
}

/*------------------------------------------------------------*/

void	TShowBit::set_iter(long it)
{
	if (it != iter) {
		iter = it;
		niter = it;
		selection.Set(-1000, -1000, -1000, -1000);
		//mand(px, py, scale, scale);
	}
}

/*------------------------------------------------------------*/

void	TShowBit::set_palette(long code)
{
	rgb_color	c;
	long		i;
	
	BScreen screen( Window() );

	if (code == 0) {
		for (i = 0; i < 256; i++) {
			palette[i] = (i & 0x1f);
			//printf("p[%ld] = %d\n", i, palette[i]); 
		}
	}
	if (code == 1) {
		for (i = 0; i < 256; i++) {
			c.red = i * 4;
			c.green = i * 7;
			c.blue = 256-(i - i * 5);
			palette[i] = screen.IndexForColor(c);
		}
	}

	if (code == 2) {
		for (i = 0; i < 256; i++) {
			c.red = (i * 7);
			c.green = i/2;
			c.blue = 256-(i * 3);
			palette[i] = screen.IndexForColor(c);
		}
	}

	if (code == 3) {
		for (i = 0; i < 256; i++) {
			c.red = 256-(i * 6);
			c.green = (i * 7);
			c.blue = 0;
			palette[i] = screen.IndexForColor(c);
		}
	}
}

/*------------------------------------------------------------*/

void	TShowBit::dcalc2()
{
	long	x, y;
	long	mx,my;
	long	bx;
	long	i;
	long	nx, ny;
	double	cx, cy;
	uchar	v;
	char	*p;
	uchar	*bits = (uchar *)the_bitmap->Bits();
	uchar	*b0;
	uchar	*b1;
	double	scale;	
	char	small = 0;
	double	tmp[size_x];
	long	cnt = 0;
	uchar	*i_copy;
	short	*tmp_p;
	double	tmp_y;
	double	vx,vy,sx,sy;
	
	acquire_sem(lock2);
	
	vx = px_map;
	vy = py_map;
	sx = pscale_map;
	sy = pscale_map;
	
	scale = sx;
	
	if (scale < 0.000025 || niter > 255) {
		small = 1;
	}
	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);
	cy = vy;
	

	v = 0;
	for (y = 2; y < size_y-1; y+=2) {
		my = remap_y[y];
		b0 = bits + (y * size_x);
		tmp_y = vy + sy*y;
		if (my < 0) {
			for (x = 0; x < size_x; x++) {
				if (small) {
					v = iterate1(vx + sx * x, tmp_y);
				}
				else {
					v = iterate(vx + sx * x, tmp_y);
				}
				*(b0+x) = palette[v];
				cnt++;
			}
		}
		else {
			if (small) {
				for (x = 0; x < size_x; x++) {
					if (remap_x[x] < 0) {
						v = iterate1(vx + sx * x, tmp_y);
						*(b0+x) = palette[v];
						cnt++;
					}
				}
			}
			else {
				for (x = 0; x < size_x; x++) {
					if (remap_x[x] < 0) {
						v = iterate(vx + sx * x, tmp_y);
						*(b0+x) = palette[v];
						cnt++;
					}
				}
			}
		}
	}
done:
	t1_done = 0;
}

/*------------------------------------------------------------*/

void	TShowBit::dcalc1()
{
	long	x, y;
	long	mx,my;
	long	bx;
	long	i;
	long	nx, ny;
	double	cx, cy;
	uchar	v;
	char	*p;
	uchar	*bits = (uchar *)the_bitmap->Bits();
	uchar	*b0;
	uchar	*b1;
	double	scale;	
	char	small = 0;
	double	tmp[size_x];
	long	cnt = 0;
	short	*tmp_p;
	double	tmp_y;
	double	vx,vy,sx,sy;
	
	acquire_sem(lock1);
	vx = px_map;
	vy = py_map;
	sx = pscale_map;
	sy = pscale_map;
	
	scale = sx;

	if (scale < 0.000025 || niter > 256) {
		small = 1;
	}
	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);
	cy = vy;
	cx = vx;
	

	for (y = 1; y < size_y-1; y+=2) {
		my = remap_y[y];
		b0 = bits + (y * size_x);
		tmp_y = vy + sy*y;
		if (my < 0) {
			for (x = 0; x < size_x; x++) {
				if (small) {
					v = iterate1(vx + sx * x, tmp_y);
				}
				else {
					v = iterate(vx + sx * x, tmp_y);
				}
				*(b0+x) = palette[v];
				cnt++;
			}
		}
		else {
			if (small) {
				for (x = 0; x < size_x; x++) {
					if (remap_x[x] < 0) {
						v = iterate1(vx + sx * x, tmp_y);
						*(b0+x) = palette[v];
						cnt++;
					}
				}
			}
			else {
				for (x = 0; x < size_x; x++) {
					if (remap_x[x] < 0) {
						v = iterate(vx + sx * x, tmp_y);
						*(b0+x) = palette[v];
						cnt++;
					}
				}
			}
		}
	}
done:
	t2_done = 0;
}

/*------------------------------------------------------------*/

long	__calc1(void *p)
{
	while(1)
	tsbb->dcalc1();
}

/*------------------------------------------------------------*/

long	__calc2(void *p)
{
	while(1)
	tsbb->dcalc2();
}


/*------------------------------------------------------------*/

TShowBit::TShowBit(BRect r, long flags) :
	BView(r, "", flags, B_WILL_DRAW | B_PULSE_NEEDED)
{
	BRect	bitmap_r;
	long	ref;
	long	i;
	char	*bits;

	lock1 = create_sem(0, "sem1");
	lock2 = create_sem(0, "sem2");
	busy = FALSE;
	exit_now = FALSE;
	tsbb = this;
	bitmap_r.Set(0, 0, size_x - 1, size_y - 1);
	selection.Set(-1000, -1000, -1000, -1000);
	iter = 255;

	the_bitmap = new BBitmap(bitmap_r, B_COLOR_8_BIT);
	bits = (char *)the_bitmap->Bits();
	memset(bits, 0x00, size_x*size_y);
	px = -1;
	py = 0;
	scale = 4.0;

	set_palette(0);
	init_mand(px, py, scale, scale);
	resume_thread(spawn_thread(__calc1, "calc1", B_DISPLAY_PRIORITY, 0));
	resume_thread(spawn_thread(__calc2, "calc1", B_DISPLAY_PRIORITY, 0));
}

/*------------------------------------------------------------*/

TShowBit::~TShowBit()
{
	delete	the_bitmap;
}

/*------------------------------------------------------------*/

void	TShowBit::Draw(BRect update_rect)
{
	DrawBitmap(the_bitmap, BPoint(0, 0));
}


/*------------------------------------------------------------*/



/*------------------------------------------------------------*/

uchar	tmp[256];
uchar	tmp1[256];

/*------------------------------------------------------------*/

void	TShowBit::init_mand(double vx, double vy, double sx, double sy)
{
	long	i;
	
	vvx = vx; vvy = vy; ssx = sx;
	calc_mand(vvx - (ssx/2.0), vvy - (ssx/2.0), ssx, ssx);
	Draw(BRect(0,0,1000,1000));
}

/*------------------------------------------------------------*/
#define	K	0.94

void	TShowBit::mand(double vx, double vy, double sx, double sy)
{
	long	x;
	double	start, end;
	long	i;
	
	
	start = system_time();
	
	i = 0;
	while(i < 60) {
		BPoint	where;
		ulong	but;

		GetMouse(&where, &but);
		if (but)
			goto out;
		Window()->UpdateIfNeeded();
		vvx = vx; vvy = vy; ssx = sx;
		if (i <1)
			calc_mand(vvx - (ssx/2.0), vvy - (ssx/2.0), ssx, ssx);
		else {
				calc_mand_delta(vvx - (ssx/2.0), vvy - (ssx/2.0), ssx, ssx);
		}
		DrawBitmapAsync(the_bitmap, BPoint(0, 0));
		i++;

		sx *= K;
		sy *= K;
		vvx = vx; vvy = vy; ssx = sx;
		calc_remap(vvx - (ssx/2.0), vvy - (ssx/2.0), ssx, ssx);
		Sync();
		if (exit_now)
			goto out;
	}
out:;
	end = system_time();
	printf("dt=%f\n", (end-start)/1000.0);
	sx /= K;
	sy /= K;
	calc_mand(vvx - (ssx/2.0), vvy - (ssx/2.0), ssx, ssx);
	Draw(BRect(0,0,1000,1000));
	px = vx;
	py = vy;
	scale = sx;
}

/*------------------------------------------------------------*/

void	TShowBit::Pulse()
{
}


/*------------------------------------------------------------*/

void
insert_sort(
	double * arg,
	int size)
{
	for (int ix=1; ix<size; ix++)
	{
		int save = ix;
		while ((ix > 0) && (arg[ix] < arg[ix-1]))
		{
			double d = arg[ix];
			arg[ix] = arg[ix-1];
			arg[ix-1] = d;
			ix--;
		}
		ix = save;
	}
#if DEBUG
	for (int ix=0; ix<size-1; ix++)
	{
		if (arg[ix] > arg[ix+1])
		{
			printf("ERROR IN INSERT_SORT!!!\n");
		}
	}
#endif
}


void
sort(
	double * arg,
	int size)
{
	double pivot;
	int top;
	int bot;
again:
	if (size < 10)
	{
		insert_sort(arg, size);
		return;
	}
	pivot = (arg[0]+arg[size-1])/2.0;
	top = size-1;
	bot = 0;
	while (top > bot)
	{
		while ((top > bot) && (arg[top] > pivot)) {
			top--;
		}
		while ((bot <= top) && (arg[bot] <= pivot)) {
			bot++;
		}
		if (top > bot)
		{
			double s = arg[bot];
			arg[bot] = arg[top];
			arg[top] = s;
			top--;
			bot++;
		}
	}
#if DEBUG
	if (bot == 0)
		printf("ERROR IN SORT - BOT is 0\n");
#endif
	sort(arg, bot);
	arg += bot;
	size = size-bot;
	goto again;	/*	tail recursion!	*/
}


int	cmp(const void *p1, const void *p2)
{
	double	v1;
	double	v2;
	
	v1 = *(double *)p1;
	v2 = *(double *)p2;
	if (v1<v2) return -1;
	else return 1;
}

void	TShowBit::calc_remap(double vx, double vy, double sx, double sy)
{
	long	x;
	long	cx;
	long	y;
	long	cy;
	double	best;
	double	delta;
	long	who;
	long	good = 0; 
	long	bad = 0;
	long	vmax;
	long	vmin;
	double	p0;
	double	err[size_x+1];
	double	e0[size_x+1];

	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);

	for (y = 0; y < size_y; y++) {
		py_exact[y] = vy + sy * y;
	}
	for (x = 0; x < size_x; x++) {
		px_exact[x] = vx + sx * x;
	}

	sx *= 0.7;
	sy *= 0.7;

	for (x = 0; x < size_x; x++) {
		best = 32000.0;
		
		vmin = x-12;
		vmax = x+12;
		if (vmax >= size_x)
			vmax = size_x;
		
		if (vmin < 0)
			vmin = 0;			
			
		p0 = px_exact[x];
		for (cx = vmin; cx < vmax; cx++) {
			delta = p0 - px_actual[cx];
			delta = fabs(delta);
			if (delta < best) {
				who = cx;
				best = delta;
			}
		}
		err[x] = best;
		e0[x] = best;
		remap_x[x] = who;
	}
	qsort(err, size_x, sizeof(double), cmp);
	p0 = err[size_x - (size_x/14)];
		
	for (x = 0; x < size_x; x++) {
		if (e0[x] > p0)
			remap_x[x] = -1;
	}
	
	for (y = 0; y < size_y; y++) {
		best = 32000.0;
		
		vmax = y+12;
		vmin = y-12;
		if (vmax >= size_y)
			vmax = size_y;
		if (vmin < 0)
			vmin = 0;			

		p0 = py_exact[y];
		for (cy = vmin; cy < vmax; cy++) {
			delta = p0 - py_actual[cy];
			delta = fabs(delta);
			if (delta < best) {
				who = cy;
				best = delta;
			}
		}
	
		if (best < sx) {
			err[x] = best;
			e0[x] = best;
			remap_y[y] = who;
		}
	}
	qsort(err, size_y, sizeof(double), cmp);
	p0 = err[size_y - (size_y/14)];
		
	for (y = 0; y < size_y; y++) {
		if (e0[y] > p0)
			remap_y[y] = -1;
	}
}

/*------------------------------------------------------------*/

void	TShowBit::calc_mand_delta(double vx, double vy, double sx, double sy)
{
	long	x, y;
	long	mx,my;
	long	bx;
	long	i;
	long	nx, ny;
	double	cx, cy;
	uchar	v;
	char	*p;
	uchar	*bits = (uchar *)the_bitmap->Bits();
	uchar	*b0;
	uchar	*b1;
	double	scale = sx;	
	char	small = 0;
	double	tmp[size_x];
	long	cnt = 0;
	uchar	*i_copy;
	short	*tmp_p;
	double	tmp_y;
	//int		pre[size_x/4][size_x/4];
	char	vb0,vb1,vb2,vb3;
	
	px_map = vx;
	py_map = vy;
	pscale_map = sx;

	i_copy = (uchar *)malloc((size_x+1)*(size_y+1));
	memcpy(i_copy, bits, (size_x)*(size_y));
	
	if (scale < 0.000025 || niter > 255) {
		small = 1;
	}
	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);
	cy = vy;
	
	
	for (y = 0; y < size_y; y++) {
		my = remap_y[y];
		b0 = bits + y * size_x;
		if (my >= 0) {
				b1 = i_copy + my * size_x;
				tmp_p = remap_x - 1;
				for (x = 0; x < size_x; x++) {
					mx = *++tmp_p; 
			 		if (mx >= 0)
						*(b0+x) = *(b1+mx);
		 		}
		}
	}
	
	free((char *)i_copy);
/*

	v = 0;
	for (y = 1; y < size_y-1; y++) {
		my = remap_y[y];
		b0 = bits + (y * size_x);
		tmp_y = vy + sy*y;
		if (my < 0) {
			for (x = 0; x < size_x; x++) {
				if (small) {
					v = iterate1(vx + sx * x, tmp_y);
				}
				else {
					v = iterate(vx + sx * x, tmp_y);
				}
				*(b0+x) = palette[v];
				cnt++;
			}
		}
		else {
			if (small) {
				for (x = 0; x < size_x; x++) {
					if (remap_x[x] < 0) {
						v = iterate1(vx + sx * x, tmp_y);
						*(b0+x) = palette[v];
						cnt++;
					}
				}
			}
			else {
				for (x = 0; x < size_x; x++) {
					if (remap_x[x] < 0) {
						v = iterate(vx + sx * x, tmp_y);
						*(b0+x) = palette[v];
						cnt++;
					}
				}
			}
		}
	}
*/


	t1_done = 1;
	t2_done = 1;
	release_sem(lock1);
	release_sem(lock2);
	while(1) {
		snooze(13000);
		if (t1_done == 0 && t2_done == 0)
			goto out;
	}
out:;


	
	for (x = 0; x < size_x; x++)
		tmp[x] = px_actual[x];
	for (x = 0; x < size_x; x++) 
		if (remap_x[x] >= 0)
			px_actual[x] = tmp[remap_x[x]];

	for (y = 0; y < size_y; y++)
		tmp[y] = py_actual[y];
		
	for (y = 0; y < size_y; y++) 
		if (remap_y[y] >= 0)
			py_actual[y] = tmp[remap_y[y]];
	
	for (x = 0; x < size_x; x++)
		if (remap_x[x] < 0) {
			px_actual[x] = px_exact[x];
		}
			
	for (y = 0; y < size_y; y++)
		if (remap_y[y] < 0) {
			py_actual[y] = py_exact[y];
		}
			
		
			
done:
	//printf("cnt=%ld\n", cnt);
	t1_done = 1;
}

/*------------------------------------------------------------*/
/*
void	TShowBit::calc_mand_delta(double vx, double vy, double sx, double sy)
{
	long	x, y;
	long	mx,my;
	long	bx;
	long	i;
	long	nx, ny;
	double	cx, cy;
	uchar	v;
	char	*p;
	uchar	*bits = (uchar *)the_bitmap->Bits();
	uchar	*b0;
	uchar	*b1;
	double	scale = sx;	
	char	small = 0;
	long	cnt = 0;
	uchar	*i_copy;
	short	*tmp_p;
	double	tmp_y;
	
	i_copy = (uchar *)malloc((size_x+1)*(size_y+1));
	memcpy(i_copy, bits, (size_x)*(size_y));
	
	px_map = vx;
	py_map = vy;
	pscale_map = sx;

	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);
	cy = vy;
	
	
	for (y = 0; y < size_y; y++) {
		my = remap_y[y];
		b0 = bits + y * size_x;
		if (my >= 0) {
				b1 = i_copy + my * size_x;
				tmp_p = remap_x - 1;
				for (x = 0; x < size_x; x++) {
					mx = *++tmp_p; 
			 		if (mx >= 0)
						*(b0+x) = *(b1+mx);
		 		}
		}
	}
	
	free((char *)i_copy);
	
/*
	release_sem(lock1);
	//release_sem(lock2);
	t1_done = 2;
	while(1) {
		snooze(20000);
		if (t1_done == 0)
			goto out;
	}
out:;
*/
/*
	snooze(150000);

	for (x = 0; x < size_x; x++)
		tmp[x] = px_actual[x];
	for (x = 0; x < size_x; x++) 
		if (remap_x[x] >= 0)
			px_actual[x] = tmp[remap_x[x]];

	for (y = 0; y < size_y; y++)
		tmp[y] = py_actual[y];
		
	for (y = 0; y < size_y; y++) 
		if (remap_y[y] >= 0)
			py_actual[y] = tmp[remap_y[y]];
	
	for (x = 0; x < size_x; x++)
		if (remap_x[x] < 0) {
			px_actual[x] = px_exact[x];
		}
			
	for (y = 0; y < size_y; y++)
		if (remap_y[y] < 0) {
			py_actual[y] = py_exact[y];
		}
}
*/
/*------------------------------------------------------------*/

void	TShowBit::calc_mand(double vx, double vy, double sx, double sy)
{
	long	x, y;
	long	bx;
	long	i;
	long	nx, ny;
	double	cx, cy;
	uchar	v;
	char	*p;
	uchar	*bits = (uchar *)the_bitmap->Bits();
	uchar	*b0;
	long	y12;
	long	x12;
	double	scale = sx;	
	char	small = 0;
	
	if (scale < 0.000025 || niter > 255) {
		small = 1;
	}
	sx = sx / (size_x * 1.0);
	sy = sy / (size_y * 1.0);
	cy = vy;

	for (y = 0; y < size_y; y++) {
		py_actual[y] = vy + sy * y;
		py_exact[y] = py_actual[y];
	}
	for (x = 0; x < size_x; x++) {
		px_actual[x] = vx + sx * x;
		px_exact[x] = px_actual[x];
	}

	for (y = 0; y < size_y; y++) {
		b0 = bits + (y * size_x);
		for (x = 0; x < size_x; x++) {
			if (small) {
				v = iterate1(vx + sx * x, vy + sy * y);
			}
			else {
				v = iterate(vx + sx * x, vy + sy * y);
			}
			*b0++ = palette[v];
		}
	}
done:
	t1_done = 1;
}


/*------------------------------------------------------------*/

long	TShowBit::limit_v(long v)
{
	if (v > (size_y - 1))
			v = (size_y - 1);

	if (v < 0)
			v = 0;
	return(v);
}

/*------------------------------------------------------------*/

long	TShowBit::limit_h(long v)
{
	if (v > (size_x - 1))
			v = size_x - 1;

	if (v < 0)
			v = 0;
	return(v);
}

/*------------------------------------------------------------*/

BRect	TShowBit::sort_rect(BRect *aRect)
{
	BRect	tmp_rect;
	long	tmp;

	tmp_rect = *aRect;
	if (tmp_rect.bottom < tmp_rect.top) {
		tmp = tmp_rect.top;
		tmp_rect.top = tmp_rect.bottom;
		tmp_rect.bottom = tmp;
	}

	if (tmp_rect.left > tmp_rect.right) {
		tmp = tmp_rect.right;
		tmp_rect.right = tmp_rect.left;
		tmp_rect.left = tmp;
	}

	tmp_rect.top = limit_v(tmp_rect.top);
	tmp_rect.left = limit_h(tmp_rect.left);
	tmp_rect.bottom = limit_v(tmp_rect.bottom);
	tmp_rect.right = limit_h(tmp_rect.right);

	return(tmp_rect);
}

/*------------------------------------------------------------*/

void	TShowBit::clip(long *h, long *v)
{
	if (*h > (size_x - 1))
			*h = (size_x - 1);
	if (*h < 0)
			*h = 0;
	if (*v > (size_y - 1))
			*v = size_y - 1;
	if (*v < 0)
			*v = 0;
}

/*------------------------------------------------------------*/

char	TShowBit::has_selection()
{
	if (((selection.bottom - selection.top) + (selection.right - selection.left)) < 5) 
		return 0;
	else
		return 1;
}

/*------------------------------------------------------------*/

void	TShowBit::change_selection(long h, long v)
{
	ulong	buttons;
	long	h0;
	long	v0;
	BRect	new_select;
	BRect	old_select;
	BRect	tmp_rect;
	long	max;
	long	width, height;
	
	clip(&h, &v);
	new_select.top = v;
	new_select.left = h;
	old_select = selection;

	SetDrawingMode(B_OP_INVERT);

	do {
		BPoint where;
		GetMouse(&where, &buttons);
		h0 = where.x;
		v0 = where.y;
		width = h0 - h;
		height = v0 - v;
		max= ((v0>v) ^ (height < width)) ? height : width;

		h0 = h+max; v0 = v+max;

		clip(&h0, &v0);
		new_select.right = h0;
		new_select.bottom = v0;

		if ((old_select.top != new_select.top) || 
		    (old_select.bottom != new_select.bottom) ||
		    (old_select.right != new_select.right) ||
		    (old_select.left != new_select.left)) {
		
			tmp_rect = sort_rect(&new_select);
			StrokeRect(tmp_rect);

			tmp_rect = sort_rect(&old_select);
			StrokeRect(tmp_rect);

			old_select = new_select;
			Flush();
		}

		snooze(20000);
	} while(buttons);

	selection = sort_rect(&new_select);
	if (!has_selection()) {
		StrokeRect(selection);
		selection.Set(-1000, -1000, -1000, -1000);
	} 
	SetDrawingMode(B_OP_COPY);
}
