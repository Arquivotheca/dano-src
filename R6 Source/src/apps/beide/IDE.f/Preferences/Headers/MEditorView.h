//========================================================================
//	MEditorView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MEDITORVIEW_H
#define _MEDITORVIEW_H

#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MPrefsStruct.h"

class MTextControl;
class BRadioButton;
class BCheckBox;
class BPopUpMenu;

class MEditorView : public MPreferencesView
{
public:
								MEditorView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MEditorView();

		void					MessageReceived(
									BMessage * inMessage);

	virtual void				GetData(
									BMessage&	inOutMessage,
									bool		isProxy);
	virtual void				SetData(
									BMessage&	inOutMessage);
	virtual	void				AttachedToWindow();


virtual const char *			Title();
virtual TargetT					Targets();

private:

	BRadioButton*				fUseDocSettings;
	BRadioButton*				fUseAppSettings;
	BCheckBox*					fUseExternalEditorSettings;
	EditorPrefs					fOldSettings;
	EditorPrefs					fNewSettings;
	
	// A list of signatures of the apps that can be used for text/x-source-code
	// The first and second entries are NULL, so that we can always match the
	// offsets in fEditorAppMenu (0 = "Preferred Application", 1 = separator)
	enum { kFirstEditorIndex = 2 };
	BList						fEditorSignatureList;
	BPopUpMenu*					fEditorAppMenu;

	virtual void				DoFactorySettings();

	void						ExtractInfo();
	void						UpdateValues();

	void						BuildEditorAppMenu();
	void						DisableExclusiveSettings();
	void						EmptyAppList();
};

#endif
