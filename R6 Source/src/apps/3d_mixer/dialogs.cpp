#include <FindDirectory.h>
#include <Path.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Debug.h>
#include <Application.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <ScrollBar.h>
#include <Button.h>
#include <Screen.h>
#include <Entry.h>
#include <TextControl.h>
#include <TextView.h>
#include <Slider.h>
#include <stdlib.h>
#include <StringView.h>

#include "dialogs.h"

//------------------------------------------------------------
const rgb_color back_color_light = {216,216,216,255};
const rgb_color back_color_dark = {184,184,184,255};
//------------------------------------------------------------

static	long	cur_cnt;
static	char	done;
static	float	f1;
static	float	f2;
static	float	f3;

//------------------------------------------------------------
 
BWindow	*front_window()
{
	for (int32 index = 0;;) {
		BWindow *window = be_app->WindowAt(index);
		index++;
		if (!window)
			break;
		if (window->IsFront())
			return window;
	}
	
	return 0;
}

//------------------------------------------------------------

long	cx()
{	
	BScreen	s;
	long	x;

	x = (int)(s.Frame().right + s.Frame().left);
	x /= 2;
	return x;
}

//------------------------------------------------------------

long	cy()
{	
	BScreen	s;
	long	y;

	y = (int)(s.Frame().right + s.Frame().left);
	y /= 2;
	return y;
}


//------------------------------------------------------------

BRect	norm(long xs, long ys)
{
	BWindow	*front;
	BScreen	s;
	BRect	frame;
	BRect	out;
	long	mx,my;

	front = front_window();

	frame = front->Frame();

	mx = (int)((frame.right + frame.left) / 2);
	my = (int)((frame.top + frame.bottom) / 2);

	out = BRect((mx - xs/2), (my - ys/2), (mx - xs/2) + xs, (my - ys/2) + ys);

	frame = s.Frame();

	frame.InsetBy(30, 20);

	if (out.right > frame.right) {
		out.OffsetBy(frame.right - out.right, 0);
	}
	if (out.left < frame.left) {
		out.OffsetBy(frame.left - out.left, 0);
	}

	if (out.bottom > frame.bottom) {
		out.OffsetBy(0, frame.bottom - out.bottom);
	}
	if (out.top < frame.top) {
		out.OffsetBy(0, frame.top - out.top);
	}

	
	return out;
}

//------------------------------------------------------------

class	bInfoBlock : public BView {
public:
				bInfoBlock(BRect r, float time, long cnt);
				~bInfoBlock();
		void	Draw(BRect r);
		void	set_count(long cnt);
private:
	
		float	time;
		long	cnt;
};

//------------------------------------------------------------

bInfoBlock::bInfoBlock(BRect r, float vtime, long vcnt)
	   : BView(r, "info", B_FOLLOW_NONE, B_WILL_DRAW)
{
	time = vtime;
	cnt = vcnt;
	cur_cnt = cnt;
	SetViewColor(back_color_light);
}

/*------------------------------------------------------------*/

void	bInfoBlock::set_count(long vcnt)
{
	cnt = vcnt;
	cur_cnt = cnt;
	Invalidate();
}

/*------------------------------------------------------------*/

bInfoBlock::~bInfoBlock()
{
}

/*------------------------------------------------------------*/

void	bInfoBlock::Draw(BRect r)
{
	char	buf[256];
	BRect	br;
	float	h1,h2;
	long	vp;
	BRect	bnd;
	float	t;

	bnd = Bounds();
	bnd.InsetBy(2,2);
	SetHighColor(140,100,100);
	StrokeRect(bnd);

	SetHighColor(0,0,80);
	SetFont(be_fixed_font);
	SetFontSize(12);

	sprintf(buf, "selected length = %2.2f seconds", time);
	MovePenTo(BPoint(20, 24));
	DrawString(buf);

	t = time / cnt;

	sprintf(buf, "single tick     = %2.2f seconds", t);
	MovePenTo(BPoint(20, 39));
	DrawString(buf);


	sprintf(buf, "ticks/minutes    = %2.2f", 60.0/t);
	MovePenTo(BPoint(20, 53));
	DrawString(buf);
	Sync();
}

