// InstallLooper.cpp
#include <Path.h>
#include <time.h>
#include "InstallLooper.h"
#include "InstallMessages.h"
#include "InstallPack.h"

#include "IArchiveItem.h"
#include "IArchivePatchItem.h"
#include "IArchiveScriptItem.h"

#include "ArcCatalog.h"
#include "DestManager.h"
#include "PatchInfoWind.h"

#include "FileTree.h"
#include "PreviewWind.h"

#include "AutoPtr.h"
//#include "PanelLooper.h" // not used anymore

#include "MyDebug.h"
#include "Util.h"

#include "InstallerType.h"
#include "PackageDB.h"
#include "PackageItem.h"
#include "PackAttrib.h"
#include "SettingsManager.h"
#include "Log.h"

#if DEBUG
#include <StopWatch.h>
int64 treeFileTime;
#endif

extern	SettingsManager *gSettings;

InstallLooper::InstallLooper(InstallPack *arc)
	: BLooper("install looper")
{
 	PRINT(("file looper created\n"));
	arcFile = arc;
}

thread_id InstallLooper::Run()
{
	PRINT(("starting install looper\n"));
	statWindow = new StatusWindow(BRect(0,0,260,90), "Install");
	statWindow->packFile = arcFile;
	arcFile->statusMessenger = BMessenger(statWindow->FindView("statview"));
	
	return BLooper::Run();
}

bool InstallLooper::QuitRequested()
{
	PRINT(("file looper got quit requested\n"));
	
	long err = statWindow->PostMessage(B_QUIT_REQUESTED);
	if (err != B_NO_ERROR) {
		doError(errBADSTATWIND);
	}
	return TRUE;
}

// Message format
// item count
// bytes
// toplevel item
// selected install directory
// destManager pointer

