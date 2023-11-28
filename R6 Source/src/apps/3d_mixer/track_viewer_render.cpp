
void	TrackViewer::fline(long xx1, long yy1, long xx2, long yy2, uchar c)
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
		yy1 = (int)(yy1 + (dy * ((-xx1)/dx)));
		xx1 = 0;
	}

	if (xx1 > (TRACK_B_H))
		return;
		
	if (xx2 > (TRACK_B_H)) {
		yy2 = (int)(yy2 - (dy * (xx2-(TRACK_B_H-1))/dx));
		xx2 = (TRACK_B_H-1);
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

	if (yy1 > (TRACK_HEIGHT))
		return;
		
	if (yy2 > TRACK_HEIGHT) {
		xx2 = (int)(xx2 - (dx * (yy2-TRACK_HEIGHT)/dy));
		yy2 = TRACK_HEIGHT;
	}

	

	ffline(xx1+1,yy1+1,xx2+1, yy2+1, c);
}


//-------------------------------------------------------------------------


void	TrackViewer::ffline(long x1,long y1,long x2, long y2, uchar c)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = TRACK_B_H;
	long	error;
	long	cpt;
	char 	*base;
	float	k;
	
	
	if (y1 < 1 || y1 > (TRACK_HEIGHT+10) || y2 < 1 || y2 > (TRACK_HEIGHT+10))
		return;
	if (x1 < 0 || x1 > TRACK_B_H || x2 < 0 || x2 > TRACK_B_H)
		return;
	
	
	dx = x2-x1;
	dy = y2-y1;
	
	base = (char *)the_off->Bits() + y1*rowbyte+x1;
	
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
			dy = (int)(dy*k);
			dx = (int)(dx*k);
			
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
			dy = (int)(dy*k);
			dx = (int)(dx*k);
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
			dy = (int)(dy*k);
			dx = (int)(dx*k);
			
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
			dy = (int)(dy*k);
			dx = (int)(dx*k);
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

//-------------------------------------------------------------------------

void	TrackViewer::vdot(long x1,long y1,long y2, uchar c)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = TRACK_B_H;
	char 	*base;
	float	k;
	
	
	if (y1 < 0)
		y1 = 0;
	if (y2 > (TRACK_HEIGHT - 1))
		y2 = TRACK_HEIGHT - 1;
	if (x1 < 0 || x1 > TRACK_B_H)
		return;
	
	
	dy = y2-y1;
	
	base = (char *)the_off->Bits() + y1*rowbyte+x1;
	
	while(dy>0) {
		*base = c;
		base += rowbyte*2;
		dy -= 2;
	}
}


//-------------------------------------------------------------------------

void	TrackViewer::vl(long x1,long y1,long y2, uchar c)
{
	long	dx,dy;
	long	sy;
	long	rowbyte = TRACK_B_H;
	char 	*base;
	float	k;
	
	if (y1 < 0)
		y1 = 0;
	if (y2 > (TRACK_HEIGHT - 1))
		y2 = TRACK_HEIGHT - 1;
	if (x1 < 0 || x1 > TRACK_B_H)
		return;
	
	
	dy = y2-y1;
	
	base = (char *)the_off->Bits() + y1*rowbyte+x1;
	
	while(dy > 4) {
		*base = c;
		base += rowbyte;
		dy -= 4;
		*base = c;
		base += rowbyte;
		*base = c;
		base += rowbyte;
		*base = c;
		base += rowbyte;
	}
	
	while(dy>0) {
		*base = c;
		base += rowbyte;
		dy--;
	}
}

//-------------------------------------------------------------------------

