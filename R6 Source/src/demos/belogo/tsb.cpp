//******************************************************************************
//
//	File:		main.cpp
//
//	Description:	show pixmap main test program.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	7/31/92		bgs	new today
//
//******************************************************************************

#include <stdlib.h>
#include <string.h>

#ifndef	_APPLICATION_H
#include <Application.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif
#ifndef _MENU_BAR_H
#include <MenuBar.h>
#endif
#ifndef	_SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#include <Window.h>
#include <OS.h>
#include <math.h>

#include "tsb.h"
#define DEBUG 1
#include	<Debug.h>


/*------------------------------------------------------------*/

#define K	20000


TShowBit	*tsbb;
char		done;
ulong		buttons;
BPoint		pos;
BRect		erase_1;
BRect		erase_2;
BRect		erase_1_old;
BRect		erase_2_old;

/*------------------------------------------------------------*/

void	TShowBit::MouseDown(BPoint where)
{
}

/*------------------------------------------------------------*/

TShowBit::TShowBit(BRect r, long flags) :
	BView(r, "", flags, B_WILL_DRAW)
{
	BRect	bitmap_r;
	long	ref;
	long	i;
	char	*bits;

	exit_now = 0;
	bitmap_r.Set(0, 0, size_x - 1, size_y - 1);

	the_bitmap = bm1 = new BBitmap(bitmap_r, B_COLOR_8_BIT);
	bits = (char *)the_bitmap->Bits();
	memset(bits, 0x00, size_x*size_y);


	bm2 = new BBitmap(bitmap_r, B_COLOR_8_BIT);
	bits = (char *)bm2->Bits();
	memset(bits, 0x00, size_x*size_y);

	poly_scratch = (long *)malloc(4096);
	bm1_sem = create_sem(1, "bm1_sem");
	bm2_sem = create_sem(1, "bm2_sem");
}

/*------------------------------------------------------------*/

TShowBit::~TShowBit()
{
	delete bm1;
	delete bm2;
	delete_sem(bm1_sem);
	delete_sem(bm2_sem);
}

/*------------------------------------------------------------*/

void	TShowBit::Draw(BRect update_rect)
{
	DrawBitmap(the_bitmap, BPoint(0, 0));
}

/*------------------------------------------------------------*/

long	color;

/*------------------------------------------------------------*/

void	hline(char *base, long count)
{
	long	c;

	c = color;

	if (count < 8) {
		while(count-- >= 0)
			*base++ = c;
		return;
	}

	while ((long)base & 0x03) {
		*base++ = c;
		count--;
	}
	while(count > 3) {
		*(long *)base = c;
		base += 4;
		count -= 4;
	}

	while(count-- >= 0) 
		*base++ = c;
}

/*------------------------------------------------------------*/

