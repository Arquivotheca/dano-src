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
#include <math.h>


#ifndef	SHAPE_H
#define	SHAPE_H

#ifndef	TSB
#include "tsb.h"
#endif

/*------------------------------------------------------------*/

#define		MAX_POINT	500
#define		MAX_S		500

/*------------------------------------------------------------*/


class	TShape {
public:
		TShowBit	*owner;
		long		num_point;
		long		num_poly;

		long		spt_x[MAX_POINT];
		long		spt_y[MAX_POINT];

		long		pt_x[MAX_POINT];
		long		pt_y[MAX_POINT];
		long		pt_z[MAX_POINT];

		long		screen_x[MAX_POINT];
		long		screen_y[MAX_POINT];
		long		screen_z[MAX_POINT];

		long		l1[MAX_S];	
		long		l2[MAX_S];	
		long		l3[MAX_S];	
		long		l4[MAX_S];	
		long		acolor[MAX_S];
		long		zs[MAX_S];

		float		cur_alpha;
		float		cur_delta;
		float		cur_zeta;

					TShape(TShowBit *aowner);
virtual				~TShape();
		void		change_view_point(float new_alpha, float new_delta, float new_zeta);
		void		add_point(long x, long y, long z);
		void		add_poly(long p1, long p2, long p3, long p4, long color);
		long		calc_mid(long pn);
		void		sort_polys();
		void		draw(float new_alpha, float new_delta, float new_zeta, BRect *a_rect);
		long		calc_color(long i);
};

/*------------------------------------------------------------*/

class	TBird : public TShape {
public:
		long		end_wing1_ref;
		long		end_wing2_ref;
		long		wing_phase;

					TBird(TShowBit *aowner, long cx, long cy, long cz);
		void		body(long cx, long cy, long cz);
		long		wing(long cx, long cy, long cz, long wlen);
		void		flap_flap();
};

/*------------------------------------------------------------*/

class	TCube : public TShape {
public:
					TCube(TShowBit *aowner, long cx, long cy, long cz);
		void		add_cube(long cx, long cy, long cz, long color);
		void		add_pyramid(long cx, long cy, long cz, long color);
		void		explode_out();
		void		explode_in();
		void		new_one();
};

/*------------------------------------------------------------*/
#endif
