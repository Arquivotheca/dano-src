
#include "Test.h"

#include <TextView.h>
#include <Button.h>
#include <Window.h>
#include <MessageRunner.h>
#include "ValidApp.h"
#include "settings.h"


Test * make_acceptance();

class AcceptanceWindow : public TestWindow {
public:
		AcceptanceWindow(bool ok) : TestWindow(BRect(100,100,400,320), ok ? "Results: Pass " : "Results: Fail", B_TITLED_WINDOW,
				B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE) {
			BRect r(Bounds());
			r.bottom -= 50;
			r.InsetBy(10,10);
			BTextView * tv = new BTextView(r, "text", r.OffsetToCopy(0,0), B_FOLLOW_ALL);
			tv->SetFontSize(15);
			if (ok) {
				tv->Insert("** Pass **\n\nAll tests completed successfully.");
			}
			else {
				tv->Insert("** Fail **\n\nNot all tests completed successfully. This unit needs further diagnostics before shipping.");
			}
			AddChild(tv);
			r.OffsetBy(0, r.bottom+10);
			r.bottom = r.top+25;
			r.left = r.right-90;
			r.right -= 10;
			BButton * b = new BButton(r, "ok", "OK", new BMessage('ok  '));
			AddChild(b);
			b->MakeDefault(true);
			AddShortcut('o', 0, new BMessage('ok  '));
			AddShortcut(13, 0, new BMessage('ok  '));
			m_ok = ok;
			bigtime_t timeout = get_setting_value("validate.oktimeout", 0);
			m_runner = 0;
			if (timeout > 0)
			{
				m_runner = new BMessageRunner(BMessenger(this), new BMessage('ok  '), timeout, 1);
			}
		}
		~AcceptanceWindow()
		{
		}
		BMessageRunner * m_runner;
		bool m_ok;
		void MessageReceived(BMessage * message) {
			if (message->what == 'ok  ') {
				this->TestDone(m_ok);
				// delete the runner now so there isn't a race condition in destructor
				delete m_runner;
			}
			else {
				TestWindow::MessageReceived(message);
			}
		}
};



Test * make_acceptance()
{
	AcceptanceWindow * aw = new AcceptanceWindow(!g_failed);
	return aw->GetTest();
}
