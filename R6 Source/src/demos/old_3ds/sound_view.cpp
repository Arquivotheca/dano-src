#include "sound_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Screen.h>
#include <Region.h>
#include <Application.h>
#include <OS.h>

#define	p1_x	150
#define	p1_y	100

#define	p2_x	120
#define	p2_y	250

#define	NC	16

#define	BVP	(206+120)

#define	HS	900
#define	VS	600

//--------------------------------------------------------------

long	init_p(void *p);
long	init_anim(void *p);

extern "C" void *fast_memset(void *s, int c, size_t n);


#define	MEMSET memset

//--------------------------------------------------------------

long	cmix(long v1, long v2)
{
	long	tmp;
	
	tmp = v2 + v1*1.6;
	tmp = tmp / 2;
	if (tmp > 255) tmp = 255;
	return tmp;
}

//--------------------------------------------------------------

float b_sqrt(float x) {
	unsigned long	val;
	float			y,z,t;
	float	        flottant, tampon;
	
	flottant = x;
	val = *((unsigned long*)&flottant);
	val >>= 1;
	val += 0x1FC00000L;
	*((unsigned long*)&tampon) = val;
	y = tampon;
	z = y*y+x;
	t = y*y-x;
	y *= (float)4.0;
	x = z*z;
	t = t*t;
	y = z*y;
	t = (float)2.0*x-t;
	return t/y;
}

//--------------------------------------------------------------


int		TSoundView::add_ppoint(float x1, float z1, float y1, long obj_id, ulong part)

{
	poly_point[ppoint_count].x1 = x1;
	poly_point[ppoint_count].y1 = -y1;
	poly_point[ppoint_count].z1 = z1;
	poly_point[ppoint_count].object_id = obj_id;
	poly_point[ppoint_count].part = part;
	poly_point[ppoint_count].touched = 1;
	ppoint_count++;
	
	return (ppoint_count - 1);
}


//--------------------------------------------------------------


void	TSoundView::add_poly(long i1, long i2, long i3, long i4, long c, long obj_id)
{
	polys[poly_count].pt1_index = i1;
	polys[poly_count].pt2_index = i2;
	polys[poly_count].pt3_index = i3;
	polys[poly_count].pt4_index = i4;
	polys[poly_count].color_index = c;
	polys[poly_count].object_id = obj_id;
	poly_count++;
	need_sort = 1;
}


//--------------------------------------------------------------

void	TSoundView::add_vector(float x1, float y1, float z1,
								float x2, float y2, float z2, long map)
{
	varray[vector_count].x1 = x1;
	varray[vector_count].y1 = y1;
	varray[vector_count].z1 = z1;	
	varray[vector_count].x2 = x2;
	varray[vector_count].y2 = y2;
	varray[vector_count].z2 = z2;	
	varray[vector_count].color = map;	
	vector_count++;
	need_vector = 1;
}

//--------------------------------------------------------------

void	TSoundView::add_halo(float size_x, float size_y, long id)
{
	float	k;
	long	pt_id;
	long	m1,m2,m3,m4;
	
	for (k = 0.3; k <= 1.0; k *= 1.2) {
		m1 = add_ppoint(-size_x*k, -size_y*k, 0, id);
		m2 = add_ppoint(+size_x*k, -size_y*k, 0, id);
		m3 = add_ppoint(+size_x*k, +size_y*k, 0, id);
		m4 = add_ppoint(-size_x*k, +size_y*k, 0, id);
		add_poly(m1,m2,m3,m4,15, id);
	}
}

//--------------------------------------------------------------

char	TSoundView::has_selection()
{
	long	i;
	
	for (i = 0; i < 32; i++) {
		if (obj_state[i]) {
			return 1;
		}
	}
	return 0;
}

//--------------------------------------------------------------

void	TSoundView::composit_pannel1()
{
	long	y,x;
	long	dx,dy;
	long	ty;
	uchar	*main;
	uchar	*pannel;
	uchar	v;
	long	drowbyte = HS;
	long	srowbyte = 256;
	long	mx;
	long	rx;
	
	
	if (!has_selection())
		return;
		
	dx  = pannel1_rect.left;
	dy  = pannel1_rect.top;

	if (dx < 0)
		mx = -dx;
	else
		mx = 0;

	if ((dx + p1_x) > size_x) {
		rx = (dx+p1_x)-size_x;
	}
	else
		rx = 0;
		
	for (y = 0; y < p1_y; y++) {	
		main = (uchar *)the_bits->Bits();
		pannel = (uchar *)pannel1->Bits();
		
		ty = y + dy;
		
		if (ty < 0 || ty > size_y)
			goto loop;
			
		pannel += (y*srowbyte);
		main += (ty) * drowbyte;
		main += dx;
		
		main += mx;
		pannel += mx;
		
		for (x = mx; x < (p1_x - rx); x+=4) {
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
			
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
			
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
			
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
		}
loop:;
	}
}

//--------------------------------------------------------------

void	TSoundView::composit_pannel2()
{
	long	y,x;
	long	dx,dy;
	long	ty;
	uchar	*main;
	uchar	*pannel;
	uchar	v;
	long	drowbyte = HS;
	long	srowbyte = 256;
	long	mx;
	long	rx;
	
	
		
	dx  = pannel2_rect.left;
	dy  = pannel2_rect.top;

	if (dx < 0)
		mx = -dx;
	else
		mx = 0;

	if ((dx + p2_x) > size_x) {
		rx = (dx+p2_x)-size_x;
	}
	else
		rx = 0;
		
	for (y = 0; y < p2_y; y++) {	
		main = (uchar *)the_bits->Bits();
		pannel = (uchar *)pannel2->Bits();
		
		ty = y + dy;
		
		if (ty < 0 || ty > size_y)
			goto loop;
			
		pannel += (y*srowbyte);
		main += (ty) * drowbyte;
		main += dx;
		
		main += mx;
		pannel += mx;
		
		for (x = mx; x < (p2_x - rx); x+=4) {
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
			
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
			
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
			
			v = composit[((*main)<<8) + (*pannel)];
			pannel++;
			*main++ = v;
		}
loop:;
	}
}

//--------------------------------------------------------------

void	TSoundView::update_infos()
{
	long	i;
	
	UpdateTime(pan2_view);
	
	for (i = 0; i < 32; i++) {
		if (obj_state[i]) {
			Draw1(pan1_view, i);
			return;
		}
	}
}

//--------------------------------------------------------------


void	TSoundView::handle_button(long i)
{
	ulong		buttons;
	BPoint		where;
	char		inv = 0;	
	float		step;
	
	
	step = 0.2;
	
	if (view_state == WAVE_VIEW) {
		step = wave_viewers[0]->scale;
		step = 0.001*step;
	}
	
	if (i == 1 && b1_state[1]) {
		b1_state[1] = 0;
		pan2_view->Window()->Lock();
		DrawButtons(pan2_view);
		pan2_view->Window()->Unlock();
		time_mode = FULL_DRAW;
		Animate();
		inv = 1;
	}
		

	time_mode  = FULL_DRAW;
	do {
		pause_main = 1;
		GetMouse(&where, &buttons);
		where.y -= pannel2_rect.top;
		where.x -= pannel2_rect.left;
		where.x -= bm_hp;
		
		if (b1_state[i] != 	(b1_rect[i].Contains(where))) {
			b1_state[i] ^= 1;
		}
		pan2_view->Window()->Lock();
		DrawButtons(pan2_view);
		pan2_view->Window()->Unlock();
		time_mode = FULL_DRAW;
		Animate();
		snooze(30000);
		
		if (b1_state[i]) {
			if (i == 0) {
				mtime-=step;
				if (mtime<0) mtime = 0;
			}
			if (i == 2) {
				mtime+=step;
			}
		}
	} while(buttons);	
	
	if (inv)
		b1_state[1] ^= 0x01;
	
	if (b1_state[1])
		goto skip;
	
	
	pause_main = 2;

skip:;

	if (i != 1)
		b1_state[i] = 0;


	time_mode = FULL_DRAW;
	Animate();
}

//--------------------------------------------------------------

char	TSoundView::pannel2_click(BPoint where0)
{
	BRect		r;
	float		minus_time;
	float		plus_time;
	float		vs;
	float		mid;
	float		dy;
	long		i;
	long		min_d;
	long		d;
	float		new_t;
	ulong		buttons;
	BPoint		where;
	float		real_d;
	float		ratio;
	BRect		cur;


	for (i = 0; i < 3; i++) {
		if (b1_rect[i].Contains(where0)) {
			handle_button(i);
			return 1;
		}
	}
	
	
	r.top = 40;
	r.bottom = p2_y - 20;
	r.left = 21;
	r.right = 80;
	
	if (r.Contains(where0)) {
		
		min_d = 100;
	
		for (i = 1; i < 9; i++) {
			real_d = where0.y - tv[i];
			d = fabs(real_d);
			
			if (d < min_d) {
				if (real_d > 0) {
					ratio = real_d/(tv[i+1]-tv[i]); 
					new_t = lt[i]*(1.0-ratio) + lt[i+1]*(ratio);
				}
				else {
					ratio = real_d/(tv[i]-tv[i-1]); 
					new_t = lt[i]*(1.0-ratio) + lt[i-1]*(ratio);
				}
				min_d = d;
			}
		}	
		 
		do {
			GetMouse(&where, &buttons);
			//dtime = (new_t + dtime*7.0) / 8.0;
			dtime = new_t;
			time_mode = CENTER_ONLY;
			Animate();
			snooze(15000.0);
		} while(buttons);
		
		mtime = new_t;
		time_mode = FULL_DRAW;
		Animate();
		time_mode = TOP_ONLY;
		return 1;
	}
	return 0;
}

//--------------------------------------------------------------

#define	mr	2

void	TSoundView::DrawButtons(BView *off)
{
	BRect		r;
	long		mid;
	long		dh,dv;
	rgb_color	but_color;
	rgb_color	but_color_h;
	
	
	but_color.red = 255;
	but_color.green = 255;
	but_color.blue = 255;
	 
	but_color_h.red = 255;
	but_color_h.green = 250;
	but_color_h.blue = 0;
	
	r.top = 33;
	r.left = 17 + mr;
	r.bottom = r.top + 18;
	r.right = r.left + 22;
	
	b1_rect[0] = r;
	
	mid = (r.top+r.bottom) / 2;
	
	off->SetHighColor(100,100,100);
	off->FillRect(r);
	off->SetHighColor(60,60,60);
	off->StrokeRect(r);
	
	if (b1_state[0]) {
		dh = -1;
		dv = -1;
	}
	else {
		dh = 1;
		dv = 1;
	}
	
	off->SetHighColor(50,30,30);
	off->StrokeTriangle(BPoint(r.left + 6 + dh, mid + dv),
					   	BPoint(r.left + 14 + dh, mid + 4 + dv),
					  	BPoint(r.left + 14 + dh, mid - 4 + dv));
		
	if (b1_state[0])
		off->SetHighColor(but_color_h);
	else			  	
		off->SetHighColor(but_color);
	
	off->FillTriangle(BPoint(r.left + 6, mid),
					   	BPoint(r.left + 14, mid + 4),
					  	BPoint(r.left + 14, mid - 4));
					  	
					  	
	
	r.left = 42 + mr;
	r.right = 74 + mr;
	b1_rect[1] = r;
	
	if (b1_state[1]) {
		dh = -1;
		dv = -1;
	}
	else {
		dh = 1;
		dv = 1;
	}

	off->SetHighColor(100,100,100);
	off->FillRect(r);
	off->SetHighColor(60,60,60);
	off->StrokeRect(r);
	off->SetHighColor(50,30,30);
	
	off->FillRect(BRect(r.left + 9 + dh, mid + dv - 4,
					   	 r.right - 9 + dh, mid + dv + 4));
					  						  	
	if (b1_state[1])
		off->SetHighColor(but_color_h);
	else			  	
		off->SetHighColor(but_color);
	off->FillRect(BRect(r.left + 9, mid - 4,
					   	 r.right - 9, mid + 4));
	
	
	
					  	
	r.top = 33;
	r.left = 77 + mr;
	
	r.bottom = r.top + 18;
	r.right = r.left + 22;
	
	b1_rect[2] = r;
	
	if (b1_state[2]) {
		dh = -1;
		dv = -1;
	}
	else {
		dh = 1;
		dv = 1;
	}

	mid = (r.top+r.bottom) / 2;
	
	off->SetHighColor(100,100,100);
	off->FillRect(r);
	off->SetHighColor(60,60,60);
	off->StrokeRect(r);
	
	
	off->SetHighColor(50,30,30);
	off->StrokeTriangle(BPoint(r.left + 14 + dh, mid + dv),
					   	BPoint(r.left + 6 + dh, mid + 4 + dv),
					  	BPoint(r.left + 6 + dh, mid - 4 + dv));
					  	
	if (b1_state[2])
		off->SetHighColor(but_color_h);
	else			  	
		off->SetHighColor(but_color);
		
	off->FillTriangle(BPoint(r.left + 14, mid),
					   	BPoint(r.left + 6, mid + 4),
					  	BPoint(r.left + 6, mid - 4));
}


