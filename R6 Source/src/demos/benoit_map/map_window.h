/*
	
	HelloWindow.h
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/

#ifndef HELLO_WINDOW_H
#define HELLO_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

class HelloWindow : public BWindow {

public:
				HelloWindow(BRect frame); 
virtual	bool	QuitRequested();
virtual	void	FrameMoved(BPoint a_point);
virtual void	FrameResized(float new_width, float new_height);

};

#endif
