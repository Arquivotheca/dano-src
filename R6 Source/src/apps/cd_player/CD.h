//--------------------------------------------------------------------
//	
//	CD.h
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef CD_H
#define CD_H
#include <EditTextView.h>
#include <TrackView.h>

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <List.h>
#include <Menu.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Point.h>
#include <Query.h>
#include <Rect.h>
#include <Roster.h>
#include <View.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#define	BROWSER_WIND		 82
#define	TITLE_BAR_HEIGHT	 25
#define	WIND_BORDER			  3	// in pixels
#define WIND_WIDTH			532
#define WIND_HEIGHT			108

#define LOGO_H				  7
#define LOGO_V				 12
#define SOUND_H				 23
#define SOUND_V				 47
#define CLOCK_H				296
#define CLOCK_V				 11

#define BUTTON_V			 70
#define STOP_H				 62
#define PLAY_H				(STOP_H + BUTTON_WIDTH)
#define PREV_H				(PLAY_H + BUTTON_WIDTH)
#define NEXT_H				(PREV_H + BUTTON_WIDTH)
#define BACK_H				(NEXT_H + BUTTON_WIDTH)
#define FOR_H				(BACK_H + BUTTON_WIDTH)
#define EJECT_H				312
#define SAVE_H				(EJECT_H + BUTTON_WIDTH)
#define MODE_H				(SAVE_H + BUTTON_WIDTH)
#define NORMAL_H			(MODE_H + BUTTON_WIDTH)
#define BLANK_H				480

#define TITLE_L				 62
#define TITLE_T				 11
#define TITLE_R				285
#define TITLE_B				 36

#define TRACK_L				313
#define TRACK_T				 11
#define TRACK_R				379
#define TRACK_B				 41

#define TRACK_TEXT_LEFT		(TRACK_L + 6)
#define TRACK_TEXT_TOP		(TRACK_T + 13)

#define TRACK_LED1_LEFT		(TRACK_L + 33 - 3)
#define TRACK_LED2_LEFT		(TRACK_L + 48 - 3)
#define TRACK_LED_TOP		(TRACK_T + 6)

#define TIME_L				386
#define TIME_T				 11
#define TIME_R				516
#define TIME_B				 41

#define TIME_TEXT1_LEFT		(TIME_L + 6)
#define TIME_TEXT1_TOP		(TIME_T + 13)
#define TIME_TEXT_RIGHT		(TIME_L + 55)

#define TIME_TEXT2_LEFT		(TIME_TEXT1_LEFT)
#define TIME_TEXT2_TOP		(TIME_TEXT1_TOP + 11)

#define TIME_LEFT			(TIME_L + 56)
#define TIME_TOP			(TIME_T + 13)

#define TIME_GUAGE_L		(TIME_L + 56)
#define TIME_GUAGE_T		(TIME_T + 17)
#define TIME_GUAGE_R		(TIME_R - 5)
#define TIME_GUAGE_B		(TIME_B - 4)

#define ITEM_L				 62
#define ITEM_T				 40
#define ITEM_R				285
#define ITEM_B				 54

#define SLIDER_BACK_LEFT	 10
#define SLIDER_BACK_TOP		 73
#define SLIDER_BACK_WIDTH	 44
#define SLIDER_BACK_HEIGHT	 15

#define CD_KEY				"CD:key"

enum	messages			{STOP_B = 1, PLAY_B, PREV_B, NEXT_B, BACK_B, FOR_B,
							 EJECT_B, SAVE_B, MODE_B, NORMAL_B, BLANK_B, CLOCK_B,
							 SLIDER_B, PAUSE_B, SHUFFLE_B, REPEAT_B, LISTITEM_B};
enum	states				{DISABLED_S, STOPPED_S, PLAYING_S, PAUSED_S};
enum	buttons				{NORMAL_M, SELECT_M, DISABLE_M};
enum	time				{TRACK_RE, DISC_RE, TRACK_EL, DISC_EL};
enum	menus				{MENU_ABOUT = 1, MENU_NEW, MENU_ID,
							 MENU_QUIT, MENU_OOPS};

class	TSaveWindow;
class	TCDWindow;
class	TCDView;

//====================================================================

class TCDApplication : public BApplication {

private:

bool			fPlayTrack;
int32			fTrack;
BMenu			*fDeviceMenu;
dev_t			fDevice;

public:
BPopUpMenu		*fMenu;

TCDWindow		*fWindow;
				TCDApplication(void);
				~TCDApplication(void);
virtual void	AboutRequested(void);
virtual void	ArgvReceived(int32, char**);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested(void);
virtual void	RefsReceived(BMessage*);
void			FindDevice(uint32, int32);
void			FindPlayer(int32, int32);
void			TryDir(const char*, BMenu*, BMenu*, int*);
};

//====================================================================

class TCDWindow : public BWindow {

public:
TCDView			*fView;

				TCDWindow(BRect, char*, int32);
				~TCDWindow(void);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested(void);
virtual void	WindowActivated(bool);
};

//====================================================================

class TCDView : public BView {

private:
bool			fReady;
int32			fMin;
int32			fSec;
float			fPercent;
int32			fState;
int32			fTrack;
int32			fTotalTracks;
int32			fMinTrack;
int32			fMaxTrack;
int32			fTimeMode;
int32			fPlayMode;
int32			fRepeatMode;
BBitmap*		fListOpenButton[3];
BBitmap*		fListCloseButton[3];
BBitmap*		fStopButton[3];
BBitmap*		fPlayButton[3];
BBitmap*		fPauseButton[2];
BBitmap*		fPrevButton[3];
BBitmap*		fNextButton[3];
BBitmap*		fBackButton[3];
BBitmap*		fForButton[3];
BBitmap*		fEjectButton[3];
BBitmap*		fSaveButton[3];
BBitmap*		fModeButton[3];
BBitmap*		fShuffleButton[3];
BBitmap*		fRepeatButton[3];
BBitmap*		fNormalButton[3];
BBitmap*		fClockButton[3];
BBitmap*		fLogo;
BBitmap*		fSound;
BBitmap*		fSlider;
BBitmap*		fLEDs[10];
BBitmap*		fOffScreen;
BView*			fOffView;
scsi_toc		fTOC;

public:
int				fCDID;
int32			fCDIndex;
uint32			fKey;
BList*			fTitleList;
BTextView*		fTextView;
TTrackView*		fTrackView;
TSaveWindow*	fSaveWindow;

				TCDView(BRect, char*, int32 index); 
				~TCDView(void);
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
virtual void	Pulse(void);
void			ChangeDriver(int32);
void			DisablePlayer(void);
void			DrawButtons(BRect);
void			DrawListItems(bool);
void			DrawListItems(int32, int32);
void			DrawTime(void);
void			DrawTitle(bool, int32 index = 0);
void			DrawTrack(int32, bool update_title = true);
void			FindRecord(uint32, bool);
bool			IsDirty(void);
void			PlayTrack(int32);
void			SetTime(int32);
void			Status(void);
};

//====================================================================

class TSliderView : public BView {

public:
bool			fReady;
float			fValue;
BBitmap*		fOffScreen;
BBitmap*		fSlider;
BView*			fOffView;
TCDView*		fCDView;

				TSliderView(BRect, char*, TCDView*);
				~ TSliderView(void);
virtual	void	AttachedToWindow(void);
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
virtual void	Pulse(void);
void			DrawSlider(void);
void			SetValue(float);
float			Value(void);
};
#endif
