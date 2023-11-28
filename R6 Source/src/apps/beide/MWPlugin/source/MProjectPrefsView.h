//========================================================================
//	MProjectPrefsView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MPROJECTPREFSVIEW_H
#define _MPROJECTPREFSVIEW_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

class BTextView;
class BPopUpMenu;

class MProjectPrefsView : public MPlugInPrefsView
{
public:
								MProjectPrefsView(
									BRect				inFrame);
		virtual					~MProjectPrefsView();

		void					MessageReceived(
									BMessage * inMessage);
	virtual	void				AttachedToWindow();

	virtual const char *		Title();
	virtual TargetT				Targets();
	virtual void				GetData(
									BMessage&	inOutMessage);
	virtual void				SetData(
									BMessage&	inOutMessage);
	virtual void				GetPointers(
									void*&	outOld,
									void*&	outNew,
									long&	outLength);
	virtual void				DoSave();
	virtual void				DoRevert();
	virtual void				DoFactorySettings();
	void						UpdateValues();
	virtual void				ValueChanged();
	virtual	bool				FilterKeyDown(
									ulong aKey);	
	virtual	bool				ProjectRequiresUpdate(
									UpdateType inType);

private:

	BTextView*					fAppNameBox;
	BTextView*					fTypeBox;
	BPopUpMenu*					fProjectKindPopup;
	ProjectPrefs				fOldSettings;
	ProjectPrefs				fNewSettings;

	void						ExtractInfo();
};

#endif
