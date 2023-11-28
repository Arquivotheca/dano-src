#ifndef FE_WINDOW_H
#define FE_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif
#include <Window.h>
#include <ListView.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Menu.h>

#include "font_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

//------------------------------------------------

#define	SAVE	1001
#define	UNDO	1002
#define	EDIT	1003
#define REVERSE 1004
#define APPLY   1005
#define COPY    1006
#define CUT     1007
#define PASTE   1008
#define INVERTH 1009
#define INVERTV 1010

/* prototypes for private font cache control (cf <shared_fonts.cpp>) */
_IMPORT void _font_control_(BFont *font, int32 cmd, void *data);

enum {
	FC_CMD_CREATE_TUNED_FONT = 0x0000,
	FC_FLUSH_FONT_SET        = 0x0001,
	FC_CLOSE_ALL_FONT_FILES  = 0x0002,
    FC_RESCAN_ALL_FILES      = 0x0003
};


//------------------------------------------------

class FontSelectionMenu;

class TWindow : public BWindow {
public:
	    FontSelectionMenu *SelectFont;
		FontMachine	*fm;
		FILE        *fp;
		BView		*background;
		char        pathname[PATH_MAX];
							 
		TWindow(BRect frame); 
virtual	bool		QuitRequested();
virtual	void		MessageReceived(BMessage *an_event);
		void        Edit();
		void        Save();
		void        Apply();
};

class FontMenu : public BPopUpMenu {
public:
	FontMenu(const BFont *, const BHandler *target);

	virtual void ShowCurrent(const BFont *);
		// set current font name and force redraw
	
private:

	void SetCurrent(const BFont *);

	// utility calls for font name composing
	static void FontName(char *, int32 bufferSize, font_family, font_style, 
		const char * divider = " ", bool addRegular = false);
	static void FontName(char *, int32 bufferSize, const BFont *, const char *divider = " ", 
		bool addRegular = false);
	static const BFont *FontFromName(font_family, font_style);
};

class FontPopUpMenu : public BMenuField {
	// a hierarchical menu of fonts grouped in families with submenus
	// for styles
public:
	FontPopUpMenu(BRect, float divider, const BFont *, const char *title, 
		const BHandler *target);

	virtual void ShowCurrent(const BFont *);
		// font has changed, update yourself
};

const float defaultSizes[] = {
	7.0, 8.0, 9.0, 10.0, 11.0, 12.0,
	13.0, 14.0, 15.0, 16.0, 17.0, 18.0
};

class FontSizePopUpMenu : public BMenuField {
public:
	FontSizePopUpMenu(BRect, float divider, const BFont *, 
		const BHandler *target);

	virtual void ShowCurrent(const BFont *);
		// font has changed, update yourself
private:

	void SetCurrent(const BFont *);
};

class SampleText : public BView {
	// used to display the font, selected by a font selector
public:
	SampleText(BRect, const char *, const BFont *);

	virtual void Draw(BRect);
	void FontChanged(const BFont *);
};

class FontSelectionMenu : public BHandler {
	
public:

	FontSelectionMenu(BRect, const char *, BWindow*);
		// passing BView here so that we can target the handler before we 
		// display it to work around a problem with not being able to 
		// target handlers without a looper

virtual ~FontSelectionMenu();

	void SetCurrentFont(const BFont *);
	void AddAsChild(BView *);
	void SetMyFont(const BFont *);
		// set the font and call FontSelected

	BFont font;
		// the current state of the font we are editing

private:

	virtual void MessageReceived(BMessage *);
		// respond to messages from FontSelectors

	FontPopUpMenu *fontFamilyAndStyleSelector;
	FontSizePopUpMenu *fontSizeSelector;
	SampleText *sample;
};

#endif