void	TShowBit::fill_triangle(BPoint pt_a, BPoint pt_b, BPoint pt_c)
{
	#define compare(a,b,c)  {if (a<b) b = a; if (a>c) c = a;}
 
	long* 	coord0;
	long* 	coord1;
	long	*tmp0;
	long	*tmp1;
	long	dx;
	long	dy;
	long	i;
	long	j;
	long	x;
	long	xe;
	long	xpoint[4];
	long	ypoint[4];
	long	xs;
	long	y;
	long	y_sign;
	long	decision;
	long	ye;
	long	ys;
   	long	scanlines;
	long	top;
	long	bottom;
	char	*ptr;
	long	left, right;    

	xpoint[3] = xpoint[0] = pt_a.x;
	ypoint[3] = ypoint[0] = pt_a.y;
	xpoint[1] = pt_b.x;
	ypoint[1] = pt_b.y;
	xpoint[2] = pt_c.x;
	ypoint[2] = pt_c.y;
	/*
	printf("pt1 = %ld %ld\n", xpoint[1], ypoint[1]);
	printf("pt2 = %ld %ld\n", xpoint[2], ypoint[2]);
	printf("pt3 = %ld %ld\n", xpoint[3], ypoint[3]);
	*/
	bottom = -10000;
	top = 10000;
	left = 10000;
	right = -100000;

	for (i = 0; i < 3; i++) {
		if (ypoint[i] > bottom)
			bottom = ypoint[i];
		if (ypoint[i] < top)
			top = ypoint[i];
		if (xpoint[i] < left)
			left = xpoint[i];
		if (xpoint[i] > right)
			right = xpoint[i];
	}
	if (top > size_y)
		return;
	if (bottom < 0)
		return;
	if (right < 0)
		return;
	if (left > size_x)
		return;
	
	scanlines = 1 + bottom - top;

	coord0 = (long*)poly_scratch;
	coord1 = coord0 + scanlines;
		
	tmp0 = coord0;
	tmp1 = coord1;

    	for (i = 0; i <= bottom - top; i++) {
		*tmp0++ = 128000;
		*tmp1++ = -128000;
	}

	for (i = 0; i < 3; i++) {
		xs = xpoint[i];
		xe = xpoint[i + 1];
		ys = ypoint[i];
		ye = ypoint[i + 1];
		
		if (xs > xe) {
			j    = xs;
			xs   = xe;
			xe   = j;
			j    = ys;
			ys   = ye;
			ye   = j;
		}

		y = ys - top;
		tmp0 = coord0 + y;
		tmp1 = coord1 + y;

		compare(xs, *tmp0, *tmp1);

		dx = abs((int)(xe - xs));
		dy = abs((int)(ye - ys));

		if ((ye - ys) < 0)
			y_sign = -1;
		else
			y_sign = 1;

		x = xs;
	
		if (dy == 0) {
			*tmp0 = xs;
			*tmp1 = xe;
			continue;
		}
		
		if (dx >= dy) {
			decision = dx / 2;

			while (x <= xe) {
				if (decision >= dx) {
					decision -= dx;
					y += y_sign;
					tmp0 += y_sign;
					tmp1 += y_sign;
				}

				if (x < *tmp0)
					*tmp0 = x;

				decision += dy;
				
				if (x > *tmp1)
					*tmp1 = x;

				x++;
			}
		}
		else {
			ye -= top;
			decision = dy / 2;
			
			while (y != ye) {
				if (decision >= dy) {
					decision -= dy;
					x++;
				}

				if (x < *tmp0)
					*tmp0 = x;

				tmp0 += y_sign;
				
				if (x > *tmp1)
					*tmp1 = x;

				decision += dx;
				tmp1 += y_sign;
				y += y_sign;
			}
			ye += top;
		}
	}

	tmp0 = coord0;
	tmp1 = coord1;
	
	ptr = (char *)the_bitmap->Bits();

	ptr = ptr + (top * size_x);

	for (i = top; i <= bottom; i++) {
		if (*tmp0 < 0)
			*tmp0 = 0;
		if (*tmp1 > size_x)
			*tmp1 = size_x;
		if ((i >= 0) && (i < (size_y - 1))) 
			hline(ptr + *tmp0, *tmp1 - *tmp0);

		tmp0++;
		tmp1++;

		ptr += size_x;
	}
}

/*------------------------------------------------------------*/

