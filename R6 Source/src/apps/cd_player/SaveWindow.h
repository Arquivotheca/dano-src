//--------------------------------------------------------------------
//	
//	SaveWindow.h
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------


#ifndef SAVE_WINDOW_H
#define SAVE_WINDOW_H

#include <Alert.h>
#include <Application.h>
#include <CheckBox.h>
#include <Bitmap.h>
#include <Button.h>
#include <File.h>
#include <FilePanel.h>
#include <Directory.h>
#include <List.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <OS.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <String.h>
#include <View.h>
#include <Volume.h>
#include <Window.h>
#include <scsi.h>
#include <MediaFile.h>

#define SAVE_WIDTH			400
#define SAVE_HEIGHT			160

#define START_MENU_H		  7
#define START_MENU_V		 12
#define END_MENU_H			 23
#define END_MENU_V			 47

#define PLAY_BUTTON_H		 10
#define PLAY_BUTTON_V		126
#define STOP_BUTTON_H		 94
#define STOP_BUTTON_V		126
#define SAVE_BUTTON_H		315
#define SAVE_BUTTON_V		126
#define SAVE_BUTTON_WIDTH	 75
#define SAVE_BUTTON_HEIGHT	 23
#define TRACK_MENU_H		 10
#define TRACK_MENU_V		  9
#define TRACK_MENU_WIDTH	200
#define TRACK_MENU_HEIGHT	 16
#define FORMAT_MENU_H		160
#define FORMAT_MENU_V		  9
#define FORMAT_MENU_WIDTH	230
#define FORMAT_MENU_HEIGHT	 16
#define DISK_TEXT_H			 10
#define DISK_TEXT_V			103
#define DISK_SPACE_H		 78
#define SELECTOR_H			  3
#define SELECTOR_V			 40
#define SELECTOR_HEIGHT		 48
#define DIVIDER				115

#define STATUS_WIDTH		300
#define STATUS_HEIGHT		 60
#define STATUS_BUTTON_WIDTH	 65
#define STATUS_BUTTON_HEIGHT 23
#define STATUS_BUTTON		  1
#define STATUS_MUTE_WIDTH	 45
#define STATUS_MUTE_HEIGHT	 13
#define STATUS_MUTE			  2
#define STATUS_TEXT_H		 10
#define STATUS_TEXT_V		 18
#define PROGRESS_H			 10
#define PROGRESS_V			 34
#define PROGRESS_WIDTH		205
#define PROGRESS_HEIGHT		 10

const uint32 M_SET_FORMAT = 'setf';

enum	save_buttons		{PLAY_BUT = 1, SAVE_BUT, STOP_BUT, AIFF, RAW, WAVE};
class	TSaveView;
class	TCDView;
class	TStatusView;

typedef struct {
	int32	flags;
	char	title[256];
	int32	length;
} track_info;
#define DIRTY	1

struct save_format {
	BString					name;
	media_file_format		file_format;
	media_codec_info		codec_info;
};


//====================================================================

class TSaveWindow : public BWindow {
public:
	TSaveWindow(BRect, char*, int32, scsi_toc*, int32, BList*);
	~TSaveWindow(void);
	virtual void MessageReceived(BMessage*);
	virtual bool QuitRequested(void);
	void ResetMenu(void);
	void ShowTrack(int32);
	void SaveRequested(BMessage*);

	int32 fCDID;
	int32 fTrack;
	BButton *fPlayButton;
	BButton *fStopButton;
	BButton *fSaveButton;
	BList *fTitleList;
	scsi_toc *fTOC;
	TSaveView *fSaveView;

private:
	status_t InitSaveFormats( void );
	void ConfigureCodec(void);
	BList				*saveFormats;
	save_format			*defaultSaveFormat;
	BMenu				*panelFormatMenu;
	BFilePanel			*filePanel;
	BMediaFile			*saveFile;
	BMediaTrack			*saveTrack;
};

class TSaveView : public BView {
public:

	TSaveView(BRect, char*, TSaveWindow*, int32);
	~TSaveView(void);
	virtual	void AttachedToWindow(void);
	virtual	void Draw(BRect);
	virtual void MouseDown(BPoint);
	virtual void Pulse(void);
	void DrawSelector(void);
	
	int32 fLastx;
	int32 fTrackStart;
	int32 fTrackLength;
	int32 fStart;
	int32 fStop;
	BPopUpMenu *fTrackMenu;
	BPopUpMenu *fFormatMenu;

private:

	int32			fCDID;
	int32			fPos;
	BBitmap			*fOffScreen;
	BView			*fOffView;
	TSaveWindow		*fWindow;
};

class TStatusWindow : public BWindow {
public:

	TStatusWindow(BRect, char*, BMediaFile*, BMediaTrack*, int32, int32, int32, int32);
	virtual void MessageReceived(BMessage*);

	bool			fStop;
	int32			fCDID;
	TStatusView		*fView;
};

class TStatusView : public BView {
public:

	TStatusView(BRect, char*, BMediaFile*, BMediaTrack*, int32, int32,
						int32, TStatusWindow*, int32); 
	virtual void AttachedToWindow(void);
	virtual	void Draw(BRect);
	static int32 _SaveThreadEntry(void*);
	void SaveThread();

	int32 fCDID;
	int32 fTrackStart;
	int32 fStart;
	int32 fStop;
	BMediaFile *fFile;
	BMediaTrack *fTrack;
	TStatusWindow *fWind;
	bool fIsEncodedFormat;
};

#endif
