#include <Window.h>

#include "SliderBits.h"
#include "gslider.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "global.h"



//-----------------------------------------------------------------

TSlider	*global_slider = 0;

//-----------------------------------------------------------------


void	make_slider()
{
	long	xs,ys;
	long	x,y;
	long	i,j;
	long	d;
	long	h;

	xs = SLIDER_WIDTH;
	ys = SLIDER_HEIGHT;

	for (y = 0; y < ys; y++)
	for (x = 0; x < xs; x++)
		slider[y][x] = 0xff;

	for (i = 0; i < 13; i++) {
		d = i;
		h = xs/2;
		slider[i + 2][h] = 0x10;
		slider[SLIDER_HEIGHT - 2 - i][h] = 0x10;

		for (j = 0; j < d; j++) {
			slider[i+2][h+j] = 0x10;
			slider[i+2][h-j] = 0x10;
		
			slider[SLIDER_HEIGHT-2-i][h+j] = 0x10;
			slider[SLIDER_HEIGHT-2-i][h-j] = 0x10;
		}
	}
}


//-----------------------------------------------------------------

float	get_global_gain()
{
	return (100.0-global_slider->Value())/15.0;
}

//-----------------------------------------------------------------

void	TSlider::DrawBack(BView *v)
{
	BRect	r;
	long	i;
	long	mid;
	long	hsize;
	long	dv = 15;
 
	max = 100.0;

	mid = S_HS/2;

	make_slider();

	r.Set(0,0,S_HS,S_VS+50);
	v->SetHighColor(S_COLOR_R,S_COLOR_G,S_COLOR_B);
	v->FillRect(r);

	v->SetHighColor(20,20,20);
	for (i = 0; i <= S_VS; i+=10) {
		v->MovePenTo(BPoint(mid-5,i+dv));
		v->StrokeLine(BPoint(mid+4, i+dv));
	}

	v->SetHighColor(110,110,110);
	v->MovePenTo(BPoint(mid, 0+dv));
	v->StrokeLine(BPoint(mid, S_VS+dv));

	v->SetHighColor(220,220,220);
	v->MovePenTo(BPoint(mid-1, 0+dv));
	v->StrokeLine(BPoint(mid-1, S_VS+dv));
	v->SetHighColor(30,30,30);
	v->MovePenTo(BPoint(mid+1, 0+dv));
	v->StrokeLine(BPoint(mid+1, S_VS+dv));

	
	for (i = 1; i <= S_VS+1; i+=20) {
		hsize = 10;
		if ((i == 1) || (i == S_VS+1))
			hsize = 14;
		v->SetHighColor(130,130,130);
		v->MovePenTo(BPoint(mid-hsize-1,i+dv));
		v->StrokeLine(BPoint(mid+hsize-1, i+dv));
		v->SetHighColor(220,220,220);
		v->MovePenTo(BPoint(mid-hsize-1,i-1+dv));
		v->StrokeLine(BPoint(mid+hsize-1, i-1+dv));
		v->SetHighColor(30,30,30);
		v->MovePenTo(BPoint(mid-hsize-1,i+1+dv));
		v->StrokeLine(BPoint(mid+hsize-1, i+1+dv));
		
		v->SetHighColor(30,30,30);
		v->MovePenTo(BPoint(mid+hsize,i-1+dv));
		v->StrokeLine(BPoint(mid+hsize, i+1+dv));

		v->SetHighColor(40,40,40);
		v->MovePenTo(BPoint(mid+hsize,i-1+dv));
		v->StrokeLine(BPoint(mid+hsize, i-1+dv));

		v->MovePenTo(BPoint(mid-hsize-1,i+1+dv));
		v->StrokeLine(BPoint(mid-hsize-1, i+1+dv));

	}
	v->SetHighColor(130,130,130);
	v->MovePenTo(BPoint(mid, 2+dv));
	v->StrokeLine(BPoint(mid, S_VS-2+dv));

	v->SetHighColor(220,220,220);
	v->MovePenTo(BPoint(mid-1, 2+dv));
	v->StrokeLine(BPoint(mid-1, S_VS-2+dv));
	v->SetFontSize(10);
	v->SetDrawingMode(B_OP_OVER);
	/*
	v->MovePenTo(BPoint(6, 10));
	v->SetHighColor(120,120,120);
	v->DrawString("100%");
	*/
	v->MovePenTo(BPoint(7, 10));
	v->SetHighColor(10,10,10);
	v->DrawString("100%");

	v->MovePenTo(BPoint(15, S_VS + 30));
	v->DrawString("0%");

	v->SetDrawingMode(B_OP_COPY);
	v->Sync();
}

