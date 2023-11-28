// InstallApp.cpp
#include "InstallApp.h"
#include "InstallWindow.h"
#include "InstallDefs.h"
#include "InstallMessages.h"
#include "InstallPack.h"
#include "HelpButton.h"

#include "MyDebug.h"
#include <FilePanel.h>

#include "FSIcons.h"
#include "HeapWindow.h"

#include "Util.h"

#if DEBUG
#define MAKE_SETTINGS 0
#include "SettingsManager.h"
#endif

#define HEAP_WINDOW 0
#ifdef HEAP_WINDOW
	HeapWindow	*gHeapWindow;
#endif

// globals
entry_ref 			InstallApp::appRef;
SettingsManager		*gSettings;


#if (SEA_INSTALLER)
int main()
{
	InstallApp *app = new InstallApp();
	app->Run();
	app->FreeGlobals();
	delete app;
	return 0;
}

InstallApp::InstallApp()
	:	BApplication(APP_SIGNATURE),
		appQuitting(FALSE),
		initialInstallPack(0)
{
	app_info	info;
	be_app->GetAppInfo(&info);
	appRef = info.ref;

	CheckSelfExtracting();
	
#if MAKE_SETTINGS
	gSettings = new SettingsManager(true); //read only
#endif
	InstallApp::SetGlobals(appRef);
	InstallWindow::SetCloseApp(TRUE);
}

// check if this is a self-extracting installer
void InstallApp::CheckSelfExtracting()
{
	BEntry			appEnt(&appRef);
#if (ARGV)
	// if argv mode check if we are really self-extracting or not!
	BFile			appFile(&appRef,O_RDONLY);
	BResources		appRes(&appFile);
	selfExtracting = appRes.HasResource(0L,SIZE_RES_TYPE);
	// set the window name based on whether self-extracting or not	
	if (selfExtracting)
		appEnt.GetName(appName);
	else
		strcpy(appName,"Be, Inc. Installer");
#else
	selfExtracting = TRUE;
	appEnt.GetName(appName);
#endif // ARGV
}

void InstallApp::ReadyToRun()
{
#if HEAP_WINDOW
	gHeapWindow = new HeapWindow();
#endif

#if (ARGV)
	// if we accept argv launch
	if (!selfExtracting && initialInstallPack) {
		PRINT(("got initial install pack name %s\n",initialInstallPack));
		
		entry_ref	fRef;
		get_ref_for_path(initialInstallPack,&fRef);
		
		BMessage msg(B_REFS_RECEIVED);
		msg.AddRef("refs",&fRef);
		
		PostMessage(&msg,this);
		return;
	}
	else if (!selfExtracting) {
		// non-self extracting installer, open file panel
		return;
	}
#endif // ARGV

	PRINT(("building from appRef\n"));
	InstallPack *pack = new InstallPack(appRef,true);
	if (!pack->pEntry) {
		delete pack;
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	char docName[B_FILE_NAME_LENGTH];
	pack->pEntry->GetName(docName);
		
	// later get package name from resources
	mainWindow = new InstallWindow(docName,pack,TRUE);
}

#if (ARGV)
void InstallApp::RefsReceived(BMessage *msg)
{
	ulong type;
	long count;
	InstallPack *packFile;
	
	msg->GetInfo("refs",&type,&count);
	for (long i = 0; i < count; i++) {
		
		entry_ref ref;
		msg->FindRef("refs",i,&ref);
		packFile = new InstallPack(ref,FALSE);
		if (!packFile->pEntry) {
			delete packFile;
			PostMessage(B_QUIT_REQUESTED);
		}
		else {
			mainWindow = new InstallWindow(appName,packFile,TRUE);
		}
	}
}
#endif // ARGV

#if (ARGV)
void InstallApp::ArgvReceived(int32 argc, char **argv)
{
	if (IsLaunching()) {
		PRINT(("checking args\n"));
		argc--;
		argv++;
		
		char *arg = *argv++;
		argc--;
		initialInstallPack = strdup(arg);
	}
}
#endif // ARGV

bool InstallApp::QuitRequested()
{
#if HEAP_WINDOW
	gHeapWindow->Quit();
#endif
	return BApplication::QuitRequested();	
}

#endif // SEA_INSTALLER


void InstallApp::SetGlobals(entry_ref appRef)
{
	SetGlobalIcons(appRef);
}

void InstallApp::FreeGlobals()
{
	// free global icons
	// free listview pictures
#if USING_HELP
	HelpButton::Free();
#endif
}
