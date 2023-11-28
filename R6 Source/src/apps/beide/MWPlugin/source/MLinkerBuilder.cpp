//========================================================================
//	MLinkerBuilder.cpp
//	Copyright 1996-97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MLinkerBuilder.h"
#include "MProject.h"
#include "PlugInPreferences.h"
#include "CString.h"
#include "MPlugin.h"

#include <Path.h>
#include <AppFileInfo.h>
#include <ByteOrder.h>
#include <Mime.h>
#include <Debug.h>

const char *	mwldName = "mwldppc";
const char *	mwdisToolName1 = "mwdisppc";

const char *	mwldTargetName = "BeOS PowerPC C/C++";

// ---------------------------------------------------------------------------
//	LinkerSettings member functions
// ---------------------------------------------------------------------------

LinkerProjectSettings::LinkerProjectSettings()
{
	MDefaultPrefs::SetLinkerDefaults(fLinkerPrefs);
	MDefaultPrefs::SetPEFDefaults(fPEFPrefs);
	MDefaultPrefs::SetDisassemblerDefaults(fDisassemblePrefs);
	MDefaultPrefs::SetProjectDefaults(fProjectPrefs);
}

// ---------------------------------------------------------------------------

LinkerProjectSettings::~LinkerProjectSettings()
{
}

// ---------------------------------------------------------------------------
//		 GetToolName
// ---------------------------------------------------------------------------

status_t
MLinkerBuilder::GetToolName(
	MProject* 	/*inProject*/,
	char* 		outName,
	int32		/*inBufferLength*/,
	MakeStageT	/*inStage*/,
	MakeActionT	inAction)
{
	switch (inAction)
	{
		// For disassembly of library files
		case kDisassemble:
			strcpy(outName, mwdisToolName1);
			break;

		// For normal linking
		default:
			strcpy(outName, mwldName);
			break;
	}
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		 TargetName
// ---------------------------------------------------------------------------

const char *
MLinkerBuilder::TargetName()
{
	return mwldTargetName;
}

// ---------------------------------------------------------------------------
//		 LinkerName
// ---------------------------------------------------------------------------

const char *
MLinkerBuilder::LinkerName()
{
	return mwldName;
}

// ---------------------------------------------------------------------------
//		 Actions
// ---------------------------------------------------------------------------

MakeActionT
MLinkerBuilder::Actions()
{
	return kLink;
}

// ---------------------------------------------------------------------------
//		 MakeStages
// ---------------------------------------------------------------------------

MakeStageT
MLinkerBuilder::MakeStages()
{
	return kLinkStage;
}

// ---------------------------------------------------------------------------
//		 Flags
// ---------------------------------------------------------------------------

ulong
MLinkerBuilder::Flags()
{
	return kIDEAware;
}

// ---------------------------------------------------------------------------
//		 MessageDataType
// ---------------------------------------------------------------------------

ulong
MLinkerBuilder::MessageDataType()
{
	return kMWLDPlugType;
}

// ---------------------------------------------------------------------------
//		 ValidateSettings
// ---------------------------------------------------------------------------
//	return true if something changed in the settings.

bool
MLinkerBuilder::ValidateSettings(
	BMessage&	inOutMessage)
{
	bool		changed = false;
	long		len;

	// Linker prefs
	LinkerPrefs*	linkerPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kLinkerPrefs, kMWLDPlugType, (const void**) &linkerPrefs, &len))
	{
		if (B_BENDIAN_TO_HOST_INT32(linkerPrefs->pVersion) != kCurrentVersion)
		{
			LinkerPrefs		linker = *linkerPrefs;
			
			linker.SwapBigToHost();
			linker.pVersion = kCurrentVersion;
			linker.SwapHostToBig();
			inOutMessage.ReplaceData(kLinkerPrefs, kMWLDPlugType, &linker, sizeof(linker));
			changed = true;
		}
	}
	else
	{
		LinkerPrefs defaultLinkerPrefs;
		MDefaultPrefs::SetLinkerDefaults(defaultLinkerPrefs);
		defaultLinkerPrefs.SwapHostToBig();
		len = sizeof(defaultLinkerPrefs);
		inOutMessage.AddData(kLinkerPrefs, kMWLDPlugType, &defaultLinkerPrefs, len);
		changed = true;
	}

	// PEF prefs
	PEFPrefs*	pefPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kPEFPrefs, kMWLDPlugType, (const void**) &pefPrefs, &len))
	{
		if (B_BENDIAN_TO_HOST_INT32(pefPrefs->pVersion) != kCurrentVersion)
		{
			PEFPrefs		pefprefs = *pefPrefs;
			
			pefprefs.SwapBigToHost();
			pefprefs.pVersion = kCurrentVersion;
			pefprefs.SwapHostToBig();
			inOutMessage.ReplaceData(kPEFPrefs, kMWLDPlugType, &pefprefs, sizeof(pefprefs));
			changed = true;
		}
	}
	else
	{
		PEFPrefs defaultPEFPrefs;
		MDefaultPrefs::SetPEFDefaults(defaultPEFPrefs);
		defaultPEFPrefs.SwapHostToBig();
		len = sizeof(defaultPEFPrefs);
		inOutMessage.AddData(kPEFPrefs, kMWLDPlugType, &defaultPEFPrefs, len);
		changed = true;
	}

	// Project prefs
	ProjectPrefs*	projPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kProjectNamePrefs, kMWLDPlugType, (const void**) &projPrefs, &len))
	{
		if (B_BENDIAN_TO_HOST_INT32(projPrefs->pVersion) != kCurrentVersion)
		{
			ProjectPrefs		projprefs = *projPrefs;
			
			projprefs.SwapBigToHost();
			projprefs.pVersion = kCurrentVersion;
			projprefs.SwapHostToBig();
			inOutMessage.ReplaceData(kProjectNamePrefs, kMWLDPlugType, &projprefs, sizeof(projprefs));
			changed = true;
		}
	}
	else
	{
		ProjectPrefs defaultProjectPrefs;
		MDefaultPrefs::SetProjectDefaults(defaultProjectPrefs);
		defaultProjectPrefs.SwapHostToBig();
		len = sizeof(defaultProjectPrefs);
		inOutMessage.AddData(kProjectNamePrefs, kMWLDPlugType, &defaultProjectPrefs, len);
		changed = true;
	}

	return changed;
}