void InstallLooper::DoExtractItems(BMessage *msg)
{
	long					err = B_NO_ERROR;
	ArchiveFolderItem		*topitem;
	ulong					chosenGroups;
	DestManager				*destObject;
	long					totalItems;
	long					count;
	bool					closeWhenComplete = false;
	
//#if (!SEA_INSTALLER)
	InitInstall();
// #endif
	
	msg->FindPointer("archive item",(void **)&topitem);
	chosenGroups = msg->FindInt32("groups");
	totalBytes = msg->FindInt32("bytes");
	totalItems = msg->FindInt32("item count");
	msg->FindPointer("destinations",(void **)&destObject);
	bool cancelInst = FALSE;
	
	installFolderGroups = destObject->attr->instFolderGroups;
	if (!destObject->doInstallFolder) installFolderGroups = 0;
	
	
#if DOES_PATCHING
	//////////////////
	// Patch stuff
	patchList = new RList<ArchivePatchItem *>;
	
	arcFile->SetFile();

	fCatalog->Set(arcFile);	
	fCatalog->Rewind();
	FindPatches(NULL,destObject,chosenGroups,TRUE);
	count = patchList->CountItems();
	
	arcFile->ClearFile();

	cancelInst = destObject->canceled;
	if (cancelInst) count = 0;
	
	if (count > 0) {
		// need at least one patch
		bool patchesOK = FALSE;

		AutoPtr<char> cbuf(PATH_MAX);
		char *buf = cbuf.Value();
		BPath path;
		for (long i = 0; i < count; i++) {
			ArchivePatchItem *patch = patchList->ItemAt(i);
			
			entry_ref pRef = patch->fileRef;
			BEntry	pEnt(&pRef);
			if (pEnt.InitCheck() < B_NO_ERROR) {
					// running modally
					
				// put up a file panel for unfound patches??
				/****
				BFilePanel *panel = new BFilePanel(B_OPEN_PANEL,
													NULL,
													NULL,
													false,
													false,
													0,
													NULL,
													true);
				BMessage *result = new BMessage();
				PanelLooper *p = new PanelLooper(panel, result);
				p->Go();
			
				result->FindRef("refs",&patch->fileRef);	
				****/
				PRINT(("PATCH FOR %s NOT FOUND (file error)\n",patch->name));
				patch->fileRef.device = patch->fileRef.directory = -1;
				
				totalItems--;
				totalBytes -= patch->uncompressedSize;
			}
			else {
				#if (DEBUG)
				pEnt.GetPath(&path);
				PRINT(("PATH FOR %s PATCH IS %s\n",patch->name,path.Path()));
				#endif
				patchesOK = TRUE;
			}
		}
		// throw up a window with the info
		// this window initially

		if (patchesOK) {
			PatchInfoWind *infoWind = new PatchInfoWind(patchList,&patchesOK,
										&(destObject->deleteOriginals));
			
			// resume the window and wait for it to quit
			PRINT(("ready to show info window\n"));
			infoWind->Go();
			// patchesOK has the result whether to continue or not
			cancelInst = !patchesOK;
		}
		else {
			cancelInst = TRUE;
			doError(errNOPATCHES);
		}
	}			
#endif 	//// end patch stuff////

//////// previewing /////////

#if (!SEA_INSTALLER)
	if (!cancelInst) {
		if (gSettings->data.FindBool("install/preview"))
			DoPreview(destObject,chosenGroups,cancelInst);
	}
#endif



	if (cancelInst) {
		updateMessenger.SendMessage(M_DONE);
		destObject->FinishInstall(err == B_CANCELED);
#if DOES_PATCHING
		for (int i = patchList->CountItems()-1; i >= 0; i--)
			delete patchList->ItemAt(i);
		delete patchList;
#endif
		return;
	}	

////////////////////////////
	////////////////////
	BMessage exMsg(M_EXTRACT_ITEMS);
	exMsg.AddInt32("item count",totalItems);
	exMsg.AddInt32("bytes",totalBytes);
	arcFile->statusMessenger.SendMessage(&exMsg);
	
	arcFile->byteUpdateSize = (long)max_c(totalBytes/STATUS_FREQUENCY,1);
	arcFile->bucket = 0;
	
	currentPatchCount = 0;
	err = arcFile->BeginDecompression();
	if (err >= B_NO_ERROR) {		
		if (destObject->logging) {
			BFile *log = destObject->LogFile();

			char buf[B_FILE_NAME_LENGTH + 80];
			sprintf(buf,"Installation: %s\n",destObject->attr->name);
			log->Write(buf,strlen(buf));
			sprintf(buf,"Started: ");
			log->Write(buf,strlen(buf));
			time_t caltime = time(NULL);
			char *locTimeStr = ctime(&caltime);
			PRINT((locTimeStr));
			log->Write(locTimeStr,strlen(locTimeStr));
			log->Write("\n\n",2);
		}
	#if DEBUG
		BStopWatch	*installTime = new BStopWatch("installation time");
	#endif
		
		// err = topitem->ExtractItem(arcFile,NULL,destObject,chosenGroups);
	
		arcFile->SetFile();
		
		fCatalog->Set(arcFile);
		fCatalog->Rewind();
		err = InstallEntries(NULL,destObject,chosenGroups,TRUE);
		arcFile->ClearFile();
			
		arcFile->EndDecompression();
			
	#if DEBUG
		delete installTime;
	#endif
		
		// this call restores items to their original location if canceled
		// otherwise it deletes the temporary items
		PackageItem *it = NULL;
		#if (DOES_REGISTRY)
		if (err >= B_NO_ERROR ) {
			// add to registry (all installer types)
			// only adds if developer has set info
			// reigstry & registration
			it = AddToRegistry(destObject->attr,destObject->installDev);
			#if (!SEA_INSTALLER)
			if (it) {
				// registration

				// log the installation
				Log	valetLog;
				
				BMessage	logMsg(Log::LOG_INSTALL);
				logMsg.AddString("pkgname",it->data.FindString("package"));
				
				const char *versn = it->data.FindString("version");
				
				char	*buf = (char *)malloc(strlen(versn) + 40);
				sprintf(buf,"Version: %s",versn);
				logMsg.AddString("strings",buf);
				free(buf);
				logMsg.AddString("strings","Installation successful");
				valetLog.PostLogEvent(&logMsg);
				
			}
			#endif
		}
		#endif
		
		if (destObject->logging) {
			BFile *log = destObject->LogFile();
			char buf[B_FILE_NAME_LENGTH];
			log->Write("\n\n",2);
			if (err == B_CANCELED)
				sprintf(buf,"Canceled: ");
			else
				sprintf(buf,"Completed: ");
			log->Write(buf,strlen(buf));
			time_t caltime = time(NULL);
			char *locTimeStr = ctime(&caltime);
			log->Write(locTimeStr,strlen(locTimeStr));
			log->Write("\n",1);
		}
		// remember the installation
		destObject->FinishInstall(err == B_CANCELED, it);		

		if (err >= B_NO_ERROR) {
			// present user with option to quit
			PRINT(("sending done message to status window\n"));
			arcFile->statusMessenger.SendMessage(M_DONE);

			int result = doError("Installation is finished. Would you like to continue performing\
 installations or close the installer?",NULL,"Continue","Close");

			closeWhenComplete = result;

#if (!SEA_INSTALLER)
			bool autoRegister = (gSettings->data.FindInt32("register/mode")
									== SettingsManager::DO_REGISTER);
			// do additional checks here to see if we should
			// register the package at all
			if (it 	&& autoRegister &&
					it->ValetSupported() &&
					it->data.FindBool("regservice") &&
					(it->data.FindInt32("registered") == PackageItem::REGISTERED_NO) )
			{	
				BMessage reg('IReg');
				reg.AddPointer("package",it);
				be_app->PostMessage(&reg);
			}
			else {
				delete it;	
			}
#endif

		}
#if (!SEA_INSTALLER)
		else if (err == B_CANCELED) {
			// err == CANCELED
			// clean up installed files
			// present user with option to remove all files/folders that were created
			if (destObject->attr->name && *(destObject->attr->name)) {
				Log	valetLog;
				BMessage	logMsg(Log::LOG_INSTALL);
				logMsg.AddString("pkgname",destObject->attr->name);
				logMsg.AddString("strings","Installation aborted!");
				valetLog.PostLogEvent(&logMsg);
			}
		}
#endif		

	#if (!SEA_INSTALLER)
		FinishInstall();	
	#endif
		
	}
	#if (DOES_PATCHING)
		for (int i = patchList->CountItems()-1; i >= 0; i--)
			delete patchList->ItemAt(i);
		delete patchList;
	#endif		

	PRINT(("sending done message to status window\n"));
	
	/* this may be redundant if the window is already closed */
	arcFile->statusMessenger.SendMessage(M_DONE);
	PRINT(("sending done message to window\n"));
	BMessage	doneMsg(M_DONE);
	doneMsg.AddBool("close",closeWhenComplete);
	updateMessenger.SendMessage(&doneMsg);
}


