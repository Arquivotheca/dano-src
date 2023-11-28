/*************************************************************************
/
/	MinitelWindow.h
/
/	Written by Robert Polic
/
/	Based on Loritel
/
/	Copyright 1999, Be Incorporated.   All Rights Reserved.
/
*************************************************************************/


#ifndef MINITEL_WINDOW
#define MINITEL_WINDOW

#include <Window.h>


//========================================================================

class MinitelWindow : public BWindow
{
	public:
							MinitelWindow		(BRect);
		void				MessageReceived		(BMessage*);
		bool				QuitRequested		();
};
#endif