// ---------------------------------------------------------------------------
//		 BuildPrecompileArgv
// ---------------------------------------------------------------------------

int32
MLinkerBuilder::BuildPrecompileArgv(
	MProject& 	/*inProject*/,
	BList& 		/*inArgv*/,
	MFileRec& 	/*inFileRec*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		 BuildPostLinkArgv
// ---------------------------------------------------------------------------

int32
MLinkerBuilder::BuildPostLinkArgv(
	MProject& 	/*inProject*/,
	BList& 		/*inArgv*/,
	MFileRec& 	/*inFileRec*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		 BuildLinkArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when Linking.  

status_t
MLinkerBuilder::BuildLinkArgv(
	MProject& 		inProject,
	BList&			inArgv)
{
	LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);
	if (cache->fLinkerPrefs.pGenerateSYMFile)
	{
		inArgv.AddItem(strdup("-sym"));

		if (cache->fLinkerPrefs.pUseFullPath)
			inArgv.AddItem(strdup("full"));
		else
			inArgv.AddItem(strdup("on"));		
	}
	if (cache->fLinkerPrefs.pGenerateLinkMap)
	{
		String		linkmap = cache->fProjectPrefs.pAppName;

		linkmap += ".xMAP";
		inArgv.AddItem(strdup("-map"));
		inArgv.AddItem(strdup(linkmap));
	}
	if (cache->fLinkerPrefs.pSuppressWarnings)
	{
		inArgv.AddItem(strdup("-warn"));
	}
	if (! cache->fLinkerPrefs.pDeadStrip)
	{
		inArgv.AddItem(strdup("-dead"));
		inArgv.AddItem(strdup("off"));
	}
	if (cache->fPEFPrefs.pExportSymbols != kExportNone)	// default is export none
	{
		switch (cache->fPEFPrefs.pExportSymbols)
		{
			case kExportAll:
				inArgv.AddItem(strdup("-export"));
				inArgv.AddItem(strdup("all"));
				break;

			case kExportUsePragma:
				inArgv.AddItem(strdup("-export"));
				inArgv.AddItem(strdup("pragma"));
			break;

			case kExportUseFile:
				inArgv.AddItem(strdup("-f"));
				inArgv.AddItem(ExpFileName(inProject));
			break;
		}
	}
	if (cache->fProjectPrefs.pProjectKind != AppType)		// default is Application
	{
		switch (cache->fProjectPrefs.pProjectKind)
		{
			case SharedLibType:
				inArgv.AddItem(strdup("-G"));
				break;

			case LibraryType:
				inArgv.AddItem(strdup("-xml"));
				break;
		}
	}

	// Check if the names for the entry points match the default names
	// and add them to the command line if they don't match
	const char *	mainName;
	const char *	initName;
	const char *	termName;
	
	// What are the names for the init, term and main entry points?
	switch (cache->fProjectPrefs.pProjectKind)
	{
		case AppType:
		case DriverType:
			mainName = kDefaultAppMainName;
			initName = kDefaultAppInitName;
			termName = kDefaultAppTermName;
			break;
		case SharedLibType:
			mainName = kDefaultSharedLibMainName;
			initName = kDefaultSharedLibInitName;
			termName = kDefaultSharedLibTermName;
			break;
		case LibraryType:
			mainName = kDefaultLibMainName;
			initName = kDefaultLibInitName;
			termName = kDefaultLibTermName;
			break;
		default:
			ASSERT(false);
			break;
	}

	// default for an app is __start, _init_routine_, _term_routine_
	// for a shared lib, nothing, _init_routine_, _term_routine_
	// library has no entry point, or init/term routines
	if (0 != strcmp(cache->fLinkerPrefs.pMain, mainName))	
	{
		inArgv.AddItem(strdup("-m"));
		inArgv.AddItem(strdup(cache->fLinkerPrefs.pMain));
	}
	if (0 != strcmp(cache->fLinkerPrefs.pInit, initName))	
	{
		inArgv.AddItem(strdup("-init"));
		inArgv.AddItem(strdup(cache->fLinkerPrefs.pInit));
	}
	if (0 != strcmp(cache->fLinkerPrefs.pTerm, termName))	
	{
		inArgv.AddItem(strdup("-term"));
		inArgv.AddItem(strdup(cache->fLinkerPrefs.pTerm));
	}

	// Add all of the object files
	MFileRec		fileRec;
	BList			targetList;
	long			i = 0;

	// Ask the project object for all the target files
	// it asks the mwccbuilder object for them
	while (inProject.GetNthFile(fileRec, targetList, i++))
	{
		for (long j = 0; j < targetList.CountItems(); j++)
		{
			// need to validate that the file is a linkable file here????
			// it's possible that a tool that generates
			// a target file that isn't a linkable file.
			// check file types ? extensions ? magic numbers ?
			// the specified linker of the builder ? 
			char *		targetFilePath = (char*) targetList.ItemAtFast(j);
			inArgv.AddItem(targetFilePath);
			// We don't free the targetfilepath since we are making it part of the argv
		}
		
		targetList.MakeEmpty();
	}
	
	char		objectPath[kPathSize];

	GetBuiltAppPath(inProject, objectPath, kPathSize);
	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(objectPath));

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		 GetTargetFilePaths
// ---------------------------------------------------------------------------
//	In a linker builder the file itself is the target file.

void
MLinkerBuilder::GetTargetFilePaths(
	MProject& 		/*inProject*/,
	MFileRec& 		inFileRec,
	BList&			inOutTargetFileList)
{
	inOutTargetFileList.AddItem(strdup(inFileRec.path));
}

