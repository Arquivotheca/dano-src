/*************************************************************************
/
/	MinitelWindow.cpp
/
/	Written by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include <stdio.h>

#include <Application.h>

#include "MinitelWindow.h"
#include "MinitelView.h"


//========================================================================

MinitelWindow::MinitelWindow(BRect frame)
	: BWindow	(frame,
				 "Minitel",
				 B_TITLED_WINDOW_LOOK,
				 B_NORMAL_WINDOW_FEEL,
				 B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	BRect			r = frame;
	MinitelView*	view;

	r.OffsetTo(0, 0);
#ifdef USE_KEYBOARD_PANEL
	r.bottom -= kKEYBOARD_BAR_HEIGHT;
#endif
	AddChild(view = new MinitelView(r));
#ifdef USE_KEYBOARD_PANEL
	r.top = r.bottom;
	r.bottom = r.top + kKEYBOARD_BAR_HEIGHT;
	r.left--;
	r.right++;
	AddChild(new KeyboardPanel(r, view));
#endif
	Show();
}

//------------------------------------------------------------------------

void MinitelWindow::MessageReceived(BMessage* msg)
{
	BWindow::MessageReceived(msg);
}

//------------------------------------------------------------------------

bool MinitelWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
