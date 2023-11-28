/*
	
	cl_wind.h
	
	Copyright 1994 Be, Inc. All Rights Reserved.
	
*/


#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef	_CL_VIEW_H_
#include "cl_view.h"
#endif

class TClockWindow : public BWindow {

public:
				TClockWindow(BRect, const char*);
virtual	bool	QuitRequested( void );

TOnscreenView	*theOnscreenView;
};
