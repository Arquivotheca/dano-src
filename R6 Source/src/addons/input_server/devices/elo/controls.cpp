// controls.cpp

#include <Window.h>
#include <Screen.h>
#include <Debug.h>
#include "elo.h"
#include "controls.h"

struct control controls[4] = {
//	{ VOLUME_CONTROL,      "Volume",     {255,75,50,0}, 3.0, 0.0 },
	{ TOGGLE_SOFTKB,       "",           {0,0,0,0},     0.0, 0.0 },
	// ACHTUNG!  You can't currently pass arguments to an EXECUTE_COMMAND control
	{ EXECUTE_COMMAND,     "/boot/preferences/calibrate2", {0,0,0,0}, 0.0, 0.0 },
	{ BRIGHTNESS_CONTROL,  "Brightness", {75,255,50,0}, 3.0, 0.0 },
	{ CONTRAST_CONTROL,    "Contrast",   {50,75,255,0}, 3.0, 0.0 }
};

static BRect
window_size()
{
	BScreen s;
	BRect r = s.Frame();
	r.top = r.bottom - 32;	

	return r;
}

ControlWindow::ControlWindow(DT300InputDevice *device, control *which)
			: BWindow(window_size(), "DT300 Control",
					B_BORDERED_WINDOW_LOOK,	B_FLOATING_ALL_WINDOW_FEEL, 0),
			  mIdleTimer(-1),
			  mLast(0),
			  mControl(which),
			  mDevice(device)
{
	BRect r = Bounds();
	mControlView = new ControlView(r, mControl->name);
	mControlView->SetMaxValue(100.0);
	mControlView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	mControlView->SetBarColor(mControl->color);
	AddChild(mControlView);
	mControlView->Update(mControl->current);

	Touch();
	mIdleTimer = spawn_thread(_idle_timer_, "Idle Timer",
								B_NORMAL_PRIORITY, this);
	resume_thread(mIdleTimer);
}

ControlWindow::~ControlWindow()
{
	kill_thread(mIdleTimer);
	wait_for_thread(mIdleTimer, &mIdleTimer);

	mDevice->ControlWindowVanished();
}

void
ControlWindow::Switch(control *which)
{
	if (mControl == which)
		PostMessage(B_QUIT_REQUESTED);

	if (Lock()) {
		mControl = which;
		mControlView->Reset(mControl->name);
		mControlView->SetBarColor(mControl->color);
		mControlView->Update(mControl->current);
		Unlock();
	}
}

void 
ControlWindow::Increase()
{
	if (!Lock())
		return;
		
	Touch();
	float new_value = min_c(mControl->current + mControl->increment, 100.0);
	mControlView->Update(new_value - mControl->current);
	mControl->current = new_value;
	mDevice->ControlChanged(mControl);
	
	Unlock();
}

void 
ControlWindow::Decrease()
{
	if (!Lock())
		return;
		
	Touch();
	float new_value = max_c(mControl->current - mControl->increment, 0.0);
	mControlView->Update(new_value - mControl->current);
	mControl->current = new_value;
	mDevice->ControlChanged(mControl);
	
	Unlock();
}

void
ControlWindow::Touch()
{
	mLast = system_time();
}

int32
ControlWindow::_idle_timer_(void *data)
{
	ControlWindow *c = (ControlWindow *)data;
	return c->IdleTimer();
}

int32
ControlWindow::IdleTimer()
{
	for (;;) {
		snooze(100000);
		if ((system_time() - mLast) > IDLE_TIMEOUT) {
			PostMessage(B_QUIT_REQUESTED);
			return 0;
		}
	}
	return 0;
}
