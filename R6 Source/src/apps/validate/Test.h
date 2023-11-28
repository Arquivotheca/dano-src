
#if !defined(Test_h)
#define Test_h

#include <OS.h>
#include <Window.h>
class BMessageRunner;

//	If you use a pulse rate, this message will be sent that often
#define TEST_PULSE_MSG 'puls'

// ---------------------------------------------------------------------------
//	class Test
//	Each test function must return a Test* that is used for Start()
// ---------------------------------------------------------------------------

class TestWindow;

class Test {
public:
		//	the test driver uses these functions
		virtual ~Test();
		virtual void Start();	//	will show the window; override if necessary
		//	each test may override these functions
		virtual bool Completed();
		virtual bool Successful();

		TestWindow * Window();

private:
		//	each test uses these functions (but through the TestWindow)
		Test(TestWindow * window, bigtime_t pulseRate = B_INFINITE_TIMEOUT);
		void TestDone(bool successful);				//	call TestDone() when you're done and have a status
		void TestAbort();							//	call TestAbort() to abort the test and all subsequent tests
		friend class TestWindow;
		
		TestWindow * m_window;
		BMessageRunner * m_runner;
		bigtime_t m_pulseRate;
		bool m_completed;
		bool m_successful;
		Test();
};

// ---------------------------------------------------------------------------
//	class TestWindow
//	All test windows must derive from TestWindow!!
// ---------------------------------------------------------------------------

class TestWindow : public BWindow {
public:
					TestWindow(BRect frame, 
							   const char *title, 
							   window_type type, 
							   uint32 flags, 
							   bigtime_t pulseRate = B_INFINITE_TIMEOUT);
	virtual			~TestWindow();
	
	void			TestDone(bool successful)			{ fTest->TestDone(successful); }

	Test*			GetTest()							{ return fTest; }
	void			SetPulse(bigtime_t puseRate);

	virtual bool	QuitRequested();
	
private:
	Test*			fTest;
};

#endif	//	Test_h
