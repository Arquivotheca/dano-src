#include "track_view.h"
#include <Screen.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <Application.h>
#include <Roster.h>
#include <Entry.h>
#include <Path.h>
#include <Screen.h>
#include <File.h>
#include <Menu.h>
#include <MenuBar.h>
#include <ScrollBar.h>
#include <MenuItem.h>
#include "edit_window.h"
#include "track_obj.h"
#include <MediaFile.h>
#include <MediaTrack.h>
#include "fftshift.h"
#include "util.h"
//-------------------------------------------------------------------------
extern	char	fast_display;

//-------------------------------------------------------------------------
void	swap(ushort *p, long cnt);
//-------------------------------------------------------------------------


		TrackObject::TrackObject()
{
	ghost = 0;
}

//-------------------------------------------------------------------------

		TrackObject::TrackObject(TrackViewer *my_dad, float from, float to)
{
	dad = my_dad;

	v_from_no_grid = v_from = from;
	v_to_no_grid = v_to = to;
	st_skip = 0;
	loop_point_no_grid = loop_point = (v_to-v_from);
	ghost = 0;
	comit();
}

//-------------------------------------------------------------------------

TrackObject	*TrackObject::clone()
{
	TrackObject	*t;

	t = new TrackObject();

	t->v_from_no_grid = v_from_no_grid;
	t->v_from = v_from;
	t->v_to_no_grid = v_to_no_grid;
	t->v_to = v_to;
	t->st_skip = st_skip;
	t->loop_point_no_grid = loop_point_no_grid;
	t->loop_point = loop_point;
	t->comit();
	selected = 0;
	dragged = 0;

	return t;
}

//-------------------------------------------------------------------------

void	TrackObject::comit()
{
	v_from_undo = v_from;
	v_to_undo = v_to;
	v_loop_undo = loop_point;
	v_skip_undo = st_skip;

	v_from_undo_ng = v_from_no_grid;
	v_to_undo_ng = v_to_no_grid;
	v_loop_undo_ng = loop_point_no_grid;
}

//-------------------------------------------------------------------------


void	TrackObject::undo()
{
	float	tmp;

#define	ex(v1, v2) {tmp = v1; v1 = v2; v2 = tmp;}

	ex(v_from_undo, v_from);
	ex(v_to_undo, v_to);
	ex(v_loop_undo, loop_point);
	ex(v_skip_undo, st_skip);
	ex(v_from_undo_ng, v_from_no_grid);
	ex(v_to_undo_ng, v_to_no_grid);
	ex(v_loop_undo_ng, loop_point_no_grid);

#undef ex
}


//-------------------------------------------------------------------------

void	TrackObject::SetDad(TrackViewer *t)
{
	dad = t;
}

//-------------------------------------------------------------------------

void	TrackObject::Load(long ref)
{
}

//-------------------------------------------------------------------------

void	TrackObject::Save(long ref)
{
}

//-------------------------------------------------------------------------

char	TrackObject::Ghost()
{
	return ghost;
}

//-------------------------------------------------------------------------

void	TrackObject::SetGhost(char g)
{
	ghost = g;
}

//-------------------------------------------------------------------------

float	TrackObject::StartSkip()
{
	return st_skip;
}

//-------------------------------------------------------------------------

void	TrackObject::Edit()
{
}

//-------------------------------------------------------------------------

void	TrackObject::Check()
{
	if (v_to_no_grid - v_from_no_grid < MIN_LOOP)
		v_to_no_grid = v_from_no_grid + MIN_LOOP;

	if (v_to - v_from < MIN_LOOP)
		v_to = v_from + MIN_LOOP;

	//if (loop_point > (v_to-v_from))
		//loop_point = (v_to-v_from);
	
	if (loop_point < (st_skip + MIN_LOOP)) {
		loop_point_no_grid = loop_point = st_skip + MIN_LOOP;
	}

	//if (loop_point_no_grid > (v_to_no_grid-v_from_no_grid))
		//loop_point_no_grid = (v_to_no_grid-v_from_no_grid);
}

//-------------------------------------------------------------------------

float	TrackObject::SetStartSkip(float v)
{
	float	st0;
	
	st0 = st_skip;

	if (v >= loop_point) {
		v = (loop_point - MIN_LOOP);
	}
	if (v>=BaseLength()) {
		v = BaseLength() - MIN_LOOP;
	}
	
	if (v < 0)
		v = 0;


	st_skip = v;
	Check();
end:;
	return (st0 - st_skip);
}

//-------------------------------------------------------------------------

float	TrackObject::LoopPoint()
{
	return loop_point - st_skip;
}

//-------------------------------------------------------------------------

float	TrackObject::LoopPointNoGrid()
{
	return loop_point_no_grid - st_skip;
}

//-------------------------------------------------------------------------

void	TrackObject::sync_positions()
{
	loop_point_no_grid = loop_point;
	v_from_no_grid = v_from;
	v_to_no_grid = v_to;
}

//-------------------------------------------------------------------------

void	TrackObject::SetLoopPoint(float v)
{
	float	v1;

	v1 = ToGrid(v);


	if (v > Length())
		v = Length();

	v += st_skip;

	if (v < MIN_LOOP)
		v = MIN_LOOP;

	loop_point_no_grid = v;
	

	if (v1 > Length())
		v1 = Length();
	v1 += st_skip;

	if (v1 < MIN_LOOP)
		v1 = MIN_LOOP;
	loop_point = v1;
	Check();
}

//-------------------------------------------------------------------------

		TrackObject::~TrackObject()
{
	delete the_cache;
}


//-------------------------------------------------------------------------

char	TrackObject::GetData(short *buffer, long skip, long max_count)
{
	return 0;
}

//-------------------------------------------------------------------------

long	TrackObject::get_type()
{
	return GENERIC_OBJECT;
}

//-------------------------------------------------------------------------

void	TrackObject::GetRange(float *from, float *to)
{
	*from = v_from;
	*to = v_to;
}

//-------------------------------------------------------------------------

void	TrackObject::GetRangeNoGrid(float *from, float *to)
{
	*from = v_from_no_grid;
	*to = v_to_no_grid;
}

