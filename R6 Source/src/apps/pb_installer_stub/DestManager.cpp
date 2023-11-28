// DestManager.cpp
#include <ClassInfo.h>
#include <FindDirectory.h>
#include <Volume.h>
#include <Path.h>
#include <NodeInfo.h>
#include "DestManager.h"
#include "DestinationList.h"
#include "PackAttrib.h"

#include "InstallerType.h"
#include "FindChoiceWind.h"
#include "PackageItem.h"


#if DOES_PATCHING
	#include "IArchivePatchItem.h"
#endif

#include "RList.h"
#include "Util.h"

#include "MyDebug.h"


void		AppendPostfix(char *buf, int32 bufsiz, const char *postfix, long atIndex = -1);
status_t	FollowSymLink(BEntry *entry);
void		PrintNodeType(BEntry *e);
const char 	*root_find_directory(directory_which which, BPath *path);


DestManager::DestManager(PackAttrib *_attr)
	: attr(_attr)
{
	fDefList = attr->defaultDest;
	fCustList = attr->customDest;
	doInstallFolder = attr->doInstallFolder;

	deletia = new RList<DeleteItem *>;
	
	fLogFile = NULL;
	replCurrent = askUserREPLACE;
	replApplyToAll = FALSE;
	canceled = false;
	
	fInstall = NULL;
	
	dirName = NULL;
	fileTag = NULL;
}

DestManager::~DestManager()
{
	ClearDestinationLists();
	
	delete fDefList;
	delete fCustList;
	delete deletia;
	// might want to delete individuals
	
	delete fInstall;
	free(dirName);
	free(fileTag);
}

/*
BDirectory *DestManager::GetInstallDir()
{
	return &fInstall;
}
*/

void DestManager::SetInstallDir(BEntry *entry)
{
	if (doInstallFolder)
		fInstall->SetTo(entry);
}

void DestManager::ClearDestinationLists()
{
	// delete any cached directories
	for (long i = fDefList->CountItems()-1; i >= 0; i--) {
		DestItem *di = fDefList->ItemAt(i);
		if (di->dir != NULL) {
			delete di->dir;
		}
		di->dir = NULL;	
	}
	for (long i = fCustList->CountItems()-1; i >= 0; i--) {
		DestItem *di = fCustList->ItemAt(i);
		if (di->dir != NULL) {
			delete di->dir;
		}
		di->dir = NULL;
		
		if (typeid(*di) == typeid(FindItem)) {
			((FindItem *)di)->queryFailed = FALSE;
		}
	}
}

