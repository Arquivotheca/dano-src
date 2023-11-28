//******************************************************************************
//
//	File:		shape.cpp
//
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992, Be Incorporated
//
//	Change History:
//
//	5/15/93		bgs	new today
//
//******************************************************************************


#include <stdlib.h>
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
#ifndef _SCREEN_H
#include <Screen.h>
#endif


#include <math.h>

#include "tsb.h"

/*------------------------------------------------------------*/

extern	long	color;

/*------------------------------------------------------------*/

uchar	mapper1[33];
uchar	mapper2[33];

/*------------------------------------------------------------*/

TShape::TShape(TShowBit *aowner)
{
	owner = aowner;
	num_point = 0;
	num_poly = 0;
	cur_alpha = 0.0;
	cur_delta = 0.0;
	cur_zeta  = 0.0;
}

/*------------------------------------------------------------*/

TShape::~TShape()
{
}

//------------------------------------------------------------------------------

long	rand_factor;
long	xrand;

//------------------------------------------------------------------------------

long	rand1()
{
	long	tmp;

	xrand = xrand * 371 + 2781;
	xrand = abs((int)xrand);

	if (rand_factor > 0) {
		tmp = xrand % 32768;
		tmp -= 16384;
		return((tmp * rand_factor) / 12000);
	}
	else
		return 0;
}


/*------------------------------------------------------------*/

void	TShape::add_point(long x, long y, long z)
{
	x *= 64;
	y *= 64;
	z *= 64;

	pt_x[num_point] = x;
	pt_y[num_point] = y;
	//spt_x[num_point] = x;
	//spt_y[num_point] = y;
	pt_z[num_point] = z;
	num_point++;
}

/*------------------------------------------------------------*/

void	TShape::add_poly(long p1, long p2, long p3, long p4, long co)
{
	l1[num_poly] = p1;
	l2[num_poly] = p2;
	l3[num_poly] = p3;
	l4[num_poly] = p4;
	acolor[num_poly] = co;
	num_poly++;
}

//------------------------------------------------------------------------------

long	TShape::calc_mid(long pn)
{
	long	sum;

	sum = screen_z[l1[pn]];
	sum += screen_z[l2[pn]];
	sum += screen_z[l3[pn]];
	sum += screen_z[l4[pn]];
	sum /= 4;
	return(sum);
}

//------------------------------------------------------------------------------

void	TShape::sort_polys()
{
	long	mid1;
	long	mid2;
	long	i;
	long	j;
	long	tmp;
	long	step;

	for (i = 0; i < (num_poly); i++)
		zs[i] = calc_mid(i);


	for (step = num_poly / 2; step > 0; step /= 2)
		for (i = step; i < num_poly; i++)
			for (j=i - step; j >= 0 && zs[j] > zs[j+step]; j -= step) {
				tmp = zs[j];
				zs[j] = zs[j + step];
				zs[j + step] = tmp;

				tmp = l1[j];	
				l1[j] = l1[j + step];
				l1[j + step] = tmp;

				tmp = l2[j];	
				l2[j] = l2[j + step];
				l2[j + step] = tmp;

				tmp = l3[j];	
				l3[j] = l3[j + step];
				l3[j + step] = tmp;

				tmp = l4[j];	
				l4[j] = l4[j + step];
				l4[j + step] = tmp;
				
				tmp = acolor[j];	
				acolor[j] = acolor[j + step];
				acolor[j + step] = tmp;
			}
}

//------------------------------------------------------------------------------

long	TShape::calc_color(long i)
{
	long	dx;
	long	dy;
	long	dz;
	long	br;

	dx = screen_x[l1[i]] - screen_x[l3[i]];

	br = 0;
	br = dx * dx;
	br = br / 20;
	br += 10;
	if (br > 26)
		br = 26;

	return(br);
}

//------------------------------------------------------------------------------