//-------------------------------------------------------------------------

void	TrackObject::SetLength(float length)
{
	if (length < LoopPoint()) {
		SetLoopPoint(length);
	}

	if (modifiers() & B_SHIFT_KEY) {
		the_cache->pitch_down_soft(length / the_cache->LengthMaster());
		length = the_cache->Length();
		SetLoopPoint(length);
	}
	else
	if (modifiers() & B_OPTION_KEY) {
		the_cache->compress_soft(length / the_cache->LengthMaster());
		length = the_cache->Length();
		SetLoopPoint(length);
	}

	v_to_no_grid = v_to = v_from + length;
	v_to = ToGrid(v_to);
	Check();
}

//-------------------------------------------------------------------------

float	TrackObject::BaseLength()
{
	return the_cache->Length();
}


//-------------------------------------------------------------------------

float	TrackObject::LengthNoGrid()
{
	return v_to_no_grid - v_from_no_grid;
}


//-------------------------------------------------------------------------

float	TrackObject::Length()
{
	return v_to - v_from;
}

//-------------------------------------------------------------------------

float	TrackObject::PosNoGrid()
{
	return v_from_no_grid;
}


//-------------------------------------------------------------------------

float	TrackObject::Pos()
{
	return v_from;
}

//-------------------------------------------------------------------------

float	TrackObject::ToGrid(float v)
{
	float	ib = get_ibeat();
	
	if (ib != 0) {

		float	ipos;
		long	int_v;

		ipos = v / ib;
		ipos += 0.5;
		int_v = (int)ipos;	
		v = int_v * ib;
	}

	return v;
}

//-------------------------------------------------------------------------

void	TrackObject::SetPos(float pos)
{
	v_from_no_grid = pos;

	pos = ToGrid(pos);
	v_to_no_grid = v_to = pos + (v_to - v_from);
	v_from = pos;
	Check();
}

//-------------------------------------------------------------------------

char	TrackObject::Selected()
{
	if (ghost)
		return 0;

	return selected;
}

//-------------------------------------------------------------------------

void	TrackObject::Select(char onoff)
{
	selected = onoff;
}


//-------------------------------------------------------------------------

void	TrackObject::SetDrag(char onoff)
{
	dragged = onoff;
}


//-------------------------------------------------------------------------

void	TrackObject::Render(float hp, long v_range, float zoom)
{
}	

//-------------------------------------------------------------------------

void	TrackObject::PostRender(float hp, long v_range, float zoom)
{
}	

//-------------------------------------------------------------------------

void	TrackObject::Dump()
{
}

//-------------------------------------------------------------------------

	SimpleObject::SimpleObject()
{
	edit = 0;
}

//-------------------------------------------------------------------------

TrackObject	*SimpleObject::clone()
{
	SimpleObject	*t;

	t = new SimpleObject();

	t->v_from_no_grid = v_from_no_grid;
	t->v_from = v_from;
	t->v_to_no_grid = v_to_no_grid;
	t->v_to = v_to;
	t->st_skip = st_skip;
	t->loop_point_no_grid = loop_point_no_grid;
	t->loop_point = loop_point;

	t->name = (char *)malloc(strlen(name) + 1);
	strcpy(t->name, name);
	if (path) {
		t->path = (char *)malloc(strlen(path) + 1);
		strcpy(t->path, path);
	}
	else
		t->path = 0;
	
	t->the_cache = the_cache->clone();

	return t;
}

//-------------------------------------------------------------------------


	SimpleObject::SimpleObject(TrackViewer *my_dad, float from, float to, const char *fname, const char *fpath) :
	TrackObject(my_dad, from, to)
{
	float	len;

	edit = 0;

	the_cache = new SampleCache(fpath, fname);

	name = (char *)malloc(strlen(fname) + 1);
	strcpy(name, fname);
	path = (char *)malloc(strlen(fpath) + 1);
	strcpy(path, fpath);
	len = the_cache->Length();
	if (len > 0)
		SetLength(len);

	loop_point_no_grid = loop_point = len;
}

//-------------------------------------------------------------------------

	SimpleObject::SimpleObject(TrackViewer *my_dad, long ref)
	:
	TrackObject(my_dad, 0, 0)
{
	edit = 0;
	name = 0;
	path = 0;
	Load(ref);
	the_cache = new SampleCache(ref);
}

//-------------------------------------------------------------------------


void	SimpleObject::Edit()
{
	if (edit) {
		if (edit->Lock()) {
			edit->Activate();
			edit->Unlock();
			return;
		}
	}
	edit = new EditWindow(BRect(200,200,800, 500), "edit", the_cache);
	edit->Show();
}

//-------------------------------------------------------------------------

void	SimpleObject::Load(long ref)
{
	long	type;	
	long	subtype;	
	float	from, to;
	long	size;

	type = read32(ref);
	subtype = read32(ref);
	size = read32(ref);
	
	if (name)
		free((char *)name);

	name = (char *)malloc(size);
	read(ref, name, size);

	from = readf(ref);
	to = readf(ref);
	st_skip = readf(ref);
	loop_point = readf(ref);
	v_from_no_grid = v_from = from;
	v_to_no_grid = v_to = to;
	loop_point_no_grid = loop_point;
}

//-------------------------------------------------------------------------

void	SimpleObject::Save(long ref)
{
	long	type;	
	long	subtype;	
	char	cr;
	long	size;
	float	from, to;

	type = 'SIMP';
	subtype = 'RAW_';
	write32(ref, type);
	write32(ref, subtype);
	
	size = strlen(name) + 1;

	if (size & 1)
		size++;
	
	write32(ref, size);
	write(ref, name, size);

	GetRange(&from, &to);
	writef(ref, from);
	writef(ref, to);
	writef(ref, st_skip);
	writef(ref, loop_point);

	the_cache->Save(ref);
}

//-------------------------------------------------------------------------

	SimpleObject::~SimpleObject()
{
	if (edit) {
		if (edit->Lock()) {
			edit->Close();
		}
	}

	free((char *)name);
	free((char *)path);
}

//-------------------------------------------------------------------------

