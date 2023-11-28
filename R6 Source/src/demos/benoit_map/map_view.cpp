#ifndef MAP_VIEW_H
#include "map_view.h"
#endif
#include <Screen.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <Application.h>
#include <Roster.h>
#include <Entry.h>
#include <Path.h>
//--------------------------------------------------------

HelloView::HelloView(BRect rect, char *name)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	LoadData();
	px0=0;
	py0=0;
	s = 10;
	the_off = new BBitmap(BRect(0,0,511,511), B_COLOR_8_BIT, TRUE);
	the_off->AddChild(off_view = new BView(BRect(0,0,511,511), "", B_FOLLOW_ALL, B_WILL_DRAW));
}

//--------------------------------------------------------

static ulong
byte_me (ulong x)
{
  return (x >> 24
		  | (x & 0x00ff0000) >> 8
		  | (x & 0x0000ff00) << 8
		  | x << 24);
}

//--------------------------------------------------------

	HelloView::~HelloView()
{
	long	i,j;
	
	for (i = 0; i < 100; i++)
	for (j = 0; j < 100; j++)
		free((char *)arrays[i][j]);
	delete the_off;
}

//--------------------------------------------------------

void	HelloView::LoadData()
{
	long		ref;
	long		i,j;
	long		k;
	ulong		tmp;
	long		v;
	char		q;
	float		dx,dy;
	long		cnt;
	app_info	info;	
	entry_ref	eref;
	BEntry		*entry;
	BPath		path;
	char		buf[256];
	char		tmpp[256];

	be_app->GetAppInfo(&info);
	eref = info.ref;
	entry = new BEntry(&eref, FALSE);
	entry->GetPath(&path);
	path.GetParent(&path);
	strcpy(buf, path.Path());

	delete entry;

	sprintf(tmpp, "%s/%s", buf, "xdata");
	ref = open(tmpp, O_RDWR);
	lseek(ref, 0, 0);
	for (i = 0; i < 100; i++)
	for (j = 0; j < 100; j++) {
		cnt= read(ref, &tmp, 4);
		tmp = byte_me(tmp);
		cnts[i][99-j] = tmp;
		arrays[i][99-j] = (long *)malloc(tmp * 4);
		read(ref, arrays[i][99-j], 4*tmp);
		for (k = 0; k < tmp; k++)
			arrays[i][99-j][k] = byte_me(arrays[i][99-j][k]);
	}
	close(ref);
	
	sprintf(tmpp, "%s/%s", buf, "odata");
	ref = open(tmpp, O_RDWR);
	read(ref, data, sizeof(data));
	close(ref);
	
	
	for (i = 0; i < 86618; i++) {
		*((ulong *)&data[i].lat1) = byte_me(*((ulong *)&data[i].lat1));
		*((ulong *)&data[i].long1) = byte_me(*((ulong *)&data[i].long1));
		*((ulong *)&data[i].lat2) = byte_me(*((ulong *)&data[i].lat2));
		*((ulong *)&data[i].long2) = byte_me(*((ulong *)&data[i].long2));
		data[i].lat2 = 100.0-data[i].lat2;
		data[i].lat1 = 100.0-data[i].lat1;
		dx = fabs(data[i].lat1-data[i].lat2);
		dy = fabs(data[i].long1-data[i].long2);
		length[i] = (dx*dx)+(dy*dy);
		if (data[i].code[0] == 'A') {
			v = 0;
			v = (data[i].code[1] - 0x30)*10;
			v += (data[i].code[2] - 0x30);
			q = 0xff;
			switch(v) {
				case	0x01:
				case	0x02:
				case	0x03:
				case	0x04:
					 q = 0x01;
					break;
				
				case	0x05:
				case	0x06:
				case	0x07:
				case	0x08:
					q = 0x02;
					break;
				case	10:
				case	11:
				case	12:
				case	13:
				case	14:
				case	15:
				case	16:
				case	17:
				case	18:
					q = 0x04;
					break;
				case	20:
					q = 0x01;
					break;
				case	21:
				case	22:
				case	23:
					q = 0x02;
					break;
				case	24:
				case	25:
				case	26:
				case	27:
				case	28:
					q = 0x04;
					break;
				case	30:
				case	31:
				case	32:
				case	33:
				case	34:
					q = 0x01;
					break;
				case	35:
				case	36:
				case	37:
				case	38:
					q = 0x02;
					break;
				case	40:
					q = 0x02;
					break;
				case	41:
				case	42:
				case	43:
				case	44:
				case	45:
				case	46:
				case	47:
				case	48:
					q = 0x01;
					break;
				case	50:
				case	51:
				case	52:
				case	53:
					q = 0x00;
			}
			if (*(ulong *)&data[i].type == 'Exwy')
				q = 0x04;
			if (*(ulong *)&data[i].type == 'Blvd')
				q = 0x04;
			if (q != 0x04)
			if (*(ulong *)&data[i].type == 'Road')
				q = 0x03;
				
				
			data[i].code[0] = q;
		}
		else {
			data[i].code[0] = 0xff;
		}
	}
}

