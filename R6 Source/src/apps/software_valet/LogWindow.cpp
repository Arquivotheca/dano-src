/*
	
	HelloWindow.cpp
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/

#include "LogWindow.h"
#include "LogView.h"

LogWindow::LogWindow(BRect)
				: BWindow(BRect(0,0,420,280), "SoftwareValet Log", B_TITLED_WINDOW, 0)
{
	AddChild(new LogView(Bounds()));
	
	SetSizeLimits(320,480,106,8192);
}

bool LogWindow::QuitRequested()
{
	//be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}