bool DestManager::SetupInstall( entry_ref *instDirRef,
								char *instDirName,
								long installVolID,
								ArchiveFolderItem *top,
								bool _lowMem)
{
#if 0
	if (!fileTag) {
		long nameLen = strlen(attr->name);
		long versLen = strlen(attr->version);
		long devLen = strlen(attr->developer);
		tagLen = nameLen + versLen + devLen + 1;
		fileTag = (char *)malloc(sizeof(char)*tagLen);
		char *identBuf = fileTag;
		
		memcpy(identBuf, attr->name, nameLen);
		identBuf += nameLen;
		memcpy(identBuf, attr->version, versLen);
		identBuf += versLen;
		memcpy(identBuf, attr->developer, devLen);
		identBuf += devLen;
		*identBuf = 0;	
	}
	PRINT(("FILE TAG is\n\t\t%s\n",fileTag));
#endif
	
	fTop = top;
	instFolderTreeItem = NULL;
	deleteOriginals = FALSE;
	lowMemMode = _lowMem;
	replApplyToAll = FALSE;
	canceled = false;
	installDev = installVolID;
	
	if (instDirName) {
		dirName = strdup(instDirName);
		fInstall = NULL;
	}
	else {
		fInstall = new BDirectory(instDirRef);
		dirName = NULL;
	}
	
	BVolume installVol(installDev);
	installVol.GetRootDirectory(&rootDir);

	struct stat			statBuf;
	rootDir.GetStat(&statBuf);
	
	PRINT(("stat got blocksize %d\n",statBuf.st_blksize));
	
	allocBlockSize = 1024;	
	fLogFile = NULL;
	char	vname[B_FILE_NAME_LENGTH];
	BEntry e;
	rootDir.GetEntry(&e);
	e.GetName(vname);
	
	ClearDestinationLists();
	
	long err;
	BEntry		tmpEnt;
	BDirectory	tmpDir;	
	
	err = CreatePathAt(&rootDir,"var/tmp",&tmpEnt);
	if (err < B_NO_ERROR) {
		doError(errMKTMPDIR,vname);
		// abort install
		return FALSE;
	}
	tmpDir.SetTo(&tmpEnt);
			
	char buf[B_FILE_NAME_LENGTH];
	sprintf(buf,"PackageInstaller%d",find_thread(NULL));
	// if the directory is already there
	// then we assume this is from a PackageBuilder which crashed earlier
	// so we should go ahead and delete the whole directory
	err = tmpDir.CreateDirectory(buf,&tempDir);
	if (err != B_NO_ERROR) {
		doError(errMKTMPDIR,vname);
		// abort install
		// should report more specific message
		return FALSE;
	}
	
	// to number folders in the tmp dir where items with
	// duplicate names are placed
	encloseFolderCount = 0;
	
#if (DOES_UNINSTALL)
	BeginRememberList();
#endif
	
	return TRUE;
}