void	SimpleObject::Dump()
{
	printf("Obj %f %f |  %s\n", v_from, v_to, name);
}

//-------------------------------------------------------------------------

void	SimpleObject::PostRender(float hp, long v_range, float zoom)
{
}

//-------------------------------------------------------------------------

long	SimpleObject::get_type()
{
	return SIMPLE_OBJECT;
}

//-------------------------------------------------------------------------

char	SimpleObject::GetData(short *buffer, long skip, long max_count)
{
	long	i;
	long	cnt;
	short	*data;
	long	start_sk;
	long	loop;
	long	cache_size;
	long	vc;
	long	to_copy;
	long	to_skip;
	long	vmin;
	float	cnt1;
	char	got_some = 0;
	float	time;
	float	max_t;
	char	exit = 0;

	start_sk = (int)(st_skip * 44100);			//how many samples to skip from start of sound data
	loop = (int)(loop_point * 44100);			//loop point from start of sound data
	loop -= start_sk;							//get loop point in samples from start of actual sound
	
	if (loop < 0)								//loop point cannot be negative...
		loop = 0;

	cnt1 = v_to - v_from;						//total number of samples which could be needed
	cnt1 *= 44100;								//in samples
	cnt1 -= skip;								//we need to skip some since we window the samples

	cnt = max_count;							//the size of the receiving buffer


	data = the_cache->samples;					//get a pointer to the data


	cache_size = the_cache->size;				//and the number of samples in the source cache
	
	data += start_sk;							//offset to data to reflect the skip... that way we can
	cache_size -= start_sk;						//totally ignore the skip stuff
	
	vc = (skip) % loop;							//modulo loop

	time = v_from;
	time += (vc / 44100.0);

	while(cnt>0) {
		if ((vc) >= cache_size) {
			to_skip = (loop - vc);
			if (to_skip > cnt)
				to_skip = cnt;

			buffer += to_skip;
			cnt -= to_skip;
			vc += to_skip;
			time += (to_skip / 44100.0);
		}
		else {
			to_copy = (loop - vc);
			vmin = cache_size - vc;
			if (to_copy > cnt)
				to_copy = cnt;
			if (to_copy > vmin)
				to_copy = vmin;
			max_t = v_to - time;
			max_t *= 44100;
			if (max_t < to_copy) {
				to_copy = (int)max_t;
				exit = 1;
			}

			memcpy(buffer, &data[vc], to_copy * 2);
			got_some = 1;
			vc += to_copy;
			time += (to_copy / 44100.0);
			cnt -= to_copy;
			buffer += to_copy;
			if (exit)
				goto out0;
		}
		if (vc == loop)
			vc = 0;
	}
out0:;

	//printf("last time = %f, parm = %f %f\n", time, v_from, v_to);
	
out:;
	return got_some;
}

//-------------------------------------------------------------------------

void	SimpleObject::Render(float hp, long v_range, float zoom)
{
	BView	*off_view;
	BBitmap	*off_bits;
	BRect	r;
	float	v,v0;
	long	i;
	float	k;
	long	mid;
	char	buf[128];
	float	time;
	short	vmin, vmax;
	float	vscale;
	long	from;
	long	to;
	char	looping;
	long	loop_gizmo;

	off_view = dad->off_view;
	off_bits = dad->the_off;

	r.top = 6;
	r.bottom = v_range - 8;
	r.left = (v_from - hp) / zoom + 1;
	r.right = (v_to - hp) / zoom - 2;
	
	r.right++;

	if (r.left > r.right) {
		r.left-=2;
		r.right+=1;
	}

	if (selected)
		dad->ffillrect(BRect(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1), 20);
	else
		dad->ffillrect(BRect(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1), 25);


	mid = (int)(r.bottom - r.top);
	mid /= 2;
	mid += (int)r.top;

	vscale = (r.bottom - r.top) / 2.5;

	v0 = -1000;
	
	k = 0;
	time = st_skip;

	vscale = vscale /35000;

	from = (int)r.left + 1;
	to = (int)r.right;


	time += zoom*2;
	if (from < 0) {
		time += (zoom * -from);
		from = 0;
	}

	if (to > 1024)
		to = 1024;

	vmin = mid;
	vmax = mid;

	looping = 0;

	for (i = from; i < to; i++) {

		if (time > (loop_point)) {
			looping = 1;
			time -= (loop_point - st_skip);
		}

		the_cache->GetSample(time, zoom, &vmin, &vmax);
		vmin = (int)(-1 + (vmin) * vscale);
		vmax = (int)(1 + (vmax) * vscale);
		vmax += mid;
		vmin += mid;
		time += zoom;

		if (looping) {
			if (vmin != mid) {
				dad->vl(i, vmin, mid, 13);
			}
			dad->vl(i, mid, vmax, 30);
		}
		else {
			if (vmin != mid) {
				dad->vl(i, vmin, mid, 45);
			}
			dad->vl(i, mid, vmax, 72);
		}
	}

	if  ((r.right - r.left) < 8000000) {
		dad->frect(r, 15);
	}
	else {
		dad->fline(r.left, r.top, r.left, r.bottom, 15);
		dad->fline(r.left + 1, r.top, r.left + 1, r.bottom - 1, 10);

		dad->fline(r.left, r.top, r.right, r.top, 15);
		dad->fline(r.left + 1, r.top + 1, r.right - 1, r.top + 1, 10);

		dad->fline(r.left + 2, r.bottom-1, r.right - 1, r.bottom-1, 24);
		dad->fline(r.left + 1, r.bottom, r.right - 1, r.bottom, 24);
		dad->fline(r.right, r.top + 1, r.right, r.bottom, 24);
		dad->fline(r.right-1, r.top + 2, r.right-1, r.bottom, 24);
	}
	
float	sw;
float	bw;

	if ((r.right - r.left) > 16) {
		dad->darken_rect(BRect(r.left + 1, r.top + 1, r.left + 4, r.bottom - 1));
		dad->darken_rect(BRect(r.right - 4, r.top + 1, r.right - 1, r.bottom - 1));
	}


	loop_gizmo = (int)(0.5 + ((v_from + loop_point - st_skip) - hp) / zoom);
	dad->darken_rect(BRect(loop_gizmo, r.top, loop_gizmo, r.bottom - 1));
	dad->darken_rect(BRect(loop_gizmo, r.top, loop_gizmo, r.bottom - 1));
	
	dad->vl(loop_gizmo, r.bottom + 1, r.bottom + 5, 45);
	dad->vl(loop_gizmo-1, r.bottom + 2, r.bottom + 5, 45);
	dad->vl(loop_gizmo+1, r.bottom + 2, r.bottom + 5, 45);
	dad->vl(loop_gizmo-2, r.bottom + 3, r.bottom + 5, 45);
	dad->vl(loop_gizmo+2, r.bottom + 3, r.bottom + 5, 45);
	dad->vl(loop_gizmo-3, r.bottom + 4, r.bottom + 5, 45);
	dad->vl(loop_gizmo+3, r.bottom + 4, r.bottom + 5, 45);

	if (name[0] != 0) {
		bw = r.right - r.left;

		if (bw < 14)
			goto skip;
		if ((r.bottom - r.top) < 8)
			goto skip;

		strcpy(buf, name);

long	vp;		

		if ((r.bottom - r.top) < 11) {
			dad->off_view->SetFontSize(9);
			vp = (int)r.bottom - 1;
		}
		else {
			dad->off_view->SetFontSize(10);
			vp = (int)r.bottom - 3;
		}
		sw = dad->off_view->StringWidth(buf);
		
long	strl;

		strl = strlen(buf);

		while ((bw - sw) < 10) {
			if (strl < 2)		
				goto skip;
			buf[strl] = '.';
			buf[strl+1] = '.';
			buf[strl+2] = 0;
			strl--;
			sw = dad->off_view->StringWidth(buf);
		}

		dad->off_view->SetHighColor(0, 0, 255);
		
long	pos;
		
		pos = (int)(r.left + (bw-sw)/2);
		
		if (pos < 2 && (r.left < 0) && r.right > (sw + 2)) {
			pos = 2;
		}

		if (pos > 760 && (r.right > (760 + sw) && r.left < (760 - 4))) {
			pos = 760;
		}

		dad->off_view->MovePenTo(BPoint(pos, vp));
		dad->off_view->DrawString(buf);

skip:;
	}
}

