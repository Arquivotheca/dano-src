/****************************************************************
Author: Zeid Derhally
Copyright © 1998 Metrowerks, Inc
*****************************************************************/

#include "ProfilerApp.h"


int main()
{
	ProfilerApp* app = new ProfilerApp();

	app->Run();

	delete app;
	exit(0);
}
ProfilerApp::ProfilerApp():BApplication("application/x-mw-Profiler")
{
	fArgvCalled=FALSE;
}

/****   drag and drop stuff ***/
void
ProfilerApp::RefsReceived(BMessage* message)
{
	char name[B_FILE_NAME_LENGTH];

	entry_ref ref;
	message->FindRef("refs", &ref);
	BEntry entry(&ref,true);
	BPath path; 
	entry.GetPath(&path); 
	entry.GetName(name);
	MWWindow *AWindow=new MWWindow();
	AWindow->SetTitle(name);
	AWindow->ReadFile(path.Path());
	AWindow->Show();
	
}

/**** Command line stuff ***/

void
ProfilerApp::ArgvReceived(int32 argc, char **argv)
{
    BEntry    entry;
    BFile     file;
	BPath path; 
    status_t  result;
    char name[B_FILE_NAME_LENGTH];
    
    fArgvCalled = TRUE;
  
    if (argc != 2) /**  Make sure user uses the right syntax **/
    {   
      cout<<"USAGE: Profiler <filename>\n";
		be_app->PostMessage(B_QUIT_REQUESTED);
      return;
    }
    
    entry.SetTo(argv[1]);
    file.SetTo(&entry, O_RDONLY);
    
    /** Check if the file really exists **/
    if (file.InitCheck() != B_NO_ERROR) 
    {
	cout <<"ERROR: failed to open file "<< &argv[2]<<endl;
    be_app->PostMessage(B_QUIT_REQUESTED);
    return;
    }
	
	entry.GetPath(&path); 
	
	entry.GetName(name);
	cout <<path.Path()<<endl;
	MWWindow *AWindow=new MWWindow();
	AWindow->SetTitle(name);
	AWindow->ReadFile(path.Path());
	AWindow->Show();
	delete OpenDialog;
}

void
ProfilerApp::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
	case B_CANCEL :
		{
			delete OpenDialog;
			if ((be_app->CountWindows()) == 0)
			{
				printf("Profiler Quiting!\n");
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
	default:
		BApplication::MessageReceived(message);
	}
}
bool
ProfilerApp:: QuitRequested()
{
	return true;	
}
void
ProfilerApp::ReadyToRun()
{
	if (!fArgvCalled && (be_app->CountWindows()==0))
	{	
		OpenDialog= new BFilePanel(B_OPEN_PANEL,NULL, NULL,0,false,NULL,NULL,false,true);
		OpenDialog->Show();
	}
}