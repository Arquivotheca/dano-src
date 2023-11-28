#include "sound_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Screen.h>
#include <Region.h>
#include <Application.h>
#include <Mime.h>
#include <Path.h>
#include "util.h"
#include "play.h"
#include "fftshift.h"
#include <math.h>

#ifndef	WAVE_WINDOW_H
#include "wave_window.h"
#endif


#ifndef	TRACK_VIEW_H
#include "track_view.h"
#endif
//--------------------------------------------------------------
#include "biquad.cpp"
//--------------------------------------------------------------

long			init_p(void *p);
long			init_anim(void *p);
void			set_cur_time(float new_one);
extern	char	slow_anim;
extern	char	sub_size;
extern	char	about_producer;
extern	uchar 	Gamma[256];

extern "C" void *fast_memset(void *s, int c, size_t n);
extern	void	v_meter(long v1, long v2);

#define	DRAG_TAG	768
#define	MAX_HINT	32768
#define	MEMSET memset


TSoundView	*me;

#if MIDI_CONTROLLER
//--------------------------------------------------------------

class	xController : public Controller {
public:
    			xController(char *device);
				~xController();
virtual void	SliderChanged(uint slider, double value);
virtual void 	DialChanged(uint direction);
virtual void    ButtonChanged(uint button, uint on);
};


//--------------------------------------------------------------

	xController::xController(char *device) : Controller(device)
{
}

//--------------------------------------------------------------

	xController::~xController() {}

//--------------------------------------------------------------

void	xController::SliderChanged(uint slider, double value)
{
	me->SetChannelPos((slider - 1), value * 512.0 - 256.0);
}

void	xController::ButtonChanged(uint button, uint on)
{
	if(on) {
		me->toggle_mute(button - 1);
	}

}

//--------------------------------------------------------------

void 	xController::DialChanged(uint direction)
{
	long	d;

	if (me->has_selection()) {
		d = (int)((direction  - 0.5) * 10.0);
		me->Window()->Lock();
		me->move_selection(d, 0, 0);
		me->Window()->Unlock();
	}
	else
		me->move_alpha((direction-0.5)/5.0);
}
			
//--------------------------------------------------------------

#endif // MIDI_CONTROLLER

float		get_start_time();
float		get_end_time();
rgb_color	pmap(rgb_color c);
void		mixer_add_channel(Channel *c, float x, float y);
void		mix_master(char *p);
void		remove_channel(Channel *c);
void		stop_mixer();
void		start_mixer();
void		get_channel_pos(Channel *a_channel, float *x, float *y);
void		get_mute_solo(Channel *a_channel, char *mute, char *solo);
void		set_mute_solo(Channel *a_channel, char mute, char solo);
rgb_color	rand_r_g_b_w();
void		hline(uchar *base, long count, uchar *mapper);
long		player_task(void *p);

//--------------------------------------------------------------

rgb_color	pmap(rgb_color c)
{
	c.red = Gamma[c.red];
	c.green = Gamma[c.green];
	c.blue = Gamma[c.blue];

	return c;
}

//--------------------------------------------------------------

void	mixer_add_channel(Channel *c, float x, float y)
{
	me->Window()->Lock();
	me->do_add_channel(c, x, y);
	me->Animate();
	me->recalc_pos_xy();
	me->Window()->Unlock();
}

//--------------------------------------------------------------

void	mix_master(char *p)
{
	me->do_final_mix(p);
}

//--------------------------------------------------------------

void	remove_channel(Channel *c)
{
	me->remove_channel(c);

}

//--------------------------------------------------------------

void	stop_mixer()
{
	if (me->Count() == 0)
		return;

	me->pause_main = 3;
	while(me->pause_main != 4) {
		snooze(62000);
	}
}

//--------------------------------------------------------------

void	start_mixer()
{
		me->pause_main = 0;
}

//--------------------------------------------------------------

void	get_channel_pos(Channel *a_channel, float *x, float *y)
{
	me->GetChannelPos(a_channel, x, y);
}

//--------------------------------------------------------------

void	get_mute_solo(Channel *a_channel, char *mute, char *solo)
{
	me->GetMuteSolo(a_channel, mute, solo);
}

//--------------------------------------------------------------

void	set_mute_solo(Channel *a_channel, char mute, char solo)
{
	me->SetMuteSolo(a_channel, mute, solo);
}

//--------------------------------------------------------------

void	TSoundView::move_alpha(float da)
{
	c += da;

	Window()->Lock();
	change_view_point(a,-b,c);
	clear();
	need_vector = 1;
	draw_vectors();
	UpdateTime(pan2_view);
	composit_pannel1();
	composit_pannel2();

	Draw(BRect(0,0,0,0));
	Window()->Unlock();

}

//--------------------------------------------------------------

void	TSoundView::SetTrackView(TrackView *t)
{
	trak = t;
}

//--------------------------------------------------------------

void	TSoundView::SetChannelPos(long c, float v)
{
	float	x,y;
	float	dy;

	if (c >= NC)
		return;

	recalc_pos_xy();
	GetChannelPos(chan[c], &x, &y);

	dy = y - v;

	MovePoly(c, 0, 0, -dy);
}

//--------------------------------------------------------------

void	TSoundView::remove_channel(Channel *c)
{
	long	i;

	for (i = 0; i < MAX_CHANNEL; i++) {
		if (c == me->chan[i]) {
			delete_channel(i);
			return;
		}
	}
}

//--------------------------------------------------------------

long	TSoundView::Count()
{
	return NC;
}

//--------------------------------------------------------------

int		TSoundView::add_ppoint(float x1, float z1, float y1, long obj_id, ulong part)

{
	long	i;
	long	pos;
	
start:;

	pos = ppoint_count;


	if (ppoint_hint == MAX_HINT) {
insert:;

		poly_point[pos].x1 = x1;
		poly_point[pos].y1 = -y1;
		poly_point[pos].z1 = z1;
		poly_point[pos].object_id = obj_id;
		poly_point[pos].part = part;
		poly_point[pos].touched = 1;
		if (pos == ppoint_count)
			ppoint_count++;
	
		return (pos);
	}
	else {
		for (i = ppoint_hint; i < ppoint_count; i++) {
			if (poly_point[i].object_id == -1) {
				pos = i;	

				goto insert;
			}
		}
		ppoint_hint = MAX_HINT;
		goto start;
	}
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
// not the fastest... but this is only used when deleting objects.
// Ha... pretty silly of me to use absolute ids which have to be
// consecutive.

void	TSoundView::remap_id(long old_one, long new_one)
{
	long	i;

	for (i = 0; i < poly_count; i++) {
		if (polys[i].object_id == old_one) {
			polys[i].object_id = new_one;
		}
	}
	
	for (i = 0; i < ppoint_count; i++) {
		if (poly_point[i].object_id == old_one) {
			poly_point[i].object_id = new_one;
		}

		if (poly_point[i].object_id == (old_one + 1024)) {
			poly_point[i].object_id = new_one + 1024;
		}
	}
}

//--------------------------------------------------------------

void	TSoundView::delete_obj(long obj_id)
{
	long	i;
	long	dest;

	dest = 0;

	for (i = 0; i < poly_count; i++) {
		if (polys[i].object_id == obj_id) {
		}
		else {
			if (dest != i) {
				polys[dest] = polys[i];
			}
			dest++;
		}
	}
	poly_count = dest;
	
	for (i = 0; i < ppoint_count; i++) {
		if (poly_point[i].object_id == obj_id) {
			poly_point[i].object_id = -1;
			if (i < ppoint_hint) 
				ppoint_hint = i;
		}

		if (poly_point[i].object_id == (obj_id+1024)) {
			poly_point[i].object_id = -1;
			if (i < ppoint_hint) 
				ppoint_hint = i;
		}
	}
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
	
	for (i = 0; i < MAX_CHANNEL; i++) {
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
	long	srowbyte = p1_x;
	long	mx;
	long	rx;
	
	return;
	if (!has_selection())
		return;
		
	dx  = (int)pannel1_rect.left;
	dy  = (int)pannel1_rect.top;

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
	
	return;	
	dx  = (int)pannel2_rect.left;
	dy  = (int)pannel2_rect.top;

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
	
	return;
	UpdateTime(pan2_view);
	
	for (i = 0; i < MAX_CHANNEL; i++) {
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
		snooze(18000);
		
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

	return 0;
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
			d = (int)fabs(real_d);
			
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
			snooze(15000);
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
	
	mid = (int)((r.top+r.bottom) / 2);
	
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

	mid = (int)((r.top+r.bottom) / 2);
	
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
	
	return;
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
	
	tmp = (int)mtime;
	
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
	
	tv[0] = (int)r.top;
	tv[9] = (int)r.bottom;
	tv[10] = (int)r.bottom;
	
	c.red = 50;
	c.green = 100;
	c.blue = 20;
	
	if (time_mode != CENTER_ONLY) {
		off->BeginLineArray(100);
		for (i = -pi/2 + pi/10.0; i < pi/2 - (pi/20.0); i+= pi/40.0) {
			m = (int)(mid + (vs*sin(i)));
			off->AddLine(BPoint(r.right+1, m),BPoint(r.right + 3,  m), c); 
		}
	
		c.red = 140;
		c.green = 10;
		c.blue = 20;
		
		for (i = -pi/2 + pi/10.0; i < pi/2; i+= pi/10.0) {
			tv[k] = (int)(mid + (vs*sin(i)));
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
		
		tmp = (int)cur;
		
		lt[(int)i+5] = cur;
		
		h = tmp / 3600;
		m = tmp - (h * 3600);
		m /= 60;
		is = (int)(cur - (h*3600) - (m*60));
		
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
	off->SetHighColor(40,40,0);
	off->FillRect(BRect(0,0, p1_x, p1_y));
	off->SetHighColor(50, 50, 255);
	off->SetFontSize(11);
	DrawStringRight(off, BPoint(70, 20), "Channel : ");
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
	off->SetDrawingMode(B_OP_OVER);
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

void	TSoundView::BuildMappers()
{
	long		intens;
   	long		i;
	rgb_color	c0;
	rgb_color	c1;
	rgb_color	col;
	BScreen		screen(B_MAIN_SCREEN_ID);


//mappers 0 to 3 are different levels of green 

    for (intens = 0; intens < 4; intens++) {
   		c0.red = 55 + (intens * 38);
   		c0.green = 0;
   		c0.blue = 0;

		if (intens == 3) {
   			c0.red = 115 + (intens * 38);
		}
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green);
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens][i] = screen.IndexForColor(pmap(col));
    	}
    }
    	
//mappers 4 to 7 are different levels of green 

    for (intens = 0; intens < 4; intens++) {
   		c0.red = 0;
   		c0.green = 55 + (intens * 38);
   		c0.blue == 0;
		if (intens == 3) {
   			c0.green = 115 + (intens * 38);
		}
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+4][i] = screen.IndexForColor(pmap(col));
    	}
    }

//mappers 8 to 11 are different levels of blue 

    for (intens = 0; intens < 4; intens++) {
   		c0.red = 0;
   		c0.green = 0;
   		c0.blue = 55 + (intens * 38);
		if (intens == 3) {
   			c0.blue = 115 + (intens * 38);
		}
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+8][i] = screen.IndexForColor(pmap(col));
    	}
    }
    
	for (intens = 0; intens < 4; intens++) {
   		c0.red = 0;
   		c0.green = 35 + (intens * 38);
   		c0.blue = 35 + (intens * 38);
		if (intens == 3) {
   			c0.green = 115 + (intens * 38);
   			c0.blue = 115 + (intens * 38);
		}
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+12][i] = screen.IndexForColor(pmap(col));
    	}
    }

	for (intens = 0; intens < 4; intens++) {
   		c0.red = 35 + (intens * 38);
   		c0.green = 0;
   		c0.blue = 35 + (intens * 38);
		if (intens == 3) {
   			c0.red = 115 + (intens * 38);
   			c0.blue = 115 + (intens * 38);
		}
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+16][i] = screen.IndexForColor(pmap(col));
    	}
    }
	
	for (intens = 0; intens < 4; intens++) {
   		c0.red = 35 + (intens * 38);
   		c0.green = 35 + (intens * 38);
   		c0.blue = 0;
		if (intens == 3) {
   			c0.red = 115 + (intens * 38);
   			c0.green = 115 + (intens * 38);
		}
   		
	    for (i = 0; i < 256; i++) {
 	   		c1 = screen.ColorForIndex(i);
			col.red = cmix(c0.red , c1.red);
			col.green = cmix(c0.green , c1.green );
			col.blue = cmix(c0.blue , c1.blue);
       		mapper[intens+20][i] = screen.IndexForColor(pmap(col));
    	}
    }
}

