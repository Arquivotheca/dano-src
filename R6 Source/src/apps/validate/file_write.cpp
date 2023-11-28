
#include "Test.h"

#include <Window.h>
#include <TextControl.h>
#include <TextView.h>
#include <OS.h>
#include <Alert.h>
#include <Debug.h>
#include "ValidApp.h"
#include <math.h>
#include <Button.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

Test * make_file_write();
static status_t file_thread(void *);

class FileWriteWindow : public TestWindow {
public:
	FileWriteWindow(bigtime_t pulseRate) : TestWindow(BRect(100,100,400,300), "File write", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE, pulseRate) {
		m_text = new BTextView(BRect(0,0,285,100), "text", BRect(10,10,290,90), B_FOLLOW_ALL);
		m_text->SetFontSize(15);
		BScrollView * sv = new BScrollView("scroll", m_text, B_FOLLOW_ALL, 0, false, true);
		AddChild(sv);
		BButton * skip = new BButton(BRect(210,110,290,135), "skip", "Skip", new BMessage('skip'));
		AddChild(skip);
		BButton * fail = new BButton(BRect(10,165,90,195), "fail", "Fail", new BMessage('fail'));
		AddChild(fail);
		AddShortcut('s', 0, new BMessage('skip'));
		AddShortcut('f', 0, new BMessage('fai2'));
		m_sem = create_sem(0, "Completion Sem");
		m_running = true;
		m_OK = true;
		m_thread = spawn_thread(file_thread, "file_thread", 10, this);
	}
	BTextView * m_text;
	thread_id m_thread;
	sem_id m_sem;
	bool m_running;
	bool m_OK;
	~FileWriteWindow() {
		m_running = false;
		status_t s;
		wait_for_thread(m_thread, &s);
	}
	
	bool QuitRequested() {
		// make sure the test is stopped when we abort
		m_running = false;
		return TestWindow::QuitRequested();
	}

	void MessageReceived(BMessage * msg) {
		switch (msg->what) {
		case TEST_PULSE_MSG:
			if (!m_running && !this->GetTest()->Completed()) {
				this->TestDone(m_OK);
			}
			break;
		case 'fai2':
			((BButton *)FindView("fail"))->SetValue(1);
		case 'fail':	//	fallthru
			m_running = false;
			fail("FileWrite failed by operator\n");
			this->TestDone(false);
			break;
		case 'skip':
			((BButton *)FindView("skip"))->SetValue(1);
			info("FileWrite test skipped by operator\n");
			m_OK = true;
		case 'done':	//	fallthru
			m_running = false;
			this->TestDone(m_OK);
			break;
		default:
			TestWindow::MessageReceived(msg);
			break;
		}
	}
};

struct file_write_test_info {
	char name[256];
	size_t size;
	void * data;
};

static file_write_test_info test_infos[20];

static status_t
file_thread(
	void * data)
{
	FileWriteWindow * frw = (FileWriteWindow *)data;
	size_t total = 1024*1024LL;
	size_t biggest = 0;
	void * buffer = 0;
	if (frw->Lock()) {
		frw->m_text->Insert("Generating...\n");
		frw->m_text->Flush();
		frw->Unlock();
	}
	//	suppose this test is used more than once
	for (int ix=0; ix<20; ix++) {
		test_infos[ix].size = 0;
		test_infos[ix].name[0] = 0;
		test_infos[ix].data = 0;
	}
	//	generate random data in /tmp
	for (int ix=0; ix<20; ix++) {

		size_t cur = (system_time()%(int)(floor(total*0.8)+1));
		test_infos[ix].size = cur;
		test_infos[ix].data = malloc(cur);
		if (!test_infos[ix].data) {
			fail("malloc(%ld): out of memory\n", cur);
			frw->m_OK = false;
			break;
		}
		if (!(rand() & 3)) {
			//	test the compressing case, too...
			memset(test_infos[ix].data, rand(), test_infos[ix].size);
		}
		else {
			//	this won't compress well...
			char * ptr = (char *)test_infos[ix].data;
			size_t cc = cur;
			srand((int)system_time());
			while (cc-- > 0) {
				*ptr++ = rand()>>7;
			}
		}
		if (cur > biggest) {
			void * n = realloc(buffer, cur+2);
			if (!n) {
				fail("realloc(%ld): out of memory\n", cur);
				frw->m_OK = false;
				break;
			}
			biggest = cur;
			buffer = n;
		}
		
		if( total < cur )
		{
			total = 0;
		}
		else
		{
			total -= cur;
		}

		sprintf(test_infos[ix].name, "/tmp/%03d-test-file-writing-data-%02d", (int)(system_time()%1000), ix);
		int fd = open(test_infos[ix].name, O_RDWR | O_CREAT | O_TRUNC, 0666);
		if (fd < 0) {
			fail("open(%s): create failed\n", test_infos[ix].name);
			frw->m_OK = false;
			break;
		}
		int w = write(fd, test_infos[ix].data, cur);
		if (w < 0) {
			fail("write(%s) failed: %s\n", test_infos[ix].name, strerror(errno));
			frw->m_OK = false;
		}
		else if ((size_t)w != cur) {
			fail("write(%s) returned %d, should be %d\n", test_infos[ix].name, w, cur);
			frw->m_OK = false;
		}
		close(fd);
	}
	if (frw->Lock()) {
		frw->m_text->Insert("Testing...\n");
		frw->m_text->Flush();
		frw->Unlock();
	}
	//	make sure the data stays there intact
	for (int ix=0; ix<20; ix++) {
		int fd = open(test_infos[ix].name, O_RDONLY);
		if (fd < 0) {
			fail("open(%s) failed: %s\n", test_infos[ix].name, strerror(errno));
			frw->m_OK = false;
		}
		else {
			int r = read(fd, buffer, test_infos[ix].size+2);
			if (r < 0) {
				fail("read(%s) failed: %s\n", test_infos[ix].name, strerror(errno));
				frw->m_OK = false;
			}
			else if ((uint32)r != test_infos[ix].size) {
				fail("read(%s) returns %d, should be %d\n", test_infos[ix].name, r,
						test_infos[ix].size);
				frw->m_OK = false;
			}
			else if (memcmp(buffer, test_infos[ix].data, test_infos[ix].size)) {
				fail("memcmp(%s) failed: file corruption!\n", test_infos[ix].name);
				frw->m_OK = false;
			}
			close(fd);
		}
	}
	if (frw->Lock()) {
		frw->m_text->Insert("Cleaning up...\n");
		frw->m_text->Flush();
		frw->Unlock();
	}
	free(buffer);
	for (int ix=0; ix<20; ix++) {
		free(test_infos[ix].data);
		test_infos[ix].data = 0;
		if (unlink(test_infos[ix].name) < 0) {
			fail("unlink(%s) failed: %s\n", test_infos[ix].name, strerror(errno));
			frw->m_OK = false;
		}
	}
	frw->m_running = false;
	return 0;
}

Test * make_file_write()
{
	FileWriteWindow * kkw = new FileWriteWindow(200000LL);
	status_t err;
	if ((err = resume_thread(kkw->m_thread)) < 0) {
		fail("FileWrite test failed: %s\n", strerror(err));
		kkw->m_OK = false;
		kkw->TestDone(false);
	}
	return kkw->GetTest();
}

