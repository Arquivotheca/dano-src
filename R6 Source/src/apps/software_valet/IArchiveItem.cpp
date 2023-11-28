// IArchiveItem.cpp
#include "IArchiveItem.h"
#include "IArchivePatchItem.h"

#include <Path.h>
#include <NodeInfo.h>
#include <string.h>
#include <AppFileInfo.h>
#include <String.h>
#include <SymLink.h>

#include "InstallMessages.h"
#include "InstallPack.h"
#include "DestinationList.h"
#include "ReplaceDialog.h"
#include "DestManager.h"

#include "Replace.h"
#include "AutoPtr.h"

#include "MyDebug.h"

#include "MyDebug.h"
#include "Util.h"


const char *errCONTINUE = "Continue";

long handleInstallError(const char *format,
						const char *text,
						long errCode,
							// for logging
						DestManager *dests);


ArchiveItem::ArchiveItem()
	:	ListItem(),
		name(NULL),
		parentFolder(NULL)
{
}

void ArchiveItem::Unset()
{
	groups = 1;
	destination = D_PARENT_FOLDER;
	customDest = false;
	replacement = R_ASK_USER;
	uncompressedSize = 0;
	creationTime = 0;
	modTime = 0;
	platform = 0xFFFFFFFF;
	seekOffset = -1;
	mode = S_IRWXU | S_IRWXG | S_IRWXO;
	memset(versionInfo,0,sizeof(versionInfo));
}

ArchiveItem::~ArchiveItem()
{
	free(name);
}

bool ArchiveFolderItem::IsFolder()
{	
	return true;
}
	
ArchiveFolderItem::ArchiveFolderItem()
	: ArchiveItem()
{
}

void ArchiveFolderItem::Unset()
{
	isTopDir = false;
	ArchiveItem::Unset();
}


ArchiveFolderItem::~ArchiveFolderItem()
{
}

bool ArchiveFileItem::IsFolder()
{
	return false;
}

ArchiveFileItem::ArchiveFileItem()
	: ArchiveItem()
{
	fType = NULL;
}

ArchiveFileItem::~ArchiveFileItem()
{
	// PRINT(("freeing file %s\n",this->name));
	free(fType);
}


// implement replacement, safe canceling
#if 0
void ArchiveFolderItem::GetGroupInfo(ulong inGrps, long &iCount, long &bytes,long blockSz)
{
	if (!(groups & inGrps))
		return;
	
	long count = entries->CountItems();
	for (long i = 0; i < count; i++) {
		ArchiveItem *arcItem = entries->ItemAt(i);
	
		if (arcItem->IsFolder()) {
			((ArchiveFolderItem *)arcItem)->GetGroupInfo(inGrps,iCount,bytes,blockSz);
		}
		else if (arcItem->groups & inGrps) {
			iCount++;
			
			long sz = arcItem->uncompressedSize;
			long rem = sz % blockSz;
			bytes += sz + (rem ? blockSz - rem : 0);
			// ideally we would like to also calc 
			// size from alloc block info but this information
			// seems to be unavailble
		}
	}
}
#endif

#if 0
#if (!SEA_INSTALLER)
void	ArchiveFolderItem::BuildDisplay(	PTreeItem *parItem,
											DestManager *dests,
											ulong inGrps,
											TreeItem *root )
{
	if (!(groups & inGrps))
		return;
	PTreeItem *tItem;
	PTreeItem *tFolder = NULL;
	
	bool top = (this == dests->TopFolder());
	if (top)
		PRINT(("at toplevel folder\n"));
	
	if ((top && dests->doInstallFolder) || !top) {
		PRINT(("dest for folder %s\n",name));
		// return the tree item for our destination
		tItem = dests->TreeItemFor(destination,parItem,root,customDest);
		tFolder = new PTreeFolderItem(name,this);
		// add us as a child of the item
		tItem->AddChild(tFolder);
		
		if (top) {
			PRINT(("setting instFolderTreeItem\n"));
			dests->instFolderTreeItem = tFolder;
		}
	}
	long count = entries->CountItems();
	for (long i = 0; i < count; i++) {
		ArchiveItem *arcItem = entries->ItemAt(i);

		if (typeid(*arcItem) == typeid(ArchiveFolderItem)) {
			ArchiveFolderItem *fold = (ArchiveFolderItem *)arcItem;
			fold->BuildDisplay(tFolder,dests,inGrps,root);
		}
		else if (typeid(*arcItem) == typeid(ArchivePatchItem)) {
			// we should know where this patch is being applied
		}
		else if (typeid(*arcItem) == typeid(ArchiveFileItem)) {
			PRINT(("dest for file %s\n",arcItem->name));
			if (arcItem->groups & inGrps) {
				tItem = dests->TreeItemFor(	arcItem->destination,
										   	tFolder,root,
										   	arcItem->customDest );
				PTreeItem *tFile = new PTreeFileItem(arcItem->name, (ArchiveFileItem *)arcItem);
				tItem->AddChild(tFile);
			}
		}
	}
}
#endif

