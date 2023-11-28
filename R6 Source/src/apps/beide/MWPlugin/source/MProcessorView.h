//========================================================================
//	MProcessorView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MPROCESSORVIEW_H
#define _MPROCESSORVIEW_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

class MTextView;
class BCheckBox;
class BPopUpMenu;


class MProcessorView : public MPlugInPrefsView
{
public:
								MProcessorView(
									BRect				inFrame);
		virtual					~MProcessorView();

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

	BCheckBox*					fStringsROnly;
	BCheckBox*					fStaticInTOC;
	BCheckBox*					fFPContractions;
	BCheckBox*					fProfile;
	BCheckBox*					fTraceback;
	BPopUpMenu*					fInstructionScheduling;
	BPopUpMenu*					fSpeedSizePopup;
	BCheckBox*					fPeephole;
	BPopUpMenu*					fOptimizationLevelPopup;
	MTextView*					fInfoView;
	ProcessorPrefs				fOldSettings;
	ProcessorPrefs				fNewSettings;

	void						UpdateCheckBoxes();
	void						UpdatePopups();
	void						SetInfoText();
};

#endif