void	TShape::draw(float new_alpha, float new_delta, float new_zeta, BRect *bounding)
{
	BPoint	p1;
	BPoint	p2;
	BPoint	p3;
	BPoint	p4;
	long	i;
	long	tmp;
	long	tmp1;
	ulong	t0, t1;
	BRect	limit;

	limit.top = 1000;
	limit.bottom = -1000;
	limit.left = 1000;
	limit.right = -1000;

	change_view_point(new_alpha, new_delta, new_zeta);
	sort_polys();

	for (i = 0; i < num_point; i++) {
		if (screen_y[i] > limit.bottom)
			limit.bottom = screen_y[i];
		if (screen_y[i] < limit.top)
			limit.top = screen_y[i];
		if (screen_x[i] > limit.right)
			limit.right = screen_x[i];
		if (screen_x[i] < limit.left)
			limit.left = screen_x[i];
	}
	limit.top += (110 - 2);
	limit.left += (170 - 2);
	limit.bottom += (110 + 2);
	limit.right += (170 + 2);
	*bounding = limit;

	for (i = 0; i < num_poly; i++) {
		/*
		p1.x = 160 + screen_x[l1[i]];
		p1.y = 120 + screen_y[l1[i]];

		p2.x = 160 + screen_x[l2[i]];
		p2.y = 120 + screen_y[l2[i]];
	
		p3.x = 160 + screen_x[l3[i]];
		p3.y = 120 + screen_y[l3[i]];
		
		p4.x = 160 + screen_x[l4[i]];
		p4.y = 120 + screen_y[l4[i]];
		*/
		p1.x = 170 + screen_x[l1[i]];
		p1.y = 110 + screen_y[l1[i]];

		p2.x = 170 + screen_x[l2[i]];
		p2.y = 110 + screen_y[l2[i]];
	
		p3.x = 170 + screen_x[l3[i]];
		p3.y = 110 + screen_y[l3[i]];
		
		p4.x = 170 + screen_x[l4[i]];
		p4.y = 110 + screen_y[l4[i]];
		tmp1 = acolor[i];
		tmp = calc_color(i);
		//tmp = 10 + ((i * 18) / num_poly);

		if (tmp1 == 0)
			tmp = mapper1[tmp];
		if (tmp1 == 1)
			tmp = mapper2[tmp];

		color = (tmp << 24) | (tmp << 16) | (tmp << 8) | tmp;

		if (l3[i] == l4[i])
			owner->fill_triangle(p1, p2, p3);
		else

			owner->fill_4(p1, p2, p3, p4);
	}
}

//------------------------------------------------------------------------------

void	TShape::change_view_point(float new_alpha, float new_delta, float new_zeta)
{
	long	sina;
	long	cosa;
	long	sinb;
	long	cosb;
	long	sinc;
	long	cosc;

	long	i;
	long	x, y, z;
	long	x0, y0, z0;
	long	hs, vs, ms;
	long	k;
	float	kk;

	
	cur_alpha = new_alpha;
	cur_delta = new_delta;
	cur_zeta  = new_zeta;


	ms = 1500;

	sina = sin(new_alpha) * 65535;
	cosa = cos(new_alpha) * 65535;

	sinb = sin(new_delta) * 65535;
	cosb = cos(new_delta) * 65535;

	sinc = sin(new_zeta) * 65535;
	cosc = cos(new_zeta) * 65535;

	for (i = 0; i < num_point; i++) {
		/*
		if ((i % 40) == 0) {
			kk = 0.95 + ((i * 0.05) / (num_point * 1.0));
			sina = sin(new_alpha * kk) * 65535;
			cosa = cos(new_alpha * kk) * 65535;

			sinb = sin(new_delta * kk) * 65535;
			cosb = cos(new_delta * kk) * 65535;

			sinc = sin(new_zeta * kk) * 65535;
			cosc = cos(new_zeta * kk) * 65535;
		}
		*/

		x = pt_x[i];
		y = pt_y[i];
		z = pt_z[i];

		x0 = x * cosa - y * sina;
		y0 = x * sina + y * cosa;

		x = x0 >> 16;
		y = y0 >> 16;
		
		x0 = x * cosc - z * sinc;
		z0 = x * sinc + z * cosc;

		if (x0 < 0)
			x0 -= 32760;
		else
			x0 += 32760;

		x = x0 >> 16;
		z = z0 >> 16;

		y0 = y * cosb - z * sinb;
		z0 = y * sinb + z * cosb;

		y = y0 >> 16;

		if (y0 < 0)
			y0 -= 32760;
		else
			y0 += 32760;

		y = y0 >> 16;

		z = z0 >> 16;
		
		z = z - 22000;		// observer distance

		k = -z / 155;

		z = z + 22000;		// observer distance
		screen_x[i] = x / k;
		screen_y[i] = y / k;
		screen_z[i] = z / 64;
	}
}

//------------------------------------------------------------------------------

