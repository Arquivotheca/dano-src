//========================================================================
//	MPlugInBuilder.h
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================

#ifndef _MPLUGINBUILDER_H
#define _MPLUGINBUILDER_H

#include <Message.h>
#include <List.h>
#include <StorageDefs.h>

#include "PlugInPreferences.h"

enum ChangeT
{
	kProjectOpened,			// a project file has opened
	kProjectClosed,			// a project file has closed
	kFilesAdded,			// files were added to the project
	kFilesRemoved,			// files were removed from the project
	kFilesRearranged,		// files were rearranged in the project window
	kBuildStarted,			// a build was initiated
	kPrefsChanged,			// The user changed something in the settings window
	kRunMenuItemChanged,	// The run/debug menu item was changed
	kLinkDone				// The link step completed
};

class MProject;
struct MFileRec;


class MPlugInBuilder
{
public:
#if 0
								MPlugInBuilder()
									{}
#endif
	virtual						~MPlugInBuilder()
									{}

	// inProject can be nil (during tool setup and target handling)
	virtual status_t			GetToolName(MProject* inProject,
											char* outName,
											int32 inBufferLength,
											MakeStageT inStage,
											MakeActionT inAction) = 0;

	virtual const char*			LinkerName() = 0;
	virtual MakeStageT			MakeStages() = 0;
	virtual MakeActionT			Actions() = 0;
	virtual PlugInFlagsT		Flags() = 0;
	virtual ulong				MessageDataType() = 0;

	// return true if something changed in the settings
	virtual bool				ValidateSettings(BMessage& inOutMessage) = 0;

	// return B_NO_ERROR if everything went OK
	virtual status_t			BuildPrecompileArgv(MProject& inProject,
													BList& inArgv,
													MFileRec& inFileRec) = 0;

	virtual status_t			BuildCompileArgv(MProject& inProject,
												 BList& inArgv,
												 MakeActionT inAction,
												 MFileRec& inFileRec) = 0;

	virtual status_t			BuildPostLinkArgv(MProject& inProject,
												  BList& inArgv,
												  MFileRec& inFileRec) = 0;

	virtual bool				FileIsDirty(MProject& inProject,
											MFileRec& inFileRec,
											MakeStageT inStage,
											MakeActionT inAction,
											time_t inModDate) = 0;

	virtual status_t			ParseMessageText(MProject& inProject,
												 const char* inText,
												 BList& outList) = 0;

	virtual void				CodeDataSize(MProject&	inProject,
											 const char* inFilePath,
											 int32& outCodeSize,
											 int32& outDataSize) = 0;

	virtual status_t			GenerateDependencies(MProject& inProject,
													 const char* inFilePath,
													 BList& outList) = 0;

	virtual void				GetTargetFilePaths(MProject& inProject,
												   MFileRec& inFileRec,
												   BList& inOutTargetFileList) = 0;

	virtual void				ProjectChanged(MProject& inProject,
											   ChangeT inChange) = 0;
};

#endif