//--------------------------------------------------------

void HelloView::AttachedToWindow()
{
}

//--------------------------------------------------------


void	HelloView::MouseDown(BPoint where)
{
	long	buttons;
	ulong	but;
	float	top,left,bottom,right;
	float	cx,cy;
	float	w;
	float	dx,dy;
	float	ncx,ncy;
	float	mul;
	float	k;
	
	
	but = 1;
	while(but) {
		top = py0;
		left = px0;
		
		w = 442.0/s;
		cx = px0 + w/2.0;
		cy = py0 + w/2.0;
		
		GetMouse(&where, &but);
		ncx = ((where.x/442.0) * w) + px0;
		ncy = ((where.y/442.0) * w) + py0;
		
		dx = ncx-cx;
		dy = ncy-cy;
			
		mul = 1.03;
		k = (fabs(where.x-221)+fabs(where.y-221)) / 50.0;
		if (k>3.0) k = 1000.0;
		
		mul = (mul + (k)) / (k+1.0);
		
		dx /= 10.0;
		dy /= 10.0;
		
		if (but==1 && (modifiers() & B_SHIFT_KEY)) {
			s = s * mul;
			px0 += dx;
			py0 += dy;
			Draw(BRect(0,0,1000,1000));
		}
		else
		if (but==1) {
			s = s / mul;
			px0 += dx;
			py0 += dy;
			Draw(BRect(0,0,1000,1000));
		}
	}
}

//--------------------------------------------------------

inline void	fline(char *base, long x1,long y1,long x2, long y2, char c)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = 512;
	long	error;
	long	cpt;
	
	dx = x2-x1;
	dy = y2-y1;
	
	base = base + y1*512+x1;
	
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
	
	if (dx > dy) {
		error = dx >> 1;
		cpt = x2 - x1;
		
		while(cpt>=0) {
			cpt--;
			*base++ = c;
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
		while(cpt >= 0) {
			cpt--;
			*base = c;
			base += rowbyte;
			error += dx;
			if (error >= dy) {
				base++;
				error -= dy;
			}
		}
	}
	
}

//--------------------------------------------------------


char	is_inside(float x1, float y1)
{
	if (x1<15) return 0;
	if (y1<15) return 0;
	if (x1>420) return 0;
	if (y1>420) return 0;
	return 1;
}

//--------------------------------------------------------


void	calc_center(float x1,float y1, float x2, float y2, float *cx, float *cy)
{
	float	dx,dy;
	long	max = 0;
	
	dx = x1-x2;
	dy = y1-y2;

	dx /= 20.0;
	dy /= 20.0;
	
	max = 20;
	
	do {
		x1 = x1 - dx;
		y1 = y1 - dy;
		if (is_inside(x1,y1)) goto out1;
		max--;
	} while(max);
out1:;

	max = 20;
	do {
		x2 = x2 + dx;
		y2 = y2 + dy;
		if (is_inside(x2,y2)) goto out2;
		max--;
	} while(max);
out2:;

	*cx = (x1+x2)/2.0;
	*cy = (y2+y1)/2.0;
}


//--------------------------------------------------------

#define left_code	0x01
#define	right_code	0x02
#define	bottom_code	0x04
#define	top_code	0x08

//--------------------------------------------------------

char line_inside(long xx1, long yy1, long xx2, long yy2)
{
	long		tmp;
	uchar		code1;
	uchar		code2;


	if (xx1 > xx2) {
		tmp = xx1;
		xx1 = xx2;
		xx2 = tmp;
		tmp = yy1;
		yy1 = yy2;
		yy2 = tmp;
	}
	
	code1 = 0;
	if (yy1 > 430)
		code1 |= bottom_code;
	else
	if (yy1 < 10)
		code1 |= top_code;

	if (xx1 < 10)
		code1 |= left_code;
	else
	if (xx1 > 430)
		code1 |= right_code;

	code2 = 0;
	if (yy2 > 430)
		code2 |= bottom_code;
	else
	if (yy2 < 10)
		code2 |= top_code;

	if (xx2 < 10)
		code2 |= left_code;
	else
	if (xx2 > 430)
		code2 |= right_code;

	if (((code1 | code2) == 0) || ((code1 ^ code2) != 0))
		return 1;
	
	return 0;
}