//--------------------------------------------------------------

void	TSoundView::UpdateTime(BView *off)
{
	char		buf[64];
	long		tmp;
	float		ftmp;
	long		h,m;
	float		s;
	BRect		r;
	BRect		erase;
	float		vs;
	float		mid;
	float		i;
	float		pi = 3.14159265;
	long		k;
	float		minus_time;
	float		plus_time;
	float		v;
	float		cur;
	long		is;
	rgb_color	c;
	
	off->Window()->Lock();
	off->SetHighColor(0,0,0);
	
	if (time_mode == FULL_DRAW) {
		off->FillRect(BRect(10,29, p2_x-10, p2_y-10));
		DrawButtons(off);
	}
		
	off->SetHighColor(80,0,0);
	off->FillRect(BRect(10,5, p2_x-10, 23));
	
	off->SetHighColor(50, 50, 255);
	off->SetFontSize(11);
	DrawStringRight(off, BPoint(51, 16), "Time : ");
	
	tmp = mtime;
	
	plus_time = dtime;
	minus_time = max_time - dtime;
	
	h = tmp / 3600;
	m = tmp - (h * 3600);
	m /= 60;
	s = mtime - (h*3600) - (m*60);
	
	sprintf(buf, "%.2ld:%.2ld:%2.2f", h, m, s);
	off->DrawString(buf);
	
	if (time_mode == TOP_ONLY)
		goto out;


	r.top = 70;
	r.bottom = p2_y - 15;
	r.left = 21;
	r.right = 30;
	
	
	
	off->SetHighColor(140, 50, 50);
	
	
	if (time_mode != CENTER_ONLY)
		off->FillRect(r);
	
	
	vs = r.bottom - r.top;
	
	off->SetHighColor(255,0,0);
	
	mid = (r.top + r.bottom) / 2;
	
	vs /= 2.0;
	
	k = 1;
	
	off->SetFontSize(10);
	
	tv[0] = r.top;
	tv[9] = r.bottom;
	tv[10] = r.bottom;
	
	c.red = 50;
	c.green = 100;
	c.blue = 20;
	
	if (time_mode != CENTER_ONLY) {
		off->BeginLineArray(100);
		for (i = -pi/2 + pi/10.0; i < pi/2 - (pi/20.0); i+= pi/40.0) {
			m = mid + (vs*sin(i));
			off->AddLine(BPoint(r.right+1, m),BPoint(r.right + 3,  m), c); 
		}
	
		c.red = 140;
		c.green = 10;
		c.blue = 20;
		
		for (i = -pi/2 + pi/10.0; i < pi/2; i+= pi/10.0) {
			tv[k] = mid + (vs*sin(i));
			off->AddLine(BPoint(r.left, tv[k]),BPoint(r.right + 8,  tv[k]), c); 
			k++;
		}
		off->EndLineArray();
	}
	
	
	if (time_mode == CENTER_ONLY) {
		erase.left = r.right + 14;
		erase.right = erase.left + 40;
		erase.top = tv[5] - 8;
		erase.bottom = tv[5] + 7;
		off->SetHighColor(0,0,0);
		off->FillRect(erase);
	}
	
	
	for (i = -4; i <= 4; i++) {
		if (time_mode == CENTER_ONLY && i != 0)
			goto skip;
			
		v = i / 4.0;
		v = (v*v);
		if (i < 0) v = -v;
		
		off->MovePenTo(BPoint(r.right + 14, tv[(int)i + 5] + 4));
		
		if (v < 0.0) {
			cur = dtime + plus_time * v;
		}
		else {
			cur = dtime + minus_time * v;
		}
		
		tmp = cur;
		
		lt[(int)i+5] = cur;
		
		h = tmp / 3600;
		m = tmp - (h * 3600);
		m /= 60;
		is = cur - (h*3600) - (m*60);
		
		sprintf(buf, "%.2ld:%.2ld:%.2ld", h, m, is);
		if (i == 0) 
			off->SetHighColor(0,200,0);
		else if (i == 1)
			off->SetHighColor(255,0,0);
		off->DrawString(buf);
skip:;
	}
out:;
	off->Sync();
	off->Window()->Unlock();
	
}	

//--------------------------------------------------------------
	
void	TSoundView::DrawStringRight(BView *off, BPoint where, char *s)
{
	float	w;
	
	w = off->StringWidth(s);
	
	where.x -= w;
	
	off->MovePenTo(where);
	off->DrawString(s);
}

//--------------------------------------------------------------

void	TSoundView::Draw1(BView *off, long c_id)
{
	char	buf[32];
	float	vol;
	float	rev;
	
	off->Window()->Lock();
	off->SetHighColor(0,0,0);
	off->FillRect(BRect(10,10, p1_x-10, p1_y-10));
	off->SetHighColor(50, 50, 255);
	off->SetFontSize(11);
	DrawStringRight(off, BPoint(70, 20), "Channel : ");
	
	off->SetHighColor(200, 150, 150);
	off->DrawString(chan[c_id]->cname);
	
	off->SetHighColor(50, 50, 255);
	DrawStringRight(off, BPoint(70, 36), "Left Level : ");
	sprintf(buf, "%2.1f %", chan[c_id]->left_weight*100);
	off->SetHighColor(200, 150, 150);
	off->DrawString(buf);
	
	off->SetHighColor(50, 50, 255);
	DrawStringRight(off, BPoint(70, 36 + 16), "Right Level : ");
	sprintf(buf, "%2.1f %", chan[c_id]->right_weight*100);
	off->SetHighColor(200, 150, 150);
	off->DrawString(buf);
	
	off->SetHighColor(50, 50, 255);
	DrawStringRight(off, BPoint(70, 36 + 16 + 16), "Volume : ");
	vol = chan[c_id]->GetGain()*100;
	vol = vol / 398.0;
	vol *= 100.0;
	if (vol > 100.0)
		vol = 100.0;
		
	if (vol < 0)
		vol = 0;
		
	sprintf(buf, "%2.1f %", vol);
	off->SetHighColor(200, 150, 150);
	off->DrawString(buf);
	
	off->SetHighColor(50, 50, 255);
	
	DrawStringRight(off, BPoint(70, 36 + 16 + 16 + 16), "Reverbe : ");
	rev = -channel_reverbe[c_id]*3.3333333333;
	if (rev > 100.0)
		rev = 100.0;
		
	if (rev < 0)
		rev = 0;
		
	sprintf(buf, "%2.1f %", rev);
	off->SetHighColor(200, 150, 150);
	off->DrawString(buf);

	off->Sync();
	off->Window()->Unlock();
	
}


//--------------------------------------------------------------

#define	C_S	5

void	TSoundView::Draw1(BView *off)
{
	off->Window()->Lock();
	off->SetHighColor(0,0,0);
	off->FillRect(BRect(0,0,1000,1000));
	//off->SetHighColor(255, 255, 255);
	//off->SetHighColor(0, 0, 255);
	off->SetDrawingMode(B_OP_OVER);
	//off->MovePenTo(BPoint(50, 50));
	//off->SetFontSize(14);
	//off->DrawString("This is a test");
	//off->MovePenTo(BPoint(50, 80));
	//off->DrawString("This is a test");
	//off->MovePenTo(BPoint(50, 120));
	//off->DrawString("This is a test");
	//off->SetHighColor(25, 25,25); 
	off->SetHighColor(125, 25,25); 
	off->StrokeRect(BRect(0,0,p1_x - 1, p1_y - 1));
	off->SetHighColor(B_TRANSPARENT_32_BIT);
	off->SetHighColor(8,8,8);

	off->FillTriangle(BPoint(C_S + 1, -1),
					   BPoint(-1, -1),
					   BPoint(-1,C_S + 1));
					   
	off->FillTriangle(BPoint(C_S + 1, p1_y + 1),
					   BPoint(-1, p1_y + 1),
					   BPoint(-1, p1_y - C_S -1));
					   
	off->FillTriangle(BPoint(p1_x - C_S -1, p1_y + 1),
					  BPoint(p1_x + 1, p1_y + 1),
					   BPoint(p1_x + 1, p1_y - C_S -1));
					   
	off->FillTriangle(BPoint(p1_x - C_S -1, -1),
					  BPoint(p1_x + 1, -1),
					   BPoint(p1_x + 1,  C_S + 1));
					   
	off->SetHighColor(125, 25,25); 
	
	off->MovePenTo(BPoint(C_S,0));
	off->StrokeLine(BPoint(0,C_S));
					   
	off->MovePenTo(BPoint(C_S,p1_y));
	off->StrokeLine(BPoint(0, p1_y - C_S));
					   
	off->MovePenTo(BPoint(p1_x - C_S, p1_y));
	off->StrokeLine(BPoint(p1_x, p1_y - C_S));
					   
	off->MovePenTo(BPoint(p1_x - C_S, 0));
	off->StrokeLine(BPoint(p1_x,  C_S));
	
	off->Sync();
	off->Window()->Unlock();
}

//--------------------------------------------------------------

void	TSoundView::Draw2(BView *off)
{
	off->Window()->Lock();
	off->SetHighColor(0,0,0);
	off->FillRect(BRect(0,0,1000,1000));
	off->SetHighColor(80,0,0);
	off->FillRect(BRect(0,0, p2_x, 25));
	
	off->SetDrawingMode(B_OP_OVER);
		off->SetHighColor(125, 25,25); 
	off->StrokeRect(BRect(0,0,p2_x - 1, p2_y - 1));
	off->SetHighColor(B_TRANSPARENT_32_BIT);
	off->SetHighColor(8,8,8);

	off->FillTriangle(BPoint(C_S + 1, -1),
					   BPoint(-1, -1),
					   BPoint(-1,C_S + 1));
					   
	off->FillTriangle(BPoint(C_S + 1, p2_y + 1),
					   BPoint(-1, p2_y + 1),
					   BPoint(-1, p2_y - C_S -1));
					   
	off->FillTriangle(BPoint(p2_x - C_S -1, p2_y + 1),
					  BPoint(p2_x + 1, p2_y + 1),
					   BPoint(p2_x + 1, p2_y - C_S -1));
					   
	off->FillTriangle(BPoint(p2_x - C_S -1, -1),
					  BPoint(p2_x + 1, -1),
					   BPoint(p2_x + 1,  C_S + 1));
					   
	off->SetHighColor(125, 25,25); 
	
	off->MovePenTo(BPoint(C_S,0));
	off->StrokeLine(BPoint(0,C_S));
					   
	off->MovePenTo(BPoint(C_S,p2_y));
	off->StrokeLine(BPoint(0, p2_y - C_S));
					   
	off->MovePenTo(BPoint(p2_x - C_S, p2_y));
	off->StrokeLine(BPoint(p2_x, p2_y - C_S));
					   
	off->MovePenTo(BPoint(p2_x - C_S, 0));
	off->StrokeLine(BPoint(p2_x,  C_S));
	
	off->Sync();
	off->Window()->Unlock();
}

//--------------------------------------------------------------

