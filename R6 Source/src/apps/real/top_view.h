
#ifndef	_VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _PICTURE_H
#include <Picture.h>
#endif


#include <math.h>

#ifndef	TOPVIEW
#define	TOPVIEW

#include "transport.h"
/*------------------------------------------------------------*/

#define	NORMAL_ANIM	1
#define	MOVE_UP		2
#define	MOVE_DOWN	3

/*------------------------------------------------------------*/

#define	NOWHERE		-1
#define	TRANSPORT	1



/*------------------------------------------------------------*/
class	Transport;
/*------------------------------------------------------------*/

class	TopView : public BView {
public:
		BBitmap		*the_bitmap;
		BBitmap		*logo;
		BView		*mv;

		Transport	*v_transport;
		
		char		mode;
		float		small_alpha;
		float		small_ellipse_alpha;

		long		big_dh;
		long		big_dv;

						TopView(BRect r, long flags);
virtual					~TopView();
virtual		void		Draw(BRect);
virtual		void		MouseDown(BPoint where);
			long		FindPart(BPoint where);
			void		DrawContent();
			void		DrawBits();
			void		UpdateTransport();
			void		thread();

			BPoint		GetSmallPos();
			BPoint		GetMiniPos(long idx);
			BPoint		GetBigPos();

			BRect		GetSmallRect();
			BRect		GetBigRect();

			void		hline(long x1, long x2, long y1, ulong c, float d);
			void		do_ellipse(long cx, long cy, ulong c);
			void		do_mini_ellipse(long cx, long cy, ulong c);

			void		tGetMouse(BPoint *where, ulong *but);
};

/*------------------------------------------------------------*/

#endif