//--------------------------------------------------------------

char	TSoundView::do_object()
{
	float	r;
	float	x0,y0;
	float	x1,y1;
	float	p2;
	float	i;
	float	h;
	float	mh;
	float	mra;
	float	hole_ra;
	long	j;
	float	tube_r = 1.0 * 25.4 * 0.5;
	float	tube_l = 15.0*3.5*25.4 * 0.5;
	
	return 0;

	p2 = 3.1415926*2.0;

	r = 220.0 * 0.5;					//cage radius
	mra = 7.5*25.4 * 0.5;				//mirror radius
	h = 160.0 * 0.5;					//cage height
	mh = 1.5*25.4 * 0.5;				//mirror height

	hole_ra = 1.0*25.4 * 0.5;			//central hole radius

float	dx,dy;

	dx = sin(p2/16.0) * r;
	dy = cos(p2/16.0) * r;

	for (i = 0; i < p2; i += p2/8.0) {
		x0 = sin(i) * tube_r;
		y0 = cos(i) * tube_r;
		x1 = sin(i + p2/8.0) * tube_r;
		y1 = cos(i + p2/8.0) * tube_r;

		x0 += dx;
		y0 += dy;
		x1 += dx;
		y1 += dy;
		add_vector(x0, 0, y0,
				   x1, 0, y1);
	
		add_vector(x0, tube_l, y0,
				   x1, tube_l, y1);
		add_vector(x0, 0, y0,
				   x0, tube_l, y0, 100);
	}
	add_vector(x0, tube_l, y0,
			   0,  tube_l, 0, 100);
	add_vector(x1, tube_l, y1,
			   0,  tube_l, 0, 100);

	add_vector(x0, tube_l-5.0, y0,
			   0,  tube_l-5.0, 0, 100);
	add_vector(x1, tube_l-5.0, y1,
			   0,  tube_l-5.0, 0, 100);
	
	add_vector(x0, tube_l-10.0, y0,
			   0,  tube_l-10.0, 0, 100);
	add_vector(x1, tube_l-10.0, y1,
			   0,  tube_l-10.0, 0, 100);


	for (i = 0; i < p2; i+=(p2/8.0)) {
		x0 = sin(i)*r;
		y0 = cos(i)*r;
		x1 = sin(i + (p2/8.0))*r;
		y1 = cos(i + (p2/8.0))*r;

		add_vector(x0,0,y0,
				   x1,0,y1);

		add_vector(x0,h,y0,
				   x1,h,y1);
		
		add_vector(x0,0,y0,
				   x0,h,y0);
	}

	i = 0;
	x0 = sin(i)*r;
	y0 = cos(i)*r;
	x1 = sin(i + (p2/8.0))*r;
	y1 = cos(i + (p2/8.0))*r;

	for (j = 0; j < h; j += 5) {
		add_vector(x0,j,y0,
				   x1,j,y1);
	}
	
	i = p2 / 4.0;
	x0 = sin(i)*r;
	y0 = cos(i)*r;
	x1 = sin(i + (p2/8.0))*r;
	y1 = cos(i + (p2/8.0))*r;

	for (j = 0; j < h; j += 5) {
		add_vector(x0,j,y0,
				   x1,j,y1);
	}
	
	for (i = 0; i < p2; i+=(p2/64.0)) {
		x0 = sin(i)*mra;
		y0 = cos(i)*mra;
		x1 = sin(i + (p2/64.0))*mra;
		y1 = cos(i + (p2/64.0))*mra;

		add_vector(x0,4,y0,
				   x1,4,y1, 100);

		add_vector(x0,4+mh,y0,
				   x1,4+mh,y1, 100);
		
		add_vector(x0,4,y0,
				   x0,4+mh,y0, 100);
	}

	for (i = 0; i < p2; i+=(p2/10.0)) {
		x0 = sin(i)*mra;
		y0 = cos(i)*mra;
		
		x1 = sin(i)*hole_ra;
		y1 = cos(i)*hole_ra;

		add_vector(x0,4,y0,
				   x1,4,y1, 100);

		add_vector(x0,4+mh,y0,
				   x1,4+mh,y1, 100);
	
		x0 = sin(i+p2/10.0)*hole_ra;
		y0 = cos(i+p2/10.0)*hole_ra;
		add_vector(x0,4,y0,
				   x1,4,y1, 100);
		add_vector(x0,4+mh,y0,
				   x1,4+mh,y1, 100);
	}
	return 1;
}

//--------------------------------------------------------------

rgb_color	rand_r_g_b_w()
{
	rgb_color	c;

	c.red = 255;
	c.green = 0;
	c.blue = 0;

	switch(rand() & 3) {
		case	0:
			c.red = 255;
			c.green = 0;
			c.blue = 0;
			break;

		case	1:
			c.red = 0;
			c.green = 255;
			c.blue = 0;
			break;
		case	2:
			c.red = 0;
			c.green = 0;
			c.blue = 255;
			break;
		case	3:
			c.red = 255;
			c.green = 255;
			c.blue = 255;
			break;
	}
	return c;
}

//--------------------------------------------------------------

void	TSoundView::do_final_mix(char *p)
{
	char		buf[512];
	BEntry		*entry;
	entry_ref	ref;

	output = new BSoundFile();

	unlink(p);
	int fd = open(p, O_RDWR | O_CREAT, 0644);
	fs_remove_attr(fd, "BEOS:TYPE");
	fs_write_attr(fd, "BEOS:TYPE", B_MIME_STRING_TYPE, 0,
				  "audio/x-wav", strlen("audio/x-wav") + 1);
	close(fd);
	
	entry = new BEntry(p);
	if (!entry->Exists()) {
		//printf("no entry\n");
		return;
	}

	entry->GetRef(&ref);

	output->SetTo(&ref, O_WRONLY);
	output->SetSampleSize(44100);
    output->SetChannelCount(2);
	output->SetFileFormat(B_WAVE_FILE);
	output->SetSampleSize(sizeof(short));
 
	f_final_mix = 1;

	delete entry;
}

//--------------------------------------------------------------

