/*
	
	Stars.h
	
	by Pierre Raynaud-Richard.
	
	Copyright 1998 Be Incorporated, All Rights Reserved.

*/

#ifndef STARS_H
#define STARS_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef STAR_WINDOW_H
#include "StarWindow.h"
#endif

/* not too much to be said... */
class StarsApp : public BApplication {
public:
					StarsApp();
					
	bool			abort_required;
private:
	StarWindow		*aWindow;
};

#endif
