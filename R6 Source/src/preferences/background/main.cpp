//*****************************************************************************
//
//	File:		 main.cpp
//
//	Description: Application entry points
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "BackgroundApp.h"
#include <image.h>
#include <OS.h>
#include <TrackerAddOn.h>
#include <Alert.h>
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <string.h>

const char *backsig = "application/x-vnd.Be-BACK";
bool find_addon_path(char *buf);

int main(int, char **)
{
	BackgroundApp	app(backsig);

	app.Run();

	return 0;
}

extern "C" _EXPORT void process_refs(entry_ref, BMessage *msg, void *)
{
	BMessenger	view;
	if(! msg->HasRef("refs") &&
		msg->FindMessenger("TrackerViewToken", &view) == B_OK &&
		view.IsValid())
	{
		// no refs, determine where it was launched from
		BMessage	path(B_GET_PROPERTY);
		BMessage	reply;
		entry_ref	ref;

		path.AddSpecifier("Path");
		view.SendMessage(&path, &reply);
		if(reply.what != B_MESSAGE_NOT_UNDERSTOOD &&
			reply.FindRef("result", &ref) == B_OK)
		{
			msg->AddRef("refs", &ref);
		}
	}

	// Launch myself as an application
	char	buf[MAXPATHLEN];
	if(find_addon_path(buf))
	{
		BEntry		e(buf);
		entry_ref	ref;
		e.GetRef(&ref);
		be_roster->Launch(&ref, msg);
	}
	else
		// addon path not found??? launch preferred
		be_roster->Launch(backsig, msg);

//	if(be_roster->IsRunning(backsig))
//	{
//		BMessenger	m(backsig);
//		m.SendMessage(msg);
//	}
//	else
//		be_roster->Launch(backsig, msg);
}


// a hackish way of finding add-on information, based on
// an old post to BeDevTalk by Jon Wätte (no less)
static char	globalvar = 1;

bool find_addon_path(char *buf)
{
	const char	*addr = &globalvar;
	image_info	info;
	int32		cookie = 0;

	// Get the image_info for every image in this team.
	while(get_next_image_info(0, &cookie, &info) == B_OK)
	{
		if (((char *)info.data <= addr) &&
			(((char *)info.data+info.data_size) > addr))
		{
			strcpy(buf, info.name);
			return true;
		}
	}

	return false;
}
