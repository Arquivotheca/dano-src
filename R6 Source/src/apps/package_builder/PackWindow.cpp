#include <Be.h>
#include "PackApplication.h"
#include "PackWindow.h"
#include "PackArc.h"
#include "PackMessages.h"
#include "FArchiveItem.h"
#include "FileLooper.h"

#include "PackListView.h"
#include "SettingsView.h"
#include "InstSetWindow.h"
#include "PackageSetWind.h"
#include "Attributes.h"
#include "PatchWindow.h"
#include "StatusWindow.h"
#include "SplashWindow.h"
#include "MFilePanel.h"

#include "ErrHandler.h"

#include "RList.h"
#include "GlobalIcons.h"
#include "IconMenuItem.h"

#include "Util.h"
#include "CopyFile.h"
#include "ErrHandler.h"
#include "ExtractUtils.h"

#include "Replace.h"
#include "MyDebug.h"

#if DEBUG
	#if HEAP_WINDOW
		extern RWindow *gHeapWindow;
	#endif
#endif


#define MBAR_HEIGHT		24
#define WIND_HEIGHT		400
#define WIND_WIDTH		380


enum {
	M_DO_CLOSE =	'DClo'
};


PackWindow::PackWindow(char *name, PackArc *_arcFile, bool immedCompress, bool cdRom)
				: 	RWindow(BRect(0,0,WIND_WIDTH,WIND_HEIGHT), name, B_TITLED_WINDOW, B_NOT_ZOOMABLE),
					arcFile(_arcFile),
					tempFile(NULL),
					realFile(NULL),
					autoCompress(immedCompress),
					isCDInstall(cdRom),
					addMode(-1),
					seaFileName(NULL),
					instFileName(NULL)
{
	Lock();
	
	for (long i = 0; i < kMaxChildWind; i++) {
		childWindows[i];
	}
	//////////////////////////////////////////////////////////
	
	// messages queued to file looper
	msgCount = 0;
	
	// semaphores for folder calculation
	calcGroupSem = create_sem(1,"calc group");
	calcThreadCount = 0;
	
	//////////////////////////////////////////////////////////	
	
	// stagger windows 20 pixels
	// get the front window, sometimes this messes up
	// if you create windows too quickly
	PackWindow *frontWindow = (PackWindow *)(Front());
	long left, top;
	if (frontWindow != this && 
			#if HEAP_WINDOW
			frontWindow != gHeapWindow &&
			#endif
			frontWindow->Lock()) {
		left = frontWindow->Frame().left + 20;
		top = frontWindow->Frame().top + 20;
		frontWindow->Unlock();
	}
	else {
		left = 100;
		top = 30;
	}
	MoveTo(left,top);

	BRect b = Bounds();	
	SetSizeLimits(b.Width(),8192.0,240.0,8192.0);

	
	BRect frame = b;
	frame.bottom = frame.top + MBAR_HEIGHT;
	BMenuBar *mbar = new BMenuBar(frame,"menubar");
	// add first menu to get height
	mbar->AddItem(new BMenu("Package"));
	AddChild(mbar);
	
	long barHeight = mbar->Frame().Height() + 1;

	PRINT(("barHeight %d\n",barHeight));

	// for menu bar
	b.top += barHeight;
	PackView *packView = new PackView(b, "PackView");
	// add view to window
	AddChild(packView);
	listing = (PackList *)(FindView("listing"));
	
	
	// lastly add the menus, do this last so we can set the targets
	// to the appropriate views
	SetUpMenus();
			
	//////////////////////////////////////////////////////////////////
	/////////				 File Garbage 
	//////////////////////////////////////////////////////////////////
	long err;
	
	realUpdateNeeded = FALSE;
	// tracks initial addition to a modified file
	modified = FALSE;
	// later get this from the file!
	isUntitled = !arcFile;

	realFile = NULL;
	// no compression for cd-installs
	if (isCDInstall)
		autoCompress = FALSE;

	if (!arcFile) {
		// this is a new document
		// setup popup menus and such
		MakeDefaultAttributes();
		if (autoCompress) {
			PRINT(("auto compression on, creating new PackArc\n"));
			// this is a new document, set to auto compress
			// tempFile will be non-null
			// realFile will be null
			arcFile = new PackArc();
			PackApp *app = dynamic_cast<PackApp *>(be_app);
			if (app && app->CompatibleMode()) {
				arcFile->SetVersion(COMPAT_VERSION);
			} else {
				arcFile->SetVersion(CURRENT_VERSION);			
			}

			tempFile = new BEntry();
			err = MakeTempFile(tempFile,name);
			if (err < B_NO_ERROR) {
				doError("Auto compression has been disabled because a temporary file could not be created");
				delete tempFile;
				tempFile = NULL;
				autoCompress = FALSE;
			}
			else {
				// arcFile->SetTypeAndApp(FILE_SIGNATURE,APP_SIGNATURE);
				// new auto compress file
				
				entry_ref	ref;
				tempFile->GetRef(&ref);
				arcFile->SetArcRef(&ref);

				arcFile->updateMessenger = BMessenger(this);
				arcFile->SetupNew(fileFlags);
				arcFile->SetupLooper(this);
			}
		}
		else {
			// this is a new document, set to non-auto compress
			// tempFile and realFile are both null
			// all activities will be taking place in RAM
			
			// done in constructor
			// tempFile = realFile = NULL;
			arcFile = NULL;
		}
	}
	else {
		// arcFile is non null (it is a file on disk)
		realFile = new BEntry();
		realFile->SetTo(arcFile->ArcRef());
		
		fileFlags = arcFile->arcFlags;
		isCDInstall = fileFlags & CD_INSTALL;
		autoCompress = fileFlags & AUTO_COMPRESS;
		
		// setup activity looper
		arcFile->SetupLooper(this);
		// read attributes
		arcFile->NewReadAttributes(&attrib);
		// read catalog tree into RAM, attach to listview
		listing->ReadFromDisk(arcFile);
		
		arcFile->updateMessenger = BMessenger(this);
		if (autoCompress) {
			// we are opening an existing file
			// since autoCompress is active
			// arcFile will refer to the to tempFile
			// realFile will refer to the realFile
			
			tempFile = new BEntry();
			err = MakeTempFile(tempFile,name);
			
			// copy the realFile to the tempFike
			CopyFile(tempFile,realFile,arcFile->statusMessenger);
			
			// make sure the arcFile refers to the tempFile
			entry_ref	ref;
			tempFile->GetRef(&ref);
			arcFile->SetArcRef(&ref);
		}
	}
	((SettingsView *)(FindView("Settings")))->SetupAttributes();
	
	UpdateDocModeDisplay();
		
	attribDirty = FALSE;
	
	Unlock();
	
	// caller shows the window!
}

