/*
	
	HelloWorld.cpp
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/

#ifndef MAP_WINDOW_H
#include "map_window.h"
#endif
#ifndef MAP_VIEW_H
#include "map_view.h"
#endif
#ifndef MAP_H
#include "map.h"
#endif

main()
{	
	HelloApplication *myApplication;

	myApplication = new HelloApplication();
	myApplication->Run();
	
	delete(myApplication);
	return(0);
}

HelloApplication::HelloApplication()
		  		  : BApplication('HLWD')
{
	HelloWindow		*aWindow;
	HelloView		*aView;
	BRect			aRect;

	// set up a rectangle and instantiate a new window
	aRect.Set(20, 20, 465, 465);
	aWindow = new HelloWindow(aRect);
	
	// set up a rectangle and instantiate a new view
	// view rect should be same size as window rect but with left top at (0, 0)
	aRect.OffsetTo(B_ORIGIN);
	aView = new HelloView(aRect, "MapView");
	
	// add view to window
	aWindow->AddChild(aView);
	
	// make window visible
	aWindow->Show();
}