TSoundView::TSoundView (BRect r)
  : BView (r, "",
		   B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	long		x,y;
	float		fx;
	long		cpt;
	long		i;
	rgb_color	col;
	long		intens;
	rgb_color	c0;
	rgb_color	c1;
	long		tmp;
	BScreen	screen(B_MAIN_SCREEN_ID); 

	dst = 300.0;	
		
	quit = -1;
	for (i = 0; i < 32; i++) {
		lrev[i] = 0.5;  
		rrev[i] = 0.5;
	}
	
	demo_mode = 0;
	
	wave_count = 0;	
	view_state = MIX_VIEW;
		
	b1_state[0] = 0;
	b1_state[1] = 0;
	b1_state[2] = 0;
	
	
	
	for (i = 0; i < 128; i++) {
		obj_state[i] = 0;
		obj_muted[i] = 0;
		obj_solo[i] = 1;
		channel_reverbe[i] = 0;
	}
	
	
	time_mode = FULL_DRAW;
	
	need_sort = 1;	
	need_vector = 1;
	no_name = 0;	
	vector_count = 0;	
	size_x = HS;
	size_y = VS-1;
	the_bits = new BBitmap(BRect(0,0,HS-1,VS), B_COLOR_8_BIT, TRUE);
	the_bits->AddChild(off_view = new BView(BRect(0,0,HS-1,VS), "", B_FOLLOW_ALL, B_WILL_DRAW));
	pannel1 = new BBitmap(BRect(0,0,255,255), B_COLOR_8_BIT, TRUE);
	pannel1->AddChild(pan1_view = new BView(BRect(0,0,255,255), "", B_FOLLOW_ALL, B_WILL_DRAW));
	pannel1_rect = BRect(660, 40,660 + p1_x, 40 + p1_y);
	
	pannel2 = new BBitmap(BRect(0,0,255,255), B_COLOR_8_BIT, TRUE);
	pannel2->AddChild(pan2_view = new BView(BRect(0,0,255,255), "", B_FOLLOW_ALL, B_WILL_DRAW));
	pannel2_rect = BRect(85, 30, 85 + p2_x, 30 + p2_y);
	
	Draw1(pan1_view);
	Draw2(pan2_view);

	SetViewColor(B_TRANSPARENT_32_BIT);
	the_base = (char *)the_bits->Bits();
	MEMSET(the_base, 31, HS*VS);
	

	for (x = 0; x < 256; x++) {
		c0 = screen.ColorForIndex(x);
		for (y = 0; y < 256; y++) {
 	   		c1 = screen.ColorForIndex(y);
 	   		col.red = cmix(c0.red, c1.red);
 	   		col.green = cmix(c0.green, c1.green);
 	   		col.blue = cmix(c0.blue, c1.blue);

 	   		if (y == 0) {
 	   			col.red = c0.red * 0.63;
 	   			col.green = c0.green * 0.63;
 	   			col.blue = c0.blue * 0.7;
 	   		}
 	   		
 	   		tmp = col.red + 15;
 	   		if (tmp>255) tmp = 255;
 	   		col.red = tmp;
 	   		
 	   		tmp = col.green + 15;
 	   		if (tmp>255) tmp = 255;
 	   		col.green = tmp;
 	   		
 	   		tmp = col.blue + 33;
 	   		if (tmp>255) tmp = 255;
 	   		col.blue = tmp;
 	   		
  	   		if (y == 1) {
 	   			col = c0;
  	   		}
 	   		
 	   		composit[(x<<8) + y] =  screen.IndexForColor(col);
		}
	}
	
   	for (intens = 0; intens < 4; intens++) {
   		if (intens == 1) {
   			c0.red = 255;
   			c0.blue = 30;
   			c0.green = 30;
   		}
  	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens][i] = screen.IndexForColor(col);
    	}
    }
	for (i = 0; i < 256; i++) {
		c1 = screen.ColorForIndex(i);
		col.red = c1.red*0.65 + 11;
		col.green = c1.green*0.65 + 11;
		col.blue = c1.blue*0.65 + 11;
		mapper[0][i] = screen.IndexForColor(col);
   }
    for (intens = 0; intens < 4; intens++) {
   		c0.red = 55 + (intens * 35);
   		c0.green = 0;
   		c0.blue = 0;
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+4][i] = screen.IndexForColor(col);
    	}
    }
    	
    for (intens = 0; intens < 4; intens++) {
   		c0.red = 0;
   		c0.green = 55 + (intens * 35);
   		c0.blue = 0;
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+8][i] = screen.IndexForColor(col);
    	}
    }

   for (intens = 0; intens < 4; intens++) {
   		c0.red = 19 + (intens * 35);
   		c0.green = 20 + (intens * 35);
   		c0.blue = 45 + (intens * 35);
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+12][i] = screen.IndexForColor(col);
    	}
    }



    for (i = 0; i < 32; i++) {
        col.red = 0;
        col.green = 0;
        //col.blue = i*5.5;
        col.blue = i * 7.0;
        blue_map[i] = screen.IndexForColor(col);
    }
    
    for (i = 0; i < 32; i++) {
        col.red = i;
        col.green = i*2.6;
        col.blue = i;
        green_map[i] = screen.IndexForColor(col);
    }
    
   
    for (intens=0; intens < 20; intens++) {
 	  for (i = 0; i < 32; i++) {
			col.red = 0;
			col.green = 0;
			col.blue = i*((intens+8)/3.3);	//was 4.5
			level_map[intens][i] = screen.IndexForColor(col);
  	  }
    }
	
 	 for (i = 0; i < 32; i++) {
		col.red = i*((8)/3.5);
		col.green = i*((8)/3.5);
		col.blue = i*((8)/3.5);
		white_map[i] = screen.IndexForColor(col);
  	  }
  	  
	a = 0;
	b = 0.32;
	off_y = BVP + - b*125.0;
	c = 0.1;


	vpx = 0;
	vpy = 100;
	vpz = -430;

	cpt = 0;
	
	for (fx = 0; fx <= 512.1; fx+= 21.333333333333) {
		add_vector( fx-256,0,-256,	
					fx-256,0,256);	
		
		add_vector(-256,0,fx-256,	
				   256,0,fx-256);
	}
	
			   
	add_vector(-140,0,290,
			   140,0,290);
	
	add_vector(-140,0,290,
			   -130,0,295);
	
	
	add_vector(-140,0,290,
			   -130,0,285);
	
	
	add_vector(130,0,285,
			   140,0,290);
	
	add_vector(130,0,295,
			   140,0,290);
	
	
	add_vector(290,0,-140,
			   290,0,140);
	
	add_vector(290,0,-140,
			   295,0,-130);
	
	add_vector(290,0,-140,
			   285,0,-130);
	
	add_vector(285,0,130,
			   290,0,140);
	
	add_vector(295,0,130,
			   290,0,140);
	
	
	ppoint_count = 0;
	
	poly_count = 0;
	
	
	long	n1,n2,n3,n4;
	long	m1,m2,m3,m4;
	long	vs;
	long	hs;
	
	
	
	for (i = 0; i < NC; i++) {
		vs = 75;
		hs = 8;
		n1 = add_ppoint(-hs,-hs,3,i, 'bomm');
		n2 = add_ppoint( hs,-hs,3,i, 'bopm');
		n3 = add_ppoint( hs, hs,3,i, 'bopp');
		n4 = add_ppoint( -hs, hs,3,i, 'bomp');
		
		m1 = add_ppoint(-hs,-hs,vs,i, 'itop');
		m2 = add_ppoint( hs,-hs,vs,i, 'itop');
		m3 = add_ppoint( hs, hs,vs,i, 'itop');
		m4 = add_ppoint(-hs,hs,vs,i, 'itop');
		
		add_ppoint(0, 0, vs*1.2, i+1024, 'itop');			//tag point for label
		add_ppoint(0, 0, vs*1.35, i, 'itop');				//tag point for label
		
		add_poly(m1,m2,m3,m4,6, i);
		add_poly(n3,n4,m4,m3,6, i);	
		add_poly(n2,n3,m3,m2,6, i);	
		add_poly(n1,n4,m4,m1,6, i);
		add_poly(n1,n2,n3,n4,4, i);							//bottom poly
		
		hs = 5;
		n1 = add_ppoint(-hs, 0, 4, i);
		n2 = add_ppoint(+hs, 0, 4, i);
		n3 = add_ppoint(-hs, 0, 30, i, 'ttop');
		n4 = add_ppoint(hs, 0, 30, i, 'ttop');
		add_poly(n1,n2,n4,n3,8, i);
	
		//MovePoly(i, rand() % 300 - 150, 0, rand() % 300 - 150);

	}	
	
	
	
	MovePoly(0,25.051323 , 0, 175.445557);
	MovePoly(1,-112.220070 , 0, 87.663002);
	MovePoly(2,44.490257, 0,  189.489120);
	MovePoly(3,140.069260, 0,  33.365788);
	MovePoly(4,249.983871 , 0, 187.952621);
	MovePoly(5,-249.991959, 0,  194.367691);
	MovePoly(6,-46.501175 , 0, 175.672302);
	MovePoly(7,173.282867, 0,  98.012032);
	MovePoly(8,-54.694016 , 0, 81.717079);
	MovePoly(9,5.921313 , 0, 189.567474);
	MovePoly(10,119.236382 , 0, 116.379951);
	MovePoly(11,24.859936 , 0, 189.898590);
	MovePoly(12,-243.929535, 0,  50.186169);
	MovePoly(13,249.003418, 0, 46.529522);
	MovePoly(14,28.421734, 0, 60.700035);
	MovePoly(15,23.795088, 0, 245.001205 );
	
	ready_sem = create_sem(0, "ready");
	resume_thread(spawn_thread(init_p,"init_p",B_REAL_TIME_PRIORITY,this));
	acquire_sem(ready_sem);
	resume_thread(spawn_thread(init_anim,"init_anim",B_NORMAL_PRIORITY,this));

	change_view_point(a,-b,c);

	MEMSET(the_base, 0, VS*HS);
	draw_vectors();
}

//--------------------------------------------------------------

void	TSoundView::BotSize(long id, float delta)
{
	long	i;
	
	delta = -delta;
	for (i=  0; i < ppoint_count; i++) {
		if (poly_point[i].object_id == id) {
			if (poly_point[i].part == 'bomm') {
				poly_point[i].x1 -= delta;
				poly_point[i].z1 -= delta;
			}
			if (poly_point[i].part == 'bopm') {
				poly_point[i].x1 += delta;
				poly_point[i].z1 -= delta;
			}
			if (poly_point[i].part == 'bopp') {
				poly_point[i].x1 += delta;
				poly_point[i].z1 += delta;
			}
			if (poly_point[i].part == 'bomp') {
				poly_point[i].x1 -= delta;
				poly_point[i].z1 += delta;
			}
		}
	}
}

//--------------------------------------------------------------

void	TSoundView::MovePoly(long id, float dx, float dy, float dz)
{
	long	i;
	
	for (i=  0; i < ppoint_count; i++) {
		if (poly_point[i].object_id == id) {
			poly_point[i].x1 += dx;
			poly_point[i].y1 += dy;
			poly_point[i].z1 += dz;
			poly_point[i].touched = 1;
		}
		if (poly_point[i].object_id == (id + 1024)) {		//handle label attach points.
			poly_point[i].x1 += dx;
			poly_point[i].y1 += dy;
			poly_point[i].z1 += dz;
			poly_point[i].touched = 1;
		}
	}
	need_sort = 1;
}

//--------------------------------------------------------------

void	TSoundView::ScalePoly(long id, ulong atype, float scale)
{
	long	i;
	
	for (i=  0; i < ppoint_count; i++) {
		if (poly_point[i].object_id == id && poly_point[i].part == atype) {
			poly_point[i].y1 *= scale;
			poly_point[i].touched = 1;
		}
	}
	need_sort = 1;
}

//--------------------------------------------------------------

void	TSoundView::SizePoly(long id, ulong atype, long vs)
{
	long	i;
	
	for (i=  0; i < ppoint_count; i++) {
		if (poly_point[i].object_id == id && poly_point[i].part == atype) {
			poly_point[i].y1 = vs;
			poly_point[i].touched = 1;
		}
	}
	need_sort = 1;
}

//--------------------------------------------------------------

TSoundView::~TSoundView() 
{
	long	max;
	long	i;
		
	for (i = 0; i < NC; i++) {
		printf("pos of %ld is %f %f\n", i, 	pos_x[i], pos_y[i]);
	}


	max = 20;
	
	quit = 3;
	while(quit > 0 && max) {
		snooze(50000);
		max--;
	}
	be_app->PostMessage(B_QUIT_REQUESTED);

}

