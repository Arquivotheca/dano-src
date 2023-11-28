//========================================================================
//	MFindDefinitionWindow.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MFINDDEFINITIONWINDOW_H
#define _MFINDDEFINITIONWINDOW_H

#include <Window.h>

class MTextView;
class MProjectWindow;


class MFindDefinitionWindow : public BWindow
{
public:

								MFindDefinitionWindow(
									MProjectWindow&	inProjectView);
								~MFindDefinitionWindow();

virtual	void					MessageReceived(
									BMessage * message);
virtual	bool					QuitRequested();

private:

		MTextView*				fTextBox;
		BButton*				fOKButton;
		MProjectWindow&			fProject;

		void					BuildWindow();
		void					ExtractInfo();
};

#endif