void InstallLooper::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_EXTRACT_ITEMS:
			DoExtractItems(msg);
			break;
		default:
			break;
	}
}


long InstallLooper::InstallEntries( BDirectory *parDir,
									DestManager *dests,
									ulong inGrps,
									bool topDir,
									bool skip )
{
	BDirectory	resultDir;
	status_t err = B_OK;

	/*** get folder ***/

	ArchiveFolderItem	*curFolder;
	fCatalog->GetFolderData((ArchiveItem **)&curFolder);
	
	/**
		do action
	**/
#if DEBUG
	PRINT(("read folder %s\n",curFolder->name));
	if (topDir)
		PRINT(("THIS IS THE TOP LEVEL FOLDER\n"));
#endif
	/* topDir is a bool, set by the caller which lets us know if we are at the top
	 of the recursion */
	/* skip is a bool set by the caller which lets us know we are taking no actions for
	 this item */
	
	/* we perform the installation if the basic groups are set and no skipping
	 and we are not the topDir or if we are the topDir then the installFolderGroups must be set*/

	if ((curFolder->groups & inGrps) && !skip &&
		(dests->InstallPlatform(curFolder->platform)) &&
		(!topDir || (inGrps & installFolderGroups))) {
		// do action if there are items in this folder
		PRINT(("CREATING FOLDER %s\n",curFolder->name));
		PRINT(("INGRPS is %.8x INSTALLFOLDERGROUPS %.8x\n",inGrps,installFolderGroups));

		curFolder->isTopDir = topDir;	 // hack
		curFolder->newDir = &resultDir;  // hack
		err = curFolder->ExtractItem(arcFile,parDir,dests,inGrps,skip);
		if (resultDir.InitCheck() != B_OK)
			skip = true;
	}
	else {
		PRINT(("folder not created, nothing will reside in it\n"));
	}
	// abort install
	if (err < B_NO_ERROR) {
		return err;
	}
	
	/** end action **/
	
	fCatalog->StartFolderGet();
	int32 subitem;
	while (subitem = fCatalog->GetFolderItemType()) {
		switch(subitem) {
			case FOLDER_FLAG: {
				err = InstallEntries(&resultDir,dests,inGrps,false,skip);
				if (err < B_NO_ERROR)
					return err;
				break;
			}
			case FILE_FLAG: 
			case SCRIPT_FLAG: 
			case LINK_FLAG:
			{
				ArchiveItem	*curItem;
				fCatalog->GetItemData(&curItem, subitem);
				/**
				do action
				**/
				if ((curItem->groups & inGrps) &&
					(dests->InstallPlatform(curItem->platform)))
				{
					err = curItem->ExtractItem(arcFile,&resultDir,dests,inGrps,skip);
					if (err < B_NO_ERROR) {
						return err;
					}
				}
				// end action
				break;
			}
			case PATCH_FLAG: {
				ArchivePatchItem	*curPatch;
				fCatalog->GetPatchData((ArchiveItem **)&curPatch);
				/**
				do action
				**/
				if ((curPatch->groups & inGrps) &&
					(dests->InstallPlatform(curPatch->platform)))
				{
					ArchivePatchItem *realPatch = patchList->ItemAt(currentPatchCount++);
					ASSERT(realPatch);
					
					err = realPatch->ExtractItem(arcFile,&resultDir,dests,inGrps,skip);
					if (err < B_NO_ERROR) {
						return err;
					}
				}
				break;
			}
		}
	}
	fCatalog->EndFolderGet();
	
	return err;
}

