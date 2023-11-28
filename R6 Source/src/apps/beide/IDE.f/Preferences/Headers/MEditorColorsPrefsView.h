//========================================================================
//	MEditorColorsPrefsView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MEDITORCOLORSPREFSVIEW_H
#define _MEDITORCOLORSPREFSVIEW_H


//#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MPrefsStruct.h"

class BCheckBox;
class BTextView;
class BColorControl;
class MPreferencesWindow;
class MBoxControl;
class MEditorColorView;
class MTextControl;


class MEditorColorsPrefsView : public MPreferencesView
{
public:
								MEditorColorsPrefsView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MEditorColorsPrefsView();

		void					MessageReceived(
									BMessage * inMessage);

	virtual	void				AttachedToWindow();

	virtual void				GetData(
									BMessage&	inOutMessage,
									bool		isProxy);
	virtual void				SetData(
									BMessage&	inOutMessage);

	virtual const char *		Title();
	virtual TargetT				Targets();

private:

	MBoxControl*				fBoxControl;
	BCheckBox*					fBalanceCB;
	BCheckBox*					fRelaxedCB;
//	BCheckBox*					fUseMultiUndoCB;
	BCheckBox*					fSortFunctionCB;
	BCheckBox*					fSelectionCB;
	MTextControl*				fDelayBoxControl;
	BTextView*					fDelayBox;
	MEditorColorView*			fHiliteColorView;
	MEditorColorView*			fBackgroundView;
	MEditorColorView*			fFocusColorView;

	BColorControl*				fColorMap;
	AppEditorPrefs				fOldSettings;
	AppEditorPrefs				fNewSettings;

	virtual void				DoFactorySettings();
	virtual void				DoRevert();

	void						UpdateValues();

	void						ColorViewChanged(
									BMessage&	inMessage);
	void						ColorChanged();
	void						DataChanged();
	void						ExtractInfo();
};

#endif