#define left_code	0x01
#define	right_code	0x02
#define	bottom_code	0x04
#define	top_code	0x08


//--------------------------------------------------------------

void	TSoundView::fline(long xx1, long yy1, long xx2, long yy2, char *mp)
{
	long	tmp;
	float	dy;
	float	dx;
	
	if (xx1 > xx2) {
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
	}
	
	dy = yy2 - yy1;
	dx = xx2 - xx1;
	
	
	if (xx1 < 0) {
		yy1 = yy1 + (dy * ((-xx1)/dx));
		xx1 = 0;
	}

	if (xx1 > (size_x-1))
		return;
		
	if (xx2 > (size_x-1)) {
		yy2 = yy2 - (dy * (xx2-(size_x-1))/dx);
		xx2 = size_x-1;
	}

	if (yy1 > yy2) {
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
	}
	
	dy = yy2 - yy1;
	dx = xx2 - xx1;
	
	
	if (yy1 < 0) {
		xx1 = xx1 + (dx * ((-yy1)/dy));
		yy1 = 0;
	}

	if (yy1 > (size_y-5))
		return;
		
	if (yy2 > (size_y-5)) {
		xx2 = xx2 - (dx * (yy2-(size_y-5))/dy);
		yy2 = (size_y-5);
	}

	

	anti_fline(xx1+1,yy1+1,xx2+1, yy2+1, mp);
}

//--------------------------------------------------------------