/*------------------------------------------------------------*/
	
		
	AskBeat::AskBeat(float duration)
	: BWindow( norm(330, 230),
 			   "Beat Info",
			    B_MODAL_WINDOW,
			    B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	BRect	bnd;
	BRect	r;
	BButton	*ok;
	long	vsize;
	long	hsize;

	bnd = Bounds();
	hsize = (int)(bnd.right - bnd.left);
	vsize = (int)(bnd.bottom - bnd.top);
	
	
	master = new BView(bnd, "master", 0, B_WILL_DRAW);
	master->SetViewColor(back_color_light);
	
	AddChild(master);



	master->AddChild(new BButton(BRect(hsize * 0.15,vsize - 45, hsize * 0.4, vsize - 25),
			 		"Cancel",
			 		"Cancel",
			 		new BMessage(32000)));

	master->AddChild(ok = 
					 new BButton(BRect(hsize * 0.6,vsize - 45, hsize * 0.85, vsize - 25),
			 		"OK",
			 		"OK",
			 		new BMessage(32001)));
	
	SetDefaultButton(ok);
	
	r.left = 33;
	r.right = 310;
	r.top = 22;
	r.bottom = 35;
	cur_beat = new BTextControl(r, "Ticks:", "Number of ticks in selection:", "8", 0, 0);
	cur_beat->SetModificationMessage(new BMessage('kkkk'));
	master->AddChild(cur_beat);

	BTextView		*t;
	
	t = cur_beat->TextView();
	
	t->ResizeBy(-110, 0);

	bInfoBlock		*b;

	b = new bInfoBlock(BRect(hsize * 0.1, 65, hsize * 0.9, 145), duration, 8);

	master->AddChild(b);

	Show();
}


//------------------------------------------------------------

	AskBeat::~AskBeat()
{
	done = 1;
}

//------------------------------------------------------------

void	AskBeat::new_value()
{
	char			buf[256];
	long			v;
	bInfoBlock		*b;

	strcpy(buf, cur_beat->Text());
	v = atol(buf);

	if (v < 1)  v = 1;
	if (v >256) v = 256;

	b = (bInfoBlock *)FindView("info");
	b->set_count(v);
}

//------------------------------------------------------------

void	AskBeat::MessageReceived(BMessage *msg)
{
	if (msg->what == 32000) {
		cur_cnt = -1;
		Close();
	}
	if (msg->what == 32001) {
		Close();
	}
	if (msg->what == 'kkkk') {
		new_value();
	}
}

//------------------------------------------------------------

long	do_ask_beat(float length, BWindow *me)
{
	AskBeat	*b;

	done = 0;
	b = new AskBeat(length);
	while(done == 0) {
		me->Lock();
		me->UpdateIfNeeded();
		me->Unlock();
		snooze(82000);
	}

	return cur_cnt;
}

//------------------------------------------------------------

class	StrBSlider : public BSlider {
public:
			StrBSlider(BRect frame, const char *name, const char *label, BMessage *message,
					 int32 minValue, int32 maxValue, thumb_style thumbType,
					 uint32 resizingMode, uint32 flags);

virtual	char	*UpdateText() const;	

private:

		char	text[256];
};

//------------------------------------------------------------

	StrBSlider::
	StrBSlider(BRect frame, const char *name, const char *label, BMessage *message,
			 int32 minValue, int32 maxValue, thumb_style thumbType=B_BLOCK_THUMB,
			 uint32 resizingMode=B_FOLLOW_LEFT|B_FOLLOW_TOP, uint32 flags=B_NAVIGABLE|B_WILL_DRAW|B_FRAME_EVENTS) :
	BSlider(frame, name, label, message,
			 minValue, maxValue, thumbType,
			 resizingMode, flags)
{
}


//------------------------------------------------------------


char	*StrBSlider::UpdateText() const
{
	float	v;

	v = Value();
	sprintf(const_cast<StrBSlider *>(this)->text, "%2.1f %", v);
	return const_cast<StrBSlider *>(this)->text;
}



//------------------------------------------------------------

class	DurBSlider : public BSlider {
public:
			DurBSlider(BRect frame, const char *name, const char *label, BMessage *message,
					 int32 minValue, int32 maxValue, thumb_style thumbType,
					 uint32 resizingMode, uint32 flags);

virtual	char	*UpdateText() const;	

private:

		char	text[256];
};

//------------------------------------------------------------

	DurBSlider::
	DurBSlider(BRect frame, const char *name, const char *label, BMessage *message,
			 int32 minValue, int32 maxValue, thumb_style thumbType=B_BLOCK_THUMB,
			 uint32 resizingMode=B_FOLLOW_LEFT|B_FOLLOW_TOP, uint32 flags=B_NAVIGABLE|B_WILL_DRAW|B_FRAME_EVENTS) :
	BSlider(frame, name, label, message,
			 minValue, maxValue, thumbType,
			 resizingMode, flags)
{
}


//------------------------------------------------------------


char	*DurBSlider::UpdateText() const
{
	long	v;

	v = Value();
	sprintf(const_cast<DurBSlider *>(this)->text, "%2d msec", v);
	return (const_cast<DurBSlider *>(this)->text);
}


//------------------------------------------------------------

