//depot/main/src/demos/3dsound/track_view.cpp#53 - edit change 31009 (xtext)
#ifndef  TRACK_VIEW_H
#include "track_view.h"
#endif

#ifndef	WAVE_WINDOW_H
#include "wave_window.h"
#endif

#ifndef	SOUND_VIEW_H
#include "sound_view.h"
#endif

#include <Screen.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <Application.h>
#include <Roster.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Screen.h>
#include <File.h>
#include <ScrollBar.h>
#include <byteorder.h>
#include <sys/stat.h>

#ifndef	DIALOGS_H
#include "dialogs.h"
#endif
#include "global.h"
#include "util.h"
//--------------------------------------------------------

#define	MAX_T					16
#define	TRACK_HEIGHT			96
#define	DEFAULT_HEIGHT			60
#define	MIN_TRACK_HEIGHT		20
#define	TRACK_B_H				800
#define	EXTRA_SPACE				8
#define	TITLE_LEFT				12
#define	FIRST_TRACK_V			30

rgb_color	backgr = {24,24,24};
rgb_color	backgr1 = {24,24,24};
rgb_color	backgr1s = {200,160,160};

//--------------------------------------------------------
#define	UNDO_POS	1000000
//--------------------------------------------------------
extern	char	slow_anim;
extern	Channel	*make_channel(TrackViewer *a_viewer, const char *name);
extern	void	remove_channel(Channel *c);
extern	void	mixer_add_channel(Channel *a_channel, float x, float y);
extern	void	get_channel_pos(Channel *a_channel, float *x, float *y);
extern	void	get_mute_solo(Channel *a_channel, char *mute, char *solo);
extern	void	set_mute_solo(Channel *a_channel, char mute, char solo);
extern	void	stop_mixer();
extern	void	start_mixer();
extern	void	enable_undo();
extern	void	disable_undo();
extern	void	update_menus();

//--------------------------------------------------------
rgb_color	mc(long r, long g, long b);
float		get_ibeat();
BRect		r_maker(long h1, long h2, long v1, long v2);
void		main_refresh();
void		mangle(char *v, const char *p);
//--------------------------------------------------------

TrackView	*g_the_view = 0;
//--------------------------------------------------------

float	cur_ibeat = 0.0;

//--------------------------------------------------------

float	get_ibeat()
{
	return cur_ibeat;
}

//--------------------------------------------------------


float	TrackView::GetBaseTime()
{
	return base_time;
}

/*------------------------------------------------------------*/

rgb_color	mc(long r, long g, long b)
{
	rgb_color	c;

	c.red = r;
	c.green = g;
	c.blue = b;
		
	return c;
}

/*------------------------------------------------------------*/

BRect	r_maker(long h1, long h2, long v1, long v2)
{
	long	tmp;

	if (v1 > v2) {
		tmp = v1;
		v1 = v2;
		v2 = tmp;
	}
	if (h1 > h2) {
		tmp = h1;
		h1 = h2;
		h2 = tmp;
	}
		
	if (v1 == v2) {
		return BRect(h1, v1-1, h2, v1);
	}
	
	return BRect(h1-1, v1, h1, v2);
}

/*------------------------------------------------------------*/

HairLine::HairLine(long h1, long v1, long h2, long v2)
	   : BView(r_maker(h1, h2, v1, v2), "", B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(255,0,0);
}

/*------------------------------------------------------------*/

HairLine::~HairLine()
{
}

/*------------------------------------------------------------*/

InfoBlock::InfoBlock(BRect r, TrackObject *t)
	   : BView(r, "", B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	tt = t;
}

/*------------------------------------------------------------*/

InfoBlock::~InfoBlock()
{
}

/*------------------------------------------------------------*/

void	InfoBlock::Draw(BRect r)
{
	char	buf[256];
	BRect	br;
	float	h1,h2;
	long	vp;

	SetFont(be_fixed_font);
	SetFontSize(12);
	
	br = Bounds();
	SetHighColor(255, 0, 0);
	StrokeRect(br);

	br.InsetBy(1,1);
	SetHighColor(190, 0, 0);
	StrokeRect(br);

	br.InsetBy(1,1);
	SetHighColor(160, 0, 0);
	StrokeRect(br);

	br.InsetBy(1,1);
	SetHighColor(60, 60, 60);
	FillRect(BRect(br.left, br.top, br.right, 50));
	SetHighColor(255, 50, 50);
	SetDrawingMode(B_OP_OVER);
	tt->GetRange(&h1, &h2);
	sprintf(buf, ": %2.3f", h1);
	
	vp = 18;

#define	TH1	13
#define	TH2	65

	MovePenTo(BPoint(TH1, vp));
	DrawString("Start");
	MovePenTo(BPoint(TH2, vp));
	DrawString(buf);

	vp += 13;

	sprintf(buf, ": %2.3f", h2);
	MovePenTo(BPoint(TH1, vp));
	DrawString("End");
	MovePenTo(BPoint(TH2, vp));
	DrawString(buf);

	vp += 13;

	sprintf(buf, ": %2.3f", h2-h1);
	MovePenTo(BPoint(TH1, vp));
	DrawString("Length");
	MovePenTo(BPoint(TH2, vp));
	DrawString(buf);

	SetHighColor(60, 60, 60);
	FillRect(BRect(br.left, 50, br.right, br.bottom));
	SetHighColor(255, 50, 50);
	
	vp += 20;

	MovePenTo(BPoint(2, vp - 12));
	StrokeLine(BPoint(br.right, vp - 12));

	sprintf(buf, ": %2.3f", tt->LoopPoint());
	MovePenTo(BPoint(TH1, vp));
	DrawString("Loop");
	MovePenTo(BPoint(TH2, vp));
	DrawString(buf);
	SetDrawingMode(B_OP_COPY);
	Sync();
}

/*------------------------------------------------------------*/

TrackView::TrackView(BRect rect, char *name, TSoundView *ss)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	long		i;
	long		k;
	BScreen		s;
	LineView	*line;
	BRect		tmp;
	char		buf[128];
	Channel		*tmp_channel;

	g_the_view = this;
	click_when = 0;
	last_click = 0;
	
	last_prj_name[0] = 0;
	last_prj_path[0] = 0;

	last_track_md = -1;
	cur_info = 0;
	clip_size = 0;
	my3d = ss;
	my3d->SetTrackView(this);
	tmp = rect;
	tmp.right -= tmp.left;
	tmp.bottom -= tmp.top;

	for (i = 0; i < MAX_FEEDBACK; i++)
		last_feedback[i] = -32000;

	line = new LineView(BRect(0, 0, 32000, 1));
	this->AddChild(line);

	backgr = s.ColorForIndex(25);
	backgr1 = s.ColorForIndex(28);
	zoom = 0.1;
	base_time = 0;
	SetViewColor(backgr);
	cur_text = 0;

	xtra_off = new BBitmap(BRect(0, 0, HEADER_SPACE - 3, TRACK_HEIGHT * 32),
						  	B_COLOR_8_BIT,
						  	TRUE);
	
	xtra_off->AddChild(xtra_view = new BView(BRect(0,0,HEADER_SPACE,TRACK_HEIGHT * 32),
										   "",
										   B_FOLLOW_ALL,
										    B_WILL_DRAW));


	nv = 0;
	MakeFocus();
}

//--------------------------------------------------------
#define	MAX_P	2048

void	TrackView::SaveMaster(const char *path, const char *master_name)
{
	long		ref;
	long		i;
	float		x,y;
	long		sig;
	char		buf[MAX_P];
	long		cnt;
	BNode		b;
	BNodeInfo	info;
	ref = open(master_name, O_RDWR | O_CREAT, 0644);

	sig = 'MAST';

	write32(ref, sig);
	cnt = nv;
	write32(ref, cnt);

	for (i = 0; i < nv; i++) {
		sprintf(buf, "%s/%s", path, viewers[i]->Name());
		write(ref, buf, MAX_P);
		get_channel_pos(viewers[i]->GetChannel(), &x, &y);
		writef(ref, x);
		writef(ref, y);
	}
	
	TimeView *tv = (TimeView *)Window()->FindView("time");
	tv->Save(ref);
	TCtrlView *ctrl = (TCtrlView *)Window()->FindView("ctrl");
	ctrl->Save(ref);
	close(ref);
	b.SetTo(master_name);
	info.SetTo(&b);	
	info.SetType("application/x-vnd.Be.3DSoundMixer_Master");
}

//--------------------------------------------------------

void	mangle(char *v, const char *p)
{
	BPath	*p1;
	BPath	*p2;

	p1 = new BPath(v);
	p2 = new BPath(p);
	p2->GetParent(p2);
	p2->Append(p1->Leaf());
	strcpy(v, p2->Path());
}

