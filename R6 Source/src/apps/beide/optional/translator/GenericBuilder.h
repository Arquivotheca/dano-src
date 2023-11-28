// ---------------------------------------------------------------------------
/*
	GenericBuilder.h
		
	Author:	John R. Dance
			26 February 1999

	GenericBuilder is a builder tool that can act in the precompile stage 
	of the build.  It has the following type of usage:
		tool [options] -o filename.outputExtension filename.inputExtension
	It can build .cpp or .rsrc files that are used in the build of the
	project itself.
	
	Some examples of tools that can be used with a BuildHelper customized
	to that tool:  flex, bison, rez, mwbres

	All tool specific actions are factored out and delegated to 
	the BuildHelper.
*/
// ---------------------------------------------------------------------------

#ifndef _GENERICBUILDER_H
#define _GENERICBUILDER_H

#include "MPlugInBuilder.h"

class BuildHelper;

class GenericBuilder : public MPlugInBuilder
{
public:
							GenericBuilder(BuildHelper* adoptHelper);
	virtual					~GenericBuilder();

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
	
private:
	BuildHelper*			fBuildHelper;
};

#endif