//--------------------------------------------------------

#define	N	20

void HelloView::AddNames()
{
	long		x,y;
	long		cnt;
	long		*p;
	long		top,left,bottom,right;
	long		j;
	float		length;
	float		dx,dy;
	float		ls[N];
	long		ix[N];
	long		k;
	long		i;
	float		x1,y1,x2,y2;
	float		tmp;
	long		v;
	float		cx,cy;
	long		max;
	long		sy,sx;

	
	for (i = 0; i < N; i++) {
		ls[i] = 0;
		ix[i] = 0;
	}
		
	the_off->Lock();
	off_view->SetFont(be_plain_font);
	off_view->SetFontSize(9);
	
	top = py0;
	left = px0;
	right = px0 + (442.0/s) + 1;
	bottom = py0 + (442.0/s) + 1;
	
	if (right>99)
		right = 99;
	if (bottom>99)
		bottom = 99;
	if (top<0)
		top = 0;
	if (left<0)
		left = 0;
	if (top > 99 || left > 99)
		return;
	
	max = 3000;
	
	
	sx = 1;
	sy = 1;
	
	if (s < 14)	{
		sx = 2;
		sy = 2;
	}
	
	if (s < 25)
		max = 22;
	if (s < 18)
		max = 11;
	if (s < 16)
		max = 7;
	if (s < 14)
		max = 5;
	if (s < 12)
		max = 3;
	if (s < 10)
		max = 2;
	
	top = top & 0xfffe;
	left = left & 0xfffe;
	
	for (y = top; y < bottom; y+=sy) {
		for (x = left; x < right; x+=sx) {
			cnt = cnts[x][y];
			if (cnt == 0)
				goto next_block;
			p = arrays[x][y];
			if (cnt > max) cnt = max;
			
			for (j = 0; j < cnt; j++) {
				i = *p++;
				if (data[i].name[0] == ' ')
					goto skip;
					
				x1 = s*(data[i].long1-px0);
				y1 = s*(data[i].lat1-py0);
				x2 = s*(data[i].long2-px0);
				y2 = s*(data[i].lat2-py0);
				
					
				if (x1 > x2) {
					tmp = x1;
					x1 = x2;
					x2 = tmp;
					tmp = y1;
					y1 = y2;
					y2 = tmp;
				}
				
				
				v = data[i].code[0];
				
				if (v == -1)
					goto skip;
					
				dx = (x1-x2);
				dy = (y1-y2);
				
				if (!line_inside(x1,y1,x2,y2))
					goto skip;
					
				length = (dx*dx+dy*dy);
				
				if (length>ls[N-1]) {
					for (k = 0; k < N; k++)
						if (ix[k] == i)
							goto out0;
					k = N-1;
					while(k>0) {
						ls[k] = ls[k-1];
						ix[k] = ix[k-1];
						if (ls[k]>length) {
							ls[k] = length;
							ix[k] = i;
							goto out0;
						}
						k--;
					}
					ls[0] = length;
					ix[0] = i;
out0:;
skip:;
				}
				else
					goto next_block;
			}
next_block:;
		}
	}
	

BRect	r;
float	w;
long	pp;
	
	for (j = 0; j < 20; j++) {
		i = ix[j];
		x1 = s*(data[i].long1-px0);
		y1 = s*(data[i].lat1-py0);
		x2 = s*(data[i].long2-px0);
		y2 = s*(data[i].lat2-py0);
		
		if (is_inside(x1,y1) && is_inside(x2,y2)) {
			cx = (x1+x2)/2.0;
			cy = (y1+y2)/2.0;
		}
		else {
			calc_center(x1,y1,x2,y2,&cx,&cy);
		}
		pp = strlen(data[i].name);
		pp--;
		while(data[i].name[pp] == ' ' && pp>0) {
			data[i].name[pp] = 0x00;
			pp--;
		}
		
		w = off_view->StringWidth(data[i].name);
		r.top = cy + 3;
		r.left = cx + 3;
		r.bottom = r.top + 12;
		r.right = r.left + w + 4;
		off_view->SetHighColor(255,255,255);
		off_view->FillRect(r);
		off_view->SetHighColor(50,50,50);
		off_view->StrokeRect(r);
		off_view->MovePenTo(BPoint(r.left + 2, r.top + 10));
		off_view->SetDrawingMode(B_OP_OVER);
		off_view->DrawString(data[i].name);

	}
	
char	buf[128];
long	alt;

	alt = 200000.0 / s;
	sprintf(buf, "Altitude : %ld feet",  alt);
	
	r.left = 20;
	r.top = 410;
	r.bottom = 422;
	r.right = 140;
	off_view->SetHighColor(255,255,255);
	off_view->FillRect(r);
	off_view->SetHighColor(50,50,50);
	off_view->StrokeRect(r);
	
	off_view->MovePenTo(BPoint(r.left + 2, r.top + 10));
	off_view->SetDrawingMode(B_OP_OVER);
	off_view->DrawString(buf);
	
	off_view->Sync();
	the_off->Unlock();
}

