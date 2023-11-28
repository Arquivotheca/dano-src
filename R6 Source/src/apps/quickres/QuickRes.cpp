#include "QuickRes.h"
#include "ResWindow.h"

#include <ResourceEditor.h>
#include <ResourceEntry.h>

#include <experimental/DocApplication.h>

#include <Alert.h>
#include <StringView.h>
#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <File.h>
#include <FindDirectory.h>
#include <Roster.h>
#include <Debug.h>
#include <Locker.h>
#include <Autolock.h>
#include <image.h>
#include <stdio.h>

const char* AppSignature = "application/x-vnd.Be.QuickRes";
const char* ResDefSignature = "text/x-vnd.Be.ResourceDef";

static void InstallDescription(BMimeType& type, const char* sdescr, const char* ldescr)
{
	char cur[B_MIME_TYPE_LENGTH];
	if( sdescr ) {
		cur[0] = 0;
		PRINT(("Checking short description...\n"));
		status_t err = type.GetShortDescription(cur);
		if( err != B_OK || cur[0] == 0 ) {
			type.SetShortDescription(sdescr);
			PRINT(("Installing mine.\n"));
		}
		PRINT(("Req short description: err=%lx, string=%s\n", err, &cur[0]));
	}
	if( ldescr ) {
		cur[0] = 0;
		PRINT(("Checking long description...\n"));
		status_t err = type.GetLongDescription(cur);
		if( err != B_OK || cur[0] == 0 ) {
			type.SetLongDescription(ldescr);
			PRINT(("Installing mine.\n"));
		}
		PRINT(("Req long description: err=%lx, string=%s\n", err, &cur[0]));
	}
}

static void InstallPrefApp(BMimeType& type, const char* app)
{
	char cur[B_MIME_TYPE_LENGTH];
	cur[0] = 0;
	PRINT(("Checking preferred app...\n"));
	status_t err = type.GetPreferredApp(cur);
	if( err != B_OK || cur[0] == 0 ) {
		type.SetPreferredApp(app);
		PRINT(("Installing mine.\n"));
	}
	PRINT(("Req preferred app: err=%lx, string=%s\n", err, &cur[0]));
}

static void InstallExtensions(BMimeType& type, const char** extensions)
{
	BMessage existing;
	PRINT(("Checking filename extensions...\n"));
	status_t err = type.GetFileExtensions(&existing);
	if( err != B_OK ) existing = BMessage();
	
	bool changed = false;
	while( extensions && *extensions ) {
		const char* e;
		bool has = false;
		for( int32 i=0;
			 !has && existing.FindString("extensions", i, &e) == B_OK;
			 i++ ) {
			if( strcmp(e, *extensions) == 0 ) has = true;
		}
		if( !has ) {
			PRINT(("Installing new extensions %s\n", *extensions));
			existing.AddString("extensions", *extensions);
			changed = true;
		}
		extensions++;
	}
	
	if( changed ) {
		err = type.SetFileExtensions(&existing);
		PRINT(("Writing new extension array: err=%lx.\n", err));
	}
}

static void AddMimeTypes()
{
	BMimeType mt;
	
	mt.SetTo(ResDefSignature);
	PRINT(("Setting up resource definition MIME type...\n"));
	if( mt.InitCheck() == B_OK ) {
		if( !mt.IsInstalled() ) {
			PRINT(("Resource definition type not installed, installing it.\n"));
			mt.Install();
		}
		static const char* extensions[] = { "rdef", NULL };
		InstallDescription(mt, "Resource Definition",
						   "Source for generating a resource file.");
		InstallExtensions(mt, &extensions[0]);
		InstallPrefApp(mt, AppSignature);
	}
	mt.Unset();
}

DocWindow *myfactory(WindowRoster *wr, entry_ref *ref, const char *title,
		window_look look, window_feel feel, uint32 flags, uint32 workspace)
{
	return new ResWindow(wr, ref, title, look, feel, flags, workspace);
}


QuickResApp::QuickResApp()
 : DocApplication(AppSignature, myfactory), firstframe(true)
{
}

void QuickResApp::ReadyToRun()
{
	AddMimeTypes();
	DocApplication::ReadyToRun();
}

void QuickResApp::AboutRequested()
{
	if( !fAboutWindow.IsValid() ) {
		BAlert* alert = new BAlert("About QuickRes",
								   "QuickRes build " __DATE__ ".\n\n"
								   "An experimental tool for viewing and "
								   "editing program resources and file "
								   "attributes.",
								   "Mmmmm" B_UTF8_ELLIPSIS " resources.",
								   NULL, NULL,
								   B_WIDTH_FROM_WIDEST, B_EVEN_SPACING,
								   B_INFO_ALERT);
		alert->SetFeel(B_FLOATING_APP_WINDOW_FEEL);
		alert->Go(NULL);
	}
}

int main(int, char **)
{
	QuickResApp	app;
	app.Run();
	return 0;
}