//-------------------------------------------------------------------------

		Silence::Silence(TrackViewer *my_dad, float from, float to) :
		TrackObject(my_dad, from, to)
{
}

//-------------------------------------------------------------------------

long	Silence::get_type()
{
	return SILENCE_OBJECT;
}

//-------------------------------------------------------------------------


void	Silence::Dump()
{
	printf("silence %f %f\n", v_from, v_to);
}

//-------------------------------------------------------------------------


void	Silence::Render(float hp, long v_range, float zoom)
{
	BBitmap	*off_bits;
	BRect	r;
	uchar	c;

	off_bits = dad->the_off;

	r.bottom = v_range - 7;
	r.top = r.bottom - 2;

	r.left = (v_from - hp) / zoom + 1;
	r.right = (v_to - hp) / zoom - 2;

	if ((r.right - r.left) > 4) {
		r.left+=1;
		r.right-=2;
	}

	c = 57;

	if (selected)
		c = 44;
	dad->fline(r.left, r.bottom, r.right, r.bottom, c);
	dad->fline(r.left, r.bottom, r.left, r.top, c);
	dad->fline(r.right, r.bottom, r.right, r.top, c);
}

//-------------------------------------------------------------------------

void	swap(ushort *p, long cnt)
{
	ushort	tmp;

	while(cnt--) {
		tmp = *p;
		tmp = (tmp>>8) | (tmp&0xff)<<8;
		*p++ = tmp;
	}
}

//-------------------------------------------------------------------------

typedef struct {
	long	form;
	long	size;
	long	sig1;
	long	sig2;
	long	unk1;
	long	unk2;
	long	unk3;
	long	unk4;
	long	unk5;
	long	unk6;
	long	unk7;
	long	unk8;
} snd_header;

//-------------------------------------------------------------------------

long	SampleCache::find_zero_cross(long v)
{
	long	max = 2048;

	if (v < 0)
		return -32768;

	do {
		if (v > size)
			return -32768;

		if ((samples_master[v] <= 0) && (samples_master[v+1] >= 0))
			return v;
		v++;
		max--;
		if (max == 0)
			return -32768;
	} while(1);
}

//-------------------------------------------------------------------------

float	SampleCache::energy(long v1, long v2)
{
	long	p = 0;
	long	i;
	short	v;

	for (i = v1; i <= v2; i++) {
		v = samples_master[i];
		p += abs(v);
	}

	
	return p;
}

//-------------------------------------------------------------------------
#define		CWI		1024
#define		CWIH	(CWI/2)
//-------------------------------------------------------------------------

void	SampleCache::merge_block(short *p, long cnt)
{
	long	i;
	float	k;
	long	dp;

	dp = (ulong)p - (ulong)samples_master;
	dp /= 2;
	if ((dp + cnt) > size_master) {
		cnt = size_master - dp;
		//printf("new cnt %ld\n", cnt);
	}
	if (cnt < CWI)
		return;
	
	if (cur_write > CWIH) {
			
		for (i = 0; i <= CWI; i++) {
			k = i / (float)CWI;
			if (i < CWIH) {
				tmp_buf[cur_write - CWIH + i] = (int)((1.0 - k) *  tmp_buf[cur_write - CWIH + i] + k * p[- CWIH + i]);
			}
			else {
				tmp_buf[cur_write + i - CWIH] = (int)((1.0 - k) *  tmp_buf[cur_write + i - CWIH] + k * p[i - CWIH]);
			}
		}
		cnt -= CWIH;
		cur_write += CWIH;
	}
	memcpy((char *)&tmp_buf[cur_write], (char *)(p + CWIH), cnt * sizeof(short) + CWIH*2);
	cur_write += cnt;
}

//-------------------------------------------------------------------------

