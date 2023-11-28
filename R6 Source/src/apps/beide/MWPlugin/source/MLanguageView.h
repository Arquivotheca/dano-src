//========================================================================
//	MLanguageView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MLANGUAGEVIEW_H
#define _MLANGUAGEVIEW_H

#include "MPlugInPrefsView.h"
#include "MPrefsStruct.h"

//class String;
//class MTextView;
class BTextView;
class BPopUpMenu;
class BCheckBox;


class MLanguageView : public MPlugInPrefsView
{
public:
								MLanguageView(
									BRect				inFrame);
		virtual					~MLanguageView();

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

	BTextView*					fPrefixBox;
	BCheckBox*					fActivateCpp;
	BCheckBox*					fEnableExceptions;
	BCheckBox*					fEnableRTTI;
	BCheckBox*					fRequireProtos;
	BCheckBox*					fPoolStrings;
	BCheckBox*					fDontReuseStrings;
	BCheckBox*					fANSIStrict;
	BCheckBox*					fANSIKeywords;
	BCheckBox*					fExpandTrigraphs;
	BCheckBox*					fEnumsRInts;
	BCheckBox*					fAutoInline;
	BCheckBox*					fEnableBool;
	BCheckBox*					fUnsignedChars;
	BCheckBox*					fRelaxedPointers;
	BPopUpMenu*					fInlinePopup;
	LanguagePrefs				fNewSettings;
	LanguagePrefs				fOldSettings;

	void						UpdateCheckBoxes();
	void						UpdatePopups();
};

#endif
