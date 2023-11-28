//
// BeCalculator
// Copyright 1997, Be Incorporated
// by Eric Shepherd
//

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <FindDirectory.h>

#include "window.h"
#include "view.h"
#include "displayview.h"
#include "application.h"
#include "main.h"

#define PREFS_VERSION 2

//
// CalcApplication::CalcApplication
//
// Construct the application object.
//
CalcApplication::CalcApplication()
				: BApplication("application/x-vnd.Be-CALC") {
	BRect r;
	
	GetPrefs();					// Get the application's preferences
	r.Set(0, 0, 125, 205);
	r.OffsetBy(windowLeft, windowTop);
	theWindow = new CalcWindow(r);
	
	// Add the main view to the window
	
	theView = new CalcView(theWindow->Bounds());
	
	// Add the display view to the main view
	
	r.Set(6, 6, 119, 21);
	dispView = new DisplayView(r);
	theView->AddChild(dispView);
	
	theWindow->AddChild(theView);
	theView->MakeFocus();
	theWindow->Show();
}


//
// CalcApplication::GetPrefs
//
// Read the settings file.
//
void CalcApplication::GetPrefs(void) {
	char path[129];
	int32 prefsVersion;
	
	// For now, we just initialize to defaults.
	// Add code to load an existing settings file.
		
	windowTop = 50;
	windowLeft = 50;
	
	initValue = 0.0;
	
	// Now try reading the settings file
	
	if (!find_directory(B_USER_SETTINGS_DIRECTORY, 0, true, path, 100)) {
		FILE *f;
		
		strcat(path, "/calc_settings");
		if (f = fopen(path, "r")) {
			fscanf(f, "%ld\n", &prefsVersion);
			if (prefsVersion == PREFS_VERSION) {
				fscanf(f, "%lf\n", &initValue);
				fscanf(f, "%ld %ld\n", &windowTop, &windowLeft);
				fclose(f);
			}
		}
	}
}


//
// CalcApplication::SavePrefs
//
// Save the settings file.
//
void CalcApplication::SavePrefs(void) {
	char path[129];
	
	BRect r;
	r = theWindow->Frame();
	windowLeft = int32(r.left);
	windowTop = int32(r.top);
	
	if (!find_directory(B_USER_SETTINGS_DIRECTORY, 0, true, path, 100)) {
		FILE *f;
		
		strcat(path, "/calc_settings");
		if (f = fopen(path, "w")) {
			fprintf(f, "%ld\n", PREFS_VERSION);
			fprintf(f, "%g\n", dispView->GetValue());
			fprintf(f, "%ld %ld\n", windowTop, windowLeft);
			fclose(f);
		}
	}
}
