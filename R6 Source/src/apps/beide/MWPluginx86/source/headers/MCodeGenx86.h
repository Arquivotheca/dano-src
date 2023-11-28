//========================================================================
//	MCodeGenx86.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#pragma once

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

class MTextView;


class MCodeGenx86 : public MPlugInPrefsView
{
public:
								MCodeGenx86(
									BRect				inFrame);
		virtual					~MCodeGenx86();

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

	BCheckBox*					fPeepCB;
	BCheckBox*					fMachineCodeCB;
	BCheckBox*					fRegColoringCB;
	BCheckBox*					fInlineIntrinsicsCB;
//	BPopUpMenu*					fAlignmentPopup;
	BCheckBox*					fPeephole;
	CodeGenx86					fOldSettings;
	CodeGenx86					fNewSettings;

	void						UpdateCheckBoxes();
	void						UpdatePopups();
	void						SetInfoText();
};