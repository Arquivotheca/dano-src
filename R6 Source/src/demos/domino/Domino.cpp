//*****************************************************************************
//
//	File:		 Domino.cpp
//
//	Description: Application class for domino game demo.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#include <stdlib.h>
#include "DomWindow.h"
#include "Domino.h"
#include <Alert.h>
#include <stdio.h>
//#include <FS.h>
#include <Debug.h>
#include <Resources.h>

// Pointer to the texture buffer
uchar    *bitmap;
uchar    bad_driver = 0;

// Just a standard application.
DominoApp::DominoApp():BApplication ("application/x-vnd.Be-DOM2") {}

// Load the texture map. You can update the texture map by using a DataFile,
// row format, 400x300 pixels, system color_map, with a 44 bytes headers. This
// is looking for a file named "Domino DataFile" in the same folder than the
// application. If found, it opens it, load the texture map and update the
// resource of the app. In the other, it just loads the resource of the app.
void DominoApp::ReadyToRun ()
{
	status_t      error;
	size_t        size;
	short         height;
	app_info	  info;
	BFile         *myFile;
	BResources    *myRes;

// Preset all the pointers to 0.
	myFile = 0L;
	window = 0L;
	bitmap = 0L;

// Get a ResourceFile object on the application and open it.
	if (be_app->GetAppInfo(&info) != B_NO_ERROR) goto end;
	myFile = new BFile(&info.ref, O_RDONLY);
	myRes = new BResources(myFile);
	bitmap = (uchar*)myRes->FindResource('GRAF', 1000, &size);
	if (size != 120000L) goto end;
	if (bitmap == 0L) goto end;
// Create a WindowScreen, 8 bits, 640x480, named Domino.
	window = new DomWindow(B_8_BIT_640x480,"Domino", &error);
	if (!window->CanControlFrameBuffer()) {
		bad_driver = 1;
		goto end;
	}
	if (error == B_ERROR) {
		bad_driver = 2;
		goto end;
	}
	window->Show();
	goto exit;
// If something goes wrong, abort the application.
 end:
	PostMessage(B_QUIT_REQUESTED);
// Close the opened files and directory.
 exit:
	if (myRes != 0L)
		delete myRes;
	if (myFile != 0L)
		delete myFile;
}

// Minimal main function.
int main ()
{
	DominoApp *myApp;

	myApp = new DominoApp();
	myApp->Run();

// Free the texture map if necessary.
	if (bitmap != 0L)
		free(bitmap);

	if (bad_driver == 1) {
		BAlert   *alert;
		
		alert = new BAlert("", "Sorry, no advanced GameKit support.\n Dominos can't run...",
						   "Too Bad...", NULL, NULL);
		alert->Go();
	}
	
	delete(myApp);
	return 0;
}













