// ---------------------------------------------------------------------------
/*
	GCCBuilder.h
	A builder (compiler) plugin for Metrowerks CodeWarrior
	Creates argument lists for gcc compiler
	Compiler is not IDE_aware
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#ifndef _GCCBUILDER_H
#define _GCCBUILDER_H

#include "MPlugInBuilder.h"
#include "IncludePathCache.h"
#include "GCCOptions.h"
#include "CommandLineText.h"
#include "ProjectSettingsMap.h"

#include <String.h>
#include <Entry.h>

// ---------------------------------------------------------------------------
//	class GCCBuilder
// ---------------------------------------------------------------------------

class ProjectSettings
{
public:
								ProjectSettings();
								~ProjectSettings();
	void						Reset();
																
	entry_ref					fObjectFileDirectory;
	BString						fObjectFileDirectoryPath;
	LanguageSettings			fLanguageSettings;
	CommonWarningSettings		fCommonWarningSettings;
	WarningSettings				fWarningSettings;
	CodeGenerationSettings		fCodeGenerationSettings;
	CommandLineText				fCommandLineText;
	IncludePathCache			fProjectIncludes;
	IncludePathCache			fSystemIncludes;
	bool						fTreatQuotesAsBrackets;

};

class GCCBuilder : public MPlugInBuilder
{
public:
							GCCBuilder();
							~GCCBuilder();
							
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

	virtual status_t			ParseMessageText(MProject& inProject,
												 const char* inText, 
												 BList& outList);
	
	virtual void				CodeDataSize(MProject& inProject,
											 const char* inFilePath,
											 int32& outCodeSize,
											 int32& outDataSize);
											
	virtual status_t			GenerateDependencies(MProject& inProject,
													 const char* inFilePath, 
													 BList& outList);

	virtual void				GetTargetFilePaths(MProject& inProject,
												   MFileRec& inFileRec, 
												   BList& inOutTargetFileList);

	virtual void				ProjectChanged(MProject& inProject, ChangeT inChange);

	virtual void				BuildStandardArgv(MProject* inProject, BList& inArgv);

private:
	void						SaveProjectSetting(MProject& inProject);

	void						CacheObjectFileDirectory(MProject& inProject);

	void						EmptyDirectoryList(BList& directoryList);

	void						BuildObjectFileName(BString& objectPath, const char* fileName, BString& objectFileName);
	void						IterateDirectoryList(BList& directoryList, IncludePathCache& theCache);

	void						BuildIntermediateFileName(const char* sourceName, 
														  char outputFile[B_FILE_NAME_LENGTH], 
														  bool fullPath);
	void						OpenIntermediateFile(const char* sourceFile);

private:
	ProjectSettingsMap<ProjectSettings>	fSettingsMap;
		
	char								fTempFileDirectoryName[B_FILE_NAME_LENGTH + 4];
	entry_ref							fTempFileDirectory;
	BLocker								fSettingsLock;
};

#endif