PackWindow::~PackWindow()
{
	delete_sem(calcGroupSem);
	delete realFile;
	delete tempFile;	
	free(instFileName);
	free(seaFileName);
//	printf("\n\n\nRemaining Memory of color %u\n",memoryColor);
//	mdump_color(memoryColor,mem_print_func);
}	

status_t PackWindow::MakeTempFile(BEntry *newEntry,const char *name)
{	
	try {
		BDirectory		tmpDir("/boot/var/tmp");
		BDirectory		ourTmpDir;
		char 			buf[B_FILE_NAME_LENGTH];
				
		sprintf(buf,"PackageBuilder%ld",Team());
		// create if non-existent

		ErrHandler	err;

		if (!tmpDir.Contains(buf)) {
			err = tmpDir.CreateDirectory(buf,&ourTmpDir);
		}
		BEntry		tmpEntry;
		err = tmpDir.FindEntry(buf,&tmpEntry);
		err = ourTmpDir.SetTo(&tmpEntry);

		BFile newFile;
		err = ourTmpDir.CreateFile(name,&newFile,false);
		err = ourTmpDir.FindEntry(name,newEntry);
	}
	catch (ErrHandler::OSErr e) {
		return e.err;
	}
	
	return B_NO_ERROR;
}

void PackWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_ADD_FILES: {
			PRINT(("got add files message\n"));
			AddItems(msg->what);
			break;
		}
		case M_ADD_FOLDERS: {
			AddItems(msg->what);
			break;
		}
		case M_ADD_SCRIPT: {
			// doError("This feature will be available in a future version.");
		
			AddItems(msg->what);
			break;
		}
		case M_SAVE: {
			PRINT(("time to save the file to disk\n"));
			if (isUntitled)
				PostMessage(M_SAVE_AS);
			else {
				long dirtyCount, byteCount;
				bool needDelete;
				// stricly not-necessary here...
				// why???
				CheckDirty(dirtyCount,byteCount,needDelete);
				DoSave(FALSE,dirtyCount,byteCount,needDelete);
			} 
			break;
		}
		case B_SAVE_REQUESTED: {
			entry_ref	r;
			msg->FindRef("directory",&r);
			//savePanel.SetDirectory(&r);
			
			DoSaveAs(msg);
			break;
		}
		case M_SAVE_AS: {		
			if (!TryActivate(savePanelMsngr)) {
				char panelTitle[B_FILE_NAME_LENGTH + 80];
				sprintf(panelTitle,"PackageBuilder : Save \"%s\" as...",Title());
				BFilePanel *panel = new MFilePanel(B_SAVE_PANEL,
													new BMessenger(this),
													NULL,
													false,
													false,
													B_SAVE_REQUESTED);
				panel->Window()->SetTitle(panelTitle);
				savePanelMsngr = BMessenger(panel->Window());				
				
				panel->Show();
				panel->SetSaveText(Title());
			}
			break;
		}
		/*
		case M_REVERT: {
			// blah
			// cheap solution is to close and reopen
			// better solution is to re-read old stuff from disk (ie just open)
			break;
		}
		*/
		case M_ITEMS_UPDATED: {
			/// assume a folder was updated
			long upIndex;
			ArchiveItem *child;
			msg->FindPointer("item",reinterpret_cast<void **>(&child));
			ArchiveFolderItem *par = child->GetParent();
			if (par == listing->CurrentFolder()) {
				upIndex = listing->IndexOf(child);
				if (upIndex >= 0) listing->InvalidateItem(upIndex);
			}
			else if (par) {
				child = par;
				par = child->GetParent();
				if (par && par == listing->CurrentFolder()) {
					upIndex = listing->IndexOf(child);
					if (upIndex >= 0) listing->InvalidateItem(upIndex);
				}
			}
			break;
		}
		case M_EXTRACT_ITEMS: {
			if (arcFile == NULL) {
				doError("Can't extract items because there is no disk file.");
				break;
			}
			if (!TryActivate(extractPanelMsngr)) {
				BFilePanel *panel = new MFilePanel(B_OPEN_PANEL,
													new BMessenger(FindView("listing")),
													NULL,
													true,
													false,
													M_EXTRACT_ITEMS,
													NULL,
													"Select");
				extractPanelMsngr = BMessenger(panel->Window());
				panel->Window()->Lock();
				BView *bg = panel->Window()->ChildAt(0);
				BRect r = bg->FindView("cancel button")->Frame();
				r.left = 12;
				r.right = r.left + 200;
				r.bottom = r.top + 20;
				BStringView *sv = new BStringView(r,B_EMPTY_STRING,"Choose the folder to extract into...",
										B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
				bg->AddChild(sv);
				panel->Window()->Unlock();
				panel->Show();
			}
			break;
		}
		case M_BUILD_PACKAGE:
		case M_BUILD_INSTALLER: {
			if (msg->HasRef("directory")) {

				const char *newname = msg->FindString("name");
				if (msg->what == M_BUILD_PACKAGE) {
					if (instFileName) free(instFileName);
					instFileName = strdup(newname);
				}
				else {
					if (seaFileName) free(seaFileName);
					seaFileName = strdup(newname);
				}
				atomic_add(&msgCount,1);
				Looper()->DetachCurrentMessage();
				arcFile->fileLooper->PostMessage(msg);
				return;
			}
			
			long dirtyCount, byteCount;
			bool needDelete;
			CheckDirty(dirtyCount,byteCount,needDelete);
			
			// would like to allow builds from tempFiles but no attributes are available ??
			if (dirtyCount > 0 || needDelete || attribDirty || isUntitled || realUpdateNeeded) {
				char text[256];
				char *amsg;
				if (dirtyCount > 1) {
					sprintf(text, "Package must be saved before building the installer\n\n%ld items remain to be compressed",dirtyCount);
					amsg = text;
				}
				else if (dirtyCount == 1) {
					sprintf(text, "Package must be saved before building the installer\n\n%ld item remains to be compressed",dirtyCount);
					amsg = text;
				}
				else
					amsg = "Package must be saved before building the installer.";
				BAlert *alert = new BAlert(B_EMPTY_STRING,amsg,"Cancel", "Save Now");
				alert->SetShortcut(0, B_ESCAPE);
				long result = alert->Go();
				
				switch (result) {
					case 0:
					
						return;
						break;
					case 1:
						if (isUntitled) {
							PostMessage(M_SAVE_AS);
							// count dirty will have to be redone	
							return;
						}
						DoSave(FALSE,dirtyCount,byteCount,needDelete,msg->what);
						
						return;
						break;
					default:
						return;
						break;
				}
			}
			
			buildPanelMsngr.SendMessage(B_QUIT_REQUESTED);
			
			BFilePanel *panel = new MFilePanel(B_SAVE_PANEL,
											new BMessenger(this),
											NULL,
											true,
											false,
											msg->what);
			buildPanelMsngr = BMessenger(panel->Window());
				
			if (msg->what == M_BUILD_INSTALLER) {
				if (!seaFileName) {
					seaFileName = (char *)malloc(B_FILE_NAME_LENGTH);
					sprintf(seaFileName,"%s.sea",Title());
					panel->Window()->SetTitle("Save Installer");
				}
				panel->SetSaveText(seaFileName);
			}
			else {
				if (!instFileName) {
					instFileName = (char *)malloc(B_FILE_NAME_LENGTH);
					sprintf(instFileName,"%s.pkg",Title());
					panel->Window()->SetTitle("Save Package");	
				}
				panel->SetSaveText(instFileName);
			}
			panel->Show();
			break;
		}
		case M_INST_SETTINGS: {
			// bring up the edit window
			if ( !childWindows[kSetWind].IsValid() ) {
				PRINT(("making installer settings window\n"));
				InstSetWindow *iWindow = new InstSetWindow("Installation Settings",this);
				childWindows[kSetWind] = BMessenger(iWindow);
			}
			else
				childWindows[kSetWind].SendMessage(M_DO_ACTIVATE);
			break;
		}
		case M_PACK_SETTINGS: {
			// bring up the package settings window
			if (!childWindows[kPackSetWind].IsValid()) {
				PackageSetWind *sWindow = new PackageSetWind("Package Settings",
															this,
															&attrib,
															&attribDirty);
				childWindows[kPackSetWind] = BMessenger(sWindow);
			}
			else
				childWindows[kPackSetWind].SendMessage(M_DO_ACTIVATE);
			break;
		}
		case M_SPLASH_SCREEN: {
			if (!childWindows[kSplashWind].IsValid()) {
				SplashWindow *sWindow = new SplashWindow("Splash Screen",
													&attrib.splashBitmap,
													this);
				childWindows[kSplashWind] = BMessenger(sWindow);
			}
			else
				childWindows[kSplashWind].SendMessage(M_DO_ACTIVATE);
			break;
		}
		case M_ADD_PATCH: {
			if (isCDInstall) {
				doError("Sorry, patches cannot be currently added to CD-ROM installs.\n\
This will be available in future versions");
				break;
			}
			if ( !childWindows[kPatchWind].IsValid() ) {
				PRINT(("making add patch window\n"));
				PatchWindow *pWindow = new PatchWindow("Add Patch",this);
				childWindows[kPatchWind] = BMessenger(pWindow);
			}
			else
				childWindows[kPatchWind].SendMessage(M_DO_ACTIVATE);
			break;
		}
		default:
			break;
	}
}