void DestManager::FinishInstall(bool canceled, PackageItem *it)
{
	bool someErrors = false;
	long err;
	
#if (!DOES_UNINSTALL)
	it;
	for (long i = deletia->CountItems()-1; i >= 0; i--) {
		PRINT(("checking item %d to restore\n",i));
		DeleteItem *d = deletia->ItemAt(i);
		/* if the installation was canceled restore items
		   to their original location */
		if (canceled) {
			PRINT(("restoring item to original location\n"));
			// orginal location
			BDirectory 	origDir(&d->dirRef);
			// item to be moved
			BEntry 		moveItem;
			BEntry		delItem;
			char name[B_FILE_NAME_LENGTH];
			
			moveItem.SetTo(&d->ref);
			moveItem.GetName(name);
			
			err = origDir.FindEntry(name,&delItem);
			if (err == B_NO_ERROR)
				RecursiveDelete(&delItem);
			
			BPath	from;
			BPath	to(&origDir, NULL);
			moveItem.GetPath(&from);
			
			// move back
			err = moveItem.MoveTo(&origDir);
			if (err != B_NO_ERROR) {
				doError(errCANTPUTAWAY,name);
				// continue
				someErrors = TRUE;
			}
		}
		deletia->RemoveIndex(i);
		delete d;
	}
#else
	BFile	storeFile;
	
	if (it) {
		if (storeFile.SetTo(&it->fileRef, O_RDWR))
			it = NULL;
		else {
			PRINT(("got registry file!! yay!\n"));
			storeFile.Seek(16,SEEK_SET);
			fPackData.WriteRecordHeader(&storeFile, 'FIds',LIS_TYPE,0);
		}
	}
	else {
		PRINT(("DIDN'T GET REGISTRY FILE\n"));
	}
	
	fPackData.WriteEndHeader(&fNewFiles);
	
	// rewind
	fNewFiles.Seek(0,SEEK_SET);
	
	// prep for read
	record_header	header;
	err = fPackData.ReadRecordHeader(&fNewFiles,&header);
	if (err < B_NO_ERROR) {
		PRINT(("file error\n"));
	}
	else if (header.type != LIS_TYPE || header.what != 'FLis') {
		// err = B_ERROR;
		
		PRINT(("no remembered files\n"));
	}
	else {
		fPackData.ReadRecordHeader(&fNewFiles,&header);
		bool readingEntries = (header.what != END_TYPE);
		while(readingEntries) {
			entry_ref	newRef;
			fPackData.ReadInt32(&fNewFiles,&newRef.device);
			fPackData.ReadRecordHeader(&fNewFiles,&header);
			fPackData.ReadInt64(&fNewFiles,&newRef.directory);
			fPackData.ReadRecordHeader(&fNewFiles,&header);
			fPackData.ReadString(&fNewFiles,&newRef.name);
			
			PRINT(("got file %s\n",newRef.name));
			
			entry_ref	oldRef;
			fPackData.ReadRecordHeader(&fNewFiles,&header);
			if (header.what == 'ODev') {
				fPackData.ReadInt32(&fNewFiles,&oldRef.device);
				fPackData.ReadRecordHeader(&fNewFiles,&header);
				fPackData.ReadInt64(&fNewFiles,&oldRef.directory);
				fPackData.ReadRecordHeader(&fNewFiles,&header);
				fPackData.ReadString(&fNewFiles,&oldRef.name);
				
				PRINT(("got old file %s\n",oldRef.name));
				
				if (canceled) {
					// need to restore to original location
				}
				else {
				
				}
				fPackData.ReadRecordHeader(&fNewFiles,&header);
			}
			else {
				if (canceled) {
					// remove this new item
				}
			}
			if (!canceled) {
				// we need to tag this file
				char buf[40];
				BNode		oNode(&oldRef);
				BNode		node(&newRef);
				
				node_ref	nodeRef;
				node.GetNodeRef(&nodeRef);
				sprintf(buf,"%Ld%Ld",nodeRef.node,system_time());
				PRINT(("Tag value is %s\n",buf));
				
				if (!oNode.InitCheck() && !oNode.Lock()) {
					attr_info	info;
					if (!oNode.GetAttrInfo("pkgtag",&info)) {
						int len = min_c(39,info.size);
						if (len == oNode.ReadAttr("pkgtag",B_STRING_TYPE,0,buf,len)) {
							// null terminate
							if (buf[len]) buf[len+1] = 0;
						}
						else {
							// danger here, buf could be garbage
							sprintf(buf,"%Ld%Ld",nodeRef.node,system_time());
						}
					}
					oNode.Unlock();
				}
				
				
				
				if (node.Lock() == B_NO_ERROR) {
					PRINT(("Tag value is %s\n",buf));
					int len = strlen(buf) + 1;
					int res;
					res = node.WriteAttr("pkgtag",B_STRING_TYPE,0,buf,len);
					node.Unlock();
					if (res == len) {
						// now save the tag
						if (it)
							fPackData.WriteString(&storeFile,buf,'FTag');
					}
					else {
						//error writing tag
					}
				}
			}
			
			
			readingEntries = header.what == 'NDev';
		}
	}
	if (it)
		fPackData.WriteEndHeader(&storeFile);

	fNewFiles.Unset();
#endif
	
	BEntry	ent;
	if (someErrors) {
		// move to trash instead??
		char buf[80];
		sprintf(buf,"Installer Rescued Items %d",(long)(system_time()/10000.0));
		tempDir.GetEntry(&ent);
		ent.Rename(buf);
	}
	else {
		/* delete all the items in the temp folder */
		PRINT(("deleting remaining items in the temp folder\n"));
		tempDir.GetEntry(&ent);
		err = RecursiveDelete(&ent);
		if (err != B_NO_ERROR) {
			// report the error with volume name
			char vname[B_FILE_NAME_LENGTH];			
			BVolume	vol;
			tempDir.GetVolume(&vol);
			vol.GetName(vname);
			doError(errRMTMPDIR,vname);
		}
		PRINT(("temp directory removed\n"));
	}
	if (fLogFile) {
		/* close the log file */
		delete fLogFile;
		fLogFile = NULL;
	}

	PRINT(("finishInstall returning\n"));
}

