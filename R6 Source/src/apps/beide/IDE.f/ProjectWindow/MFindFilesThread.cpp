//========================================================================
//	MFindFilesThread.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	A thread to find all the files in the file list.  This object is
//	created when a project view is filled from a project file.  This allows
//	the project window to have normal response when it's opened.
//	BDS

#include "MFindFilesThread.h"
#include "MProjectView.h"
#include "MSourceFile.h"
#include "IDEMessages.h"
#include "ProgressStatusWindow.h"
#include "Utils.h"
#include "MAlert.h"

#include <OS.h>

// number of seconds until I decide that I need to tell the user what I'm doing
const bigtime_t kNoStatusDelay = 3*1000000;

// ---------------------------------------------------------------------------
//		MFindFilesThread
// ---------------------------------------------------------------------------
//	Constructor

MFindFilesThread::MFindFilesThread(
	MProjectView*	inView)
	: MThread("finder"),
	fView(inView),
	fProgressStatus(NULL)
{
}

// ---------------------------------------------------------------------------
//		~MFindFilesThread
// ---------------------------------------------------------------------------
//	Destructor

MFindFilesThread::~MFindFilesThread()
{
	// If we get killed right in the middle of our work
	// make sure we clean up the progress window also
	if (fProgressStatus) {
		fProgressStatus->TaskDone();
		fProgressStatus = NULL;
	}
	
	Kill();
	LastCall();
}

// ---------------------------------------------------------------------------
//		LastCall
// ---------------------------------------------------------------------------
//	Don't delete this.

void
MFindFilesThread::LastCall()
{
}

// ---------------------------------------------------------------------------
//		Execute
// ---------------------------------------------------------------------------