void	TShowBit::fill_4(BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d)
{
	#define compare(a,b,c)  {if (a<b) b = a; if (a>c) c = a;}
 
	long* 	coord0;
	long* 	coord1;
	long	*tmp0;
	long	*tmp1;
	long	dx;
	long	dy;
	long	i;
	long	j;
	long	x;
	long	xe;
	long	xpoint[5];
	long	ypoint[5];
	long	xs;
	long	y;
	long	y_sign;
	long	decision;
	long	ye;
	long	ys;
   	long	scanlines;
	long	top;
	long	bottom;
	char	*ptr;
    
	xpoint[4] = xpoint[0] = pt_a.x;
	ypoint[4] = ypoint[0] = pt_a.y;
	xpoint[1] = pt_b.x;
	ypoint[1] = pt_b.y;
	xpoint[2] = pt_c.x;
	ypoint[2] = pt_c.y;
	xpoint[3] = pt_d.x;
	ypoint[3] = pt_d.y;
	
	bottom = -10000;
	top = 10000;

	for (i = 0; i < 4; i++) {
		if (ypoint[i] > bottom)
			bottom = ypoint[i];
		if (ypoint[i] < top)
			top = ypoint[i];
	}
	if (top > size_y)
		return;
	if (bottom < 0)
		return;
	
	scanlines = 1 + bottom - top;

	coord0 = (long*)poly_scratch;
	coord1 = coord0 + scanlines;
		
	tmp0 = coord0;
	tmp1 = coord1;

    	for (i = 0; i <= bottom - top; i++) {
		*tmp0++ = 640;
		*tmp1++ = -1;
	}

	for (i = 0; i < 4; i++) {
		xs = xpoint[i];
		xe = xpoint[i + 1];
		ys = ypoint[i];
		ye = ypoint[i + 1];
		
		if (xs > xe) {
			j    = xs;
			xs   = xe;
			xe   = j;
			j    = ys;
			ys   = ye;
			ye   = j;
		}

		y = ye - top;
		tmp0 = coord0 + y;
		tmp1 = coord1 + y;
		compare(xe, *tmp0, *tmp1);

		y = ys - top;
		tmp0 = coord0 + y;
		tmp1 = coord1 + y;

		compare(xs, *tmp0, *tmp1);

		dx = abs((int)(xe - xs));
		dy = abs((int)(ye - ys));

		if ((ye - ys) < 0)
			y_sign = -1;
		else
			y_sign = 1;

		x = xs;
	
		if (dy == 0) {
			*tmp0 = xs;
			*tmp1 = xe;
			continue;
		}
		
		if (dx >= dy) {
			decision = dx / 2;

			while (x <= xe) {
				if (decision >= dx) {
					decision -= dx;
					y += y_sign;
					tmp0 += y_sign;
					tmp1 += y_sign;
				}

				if (x < *tmp0)
					*tmp0 = x;

				decision += dy;
				
				if (x > *tmp1)
					*tmp1 = x;

				x++;
			}
		}
		else {
			ye -= top;
			decision = dy / 2;
			
			do {
				if (decision >= dy) {
					decision -= dy;
					x++;
				}

				if (x < *tmp0)
					*tmp0 = x;

				tmp0 += y_sign;
				
				if (x > *tmp1)
					*tmp1 = x;

				decision += dx;
				tmp1 += y_sign;
				y += y_sign;
			} while(y != ye);
			ye += top;
		}
	}

	tmp0 = coord0;
	tmp1 = coord1;
	
	ptr = (char *)the_bitmap->Bits();

	ptr = ptr + (top * size_x);

	for (i = top; i <= bottom; i++) {
		if (*tmp0 < 0)
			*tmp0 = 0;
		if (*tmp1 > size_x)
			*tmp1 = size_x;
		if ((i >= 0) && (i < (size_y - 1))) 
			hline(ptr + *tmp0, *tmp1 - *tmp0);

		tmp0++;
		tmp1++;

		ptr += size_x;
	}
}

/*------------------------------------------------------------*/

void	clear1(long *base, BRect erase)
{
	long	i;
	long	dx;
	long	cnt;
	long	cnt1;
	long	*base1;

	erase.left -= 10;
	erase.right += 10;
	erase.top -= 3;
	if (erase.top < 0)
		erase.top = 0;
	erase.bottom += 3;

	if (erase.bottom > (size_y - 1))
		erase.bottom = (size_y - 1);
	if (erase.left < 0)
		erase.left = 0;
	if (erase.right > (size_x - 1))
		erase.right = size_x - 1;

	base += (int(erase.top) * (size_x / 4));
	cnt = erase.bottom - erase.top;
	base += (int(erase.left) / 4);


	while(cnt) {
		cnt--;
		cnt1 = 4 + (erase.right - erase.left) / 4;
		cnt1 /= 4;
		base1 = base;

		while(cnt1--) {
			*base1++ = 0;
			*base1++ = 0;
			*base1++ = 0;
			*base1++ = 0;
		}
		base += (size_x / 4);
	}
}

