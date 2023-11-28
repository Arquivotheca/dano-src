#include "Test.h"
#include "ValidApp.h"

#include <Window.h>
#include <MessageRunner.h>
#include <TextView.h>
#include <View.h>


Test::~Test()
{
	delete m_runner;
	if (m_window->Lock()) m_window->Quit();
	m_window = 0;
}

static void
recurse(
	BView * v)
{
	// I guess the monitors sit a long way away from the factory technicians
	// Sony wanted 18 point fonts.
	v->SetFontSize(18.0);
	BTextView * tv = dynamic_cast<BTextView *>(v);
	if (tv != 0) {
		BFont f(be_plain_font);
		f.SetSize(18.0);
		tv->SetFontAndColor(&f);
	}
	for (int ix=0; ix<v->CountChildren(); ix++) {
		recurse(v->ChildAt(ix));
	}
}

void 
Test::Start()
{
	if (m_window != 0 && m_pulseRate != B_INFINITE_TIMEOUT && m_runner == 0) {
		m_runner = new BMessageRunner(BMessenger(m_window), new BMessage(TEST_PULSE_MSG),
				m_pulseRate);
	}
	if (m_window->Lock()) {
		m_window->Show();
		m_window->BeginViewTransaction();
		for (int ix=0; ix<m_window->CountChildren(); ix++) {
			recurse(m_window->ChildAt(ix));
		}
		m_window->EndViewTransaction();
		m_window->Unlock();
	}
}


Test::Test(TestWindow *window, bigtime_t pulseRate)
{
	m_runner = 0;
	m_window = window;
	m_pulseRate = pulseRate;
	m_completed = false;
	m_successful = false;
}

void 
Test::TestDone(bool successful)
{
	delete m_runner;
	m_runner = 0;
	m_successful = successful;
	m_completed = true;
	be_app->PostMessage(COMPLETED_STAGE);
}

void 
Test::TestAbort()
{
	delete m_runner;
	m_runner = 0;
	m_successful = false;
	m_completed = true;
	be_app->PostMessage(STOP_TEST);
}

TestWindow *
Test::Window()
{
	return m_window;
}

bool 
Test::Completed()
{
	return m_completed;
}

bool 
Test::Successful()
{
	return m_successful;
}

// ---------------------------------------------------------------------------
//	TestWindow - methods
// ---------------------------------------------------------------------------


TestWindow::TestWindow(BRect frame, 
					   const char *title, 
					   window_type type, 
					   uint32 flags,
					   bigtime_t pulseRate)
		   : BWindow(frame, title, type, flags)
{
	fTest = new Test(this, pulseRate);
}

// ---------------------------------------------------------------------------


TestWindow::~TestWindow()
{
	// fTest is adopted by test framework and deleted there
}

// ---------------------------------------------------------------------------

bool
TestWindow::QuitRequested()
{
	fTest->TestAbort();
	return true;

}