status_t
MFindFilesThread::Execute()
{
	int32			count = 0;
	bool			stillFinding = true;
	node_ref*		nodeRef;
	NodeList		nodeList;

	fFoundSymLink = false;
	
	fStartTime = system_time();
	
	try {
		fView->FindAllFiles(*this, nodeList);
	}
	catch (...) {
	}
	
	// Delete all the noderefs
	int32			i = 0;
	while (nodeList.GetNthItem(nodeRef, i++))
	{
		delete nodeRef;
	}

	// give each sourcefileline a chance to update its
	// filetype and fix its target
	count = 0;
	stillFinding = true;

	if (fProgressStatus) {	
		fProgressStatus->StatusUpdate("Updating file types");
	}
	
	while (stillFinding && this->Cancelled() == false)
	{
		Lock();		// Prevent being killed while have aquired sem in UpdateOneFile
		stillFinding = fView->UpdateOneFile(count);
		Unlock();
	}

	if (fProgressStatus) {
		fProgressStatus->TaskDone();
		fProgressStatus = NULL;
	}

	if (this->Cancelled()) {
		MAlert alert("Use Project Settings/Access Paths to narrow the scope of the access paths to reduce project file searching time.");
		alert.Go();
	}
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		FindFilesInDirectory
// ---------------------------------------------------------------------------

void
MFindFilesThread::FindFilesInDirectory(
	MSourceFileList&	inFileList,
	MSourceFileList&	inSlashFileList,
	BDirectory&			inDir,
	NodeList&			inNodeList,
	bool				inIsRecursive,
	bool				inIsSystem)
{
	// Index through all the files in the directory, checking if they're in
	// the list
	// [John Dance] This used to recurse immediately when a directory entry was found
	// Unfortunately, this lead to files found in subdirectories before they were found in the
	// current directory (depending on the iteration order).  Moved all recursion to the end
	// of the method so that it does a breadth first search for the files every time.
	// (This matches the style used in MFileUtils also.)
	
	BEntry			entry;
	entry_ref		ref;
	MSourceFile* 	sourceFile;
	status_t		err;
	myDirent		dirents[2];
	int32			count;
	node_ref*		nodeRef = new node_ref;
	BList			directoriesToSearch;
	
	inDir.GetNodeRef(nodeRef);
	inNodeList.AddItem(nodeRef);

	// each time we start a new directory
	// check if the user has cancelled 
	// or if enough time has elasped so that we should show a status
	// or show the current status
	if (this->Cancelled()) {
		throw true;
	}
	else if (fProgressStatus) {
		BPath dirPath(&inDir, NULL);
		fProgressStatus->StatusUpdate(dirPath.Path());
	}
	else if (system_time() - fStartTime > kNoStatusDelay) {
		fProgressStatus = new ProgressStatusWindow(BPoint(0, 0), "Looking for Project files...", this);
		CenterWindow(fProgressStatus);
		fProgressStatus->TaskStarted();
	}
	
	while ((count = inDir.GetNextDirents(dirents, sizeof(dirents))) > 0)
	{
		dirent*			ent = dirents;
		
		ASSERT(count == 1);		// will this ever change ????
		for (int i = 0; i < count; i++)
		{
			if (0 != strcmp(ent->d_name, ".") && 0 != strcmp(ent->d_name, ".."))
			{
				struct stat		rec;
				inDir.GetStatFor(ent->d_name, &rec);

				if (S_ISREG(rec.st_mode))	// regular file
				{
					int32		index;
					bool		foundInCorrectTree = inFileList.FindItem(ent->d_name, inIsSystem, index);
					bool		foundInWrongTree;
					
					if (!foundInCorrectTree)
						foundInWrongTree = inFileList.FindItem(ent->d_name, ! inIsSystem, index);
					if (foundInCorrectTree || foundInWrongTree)
					{
						err = entry.SetTo(&inDir, ent->d_name);
	
						if (B_NO_ERROR == err)
						{
							sourceFile = inFileList.ItemAtFast(index);
							
							// don't care about tree for sourcefiles, only header files
							if (foundInCorrectTree || sourceFile->IsSourceFile())
							{
								sourceFile->SetRef(entry);
								inFileList.RemoveItemAt(index);
							}
						}
					}
				}
				else
				if (S_ISDIR(rec.st_mode))	// directory
				{
					if (inIsRecursive && ! MFileUtils::Parenthesized(ent->d_name))
					{
						BDirectory		aDir(&inDir, ent->d_name);

						if (!fFoundSymLink || ! MFileUtils::AlreadyScanned(aDir, inNodeList)) {
							node_ref* ref = new node_ref;
							aDir.GetNodeRef(ref);
							directoriesToSearch.AddItem(ref);
						}
					}
				}
				else
				if (S_ISLNK(rec.st_mode))	// symlink
				{
					err = entry.SetTo(&inDir, ent->d_name, true);			// traverse
										
					if (err == B_OK && B_OK == entry.GetName(fEntryName))	// get real name
					{
						if (entry.IsDirectory())
						{
							if (inIsRecursive && ! MFileUtils::Parenthesized(fEntryName))
							{
								BDirectory		aDir(&entry);
								if (! MFileUtils::AlreadyScanned(aDir, inNodeList))
									FindFilesInDirectory(inFileList, inSlashFileList, aDir, inNodeList, inIsRecursive, inIsSystem);
								fFoundSymLink = true;
							}
						}
						else
						if (entry.IsFile())
						{
							entry.SetTo(&inDir, ent->d_name); // untraverse the link
							entry.GetName(fEntryName);
							
							int32		index;
							bool		foundInCorrectTree = inFileList.FindItem(fEntryName, inIsSystem, index);
							bool		foundInWrongTree;
							
							if (!foundInCorrectTree)
								foundInWrongTree = inFileList.FindItem(fEntryName, ! inIsSystem, index);
							if (foundInCorrectTree ||foundInWrongTree)
							{
								sourceFile = inFileList.ItemAtFast(index);
								
								// don't care about tree for sourcefiles, only header files
								if (foundInCorrectTree || sourceFile->IsSourceFile())
								{
									sourceFile->SetRef(entry);
									inFileList.RemoveItemAt(index);
								}
							}
						}
					}
				}
			}
		
			ent = (dirent*)((char*)ent + ent->d_reclen);
		}
	}
	
	// Index through all the files with slashes in their names seeing if they're contained in
	// this directory - but only if the directory isn't recursive, because that is the only time
	// slashes will occur in file names
	if (inIsRecursive == false) {
		for (int32 i = inSlashFileList.CountItems() - 1; i >= 0; i--)
		{
			sourceFile = inSlashFileList.ItemAtFast(i);
			if ((sourceFile->IsSourceFile() || inIsSystem == sourceFile->IsInSystemTree()) &&
				B_NO_ERROR == inDir.FindEntry(sourceFile->GetFileName(), &entry) &&
				(entry.IsFile() || entry.IsSymLink()))
			{
				sourceFile->SetRef(entry);
				inSlashFileList.RemoveItemAt(i);			
			}
		}
	}
		
	// Now iterate the directoriesToSearch list and recurse through the list
	for (int32 i = directoriesToSearch.CountItems() - 1; i >= 0; i--)
	{
		node_ref* node = (node_ref*) directoriesToSearch.ItemAtFast(i);
		BDirectory aDir(node);
		if (aDir.InitCheck() == B_OK && MFileUtils::AlreadyScanned(aDir, inNodeList) == false) {
			this->FindFilesInDirectory(inFileList, inSlashFileList, aDir, inNodeList, inIsRecursive, inIsSystem);
		}
		directoriesToSearch.RemoveItem(i);
		delete node;		
	}

}
