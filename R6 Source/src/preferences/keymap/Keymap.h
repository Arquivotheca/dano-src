//--------------------------------------------------------------------
//	
//	Keymap.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef KEYMAP_H
#define KEYMAP_H
#ifndef	KEYMAP_WINDOW_H 
#include "KeymapWindow.h"
#endif
#ifndef KEYMAP_VIEW_H
#include "KeymapView.h"
#endif

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _ROSTER_H
#include <Roster.h>
#endif

#define	BROWSERWIND		82
#define	TITLEBARHEIGHT	25


#define KEY_OPEN		100
#define	KEY_FRONT		101

//====================================================================

class TKeymapApplication : public BApplication {

public:
				TKeymapApplication();
virtual void	AboutRequested();
virtual void	ArgvReceived(int32 argc, char** argv);
virtual void	RefsReceived(BMessage *theMessage);

protected:
  TKeymapWindow*   fWindow;
};

#endif








