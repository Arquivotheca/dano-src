#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "fftshift.h"
#include <Roster.h>
#include <Screen.h>
#include <OS.h>

//-----------------------------------------------------------

const char *app_signature = "application/x-vnd.Be.3DSoundMixer";

//-----------------------------------------------------------

char	slow_anim = 0;

//-----------------------------------------------------------

TSoundApplication::~TSoundApplication(void)
{
}

//-----------------------------------------------------------

void
TSoundApplication::RefsReceived(
	BMessage	*inMessage)
{
	int32		count = 0;
	uint32		type = 0;
	entry_ref	ref;

	inMessage->GetInfo("refs", &type, &count);
	for (long i = 0; i < count; i++) {
		if (inMessage->FindRef("refs", i, &ref) == B_NO_ERROR) {
			ww->TakeRef(ref);
		}
	}
}


//-----------------------------------------------------------

void
TSoundApplication::ArgvReceived(
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
				if (entry.GetRef(&ref) == B_NO_ERROR) 
					ww->TakeRef(ref);
			}
		}
	}
}

//-----------------------------------------------------------

TSoundApplication::TSoundApplication()
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
	ww = new TWaveWindow(windowRect, "3DmiX"); 
	ww->Lock();
	ww->Show();
	ww->Unlock();
	snooze(280000);				//do good stuff here instead !
	ww->Lock();
	ww->UpdateIfNeeded();
	ww->Switch_H();
	ww->Unlock();
} 


//-----------------------------------------------------------

void TSoundApplication::MessageReceived(BMessage *msg)
{
	BApplication::MessageReceived(msg);
}

//-----------------------------------------------------------

int	main(int argc, char *argv[])
{
	TSoundApplication	*my_app;
	BScreen				s;	
	
	
	my_app = new TSoundApplication();
	my_app->Run();
	
	delete my_app;
	return 0;
}


//-----------------------------------------------------------