// ---------------------------------------------------------------------------
//		 MessageToPrefs
// ---------------------------------------------------------------------------
//	Copy the prefs from the BMessage to the prefs structs.

void
MLinkerBuilder::MessageToPrefs(MProject& inProject)
{
	BMessage	msg;
	long		len;

	// Remove any current settings that are now stale and add a new copy
	fSettingsMap.RemoveSettings(inProject);
	LinkerProjectSettings* cache = fSettingsMap.AddSettings(inProject);

	// Get linker prefs
	inProject.GetPrefs(kMWLDPlugType, msg);
	
	LinkerPrefs*	linkerPrefs;
	if (B_NO_ERROR == msg.FindData(kLinkerPrefs, kMWLDPlugType, (const void**) &linkerPrefs, &len))
	{
		cache->fLinkerPrefs = *linkerPrefs;
		cache->fLinkerPrefs.SwapBigToHost();
	}
	
	PEFPrefs*	pefPrefs;
	if (B_NO_ERROR == msg.FindData(kPEFPrefs, kMWLDPlugType, (const void**) &pefPrefs, &len))
	{
		cache->fPEFPrefs = *pefPrefs;
		cache->fPEFPrefs.SwapBigToHost();
	}

	ProjectPrefs*	projPrefs;
	if (B_NO_ERROR == msg.FindData(kProjectNamePrefs, kMWLDPlugType, (const void**) &projPrefs, &len))
	{
		cache->fProjectPrefs = *projPrefs;
		cache->fProjectPrefs.SwapBigToHost();
	}

	// Get disassembler prefs
	msg.MakeEmpty();
	inProject.GetPrefs(kMWCCPlugType, msg);

	DisassemblePrefs*	disassemblePrefs ;
	if (B_NO_ERROR == msg.FindData(kDisAsmPrefs, kMWCCPlugType, (const void**) &disassemblePrefs, &len))
	{
		cache->fDisassemblePrefs = *disassemblePrefs;
		cache->fDisassemblePrefs.SwapBigToHost();
	}
}

