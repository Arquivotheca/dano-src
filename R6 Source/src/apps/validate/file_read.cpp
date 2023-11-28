
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#include <string>
#include <map>

#include <Alert.h>
#include <Button.h>
#include <Debug.h>
#include <File.h>
#include <OS.h>
#include <String.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>

#include "Test.h"
#include "ValidApp.h"
#include "md5.h"

//#define EXCLUDES_FILE_NAME	"checksum-exclusions.txt"

Test * make_file_read();
static status_t file_thread(void *);

class FileReadWindow : public TestWindow {
public:
	FileReadWindow(bigtime_t pulseRate) : TestWindow(BRect(100,100,600,300), "File read", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE, pulseRate) {
		m_text = new BTextView(BRect(0,0,485,100), "text", BRect(10,10,475,90), B_FOLLOW_ALL);
		m_text->SetFontSize(15);
		BScrollView *sv = new BScrollView("scroll", m_text, B_FOLLOW_ALL, 0, false, true);
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
	~FileReadWindow() {
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
			fail("FileRead failed by operator\n");
			this->TestDone(false);
			break;
		case 'skip':
			((BButton *)FindView("skip"))->SetValue(0);
			info("FileRead test skipped by operator\n");
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

struct mdsum {
	union {
		unsigned char data[16];
		unsigned long values[4];
	};
	bool operator==(const mdsum & other) {
		return !memcmp(data, other.data, 16);
	}
	bool operator!=(const mdsum & other) {
		return memcmp(data, other.data, 16);
	}
	mdsum & operator=(const mdsum & other) {
		values[0] = other.values[0];
		values[1] = other.values[1];
		values[2] = other.values[2];
		values[3] = other.values[3];
		return *this;
	}
	mdsum(const mdsum & other) {
		values[0] = other.values[0];
		values[1] = other.values[1];
		values[2] = other.values[2];
		values[3] = other.values[3];
	}
	mdsum() {
	}
};

static std::map<string, mdsum> gManifest;

// The BString is an exclusion file name, I am really only
// using the map for finding the thing, the char is not used.
// If the Netron build had the BVector stuff, I would be using
// BOrderedVector instead.
static std::map<BString, char> gExclusions;

static void
read_manifest(
	FileReadWindow * win)
{
	// the manifest always comes from original test directory 
	// no matter where we run the test
	BString manifestName = ValidApp::s_test_directory;
	manifestName += "/MANIFEST";
	FILE * f = fopen(manifestName.String(), "r");
	if (f == NULL) {
		fail("MANIFEST is missing\n");
		win->m_OK = false;
		return;
	}
	char line[1024];
	while (1) {
		line[0] = 0;
		fgets(line, 1024, f);
		if (!line[0]) {
			break;
		}
		char * p = strchr(line, '\n');
		if (p) *p = 0;
		p = strchr(line, '\t');
		if (p == NULL) {
			fail("MANIFEST has bad data ('%s')\n", line);
			win->m_OK = false;
			break;
		}
		mdsum ms;
		if (4 != sscanf(line, "%lx:%lx:%lx:%lx", &ms.values[0], &ms.values[1],
				&ms.values[2], &ms.values[3])) {
			fail("MANIFEST has bad data ('%s')\n", line);
			win->m_OK = false;
			break;
		}
		while (*p && isspace(*p)) p++;
		if (!*p) {
			fail("MANIFEST has bad data ('%s')\n", line);
			win->m_OK = false;
			break;
		}
		gManifest.insert(pair<const string, mdsum>(p, ms));
	}
	fclose(f);
}

static void
read_excludes()
{
	// The exclusion list always comes from original test directory 
	// no matter where we run the test.
	
	// If we don't find an exclusion list, it's ok.
	BString excludesName = ValidApp::s_test_directory;
	excludesName << "/" << EXCLUDES_FILE_NAME;
	
	BFile efile(excludesName.String(), B_READ_ONLY);
	
	if( B_OK == efile.InitCheck() )
	{
		off_t size;
		efile.GetSize(&size);
		
		BString filedata;
		char* buff = filedata.LockBuffer(size + 1);
		
		efile.ReadAt(0, buff, size);
		buff[size] = '\0';

		BString name;
		size += 1;
		for( uint32 i = 0; i < size; i++ )
		{
			const char c = buff[i];
			if( ('\n' == c) || ('\0' == c) )
			{
				if( name.Length() != 0 )
				{
					// create entry.
					gExclusions[name] = true;
					
					// clear name
					name.Truncate(0);
				}
			}
			else
			{
				// build name
				name << c;
			}
		}
	
		DEBUG_ONLY(
			// print out name list
			printf("Exclusions list:\n");
			for( std::map<BString, char>::iterator i = gExclusions.begin();
	             gExclusions.end() != i;
	             i++
	           )
	        	{
	        		printf("%s\n", i->first.String());	
	        	}
		);
	}
}

/* exclusion list in place already
&& strcmp(end, "valid-results.txt") &&
   strcmp(end, "swap") &&
   strcmp(end, "eeprom.dat")
				*/
// Note: this does not support full paths, only leaf file names.
static bool
exclude_file(const char* file_name)
{
	bool result = false;
	 
	if( gExclusions.find(BString(file_name)) != gExclusions.end() )
	{
		result = true;
	}
	
	return result;
}

static void
recurse_dir(
	char * base,
	char * end,
	FileReadWindow * frw)
{
	if (!frw->m_running) return;
	ASSERT(*end == 0);
	ASSERT(end[-1] == '/');
	DIR * d = opendir(base);
	if (d == 0) {
		fail("opendir(%s) failed: %s\n", base, strerror(errno));
		frw->m_OK = false;
		return;
	}
	if (frw->Lock()) {
		frw->m_text->Insert(base);
		frw->m_text->Insert("\n");
		frw->m_text->ScrollToSelection();
		frw->m_text->Flush();
		frw->Unlock();
	}
	chdir(base);	//	to get relative symlinks right
	struct dirent * dent;
	while ((dent = readdir(d)) != 0) {
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) continue;
		if (!frw->m_running) break;
		struct stat st;
		strcpy(end, dent->d_name);
		if (lstat(base, &st) < 0) {
			fail("lstat(%s) failed: %s\n", base, strerror(errno));
			frw->m_OK = false;
		}
		else if ( (S_ISREG(st.st_mode)) &&
		          (false == exclude_file(end))
		        ) {

			map<string, mdsum>::iterator ptr(gManifest.find(base));
			if (ptr == gManifest.end()) {
				if (strncmp(base, "/boot/test/", 11) && strcmp(base, "/boot/var/swap")) {
					info("WARNING: %s is not in MANIFEST\n", base);
				}
				continue;
			}

			void * buf = malloc(st.st_size+1);
			if (!buf) {
				fail("malloc(%s): out of memory!\n", base);
				frw->m_OK = false;
			}
			else {
				int fd = open(base, O_RDONLY);
				if (fd < 0) {
					fail("open(%s): %s\n", base, strerror(errno));
					frw->m_OK = false;
				}
				else {
					long rd = read(fd, buf, st.st_size+1);
					if (rd < 0) {
						fail("read(%s): %s\n", base, strerror(errno));
						frw->m_OK = false;
					}
					else if (rd != st.st_size) {
						fail("read(%s) returns %ld, stat says %Ld\n",
							base, rd, st.st_size);
						frw->m_OK = false;
					}
					close(fd);
					if (strcmp(base, "/boot/home/config/settings/beia-bootmode")) {
						MD5_CTX c;
						MD5_Init(&c);
						MD5_Update(&c, (unsigned char *)buf, rd);
						mdsum ms;
						MD5_Final(ms.data, &c);
						if (ms != (*ptr).second) {
							fail("MD5 checksum failed for %s (got %lx:%lx:%lx:%lx  needed %lx:%lx:%lx:%lx)\n",
								base, ms.values[0], ms.values[1], ms.values[2], ms.values[3],
								(*ptr).second.values[0], (*ptr).second.values[1], (*ptr).second.values[2],
								(*ptr).second.values[3]);
							frw->m_OK = false;
						}
						memset(&c, 0, sizeof(c));
					}
				}
			}
			free(buf);
		}
		else if (S_ISLNK(st.st_mode)) {
			char link[1024];
			int l;
			if (((l = readlink(base, link, 1023)) < 0) || ((link[l] = 0) == link[0])) {
				fail("readlink(%s) failed: %s\n", base, strerror(errno));
				frw->m_OK = false;
			}
			else {
				if (stat(link, &st) < 0) {
					fail("symlink %s: stat(%s) failed: %s\n", base, link, strerror(errno));
					getcwd(link, 1023);
					fail("in directory: %s\n", link);
					frw->m_OK = false;
				}
			}
		}
		else if (S_ISDIR(st.st_mode)) {
			strcat(end, "/");
			recurse_dir(base, end+strlen(end), frw);
			*end = 0;
			chdir(base);	//	reset current directory
		}
		else {
			//	special file, ignored
		}
	}
	closedir(d);
	*end = 0;
	chdir(base);	//	to get relative symlinks right
}

static status_t
file_thread(
	void * data)
{
	char path[1024];
	read_manifest((FileReadWindow *)data);
	read_excludes();
	strcpy(path, "/boot/");
	recurse_dir(path, &path[6], (FileReadWindow *)data);
	gManifest.clear();
	((FileReadWindow *)data)->m_running = false;
	return 0;
}

Test * make_file_read()
{
	FileReadWindow * kkw = new FileReadWindow(200000LL);
	status_t err;
	if ((err = resume_thread(kkw->m_thread)) < 0) {
		fail("FileRead test failed: %s\n", strerror(err));
		kkw->m_OK = false;
		kkw->TestDone(false);
	}
	return kkw->GetTest();
}

