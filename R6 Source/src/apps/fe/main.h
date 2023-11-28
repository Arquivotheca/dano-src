#ifndef FE_H
#define FE_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#include <ScrollView.h>
#include "twindow.h"
#include "bm_view.h"
#include "picker_view.h"
#include "select_view.h"
#include "sample_view.h"

class TApplication : public BApplication {
public:
	TWindow			*aWindow;
	PickView		*pview;
	BMView			*bm_view;
	BScrollView     *scr_view;
	SelectView		*s_view;
	SampleView		*samp_view;

	TApplication();
};

#endif


