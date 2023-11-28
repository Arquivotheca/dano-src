//*****************************************************************************
//
//	File:		 SetupView.h
//
//	Description: Setup view header for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#if ! defined SETUPVIEW_INCLUDED
#define SETUPVIEW_INCLUDED

#include "Settings.h"
#include "DirPanel.h"
#include "ImagePanel.h"
#include <View.h>
#include <Box.h>
#include <Path.h>
#include <List.h>
#include <Message.h>

#define WX	400
#define WY	200

class BTranslationRoster;
class PreviewView;
class BMenuItem;
class BCheckBox;
class BMenuField;
class BTextControl;

class SetupView : public BBox
{
	BMessage	prefs;
	Settings	settings;
	ImagePanel	*imgpanel;
	ImageFilter	imgfilter;
	DirPanel	*dirpanel;
	DirFilter	dirfilter;
	BMenuItem	*lastimgpanelitem;
	BMenuItem	*lastdirpanelitem;
	BList		recentimages;
	BList		recentfolders;
	BTranslatorRoster *roster;
	entry_ref	lastimgpaneldir;
	entry_ref	lastdirpaneldir;

	// view pointers
	PreviewView		*pv;
	BCheckBox		*erase;
	BMenuField		*applyto;
	BTextControl	*x;
	BTextControl	*y;
	BMenuField		*placement;
	BButton			*apply;
	BButton			*revert;
	BButton			*dflt;
	BMenuField		*image;


public:
			SetupView();
			~SetupView();
	void	AttachedToWindow();
	void	AllAttached();
	void	DetachedFromWindow();
	void	MessageReceived(BMessage *msg);
	void	BuildUI();
	void	LoadSettings();
	void	LoadImages(BMessage *msg);
	void	OpenDir(BMessage *msg);
	void	LoadDir(BMessage *msg);
	void	ShowSettings();
	void	BuildImageMenu();
	void	CheckEnabled();
	void	EnablePlacementItems(bool enable);
	void	CheckChanges();
	void	ChangePosition(BPoint p);
	void	ImageSelection();
	void	ApplyToSelection();
	void	ShowXY();
	void	BuildFolderMenu();
	void	NoImage();
	void	AllWorkspaces();
	void	CurrentWorkspace();
	void	DefaultFolder();
};

#endif