void PackWindow::DoSave(bool closing, long dirtyCount,
						long byteCount, bool needDelete, int32 buildInstaller)
{
	if (listing && arcFile) {
		uint32 incompatibleCount = CheckCompatibility(arcFile->Version());
		if (incompatibleCount) {
			BString errStr;
			errStr << "Error: this package contains " << incompatibleCount << " item"
				   << ((incompatibleCount > 1) ? "s" : "") << " with replacement mode settings that"
				   << " are not compatible with the requested file format version.  No package"
				   << " file will be generated.  Please set all replacement modes to "
				   << " compatible settings before saving this package.  Compatible settings "
				   << " are those replacement mode menu options that are not disabled.";  
			doError(errStr.String());
			return;
		}
	} else {
		doError("PackWindow: an internal variable was unexpectedly NULL.");	
		return;
	}
	
	if (arcFile == NULL)
		doError("Cannot save since there is no disk file!");
	
	// write out files, catalog, and attribData
	if (dirtyCount > 0 || needDelete) {
		PRINT(("dirty count was %d, sending save message\n",dirtyCount));
		BMessage svMsg(M_SAVE);
		svMsg.AddPointer("delete pool",listing->deletePool);
		svMsg.AddInt32("item count",dirtyCount);
		svMsg.AddInt32("byte count",byteCount);
		svMsg.AddPointer("archive item",listing->toplevel);
		svMsg.AddPointer("top folder",listing->toplevel);
		svMsg.AddPointer("tree lock",&(listing->treeLock));
		svMsg.AddBool("closing",closing);
		if (buildInstaller) svMsg.AddInt32("installer",buildInstaller);
		svMsg.AddPointer("attributes",&attrib);
		
		#if DEBUG
		if (!realFile)
			PRINT(("		REAL FILE IS NULL\n"));
		#endif
		
		svMsg.AddPointer("realfile",realFile);
		attribDirty = FALSE;
		atomic_add(&msgCount,1);
		arcFile->fileLooper->PostMessage(&svMsg);
	}
	// write of catalog and attribData
	else if (attribDirty || realUpdateNeeded) {
		// make sure folder calculation is cool
	
		attribDirty = FALSE;
		BMessage catMsg(M_WRITE_CATALOG);
		catMsg.AddPointer("top folder",listing->toplevel);
		catMsg.AddPointer("tree lock",&(listing->treeLock));
		catMsg.AddBool("closing",closing);
		if (buildInstaller) catMsg.AddInt32("installer",buildInstaller);
		catMsg.AddPointer("attributes",&attrib);
		
		catMsg.AddPointer("realfile",realFile);
		PRINT(("ATTRIB DIRTY sending catalog write message\n"));
		atomic_add(&msgCount,1);
		arcFile->fileLooper->PostMessage(&catMsg);
	}
	realUpdateNeeded = FALSE;
}