#if (!SEA_INSTALLER)
void InstallLooper::DoPreview(DestManager *destObject,
								uint32 chosenGroups,
								bool &cancelInst)
{
	FileTree *prevTree = new FileTree(BRect(10,10,200,200));
	
	arcFile->SetFile();
	fCatalog->Rewind();

#if DEBUG
	treeFileTime = 0;
#endif
	int64 tt = system_time();
	BuildDisplay(NULL,destObject,
				chosenGroups,prevTree->Root(),TRUE);
	tt = system_time() - tt;
	PRINT(("Total time is %f seconds\n",
			(double)((double)tt/1000000.0)));
	PRINT(("File time is %f seconds\n",
			(double)((double)treeFileTime/1000000.0)));
	
	arcFile->ClearFile();
	
	TreeItem *ti = (TreeItem *)destObject->instFolderTreeItem;
	if (ti) {
		// collapse the children
		ti->DoForAllChildren(&TreeItem::Collapse);
		prevTree->Refresh(FALSE);
	}
	PRINT(("creating preview window\n"));
	PreviewWind *infoWind = new PreviewWind(prevTree,&cancelInst);
		// resume the window and wait for it to quit
		
	infoWind->Go();
}

#include "PTreeItem.h"
long	InstallLooper::BuildDisplay(PTreeItem *parItem,
									DestManager *dests,
									ulong inGrps,
									TreeItem *root,
									bool topDir )
{
	status_t err = B_NO_ERROR;

#if DEBUG
	int64 t = system_time();
#endif
	/*** get folder ***/

	ArchiveFolderItem	*curFolder;
	fCatalog->GetFolderData((ArchiveItem **)&curFolder);
	
	/**
		do action
	**/

#if DEBUG
	treeFileTime += system_time()-t;
#endif
	
	PTreeItem *tItem;
	PTreeItem *tFolder = NULL;
	
	if ((curFolder->groups & inGrps) &&
			(dests->InstallPlatform(curFolder->platform)) &&
			(!topDir || (inGrps & installFolderGroups))) {
		// do action
		#if DEBUG
		if (topDir)
			PRINT(("at toplevel folder\n"));
		#endif
		
		//PRINT(("dest for folder %s\n",curFolder->name));
		// return the tree item for our destination
		tItem = dests->TreeItemFor( curFolder->destination,
									parItem,
									root,
									curFolder->customDest);
		tFolder = new PTreeFolderItem(curFolder->name);
		// add us as a child of the item
		tItem->AddChild(tFolder);
	
		if (topDir) {
			PRINT(("setting instFolderTreeItem\n"));
			dests->instFolderTreeItem = tFolder;
		}
	}
	
	/** end action **/
	
	fCatalog->StartFolderGet();
	int32 subitem;
	while (subitem = fCatalog->GetFolderItemType()) {
		// seek to current location in catalog
		switch(subitem) {
			case FOLDER_FLAG:
				// recurse
				BuildDisplay(tFolder,dests,inGrps,root,FALSE);
				break;
			case LINK_FLAG:
			case FILE_FLAG:
			{
				// read a file
				ArchiveItem	*curItem;
				fCatalog->GetItemData(&curItem,subitem);
				
				//PRINT(("dest for file %s\n",curFile->name));
				if ((curItem->groups & inGrps) &&
					(dests->InstallPlatform(curItem->platform))) {
					tItem = dests->TreeItemFor(	curItem->destination,
											   	tFolder,root,
											   	curItem->customDest );
					PTreeItem *tFile = new PTreeFileItem(curItem->name);
					tItem->AddChild(tFile);
				}
				// end action
				break;
			}
			default:
				ArchiveItem	*curItem;
				fCatalog->GetItemData(&curItem,subitem);
				break;
		}
	}
		
	return err;
}
#endif


