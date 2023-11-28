//========================================================================
//	MmwccBuilderx86.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#pragma once

#include "MPlugInBuilder.h"
#include "PlugInPreferences.h"
#include "MDefaultPrefs.h"
#include "CString.h"


class MmwccBuilderx86 : public MPlugInBuilder
{
public:
								MmwccBuilderx86();
	virtual						~MmwccBuilderx86();

	virtual status_t			GetToolName(
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
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual status_t			BuildCompileArgv(
									BList& 		inArgv,
									MakeActionT inAction,
									MFileRec& 	inFileRec);

	virtual status_t			BuildPostLinkArgv(
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual bool				FileIsDirty(
									MFileRec& 	inFile,
									MakeStageT	inStage,
									MakeActionT	inAction,
									time_t		inModDate);

	virtual void				CodeDataSize(
									const char* inFilePath,
									int32&		outCodeSize,
									int32&		outDataSize);
	virtual status_t			ParseMessageText(
									const char*	text,
									BList&		outList);
	virtual status_t			GenerateDependencies(
									const char* inFilePath,
									BList&		outList);

	virtual void				GetTargetFilePaths(
									MFileRec& 	inFile,
									BList&		inOutTargetFileList);
	virtual void				ProjectChanged(
									ChangeT		inChange,
									MProject*	inProject);

private:

		LanguagePrefs			fLanguagePrefs;
		WarningsPrefs			fWarningsPrefs;
		ProjectPrefsx86			fProjectPrefs;
		CodeGenx86				fCodeGenPrefs;
		GlobalOptimizations		fGlobalOptsPrefs;
		entry_ref				fProjectRef;
		BDirectory				fObjectsDirectory;
		MProject*				fProject;
		String					fObjectPath;

		void					MessageToPrefs();

		void					BuildCompilerArgv(
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildPreprocessArgv(
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildPrecompileActionArgv(
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildCheckSyntaxArgv(
									BList&			inArgv,
									MFileRec& 		inFileRec);
		void					BuildDisassembleArgv(
									BList&			inArgv,
									MFileRec& 		inFileRec);

		void					WarningsArgv(
									BList&			inArgv);
		void					LanguageArgv(
									BList&			inArgv,
									bool			inUsePrefixFile = true);
		void					ProcessorArgv(
									BList&			inArgv);
		void					MachinecodeArgv(
									BList&			inArgv);
		bool					SourceFileIsDirty(
									time_t			inModDate,
									const char *	inFileName);
		bool					PCHFileIsDirty(
									time_t			inModDate,
									const char *	inFileName);
		void					UpdateObjectsDirectory();
		void					AppendDashOh(
									BList&			inArgv,
									const char *	inFileName);
		void					AppendDashCee(
									BList&			inArgv,
									const char*		inFilePath);
		void					BuildTargetFileName(
									const char*		inName,
									char*			outName) const;

};