//--------------------------------------------------------

void	TrackView::adjust_vsb()
{
	BScrollBar	*s;
	long		vp;
	long		i;

	Window()->Lock();
	s = (BScrollBar *)Window()->FindView("_VSB_");

	vp = FIRST_TRACK_V;
	
	for (i = 0; i < nv; i++) {
		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}
	
	s->SetRange(0, vp);
	Window()->Unlock();
}

//--------------------------------------------------------
	
void	TrackView::LoadMaster(const char *p)
{
	long	ref;
	long	tr_ref;
	long	cnt;
	long	sig;
	float	x,y;
	Channel	*c;
	char	buf[MAX_P];

	stop_mixer();
	snooze(150000);			//be certain that all is stopped !

	clear_all();
	ref = open(p, O_RDONLY);

	read(ref, &sig, sizeof(sig));
	read(ref, &cnt, sizeof(cnt));

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	sig = __swap_int32(sig);
	cnt = __swap_int32(cnt);
#endif	

	while(cnt>0) {
		cnt--;
		read(ref, buf, MAX_P);
		read(ref, &x, sizeof(x));
		read(ref, &y, sizeof(y));

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
		x = __swap_float(x);
		y = __swap_float(y);
#endif

		mangle(buf, p);
		printf("track = %s\n", buf);
		printf("x=%f, y=%f\n", x,y);

		if (x < -256) x = -256;
		if (x > 256) x = 256;
		if (y < -256) y = -256;
		if (y > 256) y = 256;

		tr_ref = open(buf, O_RDONLY);
		if (tr_ref > 0) {
			viewers[nv] = new TrackViewer(tr_ref, this);
			mixer_add_channel(c = make_channel(viewers[nv], viewers[nv]->Name()), x, y);

			viewers[nv]->SetChannel(c);

			nv++;
			close(tr_ref);
		}
	}

	TimeView *tv = (TimeView *)Window()->FindView("time");
	tv->Load(ref);
	TCtrlView *ctrl = (TCtrlView *)Window()->FindView("ctrl");
	ctrl->Load(ref);
	
	close(ref);
	RefreshXtra();
	Refresh();
	start_mixer();
	adjust_vsb();
}

//--------------------------------------------------------

void	TrackView::save_named()
{
	if (last_prj_name[0] != 0) {
		printf("save %s || %s\n", last_prj_path, last_prj_name);
		Save(last_prj_path, last_prj_name);
	}
}

//--------------------------------------------------------

void	TrackView::Save(char *path, char *name)
{
	long	ref;
	long	i;
	char	buf[2048];

	for (i = 0; i < nv; i++) {
		sprintf(buf, "%s/%s", path, viewers[i]->Name());
		ref = open(buf, O_RDWR | O_CREAT, 0644);
		viewers[i]->Save(ref);
		close(ref);
	}
	sprintf(buf, "%s/%s", path, name);
	SaveMaster(path, buf);
}

//--------------------------------------------------------

void	TrackView::clear_all()
{
	long	i;

	for (i = 0; i < nv; i++) {
		delete viewers[i];
	}
	nv = 0;
	Invalidate();
}

//--------------------------------------------------------

char	TrackView::HasChannel()
{
	return (nv > 0);
}

//--------------------------------------------------------

char	TrackView::HasChannelSelected()
{
	long	i;

	for (i = 0; i < nv; i++) {
		if (viewers[i]->Selected())
			return 1;
	}

	return 0;
}

//--------------------------------------------------------

void	TrackView::DeleteChannels()
{
	long	i;
	long	t;

	t = 0;

	for (i = 0; i < nv; i++) {
		if (viewers[i]->Selected()) {
			delete viewers[i];
		}
		else {
			viewers[t] = viewers[i];
			t++;
		}
	}
	nv = t;
	RefreshXtra();
	Refresh();
	adjust_vsb();
}

//--------------------------------------------------------

void	TrackView::delete_selection()
{
	long	i;

	comit();
	for (i = 0; i < nv; i++) {
		viewers[i]->delete_selection();
	}
}

//--------------------------------------------------------

char	TrackView::has_selection()
{
	long	i;

	for (i = 0; i < nv; i++) {
		if (viewers[i]->has_selection())
			return 1;
	}
	
	return 0;
}

//--------------------------------------------------------

void	TrackView::clear_clipboard()
{
	long	i;

	for (i = 0;i < clip_size; i++) {
		delete clipboard[i];
	}

	clip_size = 0;
}

//--------------------------------------------------------

void	TrackView::Cut()
{
	Copy();
	Clear();
}

//--------------------------------------------------------

void	TrackView::Clear()
{
	delete_selection();
}

//--------------------------------------------------------

char	TrackView::HasClip()
{
	return (clip_size > 0);
}

//--------------------------------------------------------

void	TrackView::Copy()
{
	long		i;
	long		j;
	TrackObject	*t;
	
	clear_clipboard();

	for (i = 0; i < nv; i++) {
		for (j = 0; j < viewers[i]->obj_count; j++) {
			if (viewers[i]->obj_array[j]->Selected()) {
				if (clip_size < MAX_CLIP) {
					clipboard[clip_size] = viewers[i]->obj_array[j]->clone();
					clip_size++;
				}
			}
		}
	}
}

//--------------------------------------------------------

void	TrackView::comit()
{
	long		i;
	long		j;
	TrackObject	*t;
	long		cnt;
	

	for (i = 0; i < nv; i++) {
		viewers[i]->remove_ghosts();
		cnt = viewers[i]->obj_count;
		for (j = 0; j < cnt; j++) {
			viewers[i]->obj_array[j]->comit();
		}
	}
	enable_undo();
}

//--------------------------------------------------------

void	TrackView::undo()
{
	long		i;
	long		j;
	TrackObject	*t;
	long		cnt;

	for (i = 0; i < nv; i++) {
		viewers[i]->revive_ghosts();
		cnt = viewers[i]->obj_count;
		for (j = 0; j < cnt; j++) {
			viewers[i]->obj_array[j]->undo();
		}
	}
	Refresh();
	disable_undo();
}


//--------------------------------------------------------

void	TrackView::Paste()
{
	long	i;
	long	k;

	if (last_track_md < 0 || last_track_md >= nv)
		return;

	k = last_index_md;

	for (i = 0; i < clip_size; i++) {
		viewers[last_track_md]->InsertAfter(k, clipboard[clip_size - i - 1]->clone());
		//k++;
	}
	viewers[last_track_md]->redo_time();
}

//--------------------------------------------------------
extern	long	do_ask_beat(float v, BWindow *caller);
//--------------------------------------------------------

void	TrackView::DoBPM()
{
	float		length;
	TimeView	*tv;
	long		v;


	tv = (TimeView *)Window()->FindView("time");

	length = tv->select_end - tv->select_start;
	if (length == 0) {
		return;
	}

	v = do_ask_beat(length, Window());

	if (v > 0) {

		length = length / v;
		TCtrlView *ctrl = (TCtrlView *)Window()->FindView("ctrl");
		
		length = 60.0/length;
		if (ctrl)
			ctrl->SetBPM(length);
	}
}


//--------------------------------------------------------

void	TrackView::DoBPM1()
{
	float		length;
	long		v;
	long		i;
	long		j;

	for (i = 0; i < nv; i++) {
		for (j = 0; j < viewers[i]->obj_count; j++) {
			if (viewers[i]->obj_array[j]->Selected()) {
				length = viewers[i]->obj_array[j]->Length();
			}
		}
	}
	
	if (length == 0) {
		return;
	}

	v = do_ask_beat(length, Window());

	if (v > 0) {

		length = length / v;
		TCtrlView *ctrl = (TCtrlView *)Window()->FindView("ctrl");
		
		length = 60.0/length;
		if (ctrl)
			ctrl->SetBPM(length);
	}
}

//--------------------------------------------------------

void	 TrackView::KeyDown(const char *key, int32 count)
{
	
	switch(*key) {
			case 8  :					//back space
				delete_selection();
				break;
			case 'W':
			case 'w':
				((TWaveWindow *)Window())->Switch();
				break;
			case 'T':
				TimeView	*tv;
		

				tv = (TimeView *)Window()->FindView("time");
				tv->Toggle();
				break;
	}
}

//--------------------------------------------------------

long	TrackView::find_index(TrackViewer *p)
{
	long	i;

	for (i = 0; i < nv; i++) {
		if (p == viewers[i])
			return i;
	}
	return -1;
}

//--------------------------------------------------------

void	TrackView::RefreshXtra()
{
	long	i;
	long	vp;
	
	xtra_off->Lock();
	xtra_view->SetHighColor(backgr);
	xtra_view->FillRect(BRect(0,0,9000,9000));

	vp = FIRST_TRACK_V;
	
	for (i = 0; i < nv; i++) {
		draw_track_extra(i, vp);
		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}
	xtra_view->Sync();
	xtra_off->Unlock();
	DrawBitmapAsync(xtra_off, BPoint(0,2));
}