TSoundView::TSoundView (BRect r)
  : BView (r, "",
		   0/*B_FOLLOW_ALL*/, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	long		x,y;
	float		fx,fy;
	long		cpt;
	long		i;
	rgb_color	col;
	rgb_color	c0;
	rgb_color	c1;
	long		tmp;
	long		intens;
	BScreen	screen(B_MAIN_SCREEN_ID);

	me = this;
	f_final_mix = 0;
	output = 0;
	dst = 300.0;	
	jump_mode = 0;		
	min_y = 0;
	max_y = VS;
	o_min_y = 0;
	o_max_y = VS;
	min_x = 0;
	max_x = HS;
	o_min_x = 0;
	o_max_x = HS;
	level1 = 0;
	level2 = 0;

	ppoint_hint = MAX_HINT;

	quit = -1;
	for (i = 0; i < MAX_CHANNEL; i++) {
		lrev[i] = 0.5;
		rrev[i] = 0.5;
	}
	
	demo_mode = 0;
	
	view_state = MIX_VIEW;
	in_move = 0;
		
	b1_state[0] = 0;
	b1_state[1] = 0;
	b1_state[2] = 0;
	do_snow = 0;
	
	for (i = 0; i < NP; i++) {
		pixels[i].x = rand()%300 - 150;
		pixels[i].y = 0;
		pixels[i].z = rand()%300 - 150;
		pixels[i].dx = 0;
		pixels[i].dy = 0;
		pixels[i].dz = 0.0;
		pixels[i].color = screen.IndexForColor(rand_r_g_b_w());
	}
	
	
	for (i = 0; i < 128; i++) {
		obj_state[i] = 0;
		obj_hilite[i] = -1;
		obj_muted[i] = 0;
		obj_solo[i] = 0;
		obj_silent[i] = 0;
		channel_reverbe[i] = 0;
	}
	
	
	time_mode = FULL_DRAW;
	
	need_sort = 1;	
	need_vector = 1;
	no_name = 0;	
	vector_count = 0;	
	size_x = HS;
	size_y = VS-1;
	w2 = (int)(size_x / 2.0);
	the_bits = new BBitmap(BRect(0,0,HS - 1,VS+2), B_COLOR_8_BIT, TRUE);
	the_bits->AddChild(off_view = new BView(BRect(0,0,HS - 1,VS), "", B_FOLLOW_ALL, B_WILL_DRAW));
	pannel1 = new BBitmap(BRect(0,0,p1_x - 1,p1_y), B_COLOR_8_BIT, TRUE);
	pannel1->AddChild(pan1_view = new BView(BRect(0,0,p1_x, p1_y), "", B_FOLLOW_ALL, B_WILL_DRAW));
	pannel1_rect = BRect(0, 300,0 + p1_x, 300 + p1_y);
	
	pannel2 = new BBitmap(BRect(0,0,255,255), B_COLOR_8_BIT, TRUE);
	pannel2->AddChild(pan2_view = new BView(BRect(0,0,255,255), "", B_FOLLOW_ALL, B_WILL_DRAW));
	pannel2_rect = BRect(85, 30, 85 + p2_x, 30 + p2_y);
	
	Draw1(pan1_view);
	Draw2(pan2_view);

	SetViewColor(B_TRANSPARENT_32_BIT);
	the_base = (char *)the_bits->Bits();
	clear();

	for (x = 0; x < 256; x++) {
		c0 = screen.ColorForIndex(x);
		for (y = 0; y < 256; y++) {
 	   		c1 = screen.ColorForIndex(y);
 	   		col.red = cmix(c0.red, c1.red);
 	   		col.green = cmix(c0.green, c1.green);
 	   		col.blue = cmix(c0.blue, c1.blue);

 	   		if (y == 0) {
 	   			col.red = (int)(c0.red * 0.63);
 	   			col.green = (int)(c0.green * 0.63);
 	   			col.blue = (int)(c0.blue * 0.7);
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
 	   		
 	   		composit[(x<<8) + y] =  screen.IndexForColor(pmap(col));
		}
	}
	

	BuildMappers();

    for (i = 0; i < 32; i++) {
        col.red = 0;
        col.green = 0;
        col.blue = (int)(i * 7.0);
        blue_map[i] = screen.IndexForColor(pmap(col));
        col.red = (int)(i * 7.0);
        col.green = 0;
        col.blue = 0;
        red_map[i] = screen.IndexForColor(pmap(col));
    }
    
    for (i = 0; i < 32; i++) {
        col.red = i;
        col.green = (int)(i*2.6);
        col.blue = i;
        green_map[i] = screen.IndexForColor(pmap(col));
    }
    
   
    for (intens=0; intens < 20; intens++) {
 	  for (i = 0; i < 32; i++) {
			col.red = 0;
			col.green = 0;
			col.blue = (int)(i*((intens+8)/3.3));								//was 4.5
			level_map[intens][i] = screen.IndexForColor(pmap(pmap(col)));		//hack on !
  	  }
    }
	
 	 for (i = 0; i < 32; i++) {
		col.red = (int)(i*((8)/3.5));
		col.green = (int)(i*((8)/3.5));
		col.blue = (int)(i*((8)/3.5));
		white_map[i] = screen.IndexForColor(pmap(col));
  	  }
  	  
	a = 0;
	b = 0.32;
	//off_y = BVP + - b*125.0;
	off_y = BVP;						// no move for rotation
	c = 0.1;


	vpx = 0;
	vpy = 100;
	vpz = -430;

	cpt = 0;
	
	if (!do_object()) {
		for (fx = 0; fx <= 512.1; fx+= 21.333333333333) {
			add_vector( fx-256,0,-256,	
						fx-256,0,256);	
			
			add_vector(-256,0,fx-256,	
					   256,0,fx-256);
		}
	}
	
		   
	add_vector(-140,0,290,
			   140,0,290);
	
	add_vector(0, 0, 282,
			   0, 0, 288);			//xx

// letter L

	add_vector(-160, -12, 290,
			   -160, 0, 290, 200);
	add_vector(-160,0,290,
			   -151,0,290, 200);

// letter R

	add_vector(151, -12, 290,
			   151, 0, 290, 200);

	add_vector(151, -12, 290,
			   160, -12, 290, 200);

	add_vector(160, -12, 290,
			   160, -6, 290, 200);

	add_vector(160, -6, 290,
			   151, -6, 290, 200);

	add_vector(151, -6, 290,
			   160, -0, 290, 200);


// end letter R

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
	
	
	NC = 0;

	for (i = 0; i < NC; i++) {
		new_block(i);
	}	
	

	vs = 45;
	hs = 16;
/*
	long	pt[256];

	for (i = 0; i < 256; i++) {
		fx = sin(6.2818*(i/256.0)) * hs;
		fy = cos(6.2818*(i/256.0)) * hs;
		pt[i] = add_ppoint(fx,fy,3, DRAG_TAG, 'bomm');
	}

	m1 = add_ppoint(0,0,vs,DRAG_TAG, 'itop');
		
	for (i = 0; i < 255; i+=2) {
		add_poly(pt[i], pt[i+1], m1, m1, 9, DRAG_TAG);
	}
	MovePoly(DRAG_TAG,-100000 , 0, -0);
*/


	tag_x = -100000;
	tag_y = -0;
	
	
	ready_sem = create_sem(0, "ready");
	channel_list_sem = create_sem(1, "channel_list");
	resume_thread(spawn_thread(init_p,"init_p",B_REAL_TIME_PRIORITY,this));
	acquire_sem(ready_sem);
	resume_thread(spawn_thread(init_anim,"init_anim",B_DISPLAY_PRIORITY,this));

	change_view_point(a,-b,c);

	clear();
	draw_vectors();
#if MIDI_CONTROLLER
	mController = new xController("/dev/midi/sonic_vibes/1");
#endif
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
		

	max = 20;
	
	quit = 3;
	while(quit > 0 && max) {
		snooze(50000);
		max--;
	}
#if MIDI_CONTROLLER
	delete mController;
#endif
	delete_sem(channel_list_sem);
	delete_sem(ready_sem);
	be_app->PostMessage(B_QUIT_REQUESTED);

}

#define left_code	0x01
#define	right_code	0x02
#define	bottom_code	0x04
#define	top_code	0x08


//--------------------------------------------------------------

void	TSoundView::fline(long xx1, long yy1, long xx2, long yy2, char *mp, char anti)
{
	long	tmp;
	float	dy;
	float	dx;
	
	if (yy1 > yy2) {
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
	}

	if (yy1 < min_y)
		min_y = yy1;
	if (yy2 > max_y)
		max_y = yy2;

	if (xx1 > xx2) {
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
	}

	if (xx1 < min_x)
		min_x = xx1;
	if (xx2 > max_x)
		max_x = xx2;
	
	dy = yy2 - yy1;
	dx = xx2 - xx1;
	
	
	if (xx1 < 0) {
		yy1 = (int)(yy1 + (dy * ((-xx1)/dx)));
		xx1 = 0;
	}

	if (xx1 > (size_x-1))
		return;
		
	if (xx2 > (size_x-1)) {
		yy2 = (int)(yy2 - (dy * (xx2-(size_x-1))/dx));
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
		xx1 = (int)(xx1 + (dx * ((-yy1)/dy)));
		yy1 = 0;
	}

	if (yy1 > (size_y-5))
		return;
		
	if (yy2 > (size_y-5)) {
		xx2 = (int)(xx2 - (dx * (yy2-(size_y-5))/dy));
		yy2 = (size_y-5);
	}

	
	if (anti)
		anti_fline(xx1+1,yy1+1,xx2+1, yy2+1, mp);
	else
		no_anti_fline(xx1+1,yy1+1,xx2+1, yy2+1, mp);
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
	char 	*mp1;
	float	k;
	short	e0;
	
	if (y1 < 1 || y1 > (VS-2) || y2 < 1 || y2 > (VS-2))
		return;
	if (x1 < 0 || x1 > HS || x2 < 0 || x2 > HS)
		return;
	
	
	dx = x2-x1;
	dy = y2-y1;
	
	base = the_base + y1*rowbyte+x1;
	
	if (dx==0 && dy==0) {
		*base = *mp;						//was bug ?? *base = c;
		return;
	}
	
	mp1 = mp + 31;
	
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

				e0 = error>>16;
				*base = mp1[-e0];					//interleave as much as possible...
				cpt--; 
				error += dy;
				*(base+rowbyte) = mp[e0];

				base++;
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
				
				e0 = error>>16;
				*base =  mp1[-e0];
				cpt--;
				error += dx;
				*(base + 1) =  mp[e0];

				base += rowbyte;
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

				e0 = error>>16;
				*base = mp1[-e0];
				cpt--; 
				error += dy;
				*(base+rowbyte) = mp[e0];

				base--;
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

				e0 = error>>16;
				*(base - 1) =  mp[e0];
				cpt--;
				error += dx;
				*base =  mp1[-e0];

				base += rowbyte;
				if (error >= dy) {
					base--; 
					error -= dy;
				}
			}
		}
	}
}

//--------------------------------------------------------------

void	TSoundView::no_anti_fline(long x1,long y1,long x2, long y2, char *mp)
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
		*base = *mp;			//was bug *base = c;
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
				*base = mp[31];
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
				*base =  mp[31];
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
				*base = mp[31];
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
				*base =  mp[31];
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
	long	top, bottom;
	long	left, right;
	BRect	src, dst;
	
	bnd = Window()->Bounds();

	w = (int)(bnd.right - bnd.left);

	bm_hp = (int)(-(HS-w)/2.0);
	//if (sub_size) {
		//bm_hp -= 32;
	//}

	if (bm_hp > 0)
		bm_hp = 0;
	
	if (rr.top != 0 || rr.bottom != 0 || rr.left != 0 || rr.right != 0) {
		DrawBitmap(the_bits, BPoint(bm_hp,0));
		return;
	}

	
	if (o_min_y < min_y)
		top = o_min_y;
	else
		top = min_y;

	if (o_max_y > max_y)
		bottom = o_max_y;
	else
		bottom = max_y;

	top -= 4;
	bottom += 4;

	if (bottom > VS)
		bottom = VS;
	if (top < 0)
		top = 0;


	if (o_min_x < min_x)
		left = o_min_x;
	else
		left = min_x;

	if (o_max_x > max_x)
		right = o_max_x;
	else
		right = max_x;

	left -= 4;
	right += 4;

	if (right > HS)
		right = HS;
	if (left < 0)
		left = 0;


	src = BRect(left, top, right, bottom);
	dst = src;
	dst.OffsetBy(bm_hp, 0);
	DrawBitmap(the_bits, src, dst);
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

