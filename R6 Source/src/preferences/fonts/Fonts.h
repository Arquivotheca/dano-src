//--------------------------------------------------------------------
//	
//	Fonts.h
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef FONTS_H
#define FONTS_H

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _LIST_VIEW_H
#include <ListView.h>
#endif
#ifndef _SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#ifndef _RADIO_BUTTON_H
#include <RadioButton.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _FILE_H
#include <File.h>
#endif
#ifndef _DIRECTORY_H
#include <Directory.h>
#endif

#define SETTINGS_FOLDER     "/boot/system/settings"
#define TT_FONTS_FOLDER     "/boot/system/fonts/ttfonts"
#define FONTS_DATA_PATH     "/boot/system/settings/Fonts_data"
#define FONTS_SETTINGS_PATH "/boot/system/settings/Fonts_settings"

#define	BROWSER_WIND		 82
#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			225
#define WIND_HEIGHT			350

enum	buttons		{CANCEL = 1, DONE, LIST_SELECT};

class	TFontsWindow;
class	TFontsView;
class	TListView;

#define ReadLong(x) (long)(*(x))
#define ReadULong(x) (ulong)(*(x))
#define ReadShort(x) (short)(*(x))
#define ReadUShort(x) (ushort)(*(x))
#define ReadChar(x) (char)(*(x))
#define ReadUChar(x) (uchar)(*(x))

enum  { REAL_FONT_NAME_MAX_SIZE = 127,
        FNT_ENABLE = 1,
		FNT_DISABLE = 2 };

typedef struct {
	char       file_name[B_FILE_NAME_LENGTH+1];
	char       full_name[2];
	char       real_name[REAL_FONT_NAME_MAX_SIZE+1];
	char       family_name[REAL_FONT_NAME_MAX_SIZE+1];
	char       style_name[REAL_FONT_NAME_MAX_SIZE+1];
	char       state;
} fnt_prefs;

typedef struct {
	long       count;
	long       used;
	fnt_prefs  *prefs;
} fnt_pref_header;

//====================================================================

class TFontsApp : public BApplication {

private:
TFontsWindow*	fWindow;

public:
fnt_pref_header Hpref;

				TFontsApp();
virtual void	AboutRequested();
        void    ExpandPrefs();
        void    AddPrefs(char *file_name, char *real_name,
						 char *family_name, char *style_name,
						 char state);
        void    ReadPrefs(int ref);
        void    WritePrefs(int ref);
        void    UpdatePrefs();
        void    SortPrefs();
        bool    GetTrueTypeName(BEntry *, char *name, char *family, char *style);
};

//--------------------------------------------------------------------

class TFontsWindow : public BWindow {
public:
TFontsView*		fView;

				TFontsWindow(BRect, char*);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested();
};

//--------------------------------------------------------------------

class TFontsView : public BView {

public:
        TListView *fList;
		
				TFontsView(BRect, char*); 
virtual void    AttachedToWindow();
virtual void    Draw(BRect);
		void    RefreshList();
};

//--------------------------------------------------------------------

class TListView : public BListView {
public:
				TListView(BRect);
};

#endif

























