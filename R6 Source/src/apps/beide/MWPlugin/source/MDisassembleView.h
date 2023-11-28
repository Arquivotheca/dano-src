//========================================================================
//	MDisassembleView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MDISASSEMBLEVIEW_H
#define _MDISASSEMBLEVIEW_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

class BCheckBox;

class MDisassembleView : public MPlugInPrefsView
{
public:
								MDisassembleView(
									BRect				inFrame);
		virtual					~MDisassembleView();

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

	BCheckBox*					fShowCodeCB;
	BCheckBox*					fUseExtendedCB;
	BCheckBox*					fShowSourceCB;
	BCheckBox*					fOnlyOperandsCB;
	BCheckBox*					fShowDataCB;
	BCheckBox*					fExceptionTableCB;
	BCheckBox*					fShowSYMCB;
	BCheckBox*					fShowNameTableCB;


	DisassemblePrefs			fOldSettings;
	DisassemblePrefs			fNewSettings;

	void						UpdateCheckBoxes();
};

#endif
