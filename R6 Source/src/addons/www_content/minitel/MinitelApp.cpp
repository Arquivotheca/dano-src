/*************************************************************************
/
/	MinitelApp.cpp
/
/	Written by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/

#include <Entry.h>

#include "MinitelApp.h"
#include "MinitelFont.h"
#include "MinitelWindow.h"
#include "MinitelView.h"

#define kMINITEL_WIDTH		(40 * kFONT_CELL_WIDTH)
#define kMINITEL_HEIGHT		(25 * kFONT_CELL_HEIGHT)


//========================================================================

int main()
{
	MinitelApp	app;

	app.Run();
	return B_NO_ERROR;
}


//========================================================================

MinitelApp::MinitelApp()
	: BApplication("application/x-vnd.Be.pluginminitel")
{

#ifdef USE_KEYBOARD_PANEL
	new MinitelWindow(BRect(150, 80,
							150 + kMINITEL_WIDTH, 80 + kMINITEL_HEIGHT + kKEYBOARD_BAR_HEIGHT));
#else
	new MinitelWindow(BRect(150, 80,
							150 + kMINITEL_WIDTH, 80 + kMINITEL_HEIGHT));
#endif
}

//------------------------------------------------------------------------

void MinitelApp::RefsReceived(BMessage* msg)
{
	int32		index = 0;
	entry_ref	ref;

	while (msg->FindRef("refs", index++, &ref) == B_NO_ERROR)
	{
		//fProtocole->doOpen(&ref);
	}
}
