// --------------------------------------------------------------------------- 
/* 
	Run Preferences View
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			March 31, 2001 
 
	Options for the application running from the Run menu.
*/ 
// --------------------------------------------------------------------------- 

#ifndef MRUNPREFSVIEW_H
#define MRUNPREFSVIEW_H

#include "MPrefsStruct.h"

#include "MPreferencesView.h"

class BCheckBox;
class BMenuField;
class BTextControl;

class MRunPrefsView : public MPreferencesView
{
	public:
									MRunPrefsView(
										MPreferencesWindow&	inWindow,
										BRect				inFrame);
		virtual						~MRunPrefsView();	
										
		virtual void				AttachedToWindow();
		virtual void				MessageReceived(BMessage* message);
											
		virtual void				LastCall();
		virtual void				DoSave();
		virtual void				DoRevert();
		virtual void				DoFactorySettings();
		virtual	bool				ProjectRequiresUpdate(
										UpdateType inType);
	
		virtual void				GetData(
										BMessage&	inOutMessage,
										bool		isProxy);
		virtual void				SetData(
										BMessage&	inOutMessage);
	
		virtual const char *		Title();
		virtual TargetT				Targets();
		virtual MakeActionT			Actions();
	
		virtual void				ValueChanged();
	
		virtual	void				SetDirty(
										bool inDirty = true);
		virtual	bool				IsDirty();
	
	protected:
	
		virtual void				UpdateValues();

	private:
		BMenuField*					fMallocDebugLevel;
		BTextControl*				fCustomMallocDLevel;
		BTextControl*				fCommandLineArgs;
		BCheckBox*					fRunInTerminal;
		RunPreferences				fOld;
		RunPreferences				fNew;
};

#endif