// ---------------------------------------------------------------------------
//		 ExpFileName
// ---------------------------------------------------------------------------
//	Generate the name of the exp file.  "Application.exp"
//	The returned string is a full path.
//	This isn't correct.  It assumes that the application is in the same
//	folder as the project.

char *
MLinkerBuilder::ExpFileName(MProject& inProject)
{
	entry_ref		projRef;
	BPath			projDirPath;
	String			path;
	
	if (B_NO_ERROR == inProject.GetProjectRef(projRef))
	{
		BEntry			project(&projRef);
		BDirectory		projDir;

		if (B_NO_ERROR == project.GetParent(&projDir))
		{
			BEntry		projDirEntry;

			if (B_NO_ERROR == projDir.GetEntry(&projDirEntry) &&
				B_NO_ERROR == projDirEntry.GetPath(&projDirPath))
			{
				LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);
				path = projDirPath.Path();
				path += '/';
				path += cache->fProjectPrefs.pAppName;
				path += ".exp";
			}
		}
	}

	return strdup(path);
}

// ---------------------------------------------------------------------------
//		 ProjectChanged
// ---------------------------------------------------------------------------

void
MLinkerBuilder::ProjectChanged(
	MProject&	inProject,
	ChangeT		inChange)
{
	switch (inChange)
	{
		case kProjectOpened:
			MessageToPrefs(inProject);
			break;

		case kProjectClosed:
			fSettingsMap.RemoveSettings(inProject);
			break;

		case kPrefsChanged:
			MessageToPrefs(inProject);
			break;

		case kRunMenuItemChanged:
			RunMenuItemChanged(inProject);
			break;

		case kLinkDone:
			LinkDone(inProject);
			break;

		case kBuildStarted:
		case kFilesAdded:	
		case kFilesRemoved:	
		case kFilesRearranged:	
			// do nothing for now
			break;
	}
}

// ---------------------------------------------------------------------------
//		 LinkDone
// ---------------------------------------------------------------------------
//	If the project is using the debugger and generate sym file is off,
//	then turn on generate sym file.

