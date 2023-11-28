//--------------------------------------------------------------------
//	
//	Desktop.h
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DESKTOP_H
#define DESKTOP_H

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _POINT_H
#include <Point.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef	_REGION_H
#include <Region.h>
#endif
#ifndef _ROSTER_H
#include <Roster.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif

#define	BROWSER_WIND		82
#define	TITLE_BAR_HEIGHT	25
#define	WIND_BORDER			 3	// in pixels
#define CELLS_WIDE			16	// in cells
#define CELLS_TALL			16	// in cells
#define CELL_WIDTH			 8	// in pixels
#define CELL_HEIGHT			 8	// in pixels

enum menuIDs		{	LAVA_DESK = 1,
						LAVA_DESK_QUIT
					};


//====================================================================

class TDesktopApplication : public BApplication {

private:
uchar			fCurrentColor;
BPopUpMenu		*fMenu;

public:
				TDesktopApplication();
virtual void	AboutRequested();
virtual void	MessageReceived(BMessage*);
};

//====================================================================

class TDesktopWindow : public BWindow {

public:
				TDesktopWindow(BRect, char*);
virtual	bool 	QuitRequested();
};

//====================================================================

class TDesktopView : public BView {

private:
short			fCellsWide;
short			fCellsTall;
short			fCellWidth;
short			fCellHeight;
short			fHorMargin;
short			fVerMargin;
thread_id		fThreadID;

public:
bool			fDie;
uchar			fTheColor;

				TDesktopView(BRect, char*); 
				~TDesktopView(); 
virtual	void	AttachedToWindow();
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
virtual void	FrameResized(float, float);
void			SetColor(uchar, bool);
static  long	LavaThread(void*);
};
#endif
