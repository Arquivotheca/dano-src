#ifndef TRACK_VIEW_H
#define TRACK_VIEW_H

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

#ifndef	_TRACK_OBJ_H
#include "track_obj.h"
#endif

#ifndef	_WAVE_WINDOW_H
//#include "wave_window.h"
#endif

#include <TextControl.h>
//------------------------------------------------

#define	CONTENT	0
#define	CORNER	1

#define	MAX_FEEDBACK	32
#define	NEW_BPM			3000
#define	MAX_CLIP		1024
//------------------------------------------------
extern	float	get_ibeat();
class	TSoundView;
//------------------------------------------------

class	THSB : public BScrollBar {
public:
		long	min;
		long	max;
public:
					THSB(char *name, BRect view_bound);
virtual		void	ValueChanged(float v);
			void	UpdateSpeed(float s);
};


class	TrackView;
class	Channel;

//------------------------------------------------

class	TCtrlView : public BView {

	BTextControl	*cur_beat;
	BBitmap			*b;
	BView			*bv;
	float			rot;

public:
				TCtrlView(BRect frame, char *name); 
				~TCtrlView();
virtual	void	MouseDown(BPoint where);
virtual	void	Draw(BRect ur);
		void	txt_msg();
		void	new_bpm(long v);
		void	Save(long ref);
		void	Load(long ref);
		void	SetBPM(float v);
		BPoint	transform(BPoint in);
		void	SetRot(float r);
		void	blure();
};

/*------------------------------------------------------------*/

class	HairLine : public BView {
public:
				HairLine(long h1, long v1, long h2, long v2);
				~HairLine();
};

/*------------------------------------------------------------*/

class	InfoBlock : public BView {
		TrackObject	*tt;
public:
				InfoBlock(BRect r, TrackObject *t);
				~InfoBlock();
		void	Draw(BRect r);
};

/*------------------------------------------------------------*/


class	TimeView : public BView {
public:
		BBitmap	*the_off;
		BView	*off_view;
		float	start;
		float	tpp;
		float	select_start;
		float	select_end;
		float	cur_t;
		long	vsize;
		float	beat_pm;
		long	beat_per_measure;

				TimeView(BRect frame, char *name); 
virtual			~TimeView();
virtual	void	MouseDown(BPoint where);

virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);
virtual	void	ChangeSelection(float new_a, float new_b);
		void	move_sb(float direction);
		void	private_draw(BRect r);
		char	ShowBeat();
		void	SetBeat(double v);
		double	ToGrid(double v);
		double	Beat();
		void	SetBPM(long v);
		void	Save(long ref);
		void	Load(long ref);
		void	draw_cur_time(double t);
		void	Toggle();
		long	time_to_h(double t);
		double	h_to_time(double h);
		void	time_to_string(float t, char *buf);
		void	set_parm(float st, float pp);
		void	vl(long x1,long y1,long y2, uchar c);
};

//------------------------------------------------


class	TrackViewer {
		friend		class TrackView;
public:
		BView		*off_view;
		BBitmap		*the_off;
		
		TrackObject	**obj_array;

private:

		char		*track_name;
		TrackView	*dad;
		char		dirty;
		char		selected;
		long		current_height;
		long		obj_count;
		long		obj_array_size;
		Channel		*the_channel;
		long		last_vp;
public:
					TrackViewer(const char *name, TrackView *main);
					TrackViewer(long ref, TrackView *main);
					~TrackViewer();
		BBitmap		*GetBitmap();
		char		*Name();
		void		SetName(const char *);
		void		SetChannel(Channel *c);
		Channel		*GetChannel();
		int			Selected();
		void		SetSelected(int v);
		void		AddObject(TrackObject *an_object);
		void		InsertAfter(long pos, TrackObject *obj);
		void		sync_positions();
		void		redo_time();
		void		unselect_all();
		long		GetHeight();
		void		SetHeight(long v);
		void		InternalDraw();
		long		find_part(BPoint where);
		long		find_insert(BPoint where);
		void		handle_hit(BPoint where);
		void		ChangeGizmos();
		TrackObject	*find_object(BPoint where, char right_extend = 0);
		char		fill_buffer(short *buffer, long count, long pos);
		void		GetMouse(BPoint *where, ulong *but);
		char		MoveSelection(float dh);

		void		ffline(long x1,long y1,long x2, long y2, uchar c);
		void		fline(long xx1, long yy1, long xx2, long yy2, uchar c);
		void		vdot(long x1,long y1,long y2, uchar c);
		void		vl(long x1,long y1,long y2, uchar c);
		void		frect(BRect r, uchar c);
		void		ffillrect(BRect r, uchar c);
		void		darken_rect(BRect r);
		void		HandleDrop(BPoint where, const char *name, const char *path);
		void		DumpTrack();
		void		delete_selection();
		void		remove_ghosts();
		void		revive_ghosts();
		char		has_selection();
		void		Save(long ref);
		void		Load(long ref);
};