static void	putnum(long cnum, char *p, char *s)
{
	char	c;

	if (cnum > 99) cnum = 99;
	
	if (cnum/10 > 0) {
		*p++ = 0x30 + (cnum/10);
	}

	*p++ = 0x30 + (cnum%10);
	*p++ = '-';
	do {
		c = *p++ = *s++;
	} while(c);
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
	char		pname[256];	
	long		cnum;

	for (i = 0; i < 32; i++)
	for (j = 0; j < 8; j++)
		space[i][j] = 0;
		
	c.red = 160;				//was white
	c.green = 160;
	c.blue = 160;
	
	off_view->Window()->Lock();
	tmp_region.Set(clip_rect);
	if (no_name) {
		off_view->ConstrainClippingRegion(&tmp_region);
	}
	
	off_view->SetHighColor(pmap(c));
	off_view->SetDrawingMode(B_OP_OVER);			//was OP_BLEND
	
	for (i = 0; i < ppoint_count; i++) {
		if (poly_point[i].object_id > 1023) {
			pos_x[poly_point[i].object_id -1024] = poly_point[i].x1;
			pos_y[poly_point[i].object_id -1024] = poly_point[i].z1;
			
			x0 = (int)(poly_point[i].sx1 + (HS/2));
			y0 = (int)(poly_point[i].sy1 + off_y);
			
			xx = (int)(poly_point[i+1].sx1 + (HS/2));
			yy = (int)(poly_point[i+1].sy1 + off_y);
			
			ok = 0;
			
			cnum = poly_point[i].object_id - 1024;

			putnum(cnum, pname, chan[cnum]->cname);

			w = (int)StringWidth(pname);
			
			while(!ok) {
				again:;
				ok = 1;
				for (k = 0; k < cpt; k++) {
					dist_x = abs(prev_x[k] - xx);
					dist_y = abs(prev_y[k] - yy);
					dist_w = dist_x - abs(w/2 + prev_w[k]);
					if ((dist_w < 15) && (dist_y < 15)) {
						yy = yy - 2;
						ok = 0;
						goto again;
					}
				}
			}		
			
			prev_x[cpt] = xx;
			prev_y[cpt] = yy;
			prev_w[cpt] = w/2;
			
			fline(x0,y0,xx,yy, white_map, 0);
			cpt++;
		}
	}
	
	cpt = 0;
	for (i = 0; i < ppoint_count; i++) {
		if (poly_point[i].object_id > 1023) {
			x0 = (int)(poly_point[i].sx1 + (HS/2));
			y0 = (int)(poly_point[i].sy1 + off_y);
			
			
			xx = prev_x[cpt];
			yy = prev_y[cpt];
			w = prev_w[cpt];
			off_view->MovePenTo(BPoint(xx - w, yy-10));
			
			if ((yy - 25) < min_y) {
				min_y = yy-25;
			}
			if ((yy + 10) > max_y) {
				max_y = yy + 10;
			}

			if ((xx - w) < min_x)
				min_x = xx - w;
			if ((xx + w) > max_x)
				max_x = xx + w;

			if (obj_state[poly_point[i].object_id - 1024]) {
				c.red = 255;		
				c.green = 60;
				c.blue = 60;
			}
			else {
				c.red = 170;				//was white
				c.green = 170;
				c.blue = 160;
			}
			off_view->SetHighColor(c);
			
			cnum = poly_point[i].object_id - 1024;
			putnum(cnum, pname, chan[cnum]->cname);
			off_view->DrawString(pname);
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
	
		
	for (i = 0; i < vector_count; i++) {
		x1 = varray[i].sx1+(HS/2);
		y1 = varray[i].sy1+off_y;
		x2 = varray[i].sx2+(HS/2);
		y2 = varray[i].sy2+off_y;
	
		dx = fabs(x1 - x2);
		dy = fabs(y1 - y2);	
		
		if (varray[i].color == 200)
			fline(x1,y1,x2,y2, red_map, 1);
		else
		if (varray[i].color == 100)
			fline(x1,y1,x2,y2, green_map, 1);
		else
		if (varray[i].z1 != varray[i].z2) {
			fline(x1,y1,x2,y2, blue_map, 1);
		}
		else {
			p = (int)(varray[i].z1 + 256);
			p = p / 16;
			p+=4;
			if (p < 0)
				p = 0;
			if (p > 19) p = 19;
			p = 19;
			fline(
				x1,y1,x2,y2, level_map[p], 1
				);
		}
	}

	move_pixels();
	change_view_point_particule(a,-b,c);
	draw_particules();
	
	render_polys();
	acquire_sem(channel_list_sem);
	draw_labels();
	release_sem(channel_list_sem);
}

//--------------------------------------------------------------

void	TSoundView::draw_particules()
{
	short	i;
	long	x,y;
	uchar	*p;
	uchar	c;
	short	l_max_y;


	if (do_snow == 0)
		return;
		
	l_max_y = size_y - 2;

	for (i = 0; i < NP; i++) {
		x = (int)(pixels[i].sx) + w2;
		y = (int)(pixels[i].sy) + off_y;
		
		if (x < 0 || (x > size_x) || (y < 0) || (y > l_max_y))
			goto skip;
		

		p = (uchar *)the_base + (size_x * y) + x;
		*p = c = pixels[i].color;	

//interleave tests with memory access to get a better paralleism on smart CPUs.

		if (y < min_y)
			min_y = y;
		*(p + 1) = c;
		if (x > max_x)
			max_x = x;
		*(p - 1) = c;
		if (x < min_x)
			min_x = x;
		*(p + size_x) = c;
		if (y > max_y)
			max_y = y;
		*(p - size_x) = c;
		skip:;
	}
	
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
		trak->Select(hit, obj_state[hit]);
	}
	else {
		if (obj_state[hit] == 0) {
			for (i = 0; i < NC; i++) {
				if (obj_state[i]) {
					obj_state[i] = 0;
					SetChannelColor(i, -1);
					trak->Select(i, obj_state[i]);
				}
			}
			obj_state[hit] ^= 1;
			trak->Select(hit, obj_state[hit]);
		}
	}
	if (obj_state[hit])
		SetChannelColor(hit, 1);
	else
		SetChannelColor(hit, -1);
	
	
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
		
		for (obj = 0; obj < NC; obj++) {
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
	
		clear();
		draw_vectors();
		composit_pannel1();
		composit_pannel2();

		Draw(BRect(0,0,0,0));
		//DrawBitmap(the_bits, BPoint(bm_hp,0));
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
	float	vx0, vy0;

	float	sina = sin(-c);
	float	cosa = cos(-c);


	vx0 = *x;
	vy0 = *y;

	x0 = vx0 * cosa - vy0 * sina;
	y0 = vx0 * sina + vy0 * cosa;
	

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
		trak->Select(hit, obj_state[hit]);
	}
	else {
		if (obj_state[hit] == 0) {
			for (i = 0; i < NC; i++) {
				if (obj_state[i]) {
					obj_state[i] = 0;
					SetChannelColor(i, -1);
					trak->Select(i, obj_state[i]);
				}
			}
			obj_state[hit] ^= 1;
			trak->Select(hit, obj_state[hit]);
		}
	}
	if (obj_state[hit])
		SetChannelColor(hit, 1);
	else
		SetChannelColor(hit, -1);
	
	
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
			
		for (obj = 0; obj < NC; obj++) {
			if (obj_state[obj]) {
				px0 = pos_x[hit];
				py0 = pos_y[hit];
				
				px1 = px0;
				py1 = py0;
			
				MovePoly(obj, dx, 0, dy);
			}
		}
		
		recalc_pos_xy();
		
		
		for (obj = 0; obj < NC; obj++) {
			if (obj_state[obj]) {
				if (pos_x[obj] > 250) {
					for(m = 0; m < 64; m++)
						if (obj_state[m])
							MovePoly(m, -1 + 0.9*(250-pos_x[obj]), 0, 0);
				}
				if (pos_x[obj] < -250) {
					for(m = 0; m < NC; m++)
						if (obj_state[m])
							MovePoly(m, 1 +  0.9*((-(pos_x[obj]+250))), 0, 0);
				}
			
				if (pos_y[obj] > 250) {
					for(m = 0; m < NC; m++)
						if (obj_state[m])
							MovePoly(m, 0,0, -1 + 0.9*(250-pos_y[obj]));
				}
				if (pos_y[obj] < -250) {
					for(m = 0; m < NC; m++)
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
	
		clear();
		draw_vectors();
		composit_pannel1();
		composit_pannel2();
		in_move++;
		Window()->Unlock();
		Window()->Lock();
		in_move--;
		Draw(BRect(0,0,0,0));
		//DrawBitmap(the_bits, BPoint(bm_hp,0));
		snooze(0*20000);
	} while(buttons);
}	

//--------------------------------------------------------------

void	TSoundView::move_selection(float dx, float dy, char draw)
{
	long	i;
	float	px0,py0,px1,py1;
	long	obj;
	long	m;
	
	
		
	for (obj = 0; obj < NC; obj++) {
		if (obj_state[obj]) {
			MovePoly(obj, dx, 0, dy);
		}
	}
	
	recalc_pos_xy();
	
	
	for (obj = 0; obj < NC; obj++) {
		if (obj_state[obj]) {
			if (pos_x[obj] > 250) {
				for(m = 0; m < NC; m++)
					if (obj_state[m])
						MovePoly(m, -1 + 0.9*(250-pos_x[obj]), 0, 0);
			}
			if (pos_x[obj] < -250) {
				for(m = 0; m < NC; m++)
					if (obj_state[m])
						MovePoly(m, 1 +  0.9*((-(pos_x[obj]+250))), 0, 0);
			}
		
			if (pos_y[obj] > 250) {
				for(m = 0; m < NC; m++)
					if (obj_state[m])
						MovePoly(m, 0,0, -1 + 0.9*(250-pos_y[obj]));
			}
			if (pos_y[obj] < -250) {
				for(m = 0; m < NC; m++)
					if (obj_state[m])
						MovePoly(m, 0, 0, 1 + 0.9*(-(pos_y[obj]+250)));
			}
		}
	}
	


		
	if (draw) {
		change_view_point(a,-b,c);

		clear();
		draw_vectors();
		update_infos();
		composit_pannel1();
		composit_pannel2();
	
		Draw(BRect(0,0,0,0));
	}
}	

//--------------------------------------------------------------

void	TSoundView::SetChannelLevel(long c, float v)
{
	level[c] = v;
	if (obj_muted[c] || obj_silent[c]) {
		SizePoly(c, 'ttop',-3);
		level[c] = 0;
	}
	else
		SizePoly(c, 'ttop', -3 + v*40.0);
}

//--------------------------------------------------------------

void	TSoundView::SetChannelColor(long ch, long color)
{
	long	i;
	long	c;

	if (obj_hilite[ch] == color)
		return;

	obj_hilite[ch] = color;

	for (i=  0; i < poly_count; i++) {
		if (polys[i].object_id == ch) {
			c = polys[i].color_index;
			c += color;
			polys[i].color_index = c;
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
			*x1 = (int)x;
		if (x > *x2)
			*x2 = (int)x;
		if (y < *y1)
			*y1 = (int)y;
		if (y > *y2)
			*y2 = (int)y;
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

long	vv = 1000;

//--------------------------------------------------------------

void	TSoundView::recycle_pixel(long i)
{
	long	nn;
	long	max;
	
	max = 5;
	
	if (NC == 0) {
		pixels[i].y = -80;
		pixels[i].x = 0 + rand()%8;
		pixels[i].z = 0 + rand()%8;
		pixels[i].dx = ((rand()%1000) - 500) / 800.0;
		vv += 10;
		if (vv > 25000) vv = 1000;
		pixels[i].dy = - (rand()%vv)/1800.0;
		pixels[i].dz = ((rand()%(vv/10)) - 500) / 800.0;
		pixels[i].color = rand();

		return;
	}


	while(max > 0) {	
		nn = rand() % NC;
		max--;
		if (level[nn] < -0.2)
			goto out;
	}
v0:;

	for (nn = 0; nn < NC; nn++) {
		if (level[nn] < -0.1)
			goto out;
	}

	nn = rand() % NC;
out:;
	

	pixels[i].y = -80;
	pixels[i].x = pos_x[nn] - 4 + rand()%8;
	pixels[i].z = pos_y[nn] - 4 + rand()%8;
	pixels[i].dx = ((rand()%1000) - 500) / 800.0;
	pixels[i].dy = - (rand()%1000)/1800.0 - (fabs(level[nn]* level[nn]))*5.0;
	pixels[i].dy *= 0.55;
	pixels[i].dz = ((rand()%1000) - 500) / 800.0;
	pixels[i].color = rand();
}

//--------------------------------------------------------------

void	TSoundView::move_pixels()
{
	long	i;

	if (do_snow == 0)
		return;

	recalc_pos_xy();
	for (i = 0; i < NP; i++) {
		pixels[i].x += pixels[i].dx;
		pixels[i].y += pixels[i].dy;
		pixels[i].z += pixels[i].dz;
		pixels[i].dy += 0.1211;
		if (pixels[i].y > -4) {
			pixels[i].dy = -pixels[i].dy * 0.4;
			if (fabs(pixels[i].dy) < 2.7) {
				recycle_pixel(i);
			}
		}
	}
}

//--------------------------------------------------------------

void	TSoundView::Animate()
{
	long	x0,y0,x1,y1;
	BRect	r;
	BRect	bnd;
	
			
	phase++;
	if (Window()->Lock()) {
		v_meter((int)level2, (int)level1);
	
		if (!slow_anim || ((phase & 3) == 0)) {
			set_cur_time(mtime);
		}

		if (slow_anim) {
			goto skip;
		}

		if (in_move)
			goto skip;
		bnd = Window()->Bounds();
		bnd.bottom -= bnd.top;
		bnd.right -= bnd.left;

		pannel1_rect.top = 0;
		pannel1_rect.bottom = p1_y;
		pannel1_rect.left = bnd.right - p1_x - bm_hp;
		pannel1_rect.right = bnd.right - bm_hp;

		if (demo_mode) {
			b += 0.01;
			c += 0.0064;
			need_vector = 1;
		}

		if (jump_mode) {
			b -= db;
			c -= dc;
			need_vector = 1;
			db *= 0.98;
			dc *= 0.98;
			if (fabs(db) < 0.004 && fabs(dc) < 0.004)
				jump_mode = 0;
		}

		change_view_point(a,-b,c);
		
		
		clear();
		draw_vectors();
		UpdateTime(pan2_view);
		composit_pannel1();
		composit_pannel2();
		
float	s,e;

		Draw(BRect(0,0,0,0));
		//DrawBitmap(the_bits, BPoint(bm_hp,0));
skip:;	
		Window()->Unlock();
	}
}

//--------------------------------------------------------------

void	TSoundView::Animate(BRect br)
{
	long	x0,y0,x1,y1;
	BRect	r;
	BRect	r1;
	
	if (Window()->Lock()) {
		change_view_point(a,-b,c);
		
		clear();
		draw_vectors();
		UpdateTime(pan2_view);
		composit_pannel1();
		composit_pannel2();
		
		r1 = br;
		br.left -= bm_hp;
		br.right -= bm_hp;
		DrawBitmap(the_bits, br,r1);
	
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

		dx = (int)(where0.x - where.x);
		dy = (int)(where0.y - where.y);
		
		
		if (dx == 0 && dy == 0) {
			snooze(35000);
		}
		
		where = where0;
		pannel2_rect.top += dy;
		pannel2_rect.bottom += dy;
		pannel2_rect.left += dx;
		pannel2_rect.right += dx;
		
		Animate();
		snooze(15000);
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

		dx = (int)(where0.x - where.x);
		dy = (int)(where0.y - where.y);
		
		
		if (dx == 0 && dy == 0) {
			snooze(35000);
		}
		
		where = where0;
		pannel1_rect.top += dy;
		pannel1_rect.bottom += dy;
		pannel1_rect.left += dx;
		pannel1_rect.right += dx;
		
		Animate();
		snooze(15000);
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
	float	c0, b0;
	char	no_jump;

	MakeFocus();
	where0 = where;
	where0.x -= bm_hp;
	if (pannel1_rect.Contains(where0) && has_selection()) {
		track_pannel1();
		return;
	}
	
	
	hit = find_hit(where);
	
	if (hit == DRAG_TAG)				//ignore any hit on the little drag and drop gizmo !
		hit = -1;

	phase = 0;
	
	if (hit >= 0) {
		Draw1(pan1_view, hit);

		handle_hit(hit, where);
	}
	else {
		deselect_all();
		do {
			in_move = 1;
			GetMouse(&where0, &buttons);
			
			if ((where0.x != where.x) || (where0.y != where.y))
				moved = 1;
				
			if (moved) {
				phase++;
				c0 = c;
				b0 = b;
				c = c - (where0.x - where.x)/500.0;
				b = b + (where0.y - where.y)/1000.0;
				no_jump = 0;
				//if (b > 1.3) b = 1.3;				//no limits
				//if (b < 0.03) b = 0.03;
				//off_y = BVP + - b*125.0;
				off_y = BVP;					// no move
				where = where0;
				need_sort = 1;
			}
			else
				no_jump = 1;

			need_vector = 1;
			change_view_point(a,-b,c);
			clear();
			draw_vectors();
			composit_pannel1();
			composit_pannel2();
			
			Draw(BRect(0,0,0,0));

			//Window()->Unlock();
			if (!moved)
				snooze(45000);
			else
				snooze(15000);
			//Window()->Lock();
			moved = 0;
		} while(buttons);
		dc = c0 - c;
		db = b0 - b;
		jump_mode = 1;
		if (no_jump)
			jump_mode = 0;
	}
	in_move = 0;
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
		x0 = x * cosa - y * sina;
		y0 = x * sina + y * cosa;
		x = x0;y = y0;
		x0 = x * cosc - z * sinc;
		z0 = x * sinc + z * cosc;
		x = x0;z = z0;
		y0 = y * cosb - z * sinb;
		z0 = y * sinb + z * cosb;

		y = y0;y = y0;z = z0;
		dx = (x0 + vpx);
		dy = (y0 + vpy);
		dz = (z0 + vpz);
		dist = b_sqrt(dz*dz);
		dist = dst/dist;
		varray[i].sx1 = x*dist;
		varray[i].sy1 = y*dist;
		varray[i].sz1 = z;
		
		x = varray[i].x2;
		y = varray[i].y2;
		z = varray[i].z2;
		x0 = x * cosa - y * sina;
		y0 = x * sina + y * cosa;
		x = x0;y = y0;
		x0 = x * cosc - z * sinc;
		z0 = x * sinc + z * cosc;
		x = x0;z = z0;
		y0 = y * cosb - z * sinb;
		z0 = y * sinb + z * cosb;
		y = y0;y = y0;z = z0;
		dx = (x0 + vpx);
		dy = (y0 + vpy);
		dz = (z0 + vpz);
		dist = b_sqrt(dz*dz);
		dist = dst/dist;
		varray[i].sx2 = x*dist;
		varray[i].sy2 = y*dist;
		varray[i].sz2 = z;

	}
	change_view_point_poly(new_alpha, new_delta, new_zeta);

}

//--------------------------------------------------------------

void	TSoundView::change_view_point_particule(float new_alpha, float new_delta, float new_zeta)
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
	float	dist;
	
	float	dx,dy,dz;
	
	if (do_snow == 0)
		return;

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

	for (i = 0; i < NP; i++) {
		x = pixels[i].x;
		y = pixels[i].y;
		z = pixels[i].z;
		x0 = x * cosa - y * sina;
		y0 = x * sina + y * cosa;
		x = x0;y = y0;
		x0 = x * cosc - z * sinc;
		z0 = x * sinc + z * cosc;
		x = x0;z = z0;
		y0 = y * cosb - z * sinb;
		z0 = y * sinb + z * cosb;
		y = y0;y = y0;z = z0;
		dx = (x0 + vpx);
		dy = (y0 + vpy);
		dz = (z0 + vpz);
		dist = b_sqrt(dz*dz);
		dist = dst/dist;
		pixels[i].sx = x * dist;
		pixels[i].sy = y * dist;
	}
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
			poly_point[i].sz1 = z;
		}
	}
}

//--------------------------------------------------------------


void	hline(uchar *base, long count, uchar *mapper)
{
	ulong	v;
	ulong	v0;

	v0 = mapper[0];
	v0 = v0 | (v0<<8);
	v0 = v0 | (v0<<16);

	while(count >= 8) {

		v = *((ulong *)base);
		if (v == 0) {
			*((ulong *)base) = v0;
			base += 4;
		}
		else {
			*base++ = mapper[v>>24];
			*base++ = mapper[(uchar)(v>>16)];
			*base++ = mapper[(uchar)(v>>8)];
			*base++ = mapper[(uchar)v];
		}

		v = *((ulong *)base);

		if (v == 0) {
			*((ulong *)base) = v0;
			base += 4;
		}
		else {
			*base++ = mapper[v>>24];
			*base++ = mapper[(uchar)(v>>16)];
			*base++ = mapper[(uchar)(v>>8)];
			*base++ = mapper[(uchar)v];
		}


		count -= 8;
	}



	while(count-- >= 0) {
		*base = mapper[*base];
		base++;
	}
}

//--------------------------------------------------------------


//--------------------------------------------------------------

long	poly_scratch[4800];

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
    
    

	xpoint[4] = xpoint[0] = (int)pt_a.x;
	ypoint[4] = ypoint[0] = (int)pt_a.y;
	xpoint[1] = (int)pt_b.x;
	ypoint[1] = (int)pt_b.y;
	xpoint[2] = (int)pt_c.x;
	ypoint[2] = (int)pt_c.y;
	xpoint[3] = (int)pt_d.x;
	ypoint[3] = (int)pt_d.y;
	
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

	if (top < min_y)
		min_y = top;
	if (bottom > max_y)
		max_y = bottom;


	for (i = top; i <= bottom; i++) {
		if (i > size_y)
			return;

short	left, right;

		left = *tmp0;
		if (left < 0)
			left = 0;

		right = *tmp1;

		if (right > size_x)
			right = size_x;

		if (left < min_x) min_x = left;
		if (right > max_x) max_x = right;

		if ((i >= 0) && (i < (size_y - 1))) 
			hline((uchar *)(ptr + left), right - left, (uchar *)mapper);

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

	xpoint[4] = xpoint[0] = (int)pt_a.x;
	ypoint[4] = ypoint[0] = (int)pt_a.y;
	xpoint[1] = (int)pt_b.x;
	ypoint[1] = (int)pt_b.y;
	xpoint[2] = (int)pt_c.x;
	ypoint[2] = (int)pt_c.y;
	xpoint[3] = (int)pt_d.x;
	ypoint[3] = (int)pt_d.y;
	
	bottom = -10000;
	top = 10000;

	for (i = 0; i < 4; i++) {
		if (ypoint[i] > bottom)
			bottom = ypoint[i];
		if (ypoint[i] < top)
			top = ypoint[i];
	}
	if (top > size_y)
		return 0;
		
	if (bottom < 0)
		return 0;
	
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
	float	temp;
	poly	tmp;
	float	zs[2048];
	
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
				temp = zs[j];
				zs[j] = zs[j + step];
				zs[j+step] = temp;
			}
		}
	}
}

