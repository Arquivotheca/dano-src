#ifndef STAT_VIEW_H
#define STAT_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif

#include <stdio.h>

#ifndef	_SCROLL_BAR_H
#include <ScrollBar.h>
#endif

#include "gslider.h"

class	StatusView	: public BView {
public:
					StatusView(BRect frame, char *name); 
					~StatusView();
virtual	void		MouseDown(BPoint where);
virtual	void		Draw(BRect ur);
		void		drawer();
		rgb_color	calc_color(long i);
		void		SetLevel(float v1, float v2);
private:
		float		level1;
		float		level2;
		float		max1;
		float		max2;
		long		phase;
		TSlider		*slider;
		long		dv;
};

#endif
