#include <Alert.h>
#include <Application.h>
#include <ClassInfo.h>
#include <Control.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <List.h>
#include <Path.h>
#include <String.h>
#include <string.h>
#include <TextView.h>

/******************** Major Application Setup *****************/
#include "PackApplication.h"
#include "PackageBuilderVersion.h"
#include "PackWindow.h"
#include "PackMessages.h"
#include "PackArc.h"
#include "MFilePanel.h"

//#if (!__INTEL__)
//	#include "DatatypesAddOn.h"
//#endif
#include "RWindow.h"

#include "MyDebug.h"
#include "GlobalIcons.h"

#include "PrefsWindow.h"
#include "Util.h"

#if HEAP_WINDOW
#include "HeapWindow.h"

RWindow *gHeapWindow;

#endif

// for activating the status window ???
enum {
	M_DO_ACTIVATE = 'DoAC'
};

long	CreatePathAt(const BDirectory *dir,const char *path,BEntry *final);
long	RecursiveDelete(BEntry *ent);
void	mallocErr(enum mcheck_status);

void	mallocErr(enum mcheck_status stat)
{
	switch (stat) {
		case MCHECK_OK:
			//"Block is fine."
			break;
   		case MCHECK_FREE:
   			DEBUGGER("Block freed twice.");
   			break;
    	case MCHECK_HEAD:
    		DEBUGGER("Memory before the block was clobbered.");
    		break;
    	case MCHECK_TAIL:
    		DEBUGGER("Memory after the block was clobbered.");
    		break;
	}
}



int main(int, char **)
{
	// mcheck(mallocErr);
	
	PackApp *theApplication;
	
	theApplication = new PackApp();
	theApplication->Run();
	
	delete theApplication;
	
	return(0);
}

PackApp::PackApp()
	: BApplication(APP_SIGNATURE),
	  fR4Compatible(false)
{
	SetGlobalIcons();
	untitledCount = 0;
	
	//app_info info;
	//be_app->GetAppInfo(&info);
	//gStrs = new IDStrings(info.ref);
	
	ReadPreferences();
	
#if 0 && (!__INTEL__)
	// setup datatypes for splash graphics
	// no error message if couldn't load
	if (DATALoadCallbacks() >= 0) {
		long curVersion, minVersion;
		DATACallbacks.DATAVersion(curVersion, minVersion);
		if (minVersion > DATA_CURRENT_VERSION) {
			//doError("The version of Datatypes currently installed is too new.\
// You should probably update to a newer version of PackageBuilder.");
			DATACallbacks.image = -1;	
		}
		else if (curVersion < DATA_MIN_VERSION) {
			doError("The version of Datatypes currently installed is too old.\
 Please update to at least version 1.6.1");
	 		DATACallbacks.image = -1;
		}
		if (DATACallbacks.image > 0) {
			long err = DATACallbacks.DATAInit(APP_SIGNATURE);
			if (err < B_NO_ERROR) {
				doError("Failed to initialize Datatypes translators.\n%s",strerror(err));
			}
		}
	}
#endif
}

status_t PackApp::GetPrefsFile(BEntry *prefEntry)
{
	status_t err;	
	bool	newSettings = FALSE;

	const char fileName[] = "PackageBuilder_settings";
	BPath settingsPath;
	BDirectory settingsDir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath, true) == B_OK) {
		settingsDir.SetTo(settingsPath.Path());
		if (!settingsDir.Contains(fileName)) {
			PRINT(("creating pref file\n"));
			BFile	f;
			err = settingsDir.CreateFile(fileName,&f);
			if (err < B_NO_ERROR) {
				doError("Error creating settings file. Using defaults.");
				return err;
			}
			newSettings = TRUE;						
		}
	}

	err = settingsDir.FindEntry(fileName, prefEntry);
	if (err < B_NO_ERROR || !prefEntry->IsFile()) {
		doError("Error finding settings file.");
		return B_ERROR;
	}
	if (newSettings) {
		BFile prefFile(prefEntry, O_WRONLY);
		err = prefFile.Write(&prefData, sizeof(UserPrefs));
	}
	return err;
}

void PackApp::ReadPreferences()
{
	// default in case of error
	prefData = 0 | AUTO_COMPRESS;
	
	BEntry prefEntry;
	if (GetPrefsFile(&prefEntry) < B_NO_ERROR)
		return;
	
	PRINT(("reading prefs data\n"));
	BFile	prefFile(&prefEntry,O_RDONLY);
	prefFile.Read(&prefData,sizeof(prefData));
	
	#if DEBUG
		PRINT(("auto compression is %s\n",
				(prefData & AUTO_COMPRESS) ? "on" : "off"));
		PRINT(("cd-rom install is %s\n",
				(prefData & CD_INSTALL) ? "on" : "off"));
	#endif 
}

