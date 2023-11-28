
#include "App.h"
#include "Wind.h"


TApp *theApp;

int main()
{	
	TApp app;
	theApp = &app;
	app.Run();
	
	return B_NO_ERROR;
}


TApp::TApp()
	:BApplication("application/x-vnd.Be-OPENGL")
{
	win = new MainWindow();
}

TApp::~TApp()
{
}

void TApp::AboutRequested()
{
}

void TApp::MessageReceived(BMessage* msg)
{
	BApplication::MessageReceived(msg);
}

void TApp::ReadyToRun()
{
	win->Show();
}

