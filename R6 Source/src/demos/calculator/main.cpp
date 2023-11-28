//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#include <string.h>

#include <Application.h>
#include <Beep.h>

#include "main.h"

//
// Globals
//
CalcWindow			*theWindow;		// Main calculator window
CalcView			*theView;		// Main calculator view
DisplayView			*dispView;		// The display view
CalcApplication		*theApp;		// The application object

//
// Global Preference Data
//
// This stuff comes from the settings file for BeCalculator
// (or will when we have one).
//
uint8				backgroundRed;	// Background color: red
uint8				backgroundGreen;// Background color: green
uint8				backgroundBlue;	// Background color: blue
uint8				displayRed;		// Display color: red
uint8				displayGreen;	// Display color: green
uint8				displayBlue;	// Display color: blue
int32				windowTop;		// Top of window
int32				windowLeft;		// Left of window
double				initValue;		// Initial value

//
// main
//
// Instantiate and run the application.  If we're
// launched from the command line, we allow some
// parameters, maybe.
//
int main(int argc, char *argv[]) {
	theApp = new CalcApplication;	// Instantiate the application
	theApp->Run();					// Run the application's looper
	delete theApp;					// Delete the app when done
	return 0;
}


//
// KeyBeep
//
// An invalid key was hit.  Beep or something.
//
void KeyBeep(void) {
	beep();
}

