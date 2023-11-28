//--------------------------------------------------------------------
//	
//	IconWorld.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1994-97 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <string.h>
#include <Debug.h>

#ifndef ICON_WORLD_H
#include "IconWorld.h"
#endif

#include <stdio.h>
#include <Screen.h>


//====================================================================

int main(void)
{
  TIconApp	*app;

  app = new TIconApp();
  app->Run();
  
  delete app;
  return B_NO_ERROR;
}


//====================================================================

TIconApp::TIconApp(void)
		 :BApplication("application/x-vnd.Be-IWLD")
{
  BRect		r;

  fRefs = FALSE;
  fWindowCount = 0;
  
  fClipFlag = FALSE;
  r.Set(0, 0, LARGEICON - 1, LARGEICON - 1);
  fClipIcon = new BBitmap(r, B_COLOR_8_BIT);
  
  {
    // Make sure to hold onto the BScreen for as short as possible.
    BScreen screen( B_MAIN_SCREEN_ID );
    BRect screen_frame = screen.Frame();
    r.left = screen_frame.right - (TOOLSIZE * TOOLWIDTH) - (WINDBORDER * 2 + 1) - 10;
    r.top = screen_frame.top + TITLEBARHEIGHT;
  }
  
  r.right = r.left + (COLORSIZE * COLORWIDTH) + (WINDBORDER * 2 + 1);
  r.bottom = r.top + (COLORSIZE * COLORHEIGHT) + (WINDBORDER * 2 + 1) + (3 * 13);
  fColorWind = new TColorWindow(r, "Colors");
  fColorWind->Show();

  r.top = r.bottom + TITLEBARHEIGHT + 10;
  r.right = r.left + (TOOLSIZE * TOOLWIDTH) + (WINDBORDER * 2 + 1);
  r.bottom = r.top + (TOOLSIZE * TOOLHEIGHT) + (WINDBORDER * 2 + 1);
  fToolWind = new TToolWindow(r, "Tools");
  fToolWind->Show();

  r.top = r.bottom + TITLEBARHEIGHT + 10;
  r.right = r.left + (PATSIZE * PATWIDTH) + (WINDBORDER * 2 + 1);
  r.bottom = r.top + (PATSIZE * PATHEIGHT) + (WINDBORDER * 2 + 1);
  fPatWind = new TPatWindow(r, "Patterns");
  fPatWind->Show();
}

//--------------------------------------------------------------------

TIconApp::~TIconApp(void)
{
	delete fClipIcon;

	if (fColorWind->Lock())
		fColorWind->Quit();
	if (fToolWind->Lock())
		fToolWind->Quit();
	if (fPatWind->Lock())
		fPatWind->Quit();
}

//--------------------------------------------------------------------

void TIconApp::AboutRequested(void)
{
	(new BAlert("", "...by Robert Polic", "OK"))->Go();
}

//--------------------------------------------------------------------

void TIconApp::ArgvReceived(int32 argc, char** argv)
{
  char		str[256];
  int			i;
  BEntry		entry;
  entry_ref	ref;
  
  for (i = 1; i < argc; i++) {
    entry.SetTo(argv[i]);
    if (entry.InitCheck() == B_NO_ERROR) {
      entry.GetRef(&ref);
      OpenIconWindow(&ref);
    }
    else {
      sprintf(str, "Error: Could not open '%s'", argv[i]);
      (new BAlert("", str, "Sorry"))->Go();
    }
  }
  fRefs = TRUE;
}

//--------------------------------------------------------------------

void TIconApp::MessageReceived(BMessage *msg)
{
  BFilePanel	*panel;

  switch (msg->what) {
  case M_NEW:
    OpenIconWindow(NULL);
    break;

  case M_OPEN:
    panel = new BFilePanel();
    panel->Window()->Show();
    break;

  case M_CLOSE_WINDOW:
    fWindowCount--;
    if (!fWindowCount)
      BApplication::Quit();
     break;
	case B_SIMPLE_DATA:
  case B_REFS_RECEIVED:
    RefsReceived(msg);
    break;
    
  default:
    BApplication::MessageReceived(msg);
  }
}

//--------------------------------------------------------------------

bool TIconApp::QuitRequested(void)
{
	int32		i;
	bool		result = TRUE;
	TIconWindow	*wind;

	i = 0;
	while ((wind = (TIconWindow *)(be_app->WindowAt(i)))) {
		if (((uint32)wind != (uint32)fColorWind) &&
			 ((uint32)wind != (uint32)fToolWind) &&
			 ((uint32)wind != (uint32)fPatWind)) {
			if (wind->Lock()) {
				//   !!!!!
				//		deal with the Open and Save, they don't respond to quitrequested
				//
				if (	(strcmp(wind->Title(),"IconWorld: Open") == 0) || 
						(strcmp(wind->Title(),"IconWorld: Save") == 0) ) {
					wind->Quit();
				} else {
					result = wind->QuitRequested();
					if (result)
						wind->Quit();
					else {
						wind->Unlock();
						return FALSE;
					}
				}
			}
			fWindowCount--;
		}
		else
			i++;
	}
	return TRUE;
}

//--------------------------------------------------------------------

void TIconApp::RefsReceived(BMessage *msg)
{
  int32		 count;
  int32		 i;
  uint32	 type;
  entry_ref	ref;

  if (msg->GetInfo("refs", &type, &count) == B_NO_ERROR) {
    for (i = 0; i < count; i++) {
      msg->FindRef("refs", i, &ref);
      OpenIconWindow(&ref);
    }
    fRefs = TRUE;
  }
}

//--------------------------------------------------------------------

void TIconApp::ReadyToRun(void)
{
  // !!!!!
  // tool windows are shown in the app constructor
  //
  // if we didn't get a refsreceived, then we were simply launched
  // show a default icon window
  //
  if ((fWindowCount == 0) && (fRefs  == false)){
    OpenIconWindow(NULL);
  }
  //
  // if something went wrong, quit the ourself
  //
  if (fWindowCount == 0)
    BApplication::Quit();  
}

//--------------------------------------------------------------------

void     TIconApp::OpenIconWindow(entry_ref *ref)
{
  float			x, y;

  x = 6.0 + (fWindowCount * 10);
  y = TITLEBARHEIGHT + (fWindowCount * 10);
  
  new TIconWindow(BRect(x, y, x + LIST32RIGHT + WINDBORDER + 100,
			y + FAT16BOTTOM + WINDBORDER), ref);
  fWindowCount++;
}