//--------------------------------------------------------

void HelloView::Draw(BRect)
{
	long		i;
	long		c;
	rgb_color	col;
	long		top,left,bottom,right;
	long		x,y;
	long		cnt;
	long		*p;
	long		j;
	long		max;
	long		*cp;
	long		v = 0x1e1e1e1e;
	long		x1,y1,x2,y2;
	
	double		st,end;
	char		fc;
	long		tmp;
	char		fc_blue;
	char		fc_red;
	s_e			*cur_p;
	char		*base;
	BScreen		screen;
	
	
	
	cp = (long *)the_off->Bits();
	base = (char *)cp;
	
	for (i = 0; i < 512*512/(16); i++) {
		*cp++ = v;
		*cp++ = v;
		*cp++ = v;
		*cp++ = v;
	}

	the_off->Lock();
	
	max = s*2.5;
	
	if (max < 3)
		max = 3;
	if (max > 80)
		max = 1000;
	
	st = system_time();
	
	top = py0;
	left = px0;
	
	right = px0 + (442.0/s) + 1;
	bottom = py0 + (442.0/s) + 1;
	
	if (right>99)
		right = 99;
	if (bottom>99)
		bottom = 99;
	if (top<0)
		top = 0;
	if (left<0)
		left = 0;
	if (top > 99 || left > 99)
		return;
	
	//col.red = 255;
	//col.green = 0;
	//col.blue = 0;
	fc_red = screen.IndexForColor(255,0,0,0);
	
	col.red = 0;
	col.green = 0;
	col.blue = 220;
	fc_blue = screen.IndexForColor(0,0,220,0);
	
	off_view->BeginLineArray(256);
	
	for (y = top; y < bottom; y++) {
		for (x = left; x < right; x++) {
			cnt = cnts[x][y];
			if (cnt>max)
				cnt = max;
			
			p = arrays[x][y];
			
			for (j = 0; j < cnt; j++) {
				i = *p++;
				cur_p = &(data[i]);
				
				switch(cur_p->code[0]) {
					case	-1 :
						col.red = 190;
						col.green = 190;
						col.blue = 190;
						fc = 190/8;
						break;
					case	0x01:
						col.red = 100;
						col.green = 100;
						col.blue = 100;
						fc = 100/8;
						break;
					case	0x02:
						col.red = 160;
						col.green = 160;
						col.blue = 160;
						fc = 160/8;
						break;
					case	0x03:
						col.red = 0;
						col.green = 0;
						col.blue = 220;
						fc = fc_blue;
						break;
					case	0x04:
						col.red = 255;
						col.green = 0;
						col.blue = 0;
						fc = fc_red;
						break;
				}
						
						
				x1 = s * (cur_p->long1 - px0);
				y1 = s * (cur_p->lat1 - py0);
				x2 = s * (cur_p->long2 - px0);
				y2 = s * (cur_p->lat2 - py0);
				
				if (x1 > x2) {
					tmp = x1;
					x1 = x2;
					x2 = tmp;
					tmp = y1;
					y1 = y2;
					y2 = tmp;
				}
				
				if ((x1 > 470) || (x2 < 0) || ((y1 > 470) && (y2 > 470)))
					goto skip;
				
				if  ((x1 >= 0) && (x2 <= 512) && (y1 >= 0) && (y1 < 512) && (y2 >= 0) && (y2 < 512)) {
					fline(base, x1, y1, x2, y2, fc);
					goto skip;
				}
				
				
				off_view->AddLine(BPoint(x1, y1),
				  		  		  BPoint(x2, y2),col);
				c++;
				if (c == 256) {
					off_view->EndLineArray();
					c = 0;
					off_view->BeginLineArray(256);
				}
skip:;
			}
		}
	}



	off_view->EndLineArray();
	off_view->Sync();
	the_off->Unlock();
	AddNames();
	end = system_time();
	DrawBitmap(the_off, BPoint(0,0));
}
