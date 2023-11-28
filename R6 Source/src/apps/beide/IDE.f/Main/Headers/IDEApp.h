//========================================================================
//	IDEApp.h
//	Copyright 1995-98  Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Application subclass for the IDE application.

#ifndef _IDEAPP_H
#define _IDEAPP_H

#include "MDynamicMenuHandler.h"
#include "MPrefsStruct.h"
#include "MFileUtils.h"
#include "ScriptHandler.h"
#include "MList.h"

class MMessageWindow;
class MFindWindow;
class MPreferencesWindow;
class MProjectWindow;
class MTextWindow;
class MOpenSelectionWindow;
class MNewProjectWindow;
class MLookupDocumentationWindow;
class MLookupDocumentationTask;
class AboutBox;
class MBuildersKeeper;

enum FilePanelStatus
{
	opNothing,
	opOpeningFiles,
	opAddingFiles,
	opAddingOthersToFind
};


class IDEApp : 	public BApplication,
				public ScriptHandler
{
friend class MOpenSelectionTask;		// access to openselectionasync
friend class MAndyFeatureTask;			// access to AndyFeatureAsync
friend class MLookupDocumentationTask;
public:
								IDEApp();
								~IDEApp();

virtual	void					MessageReceived(
									BMessage * message);

virtual	void					RefsReceived(
									BMessage *a_message);
virtual	void					ArgvReceived(
									int32 	argc, 
									char **	argv);
virtual	bool					QuitRequested();
virtual	void					AboutRequested();
virtual	void					AppActivated(bool active);

virtual	void					ReadyToRun();

		void					GetData(
									BMessage&	inOutMessage);
		void					SetData(
									BMessage&	inOutMessage);

		void					ProjectClosed(MProjectWindow* project);

		bool					FilePanelIsRunning();
		void					CloseFilePanel();
		void					ShowFilePanel(
									const char *	inTitle,
									uint32			inMessage = 0,
									const char *	inCancelButtonLabel = 0,
									const char *	inDefaultButtonLabel = 0,
									bool			inDirectorySelection = false);

		status_t				PageSetup(
									BPrintJob&	inPrintJob,
									bool alwaysShowDialog = true);
		status_t				PrintSetup(
									BPrintJob&	inPrintJob);

		void					PrinterChanged();

		bool					IsQuitting()
								{
									return fQuitting;
								}

	static IDEApp&				BeAPP()
								{
									return (IDEApp&) *be_app;
								}

	static MProjectWindow*		GetCurrentProject()
								{
									return ((IDEApp*)be_app)->CurrentProject();
								}
	
private:

		MProjectWindow*			fCurrentProject;
		MList<MProjectWindow*>	fProjectList;
		BLocker					fProjectListLock;
		MFindWindow*			fFindWindow;
		
		MPreferencesWindow*			fPreferencesWindow;
		MOpenSelectionWindow*		fOpenSelectionWind;
		MNewProjectWindow*			fNewProjectWindow;
		MLookupDocumentationWindow*	fLookupDocumentationWind;
		MLookupDocumentationTask*	fLookupDocumentationTask;
		
		BView*					fAccessPathsView;
		AboutBox*				fAbout;
		BMessage*				fPrintSettings;
		int32					fUntitledCount;

		BFilePanel*				fFilePanel;
		BFilePanel*				fNewProjectPanel;
		FilePanelStatus			fFilePanelStatus;
		entry_ref				fLastDirectory;
		BRect					fFilePanelFrame;

		// delay building this until we have our message window up
		MBuildersKeeper*		fBuilderKeeper;

		RunPreferences			fRunPrefs;
		EditorPrefs				fEditorPrefs;
		FontPrefs				fFontPrefs;
		AppEditorPrefs			fAppEditorPrefs;
		PrivateProjectPrefs		fPrivatePrefs;
		BuildExtrasPrefs		fBuildExtrasPrefs;
		SyntaxStylePrefs		fSyntaxStylePrefs;
		AccessPathsPrefs		fAccessPathsPrefs;
		TargetPrefs				fTargetPrefs;
		AccessPathList			fProjectPathList;		// project path records
		AccessPathList			fSystemPathList;		// system path records
		DirectoryList			fSystemDirectories;		// system directories
		BLocker					fPrefsLock;

		MDynamicMenuHandler		fWindowMenuObject;
		MFileUtils				fFileUtilsObject;

		bool					fLastDirectoryGood;
		bool					fQuitting;
		bool					fNewProjectPanelDoingWork;
		
		void					DoCmdNew();
		void					DoCmdNewProject();
		void					DoCmdOpen();
		void					DoAddFiles();
		bool					CloseAllProjects();
		bool					ClosePersistentWindows();

		void					OpenSourceFile(BMessage* message);
		MTextWindow*			OpenTextFile(
									BEntry*	inFile,
									bool inIsInProject = false);
		MProjectWindow*			OpenProjectFile(const entry_ref& inEntry);
		void					OpenProjectAndReply(BMessage& message);

		void					DoPreferences();
		void					OpenRawTextWindow(
									BMessage& 	inMessage);

		void					HandleFindDocumentation(BMessage& inMessage);
		
		void					SetTextFileFont(
									MTextWindow*	inWindow,
									bool			inNewFile);

		void					HandleFileSaved(
									BMessage& inMessage);
		void					AddAccessPaths(
									BMessage& inMessage);
		void					AddOtherFiles();
		void					BuildEmptyProject(
									BMessage*	inMessage);	
		void					GenerateDefaultAccessPaths();

		void					GetNewFileName(
									String& ioFileName);
		void					OpenRef(
									const entry_ref &	inRef,
									BMessage * 			inMessage = nil);
		void					GoToLineInWindow(
									BWindow*	inWindow,
									BMessage *	inMessage);
		void					SaveLastDirectory(
									const entry_ref&		inEntry);

		void					DoOpenSelection();
		void					HandleOpenSelection(
									BMessage	&inMessage);
		bool					OpenSelection(
									const char *	 	inName,
									bool				inSystemTree,
									const entry_ref&	inRef,
									MProjectWindow*		inProject,
									bool				inSyncronous = true,
									bool				inSearchMemory = true,
									bool				inSearchDisk = true);
		void					OpenSelectionAsync(
									const char * 		inName,
									bool				inSystemTree,
									const entry_ref&	inRef,
									MProjectWindow*		inProject);

		void					ShowOpenSelectionWindow(MProjectWindow* inProject);
		void					ShowLookupDocumentationWindow();
		void					FindDocumentationDone();

		void					HandleAndyFeature(
									BMessage	&inMessage);
		void					AndyFeatureAsync(
									const char *	fileName,
									const char **	suffixArray,
									bool			isSourceFile,
									entry_ref&		ref,
									MProjectWindow*	inProject);

		void					RemoveAllPreferences();
		void					RemovePreferences();

		void					GetPrintSettings();
		void					SetPrintSettings();

		void					HandleScript(
									BMessage*	inMessage);
virtual	ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);

		void					UpdateProjectPrefs();
		void					UpdatePrefs();
		void					GetPrefs();
		void					SetPrefs();
		void					LockPrefs()
								{
									fPrefsLock.Lock();
								}
		void					UnlockPrefs()
								{
									fPrefsLock.Unlock();
								}


		void					RemoveProject(MProjectWindow* project);
		void					ProjectActivated(MProjectWindow* project);
		void					NewCurrentProject(MProjectWindow* newProject);
		MProjectWindow*			CurrentProject();
		MProjectWindow*			GetBackmostProject();
		MProjectWindow*			IsOpenProject(const entry_ref& inRef);
		void					MessageToAllProjectWindows(BMessage& inMessage);


};

#endif