BFile	*DestManager::LogFile()
{
	if (fLogFile == NULL) {
		char lfilename[B_FILE_NAME_LENGTH];
		strcpy(lfilename, attr->name);
		const char postfix[] = " Install Log";
		
		if (*lfilename) {
			AppendPostfix(lfilename,B_FILE_NAME_LENGTH,postfix);
		}
		else {
			// for installs with null name
			strcpy(lfilename,postfix+1);
		}	
		long err = B_NO_ERROR;
		fLogFile = new BFile();

#if (SEA_INSTALLER)
		logDir = rootDir;
#endif
		BEntry fLogEntry;
		if (logDir.FindEntry(lfilename,&fLogEntry) >= B_NO_ERROR) {
			err = fLogFile->SetTo(&fLogEntry,O_RDWR);
			
			if (err >= B_NO_ERROR)
				err = fLogFile->SetSize(0);
		}
		else {
			err = logDir.CreateFile(lfilename,fLogFile);
			if (err >= B_NO_ERROR) {
				err = logDir.FindEntry(lfilename,&fLogEntry);
				fLogFile->SetTo(&fLogEntry,O_RDWR);
			}
		}
		if (err < B_NO_ERROR) {
			// error opening/creating file
			doError(errMKLOGFILE);
			logging = false;
		}
		else {
			// set mime type of logfile
			BNode	node(&fLogEntry);
			BNodeInfo	inf(&node);
			inf.SetType(LOG_FILETYPE);
		}
	}
	return fLogFile;
}

#include "REntryList.h"

status_t DestManager::FetchQuery(FindItem *criteria,
									REntryList *result)
{
	status_t	err = B_NO_ERROR;
	BVolume		instVol(installDev);
	int32 count = criteria->CountPredicates();
	BQuery	*squery = new BQuery();
	REntryList	*tempList = new REntryList();
		
	if (count) {
		squery->Clear();
		
		squery->SetVolume(&instVol);
		err = squery->SetPredicate(criteria->PredicateAt(0));
		if (err != B_NO_ERROR) goto err;
		
		PRINT(("Fetching %s\n",criteria->PredicateAt(0)));
		err = squery->Fetch();
		if (err != B_NO_ERROR) goto err;
		
		
		int32 num = squery->CountEntries();
		// add all the found items
		
		result->SetTo(squery);
	}
	else {
		err = B_ERROR;
		goto err;	
	}

	for(int32 i = 1; i < count; i++) {
		// run a query
		squery->Clear();
		squery->SetVolume(&instVol);
		err = squery->SetPredicate(criteria->PredicateAt(i));
		if (err) goto err;
		PRINT(("Fetching %s\n",criteria->PredicateAt(i)));

		err = squery->Fetch();
		if (err) goto err;
		
		PRINT(("found %d items\n",squery->CountEntries()));
		// intersect the results with what we already have
		tempList->SetTo(squery);
		result->QueryAnd(tempList);
		if (result->CountEntries() == 0) {
			PRINT(("result is now zero\n"));
			break;
		}
	}
	
err:
	delete squery;
	delete tempList;
	return err;
}