//--------------------------------------------------------

void	TrackView::FeedBackLine(float h1, TrackViewer *exclude, long line_number, rgb_color c)
{
	long	i;
	BRect	src,dst;
	long	vp;
	BBitmap	*b;

	if (last_feedback[line_number] > -32000) {
		vp = FIRST_TRACK_V;
		SetHighColor(backgr);
		if (last_feedback[line_number] > 0) {
			MovePenTo(BPoint(last_feedback[line_number] + HEADER_SPACE, 0));
			StrokeLine(BPoint(last_feedback[line_number] + HEADER_SPACE, 32000));
		}
		SetHighColor(c);
		if (h1 > 0) {
			SetDrawingMode(B_OP_BLEND);
			MovePenTo(BPoint(h1+HEADER_SPACE, 0));
			StrokeLine(BPoint(h1+HEADER_SPACE, 32000));
			SetDrawingMode(B_OP_COPY);
		}
		
		for (i = 0; i < nv; i++) {
			b = viewers[i]->GetBitmap();
	
			src.top = 0;
			src.bottom = viewers[i]->GetHeight();
			src.left = 0;
			src.right = TRACK_B_H;
			src.left = last_feedback[line_number] ;
			src.right = last_feedback[line_number];
			dst = src;
			dst.OffsetBy(HEADER_SPACE, vp);
			DrawBitmap(b, src, dst);
			vp += viewers[i]->GetHeight() + EXTRA_SPACE;

			SetDrawingMode(B_OP_BLEND);
			SetHighColor(c);
			if (h1 > 0) {
				MovePenTo(BPoint(h1+HEADER_SPACE, dst.top));
				StrokeLine(BPoint(h1+HEADER_SPACE, dst.bottom));
			}
			SetDrawingMode(B_OP_COPY);

		}
	}
	last_feedback[line_number] = h1;
	h1 += HEADER_SPACE;
	
	Sync();
}

//--------------------------------------------------------

void	TrackView::Refresh(TrackViewer *v)
{
	long	i;
	long	vp;
	BBitmap	*b;
	BRect	src,dst;
	double	s,e;

	Window()->Lock();
	vp = FIRST_TRACK_V;
	for (i = 0; i < nv; i++) {
		
		if (v == viewers[i]) {
			b = viewers[i]->GetBitmap();
			viewers[i]->InternalDraw();
	
			src.top = 0;
			src.bottom = viewers[i]->GetHeight();
			src.left = 0;
			src.right = TRACK_B_H;
			dst = src;
			dst.OffsetBy(HEADER_SPACE, vp);
			viewers[i]->last_vp = (int)dst.top;
			DrawBitmap(b, src,dst);
			goto out;
		}

		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}

out:;
	Sync();
	Window()->Unlock();
}

//--------------------------------------------------------


void	TrackView::Refresh(long index)
{
	long	i;
	long	vp;
	BBitmap	*b;
	BRect	src,dst;

	Window()->Lock();
	vp = FIRST_TRACK_V;
	for (i = 0; i < nv; i++) {
		
		if (i == index) {
			b = viewers[i]->GetBitmap();
			viewers[i]->InternalDraw();
	
			src.top = 0;
			src.bottom = viewers[i]->GetHeight();
			src.left = 0;
			src.right = TRACK_B_H;
			dst = src;
			dst.OffsetBy(HEADER_SPACE, vp);
			viewers[i]->last_vp = (int)dst.top;
			DrawBitmap(b, src,dst);
			goto out;
		}

		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}

out:;
	Sync();
	Window()->Unlock();
}

//--------------------------------------------------------

void	TrackView::RefreshSize(long index)
{
	long	i;
	long	vp;
	BBitmap	*b;
	BRect	src,dst;

	vp = FIRST_TRACK_V;
	for (i = 0; i < nv; i++) {
		b = viewers[i]->GetBitmap();
		if (i == index)
			viewers[i]->InternalDraw();

		src.top = 0;
		src.bottom = viewers[i]->GetHeight();
		src.left = 0;
		src.right = TRACK_B_H;
		dst = src;
		dst.OffsetBy(HEADER_SPACE, vp);
		viewers[i]->last_vp = (int)dst.top;
		DrawBitmapAsync(b, src,dst);

		SetHighColor(backgr);
	
		FillRect(BRect(HEADER_SPACE, dst.bottom + 1,	
					   1000, dst.bottom + EXTRA_SPACE - 1));
		Sync();

		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}
	
	SetHighColor(backgr);
	FillRect(BRect(HEADER_SPACE, dst.bottom + 1,	
				   1000, dst.bottom + 50));
	Sync();
}


//--------------------------------------------------------

void	main_refresh()
{
	g_the_view->Window()->Lock();
	g_the_view->Refresh();
	g_the_view->Window()->Unlock();
}

//--------------------------------------------------------

void	TrackView::Refresh()
{
	long	i;
	long	vp;
	BBitmap	*b;
	BRect	src,dst;

	vp = FIRST_TRACK_V;
	for (i = 0; i < nv; i++) {
		b = viewers[i]->GetBitmap();
		viewers[i]->InternalDraw();

		src.top = 0;
		src.bottom = viewers[i]->GetHeight();
		src.left = 0;
		src.right = TRACK_B_H;
		dst = src;
		dst.OffsetBy(HEADER_SPACE, vp);
		viewers[i]->last_vp = (int)dst.top;
		DrawBitmapAsync(b, src,dst);

		SetHighColor(backgr);
	
		FillRect(BRect(HEADER_SPACE, dst.bottom + 1,	
					   1000, dst.bottom + EXTRA_SPACE - 1));
		Sync();

		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}
	SetHighColor(backgr);
	FillRect(BRect(HEADER_SPACE, dst.bottom + 1,	
				   1000, dst.bottom + 500));
	Sync();
}

//--------------------------------------------------------

void	TrackView::SetHPos(float v)
{
	TimeView	*tv;

	base_time = v/35.0;
	tv = (TimeView *)Window()->FindView("time");
	tv->set_parm(GetBaseTime(), zoom);
	Refresh();
	tv->Draw(BRect(0,0,32000,32000));
}

//--------------------------------------------------------

	TrackView::~TrackView()
{
}

//--------------------------------------------------------
	
void	TrackView::MessageReceived(BMessage *message)
{
	bool handled = false;
	
	// was this message dropped?
	
	if (message->WasDropped()) {
		BPoint dropLoc;
		BPoint offset;
		
		dropLoc = message->DropPoint(&offset);
		ConvertFromScreen(&dropLoc);
		ConvertFromScreen(&offset);

		handled = MessageDropped(message, dropLoc, offset);
		adjust_vsb();
	}
	update_menus();
}

//--------------------------------------------------------


char	TrackView::TryDropInTrack(BPoint where, const char *name, const char *path)
{
	long	i;
	BRect	src;
	BRect	dst;
	long	vp;

	vp = FIRST_TRACK_V;
	
	for (i = 0; i < nv; i++) {
		src.top = 0;
		src.bottom = viewers[i]->GetHeight();
		src.left = 0;
		src.right = TRACK_B_H;
		dst = src;
		dst.OffsetBy(HEADER_SPACE, vp);
		
		if (dst.Contains(where)) {
			where.x -= dst.left;
			where.y -= dst.top;
			viewers[i]->HandleDrop(where, name, path);
			return 1;
		}

		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}
	return 0;
}

//--------------------------------------------------------

char	TrackView::is_master(const char *path, const char *name)
{
	char	buf[1024];
	long	ref;
	long	sig;

	sprintf(buf, "%s/%s", path, name);

	ref = open(buf, O_RDONLY);
	read(ref, &sig, sizeof(sig));

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	sig = __swap_int32(sig);
#endif

	close(ref);

	if (sig == 'MAST')
		return 1;
	return 0;
}


//--------------------------------------------------------

char	TrackView::is_track(const char *path, const char *name)
{
	char	buf[1024];
	long	ref;
	long	sig;

	sprintf(buf, "%s/%s", path, name);

	ref = open(buf, O_RDONLY);
	read(ref, &sig, sizeof(sig));
	close(ref);

#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
	sig = __swap_int32(sig);
#endif
	
	if (sig == '!TRK')
		return 1;
	return 0;
}

//--------------------------------------------------------


void	TrackView::new_track()
{
	Channel	*c;

	viewers[nv] = new TrackViewer("untitled", this);
	mixer_add_channel(c = make_channel(viewers[nv], viewers[nv]->Name()), 100 + (nv % 5) * 20, 100);

	viewers[nv]->SetChannel(c);

	nv++;
	RefreshXtra();
	Refresh();
	adjust_vsb();
}