/*------------------------------------------------------------*/

void	TShowBit::blit_task()
{
	long	*base;
	long	cnt;
	ulong	t0;
	long	t1;	

	t0 = system_time()/1000;
	cnt = 0;

	while(1) {
		t1 = system_time()/1000;
		if ((t1-t0) > 2000) {
//			SPRINT(("%ld frames per second\n", cnt/2));
			cnt = 0;
			t0 = t1;
		}

		acquire_sem(bm1_sem);

		while(done)
			snooze(10000);
		if (!Window()->Lock())
			break;
		base = (long *)bm1->Bits();
		SetDrawingMode(B_OP_COPY);

		DrawBitmap(bm1, erase_1, erase_1);
		cnt++;
		Window()->Unlock();

		clear1((long *)base, erase_1);
		
		release_sem(bm1_sem);
		

		acquire_sem(bm2_sem);
		while(done)
			snooze(10000);
		if (!Window()->Lock())
			break;
		base = (long *)bm2->Bits();
		SetDrawingMode(B_OP_COPY);
		DrawBitmap(bm2, erase_2, erase_2);
		cnt++;
		Window()->Unlock();
		clear1((long *)base, erase_2);
		release_sem(bm2_sem);
		while(done)
			snooze(10000);
	}
}

/*------------------------------------------------------------*/

long blit_tsk(void *arg)
{
	tsbb->blit_task();
	return 0;
}

long __demo1(void *arg)
{
	tsbb->demo2();
	return 0;
}

/*------------------------------------------------------------*/

void	TShowBit::demo()
{
	exit_now = 2;
	tsbb = this;
	resume_thread(spawn_thread(__demo1, "btsk0", B_NORMAL_PRIORITY, NULL));
}

/*------------------------------------------------------------*/

void	TShowBit::demo1()
{
	TBird	*a_bird;
	BRect	tmp_rect;
	float	a, b, c;
	long	*base;
	long	i;
	long	j;
	long	blit_task;
	long	cpt;
	float	k;

	tsbb = this;
	done = 0;

	acquire_sem(bm1_sem);
	acquire_sem(bm2_sem);

	blit_task = spawn_thread(blit_tsk, "btsk", B_DISPLAY_PRIORITY, NULL);
	resume_thread(blit_task);	

	a_bird = new TBird(this, 0, 0, 0);

	c = 0.1;
	k = 1.0;
	cpt = 0;

	release_sem(bm1_sem);
	release_sem(bm2_sem);

	acquire_sem(bm1_sem);
	erase_1.Set(0, 0, 0, 0);
	erase_2.Set(0, 0, 0, 0);
	erase_1_old.Set(0, 0, 0, 0);
	erase_2_old.Set(0, 0, 0, 0);

	for (j = 0; j < 9000; j++) {
		
		if (exit_now == 1) {
			release_sem(bm1_sem);
			release_sem(bm2_sem);
			done = 1;
			snooze(200*1000);
			kill_thread(blit_task);
			exit_now = 0;
			return;
		}
	
		a = pos.x / 60.0;
		b = pos.y / 60.0;

		the_bitmap = bm1;
		a_bird->flap_flap();
		base = (long *)the_bitmap->Bits();
		erase_1_old = erase_1;
		a_bird->draw(a, b, c, &erase_1);
		tmp_rect = erase_1;
		erase_1 = tmp_rect | erase_1_old;
		erase_1_old = tmp_rect;

		acquire_sem(bm2_sem);
		release_sem(bm1_sem);
		
		if (buttons) 
			c = c + 0.02;

		if (k < 0.9)
			k = 0.9;

		a = pos.x / 60.0;
		b = pos.y / 60.0;

		the_bitmap = bm2;
		a_bird->flap_flap();
		base = (long *)the_bitmap->Bits();
		
		erase_2_old = erase_2;
		a_bird->draw(a, b, c, &erase_2);
		tmp_rect = erase_2;
		erase_2 = tmp_rect | erase_2_old;
		erase_2_old = tmp_rect;

		acquire_sem(bm1_sem);
		release_sem(bm2_sem);
	}
	release_sem(bm1_sem);

	done = 1;
	snooze(300*1000);
	kill_thread(blit_task);
	the_bitmap = bm1;
	a_bird->draw(a, b, c, &erase_1);
	the_bitmap = bm2;
	a_bird->draw(a, b, c, &erase_2);
}

