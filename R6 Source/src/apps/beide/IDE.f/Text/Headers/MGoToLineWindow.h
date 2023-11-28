//========================================================================
//	MGoToLineWindow.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MGOTOLINEWINDOW_H
#define _MGOTOLINEWINDOW_H

#include <Window.h>

class MTextView;

class MGoToLineWindow : public BWindow
{
public:

								MGoToLineWindow(
									BWindow& 		inWindow,
									BPoint			inTopLeft);
								~MGoToLineWindow();

virtual	void					MessageReceived(
									BMessage * message);
virtual	void					WindowActivated(bool state);
virtual	bool					QuitRequested();

private:

		BWindow&				fWindow;
		MTextView*				fTextBox;

		void					BuildWindow();
		void					ExtractInfo();
};

#endif
