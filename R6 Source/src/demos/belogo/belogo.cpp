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


#include "app.h"
#include "window.h"
#include "offscreen.h"
#include "view.h"
#include "menubar.h"
#include "check_box.h"
#include "gr_types.h"
#include "scroll_bar.h"
#include "imagebuffer.h"
#include "proto.h"
#include "scroll_view.h"

#include <math.h>

#include "tsb.h"

/*------------------------------------------------------------*/

extern	long	color;

/*------------------------------------------------------------*/

void	TShape::TShape(TShowBit *aowner)
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
	xrand = abs(xrand);

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

void	TShape::draw(float new_alpha, float new_delta, float new_zeta)
{
	point	p1;
	point	p2;
	point	p3;
	point	p4;
	long	i;
	long	tmp;
	ulong	t0, t1;

	change_view_point(new_alpha, new_delta, new_zeta);
	sort_polys();

	for (i = 0; i < num_poly; i++) {
		p1.h = 200 + screen_x[l1[i]];
		p1.v = 200 + screen_y[l1[i]];

		p2.h = 200 + screen_x[l2[i]];
		p2.v = 200 + screen_y[l2[i]];
	
		p3.h = 200 + screen_x[l3[i]];
		p3.v = 200 + screen_y[l3[i]];
		
		p4.h = 200 + screen_x[l4[i]];
		p4.v = 200 + screen_y[l4[i]];
		
		tmp = acolor[i];
		tmp = calc_color(i);
		tmp = 10 + ((i * 18) / num_poly);

		color = (tmp << 24) | (tmp << 16) | (tmp << 8) | tmp;

		owner->fill_4(&p1, &p2, &p3, &p4);
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
		
		screen_x[i] = x / 64;
		screen_y[i] = y / 64;
		screen_z[i] = z / 64;
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

void	TCube::add_cube(long cx, long cy, long cz)
{
	add_cube1(cx, cy, cz);
}

//------------------------------------------------------------------------------

void	TCube::add_cube1(long cx, long cy, long cz)
{
	long	hs = 10;
	long	vs = 10;
	long	j;

	cx = (cx * 25) - 100 + rand1();
	cy = (cy * 25) - 60 + rand1();
	cz = (cz * 25) - 5 + rand1();

	j = num_point;

	add_point(cx - hs, cy + vs, cz + vs);		//0
	add_point(cx + hs, cy + vs, cz + vs);		//1
	add_point(cx + hs, cy - vs, cz + vs);		//2
	add_point(cx - hs, cy - vs, cz + vs);		//3

	add_point(cx - hs, cy + vs, cz - vs);	//4
	add_point(cx + hs, cy + vs, cz - vs);	//5
	add_point(cx + hs, cy - vs, cz - vs);	//6
	add_point(cx - hs, cy - vs, cz - vs);	//7

	add_poly(j + 0, j + 1, j + 2, j + 3, 15);
	add_poly(j + 4, j + 5, j + 6, j + 7, 40);

	add_poly(j + 0, j + 4, j + 7, j + 3, 60);
	add_poly(j + 3, j + 7, j + 6, j + 2, 80);
	add_poly(j + 1, j + 5, j + 6, j + 2, 150);
}

//------------------------------------------------------------------------------

void	TCube::new_one()
{
	long	i;
	
	rand_factor -= 1;

	if (rand_factor >= 0) {
		xrand = 0;
		num_point = 0;
		num_poly = 0;
		for (i = 0; i < 5; i++)
			add_cube(0, i, 0);

		add_cube(1, 0, 0);
		add_cube(2, 0, 0);
		add_cube(2, 1, 0);
		add_cube(3, 3, 0);

		for (i = 1; i < 4; i++) {
			add_cube(i, 2, 0);
			add_cube(i, 4, 0);
		}
		add_cube(1, 4, 0);
		add_cube(2, 4, 0);


		for (i = 0; i < 5; i++) 
			add_cube(6, i, 0);

		for (i = 7; i < 9; i++) {
			add_cube(i, 0, 0);
			add_cube(i, 2, 0);
			add_cube(i, 4, 0);
		}
	}
}

//------------------------------------------------------------------------------

void	TCube::TCube(TShowBit *aowner, long cx, long cy, long cz) :
	TShape(aowner)
{
	srand(1);

	rand_factor = 155;
}

