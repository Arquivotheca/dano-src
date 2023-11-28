
//	The vendor tests live in add-ons with the names
//	"vendor1" "vendor2" and "vendor3" (typically in the
//	add-ons folder of the validate program).
//	They export a hook named "run_test" with the following
//	prototype:
//	extern "C" status_t run_test(BMessenger &, ValidateInterface *);
//	Typically, you pass/fail the test by returning 0 (pass) or <0 (fail).
//	You can log text to the window (and stderr) by calling the
//	supplied action function (printf-style).

#include "Test.h"
#include "ValidApp.h"

#include <Screen.h>
#include <Window.h>
#include <TextControl.h>
#include <TextView.h>
#include <OS.h>
#include <stdio.h>
#include <Alert.h>
#include "ValidApp.h"
#include <math.h>
#include <Button.h>
#include "settings.h"
#include <string>
//#include <image.h>
#include <Messenger.h>
#include  "ValidateInterface.h"


Test * make_vendor1();
Test * make_vendor2();
Test * make_vendor3();

const uint32 msgTestPassed = 'pass';
const uint32 msgTestFailed = 'fail';

class VendorTestWindow : public TestWindow {
public:
	VendorTestWindow(const BRect & r, const char * name, BView* (*func)(const BRect&, ValidateInterface *)) :
			TestWindow(r, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE) {

		ValidateInterface vi = {
			sizeof(vi),
			&fail,
			get_setting,
			get_setting_value,
			&ValidApp::GetCurrentDirectory,
			&ValidApp::GetTestDirectory
		};
		
		BView* vendorView = (*func)(r, &vi);
		this->AddChild(vendorView);
	}
	~VendorTestWindow() {
	}

	void MessageReceived(BMessage * msg) {
		switch (msg->what) {
		case msgTestFailed:
			this->TestDone(false);
			break;
		case msgTestPassed:
			this->TestDone(true);
			break;
		default:
			TestWindow::MessageReceived(msg);
			break;
		}
	}
};


Test * make_alternate_test(const char * name, BView* (*func)(const BRect&, ValidateInterface *))
{
	BRect r = BScreen().Frame();
	r.left += 50.;
	r.right -= 50.;
	r.top += 50.;
	r.bottom -= 400.;
	VendorTestWindow * kkw = new VendorTestWindow(r, name, func);
	return kkw->GetTest();
}


class VendorScriptWindow : public TestWindow {
public:
	VendorScriptWindow(const char* scriptName) : TestWindow(BRect(100,100,500,400), "Vendor Script", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE), fScriptName(scriptName) {
		BTextView* tv = new BTextView(BRect(0,0,400,200), "text", BRect(10,10,390,190), B_FOLLOW_ALL);
		AddChild(tv);
		tv->SetFontSize(18);
		char msg[100];
		sprintf(msg, "Running vendor script %s.\n", fScriptName.String());
		tv->SetText(msg);
		BButton* fail = new BButton(BRect(10,265,90,295), "fail", "Fail", new BMessage(msgTestFailed));
		AddChild(fail);

		resume_thread(spawn_thread(ScriptRunner, "vendor_script", 10, this));

	}
	void MessageReceived(BMessage* msg) {
		switch (msg->what) {
			case 'text':
			{
				const char * text;
				if (!msg->FindString("text", &text)) {
					BTextView* tv = static_cast<BTextView *>(FindView("text"));
					tv->Insert(tv->TextLength(), text, strlen(text));
					int line = tv->LineAt(tv->TextLength());
					float h = tv->TextHeight(0, line);
					if (h > tv->Bounds().Height())
						tv->ScrollTo(BPoint(0, h-tv->Bounds().Height()));
					tv->Draw(tv->Bounds());
					tv->Flush();
				}
				break;
			}
			case msgTestFailed:
				fail("Vendor script cancelled by operator\n");
				this->TestDone(false);
				break;
			case msgTestPassed:
				this->TestDone(true);
				break;
			default:
				TestWindow::MessageReceived(msg);
				break;
		}
	}
	
	BString fScriptName;

	static void action(void* win, const char * txt, ...) {
		char str[600];
		va_list vl;
		va_start(vl, txt);
		vsprintf(str, txt, vl);
		va_end(vl);
		fprintf(stderr, "action: %s\n", str);
		BMessage msg('text');
		string s(str);
		s += "\n";
		msg.AddString("text", s.c_str());
		((BWindow *)win)->PostMessage(&msg);
	}

	static status_t ScriptRunner(void* arg)
	{
		VendorScriptWindow* window = (VendorScriptWindow *)arg;
		const char* argv[2] = { window->fScriptName.String(), NULL };
		thread_id thread = load_image(1, argv, const_cast<const char **>(environ));
		if (thread < 0) {
			action(window, "%s cannot be run: %s\n", window->fScriptName.String(), strerror(thread));
			window->PostMessage(msgTestFailed);
			return -1;
		}
		action(window, "Thread %ld created to run %s\n", thread, window->fScriptName.String());
		
		// run the script and wait for it, and then report result
		resume_thread(thread);
		status_t status = B_ERROR;
		if ((wait_for_thread(thread, &status) != B_OK)) {
			action(window, "Thread unable to run");
			window->PostMessage(msgTestFailed);
			status = -1;
		}
		else if (status == 0) {
			action(window, "%s returns pass\n", window->fScriptName.String());
			window->PostMessage(msgTestPassed);
			status = B_OK;
		}
		else {
			action(window, "%s returns failure (%ld)\n", window->fScriptName.String(), status);
			window->PostMessage(msgTestFailed);
			status = -1;
		}
		
		return status;
	}
};

Test * make_script_test(const char* scriptName)
{
	VendorScriptWindow* scriptWindow = new VendorScriptWindow(scriptName);
	return scriptWindow->GetTest();
}
