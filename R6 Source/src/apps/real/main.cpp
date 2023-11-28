//******************************************************************************
//
//	File:		main.cpp
//
//
//******************************************************************************


#include <Debug.h>

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _ROSTER_H
#include <Roster.h>
#endif
#ifndef	_BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MENU_H
#include <Menu.h>
#endif
#ifndef _MENU_BAR_H
#include <MenuBar.h>
#endif
#ifndef	_SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#ifndef	_ALERT_H
#include <Alert.h>
#endif

#include <stdio.h>

#include "top_view.h"
#include "transport.h"
#include "snd_src.h"


#include <math.h>

/*------------------------------------------------------------*/

class	TMainWindow : public BWindow {
public:
					TMainWindow(BRect bound, char *name, window_type type,
				    			long flags);
virtual		void	MessageReceived(BMessage *an_event);
virtual		bool	QuitRequested();


private:
		TopView		*top;
};

/*------------------------------------------------------------*/

TMainWindow::TMainWindow(BRect bound, char *name, window_type type, long flags)
	: BWindow(bound, name, type, flags)
{
	BMenu		*a_menu;
	BRect		a_rect;

	Lock();


	top = new TopView(BRect(0,0,799,599), B_FOLLOW_NONE | B_WILL_DRAW);
	
	AddChild(top);



	SetPulseRate(10000);
	Show();
	Unlock();
}

/*------------------------------------------------------------*/

bool	TMainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

/*------------------------------------------------------------*/

void	TMainWindow::MessageReceived(BMessage *an_event)
{
}

/*------------------------------------------------------------*/

int main()
{
	BApplication	*my_app;
	TMainWindow	*a_window;
	BRect		a_rect;

	set_thread_priority(find_thread(NULL), B_DISPLAY_PRIORITY);
	
	my_app = new BApplication("application/x-vnd.Be-MAND");

	a_rect.Set(50, 50, 849, 649);
	a_window = new TMainWindow(a_rect, "RealDemo", B_DOCUMENT_WINDOW,
		B_WILL_ACCEPT_FIRST_CLICK | B_PULSE_NEEDED);
	my_app->Run();

	delete my_app;
	return 0;
}
