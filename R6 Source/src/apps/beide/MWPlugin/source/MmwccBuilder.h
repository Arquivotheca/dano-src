//========================================================================
//	MmwccBuilder.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MMWCCBUILDER_H
#define _MMWCCBUILDER_H

#include "MPlugInBuilder.h"
#include "PlugInPreferences.h"
#include "MDefaultPrefs.h"
#include "CString.h"
#include "ProjectSettingsMap.h"

#include <Entry.h>

// ---------------------------------------------------------------------------
//	class mwccSettings
// ---------------------------------------------------------------------------

class mwccSettings
{
public:
								mwccSettings();
								~mwccSettings();

		ProcessorPrefs			fProcessorPrefs;
		LanguagePrefs			fLanguagePrefs;
		WarningsPrefs			fWarningsPrefs;
		DisassemblePrefs		fDisassemblePrefs;
		GlobalOptimizations		fGlobalOptsPrefs;
		entry_ref				fProjectRef;
		entry_ref				fObjectsDirectory;
		String					fObjectPath;		
};

// ---------------------------------------------------------------------------
//	class MmwccBuilder
// ---------------------------------------------------------------------------

class MmwccBuilder : public MPlugInBuilder
{
public:
								MmwccBuilder();
	virtual						~MmwccBuilder();

	virtual status_t			GetToolName(
									MProject*	inProject,
									char* 		outName,
									int32		inBufferLength,
									MakeStageT	inStage,
									MakeActionT	inAction);

	virtual const char *		LinkerName();
	virtual MakeActionT			Actions();
	virtual MakeStageT			MakeStages();
	virtual PlugInFlagsT		Flags();
	virtual ulong				MessageDataType();

	virtual bool				ValidateSettings(
									BMessage&	inOutMessage);

	virtual status_t			BuildPrecompileArgv(
									MProject&	inProject,
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual status_t			BuildCompileArgv(
									MProject&	inProject,
									BList& 		inArgv,
									MakeActionT inAction,
									MFileRec& 	inFileRec);

	virtual status_t			BuildPostLinkArgv(
									MProject&	inProject,
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual bool				FileIsDirty(
									MProject&	inProject,
									MFileRec& 	inFile,
									MakeStageT	inStage,
									MakeActionT	inAction,
									time_t		inModDate);

	virtual void				CodeDataSize(
									MProject&	inProject,
									const char* inFilePath,
									int32&		outCodeSize,
									int32&		outDataSize);
	virtual status_t			ParseMessageText(
									MProject&	inProject,
									const char*	text,
									BList&		outList);
	virtual status_t			GenerateDependencies(
									MProject&	inProject,
									const char* inFilePath,
									BList&		outList);

	virtual void				GetTargetFilePaths(
									MProject&	inProject,
									MFileRec& 	inFile,
									BList&		inOutTargetFileList);
	virtual void				ProjectChanged(
									MProject&	inProject,
									ChangeT		inChange);

private:
		ProjectSettingsMap<mwccSettings>	fSettingsMap;

		void					MessageToPrefs(MProject& inProject);

		void					BuildCompilerArgv(
									MProject&		inProject,
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildPreprocessArgv(
									MProject&		inProject,
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildPrecompileActionArgv(
									MProject&		inProject,
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildCheckSyntaxArgv(
									MProject&		inProject,
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildDisassembleArgv(
									MProject&		inProject,
									BList&			inArgv,
									MFileRec& 		inFileRec);

		void					WarningsArgv(
									const WarningsPrefs&	warningsPrefs,
									BList&					inArgv);
		void					LanguageArgv(
									const LanguagePrefs&	languagePrefs,
									BList&					inArgv,
									bool					inUsePrefixFile = true);
		void					ProcessorArgv(
									const ProcessorPrefs&		processorPrefs,
									const GlobalOptimizations&	globalOptPrefs,
									BList&						inArgv);

		bool					SourceFileIsDirty(
									MProject&		inProject,
									time_t			inModDate,
									const char *	inFileName);
		bool					PCHFileIsDirty(
									time_t			inModDate,
									const char *	inFileName);
		void					UpdateObjectsDirectory(MProject& inProject);
		void					AppendDashOh(
									String			objectPath,
									BList&			inArgv,
									const char *	inFileName);
		void					AppendDashCee(
									BList&			inArgv,
									const char*		inFilePath);
		void					BuildTargetFileName(
									const char*		inName,
									char*			outName) const;

};

#endif
