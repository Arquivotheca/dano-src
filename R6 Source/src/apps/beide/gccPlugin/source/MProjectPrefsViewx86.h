//========================================================================
//	MProjectPrefsViewx86.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MPROJECTPREFSVIEWX86_H
#define _MPROJECTPREFSVIEWX86_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

class MTextControl;
class BPopUpMenu;
class BTextView;

class MProjectPrefsViewx86 : public MPlugInPrefsView
{
public:
								MProjectPrefsViewx86(
									BRect				inFrame);
		virtual					~MProjectPrefsViewx86();

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
	void						SetProjectDefaults();

private:

	BTextView*					fAppNameBox;
	BTextView*					fTypeBox;
	BPopUpMenu*					fProjectKindPopup;
	ProjectPrefsx86				fOldSettings;
	ProjectPrefsx86				fNewSettings;

	void						ExtractInfo();
};

#endif