void	SampleCache::pitch_down_soft(float ratio)
{
	long	i;
	long	grain_size;
	long	overlap;
	long	skip;
	long	p1;
	long	p2;
	short	*src1;
	short	*src2;
	short	*dst;
	short	*dst0;
	long	pos;
	float	k;
	long	d;
	short	*max;
	float	k1;
	float	k2;
	double	t0;
	double	t1;	

	t0 = system_time();
	if (ratio < 0.5)
		ratio = 0.5;

	if (ratio > 1.0)	{		//do not make longer !
		if (samples_master) {
short	*tmp;
			tmp = samples;
			samples = samples_master;
			free((char *)tmp);
			samples_master = 0;
			size = size_master;
		}
		return;
	}
	
	ratio = 1.0 - ratio;
	if (samples_master == 0) {
		samples_master = (short *)malloc(sizeof(short) * size + 512);

		if (samples_master == 0) {
			NotifyMemory("3DmiX could not Scale this sound: Not enough memory", "OK");
			return;
		}

		memcpy(samples_master, samples, sizeof(short) * size);
		size_master = size;
	}

	p1 = 0;
	skip = 0;

	src1  = samples_master;
	src2 = samples_master;

	dst = samples;
	pos = 0;
	max = dst + (size_master);

	do {
		d = size_master - pos;
		if (d <= 128)
			goto out;
	
		grain_size = 8192 + rand() % 1000;	//8192 3000
		
		if (grain_size > (d)) {
			grain_size = d;
		}

		overlap = (int)(grain_size * ratio);

		dst0 = dst;
		memcpy((char *)dst, (char *)src1, (grain_size-overlap)*sizeof(short));

		dst += (grain_size - overlap);
		src1 += (grain_size - overlap);

		dst -= overlap;									

float	k1,k2;
float	iover;

		iover = 1.0/overlap;

		for (i = 0; i < overlap; i++) {
			k = (float)i*iover;
			k = k * (3.1415926/2.0);

		
			k1 = cos(k);
			k2 = sin(k);

			k = k1 + k2;
			*dst =  (int)(((*dst * k1) + (*src1 * k2)) / k);
			dst++;
			src1++;
		}

		pos += grain_size;		
		size = (ulong)dst - (ulong)samples;
		size /= 2;
	} while(1);
out:;

	size = (ulong)dst - (ulong)samples;
	size /= 2;

float	e1,e2;
long	ipos;
long	ik2;
double	dk1;
double	vk1;

	vk1 = size;
	dk1 = (float)size / (float)size_master;

	for (ik2 = size_master; ik2>0; ik2--) {
		//k1 = (float)size * (ik2 / (float)size_master);
		ipos = (int)vk1;
		e1 = vk1 - ipos;
		e2 = 1.0 - e1;
		samples[ik2] = (int)((samples[ipos] * e2) + (samples[ipos+1] * e1));
		vk1 -= dk1;
	}

	size = size_master;
}

//-------------------------------------------------------------------------

void	SampleCache::compress_soft(float ratio)
{
	long	i;
	long	grain_size;
	long	overlap;
	long	skip;
	long	p1;
	long	p2;
	short	*src1;
	short	*src2;
	short	*dst;
	short	*dst0;
	long	pos;
	float	k;
	long	d;
	short	*max;
	float	k1;
	float	k2;
	double	t0;
	double	t1;	

	t0 = system_time();
	if (ratio < 0.5)
		ratio = 0.5;
	if (ratio > 1.0)
		ratio = 1.0;

	if (ratio == 1.0)	{		//do not make longer !
		if (samples_master) {
short	*tmp;
			tmp = samples;
			samples = samples_master;
			free((char *)tmp);
			samples_master = 0;
			size = size_master;
		}
		return;
	}

	if (ratio > 1.0) {
		expand_soft(ratio);
		return;
	}
	
	ratio = 1.0 - ratio;
	if (samples_master == 0) {
		samples_master = (short *)malloc(sizeof(short) * size + 512);
		if (samples_master == 0) {
			NotifyMemory("3DmiX could not Scale this sound: Not enough memory", "OK");
			return;
		}
		memcpy(samples_master, samples, sizeof(short) * size);
		size_master = size;
	}

	p1 = 0;
	skip = 0;

	src1  = samples_master;
	src2 = samples_master;

	dst = samples;
	pos = 0;
	max = dst + (size_master);

	do {
		d = size_master - pos;
		if (d <= 128)
			goto out;
	
		grain_size = 8192 + rand() % 1000;	//8192 3000
		
		if (grain_size > (d)) {
			grain_size = d;
		}

		overlap = (int)(grain_size * ratio);

		dst0 = dst;
		memcpy((char *)dst, (char *)src1, (grain_size-overlap)*sizeof(short));

		dst += (grain_size - overlap);
		src1 += (grain_size - overlap);

		dst -= overlap;									

float	k1,k2;
float	iover;

		iover = 1.0/overlap;

		for (i = 0; i < overlap; i++) {
			k = (float)i*iover;
			k = k * (3.1415926/2.0);
			k1 = cos(k);
			k2 = sin(k);
			k = k1 + k2;
			*dst =  (int)(((*dst * k1) + (*src1 * k2)) / k);
			dst++;
			src1++;
		}

		pos += grain_size;		
		size = (ulong)dst - (ulong)samples;
		size /= 2;
	} while(1);
out:;

	size = (ulong)dst - (ulong)samples;
	size /= 2;

/*
float	e1,e2;
long	ipos;
long	ik2;
double	dk1;
double	vk1;
	vk1 = size;
	dk1 = (float)size / (float)size_master;

	for (ik2 = size_master; ik2>0; ik2--) {
		//k1 = (float)size * (ik2 / (float)size_master);
		ipos = (int)vk1;
		e1 = vk1 - ipos;
		e2 = 1.0 - e1;
		samples[ik2] = (int)((samples[ipos] * e2) + (samples[ipos+1] * e1));
		vk1 -= dk1;
	}
	size = size_master;
*/

}

