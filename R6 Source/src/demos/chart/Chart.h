/*
	
	Chart.h
	
	by Pierre Raynaud-Richard.
	
	Copyright 1998 Be Incorporated, All Rights Reserved.

*/

#ifndef CHART_H
#define CHART_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef CHART_WINDOW_H
#include "ChartWindow.h"
#endif

/* not too much to be said... */
class ChartApp : public BApplication {
public:
					ChartApp();
private:
	ChartWindow		*aWindow;
};

#endif