//-----------------------------------------------------------

float	global_gain = 3.0;

//-----------------------------------------------------------

extern	float	get_global_gain();

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

void	TSoundView::SetMuteSolo(Channel *c, char mute, char solo)
{
	long	i;
	long	n;
	float	k;
	char	any_solo;

	for (i = 0; i < NC; i++) {
		if (chan[i] == c) {
			if (obj_muted[i] != mute) {
				if (mute)
					k = 0.68;
				else
					k = 1.0/0.68;
				for (n = 0; n < 3; n++) {
					ScalePoly(i, 'itop', k);
					ScalePoly(i+1024, 'itop', k);
				}
			}

			obj_muted[i] = mute;
			obj_solo[i] = solo;
		}
	}

	any_solo = 0;
	for (i = 0; i < NC; i++) 
		if (obj_solo[i])
			any_solo = 1;

	if (any_solo == 0) {
		for (i = 0; i < NC; i++) {
			if (obj_silent[i]) {
				obj_silent[i] = 0;
				for (n = 0; n < 3; n++) {
					ScalePoly(i, 'itop', 1.0/0.68);
					ScalePoly(i+1024, 'itop', 1.0/0.68);
				}
			}
		}
	}
	else {
		for (i = 0; i < NC; i++) {
			if (obj_silent[i] == 1 && obj_solo[i] == 1) {
				obj_silent[i] = 0;
				for (n = 0; n < 3; n++) {
					ScalePoly(i, 'itop', 1.0/0.68);
					ScalePoly(i+1024, 'itop', 1.0/0.68);
				}
			}

			if (obj_silent[i] == 0 && (obj_solo[i] == 0)) {
				obj_silent[i] = 1;
				for (n = 0; n < 3; n++) {
					ScalePoly(i, 'itop', 0.68);
					ScalePoly(i+1024, 'itop', 0.68);
				}
			}
		}
	}
}


