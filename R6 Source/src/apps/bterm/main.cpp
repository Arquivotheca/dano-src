
#include <stdio.h>

#include <Application.h>
#include <List.h>
#include <unistd.h>

#include "termwindow.h"

class bTermApp : public BApplication
{
	BList fWindows;
	virtual void MessageReceived(BMessage *msg);
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void RefsReceived(BMessage *msg);
	
public:
	bTermApp();
};

void 
bTermApp::RefsReceived(BMessage *msg)
{
	msg->PrintToStream();
}


void 
bTermApp::ArgvReceived(int32 argc, char **argv)
{
	bTermWindow *win = new bTermWindow(argc-1, argv+1);
	win->Show();
	fWindows.AddItem(win);
}


void 
bTermApp::MessageReceived(BMessage *msg)
{
	switch(msg->what){		
	case 'win+':{
		bTermWindow *win = new bTermWindow(0,NULL);
		win->Show();
		fWindows.AddItem(win);
		break;
	}
		
	case 'win-':{
		void *winptr;
		if(msg->FindPointer("window",&winptr) == B_OK){
			fWindows.RemoveItem(winptr);
			if(fWindows.CountItems() == 0) {
				PostMessage(B_QUIT_REQUESTED);
			}
		}
		break;
	}

	case 'win?':{
		bTermWindow *winptr;
		if(msg->FindPointer("window",(void**)&winptr) == B_OK){
			if(winptr) winptr->DumpHistory();
		}
		break;
	}
	
	case B_SILENT_RELAUNCH:{
		bTermWindow *win = new bTermWindow(0,NULL);
		win->Show();
		fWindows.AddItem(win);
		break;
	}
		
	}
}

bTermApp::bTermApp() 
 : BApplication("application/x-vnd.bjs-bterm")
{
}


int main(int argc, char *argv[])
{
	bTermApp app;
	if(argc == 1){
		// if there are no args, but we init'd the app, we need to post 
		// a message so that we construct the window
		app.PostMessage(B_SILENT_RELAUNCH);
	}	
	app.Run();
	return 0;	
}

