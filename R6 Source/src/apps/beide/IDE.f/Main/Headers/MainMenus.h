//========================================================================
//	MainMenus.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Utilities and data for setting up the main window menubar
//	Jon Watte

#ifndef _MAINMENUS_H
#define _MAINMENUS_H

#include "IDEConstants.h"
#include <InterfaceKit.h>

class BMenuBar;
class MPathMenu;


	BMenuBar *					MakeProjectMenus(
									BRect area);
	BMenuBar *					MakeTextMenus(
									BRect area);
	BMenuBar *					MakeMessageWindowMenus(
									BRect area);
	MPathMenu*					MakeProjectContextMenu(
									entry_ref& 		inRef,
									BPopUpMenu&		inOutMenu,
									BWindow*		inWindow);


	void						UpdateTextMenus(
									BMenuBar * bar);
	void						UpdateProjectMenus(
									BMenuBar * bar);
	void						UpdateMessageWindowMenus(
									BMenuBar * bar);
	void						UpdateMenuItemsForModifierKeys(
									BMenuBar*	inMenuBar);

	void						CommonWindowSetupSave(
									BWindow& 	inWindow);
	void						CommonWindowSetupBuild(
									BWindow& 	inWindow);
	void						CommonWindowSetupExecute(
									BWindow& 	inWindow);
	void						CommonWindowSetupFind(
									BWindow& 	inWindow);
	void						CommonWindowSetupWindow(
									BWindow& inWindow);
	void						CommonWindowSetupFile(
									BWindow& inWindow);

	bool						SpecialMessageReceived(
									BMessage& 	inMessage,
									BWindow*	inWindow);

// ---------------------------------------------------------------------------
// Constants based on menu positions or names
// (defined next to menu in MainMenus.cpp)
// ---------------------------------------------------------------------------

extern const int32 kEnableDebuggerID;
extern const int32 kRunID;
extern const int32 kOpenRecentMenuItemIndex;
extern const int32 kWinMenuMinItemCount;
extern const CommandT kRecentProjectMenuKey;
extern const char* kFileMenuName;
extern const char* kEditMenuName;
extern const char* kSearchMenuName;
extern const char* kProjectMenuName;
extern const char* kWindowMenuName;

extern const int32 kProjectMenuIndex;

#endif