void	TSoundView::anti_fline(long x1,long y1,long x2, long y2, char *mp)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = HS;
	long	error;
	long	cpt;
	char 	*base;
	float	k;
	
	
	if (y1 < 1 || y1 > (VS-2) || y2 < 1 || y2 > (VS-2))
		return;
	if (x1 < 0 || x1 > HS || x2 < 0 || x2 > HS)
		return;
	
	
	dx = x2-x1;
	dy = y2-y1;
	
	base = the_base + y1*rowbyte+x1;
	
	if (dx==0 && dy==0) {
		*base = c;
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
			dy *= k;
			dx *= k;
			
			while(cpt>=0) {
				cpt--; 
				*base = mp[31 - (error>>16)];
				*(base+rowbyte) = mp[(error>>16)];
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
			dy *= k;
			dx *= k;
			while(cpt >= 0) {
				cpt--;
				*base =  mp[31 - (error>>16)];
				*(base+1) =  mp[(error>>16)];
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
			dy *= k;
			dx *= k;
			
			while(cpt>=0) {
				cpt--; 
				*base = mp[31 - (error>>16)];
				*(base+rowbyte) = mp[(error>>16)];
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
			dy *= k;
			dx *= k;
			while(cpt >= 0) {
				cpt--;
				*base =  mp[31 - (error>>16)];
				*(base-1) =  mp[(error>>16)];
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

//--------------------------------------------------------------

void	TSoundView::Draw(BRect rr)
{
	BRect	bnd;
	long	w;
	
	bnd = Window()->Bounds();
	w = bnd.right - bnd.left;
	bm_hp = -(HS-w)/2.0;
	DrawBitmap(the_bits, BPoint(bm_hp,0));
}


//--------------------------------------------------------------


long	TSoundView::find_hit(BPoint where)
{
	long	i;
	point	i1,i2,i3,i4;
	long	m;
	
	sort_polys();
	
	where.x -= bm_hp;
	
	for (i = 0; i < poly_count; i++) {
		i1 = poly_point[polys[i].pt1_index];
		i2 = poly_point[polys[i].pt2_index];
		i3 = poly_point[polys[i].pt3_index];
		i4 = poly_point[polys[i].pt4_index];
		m = polys[i].color_index;
		if (test_hit(where,
					 BPoint(i1.sx1+(HS/2),i1.sy1+off_y),
			   		 BPoint(i2.sx1+(HS/2),i2.sy1+off_y),
			   		 BPoint(i3.sx1+(HS/2),i3.sy1+off_y),
			         BPoint(i4.sx1+(HS/2),i4.sy1+off_y))) {
		
			return i1.object_id;
		}
	}
	return -1;
}

//--------------------------------------------------------------


void	TSoundView::render_polys()
{
	long	i;
	point	i1,i2,i3,i4;
	long	m;
	
	sort_polys();
	
	for (i = 0; i < poly_count; i++) {
		i1 = poly_point[polys[i].pt1_index];
		i2 = poly_point[polys[i].pt2_index];
		i3 = poly_point[polys[i].pt3_index];
		i4 = poly_point[polys[i].pt4_index];
		m = polys[i].color_index;
		
		fill_4(BPoint(i1.sx1+(HS/2),i1.sy1+off_y),
			   BPoint(i2.sx1+(HS/2),i2.sy1+off_y),
			   BPoint(i3.sx1+(HS/2),i3.sy1+off_y),
			   BPoint(i4.sx1+(HS/2),i4.sy1+off_y), mapper[m]);
	}
}
	

//--------------------------------------------------------------


void	TSoundView::find_space(long h, long *hp, long *vp)
{
	long	v;
	long	h0,v0;
	long	cpt;
	
	v = 0;
	
	h /= 64;
	
	h0 = h;
	v0 = v;
	
	cpt = 0;
	
	while(1) {
		if (space[h][v] == 0) {
			space[h][v] = 1;
			*hp = h * 64;
			*vp = v * 32;
			return;
		}
		h = h0 + (cpt % 4);
		v = v0 + (cpt / 4);
		cpt++;
	}
}

//--------------------------------------------------------------

void	TSoundView::recalc_pos_xy()
{
	long		i;
	long		j;
	
	
	for (i = 0; i < ppoint_count; i++) {
		if (poly_point[i].object_id > 1023) {
			pos_x[poly_point[i].object_id -1024] = poly_point[i].x1;
			pos_y[poly_point[i].object_id -1024] = poly_point[i].z1;
		}
	}
}

//--------------------------------------------------------------

void	TSoundView::draw_labels()
{
	long		i;
	long		j;
	rgb_color	c;
	long		x0,y0;
	char		buf[32];
	long		xx,yy;
	long		prev_x[64];
	long		prev_y[64];
	long		prev_w[64];
	long		cpt = 0;
	long		k;
	char		ok;
	long		dist_x;
	long		dist_y;
	long		dist_w;
	long		w;
	BRegion		tmp_region;
	

	for (i = 0; i < 16; i++)
	for (j = 0; j < 8; j++)
		space[i][j] = 0;
		
	c.red = 255;
	c.green = 255;
	c.blue = 255;
	
	off_view->Window()->Lock();
	tmp_region.Set(clip_rect);
	if (no_name) {
		off_view->ConstrainClippingRegion(&tmp_region);
	}
//	no_name = 0;
	
	off_view->SetHighColor(c);
	off_view->SetDrawingMode(B_OP_BLEND);
	
	for (i = 0; i < ppoint_count; i++) {
		if (poly_point[i].object_id > 1023) {
			pos_x[poly_point[i].object_id -1024] = poly_point[i].x1;
			pos_y[poly_point[i].object_id -1024] = poly_point[i].z1;
			
			x0 = poly_point[i].sx1 + (HS/2);
			y0 = poly_point[i].sy1 + off_y;
			
			xx = poly_point[i+1].sx1 + (HS/2);
			yy = poly_point[i+1].sy1 + off_y;
			
			ok = 0;
			
			w = StringWidth(chan[poly_point[i].object_id -1024]->cname);
			while(!ok) {
				again:;
				ok = 1;
				for (k = 0; k < cpt; k++) {
					dist_x = abs(prev_x[k] - xx);
					dist_y = abs(prev_y[k] - yy);
					dist_w = dist_x - abs(w/2 + prev_w[k]);
					if ((dist_w < 15) && (dist_y < 15)) {
						yy = yy - 10;
						ok = 0;
						goto again;
					}
				}
			}		
			
			prev_x[cpt] = xx;
			prev_y[cpt] = yy;
			prev_w[cpt] = w/2;
			
			fline(x0,y0,xx,yy, white_map);
			cpt++;
		}
	}
	
	cpt = 0;
	for (i = 0; i < ppoint_count; i++) {
		if (poly_point[i].object_id > 1023) {
			x0 = poly_point[i].sx1 + (HS/2);
			y0 = poly_point[i].sy1 + off_y;
			
			
			xx = prev_x[cpt];
			yy = prev_y[cpt];
			w = prev_w[cpt];
			off_view->MovePenTo(BPoint(xx - w, yy-10));
			off_view->DrawString(chan[poly_point[i].object_id -1024]->cname);
			cpt++;
		}
	}

	
	if (no_name) {
		tmp_region.Set(BRect(0,0,5000,5000));
		off_view->ConstrainClippingRegion(&tmp_region);
	}
	no_name = 0;
	off_view->Sync();
	off_view->Window()->Unlock();
}

//--------------------------------------------------------------

void	TSoundView::draw_vectors()
{
	long	i;
	float	x1,y1,x2,y2;
	float	dx,dy;
	long	p;
	
	if (view_state == WAVE_VIEW)
		return;
		
	for (i = 0; i < vector_count; i++) {
		x1 = varray[i].sx1+(HS/2);
		y1 = varray[i].sy1+off_y;
		x2 = varray[i].sx2+(HS/2);
		y2 = varray[i].sy2+off_y;
	
		dx = fabs(x1 - x2);
		dy = fabs(y1 - y2);	
		
		if (varray[i].color == 100)
			fline(x1,y1,x2,y2, green_map);
		else
		if (varray[i].z1 != varray[i].z2) {
			fline(x1,y1,x2,y2, blue_map);
		}
		else {
			p = varray[i].z1 + 256;
			p = p / 15;
			p+=4;
			if (p > 19) p = 19;
			fline(
				x1,y1,x2,y2, level_map[p]
				);
		}
	}
	
	render_polys();
	draw_labels();
}

//--------------------------------------------------------------

void	TSoundView::move_obj(float k)
{
	MovePoly(0, 2*sin(k), 0, 2*cos(k));
	MovePoly(0, 2*sin(k*1.5), 0, 2*cos(k));
	MovePoly(1, 3*sin(k*0.95), 0, 2*cos(k));
}

//--------------------------------------------------------------

void	TSoundView::vertical_move(long hit, BPoint where)
{
	BPoint	where0;
	float	dx,dy;
	long	i;
	ulong	buttons;
	long	modif;
	char	again = 0;
	float	px0,py0,px1,py1;
	long	obj;
	long	m;
	float	old;
	float	delta;
	
	modif = modifiers();
	 
	 
	if ((modif & B_SHIFT_KEY) != 0) {
		obj_state[hit] ^= 1;
	}
	else {
		if (obj_state[hit] == 0) {
			for (i = 0; i < 64; i++) {
				if (obj_state[i]) {
					obj_state[i] = 0;
					SetChannelColor(i, 6);
				}
			}
			obj_state[hit] ^= 1;
		}
	}
	if (obj_state[hit])
		SetChannelColor(hit, 2);
	else
		SetChannelColor(hit, 6);
	
	
	do {
		GetMouse(&where0, &buttons);
		dx = where0.x - where.x;
		dy = where0.y - where.y;
		
		if (modif & B_CONTROL_KEY)
			dx = 0;
		
		
		if (dx == 0.0 && dy == 0.0)
			snooze(20000); 
		
		where = where0;
			
		dy /= 4.0;
		
		for (obj = 0; obj < 64; obj++) {
			if (obj_state[obj]) {
				px0 = pos_x[hit];
				py0 = pos_y[hit];
				
				px1 = px0;
				py1 = py0;
				old = channel_reverbe[obj];
				channel_reverbe[obj] += dy;
				
				if (channel_reverbe[obj] > 0) {
					channel_reverbe[obj] = 0;
				}
				if (channel_reverbe[obj] < -30) {
					channel_reverbe[obj] = -30;
				}
				delta = channel_reverbe[obj] - old;
				BotSize(obj, delta);
			}
		}
		update_infos();
		change_view_point(a,-b,c);
	
		MEMSET(the_base, 0, VS*(HS));
		draw_vectors();
		update_waves();
		composit_pannel1();
		composit_pannel2();

		DrawBitmap(the_bits, BPoint(bm_hp,0));
		snooze(20000);
	} while(buttons);
}

//--------------------------------------------------------------

float	TSoundView::get_reverbe_level(long c)
{
	return -(channel_reverbe[c])/30;
}

//--------------------------------------------------------------

void	TSoundView::transform(float *x, float *y)
{
	float	x0, y0;

	float	sina = sin(-c);
	float	cosa = cos(-c);

	x0 = (*x) * cosa - (*y) * sina;
	y0 = (*x) * sina + (*y) * cosa;
	
	*x = x0;
	*y = y0;
}

//--------------------------------------------------------------

void	TSoundView::handle_hit(long hit, BPoint where)
{
	BPoint	where0;
	float	dx,dy;
	long	i;
	ulong	buttons;
	long	modif;
	char	again = 0;
	float	px0,py0,px1,py1;
	long	obj;
	long	m;
	
	modif = modifiers();
	 
	 
	if ((modif & B_COMMAND_KEY) != 0) {
		vertical_move(hit, where);
		return;
	}
	 
	if ((modif & B_SHIFT_KEY) != 0) {
		obj_state[hit] ^= 1;
	}
	else {
		if (obj_state[hit] == 0) {
			for (i = 0; i < 64; i++) {
				if (obj_state[i]) {
					obj_state[i] = 0;
					SetChannelColor(i, 6);
				}
			}
			obj_state[hit] ^= 1;
		}
	}
	if (obj_state[hit])
		SetChannelColor(hit, 2);
	else
		SetChannelColor(hit, 6);
	
	
	do {
		GetMouse(&where0, &buttons);
		dx = where0.x - where.x;
		dy = where0.y - where.y;
		
		transform(&dx, &dy);
		
		if (modif & B_CONTROL_KEY)
			dx = 0;
		
		
		if (dx == 0.0 && dy == 0.0)
			snooze(20000); 
		
		where = where0;
			
		for (obj = 0; obj < 64; obj++) {
			if (obj_state[obj]) {
				px0 = pos_x[hit];
				py0 = pos_y[hit];
				
				px1 = px0;
				py1 = py0;
			
				MovePoly(obj, dx, 0, dy);
			}
		}
		
		recalc_pos_xy();
		
		
		for (obj = 0; obj < 64; obj++) {
			if (obj_state[obj]) {
				if (pos_x[obj] > 250) {
					for(m = 0; m < 64; m++)
						if (obj_state[m])
							MovePoly(m, -1 + 0.9*(250-pos_x[obj]), 0, 0);
				}
				if (pos_x[obj] < -250) {
					for(m = 0; m < 64; m++)
						if (obj_state[m])
							MovePoly(m, 1 +  0.9*((-(pos_x[obj]+250))), 0, 0);
				}
			
				if (pos_y[obj] > 250) {
					for(m = 0; m < 64; m++)
						if (obj_state[m])
							MovePoly(m, 0,0, -1 + 0.9*(250-pos_y[obj]));
				}
				if (pos_y[obj] < -250) {
					for(m = 0; m < 64; m++)
						if (obj_state[m])
							MovePoly(m, 0, 0, 1 + 0.9*(-(pos_y[obj]+250)));
				}
			}
		}
		
		px0 = pos_x[hit];
		py0 = pos_y[hit];
	
	
		if (px0 != px1 || py0 != py1) {
			//add_vector(px0,0,py0,px1,0,py1, 100);
		}
		update_infos();	
		change_view_point(a,-b,c);
	
		MEMSET(the_base, 0, HS*VS);
		draw_vectors();
		update_waves();
		composit_pannel1();
		composit_pannel2();
		
		DrawBitmap(the_bits, BPoint(bm_hp,0));
		snooze(20000);
	} while(buttons);
}	

//--------------------------------------------------------------

void	TSoundView::move_selection(float dx, float dy)
{
	long	i;
	float	px0,py0,px1,py1;
	long	obj;
	long	m;
	
	
		
	for (obj = 0; obj < 64; obj++) {
		if (obj_state[obj]) {
			MovePoly(obj, dx, 0, dy);
		}
	}
	
	recalc_pos_xy();
	
	
	for (obj = 0; obj < 64; obj++) {
		if (obj_state[obj]) {
			if (pos_x[obj] > 250) {
				for(m = 0; m < 64; m++)
					if (obj_state[m])
						MovePoly(m, -1 + 0.9*(250-pos_x[obj]), 0, 0);
			}
			if (pos_x[obj] < -250) {
				for(m = 0; m < 64; m++)
					if (obj_state[m])
						MovePoly(m, 1 +  0.9*((-(pos_x[obj]+250))), 0, 0);
			}
		
			if (pos_y[obj] > 250) {
				for(m = 0; m < 64; m++)
					if (obj_state[m])
						MovePoly(m, 0,0, -1 + 0.9*(250-pos_y[obj]));
			}
			if (pos_y[obj] < -250) {
				for(m = 0; m < 64; m++)
					if (obj_state[m])
						MovePoly(m, 0, 0, 1 + 0.9*(-(pos_y[obj]+250)));
			}
		}
	}
	


		
	change_view_point(a,-b,c);

	MEMSET(the_base, 0, HS*VS);
	draw_vectors();
	update_infos();
	update_waves();
	composit_pannel1();
	composit_pannel2();
	
	DrawBitmap(the_bits, BPoint(bm_hp,0));
}	

//--------------------------------------------------------------

void	TSoundView::SetChannelLevel(long c, float v)
{
	if (obj_muted[c] || (obj_solo[c] == 0)) {
		SizePoly(c, 'ttop',-7/* - channel_reverbe[c]*/);
	}
	else
		SizePoly(c, 'ttop', 5 + v*40.0/* + channel_reverbe[c]*/);
}

//--------------------------------------------------------------

void	TSoundView::SetChannelColor(long ch, long color)
{
	long	i;
	
	for (i=  0; i < poly_count; i++) {
		if (polys[i].object_id == ch) {
			if ((polys[i].color_index != 8) && (polys[i].color_index < 12))
				polys[i].color_index = color;
		} 
	}
}

//--------------------------------------------------------------

void	TSoundView::calc_poly_range(long *x1, long *y1, long *x2, long *y2)
{
	long	i;
	float	x,y;
	
	*x1 = 4096;
	*x2 = -4096;
	*y1 = VS;
	*y2 = 0;
	
	for (i = 0; i < ppoint_count; i++) {
		x = poly_point[i].sx1+(HS/2);
		y = poly_point[i].sy1+off_y;
		if (x < *x1)
			*x1 = x;
		if (x > *x2)
			*x2 = x;
		if (y < *y1)
			*y1 = y;
		if (y > *y2)
			*y2 = y;
	}
	*x1 -= 2;
	if (*x1 < 0) *x1 = 0;
	*y1 -= 2;
	if (*y1 < 0) *y1 = 0;
	
	*x2 += 2;
	if (*x2 > (HS-1)) *x1 = (HS-1);
	*y2 += 2;
	if (*y2 > 511) *y2 = 511;
}

//--------------------------------------------------------------

void	TSoundView::Animate()
{
	long	x0,y0,x1,y1;
	BRect	r;
	
	
	if (Window()->Lock()) {
		if (demo_mode) {
			//a += 0.01;
			b = fabs(sin(mtime/20.0));
			c += 0.0064;
			need_vector = 1;
		}
		change_view_point(a,-b,c);
		
		
		MEMSET(the_base, 0, HS*VS);
		draw_vectors();
		UpdateTime(pan2_view);
		update_waves();
		composit_pannel1();
		composit_pannel2();
		
		double	s,e;
	
		s = system_time();
		DrawBitmap(the_bits, BPoint(bm_hp,0));
		e = system_time();
		//printf("dt=%f\n", (e-s)/1000.0);
		MakeFocus();
	
		Window()->Unlock();
	}
}

//--------------------------------------------------------------

void	TSoundView::track_pannel2()
{
	BPoint	where;
	BPoint	where0;
	ulong	buttons;
	long	dx,dy;
	
	GetMouse(&where, &buttons);
	where0 = where;
	
	where0.y -= pannel2_rect.top;
	where0.x -= pannel2_rect.left;
	
	where0.x -= bm_hp;

	
	if (pannel2_click(where0))
		return;
		
		
	do {
		GetMouse(&where0, &buttons);

		dx = where0.x - where.x;
		dy = where0.y - where.y;
		
		
		if (dx == 0 && dy == 0) {
			snooze(35000);
		}
		
		where = where0;
		pannel2_rect.top += dy;
		pannel2_rect.bottom += dy;
		pannel2_rect.left += dx;
		pannel2_rect.right += dx;
		
		Animate();
		snooze(25000);
	} while(buttons);
}

//--------------------------------------------------------------

void	TSoundView::track_pannel1()
{
	BPoint	where;
	BPoint	where0;
	ulong	buttons;
	long	dx,dy;
	
	GetMouse(&where, &buttons);
	
	
	do {
		GetMouse(&where0, &buttons);

		dx = where0.x - where.x;
		dy = where0.y - where.y;
		
		
		if (dx == 0 && dy == 0) {
			snooze(35000);
		}
		
		where = where0;
		pannel1_rect.top += dy;
		pannel1_rect.bottom += dy;
		pannel1_rect.left += dx;
		pannel1_rect.right += dx;
		
		Animate();
		snooze(25000);
	} while(buttons);
}

//--------------------------------------------------------------

void	TSoundView::MouseDown(BPoint where)
{
	ulong	buttons;
	BPoint	where0;
	float	k = 0;
	long	hit;
	char	moved;
	long	phase;
	
	where0 = where;
	where0.x -= bm_hp;
	if (pannel1_rect.Contains(where0) && has_selection()) {
		track_pannel1();
		return;
	}
	
	if (pannel2_rect.Contains(where0)) {
		track_pannel2();
		return;
	}
	
	
	if (view_state == WAVE_VIEW) {
		handle_wave_click(where0);
		return;
	}
	
	hit = find_hit(where);
	
	phase = 0;
	
	if (hit >= 0) {
		Draw1(pan1_view, hit);

		handle_hit(hit, where);
	}
	else {
		deselect_all();
		do {
			GetMouse(&where0, &buttons);
			
			if ((where0.x != where.x) || (where0.y != where.y))
				moved = 1;
				
			if (moved) {
				phase++;
				c = c - (where0.x - where.x)/500.0;
				b = b + (where0.y - where.y)/1000.0;
			
				if (b > 1.3) b = 1.3;
				if (b < 0.03) b = 0.03;
				off_y = BVP + - b*125.0;
				where = where0;
				need_sort = 1;
			}
			need_vector = 1;
			change_view_point(a,-b,c);
			/*
			off_view->Window()->Lock();
			SetDrawingMode(B_OP_BLEND);
			off_view->SetHighColor(0,0,0);
			off_view->FillRect(BRect(0,0,5000,5000));
			SetDrawingMode(B_OP_COPY);
			off_view->Sync();
			off_view->Window()->Unlock();
			*/
			MEMSET(the_base, 0, HS*VS);
			draw_vectors();
			update_waves();
			composit_pannel1();
			composit_pannel2();
			
			DrawBitmap(the_bits, BPoint(bm_hp,0));
			
			if (!moved)
				snooze(30000);
			else
				snooze(25000);
			moved = 0;
		} while(buttons);
	}
}

//--------------------------------------------------------------

void	TSoundView::change_view_point(float new_alpha, float new_delta, float new_zeta)
{
	float	sina;
	float	cosa; 
	float	sinb;
	float	cosb;
	float	sinc;
	float	cosc;

	long	i;
	float	x, y, z;
	float	x0, y0, z0;
	float	hs, vs, ms;
	float	dist;
	
	float	dx,dy,dz;
	
	
	if (view_state == WAVE_VIEW)
		return;
		
	if (need_vector == 0) {
		change_view_point_poly(new_alpha, new_delta, new_zeta);
		return;
	}
	
	need_vector = 0;
	cur_alpha = new_alpha;
	cur_delta = new_delta;
	cur_zeta  = new_zeta;


	sina = sin(new_alpha);
	cosa = cos(new_alpha);

	sinb = sin(new_delta);
	cosb = cos(new_delta);

	sinc = sin(new_zeta);
	cosc = cos(new_zeta);

	for (i = 0; i < vector_count; i++) {
		x = varray[i].x1;
		y = varray[i].y1;
		z = varray[i].z1;
		x0 = x * cosa - y * sina; y0 = x * sina + y * cosa;
		x = x0;y = y0;
		x0 = x * cosc - z * sinc;z0 = x * sinc + z * cosc;
		x = x0;z = z0;
		y0 = y * cosb - z * sinb;z0 = y * sinb + z * cosb;
		y = y0;y = y0;z = z0;
		dx = (x0 + vpx);
		dy = (y0 + vpy);
		dz = (z0 + vpz);
		dist = b_sqrt(dz*dz);
		dist = dst/dist;
		varray[i].sx1 = x*dist;
		varray[i].sy1 = y*dist;
		
		x = varray[i].x2;
		y = varray[i].y2;
		z = varray[i].z2;
		x0 = x * cosa - y * sina; y0 = x * sina + y * cosa;
		x = x0;y = y0;
		x0 = x * cosc - z * sinc;z0 = x * sinc + z * cosc;
		x = x0;z = z0;
		y0 = y * cosb - z * sinb;z0 = y * sinb + z * cosb;
		y = y0;y = y0;z = z0;
		dx = (x0 + vpx);
		dy = (y0 + vpy);
		dz = (z0 + vpz);
		dist = b_sqrt(dz*dz);
		dist = dst/dist;
		varray[i].sx2 = x*dist;
		varray[i].sy2 = y*dist;

	}
	change_view_point_poly(new_alpha, new_delta, new_zeta);

}

//--------------------------------------------------------------

void	TSoundView::change_view_point_poly(float new_alpha, float new_delta, float new_zeta)
{
	float	sina;
	float	cosa; 
	float	sinb;
	float	cosb;
	float	sinc;
	float	cosc;

	long	i;
	float	x, y, z;
	float	x0, y0, z0;
	float	hs, vs, ms;
	float	dist;
	
	float	dx,dy,dz;
	
	if (view_state == WAVE_VIEW)
		return;

	cur_alpha = new_alpha;
	cur_delta = new_delta;
	cur_zeta  = new_zeta;


	sina = sin(new_alpha);
	cosa = cos(new_alpha);

	sinb = sin(new_delta);
	cosb = cos(new_delta);

	sinc = sin(new_zeta);
	cosc = cos(new_zeta);

	for (i = 0; i < ppoint_count; i++) {
		if ((poly_point[i].touched == 1) || (need_sort)) {
			poly_point[i].touched = 0;
			x = poly_point[i].x1;
			y = poly_point[i].y1;
			z = poly_point[i].z1;
			x0 = x * cosa - y * sina; y0 = x * sina + y * cosa;
			x = x0;y = y0;
			x0 = x * cosc - z * sinc;z0 = x * sinc + z * cosc;
			x = x0;z = z0;
			y0 = y * cosb - z * sinb;z0 = y * sinb + z * cosb;
			y = y0;y = y0;z = z0;
			dx = (x0 + vpx);
			dy = (y0 + vpy);
			dz = (z0 + vpz);
			dist = b_sqrt(dz*dz);
			dist = dst/dist;
			poly_point[i].sx1 = x*dist;
			poly_point[i].sy1 = y*dist;
		}
	}
}

//--------------------------------------------------------------


void	hline(char *base, long count, char *mapper)
{

	while(count > 8) {
		*base = mapper[*base];
		base++;
		*base = mapper[*base];
		base++;
		*base = mapper[*base];
		base++;
		*base = mapper[*base];
		base++;
		*base = mapper[*base];
		base++;
		*base = mapper[*base];
		base++;
		*base = mapper[*base];
		base++;
		*base = mapper[*base];
		base++;
		count -=8;
	}
	
	while(count-- >= 0) {
		*base = mapper[*base];
		base++;
	}
}

//--------------------------------------------------------------


//--------------------------------------------------------------

long	poly_scratch[1200];

//--------------------------------------------------------------

void	TSoundView::fill_4(BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d, char *mapper)
{
	#define compare(a,b,c)  {if (a<b) b = a; if (a>c) c = a;}
 
	long* 		coord0;
	long* 		coord1;
	long		*tmp0;
	long		*tmp1;
	long		dx;
	long		dy;
	long		i;
	long		j;
	long		x;
	long		xe;
	long		xpoint[5];
	long		ypoint[5];
	long		xs;
	long		y;
	long		y_sign;
	long		decision;
	long		ye;
	long		ys;
   	long		scanlines;
	long		top;
	long		bottom;
	char		*ptr;
	rgb_color	c1;
	rgb_color	col;
    
    

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
		*tmp0++ = (HS-1);
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

		dx = abs(xe - xs);
		dy = abs(ye - ys);

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
	
	ptr = (char *)the_bits->Bits();

	ptr = ptr + (top * size_x);

	for (i = top; i <= bottom; i++) {
		if (i > size_y)
			return;
		if (*tmp0 < 0)
			*tmp0 = 0;
		if (*tmp1 > size_x)
			*tmp1 = size_x;
		if ((i >= 0) && (i < (size_y - 1))) 
			hline(ptr + *tmp0, *tmp1 - *tmp0, mapper);

		tmp0++;
		tmp1++;

		ptr += size_x;
	}
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

char	TSoundView::test_hit(BPoint hit, BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d)
{
	#define compare(a,b,c)  {if (a<b) b = a; if (a>c) c = a;}
 
	long* 		coord0;
	long* 		coord1;
	long		*tmp0;
	long		*tmp1;
	long		dx;
	long		dy;
	long		i;
	long		j;
	long		x;
	long		xe;
	long		xpoint[5];
	long		ypoint[5];
	long		xs;
	long		y;
	long		y_sign;
	long		decision;
	long		ye;
	long		ys;
   	long		scanlines;
	long		top;
	long		bottom;
	rgb_color	c1;
	rgb_color	col;
    
    

	if (hit.x > pt_a.x && hit.x > pt_b.x && hit.x > pt_c.x && hit.x > pt_d.x)
		return 0;
		
	if (hit.x < pt_a.x && hit.x < pt_b.x && hit.x < pt_c.x && hit.x < pt_d.x)
		return 0;

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
		*tmp0++ = (HS-1);
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

		dx = abs(xe - xs);
		dy = abs(ye - ys);

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
	
	for (i = top; i <= bottom; i++) {
		if (i == hit.y) {
			if (*tmp0 < hit.x && *tmp1 >= hit.x)
				return 1;
		}
		tmp0++;
		tmp1++;
	}
	
	return 0;
}

//------------------------------------------------------------------------------

float	TSoundView::calc_mid(long pn)
{
	float	i1,i2,i3,i4;
	float	sum;
	
	i1 = poly_point[polys[pn].pt1_index].sz1;
	i2 = poly_point[polys[pn].pt2_index].sz1;
	i3 = poly_point[polys[pn].pt3_index].sz1;
	i4 = poly_point[polys[pn].pt4_index].sz1;

	sum = i1 + i2 + i3 + i4;
	sum *= 0.25;
	return(sum);
}

//------------------------------------------------------------------------------

void	TSoundView::sort_polys()
{
	long	mid1;
	long	mid2;
	long	i;
	long	j;
	long	step;
	float	zs[512];
	poly	tmp;
	
	
	if (need_sort == 0)
		return;
		
	need_sort = 0;
	
	for (i = 0; i < (poly_count); i++)
		zs[i] = calc_mid(i);


	for (step = poly_count / 2; step > 0; step /= 2) {
		for (i = step; i < poly_count; i++) {
			for (j=i - step; j >= 0 && zs[j] > zs[j+step]; j -= step) {
				tmp = polys[j];
				polys[j] = polys[j + step];
				polys[j + step] = tmp;
			}
		}
	}
}

//-----------------------------------------------------------

short	glb_buffer[PLAY_SAMPLE_COUNT*2];
long	data_sem;
long	buffer_sem;

//-----------------------------------------------------------

bool  record_func(void *data, char *buffer, size_t count, 
				  void *header);

//-----------------------------------------------------------


init_stuff()
{
	BDACStream *dac;
	BSubscriber *sub;
	
	data_sem = create_sem(0, "data_sem");
	buffer_sem = create_sem(1, "buffer_sem");
	dac = new BDACStream();
	
	dac->SetStreamBuffers(PLAY_BUFFER_SIZE, 4);
	dac->SetSamplingRate(44100);
	
	sub = new BSubscriber("subscriber");
	sub->Subscribe(dac);
	sub->EnterStream(NULL, false, 0, record_func, NULL, false);
}

//-----------------------------------------------------------

int length = 0;
#define MAX_FRAMES 1500000

//-----------------------------------------------------------

bool  record_func(void *data, char *buffer, size_t count, 
				  void *header)
{
	int16 *ptr = (int16 *)buffer;
	int16 datum;
	
	acquire_sem(data_sem);
	memcpy(ptr, glb_buffer, sizeof(glb_buffer));
	release_sem(buffer_sem);
	
	//if ((length += PLAY_SAMPLE_COUNT) > MAX_FRAMES)
	//	return false;
	
	return true;
	
}

//-----------------------------------------------------------

float	global_gain = 3.0;

//-----------------------------------------------------------

void	TSoundView::upd_revs(long n, Channel **p)
{
	long	i;
	
	for (i = 0; i < n; i++) {
		lrev[i] = lrev[i] * 31.0 + p[i]->left_weight;
		lrev[i] = lrev[i] * (1.0/32.0);
		rrev[i] = rrev[i] * 31.0 + p[i]->right_weight;
		rrev[i] = rrev[i] * (1.0/32.0);
	}
}

//-----------------------------------------------------------
int	fref = 0;	


void	TSoundView::mix(Channel **p, long n, long step)
{
	long	i;
	long	j;
	long	k;
	float	sum1;
	float	sum2;
	short	*bufs[32];
	float	lgains[32];
	float	rgains[32];
	float	reverbe_l[32];
	long	reverbe_d[32];
	char	do_reverbe[32];
	long	dleft[32];
	long	dright[32];
	short	*out;
	long	tmp;
	long	tmp1;
	float	mult;
/*
	if (fref == 0) {
		fref = open("out", O_BINARY | O_RDWR | O_CREAT);
	}
*/	

	mult = (global_gain/n);
	while(pause_main == 1) {
		if (quit > 0) {
			quit--;
			return;
		}
		snooze(8000);
	}
		
	if (pause_main == 2) {
		snooze(8000);
		return;
	}
	
		
	for (i = 0; i < n; i++) {
		bufs[i] = (short *)(p[i]->GetBuffer() + (sizeof(short) * step * PLAY_SAMPLE_COUNT));
		lgains[i] = p[i]->GetGain() * lrev[i]/*p[i]->left_weight*/;
		rgains[i] = p[i]->GetGain() * rrev[i]/*p[i]->right_weight*/;
		dleft[i] = lrev[i] * 200;
		dright[i] = rrev[i] * 200;
		
		reverbe_l[i] = get_reverbe_level(i) / 2.0;
		if (reverbe_l[i] > 0.01) 
			do_reverbe[i] = 1;
		else
			do_reverbe[i] = 0;
		reverbe_d[i] = p[i]->GetReverbe_distance();
	}
	
	acquire_sem(buffer_sem);
	
	out = glb_buffer;
	
	for (i = 0; i < PLAY_SAMPLE_COUNT; i++) {
		if ((i % 400) == 0) {
			upd_revs(n, p);
			for (k = 0; k < n; k++) {
				lgains[k] = p[k]->GetGain() * lrev[k];
				rgains[k] = p[k]->GetGain() * rrev[k];
				dleft[k] = lrev[k] * 200;
				dright[k] = rrev[k] * 200;
			}
		}
		sum1 = 0;
		sum2 = 0;
		for (j = 0; j < n; j++) {
			if (obj_muted[j] == 0 && obj_solo[j] == 1 ) {
				short	*pt0;
				short	*pt1;
				
				tmp = *(bufs[j] + i);
				
				if (do_reverbe[j]) {
					tmp1 = *(bufs[j] + i - dleft[j]) * 4;
					tmp1+= *(bufs[j] + i - dright[j]) * 4;
					tmp1 *= reverbe_l[j];
					tmp += tmp1;
				}
				
				
				sum1 += tmp * lgains[j];
				sum2 += tmp * rgains[j];
			}
		}
			
		sum1 *= mult;
		sum2 *= mult;
		if (sum1 > 32000) sum1 = 32000;
		if (sum1 < -32000) sum1 = -32000;
		if (sum2 > 32000) sum2 = 32000;
		if (sum2 < -32000) sum2 = -32000;
		*out++ = sum2;
		*out++ = sum1;
	}
	
	//write(fref, glb_buffer, PLAY_SAMPLE_COUNT*2*2);
	release_sem(data_sem);
	if (quit > 0) {
		quit--;
		return;
	}
}

//-----------------------------------------------------------

long player_task(void *p)
{
	init_stuff();
}

//-----------------------------------------------------------

long	normalize(float v)
{
	long	tmp;
	
	tmp = v;
	
	tmp = tmp & (0xfffffffe);
	tmp &= ~(0x3ff);
	return tmp;
}

//-----------------------------------------------------------

long	TSoundView::window_control_task()
{
	long	j;
	long	k;
	long	m;
	float	acc;
	long	n;
	short	*buf;
	long	off;
	short	*xx;
	long	zz;
	double	mt0;
	double	ts;
	
	
	ts =  (AVAIL_BUF * 1.0);
	ts = ts / (44100.0*2.0);
	ts = ts / (MIX_STEP);

	
	mt0 = 0;
	resume_thread(spawn_thread(player_task,"player",B_REAL_TIME_PRIORITY,0));
		
	chan[0] = new Channel("bass");
	chan[1] = new Channel("crash");
	chan[2] = new Channel("fish&clap");
	chan[3] = new Channel("flute");
	chan[4] = new Channel("guitar1");
	chan[5] = new Channel("guitar2");
	chan[6] = new Channel("guitarsolo");
	chan[7] = new Channel("hicrash&china");
	chan[8] = new Channel("hihat");
	chan[9] = new Channel("kick");
	chan[10] = new Channel("ride");
	chan[11] = new Channel("snare");
	chan[12] = new Channel("strings.left");
	chan[13] = new Channel("strings.right");
	chan[14] = new Channel("warmpad");
	chan[15] = new Channel("vocal.final");
	//chan[15]->SetOffset(-44000*0.2*1);
/*
	chan[0] = new Channel("4benoit.bass");
	chan[1] = new Channel("4benoit.bells");
	chan[2] = new Channel("4benoit.drums");
	chan[3] = new Channel("4benoit.gtr1");
	chan[4] = new Channel("4benoit.gtr2");
	chan[5] = new Channel("4benoit.lowstrings");
	chan[6] = new Channel("4benoit.rhodes");
	chan[7] = new Channel("4benoit.saxsolo");
*/

	release_sem(ready_sem);
	mtime = 0.0;
	max_time = 186.0;
	dtime = max_time / 2.0;
	
	while(1) {	
		for (j = 0; j < NC; j++) {
			chan[j]->SetPos(normalize((mtime) * 44100.0 * 2.0));
		}
		for (j = 0; j < MIX_STEP; j++) {
			if (quit > 0) {
				quit--;
				return;
			}
			for (k = 0; k < NC; k++) {
				acc = 0;
				buf = (short *)(chan[k]->GetBuffer() + (sizeof(short) * j * PLAY_SAMPLE_COUNT));
				for (n = 0; n < 32; n++) {
					acc += fabs(*buf);
					buf += 20;
				}

				acc /= 32;
				acc = acc / 512;
				if (acc > 15) acc = 15;
				if (acc < 2) acc = 2;
				SetChannelLevel(k, -acc/11.0);
					
				float	tmp;
				 
				tmp  = ((pos_y[k]+150)/200.0);
				chan[k]->SetLeftRight(1.0-(256+pos_x[k])/512.0);
				if (tmp < 0) tmp = 0;
				chan[k]->SetGain(tmp * tmp);
			}
			mix(chan, NC, j);
			mtime = mtime + ts;
		}
		if (pause_main == 2)
			pause_main = 0;
		
		if (mtime > max_time) {
			mtime = 0;
		}
	}

long	i;
	
	for (i = 0; i < 3; i++) 
		delete chan[i];
}

//-----------------------------------------------------------

long	init_p(void *p)
{
	TSoundView	*x;
	
	x = (TSoundView*)p;
	
	x->window_control_task();
}


//-----------------------------------------------------------

void	TSoundView::do_anim()
{
	float	t0;
	
	snooze(1500000);
	Animate();
	while(1) {
		if (quit > 0) {
			quit--;
			return;
		}
		
		time_mode = TOP_ONLY;
		if (view_state != WAVE_VIEW)
			Animate();
		else {
			if (t0 != mtime) {
				t0 = mtime;
				Animate();
			}
		}
		if (demo_mode)
			snooze(25000);
		else
			snooze(35000);
	}
}

//-----------------------------------------------------------


void	TSoundView::mute_selection()
{
	long	obj;
	char	all_select_muted = 1;
	long	n;
	char	marked[64];
	
	for (obj = 0; obj < 64; obj++)
		marked[obj] = 0;
		
	for (obj = 0; obj < 64; obj++) {
		if (obj_state[obj]) {
			if (obj_muted[obj] == 0) {
				all_select_muted = 0;
				goto out;
			}
		}
	}
	
out:;

	if (all_select_muted) {
		for (obj = 0; obj < 64; obj++) {
			if (obj_state[obj]) {
				obj_muted[obj] = 0;
				marked[obj] = 1;
			}
		}
		for (n = 0; n < 3; n++) {
			for (obj = 0; obj < 64; obj++) {
				if (marked[obj]) {
					ScalePoly(obj, 'itop', 1.0/0.68);
					ScalePoly(obj+1024, 'itop', 1.0/0.68);
				}
			}
			Animate();
		}
	}
	else {
		for (obj = 0; obj < 64; obj++) {
			if (obj_state[obj]) {
				if (obj_muted[obj] == 0) {
					marked[obj] = 1;
					obj_muted[obj] = 1;
				}
			}
		}
		for (n = 0; n < 3; n++) {
			for (obj = 0; obj < 64; obj++) {
				if (marked[obj]) {
					ScalePoly(obj, 'itop', 0.68);
					ScalePoly(obj+1024, 'itop', 0.68);
				}
			}
			Animate();
		}
	}
}

//-----------------------------------------------------------

void	TSoundView::deselect_all()
{
	long	i;
	
	for (i = 0; i < NC; i++) {
		if (obj_state[i]) {
			obj_state[i] = 0;
			SetChannelColor(i, 6);
		}
	}
	update_infos();
}

//-----------------------------------------------------------

void	TSoundView::select_all()
{
	long	i;
	
	for (i = 0; i < NC; i++) {
		obj_state[i] = 1;
		SetChannelColor(i, 2);
	}
	update_infos();
}

//-----------------------------------------------------------

void	TSoundView::solo_selection()
{
	long	obj;
	char	old;
	char	all_selected_solo = 1;
	long	n;
	char	marked[64];
	
	for (obj = 0; obj < 64; obj++)
		marked[obj] = 0;
		

	for (obj = 0; obj < 64; obj++) {
		if (obj_state[obj]) {
			if (obj_solo[obj] == 0) {
				all_selected_solo = 0;
				goto out;
			}
		}
		else {
			if (obj_solo[obj] == 1) {
				all_selected_solo = 0;
				goto out;
			}
		}
	}
out:;
	
	if (all_selected_solo) {
		for (obj = 0; obj < 64; obj++) {
			if (obj_state[obj] == 0) {
				if (obj_solo[obj] == 0) {
					obj_solo[obj] = 1;
					marked[obj] = 1;
				}
			}
		}
		
		for (n = 0; n < 3; n++) {
			for (obj = 0; obj < 64; obj++) {
				if (marked[obj]) {
					ScalePoly(obj, 'itop', 1.0/0.68);
					ScalePoly(obj+1024, 'itop', 1.0/0.68);
				}
			}
			Animate();
		}
	}
	else {
		for (obj = 0; obj < 64; obj++) {
			if (obj_state[obj] == 0) {
				if (obj_solo[obj] == 1) {
					obj_solo[obj] = 0;
					marked[obj] = 1;
				}
			}
			else {
				if (obj_solo[obj] == 0) {
					obj_solo[obj] = 1;
					marked[obj] = 1;
				}
			}
		}
		
		for (n = 0; n < 3; n++) {
			for (obj = 0; obj < 64; obj++) {
				if (obj_state[obj] == 0) {
					if (marked[obj]) {
						ScalePoly(obj, 'itop', 0.68);
						ScalePoly(obj+1024, 'itop', 0.68);
					}
				}
				else {
					if (marked[obj]) {
						ScalePoly(obj, 'itop', 1.0/0.68);
						ScalePoly(obj+1024, 'itop', 1.0/0.68);
					}
				}
			}
			Animate();
		}
	}
}

//-----------------------------------------------------------

void	TSoundView::remove_waves()
{
	long	i;
	
	for (i = 0; i < wave_count; i++)
		delete wave_viewers[i];
	wave_count = 0;
}

//-----------------------------------------------------------

void	TSoundView::change_scale(long ds)
{
	long	i;

	for (i = 0; i < wave_count; i++)
		wave_viewers[i]->change_scale(ds);
		
	
	update_waves();
	Animate();
}

//-----------------------------------------------------------

void	TSoundView::update_waves()
{
	long	i;
	
	if (view_state == MIX_VIEW)
		return;
		
	for (i = 0; i < wave_count; i++)
		wave_viewers[i]->Draw();
}

//-----------------------------------------------------------

void	TSoundView::setup_waves()
{
	long	i;
	long	vp;
	long	index;
	BRect	r;
	long	cnt;
	long	step;
	
	cnt = 0;
	
	for (i = 0;i < 32; i++) {
		if (obj_state[i])
			cnt++;
	}
	
	step = 450 / cnt;
	vp = 30;
	wave_count = 0;
	
	for (i = 0; i < 32; i++) {
		if (obj_state[i]) {
			r.top = vp;
			r.bottom = vp + step - 24;
			r.left = 512 - 250;
			r.right = 512 + 250;
			
			vp += step;
			wave_viewers[wave_count] = new WaveViewer(chan[i], this, r);	
			wave_viewers[wave_count]->Draw();	
			wave_count++;
		}
	}
}


//-----------------------------------------------------------

void	TSoundView::do_wave()
{
	float	delta;
	float	v;
	
	v = BVP + -b*125.0;
	view_state = BOTH;
	
	if (off_y <= 0) {
		delta = 6;
		do {
			off_y += delta;
			delta *= 1.35;
			Animate();
		} while(off_y < v);
		off_y = v;
		view_state = MIX_VIEW;
		remove_waves();
		pause_main = 2;
	}
	else {
		delta = 6;
		setup_waves();
		do {
			off_y -= delta;
			delta *= 1.35;
			Animate();
		} while(off_y >= -360);
		view_state = WAVE_VIEW;
		Animate();
		pause_main = 1;
	}
}

//-----------------------------------------------------------

void TSoundView::KeyDown(const char *key, int32 count)
{
	
	if (view_state == MIX_VIEW) {
		switch(*key) {
			case '=':
			case '+':
				dst = dst * 1.05;
				if (dst > 900.0) dst = 900.0;
				need_vector = 1;
				break;
			case '-':
			case '_':
				dst = dst / 1.05;
				if (dst < 30.0) dst = 30.0;
				need_vector = 1;
				break;
			case 'd':
			case 'D':
				demo_mode ^= 0x01;
				break;
			case 28:
				move_selection(-15, 0);
				break;
			case 29:
				move_selection(15, 0);
				break;
			case 30:
				move_selection(0, -15);
				break;
			case 31:
				move_selection(0, 15);
				break;
			case 'm':
			case 'M':
				mute_selection();
				break;
			case 's':
			case 'S':
				solo_selection();
				break;
			case 'a':
			case 'A':
				select_all();
				break;
			case 'W':
			case 'w':
				do_wave();
				break;
		}
		return;
	}
	
	if (view_state == WAVE_VIEW)
	switch(*key) {
		case 28:
			change_scale(+2);
			break;
		case 29:
			change_scale(-2);
			break;
		case 30:
			move_selection(0, -15);
			break;
		case 31:
			move_selection(0, 15);
			break;
		case 'W':
		case 'w':
			do_wave();
			break;
	}
}


//-----------------------------------------------------------

long	init_anim(void *p)
{
	TSoundView	*x;
	
	x = (TSoundView*)p;
	
	x->do_anim();
}


//-----------------------------------------------------------




//-----------------------------------------------------------

	WaveViewer::WaveViewer(Channel *a_channel, TSoundView *powner, BRect pbound)
{
	owner = powner;
	bound = pbound;
	channel = a_channel;
	off_view = owner->off_view;
	imult = (1.0/65535.0)*(bound.bottom - bound.top);
	
	scale = 4;
}

//-----------------------------------------------------------

	WaveViewer::~WaveViewer()
{
}

//-----------------------------------------------------------

long	WaveViewer::w_map(long v)
{
	long	height;
	
	height = bound.bottom - bound.top;
	height /= 2;
	height -= 8;
	
	v = v * imult;
	
	if (v > height)
		v = height;
	if (v < -height)
		v = -height;
		
	return v;
}

//-----------------------------------------------------------

void	WaveViewer::Draw()
{
	short		*buffer;
	long		i;
	long		w;
	long		mid;
	rgb_color	c;
	long		y0,y1;
	float		sw;
	long		st_w;
	char		*name;
	long		p;
	short		mmax0;
	short		mmin0;
	short		mmax1;
	short		mmin1;
	char		easy_mode;
	
	if (owner->view_state == BOTH)
		easy_mode = 1;
	else
		easy_mode = 0;

	p = normalize(owner->mtime * 44100.0 * 2.0);

	p /= 2;

	name = channel->cname;


	
	imult = (1.0/65535.0)*(bound.bottom - bound.top);

	c.red = 0;
	c.green = 0;
	c.blue = 255;
	
	w = bound.right - bound.left;
	mid = bound.top + bound.bottom;
	mid /= 2;
	mid -= bound.top;
	
	off_view->Window()->Lock();
	off_view->SetHighColor(255, 0, 0);
	off_view->StrokeRect(bound);
	
	
	st_w = off_view->StringWidth(name) + 15;
	
	off_view->StrokeRect(BRect(bound.left,
							    bound.top - 16,
							    bound.left + st_w,
							    bound.top));
							    
							    
	
	off_view->FillTriangle(BPoint(bound.right - 9, bound.bottom),
						    BPoint(bound.right, bound.bottom),
						    BPoint(bound.right, bound.bottom - 9));
						    
	off_view->MovePenTo(BPoint(bound.left + 5, bound.top - 5));
	off_view->SetHighColor(255, 80, 80);
	off_view->DrawString(name);
	
	off_view->Sync();
	
	sw = w / 10.0;
	
	for (i = 1; i < 10; i++) {
		fline(i * sw, 1, i * sw, (bound.bottom-bound.top) - 1, 3);
	} 	
	
	for (i = 1; i < w; i++) {
		if (!easy_mode) {
			channel->get_sample(p + (scale * i - scale), scale, &mmax0, &mmin0);
			channel->get_sample(p + (scale * i), scale, &mmax1, &mmin1);
		
			y0 = mid + w_map(mmax0);
			y1 = mid + w_map(mmax1);
			fline(i, y0, (i + 1), y1, 32);
		
			y0 = mid + w_map(mmin0);
			y1 = mid + w_map(mmin1);
			fline(i, y0, (i + 1), y1, 42);
		}
	}
	
	off_view->Window()->Unlock();
}

//-----------------------------------------------------------

void	WaveViewer::change_scale(long ds)
{
	long	s0;
	
	if (ds > 0) {
		s0 = scale;
		scale = scale * 1.15;
		if (s0 == scale) scale++;
	}
	else {
		s0 = scale;
		scale = scale / 1.15;
		if (s0 == scale)
			scale--;
	}
	
	if (scale < 1)
		scale = 1;
}

//-----------------------------------------------------------

void	WaveViewer::Move(long new_x, long new_y)
{
	BRect	newr;
	
	newr.top = new_y;
	newr.left = new_x;
	newr.right = newr.left + (bound.right - bound.left);
	newr.bottom = newr.top + (bound.bottom - bound.top);
	
	bound = newr;
}

//-----------------------------------------------------------

void	WaveViewer::fline(long xx1, long yy1, long xx2, long yy2, uchar c)
{
	long	tmp;
	float	dy;
	float	dx;
	
	xx1 += bound.left;
	xx2 += bound.left;
	yy1 += bound.top;
	yy2 += bound.top;
	
	if (xx1 > xx2) {
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
	}
	
	dy = yy2 - yy1;
	dx = xx2 - xx1;
	
	
	if (xx1 < 0) {
		yy1 = yy1 + (dy * ((-xx1)/dx));
		xx1 = 0;
	}

	if (xx1 > (HS-1))
		return;
		
	if (xx2 > (HS-1)) {
		yy2 = yy2 - (dy * (xx2-(HS-1))/dx);
		xx2 = (HS-1);
	}

	if (yy1 > yy2) {
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
	}
	
	dy = yy2 - yy1;
	dx = xx2 - xx1;
	
	
	if (yy1 < 0) {
		xx1 = xx1 + (dx * ((-yy1)/dy));
		yy1 = 0;
	}

	if (yy1 > (VS))
		return;
		
	if (yy2 > VS) {
		xx2 = xx2 - (dx * (yy2-VS)/dy);
		yy2 = VS;
	}

	

	ffline(xx1+1,yy1+1,xx2+1, yy2+1, c);
}

//--------------------------------------------------------------

void	TSoundView::do_size(long cur)
{
	BPoint	where0;
	BPoint	where;
	BRect	tmp_bound;
	ulong	buttons;
	long	dh,dv;	
	float	totv_but;
	long	i;
	float	ratio;		
	float	cvp;
	float	old_v;
		
	Window()->Lock();
	
	GetMouse(&where0, &buttons);
	
	while(buttons) {
		snooze(25000);
		GetMouse(&where, &buttons);

		if ((where.x != where0.x) || (where.y != where0.y)) {
			dh = where.x - where0.x;
			dv = where.y - where0.y;
			where0 = where;
			
			totv_but = 0;
			
			wave_viewers[cur]->bound.bottom += dv;
			
			if ((wave_viewers[cur]->bound.bottom - wave_viewers[cur]->bound.top) < 18) {
				wave_viewers[cur]->bound.bottom = wave_viewers[cur]->bound.top + 18;
				dv = 0;
			}
			
			for (i = cur + 1; i < wave_count; i++) {
				tmp_bound = wave_viewers[i]->bound;
				totv_but += tmp_bound.bottom - tmp_bound.top;
			}

			if (totv_but == 0)
				totv_but = 1.2224324;
				
			ratio = (totv_but - dv) / totv_but;


			if (dv) {
				cvp = wave_viewers[cur]->bound.bottom + 24;
				
				for (i = cur + 1; i < wave_count; i++) {
					old_v = wave_viewers[i]->bound.bottom - wave_viewers[i]->bound.top;
					wave_viewers[i]->bound.top = cvp;
					wave_viewers[i]->bound.bottom = cvp + (old_v * ratio);
					if ((wave_viewers[i]->bound.bottom - wave_viewers[i]->bound.top) < 18)
						wave_viewers[i]->bound.bottom = wave_viewers[i]->bound.top + 18;
						
					cvp = wave_viewers[i]->bound.bottom + 24;
				}
			}
			Animate();
		}
	}
	Window()->Unlock();
}

//--------------------------------------------------------------

void	TSoundView::handle_wave_click(BPoint where)
{
	long	i;
	BRect	bnd;
	
	for (i = 0; i < wave_count; i++) {
		if (wave_viewers[i]->bound.Contains(where)) {
			bnd = wave_viewers[i]->bound;
			if (((bnd.right - where.x) < 9) &&
			    ((bnd.bottom - where.y) < 9)) {
				do_size(i);
			}
		}
	}
}

//--------------------------------------------------------------

void	WaveViewer::ffline(long x1,long y1,long x2, long y2, uchar c)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = HS;
	long	error;
	long	cpt;
	char 	*base;
	float	k;
	
	
	if (y1 < 1 || y1 > (VS-2) || y2 < 1 || y2 > (VS-2))
		return;
	if (x1 < 0 || x1 > HS || x2 < 0 || x2 > HS)
		return;
	
	
	dx = x2-x1;
	dy = y2-y1;
	
	base = owner->the_base + y1*rowbyte+x1;
	
	if (dx==0 && dy==0) {
		*base = c;
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
			dy *= k;
			dx *= k;
			
			while(cpt>=0) {
				cpt--; 
				*base = c;
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
			dy *= k;
			dx *= k;
			while(cpt >= 0) {
				cpt--;
				*base =  c;
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
			dy *= k;
			dx *= k;
			
			while(cpt>=0) {
				cpt--; 
				*base = c;
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
			dy *= k;
			dx *= k;
			while(cpt >= 0) {
				cpt--;
				*base =  c;
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


void *bmemset(void *s, int c, size_t n)
{
	long	*p;
	long	cnt;
	
	cnt = n;
	cnt /= 16;
	
	p = (long *)s;
	
	while(cnt--) {
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	}
}


/*
__asm void *fast_memset(void *s, int c, size_t n) {
	cmpli	cr2, 0, r5, 31
	subi	r3, r3, 4
	rlwimi	r4, r4, 8, 16, 23
	rlwimi	r4, r4, 16, 0, 15
	blt		cr2, queue
	
	subfic	r9, r3, 28
	rlwinm	r6, r9, 29, 30, 31
	rlwinm	r7, r9, 0, 31, 31
	cmpli	cr7, 0, r6, 0
	cmpli	cr6, 0, r7, 0
	rlwinm	r10, r9, 0, 27, 31
	rlwinm	r8, r9, 0, 30, 30
	rlwinm	r7, r9, 0, 29, 29
	sub		r10, r5, r10
	cmpli	cr5, 0, r8, 0
	cmpli	cr4, 0, r7, 0
	cmpli	cr2, 0, r10, 32
	
	beq		cr6, al1
	subi	r5, r5, 1
	addi	r3, r3, 1
	stb		r4, +3(r3)
al1:
	beq		cr5, al2
	subi	r5, r5, 2
	addi	r3, r3, 2
	sth		r4, +2(r3)
al2:
	beq		cr4, alt4
	subi	r5, r5, 4
	stwu	r4, +4(r3)
alt4:
	beq		cr7, alt8
loop4:
	subic.	r6, r6, 1
	subi	r5, r5, 8
	stw	 	r4, +4(r3)
	stwu	r4, +8(r3)
	bne		cr0, loop4
alt8:
	blt		cr2, queue
	li		r7, 4
loop:
	dcbz	r7, r3
	subi	r5, r5, 32
	cmpli	cr2, 0, r5, 32
	stw 	r4, +4(r3)
	stw 	r4, +8(r3)
	stw 	r4, +12(r3)
	stw 	r4, +16(r3)
	stw 	r4, +20(r3)
	stw 	r4, +24(r3)
	stw 	r4, +28(r3)
	stwu	r4, +32(r3)
	bge		cr2, loop
	
queue:
	rlwinm	r6, r5, 29, 30, 31
	rlwinm	r7, r5, 0, 29, 29
	cmpli	cr3, 0, r6, 0
	cmpli	cr4, 0, r7, 0
	rlwinm	r8, r5, 0, 30, 30
	rlwinm	r9, r5, 0, 31, 31
	cmpli	cr5, 0, r8, 0
	cmpli	cr6, 0, r9, 0
	beq		cr3, no8
loop3:
	subic.	r6, r6, 1
	stw	 	r4, +4(r3)
	stwu	r4, +8(r3)
	bne		cr0, loop3
no8:
	beq		cr4, no4
	stwu	r4, +4(r3)
no4:
	beq		cr5, no2
	addi	r3, r3, 2
	sth		r4, +2(r3)
no2:
	beq		cr6, no1
	addi	r3, r3, 1
	stb		r4, +3(r3)
no1:
	addi	r3, r3, 4
	blr
}
*/
