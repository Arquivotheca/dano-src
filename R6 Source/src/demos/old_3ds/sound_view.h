#ifndef _VIEW_H
#include <View.h>
#endif
#include "channel.h"
#include <Bitmap.h>
#include <R3MediaDefs.h>

#include <AudioStream.h>
#include <BufferMsgs.h>
#include <BufferStream.h>
#include <BufferStreamManager.h>
#include <MediaDefs.h>
#include <SoundFile.h>
#include <Subscriber.h>


// time redraw modes:
//

#define	FULL_DRAW	0x01
#define	TOP_ONLY	0x02
#define	CENTER_ONLY	0x03


#define	MIX_VIEW	0x01
#define	WAVE_VIEW	0x02
#define	BOTH		0x03

typedef	struct {
	float	x1;
	float	y1;
	float	z1;
	float	x2;
	float	y2;
	float	z2;
	float	sx1;
	float	sy1;
	float	sz1;
	float	sx2;
	float	sy2;
	float	sz2;
	long	color;
} vector;

typedef	struct {
	float	x1;
	float	y1;
	float	z1;
	float	sx1;
	float	sy1;
	float	sz1;
	long	object_id;
	ulong	part;
	char	touched;
} point;

typedef	struct {
	long	pt1_index;
	long	pt2_index;
	long	pt3_index;
	long	pt4_index;
	long	color_index;
	long	object_id;
} poly;


class	WaveViewer;

class TSoundView : public BView {

friend	WaveViewer;

public:
 	 				TSoundView (BRect r);
  					~TSoundView();
virtual  	void	Draw(BRect r);
virtual		void	MouseDown(BPoint where);
			void	fline(long x1,long y1,long x2, long y2, char *c);
			void	anti_fline(long x1,long y1,long x2, long y2, char *c);
			void	change_view_point(float new_alpha, float new_delta, float new_zeta);
			void	change_view_point_poly(float new_alpha, float new_delta, float new_zeta);
			void	draw_vectors();
			void	SetChannelColor(long ch, long color);
			void	fill_4(BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d, char *mapper);
			void	fill_rand(BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d, char *mapper);
			int		add_ppoint(float x1, float y1, float z1, long obj_id, ulong part=0);
			void	add_poly(long i1, long i2, long i3, long i4, long c, long obj_id);
			void	render_polys();
			void	move_obj(float k);
			void	MovePoly(long id, float dx, float dy, float dz);
			char	test_hit(BPoint hit, BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d);
			long	find_hit(BPoint where);
			void	handle_hit(long hit, BPoint where);
			void	draw_labels();
			void	find_space(long h, long *hp, long *vp);
			void	BotSize(long id, float delta);

			long	window_control_task();
			void	SizePoly(long id, ulong type, long vs);
			void	Animate();
			void	SetChannelLevel(long c, float v);
			void	do_anim();
			void	recalc_pos_xy();
			void	add_vector(float x1, float y1, float z1,
								float x2, float y2, float z2, long map = -1);
			void	Draw1(BView *off);
			void	Draw2(BView *off);
			void	Draw1(BView *off, long c_id);
			void	DrawStringRight(BView *off, BPoint where, char *s);
			char	has_selection();
			void	update_waves();
			void	transform(float *x, float *y);

			void	composit_pannel1();
			void	composit_pannel2();

virtual		void 	KeyDown(const char *key, int32 count);
			void	move_selection(float dx, float dy);
			void	mute_selection();
			void	solo_selection();
			void	ScalePoly(long id, ulong atype, float scale);
			void	mix(Channel **p, long n, long step);
			void	select_all();
			void	calc_poly_range(long *x1, long *y1, long *x2, long *y2);
			void	vertical_move(long hit, BPoint where);
			void	add_halo(float size_x, float size_y, long id);
			float	get_reverbe_level(long c);
			void	track_pannel1();
			void	track_pannel2();
			void	update_infos();
			void	UpdateTime(BView *off);
			char	pannel2_click(BPoint where0);
			void	DrawButtons(BView *off);
			void	handle_button(long i);
			void	do_wave();
			void	setup_waves();
			void	remove_waves();
			void	handle_wave_click(BPoint where);
			void	do_size(long i);
			void	change_scale(long ds);
			void	deselect_all();
			void	upd_revs(long n, Channel **p);

			
			float	pos_x[512];
			float	pos_y[512];
private:
			void	sort_polys();
			float	calc_mid(long pn);
			long	last_obj_select;
			char	space[16][8];
			BBitmap	*the_bits;
			
			BBitmap	*pannel1;
			BView	*pan1_view;
			BRect	pannel1_rect;
			
			BBitmap	*pannel2;
			BView	*pan2_view;
			BRect	pannel2_rect;
			
			char	*the_base;
			BView	*off_view;
			float	cur_alpha;
			float	cur_delta;
			float	cur_zeta;
			float	a,b,c;
			long	bm_hp;
			float	dst;
			char	obj_state[128];
			char	obj_muted[128];
			char	obj_solo[128];
			long	vector_count;
			vector	varray[1024];
			long	ppoint_count;
			point	poly_point[512];
			poly	polys[128];
			long	poly_count;
			
			char	white_map[32];
			char	blue_map[32];
			char	green_map[32];
			char	level_map[20][32];
			
			float	vpx,vpy,vpz;
			long	size_x;
			long	size_y;
			char	mapper[16][256];
			float	channel_reverbe[128];
			long	ready_sem;
			double	mtime;
			float	dtime;
			float	max_time;
			float	main_t;
			Channel	*chan[32];
			char	need_sort;
			char	need_vector;
			char	no_name;
			BRect	clip_rect;
			float	lt[16];
			long	tv[11];
			float	lrev[32];
			float	rrev[32];
			long	time_mode;
			char	b1_state[3];
			BRect	b1_rect[3];
			char	pause_main;
			long	off_y;
			char	view_state;
			long	quit;
			WaveViewer *
					wave_viewers[32];
			long	wave_count;
			char	demo_mode;
			uchar	composit[65536];
};



class	WaveViewer {
public:
					WaveViewer(Channel *a_channel, TSoundView *owner, BRect bound);
					~WaveViewer();
			void	Draw();
			void	Move(long new_x, long new_y);
			void	UpdateMe();
			long	w_map(long v);
			void	change_scale(long ds);

			void	fline(long xx1, long yy1, long xx2, long yy2, uchar c);
			void	ffline(long x1,long y1,long x2, long y2, uchar c);
			BRect	bound;
			float	imult;
			long	scale;
private:

		TSoundView	*owner;
		Channel		*channel;
		BView		*off_view;
		BBitmap		*off;
		
};