void	TrackViewer::frect(BRect r, uchar c)
{
	long	rowbyte = TRACK_B_H;
	char 	*base;
	ulong	tmp;
	long	y1,y2;	
	long	x1,x2;

	long	dx,dy;
	
	if (r.right < r.left)
		return;

	y1 = (int)r.top;
	y2 = (int)r.bottom;
	if (y1 >= TRACK_HEIGHT)
		return;

	if (y2 < 0)
		return;

	
	tmp = (c<<24)|(c<<16)|(c<<8)|(c);
	
	if (y1 < 0) {
		y1 = 0;
	}

	if (y2 > (TRACK_HEIGHT-1)) {
		y2 = TRACK_HEIGHT-1;
	}

	dy = (y2-y1);

	x1 = (int)(r.left);

	if (x1 > (TRACK_B_H))
		return;

	if (x1 >= 0) {
		base = (char *)the_off->Bits() + y1*rowbyte+x1;
		
		while(dy>=0) {
			*base = c;
			base += rowbyte;
			dy--;
		}
	}
	else
		x1 = 0;

	x2 = (int)r.right;

	dy = (y2-y1);

	if (x2 < TRACK_B_H) {
		base = (char *)the_off->Bits() + y1*rowbyte+x2;
		
		while(dy>=0) {
			*base = c;
			base += rowbyte;
			dy--;
		}
	}
	else {
		x2 = TRACK_B_H;
	}

	if (r.top >= 0) {
		dx = x2 - x1;
		base = (char *)the_off->Bits() + y1*rowbyte+x1;
		
		while(dx >= 4) {
			*((ulong *)base) = tmp;
			base += 4;
			dx -= 4;
		}
		while(dx>=0) {
			*base++ = c;
			dx--;
		}
	}

	if (r.bottom < TRACK_HEIGHT) {
		dx = x2 - x1;
		base = (char *)the_off->Bits() + y2*rowbyte+x1;
		
		while(dx >= 4) {
			*((ulong *)base) = tmp;
			base += 4;
			dx -= 4;
		}
		while(dx>=0) {
			*base++ = c;
			dx--;
		}
	}
}

//-------------------------------------------------------------------------

void	TrackViewer::ffillrect(BRect r, uchar c)
{
	long	rowbyte = TRACK_B_H;
	char 	*base;
	ulong	tmp;
	long	y1,y2;	
	long	x1,x2;
	long	dx,dy;
	
	y1 = (int)r.top;
	y2 = (int)r.bottom;
	if (r.right < r.left)
		return;

	if (y1 >= TRACK_HEIGHT)
		return;

	if (y2 < 0)
		return;

	
	tmp = (c<<24)|(c<<16)|(c<<8)|(c);
	
	if (y1 < 0) {
		y1 = 0;
	}

	if (y2 > (TRACK_HEIGHT - 1)) {
		y2 = TRACK_HEIGHT - 1;
	}

	dy = (y2-y1);

	x1 = (int)(r.left);
	x2 = (int)(r.right);

	if (x1 > (TRACK_B_H))
		return;
	if (x2 < 0)
		return;

	if (x1 < 0)
		x1 = 0;
	if (x2 > (TRACK_B_H))
		x2 = TRACK_B_H;

	if (r.top >= 0) {
		while(y1 <= y2) {
			dx = x2 - x1;
			base = (char *)the_off->Bits() + y1*rowbyte+x1;
			y1++;

			while((dx >= 0) && ( ((long)base) & 0x03)) {
				*base++ = c;
				dx--;
			}		

			while(dx >= 4) {
				*((ulong *)base) = tmp;
				base += 4;
				dx -= 4;
			}

			while(dx>=0) {
				*base++ = c;
				dx--;
			}
		}
	}
}


//-------------------------------------------------------------------------

char	dark_table_inited = 0;
uchar	dark_table[256];

//-------------------------------------------------------------------------

void	init_dark_table()
{
	ushort		v;
	uchar		ov;
	rgb_color	c;
	BScreen		s;

	for (v = 0; v < 256; v++) {
		c = s.ColorForIndex(v);	
		c.red = (int)(c.red*0.75);
		c.green = (int)(c.green*0.75);
		c.blue = (int)(c.blue*0.75);
		
		ov = s.IndexForColor(c);
		dark_table[v] = ov;
	}
	dark_table_inited = 1;
}

//-------------------------------------------------------------------------

void	TrackViewer::darken_rect(BRect r)
{
	long	rowbyte = TRACK_B_H;
	char 	*base;
	long	y1,y2;	
	long	x1,x2;
	long	dx,dy;
	
	if (dark_table_inited == 0) {
		init_dark_table();
	}

	y1 = (int)r.top;
	y2 = (int)r.bottom;
	if (r.right < r.left)
		return;

	if (y1 >= TRACK_HEIGHT)
		return;

	if (y2 < 0)
		return;

	
	
	if (y1 < 0) {
		y1 = 0;
	}

	if (y2 > (TRACK_HEIGHT - 1)) {
		y2 = TRACK_HEIGHT - 1;
	}

	dy = (y2-y1);

	x1 = (int)(r.left);
	x2 = (int)(r.right);

	if (x1 > (TRACK_B_H))
		return;
	if (x2 < 0)
		return;

	if (x1 < 0)
		x1 = 0;
	if (x2 > (TRACK_B_H))
		x2 = TRACK_B_H;

	if (r.top >= 0) {
		while(y1 <= y2) {
			dx = x2 - x1;
			base = (char *)the_off->Bits() + y1*rowbyte+x1;
			y1++;

			while(dx>=0) {
				*base = dark_table[*base];
				base++;
				dx--;
			}
		}
	}
}