// allows us to know destinations for patches in advance
void 	ArchiveFolderItem::InitDestinations( BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									RList<ArchivePatchItem *> *patchList)
{
	if (!(groups & inGrps))
		return;
	if (!dests->InstallPlatform(platform))
		return;

	BDirectory	*newDir;
	BDirectory	foundDir;
	
	/*
	if (dests->TopFolder() == this && !dests->doInstallFolder)
		newDir = parDir;
	else {
	*/
    BDirectory *dest = dests->DirectoryFor(destination,parDir,customDest,FALSE);
	// check if the path for this folder exists
	// set foundDir to existing folder (or null if folder not available
	if (dests->canceled)
		return;

	newDir = NULL;	// initialize to failure
	if (dest) {
		BEntry	dirEnt;
		if (dest->FindEntry(name,&dirEnt) == B_NO_ERROR) {
			if (foundDir.SetTo(&dirEnt) == B_NO_ERROR)
				newDir = &foundDir;
		}	
		PRINT(("got destination for folder %s, index %d, custom %d\n",name,destination,customDest));
			
#if DEBUG
		AutoPtr<char> pbuf(1024);
		char *buf = pbuf.Value();
		ArchivePatchItem::PathForDir(dest,buf,buf+1024);
		
		PRINT(("the path is %s\n",buf));
#endif
	}
	else {
		PRINT(("failed to get destination for folder %s\n",name));
	}	
	
	
	long count = entries->CountItems();
	for (long i = 0; i < count; i++) {
		ArchiveItem *arcItem = entries->ItemAt(i);
		
		if (typeid(*arcItem) == typeid(ArchiveFolderItem)) {
			ArchiveFolderItem *fold = (ArchiveFolderItem *)arcItem;
			fold->InitDestinations(newDir,dests,inGrps,patchList);
		}
		else if (typeid(*arcItem) == typeid(ArchivePatchItem)) {
			if ((arcItem->groups & inGrps) &&
				(dests->InstallPlatform(arcItem->platform)))
			{
				ArchivePatchItem *patch = (ArchivePatchItem *)arcItem;
				
				long dInd = patch->destination;
				long cust = patch->customDest;

				PRINT(("checking destination for patch %s ...\n",patch->name));
				BDirectory *dest = dests->DirectoryFor(dInd,newDir,cust,FALSE,patch);
				PRINT(("got it, the result was %d\n",dest));
				
				// patch may or may not have been found
				patchList->AddItem(patch);
				if (dests->canceled)
					return;
			}
		}
		// don't worry about files, do these lazily during install
	}
}
#endif


////////////////////////////////////////////////////////////////
// specifc error stuff
////////////////////////////////////////////////////////////////

long handleInstallError(const char *format,
						const char *text,
						long errCode,
							// for logging
						DestManager *dests)
{
//	errCode;
//	dests;
	// LOG THAT AN ERROR OCCURRED
	
	long result = doError(format,text,errCONTINUE,"Cancel",NULL);
	if (result == 0) {
		// LOG THAT WE ARE CONTINUING	
		// should this be return errCode;
		return B_NO_ERROR;
	}
	else {			
		return B_CANCELED;	
	}
}

void AppendPostfix(char *buf, int32 bufsiz, const char *postfix, long atIndex = -1);

