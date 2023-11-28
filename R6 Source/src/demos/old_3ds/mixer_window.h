#ifndef _WINDOW_H
#include <Window.h>
#endif
#include "Slider.h"
#include "channel.h"
#include "wave_view.h"
#include "ctrl_view.h"
#include "xCheckBox.h"
#include <MediaKit.h>
#include "ctrl2d_view.h"

class TMixerWindow : public BWindow {

public:
								TMixerWindow(BRect, const char*);
						long	window_control_task();
virtual					bool	QuitRequested( void );
		TMonoSlider				*sliders[16];
		TWaveView				*waves[16];
		TCtrlView				*balance[16];
		TCtrl2dView				*c2d[16];
		Channel					*c[32];
};