//--------------------------------------------------------------------
/*
void	SampleCache::expand_soft(float ratio)
{
	long	pos_src;
	long	pos_dst;
	long	v;
	long	i;
	long	len;
	long	blk;
	long	skip;
	long	overlap;
	float	acc;

	ratio = 2.0;
	
	if (samples_master == 0) {
		samples_master = (short *)malloc(sizeof(short) * size + 1024);
		if (samples_master == 0) {
			NotifyMemory("3DmiX could not Scale this sound: Not enough memory", "OK");
			return;
		}
		memcpy(samples_master, samples, sizeof(short) * size);
		free((char *)samples);
		samples = (short *)malloc(2048 + sizeof(short) * (size * ratio));
		memset(samples, 0, sizeof(short)*size*ratio);
		size_master = size;
	}
	else {
		free((char *)samples);
		samples = (short *)malloc(2048 + sizeof(short) * (size * ratio));
		memset(samples, 0, sizeof(short)*size*ratio);
	}

	pos_src = 0;
	pos_dst = 0;


	do {
		blk = 2048;
		overlap = 300;
		skip = blk - overlap;
		for (i = 0; i < blk; i++) {
			samples[pos_dst + i] = samples_master[pos_src + i];
		}

		pos_dst += skip;

		for (i = 0; i < overlap; i++) {
float	k;
float	k1;
float	k2;
float	xk;

			k = (float)i/(float)overlap;

			k1 = cos(k) + cos(2.0*k);
			k1 *= 0.5;


			xk = 1.0-k;
			k2 = cos(xk) + cos(2.0*xk);
			k2 *= 0.5;

			//k = k1 + k2;
			
			//k1 /= k;
			//k2 /= k;
			//printf("%ld %f %f\n", i, k1, k2);



//			k = (float)i/(float)overlap;
//			k = k * (3.1415926);
//			k2 = cos(k);
//			k1 = cos(k);


			samples[pos_dst + i] = (samples[pos_dst + i] * k1) + (samples_master[pos_src + i] * k2);
		}
long	x1,x2;
		x1 = pos_dst;
		x2 = pos_dst + overlap;
		pos_dst += overlap;
		pos_src += overlap;
		for (i = 0; i < skip; i++) {
			samples[pos_dst + i] = samples_master[pos_src + i];
		}
		//samples[x1] = 32000;
		//samples[x2] = -32000;
		pos_dst += skip;
		pos_src += skip;
	} while(pos_src < (size_master - 8192));
	
	size = size_master * 2;
}
*/
//-------------------------------------------------------------------------

void	SampleCache::expand_soft(float ratio)
{
	FFTSHIFTER	*shift;
	long		ss1;
	long		mrg;
	long		p;
	long		v;
	long		i;
	long		base;
	long		skip_l;

	ratio = 1.0;

	if (samples_master == 0) {
		samples_master = (short *)malloc(sizeof(short) * size + 1024);
		if (samples_master == 0) {
			NotifyMemory("3DmiX could not scale this sound: Not enough memory", "OK");
			return;
		}
		memcpy(samples_master, samples, sizeof(short) * size);
		free((char *)samples);
		samples = (short *)malloc((int)(2048 + sizeof(short) * (size * ratio)));
		memset(samples, 0, (int)(sizeof(short)*size*ratio));
		size_master = size;
	}
	else {
		free((char *)samples);
		samples = (short *)malloc((int)(2048 + sizeof(short) * (size * ratio)));
		memset(samples, 0, (int)(sizeof(short)*size*ratio));
	}

	shift = new FFTSHIFTER();

	ss1 = 4096;				//was 1024
	mrg = 384;				//was 256
	skip_l = 128;
	shift->SetSize(ss1 * 4);

	base = 0;


	while(1) {
		shift->Reset();
		if (base > (size - ss1))
			goto out;

		for (i = 0; i < ss1 * 4; i++) {
			p = base + i - mrg - skip_l;
			if (p < 0)
				p = 0;

			shift->Add(samples_master[p]);
		}			

		shift->SetParameters(2);
		shift->Perform_FFT();

float	k,k1,k2;


		for (i = 0; i < mrg; i++) {
			v = shift->Get_Result(i + skip_l);
			k = (float)i/mrg;
			k = k * (3.1415926/2.0);
			k1 = cos(k);					//1 at 0
			k2 = sin(k);
			k = k1 + k2;
			p = i + base - mrg;
			if (p < 0)
				p = 0;

			v =  (int)(((v * k2) + (samples[p] * k1)) / k);
			if (v > 32760) v = 32760;
			if (v < -32760) v = -32760;
			samples[p] = v;
		}
		
		for (i = 0; i < ss1 - mrg; i++) {
			v = shift->Get_Result(i + mrg + skip_l);
			samples[i + base] = v;
		}
		//samples[base - mrg] = -31000;
		//samples[base] = 31000;

		base += (ss1 - (mrg));
	}
	
out:;	
	delete shift;
}