void AppendPostfix(char *buf, int32 bufsiz, const char *postfix, long atIndex)
{
	long len;
	if (atIndex < 0)
		len = strlen(buf);
	else
		len = atIndex;
		
	long postfixLen = strlen(postfix) + 1;
	
	if (len > bufsiz - postfixLen) {
		memcpy(buf+bufsiz-postfixLen,postfix,postfixLen);
	}
	else {
		memcpy(buf+len,postfix,postfixLen);
	}
}


status_t RenameExisting(BDirectory *destDir, BEntry *item, char *postfix)
{
	if (!item)
		return B_NO_ERROR;
			
	char buf[B_FILE_NAME_LENGTH];
	item->GetName(buf);

	AppendPostfix(buf,B_FILE_NAME_LENGTH,postfix);

	PRINT(("NEW NAME IS: %s\n",buf));
	if (destDir->Contains(buf) ){
		PRINT(("trying to find unused name"));
		long appendLoc = strlen(buf);
		long i;
		for( i = 1; i < LONG_MAX; i++) {
			char tail[32];
			sprintf(tail,"%s%d",postfix,i);
			AppendPostfix(buf,B_FILE_NAME_LENGTH,tail,appendLoc);
			// we have a good name
			if (!destDir->Contains(buf))
				break;	
		}
		// could get a unique name
		if (i == LONG_MAX)
			doError(errNOUNIQNAME);
	}
	// perforrm the rename
	return item->Rename(buf);
}

int  CompareVersions(version_info *a, version_info *b);

int  CompareVersions(version_info *a, version_info *b)
{
	/**** code to compare versions ****/
	int32 vdiff[5];
	
	vdiff[0] = a->major - b->major;
	vdiff[1] = a->middle - b->middle;
	vdiff[2] = a->minor - b->minor;
	vdiff[3] = a->variety - b->variety;
	vdiff[4] = a->internal - b->internal;

	if (vdiff[0] > 0)
		return 1;
	else if (vdiff[0] == 0 && vdiff[1] > 0)
		return 1;
	else if (vdiff[1] == 0 && vdiff[2] > 0)
		return 1;
	else if (vdiff[2] == 0 && vdiff[3] > 0)
		return 1;
	else if (vdiff[3] == 0 && vdiff[4] > 0)
		return 1;
	else if (vdiff[4] == 0)
		return 0;
	return -1;
}


