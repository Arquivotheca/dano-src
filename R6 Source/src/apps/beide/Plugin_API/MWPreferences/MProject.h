//========================================================================
//	MProject.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPROJECT_H
#define _MPROJECT_H

#include "PlugInPreferences.h"
#include <Directory.h>

class BMessage;
class BList;

struct MFileRec
{
	char		path[256];		// full path to file
	char*		name;			// unique name of file
	uint32		fileType;
	MakeStageT	makeStage;
	bool		hasResources;
	short		fileID;
	// should also have something to indicate the target tool ????
};

class MProjectWindow;

class AccessDirectoryInfo
{
public:
	AccessDirectoryInfo(const entry_ref& dir_entry, bool isRecursive)
						: fDirectory(&dir_entry),
						  fSearchRecursive(isRecursive)
						  { }

	BDirectory	fDirectory;
	bool		fSearchRecursive;	
};

class MProject
{
public:
								MProject(
									MProjectWindow&	inProject);
	virtual						~MProject();

	virtual status_t			GetProjectRef(
									entry_ref&	outEntryRef);
	virtual status_t			GetExecutableRef(
									entry_ref&	outEntryRef);
	virtual void				GetBuiltAppPath(
									char * 	outPath,
									int32	inBufferLength);
	virtual int32				FileCount();
	virtual bool				GetNthFile(
									MFileRec&	outRec,
									int32		inIndex);
	virtual bool				GetNthFile(
									MFileRec&	outRec,
									BList&		outTargetList,
									int32		inIndex);
	virtual const char *		LinkerName();
	virtual	void				GetPrefs(
									uint32		inPrefsType,
									BMessage&	inoutMessage);
	virtual	void				SetPrefs(
									BMessage&	inMessage,
									uint32		inUpdateType);
	virtual bool				RunsWithDebugger();
	virtual status_t			Launch(
									entry_ref&	inRef);

	// returns BList of AccessDirectoryInfo
	virtual void				GetAccessDirectories(
									BList&	inOutProjectList,
									BList&	inOutSystemList,
									bool&	outSearchProjectTreeFirst);

private:

	MProjectWindow&				fProject;
};

#endif
