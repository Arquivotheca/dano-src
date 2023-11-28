//========================================================================
//	BResBuilder.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Jon Watte

#ifndef _BRESBUILDER_H
#define _BRESBUILDER_H

#include "MPlugInPrefsView.h"
#include "MPlugInBuilder.h"

class ErrorMessage;

class BResBuilder : public MPlugInBuilder
{
	virtual long				GetToolName(MProject* inProject,
											char* outName,
											long inBufferLength,
											MakeStageT inStage,
											MakeActionT inAction);

	virtual const char *		LinkerName();
	virtual MakeStageT			MakeStages();
	virtual MakeActionT			Actions();
	virtual PlugInFlagsT		Flags();
	virtual ulong				MessageDataType();

	// return true if something changed in the settings
	virtual bool				ValidateSettings(BMessage& inOutMessage);

	// return B_NO_ERROR if everything went OK
	virtual long				BuildPrecompileArgv(
									MProject&	inProject,
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual long				BuildCompileArgv(
									MProject&	inProject,
									BList& 		inArgv,
									MakeActionT inAction,
									MFileRec& 	inFileRec);

	virtual long				BuildPostLinkArgv(
									MProject&	inProject,
									BList& 		inArgv,
									MFileRec& 	inFileRec);

	virtual bool				FileIsDirty(
									MProject&	inProject,
									MFileRec& 	inFileRec,
									MakeStageT	inStage,
									MakeActionT	inAction,
									time_t		inModDate);

	virtual long				ParseMessageText(
									MProject&	inProject,
									const char*	inText,
									BList&		outList);
	virtual void				CodeDataSize(
									MProject&	inProject,
									const char* inFilePath,
									long&		outCodeSize,
									long&		outDataSize);
	virtual long				GenerateDependencies(
									MProject&	inProject,
									const char* inFilePath,
									BList&		outList);

	virtual void				GetTargetFilePaths(
									MProject&	inProject,
									MFileRec& 	inFileRec,
									BList&		inOutTargetFileList);

	virtual void				ProjectChanged(
									MProject&	inProject,
									ChangeT		inChange);
private:

	static	long				FindEndOfLine(
									const char*	inText,
									long		inOffset);
	static	ErrorMessage *		ParseError(
									const char*	inBeg,
									const char*	inEnd);

};

#endif
