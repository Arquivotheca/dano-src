//========================================================================
//	MFileUtils.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#include <string.h>

#include "MFileUtils.h"
#include "MInformationMessageWindow.h"
#include "IDEMessages.h"
#include "MMessageItem.h"

#include <StorageKit.h>
#include <KernelKit.h>
#include <Application.h>
#include <Roster.h>
#include <String.h>

BDirectory		MFileUtils::sSystemDirectory;
BDirectory		MFileUtils::sToolsDirectory;
BDirectory		MFileUtils::sBinDirectory;
String			MFileUtils::sBinPath;
String			MFileUtils::sTmpPath;
dev_t			MFileUtils::sBootDevice;

const AccessPathData		accessData[] =
{
{ kAbsolutePath, true, "/boot/develop/headers/be"  },
{ kAbsolutePath, false, "/boot/develop/headers/cpp"  },
{ kAbsolutePath, false, "/boot/develop/headers/posix"  },
{ kAbsolutePath, false, "/boot/develop/lib"  },
{ kAbsolutePath, false, "/boot/beos/system/lib"  },
{ (AccessPathT) 255, false, ""  }
};

// ---------------------------------------------------------------------------
//		MFileUtils
// ---------------------------------------------------------------------------
//	Get the system directory.

MFileUtils::MFileUtils()
{
	app_info		info;

	be_app->GetAppInfo(&info);

	BEntry 			item(&info.ref);

	item.GetParent(&sSystemDirectory);
	status_t		err = FindDirectory(sSystemDirectory, kToolsFolderName, &sToolsDirectory);

	// Save bin directory
	BPath			path;
	err = find_directory(B_BEOS_BIN_DIRECTORY, &path, true);
	ASSERT(err == B_NO_ERROR);

	if (err == B_OK)	// report error ????
	{
		sBinDirectory.SetTo(path.Path());
		sBinPath = path.Path();
		
		node_ref		binRef;
		
		if (B_OK == sBinDirectory.GetNodeRef(&binRef))
			sBootDevice = binRef.device;
	}

	// Save tmp directory
	err = find_directory(B_COMMON_TEMP_DIRECTORY, &path, true);
	ASSERT(err == B_NO_ERROR);

	if (err == B_OK)
	{
		sTmpPath = path.Path();
	}
}

// ---------------------------------------------------------------------------
//		Parenthesized
// ---------------------------------------------------------------------------

bool
MFileUtils::Parenthesized(
	const BDirectory&		inDir)
{
	FileNameT		name;
	BEntry			entry;
	
	status_t		err = inDir.GetEntry(&entry);
	
	return (B_NO_ERROR == entry.GetName(name) &&
		name[0] == '(' && 
		name[strlen(name) - 1] == ')');
}

// ---------------------------------------------------------------------------
//		Parenthesized
// ---------------------------------------------------------------------------

bool
MFileUtils::Parenthesized(
	const BEntry&		inEntry)
{
	FileNameT		name;
	
	return (B_NO_ERROR == inEntry.GetName(name) &&
		name[0] == '(' && 
		name[strlen(name) - 1] == ')');
}

// ---------------------------------------------------------------------------
//		Parenthesized
// ---------------------------------------------------------------------------

bool
MFileUtils::Parenthesized(
	const char *	inName)
{
	return (inName[0] == '(' && inName[strlen(inName) - 1] == ')');
}

// ---------------------------------------------------------------------------
//		AlreadyScanned
// ---------------------------------------------------------------------------
//	Look through the nodelist for a node_ref.  This prevents us from
//	following circular symlinks.  The nodelist could store the nodes in
//	a way that was faster to find ????

bool
MFileUtils::AlreadyScanned(
	const BDirectory&		inDir,
	NodeList&				inNodeList)
{
	bool		found = false;
	node_ref	dirRef;
	node_ref*	nodeRef;
	int32		i = 0;
	
	if (B_OK == inDir.GetNodeRef(&dirRef))
	{
		while (inNodeList.GetNthItem(nodeRef, i++))
		{
			if (dirRef == *nodeRef)
			{
				found = true;
				break;
			}
		}
	}

	return found;
}

// ---------------------------------------------------------------------------
//		FindFileInDirectoryList
// ---------------------------------------------------------------------------
//	Do a search for the presence of this file in this directory list.
//	The file is specified only by the file name and the directory.
//	The search is deep or shallow depending on the last parameter passed.
//	Note the default parameter.

