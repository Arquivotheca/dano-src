//========================================================================
//	MSyntaxStylePrefsView.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MSYNTAXSTYLEPREFSVIEW_H
#define _MSYNTAXSTYLEPREFSVIEW_H

#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MPrefsStruct.h"

class BTextView;
class BPopUpMenu;
class BCheckBox;
class BColorControl;
class MSyntaxStyleView;
class MTextView;
class MBoxControl;


class MSyntaxStylePrefsView : public MPreferencesView
{
public:
								MSyntaxStylePrefsView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MSyntaxStylePrefsView();

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

	MBoxControl*			fStyleBox;
	MSyntaxStyleView*		fFocusStyleView;
	MSyntaxStyleView*		fTextStyleView;
	MSyntaxStyleView*		fCommentsStyleView;
	MSyntaxStyleView*		fKeyWordsStyleView;
	MSyntaxStyleView*		fStringsStyleView;

	BTextView*				fSampleText;
	BPopUpMenu*				fFontPopup;
	BPopUpMenu*				fSizePopup;
	BCheckBox*				fUseSyntaxStylingCB;
	BColorControl*			fColorMap;
	SyntaxStylePrefs		fOldSettings;
	SyntaxStylePrefs		fNewSettings;

	virtual void				DoFactorySettings();
	virtual void				DoRevert();

	void						UpdateValues();

	void						FontChanged(
									BMessage&	inMessage);
	void						SizeChanged(
									BMessage&	inMessage);
	void						SyntaxViewChanged(
									BMessage&	inMessage);
	void						ColorChanged();
	void						DataChanged();
};

#endif