#define	STRENGTH	32005
#define	DELAY		32006

//------------------------------------------------------------
		
	AskFilter::AskFilter(char *title)
	: BWindow( norm(330, 230),
 			   title,
			    B_MODAL_WINDOW,
			    B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	BRect		bnd;
	BRect		r;
	BButton		*ok;
	long		vsize;
	long		hsize;
	BSlider		*s1;
	BSlider		*s2;
	char		buf[256];

	
	bnd = Bounds();
	hsize = (int)(bnd.right - bnd.left);
	vsize = (int)(bnd.bottom - bnd.top);
	
	
	master = new BView(bnd, "master", 0, B_WILL_DRAW);
	master->SetViewColor(back_color_light);

	sprintf(buf, "%s Settings", title);
	BStringView *s = new BStringView(BRect(5,5, 200, 20), "", buf);
	s->SetFontSize(14);
	master->AddChild(s);
	
	AddChild(master);



	master->AddChild(new BButton(BRect(hsize * 0.15,vsize - 45, hsize * 0.4, vsize - 25),
			 		"Cancel",
			 		"Cancel",
			 		new BMessage(32000)));

	master->AddChild(ok = 
					 new BButton(BRect(hsize * 0.6,vsize - 45, hsize * 0.85, vsize - 25),
			 		title,
			 		title,
			 		new BMessage(32001)));
	
	SetDefaultButton(ok);
	
	BRect	r1;

	r1.left = hsize * 0.2;
	r1.right = hsize * 0.8;
	r1.top = 40;
	r1.bottom = 80;

	s1 = new StrBSlider(r1, "Strength", "Strength", new BMessage(STRENGTH), 0, 100.0, B_TRIANGLE_THUMB);
	s1->SetValue(f1);
	s1->SetLimitLabels("0.0%","100.0%");
	rgb_color c;
	c.green = c.red = 102; c.blue = 152;
	s1->UseFillColor(true, &c);
	master->AddChild(s1);

	r1.OffsetBy(0,60);
	s2 = new DurBSlider(r1, "Delay", "Delay", new BMessage(DELAY), 1, 1000.0, B_TRIANGLE_THUMB);
	s2->SetValue(f2);
	s2->SetLimitLabels("1 msec","1 sec");
	c.green = c.red = 102; c.blue = 152;
	s2->UseFillColor(true, &c);
	master->AddChild(s2);


	Show();
}


//------------------------------------------------------------

	AskFilter::~AskFilter()
{
	done = 1;
}

//------------------------------------------------------------

void	AskFilter::new_value()
{
	f1 = ((BSlider *)(FindView("Strength")))->Value();
	f2 = ((BSlider *)(FindView("Delay")))->Value();
}

//------------------------------------------------------------

void	AskFilter::MessageReceived(BMessage *msg)
{
	if (msg->what == 32000) {
		f1 = -1;
		f2 = -1;
		Close();
	}
	if (msg->what == 32001) {
		Close();
	}
	if (msg->what == STRENGTH) {
		new_value();
	}
	if (msg->what == DELAY) {
		new_value();
	}
}

//------------------------------------------------------------
	
	AskStrength::AskStrength(char *title)
	: BWindow( norm(330, 180),
 			   title,
			    B_MODAL_WINDOW,
			    B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	BRect		bnd;
	BRect		r;
	BButton		*ok;
	long		vsize;
	long		hsize;
	BSlider		*s1;
	char		buf[256];

	
	bnd = Bounds();
	hsize = (int)(bnd.right - bnd.left);
	vsize = (int)(bnd.bottom - bnd.top);
	
	
	master = new BView(bnd, "master", 0, B_WILL_DRAW);
	master->SetViewColor(back_color_light);

	sprintf(buf, "%s Settings", title);
	BStringView *s = new BStringView(BRect(5,5, 200, 20), "", buf);
	s->SetFontSize(14);
	master->AddChild(s);
	
	AddChild(master);



	master->AddChild(new BButton(BRect(hsize * 0.15,vsize - 45, hsize * 0.4, vsize - 25),
			 		"Cancel",
			 		"Cancel",
			 		new BMessage(32000)));

	master->AddChild(ok = 
					 new BButton(BRect(hsize * 0.6,vsize - 45, hsize * 0.85, vsize - 25),
			 		title,
			 		title,
			 		new BMessage(32001)));
	
	SetDefaultButton(ok);
	
	BRect	r1;

	r1.left = hsize * 0.2;
	r1.right = hsize * 0.8;
	r1.top = 40;
	r1.bottom = 80;

	s1 = new StrBSlider(r1, "Strength", "Strength", new BMessage(STRENGTH), 0, 100.0, B_TRIANGLE_THUMB);
	s1->SetValue(f1);
	s1->SetLimitLabels("0.0%","100.0%");
	rgb_color c;
	c.green = c.red = 102; c.blue = 152;
	s1->UseFillColor(true, &c);
	master->AddChild(s1);

	Show();
}


