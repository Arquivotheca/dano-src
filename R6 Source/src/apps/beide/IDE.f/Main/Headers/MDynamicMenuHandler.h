// ---------------------------------------------------------------------------
/*
	MDynamicMenuHandler.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			5 January 1999

	Taken from...
	MWindowsMenu.h
	(Copyright 1995 Metrowerks Corporation, All Rights Reserved.)
	
	Static class for handling window and project lists.
	One instance exists as a member of the app so its constructor and
	destructor will be called automatically.
*/
// ---------------------------------------------------------------------------

#ifndef _MDYNAMICMENUHANDLER_H
#define _MDYNAMICMENUHANDLER_H

#include "IDEConstants.h"
#include "MList.h"
#include <StorageKit.h>
#include <MessageFilter.h>

class MTextWindow;
class MProjectWindow;
class MMessageWindow;
class WindowRec;
class BMenuBar;


enum TileType
{
	kMatrixTile,
	kVerticalTile
};

class MDynamicMenuHandler
{

public:
									MDynamicMenuHandler();
									~MDynamicMenuHandler();
									
	static void						TextWindowOpened(MTextWindow* inWindow);
	static void						TextWindowClosed(MTextWindow* inWindow);

	static void						MessageWindowOpened(BWindow* inWindow);
	static void						MessageWindowClosed(BWindow* inWindow);
	
	static void						WindowActivated(BWindow* inWindow);

	static void						ProjectOpened(MProjectWindow* inProject);
	static void						ProjectClosed(MProjectWindow* inProject);

	static void						MessageToAllTextWindows(BMessage& inMessage);
	static bool						CloseAllTextWindows();
	static bool						SaveAllTextWindows();

	static void						UpdateDynamicMenus(BMenuBar& inMenuBar, MProjectWindow* project);
	static void						AddHierarchalMenuItems(BMenuBar& inMenuBar);
	
	static void						MenuClicked(BMessage& inMessage);

	static MTextWindow*				GetFrontTextWindow();

	static MTextWindow *			FindTextWindowForFile(BEntry* inFile);
	
	static MTextWindow*				FindWindowByName(const char* inName);

	static MTextWindow*				FindWindow(entry_ref inRef);

	static int32					IndexOf(const BWindow* inWindow);
	static int32					IndexOf(int32 inShortcut);

	static void						Stack();
	static void						Tile(TileType inTileType);

	static void						GetRecentDocumentList(BMessage& outMessage);
	static void						PutRecentDocumentList(const BMessage& inMessage);
	
	static void						HandleNodeMonitorChange(const BMessage& message);

	enum EWindowKind				{ kText, kMessage, kProject };
	
private:
		
	static BLocker					fLocker;
	static MList<WindowRec*>		fWindowList;
	static MList<entry_ref*>		fRecentProjectList;
	static MList<entry_ref*>		fRecentWindowList;
	static bool						fShortcuts[10];

	static void						Lock();
	static void						Unlock();
	
	static void						DoWindowOpened(BWindow* inWindow, EWindowKind which);
	static void						DoWindowClosed(BWindow* inWindow);

	static WindowRec*				GetItem(BWindow* inWindow);
	static int32					GetNextShortcut();

	static entry_ref*				FindRecent(const MList<entry_ref*>& list, 
											   const entry_ref& alias);

	static void						AddRecent(MList<entry_ref*>& list, 
											  entry_ref* inRef, 
											  BEntry* entry = NULL);

	static int32					FindDeletedNode(const MList<entry_ref*>& recentList, 
												   const node_ref& node);

	static int32					TextWindowsInThisWorkSpace(BWindow*& outWindow);
};

inline void
MDynamicMenuHandler::Lock()
{
	fLocker.Lock();
}

inline void
MDynamicMenuHandler::Unlock()
{
	fLocker.Unlock();
}


class MWindowMenuFilter : public BMessageFilter
{
public:
								MWindowMenuFilter();

virtual	filter_result			Filter(BMessage* message, BHandler** target);
};

#endif
