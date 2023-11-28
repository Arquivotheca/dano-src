// InstallWindow.cpp
#include <string.h>
#include <Button.h>
#include "InstallWindow.h"
#include "InstallView.h"
#include "InstallMessages.h"
#include "InstallDefs.h"
#include "IArchiveItem.h"
#include "LicenseWindow.h"
#include "DestManager.h"
#include "InstallApp.h"
#include "InstallPack.h"
#include "ArcCatalog.h"
#include "InstallLooper.h"
#include "PackAttrib.h"
#include "SplashWindow.h"
#include "SettingsManager.h"

#include "Util.h"

#include "MyDebug.h"
#include "InstallerType.h"

extern SettingsManager *gSettings;

RList<InstallWindow *>	InstallWindow::windowList;

BLocker					InstallWindow::windowListLock;
bool					InstallWindow::fCloseApp;

void WaitForWindow(BWindow *w);

InstallWindow::InstallWindow(const char *name,InstallPack *arc,bool _lowMem)
	:	BWindow(BRect(0,0,480,310),name,
			B_TITLED_WINDOW,B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	 	dest(NULL),
	  	archiveFile(arc),
	  	entryList(NULL),
	  	entryCount(0),
	  	topfolder(NULL),
	  	installLooper(NULL),
	  	attr(NULL),
	  	selectedGroups(0),
	  	selectedPlatforms(0),
	  	volumeID(-1),
	  	custom(false),
	  	dirName(NULL),
		installing(false),
	  	lowMemMode(_lowMem),
	  	fCatalog(NULL)
{
	PRINT(("InstallWindow::InstallWindow\n"));
	
	LockWindowList();
	windowList.AddItem(this);
	UnlockWindowList();

	Lock();
	
	status_t err;
	///////////// get platform information /////////////////////
	
#if __INTEL__
	// intel platform
	selectedPlatforms = (1 << 1);
#else
	// powerpc platform
	selectedPlatforms = (1 << 0);
#endif
	
	///////////// start processing group & destination information ////////////////////	
	
	attr = new PackAttrib();
	err = archiveFile->NewReadAttributes(attr);
	if (err < B_NO_ERROR) {
		doError("Fatal error reading general package attributes.");
		Abort();
		return;
	}
	
	// show splash window
	BWindow *splashWind = NULL;	
#if (!SEA_INSTALLER)
	if (attr->splashBitmap) {
		splashWind = new SplashWindow(attr->splashBitmap);
	}
#endif

	// fixup attr->name
	if (!attr->name)
		attr->name = strdup(B_EMPTY_STRING);
	// set the window title
	if (attr->name[0])
		SetTitle(attr->name);
	dest = new DestManager(attr);

	///////////////////////////////////////////////
	/////////// Read Catalog //////////////////////
	long offset = 0;
	PRINT(("READING catalog\n"));
	archiveFile->SetFile();

	fCatalog = new ArcCatalog();
	fCatalog->Set(archiveFile);
	err = fCatalog->Rewind();
	if (err >= B_NO_ERROR) {
		// read in the listing of all the files
		// cache in memory to display group and size information
		// for the selected groups
		attr->fileCount = 8192;

		// it is an array of 8192 file entries (maximum)
		// should make dynamic
		entryList = new FileEntry[attr->fileCount];
		// clear the array
		memset(entryList,0,attr->fileCount*sizeof(FileEntry));
		FileEntry *entryP = entryList;		
		
		PRINT(("file count is %d\n",attr->fileCount));
		// build the list
		BuildGroupList(&entryP,
						entryList+attr->fileCount,
						TRUE,
						attr->instFolderGroups);
		
		// determine the number of files actually read
		entryCount = entryP - entryList;		
		PRINT(("entry count is %d\n",entryCount));
		topfolder = NULL;
	}
	///////////////////////////////////////////////
	// clean up after reading catalog
	delete fCatalog;
	fCatalog = 0;
	archiveFile->ClearFile();
	
	// check for bad catalog
	if (err < B_NO_ERROR)
	{
		doError(errBADCATALOG);
		Abort();
		return;
	}

	// move the window to the top left 3rd of the screen
	PositionWindow(this,0.4,0.4);	
	///////////////////////////////////////////////
	PRINT(("creating install view...\n"));
	InstallView *windView = new InstallView(Bounds(),"installview",
		B_FOLLOW_ALL,B_WILL_DRAW,attr->groupList,attr->descriptionText);
	windView->SetViewColor(light_gray_background);
	AddChild(windView);
	PRINT(("done\n"));
	///////////////////////////////////////////////

	installLooper = NULL;	
	
	// throw up the splash screen
#if (!SEA_INSTALLER)	
	if (splashWind->Lock()) {
		WaitForWindow(splashWind);
	}
#endif
	// throw up the license window
	LicenseWindow *lw = NULL;
	if (attr->licenseText != NULL) {
		bool licenseCancel;
		PRINT(("CREATING LICENSE WINDOW\n"));
		lw = new LicenseWindow(attr->licenseText, &licenseCancel, attr->licenseSize, attr->licenseStyle);
		if (lw->Lock())
			WaitForWindow(lw);
		if (licenseCancel) {
			Abort();
			return;	
		}
	}
	Show();
	Unlock();	
}

InstallWindow::~InstallWindow()
{
	delete archiveFile;	// file
	delete attr;		// package attributes

	delete dest;		// DestManager (may not have there in constructor)
	delete topfolder;
	delete[] entryList;
	delete fCatalog;
	if (installLooper) {
		installLooper->PostMessage(B_QUIT_REQUESTED);
	}
	LockWindowList();
	windowList.RemoveItem(this);
	UnlockWindowList();
	
	free(dirName);
}

bool InstallWindow::QuitRequested()
{
	if (!installing) {
		LockWindowList();
		PRINT(("checking window list\n"));
		if (windowList.CountItems() <= 1 && fCloseApp) {
			PRINT(("posting quit req to app\n"));
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
		UnlockWindowList();
		return TRUE;
	}
	return FALSE;
}

void	InstallWindow::Abort()
{
		Quit();
#if (SEA_INSTALLER)
		be_app->PostMessage(B_QUIT_REQUESTED);
#endif
}

// check if volume can be installed onto
bool InstallWindow::CheckPossibleInstall(BVolume &installVol)
{
	char vname[B_FILE_NAME_LENGTH];
	installVol.GetName(vname);
	// read-only
	if (installVol.IsReadOnly()) {
		doError(errREADONLY,vname);
		return false;
	}
	// not enough space
	if (groupBytes > installVol.FreeBytes()) {
		doError(errNOSPACE,vname);
		return false;
	}
	// doesn't do mime, attributes or queries
	if ( !(installVol.KnowsMime() &&
			installVol.KnowsAttr() &&
			installVol.KnowsQuery() )) {
		doError(errNONBVOL,vname);
		return false;
	}
	return true;
}

// begin installation
void InstallWindow::DoInstall(BMessage *msg)
{
	msg;
	
	BVolume installVol(volumeID);

	// check if volume can be installed onto
	if (!CheckPossibleInstall(installVol)) {
		PostMessage(M_DONE);
		return;
	}
	// setup looper to perform installation	
	if (installLooper == NULL) {
		installLooper = new InstallLooper(archiveFile);
		installLooper->updateMessenger = BMessenger(this);
		
		// creates a status window
		installLooper->Run();
	}
				
	bool ok;
	BDirectory	rootDir;
	ok = dest->SetupInstall(&instDirRef,
							custom ? NULL : dirName,
							volumeID,
							topfolder,
							lowMemMode);
	
	if (!ok) {
		doError(errCANTINST);
		PostMessage(M_DONE);
		return;
	}
	// setup logging
#if (SEA_INSTALLER)
	dest->logging = true;
#else
	dest->logging = gSettings->data.FindBool("install/log");
	const char *lp = gSettings->data.FindString("install/logpath");
	if (lp && dest->logging) {
		dest->logDir.SetTo(lp);
	}
#endif

	// send message to looper to begin installing	
	BMessage exMsg(M_EXTRACT_ITEMS);
	
	exMsg.AddPointer("archive item",topfolder);
	exMsg.AddInt32("groups",selectedGroups);
	exMsg.AddInt32("bytes",groupBytes);
	exMsg.AddInt32("item count",itemCount);
	exMsg.AddPointer("destinations",dest);
	installLooper->PostMessage(&exMsg);
}

void InstallWindow::MessageReceived(BMessage *msg)
{
	BButton *btn;
	switch(msg->what) {
		case M_DO_INSTALL:
		{
			// if we're installing don't install again
			if (installing)
				break;
			// gray out the install button
			btn = (BButton *)FindView("installbutton");
			btn->SetEnabled(FALSE);
			

			installing = TRUE;

			DoInstall(msg);
			break;
		}
		case M_DONE:
		{
			PRINT(("main window recieved the done message\n"));
			installing = FALSE;

			btn = (BButton *)FindView("installbutton");
			btn->SetEnabled(TRUE);
			PRINT(("done responding to the done message\n"));
			
			if (msg->FindBool("close")) {
				PostMessage(B_QUIT_REQUESTED);
			}
			//DEBUGGER("Main window finished responding to done\n");
			//snooze(1000*500);
			break;
		}
		default:
			break;
	}
}

bool InstallWindow::LockWindowList()
{
	return windowListLock.Lock();
}

void InstallWindow::UnlockWindowList()
{
	windowListLock.Unlock();
}

void InstallWindow::SetCloseApp(bool value)
{
	fCloseApp = value;
}

void WaitForWindow(BWindow *w)
{
	thread_id id = w->Thread();
	if (w->IsLocked()) w->Unlock();
	int32 ret;
	wait_for_thread(id,&ret);
}
