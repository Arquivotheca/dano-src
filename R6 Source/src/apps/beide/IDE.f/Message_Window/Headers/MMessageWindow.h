// ---------------------------------------------------------------------------
/*
	MMessageWindow.h
	
	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.

	heavily modified for separation of Errors&Warnings...
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			10 June 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MMESSAGEWINDOW_H
#define _MMESSAGEWINDOW_H

#include "MMessageView.h"
#include "MPrefsStruct.h"
#include "BeIDEComm.h"
#include <Window.h>

class BList;
class MMessageInfoView;
class BView;
class BCheckBox;
class BPictureButton;
class BStringView;
class MProjectWindow;
class BMenu;

// This is a short version of this struct used
// for certain internal errors
struct ErrorNotificationMessageShort 
{
	CompilerErrorRef		errorRef;
	bool					hasErrorRef;
	bool					isWarning;
	char					errorMessage[256];
};

// ---------------------------------------------------------------------------
// class MMessageWindow
// ---------------------------------------------------------------------------

class MMessageWindow : public BWindow
{
public:
								MMessageWindow(const char* title, 
											   const char* infoTitle);
	virtual						~MMessageWindow();

	virtual	void				MessageReceived(BMessage * message);

	virtual	void				MenusBeginning();
	virtual void				Zoom(BPoint rec_position, float rec_width, float rec_height);

	void						PostErrorMessage(const ErrorNotificationMessage& inError,
												 bool inShowAndActivate = false);

	void						PostErrorMessage(const ErrorNotificationMessageShort& inError,
												 bool inShowAndActivate = false);

	void						SaveRequested(entry_ref* directory, const char* name);

	void						ClearMessages(bool inSync = false);

	virtual bool				QuitRequested();
	virtual	void				WindowActivated(bool inActive);

	void						GetData(BMessage& inOutMessage);
	void						SetData(BMessage& inOutMessage);

	int32						Errors();
	int32						Warnings();
	int32						Infos();
	
	static void					RemovePreferences();
	static MMessageWindow*		GetRecentMessageWindow();
	static void					MessageToAllMessageWindows(BMessage& inMessage);
	static void					MessageToMostRecent(BMessage& inMessage);
	static MMessageWindow*		GetGeneralMessageWindow();
	static int32				MessageWindowCount();
	
protected:
	void						BuildControlGroup(BPoint start,
												  BView* inView,
												  const BBitmap* bitMap,
												  BCheckBox** checkBox,
												  BPictureButton** pictureButton,
												  BStringView** caption);
	
	void						ShowAndActivate();
	
	MMessageView*				fMessageView;

private:
	BMenuBar*					fMenuBar;
	BMenu*						fWindowMenu;
	BMenu*						fFontMenu;
	BMenu*						fSizeMenu;
	BFilePanel*					fSavePanel;

	MessagesType				fVisibleTypes;
	FontPrefs					fFontPrefs;
	BRect						fUserRect;

	bool						fUserState;	
	
	static BList				fgMessageWindowList;
	static BLocker				fgMessageWindowListLock;
	static MMessageWindow*		fgGeneralMessageWindow;
	
	void						BuildWindow(const char* infoTitle);
	
	void						DoCopy();
	void						GetPrefs();
	void						SetPrefs();
	void						DoSetData();
	
	bool						SavePanelIsRunning();
	void						ShowSavePanel();
	
	void						ChangeFont(const font_family family, const font_style style);
	void						ChangeFontSize(float size);


	virtual MMessageInfoView*	CreateInfoView(BRect& bounds, const char* infoTitle) = 0;
	virtual void				BuildInfoView() = 0;
	virtual void				ClearMessages(BMessage& inMessage) = 0;
	virtual MProjectWindow*		GetAssociatedProject() = 0;
};

#endif
