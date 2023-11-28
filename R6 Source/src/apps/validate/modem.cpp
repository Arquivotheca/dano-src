
#include "Test.h"

#include <Window.h> 
#include <TextControl.h> 
#include <TextView.h> 
#include <OS.h> 
#include <stdio.h> 
#include <Alert.h> 
#include "ValidApp.h"
#include "settings.h"
#include <Button.h> 
#include <stdarg.h> 
#include <unistd.h> 
#include <sys/fcntl.h> 
#include <errno.h> 
#include <signal.h> 
#include <termios.h> 
#include <string> 

Test* make_modem();

class ModemWindow : public TestWindow
{
		char m_phoneNumber[100];
	public:
	ModemWindow() : TestWindow(BRect(100, 100, 400, 350), "Modem connect", B_TITLED_WINDOW,
		        B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE)
		{
			BTextView * tv = new BTextView(BRect(0, 0, 300, 200), "text", BRect(10, 10, 290, 90), B_FOLLOW_ALL);
			tv->SetFontSize(15);
			AddChild(tv);
			char msg[100];
			BString file = ValidApp::s_current_directory;
			file += "/phone.txt";
			int pnf = open(file.String(), O_RDONLY);
			
			if (pnf >= 0)
			{
				int bytesRead = read(pnf, m_phoneNumber, sizeof(m_phoneNumber) - 1);
				close(pnf);
				
				if (bytesRead < 0)
				{
					goto hardcode;
				}
				
				m_phoneNumber[bytesRead] = 0;
				while (bytesRead > 0 && m_phoneNumber[bytesRead - 1] == '\n')
				{
					m_phoneNumber[--bytesRead] = 0;
				}
			}
			else
			{
hardcode:
				//			fprintf(stderr, "using hard-coded number; fix in test/phone.txt\n");
				strcpy(m_phoneNumber, "7672676");
				get_setting("modem.number", m_phoneNumber, sizeof(m_phoneNumber));
			}
			sprintf(msg, "Please wait; dialing %s.\n", m_phoneNumber);
			tv->SetText(msg);
			tv->SetFont(be_fixed_font);
			tv->SetFontSize(12);
			m_failCount = 0;
			BButton * fail = new BButton(BRect(10, 202, 90, 230), "fail", "Fail", new BMessage('fail'));
			AddChild(fail);
			m_thread = -1;
			m_listenLock = create_sem(1, "listen_lock");
			m_listenSize = 0;
		}

		~ModemWindow()
		{
			delete_sem(m_listenLock);
			m_listenLock = -1;
		}

		//	this is skanky and doesn't clean up 100% properly...
		thread_id m_thread;
		thread_id m_listener;
		static char m_listenBuf[512];
		static int m_listenSize;
		static sem_id m_listenLock;
		static int fd;
		bigtime_t m_startTime;
		static volatile bool m_quitting;
		
		void StartTest()
		{
			if (m_thread < 0)
			{
				m_quitting = false;
				m_thread = spawn_thread(dial_thread, "dial_thread", 10, this);
				resume_thread(m_thread);
				m_startTime = system_time();
			}
		}
		
		void StopTest()
		{
			m_quitting = true;
			while (m_thread > 0)
			{
				fprintf(stderr, "signalling threads to quit\n");
				send_signal(m_thread, 1);
				send_signal(m_listener, 1);
				snooze(500000);
			}
		}
		
		void Show()
		{
			TestWindow::Show();
			StartTest();
		}

		bool QuitRequested()
		{
			// make sure the test is stopped when we abort
			StopTest();
			return TestWindow::QuitRequested();
		}

		static status_t waitfor (void * win, const char * what)
		{
			//	45 seconds for any one wait
			fprintf(stderr, "waitfor(%s)\n", what);
			bigtime_t timeout = system_time() + 45000000;
			int len = strlen(what);
			bool ok = false;
			while (system_time() < timeout)
			{
				if (m_quitting) return EINTR;
				if (acquire_sem_etc(m_listenLock, 1, B_TIMEOUT, 2000000) < 0)
				{
					fprintf(stderr, "acquire_sem_etc(m_listenLock) failed\n");
					return B_ERROR;
				}
				if (m_listenSize >= len)
				{
					for (int ix = 0; ix <= m_listenSize - len; ix++)
					{
						if (!strncasecmp(&m_listenBuf[ix], what, len))
						{
							ok = true;
							memmove(m_listenBuf, &m_listenBuf[(ix + len)], m_listenSize - (ix + len));
							m_listenSize -= (ix + len);
							break;
						}
					}
				}
				release_sem_etc(m_listenLock, 1, B_DO_NOT_RESCHEDULE);
				if (ok) return B_OK;
				if (snooze(500000) < 0)
				{
					fprintf(stderr, "snooze() interrupted by signal\n");
					return B_ERROR;
				}
			}
			action(win, "waitfor(%s) failed", what);
			return B_TIMED_OUT;
		}
		
