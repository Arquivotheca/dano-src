//*****************************************************************************
//
//	File:		 DomWindow.h
//
//	Description: WindowScreen class playing dominos, headers.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

#ifndef __DomWindow
#define __DomWindow

#ifndef _WINDOW_SCREEN_H
#include <WindowScreen.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

class DomWindow : public BWindowScreen {
public:
// This view is used only to access the keyboard (GetKeys()).
BView               *view;
	                DomWindow(long space, char *name, status_t *error);
virtual void        Quit();
virtual	void		ScreenConnected(bool active);
virtual	void	MessageReceived(BMessage *message);
// Domino player thread.
thread_id           draw;
};

#endif



