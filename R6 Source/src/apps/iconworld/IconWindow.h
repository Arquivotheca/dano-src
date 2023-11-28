//--------------------------------------------------------------------
//	
//	IconWindow.h
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef ICON_WINDOW_H
#define ICON_WINDOW_H

#ifndef _FILE_PANEL_H
#include <FilePanel.h>
#endif
#ifndef _MENU_BAR_H
#include <MenuBar.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif

enum WINDOW_MESSAGES	{M_SAVE = 256, M_SAVE_AS, M_CLOSE, M_INFO,

						 M_UNDO, M_CUT, M_COPY, M_PASTE, M_CLEAR, M_SELECT,

						 M_CREATE, M_DELETE,

						 M_PEN,

						 M_MODE_COPY, M_MODE_OVER, M_MODE_ERASE, M_MODE_INVERT,
						 M_MODE_ADD, M_MODE_SUBTRACT, M_MODE_BLEND, M_MODE_MIN,
						 M_MODE_MAX,
#ifdef APPI_EDIT
						 M_CREATE_APPI, 
#endif						 
						 M_SINGLE_LAUNCH, M_MULTI_LAUNCH,
						 M_EXCLUSIVE_LAUNCH, M_BACKGROUND_APP, M_ARGV_ONLY,
						 M_SIGNATURE};

class TIconView;
class TColorView;
class TPatView;
class TToolView;


//====================================================================

class TIconWindow : public BWindow {

public:
BMenu			*fFileMenu;
BMenu			*fEditMenu;
BMenu			*fIconMenu;
BMenu			*fSizeMenu;
BMenu			*fModeMenu;
#ifdef APPI_EDIT
BMenu			*fAPPIMenu;
#endif
TIconView		*fView;

				TIconWindow(BRect, entry_ref*);
virtual void	MenusBeginning(void);
virtual void	MessageReceived(BMessage*);
virtual	bool 	QuitRequested(void);
virtual void	Show(void);
virtual void	WindowActivated(bool);
};


//====================================================================

class TColorWindow : public BWindow {

public:
TColorView		*fView;

				TColorWindow(BRect, char*);
};


//====================================================================

class TToolWindow : public BWindow {

public:
TToolView		*fView;

				TToolWindow(BRect, char*);
};


//====================================================================

class TPatWindow : public BWindow {

public:
TPatView		*fView;

				TPatWindow(BRect, char*);
};
#endif