//--------------------------------------------------------


void	TrackView::DropTrack(BPoint where, const char *path, const char *name)
{
	char	buf[1024];
	long	ref;
	Channel	*c;

	sprintf(buf, "%s/%s", path, name);
	
	ref = open(buf, O_RDONLY);
	viewers[nv] = new TrackViewer(ref, this);
	close(ref);
	mixer_add_channel(c = make_channel(viewers[nv], viewers[nv]->Name()), 100 + (nv % 5) * 20, 100);

	viewers[nv]->SetChannel(c);

	nv++;
	RefreshXtra();
	Refresh();
}

//--------------------------------------------------------

void	TrackView::DropFolder(BPoint where, const char *path, const char *name)
{
	char	buf[1024];
	long	ref;
	long	sig;

	return;
	sprintf(buf, "%s/%s", path, name);

	ref = open(buf, O_RDONLY);
	read(ref, &sig, sizeof(sig));
	close(ref);

}

//--------------------------------------------------------

char	TrackView::is_folder(const char *path, const char *name)
{
        struct stat     s;
		char			buf[1024];

		sprintf(buf, "%s/%s", path, name);
        stat(buf, &s);
        if (S_ISDIR( s.st_mode))
			return 1;

        return (0);
}

//--------------------------------------------------------

void	TrackView::do_handle_drop(BPoint where, const char *name, const char *path)
{
	Channel	*c;
	char	buf[2048];

	if (is_folder(path, name)) {
		DropFolder(where, path, name);
		return;
	}

	if (is_track(path, name)) {
		DropTrack(where, path, name);
		return;
	}

	if (is_master(path, name)) {
		strcpy(last_prj_name, name);
		strcpy(last_prj_path, path);

		sprintf(buf, "%s/%s", path, name);
		LoadMaster(buf);
		return;
	}
	
	if (TryDropInTrack(where, name, path)) {
		RefreshXtra();
		Refresh();
		return;
	}

	viewers[nv] = new TrackViewer(name, this);
	mixer_add_channel(c = make_channel(viewers[nv], viewers[nv]->Name()), 100 + (nv % 5) * 20, 100);
	

	viewers[nv]->SetChannel(c);
	viewers[nv]->HandleDrop(BPoint(0,0), name, path);

	nv++;
	RefreshXtra();
	Refresh();
}

//--------------------------------------------------------

void	TrackView::TakeRef(entry_ref ref)
{
	BEntry		*an_entry;
	BPath		p;

	an_entry = new BEntry(&ref);
	an_entry->GetPath(&p);
	p.GetParent(&p);

	delete an_entry;
	do_handle_drop(BPoint(32000,32000), ref.name, p.Path());
}

//--------------------------------------------------------

bool	TrackView::MessageDropped(BMessage *inMessage, BPoint where, BPoint offset)
{
	entry_ref	ref;
	BEntry		*an_entry;
	BPath		p;

	switch (inMessage->what ){
   		case B_SIMPLE_DATA:
   		case B_REFS_RECEIVED:
			if (inMessage->HasRef("refs")) {
				for (int32 index = 0; ; index++) {
					if(inMessage->FindRef("refs", index, &ref) != B_OK)
						break;

					an_entry = new BEntry(&ref);
					an_entry->GetPath(&p);
					p.GetParent(&p);

					delete an_entry;
					do_handle_drop(where, ref.name, p.Path());
   				}
   			}
		break;
	}
	return 0;
}

//--------------------------------------------------------

void TrackView::AttachedToWindow()
{
	Refresh();
	RefreshXtra();
	MakeFocus();
}

//--------------------------------------------------------


void	TrackView::GMouse(BPoint *w, ulong *buttons, TrackViewer *me)
{
	long	i;
	long	hit;
	long	vp;
	BRect	src;
	BPoint	where;

	vp = FIRST_TRACK_V;

	for (i = 0; i < nv; i++) {
		if (viewers[i] == me) {
			src.top = 0;
			src.bottom = viewers[i]->GetHeight();
			src.left = 0;
			src.right = TRACK_B_H;
			src.OffsetBy(HEADER_SPACE, vp);

			GetMouse(w, buttons);
			w->x -= src.left;
			w->y -= src.top;
			return;

		}
		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}
}


//--------------------------------------------------------

void	TrackView::deselect_others(TrackViewer *me)
{
	long	i;

	if (modifiers() & B_SHIFT_KEY)
		return;

	for (i = 0; i < nv; i++) {
		if (viewers[i] != me) {
			if (viewers[i]->Selected()) {
				viewers[i]->SetSelected(0);
			}
		}
	}
}

//--------------------------------------------------------

void	TrackView::HitInViewer(BPoint view_where, long viewer_index, BRect cur_viewer_frame)
{
	BPoint	w0;
	BPoint	where;
	ulong	b;
	long	dv;
	long	part;
	long	i;

	w0 = view_where;

	w0.x -= cur_viewer_frame.left;
	w0.y -= cur_viewer_frame.top;
	
	part = viewers[viewer_index]->find_part(w0);

	switch(part) {
		case CORNER :
			deselect_others(viewers[viewer_index]);
			viewers[viewer_index]->SetSelected(viewers[viewer_index]->Selected() ^ 0x01);
			if (part == CORNER)
			do {
				Window()->Unlock();
				Window()->Lock();
				GetMouse(&where, &b);
				where.x -= cur_viewer_frame.left;
				where.y -= cur_viewer_frame.top;
				dv = where.y - w0.y;
				if (dv == 0) {
					snooze(20000);
				}
				else {
					w0 = where;
					if (modifiers() & B_SHIFT_KEY) {
						viewers[viewer_index]->SetHeight(viewers[viewer_index]->GetHeight() + dv);
						for (i = 0; i < nv; i++) {
							if (i != viewer_index)
								viewers[i]->SetHeight(viewers[viewer_index]->GetHeight());
						}
						RefreshXtra();
						Refresh();
						Sync();
					}
					else {
						viewers[viewer_index]->SetHeight(viewers[viewer_index]->GetHeight() + dv);
						RefreshXtra();
						RefreshSize(viewer_index);
						Sync();
					}
					adjust_vsb();
				}
			} while(b);
			break;

		case CONTENT :
			viewers[viewer_index]->handle_hit(w0);	
			break;
	}
}

//--------------------------------------------------------

class	xBTextControl : public BTextControl {
public:
				xBTextControl(BRect frame,
							const char *name,
							const char *label, 
							const char *initial_text, 
							BMessage *message,
							uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
};

//--------------------------------------------------------

