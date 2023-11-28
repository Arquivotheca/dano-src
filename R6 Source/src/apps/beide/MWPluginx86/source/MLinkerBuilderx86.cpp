//========================================================================
//	MLinkerBuilderx86.cpp
//	Copyright 1996-97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MLinkerBuilderx86.h"
#include "MProject.h"
#include "PlugInPreferences.h"
#include "CString.h"

const char *	mwldName = "mwldx86";
const char *	mwlddisName = "mwdisx86";
const char *	mwldTargetName = "BeOS x86 C/C++";
const int32 kPathSize = 256;

// ---------------------------------------------------------------------------
//		¥ GetToolName
// ---------------------------------------------------------------------------

status_t
MLinkerBuilderx86::GetToolName(
	char* 		outName,
	int32		/*inBufferLength*/,
	MakeStageT	/*inStage*/,
	MakeActionT	inAction)
{
	switch (inAction)
	{
		case kDisassemble:
			strcpy(outName, mwlddisName);
			break;

		// For normal linking
		default:
			strcpy(outName, mwldName);
			break;
	}
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ TargetName
// ---------------------------------------------------------------------------

const char *
MLinkerBuilderx86::TargetName()
{
	return mwldTargetName;
}

// ---------------------------------------------------------------------------
//		¥ LinkerName
// ---------------------------------------------------------------------------

const char *
MLinkerBuilderx86::LinkerName()
{
	return mwldName;
}

// ---------------------------------------------------------------------------
//		¥ Actions
// ---------------------------------------------------------------------------

MakeActionT
MLinkerBuilderx86::Actions()
{
	return kLink;
}

// ---------------------------------------------------------------------------
//		¥ MakeStages
// ---------------------------------------------------------------------------

MakeStageT
MLinkerBuilderx86::MakeStages()
{
	return kLinkStage;
}

// ---------------------------------------------------------------------------
//		¥ Flags
// ---------------------------------------------------------------------------

ulong
MLinkerBuilderx86::Flags()
{
	return kIDEAware;
}

// ---------------------------------------------------------------------------
//		¥ MessageDataType
// ---------------------------------------------------------------------------

ulong
MLinkerBuilderx86::MessageDataType()
{
	return kMWLDx86Type;
}

// ---------------------------------------------------------------------------
//		¥ ValidateSettings
// ---------------------------------------------------------------------------
//	return true if something changed in the settings.

bool
MLinkerBuilderx86::ValidateSettings(
	BMessage&	inOutMessage)
{
	bool		changed = false;
	long		len;

	// Linker prefs
	LinkerPrefsx86*	linkerPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kLinkerPrefsx86, kMWLDx86Type, &linkerPrefs, &len))
	{
		if (B_LENDIAN_TO_HOST_INT32(linkerPrefs->pVersion) != kCurrentVersion)
		{
			LinkerPrefsx86		linker = *linkerPrefs;
			linker.SwapLittleToHost();
			linker.pVersion = kCurrentVersion;
			linker.SwapHostToLittle();
			inOutMessage.RemoveName(kLinkerPrefsx86);
			inOutMessage.AddData(kLinkerPrefsx86, kMWLDx86Type, &linker, sizeof(linker));
			changed = true;
		}
	}
	else
	{
		MDefaultPrefs::SetLinkerDefaultsx86(fLinkerPrefs);
		len = sizeof(fLinkerPrefs);
		fLinkerPrefs.SwapHostToLittle();			
		inOutMessage.AddData(kLinkerPrefsx86, kMWLDx86Type, &fLinkerPrefs, len);
		fLinkerPrefs.SwapLittleToHost();			
		changed = true;
	}

	// mwccx86 builder validates the project prefs

	return changed;
}

// ---------------------------------------------------------------------------
//		¥ BuildPrecompileArgv
// ---------------------------------------------------------------------------

