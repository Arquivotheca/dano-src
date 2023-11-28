#include <Directory.h>
#include <File.h>
#include <InterfaceDefs.h>
#include <Path.h>
#include <Protocol.h>
#include <Roster.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <StringBuffer.h>
#include <TextView.h>
#include <URL.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

const BRect windowRect(0, 0, 350, 350);

int32 ViewSourceWorker(void*);

void ViewSource(const URL &source)
{
	// Disable view source in released version.  This currently
	// uses the ALTQ variable to determine what type of build this
	// is (ALTQ is 0 in release versions).
	const char *altq = getenv("ALTQ");
	if (altq && atoi(altq) == 0)
		return;

	URL *src = new URL(source);
	resume_thread(spawn_thread(ViewSourceWorker, "view source", B_NORMAL_PRIORITY, src));
}

int32 ViewSourceWorker(void *_src)
{
	URL url(*(URL*) _src);
	delete (URL*) _src;
		
	BVolume volume;
	BVolumeRoster roster;
	
	roster.GetBootVolume(&volume);
	
	bool internal = !(volume.KnowsQuery());
	
	BFile file;
	BPath path;
	BTextView *textView = NULL;
	BWindow *win = NULL;
	
	if (!internal) {
		BString string;
		string << "/tmp/html";
		string << "/" << url.GetHostName();

		if(strcmp(url.GetPath(), "/") == 0)
			string << "/index.html";
		else 
			string << url.GetPath();
		
		/* This is very nasty, but I have no choice. DAMN YOU BPath! */
		string.ReplaceAll("//", "/");
		
		if (path.SetTo(string.String()) == B_OK) {
			BPath parent;
			path.GetParent(&parent);
		
			create_directory(parent.Path(), 0777);
		}
						
		if (file.SetTo(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE) != B_OK) 
			internal = true;
	}
	
	if (internal) {
		StringBuffer title;
		title << "Source for \"" << url << "\"";
		// The const_cast is to shutup the compiler... 
	
		win = new BWindow(const_cast<BRect>(windowRect).OffsetToCopy(50, 50), title.String(), B_DOCUMENT_WINDOW, 0);
		BRect tv(windowRect);
		tv.right -= B_V_SCROLL_BAR_WIDTH;
		tv.bottom -= B_H_SCROLL_BAR_HEIGHT;
		textView = new BTextView(tv, "source", BRect(5, 5, 2000, 1000000),
			B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
		textView->SetWordWrap(false);
		textView->MakeEditable(false);
		BScrollView *scroller = new BScrollView("scroll", textView, B_FOLLOW_ALL_SIDES,
			0, true, true, B_PLAIN_BORDER);
		win->AddChild(scroller);
		win->Show();
	}		
	
	
	Protocol *protocol = 0;
	
	for (;;) {
		// Open a connection
		protocol = Protocol::InstantiateProtocol(url.GetScheme());
		if (protocol == 0) {
			break;
		}
		
		BMessage msg;
		status_t error = protocol->Open(url, url, &msg, 0);
		if (error < 0) {
			delete protocol;
			protocol = 0;
			break;
		}
		
		bigtime_t delay;
		if (protocol->GetRedirectURL(url, &delay)) {
			delete protocol;
			protocol = 0;
			continue;		// This is a redirect, loop and continue
		}
		
		break;
	}

	if (protocol) {
		int32 offset = 0;
		for (;;) {
			const int kBufferSize = 0x4000;
			char buf[kBufferSize];
			ssize_t count = protocol->Read(buf, kBufferSize);	
			if (count <= 0)
				break;

			if (internal) {
				if (!win->Lock())
					break;
					
				textView->Insert(offset, buf, count);
				offset += count;
				win->Unlock();
			}
			else
				file.Write(buf, count);
		}

		delete protocol;
	}
	else {
		BString error = "Couldn't open connection";
		if (internal) {
			if (win->Lock()) {
				textView->Insert(error.String());
				win->Unlock();
			}
		}
		else 
			file.Write(error.String(), error.Length());
	}
	
	if (!internal) {
		int argc = 1;
		char *argv[2];
		
		argv[0] = (char *)path.Path();
		argv[1] = NULL;
			
		be_roster->Launch("text/plain", argc, argv);		
	}
		
	return B_OK;
}

