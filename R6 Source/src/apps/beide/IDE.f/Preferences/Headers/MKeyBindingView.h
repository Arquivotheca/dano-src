//========================================================================
//	MKeyBindingView.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MKEYBINDINGVIEW_H
#define _MKEYBINDINGVIEW_H

#include "MPreferencesWindow.h"
#include "MPreferencesView.h"
#include "MPrefsStruct.h"
#include "MKeyBindingManager.h"

class String;
class MTextControl;
class MKeyBindingsListView;
class MBindingWindow;
class MKeyBindingManager;
class BTextView;


class MKeyBindingView : public MPreferencesView
{
public:
								MKeyBindingView(
									MPreferencesWindow&	inWindow,
									BRect				inFrame);
		virtual					~MKeyBindingView();

		void					MessageReceived(
									BMessage * inMessage);

	virtual void				GetData(
									BMessage&	inOutMessage,
									bool		isProxy);
	virtual void				SetData(
									BMessage&	inOutMessage);
	virtual	void				AttachedToWindow();

	virtual const char *		Title();
	virtual TargetT				Targets();
	virtual	bool				ProjectRequiresUpdate(
									UpdateType inType);
	virtual void				LastCall();
	virtual void				UpdateValues();
	virtual void				ValueChanged();
	virtual void				DoSave();
	virtual void				DoRevert();

private:

	MKeyBindingsListView*		fListView;
	BTextView*					fTimeoutBox;
	MTextControl*				fTextControl;
	MBindingWindow*				fBindingWindow;
	MKeyBindingManager*			fNewBindingManager;
	MKeyBindingManager*			fOldBindingManager;
	BFilePanel*					fImportPanel;
	BFilePanel*					fExportPanel;

	virtual void				DoFactorySettings();
	void						OpenBindingWindow();
	void						CloseBindingWindow();
	void						UpdateBinding(
									KeyBindingContext			inContext,
									const KeyBindingInfo&		inInfo);
	void						StartImport();
	void						StartExport();
	void						ClosePanels();
	void						DoExport(
									BMessage*	inMessage);
	void						DoImport(
									BMessage*	inMessage);
};

#endif
