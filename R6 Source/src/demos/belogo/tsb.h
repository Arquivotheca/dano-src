
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
#include "shape.h"

#include <math.h>

#ifndef	TSB
#define	TSB

/*------------------------------------------------------------*/

#define	size_x	340
#define	size_y	220
#define	max_x	(size_x - 1)
#define	max_y	(size_y - 1)

/*------------------------------------------------------------*/

class	TShowBit : public BView {
public:
		BBitmap		*the_bitmap;
		BBitmap		*bm1;
		BBitmap		*bm2;
		long		bm1_sem;
		long		bm2_sem;

		long		*poly_scratch;
		int		exit_now;

					TShowBit(BRect r, long flags);
virtual				~TShowBit();
virtual		void		Draw(BRect);
virtual		void		MouseDown(BPoint where);
		void		fill_triangle(BPoint pt_a, BPoint pt_b, BPoint pt_c);
		void		fill_4(BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d);
		void		demo();
		void		demo1();
		void		demo2();
		void		blit_task();
};

/*------------------------------------------------------------*/

#endif