//-------------------------------------------------------------------------
/*
void	SampleCache::expand_soft(float ratio)
{
	long	i;
	long	grain_size;
	long	skip;
	short	*src1;
	short	*tsrc;
	short	*dst;
	long	pos;
	long	gs;
	long	gover;
	short	*last;

	gover = 256;

	gs = 2048;

	if (ratio > 1.5)	{		//do not make longer !
		ratio = 1.5;
	}
	
	if (samples_master == 0) {
		samples_master = (short *)malloc(sizeof(short) * size + 1024);
		if (samples_master == 0) {
			NotifyMemory("3DmiX could not scale this sound: Not enough memory", "OK");
			return;
		}
		memcpy(samples_master, samples, sizeof(short) * size);
		free((char *)samples);
		samples = (short *)malloc(128 + sizeof(short) * (size * ratio));
		memset(samples, 0, sizeof(short)*size*ratio);
		size_master = size;
	}
	else {
		free((char *)samples);
		samples = (short *)malloc(1024 + sizeof(short) * (size * ratio));
		memset(samples, 0, sizeof(short)*size*ratio);
	}

	skip = 0;

	src1  = samples_master;
	dst = samples;

	pos = 0;

	ratio = ratio - 1.0;

	last = 0;

	do {
		long	d = size_master - pos;

		if (d <= 128)
			goto out;
	
		grain_size = gs;
		
		if (grain_size > (d)) {
			grain_size = d;
		}


		skip = grain_size * ratio;

		if (pos > 0) {
		}

		memcpy((char *)dst, (char *)src1, grain_size*sizeof(short));

		if (last) {
			for (i = 0; i < gover; i++) {
				
				float	k,k1,k2;

				k = (float)i/gover;
				k = k * (3.1415926/2.0);
				k2 = cos(k);				//is 0 when 0
				k1 = sin(k);
				k = k1 + k2;
				dst[i] =  (dst[i] * k1 + last[i] * k2) / k;
			}
		}

		dst += grain_size;

		tsrc = src1 + grain_size - skip;

		
		dst -= gover;
		tsrc -= gover;


		for (i = 0; i < gover; i++) {
			
			float	k,k1,k2;

			k = (float)i/gover;
			k = k * (3.1415926/2.0);
			k1 = cos(k);				//is 0 when 0
			k2 = sin(k);
			k = k1 + k2;
			dst[i] =  (dst[i] * k1 + tsrc[i] * k2) / k;
		}
		dst += gover;
		tsrc += gover;

		for (i = 0; i < skip; i++) {
			dst[i] = tsrc[i];
		}
		dst += skip;
		last = tsrc + skip;

		src1 += grain_size;
		pos += grain_size;
	} while(1);
out:;

	size = (ulong)dst - (ulong)samples;
	//printf("size=%ld\n", size);
	size /= 2;
}
*/
//-------------------------------------------------------------------------

void	SampleCache::compress_hard(float co)
{
	long	p_old;	
	long	p1,p2;
	long	p3;	
	long	last;
	long	chunk;
	float	skip_r;
	long	pos;
	long	i;

	long	p1l;
	long	p2l;
	float	best;
	long	best_p2;
	float	ratio;
	long	base_pos;

	if (co > 1)	{		//do not make longer !
		if (samples_master) {
			memcpy(samples, samples_master, sizeof(short)*size_master);
			//free((char *)samples_master);
			//samples_master = 0;
		}
		return;
	}

	if (samples_master == 0) {
		samples_master = (short *)malloc(sizeof(short) * size + 32768);
		if (samples_master == 0) {
			NotifyMemory("3DmiX could not scale this sound: Not enough memory", "OK");
			return;
		}
		memcpy(samples_master, samples, sizeof(short) * size);
		size_master = size;
	}
	
	chunk = size_master / 32;
	if (chunk > 16384)
		chunk = 16384;

	skip_r = 1.0 - co;

	pos = 0;

	tmp_buf = samples;

	cur_write = 0;

	p_old = 0;

	while(pos < size_master) {
		p1 = find_zero_cross(pos);
		p2 = find_zero_cross(pos + (chunk * skip_r));
		
		if (p1 > 0 && p2 > 0) {
			p1l = find_zero_cross(p1 + 1) - p1;
			p2l = find_zero_cross(p2 + 1) - p2;
			ratio = (p1l/p2l);
			best_p2 = p2;

			if (ratio < 1.0)
				ratio = 1.0/ratio;
			best = ratio;

long	cnt = 7;

			base_pos = p2;

			do {
				p2 = find_zero_cross(p2 + 1);
				p2l = find_zero_cross(p2 + 1) - p2;
				if ((p2 - base_pos) > 200) {
					goto out;
				}
				ratio = (p1l/p2l);
				if (ratio < 1.0)
					ratio = 1.0/ratio;
				if (ratio < best) {
					best = ratio;
					best_p2 = p2;
				}
			} while(cnt--);


		}
out:;		

		p2 = best_p2;

		p1++;
		p2++;

		if (p1 > 0 && p2 > 0) {
			if (p_old >= 0) {
				merge_block(&samples_master[p_old],(p1 - p_old));
				p_old = p2;
			}
		}
		pos += (chunk);
	}
	merge_block(&samples_master[p_old], size_master-p_old);
	//size = cur_write;
}