BDirectory *DestManager::DirectoryFor(long index, 
			BDirectory *parent, bool custom, bool create,
			ArchivePatchItem *patch)
{
	DestItem *di = NULL;
	
	BDirectory	*result = NULL;
	
	if (custom)
	{
		PRINT(("custom path\n"));
		di = fCustList->ItemAt(index);
		
		if (!di)
			doError(errBADDESTLIST);
		// has full pathname/query
	}
	else if (index == D_PARENT_FOLDER)
	{
		//PRINT(("parent folder\n"));
		result = parent;
		// pathname must be built	
	}
	else if (index == D_INSTALL_FOLDER)
	{
		PRINT(("install folder\n"));
		if (!fInstall) {
			PRINT(("need to init install directory\n"));
			if (dirName) {
				// non custom
				PRINT(("non-custom path %s\n",dirName));
				BEntry		installEntry;
				
				long err;
				err = rootDir.FindEntry(dirName,&installEntry);
				if (!installEntry.Exists() && create) {
					// paths need to be created
					err = CreatePathAt(&rootDir,dirName,&installEntry);
					if (err < B_NO_ERROR) {
						doError(errINSTDIR,dirName);
					}
				}
				if (installEntry.Exists() && installEntry.IsDirectory()) {
					fInstall = new BDirectory(&installEntry);
					if (fInstall->InitCheck() < B_NO_ERROR) {
						delete fInstall;
						fInstall = NULL;
						PRINT(("fInstall is invalid\n"));
					}
					PRINT(("fInstall is set!\n"));	
				}
			}
		}
		
		result = fInstall;
		// has pathname (must be set at start)
	}
	else
	{
		di = fDefList->ItemAt(index);
		// has full pathname/query
	}
	
	// lazy initialization of directory list
	if (di && di->dir) {
		PRINT(("path already initialized, di->dir is %d\n",di->dir));
		result = di->dir;
	}
	else if (di) {
		// do query paths here
		PRINT(("initializing path\n"));
		
#if DOES_PATCHING
		
		if (typeid(*di) == typeid(FindItem)) {
			PRINT(("find item\n"));
			FindItem *fi = (FindItem *)di;
			
			// if we have already seen this query and it failed
			// don't bother to do it again
			// although maybe now it will succeed?
			
			// for dr9
			fi->queryFailed = FALSE;
			
			if (fi->queryFailed) {
				PRINT(("QUERY MARKED AS FAILED\n"));	
				return NULL;
			}
		
			status_t err;
			
			REntryList *query = new REntryList();
			err = FetchQuery(fi,query);
		
			// count is reliable on REntryList
			long count = query->CountEntries();
			if (err < B_NO_ERROR || count < 1) {
				PRINT(("\n*************\nQUERY FAILED\n\n"));
				PRINT(("ID COUNT IS %d\n",query->CountEntries()));
				PRINT(("ERROR CODE IS %d\n",err));
				fi->queryFailed = TRUE;
			}
			else {
				entry_ref foundItem;
				query->Rewind();
					
				if (count == 1) {
					query->GetNextRef(&foundItem);
				}
				else {
					bool continueInstall;	
					// multiple items found, present a choice
					FindChoiceWind *w = new FindChoiceWind(query,
												&continueInstall,
							 					&foundItem,fi);
					// wait for a result
					w->Go();
					canceled = !continueInstall;
				}
				fi->foundItem = foundItem;

				BEntry		foundEnt(&foundItem);
				
				PRINT(("getting parent directory of found item\n"));
				fi->dir = new BDirectory();
				err = foundEnt.GetParent(fi->dir);			
				if (err < B_NO_ERROR) {
					PRINT(("parent directory of find item not found\n"));
					fi->queryFailed = TRUE;
					delete fi->dir;
					fi->dir = NULL;
				}
			}
			delete query;
		}
		else
#endif	// DOES_PATCHING

		if (typeid(*di) == typeid(DestItem)) {
			// for regular paths

			status_t 	err;
			BEntry		destPath;

			if (di->findCode >= 0) {
				PRINT(("find directory path\n"));
				// find directory call
				BPath		tpath;
				const char	*path;
				path = root_find_directory((directory_which)di->findCode,&tpath);
				if (path) {
					err = rootDir.FindEntry(path,&destPath);
					PRINT(("destPath set err %d %s\n",err,path));
					if (!destPath.Exists() && create) {
						PRINT(("creating find directory path\n"));
						err = CreatePathAt(&rootDir,path,&destPath);
						if (err  < B_OK) {
							doError(errINSTDIR,path);
						}
					}
					if (destPath.Exists() && destPath.IsDirectory())
					{
						PRINT(("Bdirectory set\n"));
						di->dir = new BDirectory(&destPath);
					}
				}
			}
			else {
				PRINT(("full or relative directory path\n"));
				// deal with null pathnames!!!
				if (di->path[0] == '/') {
					err = destPath.SetTo(di->path);
				}
				else {
					err = rootDir.FindEntry(di->path,&destPath);
				}
				if (!destPath.Exists() && create) {
					if (di->path[0] == '/') {
						// must try and start with first part
						// of pathname
						char *slash = strchr(&(di->path[1]),'/');
						
						if (slash) {
							// do all this to find the root directory of the path
							long prefixLen = slash - di->path;
							char rootName[B_FILE_NAME_LENGTH+2];
							strncpy(rootName,di->path,prefixLen);
							rootName[prefixLen] = '\0';
							
							BDirectory	tempRoot(rootName);
							err = CreatePathAt(&tempRoot, di->path + prefixLen+1, &destPath);
						}
					}
					else {
						err = CreatePathAt(&rootDir,di->path,&destPath);
					}
					if (err < B_OK) {
						doError(errMKCUSTPATH,di->path);
						di->dir = NULL;
					}
				}
				if (destPath.Exists() && destPath.IsDirectory()) {
					di->dir = new BDirectory(&destPath);
				}
			}
		}
		// may be null
		result = di->dir;
	}

#if DOES_PATCHING	
	if (patch && result) {
		// this destination and this patch item are associated	
		
		// case 1 if the destination is null then this patch item could not
		// be found, return null
		
		// case 2 the destination is a query item with the same
		// params as this patch item
		FindItem *fi;
		// we know the query succeeded since result was not NULL

		bool patchFound = FALSE;
		
		if ( (fi = cast_as(di,FindItem)) && patch->oldFileSize == fi->size)
		{
			// verify type strings
			// should be a regexp match???
			patch->fileRef = fi->foundItem;
			patchFound = TRUE;
		}
		// case 3 the destination is or is not a query item
		// do a separate query for this item and see if its parent
		// (there may be a separate query needed somewhere down the line
		//  if a query destination derived from this item is used elsewhere)

		// more simply stated
		// we want to see if this patch resides in the result folder!
		if (!patchFound) {
			// RList<entry_ref *>	foundList;	

			bool found = FALSE;
			result->Rewind();
			BEntry	dirEnt;
			while (!found && result->GetNextEntry(&dirEnt) == B_NO_ERROR) {
				// don't follow symlinks
				//if (FollowSymLink(&dirEnt) != B_NO_ERROR)
				//	continue;
				if (dirEnt.IsFile()) {
					off_t fsize;
					dirEnt.GetSize(&fsize);
					if (fsize == patch->oldFileSize) {
						// this might be the right file
						
						// verify type and possibly checksum?
						found = TRUE;
						// foundList.AddItem(new record_ref(foundItem));
					}
				}
			}
			if (!found) {
				PRINT(("file query failed to find item\n"));
				result = NULL;
			}
			else {
				dirEnt.GetRef(&patch->fileRef);
			}

			// delete all items in foundList;
			//for (long i = 0; i < count; i++)
			//	delete foundList.ItemAt(i);
		}					
	}
#endif

	// PRINT(("returning directory %d\n",result));
	return result;
}

