// (c) 1997-98 Be Incorporated
//

#include <Debug.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <TrackerAddon.h>
#include <TypedList.h>
#include <Alert.h>
#include <Path.h>

#include <string.h>

#include "MIMEApp.h"
#include "MIMEPanel.h"
#include "MIMEAttributes.h"
#include "AppMimeWindow.h"
#include "FileTypeWindow.h"

rgb_color kWhite = { 255, 255, 255, 255};
rgb_color kBlack = { 0, 0, 0, 255};
rgb_color kViewGray;
rgb_color kThinGray;
rgb_color kDarkGray;
rgb_color kMediumGray;
rgb_color kGridGray;
rgb_color kLightGray;

BFilePanel *openPanel = NULL;

BFilePanel *
FileTyperOpenPanel()
{
	if (!openPanel)
		openPanel = new BFilePanel(B_OPEN_PANEL);

	return openPanel;
}

BPoint where(40, 40);

static BWindow *
OpenWindow(BList *refs)
{
	//
	//	Mimeset all of the files
	//
	int32 numItems = refs->CountItems();
	for (int32 item = 0; item < numItems; ++item) {
		BEntry entry((const entry_ref*)refs->ItemAt(item), true);
		BPath path;
		if (entry.GetPath(&path) == B_OK)
			update_mime_info(path.Path(), false, true, false);
	}

	//
	//	If the user has selected a single application type file, show the
	//	application dialog.  Otherwise, show the file types dialog with
	//	all selected applications in it.
	//
	if (refs->CountItems() == 0) {
		return NULL;
	} else if (refs->CountItems() == 1) {
		entry_ref *ref = (entry_ref*) refs->ItemAt(0);

		char buffer[255];
		BFile file(ref, O_RDWR);
		status_t result = file.InitCheck();
		if (result != B_NO_ERROR) {
			sprintf(buffer, "Could not open file %s, %s", ref->name,
				strerror(result));
			(new BAlert("", buffer, "OK"))->Go();
			return NULL;
		}

		//
		// Check the type to see if this is an application.
		//
		BNodeInfo nodeInfo(&file);
		if (nodeInfo.InitCheck() != B_NO_ERROR) {
			PRINT(("error opening nodeinfo for file\n"));
			return NULL;
		}

		nodeInfo.GetType(buffer);

		if (strcmp(buffer, B_APP_MIME_TYPE) == 0 || 
		  strcmp(buffer, B_RESOURCE_MIME_TYPE) == 0) 
		{
			return new AppMimeTypeWindow(BPoint(100, 100), ref);
		}
	} 

	return new FileTypeWindow(BPoint(100, 100), refs);
}

static void
RefsReceived(BMessage *message)
{
	int32 count = 0;
	uint32 type = 0;
	entry_ref ref;

	PRINT(("refs received \n"));

	// need to keep track of windows we open so we can block, waiting for them
	// to finish
	// if we don't do this and process_refs returns, the addon code will get unloaded
	// and the windows will die a horrible death
	TypedList<BWindow *> windowList;

	message->GetInfo("refs", &type, &count);
	BList *refList = new BList(count);
	for (long i = 0; i < count; i++) {
		entry_ref *ref = new entry_ref;
		if (message->FindRef("refs", i, ref) == B_NO_ERROR) {
			refList->AddItem(ref);
		} else {
			delete ref;
			break;
		}
	}
	
	BWindow *win = OpenWindow(refList);
	if ( win != NULL )
		windowList.AddItem(win);
	else
		delete refList;
	
	long dummy;
	// block on all the windows, waiting for them to return
	for (int32 index = windowList.CountItems() - 1; index >= 0; index--) {
		BWindow *window = windowList.ItemAt(index);
		if (window->Lock()) {
			// if lock failed here, the window is already gone,
			// no point waiting for it
			thread_id thread = window->Thread();
			window->Unlock();
			wait_for_thread(thread, &dummy);
		}
	}
}

void
process_refs(entry_ref /*dir_ref*/, BMessage* msg, void*)
{
	kViewGray = ui_color(B_PANEL_BACKGROUND_COLOR);
	kLightGray = ui_color(B_PANEL_BACKGROUND_COLOR);
	kThinGray = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT);
	kDarkGray = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT);
	kMediumGray = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
	kGridGray = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), 1.0 + (B_DARKEN_1_TINT-1.0)*0.5);

	RefsReceived(msg);
	delete openPanel;
	openPanel = NULL;
}