//------------------------------------------------------------

	AskStrength::~AskStrength()
{
	done = 1;
}

//------------------------------------------------------------

void	AskStrength::new_value()
{
	f1 = ((BSlider *)(FindView("Strength")))->Value();
}

//------------------------------------------------------------

void	AskStrength::MessageReceived(BMessage *msg)
{
	if (msg->what == 32000) {
		f1 = -1;
		f2 = -1;
		Close();
	}
	if (msg->what == 32001) {
		Close();
	}
	if (msg->what == STRENGTH) {
		new_value();
	}
}

//------------------------------------------------------------
	
	AskVolume::AskVolume(char *title)
	: BWindow( norm(330, 180),
 			   title,
			    B_MODAL_WINDOW,
			    B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	BRect		bnd;
	BRect		r;
	BButton		*ok;
	long		vsize;
	long		hsize;
	BSlider		*s1;
	char		buf[256];

	
	bnd = Bounds();
	hsize = (int)(bnd.right - bnd.left);
	vsize = (int)(bnd.bottom - bnd.top);
	
	
	master = new BView(bnd, "master", 0, B_WILL_DRAW);
	master->SetViewColor(back_color_light);

	sprintf(buf, "%s Settings", title);
	BStringView *s = new BStringView(BRect(5,5, 200, 20), "", buf);
	s->SetFontSize(14);
	master->AddChild(s);
	
	AddChild(master);



	master->AddChild(new BButton(BRect(hsize * 0.15,vsize - 45, hsize * 0.4, vsize - 25),
			 		"Cancel",
			 		"Cancel",
			 		new BMessage(32000)));

	master->AddChild(ok = 
					 new BButton(BRect(hsize * 0.6,vsize - 45, hsize * 0.85, vsize - 25),
			 		title,
			 		title,
			 		new BMessage(32001)));
	
	SetDefaultButton(ok);
	
	BRect	r1;

	r1.left = hsize * 0.2;
	r1.right = hsize * 0.8;
	r1.top = 40;
	r1.bottom = 80;

	s1 = new StrBSlider(r1, "Volume Change", "Volume Change", new BMessage(STRENGTH), 0, 200.0, B_TRIANGLE_THUMB);
	s1->SetValue(f1);
	s1->SetLimitLabels("0.0%","200.0%");
	rgb_color c;
	c.green = c.red = 102; c.blue = 152;
	s1->UseFillColor(true, &c);
	master->AddChild(s1);

	Show();
}


//------------------------------------------------------------

	AskVolume::~AskVolume()
{
	done = 1;
}

//------------------------------------------------------------

void	AskVolume::new_value()
{
	f1 = ((BSlider *)(FindView("Volume Change")))->Value();
}

//------------------------------------------------------------

void	AskVolume::MessageReceived(BMessage *msg)
{
	if (msg->what == 32000) {
		f1 = -1;
		f2 = -1;
		Close();
	}
	if (msg->what == 32001) {
		Close();
	}
	if (msg->what == STRENGTH) {
		new_value();
	}
}

//------------------------------------------------------------

long	do_ask_filter1(BWindow *dad, char *title, float *vf1, float *vf2)
{
	AskFilter	*b;

	
	f1 = *vf1;
	f2 = *vf2;

	//printf("app lock = %d\n", be_app->IsLocked());
	//printf("ask filter\n");
	done = 0;
	b = new AskFilter(title);
	//printf("const done\n");
	while(done == 0) {
		dad->Lock();
		dad->UpdateIfNeeded();
		dad->Unlock();
		snooze(42000);
	}

	*vf1 = f1;
	*vf2 = f2;

	return 0;
}

//------------------------------------------------------------

long	do_ask_strength(BWindow *dad, char *title, float *vf1)
{
	AskStrength	*b;

	
	f1 = *vf1;

	done = 0;
	b = new AskStrength(title);
	while(done == 0) {
		dad->Lock();
		dad->UpdateIfNeeded();
		dad->Unlock();
		snooze(42000);
	}

	*vf1 = f1;

	return 0;
}

//------------------------------------------------------------

long	do_ask_volume(BWindow *dad, char *title, float *vf1)
{
	AskVolume	*b;

	
	f1 = *vf1;

	done = 0;
	b = new AskVolume(title);
	while(done == 0) {
		dad->Lock();
		dad->UpdateIfNeeded();
		dad->Unlock();
		snooze(42000);
	}

	*vf1 = f1;

	return 0;
}

//------------------------------------------------------------