// deals with files and folders!!
bool ArchiveItem::HandleExisting(BDirectory *destDir, BEntry *existingItem, 
								bool &skip, DestManager *dests, long *returnError)
{
	skip = FALSE;
	int32 type(-1);
	char *replaceMsg(NULL);
	
	switch(replacement) {	
		case R_NEVER_REPLACE:
			skip = TRUE;
			break;
		case R_RENAME:
			// do rename then proceed as usual
			// should check for error
			RenameExisting(destDir,existingItem);
			break;
		case R_REPLACE_VERSION_NEWER: {
			// conditional continue
			BFile	f(existingItem,O_RDONLY);
			if (f.InitCheck() >= B_OK) {
				// won't work cross platform ---
				BAppFileInfo	vi(&f);
				version_info	curInfo;
				// this call will fail on the opposite platform
				if (vi.GetVersionInfo(&curInfo,B_APP_VERSION_KIND) > B_OK)
				{
					version_info	newInfo;
					
					newInfo.major = versionInfo[0];
					newInfo.middle = versionInfo[1];
					newInfo.minor = versionInfo[2];
					newInfo.variety = versionInfo[3];
					newInfo.internal = versionInfo[4];
					
					if (CompareVersions(&curInfo,&newInfo) > 0) {
						// the currently installed item is newer so skip
						// installing over it
						skip = true;
						break;
					}
				}
			}
			dests->SafeDelete(existingItem,destDir);
			break;
		}
		case R_REPLACE_CREATION_NEWER: {
			// conditional continue
			time_t tim;
			existingItem->GetCreationTime(&tim);
			if (creationTime > tim)
				dests->SafeDelete(existingItem,destDir);
			else
				skip = TRUE;
			break;
		}
		case R_REPLACE_MODIFICATION_NEWER: {
			// conditional continue
			time_t tim;
			existingItem->GetModificationTime(&tim);
			if (modTime > tim)
				dests->SafeDelete(existingItem,destDir);
			else
				skip = TRUE;
			break;
		}
		case R_ASK_VERSION_NEWER:
			if (type < 0) {
				type = ASK_IF_VERSION;
				// check the version
				BFile	f(existingItem,O_RDONLY);
				if (f.InitCheck() >= B_OK) {
					// won't work cross platform ---
					BAppFileInfo	vi(&f);
					version_info	curInfo;
					// this call will fail on the opposite platform
					if (vi.GetVersionInfo(&curInfo,B_APP_VERSION_KIND) > B_OK)
					{
						version_info	newInfo;
						newInfo.major = versionInfo[0];
						newInfo.middle = versionInfo[1];
						newInfo.minor = versionInfo[2];
						newInfo.variety = versionInfo[3];
						newInfo.internal = versionInfo[4];
					
						if (CompareVersions(&curInfo,&newInfo) >= 0) {
							// the currently installed item is newer so
							// ask user what to do
							BEntry	ent;
							BPath	destPath;
							destDir->GetEntry(&ent);
							ent.GetPath(&destPath);
							BString buf;
							buf << "An item named \"" << name << "\"\n";
							buf << "with a newer version already exists in\n\"";
							buf << destPath.Path() << "\"";
							replaceMsg = strdup(buf.String());
						} else {
							dests->SafeDelete(existingItem, destDir);
							break;
						}
					} else {
						dests->SafeDelete(existingItem, destDir);
						break;
					}
				} else {
					// version could not be determined, delete old item.
					dests->SafeDelete(existingItem, destDir);
					break;
				}
			}
			// fall through to ask
		case R_ASK_CREATION_NEWER:
			if (type < 0) {
				type = ASK_IF_CREATION;

				time_t tim;
				existingItem->GetCreationTime(&tim);
				if (creationTime >= tim) {
					// this item is newer than the item on disk
					dests->SafeDelete(existingItem,destDir);
					break;					
				} else {
					// the item on disk is newer, ask user what to do
					BEntry	ent;
					BPath	destPath;
					destDir->GetEntry(&ent);
					ent.GetPath(&destPath);
					BString buf;
					buf << "An item named \"" << name << "\"\n";
					buf << "with a newer creation time already exists in\n\"";
					buf << destPath.Path() << "\"";
					replaceMsg = strdup(buf.String());
				}				
			}
			// fall through to ask
		case R_ASK_MODIFICATION_NEWER:
			if (type < 0) {
				type = ASK_IF_MODIFICATION;

				time_t tim;
				existingItem->GetModificationTime(&tim);
				if (modTime >= tim) {
					dests->SafeDelete(existingItem,destDir);
					break;	
				}
				else {
					// the item on disk is newer, ask user what to do
					BEntry	ent;
					BPath	destPath;
					destDir->GetEntry(&ent);
					ent.GetPath(&destPath);
					BString buf;
					buf << "An item named \"" << name << "\"\n";
					buf << "with a newer modification time already exists in\n\"";
					buf << destPath.Path() << "\"";
					replaceMsg = strdup(buf.String());
				}
			}
			// fall through to ask
		case R_MERGE_FOLDER:
			// if the replacement is merge and (merging can't happen with files)
			// just go ahead and fall back on asking the user
		case R_ASK_USER: {
			long result;
		
			result = dests->replCurrent;
			if (!dests->replApplyToAll) { // XXX: check separately for each type of question?
				if (type < 0) {
					type = ASK_ALWAYS;
					BEntry	ent;
					BPath	destPath;
					destDir->GetEntry(&ent);
					ent.GetPath(&destPath);
					BString buf;
					buf << "An item with the name \"" << name << "\" already exists in\n\"";
					buf << destPath.Path() << "\"";
					replaceMsg = strdup(buf.String());
				}	

				ReplaceDialog	*rd = new ReplaceDialog(&result,
														&(dests->replApplyToAll),
														replaceMsg,
														type);
				status_t err = rd->Go();
				if (result == B_CANCELED) {
					PRINT(("canceled\n"));
					skip = TRUE;
					*returnError = B_CANCELED;
					break;
				}
				else
					dests->replCurrent = result;
			}			
			switch (result) {
				case askUserSKIP:
					skip = TRUE;
					break;
				case askUserRENAME:
					// rename
					RenameExisting(destDir,existingItem);
					break;
				case askUserREPLACE:
					// move file to temporary directory
					// add to list
					// list contains a ref for the file
					// and a ref for the original directory
					dests->SafeDelete(existingItem,destDir);
					break;
			}
			break;
		}
		case R_INSTALL_IF_EXISTS: {
			// the item exists so remove it and continue
			dests->SafeDelete(existingItem,destDir);
			break;
		}
		default:
			doError(errBADREPLOPT);
			skip = TRUE;
			break;
	}
	return !skip;
}