		static void action(void * win, const char * txt, ...)
		{
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
		
		static void sig_foo(int)
		{
			m_quitting = true;
		}
		
		static status_t listen_thread(void * win)
		{
			signal(1, sig_foo);
			char ch;
			ModemWindow * mw = (ModemWindow *)win;

			while (true)
			{
				int nread = read(mw->fd, &ch, 1);

				if ((nread < 0) && (errno != EAGAIN))
				{
					break;
				}
				if (nread == 0)
				{
					//
					// timed out
					//
					continue;
				}

				fprintf(stderr, "%c", ch);
				if (acquire_sem_etc(mw->m_listenLock, 1, B_TIMEOUT, 2000000) < 0)
				{
					break;
				}
				if (m_listenSize > 500)
				{
					memmove(m_listenBuf, &m_listenBuf[300], m_listenSize - 300);
					m_listenSize -= 300;
				}
				m_listenBuf[m_listenSize++] = ch;
				release_sem_etc(mw->m_listenLock, 1, B_DO_NOT_RESCHEDULE);
			}
			mw->m_listener = -1;
			return 0;
		}

#define OPEN_ITERS 10
		static status_t dial_thread(void * win)
		{
			int openIters;
			signal(1, sig_foo);
			ModemWindow * mw = (ModemWindow *)win;
			status_t err = B_OK;
			openIters = OPEN_ITERS;
			while (openIters > 0)
			{
				char devname[80];
				strcpy(devname, "/dev/ports/lucent");
				get_setting("modem.device", devname, sizeof(devname));
				action(win, "opening %s", devname);
				fd = open(devname, O_RDWR | O_NONBLOCK | O_EXCL);
				if (fd >= 0)
				{
					snooze(4000000);
					break;
				}
				err = errno;
				if (openIters == OPEN_ITERS)
					action(win, "open modem: %s", strerror(err));
				if (--openIters)
					snooze(4000000);
			}
			
			fprintf(stderr, "open returns file descriptor %d\n", fd);
			if (fd < 0)
			{
				goto done_1;
			}
			
			fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);
			action(win, "setting line attributes");
			termios attrs;
			err = tcgetattr(fd, &attrs);
			if (err < 0) fprintf(stderr, "tcgetattr(): %s\n", strerror(errno));

			attrs.c_iflag &= ~(IXON | IXOFF | IXANY | INPCK);
			attrs.c_lflag &= ~(ICANON | ECHO | ECHONL | ISIG);
			attrs.c_cflag &= ~(CBAUD | CSTOPB | CSIZE | PARENB | PARODD | CRTSCTS);
			attrs.c_cflag |= B57600 | CLOCAL;
			attrs.c_cflag |= CS8;
			attrs.c_cc[VTIME] = 10;
			attrs.c_cc[VMIN] = 0; //1;
			err = tcsetattr(fd, TCSANOW, &attrs);
			if (err < 0) fprintf(stderr, "tcsetattr(): %s\n", strerror(errno));
			snooze(1000000);
			mw->m_listener = spawn_thread(listen_thread, "listen_thread", 10, mw);
			resume_thread(mw->m_listener);
			action(win, "Sending ATZ");
			if (write(fd, "ATZ\r\n", 4) < 4)
			{
				fprintf(stderr, "write() returns < 4\n");
				err = errno;
				if (err >= 0) err = B_ERROR;
				goto done_1;
			}
			if ((err = waitfor (win, "ATZ")) < 0)
			{
				goto done_1;
			}
			snooze(100000);
			{
				const char * init = get_setting("modem.init");
				if (init != NULL)
				{
					action(win, "Sending init string");
					if ((write(fd, init, strlen(init)) < (int)strlen(init)) ||
					        (write(fd, "\r\n", 2) < 2))
					{
						fprintf(stderr, "write() returns < init string\n");
						err = errno;
						if (err >= 0) err = B_ERROR;
						goto done_1;
					}
					if ((err = waitfor (win, "OK")) < 0)
					{
						goto done_1;
					}
				}
			}
			char atdt[200];
			sprintf(atdt, "ATDT %s\r\n", mw->m_phoneNumber);
			action(win, "Sending phone number %s", atdt);
			if (write(fd, atdt, strlen(atdt)) < (int)strlen(atdt))
			{
				err = errno;
				if (err >= 0) err = B_ERROR;
				goto done_1;
			}
			openIters = strlen(atdt);
			atdt[openIters - 2] = 0; // strip \r\n
			if (openIters > 5) openIters -= 5;
			else openIters = 0;
			if ((err = waitfor (win, &atdt[openIters])) < 0)
			{
				goto done_1;
			}
			action(win, "Waiting for CONNECT");
			if ((err = waitfor (win, "CONNECT")) < 0)
			{
				goto done_1;
			}
			snooze(500000);
			openIters = 0;
			sscanf(mw->m_listenBuf, "%d", &openIters);
			action(win, "speed: %d", openIters);
			err = B_OK;
			action(win, "Hanging up");
			snooze(1500000);
			
			action(win, "+++");
			if (write(fd, "+++", 3) < 3)
			{
				err = errno;
				if (err >= 0) err = B_ERROR;
				goto done_1;
			}
			
			if ((err = waitfor (win, "OK")) < 0)
			{
				goto done_1;
			}
			
			action(win, "ATH");
			if (write(fd, "ATH\r\n", 5) < 5)
			{
				err = errno;
				if (err >= 0) err = B_ERROR;
				goto done_1;
			}
			
			if ((err = waitfor (win, "OK")) < 0)
			{
				goto done_1;
			}
done_1:
			action(win, "closing modem fd %d", fd);
			fprintf(stderr, "closing modem\n");
			close(fd);
			fd = -1;
			mw->m_thread = -1;
			action(win, "Test result %s", strerror(err));
			
			// why sleep here?
			snooze((err < 0) ? 6000000 : 1000000);
			if (!m_quitting)
			{
				BMessage msg('done');
				msg.AddInt32("error", err);
				((BWindow *)win)->PostMessage(&msg);
			}
			
			status_t s;
			fprintf(stderr, "waiting for mw->m_listener\n");
			wait_for_thread(mw->m_listener, &s);
			fprintf(stderr, "mw->m_listener returned\n");
			return err;
		}
		