void MLinkerBuilder::LinkDone(MProject& inProject)
{
	status_t		err;
	entry_ref		executableRef;

	if (B_NO_ERROR == GetExecutableRef(inProject, executableRef))
	{
		LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);

		if (0 == strcmp(cache->fProjectPrefs.pAppType, B_PEF_APP_MIME_TYPE))
		{		
			// Update the mime info
			BEntry		app(&executableRef);
			BPath		path;
			if (B_NO_ERROR == app.GetPath(&path))
			{
				err = update_mime_info(path.Path(), false, true, true);
		
		#if DEBUG
				if (B_NO_ERROR != err)
					printf("update_mime_info failed, err = %p\n", err);
		#endif
			}
			// Set the Unix access permissions
			err = chmod(cache->fProjectPrefs.pAppName, S_IRWXU + S_IRWXG + S_IRWXO);
		}
		
		// Set the file type
		BFile			file(&executableRef, B_READ_WRITE);
		BAppFileInfo	info(&file);
		
		info.SetType(cache->fProjectPrefs.pAppType);
		// should also set the signature maybe ????
	}
}

// ---------------------------------------------------------------------------
//		 RunMenuItemChanged
// ---------------------------------------------------------------------------
//	If the project is using the debugger and generate sym file is off,
//	then turn on generate sym file.

void MLinkerBuilder::RunMenuItemChanged(MProject& inProject)
{
	LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);
	if (inProject.RunsWithDebugger() && ! cache->fLinkerPrefs.pGenerateSYMFile)
	{
		cache->fLinkerPrefs.pGenerateSYMFile = true;

		BMessage	msg;
		
		msg.AddData(kLinkerPrefs, kMWLDPlugType, &cache->fLinkerPrefs, sizeof(LinkerPrefs));

		inProject.SetPrefs(msg, kLinkUpdate);
	}
}

// ---------------------------------------------------------------------------
//		 ParseMessageText
// ---------------------------------------------------------------------------

