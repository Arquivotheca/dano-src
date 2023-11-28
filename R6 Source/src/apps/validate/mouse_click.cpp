
#include "Test.h"

#include <Window.h>
#include <TextControl.h>
#include <TextView.h>
#include <OS.h>
#include <stdio.h>
#include <Alert.h>
#include "ValidApp.h"
#include <Button.h>

Test * make_mouse_click();

class MouseClickWindow : public TestWindow {
public:
	MouseClickWindow() : TestWindow(BRect(100,100,400,300), "Mouse click", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE) {
		BTextView * tv = new BTextView(BRect(0,0,300,100), "text", BRect(10,10,290,90), B_FOLLOW_ALL);
		tv->SetFontSize(15);
		AddChild(tv);
		char msg[100];
		sprintf(msg, "Click the 'Click me' button.");
		tv->SetText(msg);
		BButton * click = new BButton(BRect(210,110,290,135), "click_me", "Click Me", new BMessage('done'));
		AddChild(click);
		BButton * fail = new BButton(BRect(10,165,90,195), "fail", "Fail", new BMessage('fail'));
		AddChild(fail);
	}
	void MessageReceived(BMessage * msg) {
		switch (msg->what) {
		case 'fail':
			fail("Keyboard input cancelled by operator\n");
			this->TestDone(false);
			break;
		case 'done':
			this->TestDone(true);
			break;
		default:
			TestWindow::MessageReceived(msg);
			break;
		}
	}
};

Test * make_mouse_click()
{
	MouseClickWindow * kkw = new MouseClickWindow;
	return kkw->GetTest();
}

