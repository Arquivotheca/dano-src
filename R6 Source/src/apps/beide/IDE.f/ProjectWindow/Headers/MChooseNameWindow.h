//========================================================================
//	MChooseNameWindow.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MCHOOSENAMEWINDOW_H
#define _MCHOOSENAMEWINDOW_H

#include <Window.h>

class MSectionLine;
class MTextView;
class BTextView;


class MChooseNameWindow : public BWindow
{
public:

								MChooseNameWindow(
									BWindow& 		inWindow, 
									MSectionLine&	inSection,
									const char* 	inText);
								~MChooseNameWindow();

virtual void					MessageReceived(
									BMessage * message);

private:

		BWindow&				fWindow;
		MSectionLine&			fSection;
		BTextView*				fTextBox;

		void					BuildWindow(
									const char* inText);
		void					ExtractInfo();
};

#endif
