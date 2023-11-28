// ---------------------------------------------------------------------------
/*
	MProjectSettingsWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			12 January 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MPROJECTSETTINGSWINDOW_H
#define _MPROJECTSETTINGSWINDOW_H

#include "MPreferencesWindow.h"

class MTargetView;
class MProjectWindow;

class MProjectSettingsWindow : public MPreferencesWindow
{
public:
								MProjectSettingsWindow(const char* title, 
													   MProjectWindow& project);
	virtual						~MProjectSettingsWindow();

	virtual	void				MessageReceived(BMessage* inMessage);
	virtual bool				CanChangeSettings() const;
	
protected:
	virtual void				BuildViews();		
	virtual bool				OKToSave(BMessage& outMessage);
	virtual void				GetTargetData(BMessage& outMessage);
	virtual void				SetTargetData(BMessage& message);

private:
	void						InitializeTargetView();
	void						FilterViewList(const char* inLinkerName);

private:
	MTargetView*				fTargetView;
	MProjectWindow&				fProject;

};

#endif
