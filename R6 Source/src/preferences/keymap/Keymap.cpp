//--------------------------------------------------------------------
//	
//	Keymap.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "Keymap.h"
#include <fcntl.h>
#include <unistd.h>
#include <Entry.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <Screen.h>

//====================================================================

int main()
{	
	TKeymapApplication	*myApplication;

	myApplication = new TKeymapApplication();
	myApplication->Run();

	delete myApplication;
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TKeymapApplication::TKeymapApplication()
		  :BApplication("application/x-vnd.Be-KBRD")
{
  BRect			keymapTempRect;
  BPath                 path;

  //_setDebugFlag(TRUE);
  //printf("Creating application\n");
  keymapTempRect.Set(BROWSERWIND, TITLEBARHEIGHT,
		     BROWSERWIND + KEYMAPWIDTH + WINDBORDER * 2 + SIDEPANELWIDTH,
		     TITLEBARHEIGHT + KEYMAPHEIGHT + WINDBORDER * 2 + KEYSMENU + 1+
		     BUTTONBOXHEIGHT);
  //if ((ref = open("/system/settings/Keymap_data", 0)) >= 0) 
  if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_NO_ERROR)
    {
      #if 0
      //printf("Appending path\n");
      if (path.Append("Keymap_Data") ==B_NO_ERROR)
	{
	  printf("Opening file\n");
	  if (ref = open(path.Path(), 0) >= 0)
	    {
	      printf("Reading data\n");
	      read(ref, &keymapWindPos, sizeof(BPoint));
	      printf("Closing file\n");
	      close(ref);
	      if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(win_pos))
		keymapTempRect.OffsetTo(keymapWindPos);
	    }
	}
      #endif
      
    }
  //printf("Creating keymap window\n");
  fWindow = new TKeymapWindow( (BEntry*)NULL, FALSE, keymapTempRect, "Keymap");
  //printf("Showing keymap window\n");
  fWindow->Show();
}

//--------------------------------------------------------------------

void TKeymapApplication::ArgvReceived(int32 , char** )
{
  
#if 0
  const char		*badFile = "Sorry: Bad file name!";
  BEntry		entry;
  entry_ref	ref;
  status_t	status;

  for (int i = 1; i < argc; i++) {
    status = entry.SetTo(argv[i]);
    if (status == B_NO_ERROR) {
      entry.GetRef(&ref);
      fWindow->OpenFile(ref);
    }
    else 
      (new BAlert("",badFile,"Sorry"))->Go();
  }
#endif
}

//--------------------------------------------------------------------

void TKeymapApplication::RefsReceived(BMessage *theMessage)
{
  entry_ref	ref;
  if (theMessage->HasRef("refs")) {
    theMessage->FindRef("refs", &ref);
    fWindow->OpenFile(ref, FALSE);
  }
}

//--------------------------------------------------------------------

void TKeymapApplication::AboutRequested()
{
	BAlert	*myAlert;

	myAlert = new BAlert("", "...by Robert Polic,\n   Geoff Woodcock,\n   & George Hoffman","Big Deal");
	myAlert->Go();
}


