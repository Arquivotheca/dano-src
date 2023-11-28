//========================================================================
//	MLinkerBuilderx86.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#pragma once

#include "MPlugInLinker.h"
#include "PlugInPreferences.h"
#include "MDefaultPrefs.h"

struct MFileRec;

class MLinkerBuilderx86 : public MPlugInLinker
{
public:

	virtual status_t			GetToolName(
									char* 		outName,
									int32		inBufferLength,
									MakeStageT	inStage,
									MakeActionT	inAction);

	virtual const char *		LinkerName();
	virtual const char *		TargetName();
	virtual MakeActionT			Actions();
	virtual MakeStageT			MakeStages();
	virtual PlugInFlagsT		Flags();
	virtual ulong				MessageDataType();

	virtual bool				ValidateSettings(
									BMessage&	inOutMessage);

	virtual status_t			BuildPrecompileArgv(
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual status_t			BuildCompileArgv(
									BList& 		inArgv,
									MakeActionT inAction,
									MFileRec& 	inFileRec);

	virtual status_t			BuildPostLinkArgv(
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual status_t			BuildLinkArgv(
									BList&		inArgv);

	virtual bool				FileIsDirty(
									MFileRec& 	inFile,
									MakeStageT	inStage,
									MakeActionT	inAction,
									time_t		inModDate);

	virtual status_t			ParseMessageText(
									const char*	text,
									BList&		outList);
	virtual status_t			GenerateDependencies(
									const char* inFilePath,
									BList&		outList);

	virtual void				ProjectChanged(
									ChangeT		inChange,
									MProject*	inProject);

	virtual void				CodeDataSize(
									const char* inFilePath,
									int32&	outCodeSize,
									int32&	outDataSize);

	virtual void				GetTargetFilePaths(
									MFileRec& 	inFileRec,
									BList&		inOutTargetFileList);

	virtual	status_t			GetExecutableRef(
									entry_ref& outExecutableRef);
	virtual	status_t			GetBuiltAppPath(
										char * 	outPath,
										int32	inBufferLength);

	// Can the executable be launched or run?
	virtual bool				IsLaunchable();

	// Launch the executable with or without the debugger
	virtual status_t			Launch(
									bool	inRunWithDebugger);

private:

		ProjectPrefsx86			fProjectPrefs;
		LinkerPrefsx86			fLinkerPrefs;
		MProject*				fProject;

		void					MessageToPrefs();
		void					RunMenuItemChanged();
		void					LinkDone();
		char *					ExpFileName();
		void					BuildDisassembleArgv(
									BList&			inArgv,
									const char *	inFilePath);
		status_t				GetSYMFileRef(
									entry_ref& outSYMFileRef);
};