bool
MFileUtils::FindFileInDirectoryList(
	const char* 			inFileName,
	const DirectoryList&	inList,
	entry_ref&				outEntry)
{
	DirectoryInfo*	info;
	bool			found = false;
	int32			i = 0;
	BEntry			resultEntry;
	NodeList		list;
	bool			symLinkFound = false;

	while (inList.GetNthItem(info, i++))
	{
		BDirectory dir(&info->dDir);
		if (!symLinkFound || ! AlreadyScanned(dir, list))
		{
			if (info->dRecursiveSearch)
			{
				if (FindFileInDirectory(inFileName, dir, resultEntry, list, symLinkFound))
				{
					found = true;
					break;
				}
			}
			else
			if (B_NO_ERROR == dir.FindEntry(inFileName, &resultEntry))
			{
				found = true;
				break;
			}
		}
	}

	EmptyNodeList(list);

	if (found)
	{
		if (B_NO_ERROR != resultEntry.GetRef(&outEntry))
			found = false;
	}
		
	return found;
}

// ---------------------------------------------------------------------------
//		FindFileInDirectory
// ---------------------------------------------------------------------------
//	Do a deep search for the presence of this file in this directory.
//	The file is specified only by the file name and the directory.

bool
MFileUtils::FindFileInDirectory(
	const char* 		inFileName,
	BDirectory& 		inDir,
	BEntry&				outEntry,
	NodeList&			inNodeList,
	bool&				inoutSymLinkFound)
{
	bool		found = false;
	status_t	err;
	
	if (B_NO_ERROR == inDir.FindEntry(inFileName, &outEntry) && outEntry.IsFile())
	{
		found = true;
	}
	else
	{
		BEntry			entry;
		BDirectory		dir;
		myDirent		dirents[2];
		int32			count;
		node_ref*		nodeRef = new node_ref;
		
		inDir.GetNodeRef(nodeRef);
		inNodeList.AddItem(nodeRef);

		while ((count = inDir.GetNextDirents(dirents, sizeof(dirents))) > 0)
		{
			dirent*			ent = dirents;
			
			for (int i = 0; i < count; i++)
			{
				if (0 != strcmp(ent->d_name, ".") && 0 != strcmp(ent->d_name, ".."))
				{
					struct stat		rec;
					inDir.GetStatFor(ent->d_name, &rec);

					if (S_ISDIR(rec.st_mode) && ! Parenthesized(ent->d_name))
					{
						err = dir.SetTo(&inDir, ent->d_name);
						if (!inoutSymLinkFound || ! AlreadyScanned(dir, inNodeList))
						{
							if (FindFileInDirectory(inFileName, dir, outEntry, inNodeList, inoutSymLinkFound))
							{
								return true;		// early exit
							}
						}
					}
					else
					if (S_ISLNK(rec.st_mode))
					{
						BPath		path;
						
						err = entry.SetTo(&inDir, ent->d_name, true);			// traverse

						if (err == B_OK && B_OK == entry.GetPath(&path))
						{
							if (entry.IsDirectory())
							{
								if (! MFileUtils::Parenthesized(path.Leaf()))
								{
									BDirectory		aDir(&entry);
									if (!inoutSymLinkFound || ! AlreadyScanned(dir, inNodeList))
									{
										inoutSymLinkFound = true;
										if (FindFileInDirectory(inFileName, aDir, outEntry, inNodeList, inoutSymLinkFound))
										{
											return true;		// early exit
										}
									}
								}
							}
							else
							if (entry.IsFile())
							{
								if (0 == strcmp(inFileName, path.Leaf()))
								{
									outEntry = entry;
									return true;		// early exit
								}
							}
						}
					}
				}
			}
		}
	}

	return found;
}

void
MFileUtils::EmptyNodeList(
	NodeList&		inNodeList)
{
	// Delete all the noderefs
	node_ref*		nodeRef;
	int32			i = 0;

	while (inNodeList.GetNthItem(nodeRef, i++))
	{
		delete nodeRef;
	}
}

// ---------------------------------------------------------------------------
//		FindFileInDirectory
// ---------------------------------------------------------------------------
//	Do a deep search for the presence of this file in this directory.
//	The file is specified only by the file name and the directory.
//	The inForce parameter indicates whether inDirectory should be searched
//	regardless of whether it is parenthesized.  This should normally be true
//	when starting a search at the top level of an access path.  Directories
//	inside the top level directory will always be guarded by parentheses
//	even if inForce is true.  Note the default parameter so that inForce
//	shouild normally not ever need to be set by client code.

