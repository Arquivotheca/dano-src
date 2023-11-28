#include <Alert.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "TimeWindow.h"
#include "DateTime.h"


rgb_color kWhite = {255,255,255,255};
rgb_color kBlack = {0,0,0,255};
rgb_color kViewGray;
rgb_color kLightGray;
rgb_color kMediumGray;
rgb_color kDarkGray;

// ************************************************************************** //

int
main()
{
	TApp app;

	// XXXbjs Must Alter Global Settings -- temporary hack	
	setgid(0);
	setuid(0);

	app.Run();
	return 0;
}

// ************************************************************************** //

TApp::TApp() 
	: BApplication("application/x-vnd.Be-TIME")
{
}

void TApp::ReadyToRun()
{
	kViewGray 	= ui_color(B_PANEL_BACKGROUND_COLOR);
	kLightGray 	= tint_color(kViewGray, B_DARKEN_1_TINT);
	kMediumGray = tint_color(kViewGray, B_DARKEN_2_TINT);
	kDarkGray 	= tint_color(kViewGray, B_DARKEN_4_TINT);

	TTimeWindow* window = new TTimeWindow();
	window->Show();
}

TApp::~TApp()
{
}

void 
TApp::AboutRequested()
{
	BAlert *myAlert;
	
	myAlert = new BAlert("Date & Time", "The Date & Time preference panel.", "Okay");
	myAlert->Go();
}

void 
TApp::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