void PackApp::WritePreferences()
{
	BEntry prefEntry;
	if (GetPrefsFile(&prefEntry) < B_NO_ERROR)
		return;
	
	PRINT(("writing prefs data\n"));
	BFile	prefFile(&prefEntry,O_WRONLY);
	prefFile.Write(&prefData,sizeof(prefData));
}

void PackApp::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_DO_PREFS: {
			if (prefsWindowMessenger.IsValid())
				prefsWindowMessenger.SendMessage(M_DO_ACTIVATE);
			else
				prefsWindowMessenger = BMessenger(new PrefsWindow(prefData));
			break;
		}
		case M_NAME_WINDOW: {
			if (prefsWindowMessenger.IsValid()) {
				DetachCurrentMessage();
				prefsWindowMessenger.SendMessage(msg);
			}
			break;
		}
		case M_DO_NEW: {
			untitledCount++;
			char windowTitle[B_FILE_NAME_LENGTH + 1 + 24];
			sprintf(windowTitle,"Untitled %ld",untitledCount);
			PackWindow *pw = new PackWindow(windowTitle,NULL,
											prefData & AUTO_COMPRESS,
											prefData & CD_INSTALL);
			if (prefsWindowMessenger.IsValid()) {
				BMessage newWindow(M_DO_NEW);
				newWindow.AddPointer("window",pw);
				prefsWindowMessenger.SendMessage(&newWindow);
			}
			pw->Show();
			//pw->PostMessage(M_PACK_SETTINGS);
			break;
		}
		case M_DO_OPEN: {
			
			if (!TryActivate(openPanelMsngr)) {
				BFilePanel *panel = new MFilePanel(B_OPEN_PANEL,
													new BMessenger(this),
													NULL,
													false,
													true,
													B_REFS_RECEIVED);
				openPanelMsngr = BMessenger(panel->Window());
				panel->Show();
			}
			break;
		}
		case M_SAVE: {
			WritePreferences();
			break;
		}
		case M_TOGGLE_R4_COMPATIBLE: {
			SetCompatibleMode(!fR4Compatible);
			break;
		}
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

void PackApp::ReadyToRun()
{
	if (RWindow::CountWindows() == 0) {
		PostMessage(M_DO_NEW);
	}
	
	#if HEAP_WINDOW
	gHeapWindow = new HeapWindow();
	#endif
}

void PackApp::ArgvReceived(int32 /*argc*/, char **/*argv*/)
{
	PRINT(("PackApp::ArgvReceived\n"));
}

bool PackApp::IsOpen(BEntry *entry)
{
	// get a copy of the windowlist
	BList	windList = RWindow::WindowList();
	
	bool found = FALSE;	
	for (int32 i = windList.CountItems()-1; i >= 0 && !found; i--) {
		BWindow *w = (BWindow *)windList.ItemAt(i);
		PackWindow *pw = cast_as(w,PackWindow);
		if (pw->Lock()) {
		
			if (pw->realFile && (*pw->realFile == *entry)) {
				pw->Activate(TRUE);
				found = TRUE;
			}
			pw->Unlock();
		}
	}
	return found;
}

void PackApp::RefsReceived(BMessage *msg)
{
	PRINT(("PackApp::RefsReceived\n"));
	// this means we are opening an archive file	
	ulong type;
	long count;

	msg->GetInfo("refs", &type, &count);
	for (long i = 0; i < count; i++) {
		entry_ref item;
		msg->FindRef("refs",i,&item);
		
		BEntry	entry(&item);
		if (!entry.IsFile())
			continue;
		
		if (IsOpen(&entry))
			continue;
							
		PRINT(("creating new PackFile object\n"));
		PackArc *aFile = new PackArc(&item, CURRENT_VERSION);
			
		if (aFile->InitCheck() < B_NO_ERROR) {
			PRINT(("error opening the package file!\n"));
			delete aFile;
		}
		else {
			char windowTitle[B_FILE_NAME_LENGTH+1];
			entry.GetName(windowTitle);
			// the real settings for autocompress, CD-rom etc will be read from
			// the file in the constructor
			PackWindow *pw = new PackWindow(windowTitle,aFile,
								prefData & AUTO_COMPRESS);
			//
			// notify prefs window
			if (prefsWindowMessenger.IsValid()) {
				BMessage newWindow(M_DO_NEW);
				newWindow.AddPointer("window",pw);
				prefsWindowMessenger.SendMessage(&newWindow);
			}
			pw->Show();
		}
	}
}

void DisplayViews(BView *v);