long	TBird::wing(long cx, long cy, long cz, long wlen)
{
	long	j;
	long	width1;
	long	width2;
	long	hs;

	j = num_point;

	width1 = 35;
	width2 = 12;
	hs     = 11;
	if (wlen < 0)
		hs = -hs;

	add_point(cx + hs, cy, cz - (width1 / 2));		// 0
	add_point(cx + hs, cy, cz + (width1 / 2));
	add_point(cx + hs + wlen, cy, cz - (width2 / 2));
	add_point(cx + hs + wlen, cy, cz + (width2 / 2));

	add_poly(j + 0, j + 1, j + 3, j + 2, 60);

	add_point(cx + hs + wlen * 2, cy, cz);

	add_poly(j + 3, j + 2, j + 4, j + 4, 60);
	return(j + 4);
}

//------------------------------------------------------------------------------

void	TBird::body(long cx, long cy, long cz)
{
	long	hs;
	long	vs;
	long	sz;
	long	j;
	long	dd;
	long	tl;

	j = num_point;

	hs = 11;
	vs = 6;
	sz = 80;
	dd = 7;
	tl = 20;

	add_point(cx - hs, cy + vs, cz + sz);		//0
	add_point(cx + hs, cy + vs, cz + sz);		//1
	add_point(cx + hs, cy - vs, cz + sz);		//2
	add_point(cx - hs, cy - vs, cz + sz);		//3

	add_point(cx - hs, cy + vs, cz - sz);	//4
	add_point(cx + hs, cy + vs, cz - sz);	//5
	add_point(cx + hs, cy - vs, cz - sz);	//6
	add_point(cx - hs, cy - vs, cz - sz);	//7

	add_point(cx, cy + 25, cz + sz + 80);

	add_point(cx - hs - dd, cy, cz - sz - tl);
	add_point(cx + hs + dd, cy, cz - sz - tl);

	add_poly(j + 7, j + 6, j + 9, j + 10, 80);
	add_poly(j + 4, j + 5, j + 10, j + 9, 80);
	add_poly(j + 7, j + 4, j + 9, j + 9, 80);
	add_poly(j + 6, j + 5, j + 10, j + 10, 80);

	add_poly(j + 0, j + 1, j + 2, j + 3, 15);
	add_poly(j + 4, j + 5, j + 6, j + 7, 40);

	add_poly(j + 0, j + 4, j + 7, j + 3, 60);
	add_poly(j + 3, j + 7, j + 6, j + 2, 80);
	add_poly(j + 1, j + 5, j + 6, j + 2, 150);
	add_poly(j + 0, j + 4, j + 5, j + 1, 150);

	add_poly(j + 0, j + 8, j + 1, j + 1, 60);
	add_poly(j + 0, j + 3, j + 8, j + 8, 60);
	add_poly(j + 3, j + 8, j + 2, j + 2, 60);
	add_poly(j + 2, j + 1, j + 8, j + 8, 60);
}

//------------------------------------------------------------------------------

TBird::TBird(TShowBit *aowner, long cx, long cy, long cz) :
	TShape(aowner)
{
	body(0, 0, 0);
	wing_phase = 4;
	end_wing1_ref = wing(0, 0, 0, -80);
	end_wing2_ref = wing(0, 0, 0, 80);
}

//------------------------------------------------------------------------------

void	TBird::flap_flap()
{
	long	wspeed;

	wspeed = 5;

	wing_phase += 1;
	wing_phase = wing_phase % 32;


	if (wing_phase < 16) {
		pt_y[end_wing1_ref] += 120 * wspeed;
		pt_y[end_wing2_ref] += 120 * wspeed;
	
		pt_y[end_wing1_ref - 1] += 32 * wspeed;
		pt_y[end_wing1_ref - 2] += 32 * wspeed;
		pt_y[end_wing2_ref - 1] += 32 * wspeed;
		pt_y[end_wing2_ref - 2] += 32 * wspeed;
	}	
	else {
		pt_y[end_wing1_ref] -= 120 * wspeed;
		pt_y[end_wing2_ref] -= 120 * wspeed;

		pt_y[end_wing1_ref - 1] -= 32 * wspeed;
		pt_y[end_wing1_ref - 2] -= 32 * wspeed;
		pt_y[end_wing2_ref - 1] -= 32 * wspeed;
		pt_y[end_wing2_ref - 2] -= 32 * wspeed;
	}	
}

//------------------------------------------------------------------------------

void	TCube::explode_in()
{
	long	i;
	long	j;
	long	dh;
	long	dv;

	for (j = 0; j < num_point; j++) {
		if ((j % 8) == 0) {
			dh = spt_x[j] / 32;
			dv = spt_y[j] / 32;
		}
		pt_x[j] -= dh;
		pt_y[j] -= dv;
	}
}

//------------------------------------------------------------------------------

