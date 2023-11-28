
#ifndef _VIEW_H
#include <View.h>
#endif
#include "channel.h"
#include <Bitmap.h>
#include <Midi.h>
#include <MidiPort.h>
#include <SoundFile.h>
#include "controller.h"

// time redraw modes:
//

#ifndef	SOUND_VIEW_H
#define	SOUND_VIEW_H

#define	FULL_DRAW	0x01
#define	TOP_ONLY	0x02
#define	CENTER_ONLY	0x03


#define	MIX_VIEW	0x01
#define	WAVE_VIEW	0x02
#define	BOTH		0x03

#define	p1_x	80
#define	p1_y	600

#define	p2_x	120
#define	p2_y	250

//---------------------------------------------------------------------

#define	MAX_CHANNEL	48
#define	NP			2048
//---------------------------------------------------------------------


#define	BVP	(206+50)

#define	HS	1024
#define	VS	480

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

typedef	struct {
	float	x;
	float	y;
	float	z;
	//float	sx;
	//float	sy;
	short	sx;
	short	sy;
	float	dx,dy,dz;
	char	color;
} particule;


long	normalize(float v);


class TSoundView : public BView {


public:
 	 					TSoundView (BRect r);
  						~TSoundView();
			void		SetTrackView(TrackView *t);
virtual  	void		Draw(BRect r);
virtual		void		MouseDown(BPoint where);
			void		fline(long x1,long y1,long x2, long y2, char *c, char anti);
			void		anti_fline(long x1,long y1,long x2, long y2, char *c);
			void		no_anti_fline(long x1,long y1,long x2, long y2, char *c);
			void		change_view_point(float new_alpha, float new_delta, float new_zeta);
			void		change_view_point_poly(float new_alpha, float new_delta, float new_zeta);
			void		draw_vectors();
			void		SetChannelColor(long ch, long color);
			void		fill_4(BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d, char *mapper);
			void		fill_rand(BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d, char *mapper);
			int			add_ppoint(float x1, float y1, float z1, long obj_id, ulong part=0);
			void		add_poly(long i1, long i2, long i3, long i4, long c, long obj_id);
			void		render_polys();
			void		move_obj(float k);
			char		do_object();
			void		MovePoly(long id, float dx, float dy, float dz);
			char		test_hit(BPoint hit, BPoint pt_a, BPoint pt_b, BPoint pt_c, BPoint pt_d);
			long		find_hit(BPoint where);
			void		handle_hit(long hit, BPoint where);
			void		draw_labels();
			void		find_space(long h, long *hp, long *vp);
			void		BotSize(long id, float delta);
			long		Count();
			long		window_control_task();
			void		SizePoly(long id, ulong type, long vs);
			void		Animate();
			void		Animate(BRect rr);
			void		SetChannelLevel(long c, float v);
			void		do_anim();
			void		recalc_pos_xy();
			void		add_vector(float x1, float y1, float z1,
								   float x2, float y2, float z2, long map = -1);
			void		Draw1(BView *off);
			void		Draw2(BView *off);
			void		Draw1(BView *off, long c_id);
			void		DrawStringRight(BView *off, BPoint where, char *s);
			char		has_selection();
			void		transform(float *x, float *y);