void	DestManager::SafeDelete(BEntry *item, BDirectory *parent)
{
	BEntry parEnt;
	parent->GetEntry(&parEnt);

	long err = item->MoveTo(&tempDir);
	if (err != B_NO_ERROR) {
		char name[B_FILE_NAME_LENGTH];
		item->GetName(name);
		if (tempDir.Contains(name)) {
			BDirectory enclFolder;
			sprintf(name,"Installer Ï·«¨  Temp%d",encloseFolderCount++);
			err = tempDir.CreateDirectory(name,&enclFolder);
			err = item->MoveTo(&enclFolder);
		}
		if (err != B_NO_ERROR) 
			doError(errMOVETOTEMP,name);
	}
	
#if (!DOES_UNINSTALL)
	DeleteItem *di = new DeleteItem;
	parEnt.GetRef(&di->dirRef);
	item->GetRef(&di->ref);
	deletia->AddItem(di);
#endif
}

status_t	DestManager::BeginRememberList()
{
#if (DOES_UNINSTALL)
	char buf[40];
	sprintf(buf,"%Ldnew_files",system_time());
	fNewFiles.SetTo(TempDir(),buf,O_RDWR | O_CREAT);
	fNewFiles.Seek(0,SEEK_SET);
	
	fPackData.WriteRecordHeader(&fNewFiles,'FLis',LIS_TYPE,0);
#endif
	return B_NO_ERROR;
}

