// ---------------------------------------------------------------------------
/*
	NASMBuilder.h
		
	Author:	John R. Dance
			21 June 1999

	NASMBuilder drives NASM the x86 assembler.

*/
// ---------------------------------------------------------------------------

#ifndef _NASMBUILDER_H
#define _NASMBUILDER_H

#include "MPlugInBuilder.h"
#include "CommandLineText.h"
#include "ProjectSettingsMap.h"

#include <Entry.h>
#include <String.h>

class ErrorMessage;
class MProject;

class NasmSettings
{
public:
							NasmSettings();
							~NasmSettings();
	CommandLineText			fCommandLineOptions;

	entry_ref				fObjectFileDirectory;
	BString					fObjectFileDirectoryPath;
};

class NASMBuilder : public MPlugInBuilder
{
public:
							NASMBuilder();
	virtual					~NASMBuilder();

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

	virtual status_t		BuildPrecompileArgv(MProject& inProject, BList& inArgv, MFileRec& inFileRec);

	virtual status_t		BuildCompileArgv(MProject& inProject,
											 BList& inArgv, 
											 MakeActionT inAction, 
											 MFileRec& inFileRec);

	virtual status_t		BuildPostLinkArgv(MProject& inProject, BList& inArgv, MFileRec& inFileRec);

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

private:
	void					BuildObjectFileName(BString& objectPath, const char* fileName, BString& objectFileName);

	void					CacheObjectFileDirectory(MProject& inProject, NasmSettings* cache);

	void					SaveProjectSettings(MProject& inProject);
	void					BuildIntermediateFileName(const char* sourceName, char outputFile[B_FILE_NAME_LENGTH], bool fullPath);
	void					OpenIntermediateFile(const char* sourceFile);
	ErrorMessage*			CreateEmptyError();
	ErrorMessage*			CreateTextOnlyMessage(const BString& text);
	ErrorMessage*			CreateFileLineError(const BString& text);	

private:
	ProjectSettingsMap<NasmSettings>	fSettingsMap;
	
	char								fTempFileDirectoryName[B_FILE_NAME_LENGTH + 4];
	entry_ref							fTempFileDirectory;
};

#endif
