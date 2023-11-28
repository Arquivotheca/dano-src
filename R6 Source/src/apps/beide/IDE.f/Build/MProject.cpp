//========================================================================
//	MProject.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MProject.h"
#include "MProjectWindow.h"
#include "MBuildCommander.h"

// ---------------------------------------------------------------------------
//		MProject
// ---------------------------------------------------------------------------

MProject::MProject(
	MProjectWindow&		inProject)
	: fProject(inProject)
{
}

// ---------------------------------------------------------------------------
//		~MProject
// ---------------------------------------------------------------------------

MProject::~MProject()
{
}

// ---------------------------------------------------------------------------
//		ProjectRef
// ---------------------------------------------------------------------------
//	return entry_ref of the project.

status_t
MProject::GetProjectRef(
	entry_ref&	outEntryRef)
{
	return fProject.GetRef(outEntryRef);
}

// ---------------------------------------------------------------------------
//		ExecutableRef
// ---------------------------------------------------------------------------
//	return entry_ref of the executable.  Will be invalid
//	if the executable hasn't been generated yet.

status_t
MProject::GetExecutableRef(
	entry_ref&	outEntryRef)
{
	return fProject.ProjectView().BuildCommander().GetExecutableRef(outEntryRef);
}

// ---------------------------------------------------------------------------
//		GetBuiltAppPath
// ---------------------------------------------------------------------------
//	return the path to the application.  Will be correct
//	whether the executable exists or not.

void
MProject::GetBuiltAppPath(
	char * 	outPath,
	int32	inBufferLength)
{
	fProject.ProjectView().BuildCommander().GetBuiltAppPath(outPath, inBufferLength);
}

// ---------------------------------------------------------------------------
//		FileCount
// ---------------------------------------------------------------------------
//	return numbe of files in the project window.

int32
MProject::FileCount()
{
	return fProject.ProjectView().FileCount();
}

// ---------------------------------------------------------------------------
//		GetNthFile
// ---------------------------------------------------------------------------
//	Get the nth file in the project window.  returns false
//	if inIndex is out of bounds.

bool
MProject::GetNthFile(
	MFileRec&	outRec,
	int32		inIndex)
{
	return fProject.ProjectView().GetNthFile(outRec, inIndex);
}

// ---------------------------------------------------------------------------
//		GetNthFile
// ---------------------------------------------------------------------------
//	Get the nth file in the project window.  returns false
//	if inIndex is out of bounds.

bool
MProject::GetNthFile(
	MFileRec&	outRec,
	BList&		outTargetList,
	int32		inIndex)
{
	return fProject.ProjectView().GetNthFile(outRec, outTargetList, inIndex);
}

// ---------------------------------------------------------------------------
//		GetAccessDirectories
// ---------------------------------------------------------------------------
//	Get the access directories that are currently set up


void
MProject::GetAccessDirectories(
	BList&	inOutProjectList,
	BList&	inOutSystemList,
	bool&	outSearchProjectTreeFirst)
{
	fProject.ProjectView().GetAccessDirectories(inOutProjectList, inOutSystemList, outSearchProjectTreeFirst);
}

// ---------------------------------------------------------------------------
//		LinkerName
// ---------------------------------------------------------------------------
//	return the name of the linker for this project.  Allows builders
//	to determine if they can be used in this project.

const char *
MProject::LinkerName()
{
	return fProject.ProjectView().BuildCommander().LinkerName();
}

// ---------------------------------------------------------------------------
//		GetPrefs
// ---------------------------------------------------------------------------
//	Fill the message with preferences of the type specified.

void
MProject::GetPrefs(
	uint32		inPrefsType,
	BMessage&	inoutMessage)
{
	fProject.ProjectView().BuildCommander().FillMessage(inoutMessage, inPrefsType);
}

// ---------------------------------------------------------------------------
//		SetPrefs
// ---------------------------------------------------------------------------
//	Add or replace the preferences specified.

void
MProject::SetPrefs(
	BMessage&	inMessage,
	uint32		inUpdateType)
{
	fProject.ProjectView().SetPrefs(inMessage, inUpdateType);
//	fProject.ProjectView().BuildCommander().SetData(inMessage);
}

// ---------------------------------------------------------------------------
//		RunsWithDebugger
// ---------------------------------------------------------------------------
//	What is the state of the Run/Debug menu item?

bool
MProject::RunsWithDebugger()
{
	return fProject.RunsWithDebugger();
}

// ---------------------------------------------------------------------------
//		Launch
// ---------------------------------------------------------------------------
//	What is the state of the Run/Debug menu item?

status_t
MProject::Launch(
	entry_ref&	inRef)
{
	return fProject.ProjectView().BuildCommander().Launch(inRef);
}