void PackWindow::DoSaveAs(BMessage *msg)
{
	if (listing && arcFile) {
		uint32 incompatibleCount = CheckCompatibility(arcFile->Version());
		if (incompatibleCount) {
			BString errStr;
			errStr << "Error: this package contains " << incompatibleCount << " item"
				   << ((incompatibleCount > 1) ? "s" : "") << " with replacement mode settings that"
				   << " are not compatible with the requested file format version.  No package"
				   << " file will be generated.  Please set all replacement modes to "
				   << " compatible settings before saving this package.  Compatible settings "
				   << " are those replacement mode menu options that are not disabled.";  
			doError(errStr.String());
			return;
		}
	} else {
		doError("PackWindow: an internal variable was unexpectedly NULL.");	
		return;
	}

	entry_ref dirRef;
	const char *name;
	
	msg->FindRef("directory",&dirRef);
	name = msg->FindString("name");

	status_t err;
	
	/// implements save as
	BDirectory	dir(&dirRef);
//	PackList *list = (PackList *)FindView("listing");

	// save as from an untitled file
	// we may have a temp file or not
	if (isUntitled) {
		PRINT(("is untitled is TRUE\n"));
		BFile	newFile;
		////////////////////
		// create new file
		err = dir.CreateFile(name,&newFile);
		if (err != B_NO_ERROR) {
			char buf[B_FILE_NAME_LENGTH+40];
			sprintf(buf,"There was an error creating the file \"%s\"\nTry saving later.",name);
			doError(buf);
			return;
		}
		// if autoCompress then we have a temp file
		if (autoCompress) {
			PRINT(("autocompress untitled is TRUE\n"));
			ASSERT(realFile == NULL);

			realFile = new BEntry();
			dir.FindEntry(name,realFile);
			PRINT(("			created realFile!!!\n"));
			
			// realFile now non-null
			// arcFile stills references tempFile
			SetEntryType(realFile,FILE_SIGNATURE);
		}
		else {
			PRINT(("autocompress is FALSE\n"));
			// we need 
			ASSERT(realFile == NULL);
			ASSERT(arcFile == NULL);

			realFile = new BEntry();
			dir.FindEntry(name,realFile);
			entry_ref	r;
			realFile->GetRef(&r);
			
			arcFile = new PackArc();
			// set version properly
			PackApp *app = dynamic_cast<PackApp *>(be_app);
			if (app && app->CompatibleMode()) {
				arcFile->SetVersion(COMPAT_VERSION);
			} else {
				arcFile->SetVersion(CURRENT_VERSION);			
			}
			arcFile->SetArcRef(&r);
			arcFile->SetupNew(fileFlags);
			arcFile->SetupLooper(this);
			
			SetEntryType(realFile,FILE_SIGNATURE);
		}
	}
	else {
		// doError("Not implemented yet, sorry.");
		ASSERT(realFile);
		
		BFile newFile;
		////////////////////
		// create new file
		err = dir.CreateFile(name,&newFile);
		if (err != B_NO_ERROR) {
			char buf[B_FILE_NAME_LENGTH+40];
			sprintf(buf,"There was an error creating the file \"%s\"\nTry saving later.",name);
			doError(buf);
			return;
		}
		BEntry	newEntry;
		dir.FindEntry(name,&newEntry);
		
		err = CopyFile(&newEntry,realFile,arcFile->statusMessenger);
		if (err != B_NO_ERROR)
			doError("Error copying file.");
			
		SetEntryType(&newEntry,"application/x-scode-DPkg");
		
		*realFile = newEntry;
		if (!autoCompress) {
			entry_ref	ref;
			realFile->GetRef(&ref);
			arcFile->SetArcRef(&ref);
		}
	}
	
	char windowTitle[B_FILE_NAME_LENGTH+10];
	sprintf(windowTitle, "Status \"%s\"", name);
	arcFile->fileLooper->statWindow->SetTitle(windowTitle);
	// update the window title and all that stuff
	SetTitle(name);
	
	long dirtyCount = 0;
	long byteCount = 0;
	bool needDelete = FALSE;

	// if (!isCDInstall) {
	CheckDirty(dirtyCount,byteCount,needDelete);
	// }
	attribDirty = TRUE;
	isUntitled = FALSE;
	DoSave(FALSE,dirtyCount,byteCount,needDelete);
	// update the status window name
}

