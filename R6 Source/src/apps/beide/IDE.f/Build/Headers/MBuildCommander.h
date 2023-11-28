//========================================================================
//	MBuildCommander.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MBUILDCOMMANDER_H
#define _MBUILDCOMMANDER_H


#include "MProjectView.h"
#include "MBlockFile.h"
#include "MCompileState.h"
#include "MTargetTypes.h"
#include "MPrefsContainer.h"
#include "MCompileGenerator.h"

class MSourceFileLine;
class MCompile;
class MDeferredScriptHandler;
class MPlugInLinker;

const uint32 DONT_LAUNCH = 0;


class MBuildCommander
{
	friend class MStartBuildThread;		// needs access to MakeStepOne
public:
								MBuildCommander(	
 									MProjectView&		inProjectView,
									MProjectLineList&	inFileList,
									BLocker&			inFileListLocker);
								~MBuildCommander();

		void					InitCommander(
									const TargetRec*	inRecArray,
									int32				inRecCount);

		ProjectTargetRec*		GetTargetRec(
									const char*		inMimeType,
									const char*		inExtension);
		ProjectTargetRec*		GetTargetRec(
									const BEntry&			inFile);

		void					GetToolPath(
									ProjectTargetRec*	inRec,
									char*				outPath,
									MakeStageT			inStage,
									MakeActionT			inAction);
		void					GetToolPath(
									MPlugInBuilder*		inBuilder,
									char*				outPath,
									MakeStageT			inStage,
									MakeActionT			inAction);

		void					BuildAction(
									uint32 inCommand);
		void					BuildAction(
									BMessage& 	inMessage);
		status_t				PerformScriptAction(
									BMessage *				message,
									BMessage * &			reply,
									bool&					wasDeferred);

		void					GetData(
									BMessage&	inOutMessage);
		void					SetData(
									BMessage&	inOutMessage);
		void					ReadFromFile(
									MBlockFile & inFile);
		void					WriteToFile(
									MBlockFile & inFile);
		void					ValidateGenericData();

		void					InitializeData(
									BMessage&	inOutMessage);
		void					FillMessage(
									BMessage&		inMessage,
									uint32			inType);

		void					ShowCompileStatus();
		void					ProjectChanged();
		void					RunMenuItemChanged(
									MProject& inProject);
		void					CompileDone();
		void					LinkDone();
		void					OneCompileDone();
		void					PreCompileDone();
		void					BuildActionDone();
		status_t				BuildPrecompileArgv(
									BuilderRec*		inBuilderRec,
									StringList&		inArgv,
									MFileRec& 		inFileRec);
		status_t				BuildCompileArgv(
									MakeActionT 	inAction,
									BuilderRec*		inBuilderRec,
									StringList&		inArgv,
									MFileRec& 		inFileRec);
		status_t				BuildPostLinkArgv(
									BuilderRec*		inBuilderRec,
									StringList&		inArgv,
									MFileRec& 		inFileRec);

		void					GetBuiltAppPath(
									char* 	outPath,
									int32	inBufferLength);
		const char *			LinkerName();
		status_t				ParseLinkerMessageText(
									const char*	inText,
									BList& 		outList);
		status_t				GetExecutableRef(
									entry_ref&	outRef);
		status_t				Launch(
									entry_ref&	inRef);

		void					SetConcurrentCompiles(
									uint32	inHowMany);

		void					SetStopOnErrors(bool stopOnErrors);
		void					SetBuildPriority(int32 priority);
		int32					GetPriority() const
								{
									return fBuildThreadPriority;
								}
		
		void					SetAutoOpenErrorWindow(bool openErrorWindow);
		bool					GetAutoOpenErrorWindow()
								{
									return fAutoOpenErrorWindow;
								}
								
		bool					IsRunnable();
		void					Cancel();
		bool					IsIdle();
		CompileState			CompilingState()
								{
									return fCompilingState;
								}
		void					SetProject(
									const entry_ref&	inProject)
								{
									fProjectRef = inProject;
								}
		void					SetLinker(
									MPlugInLinker*	inLinker)
								{
									fLinker = inLinker;
								}

private:

		MProjectView&			fProjectView;
		BWindow*				fWindow;
		BList					fCompileList;			// Files in the current compile
		BList					fDisassembleList;		// Files in the current disassemble
		int32					fCompleted;				// Number of compiles completed
		int32					fToBeCompleted;			// Number of compiles to be completed
		uint32					fConcurrentCompiles;
		CompileState			fCompilingState;
		MCompileGenerator		fCompileGenerator;
		
		uint32					fLaunchCommand;
		MCompile*				fCompileObj;
		entry_ref				fProjectRef;
		entry_ref				fExecutableRef;
		MStartBuildThread*		fBuildThread;
		MDeferredScriptHandler*	fScriptHandler;
		BMessage*				fBuildWaiter;

		BLocker&				fFileListLocker;
		MProjectLineList&		fFileList;				// All the sourcefileLines in the project window
		MPlugInLinker*			fLinker;
		ProjectTargetList		fTargetList;
		BLocker					fLock;
		MPrefsContainer			fPrefsContainer;

		ProjectPrefs			fProjectPrefs;
		bool					fNeedsToLink;
		bool					fStopOnErrors;
		bool					fAutoOpenErrorWindow;
		int32					fBuildThreadPriority;
			
		void					DoLink();
		bool					NeedsToLink();
		void					DoCompileOne(
									BMessage& inMessage);
		void					DoPreprocessOne(
									BMessage& inMessage);
		void					CompileSelection();
		void					PrecompileSelection();
		status_t				DoPrecompile(
									BMessage&	inMessage);
		void					BuildPrecompileArgv(
									const char* 	inFilePath,
									StringList&		inArgv);
		void					ResetPrecompiledHeaderFile(
									const char*		inFileName);

		void					StartBuild(CompileState inState);
		void					MakeSubProjects(CompileState inState);
		void					MakeStepOne(CompileState inState);
		void					MakeStepOneExtra(CompileState inState);
		void					MakeStepTwo(CompileState inState);
		void					MakeStepFour();
		void					ExecuteStageFourLines();
		void					MakeStepFourFinal();
		void					Launch();
		void					ReportLaunchError(
									status_t	inErr);

		void					DoBuildAndReply();

		void					LastState();
		void					SetExecutableRef();
		void					GetExecutableRef();

		status_t				ExecuteOneFile(
									BMessage& 		inMessage,
									MakeActionT		inAction);


		void					PostNoFileMessage(
									MSourceFileLine*	line);
	
		void					FillCompileList();

		void					PreprocessSelection();
		void					DoPreCompileOne(
									BMessage& inMessage);
		void					CheckSyntaxSelection();
		void					DoCheckSyntaxOne(
									BMessage& inMessage);
		void					DisassembleSelection();
		void					DoDisassembleOnePartOne(
									BMessage& inMessage);
		void					DisassemblePartTwo();

		void					CopyAllResources(
									BResources&			inFrom,
									BResources&			inTo,
									entry_ref&			inFromRef,
									bool				inReplace = false);
		void					LockFileList();
		void					UnlockFileList();
		void					EmptyTargetList();
		static void				AppendToList(
									BList&			inFromList,
									BList&			inToList);

		static void				ArgvToList(
									char**&			inArgv,
									MList<char*>&	inList);
		static void				ListToArgv(
									char**&			inArgv,
									MList<char*>&	inList);
		void					LockData();
		void					UnlockData();

		void					ClearMessages();
		void					Kill();

		void					FillFileRec(
									BEntry&		inFile,
									MFileRec&	outRec) const;

		BWindow*				Window()
								{
									return fWindow;
								}
};

inline void
MBuildCommander::LockData()
{
	fLock.Lock();
}
inline void
MBuildCommander::UnlockData()
{
	fLock.Unlock();
}

#endif