status_t
MLinkerBuilder::ParseMessageText(
	MProject& 	/*inProject*/,
	const char*	/*text*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		 GenerateDependencies
// ---------------------------------------------------------------------------

status_t
MLinkerBuilder::GenerateDependencies(
	MProject& 	/*inProject*/,
	const char*	/*inFilePath*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		 CodeDataSize
// ---------------------------------------------------------------------------

void
MLinkerBuilder::CodeDataSize(
	MProject& 	/*inProject*/,
	const char* /*inFilePath*/,
	int32&	outCodeSize,
	int32&	outDataSize)
{
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------
//		 FileIsDirty
// ---------------------------------------------------------------------------
//	the only files targetted to the linker builder are library files
//	and they're never dirty.

bool
MLinkerBuilder::FileIsDirty(
	MProject& 		/*inProject*/,
	MFileRec& 		/*inFileRec*/,	
	MakeStageT		/*inStage*/,
	MakeActionT		/*inAction*/,
	time_t			/*inModDate*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		 BuildCompileArgv
// ---------------------------------------------------------------------------

status_t
MLinkerBuilder::BuildCompileArgv(
	MProject& 		inProject,
	BList& 			inArgv,
	MakeActionT 	inAction,
	MFileRec& 		inFileRec)
{
	if (inAction == kDisassemble)
	{
		BuildDisassembleArgv(inProject, inArgv, inFileRec.path);

		return B_NO_ERROR;
	}
	else
		return B_ERROR;
}

// ---------------------------------------------------------------------------
//		 BuildDisassembleArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when disassembling a library file.
//	This code is similar to but not identical to the code in MmwccBuilder.

void
MLinkerBuilder::BuildDisassembleArgv(
	MProject& 		inProject,
	BList&			inArgv,
	const char *	inFilePath)
{
	LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);

	// Build the command line options
	if (! cache->fDisassemblePrefs.pShowCode)			// default is show code
		inArgv.AddItem(strdup("-d"));
	else
	{
		if (! cache->fDisassemblePrefs.pUseExtended)	// default is use extended
		{
			inArgv.AddItem(strdup("-fmt"));
			inArgv.AddItem(strdup("nox"));
		}
		if (cache->fDisassemblePrefs.pOnlyOperands)		// default is off
			inArgv.AddItem(strdup("-h"));
	}
	if (! cache->fDisassemblePrefs.pShowData)			// default is show data
	{
		inArgv.AddItem(strdup("-nodata"));
	}
	else
	if (! cache->fDisassemblePrefs.pExceptionTable)		// default is on
	{
		inArgv.AddItem(strdup("-xtables"));
		inArgv.AddItem(strdup("off"));
	}
	if (cache->fDisassemblePrefs.pShowSYM)				// default is off
	{
		inArgv.AddItem(strdup("-sym"));
		inArgv.AddItem(strdup("on"));
	}
	if (! cache->fDisassemblePrefs.pShowNameTable)		// default is on
	{
		inArgv.AddItem(strdup("-nonames"));
	}

	inArgv.AddItem(strdup(inFilePath));
}

// ---------------------------------------------------------------------------
//		 SYMFileRef
// ---------------------------------------------------------------------------
//	return an entry_ref that points to the sym file for this project.
//	

status_t
MLinkerBuilder::GetSYMFileRef(
	MProject&	inProject,
	entry_ref&	outSYMFileRef)
{
	char			appName[B_FILE_NAME_LENGTH];
	char			path[500] = { 0 };
	entry_ref		appRef;
	status_t		err = B_ERROR;

	if (B_NO_ERROR == GetExecutableRef(inProject, appRef))
	{
		BEntry			app(&appRef);
		BDirectory		appDir;

		if (B_NO_ERROR == app.GetParent(&appDir))
		{
			BEntry		appDirEntry;

			if (B_NO_ERROR == appDir.GetEntry(&appDirEntry) &&
				B_NO_ERROR == app.GetName(appName))
			{
				BEntry		symEntry;
				String		symName = appName;
				symName += ".xSYM";
				err = appDir.FindEntry(symName, &symEntry);

				if (err == B_NO_ERROR)
					err = symEntry.GetRef(&outSYMFileRef);
			}
		}
	}

	return err;
}

// ---------------------------------------------------------------------------
//		 GetExecutableRef
// ---------------------------------------------------------------------------

status_t
MLinkerBuilder::GetExecutableRef(
		MProject& 	inProject,
		entry_ref& 	outExecutableRef)
{
	entry_ref		projectRef;
	status_t		err = inProject.GetProjectRef(projectRef);
	
	if (B_OK == err)
	{
		BEntry			project(&projectRef);
		BDirectory		projectDir;
		BEntry			executable;
	
		err = project.GetParent(&projectDir);
		if (err == B_NO_ERROR)
		{
			LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);
			err = projectDir.FindEntry(cache->fProjectPrefs.pAppName, &executable);
			if (err == B_NO_ERROR)
				err = executable.GetRef(&outExecutableRef);
		}
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		 GetBuiltAppPath
// ---------------------------------------------------------------------------
//	Place the application in the same folder as the project and give it
//	the name 'Application'.

status_t
MLinkerBuilder::GetBuiltAppPath(
	MProject&	inProject,
	char* 		outPath, 
	int32		/*inBufferLength*/)
{
	LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);
	if (cache->fProjectPrefs.pAppName[0] != 0)
		strcpy(outPath, cache->fProjectPrefs.pAppName);
	else
		strcpy(outPath, "Application");
		
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		 Launch
// ---------------------------------------------------------------------------

status_t
MLinkerBuilder::Launch(
	MProject&	inProject,
	bool		inRunWithDebugger)
{
	status_t	err;
	entry_ref	ref;

	if (inRunWithDebugger)
	{
		err = GetSYMFileRef(inProject, ref);
	}
	else
	{
		err = GetExecutableRef(inProject, ref);
	}
	
	if (err == B_OK)
		err = inProject.Launch(ref);
	
	return err;
}

// ---------------------------------------------------------------------------
//		 IsLaunchable
// ---------------------------------------------------------------------------

bool
MLinkerBuilder::IsLaunchable(MProject& inProject)
{
	LinkerProjectSettings* cache = fSettingsMap.GetSettings(inProject);
	return cache->fProjectPrefs.pProjectKind == AppType;
}