void PackWindow::SetTitle(const char *name)
{
	PRINT(("setting window title\n"));
	PackList *pv = (PackList *)FindView("listing");
		
	
	// update the popup menu title
	BMenuBar *popBar = (BMenuBar *)FindView("foldPopup");
	IconMenuItem *superItem = (IconMenuItem *)(popBar->ItemAt(0));		
	superItem->Submenu()->ItemAt(0)->SetLabel(name);
	if (pv->TopLevel() == pv->CurrentFolder()) {
		// if the top folder is in view
		superItem->SetLabel(name);
	}
	
	// update top folder title
	PRINT(("The window %s untitled\n",isUntitled ? "IS" : "IS NOT"));
	PRINT(("The folder toplevel name is \"%s\" and the wind title is \"%s\"\n",
				pv->toplevel->name,Title()));
	
	if (isUntitled && (0 == strcmp(pv->toplevel->name,Title()))) {
		free(pv->toplevel->name);
		pv->toplevel->name = strdup(name);
	}
	
	RWindow::SetTitle(name);	
		
	// update the child window names
	
	BMessage nt(M_NEW_TITLE);
	nt.AddString("title",name);
	PRINT(("sending title message %s\n",name));
	
	for (long i = 0; i < kMaxChildWind; i++) {
		childWindows[i].SendMessage(&nt);
	}
		
	// update the prefs window popups
	BMessage nw(M_NAME_WINDOW);
	nw.AddPointer("window",this);
	nw.AddString("title",name);
	be_app->PostMessage(&nw);
}

void PackWindow::AddItems(int32 mode)
{
	if (TryActivate(addPanelMsngr)) {
		if (mode != addMode) {
			addPanelMsngr.SendMessage(B_QUIT_REQUESTED);
		}
		else
			return;	// panel is ok
	}
	addMode = mode;

	// need to make a new panel
	char buf[B_FILE_NAME_LENGTH + 60];
	char *kind;
	if (mode == M_ADD_SCRIPT)
		kind = "script";
	else if (mode == M_ADD_FOLDERS)
		kind = "folders";
	else
		kind = "files";
	sprintf(buf,"PackageBuilder : Add %s to \"%s\"", kind ,Title());
		
	
	BFilePanel *panel;
	if (mode == M_ADD_SCRIPT) {
		panel = new MFilePanel(B_OPEN_PANEL,
								new BMessenger(FindView("listing")),
								NULL, false, false, M_ADD_SCRIPT,
								NULL,"Add");
	}
	else {
		panel = new MFilePanel(B_OPEN_PANEL,
								new BMessenger(FindView("listing")),
								NULL,
								(mode == M_ADD_FOLDERS),
								true, B_REFS_RECEIVED,
								NULL, "Add");
	}
	addPanelMsngr = BMessenger(panel->Window());
										
	panel->Window()->SetTitle(buf);
	panel->Show();
}

void PackWindow::MakeDefaultAttributes()
{	
	attrib.SetupDefaults();
	
	fileFlags = 0;
	if (autoCompress)
		fileFlags |= AUTO_COMPRESS;
	if (isCDInstall)
		fileFlags |= CD_INSTALL;
}

