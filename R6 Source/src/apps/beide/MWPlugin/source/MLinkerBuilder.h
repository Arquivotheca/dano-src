//========================================================================
//	MLinkerBuilder.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MLINKERBUILDER_H
#define _MLINKERBUILDER_H

#include "MPlugInLinker.h"
#include "PlugInPreferences.h"
#include "MDefaultPrefs.h"
#include "ProjectSettingsMap.h"

struct MFileRec;

// ---------------------------------------------------------------------------
// class LinkerProjectSettings
// ---------------------------------------------------------------------------

class LinkerProjectSettings
{
public:
						LinkerProjectSettings();
						~LinkerProjectSettings();
	ProjectPrefs		fProjectPrefs;
	LinkerPrefs			fLinkerPrefs;
	PEFPrefs			fPEFPrefs;
	DisassemblePrefs	fDisassemblePrefs;
};

// ---------------------------------------------------------------------------
// class MLinkerBuilder
// ---------------------------------------------------------------------------


class MLinkerBuilder : public MPlugInLinker
{
public:

	virtual status_t			GetToolName(
									MProject* 	inProject,
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
									MProject& 	inProject,
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual status_t			BuildCompileArgv(
									MProject& 	inProject,
									BList& 		inArgv,
									MakeActionT inAction,
									MFileRec& 	inFileRec);

	virtual status_t			BuildPostLinkArgv(
									MProject& 	inProject,
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual status_t			BuildLinkArgv(
									MProject& 	inProject,
									BList&		inArgv);

	virtual bool				FileIsDirty(
									MProject& 	inProject,
									MFileRec& 	inFile,
									MakeStageT	inStage,
									MakeActionT	inAction,
									time_t		inModDate);

	virtual status_t			ParseMessageText(
									MProject& 	inProject,
									const char*	text,
									BList&		outList);
	virtual status_t			GenerateDependencies(
									MProject& 	inProject,
									const char* inFilePath,
									BList&		outList);

	virtual void				ProjectChanged(
									MProject& 	inProject,
									ChangeT		inChange);

	virtual void				CodeDataSize(
									MProject& 	inProject,
									const char* inFilePath,
									int32&	outCodeSize,
									int32&	outDataSize);

	virtual void				GetTargetFilePaths(
									MProject& 	inProject,
									MFileRec& 	inFileRec,
									BList&		inOutTargetFileList);

	virtual	status_t			GetExecutableRef(
									MProject& 	inProject,
									entry_ref& outExecutableRef);
	virtual	status_t			GetBuiltAppPath(
										MProject&	inProject,
										char * 		outPath,
										int32		inBufferLength);

	// Can the executable be launched or run?
	virtual bool				IsLaunchable(MProject& 	inProject);

	// Launch the executable with or without the debugger
	virtual status_t			Launch(
									MProject& 	inProject,
									bool		inRunWithDebugger);

private:
	ProjectSettingsMap<LinkerProjectSettings>	fSettingsMap;
	
	void						MessageToPrefs(MProject& inProject);
	void						RunMenuItemChanged(MProject& inProject);
	void						LinkDone(MProject& inProject);
	char *						ExpFileName(MProject& inProject);
	void						BuildDisassembleArgv(
									MProject&		inProject,
									BList&			inArgv,
									const char *	inFilePath);
	status_t					GetSYMFileRef(
									MProject&	inProject,
									entry_ref&	outSYMFileRef);
};

#endif
