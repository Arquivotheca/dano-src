//========================================================================
//	MLinkerView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MLINKERVIEW_H
#define _MLINKERVIEW_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

//class MTextView;
class BCheckBox;
class BTextView;


class MLinkerView : public MPlugInPrefsView
{
public:
								MLinkerView(
									BRect				inFrame);
		virtual					~MLinkerView();

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
	BCheckBox*					fDeadSTripCB;
	BTextView*					fInitBox;
	BTextView*					fMainBox;
	BTextView*					fTermBox;
	LinkerPrefs					fOldSettings;
	LinkerPrefs					fNewSettings;

	void						UpdateCheckBoxes();
	void						UpdateTextBoxes();
};

#endif
