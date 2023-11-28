
#include <Window.h>
#include <Bitmap.h>
#include <View.h>
#include <Locker.h>
#include <Autolock.h>
#include <Screen.h>
#include <stdio.h>

#include "miniplay.h"


#define TIMEOUT_TIME 5000000LL
#define POLL_TIME 50000LL
#define STEP 5

class VolumeView;
class VolumeWindow;
void show_volume_window();

static BLocker g_lock("MakeVolumeWindow");
static VolumeWindow * g_volumeWindow;
static float g_targetVisible;
static float g_targetInvisible;

class VolumeWindow : public BWindow {
public:
		VolumeWindow(BRect area);
		~VolumeWindow();
		void Idle();
		void ResetTimeout();
private:
		VolumeView * m_view;
		enum {
			kSlidingUp,
			kWaiting,
			kSlidingDown
		} m_state;
		bigtime_t m_timeoutAt;
};

class VolumeView : public BView {
public:
		VolumeView(BRect area);
		~VolumeView();
		void Draw(BRect area);
		void Pulse();
		void AttachedToWindow();
private:
		void GenerateBitmap();
		float ReadVolume();
		float m_volume;
		bigtime_t m_time;
		BBitmap * m_bitmap;
		BView * m_bmView;
		bool m_mute;
};

class Gonner {
public:
	Gonner() { g_volumeWindow = 0; }
	~Gonner() { if (g_volumeWindow->Lock()) g_volumeWindow->Quit(); }
};

static Gonner s_theGonner;

void
show_volume_window()
{
	BAutolock lock(g_lock);
	if (g_volumeWindow->Lock()) {
		g_volumeWindow->ResetTimeout();
		g_volumeWindow->Unlock();
		return;
	}
	BScreen scrn;
	BRect r(scrn.Frame());
	r.top = r.bottom-20;
	float d = (r.right-r.left)/3;
	r.left += d;
	r.right -= d;
	g_targetVisible = r.top-10;
	r.OffsetBy(0, r.Height()+1);
	g_targetInvisible = r.top;
	g_volumeWindow = new VolumeWindow(r);
	g_volumeWindow->Show();
}

VolumeWindow::VolumeWindow(BRect area) :
	BWindow(area, "Volume", B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE | B_AVOID_FOCUS)
{
	m_view = new VolumeView(Bounds());
	AddChild(m_view);
	SetPulseRate(50000LL);
	m_state = kSlidingUp;
	m_timeoutAt = system_time()+TIMEOUT_TIME+1000000LL;
}


VolumeWindow::~VolumeWindow()
{
	if (g_volumeWindow == this) g_volumeWindow = 0;
}


void
VolumeWindow::Idle()
{
	bigtime_t s = system_time();
	switch (m_state) {
	case kSlidingUp:
		{
			BPoint p(Frame().LeftTop());
			if (p.y > g_targetVisible) {
				p.y -= STEP;
			}
			if (p.y <= g_targetVisible) {
				p.y = g_targetVisible;
				m_state = kWaiting;
			}
			MoveTo(p);
		}
		break;
	case kWaiting:
		if (s > m_timeoutAt) {
			m_state = kSlidingDown;
		}
		break;
	case kSlidingDown:
		{
			BPoint p = Frame().LeftTop();
			p.y += STEP;
			MoveTo(p);
			if (p.y >= g_targetInvisible) {
				Quit();
			}
		}
		break;
	}
}


void
VolumeWindow::ResetTimeout()
{
	if (m_state <= kWaiting) {
		m_timeoutAt = system_time()+TIMEOUT_TIME;
	}
	else {
		m_state = kSlidingUp;
		m_timeoutAt = system_time()+TIMEOUT_TIME+1000000LL;
	}
}



VolumeView::VolumeView(BRect area) :
	BView(area, "VolumeBar", B_FOLLOW_NONE, B_WILL_DRAW|B_PULSE_NEEDED)
{
	m_bitmap = new BBitmap(area, B_RGB16, true);
	m_bitmap->Lock();
	m_bmView = new BView(m_bitmap->Bounds(), "", B_FOLLOW_NONE, B_WILL_DRAW);
	m_bitmap->AddChild(m_bmView);
	m_bitmap->Unlock();
	m_time = 0;
	m_volume = 0.0;
	m_mute = true;
	GenerateBitmap();
}


VolumeView::~VolumeView()
{
	delete m_bitmap;
}

void 
VolumeView::Draw(BRect area)
{
	DrawBitmap(m_bitmap, B_ORIGIN);
}

void 
VolumeView::Pulse()
{
	bigtime_t st = system_time();
	if (st > m_time+POLL_TIME) {
		m_time = st;
		float vl = 0.0, vr = 0.0;
		bool mute;
		(void)mini_get_volume(0, &vl, &vr, &mute);
		if ((vl != m_volume) || (m_mute != mute)) {
			m_volume = vl;
			m_mute = mute;
			GenerateBitmap();
			Draw(Bounds());
			Flush();
			((VolumeWindow *)Window())->ResetTimeout();
		}
	}
	((VolumeWindow *)Window())->Idle();
}

void
VolumeView::AttachedToWindow()
{
	(void)Pulse();
}

void 
VolumeView::GenerateBitmap()
{
	rgb_color gray = { 216, 216, 216, 255 };
	rgb_color dark = { 128, 128, 128, 255 };
	rgb_color light = { 255, 255, 255, 255 };
	rgb_color blue = { 255, 255, 255, 255 };

	m_bitmap->Lock();

	//	 background
	m_bmView->SetHighColor(gray);
	BRect r(m_bitmap->Bounds());
	m_bmView->FillRect(r, B_SOLID_HIGH);

	//	window frame
	m_bmView->BeginLineArray(4);
	m_bmView->AddLine(r.LeftTop(), r.RightTop(), light);
	m_bmView->AddLine(r.LeftTop(), r.LeftBottom(), light);
	m_bmView->AddLine(r.LeftBottom(), r.RightBottom(), dark);
	m_bmView->AddLine(r.RightBottom(), r.RightTop(), dark);
	m_bmView->EndLineArray();
	
	//	actual bar
	r.InsetBy(5, 5);
	int top = (int)r.top;
	int mid = (int)((r.top*2+r.bottom)/3);
	int bot = (int)r.bottom;
	m_bmView->BeginLineArray((bot-top)+1);
	rgb_color c;
	r.right = (r.right-r.left)*m_volume+r.left;
	for (int iy=top; iy<=bot; iy++) {
		c = blue;
		c.red -= abs(mid-iy)*80/(mid-top);
		c.green -= abs(mid-iy)*80/(mid-top);
		if (m_mute) {
			uchar rd = c.red;
			c.red = c.blue;
			c.blue = rd;
		}
		m_bmView->AddLine(BPoint(r.left,iy), BPoint(r.right,iy), c);
	}
	m_bmView->EndLineArray();

	//	done
	m_bmView->Sync();
	m_bitmap->Unlock();
}

