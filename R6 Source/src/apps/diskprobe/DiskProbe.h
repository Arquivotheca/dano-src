//--------------------------------------------------------------------
//	
//	DiskProbe.h
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DISKPROBE_H
#define DISKPROBE_H

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Directory.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Looper.h>
#include <Point.h>
#include <Rect.h>
#include <Roster.h>
#include <Window.h>

#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			457
#define WIND_HEIGHT			400

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 20

enum	BASES				{B_DECIMAL = 10, B_HEX = 16};
enum	MESSAGES			{M_OPEN_DEVICE = 64, M_OPEN_FILE, M_WINDOW_CLOSED};
enum	TYPE				{T_NONE = 0, T_FILE, T_DEVICE};
enum	WINDOWS				{W_SELECT_OK = 1, W_SELECT_CANCEL, W_PROBE};

#define FONT_SIZE			 10.0

typedef struct {
	int32	base;
	int32	font_size;
	bool	case_sensitive;
} prefs;

class	TProbeWindow;
class	TSelectWindow;


//====================================================================

class TDiskApp : public BApplication {

private:

int32			fBase;
int32			fFontSize;
int32			fWindowCount;
BFile			*fPrefFile;
BFilePanel		*fPanel;
BRect			fDiskWindow;
entry_ref		fDir;
prefs			fPrefs;
TSelectWindow	*fSelect;

public:

				TDiskApp(void);
				~TDiskApp(void);
virtual void	AboutRequested(void);
virtual void	ArgvReceived(int32, char**);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested(void);
virtual void	ReadyToRun(void);
virtual void	RefsReceived(BMessage*);
TProbeWindow*	FindWindow(entry_ref ref);
TProbeWindow*	NewWindow(int32, void *data = NULL);
void			SelectWindow(void);
};
#endif
