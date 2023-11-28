#include "app.h"
#include "window.h"
#include "offscreen.h"
#include "view.h"
#include "menubar.h"
#include "check_box.h"
#include "gr_types.h"
#include "scroll_bar.h"
#include "imagebuffer.h"
#include "proto.h"
#include "scroll_view.h"
#include <math.h>


#ifndef	SHAPE_H
#define	SHAPE_H

#ifndef	TSB
#include "tsb.h"
#endif

/*------------------------------------------------------------*/

#define		MAX_POINT	450
#define		MAX_S		450

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

		void		TShape(TShowBit *aowner);
virtual				~TShape();
		void		change_view_point(float new_alpha, float new_delta, float new_zeta);
		void		add_point(long x, long y, long z);
		void		add_poly(long p1, long p2, long p3, long p4, long color);
		long		calc_mid(long pn);
		void		sort_polys();
		void		draw(float new_alpha, float new_delta, float new_zeta);
		long		calc_color(long i);
};

/*------------------------------------------------------------*/

class	TCube : public TShape {
public:
		void		TCube(TShowBit *aowner, long cx, long cy, long cz);
		void		add_cube(long cx, long cy, long cz);
		void		explode_out();
		void		explode_in();
		void		new_one();
};

/*------------------------------------------------------------*/
#endif
