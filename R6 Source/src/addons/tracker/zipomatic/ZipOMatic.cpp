#include <Roster.h>
#include <TrackerAddOn.h>
#include <stdio.h>

#include "ZipOMatic.h"
#include "ZipOWindow.h"

const char *kSignature = "application/x-vnd.Be.ZipOMatic";


ZipOMaticApp::ZipOMaticApp(const char *signature)
	:	BApplication(signature),
		windowCount(0)
{
}

void 
ZipOMaticApp::ReadyToRun()
{
	if (!WindowAt(0)) {
		// didn't get any refs, start an open window
		windowCount++;
		ZipOWindow *window = new ZipOWindow(BPoint(40, 40), false);
		window->Show();
	}
}

void 
ZipOMaticApp::RefsReceived(BMessage *message)
{
	if (message->HasRef("refs")) {
		windowCount++;
		ZipOWindow *window = new ZipOWindow(BPoint(40 + 15 * (windowCount % 10),
			40+ 15 * (windowCount % 10)), true);
		window->Show();
		if (window->Lock()) {
			message->what = B_SIMPLE_DATA;
			window->PostMessage(message);
			window->Unlock();
		}
	}
}

void 
ZipOMaticApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kWindowClosed:
			if (--windowCount <= 0)
				PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			_inherited::MessageReceived(message);
			break;		
	}
}



extern "C"
void process_refs(entry_ref, BMessage *message, void *)
{
	// Short-lived career as a Tracker addon, launch self as an app
	// Forward RefsReceived to myself as an application
	be_roster->Launch(kSignature, message);
}

int
main(int, char **)
{
	ZipOMaticApp app(kSignature);
	app.Run();
	
	return 0;
}
