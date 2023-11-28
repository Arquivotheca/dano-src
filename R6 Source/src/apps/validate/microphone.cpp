
#include "Test.h"

#include <Window.h>
#include <TextControl.h>
#include <TextView.h>
#include <OS.h>
#include <stdio.h>
#include <Alert.h>
#include "ValidApp.h"
#include <Button.h>

#include "microphone.h"

Test * make_microphone();

class MicrophoneWindow : public TestWindow {
	MicrophoneTest m_mic;
public:
	MicrophoneWindow() : TestWindow(BRect(100,100,400,300), "Microphone Test", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE) {
		BTextView * tv = new BTextView(BRect(0,0,300,100), "text", BRect(10,10,290,90), B_FOLLOW_ALL);
		tv->SetFontSize(18);
		AddChild(tv);
		char msg[100];
		sprintf(msg, "Testing Microphone.");
		tv->SetText(msg);
		BButton * click = new BButton(BRect(210,110,290,135), "pass", "Pass", new BMessage('done'));
		AddChild(click);
		BButton * fail = new BButton(BRect(10,165,90,195), "fail", "Fail", new BMessage('fail'));
		AddChild(fail);
		SetDefaultButton(click);

		m_mic.Begin(new BMessenger(this), new BMessage('done'));
	}
	void MessageReceived(BMessage * msg) {
		switch (msg->what) {
		case 'fail':
			fail("Microphone cancelled by operator\n");
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

Test * make_microphone()
{
	MicrophoneWindow * kkw = new MicrophoneWindow;
	return kkw->GetTest();
}