/*------------------------------------------------------------*/

void	TShowBit::demo2()
{
	TCube	*a_cube;
	BRect	tmp_rect;
	float	a, b, c;
	long	*base;
	long	i;
	long	j;
	long	blit_task;
	long	cpt;
	float	k;	
	int	posx0;
	int	posy0;

	tsbb = this;
	done = 0;

	a = b = c = 0;
	acquire_sem(bm1_sem);
	acquire_sem(bm2_sem);

	blit_task = spawn_thread(blit_tsk, "btsk", B_DISPLAY_PRIORITY, NULL);
	resume_thread(blit_task);	

	a_cube = new TCube(this, 0, 0, 0);
	a_cube->new_one();
	c = 0.1;
	k = 1.0;
	cpt = 0;

	release_sem(bm1_sem);
	release_sem(bm2_sem);

	acquire_sem(bm1_sem);
	erase_1.Set(0, 0, 0, 0);
	erase_2.Set(0, 0, 0, 0);
	erase_1_old.Set(0, 0, 0, 0);
	erase_2_old.Set(0, 0, 0, 0);

	for (j = 0; j < 9000000; j++) {
		if (exit_now == 1) {
			printf("exit now\n");
			release_sem(bm1_sem);
			release_sem(bm2_sem);
			done = 1;
			snooze(50*1000);
			printf("kill now\n");
			kill_thread(blit_task);
			exit_now = 0;
			return;
		}
	

		posx0 = pos.x;
		posy0 = pos.y;

		a += 3*(0.03 * 1.5);
		b -= 3*(0.02141321321 * 1.5);
		c += 3*(0.00241421412 * 2.3);
		snooze(K);

		the_bitmap = bm1;
		base = (long *)the_bitmap->Bits();
		a_cube->draw(a, b, c, &erase_1);
		tmp_rect = erase_1;
		erase_1 = tmp_rect | erase_1_old;
		erase_1_old = tmp_rect;

		acquire_sem(bm2_sem);
		release_sem(bm1_sem);
		
		if (buttons) 
			c = c + 0.02;

		if (k < 0.9)
			k = 0.9;

		posx0 = pos.x;
		posy0 = pos.y;

		a += 3*(0.03 * 1.5);
		b -= 3*(0.02141321321 * 1.5);
		c += 3*(0.00241421412 * 2.3);
		snooze(K);

		the_bitmap = bm2;
		base = (long *)the_bitmap->Bits();
		
		//erase_2_old = erase_2;
		a_cube->draw(a, b, c, &erase_2);
		tmp_rect = erase_2;
		erase_2 = tmp_rect | erase_2_old;
		erase_2_old = tmp_rect;

		acquire_sem(bm1_sem);
		release_sem(bm2_sem);
	}
	release_sem(bm1_sem);

	done = 1;
	snooze(300*1000);
	kill_thread(blit_task);
	the_bitmap = bm1;
	a_cube->draw(a, b, c, &erase_1);
	the_bitmap = bm2;
	a_cube->draw(a, b, c, &erase_2);
}

/*------------------------------------------------------------*/