		void MessageReceived(BMessage * msg)
		{
			int32 error = B_ERROR;
			const char * text;
			switch (msg->what)
			{
				case 'fail':
					fail("dialing cancelled by operator\n");
					StopTest();
					this->TestDone(false);
					break;
					
				case 'done':
					if (msg->FindInt32("error", &error) || error)
					{
						m_failCount++;
						char str[100];
						sprintf(str, "Modem dialing failed: %s\n", strerror(error));
						BAlert* alert = new BAlert("", str, m_failCount < 3 ? "Again" : "Fail");
						SetLargeAlertFont(alert);
						alert->Go();
						if (m_failCount >= 3)
						{
							fail(str);
							this->TestDone(false);
						}
						else
						{
							StartTest();
						}
					}
					else
					{
						this->TestDone(true);
					}
					break;
					
				case 'text':
					if (!msg->FindString("text", &text))
					{
						BTextView * tv = static_cast<BTextView*>(FindView("text"));
						tv->Insert(tv->TextLength(), text, strlen(text));
						int line = tv->LineAt(tv->TextLength());
						float h = tv->TextHeight(0, line);
						if (h > tv->Bounds().Height())
							tv->ScrollTo(BPoint(0, h - tv->Bounds().Height()));
						tv->Draw(tv->Bounds());
						tv->Flush();
					}
					break;
					
				default:
					TestWindow::MessageReceived(msg);
					break;
			}
		}
		
		int m_failCount;
};

char ModemWindow::m_listenBuf[512];
int ModemWindow::m_listenSize;
sem_id ModemWindow::m_listenLock;
int ModemWindow::fd;
volatile bool ModemWindow::m_quitting;

Test* make_modem()
{
	ModemWindow* kkw = new ModemWindow;
	return kkw->GetTest();
}