bool ArchiveFolderItem::HandleExisting(BDirectory *destDir, BEntry *existingItem, 
								bool &skip, DestManager *dests, long *returnError)
{
	skip = FALSE;
	bool create = TRUE;
	switch(replacement) {
		case R_MERGE_FOLDER: {
			if (existingItem->IsDirectory()) {
				create = FALSE;
				break;
			}
		}
		default:
			create = ArchiveItem::HandleExisting(destDir,existingItem,skip,dests,returnError);
			break;
	}
	return create;
}


long ArchiveFolderItem::ExtractItem(InstallPack *archiveFile, 
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip)
{
	long err = B_NO_ERROR;
	BDirectory	*destDir;
	bool makefold;
	
	if (!(groups & inGrps) || !(dests->InstallPlatform(platform))) {
		PRINT(("folder %s not in group\n",name));
		return err;
	}
	
	makefold = true;
	
	//if (!(dests->lowMemMode) && !isTopDir) {
	//	isTopDir = (this == dests->TopFolder());
	//	newDir = &theNewDir;
	//}
	
	if (isTopDir)
		makefold = dests->doInstallFolder;
	
	if (makefold) 	{
		bool create = true;
		if (!skip) {
			if (dests->canceled)
				return B_CANCELED;
			//PRINT(("extracting folder %s\n",name));
			destDir = dests->DirectoryFor(destination,parDir,customDest);				
			if (!destDir) {
				// give the option to continue or cancel
				return handleInstallError(errNODESTDIR,name,err,dests);
			}
			
			// check for existing entry	
			BEntry	existingItem;
			if (destDir->FindEntry(name,&existingItem) >= B_NO_ERROR &&
				existingItem.Exists())
			{
				/// implement replacement options here
				PRINT(("contains duplicate, resolving conflict\n"));
				// the duplicate may be a file OR folder!
	
				create = HandleExisting(destDir,&existingItem,skip,dests,&err);
				if (!skip && !create && existingItem.IsDirectory()) {
					newDir->SetTo(&existingItem);
				}
			}
			else {
				if (replacement == R_INSTALL_IF_EXISTS) {
					skip = true; create = false;
				}
			}
		}
		if (skip) {
			if (err != B_CANCELED) {
				BMessage skipMsg(M_UPDATE_PROGRESS);
				skipMsg.AddInt32("bytes",uncompressedSize);
				archiveFile->statusMessenger.SendMessage(&skipMsg);
			}
			newDir->Unset();
			return err;
		}
		
		// if not toplevel directory
		if (create) {
			// the folder must be created
			//PRINT(("creating new folder\n"));
			err = destDir->CreateDirectory(name,newDir);
			// handle error !!!
		
			if (dests->logging) {
				char buf[B_FILE_NAME_LENGTH+40];
				WritePath(destDir,dests->LogFile(),buf);
				sprintf(buf,"%s/\n\tType: Folder\n",name);
				dests->LogFile()->Write(buf,strlen(buf));
			}
		}

		if (isTopDir) {
			BEntry	ent;
			newDir->GetEntry(&ent);
			dests->SetInstallDir(&ent);
		}
		if (seekOffset >= 0)
		{
			if (isTopDir) {
				doError("Extracting attrs for top dir!");
			}
			BNode	outNode(newDir,".");
			archiveFile->ExtractFile(&outNode,seekOffset);	
		}	
	}
		
	//if (!(dests->lowMemMode)) {	
	#if 0
		PRINT(("looping through folder entries\n"));
		long count = entries->CountItems();
		for (long i = 0; i < count; i++) {
			ArchiveItem *arcItem = entries->ItemAt(i);
			err = arcItem->ExtractItem(archiveFile,newDir,dests,inGrps);
			if (err < B_NO_ERROR)
				return err;
		}
	#endif
	
	return err;
}

