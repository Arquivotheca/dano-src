//*****************************************************************************
//
//	File:		 SetupWin.h
//
//	Description: Setup window header for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#if ! defined SETUPWIN_INCLUDED
#define SETUPWIN_INCLUDED

#include "Settings.h"
#include "MockupView.h"
#include "DirPanel.h"
#include "ImagePanel.h"
#include <Window.h>
#include <Path.h>
#include <List.h>
#include <Message.h>

#define WX	470
#define WY	225

class BTranslationRoster;
class BTranslatorRoster;

class SetupWin : public BWindow
{
	BMessage	prefs;
	Settings	settings;
	ImagePanel	*imgpanel;
	ImageFilter	imgfilter;
	DirPanel	*dirpanel;
	DirFilter	dirfilter;
	BMenuItem	*lastimgpanel;
	BMenuItem	*lastdirpanel;
	BList		recentimages;
	BList		recentfolders;
	BTranslatorRoster *roster;

public:
			SetupWin();
			~SetupWin();
	void	WorkspaceActivated(int32 workspace, bool active);
	void	ScreenChanged(BRect, color_space);
	bool	QuitRequested();
	void	MessageReceived(BMessage *msg);

	void	ShowSettings();
	void	BuildImageMenu();
	void	CheckEnabled();
	void	EnablePlacementItems(bool enable);
	void	CheckChanges();
	void	ChangeColor(rgb_color col);
	void	ChangePosition(BPoint p);
	void	ViewAs(MockupView::mockup_t mode);
	void	ImageSelection();
	void	ApplyToSelection();
	void	ShowXY();
	void	BuildFolderMenu();
};

#endif
