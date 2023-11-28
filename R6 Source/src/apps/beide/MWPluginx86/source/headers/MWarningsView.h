//========================================================================
//	MWarningsView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#pragma once

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"



class MWarningsView : public MPlugInPrefsView
{
public:
								MWarningsView(
									BRect				inFrame);
		virtual					~MWarningsView();

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

	BCheckBox*					fWarningsAreErrors;
	BCheckBox*					fIllegalPragmas;
	BCheckBox*					fEmptyDeclarations;
	BCheckBox*					fPossibleErrors;
	BCheckBox*					fUnusedVariables;
	BCheckBox*					fUnusedArguments;
	BCheckBox*					fExtraCommas;
	BCheckBox*					fExtendedChecking;
	BCheckBox*					fHiddenVirtuals;
	BCheckBox*					fLargeArgs;
	WarningsPrefs				fOldSettings;
	WarningsPrefs				fNewSettings;


	void						UpdateCheckBoxes();
};