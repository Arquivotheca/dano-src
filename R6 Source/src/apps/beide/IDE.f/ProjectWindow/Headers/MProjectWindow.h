//========================================================================
//	MProjectWindow.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPROJECTWINDOW_H
#define _MPROJECTWINDOW_H

#include "MProjectView.h"
#include "ScriptHandler.h"
#include "MScripting.h"

class MProjectView;
class MBlockFile;
class MPopupMenu;
class MSourceFileList;
class MPreferencesWindow;

enum SavingType
{
	sNotSaving,
	sSaving,
	sSavingAs
};

const uint32 sProjectVerbs[] = {
	kOpenVerb,
	kSaveVerb,
	kCloseVerb,
	kSetVerb,
	kGetVerb,
//	kMakeVerb,
	kCreateVerb,
	kDeleteVerb,
	kCountVerb,
	kMakeProject,
	0
};


class MProjectWindow : 	public BWindow,
						public ScriptHandler
{
public:
								MProjectWindow(
									const char * title); 
								MProjectWindow(
									const entry_ref&	inRef,
									bool				inEmptyProject = false);
								~MProjectWindow();

virtual	bool					QuitRequested();
virtual	void					WindowActivated(
									bool inActive);

virtual	void					MessageReceived(
									BMessage * message);
virtual void					Zoom(	
									BPoint 	rec_position,
									float 	rec_width,
									float 	rec_height);
virtual	void					MenusBeginning();
								
		void					NewProjectCreated();
		void					SetBuildParameters(const BuildExtrasPrefs& buildparams);
		void					SetBuildParameters(const BMessage& inMessage);
		
		void					DoSave(
									const char * inName	 = NULL);
		void					SaveRequested(
									entry_ref*	directory, 
									const char*	name);

		void					ShowProjectWindow(BMessage* inMessage);

		void					UpdateRunMenuItems(
									bool	inRunWithDebugger);
		bool					RunsWithDebugger();

		bool					FileIsInProjectByFile(
									BEntry* 		inFile) const;
		bool					FileIsProjectFile(
									const entry_ref &inRef) const;
		bool					FileIsProjectFile(
									const BEntry & inRef) const;

		void					GetData(
									BMessage&	inOutMessage);
		void					SetData(
									BMessage&	inOutMessage);

		bool					OpenSelection(
									const char * 	inFileName,
									bool			inSystemTree,
									entry_ref&		outRef,
									bool			inSearchInMemory = true,
									bool			inSearchOnDisk = true);

		void					BuildPopupMenu(
									const char *	inName,
									MPopupMenu& 	inPopup);
		void					FillFileList(
									MSourceFileList&	inList, 
									SourceListT			inKind);

		status_t				GetRef(
									entry_ref&	inoutRef);

		BDirectory&				GetProjectDirectory()
								{
									return fProjectView->GetProjectDirectory();
								}
		MProjectView&			ProjectView()
								{
									return *fProjectView;
								}
		bool					OKToModifyProject()
								{
									return fProjectView->OKToModifyProject();
								}
private:

		MBlockFile*				fFile;
		MProjectView*			fProjectView;
		BStringView*			fFileCountCaption;
		BStringView*			fStatusCaption;
		BMenu*					fWindowMenu;
		BMenuBar*				fMenuBar;
		BFilePanel*				fSavePanel;
		SavingType				fSavingStatus;
		MPreferencesWindow*		fSettingsWindow;
		BRect					fUserRect;
		bool					fDirty;
		bool					fUserState;

		void					GetPrefs();
		void					SetPrefs();

		void					InitWindow();
		void					BuildWindow();
		void					BuildProject();
		void					DoAndyFeature();

		bool					SavePanelIsRunning();
		void					ShowSavePanel(
									const char *	inSaveText);
		void					CloseSavePanel();
		
		void					DoProjectSettings();

		uint32					CurrentRunMenuCommand();

		void					SetCaptionText(
									BMessage&		inMessage);
		void					SetStatusText(
									BMessage&		inMessage);
		void					SetInfoText(
									BMessage&		inMessage, 
									BStringView*	captionWindow);

		void					BuildProjectAndReply(BMessage& message);

virtual	ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);
virtual	status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);
};

#endif