bool
MFileUtils::FindFileInDirectory(
	const char* 		inFileName,
	BDirectory& 		inDirectory,
	BEntry&				outEntry)
{
	NodeList		list;
	bool			symLinkFound = false;
	bool			result = FindFileInDirectory(inFileName, inDirectory, outEntry, list, symLinkFound);

	EmptyNodeList(list);

	return result;
}

// ---------------------------------------------------------------------------
//		IsDirectory
// ---------------------------------------------------------------------------
//	Does the specified ref refer to a directory.

bool
MFileUtils::IsDirectory(
	const entry_ref&	inRef)
{
	BEntry		dir(&inRef);
	
	return dir.IsDirectory();
}

// ---------------------------------------------------------------------------
//		DirectoryFromFullPath
// ---------------------------------------------------------------------------

bool
MFileUtils::DirectoryFromFullPath(const char* inPathName, entry_ref& outRef)
{
	bool result = false;
	if (get_ref_for_path(inPathName, &outRef) == B_NO_ERROR) {
		BDirectory testDir(&outRef);
		if (testDir.InitCheck() == B_OK) {		// Does it exist?
			result = true;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		DirectoryFromRelPath
// ---------------------------------------------------------------------------
//	Given a directory and a relative path from it, return a BDirectory
//	object that corresponds to the final directory or nil if it doesn't exist.
//  The relative path may be of the forms:
//	../../                 to indicate up two directories
//	../../folder1/folder2  to indicate up two and down two into folder2
//	/folder3/folder4       to indicate down two directories into folder4

bool
MFileUtils::DirectoryFromRelPath(const char* inToPath, BDirectory& inFromDir, entry_ref& outRef)
{
	const char* leaf = inToPath;
	// when specifying a partial path in leaf, BPath doesn't like it
	// to start with a slash
	if (leaf && leaf[0] == '/') {
		leaf++;
	}
	
	BPath fullPath(&inFromDir, leaf);
	// If the project has been moved around, we can create a totally
	// bogus path here which results in the fullPath being nil.
	// Test against that case because get_ref_for_path doesn't
	// like being passed a nil.
	if (fullPath.Path() == nil) {
		return false;
	}
	else {
		return DirectoryFromFullPath(fullPath.Path(), outRef);
	}
}

// ---------------------------------------------------------------------------
//		BuildRelativePath
// ---------------------------------------------------------------------------
//	returns false if dirs are on diff volumes
//	Works correctly if one dir is inside the other.
//	Output looks like:
//	from:   /Disk/Folder1/Folder2
//	to:     /Disk/Folder1/Folder3
//	result: ../Folder3
//	from:   /Disk/Folder1/Folder2
//	to:     /Disk/Folder1
//	result: ../
//	from:   /Disk/Folder1
//	to:     /Disk/Folder1/Folder2
//	result: /Folder2

bool
MFileUtils::BuildRelativePath(
	BDirectory&		inFromDir,
	BDirectory&		inToDir,
	char*			outPath)
{
	bool 			result = false;
	char			fromPath[kPathSize];
	char			toPath[kPathSize];
	BPath			fromPathObject;
	BPath			toPathObject;
	int32			fork = 0L;		// offset of the slash
	int32			len = 1;
	BEntry			fromEntry;
	BEntry			toEntry;
	
	if (B_NO_ERROR == inFromDir.GetEntry(&fromEntry) &&
		B_NO_ERROR == inToDir.GetEntry(&toEntry) &&
		B_NO_ERROR == fromEntry.GetPath(&fromPathObject) &&
		B_NO_ERROR == toEntry.GetPath(&toPathObject))
	{
		// These paths are not terminated by a slash
		strcpy(fromPath, fromPathObject.Path());
		strcpy(toPath, toPathObject.Path());
		strcat(fromPath, "/");			// Terminate them temporarily
		strcat(toPath, "/");
		int32 term = strlen(fromPath);
		
		// Find the last directory these two
		// paths have in common
		while (memcmp(fromPath, toPath, len) == 0) {
			fork = len;
			char* next = strchr(fromPath + fork, '/');
			if (next == 0) {
				break;
			}
			// bump next to include slash in compare
			next += 1;
			len = next - fromPath;
		}
		
		outPath[0] = 0;
	
		// Is the fork after the leading slash?
		if (fork > 1)
		{
			// Add the uppath tokens
			char* ptr = fromPath + fork;
			while ((ptr = strchr(ptr, '/')) != nil) {
				strcat(outPath, "../");	
				// bump past the slash 
				ptr += 1;
			}
			
			// Add the downpath folder names			
			// If we are just appending to fromPath, then start with a slash
			// (I don't think this really belongs, we should treat the 
			// {project} or whatever as ending in a slash.  The uppaths
			// do, since they don't start as {project}/../etc.  But to
			// remain compatible with current accesspath strings, I put it in. 
			// John Dance - 4/16/99)
			if (strlen(outPath) == 0) {
				strcat(outPath, "/");
			}
			
			toPath[strlen(toPath) - 1] = 0;		// remove extra slash from toPath
			strcat(outPath, toPath + fork);
			
			result = true;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		BuildDirectoriesList
// ---------------------------------------------------------------------------
//	Take all the paths in the paths list and add a BDirectory* to the directories
//	list for each of them.

void
MFileUtils::BuildDirectoriesList(
	const AccessPathList&	inPathsList,
	DirectoryList&			inDirectoryList,
	BDirectory*				inProjectDirectory)
{
	AccessPathData* accessPath;
	DirectoryInfo* dirInfo;
	int32 i = 0;
	bool pathOK = false;
	entry_ref pathRef;
	
	while (inPathsList.GetNthItem(accessPath, i++))
	{
		switch (accessPath->pathType)
		{
			case kAbsolutePath:
				pathOK = MFileUtils::DirectoryFromFullPath(accessPath->pathName, pathRef);
				break;
			
			case kProjectRelativePath:
				if (inProjectDirectory != nil)
					pathOK = MFileUtils::DirectoryFromRelPath(accessPath->pathName, *inProjectDirectory, pathRef);
				else
					pathOK = false;
				break;
			
			case kSystemRelativePath:
				pathOK = MFileUtils::DirectoryFromRelPath(accessPath->pathName, MFileUtils::SystemDirectory(), pathRef);
				break;
			
			case kPathToProjectTree:
			{
				BEntry entry;
				inProjectDirectory->GetEntry(&entry);
				pathOK = (entry.GetRef(&pathRef) == B_OK);
				break;
			}
			
			case kPathToSystemTree:
			{
				BEntry entry;
				MFileUtils::SystemDirectory().GetEntry(&entry);
				pathOK = (entry.GetRef(&pathRef) == B_OK);
				break;
			}
		}
		
		if (pathOK)
		{
			dirInfo = new DirectoryInfo;
			dirInfo->dDir = pathRef;
			dirInfo->dRecursiveSearch = accessPath->recursiveSearch;
			inDirectoryList.AddItem(dirInfo);
		}
		else
		if (! (accessPath->pathType == kPathToProjectTree &&
			inProjectDirectory == nil))
		{
			// Post a message to the message window
			BString text = "Couldn't find an access path.\n";
			switch (accessPath->pathType) {
				case kProjectRelativePath:
					text += kProjectPathName;
					break;
				
				case kSystemRelativePath:
					text += kSystemPathName;
					break;
			}
			text += accessPath->pathName;
	
			InfoStruct info;
			info.iTextOnly = true;
			strncpy(info.iLineText, text.String(), kLineTextLength);
			info.iLineText[kLineTextLength-1] = '\0';
			BMessage msg(msgAddInfoToMessageWindow);
	
			msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));
	
			MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
			MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);
		}
	}
}

// ---------------------------------------------------------------------------
//		EmptyDirectoryList
// ---------------------------------------------------------------------------

void
MFileUtils::EmptyDirectoryList(
	DirectoryList&				inDirectoryList,
	const BDirectory * const	inProjectDirectory)
{
	DirectoryInfo*		dirInfo;
	int32				i = 0;

	while (inDirectoryList.GetNthItem(dirInfo, i++)) {
		delete dirInfo;
	}
	
	inDirectoryList.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		AddAccessPathsToMessage
// ---------------------------------------------------------------------------

void
MFileUtils::AddAccessPathsToMessage(
	BMessage&				inOutMessage,
	const AccessPathList&	inSystemPathList,
	const AccessPathList&	inProjectPathList)
{
	// Add both sets of access paths to the message
	int32				count = inSystemPathList.CountItems() + inProjectPathList.CountItems();

	if (count > 0 )
	{
		AccessPathData*		accessPathArray;
		AccessPathData*		accessPath;
		int32				i = 0;
		int32				j = 0;
		int32				size = count * sizeof(AccessPathData);

		accessPathArray = new AccessPathData[count];

		while (inSystemPathList.GetNthItem(accessPath, i++))
		{
			accessPathArray[j++] = *accessPath;
		}
		
		i = 0;
		while (inProjectPathList.GetNthItem(accessPath, i++))
		{
			accessPathArray[j++] = *accessPath;
		}
		
		if (inOutMessage.HasData(kAccessPathsData, kMWPrefs))
			inOutMessage.ReplaceData(kAccessPathsData, kMWPrefs, accessPathArray, size);
		else
			inOutMessage.AddData(kAccessPathsData, kMWPrefs, accessPathArray, size, false);
			
		delete [] accessPathArray;
	}
}

// ---------------------------------------------------------------------------
//		FixUpSystemPaths
// ---------------------------------------------------------------------------
// Remove the {beide} recursive path and replace it with other paths.

void
FixUpSystemPaths(
	AccessPathsPrefs&	inPathsPrefs,
	AccessPathList&		inPathList)
{
	AccessPathData*		path;
	int32				count = inPathsPrefs.pSystemPaths;
	int32				i = inPathList.CountItems() - 1;

	while (inPathList.GetNthItem(path, i))
	{
		if (path->pathType == kPathToSystemTree ||
			path->pathType == kSystemRelativePath ||
			! NotFound(path, inPathList))	// remove and readd it below, this puts it into the order that I want in the access paths view
		{
			inPathList.RemoveItemAt(i);		// remove obsolete path
			count--;
		}
		
		i--;
	}

	int32		j = 0;

	while (accessData[j].pathType <= kPathToSystemTree)	// look for sentinal
	{
		if (NotFound(&accessData[j], inPathList))		// add new path
		{
			AccessPathData*		newpath = new AccessPathData;
			
			memset(newpath, '\0', sizeof(AccessPathData));
			newpath->pathType = accessData[j].pathType;
			newpath->recursiveSearch = accessData[j].recursiveSearch;
			strcpy(newpath->pathName, accessData[j].pathName);

			inPathList.AddItem(newpath);
			count++;
		}
		j++;
	}

	inPathsPrefs.pSystemPaths = count;
	inPathsPrefs.pVersion = kCurrentAccessPathsVersion;
	
	ASSERT(count == inPathList.CountItems());
}

// ---------------------------------------------------------------------------
//		NotFound
// ---------------------------------------------------------------------------
// Don't want to add paths that are already there

bool
NotFound(
	const AccessPathData*	inPath,
	const AccessPathList&	inPathList)
{
	bool				found = false;
	int32				i = 0;
	AccessPathData*		path;

	while (inPathList.GetNthItem(path, i++))
	{
		if (path->pathType == inPath->pathType &&
			path->recursiveSearch == inPath->recursiveSearch &&
			0 == strcmp(path->pathName, inPath->pathName))
		{
			found = true;
			break;
		}
	}

	return ! found;
}

// ---------------------------------------------------------------------------
//		FileIsInDirectoriesList
// ---------------------------------------------------------------------------
//	return true if the file is contained in one of the directories in the list.
//	Takes into account whether the directories in the list are to be searched
//	recursively or not.

bool
MFileUtils::FileIsInDirectoriesList(
	DirectoryList&		inDirectoryList,
	const BEntry&		inFile,
	DirectoryInfo*&		outDirectory)
{
	bool			found = false;
	status_t		err = B_NO_ERROR;
	BDirectory		filePath;
	BEntry			pathEntry;
	
	err = inFile.GetParent(&filePath);		// start at the file's directory
	// If a directory is parenthesized then we don't search any more

	while (err == B_NO_ERROR && ! found) {
		err = filePath.GetEntry(&pathEntry);
	
		if (err == B_NO_ERROR) {
			if (Parenthesized(pathEntry)) {
				break;
			}
			
			DirectoryInfo* info;
			int32 count = 0;
			entry_ref pathRef;
			pathEntry.GetRef(&pathRef);
			// Search all the directories in the directory list
			while (inDirectoryList.GetNthItem(info, count++)) {
				if (pathRef == info->dDir) {
					found = true;
					outDirectory = info;
					break;
				}
			}
			
			if (filePath.IsRootDirectory())	{		// done after checking the root
				break;
			}
			if (! found) {
				err = pathEntry.GetParent(&filePath);	// move up one directory
			}
		}
	}

	return found;
}

// ---------------------------------------------------------------------------
//		GetAccessPathsFromMessage
// ---------------------------------------------------------------------------

void
MFileUtils::GetAccessPathsFromMessage(
	const BMessage&		inOutMessage,
	AccessPathList&		inSystemPathList,
	AccessPathList&		inProjectPathList,
	int32				inSystemPathCount,
	int32				inProjectPathCount)
{
	// Get all the paths from the message
	int32				count;
	int32				j = 0;
	int32				i;
	ssize_t				length;
	AccessPathData*		newAccessPath;

	// Delete our existing paths
	MAccessPathsView::EmptyList(inSystemPathList);
	MAccessPathsView::EmptyList(inProjectPathList);

	AccessPathData*		accessPathArray;

	if (B_NO_ERROR == inOutMessage.FindData(kAccessPathsData, kMWPrefs, (const void **)&accessPathArray , &length))
	{
		// Get the system paths
		if (inSystemPathCount > 0)
		{
			count = inSystemPathCount;
			for (i = 0; i < count; i++)
			{
				newAccessPath = new AccessPathData;
				*newAccessPath = accessPathArray[j++];
				inSystemPathList.AddItem(newAccessPath);
			}
		}
		
		// Get the project paths
		if (inProjectPathCount > 0)
		{
			count = inProjectPathCount;
			for (i = 0; i < count; i++)
			{
				newAccessPath = new AccessPathData;
				*newAccessPath = accessPathArray[j++];
				inProjectPathList.AddItem(newAccessPath);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		GetTempFile
// ---------------------------------------------------------------------------
//	Get a temp file object or just the index for a temp file in
//	our list of temp files.  The index will not be duplicated by
//	this function.  The only possibility of failure is if another
//	app were to generate a temp file with the same name 
//	concurrently.  Note default params.
//	If inReferencefile is nil the temp file goes on the boot volume.

int32
MFileUtils::GetTempFile(
	BEntry&			inOutFile,
	const BEntry *	inReferenceFile,	// The temp file must be on the same volume
	bool			inCreate)
{
	static int32 	count = 0;
	status_t		err;
	int32			localCount;
	String			fileName;
	BDirectory		tempDir;
	
	if (inReferenceFile == nil)
	{
		// Get the tmp directory on the boot volume
		tempDir.SetTo(BootTmp());
	}
	else
	{
		ASSERT(inReferenceFile->IsFile());
		// Get the tmp directory on the volume the inReference file is on
		entry_ref		ref;
		
		err = inReferenceFile->GetRef(&ref);

		if (B_NO_ERROR == err)
		{
			BVolume		vol(ref.device);

			if (ref.device == BootDev())
			{
				tempDir.SetTo(BootTmp());
			}
			else
			{
				BDirectory	root;
	
				err = vol.GetRootDirectory(&root);
				if (B_NO_ERROR == err)
				{
					if (B_NO_ERROR != FindDirectory(root, "tmp", &tempDir))
						err = root.CreateDirectory("tmp", &tempDir);
				}
			}
		}
	}

	bool	exists = true;

	do {
		localCount = atomic_add(&count, 1);
		fileName = "mwtmp";
		fileName += localCount;
		if (! inCreate)
			exists = tempDir.Contains(fileName);
		else
		{
			// Try to create the file
			BFile	file(&tempDir, fileName, B_READ_WRITE | B_CREATE_FILE | B_FAIL_IF_EXISTS);
			if (B_OK == file.InitCheck() &&
				B_OK == tempDir.FindEntry(fileName, &inOutFile))
			{
				exists = false;
			}
		}
	} while (exists);

	return localCount;
}

// ---------------------------------------------------------------------------
//		CopyAttrs
// ---------------------------------------------------------------------------
//	Copy the attributes from one node to another.

status_t
CopyAttrs(
	BNode&	inFrom,
	BNode&	inTo)
{
	attr_name_t		attrname;
	status_t		err;
	char*			buffer = nil;
	off_t			bufferSize = 0;
	const off_t		kMinBufferSize = 1024;
	
	inFrom.RewindAttrs();
	inTo.RewindAttrs();
	
	while (B_NO_ERROR == inFrom.GetNextAttrName(attrname))
	{
		attr_info		fromInfo;
		attr_info		toInfo;
		ssize_t			size;

		err = inFrom.GetAttrInfo(attrname, &fromInfo);
		
		if (err == B_NO_ERROR)
		{
			if (buffer == nil || fromInfo.size > bufferSize)
			{
				bufferSize = max(fromInfo.size, kMinBufferSize);
				delete[] buffer;
				buffer = nil;
				buffer = new char[bufferSize];
				ASSERT(buffer != nil);
			}
			
			ssize_t		sizeRead = inFrom.ReadAttr(attrname, fromInfo.type, 0, buffer, fromInfo.size);

			err = inTo.GetAttrInfo(attrname, &toInfo);
			// Attribute already exists
			if (err == B_NO_ERROR && fromInfo.size > toInfo.size)
				err = inTo.RemoveAttr(attrname);
			if (err == B_NO_ERROR || err == ENOENT)	// ENOENT means entry doesn't exist
			{
				// Attribute doesn't already exist
				size = inTo.WriteAttr(attrname, fromInfo.type, 0, buffer, fromInfo.size);
				err = size != fromInfo.size ? B_ERROR : B_NO_ERROR;		
			}
		}
	}

	delete[] buffer;
	
	return err;
}

// ---------------------------------------------------------------------------
//		CopyFile
// ---------------------------------------------------------------------------
//	Copy the contents of one file to another, including attributes.  Assumes
//	that the toFile is empty.

status_t
CopyFile(
	BFile&	inFrom,
	BFile&	inTo)
{
	const size_t 	chunkSize = 64 * 1024;
	char*			buffer = new char[chunkSize];
	ssize_t			readSize = 0;
	status_t		err = B_OK;

	while ((readSize = inFrom.Read(buffer, chunkSize)) > 0)
	{
		inTo.Write(buffer, readSize);
	}

	delete [] buffer;
	
	err = CopyAttrs(inFrom, inTo);

	return err;
}

// ---------------------------------------------------------------------------
//		CopyDirectory
// ---------------------------------------------------------------------------
//	Copy the contents of one directory to another.  Files are copied, 
//	directories are recursively copied, and symlinks are copied, not followed.

status_t
CopyDirectory(
	BDirectory&	inFrom,
	BDirectory&	inTo)
{
	status_t		err = B_OK;
	BFile			toFile;
	BFile			fromFile;
	myDirent		dirents[2];
	int32			count;

	inFrom.Rewind();

	while ((count = inFrom.GetNextDirents(dirents, sizeof(dirents))) > 0)
	{
		dirent*			ent = dirents;
		
		for (int i = 0; i < count; i++)
		{
			if (0 != strcmp(ent->d_name, ".") && 0 != strcmp(ent->d_name, ".."))
			{
				struct stat		rec;
				inFrom.GetStatFor(ent->d_name, &rec);

				if (S_ISREG(rec.st_mode))	// regular file
				{
					// if we are copying into an existing directory
					// don't blow current files away (true = failIfExist)
					err = inTo.CreateFile(ent->d_name, &toFile, true);
					if (err == B_OK) {
						err = fromFile.SetTo(&inFrom, ent->d_name, B_READ_ONLY);
						err = CopyFile(fromFile, toFile);
						toFile.SetCreationTime(rec.st_crtime);	// restore creation time
						toFile.SetPermissions(rec.st_mode);		// restore permissions
					}
				}
				else
				if (S_ISDIR(rec.st_mode))	// directory
				{
					BDirectory		newDir;
					BDirectory		existingDir(&inFrom, ent->d_name);
					err = inTo.CreateDirectory(ent->d_name, &newDir);
					err = CopyDirectory(existingDir, newDir);
				}
				else
				if (S_ISLNK(rec.st_mode))		// symlink
				{
					BSymLink		link(&inFrom, ent->d_name);
					BSymLink		temp;
					char			path[B_PATH_NAME_LENGTH];
					
					link.ReadLink(path, B_PATH_NAME_LENGTH);
					err = inTo.CreateSymLink(ent->d_name, path, &temp);
				}
			}
		
			ent = (dirent*)((char*)ent + ent->d_reclen);
		}
	}
		
	return err;
}

// ---------------------------------------------------------------------------
//		KillTempFile
// ---------------------------------------------------------------------------
//	Delete the file from disk.

void
MFileUtils::KillTempFile(
	BEntry&	inFile)
{
	inFile.Remove();		
}

// ---------------------------------------------------------------------------
//		FindDirectory
// ---------------------------------------------------------------------------
//	Should be in BDirectory.

status_t 
FindDirectory(
	BDirectory&		inDir,
	const char *	inPath,
	BDirectory*		inoutDirectory)
{
	BEntry		entry;
	status_t	err = inDir.FindEntry(inPath, &entry);

	if (err == B_NO_ERROR)
	{
		err = entry.IsDirectory() ? err : B_FILE_NOT_FOUND;
	
		if (err == B_NO_ERROR)
			err = inoutDirectory->SetTo(&entry);
	}

	return err;
}

// ---------------------------------------------------------------------------
//		FindFile
// ---------------------------------------------------------------------------
//	Should be in BDirectory.

status_t 
FindFile(
	BDirectory&		inDir,
	const char *	inPath,
	BEntry*			outEntry)
{
	status_t	err = inDir.FindEntry(inPath, outEntry);

	if (err == B_NO_ERROR)
		err = outEntry->IsFile() ? err : B_FILE_NOT_FOUND;
	
	return err;
}

// ---------------------------------------------------------------------------
//		FileWriteableState
// ---------------------------------------------------------------------------
//	This needs to return an enum indicating if the file is writable, not writeable
//	or is on read-only medium.

FileWriteAble FileWriteableState(
	BStatable&	inFile)
{
	BVolume			vol;
	status_t		err = inFile.GetVolume(&vol);
	
	bool			readOnlyVolume = vol.IsReadOnly();	
	FileWriteAble	fileState = kIsWritable;

	if (readOnlyVolume)
		fileState = kReadOnly;
	else
	{
		struct stat		info;
		err = inFile.GetStat(&info);
		
		bool			permissionReadable = (info.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0;
		if (!permissionReadable)
			fileState = kPermissionLocked;
	}
	
#if 0
	// GetPermissions always returns 0 :-/
	err = inFile.GetPermissions(&info.st_mode);
	result = (info.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) != 0;
#endif

	return fileState;
}

// ---------------------------------------------------------------------------
//		IterateDirectory
// ---------------------------------------------------------------------------
//	Iterate over the contents of the directory passing BEntry objects to
//	the IteratorFunc.  The nodeflavors indicates whether either files,
//	directories, or both are to be passed to the iterator func.  If
//	traverselinks is true then the object pointed to by the link is passed
//	to the iterator func.  The symlinks themselves are never passed directly
//	to the iterator func.  If traverselinks is false then links are ignored.


status_t
IterateDirectory(
	BDirectory& 		inDir,
	MDirIteratorFunc&	inFunc,
	uint32				inNodeFlavors,		// some combo of B_FILE_NODE and B_DIRECTORY_NODE
	bool				inTraverseLinks)
{
	status_t		err = B_OK;
	BEntry			entry;
	myDirent		dirents[2];
	int32			count;
	bool			doFiles = (inNodeFlavors & B_FILE_NODE) != 0;
	bool			doFolders = (inNodeFlavors & B_DIRECTORY_NODE) != 0;
	bool			more = true;

	ASSERT(doFiles || doFolders);	// need to look for at least one of them
	
	while (more && (count = inDir.GetNextDirents(dirents, sizeof(dirents))) > 0)
	{
		dirent*			ent = dirents;
		
		for (int i = 0; i < count; i++)
		{
			if (0 != strcmp(ent->d_name, ".") && 0 != strcmp(ent->d_name, ".."))
			{
				struct stat		rec;
				inDir.GetStatFor(ent->d_name, &rec);

				if (S_ISREG(rec.st_mode) && doFiles)	// regular file
				{
					err = entry.SetTo(&inDir, ent->d_name);
					more = inFunc.ProcessItem(entry, B_FILE_NODE, inDir);
				}
				else
				if (S_ISDIR(rec.st_mode) && doFolders)	// directory
				{
					err = entry.SetTo(&inDir, ent->d_name);
					more = inFunc.ProcessItem(entry, B_DIRECTORY_NODE, inDir);
				}
				else
				if (inTraverseLinks && S_ISLNK(rec.st_mode))		// symlink
				{
					err = entry.SetTo(&inDir, ent->d_name, true);	// traverse
					
					if (err == B_OK)
					{
						if (entry.IsFile() && doFiles)
						{
							more = inFunc.ProcessItem(entry, B_FILE_NODE, inDir);
						}
						else
						if (entry.IsDirectory() && doFolders)
						{
							more = inFunc.ProcessItem(entry, B_DIRECTORY_NODE, inDir);
						}
					}
				}
			}
		
			ent = (dirent*)((char*)ent + ent->d_reclen);
		}
	}
	
	return err;
}


