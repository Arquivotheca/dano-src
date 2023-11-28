//========================================================================
//	MPreferencesWindow.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	The Preferences window.
//	BDS

#ifndef _MPREFERENCESWINDOW_H
#define _MPREFERENCESWINDOW_H

#include "MList.h"
#include "CString.h"

#include <Window.h>

class MPreferencesView;
class MPrefsListView;
class PrefsRowData;

class MPreferencesWindow : public BWindow
{
public:
								MPreferencesWindow(const char* title);
	virtual						~MPreferencesWindow();

	virtual	void				MessageReceived(BMessage* inMessage);
	virtual	void				WindowActivated(bool inActive);
	virtual	bool				QuitRequested();
	virtual	void				Show();
	virtual bool				OKToQuit();

	void						ShowAndActivate();
	virtual bool				CanChangeSettings() const;
	static void					RemovePreferences();

protected:
	MList<PrefsRowData*>		fDataList;
	MPrefsListView*				fListView;

private:
	BButton*					fFactoryButton;
	BButton*					fRevertButton;
	BButton*					fCancelButton;
	BButton*					fSaveButton;

	BView*						fTopView;
	MPreferencesView*			fCurrentView;
	MList<MPreferencesView*>	fViewList;

	int32						fListViewIndex;
	bool						fWindowIsBuilt;

protected:
	virtual void				BuildViews() = 0;		
	virtual bool				OKToSave(BMessage& outMessage) = 0;
	virtual void				GetTargetData(BMessage& outMessage) = 0;
	virtual void				SetTargetData(BMessage& message) = 0;
	
	
	void						VerifyWindowIsBuilt();
							
	void						ShowView(MPreferencesView* inView);
	void						AddView(MPreferencesView* inView, void* cookie);
	void						AddView(MPreferencesView* inView, void* cookie,
										int32 inIndex, const char* inTitle);

	void						AddPreferenceCategory(const char* title);
	
	int32						IndexOf(const char*	inTitle);
	int32						VisibleIndexOf(const char*	inTitle);

	void						DoSave();
	void						ChangeView();
	void						PrefsViewModified();
	bool						AttemptCloseView();
	
	void						GetDataForCurrentView();	
	MPreferencesView*			GetCurrentView() const 		{ return fCurrentView; }
	
	static int32				fgLeftMargin;
	static int32				fgTopMargin;
	static int32				fgRightMargin;
	static int32				fgBottomMargin;

private:
	void						BuildWindow();

};

#endif

