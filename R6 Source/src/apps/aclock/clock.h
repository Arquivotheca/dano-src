/*
	
	clock.h
	
	Copyright 1994 Be, Inc. All Rights Reserved.

*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include "cl_view.h"
#include "cl_wind.h"

#define SHOW_SECONDS	'ssec'

extern const char *app_signature;

class THelloApplication : public BApplication {

public:
						THelloApplication();
virtual	void			MessageReceived(BMessage *msg);

private:
		TClockWindow*	myWindow;
		TOnscreenView	*myView;
};