//-----------------------------------------------------------

void	TSoundView::GetMuteSolo(Channel *c, char *mute, char *solo)
{
	long	i;

	*mute = 0;
	*solo = 0;

	for (i = 0; i < NC; i++) {
		if (chan[i] == c) {
			*mute = obj_muted[i];
			*solo = obj_solo[i];
			return;
		}
	}
}

//-----------------------------------------------------------

void	TSoundView::GetChannelPos(Channel *c, float *x, float *y)
{
	long	i;

	*x = 0;
	*y = 0;

	for (i = 0; i < NC; i++) {
		if (chan[i] == c) {
			*x = pos_x[i];
			*y = pos_y[i];
			return;
		}
	}
}

//-----------------------------------------------------------
#define	ACTIVE(i) (obj_muted[i] == 0 && obj_silent[i] == 0 && p[i]->got_data)
//-----------------------------------------------------------
long	sat = 0;
//-----------------------------------------------------------
double	mtot = 0;
long	mcnt = 0;
//-----------------------------------------------------------

void	TSoundView::bi_mix(Channel **p, long n, long step)
{
	long	i;
	long	j;
	long	k;
	float	sum1;
	float	sum2;
	float	sum3;
	float	sum4;

	short	*bufs[32];
	float	lgains[32];
	float	rgains[32];
	char	act[32];
	short	act_idx[32];
	short	delay_left[32];
	short	delay_right[32];
	short	*out;
	long	tmp;
	long	tmp1;
	float	gg;	
	long	cnt;
	long	l1, l2;
	double	t0,t1;

	global_gain = get_global_gain();
	cnt = 0;

	for (i = 0;i < n; i++) {
		act[i] = ACTIVE(i);
		if (act[i]) {
			act_idx[cnt] = i;
			cnt++;
		}
	}

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
		bufs[i] = (short *)(p[i]->GetBuffer() + (step * PLAY_SAMPLE_COUNT));
	}
	
	gg = global_gain/n;
	acquire_sem(buffer_sem);
	


	out = glb_buffer;

	l1 = 0;
	l2 = 0;

	for (i = 0; i < PLAY_SAMPLE_COUNT; i+=2) {
		
		if ((i & 0xff) == 0) {
			upd_revs(n, p);
			for (k = 0; k < n; k++) {
				lgains[k] = p[k]->GetGain() * lrev[k];
				rgains[k] = p[k]->GetGain() * rrev[k];
				delay_left[k] = (int)((pos_x[k]) / 3.5);
				
				if (delay_left[k] < 0) {
					delay_right[k] = -delay_left[k];
					delay_left[k] = 0;
				}
				else 
					delay_right[k] = 0;
			}
		}

		sum1 = 0;
		sum2 = 0;
		sum3 = 0;
		sum4 = 0;

		for (j = 0; j < cnt; j++) {
				k = act_idx[j];

				tmp = *(bufs[k] + i - delay_right[k]);
				if (tmp) {
					sum1 += tmp * lgains[k];
				}
				tmp = *(bufs[k] + i - delay_right[k] + 1);
				if (tmp) {
					sum3 += tmp * lgains[k];
				}


				tmp = *(bufs[k] + i - delay_left[k]);
				if (tmp) {
					sum2 += tmp * rgains[k];
				}
				tmp = *(bufs[k] + i - delay_left[k] + 1);
				if (tmp) {
					sum4 += tmp * rgains[k];
				}
		}
			

		sum2 *= gg;
		if (sum2 > 32766) {sum2 = 32766;}
		if (sum2 < -32766) {sum2 = -32766;}
		*out++ = (int)sum2;
		sum1 *= gg;
		if (sum1 > 32766) {sum1 = 32766;}
		if (sum1 < -32766) {sum1 = -32766;}
		*out++ = (int)sum1;
		sum4 *= gg;
		if (sum4 > 32766) {sum4 = 32766;}
		if (sum4 < -32766) {sum4 = -32766;}
		*out++ = (int)sum4;
		sum3 *= gg;
		if (sum3 > 32766) {sum3 = 32766;}
		if (sum3 < -32766) {sum3 = -32766;}
		*out++ = (int)sum3;

		l1 += abs((int)sum1);
		l2 += abs((int)sum2);


	}

	l1 /= (PLAY_SAMPLE_COUNT/2);
	l2 /= (PLAY_SAMPLE_COUNT/2);

	level1 = l1;
	level2 = l2;


	release_sem(data_sem);
	if (quit > 0) {
		quit--;
		return;
	}
}

//-----------------------------------------------------------

void	TSoundView::w_mix(Channel **p, long n, long step)
{
	long	i;
	long	j;
	long	k;
	float	sum1;
	float	sum2;
	float	sum3;
	float	sum4;

	short	*bufs[32];
	float	lgains[32];
	float	rgains[32];
	char	act[32];
	short	act_idx[32];
	short	delay_left[32];
	short	delay_right[32];
	short	*out;
	long	tmp;
	long	tmp1;
	float	gg;	
	long	cnt;
	long	l1, l2;
	double	t0,t1;

	global_gain = get_global_gain();
	cnt = 0;

	for (i = 0;i < n; i++) {
		act[i] = ACTIVE(i);
		if (act[i]) {
			act_idx[cnt] = i;
			cnt++;
		}
	}

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
		bufs[i] = (short *)(p[i]->GetBuffer() + (step * PLAY_SAMPLE_COUNT));
	}
	
	gg = global_gain/n;
	acquire_sem(buffer_sem);
	


	
	out = glb_buffer;

	l1 = 0;
	l2 = 0;

	for (i = 0; i < PLAY_SAMPLE_COUNT; i+=2) {
		
		if ((i & 0xff) == 0) {
			upd_revs(n, p);
			for (k = 0; k < n; k++) {
				lgains[k] = p[k]->GetGain() * lrev[k];
				rgains[k] = p[k]->GetGain() * rrev[k];
				delay_left[k] = (int)((pos_x[k]) / 3.5);
				
				if (delay_left[k] < 0) {
					delay_right[k] = -delay_left[k];
					delay_left[k] = 0;
				}
				else 
					delay_right[k] = 0;
			}
		}

		sum1 = 0;
		sum2 = 0;
		sum3 = 0;
		sum4 = 0;

		for (j = 0; j < cnt; j++) {
				k = act_idx[j];

				tmp = *(bufs[k] + i - delay_right[k]);
				if (tmp) {
					sum1 += tmp * lgains[k];
				}
				tmp = *(bufs[k] + i - delay_right[k] + 1);
				if (tmp) {
					sum3 += tmp * lgains[k];
				}


				tmp = *(bufs[k] + i - delay_left[k]);
				if (tmp) {
					sum2 += tmp * rgains[k];
				}
				tmp = *(bufs[k] + i - delay_left[k] + 1);
				if (tmp) {
					sum4 += tmp * rgains[k];
				}
		}
			

		sum2 *= gg;
		if (sum2 > 32766) {sum2 = 32766;}
		if (sum2 < -32766) {sum2 = -32766;}
		*out++ = (int)sum2;
		sum1 *= gg;
		if (sum1 > 32766) {sum1 = 32766;}
		if (sum1 < -32766) {sum1 = -32766;}
		*out++ = (int)sum1;
		sum4 *= gg;
		if (sum4 > 32766) {sum4 = 32766;}
		if (sum4 < -32766) {sum4 = -32766;}
		*out++ = (int)sum4;
		sum3 *= gg;
		if (sum3 > 32766) {sum3 = 32766;}
		if (sum3 < -32766) {sum3 = -32766;}
		*out++ = (int)sum3;

		l1 += abs((int)sum1);
		l2 += abs((int)sum2);


	}

	l1 /= (PLAY_SAMPLE_COUNT/2);
	l2 /= (PLAY_SAMPLE_COUNT/2);

	level1 = l1;
	level2 = l2;

	if (output) {
#if defined (__POWERPC__) || defined (__ARMEB__)	/* FIXME: This should probably use <endian.h> for the right define */
		ushort	v;
		ushort	w;
		ushort	*p;

		p = (ushort *)glb_buffer;
		for (i = 0; i < PLAY_SAMPLE_COUNT; i++) {
			v = *p;
			w = (v>>8) | (v&0xff)<<8;
			*p++ = w;
			v = *p;
			w = (v>>8) | (v&0xff)<<8;
			*p++ = w;
		}
#endif

		output->WriteFrames((char *)glb_buffer, PLAY_SAMPLE_COUNT);
		
#if defined(__POWERPC__) || defined(__ARMEB__)	/* FIXME: This should probably use <endian.h> for the right define */
		p = (ushort *)glb_buffer;

		for (i = 0; i < PLAY_SAMPLE_COUNT; i++) {
			v = *p;
			w = (v>>8) | (v&0xff)<<8;
			*p++ = w;
			v = *p;
			w = (v>>8) | (v&0xff)<<8;
			*p++ = w;
		}
#endif
	}

	release_sem(data_sem);
	if (quit > 0) {
		quit--;
		return;
	}
}


//-----------------------------------------------------------

void	TSoundView::mix(Channel **p, long n, long step)
{
/*
	long	i;
	long	j;
	long	k;
	float	sum1;
	float	sum2;

	short	*bufs[32];
	float	lgains[32];
	float	rgains[32];
	char	act[32];
	short	act_idx[32];
	short	*out;
	long	tmp;
	long	tmp1;
	float	gg;	
	long	cnt;
	float	l1, l2;
*/

	bi_mix(p, n, step);

	return;

/*
	if (modifiers() & B_SHIFT_KEY) {
			return;
	}

	global_gain = get_global_gain();

	cnt = 0;

	for (i = 0;i < n; i++) {
		act[i] = ACTIVE(i);
		if (act[i]) {
			act_idx[cnt] = i;
			cnt++;
		}
	}

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
		bufs[i] = (short *)(p[i]->GetBuffer() + (step * PLAY_SAMPLE_COUNT));
	}
	
	gg = global_gain/n;
	acquire_sem(buffer_sem);
	
	out = glb_buffer;
	sat = sat * 0.84;

	l1 = level1;
	l2 = level2;

	for (i = 0; i < PLAY_SAMPLE_COUNT; i++) {

		if ((i % 400) == 0) {
			upd_revs(n, p);
			for (k = 0; k < n; k++) {
				lgains[k] = p[k]->GetGain() * lrev[k];
				rgains[k] = p[k]->GetGain() * rrev[k];
			}
		}

		sum1 = 0;
		sum2 = 0;

		for (j = 0; j < cnt; j++) {
				short	*pt0;
				short	*pt1;
				
				k = act_idx[j];

				tmp = *(bufs[k] + i);
				if (tmp) {
					sum1 += tmp * lgains[k];
					sum2 += tmp * rgains[k];
				}
		}
			

		sum2 *= gg;
		if (sum2 > 32766) {sat++;sum2 = 32766;}
		if (sum2 < -32766) {sat++;sum2 = -32766;}
		*out++ = sum2;
		sum1 *= gg;
		if (sum1 > 32766) {sat++;sum1 = 32766;}
		if (sum1 < -32766) {sat++;sum1 = -32766;}
		*out++ = sum1;

		if (i & 3 == 0) {
			if (sum2 > l1) {
				l1 = (l1 + sum2) / 2;
			}
			else {
				l1 *= 0.999;
			}

			if (sum1 > l2) {
				l2 = (l2 + sum1) / 2;
			}
			else {
				l2 *= 0.999;
			}
		}
	}


	level1 = l1;
	level2 = l2;


	if (sat > 4) {
		global_gain *= 0.99;
	}
	else {
		global_gain *= 1.01;
		if (global_gain > 3.0) global_gain = 3.0;
	}

	release_sem(data_sem);
	if (quit > 0) {
		quit--;
		return;
	}
*/
}

