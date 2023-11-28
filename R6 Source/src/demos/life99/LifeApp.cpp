/*
	
	HelloWorld.cpp
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_WINDOW_H
#include "LifeWindow.h"
#endif
#ifndef HELLO_WORLD_H
#include "LifeApp.h" 
#endif

#include <Alert.h>
	
int main(int, char**)
{	
	HelloApplication	myApplication;

	myApplication.Run();

	return(0);
}
	
HelloApplication::HelloApplication()
		  		  : BApplication("application/x-vnd.Be-HelloWorldSample")
{
	HelloWindow		*aWindow;
	BRect			aRect;

	// set up a rectangle and instantiate a new window
	aRect.Set(20, 20, (20 + XMAX*8), (68 + YMAX*8));
	aWindow = new HelloWindow(aRect);
			
	// make window visible
	aWindow->Show();
}

void	HelloApplication::AboutRequested (void)
{
	(new BAlert("", "Life '99\nBy Eric Rall\n(C) 1999 Be, Inc\n\nClick the cells to turn them on and off\nTime Warp runs Life for 1000 cycles without redrawing\n", "Close"))->Go();
}

