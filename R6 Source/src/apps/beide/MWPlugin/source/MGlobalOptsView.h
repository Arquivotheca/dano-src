//========================================================================
//	MGlobalOptsView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MGLOBALOPTSVIEW_H
#define _MGLOBALOPTSVIEW_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

class MTextView;
class BCheckBox;


class MGlobalOptsView : public MPlugInPrefsView
{
public:
								MGlobalOptsView(
									BRect				inFrame);
		virtual					~MGlobalOptsView();

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

	BCheckBox*					fSpaceOptimizationsCB;
	BCheckBox*					fCSECB;
	BCheckBox*					fCopyPropagationCB;
	BCheckBox*					fStrenghReduction;
	BCheckBox*					fLifetimeAnalysisCB;
	BCheckBox*					fSpeedOptimizationsCB;
	BCheckBox*					fLoopInvariantsCB;
	BCheckBox*					fDeadStoreEliminationCB;
	BCheckBox*					fDeadCodeEliminationCB;
	GlobalOptimizations			fOldSettings;
	GlobalOptimizations			fNewSettings;

	void						UpdateCheckBoxes();
	void						UpdatePopups();
	void						SetInfoText();
};

#endif
