
#include <stdio.h>
#include <Window.h>
#include <Application.h>
#include <Path.h>
#include <FindDirectory.h>

#include "SoftKeyboard.h"

class TestWindow : public BWindow
{
public:
	SoftKeyboard * sk ;
	
	TestWindow()
		:BWindow(BRect(100, 100, 200, 200), "TestWindow", (window_look) 25,	// 25 has the yellow tab on the left
					B_FLOATING_ALL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_WILL_ACCEPT_FIRST_CLICK)
	{
		BPath configFilePath;
		if (B_OK != find_directory(B_BEOS_ETC_DIRECTORY, &configFilePath))
			return ;
		
		configFilePath.Append("softkeyboard");
		configFilePath.Append("tools-qwerty.kbd");
		
		sk = new SoftKeyboard(Bounds(), "SK", B_FOLLOW_LEFT | B_FOLLOW_TOP, configFilePath.Path(), NULL, NULL);
		AddChild(sk);
		ResizeTo(sk->Bounds().Width(), sk->Bounds().Height());
	}
	
	virtual bool QuitRequested()
	{
		sk->RemoveSelf();
		delete sk;
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
		
};

class TestApp : public BApplication
{
public:
	TestWindow * tw;
	
	TestApp()
		:BApplication("application/x-vnd.Be.SoftKeyboardApp")
	{
		tw = new TestWindow();
		tw->Show();
	}
	virtual bool QuitRequested()
	{
		tw->Lock();
		tw->Quit();
		return true;
	}
};


int
main(int argc, char ** argv)
{
	TestApp app;
	app.Run();
}