				xBTextControl::xBTextControl(BRect frame,
											const char *name,
											const char *label, 
											const char *initial_text, 
											BMessage *message,
											uint32 rmask,
											uint32 flags) :
				BTextControl(frame, name, label, initial_text, message, rmask, flags)
{
} 

//--------------------------------------------------------


void	TrackView::do_text(BRect r, long ct)
{
	cur_text_index = ct;
	r.top -= 5;
	r.bottom -= 5;
	r.right = HEADER_SPACE - 26; 
	r.left = 5;
	cur_text = new xBTextControl(r, 0, 0, viewers[ct]->Name(), 0, 0);
	cur_text->SetModificationMessage(new BMessage('txt!'));
	AddChild(cur_text);
}

//--------------------------------------------------------

void	TrackView::txt_msg()
{
	Window()->Lock();
	viewers[cur_text_index]->SetName(cur_text->Text());
	Window()->Unlock();
}

//--------------------------------------------------------

void	TrackView::special_char(char c)
{
	if (c == 0x0a) {
		if (cur_text) {
			viewers[cur_text_index]->SetName(cur_text->Text());
			RemoveChild(cur_text);
			delete cur_text;
			cur_text = 0;
			RefreshXtra();
			Sync();
		}
		MakeFocus();
	}
}


//--------------------------------------------------------

void	TrackView::MouseDown(BPoint where)
{
	long	i;
	long	hit;
	long	vp;
	BRect	src,dst;
	long	mid;

	if (cur_text) {
		viewers[cur_text_index]->SetName(cur_text->Text());
		RemoveChild(cur_text);
		delete cur_text;
		cur_text = 0;
		RefreshXtra();
		Sync();
	}
	MakeFocus();
	vp = FIRST_TRACK_V;

	for (i = 0; i < nv; i++) {
		src.top = 0;
		src.bottom = viewers[i]->GetHeight();
		src.left = 0;
		src.right = TRACK_B_H;
		dst = src;
		dst.OffsetBy(HEADER_SPACE, vp);
		
		if (dst.Contains(where)) {
			slow_anim++;				
			comit();
			HitInViewer(where, i, dst);
			slow_anim--;	
			update_menus();
			return;
		}

		dst.left = 0;
		mid = dst.bottom + dst.top;
		mid /= 2;
		dst.bottom = mid + 7;
		dst.top = mid - 7;
		dst.right = (HEADER_SPACE - 35);
		if (dst.Contains(where)) {
			do_text(dst, i);
			update_menus();
			return;
		}

long	mid;

		mid = dst.top + dst.bottom;
		mid /= 2;

		dst.top = mid - 12;
		dst.bottom = mid + 12;
		dst.left = HEADER_SPACE - 20;
		dst.right = dst.left + 12;

		if (dst.Contains(where)) {
			do_mute_solo(dst, i);
			update_menus();
			return;
		}

		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}

	update_menus();
}


//--------------------------------------------------------

void	TrackView::do_mute_solo(BRect dst, long i)
{
	BRect	mute;
	BRect	solo;
	BPoint	where;
	ulong	but;
	char	vmute;
	char	vsolo;
	char	state;
	char	do_mute = 0;
	char	do_solo = 0;

	mute = dst;
	solo = dst;

	mute.bottom -= 12;
	solo.top += 12;

	get_mute_solo(viewers[i]->the_channel, &vmute, &vsolo);
	GetMouse(&where, &but);

	if (mute.Contains(where)) {
		do_mute = 1;
		vmute ^= 1;
	}
	else {
		do_solo = 1;
		vsolo ^= 1;
	}

	//if (do_solo && (vsolo == 1)) {
			//vmute = 0;
	//}

	set_mute_solo(viewers[i]->the_channel, vmute, vsolo);
	RefreshXtra();

	state = 1;

	do {
		Window()->Unlock();
		snooze(15000);
		Window()->Lock();

		GetMouse(&where, &but);
		if (do_mute) {
			if (mute.Contains(where) != state) {
				state ^= 1;
				vmute ^= 1;
				set_mute_solo(viewers[i]->the_channel, vmute, vsolo);
				RefreshXtra();
			}
		}
		else {
			if (solo.Contains(where) != state) {
				state ^= 1;
				vsolo ^= 1;
				set_mute_solo(viewers[i]->the_channel, vmute, vsolo);
				RefreshXtra();
			}
		}
	}  while(but);

}


//--------------------------------------------------------

static void	putnum(long cnum, char *p, char *s)
{
	char	c;

	if (cnum > 99) cnum = 99;
	
	if (cnum/10 > 0) {
		*p++ = 0x30 + (cnum/10);
	}

	*p++ = 0x30 + (cnum%10);
	*p++ = '-';
	do {
		c = *p++ = *s++;
	} while(c);
}

//--------------------------------------------------------


void	TrackView::draw_track_extra(long track_index, long vp)
{
	long	len;
	long	slen;
	char	buf[256];

	xtra_view->SetDrawingMode(B_OP_OVER);
	xtra_view->SetHighColor(0,80,180);
	xtra_view->MovePenTo(TITLE_LEFT, -1 + vp + viewers[track_index]->GetHeight() / 2);
	xtra_view->SetFontSize(11);
	putnum(track_index, buf, viewers[track_index]->Name());
	len = xtra_view->StringWidth(buf);

	if (len > (HEADER_SPACE - 35)) {
		slen = strlen(buf);
		do {
			slen -= 2;
			if (slen > 0) {
				buf[slen] = 0;
			}
			else
				goto out;
			len = xtra_view->StringWidth(buf);
		} while(len > (HEADER_SPACE-38));

out:;
		xtra_view->DrawString(buf);
		xtra_view->DrawString("...");
	}
	else
		xtra_view->DrawString(buf);

	xtra_view->SetDrawingMode(B_OP_COPY);

	BRect	r;
	long	vs;

	vs = viewers[track_index]->GetHeight()/2;

	vp += (vs);
	vp -= 2;

	if (vs > 12)
		vs = 12;
	
	r.top = vp - vs;
	r.bottom = vp + vs;
	r.left = HEADER_SPACE - 20;
	r.right = r.left + 12;

char	mute;
char	solo;
BRect	tmp_r;

	get_mute_solo(viewers[track_index]->the_channel, &mute, &solo);
	
	xtra_view->SetHighColor(80,200,80);
	xtra_view->FillRect(r);

	if (mute) {
		tmp_r = r;
		tmp_r.bottom = vp;
		xtra_view->SetHighColor(200,80,80);
		xtra_view->FillRect(tmp_r);
	}

	if (solo) {
		tmp_r = r;
		tmp_r.top = vp;
		xtra_view->SetHighColor(200,80,80);
		xtra_view->FillRect(tmp_r);
	}


	xtra_view->SetFontSize(9);
	xtra_view->SetHighColor(20,20,20);
	xtra_view->MovePenTo(BPoint(r.left + 3, vp - 2));
	xtra_view->SetDrawingMode(B_OP_OVER);
	xtra_view->DrawString("M");
	xtra_view->MovePenTo(BPoint(r.left + 4, r.bottom - 2));
	xtra_view->DrawString("S");
	xtra_view->SetDrawingMode(B_OP_COPY);
	xtra_view->SetHighColor(150,0,0);
	xtra_view->StrokeRect(r);
	xtra_view->MovePenTo(BPoint(r.left, vp));
	xtra_view->StrokeLine(BPoint(r.right, vp));
}

//--------------------------------------------------------

void	TrackView::draw_bar(long hp, long vp)
{
}

//--------------------------------------------------------

void	TrackView::SetZoom(float zoom_level)
{
	TimeView	*tv;

	zoom = zoom_level;
	Window()->Lock();
	Refresh();
	tv = (TimeView *)Window()->FindView("time");
	tv->set_parm(GetBaseTime(), zoom);
	tv->Draw(BRect(0,0,32000,32000));
	Window()->Unlock();
}

//--------------------------------------------------------

void TrackView::Draw(BRect)
{
	BRect	r;
	long	i;
	long	vp;
	BBitmap	*b;

	
	r = Bounds();

	vp = FIRST_TRACK_V;

	DrawBitmapAsync(xtra_off, BPoint(0,2));

	for (i = 0; i < nv; i++) {
		b = viewers[i]->GetBitmap();
		BRect	src,dst;

		src.top = 0;
		src.bottom = viewers[i]->GetHeight();
		src.left = 0;
		src.right = TRACK_B_H;
		dst = src;
		dst.OffsetBy(HEADER_SPACE, vp);
		viewers[i]->last_vp = (int)dst.top;
		DrawBitmapAsync(b, src,dst);
		
		vp += viewers[i]->GetHeight() + EXTRA_SPACE;
	}

	SetDrawingMode(B_OP_COPY);
}


//--------------------------------------------------------

void	TrackViewer::Load(long ref)
{
	long	sig;
	long	size;
	long	cnt;
	long	i;

	sig = read32(ref);
	size = read32(ref);
	track_name = (char *)malloc(size);
	read(ref, track_name, size);

	cnt = read32(ref);
	obj_count = cnt;
	obj_array = (TrackObject **)malloc(sizeof(TrackObject *) * (obj_count + 128));
	obj_count = obj_count;
	obj_array_size = (obj_count + 128);
	dirty = 1;
	for (i = 0; i < obj_count; i++) {
		obj_array[i] = new SimpleObject(this, ref);
	}
}

//--------------------------------------------------------

void	TrackViewer::Save(long ref)
{
	long	cnt;
	long	size;
	long	i;
	long	sig;

	sig = '!TRK';
	write32(ref, sig);

	size = strlen(track_name) + 1;
	if (size & 0x01)
		size++;
	write32(ref, size);
	write(ref, track_name, size);

	cnt = obj_count;
	write32(ref, cnt);

	for (i = 0; i < obj_count; i++) {
		obj_array[i]->Save(ref);
	}
}

//--------------------------------------------------------

void	TrackViewer::AddObject(TrackObject *an_object)
{
	TrackObject	**p;

	an_object->SetDad(this);	
	if (obj_count == obj_array_size) {
		p = (TrackObject **)malloc(sizeof(TrackObject *) * (obj_count + 256));
		memcpy(p, obj_array, obj_count * sizeof(TrackObject *));
		free((char *)obj_array);
		obj_array = p;
	}
	obj_array[obj_count] = an_object;
	obj_count++;
}

//--------------------------------------------------------


