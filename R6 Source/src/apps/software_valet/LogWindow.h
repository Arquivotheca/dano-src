/*
	
	HelloWindow.h
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _LOGWINDOW_H_
#define _LOGWINDOW_H_

#include <Window.h>

class LogWindow : public BWindow {

public:
				LogWindow(BRect frame); 
virtual	bool	QuitRequested();
};

#endif
