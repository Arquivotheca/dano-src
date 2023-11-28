
#include "ValetApp.h"
#include "MyDebug.h"
#include "SettingsManager.h"

// dangerous to include from installer directory!
#include "InstallApp.h"
#include <NodeInfo.h>

#include "LicenseWindow.h"

#include "DoIcons.h"
#include "GraphicButton.h"
#include "Util.h"
#include "Log.h"
#include "LogWindow.h"
#include "UpdateWindow.h"
#include "SetupWindow.h"
#include "MFilePanel.h"
#include "SplashWindow.h"
#include "DownloadWindow.h"
#include "ValetWindow.h"
#include "ValetVersion.h"

#include <Alert.h>
#include <AppFileInfo.h>
#include <ClassInfo.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <string.h>
#include <fs_info.h>

#include "PackageDB.h"
#include "RegisterWindow.h"

//#ifndef _dr9_
/********************  IMPORTS ***********************/

/** for InstallWindow::WindowList, new InstallWindow**/
#include "InstallWindow.h"
/** for new InstallPack **/
#include "InstallPack.h"
//#endif

/*****************************************************/
#include "RegisterWindow.h"
#include "malloc_internal.h"

// first launch
void	WaitForWindow(BWindow *w);
long	RecursiveDelete(BEntry *ent);

// memdebugging
void	install_my_hooks();

extern SettingsManager	*gSettings;
entry_ref	ValetApp::appRef;

const char *kValetSignature = "application/x-scode-SValet";
const char *kListenerSig = "application/x-scode-VList";
const char *kDPkgSig = "application/x-scode-DPkg";
const char *kUPkgSig = "application/x-scode-UPkg";
const char *kIPkgSig = "application/x-scode-IPkg";
const char *kPkgDlSig = "application/x-scode-VDwn";
const char *kPPkgSig = "application/x-scode-VPart";
const char *kPkgPrefix = "application/x-scode";
const int kListenerMajorVers = 4;
const int kListenerMiddleVers = 5;
const int kListenerMinorVers = 0;

PackageItem *ValetApp::sBeOSPackageItem = NULL;

#define FIRST_LAUNCH 1

void AddFileExtension(const char *type, const char *next, const char *desc);

void
do_checkregistered(void)
{
    PackageDB *pdb = new PackageDB();
    PackageItem *pitem = new PackageItem();
    dev_t boot_volid = dev_for_path("/boot");
    int32 registered = 0;

    while (pdb->GetNextPackage(pitem, PackageDB::ALL_DATA, boot_volid) == B_NO_ERROR) {
        const char *pkg_name;

        if (pitem->data.FindString("package", &pkg_name) != B_OK || pkg_name == NULL)
            continue;

        if (strcmp(pkg_name, "BeOS" B_UTF8_TRADEMARK) == 0) {

            if (pitem->data.FindInt32("registered", &registered) == B_OK) {
                PRINT(("Got the BeOS package (%s)!\n", pkg_name));
                PRINT(("registered: %d\n", registered));
                if (registered) {
                    ValetApp::sBeOSPackageItem = NULL;
                    break;
                } else {
                    ValetApp::sBeOSPackageItem = pitem;
                    pitem = new PackageItem();
                }
            }
        }
    }

    delete(pitem);
    delete(pdb);

if (ValetApp::sBeOSPackageItem == NULL)
    printf("beos is registerd\n");
else
    printf("beos is NOT registerd\n");

}

int main(int argc, char **argv)
{
#if MEMDEBUG
	install_my_hooks();
#endif
	if (argv[1] && strcmp(argv[1], "-checkregistered") == 0)
		do_checkregistered();

	BApplication *theApp = new ValetApp();
	theApp->Run();
	
	delete theApp;
	return 0;
}

///////////////////////////////////////////////////////////

ValetApp::ValetApp()
	: 	BApplication(kValetSignature),
		downloadManager(NULL), fSomeWindowShowing(false), fRefsUnprocessed(false), valetWindow(NULL)
{
	app_info ai;
	be_app->GetAppInfo(&ai);
	appRef = ai.ref;
	
	SetValetIcons(appRef);

	// set icon and globals for the installer
	InstallApp::SetGlobals(appRef);
	// install windows closing will not trigger app quit
	InstallWindow::SetCloseApp(true);

	downloadManager = new DownloadManager();
	downloadManager->Run();
	gSettings = new SettingsManager(false);	// read and write

	if (sBeOSPackageItem) {
		(new RegisterWindow(sBeOSPackageItem, BMessenger(), true))->Show();
		return;
	}

	valetWindow = new ValetWindow();
	valetWindMess = BMessenger(valetWindow);
	valetWindow->Run();
	
	AddFileExtension(kUPkgSig,"pkg","SoftwareValet Package");
	AddFileExtension(kPkgDlSig,"vdwn","SoftwareValet Download");


#if FIRST_LAUNCH	
	new SetupWindow();
#endif
}

