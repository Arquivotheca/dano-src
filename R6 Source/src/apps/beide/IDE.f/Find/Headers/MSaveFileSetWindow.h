//========================================================================
//	MSaveFileSetWindow.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MSAVEFILESETWINDOW_H
#define _MSAVEFILESETWINDOW_H

#include <Window.h>

class MTextView;
class BPopUpMenu;
class BRadioButton;

class MSaveFileSetWindow : public BWindow
{
public:

								MSaveFileSetWindow(
									BPopUpMenu&	inFileSetMenuBar,
									bool		inHasProject);
								~MSaveFileSetWindow();

		void					MessageReceived(
									BMessage * message);

private:

		MTextView*				fTextBox;
		BButton*				fOKButton;
		BRadioButton*			fSpecific;
		BRadioButton*			fGlobal;
		BPopUpMenu&				fPopup;

		void					BuildWindow(
									bool		inHasProject);
		void					ExtractInfo();
};

#endif