		TrackViewer::TrackViewer(const char *name, TrackView *main)
{
	dad = main;

	current_height = DEFAULT_HEIGHT;
	the_off = new BBitmap(BRect(0,0,TRACK_B_H-1,TRACK_HEIGHT+2),
						  B_COLOR_8_BIT,
						  TRUE);

	the_off->AddChild(off_view = new BView(BRect(0,0,TRACK_B_H,TRACK_HEIGHT),
										   "",
										   B_FOLLOW_ALL,
										    B_WILL_DRAW));

	track_name = (char *)malloc(strlen(name) + 1);
	memcpy(track_name, name, strlen(name) + 1);

	obj_array = (TrackObject **)malloc(sizeof(TrackObject *) * 256);
	obj_count = 0;
	obj_array_size = 256;
	dirty = 1;
	selected = 0;

	InternalDraw();
}

//--------------------------------------------------------

	TrackViewer::TrackViewer(long ref, TrackView *main)
{
	long	sig;

	dad = main;

	current_height =DEFAULT_HEIGHT;
	the_off = new BBitmap(BRect(0,0,TRACK_B_H-1,TRACK_HEIGHT+2),
						  B_COLOR_8_BIT,
						  TRUE);

	the_off->AddChild(off_view = new BView(BRect(0,0,TRACK_B_H,TRACK_HEIGHT),
										   "",
										   B_FOLLOW_ALL,
										    B_WILL_DRAW));

	track_name = 0;
	selected = 0;

	Load(ref);
	InternalDraw();
}

//--------------------------------------------------------

char	TrackViewer::fill_buffer(short *buffer, long count, long pos)
{
	double	time_start;
	double	time_end;
	double	delta_time;
	float	vh1, vh2;
	long	tmp_count;
	long	tmp_pos;
	long	skip;
	long	i;
	char	got_some = 0;

	time_start = pos / 44100.0;
	time_end = (pos+count) / 44100.0;

	memset(buffer, 0, count*2);

	for (i = 0; i < obj_count; i++) {
		if (!obj_array[i]->Ghost()) {
			obj_array[i]->GetRange(&vh1, &vh2);
			if ((vh1 <= time_end) && (vh2 >= time_start)) {
				delta_time = (vh1 - time_start);
				tmp_pos = delta_time * 44100.0;
				tmp_count = count - tmp_pos;
				if (tmp_pos < 0)
					got_some |= obj_array[i]->GetData(buffer, -tmp_pos, count);
				else 
					got_some |= obj_array[i]->GetData(buffer + tmp_pos, 0, tmp_count);
			}
		}
	}
	return got_some;
}

//--------------------------------------------------------


void	TrackViewer::redo_time()
{
	long	i,j;
	float	cur;
	float	h1,h2;
	float	vh1,vh2;

	if (obj_count == 0)
		return;

	j = 0;

	do {
		if (!obj_array[j]->Ghost()) {
			obj_array[j]->GetRange(&h1, &h2);
			goto out;
		}
		j++;
	} while(j < obj_count);

out:;
	j++;

	for (i = j; i < obj_count; i++) {
		if (!obj_array[i]->Ghost()) {
			obj_array[i]->GetRange(&vh1, &vh2);
				if (vh1 < h2) {
					obj_array[i]->SetPos(h2);
				}
				
			obj_array[i]->GetRange(&h1, &h2);
		}
	}
	dad->Refresh(dad->find_index(this));
}


//--------------------------------------------------------

