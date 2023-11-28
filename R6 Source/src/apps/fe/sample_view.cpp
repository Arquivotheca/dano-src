#include "sample_view.h"
#include "main.h"
#include <stdio.h>
#include <Region.h>
#include <Debug.h>
#include "twindow.h"

//---------------------------------------------------------

SampleView::SampleView(BRect rect, char *name, FontMachine *f)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect	r;

	fm = f;
	r.top = 0;
	r.left = 0;
	r.right = 467;
	r.bottom = 80;
	the_off = new BBitmap(r, B_COLOR_8_BIT);

	cur_test = 0;
	memset(test_string, 0xff, 32*sizeof(uint16));
}

//---------------------------------------------------------

void	SampleView::clear_off()
{
	long	i;
	long	zero = 0x3f3f3f3f;
	long	*p;

	p = (long *)the_off->Bits();
	for (i = 0; i < (468*80)/16; i++) {	
		*p++ = zero;
		*p++ = zero;
		*p++ = zero;
		*p++ = zero;
	}
}

//---------------------------------------------------------

static uchar color_table[8] = { 63, 27, 22, 18, 13, 9, 4, 0 };

void SampleView::plot(int h, int v, fc_char *ch) {
	int       i, j, val, dx, row;
	char      *base;

	base = (char*)the_off->Bits();
	row = the_off->BytesPerRow();
	
	dx = (ch->bbox.right-ch->bbox.left+2)>>1;
	for (i=0; i<=(ch->bbox.right-ch->bbox.left); i++)
		for (j=0; j<=(ch->bbox.bottom-ch->bbox.top); j++) {
			val = ch->bitmap[j*dx+(i>>1)];
			val = (val>>(((i+1)&1)<<2))&7;
			base[(v+j+ch->bbox.top)*row+(h+i)] = color_table[val];
		}	
}

//---------------------------------------------------------

void SampleView::draw_string(char *p) {
	int 	      i, step, index;	
	uint32        count_char;
	uint32        *list_code;
	fc_char       *ch;
	fc_char       **list_char;
	FontMachine   *fm;

	/* get font description */
	fm = ((TApplication*)be_app)->bm_view->fm;
	list_code = fm->list_code;
	list_char = fm->list_char;
	count_char = fm->count_char;

	if (list_char == 0L)
		return;
	
	/* redraw the good lines */
	while (p[0] != 0) {
		index = p[0];

		i = (count_char+1)>>1;
		step = (i+1)>>1;
		while (step >= 0) {
			if (list_code[i] == index) {
				ch = list_char[i];
				plot(x, y, ch);
				if (ch->bbox.right >= ch->bbox.left)
					x += ch->bbox.right-ch->bbox.left+2;
				else
					x += (int)(ch->escape.x_escape+0.5);
				break;
			}
			if (list_code[i] > index) {
				i -= step;
				if (i<0) i = 0;
			}
			else {
				i += step;
				if (i>=count_char) i = count_char-1;
			}
			if (step < 2) step--;
			else step = (step+1)>>1;
		}
		p++;
	}
}

//---------------------------------------------------------

void SampleView::draw_test_string() {
	int 	      i;	
	uint32        count_char;
	fc_char       *ch;
	fc_char       **list_char;
	FontMachine   *fm;

	/* get font description */
	fm = ((TApplication*)be_app)->bm_view->fm;
	list_char = fm->list_char;
	count_char = fm->count_char;

	if (list_char == 0L)
		return;
	
	/* redraw the good lines */
	for (i=0; i<32; i++)
		if (test_string[i] != 0xffff) {
			ch = list_char[test_string[i]];			
			plot(x, y, ch);
			if (ch->bbox.right >= ch->bbox.left)
				x += ch->bbox.right-ch->bbox.left+2;
			else
				x += (int)(ch->escape.x_escape+0.5);
		}
}

//---------------------------------------------------------

void SampleView::render()
{
	clear_off();
	x = 5;
	y = 18;
	draw_string("the quick brown fox jumped over the lazy dogs");
	x = 5;
	y = 37;
	draw_string("THE QUICK BROWN FOX JUMPED OVER THE LAZY DOGS");
	x = 5;
	y = 56;
	draw_string("0123456789!@#$%^&*()-+=\|[]{};:'/?><,\".~`");
	x = 5;
	y = 75;
	draw_test_string();
}

//---------------------------------------------------------

void SampleView::add_test_char(uint16 code, bool redraw) {
	if (cur_test == 31) {
		memmove(test_string, test_string+1, 31*sizeof(uint16));
		test_string[31] = code;
	}
	else
		test_string[cur_test++] = code;
	if (redraw)
		Draw(Bounds());
}

//---------------------------------------------------------

void SampleView::Draw(BRect r)
{	
	BRect	tmp;
	double	t0;
	double	t1;
	
	render();
	tmp = Bounds();
	StrokeRect(tmp);
	DrawBitmap(the_off, BPoint(1,1));
}
















