//========================================================================
//	MOpenSelectionWindow.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MOPENSELECTIONWINDOW_H
#define _MOPENSELECTIONWINDOW_H

#include <Window.h>

class MTextView;
class MProjectWindow;
class BCheckBox;

class MOpenSelectionWindow : public BWindow
{
public:

							MOpenSelectionWindow(MProjectWindow* inProject);
							~MOpenSelectionWindow();

	void					MessageReceived(BMessage * message);
	virtual	void			WindowActivated(bool state);
	virtual	bool			QuitRequested();

	static	void			RemovePreferences();
	
	void					SetLookupProject(MProjectWindow* inProject)
							{
								fProject = inProject;
							}

private:
	MProjectWindow*			fProject;
	MTextView*				fTextBox;
	BCheckBox*				fSystemSearchCB;
	BButton*				fOKButton;

	void					BuildWindow();
	void					ExtractInfo();
	void					GetPrefs();
	void					SetPrefs();
};

#endif