//-----------------------------------------------------------

long player_task(void *p)
{
	init_stuff();
	return 0;
}

//-----------------------------------------------------------

long	normalize(float v)
{
	long	tmp;
	
	tmp = (int)v;
	
	return tmp;
}

//-----------------------------------------------------------

long	TSoundView::final_mix()
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
	

	release_sem(ready_sem);
	mtime = get_start_time();
	dtime = max_time / 2.0;
	
	while(1) {	
		acquire_sem(channel_list_sem);
		for (j = 0; j < NC; j++) {
			chan[j]->SetPos(normalize((mtime) * 44100.0));
		}
		release_sem(channel_list_sem);
		for (j = 0; j < MIX_STEP; j++) {
			if (quit > 0) {
				quit--;
				return 0;
			}
			acquire_sem(channel_list_sem);
			for (k = 0; k < NC; k++) {
				acc = 0;
				buf = (short *)(chan[k]->GetBuffer() + (j * PLAY_SAMPLE_COUNT));
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
			w_mix(chan, NC, j);
			mtime = mtime + ts;
			if (mtime > get_end_time()) {
				release_sem(channel_list_sem);
				goto outa;
			}
			release_sem(channel_list_sem);
		}
		
outa:;
		if (mtime > get_end_time()) {
			goto out;
			mtime = get_start_time();
		}
	}
out:;
	f_final_mix = 0;
	delete output;
	output = 0;

	return 0;
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
	//resume_thread(spawn_thread(player_task,"player",B_URGENT_DISPLAY_PRIORITY,0));
	resume_thread(spawn_thread(player_task,"player",B_REAL_TIME_PRIORITY,0));
	

	release_sem(ready_sem);
	mtime = get_start_time();
	dtime = max_time / 2.0;
	
	while(1) {	

		if (pause_main == 3) {
			pause_main = 4;
			while(pause_main == 4) {
				if (f_final_mix) {
					final_mix();
				}
				snooze(80000);
			}
		}

		while ((NC == 0) && (about_producer == 0)) {
			snooze(80000);
			if (f_final_mix) {
				final_mix();
			}
		}

		if (f_final_mix) {
			final_mix();
		}

		acquire_sem(channel_list_sem);
		for (j = 0; j < NC; j++) {
			chan[j]->SetPos(normalize((mtime) * 44100.0));
		}
		release_sem(channel_list_sem);
		for (j = 0; j < MIX_STEP; j++) {
			if (quit > 0) {
				quit--;
				return 0;
			}
			acquire_sem(channel_list_sem);
			for (k = 0; k < NC; k++) {
				acc = 0;
				buf = (short *)(chan[k]->GetBuffer() + (j * PLAY_SAMPLE_COUNT));
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
			if (mtime > get_end_time()) {
				release_sem(channel_list_sem);
				goto outa;
			}
			release_sem(channel_list_sem);
		}
		if (pause_main == 2)
			pause_main = 0;
		
outa:;
		if (mtime > get_end_time()) {
			mtime = get_start_time();
		}
		if (mtime < get_start_time()) {
			mtime = get_start_time();
		}
	}

long	i;
	
	for (i = 0; i < NC; i++) 
		delete chan[i];

	return 0;
}

//-----------------------------------------------------------

long	init_p(void *p)
{
	TSoundView	*x;
	
	x = (TSoundView*)p;
	
	x->window_control_task();
	return 0;
}


//-----------------------------------------------------------

void	TSoundView::do_anim()
{
	float	t0;
	float	s,e;
	BPoint	w;
	ulong	b;

	snooze(1500000);
	Animate();
	while(1) {


		if (tag_x != -10000) {
			if (Window()->Lock()) {
				GetMouse(&w,&b,false);
				Window()->Unlock();
			}
			if (b == 0) {
				float	dx,dy;

				dx = -100000 - tag_x;
				dy = 0 - tag_y;

				tag_x = -100000;
				tag_y = 0;	
				MovePoly(DRAG_TAG, dx, 0, dy);
			}	
		}

		if (quit > 0) {
			quit--;
			return;
		}
		
		time_mode = TOP_ONLY;
		Animate();

		if (do_snow) {
			snooze(1*5000);
		}
		else
		if (demo_mode || jump_mode)
			snooze(1*10000);
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
	
	for (obj = 0; obj < NC; obj++)
		marked[obj] = 0;
		
	for (obj = 0; obj < NC; obj++) {
		if (obj_state[obj]) {
			if (obj_muted[obj] == 0) {
				all_select_muted = 0;
				goto out;
			}
		}
	}
	
out:;

	if (all_select_muted) {
		for (obj = 0; obj < NC; obj++) {
			if (obj_state[obj]) {
				obj_muted[obj] = 0;
				marked[obj] = 1;
			}
		}
		for (n = 0; n < 3; n++) {
			for (obj = 0; obj < NC; obj++) {
				if (marked[obj]) {
					ScalePoly(obj, 'itop', 1.0/0.68);
					ScalePoly(obj+1024, 'itop', 1.0/0.68);
				}
			}
			Animate();
		}
	}
	else {
		for (obj = 0; obj < NC; obj++) {
			if (obj_state[obj]) {
				if (obj_muted[obj] == 0) {
					marked[obj] = 1;
					obj_muted[obj] = 1;
				}
			}
		}
		for (n = 0; n < 3; n++) {
			for (obj = 0; obj < NC; obj++) {
				if (marked[obj]) {
					ScalePoly(obj, 'itop', 0.68);
					ScalePoly(obj+1024, 'itop', 0.68);
				}
			}
			Animate();
		}
	}
	trak->redo_mute_solo();
}

//-----------------------------------------------------------

void	TSoundView::toggle_mute(long c)
{
	long	i;
	float	k;

	if(c >= NC) return;

	if (obj_muted[c])
		k = 1.0/0.68;
	else
		k = 0.68;

	for (i = 0; i < 3; i++) {
		ScalePoly(c, 'itop', k);
		ScalePoly(c+1024, 'itop', k);
	}

	obj_muted[c] ^= 1;
}


//-----------------------------------------------------------

void	TSoundView::deselect_all()
{
	long	i;
	
	for (i = 0; i < NC; i++) {
		if (obj_state[i]) {
			obj_state[i] = 0;
			SetChannelColor(i, -1);
		}
	}
	for (i = 0; i < NC; i++) {
		trak->Select(i, obj_state[i]);
	}
	update_infos();
}

//-----------------------------------------------------------

void	TSoundView::select(Channel *c, char yesno)
{
	long	i;

	for (i = 0; i < NC; i++) {
			if (chan[i] == c) {
				obj_state[i] = yesno;
				if (yesno)
					SetChannelColor(i, 1);
				else
					SetChannelColor(i, -1);
			}
	}
}

//-----------------------------------------------------------

void	TSoundView::select_all()
{
	long	i;
	
	for (i = 0; i < NC; i++) {
		obj_state[i] = 1;
		SetChannelColor(i, 1);
	}
	Animate();
	for (i = 0; i < NC; i++) {
		trak->Select(i, obj_state[i]);
	}
}

//-----------------------------------------------------------

void	TSoundView::solo_selection()
{
	long	obj;
	long	i;
	char	old;
	char	has_solo;
	long	n;
	char	solo_all = 0;
	char	unsolo_all= 0;


	for (obj = 0; obj < NC; obj++) {
		if (obj_state[obj]) {
			if (obj_solo[obj] == 0) {
				solo_all = 1;
				goto out;
			}
			else {
				solo_all = 0;
				goto out;
			}
		}
	}
out:;
	
	for (obj = 0; obj < NC; obj++) {
		if (obj_state[obj]) {
			if (obj_solo[obj] != solo_all) {
				obj_solo[obj] = solo_all;
			}
		}
	}


done:
	char	any_solo = 0;

	for (i = 0; i < NC; i++) 
		if (obj_solo[i])
			any_solo = 1;

	if (any_solo == 0) {
		for (i = 0; i < NC; i++) {
			if (obj_silent[i]) {
				obj_silent[i] = 0;
				for (n = 0; n < 3; n++) {
					ScalePoly(i, 'itop', 1.0/0.68);
					ScalePoly(i+1024, 'itop', 1.0/0.68);
				}
			}
		}
	}
	else {
		for (i = 0; i < NC; i++) {
			if (obj_silent[i] == 1 && obj_solo[i] == 1) {
				obj_silent[i] = 0;
				for (n = 0; n < 3; n++) {
					ScalePoly(i, 'itop', 1.0/0.68);
					ScalePoly(i+1024, 'itop', 1.0/0.68);
				}
			}
			if (obj_silent[i] == 0 && (obj_solo[i] == 0)) {
				obj_silent[i] = 1;
				for (n = 0; n < 3; n++) {
					ScalePoly(i, 'itop', 0.68);
					ScalePoly(i+1024, 'itop', 0.68);
				}
			}
		}
	}
	trak->redo_mute_solo();
}

//-----------------------------------------------------------

void	TSoundView::delete_channel(long channel_id)
{
	long	i;
	Channel	*c;

	delete_obj(channel_id);

	acquire_sem(channel_list_sem);

	delete chan[channel_id];

	for (i = channel_id; i < NC; i++) {
		chan[i] = chan[i+1];
		remap_id(i+1, i);
		obj_state[i] = obj_state[i+1];
		obj_muted[i] = obj_muted[i+1];
		obj_solo[i] = obj_solo[i+1];
		obj_hilite[i] = obj_hilite[i+1];
		obj_silent[i] = obj_silent[i+1];
	}

	NC--;
	release_sem(channel_list_sem);
}

//-----------------------------------------------------------

void	TSoundView::delete_channels()
{
	long	i;

again:;

	for (i = 0; i < NC; i++) {
		if (obj_state[i]) {
			obj_state[i] = 0;
			delete_channel(i);
			goto again;
		}
	}
}

//-----------------------------------------------------------

void TSoundView::KeyDown(const char *key, int32 count)
{
	
	if (view_state == MIX_VIEW) {
		switch(*key) {
			case 8  :					//back space
				//delete_channels();
				break;
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
			case ',':
				vpz += 5;
				need_vector = 1;
				break;
			case '.':
				vpz -= 5;
				if (vpz > -300)
					vpz = -300;
				need_vector = 1;
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
				((TWaveWindow *)Window())->Switch();
				break;
			case 'p':
			case 'P':
				do_snow ^= 0x01;
				break;
		}
		return;
	}
}


//-----------------------------------------------------------

long	init_anim(void *p)
{
	TSoundView	*x;
	
	x = (TSoundView*)p;
	
	x->do_anim();
	return 0;
}

//-----------------------------------------------------------


void	TSoundView::MessageReceived(BMessage *message)
{
	bool handled = false;
	
	// was this message dropped?
	if (message->WasDropped()) {
		BPoint dropLoc;
		BPoint offset;
		
		dropLoc = message->DropPoint(&offset);
		ConvertFromScreen(&dropLoc);
		ConvertFromScreen(&offset);

		handled = MessageDropped(message, dropLoc, offset);
	}
}


//--------------------------------------------------------------

void	TSoundView::do_add_channel(Channel *c, float x, float y)
{
	float	dx,dy;
	BPoint	w;
	ulong	b;
	float	obj_x;
	float	obj_y;

	chan[NC] = c;
	
	long	n1,n2,n3,n4,m1,m2,m3,m4;
	float	vs,hs;

	vs = 75;
	hs = 8;
	n1 = add_ppoint(-hs,-hs,3,NC, 'bomm');
	n2 = add_ppoint( hs,-hs,3,NC, 'bopm');
	n3 = add_ppoint( hs, hs,3,NC, 'bopp');
	n4 = add_ppoint( -hs, hs,3,NC, 'bomp');

	m1 = add_ppoint(-hs,-hs,vs,NC, 'itop');
	m2 = add_ppoint( hs,-hs,vs,NC, 'itop');
	m3 = add_ppoint( hs, hs,vs,NC, 'itop');
	m4 = add_ppoint(-hs,hs,vs,NC, 'itop');

	add_ppoint(0, 0, vs*1.2, NC+1024, 'itop');			//tag point for label
	add_ppoint(0, 0, vs*1.35, NC, 'itop');				//tag point for label

long	co;
long	rn;
	
	rn = NC % 3;

	co = rn * 4 + 2;										//was + 2

	add_poly(m1,m2,m3,m4,co, NC);
	
	add_poly(n1,n2,m2,m1,co, NC);							//back face
	
	add_poly(n3,n4,m4,m3,co, NC);	
	add_poly(n2,n3,m3,m2,co, NC);	
	add_poly(n1,n4,m4,m1,co, NC);
	add_poly(n1,n2,n3,n4,co, NC);							//bottom poly

	hs = 5;
	n1 = add_ppoint(-hs, 0, 4, NC);
	n2 = add_ppoint(+hs, 0, 4, NC);
	n3 = add_ppoint(-hs, 0, 30, NC, 'ttop');
	n4 = add_ppoint(hs, 0, 30, NC, 'ttop');
	add_poly(n1,n2,n4,n3,4, NC);

	MovePoly(NC,x , 0, y);

	NC++;
}

//--------------------------------------------------------------

void	TSoundView::do_add_channel(char *name, TrackViewer *viewer)
{
	float	dx,dy;
	BPoint	w;
	ulong	b;
	float	obj_x;
	float	obj_y;

	dx = -100000 - tag_x;
	dy = 0 - tag_y;

	tag_x = -100000;
	tag_y = 0;	
	MovePoly(DRAG_TAG, dx, 0, dy);
	Animate();
	chan[NC] = new Channel(name, viewer);
	
	long	n1,n2,n3,n4,m1,m2,m3,m4;
	float	vs,hs;

	vs = 75;
	hs = 8;
	n1 = add_ppoint(-hs,-hs,3,NC, 'bomm');
	n2 = add_ppoint( hs,-hs,3,NC, 'bopm');
	n3 = add_ppoint( hs, hs,3,NC, 'bopp');
	n4 = add_ppoint( -hs, hs,3,NC, 'bomp');

	m1 = add_ppoint(-hs,-hs,vs,NC, 'itop');
	m2 = add_ppoint( hs,-hs,vs,NC, 'itop');
	m3 = add_ppoint( hs, hs,vs,NC, 'itop');
	m4 = add_ppoint(-hs,hs,vs,NC, 'itop');

	add_ppoint(0, 0, vs*1.2, NC+1024, 'itop');			//tag point for label
	add_ppoint(0, 0, vs*1.35, NC, 'itop');				//tag point for label

	add_poly(m1,m2,m3,m4,6, NC);
	add_poly(n3,n4,m4,m3,6, NC);	
	add_poly(n2,n3,m3,m2,6, NC);	
	add_poly(n1,n4,m4,m1,6, NC);
	add_poly(n1,n2,n3,n4,4, NC);							//bottom poly

	hs = 5;
	n1 = add_ppoint(-hs, 0, 4, NC);
	n2 = add_ppoint(+hs, 0, 4, NC);
	n3 = add_ppoint(-hs, 0, 30, NC, 'ttop');
	n4 = add_ppoint(hs, 0, 30, NC, 'ttop');
	add_poly(n1,n2,n4,n3,8, NC);

	GetMouse(&w, &b);
	w.x -= bm_hp;

	invert_map(w.x, w.y, &obj_x, &obj_y);

	MovePoly(NC,obj_x , 0, obj_y);

	NC++;
}

//--------------------------------------------------------------

bool	TSoundView::MessageDropped(BMessage *inMessage, BPoint where, BPoint offset)
{
	return 0;
/*
	entry_ref	ref;
	BEntry		*an_entry;
	BPath		p;

	switch (inMessage->what ){
   		case B_SIMPLE_DATA:
   		case B_REFS_RECEIVED:
			if (inMessage->HasRef("refs")) {
				for (int32 index = 0; ; index++) {
					if(inMessage->FindRef("refs", index, &ref) != B_OK)
						break;

					an_entry = new BEntry(&ref);
					an_entry->GetPath(&p);
					p.GetParent(&p);
					//printf("path=%s\n", p.Path());

					delete an_entry;
					do_add_channel(ref.name, p.Path());
   				}
   			}
		break;
	}
*/
}

//--------------------------------------------------------------

void	TSoundView::MouseMoved(BPoint p, uint32, const BMessage *m)
{
	BPoint	w;
	ulong	b;
	float	obj_x;
	float	obj_y;
	float	dx,dy;
	float	s,e;

	if (m)
	if (m->HasRef("refs")) {
		GetMouse(&w, &b, false);
		w.x -= bm_hp;

		invert_map(w.x, w.y, &obj_x, &obj_y);

		dx = obj_x - tag_x;
		dy = obj_y - tag_y;

		tag_x = obj_x;
		tag_y = obj_y;	
		MovePoly(DRAG_TAG, dx, 0, dy);
		w.x += bm_hp;
		Animate(BRect(w.x-100,w.y-100,w.x+100,w.y+100));
	}
}

//--------------------------------------------------------------
// invert_map is a little bit brute force... still only takes
// 180 msec on my 200 Mhz... and is called just a few times per second !			
//--------------------------------------------------------------

void	TSoundView::invert_map(float sx, float sy, float *obj_x, float *obj_y)
{
	float	wx,wz,wy;
	float	x0,y0,z0;
	float	x,y,z;
	float	tx,tz;
	float	bx;
	float	bz;
	float	sina;
	float	cosa;
	float	sinb;
	float	cosb;
	float	sinc;
	float	cosc;
	float	sum;
	float	dz;
	float	dist;
	float	min_x,max_x;
	float	min_y,max_y;
	long	i;
	float	step;
	float	min;

	sina = sin(cur_alpha);
	cosa = cos(cur_alpha);

	sinb = sin(cur_delta);
	cosb = cos(cur_delta);

	sinc = sin(cur_zeta);
	cosc = cos(cur_zeta);


	sx -= HS/2;
	sy -= off_y;


	min_x = -500;
	max_x = 500;
	min_y = -500;
	max_y = 500;

	step = 500;

	for (i = 0; i < 5; i++) {
		bx = -1000;
		bz = -1000;
		wy = 0;
		step /= 5.0;
		min = 1000000;
		for (wx = min_x; wx < max_x; wx += step) {
			for (wz = min_y; wz < max_y; wz += step) {
				x = wx;
				y = wy;
				z = wz;

				x0 = x * cosa - y * sina;
				y0 = x * sina + y * cosa;
				x = x0;
				y = y0;
				x0 = x * cosc - z * sinc;
				z0 = x * sinc + z * cosc;
				x = x0;
				z = z0;
				y0 = y * cosb - z * sinb;
				z0 = y * sinb + z * cosb;

				y = y0;
				y = y0;z = z0;

				dz = (z0 + vpz);
				dist = b_sqrt(dz*dz);
				dist = dst/dist;
				x = x*dist;
				y = y*dist;
			
				sum = fabs(x-sx) + fabs(y-sy);
				if (sum < min) {
					min = sum;
					bx = wx;
					bz = wz;
				}
			}
		}
		min_x = bx-(step*1.2);
		max_x = bx+(step*1.2);
		min_y = bz-(step*1.2);
		max_y = bz+(step*1.2);
	}

	if (bx < -250) bx = -250;
	if (bx > 250) bx = 250;
	if (bz < -250) bz = -250;
	if (bz > 250) bz = 250;

	*obj_x = bx;
	*obj_y = bz;
}

//--------------------------------------------------------------

void	TSoundView::clear()
{
	long	x,y;
	uchar	*p;
	long	v;
	long	d;
	double	s,e;



	o_min_y = min_y;
	o_max_y = max_y;
	o_min_x = min_x;
	o_max_x = max_x;

	min_x -= 2;
	max_x += 2;


	if (min_x < 0) min_x = 0;
	if (max_x > HS) max_x = HS;
	

	min_y -= 3;
	max_y += 3;
	
	if (min_y > max_y)
		return;


	if (min_y < 0) 
		min_y = 0;

	if (max_y > (VS-1))
		max_y = VS-1;

	p = (uchar *)the_base + min_y * HS;

	d = (max_y - min_y) * HS;

double	t0,t1;

	//t0 = system_time();

	long	i;
	


	i = max_y - min_y;

	p += min_x;

	while(i > 0) {
		i--;
		memset(p, 0, (max_x - min_x));
		p += HS;
	}
	
	//t1 = system_time();
	//printf("dt=%f\n", (t1-t0)/1000.0);

	min_y = 640;
	max_y = 0;
	min_x = HS;
	max_x = 0;
	return;

	p = (uchar *)the_base;

	d = (int)(mtime * 20);

	for (y = 0; y < VS; y++)
	for (x = 0; x < HS; x++) {
		v = (x+y)*x*d;
		v &= 0x1f;
		v >>= 3;
		*p++ = v;
	}

test:
	s = system_time();
	for (long i = 0; i < 5000; i++) {
		hline((uchar *)the_base, 30, (uchar *)mapper[3]);
	}
	e = system_time();
}

//--------------------------------------------------------------

void	TSoundView::new_block(long i)
{
	long	n1,n2,n3,n4,m1,m2,m3,m4;
	float	vs,hs;

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
}