long ArchiveFileItem::ExtractItem(InstallPack *archiveFile,
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip)
{
	long err = B_NO_ERROR;
	BEntry 			outFile;
	BEntry 			existingItem;
	BDirectory		*destDir;
	
	if (!(groups & inGrps) || !(dests->InstallPlatform(platform))) {
		PRINT(("file %s not in group\n",name));
		return err;
	}
	if (!skip) {
		PRINT(("extracting file %s\n",name));
	
		BMessage upMsg(M_CURRENT_FILENAME);
		
		upMsg.AddString("filename",name);
		archiveFile->statusMessenger.SendMessage(&upMsg);

		/// DO CANCEL CHECK HERE	
		if (dests->canceled)
			return B_CANCELED;
		
		destDir = dests->DirectoryFor(destination,parDir,customDest);
		
		if (!destDir) {
			return handleInstallError(errNODESTDIR,name,err,dests);
		}
		
		// look for an existingItem
		if (destDir->FindEntry(name,&existingItem) >= B_NO_ERROR &&
			existingItem.Exists())
		{
			/// implement replacement options here
			/// existing item may be a file OR folder
			
			bool create;	// do we need a new file (irrelevant for files)
			create = HandleExisting(destDir,&existingItem,skip,dests,&err);
		}
		else {
			// if install only if exists and not found then skip!
			if (replacement == R_INSTALL_IF_EXISTS)
				skip = true;
		}
	}
	if (skip) {
		if (err != B_CANCELED) {
			BMessage skipMsg(M_UPDATE_PROGRESS);
			skipMsg.AddInt32("bytes",dests->AllocSize(uncompressedSize));
			archiveFile->statusMessenger.SendMessage(&skipMsg);
		}
		return err;
	}
	
	// not skipping
	{
		BFile	newfile;
		err = destDir->CreateFile(name, &newfile);
		// get the real thing
		destDir->FindEntry(name,&outFile);
#if (DOES_UNINSTALL)		
		// remember the new file and any possible existing item
		dests->RememberFile(&outFile, &existingItem);
#endif
	}
	if (err != B_NO_ERROR) {
		return handleInstallError(errMKFILE,name,err,dests);
	}
	

	BFile	f(&outFile,O_RDWR);
	err = f.InitCheck();
	if (err >= B_OK)
		err = archiveFile->ExtractFile(&f,seekOffset);
	if (err == B_CANCELED) {
		outFile.Remove();
		return err;
	}
	if (err < B_NO_ERROR) {
		// reset decompression so we can continue if possible!
		//archiveFile->ResetDecompress();
		// might want error codes here??
		return handleInstallError(errDECOMPRESS,name,err,dests);
	}
		//if (archiveFile->checksum != adler32) {
		//	doError(errCHKSUM,name,errCONTINUE,NULL,NULL);
		//	PRINT(("IN %d OUT %d\n",archiveFile->checksum,adler32));
		//}
		//archiveFile->ResetDecompress();
		
		//off_t outSize;
		//outFile.GetSize(&outSize);
		//if (uncompressedSize != outSize) {
		//	doError(errBADSIZE,name,errCONTINUE,NULL,NULL);
		//}
	
	long extra = dests->AllocExtra(uncompressedSize);
	if (extra) {
		BMessage upMsg(M_UPDATE_PROGRESS);
		upMsg.AddInt32("bytes",extra);
		archiveFile->statusMessenger.SendMessage(&upMsg);
	}
	if (dests->logging) {
		char buf[B_FILE_NAME_LENGTH*2];
		WritePath(destDir,dests->LogFile(),buf);
		sprintf(buf,"%s\n\tSize: %Ld bytes\t\tType: %s\n",name,uncompressedSize,
												(fType && *fType) ? fType : "N/A");
		dests->LogFile()->Write(buf,strlen(buf));
		//if (version) {
		//	ParseVersion(version,buf);
		//}
		//else {
		//	buf[0] = '\n'; buf[1] = 0;
		//}
		//dests->LogFile()->Write(buf,strlen(buf));	
	}
	ISetType(&outFile);

	return err;
}