void ValetApp::ReadyToRun()
{
	// if we haven't put up a window yet, put up the main window now
	if (!fRefsUnprocessed && !fSomeWindowShowing && !sBeOSPackageItem) {
		InstallWindow::SetCloseApp(false); // don't quit the app when an install window quits
		valetWindow->Show();
	}
}


void AddFileExtension(const char *type, const char *next, const char *desc)
{
	// fix up the mime type extension
	BMessage	exts;

	BMimeType	pkgMime(type);

	pkgMime.GetFileExtensions(&exts);
	const char *ext;
	int i=0;
	while(exts.FindString("extensions",i++,&ext) == B_NO_ERROR) {
		if (!strcmp(ext,next))
			break;
	}
	if (!ext || strcmp(ext,next)) {
		exts.AddString("extensions",next);
		pkgMime.SetFileExtensions(&exts);
	}
	pkgMime.SetShortDescription(desc);
}

void ValetApp::AboutRequested()
{
	BString aboutMsg;
	aboutMsg << kProductNameString << B_UTF8_TRADEMARK << " v" << kVersionNumberString
		<< "\n" << B_UTF8_COPYRIGHT << "1996-1999 Be, Inc.\n\nzlib library"
		<< B_UTF8_COPYRIGHT << "1995-1996 Jean-loup Gailly and Mark Adler\n\n"
		<< "\tBy Michael Klingbeil\n\nServer:\t\tJustin Rowe, Carlin Wiegner\n"
		<< "\t\t\tMichael Klingbeil\nGraphics:\t\tWilliam Bull\n"
		<< "Documentation:\tStarCode support team";
	BAlert *alert =
	new BAlert(B_EMPTY_STRING, aboutMsg.String(), "Continue", NULL, NULL,
		B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Run();

	alert->Lock();
	alert->ResizeTo(340,220);
	PositionWindow(alert,0.5,1.0/3.0);
	BFont	f;
	font_family	fam;
	font_style sty;
	strcpy(fam,"Baskerville");
	strcpy(sty,"Roman");
	f.SetFamilyAndStyle(fam,sty);
	BTextView *v;
//	int i = 0;
	
	if ((v = cast_as(alert->FindView("_tv_"),BTextView))) {
		v->SetStylable(true);
		int line = v->OffsetAt(1);	// offset at line 1
		
		f.SetSize(18.0);
		v->SetFontAndColor(0,line,&f,B_FONT_ALL);
		f.SetSize(11.0);
	}
	alert->Unlock();
	
	alert->Go();
	key_info	info;
	get_key_info(&info);
	if ((info.modifiers & B_COMMAND_KEY) &&
		(info.modifiers & B_OPTION_KEY))
	{
		BFile		appFile(&appRef,O_RDONLY);
		BResources	appRes;
		if (appRes.SetTo(&appFile) < B_OK)
			return;
		size_t sz;
		void *bits = appRes.FindResource('BMAP',13L,&sz);
		if (bits)
		{
			BRect r(0,0,320-1,240-1);
			if (sz == 320*240) {
				BBitmap	*bmap = new BBitmap(r,B_RGB_32_BIT);
				bmap->SetBits(bits,320*240,0,B_GRAYSCALE_8_BIT);
				new SplashWindow(bmap);
			}
		}
		
	}
}


bool ValetApp::QuitRequested()
{
	bool okToQuit = TRUE;
	// try to close installer windows
	
	PRINT(("CLOSING installer windows\n"));
	
//	InstallWindow *prevWindow = NULL;
	do {
		InstallWindow::LockWindowList();	
		InstallWindow	*wind = InstallWindow::windowList.ItemAt(0);
		InstallWindow::UnlockWindowList();
		if (!wind)
			break;
		if (wind->Lock()) {
			okToQuit = wind->QuitRequested();
			if (okToQuit) {
				// windows now locked when calling Quit
				wind->Quit();
			}
			else {
				wind->Unlock();
				break;
			}
		}
		else
			;	// not sure what is best here!
	} while(TRUE);
	
	if (!okToQuit) {
		doError("Please finish any installations before quitting.");
		return okToQuit;	
	}

	// if no valetWindow, just bail out early...
	if (valetWindow == NULL)
		return BApplication::QuitRequested();
	
	// check manager window
	BLooper	*manWindow;
	valetWindow->manaWindow.Target(&manWindow);
	if (manWindow->Lock()) {
		okToQuit = manWindow->QuitRequested();
		if (okToQuit)
			manWindow->Quit();
		else {
			manWindow->Unlock();
			return okToQuit;	
		}
	}

	PRINT(("CLOSING main window\n"));	
	// try to close main window
	if (valetWindow->Lock()) {
		// locked when quitting
		valetWindow->Quit();
//		okToQuit;
	}
	okToQuit = true;
	
#if 0
	// delete the file panels and close their windows
	// HACK
	BAutolock	l(MFilePanel::dataLock);
	int count = MFilePanel::panelList.CountItems();
	while(--count >= 0)
		delete MFilePanel::panelList.ItemAt(count);
#endif	
		
	//Log	vLog;
	
	//BMessage	event(Log::VALET_QUIT);
	//vLog.PostLogEvent(&event);
	

	PRINT(("CLOSING other windows\n"));
	return BApplication::QuitRequested();
}

void ValetApp::MessageReceived(BMessage *msg)
{
	PRINT(("Valet App message received\n"));
	switch(msg->what) {
		// install register
		case 'IReg': {
			PRINT(("valet IReg message\n"));
			PackageItem *pkg;
			bool quitWhenDone = false;
			if (msg->HasBool("quit_after_reg")) {
				quitWhenDone = msg->FindBool("quit_after_reg");
			}
			msg->FindPointer("package",(void**)&pkg);
			if (pkg) {
				PRINT(("making register window\n"));
				new RegisterWindow(pkg,valetWindow->manaWindow,quitWhenDone);
			}
			break;
		}
		// update package display
		case 'PNew':
		case 'PDis': {
			PRINT(("forwarding package display message\n"));
			DetachCurrentMessage();
			valetWindow->manaWindow.SendMessage(msg);
			break;
		}
		case M_DO_DOWNLOAD : {
			valetWindow->PostMessage(msg);
			break;
		}
		case 'SLog': {
			// Show the low
			if (!TryActivate(logWindMess)) {
				BWindow *w = new LogWindow(BRect(0,0,300,460));
				PositionWindow(w,0.6,0.6);
				logWindMess = BMessenger(w);
				w->Show();
			}
			break;
		}
		case 'Url ': {
			downloadManager->PostMessage(msg);
			break;
		}
		case 'Rept': {		
			// view the upgrade report!!
			if (TryActivate(reportWindMess)) {
				reportWindMess.SendMessage(B_QUIT_REQUESTED);
			}
			reportWindMess = BMessenger(new UpdateWindow(msg->IsSourceRemote()));
			break;
		}
		default:
			BApplication::MessageReceived(msg);
	}
}

void ValetApp::DispatchMessage(BMessage *msg, BHandler *hand)
{
	enum {
		S_DLPATH	= 'SDPa'
	};
	
	switch(msg->what) {
		case S_DLPATH:
			if (msg->HasMessenger("messenger")) {
				DetachCurrentMessage();

				BMessenger dest;
				msg->FindMessenger("messenger",&dest);
				dest.SendMessage(msg);
			}
			else
				;
			break;
		default:
			BApplication::DispatchMessage(msg,hand);
	}
}

void ValetApp::RefsReceived(BMessage *msg)
{
	int32 count;
	uint32 type;
	msg->GetInfo("refs",&type,&count);
	for (int i = 0; i < count; i++) {
		entry_ref ref;
		if (msg->FindRef("refs",i,&ref) < B_NO_ERROR) {
			break;
		}
			
		BNode		node(&ref);
		if (!node.IsFile()) {
			continue;
		}
		BNodeInfo	nodeinf(&node);
		
		char typebuf[B_MIME_TYPE_LENGTH];
		
		status_t err = nodeinf.GetType(typebuf);
		if (err == B_NO_ERROR)
		{
			if (!strcmp(typebuf, kIPkgSig)) {
				BMessenger	temp(valetWindow);
				BMessage	reply;
				temp.SendMessage(M_DO_MANAGE,&reply);
				// wait for the window to show up			
				valetWindow->manaWindow.SendMessage(msg);
				fSomeWindowShowing = true;
				continue;
			}
			else if (!strcmp(typebuf, kPkgDlSig)) {
				// add to the download queue
				BMessage	dl('NwDl');
				dl.AddRef("refs",&ref);
				downloadManager->PostMessage(&dl);
				continue;
			}
			else if (!strcmp(typebuf, kPPkgSig)) {
				downloadManager->PostMessage(msg);
				continue;
			}
		}
		// need to look through the installWindow list to see if it already exists
		bool gotWindow = false;
		
		InstallWindow::LockWindowList();
		int count = InstallWindow::windowList.CountItems();
		for (int i = 0; i < count; i++) {
			InstallWindow	*wind = InstallWindow::windowList.ItemAt(i);
			BEntry ent(&ref);
			if (*wind->archiveFile->pEntry == ent) {
				wind->Activate(TRUE);
				gotWindow = true;
				break;
			}
		}
		InstallWindow::UnlockWindowList();
	
		if (!gotWindow) {
			InstallPack *rFile = new InstallPack(ref,FALSE);
			if (!rFile->pEntry) {
				delete rFile;
			}
			else {
				// fix up types if needed
				{
					BNode		node(rFile->pEntry);
					BNodeInfo	nodeInf(&node);
					char typ[B_MIME_TYPE_LENGTH];
					if ((nodeInf.GetType(typ) != B_NO_ERROR) ||
						 strncmp(typ,kPkgPrefix,strlen(kPkgPrefix)) != 0) {
						nodeInf.SetType(kUPkgSig);
					}
				}
				InstallWindow *aWindow = new InstallWindow("Installer",rFile,FALSE);
				fSomeWindowShowing = true;
				
				PRINT(("install window ptr %.8x\n",aWindow));
			}
		}
	}
	if (msg->HasBool("refs_from_argv")) {
		fRefsUnprocessed = false;
		ReadyToRun();
	}
}

void ValetApp::ArgvReceived(int32 argc, char **argv)
{
	PRINT(("checking args\n"));
	argc--;
	argv++;
	
	BMessage *msg = NULL;
		
	while(argc) {
		char *arg = *argv++;
		argc--;
			
		entry_ref r;
		if (get_ref_for_path(arg,&r) >= B_NO_ERROR) {
			PRINT(("got ref for %s\n",arg));
			if (!msg) {
				msg = new BMessage(B_REFS_RECEIVED);
			}
			msg->AddRef("refs",&r);
		}
	}
	if (msg) {
		PRINT(("sending refs received message to self\n"));
		msg->AddBool("refs_from_argv", true);
		fRefsUnprocessed = true;
		PostMessage(msg);
		delete msg;
	}
}

int verscompare(int amaj, int amid, int amin, int bmaj, int bmid, int bmin);

int verscompare(int amaj, int amid, int amin, int bmaj, int bmid, int bmin)
{
	int diff = amaj - bmaj;
	if (diff != 0)
		return diff;
	diff = amid - bmid;
	if (diff != 0)
		return diff;
	diff = amin - bmin;
	if (diff != 0)
		return diff;
	return 0;
}

status_t CheckLaunchListener(bool alwaysLaunch)
{
	status_t 	err = B_NO_ERROR;
	bool running = be_roster->IsRunning(kListenerSig);
	if (running && !alwaysLaunch)
		return err;
		
	team_id		team;	
	err = be_roster->Launch(kListenerSig,(BMessage *)NULL,&team);
	if (err != B_OK /*< B_NO_ERROR*/) {
		doError("Could not launch the SoftwareValet Transceiver. Make sure the Transceiver is installed.");
	}
	else {
		app_info	info;
		be_roster->GetRunningAppInfo(team,&info);
		
		BFile			appFile(&info.ref,O_RDONLY);
		if (appFile.InitCheck() == B_NO_ERROR) {
			PRINT(("CHECKING VERSION\n"));
			BAppFileInfo	fInfo(&appFile);
			version_info	vers;
//			bool old = true;
			if (fInfo.GetVersionInfo(&vers,B_APP_VERSION_KIND) != B_NO_ERROR)
				vers.major = vers.middle = vers.minor = 0;
			
			PRINT(("GOT VERSION\n"));
				
			if (verscompare(vers.major, vers.middle, vers.minor,
					kListenerMajorVers, kListenerMiddleVers, kListenerMinorVers) < 0) {
				char versString[24];
				if (kListenerMinorVers)
					sprintf(versString,"%d.%d.%d",kListenerMajorVers,kListenerMiddleVers,kListenerMinorVers);
				else
					sprintf(versString,"%d.%d",kListenerMajorVers,kListenerMiddleVers);
					
				doError("The version of SoftwareValet Transceiver found is too old. Please be sure you have installed at least version %s.",
					versString);
			}
		}
	}
	return err;
}

