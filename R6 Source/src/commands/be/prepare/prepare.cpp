
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include "md5.h"

#include <Application.h>
#include <Window.h>
#include <StringView.h>

const char * NETSERVER_MAGIC = "/tmp/dont_start_the_net_server";
const char * REMOVEFILE = "/boot/test/removals.txt";
const char * DIGESTFILE = "/boot/test/MANIFEST";

static const char * dont_checksum[] =
{
	"/boot/home/trimodem/EEPROM",		//	tri_modem file (bleach!)
	"/boot/test",
	"/boot/var/swap"
};



static BWindow * gWindow;
static BStringView * gStringView;
FILE * gLogfile;
static bigtime_t gLastTime = 0;

class ZWindow : public BWindow {
public:
		ZWindow(const BRect & frame, const char * name, window_type type, uint32 flags) :
			BWindow(frame, name, type, flags) { }
		bool QuitRequested() { return false; }
};

class PrepApp : public BApplication {
public:
		PrepApp(const char * sig) : BApplication(sig) {
			gWindow = new ZWindow(BRect(100,100,700,140), "Prepare", B_TITLED_WINDOW,
				B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_MINIMIZABLE);
			gStringView = new BStringView(gWindow->Bounds(), "status", "");
			gWindow->AddChild(gStringView);
			gStringView->SetFontSize(18);
			gWindow->Show();
		}
		void MessageReceived(BMessage * msg) {
			if (msg->what == 'quit') {
				gWindow->Lock();
				gWindow->Quit();
				gWindow = 0;
				Quit();
			}
			else {
				BApplication::MessageReceived(msg);
			}
		}
		bool QuitRequested() {
			return false;
		}
};

static void
log(
	const char * fmt,
	...)
{
	char str[1024];
	va_list lst;
	va_start(lst, fmt);
	vsprintf(str, fmt, lst);
	va_end(lst);
	fprintf(stderr, "%s\n", str);
	if (!gLogfile) {
		gLogfile = fopen("/boot/test/testresult.html", "w");
		if (gLogfile) {
			fprintf(gLogfile, "<html><head>Prepare Results</head><body><pre>\n");
		}
	}
	if (gLogfile) {
		fprintf(gLogfile, "%s\n", str);
	}
	snooze_until(gLastTime+1000000, 0);
	if (gWindow->Lock()) {
		gStringView->SetText(str);
		gStringView->Invalidate();
		gWindow->Unlock();
		gLastTime = system_time();
	}
}


static int ret = 0;

static void
recurse(
	char * base,
	char * end,
	FILE * file)
{
	struct stat st;
	DIR * d;
	struct dirent * ent;
	void * buf = 0;
	ssize_t r;

	d = opendir(base);
	if (d == NULL) {
		log("%s: %s", base, strerror(errno));
		return;
	}
	while ((ent = readdir(d)) != NULL) {
		strcpy(end, ent->d_name);
		for (int ix=0; ix<sizeof(dont_checksum)/sizeof(char*); ix++)
		{
			if (!strcmp(base, dont_checksum[ix]) || !strcmp(end, dont_checksum[ix]))
			{
				fprintf(stderr, "don't checksum: %s\n", ent->d_name);
				goto skip_this;
			}
		}
		if (lstat(base, &st) < 0) {
			log("%s: %s", base, strerror(errno));
			ret = 1;
			return;
		}
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			strcat(end, "/");
			recurse(base, end+strlen(end), file);
		}
		else if (S_ISREG(st.st_mode)) {
			int fd = open(base, O_RDONLY);
			MD5_CTX c;
			unsigned char mv[16];
			if (fd < 0) {
				log("%s: %s", base, strerror(errno));
				ret = 1;
				continue;
			}
			buf = realloc(buf, st.st_size);
			if ((r = read(fd, buf, st.st_size)) != st.st_size) {
				if (errno == 0) {
					log("%s: short read", base);
				}
				else {
					log("%s: %s", base, strerror(errno));
				}
				close(fd);
				ret = 1;
				continue;
			}
			close(fd);
			MD5_Init(&c);
			MD5_Update(&c, (unsigned char *)buf, r);
			MD5_Final(mv, &c);
			memset(&c, 0, sizeof(c));
			for (r=0; r<4; r++) {
				fprintf(file, "%s%x", r ? ":" : "", ((unsigned int *)mv)[r]);
			}
			fprintf(file, "\t%s\n", base);
		}
		else if (S_ISLNK(st.st_mode)) {
			if (stat(base, &st) < 0) {
				log("%s: %s", base, strerror(errno));
				ret = 1;
			}
		}
skip_this:
		;
	}
	free(buf);
	closedir(d);
}