// we default files to the current umask
// execpt if the original file was marked executable
void ArchiveItem::ISetType(BEntry *entry)
{
	// set permissions mime info
	mode_t execBits = (mode & (S_IXOTH | S_IXGRP | S_IXUSR));
	// if any executable bits set, update mime info
	if (execBits) {	
		// the permissions are all bits cleared of the umask
		// and any executable bits
		// get the current umask
		mode_t curMask = umask(0);
		umask(curMask);
		
		entry->SetPermissions(((S_IRWXU | S_IRWXG | S_IRWXO) & ~curMask) | execBits);
		BPath	fpath;
		entry->GetPath(&fpath);
		// call synchronous so we can fix update time
		update_mime_info(fpath.Path(),false,true,false);
	}
	
	// fix up creation and modification times
	entry->SetModificationTime(modTime);
	entry->SetCreationTime(creationTime);
}

void ArchiveFileItem::ISetType(BEntry *entry)
{
	if (fType) {
		BNode		node(entry);
		if (node.InitCheck() >= B_OK) {
			BNodeInfo	ninf(&node);
			ninf.SetType(fType);
		}
	}
	ArchiveItem::ISetType(entry);
}

/////////////////////////////////////////////////


ArchiveLinkItem::ArchiveLinkItem()
	:	ArchiveItem(),
		linkPath(NULL)
{
}

ArchiveLinkItem::~ArchiveLinkItem()
{
	free(linkPath);
}

bool ArchiveLinkItem::IsFolder()
{	
	return false;
}
	
long ArchiveLinkItem::ExtractItem(InstallPack *archiveFile, 
									BDirectory *parDir, 
									DestManager *dests, 
									ulong inGrps,
									bool skip)
{
	BDirectory		*destDir;
	status_t		err = B_NO_ERROR;
	
	if (!(groups & inGrps) || !(dests->InstallPlatform(platform))) {
		PRINT(("symlink %s not in group\n",name));
		return err;
	}
	
	if (dests->canceled)
		return B_CANCELED;
	
	//PRINT(("extracting symlink %s\n",name));
	destDir = dests->DirectoryFor(destination,parDir,customDest);				
	if (!destDir) {
		// give the option to continue or cancel
		return handleInstallError(errNODESTDIR,name,err,dests);
	}

	// check for existing entry				
	BEntry	existingItem;	
	if (destDir->FindEntry(name,&existingItem) >= B_NO_ERROR &&
		existingItem.Exists())
	{
		/// implement replacement options here
		// the duplicate may be a file OR folder OR symlink OR other node type!
		bool create;
		create = HandleExisting(destDir,&existingItem,skip,dests,&err);
	}
	else {
		if (replacement == R_INSTALL_IF_EXISTS)
			skip = true;
	}
	if (skip)
		return err;
	{	
		BMessage upMsg(M_CURRENT_FILENAME);
		
		upMsg.AddString("filename",name);
		archiveFile->statusMessenger.SendMessage(&upMsg);

		// create the symlink
		BSymLink	symLink;
		err = destDir->CreateSymLink(name,linkPath,&symLink);
		if (err < B_OK) {
			return handleInstallError(errMKLINK,name,err,dests);
		}
		
		if (seekOffset >= 0)
		{
			archiveFile->ExtractFile(&symLink,seekOffset);
		}
		if (dests->logging) {
			char buf[B_FILE_NAME_LENGTH*2];
			WritePath(destDir,dests->LogFile(),buf);
			sprintf(buf,"%s\n\tSymblic link to %s\n",name,linkPath);
			dests->LogFile()->Write(buf,strlen(buf));
		}
	}
	
	BEntry	linkEntry;
	destDir->FindEntry(name,&linkEntry);
	ISetType(&linkEntry);
	
	return err;
}