// private...
void PackWindow::SetUpMenus()
{
	BMenuBar *mbar = (BMenuBar *)FindView("menubar");
	//////////////////// Package Menu ///////////////////////////////
	BMenuItem *item;
	BMenu *menu = mbar->SubmenuAt(0);

	// item 0
	item = new BMenuItem("New Package",
				new BMessage(M_DO_NEW),'N',B_COMMAND_KEY);
	item->SetTarget(be_app);
	menu->AddItem(item);
	
	// item 1
	item = new BMenuItem("Open...",
				new BMessage(M_DO_OPEN),'O',B_COMMAND_KEY);
	item->SetTarget(be_app);
	menu->AddItem(item);

	// item 2
	menu->AddSeparatorItem();
		
	// item 3
	item = new BMenuItem("Edit Groups...",new BMessage(M_EDIT_GROUPS));
	item->SetTarget(FindView("Settings"));
	menu->AddItem(item);
	
	// item 4
	item = new BMenuItem("Edit Destinations...",new BMessage(M_EDIT_DEST));
	item->SetTarget(FindView("Settings"));
	menu->AddItem(item);
	
	//
#if CONDITIONAL_INSTALL
	item = new BMenuItem("Edit Conditions...",NULL);
	item->SetTarget(FindView("Settings"));
	menu->AddItem(item);
#endif	
	// item 5
	menu->AddSeparatorItem();
	
	// item 6
	item = new BMenuItem("Save Package",new BMessage(M_SAVE),'S');
	menu->AddItem(item);
	
	// item 7
	item = new BMenuItem("Save Package As...",new BMessage(M_SAVE_AS));
	menu->AddItem(item);
	//item = new BMenuItem("Revert",new BMessage(M_REVERT));
	//menu->AddItem(item);
	
	// item 8
	item = new BMenuItem("Close",new BMessage(B_QUIT_REQUESTED),'W');
	//item->SetTarget(this);
	menu->AddItem(item);
	
	// item 9
	menu->AddSeparatorItem();
	
	// item 10
	item = new BMenuItem("Preferences...",
				new BMessage(M_DO_PREFS));
	item->SetTarget(be_app);
	menu->AddItem(item);

	// item 11
	item = new BMenuItem("About Package Builder...",
		new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);
	
	// item 12
	menu->AddSeparatorItem();

	// item 13
	item = new BMenuItem("Quit",
				new BMessage(B_QUIT_REQUESTED),'Q',B_COMMAND_KEY);
	item->SetTarget(be_app);
	menu->AddItem(item);
	
	//////////////////// Items Menu ///////////////////////////////
	
	menu = new BMenu("Items");
	
	// item 0		
	item = new BMenuItem("Add Files...",new BMessage(M_ADD_FILES),'F');
	menu->AddItem(item);
	
	// item 1
	item = new BMenuItem("Add Folders...",new BMessage(M_ADD_FOLDERS));
	menu->AddItem(item);
	
	// item 2
	menu->AddSeparatorItem();
	
	// item 3
	item = new BMenuItem("Add Patch...",new BMessage(M_ADD_PATCH),'P');
	menu->AddItem(item);

	// item 4
	item = new BMenuItem("Add Shell Script...",new BMessage(M_ADD_SCRIPT));
	menu->AddItem(item);
		
#if 0
	item = new BMenuItem("Test Patch...",new BMessage(M_TEST_PATCH));
	item->SetTarget(listing);
	menu->AddItem(item);
#endif
	// item 5
	menu->AddSeparatorItem();
	
	// item 6
	item = new BMenuItem("Extract Items...",new BMessage(M_EXTRACT_ITEMS),'E');
	menu->AddItem(item);

	// item 7
	item = new BMenuItem("Delete",new BMessage(M_REMOVE_ITEMS),'D');
	item->SetTarget(listing);
	menu->AddItem(item);

	// item 8
	item = new BMenuItem("New Folder",new BMessage(M_NEW_FOLDER));
	item->SetTarget(listing);
	menu->AddItem(item);
	
	// item 9
	item = new BMenuItem("Rename Item",new BMessage(M_CHANGE_NAME),'R');
	item->SetTarget(listing);
	menu->AddItem(item);
	
	// item 10
	menu->AddSeparatorItem();
	
	// item 11
	item = new BMenuItem("Select All",new BMessage(M_SELECT_ALL),'A');
	item->SetTarget(listing);
	menu->AddItem(item);
	
	// item 12
	//item = new BMenuItem("Update All",new BMessage(M_UPDATE_ITEMS),'U');
	//item->SetTarget(listing);
	//menu->AddItem(item);
	
	
	mbar->AddItem(menu);
	
	
	//////////////////// Installer Menu ///////////////////////////////
	
	menu = new BMenu("Installation");

	item = new BMenuItem("Generate R4 Compatible Packages", new BMessage(M_TOGGLE_R4_COMPATIBLE));
	item->SetTarget(be_app);
	menu->AddItem(item);

	//item = new BMenuItem("Edit Readme...",new BMessage(M_EDIT_README));
	//menu->AddItem(item);
	item = new BMenuItem("Splash Screen...",new BMessage(M_SPLASH_SCREEN));
	menu->AddItem(item);
	item = new BMenuItem("Installation Settings...",new BMessage(M_INST_SETTINGS));
	menu->AddItem(item);
	item = new BMenuItem("Package Settings...",new BMessage(M_PACK_SETTINGS));
	menu->AddItem(item);

	menu->AddSeparatorItem();
	
	BMenu *submenu = new BMenu("Build");
	item = new BMenuItem("SoftwareValet Package...",
		new BMessage(M_BUILD_PACKAGE),'B',B_COMMAND_KEY);
	submenu->AddItem(item);
	item = new BMenuItem("Self-Extracting Installer...",
		new BMessage(M_BUILD_INSTALLER),'B',B_COMMAND_KEY | B_OPTION_KEY);
	submenu->AddItem(item);

	item = new BMenuItem(submenu);
	menu->AddItem(item);

	mbar->AddItem(menu);
}

#include "PackMenuItems.h"

void PackWindow::MenusBeginning()
{
	RWindow::MenusBeginning();
	
	PackList *plv = listing;
	bool enable;

	BMenuBar *bar = (BMenuBar *)FindView("menubar");
	BMenu *package = (BMenu *)(bar->SubmenuAt(0));	
	ASSERT(package);

	BMenu *items = (BMenu *)(bar->SubmenuAt(1));
	ASSERT(items);
	
	bool patchSel = 1 && plv->SelectedPatch();
	// enable/disable extract & delete
	// we also disable extract
	// when a single item is selected and it is a patch
	enable = !(plv->lowSelection == -1);
	items->ItemAt(MENU_EXTRACT)->SetEnabled(enable && !patchSel);
	items->ItemAt(MENU_DELETE)->SetEnabled(enable);
	enable = (plv->lowSelection != -1 && plv->lowSelection == plv->highSelection);
	items->ItemAt(MENU_RENAME)->SetEnabled(enable);

	// enable disable test patch
#if 0
	items->ItemAt(MENU_TEST_PATCH)->SetEnabled(patchSel);
#endif
	// enable/disable save,save as,close
	// build package
	BMenu *inst = (BMenu *)(bar->SubmenuAt(2));
		
	enable = (msgCount == 0);
	package->ItemAt(MENU_SAVE_AS)->SetEnabled(enable);
	package->ItemAt(MENU_CLOSE)->SetEnabled(enable);
	package->ItemAt(MENU_QUIT)->SetEnabled(enable);
	inst->ItemAt(MENU_BUILD_INSTALLER)->SetEnabled(enable);
	// items->ItemAt(MENU_UPDATE)->SetEnabled(enable);
		
	enable = (msgCount == 0 && ( realUpdateNeeded || attribDirty));
	package->ItemAt(MENU_SAVE)->SetEnabled(enable);
}