//-------------------------------------------------------------------------
	
	SampleCache::SampleCache(const char *path, const char *name)
{
	char			buf[512];
	long			i;
	BMediaFile		*f = NULL;
	entry_ref		ref;
	double			k = 0.0;

	samples = 0;

	sprintf(buf, "%s/%s", path, name);

	samples_master = 0;

	BEntry entry(buf);
	if (!entry.Exists()) {
		size = 0;
		return;
	}
	entry.GetRef(&ref);

	f = new BMediaFile(&ref);
	if (f->InitCheck() < B_OK) {
		size = 0;
		delete f;
		return;
	}
	
	size = 0;

	// find the first raw audio track and negotiate format
	BMediaTrack *t = NULL;
	media_format mf;
	for (int i = 0; i < f->CountTracks(); i++) {
		t = f->TrackAt(i);
		mf.u.raw_audio = media_raw_audio_format::wildcard;
		mf.type = B_MEDIA_RAW_AUDIO;
		mf.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
		mf.u.raw_audio.byte_order =
				B_HOST_IS_BENDIAN ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
		mf.u.raw_audio.buffer_size = 1024;
		t->DecodedFormat(&mf);
		if (mf.type == B_MEDIA_RAW_AUDIO)
			break;
		f->ReleaseTrack(t);
		t = NULL;
	}

	if (t == NULL) {
		size = 0;
		delete f;
		return;
	}
		
	// read the sample into memory and convert it to 16-bit if necessary
	size = t->CountFrames() * mf.u.raw_audio.channel_count;
	samples = (short *)malloc(size * sizeof(short));

	if (samples == 0) {
		NotifyMemory("3DmiX could not load this file:Not Enough memory", "OK");
		samples = (short *)malloc(2);
		size = 1;
		return;
	}
	

	switch(mf.u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR: {
			uchar	*s2 = (uchar *)malloc(32 + size * sizeof(uchar));
			char	*p = (char *)s2;
			int64	frames;

			while (t->ReadFrames(p, &frames) == B_OK)
				p += frames * mf.u.raw_audio.channel_count;
	
			for (i = 0; i < size; i++) {
				samples[i] = ((long)s2[i] - 128) * 128;
			}
	
			free(s2);
			break;
		}
	
	case media_raw_audio_format::B_AUDIO_SHORT: {
			char *p = (char *)samples;
			int64 frames;

			while (t->ReadFrames(p, &frames) == B_OK) {
				p += frames * sizeof(short) * mf.u.raw_audio.channel_count;
			}

#if B_HOST_IS_LENDIAN
			if (mf.u.raw_audio.byte_order != B_MEDIA_LITTLE_ENDIAN)
				swap((ushort *)samples, size);
#else
			if (mf.u.raw_audio.byte_order != B_MEDIA_BIG_ENDIAN)
				swap((ushort *)samples, size);
#endif
		}
		break;
		
	default:
		printf("sample format 0x%x...confused\n", mf.u.raw_audio.format);
		break;
	}

	// deal with stereo files
	if (mf.u.raw_audio.channel_count == 2) {
		size /= 2;
		short *s2 = (short *)malloc(size * sizeof(short));


		if (s2 == 0) {
			NotifyMemory("3DmiX could not load this file:Not Enough memory", "OK");
			free((char *)samples);
			samples = (short *)malloc(2);
			size = 1;
			return;
		}
	
		uint32 mods = modifiers();
		
		if (mods & B_OPTION_KEY) {
			if (mods & B_SHIFT_KEY) {
				// right channel
				for (i = 0; i < size; i++) {
					s2[i] = samples[i*2+1];
				}
			} else if (mods & B_CONTROL_KEY) {
				// difference between the two channels
				for (i = 0; i < size; i++) {
					s2[i] = ((long)samples[i*2] - (long)samples[i*2+1]) / 2;
				}
			} else {
				// left channel
				for (i = 0; i < size; i++) {
					s2[i] = samples[i*2];
				}
			}
		} else {
			// mix down to mono
			for (i = 0; i < size; i++) {
				s2[i] = ((long)samples[i*2] + (long)samples[i*2+1]) / 2;
			}
		}
				
		free(samples);
		samples = s2;
	}
		
	// resample
	float	source_rate;
	long	dst_buf_size;

	source_rate = mf.u.raw_audio.frame_rate;

	if (source_rate != 44100) {
		short *s1;
		dst_buf_size = (long)((44100.0/source_rate) * size);
		k = source_rate / 44100.0;

		s1 = (short *)malloc(32 + sizeof(short) * dst_buf_size);
		for (i = 0; i < dst_buf_size; i++) {
			s1[i] = samples[(long)(i * k + 0.5)];
		}
		free((char *)samples);
		samples = s1;
		size = dst_buf_size;
	}

	delete f;
}

//-------------------------------------------------------------------------

float	SampleCache::LengthMaster()
{
	if (samples_master == 0)
		return Length();

	return size_master / 44100.0;
}

//-------------------------------------------------------------------------

float	SampleCache::Length()
{
	return size / 44100.0;
}

//-------------------------------------------------------------------------


	SampleCache::~SampleCache()
{
	free((char *)samples);
	free((char *)samples_master);
}

//-------------------------------------------------------------------------

void	SampleCache::Save(long ref)
{
	long	cnt;

	cnt = size;
	write32(ref, cnt);

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	swap((ushort *)samples, size);
#endif

	write(ref, samples, size * sizeof(short));

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	swap((ushort *)samples, size);
#endif
}

//-------------------------------------------------------------------------

	SampleCache::SampleCache()
{
	samples = 0;
	samples_master = 0;
}

//-------------------------------------------------------------------------

	SampleCache::SampleCache(long ref)
{
	samples = 0;
	samples_master = 0;

	Load(ref);
}

//-------------------------------------------------------------------------

bool	SampleCache::Resize(float new_length)
{
	long	cnt;
	short	*p;
	long	pcnt;

	cnt = (int)(new_length * 44100.0);
	

	pcnt = cnt;
	if (size < pcnt)
		pcnt = size;

	p = (short *)malloc(32 + cnt * sizeof(short));
	if (p == 0) {
		NotifyMemory("3DmiX ran out of memory", "OK");
		return FALSE;
	}

	memcpy((char *)p, (char *)samples, pcnt * sizeof(short));
	free((char *)samples);
	samples = p;
	size = cnt;
	return TRUE;
}

//-------------------------------------------------------------------------

SampleCache	*SampleCache::clone()
{
	SampleCache	*s;

	s = new SampleCache();

	s->size = size;
	s->samples = (short *)malloc(32 + size * sizeof(short));
	memcpy(s->samples, samples, size * sizeof(short));
		
	return s;
}

//-------------------------------------------------------------------------

void	SampleCache::Load(long ref)
{
	long	cnt;

	cnt = read32(ref);
	size = cnt;

	if (samples)
		free((char *)samples);

	samples = (short *)malloc(32 + size * sizeof(short));
	read(ref, samples, size * sizeof(short));

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	swap((ushort *)samples, size);
#endif
}

//-------------------------------------------------------------------------


void	SampleCache::GetSample(float t, float dt, short *min, short *max)
{
	long	i;
	long	j;	
	long	k;
	long	di;
	short	vmin;
	short	vmax;
	short	v;
	short	step;

	vmin = -1;
	vmax = 1;

	if (t < 0) {
		*min = vmin;
		*max = vmax;
		return;
	}

	i = (int)(t * 44100.0);

	if (i > size) {
		*min = vmin;
		*max = vmax;
		return;
	}
	
	di = (int)(dt * 44100.0);
	
	if (di == 0)
		di = 1;

	if (fast_display) {
		step = 1 + (di >> 1);			//was 4
	}
	else
		step = 1 + (di >> 3);			//was 4



	j = i + di;
	
	if (j > (size-step-1))
		j = (size-step-1);
	
	k = i;
	if (k > (size-step-1))
		k = (size-step-1);

	do {
		k += step;
		v = samples[k];
		if (v > vmax)
			vmax = v;
		if (v < vmin)
			vmin = v;
	} while(k < j);

	*min = vmin;
	*max = vmax;
}


//-------------------------------------------------------------------------

short	*SampleCache::p_samples()
{
	return samples;
}

//-------------------------------------------------------------------------
