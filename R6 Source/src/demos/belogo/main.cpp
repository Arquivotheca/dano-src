//******************************************************************************
//
//	File:		main.cpp
//
//	Description:	3d hidden line main program
//
//	Written by:	Benoit Schillings
//
//	Copyright 1993, Be Incorporated
//
//	Change History:
//
//	7/31/93		bgs	new today
//	12/14/95	CKH	removing menu stuff
//
//******************************************************************************

#include <Debug.h>
#include <time.h>

#include <OS.h>
#include "tsb.h"

#include <math.h>

#ifndef	_APPLICATION_H
#include <Application.h>
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
#include <stdio.h>
#ifndef _ALERT_H
#include <Alert.h>
#endif
/*------------------------------------------------------------*/
/* Those are the menu item id's of the main window	      */
/*------------------------------------------------------------*/

// #define	ABOUT	0x01
// #define	QUIT	0x02
// #define	DEMO	0x03


/*------------------------------------------------------------*/
TShowBit	*the_main_view;
/*------------------------------------------------------------*/

class Belogo : public BApplication {

public:
					Belogo ();
		bool		QuitRequested ();
virtual 	void	AboutApplication();
};

/*------------------------------------------------------------*/

	Belogo::Belogo()
	  	  :BApplication ("application/x-vnd.Be-BE3D")
{
}

void Belogo::AboutApplication()
{
BAlert *alert = new BAlert("",
	"BeLogo\n A simple Be application\n Benoit Shillings \n Copyright 1995, Be Inc.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_INFO_ALERT);
alert->Go();


}
/*------------------------------------------------------------*/

bool	Belogo::QuitRequested()
{
	if (the_main_view->exit_now == 2) {
		the_main_view->exit_now = 1;
		snooze(100*1000);
	}

	return(TRUE);
}

/*------------------------------------------------------------*/

class	TMainWindow : public BWindow {
public:
		char		quit;
		TShowBit	*the_view;
		BScrollView	*scroll_view;
		
					TMainWindow(BRect bound,
				    char *name,
				    window_type type,
				    long flags);
virtual		void	MessageReceived(BMessage *an_event);
virtual		bool	QuitRequested();
virtual		void	FrameResized(float new_width, float new_height);
};

/*------------------------------------------------------------*/

	TMainWindow::TMainWindow(BRect bound, char *name, window_type type, long flags) :
	BWindow(bound, name, type, flags)
{
	BMenu		*a_menu;
	BRect		a_rect;
	BMenuBar	*menubar;
	TShowBit	*my_view;


	quit = 0;
	a_rect.top = a_rect.left = 0;
	a_rect.right = max_x;
	a_rect.bottom = max_y;

	the_view = my_view = new TShowBit(a_rect, B_FOLLOW_ALL | B_WILL_DRAW);
	the_main_view = the_view;
//+	scroll_view = new BScrollView("noname", my_view, B_FOLLOW_ALL,
//+		B_WILL_DRAW, TRUE, TRUE, B_NO_BORDER);
//+	AddChild(scroll_view);
	AddChild(my_view);
	Show();
	the_view->demo();
}

/*------------------------------------------------------------*/

bool	TMainWindow::QuitRequested()
{
	be_app->PostMessage(new BMessage(B_QUIT_REQUESTED));
	return(FALSE);
}

/*------------------------------------------------------------*/

void	TMainWindow::FrameResized(float new_width, float new_height)
{
#if 0
	BScrollBar *hs = scroll_view->ScrollBar(B_HORIZONTAL);
	BScrollBar *vs = scroll_view->ScrollBar(B_VERTICAL);

	new_width -= B_V_SCROLL_BAR_WIDTH;
	new_height -= B_H_SCROLL_BAR_HEIGHT;
//	PRINT_OBJECT(Bounds());
	if (new_width < max_x) {
		hs->SetRange(0, max_x - new_width);
		PRINT(("H max=%.2f", max_x - new_width));
	} else {
		hs->SetRange(0,0);
		PRINT(("H=%.2f", 0.0));
	}
	if (new_height < max_y) {
		vs->SetRange(0, max_y - new_height);
		PRINT((", V max=%.2f\n", max_y - new_height));
	} else {
		vs->SetRange(0,0);
		PRINT((", V=%.2f\n", 0.0));
	}
#endif
}

/*------------------------------------------------------------*/

void	TMainWindow::MessageReceived(BMessage *an_event)
{
	inherited::MessageReceived(an_event);	
}

/*------------------------------------------------------------*/

int
main()
{
	Belogo		*my_app;
	TMainWindow	*a_window;
	BRect		a_rect;

	set_thread_priority(find_thread(NULL), B_DISPLAY_PRIORITY);
	
	my_app = new Belogo();

	a_rect.Set(100, 100, 100+max_x, 100+max_y);
	a_window = new TMainWindow(a_rect, "BeLogo", B_TITLED_WINDOW, 0);
	a_window->SetZoomLimits(max_x, max_y);
	a_window->SetSizeLimits(80, max_x, 80, max_y);
	my_app->Run();


	delete my_app;
	return 0;
}
