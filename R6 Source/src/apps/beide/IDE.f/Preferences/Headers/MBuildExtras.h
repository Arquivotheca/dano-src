//========================================================================
//	MBuildExtras.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MBUILDEXTRAS_H
#define _MBUILDEXTRAS_H

#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MPrefsStruct.h"

class MTextControl;
class BPopUpMenu;
class BCheckBox;


class MBuildExtras : public MPreferencesView
{
public:
								MBuildExtras(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MBuildExtras();

		void					MessageReceived(
									BMessage * inMessage);

	virtual void				GetData(
									BMessage&	inOutMessage,
									bool		isProxy);
	virtual void				SetData(
									BMessage&	inOutMessage);
	virtual	void				AttachedToWindow();


virtual const char *			Title();
virtual TargetT					Targets();

private:

	BPopUpMenu*					fCompileCountPopup;
	BPopUpMenu*					fBuildPriorityPopup;
	BCheckBox*					fStopOnErrorsBox;
	BCheckBox*					fOpenErrorWindowBox;
	
	BuildExtrasPrefs			fOldSettings;
	BuildExtrasPrefs			fNewSettings;

	virtual void				DoFactorySettings();

	void						ExtractInfo();
	void						UpdateValues();
};

#endif
