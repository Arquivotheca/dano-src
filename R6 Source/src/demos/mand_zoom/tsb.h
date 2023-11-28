
#ifndef	_VIEW_H
#include <View.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif

#include <math.h>

#ifndef	TSB
#define	TSB

/*------------------------------------------------------------*/

#define	size_x	256
#define	size_y	256

/*------------------------------------------------------------*/

class	TShowBit : public BView {
public:
		BBitmap		*the_bitmap;
		bool		busy;
		bool		exit_now;
		BRect		selection;
		double		px;
		double		py;
		double		scale;
		long		iter;
		int			lock1;
		int			lock2;
		double		px_map;
		double		py_map;
		double		pscale_map;
		double		px_exact[size_x];
		double		py_exact[size_y];
		double		px_actual[size_x];
		double		py_actual[size_y];
		short		remap_x[size_x];
		short		remap_y[size_y];
		short		foo[100];

						TShowBit(BRect r, long flags);
virtual					~TShowBit();
virtual		void		Draw(BRect);
virtual		void		MouseDown(BPoint where);
virtual		void		Pulse();
			void		mand(double vx, double vy, double sx, double sy);
			void		init_mand(double vx, double vy, double sx, double sy);
			long		limit_v(long v);
			long		limit_h(long h);
			BRect		sort_rect(BRect *aRect);
			void		clip(long *h, long *v);
			void		change_selection(long h, long v);
			char		has_selection();
			void		redraw_mand();
			void		calc_mand(double vx, double vy, double sx, double sy);
			void		set_palette(long code);
			void		set_iter(long i);
			void		calc_remap(double vx, double vy, double sx, double sy);
			void		calc_mand_delta(double vx, double vy, double sx, double sy);
			void		dcalc1();
			void		dcalc2();

};

/*------------------------------------------------------------*/

#endif