static int
recursive_unlink(
	char * buf,
	char * end,
	struct stat * stbuf)
{
	strcpy(end, ".");
	DIR * d = opendir(buf);
	int ret = 0;
	if (d)
	{
		struct dirent * dent;
		while (NULL != (dent = readdir(d)))
		{
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;
			strcpy(end, dent->d_name);
			if (lstat(buf, stbuf) < 0)
			{
				log("%s: %s\n", buf, strerror(errno));
				ret = 1;
			}
			else if (S_ISDIR(stbuf->st_mode))
			{
				char * e = end+strlen(end);
				*e++ = '/';
				*e = 0;
				ret = recursive_unlink(buf, e, stbuf) || ret;
				e[-1] = 0;
				if (rmdir(buf) < 0) {
					log("%s: %s\n", buf, strerror(errno));
					ret = 1;
				}
				else {
					fprintf(stderr, "DIR %s removed\n", buf);
				}
			}
			else if ((S_ISREG(stbuf->st_mode)) ||
				(S_ISLNK(stbuf->st_mode)))
			{
				if (unlink(buf) < 0)
				{
					log("%s: %s\n", buf, strerror(errno));
					ret = 1;
				}
				else {
					fprintf(stderr, "%s removed\n", buf);
				}
			}
			else
			{
				log("%s: unknown st_mode 0x%08x\n", stbuf->st_mode);
				ret = 1;
			}
		}
		closedir(d);
	}
	else
	{
		log("%s: cannot opendir()\n");
		ret = 1;
	}
	return ret;
}

static status_t
prepare_thread(
	void * /* arg */)
{
	FILE * f = fopen(REMOVEFILE, "r");
	char line[1024];
	struct stat stbuf;
	if (f == NULL) {
		log("%s missing; not cleaning any files!", REMOVEFILE);
	}
	else {
		chdir("/boot");
		log("removing unwanted files, please wait...");
		while (1) {
			char * p;
			line[0] = 0;
			fgets(line, 1024, f);
			if (!line[0]) break;
			p = strrchr(line, '\n');
			if (p != NULL) *p = 0;
			if (!lstat(line, &stbuf) && S_ISDIR(stbuf.st_mode))
			{
				char * e = line+strlen(line);
				*e++ = '/';
				*e = 0;
				ret = recursive_unlink(line, e, &stbuf) || ret;
				e[-1] = 0;
				if (rmdir(line) < 0) {
					log("%s: %s\n", line, strerror(errno));
					ret = 1;
				}
				else {
					fprintf(stderr, "DIR %s removed\n", line);
				}
			}
			else if (unlink(line) < 0) {
				if (errno != ENOENT) {
					log("%s: %s", line, strerror(errno));
					ret = 1;
				}
				else {
					fprintf(stderr, "%s: %s\n", line, strerror(errno));
				}
			}
			else {
				fprintf(stderr, "%s removed\n", line);
			}
		}
	}
	strcpy(line, "/boot/");
	fclose(f);
	f = fopen(DIGESTFILE, "w");
	if (!f) {
		log("cannot create %s!", DIGESTFILE);
		ret = 1;
		goto outahere;
	}
	log("calculating checksums, please wait...");
	recurse(line, &line[6], f);
	fclose(f);
	sync();
	log("All done!");
outahere:
	snooze(3000000);
	be_app->PostMessage('quit');
	return 0;
}

const char * sigs[] = {
	"application/x-vnd.Be-TellBrowser",
	"application/x-vnd.Be-NETS",
	"application/x-vnd.Be-TRAK",
	"application/x-vnd.Be-TSKB",
	"application/x-vnd.Web",
	"application/x-vnd.Be-ROST",
	"application/x-vnd.be.snd_server",
	0
};

static void
quit_shit()
{
	for (int ix=0; sigs[ix] != 0; ix++) {
		BMessenger msgr(sigs[ix]);
		if (msgr.IsValid()) {
			log("Asking app %s to quit", sigs[ix]);
			msgr.SendMessage(B_QUIT_REQUESTED);
		}
	}
	snooze(1000000);
	for (int ix=0; sigs[ix] != 0; ix++) {
		BMessenger msgr(sigs[ix]);
		if (msgr.IsValid()) {
			team_id team = msgr.Team();
			if (team > 0) {
				log("Killing team %ld (%s)", team, sigs[ix]);
				kill_team(team);
			}
		}
	}
}

int
main()
{
	close(open(NETSERVER_MAGIC, O_RDWR|O_CREAT|O_TRUNC, 0777));
	fprintf(stderr, "Prepare copyright 2000 Be, Incorporated.\n");
	fprintf(stderr, "MD5 checksum by Eric Young from SSLeay.\n");
	PrepApp app("application/x-vnd.be.prepare-install");
	quit_shit();
	resume_thread(spawn_thread(prepare_thread, "prepare_thread", 10, NULL));
	app.Run();
	if (gLogfile) {
		fprintf(gLogfile, "</pre></body></html>\n");
		fclose(gLogfile);
	}
	unlink(NETSERVER_MAGIC);
	return ret;
}

