//--------------------------------------------------------------------
//	
//	IconWorld.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_WORLD_H
#define ICON_WORLD_H

#ifndef ICON_WINDOW_H
#include "IconWindow.h"
#endif
#ifndef ICON_VIEW_H
#include "IconView.h"
#endif
#ifndef ICON_COLOR_H
#include "IconColor.h"
#endif
#ifndef ICON_TOOLS_H
#include "IconTools.h"
#endif
#ifndef ICON_PAT_H
#include "IconPat.h"
#endif

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef	_FILE_PANEL_H
#include <FilePanel.h>
#endif

#include <PopUpMenu.h>

#define	TITLEBARHEIGHT	25

enum	APP_MESSAGES	{M_NEW = 128,
						 M_OPEN,
						 M_CLOSE_WINDOW};


//====================================================================

class TIconApp : public BApplication {

public:
bool				fClipFlag;
bool				fRefs;
short				fWindowCount;
BBitmap*			fClipIcon;
BPopUpMenu			*fMenu;
BRect				fClipRect;
TColorWindow		*fColorWind;
TToolWindow			*fToolWind;
TPatWindow			*fPatWind;

					TIconApp(void);
					~TIconApp(void);
virtual void		AboutRequested(void);
virtual void		ArgvReceived(long argc, char** argv);
virtual void		MessageReceived(BMessage*);
virtual bool		QuitRequested(void);
virtual void		RefsReceived(BMessage *theMessage);
virtual void		ReadyToRun(void);
        void            OpenIconWindow(entry_ref*);
};

#endif














