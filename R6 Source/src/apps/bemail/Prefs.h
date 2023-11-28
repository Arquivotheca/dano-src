//--------------------------------------------------------------------
//	
//	Prefs.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef PREFS_H
#define PREFS_H

#include <Font.h>
#include <Window.h>

#define	PREF_WIDTH			300
#define PREF_HEIGHT			250

#define SIG_NONE			"None"
#define SIG_RANDOM			"Random"

class Button;

//====================================================================

class TPrefsWindow : public BWindow {
public:

					TPrefsWindow(BRect, BFont*, int32*, bool*, char**, 
					  			uint32 *encoding, bool *buttonBar);
					~TPrefsWindow();
	virtual void	MessageReceived(BMessage*);
	BPopUpMenu*		BuildFontMenu(BFont*);
	BPopUpMenu*		BuildLevelMenu(int32);
	BPopUpMenu*		BuildSignatureMenu(char*);
	BPopUpMenu*		BuildSizeMenu(BFont*);
	BPopUpMenu*		BuildWrapMenu(bool);
	BPopUpMenu*		BuildEncodingMenu(uint32 encoding);
	BPopUpMenu*		BuildButtonBarMenu(bool show);

private:

	bool			*fNewWrap;
	bool			*fNewButtonBar;
	char			*fSignature;
	char			**fNewSignature;
	int32			fLevel;
	int32			*fNewLevel;
	BButton			*fOK;
	BButton			*fCancel;
	BButton			*fRevert;
	BFont			fFont;
	BFont			*fNewFont;
	uint32			fEncoding;
	uint32			*fNewEncoding;
	BPopUpMenu		*fFontMenu;
	BPopUpMenu		*fSizeMenu;
	BPopUpMenu		*fLevelMenu;
	BPopUpMenu		*fWrapMenu;
	BPopUpMenu		*fSignatureMenu;
	BPopUpMenu		*fEncodingMenu;
	BPopUpMenu		*fButtonBarMenu;
	bool			fWrap;
	bool			fButtonBar;
};

#endif