void InstallLooper::InitInstall()
{
	arcFile->doCancel = 		FALSE;
	fCatalog = new ArcCatalog();
	//fCatalog->Set(arcFile);
	//curFolder = new ArchiveFolderItem();
	//curFile = new ArchiveFileItem();
}

void InstallLooper::FinishInstall()
{
	delete fCatalog;
	//delete curFolder;
	//delete curFile;
	//curFolder = NULL;
	//curFile = NULL;
}


#if (DOES_REGISTRY)
PackageItem *InstallLooper::AddToRegistry(PackAttrib *attr,
										dev_t installDev)
{
	// add/update package in registry
	
	// developer has not specified a pkgname
	if (attr->name[0] == 0)
		return NULL;

	PackageItem *it = new PackageItem();
	PackageDB db;
	
	char	newname[256];
	char	depotsn[32];
	char	vid[32];
	int32	media = 0;	// esd type
	int32	soft = 255; // unknown type
	bool	service;
	bool	remapped = false;
	
	struct tm tm;
	memset(&tm,0,sizeof(struct tm));
	tm.tm_year = 1998 - 1900;
	tm.tm_mon = 2;	// mar.
	tm.tm_mday = 13;	// 13th
	time_t	cutOffDate = mktime(&tm);
	
	if (attr->releaseDate < cutOffDate) {
		remapped = db.MapOldPackageName(attr->name,newname,depotsn,media,soft,service,vid);
	}	
	bool found;
	// frist search for matching serialID, prefixID
	const char *serialID = attr->downloadInfo.FindString("sid");
	const char *prefixID = attr->downloadInfo.FindString("pid");
	if (serialID && *serialID && prefixID && *prefixID &&
		db.FindSerialID(it, serialID, prefixID, installDev) >= B_OK)
	{
		// found the exact serial number
		found = true;			
	}
	else if (db.FindPackage(it,remapped ? newname : attr->name,attr->version,
			attr->developer,installDev) >= B_OK)
	{
		// found the exact version installed
		found = true;
	}
	else if (db.FindNextOlderVersion(it, attr->name, attr->developer,
										attr->releaseDate,
										installDev) >= B_OK) {
		// found the cloeset version for an update
		found = true;
	}
	else {
		found = false;
	}
	
	BMessage	&data = it->data;
	
	ReplaceString(&data,"package",remapped ? newname : attr->name);
	ReplaceString(&data,"version",attr->version);
	ReplaceString(&data,"developer",attr->developer);
	ReplaceString(&data,"description",attr->description);
	ReplaceString(&data,"platforms",B_EMPTY_STRING); // to do in 2.0
	ReplaceString(&data,"languages",B_EMPTY_STRING); // to do in 2.0
	ReplaceInt32(&data,"mediatype",media);				
	if (attr->downloadInfo.HasInt32("softtype"))
		soft = attr->downloadInfo.FindInt32("softtype");
	ReplaceInt32(&data,"softtype",soft);
	ReplaceInt32(&data,"releasedate",attr->releaseDate);
	ReplaceInt32(&data,"installdate",time(0));
	if (totalBytes > data.FindInt32("installsize"))
		ReplaceInt64(&data,"installsize",totalBytes);
	// registered (don't touch)
	const char *psid = attr->downloadInfo.FindString("sid");
	ReplaceString(&data,"sid",psid);
	ReplaceString(&data,"pid",attr->downloadInfo.FindString("pid"));
	ReplaceString(&data,"vid",attr->downloadInfo.FindString("vid"));
	if (remapped && service && !(psid && *psid)) {
		if (soft == 0)
			ReplaceString(&data,"sid","old");
		ReplaceString(&data,"pid","10000");
		ReplaceString(&data,"vid",vid);
	}
	
	ReplaceString(&data,"depotsn",remapped ? depotsn : attr->devSerial);
	
	ReplaceBool(&data,"regservice",remapped ? service : attr->doReg);
	ReplaceBool(&data,"upservice",remapped ? service : attr->doUpdate);

#if DEBUG
	data.PrintToStream();
#endif
		
	if (found) {
		PRINT(("calling db write package\n"));
		db.WritePackage(it,PackageDB::BASIC_DATA);
	}
	else {
		PRINT(("calling db add package\n"));
		db.AddPackage(it,installDev);
	}
#if (!SEA_INSTALLER)
	BMessage	updtDisplay('PDis');
	updtDisplay.AddRef("refs",&it->fileRef);
	be_app->PostMessage(&updtDisplay);
#endif
	return it;
}
#endif