		TrackViewer::~TrackViewer()
{
	long	i;

	remove_channel(the_channel);
	for (i = 0; i < obj_count; i++)
		delete obj_array[i];

	delete the_off;
	free(track_name);
}

//--------------------------------------------------------

void	TrackViewer::SetName(const char *n)
{
	free((char *)track_name);

	track_name = (char *)malloc(strlen(n) + 1);
	memcpy(track_name, n, strlen(n) + 1);
	the_channel->SetName(n);
}

//--------------------------------------------------------

char	*TrackViewer::Name()
{
	return (track_name);
}

//--------------------------------------------------------

Channel		*TrackViewer::GetChannel()
{
	return the_channel;
}

//--------------------------------------------------------
void	TrackViewer::SetChannel(Channel *c)
{
	the_channel = c;
}

//--------------------------------------------------------

void	TrackViewer::SetHeight(long v)
{
	if (v > TRACK_HEIGHT)
		v = TRACK_HEIGHT;

	if (v < MIN_TRACK_HEIGHT)
		v = MIN_TRACK_HEIGHT;

	current_height = v;
}

//--------------------------------------------------------

long	TrackViewer::GetHeight()
{
	return current_height;
}

//--------------------------------------------------------

BBitmap	*TrackViewer::GetBitmap()
{
	return the_off;
}

//--------------------------------------------------------


long	TrackViewer::find_part(BPoint where)
{
	if ((where.y > current_height - 10) &&
	    (where.x < 10))
		return CORNER;

	return CONTENT;
}

//--------------------------------------------------------


TrackObject	*TrackViewer::find_object(BPoint where, char right_extend)
{
	float	off_h;
	long	i,j;
	float	h1,h2;

	off_h = dad->base_time;
	

	for (j = 0; j < obj_count; j += 20) {
		obj_array[j]->GetRange(&h1, &h2);
		if (right_extend)
			h2 += (14.0*dad->zoom);
		h2 -= off_h;
		if (h2 >= 0) {
			goto out0;
		}
	}

out0:;

	j -= 20;
	if (j < 0) 
		j = 0;


	for (i = j; i < obj_count; i++) {
		obj_array[i]->GetRange(&h1, &h2);
		h1 -= off_h;
		h2 -= off_h;
		if (right_extend)
			h2 += (14.0*dad->zoom);
		
		if (h1 > TRACK_B_H) {
			goto out;
		}

		if (h1 <= where.x && h2 >= where.x) {
			return obj_array[i];
		} 
	}

out:;

	return 0;
}


//--------------------------------------------------------

long	TrackViewer::find_insert(BPoint where)
{
	float	off_h;
	long	j;
	float	h1,h2;
	long	best;
	float	best_dist;
	float	dist;

	best = -1;

	off_h = dad->base_time;
	
	best_dist = 1e10;

	for (j = 0; j < obj_count; j ++) {
		obj_array[j]->GetRange(&h1, &h2);
		h2 -= off_h;
		h1 -= off_h;

		dist = fabs(h1 - where.x);			//distance between start of object and cur point
		if (dist < best_dist) {
			best_dist = dist;
			best = j;
		}
		dist = fabs(h2 - where.x);			//distance between end of object and cur point
		if (dist < best_dist) {
			best_dist = dist;
			best = j+1;
		}

		if (h2 >= where.x) {
			goto out0;
		}
	}
out0:;

	return best;
}

//--------------------------------------------------------

void	TrackView::unselect_all()
{
	long	i;

	for (i = 0; i < nv; i++) 
		viewers[i]->unselect_all();
}

//--------------------------------------------------------

void	TrackViewer::unselect_all()
{
	TrackObject	*obj;
	long		i;
	
	for (i = 0; i < obj_count; i++) {
		obj_array[i]->Select(0);
	}
	dad->Refresh(dad->find_index(this));
}

//--------------------------------------------------------

void	TrackViewer::sync_positions()
{
	long			i;

	for (i = 0; i < obj_count; i++) {
		obj_array[i]->sync_positions();
	}
}

//--------------------------------------------------------

char	TrackViewer::MoveSelection(float dh)
{
	float			new_pos;
	float			p0, p1;
	long			i;
	char			yes = 0;
	TimeView		*t;
	long			bpm;
	float			ipos;
	long			int_v;

	t = (TimeView*)dad->Window()->FindView("time");
	bpm = t->Beat();
	cur_ibeat = 0;

	if ((modifiers() & B_CONTROL_KEY) != 0) {
		cur_ibeat = 60.0/bpm;
	}

	for (i = 0; i < obj_count; i++) {
		if (obj_array[i]->Selected()) {
			obj_array[i]->GetRangeNoGrid(&p0, &p1);
					new_pos = p0 - dh;
					if (new_pos < 0)
						new_pos = 0;
					obj_array[i]->SetPos(new_pos);
				yes = 1;
		}
	}

	cur_ibeat = 0;

	return yes;
}

//--------------------------------------------------------

void	TrackView::sync_positions()
{
	long	i;

	for (i = 0; i < nv; i++) {
		viewers[i]->sync_positions();
	}
}

//--------------------------------------------------------

void	TrackView::MoveSelection(float dh)
{
	long	i;

	for (i = 0; i < nv; i++) {
		if (viewers[i]->MoveSelection(dh))
			viewers[i]->redo_time();
	}
}

//--------------------------------------------------------

void	TrackView::SetInfo(BPoint p, TrackObject *tt)
{
	BRect		r;
	BRect		dad_rect;

	dad_rect = this->Bounds();
	dad_rect.OffsetBy(-HEADER_SPACE, 0);
	dad_rect.left += 50;
	if (p.x < (dad_rect.left + 5))
		p.x = dad_rect.left + 5;
	if (p.x > (dad_rect.right - 69))
		p.x = dad_rect.right - 69;
	
	cur_info_pos = p.x;
	cur_info_pos_base = p.x;

	p.x += HEADER_SPACE;
	r.top = p.y + 50;
	r.bottom = p.y + 125;
	r.left = p.x - 65;
	r.right = p.x + 65;
	cur_info = new InfoBlock(r, tt);

	cur_infohairline1 = new HairLine(p.x, p.y, p.x, p.y + 20);
	cur_infohairline2 = new HairLine(p.x, p.y+20, p.x, p.y + 21);
	cur_infohairline3 = new HairLine(p.x, p.y+21, p.x, p.y + 49);
	
	AddChild(cur_info);
	AddChild(cur_infohairline1);
	AddChild(cur_infohairline2);
	AddChild(cur_infohairline3);
}

//--------------------------------------------------------

void	TrackView::MoveInfo(long new_h)
{
	long		dh;
	BRect		frame;
	BRect		dad_rect;

	dad_rect = this->Bounds();
	dad_rect.OffsetBy(-HEADER_SPACE, 0);
	dad_rect.left += 120;
	if (new_h < (dad_rect.left + 5))
		new_h = dad_rect.left + 5;
	if (new_h > (dad_rect.right - 5))
		new_h = dad_rect.right - 5;

	dh = new_h - cur_info_pos;
	cur_info_pos = new_h;
	
	frame = cur_infohairline2->Frame();
	if (new_h >= cur_info_pos_base) {
		cur_infohairline2->ResizeTo(new_h - cur_info_pos_base, 1);
		cur_infohairline2->MoveTo(BPoint(cur_info_pos_base + HEADER_SPACE - 1, frame.top));
	}
	else {
		cur_infohairline2->ResizeTo(cur_info_pos_base - new_h, 1);
		cur_infohairline2->MoveTo(BPoint(new_h + HEADER_SPACE, frame.top));
	}

	cur_info->Draw(BRect(0,0,1000,1000));
	cur_infohairline1->MoveBy(dh, 0);
}

//--------------------------------------------------------

void	TrackView::RemoveInfo()
{
	RemoveChild(cur_info);
	delete cur_info;
	RemoveChild(cur_infohairline1);
	RemoveChild(cur_infohairline2);
	RemoveChild(cur_infohairline3);
	delete cur_infohairline1;
	delete cur_infohairline2;
	delete cur_infohairline3;
}

//--------------------------------------------------------

void	TrackViewer::ChangeGizmos()
{
	BPoint		w0;
	BPoint		w;
	ulong		but;
	TrackObject	*obj;
	float		loop0;
	float		new_loop;
	float		dh;
	float		h1,h2;
	float		sx;
	float		dh0;

	GetMouse(&w, &but);
	obj = find_object(w, 0);
	if (obj == 0) {
		obj = find_object(w, 1);
		if (obj == 0) {
			return;
		}
	}

	loop0 = obj->LoopPoint();
	
	obj->GetRange(&h1, &h2);

	h1 += obj->LoopPointNoGrid();
	h1 -= dad->base_time;
	h1 /= dad->zoom;
	sx = w.x / dad->zoom;

	if (fabs(sx-h1) > 5)
		return;

	dh0 = -32000;

	loop0 = obj->LoopPointNoGrid();

	dad->SetInfo(BPoint(sx, last_vp + GetHeight()), obj);
	do {
		GetMouse(&w0, &but);
		dh = w.x - w0.x;
		if (dh != 0) {
			new_loop = loop0 - dh;
			if (new_loop < 0)
					new_loop = 0;

			if (dh0 != dh) {
				obj->SetLoopPoint(new_loop);
				redo_time();

				obj->GetRange(&h1, &h2);

				h1 += obj->LoopPoint();
				h1 -= dad->base_time;
				h1 /= dad->zoom;

				dad->FeedBackLine(h1, this, 0, mc(0,0,180));
				dad->MoveInfo(h1);
				obj->GetRange(&h1, &h2);

				h1 += obj->LoopPoint()/2.0;
				h1 -= dad->base_time;
				h1 /= dad->zoom;

				dad->FeedBackLine(h1, this, 1, mc(200,250,250));
				dh0 = dh;
			}
		}

		dad->Window()->Unlock();
		snooze(18000);
		dad->Window()->Lock();
	} while(but);
	dad->FeedBackLine(-32000, 0, 0, mc(0,0,0));
	dad->FeedBackLine(-32000, 0, 1, mc(0,0,0));
	dad->RemoveInfo();
}

//--------------------------------------------------------

int		TrackViewer::Selected()
{
	return selected;
}

//--------------------------------------------------------

void	TrackView::Select(long i, long v)
{
	viewers[i]->SetSelected(v);
}

//--------------------------------------------------------

void	TrackView::redo_mute_solo()
{
	RefreshXtra();	
	Flush();
}

//--------------------------------------------------------

void	TrackViewer::SetSelected(int v)
{
	selected = v;
	dad->Refresh(this);
	dad->my3d->select(the_channel, v);
}

//--------------------------------------------------------


void	TrackViewer::handle_hit(BPoint where)
{
	char		need_move;
	char		need_size;
	char		need_move_size;
	ulong		but;
	long		index;
	TrackObject	*obj;
	BPoint		w;
	BPoint		w0;
	float		l0;
	float		p0;
	float		dh;
	float		dh0;
	float		h1,h2;	
	float		th1,th2;	
	float		mh;
	float		new_pos;
	float		new_length;
	float		sk0;
	float		dt;

	need_move = 0;
	need_move_size = 0;
	need_size = 0;
	dh0 = -32000;
	TimeView	*t = (TimeView*)dad->Window()->FindView("time");

	float bpm = t->Beat();
	cur_ibeat = 0;

	if ((modifiers() & B_CONTROL_KEY) != 0) {
		cur_ibeat = 60.0/bpm;
	}


	GetMouse(&w, &but);
	index = find_insert(w);
	dad->last_index_md = index;
	dad->last_track_md = dad->find_index(this);

	obj = find_object(w);

	if (obj == 0) {
		obj = find_object(w, 1);
	}
	if (obj == 0) {
		dad->deselect_others(this);
		this->SetSelected(Selected() ^ 0x01);
		dad->Refresh(this);
	}

	if (obj) {
		if (w.y > (GetHeight() - 8)) {
			ChangeGizmos();
			cur_ibeat = 0;
			sync_positions();
			return;
		}
		
		
		if (dad->last_click) {
			if (dad->last_click == obj) {
				dt = system_time() - dad->click_when;
				dt /= 1000.0;
				if (dt < 400.0) {
					obj->Edit();
					return;
				}
			}
		}
		
		dad->click_when = system_time();
		dad->last_click = obj;

		obj->GetRange(&h1, &h2);
		h1 -= dad->base_time;
		h2 -= dad->base_time;
		h1 /= dad->zoom;
		h2 /= dad->zoom;
		mh = w.x;
		mh /= dad->zoom;	
		if ((h2 - h1) > 16) {
			if ((h2 - mh) < 8)
				need_size = 1;
			if ((mh - h1) < 8)
				need_move_size = 1;
		}

		need_move = (need_size | need_move_size) ^ 0x01;

		if (need_move) {
			dad->SetInfo(BPoint((h1+h2)/2.0, last_vp + GetHeight()), obj);
		}

		if ((modifiers() & B_SHIFT_KEY) == 0) {
			dad->unselect_all();
		}
		obj->Select(1);
		l0 = obj->LengthNoGrid();
		p0 = obj->PosNoGrid();
		sk0 = obj->StartSkip();

		if (need_move_size) {
			dad->SetInfo(BPoint(h1, last_vp + GetHeight()), obj);
		}
		else
		if (need_size) {
			dad->SetInfo(BPoint(h2, last_vp + GetHeight()), obj);
		}

		do {
			GetMouse(&w0, &but);
			dh = w.x - w0.x;
			if (dh == dh0)
				goto same_point;

			dh0 = dh;

			if (need_move) {
				obj->GetRange(&th1, &th2);
				th1 += th2;
				th1 /= 2.0;
				th1 -= dad->base_time;
				th1 /= dad->zoom;
				dad->MoveInfo(th1);
			}
			if (need_move_size) {
				obj->SetStartSkip(sk0 - dh);
				dh = sk0 - obj->StartSkip();
				new_pos = p0 - dh;
				if (new_pos < 0) {
					dh = -new_pos;
					new_pos = 0;
				}
				obj->SetPos(new_pos);
				new_length = l0 + dh;
				if (new_length < 0)
					new_length = 0;
				obj->SetLength(new_length);
				redo_time();
				obj->GetRange(&th1, &th2);
				th1 -= dad->base_time;
				th1 /= dad->zoom;
				dad->MoveInfo(th1);
			}
			else {
				if (need_size) {
					obj->GetRange(&th1, &th2);
					th2 -= dad->base_time;
					th2 /= dad->zoom;
					dad->MoveInfo(th2);
				
				}
				else
				if (need_move) {
				}

				if (need_move) {
					dad->MoveSelection(dh);
					w = w0;

					obj->GetRange(&th1, &th2);

					th2 -= dad->base_time;
					th2 /= dad->zoom;

					dad->FeedBackLine(th2-1, this,0, mc(0,0,200));
				
					th1 -= dad->base_time;
					th1 /= dad->zoom;
					dad->FeedBackLine(th1+1, this,1, mc(200,0,0));
				}

				if (need_size) {
					new_length = l0 - dh;
					if (new_length < 0)
						new_length = 0;
					obj->SetLength(new_length);
					redo_time();
					obj->GetRange(&th1, &th2);

					th2 -= dad->base_time;
					th2 /= dad->zoom;

					dad->FeedBackLine(th2-1, this, 0, mc(0,0,200));
				}
			}

same_point:;
			dad->Window()->Unlock();
			snooze(18000);
			dad->Window()->Lock();
		} while(but);
	}
	else
		goto no_hit;
out:;
	dad->FeedBackLine(-32000, this, 0, mc(0,0,0));
	dad->FeedBackLine(-32000, this, 1, mc(0,0,0));
	dad->RemoveInfo();
	return;

no_hit:;
	if ((modifiers() & B_SHIFT_KEY) == 0) {
		dad->unselect_all();
	}
	cur_ibeat = 0;
	sync_positions();
}

//--------------------------------------------------------
// GetMouse in Track coordinate space (time !!)
//--------------------------------------------------------

void	TrackViewer::GetMouse(BPoint *where, ulong *but)
{
	dad->GMouse(where, but, this);
	where->x *= dad->zoom;
}

//--------------------------------------------------------
// Main drawing of all the objects in a track.
// Rendering is done in the off-screen bitmap associated 
// with the track
//--------------------------------------------------------


void	TrackViewer::InternalDraw()
{
	BRect	r;
	long	i,j;
	long	v0;
	long	v;
	char	*base;
	float	vscale;
	float	h1,h2;
	float	start;
	float	end;

	r.top = 0;
	r.left = 0;
	r.bottom = current_height;
	r.right = TRACK_B_H - 1;

	base = (char *)the_off->Bits();


	the_off->Lock();

	if (selected)
		off_view->SetHighColor(backgr1s);
	else
		off_view->SetHighColor(backgr1);

	off_view->FillRect(r);
	off_view->Sync();
	v0 = 30;

	start = dad->base_time;			// this is time0 !
	end = start + (TRACK_B_H * dad->zoom);

	for (j = 0; j < obj_count; j += 20) {
		obj_array[j]->GetRange(&h1, &h2);
		h2 -= start;
		if (h2 >= 0) {
			goto out0;
		}
	}

out0:;

	j -= 20;
	if (j < 0) 
		j = 0;


	off_view->SetDrawingMode(B_OP_OVER);
	for (i = j; i < obj_count; i++) {
		if (!obj_array[i]->Ghost()) {
			obj_array[i]->GetRange(&h1, &h2);
			h1 -= start;
			h2 -= start;
			if (h1 > end) {
				goto out;
			}

			if (h2 >= 0)
				obj_array[i]->Render(start, current_height, dad->zoom);
		}
	}

out:;

	off_view->SetDrawingMode(B_OP_COPY);
	off_view->Sync();

	fline(0, current_height-2, 0, current_height - 2, 18);
	fline(0, current_height-3, 1, current_height - 2, 16);
	fline(0, current_height-4, 2, current_height - 2, 14);
	fline(0, current_height-5, 3, current_height - 2, 12);
	fline(0, current_height-6, 4, current_height - 2, 10);
	fline(0, current_height-7, 5, current_height - 2, 8);
	fline(0, current_height-8, 6, current_height - 2, 6);

	dirty = 0;
	r.bottom-=2;
	r.right-=2;
	off_view->SetHighColor(40,40,40);
	off_view->StrokeRect(r);
	r.bottom++;
	r.right++;
	off_view->SetHighColor(120,120,120);
	off_view->MovePenTo(BPoint(r.right, r.top+2));
	off_view->StrokeLine(BPoint(r.right, r.bottom));
	off_view->StrokeLine(BPoint(r.left + 2, r.bottom));
	r.bottom++;
	r.right++;
	off_view->MovePenTo(BPoint(r.right, r.top+2));
	off_view->StrokeLine(BPoint(r.right, r.bottom));
	off_view->StrokeLine(BPoint(r.left + 2, r.bottom));
	off_view->SetHighColor(backgr);

	off_view->MovePenTo(BPoint(r.left, r.bottom));
	off_view->StrokeLine(BPoint(r.left + 1, r.bottom));
	off_view->MovePenTo(BPoint(r.left, r.bottom-1));
	off_view->StrokeLine(BPoint(r.left + 1, r.bottom-1));
	
	off_view->MovePenTo(BPoint(r.right, r.top));
	off_view->StrokeLine(BPoint(r.right - 1, r.top));
	off_view->MovePenTo(BPoint(r.right, r.top + 1));
	off_view->StrokeLine(BPoint(r.right - 1, r.top + 1));
	
	off_view->Sync();
	the_off->Unlock();
}

//--------------------------------------------------------
// Used for object management in tracks...
// Will insert a new object <obj> just after object <pos>
// in the track
//--------------------------------------------------------

void	TrackViewer::InsertAfter(long pos, TrackObject *obj)
{
	long	i;
	float	h1,h2;

	if (pos < 0)
		pos = 0;

	obj->SetDad(this);	
	h1 = 0;
	h2 = 0;

	if (pos < obj_count)
		obj_array[pos]->GetRange(&h1, &h2);

	AddObject(obj);			//just to get the array size right
	
	if (pos == 0)
		obj->SetPos(0);
	else
		obj->SetPos(h2);
	

	for (i = obj_count + 1; i > pos; i--) {
		obj_array[i] = obj_array[i - 1];
	} 

	obj_array[pos] = obj;
}

//--------------------------------------------------------
// For debugging purpose... will tell you everything
//--------------------------------------------------------

void	TrackViewer::DumpTrack()
{
	long	i;

	for (i = 0;i < obj_count; i++) {
		obj_array[i]->Dump();
	}
}

//--------------------------------------------------------

char	TrackViewer::has_selection()
{
	long	i;

	for (i = 0; i < obj_count; i++) {
		if (obj_array[i]->Selected()) return 1;
	}

	return 0;
}

//--------------------------------------------------------

void	TrackViewer::delete_selection()
{
	long			i;
	long			dst;

	dst = 0;

	for (i = 0; i < obj_count; i++) {
		if (obj_array[i]->Selected()) {
			obj_array[i]->Select(0);
			obj_array[i]->SetGhost(1);
		}
	}

	i = 0;

	redo_time();
	dad->Refresh(dad->find_index(this));
}

//--------------------------------------------------------
// bring back to life items deleted during the previous
// command.
// This is used when UNDO.
//--------------------------------------------------------

void	TrackViewer::revive_ghosts()
{
	long	i;
	long	dst;

	dst = 0;

	for (i = 0; i < obj_count; i++) {
		if (obj_array[i]->Ghost()) {
			obj_array[i]->SetGhost(0);
		}
	}
	redo_time();
	dad->Refresh(dad->find_index(this));
}


//--------------------------------------------------------
// clean up items marked for delete when the previous undo
// state has been used.
//--------------------------------------------------------

void	TrackViewer::remove_ghosts()
{
	long	i;
	long	dst;

	dst = 0;

	for (i = 0; i < obj_count; i++) {
		if (obj_array[i]->Ghost()) {
			delete obj_array[i];
		}
		else {
			obj_array[dst] = obj_array[i];
			dst++;
		}
	}
	obj_count = dst;
	redo_time();
	dad->Refresh(dad->find_index(this));
}

//--------------------------------------------------------

void	TrackViewer::HandleDrop(BPoint where, const char *name, const char *path)
{
	long			index;
	SimpleObject	*obj;			
	Silence			*sil;
	char			need_a_silence;

	where.x *= dad->zoom;

	index = find_insert(where);
	obj = new SimpleObject(this, 2.0, 3.0, name, path);
	
	if (index < 0)
		index = 0;

	InsertAfter(index, obj);
	obj->SetPos(where.x);

	redo_time();
	dad->Refresh(dad->find_index(this));
}

//--------------------------------------------------------

#include "track_viewer_render.cpp"
#include "util_views.cpp"
#include "time_view.cpp"
//-------------------------------------------------------------------------
