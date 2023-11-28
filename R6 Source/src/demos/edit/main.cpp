#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include <Screen.h>
#include <OS.h>
#include <Directory.h>
//-----------------------------------------------------------

const char *app_signature = "application/x-vnd.Be.medit";

//-----------------------------------------------------------


TEditApplication::~TEditApplication(void)
{
}

//-----------------------------------------------------------

void
TEditApplication::RefsReceived(
	BMessage	*inMessage)
{
	int32		count = 0;
	uint32		type = 0;
	entry_ref	ref;

	inMessage->GetInfo("refs", &type, &count);
	for (long i = 0; i < count; i++) {
		if (inMessage->FindRef("refs", i, &ref) == B_NO_ERROR) {
			//ww->TakeRef(ref);
		}
	}
}


//-----------------------------------------------------------

void
TEditApplication::ArgvReceived(
	int32	argc,
	char	**argv)
{
	BMessage	*message = CurrentMessage();
	const char	*cwd = NULL;

	if (message->FindString("cwd", &cwd) == B_NO_ERROR) {
		BDirectory dir(cwd);
		for (int32 i = 1; i < argc; i++) {
			BEntry entry;
			if (entry.SetTo(&dir, argv[i]) == B_NO_ERROR) {
				entry_ref ref;
				//if (entry.GetRef(&ref) == B_NO_ERROR) 
					//ww->TakeRef(ref);
			}
		}
	}
}

//-----------------------------------------------------------

TEditApplication::TEditApplication()
		  :BApplication(app_signature)
{
	BRect			windowRect, viewRect;
	BPoint			wind_loc;
	int				ref;
	short			face;
	bool			secs;
	app_info		infos;
	
	
	GetAppInfo(&infos);

	windowRect.Set(30, 30, 30+700, 30+400);
	ww = new TEditWindow(windowRect, "img"); 
	ww->Lock();
	ww->Show();
	ww->Unlock();
} 


//-----------------------------------------------------------

void TEditApplication::MessageReceived(BMessage *msg)
{
	BApplication::MessageReceived(msg);
}

//-----------------------------------------------------------

int	main(int argc, char *argv[])
{
	TEditApplication	*my_app;
	BScreen				s;	
	
	
	my_app = new TEditApplication();
	my_app->Run();
	
	delete my_app;
	return 0;
}


//-----------------------------------------------------------

