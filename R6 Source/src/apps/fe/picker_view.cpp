#include "picker_view.h"
#include <stdio.h>
#include <Region.h>
#include <File.h>
#include "twindow.h"

PickView::PickView(BRect rect, char *name)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	cur = 0;
}


void PickView::Draw(BRect)
{
	BRect	r;
	long	i;
	long	c;

	for (i = 0; i < 8; i++) {
		c = (255*(7-i))/7;
		r.top = 0;
		r.bottom = 32;
		r.left = 20*i;
		r.right = (20*(i+1));
		SetHighColor(c,c,c);
		FillRect(r);
		if (i != cur) {
			SetHighColor(255,0,0);
			StrokeRect(r);
		}
		else {
			SetHighColor(0,0,255);
			StrokeRect(r);
			r.InsetBy(1,1);
			StrokeRect(r);
			r.InsetBy(1,1);
			StrokeRect(r);
			r.InsetBy(-2,-2);
		}
	}
}


void	PickView::MouseDown(BPoint where)
{
	cur = where.x/20;
	if (cur > 7) cur = 7;
	Draw(BRect(0,0,0,0));
}










