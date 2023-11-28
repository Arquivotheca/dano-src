//--------------------------------------------------------------------
//	
//	Minesweeper.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Directory.h>
#include <File.h>
#include <Looper.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Point.h>
#include <Handler.h>
#include <RadioButton.h>
#include <Rect.h>
#include <Roster.h>
#include <View.h>
#include <Window.h>

#define	BROWSER_WIND		 82
#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			289
#define WIND_HEIGHT			168

#define BORDER1				  3
#define BORDER2				  5
#define BORDER3				  3
#define TILE				 16

#define HEADER_BORDER		  2
#define HEADER_HEIGHT		 33
#define LED_BORDER			  1
#define LED_SIZE			((3 * 13) - 1)

#define DARK_GRAY			 80,  80,  80
#define LIGHT_GRAY			184, 184, 184
#define WHITE				255, 255, 255

enum	TILES				{T0 = 0, T1, T2, T3, T4, T5, T6, T7, T8,
							 T_BLANK, T_FLAG, T_GUESS, T_BOMB_OK,
							 T_BOMB_DEAD, T_BOMB_WRONG};

enum	LEDS				{L0 = 0, L1, L2, L3, L4, L5, L6, L7, L8,
							 L9, LDASH, LP, LA, LU, LS, LE, LD};

enum	FACES				{F_UP = 0, F_DOWN, F_GUESS, F_WON, F_LOST};

enum	GAME				{G_NEW = 0, G_STARTED, G_FINISHED, G_PAUSED};

enum	MENU				{M_NEW = 1, M_PAUSE, M_BEG,
							 M_INT, M_EXP, M_CUS, M_CUSTOM, M_BEST};

enum	FLAGS				{F_FREE = 0, F_BOMB};

typedef struct {
	char	flags;
	char	state;
} tile;

typedef struct {
	int32	x;
	int32	y;
	int32	flags;
} position;

enum	POSITIONS			{TOP_LEFT = 1,     TOP = 2,     TOP_RIGHT = 4,
							 LEFT = 8,                      RIGHT = 16,
							 BOTTOM_LEFT = 32, BOTTOM = 64, BOTTOM_RIGHT = 128};

class	TMinesWindow;
class	TMinesView;


//====================================================================

class TMinesApp : public BApplication {

private:

TMinesWindow*	fWindow;

typedef BApplication inherited;

public:

BFile*			fPrefs;
BPopUpMenu		*fMenu;

				TMinesApp(void);
				~TMinesApp(void);
virtual void	AboutRequested(void);
virtual void	MessageReceived(BMessage*);
void			NewWindow(int32, int32, int32);
void			SetTime(BMessage*);
};

//--------------------------------------------------------------------

class TMinesWindow : public BWindow {

private:

bool			fPaused;

public:

TMinesView*		fView;

				TMinesWindow(BRect, char*);
virtual void	MessageReceived(BMessage*);
virtual bool	QuitRequested();
virtual void	WindowActivated(bool);
};

//--------------------------------------------------------------------

class TMinesView : public BView {

private:

int32			fFaceState;
int32			fTime;
int32			fBombs;
double			fStartTime;
double			fPauseTime;
tile*			fBoard;
BBitmap*		fTiles[15];
BBitmap*		fLEDs[17];
BBitmap*		fFaces[5];

public:

int32			fGameState;

				TMinesView(BRect, char*); 
				~TMinesView();
virtual	void	AttachedToWindow();
virtual	void	Draw(BRect);
virtual void	KeyDown(const char*, int32);
virtual void	MouseDown(BPoint);
virtual void	Pulse();
void			DrawFace();
void			DrawBombs();
void			DrawBoard(BRect);
void			DrawTime();
int32			Bombs(int32, int32);
void			MoveIt(int32, int32);
void			NewGame();
void			Pause();
void			ShowGroup(int32, int32, int32, int32, int32*);
void			TrackFace(BRect);
void			TrackTile(int32, int32, BPoint, uint32);
};

int32			GetTime(char *, int32);
#endif