// patchlist is member,
// DestManager is memeber?
void InstallLooper::FindPatches( BDirectory *parDir,
									DestManager *dests,
									ulong inGrps,
									bool topDir)
{
	BDirectory	resultDir;
	BDirectory	*newDir = NULL;
	BDirectory	*destDir = NULL;
	
	status_t err = B_NO_ERROR;

	/*** get folder ***/

	ArchiveFolderItem	*curFolder;
	fCatalog->GetFolderData((ArchiveItem **)&curFolder);
	
	/**
		do action
	**/
#if DEBUG
	PRINT(("read folder %s\n",curFolder->name));
	if (topDir)
		PRINT(("THIS IS THE TOP LEVEL FOLDER\n"));
#endif
	if ((curFolder->groups & inGrps) &&
			(dests->InstallPlatform(curFolder->platform)) &&
			(!topDir || (inGrps & installFolderGroups))) {
		// do action if there are items in this folder
		PRINT(("CREATING FOLDER %s\n",curFolder->name));
		PRINT(("INGRPS is %.8x INSTALLFOLDERGROUPS %.8x\n",inGrps,installFolderGroups));

		destDir = dests->DirectoryFor(curFolder->destination,parDir,
							curFolder->customDest,FALSE);
	}
#if (DEBUG)
	else {
		PRINT(("folder not checked, nothing will reside in it\n"));
	}
#endif
	if (dests->canceled) {
		return;
	}
	
	if (destDir) {
		// we found the folder for this folder
		// if this is topDir and not creating the topDir
		// then we have the real folder
		if (topDir && dests->doInstallFolder) {
			// may be null
			newDir = destDir;
		}
		else {
			// otherwise check if destDir contains this folder
			// if it does, set newDir to point to it
			BEntry dirEnt;
			err = destDir->FindEntry(curFolder->name, &dirEnt);
			if (err == B_NO_ERROR) {
				err = resultDir.SetTo(&dirEnt);
				if (err == B_NO_ERROR)
					newDir = &resultDir;
			}
		}
	}
	
	/** end action **/
	
	fCatalog->StartFolderGet();
	int32 subitem;
	while (subitem = fCatalog->GetFolderItemType()) {
		switch(subitem) {
			case FOLDER_FLAG:
				FindPatches(newDir,dests,inGrps);
				//if (err < B_NO_ERROR)
				//	return;
				break;
			case PATCH_FLAG:
				PRINT(("FindPatches::PATCH_FLAG\n"));
				ArchivePatchItem	*curPatch;
				fCatalog->GetPatchData((ArchiveItem **)&curPatch);
				if ((curPatch->groups & inGrps) &&
					dests->InstallPlatform(curPatch->platform))
				{
					// do action
					// save current offset
					//entriesOffset = arcFile->pFile->Position();
					PRINT(("FindPatches::dests->DirectoryFor()\n"));
					
					curPatch->fileRef.device = -1;
					curPatch->fileRef.directory = -1;
					BDirectory *d = dests->DirectoryFor(curPatch->destination,
														newDir,
														curPatch->customDest,
														FALSE,
														curPatch);
					// patch may or may not have been found
					patchList->AddItem(new ArchivePatchItem(*curPatch));
					if (dests->canceled)
						return;
					//arcFile->pFile->Seek(entriesOffset,SEEK_SET);
				}
				break;
			default:
				ArchiveItem	*curItem;
				fCatalog->GetItemData(&curItem,subitem);
				break;
		}
	}
	fCatalog->EndFolderGet();

	return;
}
