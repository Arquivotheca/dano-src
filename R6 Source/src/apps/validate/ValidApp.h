
#if !defined(ValidApp_h)
#define ValidApp_h

#include <Application.h>
#include <stdio.h>
#include <Autolock.h>
#include <Locker.h>
#include <TextView.h>
#include <Window.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>

class Test;
extern bool g_valid;
extern bool g_failed;
extern int32 g_return_code;

#define ENTER_STAGE 'ents'
#define COMPLETED_STAGE 'comp'
#define STOP_TEST 'stop'
#define ALL_STAGES_DONE 'alld'

extern FILE * m_results;
extern void fail(const char * msg, ...);
extern void reset_failure();
extern void attempt(const char * msg, ...);
extern void info(const char * msg, ...);

struct OSInfo {
	BString name;
	BString version;
	BString date;
};

extern void SetLargeAlertFont(BWindow* aWindow);

class ValidApp : public BApplication {
public:
		ValidApp();
		virtual ~ValidApp();
		
		void SetUp();
		void AdoptVendorInformation(BMessage* msg);
		void MessageReceived(BMessage * msg);
		void ReadyToRun();
		void EnterStage(
				int32 stage);
		void CompletedStage();
		bool QuitRequested();
		void Pulse();

		static const char*	GetCurrentDirectory()	{ return s_current_directory.String(); }		
		static const char*	GetTestDirectory() 		{ return s_test_directory.String(); }
public:
		static OSInfo		s_os_info;
		static BString		s_machine_id;
		static BString		s_bios_version;
		static BString		s_bios_vendor;
		static BString		s_bios_date;
		static BString		s_mac_address;
		static char			s_start_time[32];
		static system_info	s_system_data;

		static BString		s_current_directory;
		static BString		s_test_directory;

private:
		void	EnableScreenSaver(bool Enable);

private:
		Test * m_curTest;
		int m_stage;
		BWindow * m_stages_window;
		BMessage * m_vendor_info;
};

class StatusWindow : public BWindow {
public:
static	void AddText(const char * fmt, va_list list) {
			BAutolock lock(s_lock);
			if (!s_window) {
				BRect r = BScreen().Frame();
				r.InsetBy(5, 5);
				r.top = r.bottom - 110;
				s_window = new StatusWindow(r);
				s_window->Show();
			}
			if (s_window->Lock()) {
				char buf[1300];
				vsnprintf(buf, 1299, fmt, list);
				s_window->m_text->Insert(buf);
				s_window->m_text->ScrollToSelection();
				s_window->m_text->Flush();
				s_window->Unlock();
			}
		}
static	void Close() {
			BAutolock lock(s_lock);
			if (s_window->Lock()) {
				s_window->Quit();
			}
			s_window = 0;
		}
private:
static	BLocker s_lock;
static	StatusWindow * s_window;
		StatusWindow(BRect area) : BWindow(area, "Test Status", B_TITLED_WINDOW,
				B_NOT_CLOSABLE | B_NOT_MOVABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE) {
			BRect r(Bounds());
			r.right -= 15;
			m_text = new BTextView(r, "status", r, B_FOLLOW_ALL);
			BScrollView * sv = new BScrollView("scroll", m_text, B_FOLLOW_ALL, 0, false, true);
			AddChild(sv);
			m_text->SetFontSize(15);
		}
		BTextView * m_text;
};


#endif	//	ValidApp_h