			void		composit_pannel1();
			void		composit_pannel2();
			void		remove_channel(Channel *c);
			void		delete_channel(long channel_id);
			void		delete_channels();
			void		delete_obj(long obj_id);
			void		remap_id(long old_one, long new_one);

virtual		void 		KeyDown(const char *key, int32 count);
			void		move_selection(float dx, float dy, char draw = 1);
			void		mute_selection();
			void		toggle_mute(long c);
			void		solo_selection();
			void		ScalePoly(long id, ulong atype, float scale);
			void		mix(Channel **p, long n, long step);
			long		final_mix();
			void		w_mix(Channel **p, long n, long step);
			void		bi_mix(Channel **p, long n, long step);
			void		select_all();
			void		calc_poly_range(long *x1, long *y1, long *x2, long *y2);
			void		vertical_move(long hit, BPoint where);
			void		add_halo(float size_x, float size_y, long id);
			float		get_reverbe_level(long c);
			void		track_pannel1();
			void		track_pannel2();
			void		update_infos();
			void		UpdateTime(BView *off);
			char		pannel2_click(BPoint where0);
			void		DrawButtons(BView *off);
			void		handle_button(long i);
			void		deselect_all();
			void		select(Channel *c, char yesno);
			void		upd_revs(long n, Channel **p);
			void		GetMuteSolo(Channel *c, char *mute, char *solo);
			void		SetMuteSolo(Channel *c, char mute, char solo);
virtual		void		MessageReceived(BMessage *message);
			bool		MessageDropped(BMessage *inMessage, BPoint where, BPoint offset);
virtual		void		MouseMoved(BPoint, uint32, const BMessage *);
			void		invert_map(float sx, float sy, float *obj_x, float *obj_y);
			void		do_add_channel(char *name, TrackViewer *viewer);
			void		do_add_channel(Channel *c, float x, float y);
			void		new_block(long i);
			void		do_text(BRect r);
			void		GetChannelPos(Channel *c, float *x, float *y);
			void		SetChannelPos(long c, float v);
			void		clear();
			void		BuildMappers();
			void		move_alpha(float a);
			void		change_view_point_particule(float new_alpha, float new_delta, float new_zeta);
			void		draw_particules();
			void		move_pixels();
			void		recycle_pixel(long i);
			void		do_final_mix(char *p);

			float		pos_x[512];
			float		pos_y[512];
			char		pause_main;
			char		f_final_mix;
			BSoundFile	*output;
private:
			void		sort_polys();
			float		calc_mid(long pn);
			long		last_obj_select;
			char		space[MAX_CHANNEL][8];
			float		level[MAX_CHANNEL];
			BBitmap		*the_bits;
			
			BBitmap		*pannel1;
			BView		*pan1_view;
			BRect		pannel1_rect;
			
			BBitmap		*pannel2;
			BView		*pan2_view;
			BRect		pannel2_rect;
			TrackView	*trak;
			char		*the_base;
			BView		*off_view;
			float		cur_alpha;
			float		cur_delta;
			float		cur_zeta;
			float		a,b,c;
			long		bm_hp;
			float		dst;
			char		obj_state[128];
			char		obj_muted[128];
			char		obj_solo[128];
			char		obj_silent[128];
			char		obj_hilite[128];
			float		lrev[MAX_CHANNEL];
			float		rrev[MAX_CHANNEL];
			long		vector_count;
			vector		varray[1024];
			long		ppoint_count;
			long		ppoint_hint;
			point		poly_point[4096];
			poly		polys[1024];
			long		poly_count;
			int			w2;
			char		white_map[32];
			char		blue_map[32];
			char		green_map[32];
			char		red_map[32];
			char		level_map[20][32];
			
			float		vpx,vpy,vpz;
			long		size_x;
			long		size_y;
			char		mapper[32][256];
			float		channel_reverbe[128];
			long		ready_sem;
			double		mtime;
			float		dtime;
			float		max_time;
			float		cur_time;
			float		main_t;
			Channel		*chan[MAX_CHANNEL];
			char		need_sort;
			char		need_vector;
			char		no_name;
			BRect		clip_rect;
			float		lt[MAX_CHANNEL];
			long		tv[11];
			long		time_mode;
			char		b1_state[3];
			BRect		b1_rect[3];
			long		off_y;
			char		view_state;
			long		quit;
			long		channel_list_sem;
			char		demo_mode;
			char		jump_mode;
			float		da,db,dc;
			float		tag_x;
			float		tag_y;
			long		NC;
			long		phase;
			char		in_move;				//if currently dragging the grid, set to 1
			long		min_y, max_y;
			long		o_min_y, o_max_y;
			long		min_x, max_x;
			long		o_min_x, o_max_x;
			float		level1, level2;
			char		do_snow;
			particule	pixels[NP];
			uchar		composit[65536];
			Controller	*mController;
};


#endif