//-----------------------------------------------------------------

	TSlider::TSlider(BRect r, char *name, float v) :
		  	BView(r, name, B_FOLLOW_NONE,B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	
	back = new BBitmap(BRect(0, 0, S_HS-1, S_VS+40),
						  	B_COLOR_8_BIT,
						  	TRUE);

	back->AddChild(bv = new BView(BRect(0,0,S_HS,S_VS+40), "back_view", B_FOLLOW_NONE, B_WILL_DRAW));

	back->Lock();
	DrawBack(bv);
	back->Unlock();	

	composit = new BBitmap(BRect(0, 0, S_HS-1, S_VS+40),
						   B_COLOR_8_BIT,
						   FALSE);
	
	dt = 0;

	SetValue(v);
	global_slider = this;
}

//-----------------------------------------------------------------

void	TSlider::Draw(BRect r)
{	
	DrawBitmap(composit, BPoint(0,0));
}

//-----------------------------------------------------------------

	TSlider::~TSlider()
{
	delete composit;
	delete back;
}

//-----------------------------------------------------------------

void	TSlider::MouseDown(BPoint where)
{
	BPoint	w;
	BPoint	w0;
	ulong	but;	
	float	v;
	long	vp;
	long	offset;

	vp = (int)((val/max) * (S_VS)); 
	vp += 14;
	
	if (fabs(where.y - vp) > 14) {
		offset = 0;
	}
	else {
		offset = (int)(where.y - vp);
	}

	do {
		GetMouse(&w, &but);
		if (w != w0) {
			w0 = w;
			v = (w.y-14-offset) / S_VS;
			v *= max;
			xSetValue(v);
		}
		Window()->Unlock();
		snooze(20000);
		Window()->Lock();
	} while(but);
}

//-----------------------------------------------------------------

float	TSlider::Value()
{
	return	val;
}

//-----------------------------------------------------------------

void	TSlider::DoComposit()
{
	uchar	*dest;
	uchar	*src;
	long	len;
	long	vp;
	uchar	*b_t;
	uchar	*b_c;
	long	y,x;
	long	s_byte;
	long	d_byte;
	long	mid = S_HS/2;
	ushort	v;

	len = composit->BitsLength();
	dest = (uchar *)composit->Bits();
	src = (uchar *)back->Bits();
	memcpy(dest, src, len);	
	
	vp = (int)((val/max) * (S_VS)); 
	

	s_byte = SLIDER_WIDTH;
	d_byte = composit->BytesPerRow();

	for (y = 0; y < SLIDER_HEIGHT; y++) {
		b_t = (uchar *)slider + (y * s_byte);
		b_c = dest + ((vp + 3 + y) * d_byte) + mid - SLIDER_WIDTH/2 + 4;
		for (x = 0; x < SLIDER_WIDTH-2; x++) {
			v = *b_t++;
			if (v != 0xff) {
				v = (*b_c);
				v = v / 2;
				*b_c = v;
			}
			b_c++;
		}
	}
	
	for (y = 0; y < SLIDER_HEIGHT; y++) {
		b_t = (uchar *)slider + (y * s_byte);
		b_c = dest + ((vp + y) * d_byte) + mid - SLIDER_WIDTH/2 - 1;
		for (x = 0; x < SLIDER_WIDTH-2; x++) {
			v = *b_t++;
			if (v != 0xff) {
				v = (v) + (*b_c);
				if (v > 31) v = 31;
				*b_c = v;
			}
			b_c++;
		}
	}
}

//-----------------------------------------------------------------

void	TSlider::SetValue(float v)
{
	float	v0;

	v0 = val;

	if (v < 0)
		v = 0;

	if (v > max)
		val = max;
	else
		val = v;

	if (v0 != val) {
		DoComposit();
		Draw(BRect(0,0,32000,32000));
	}
}

//-----------------------------------------------------------------

void	TSlider::xSetValue(float v)
{
	float	delta;
	long	i;
	float	v0;
	long	n;
	float	t1,t0;

	if (v > max)
		v = max;
	if (v < 0)
		v = 0;

	delta = val - v;

	n = 8;

	if (fabs(delta) < 40)
		n = 5;
	if (fabs(delta) < 20)
		n = 3;

	if (dt > 13000) {
		n = n / 2;
	}
	if (n < 1)
		n = 1;

	delta /= (float)n;

	v0 = val;

	for (i = 0; i < n; i++) {
		v0 -= (delta);
		t0 = system_time();
		SetValue(v0);
			//Window()->Unlock();
			snooze(10000);
			//Window()->Lock();
		t1 = system_time();
		dt = dt*15.0 + (t1-t0);
		dt /= 16.0;
	}
	SetValue(v);
}


//-----------------------------------------------------------------
