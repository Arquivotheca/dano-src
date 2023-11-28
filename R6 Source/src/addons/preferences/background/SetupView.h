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

#include "PPAddOn.h"
#include "Settings.h"
#include "ImagePanel.h"
#include <View.h>
#include <Path.h>
#include <List.h>
#include <Message.h>

#define WX	400
#define WY	330

class BTranslationRoster;
class PreviewView;
//class PositionWindow;

class SetupView : public BView
{
	BMessage	prefs;
	Settings	settings;
	ImagePanel	*imgpanel;
	ImageFilter	imgfilter;
	BMenuItem	*lastimgpanelitem;
	BList		recentimages;
	BTranslatorRoster *roster;
	entry_ref	lastimgpaneldir;
	BBitmap			*currentbitmap;

	// view pointers
	PreviewView		*pv;
	BCheckBox		*erase;
	BMenuField		*applyto;
	BMenuField		*placement;
	BTextControl	*x;
	BTextControl	*y;
	BColorControl	*bcc;
	BButton			*dflt;
	BButton			*apply;
	BButton			*revert;
	BMenuField		*image;
	PPAddOn			*addon;

public:
			SetupView(PPAddOn *adn);
			~SetupView();
	void	WorkspaceActivated(int32 workspace, bool active);
	void	ScreenChanged(BRect, color_space);
	void	AttachedToWindow();
	void	DetachedFromWindow();
	void	MessageReceived(BMessage *msg);
	void	BuildUI();
	void	LoadSettings();
	void	LoadImages(BMessage *msg);
	void	ShowSettings();
	void	BuildImageMenu();
	void	CheckEnabled();
	void	EnablePlacementItems(bool enable);
	void	CheckChanges();
	void	ChangeColor(rgb_color col);
	void	ChangePosition(BPoint p);
	void	ImageSelection();
	void	ShowXY();
	void	NoImage();
	void	AllWorkspaces();
	void	CurrentWorkspace();
};

#endif