// remember old/newfile pair
status_t	DestManager::RememberFile(BEntry *file, BEntry *oldFile)
{
#if (DOES_UNINSTALL)
	PRINT(("remembering file\n"));
	entry_ref ref;
	file->GetRef(&ref);
	
	// device
	fPackData.WriteInt32(&fNewFiles,ref.device,'NDev');
	// directory
	fPackData.WriteInt64(&fNewFiles,ref.directory,'NIno');
	// name
	fPackData.WriteString(&fNewFiles,ref.name,'NNam');
	
	if (oldFile->InitCheck() >= B_NO_ERROR) {
		oldFile->GetRef(&ref);
		// device
		fPackData.WriteInt32(&fNewFiles,ref.device,'ODev');
		// directory
		fPackData.WriteInt64(&fNewFiles,ref.directory,'OIno');
		// name
		fPackData.WriteString(&fNewFiles,ref.name,'ONam');
	}
#else
	file; oldFile;
#endif

	return B_NO_ERROR;
}

long	DestManager::AllocSize(long size)
{
	return size + AllocExtra(size);
}

long	DestManager::AllocExtra(long size)
{
	// not right!!
	long rem = size % allocBlockSize;
	return rem ? allocBlockSize - rem : 0;
}

bool	DestManager::InstallPlatform(int32 platform)
{
	int32 curPlat;
#if __INTEL__
	curPlat = (1 << 1);
#else
	curPlat = (1 << 0);
#endif
	return platform & curPlat;
}

// delete everything recursively
long	RecursiveDelete(BEntry *ent)
{
	if (ent->IsDirectory()) {
		// directory
		BDirectory	dir(ent);
		dir.Rewind();
		BEntry dEnt;
		while(dir.GetNextEntry(&dEnt) >= B_NO_ERROR) {
			RecursiveDelete(&dEnt);
		}
		return ent->Remove();
	}
	else {
		// file
		return ent->Remove();
	}
}

#if 0
status_t FollowSymLink(BEntry *entry)
{
	int32 count = 0;	// limit depth in case of loops
	BPath	linkPath;
	while(entry->IsSymLink() && count < 64) {
		BSymLink	link(entry);
		link.GetLinkedPath(&linkPath);
		entry->SetTo(linkPath.Path());
		count++;
	}
	if (count >= 64)
		return B_ERROR;
	return B_NO_ERROR;
}
#endif

void WritePath(BDirectory *dir, BFile *output, char *buf)
{
	BEntry			dEnt;
	dir->GetEntry(&dEnt);
	
	BDirectory	parent;
	
	if ((dEnt.GetParent(&parent) == B_NO_ERROR) && (parent != *dir)) {
		WritePath(&parent,output,buf);
	}
	else {
		output->Write("/",1);
		return;
	}
	dEnt.GetName(buf);
	output->Write(buf,strlen(buf));
	output->Write("/",1);
}

void PrintNodeType(BEntry *e)
{
	struct stat s;
	e->GetStat(&s);
	if (S_ISREG(s.st_mode)) {
		PRINT(("Node type is regular\n"));
	}	
	else if (S_ISLNK(s.st_mode)) {
		PRINT(("Node type is link\n"));
	}
	else if (S_ISBLK(s.st_mode)) {
		PRINT(("Node type is block\n"));
	}
	else if (S_ISDIR(s.st_mode)) {
		PRINT(("Node type is directory\n"));
	}
	else if (S_ISCHR(s.st_mode)) {
		PRINT(("Node type is char\n"));
	}
	else if (S_ISFIFO(s.st_mode)) {
		PRINT(("Node type is FIFO\n"));
	}
}