//------------------------------------------------

class TrackView : public BView {

		friend	TrackViewer;
public:
		BBitmap			*xtra_off;
		BView			*xtra_view;
		float			zoom;
		float			last_feedback[MAX_FEEDBACK];
		TSoundView		*my3d;
		TrackObject		*clipboard[MAX_CLIP];
		long			clip_size;
		InfoBlock		*cur_info;
		HairLine		*cur_infohairline1;
		HairLine		*cur_infohairline2;
		HairLine		*cur_infohairline3;
		long			cur_info_pos;
		long			cur_info_pos_base;
		long			last_track_md;
		long			last_index_md;
		TrackObject		*last_click;
		double			click_when;
		char			last_prj_name[2048];
		char			last_prj_path[2048];

				TrackView(BRect frame, char *name, TSoundView *m3d); 
virtual			~TrackView();
virtual	void	MouseDown(BPoint where);

virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);
		void	clear_all();
		void	special_char(char c);
		void	clear_clipboard();
		void	adjust_vsb();
		void	save_named();
		void	undo();
		void	comit();
		void	Copy();
		char	HasClip();
		void	Paste();
		void	Clear();
		void	Cut();
		char	has_selection();
		void	DoBPM();
		void	DoBPM1();
		char	HasChannel();
		char	HasChannelSelected();
		void	Select(long i, long v);
		void	draw_bar(long hp, long vp);
		void	SetZoom(float z);
		void	delete_selection();
		void	do_mute_solo(BRect dst, long i);
		void	draw_track_extra(long track_index, long vp);
		void	SetHPos(float v);
		void	HitInViewer(BPoint view_where, long viewer_index, BRect cur_viewer_frame);
		void	unselect_all();
		void	Refresh();
		void	DeleteChannels();
		void	deselect_others(TrackViewer *me);
		void	redo_mute_solo();
		void	RefreshSize(long index);
		void	FeedBackLine(float h1, TrackViewer *exclude, long linen, rgb_color c);
		void	MoveSelection(float dh);
		void	Refresh(long index);
		void	Refresh(TrackViewer *v);
		void	sync_positions();
		void	RefreshXtra();
		long	find_index(TrackViewer *p);
		float	GetBaseTime();
		void	do_handle_drop(BPoint where, const char *name, const char *path);
		bool	MessageDropped(BMessage *inMessage, BPoint where, BPoint offset);
		void	MessageReceived(BMessage *message);
		void	TakeRef(entry_ref ref);
		void	new_track();
		void	DropTrack(BPoint where, const char *path, const char *name);
		char	TryDropInTrack(BPoint where, const char *name, const char *path);
		void	txt_msg();
		void	Save(char *path, char *name);
		void	Save();
		void	LoadMaster(const char *p);
		void	SaveMaster(const char *path, const char *master_name);
		char	is_track(const char *path, const char *name);
		char	is_master(const char *path, const char *name);
		void	DropFolder(BPoint where, const char *path, const char *name);
		char	is_folder(const char *path, const char *name);
		void	SetInfo(BPoint p, TrackObject *tt);
		void	RemoveInfo();
		void	MoveInfo(long new_h);
virtual	void	KeyDown(const char *key, int32 count);
private:
		void		do_text(BRect r, long ct);
		void		GMouse(BPoint *w, ulong *buttons, TrackViewer *me);
		long		nv;
		float		base_time;
		BTextControl*cur_text;
		int			cur_text_index;
		TrackViewer
					*viewers[16];

};

//------------------------------------------------

class ZoomControl : public BView {
public:
		BBitmap	*off;
		BView	*off_view;
		long	zlevel;

				ZoomControl(BRect r);
virtual			~ZoomControl();
virtual	void	MouseDown(BPoint where);
virtual	void	Draw(BRect updateRect);
		void	draw_internal(BView *v);
virtual	void	HandleValue(float v);
};

//------------------------------------------------

class LineView : public BView {
public:
				LineView(BRect r);
virtual			~LineView();
virtual	void	Draw(BRect updateRect);
};

//------------------------------------------------

class ToolView : public BView {
public:
				ToolView(BRect r);
virtual			~ToolView();
virtual	void	Draw(BRect updateRect);
};

//------------------------------------------------

void	fInvertRect(BBitmap *the_off, BRect r);
void	fDimRect(BBitmap *the_off, BRect r);

//------------------------------------------------
#endif