int32
MLinkerBuilderx86::BuildPrecompileArgv(
	BList& 		/*inArgv*/,
	MFileRec& 	/*inFileRec*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ BuildPostLinkArgv
// ---------------------------------------------------------------------------

int32
MLinkerBuilderx86::BuildPostLinkArgv(
	BList& 		/*inArgv*/,
	MFileRec& 	/*inFileRec*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ BuildLinkArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when Linking.  

status_t
MLinkerBuilderx86::BuildLinkArgv(
	BList&			inArgv)
{
#ifdef LIMITED
	if (LinkerSize(true) != kMWLDSize)
		return B_ERROR;
#endif

	if (fLinkerPrefs.pGenerateSYMFile)
	{
		inArgv.AddItem(strdup("-sym"));

		if (fLinkerPrefs.pUseFullPath)
			inArgv.AddItem(strdup("full"));
		else
			inArgv.AddItem(strdup("on"));		
	}
	if (fLinkerPrefs.pGenerateLinkMap)
	{
		String		linkmap = fProjectPrefs.pAppName;

		linkmap += ".xMAP";
		inArgv.AddItem(strdup("-map"));
		inArgv.AddItem(strdup(linkmap));
	}
	if (fLinkerPrefs.pSuppressWarnings)
	{
		inArgv.AddItem(strdup("-warn"));
	}
#if 0
	if (fLinkerPrefs.pGenerateCVInfo)
	{
	}
#endif
	if (fProjectPrefs.pProjectKind != AppType)		// default is Application
	{
		switch (fProjectPrefs.pProjectKind)
		{
			case SharedLibType:
				inArgv.AddItem(strdup("-G"));
				break;

			case LibraryType:
				inArgv.AddItem(strdup("-xml"));
				break;
		}
	}

	// Set up the base address
	char		baseaddress[24];
	
	sprintf(baseaddress, "%p", fProjectPrefs.pBaseAddress);
	inArgv.AddItem(strdup("-imagebase"));
	inArgv.AddItem(strdup(baseaddress));

	// Check if the names for the entry points match the default names
	// and add them to the command line if they don't match
	const char *	mainName;
	
	// What are the names for the init, term and main entry points?
	switch (fProjectPrefs.pProjectKind)
	{
		case AppType:
			mainName = kDefaultAppMainName;
			break;
		case SharedLibType:
			mainName = kDefaultSharedLibMainName;
			break;
		case LibraryType:
			mainName = kDefaultLibMainName;
			break;
		default:
			ASSERT(false);
			break;
	}

	// default for an app is __start
	// for a shared lib, nothing
	// library has no entry point
	if (0 != strcmp(fLinkerPrefs.pMain, mainName))	
	{
		inArgv.AddItem(strdup("-m"));
		inArgv.AddItem(strdup(fLinkerPrefs.pMain));
	}
	if (fLinkerPrefs.pCommand[0] != '\0')	
	{
		inArgv.AddItem(strdup("-commandfile"));
		inArgv.AddItem(strdup(fLinkerPrefs.pCommand));
	}

	// Add all of the object files
	MFileRec		fileRec;
	BList			targetList;
	long			i = 0;

	// Ask the project object for all the target files
	// it asks the mwccbuilder object for them
	while (fProject->GetNthFile(fileRec, targetList, i++))
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

	GetBuiltAppPath(objectPath, kPathSize);
	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(objectPath));

	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ GetTargetFilePaths
// ---------------------------------------------------------------------------
//	In a linker builder the file itself is the target file.

void
MLinkerBuilderx86::GetTargetFilePaths(
	MFileRec& 		inFileRec,
	BList&			inOutTargetFileList)
{
	inOutTargetFileList.AddItem(strdup(inFileRec.path));
}

// ---------------------------------------------------------------------------
//		¥ MessageToPrefs
// ---------------------------------------------------------------------------
//	Copy the prefs from the BMessage to the prefs structs.

void
MLinkerBuilderx86::MessageToPrefs()
{
	BMessage	msg;
	long		len;
	
	// Get linker prefs
	fProject->GetPrefs(kMWLDx86Type, msg);
	
	LinkerPrefsx86*	linkerPrefs;
	if (B_NO_ERROR == msg.FindData(kLinkerPrefsx86, kMWLDx86Type, &linkerPrefs, &len))
	{
		fLinkerPrefs = *linkerPrefs;
		fLinkerPrefs.SwapLittleToHost();
	}
	
	// Get Project prefs
	msg.MakeEmpty();
	fProject->GetPrefs(kMWCCx86Type, msg);

	ProjectPrefsx86*	projPrefs;
	if (B_NO_ERROR == msg.FindData(kProjectPrefsx86, kMWCCx86Type, &projPrefs, &len))
	{
		fProjectPrefs = *projPrefs;
		fProjectPrefs.SwapLittleToHost();
	}
}

// ---------------------------------------------------------------------------
//		¥ ProjectChanged
// ---------------------------------------------------------------------------

void
MLinkerBuilderx86::ProjectChanged(
	ChangeT		inChange,
	MProject*	inProject)
{
	switch (inChange)
	{
		case kProjectOpened:
			fProject = inProject;
			MessageToPrefs();
			break;

		case kProjectClosed:
			fProject = nil;
			break;

		case kPrefsChanged:
			MessageToPrefs();
			break;

		case kRunMenuItemChanged:
			RunMenuItemChanged();
			break;

		case kLinkDone:
			LinkDone();
			break;

		case kFilesAdded:	
		case kFilesRemoved:	
		case kFilesRearranged:	
		case kBuildStarted:	
			// do nothing for now
			break;
	}
}

// ---------------------------------------------------------------------------
//		¥ LinkDone
// ---------------------------------------------------------------------------
//	If the project is using the debugger and generate sym file is off,
//	then turn on generate sym file.

void MLinkerBuilderx86::LinkDone()
{
	status_t		err;
	entry_ref		executableRef;

	if (B_NO_ERROR == GetExecutableRef(executableRef))
	{
		if (0 == strcmp(fProjectPrefs.pAppType, B_APP_MIME_TYPE))
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
			err = chmod(fProjectPrefs.pAppName, S_IRWXU + S_IRWXG + S_IRWXO);
		}
		
		// Set the file type
		BFile			file(&executableRef, B_READ_WRITE);
		BAppFileInfo	info(&file);
		
		info.SetType(fProjectPrefs.pAppType);
		// should also set the signature maybe ????
	}
}

// ---------------------------------------------------------------------------
//		¥ RunMenuItemChanged
// ---------------------------------------------------------------------------
//	If the project is using the debugger and generate sym file is off,
//	then turn on generate sym file.

void MLinkerBuilderx86::RunMenuItemChanged()
{
	if (fProject->RunsWithDebugger() && ! fLinkerPrefs.pGenerateSYMFile)
	{
		fLinkerPrefs.pGenerateSYMFile = true;

		BMessage	msg;
		
		msg.AddData(kLinkerPrefs, kMWLDType, &fLinkerPrefs, sizeof(fLinkerPrefs));

		fProject->SetPrefs(msg, kLinkUpdate);
	}
}

// ---------------------------------------------------------------------------
//		¥ ParseMessageText
// ---------------------------------------------------------------------------

status_t
MLinkerBuilderx86::ParseMessageText(
	const char*	/*text*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ GenerateDependencies
// ---------------------------------------------------------------------------

status_t
MLinkerBuilderx86::GenerateDependencies(
	const char*	/*inFilePath*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ CodeDataSize
// ---------------------------------------------------------------------------

void
MLinkerBuilderx86::CodeDataSize(
	const char* /*inFilePath*/,
	int32&	outCodeSize,
	int32&	outDataSize)
{
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------
//		¥ FileIsDirty
// ---------------------------------------------------------------------------
//	the only files targetted to the linker builder are library files
//	and they're never dirty.

bool
MLinkerBuilderx86::FileIsDirty(
	MFileRec& 		/*inFileRec*/,	
	MakeStageT		/*inStage*/,
	MakeActionT		/*inAction*/,
	time_t			/*inModDate*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		¥ BuildCompileArgv
// ---------------------------------------------------------------------------

status_t
MLinkerBuilderx86::BuildCompileArgv(
	BList& 			inArgv,
	MakeActionT 	inAction,
	MFileRec& 		inFileRec)
{
	if (inAction == kDisassemble)
	{
		BuildDisassembleArgv(inArgv, inFileRec.path);

		return B_NO_ERROR;
	}
	else
		return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ BuildDisassembleArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when disassembling a library file.
//	The x86 linker doesn't dissasemble libraries.

void
MLinkerBuilderx86::BuildDisassembleArgv(
	BList&			inArgv,
	const char *	inFilePath)
{
	inArgv.AddItem(strdup(inFilePath));
}

// ---------------------------------------------------------------------------
//		¥ SYMFileRef
// ---------------------------------------------------------------------------
//	return an entry_ref that points to the sym file for this project.
//	

status_t
MLinkerBuilderx86::GetSYMFileRef(
	entry_ref& outSYMFileRef)
{
	char			appName[B_FILE_NAME_LENGTH];
	char			path[500] = { 0 };
	entry_ref		appRef;
	status_t		err = B_ERROR;

	if (B_NO_ERROR == GetExecutableRef(appRef))
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
//		¥ GetExecutableRef
// ---------------------------------------------------------------------------

status_t
MLinkerBuilderx86::GetExecutableRef(
		entry_ref& outExecutableRef)
{
	entry_ref		projectRef;
	status_t		err = fProject->GetProjectRef(projectRef);
	
	if (B_OK == err)
	{
		BEntry			project(&projectRef);
		BDirectory		projectDir;
		BEntry			executable;
	
		err = project.GetParent(&projectDir);
		if (err == B_NO_ERROR)
		{
			err = projectDir.FindEntry(fProjectPrefs.pAppName, &executable);
			if (err == B_NO_ERROR)
				err = executable.GetRef(&outExecutableRef);
		}
	}
	
	return err;
}

// ---------------------------------------------------------------------------
//		¥ GetBuiltAppPath
// ---------------------------------------------------------------------------
//	Place the application in the same folder as the project and give it
//	the name 'Application'.

status_t
MLinkerBuilderx86::GetBuiltAppPath(
	char* 	outPath, 
	int32	/*inBufferLength*/)
{
	if (fProjectPrefs.pAppName[0] != 0)
		strcpy(outPath, fProjectPrefs.pAppName);
	else
		strcpy(outPath, "Application");
		
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ Launch
// ---------------------------------------------------------------------------

status_t
MLinkerBuilderx86::Launch(
	bool	inRunWithDebugger)
{
	status_t	err;
	entry_ref	ref;

	if (inRunWithDebugger)
	{
		err = GetSYMFileRef(ref);
	}
	else
	{
		err = GetExecutableRef(ref);
	}
	
	if (err == B_OK)
		err = fProject->Launch(ref);
	
	return err;
}

// ---------------------------------------------------------------------------
//		¥ IsLaunchable
// ---------------------------------------------------------------------------

bool
MLinkerBuilderx86::IsLaunchable()
{
	return B_HOST_IS_LENDIAN && fProjectPrefs.pProjectKind == AppType;
}

