// ---------------------------------------------------------------------------
/*
	GCCLinker.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#ifndef _GCCLINKER_H
#define _GCCLINKER_H

#include "MPlugInLinker.h"
#include "MProject.h"
#include "GCCOptions.h"
#include "CommandLineText.h"
#include "MPrefsStruct.h"
#include "ProjectSettingsMap.h"

class BString;

class ProjectLinkerSettings {
public:
							ProjectLinkerSettings();
							~ProjectLinkerSettings();
							
	LinkerSettings			fLinkerSettings;
	CodeGenerationSettings	fCodeGenerationSettings;
	CommandLineText			fCommandLineText;
	ProjectPrefsx86			fProjectOptions;
};

// ---------------------------------------------------------------------------
//	class GCCLinker
// ---------------------------------------------------------------------------

class GCCLinker : public MPlugInLinker
{
public:
							GCCLinker();
							~GCCLinker();
								
	// MPlugInLinker Overrides
	virtual const char*		TargetName();
	virtual status_t		BuildLinkArgv(MProject& inProject, BList& inArgv);

	virtual	status_t		GetExecutableRef(MProject& inProject, entry_ref& outExecutableRef);
	virtual	status_t		GetBuiltAppPath(MProject& inProject, char* outPath, int32 inBufferLength);

	virtual bool			IsLaunchable(MProject& inProject);
	virtual status_t		Launch(MProject& inProject, bool inRunWithDebugger);

	// MPlugInBuilder Overrides
	virtual status_t		GetToolName(MProject* inProject,
										char* outName,
										int32 inBufferLength,
										MakeStageT inStage,
										MakeActionT inAction);

	virtual const char*		LinkerName();
	virtual MakeStageT		MakeStages();
	virtual MakeActionT		Actions();
	virtual PlugInFlagsT	Flags();
	virtual ulong			MessageDataType();

	virtual bool			ValidateSettings(BMessage& inOutMessage);

	virtual status_t		BuildPrecompileArgv(MProject& inProject,
												BList& inArgv, 
												MFileRec& inFileRec);

	virtual status_t		BuildCompileArgv(MProject& inProject,
											 BList& inArgv, 
											 MakeActionT inAction, 
											 MFileRec& inFileRec);

	virtual status_t		BuildPostLinkArgv(MProject& inProject,
											  BList& inArgv, 
											  MFileRec& inFileRec);

	virtual bool			FileIsDirty(MProject& inProject,
										MFileRec& inFileRec,
										MakeStageT inStage,
										MakeActionT inAction,
										time_t inModDate);

	virtual status_t		ParseMessageText(MProject& inProject,
											 const char* inText, 
											 BList& outList);
	
	virtual void			CodeDataSize(MProject& inProject,
										 const char* inFilePath,
										 int32& outCodeSize,
										 int32& outDataSize);
											
	virtual status_t		GenerateDependencies(MProject& inProject,
												 const char* inFilePath, 
												 BList& outList);

	virtual void			GetTargetFilePaths(MProject& inProject,
											   MFileRec& inFileRec, 
											   BList& inOutTargetFileList);

	virtual void			ProjectChanged(MProject& inProject, ChangeT inChange);

	virtual void			BuildStandardArgv(MProject* inProject, BList& inArgv);

private:
	void					SaveProjectSetting(MProject& inProject);
	void					LinkDone(MProject& inProject);
	void					MakeTargetApplicationName(ProjectPrefsx86& options, BString& appName);
	void					CleanArchive(MProject& inProject, const BString& archiveName);
	void					CheckDebuggingOptions(MProject& inProject);

private:
	ProjectSettingsMap<ProjectLinkerSettings>	fSettingsMap;
};

#endif