void	TCube::explode_out()
{
	long	i;
	long	dh;
	long	dv;
	long	j;

	for (j = 0; j < num_point; j++) {
		if ((j % 8) == 0) {
			dh = spt_x[j] / 32;
			dv = spt_y[j] / 32;
		}
		pt_x[j] += dh;
		pt_y[j] += dv;
	}
}


//------------------------------------------------------------------------------

void	TCube::add_pyramid(long cx, long cy, long cz, long color)
{
	long	j;

	cx = (cx * 45) - 100 + rand1();
	cy = (cy * 45) - 60 + rand1();
	cz = (cz * 45) - 5 + rand1();

	j = num_point;

	add_point(cx, cy, cz);		//0
	add_point(cx + 20, cy + 0, cz + 0);		//1
	add_point(cx + 10, cy + 0, cz + 22);		//2
	add_point(cx + 10, cy + 22, cz + 10);		//3


	add_poly(j + 0, j + 1, j + 2, j + 2, color);
	add_poly(j + 0, j + 2, j + 3, j + 3, color);

	add_poly(j + 0, j + 1, j + 3, j + 3, color);
	add_poly(j + 1, j + 2, j + 3, j + 3, color);
}

//------------------------------------------------------------------------------

void	TCube::add_cube(long cx, long cy, long cz, long color)
{
	long	hs = 16;
	long	vs = 16;
	long	j;

	add_pyramid(cx, cy, cz, color);
	return;
	cx = (cx * 45) - 100 + rand1();
	cy = (cy * 45) - 60 + rand1();
	cz = (cz * 45) - 5 + rand1();

	j = num_point;

	add_point(cx - hs, cy + vs, cz + vs);		//0
	add_point(cx + hs, cy + vs, cz + vs);		//1
	add_point(cx + hs, cy - vs, cz + vs);		//2
	add_point(cx - hs, cy - vs, cz + vs);		//3

	add_point(cx - hs, cy + vs, cz - vs);	//4
	add_point(cx + hs, cy + vs, cz - vs);	//5
	add_point(cx + hs, cy - vs, cz - vs);	//6
	add_point(cx - hs, cy - vs, cz - vs);	//7

	add_poly(j + 0, j + 1, j + 2, j + 3, color);
	add_poly(j + 4, j + 5, j + 6, j + 7, color);

	add_poly(j + 0, j + 4, j + 7, j + 3, color);
	add_poly(j + 3, j + 7, j + 6, j + 2, color);
	add_poly(j + 1, j + 5, j + 6, j + 2, color);
}

//------------------------------------------------------------------------------

void	init_mapper()
{
	long		i;
	rgb_color	c;

	BScreen screen( B_MAIN_SCREEN_ID );
	
	for (i = 0; i < 32; i++) {
		c.red = 60 + i * 6;
		c.green = 20;
		c.blue = 20;
		mapper2[i] = screen.IndexForColor(c);
		c.red = 20;
		c.green = 20;
		c.blue = 60 + i * 6;
		mapper1[i] = screen.IndexForColor(c);
	}
	mapper1[32] = mapper1[31];
	mapper2[32] = mapper2[31];
}

//------------------------------------------------------------------------------

void	TCube::new_one()
{
	long	i;
	
	init_mapper();
	rand_factor = 0;

	if (rand_factor >= 0) {
		xrand = 0;
		num_point = 0;
		num_poly = 0;
		for (i = 0; i < 5; i++)
			add_cube(0, i, 0, 0);

		add_cube(1, 0, 0, 0);
		add_cube(2, 0, 0, 0);
		add_cube(2, 1, 0, 0);
		add_cube(3, 3, 0, 0);

		for (i = 1; i < 4; i++) {
			add_cube(i, 2, 0, 0);
			add_cube(i, 4, 0, 0);
		}
		add_cube(1, 4, 0, 0);
		add_cube(2, 4, 0, 0);


		for (i = 0; i < 5; i++) 
			add_cube(6, i, 0, 1);

		for (i = 7; i < 9; i++) {
			add_cube(i, 0, 0, 1);
			add_cube(i, 2, 0, 1);
			add_cube(i, 4, 0, 1);
		}
	}

	for (i = 0; i < 5; i++) {
		add_cube(i, 2, 3, i % 2);
	}

	for (i = 0; i < 5; i++) {
		add_cube(2, i, -2, i % 2);
	}
}

//------------------------------------------------------------------------------

TCube::TCube(TShowBit *aowner, long cx, long cy, long cz) :
	TShape(aowner)
{
	srand(1);

	rand_factor = 155;
}

