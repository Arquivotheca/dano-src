//========================================================================
//	MFontView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MFONTVIEW_H
#define _MFONTVIEW_H

#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MPrefsStruct.h"

class MTextView;
class MTextControl;
class BTextView;
class BCheckBox;
class BPopUpMenu;


class MFontView : public MPreferencesView
{
public:
								MFontView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MFontView();

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

	BTextView*				fSampleText;
	BTextView*				fTabText;
	MTextControl*			fTabTextControl;
	BCheckBox*				fAutoIndentCB;
	BPopUpMenu*				fFontPopup;
	BPopUpMenu*				fSizePopup;
	FontPrefs				fOldSettings;
	FontPrefs				fNewSettings;

	virtual void				DoFactorySettings();

	void						UpdateValues();

	void						FontChanged(
									BMessage&	inMessage);
	void						SizeChanged(
									BMessage&	inMessage);
	void						DataChanged();
};

#endif
