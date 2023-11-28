//========================================================================
//	MLinkerViewx86.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#pragma once

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

//class MTextView;


class MLinkerViewx86 : public MPlugInPrefsView
{
public:
								MLinkerViewx86(
									BRect				inFrame);
		virtual					~MLinkerViewx86();

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

	BCheckBox*					fGenSymCB;
	BCheckBox*					fFulPathCB;
	BCheckBox*					fLinkMapCB;
	BCheckBox*					fSuppressWarningsCB;
	BCheckBox*					fGenCVCB;
	BTextView*					fMainBox;
	BTextView*					fCommandFileBox;
	LinkerPrefsx86				fOldSettings;
	LinkerPrefsx86				fNewSettings;

	void						UpdateCheckBoxes();
	void						UpdateTextBoxes();
};
