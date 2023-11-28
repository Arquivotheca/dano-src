//--------------------------------------------------------------------
//	
//	Workspace.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Path.h>
#include <Rect.h>
#include <Roster.h>
#include <Window.h>

#define	BROWSER_WIND		 82
#define	TITLE_BAR_HEIGHT	 25

enum	menus				{MENU_GET_COUNT = 1, MENU_SET_COUNT, MENU_QUIT};

class TWSWindow;

//====================================================================

class TWSApp : public BApplication {

private:
bool			arg_only;
TWSWindow		*fWSWindow;
BPopUpMenu		*fMenu;
public:

				TWSApp();
virtual void	AboutRequested();
virtual void	ArgvReceived(int32, char**);
virtual void	MessageReceived(BMessage*);
virtual void	ReadyToRun(void);
};

//--------------------------------------------------------------------

class TWSWindow : public BWindow {

private:
BRect			fBounds;

public:

				TWSWindow(BRect, char*);
virtual	void	MessageReceived(BMessage*);
		bool	HandleMessageDropped(BMessage*, BPoint);
virtual bool	QuitRequested();
virtual void	ScreenChanged(BRect, color_space);
virtual void	WorkspaceActivated(long, bool);
virtual void	Zoom(BPoint, float, float);
};
#endif
