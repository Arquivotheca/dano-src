
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

#ifndef	PTRANSPORT
#define	PTRANSPORT

#include "snd_src.h"
#include "top_view.h"
/*------------------------------------------------------------*/

#define	TRANSPORT_H_SIZE	300
#define	TRANSPORT_V_SIZE	300

#define	TRANSPORT_H_POS		0
#define	TRANSPORT_V_POS		0

/*------------------------------------------------------------*/
class	TopView;

class	Transport : public BView {
public:
		BBitmap		*the_bitmap;
		BBitmap		*up_on;
		BBitmap		*down_on;
		BBitmap		*main;
		BBitmap		*circle;
		BBitmap		*yellow;
		BView		*mv;
		BView		*junk;
		long		wave_color_index;
		float		yellow_pos;
		char		do_anim;
		char		need_update;
		SndSrc		*the_source;
		TopView		*dad;

						Transport(BRect r, long flags, TopView *vdad);
virtual					~Transport();
virtual		void		Draw(BRect);
virtual		void		MouseDown(BPoint where);
virtual		void		Pulse();
			void		SetSource(SndSrc *src);

			void		BuildGraphics();
			long		find_object(BPoint where);
			void		copy_to_main(BBitmap *src, long obj_code);
			BPicture	*make_mask(long code);
			void		fline(long x1,long y1,long x2, long y2, ulong color);
			void		DrawWave(float delta, long wave_color_index);
			void		init_colors();
			void		AnimateColor();
			void		DrawYellow();
			float		PosToAngle(BPoint p);
			void		norm(float *p1, float *p2);
			void		TrackYellow();
			void		MoveYellow(float a0);
			void		xGetMouse(BPoint *where, ulong *but);
			BBitmap		*get_image();

			char		NeedUpdate();
};

/*------------------------------------------------------------*/

extern "C"	BBitmap		*load(char *filename);


#endif
