#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "wave_window.h"

const char *app_signature = "application/x-vnd.Be-SoundMixer";

//-----------------------------------------------------------

TSoundApplication::TSoundApplication()
		  :BApplication(app_signature)
{
	BRect			windowRect, viewRect;
	BPoint			wind_loc;
	int				ref;
	short			face;
	bool			secs;
	TWaveWindow		*ww;
	app_info		infos;
	
	
	GetAppInfo(&infos);

	windowRect.Set(30, 30, 30+865, 30+550);
	ww = new TWaveWindow(windowRect, "Benoit's Mix"); 
	ww->Lock();
	ww->Show();
	ww->Unlock();

} 


void TSoundApplication::MessageReceived(BMessage *msg)
{
	BApplication::MessageReceived(msg);
}

//-----------------------------------------------------------

int	main(int argc, char *argv[])
{
	TSoundApplication	*my_app;
	
	my_app = new TSoundApplication();
	my_app->Run();
	
	delete my_app;
	return 0;
}


//-----------------------------------------------------------

