// controls.h

#ifndef CONTROLS_H
#define CONTROLS_H

#define CONTRAST_CONTROL	0
#define BRIGHTNESS_CONTROL	1
#define VOLUME_CONTROL		2
#define EXECUTE_COMMAND		3
#define TOGGLE_SOFTKB		4

#include <StatusBar.h>
#include <Window.h>

struct control {
	int			type;
	const char	*name;
	rgb_color	color;
	float		increment;
	float		current;
};

extern struct control controls[4];

class ControlView : public BStatusBar
{
public:
	ControlView(BRect r, const char *label)
		: BStatusBar(r, "", label) { }
};


#define IDLE_TIMEOUT		5000000

class DT300InputDevice;

class ControlWindow : public BWindow
{
public:
	ControlWindow(DT300InputDevice *device, control *which);
	~ControlWindow();
		
	void Switch(control *which);
	void Increase();
	void Decrease();
	
private:
	void Touch();
	
	static int32		_idle_timer_(void *data);
	int32				IdleTimer();
	thread_id			mIdleTimer;
	bigtime_t			mLast;
	control				*mControl;
	ControlView			*mControlView;
	DT300InputDevice	*mDevice;
};

#endif // CONTROLS_H
