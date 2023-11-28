/*
	
	HelloWindow.h
	
*/
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef HELLO_WINDOW_H
#define HELLO_WINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

#include <Button.h>
#include "LifeView.h"

#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <FilePanel.h>

class HelloWindow : public BWindow 
{
public:
				HelloWindow(BRect frame); 
virtual	bool	QuitRequested();
		void	DrawGrid(void);
virtual void	MessageReceived(BMessage* message);
	thread_id	fLifeThread;

private:
		HelloView*	fView;
		BView*		fButtonsView;
		BButton*	fButton;
		BMenuBar*	fMenuBar;
		BMenu		*fFile, *fOptions;
		BMenuItem	*fNew, *fAbout, *fQuit;
		BMenuItem	*fOld, *fWrap;
		BFilePanel	*fOpenPanel, *fSavePanel;
};

#endif //HELLO_WINDOW_H
