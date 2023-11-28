// ---------------------------------------------------------------------------
/*
	MEnvironmentPreferencesWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			12 January 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MENVIRONMENTPREFERENCESWINDOW_H
#define _MENVIRONMENTPREFERENCESWINDOW_H

#include "MPreferencesWindow.h"

class MEnvironmentPreferencesWindow : public MPreferencesWindow
{
public:
								MEnvironmentPreferencesWindow();
	virtual						~MEnvironmentPreferencesWindow();

	virtual	bool				QuitRequested();

protected:
	virtual void				BuildViews();		
	virtual bool				OKToSave(BMessage& outMessage);
	virtual void				GetTargetData(BMessage& outMessage);
	virtual void				SetTargetData(BMessage& message);
};

#endif