void PackApp::AboutRequested()
{
	BString aboutMsg;
	aboutMsg << kProductNameString << B_UTF8_TRADEMARK << " v" << kVersionNumberString
		<< "\n" << B_UTF8_COPYRIGHT << "1996-1999 Be, Inc.\n\nzlib library"
		<< B_UTF8_COPYRIGHT << "1995-1996 Jean-loup Gailly and Mark Adler\n\n"
		<< "E-Mail:\tpbcustsupport@be.com\nWeb:\t\thttp://www.be.com\n";
	BAlert *alert =
	new BAlert(B_EMPTY_STRING, aboutMsg.String(), "Continue", NULL, NULL,
		B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Run();

	alert->Lock();
	alert->ResizeTo(340,200);
	PositionWindow(alert,0.5,1.0/3.0);
	BFont	f;
	font_family	fam;
	font_style sty;
	strcpy(fam,"Baskerville"); // This is bad -- assumes a specific font is on the system
	strcpy(sty,"Roman");
	f.SetFamilyAndStyle(fam,sty);
	BTextView *v;
//	int i = 0;
	
	if ((v = cast_as(alert->FindView("_tv_"),BTextView)) != NULL) {
		v->SetStylable(true);
		int line = v->OffsetAt(1);	// offset at line 1
		
		f.SetSize(18.0);
		v->SetFontAndColor(0,line,&f,B_FONT_ALL);
		f.SetSize(11.0);
	}
	alert->Unlock();
	
	alert->Go();
}

void DisplayViews(BView *m)
{
	BView *v;
	int i = 0;
	while ((v = m->ChildAt(i++)) != NULL) {
		doError("View %s",v->Name());
		DisplayViews(v);
	}
}


void PackApp::CleanUpTempFiles()
{
	status_t err;
	// XXX: change to find_directory
	BDirectory tmpDir("/boot/var/tmp");
	err = tmpDir.InitCheck();
	if (err < B_NO_ERROR) {
		doError("Could not find \"/boot/var/tmp\"");
		return;
	}
	BEntry ourTmpDir;
	char buf[B_FILE_NAME_LENGTH];
	sprintf(buf,"PackageBuilder%ld",Team());
	
	PRINT(("finding directory %s\n",buf));
	
	err = tmpDir.FindEntry(buf,&ourTmpDir);
	if (err == B_NO_ERROR && ourTmpDir.IsDirectory()) {
		RecursiveDelete(&ourTmpDir);
	}
	else {
		// didn't find temporary directory so maybe we didn't create
		// one (if launched with file??)
	}
}


bool PackApp::QuitRequested()
{
	// should order this in terms of document windows only
	// look for our temp directory

	bool res = TRUE;
		
	//res = BApplication::QuitRequested();
	RWindow *wind, *prevWind;
	
	// this sometimes goes into an infinite loop!!!!
	// this means a window which has been destroyed
	// hasn't been removed from the window list
	
	// window closed
	// returns true
	// app server calls Quit()
	prevWind = NULL;
	// meanwhile we get this window from the window list
	while ((wind = RWindow::Front()) != NULL) {
		PRINT(("window loop value is %d\n",wind));
		if (prevWind == wind)
			break;
		prevWind = wind;
		if (wind->Lock()) {
			PRINT(("calling quit requested\n"));
			res = wind->QuitRequested();
			if (res == TRUE) {
				PRINT(("Window returned true so quitting it\n"));
				wind->Quit();
			}
			else {
				wind->Unlock();
				break;
			}
		}
	}
	if (res == TRUE) {
		// remove temp files
		CleanUpTempFiles();

//#if (!__INTEL__)		
//		// shutdown datatypes
//		if (DATACallbacks.image > 0)
//			DATACallbacks.DATAShutdown();
//#endif
		Quit();
	}
	return res;
}

void PackApp::SetCompatibleMode(bool compatible)
{
	if (fR4Compatible != compatible) {
		fR4Compatible = compatible;
		// iterate through all windows and set compatible menu item appropriately
		PackWindow *win;
		int32 winCount = be_app->CountWindows();
		for (int32 i = 0; i < winCount; i++) {
			win = dynamic_cast<PackWindow *>(be_app->WindowAt(i));
			if (win) {
				win->SetCompatibleMode(compatible);
			}
		}
	}
}

bool TryActivate(BMessenger &mess)
{
	BLooper *loop;
	BWindow *wind;
	if (mess.IsValid()) {
		mess.Target(&loop);
		wind = cast_as(loop,BWindow);
		if (wind->Lock()) {
			wind->Activate(TRUE);
			wind->Unlock();
			return TRUE;
		}
	}
	return FALSE;
}

bool GetBoolControlValue(BMessage *m);

bool GetBoolControlValue(BMessage *m)
{
	BControl *p;
	m->FindPointer("source",(void **)&p);
	
	ASSERT(p);
	
	bool state = p->Value() == B_CONTROL_ON;
	return state;
}

