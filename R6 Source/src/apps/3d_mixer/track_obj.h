#ifndef TRACK_OBJ_H
#define TRACK_OBJ_H

#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif
#include <stdio.h>

//------------------------------------------------
class	TrackViewer;
class	EditWindow;
//------------------------------------------------

#define	GENERIC_OBJECT	0
#define	SIMPLE_OBJECT	1
#define	SILENCE_OBJECT	2


//------------------------------------------------
#define	MIN_LOOP	0.01
//------------------------------------------------

//------------------------------------------------

class	SampleCache {
public:

						SampleCache(const char *path, const char *name);
						SampleCache(long ref);
						SampleCache();
virtual					~SampleCache();	
virtual	SampleCache		*clone();
		void			GetSample(float t, float dt, short *min, short *max);
		void			Load(long ref);
		void			Save(long ref);
		float			energy(long v1, long v2);
		long			find_zero_cross(long v);
		float			Length();
		float			LengthMaster();
		bool			Resize(float new_length);
		void			compress_hard(float co);
		void			compress_soft(float ratio);
		void			expand_soft(float ratio);
		void			pitch_down_soft(float ratio);
		void			merge_block(short *p, long cnt);
		short			*p_samples();

		char			*name;
		char			*path;

		long			size;
		short			*samples;
		short			*samples_master;
		long			size_master;

		short			*tmp_buf;
		long			cur_write;
};

//------------------------------------------------


class	TrackObject {

public:
		char		selected;
		char		dragged;
		char		ghost;
		double		v_from;
		double		v_to;
		double		v_from_no_grid;
		double		v_to_no_grid;
		double		st_skip;
		TrackViewer	*dad;
		SampleCache	*the_cache;
		double		loop_point;
		double		loop_point_no_grid;

		double		v_from_undo;
		double		v_to_undo;
		double		v_loop_undo;
		double		v_skip_undo;

		double		v_from_undo_ng;
		double		v_to_undo_ng;
		double		v_loop_undo_ng;

public:
				TrackObject();
				TrackObject(TrackViewer *my_dad, float from, float to);
				TrackObject(TrackViewer *my_dad, long ref);
virtual			~TrackObject();
virtual
TrackObject		*clone();
		void	SetDad(TrackViewer *t);
		void	Check();
virtual	void	Edit();
		float	ToGrid(float v);
		void	GetRange(float *from, float *to);
		void	GetRangeNoGrid(float *from, float *to);
virtual	void	Render(float hp, long v_range, float zoom);
virtual	void	PostRender(float hp, long v_range, float zoom);
virtual	void	Dump();
		void	SetLength(float length);
		void	SetPos(float pos);
		float	Length();
		float	BaseLength();
		float	Pos();
		float	LengthNoGrid();
		float	PosNoGrid();
		float	LoopPoint();
		float	LoopPointNoGrid();
		void	SetLoopPoint(float v);
		float	StartSkip();
		char	Ghost();
		void	SetGhost(char g);
		float	SetStartSkip(float v);
		void	Select(char onoff);
		void	SetDrag(char onoff);
		char	Selected();
		void	sync_positions();
virtual	long	get_type();
virtual	char	GetData(short *buffer, long skip, long max_count);
virtual	void	Save(long ref);
virtual	void	Load(long ref);

virtual	void	comit();
virtual	void	undo();
};


//------------------------------------------------

class	SimpleObject : public TrackObject {
public:

		char		*name;
		char		*path;
		EditWindow	*edit;

					SimpleObject();
					SimpleObject(TrackViewer *my_dad, float from, float to, const char *name, const char *path);
					SimpleObject(TrackViewer *my_dad, long ref);
virtual				~SimpleObject();
virtual
TrackObject			*clone();
		float		CalcLength();
virtual	void		Edit();
virtual	void		Render(float hp, long v_range, float zoom);
virtual	void		PostRender(float hp, long v_range, float zoom);
virtual	void		Dump();
virtual	long		get_type();
virtual	char		GetData(short *buffer, long skip, long max_count);
virtual	void		Save(long ref);
virtual	void		Load(long ref);
};


//------------------------------------------------

class	Silence : public TrackObject {
public:

				Silence(TrackViewer *my_dad, float from, float to);
virtual	void	Render(float hp, long v_range, float zoom);
virtual	void	Dump();
virtual	long	get_type();
};

//------------------------------------------------


#endif