void PackWindow::CheckDirty(long &dirtyCount, long &byteCount, bool &needDelete)
{
	PackList *list = (PackList *)FindView("listing");
	dirtyCount = 0;
	byteCount = 0;
	needDelete = FALSE;

	/*
	long thrCount;
	get_sem_count(calcGroupSem,&thrCount);
	if (thrCount < 1)
		doError("Waiting for folder calculation to complete...");
	*/
	// synchronize -- FIGURE OUT A BETTER WAY!!!
	acquire_sem(calcGroupSem);
	release_sem(calcGroupSem);
	
	if (!isCDInstall) {
		list->treeLock.Lock();
		dirtyCount = list->toplevel->CountDirty(byteCount);
		list->treeLock.Unlock();
	}

	list->deletePoolLock.Lock();
	if (list->deletePool)
		needDelete = TRUE;
	else
		needDelete = FALSE;
	
	list->deletePoolLock.Unlock();
	// file could get added or removed after here!
}

bool PackWindow::QuitRequested()
{
	PRINT(("PACK WINDOW QUIT REQUESTED CALLED\n"));
	// don't close it until all the compression is done!
	if (msgCount > 0) {
		char mbuf[B_FILE_NAME_LENGTH+40];
		sprintf(mbuf,"Sorry, cannot close \"%s\" until all file operations are complete",
				Title());
		doError(mbuf);
		return FALSE;
	}	
	
	// first check if the window is dirty (up to date)
	long dirtyCount, byteCount;
	bool needDelete;
	CheckDirty(dirtyCount,byteCount,needDelete);
	if (dirtyCount > 0 || attribDirty || needDelete || realUpdateNeeded || (isUntitled && modified)) {
		char text[256];
		if (dirtyCount > 1)
			sprintf(text, "Save changes to \"%s\"?\n\n%ld items remain to be compressed", Title(),dirtyCount);
		else if (dirtyCount == 1)
			sprintf(text, "Save changes to \"%s\"?\n\n%ld item remains to be compressed", Title(),dirtyCount);
		else
			sprintf(text,"Save changes to \"%s\"?",Title());
		BAlert *alert = new BAlert(B_EMPTY_STRING,text, "Don't Save", "Cancel", 
			"Save", B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetShortcut(0, 'd');
		alert->SetShortcut(1, B_ESCAPE);
		long result = alert->Go();
		switch (result) {
			case 0: /* Don't Save */
				// post catalog message with quit requested set to true??
				// falls through
				break;
			case 1: /* Cancel */
				return FALSE;
			case 2: /* Save */
				if (isUntitled) {
					PostMessage(M_SAVE_AS);
					// count dirty will have to be redone	
				}
				else {
					DoSave(TRUE,dirtyCount,byteCount,needDelete);
				}
				return FALSE;
		}
	}	

/**
	if (be_app->IsFilePanelRunning() &&
		  (gPanelInfo.OpenOwner() == this)) {
			// if the file panel was running for this window
			// then close it so that messages don't get sent
			// to this window (which will no longer exist!)
			be_app->CloseFilePanel();
			gPanelInfo.SetOpenOwner(NULL);
	}
**/
	// hope this is ok!!!
//	if (autoCompress && arcFile != NULL) {
	if (tempFile) {
		PRINT(("      %s is removing temp file\n",Title()));
	
		status_t err = tempFile->Remove();
		if (err != B_NO_ERROR) {
			doError("There was an error removing the temporary file");
		}
	}
	else {
		PRINT(("      %s is NOT removing temp file!!!!!!\n",Title()));
	}
	// do cleanup here rather than in destructor
	// this must happen before posting quit requested to the app
	if ( arcFile != NULL)
		delete arcFile;
	
	
	PRINT(("gOT HERE!!!!\n"));
	
	// using messengers since theu assure serialized safe access to loopers
	// wait for replies to make sure the windows have shut down
	// since they may continue to set data 
	for (long i = 0; i < kMaxChildWind; i++) {
		CloseChild(childWindows[i]);
	}
	
	// hide in case there are long folder computation threads
	// we are waiting for	
	// Hide();
	// let threads shutdown
	PRINT(("waiting for folder calculation threads...... "));
	long thrCount;
	get_sem_count(calcGroupSem,&thrCount);
	if (thrCount < 1)
		doError("Waiting for folder calculation to complete...");
	acquire_sem(calcGroupSem);
	PRINT(("got em!!\n"));


	if (((PackApp *)be_app)->prefsWindowMessenger.IsValid()) {
		BMessage *closeWindow = new BMessage(M_DO_CLOSE);
		closeWindow->AddPointer("window",this);
		((PackApp *)be_app)->prefsWindowMessenger.SendMessage(closeWindow);
	}

	long winCount = CountWindows();
	PRINT(("----------> Window count is %d\n",winCount));
	if (winCount <= 1) {
		BMessage *quitMsg = new BMessage(B_QUIT_REQUESTED);
		quitMsg->AddBool("final",TRUE);
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	
	return TRUE;
}


void PackWindow::CloseChild(BMessenger &mess)
{
	BMessage *reply = new BMessage();

	if (mess.IsValid()) {
		PRINT(("asking child window to quit\n"));
		mess.SendMessage(B_QUIT_REQUESTED,reply);
		if (reply) {
			delete reply;
		}
		else
			doError("Error closing child window");
	}
}

void PackWindow::UpdateDocModeDisplay()
{
	BStringView *sv = (BStringView *)FindView("docmode");
	
	if (isCDInstall)
		sv->SetText("Mode: CD");
	else if (autoCompress)
		sv->SetText("Mode: Auto");
	else
		sv->SetText("Mode: Std");
}

void PackWindow::SetCompatibleMode(bool compatible)
{
	uint32 incompatibleCount = 0;
	if (Lock()) {
		// mark or unmark the toggle item in the window menu 
		BMenuBar *mBar = dynamic_cast<BMenuBar *>(FindView("menubar"));
		if (mBar) {
			BMenuItem *mItem = mBar->FindItem(M_TOGGLE_R4_COMPATIBLE);
			if (mItem) {
				mItem->SetMarked(compatible);
			}
		}
		// enable or disable appropriate items in the replacement menu
		SettingsView *sView = dynamic_cast<SettingsView *>(FindView("Settings"));
		if (sView) {
			sView->EnableDisableReplacementOptions(
				(compatible) ? COMPAT_VERSION : CURRENT_VERSION);
		}
		// set this PackArc to the appropriate version
		if (arcFile) {
			arcFile->SetVersion((compatible) ? COMPAT_VERSION : CURRENT_VERSION);
			// check all archive items for compatibility
			if (listing) {
				incompatibleCount = CheckCompatibility(arcFile->Version());
			}
		}
		Unlock();
	}
	if (incompatibleCount) {
		BString errStr;
		errStr << "Warning! This package contains " << incompatibleCount << " item"
			   << ((incompatibleCount > 1) ? "s" : "") << " with replacement mode settings that"
			   << " are not compatible with the requested file format version.  The saved package"
			   << " file will not install correctly.  Please set all replacement modes to "
			   << " compatible settings and regenerate the package file.  Compatible settings "
			   << " are those menu options that are not disabled.";  
		doError(errStr.String());
	}
}

// returns the number of incompatible items
uint32 PackWindow::CheckCompatibility(uint16 version)
{
	uint32 incompatibleCount = 0;
	// check each archive item for incompatible replacement settings
	if (listing) {
		int32 i = 0;
		ArchiveItem *item;
		while ((item = (ArchiveItem*)listing->ItemAt(i++)) != NULL) {
			// check for compatibility of replacement options
			if (!IsCompatibleReplacementOption(version, item->replacement)) {
				incompatibleCount++;
			}
		}
	}
	return incompatibleCount;
}


/************************************************************/

PackView::PackView(BRect rect, char *name)
	   	   : BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
}

void PackView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	BRect current = Bounds();
	
	long mbarMargin = 10;
	
	current.InsetBy(mbarMargin,mbarMargin);
	
	// create rect for popup menu
	BRect popupRect = current;
	popupRect.right = popupRect.left + 50;
	popupRect.bottom = popupRect.top + 22;
	
	BMenuBar *popBar = new BMenuBar(popupRect,"foldPopup",
		B_FOLLOW_LEFT | B_FOLLOW_TOP,B_ITEMS_IN_COLUMN,TRUE);
	
	//////////////////////////////////////////////////////////////////
	// adding folders menu to list view object
	// BMenuBar *popBar = (BMenuBar *)FindView("foldPopup");
	BPopUpMenu *popMenu = new BPopUpMenu("Folders");
		
	IconMenuItem *superItem = new IconMenuItem(gTopLevelIcon,popMenu);
	popBar->AddItem(superItem);
	
	BMessage *folderChangeMsg = new BMessage(M_CHANGE_FOLDER_LEVEL);
	IconMenuItem *menuitem = new IconMenuItem(Window()->Title(),
			gTopLevelIcon, folderChangeMsg);
	
	popMenu->AddItem(menuitem);	
	menuitem->SetMarked(TRUE);
	AddChild(popBar);
	
	/////////////////////////////////////////////////////////////////
	
	popupRect.right = current.right;
	popupRect.left = popupRect.right - 80;
	
	BStringView *sv = new BStringView(popupRect,"docmode",B_EMPTY_STRING,
						B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	AddChild(sv);
	sv->SetAlignment(B_ALIGN_RIGHT);
	
	/////////////////////////////////////////////////////////////////
	
	long mbarHeight = popBar->Frame().Height() + mbarMargin/2.0;
	
	// offset for popupmenu & column titles
	current.top += 30 + mbarHeight;
	// offset for scrollbar
	current.right -= B_V_SCROLL_BAR_WIDTH;
	// leave space for the settings view
	
#if CONDITIONAL_INSTALL
	current.bottom -= 140;
#else
	current.bottom -= 120;
#endif
	
	// main list view
	PackList *list = new PackList(current, "listing",B_FOLLOW_ALL);
	
	// show the associated column view
	list->SetShowColumns(TRUE);
		
	// setup the main scroll view
	scroller = new BScrollView("scroller",list,
			B_FOLLOW_ALL,0,FALSE,TRUE);
	
	AddChild(scroller);
	
	// menus that target the list view
	menuitem->SetTarget(list);
	folderChangeMsg->AddPointer("folder",list->toplevel);	
	list->foldersMenu = popMenu;
	list->superItem = superItem;
	
	
	// reoffset from scorllbar
	current.right += B_V_SCROLL_BAR_WIDTH + 1;
	// space ten pixels border for settings view
	current.top = current.bottom + 10;
	current.bottom = Bounds().bottom - 10;
	
	SettingsView *box = new SettingsView(current,"Settings",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	box->SetLabel("Item Settings");
	
	AddChild(box);
	
	//font_info finf;
	//GetFontInfo(&finf);
	
	/***
	PRINT(("FONT INFO\n"));
	PRINT(("Name: %s\n",finf.name));
	PRINT(("Size: %f\n",finf.size));
	PRINT(("Ascent: %f\n",finf.ascent));
	PRINT(("Descent: %f\n",finf.descent));
	PRINT(("Leading: %f\n",finf.leading));
	***/
}
