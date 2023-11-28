/*
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved
 */
/* 
 * Poor man's web server 
 *
 * The referrer stuff doesn't work yet
 */
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include <File.h>
#include <NodeInfo.h>
#include <OS.h>
#include <Mime.h>
#include <unistd.h>
#include <sys/socket.h>
#include "web_server.h"
#include "pmprefs.h"
#include "pmlog.h"
#include "hardcoded.h"
#include "pmapp.h"
#include <dirent.h>
#include <arpa/inet.h>

#define RBUF_SIZE 8192

#define REFERER 0	// Test referrer support

#ifndef NOBFILE
#define NOBFILE 0	/* to turn off bfile (debugging only) */
#endif

#ifndef NOHITS
#define NOHITS 0	/* to turn off hits update to window (debugging only) */
#endif

#if TESTING
const char INDEX_DATA[] = "Hello, world!\n";
#endif

extern "C" int strcasecmp(const char *a, const char *b);

char *put_header(
		   const char *fake,
		   const char *filename,
		   const char *protocol,
		   struct stat *st,
		   char *buf
		   );
void getfile(
		int s,
		const char *fake,
		const char *filename,
		const char *protocol,
		struct stat *st,
		const char *addr
		);

#define RECV_TIMEOUT 60		/* in seconds */

#define B_SUCCESS(status) ((status) >= B_OK)

typedef struct handler_args {
	int s;
	char *dirname;
	char addr[16];
	char *index_file_name;
	bool dir_list_flag;
} handler_args_t;

static const char HEAD_TEMPLATE[] = 
"<HTML><HEAD><TITLE>%s directory</TITLE></HEAD><BODY><H1>%s</H1>"
"<H2>Entries:</H2><TABLE>";

static const char ENTRY_TEMPLATE[] = 
"<TR><TD><A HREF=\"%s/%s\">%s</A></TD><TD ALIGN=RIGHT>%Ld bytes</TD></TR>";

static const char ENTRY_DIR_TEMPLATE[] = 
"<TR><TD><A HREF=\"%s/%s\">%s/</A></TD><TD>&nbsp;</TD></TR>";

static const char TAIL_TEMPLATE[] = "</TABLE></BODY></HTML>";

static const char NOT_FOUND[] =
"<HTML><HEAD><TITLE>404 File Not Found</TITLE></HEAD>"
"<BODY><H1>404 File Not Found</H1>"
"The requested file could not be found:"
"<STRONG>%s</STRONG></BODY></HTML>";

void log_address(const char *addr) {
	char s[20];
	sprintf(s, "(%s) ", addr);
	logger->AddToLog(s);
}

char *
append(char *buf, const char *end)
{
	int buflen = buf ? strlen(buf) : 0;
	char *nbuf;

	nbuf = (char *)realloc(buf, buflen + strlen(end) + 1);
	strcpy(&nbuf[buflen], end);
	return (nbuf);
}

int
is_good_html(char c)
{
	switch (c) {
	case '@':
	case '*':
	case '+':
	case '-':
	case '_':
	case '.':
		return (1);
	default:
		return (isalnum(c));
	}
}

void
html_name(char *dst, const char *src)
{
	while (*src) {
		if (is_good_html(*src)) {
			*dst++ = *src++;
		} else {
			sprintf(dst, "%%%02X", (unsigned char)*src++);
			dst += 3;
		}
	}
	*dst = 0;
}

#if 0
void
printstats(char *msg)
{
	struct mstats stats = mstats();

	printf("%s: total=%d cused=%d bused=%d cfree=%d bfree=%d\n", 
		   msg,
		   stats.bytes_total,
		   stats.chunks_used,
		   stats.bytes_used,
		   stats.chunks_free,
		   stats.bytes_free);
}
#endif

unsigned char 
unhex(
	  char a,
	  char b
	  )
{
	char x[3];
	unsigned u;

	x[0] = a;
	x[1] = b;
	x[2] = 0;
	
	sscanf(x, "%02X", &u);
	return (u);
}

void
unhtml(char *filename)
{
	char *p;

	for (p = filename; *p; p++) {
		if (*p == '%') {
			*p = unhex(p[1], p[2]);
			strcpy(p + 1, p + 3);
		}
		if (*p == '?') { // chop off a query string
			*p = '\0';
			break;
		}
	}
}

char *
fake_page(const char *fullname, const char *dirname, const char *leafname)
{
	DIR *dd;
	struct dirent *d;
	char *buf = NULL;
	char entry[1024];
	char *dirdup = strdup(fullname);
	char *p;
	char hname[256];
	struct stat st;
	int pathskip;	// how much of fullname to skip to get the web path

	if (!dirdup) {
		return NULL;
	}

	p = strrchr(leafname, '/');
	if (p != NULL) {
		leafname = p + 1;
	}
	dd = opendir(fullname);
	if (dd == NULL) {
		perror(fullname);
		free(dirdup);
		return (NULL);
	}
	
	if (!strncmp(dirname, dirdup, strlen(dirname))) {
		p = dirdup+strlen(dirname);
		if (!strlen(p)) {
			p = "/";
		}
	}
	else {
		p = dirdup;
	}

	pathskip = strlen(dirname);

	sprintf(entry, HEAD_TEMPLATE, p, p);
	buf = append(buf, entry);
	while (d = readdir(dd)) {
		char *templ;
		char *path;

		templ = (char *) ENTRY_TEMPLATE;
		path = (char *) malloc(2+strlen(fullname)+strlen(d->d_name));
		if (path) {
			strcpy(path, fullname);
			strcat(path, "/");
			strcat(path, d->d_name);
			if (stat(path, &st) >= 0) {
				if ((st.st_mode & S_IFMT) == S_IFDIR) {
					templ = (char *) ENTRY_DIR_TEMPLATE;
				}
			}
			free(path);
		}
		html_name(hname, d->d_name);
		sprintf(entry, templ, fullname+pathskip, hname, d->d_name, st.st_size);
		buf = append(buf, entry);
	}
	buf = append(buf, TAIL_TEMPLATE);
	closedir(dd);
	free(dirdup);
	return (buf);
}

int
recv_timeout(
			 int s,
			 char *c,
			 int len,
			 int flags,
			 int seconds
			 )
{
	struct timeval tv;
	struct fd_set fds;
	int n;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(s, &fds);
	switch (n = select(s + 1, &fds, 0, 0, &tv)) {
	case 0:
	case -1:
		return (0);
	default:
		n = recv(s, c, len, flags);
		return (n);
	}
}
		
int
parse_command(char *buf, char *cmd, int cmdlen, char *ref, int reflen)
{
	int i;
	int line = 0;
	int j = 0;

	// This loop scans the buffer and yanks out two things:
	// The first line is always the HTTP command.  This is
	// yanked and stuffed into "cmd".  Then each following
	// line is yanked, checked to see if it's a "Referer:"
	// entry, and if it is, it's returned.  Otherwise,
	// it's ignored and the next line is read, until the
	// end of the message.

	for (i = 0; buf[i]; i++) {
		if (line == 0) {
			cmd[i] = buf[i];
			if (cmd[i] == '\n') {
				cmd[i] = 0;
				if (cmd[i - 1] == '\r') {
					cmd[i - 1] = 0;
				}
				line++;
			} 
		} else {
			ref[j] = buf[i];
			if (ref[j] == '\n') {
				ref[j] = 0;
				if (ref[j - 1] == '\r') {
					ref[j - 1] = 0;
				}
				if (strncmp(ref, "Referer:", 8) == 0) {
					//printf("Referer line found: %s\n", ref);
					return (1);
				}
				j = 0;
			} else {
				j++;
			}
		}
	}
	ref[0] = 0;
	return (line > 0);
}

int 
get_command(int s, char *buf, int buflen, char *ref, int reflen)
{
	char thisbuf[RBUF_SIZE];
	int thislen = 0;
	int i;
	int count;
	int newlines = 0;

	for (;;) {
		if (thislen == RBUF_SIZE) {
			return (0);
		}
		count = recv_timeout(s, &thisbuf[thislen], RBUF_SIZE - thislen,
							 0, RECV_TIMEOUT);
		if (count <= 0) {
			break;
		}
		thisbuf[count] = '\0';		// temp
		//printf("====\nCommand:\n%s\n", thisbuf);
		for (i = 0; i < count; i++) {
			switch (thisbuf[thislen + i]) {
			case '\n':
				if (++newlines == 2) {
					thisbuf[thislen + i] = 0;
					if (!parse_command(thisbuf, buf, buflen, ref, reflen)) {
						return (0);
					}
					return (1);
				}
				break;
			case '\r':
				break;
			default:
				newlines = 0;
				break;
			}
		}
		thislen += count;
	}
	return (0);
}


void
not_found(int s, const char *filename, const char *addr)
{
	char buf[BUFSIZ];
void getfile(
		int s,
		const char *fake,
		const char *filename,
		const char *protocol,
		struct stat *st,
		const char *addr
		);
	
	sprintf(buf, NOT_FOUND, filename);
	getfile(s, buf, filename, " ", NULL, addr);
#if OLD_ERROR
	sprintf(buf, "HTTP/1.0 404 Not found\r\n");
	send(s, buf, strlen(buf), 0);
#endif
	logger->Lock();
	logger->AddToLog(NULL);
	log_address(addr);
	logger->AddToLog("Error 404 File not found: ");
	logger->AddToLog((char *) filename);
	logger->AddToLog("\n");
	logger->Unlock();
}

char *
ok(char *buf)
{
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	return (buf + strlen(buf));
}

char *moved1 =
	"HTTP/1.0 301 Moved Permanently\r\n"
	"Location: http://";

void moved_permanently(char *filename, const char *addr, int s) {
	char buf[BUFSIZ];
	char host[BUFSIZ];
	struct hostent *hostInfo;
	char c;
	bool hostNameOK;

	// Get the hostname

	if (gethostname(host, BUFSIZ)) {
		hostInfo = gethostbyname(host);
		if (!hostInfo) {
			hostNameOK = false;
		}
		else {
			hostNameOK = true;
		}
	}
	else {
		hostNameOK = false;
	}

	if (strlen(filename) && filename[strlen(filename)-1] != '/') {
		if (hostNameOK) {
			sprintf(buf, "%s%s/%s/\r\n", moved1, hostInfo->h_name, filename);
		}
		else {
			not_found(s, filename, addr);		// just error instead
			return;
		}
	}
	else {
		sprintf(buf, "%s%s/%s\r\n", moved1, hostInfo->h_name, filename);
	}
	send(s, buf, strlen(buf), 0);
	printf(buf);
}

void
not_implemented(int s)
{
	char buf[BUFSIZ];

	sprintf(buf, "HTTP/1.0 501 Not implemented\r\n");
	send(s, buf, strlen(buf), 0);
}

int 
is_ext(
	   const char *filename,
	   const char *ext
	   )
{
	int elen;
	int len;

	len = strlen(filename);
	elen = strlen(ext);

	if (elen >= len) {
		return (0);
	}
	if (filename[(len - 1) - elen] != '.') {
		return (0);
	}
	if (strcasecmp(&filename[len - elen], ext) != 0) {
		return (0);
	}
	return (1);
}

// Returns true if a type is being returned; false if
// a MIME update was done (in which case you should call again).
bool get_mimetype(const char *filename, char *buf, bool update) {
	BFile file;
	char type[BUFSIZ];
	status_t status;
	
#if !NOBFILE
	status = file.SetTo(filename, O_RDONLY);
	if (B_SUCCESS(status)) {
		BNodeInfo nm(&file);
		if (nm.GetType(type) != B_OK) {
			if (update) {
				update_mime_info(filename, 0, 1, 0);
				return false;
			}
			else {
				strcpy(buf, "Content-type: application/octet-stream\r\n");
			}
		}
		else {
			if (!strcmp(type, "text/plain")) {	// double-check plain text
				if (is_ext(filename, "html")) {
					strcpy(buf, "Content-type: text/html\r\n");
					return true;
				}
			}
			sprintf(buf, "Content-type: %s\r\n", type);
		}
	}
#endif
	else {
		if (is_ext(filename, "html")) {
			strcpy(buf, "Content-type: text/html\r\n");
		} else if (is_ext(filename, "gif")) {
			strcpy(buf, "Content-type: image/gif\r\n");
		} else if (is_ext(filename, "jpg")) {
			strcpy(buf, "Content-type: image/jpeg\r\n");
		} else {
			strcpy(buf, "Content-type: text/plain\r\n");
		}
	}
	return true;
}

char *
put_header(
		   const char *fake,
		   const char *filename,
		   const char *protocol,
		   struct stat *st,
		   char *buf
		   )
{
	char type[BUFSIZ];
	status_t status;
	BFile file;
	char *p;

	p = ok(buf);
	if (fake) {
		strcpy(p, "Content-type: text/html\r\n");
	} else {
		if (!get_mimetype(filename, p, true)) {
			get_mimetype(filename, p, false);
		}
	}
	p += strlen(p);

	if (fake) {
		sprintf(p, "Content-length: %d\r\n", strlen(fake));
	} else {
		sprintf(p, "Content-length: %d\r\n", (int)st->st_size);
	}
	p += strlen(p);

	sprintf(p, "Server: BeOS/PoorMan\r\n");
	return (p + strlen(p));
}

void send_file_error(const char *filename, const char *addr) {
	logger->Lock();
	logger->AddToLog(NULL);
	log_address(addr);
	logger->AddToLog("Error sending file: ");
	logger->AddToLog((char *) filename);
	logger->AddToLog("\n");
	logger->Unlock();
}

void
getfile(
		int s,
		const char *fake,
		const char *filename,
		const char *protocol,
		struct stat *st,
		const char *addr
		)
{
	FILE *f;
	char buf[1024];
	int len;
	char *p;

	if (!fake) {
		logger->Lock();
		logger->AddToLog(NULL);
		log_address(addr);
		logger->AddToLog("Sending file: ");
		logger->AddToLog((char *) filename);
		logger->AddToLog("\n");
		logger->Unlock();
	}

#if !TESTING
	if (!fake) {
		f = fopen(filename, "rb");
		if (f == NULL) {
			not_found(s, (char *) filename, addr);
			return;
		}
	}
#endif

	if (protocol) {
		p = put_header(fake, filename, protocol, st, buf);
	} else {
		p = ok(buf);
	}
	sprintf(buf, "%s%s",buf,"\r\n");

	if (send(s, buf, strlen(buf), 0) != strlen(buf)) {
		#if !TESTING
		if (!fake) {
			fclose(f);
		}
		#endif
		send_file_error(filename, addr);
		return;
	}

#if TESTING
	/*
	 * Avoid using the filesystem for testing
	 */
	send(s, INDEX_DATA, strlen(INDEX_DATA), 0);
#else /* TESTING */
	if (fake) {
		if (send(s, fake, strlen(fake), 0) != strlen(fake)) {
			send_file_error(filename, addr);
		}
	} else {
		while ((len = fread(buf, 1, sizeof(buf), f)) > 0) {
			ssize_t bytes_sent;
			bytes_sent = send(s, buf, len, 0);

			if (bytes_sent < len) {		// If error sending data
				send_file_error(filename, addr);
				break;
			}

			if (len < sizeof(buf)) {
				break;			// If finished sending file
			}
		}
		fclose(f);
	}
#endif /* TESTING */
}

// Returns nonzero if the name is altered.
bool
no_trailing_slashes(char *filename)
{
	int len = strlen(filename);
	bool result = false;

	while (len > 0 && filename[len - 1] == '/') {
		filename[len - 1] = 0;
		result = true;
	}
	return result;
}

/*
 * Remove '..'s from filename, so that people can't ..
 * out of the html directory.
 *
 */
char *
undotdot(char *filename)
{
	char *p;
	int seensep;
	char *save;


	/*
	 * First, remove any leading ..
	 */
	p = filename;
	while (p[0] == '.' && p[1] == '.' &&
		   (p[2] == 0 || p[2] == '/')) {
		strcpy(p, &p[2]);
	}

	/*
	 * Then, convert any "foo/.." to ""
	 */
	for (p = filename; *p; p++) {
		if ((p == filename || p[-1] == '/') &&
			(p[0] == '.' && p[1] == '.' &&
			 (p[2] == 0 || p[2] == '/'))) {
			/*
			 * Found a ".." component
			 */
			save = &p[2];

			/*
			 * skip slashes before ".."
			 */
			p--;
			while (p > filename && *p == '/') {
				p--;
			}
			
			/*
			 * skip component before ".."
			 */
			while (p > filename && *p != '/') {
				p--;
			}

			/*
			 * Remove component and ".."
			 */
			strcpy(p, save);
			if (*p == 0) {
				/*
				 * We are done
				 */
				break;
			}

		}
	}
	return (filename);
}

#if REFERER
int
findref(char *ref)
{
	char *p;

	//printf("--in findref:\n");
	p = strchr(ref, ' ');
	if (p == NULL) {
		return (0);
	}
	//printf("  1: p=%s\n", p);
	while (*p == ' ') {
		p++;
	}
	//printf("  2: p=%s\n", p);
	if (strncmp(p, "http://", 7) != 0) {
		return (0);
	}
	//printf("  3: got here\n");
	p += 7;
	//printf("  4: p=%s\n", p);
	p = strchr(p, '/');
	if (p == NULL) {
		return (0);
	}
	//printf("  5: p=%s\n", p);
	p += 1;
	//printf("  6: p=%s\n", p);
	if (*p == 0) {
		return (0);
	}
	strcpy(ref, p);
	return (1);
}
#endif

int32
handler(
		int s,
		const char *dirname,
		const char *addr,			// requester's address in ASCII
		const char *index_file_name,
		bool dir_list_flag
		)

{
	char buf[BUFSIZ];
	char ref[BUFSIZ];
	char expanded[BUFSIZ];
	char newfilename[BUFSIZ];
	char *op;
	char *filename;
	char *protocol;
	char *space1;
	char *space2;
	struct stat st;
	char *fake = NULL;
	char *p;

	if (get_command(s, buf, sizeof(buf), ref, sizeof(ref)) == 0) {
		not_implemented(s);
		return (0);
	}
	op = buf;
	space1 = strchr(buf, ' ');
	if (space1 == NULL) {
		return (0);
	}
	*space1 = 0;

	filename = space1 + 1;

	space2 = strchr(space1 + 1, ' ');
	if (space2 != NULL) {
		*space2 = 0;
		protocol = space2 + 1;
	} else {
		protocol = NULL;
	}
	while (*filename == '/') {
		filename++;
	}

// If the filename is relative and there's a
// Referer, use that to build the full path.
// Otherwise, just tack the filename onto the
// base directory of the web site.

#if REFERER
	unhtml(filename);
	if ((filename[0] != '/') && findref(ref)) {
		//printf("found referer: %s\n", ref);
		unhtml(ref);
		undotdot(ref);
		sprintf(expanded, "%s/%s/%s", dirname, ref, undotdot(filename));
		//printf("referer-expanded: %s\n", expanded);
	}
	else {
		char *pp = filename;
		if (filename[0] == '/') {
			pp++;
		}
		sprintf(expanded, "%s/%s", dirname, undotdot(pp));
		//printf("expanded: %s\n", expanded);
	}
#else
	unhtml(filename);
	sprintf(expanded, "%s/%s", dirname, undotdot(filename));
#endif

	int send_moved_permanently;

	// If it's a directory, and there's no trailing slash on
	// the name, we need to return a 301 Moved Permanently
	// message so the braindead client can figure out that
	// this is a directory.

	send_moved_permanently = !no_trailing_slashes(expanded);
	if (stat(expanded, &st) < 0) {
		not_found(s, filename, addr);
		return (0);
	}
	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		if (send_moved_permanently /*|| !strlen(filename)*/) {
			moved_permanently(filename, addr, s);
			return 1;
		}
		sprintf(newfilename, "%s/%s", expanded, index_file_name);
		if (stat(newfilename, &st) < 0) {
			if (dir_list_flag) {
				fake = fake_page(expanded, dirname, filename/*undotdot(filename)*/);
			}
			else {
				fake = NULL;
			}
			if (fake == NULL) {
				not_found(s, newfilename, addr);
				return (0);
			}
			else {
				logger->Lock();
				logger->AddToLog(NULL);
				log_address(addr);
				logger->AddToLog("Directory ");
				logger->AddToLog(expanded);
				logger->AddToLog(" has no ");
				logger->AddToLog(index_file_name);
				logger->AddToLog(".  Sending directory listing.\n");
				logger->Unlock();
			}
		} else if ((st.st_mode & S_IFMT) == S_IFDIR) {
			if (dir_list_flag) {
				fake = fake_page(expanded, dirname, filename/*undotdot(filename)*/);
			}
			else {
				not_found(s, newfilename, addr);
				return (0);
			}
		}
		strcpy(expanded, newfilename);
	}
	if (strcmp(op, "GET") == 0) {
		getfile(s, fake, expanded, protocol, &st, addr);
	} else if (strcmp(op, "HEAD") == 0) {
		p = put_header(fake, expanded, protocol, &st, buf);
		sprintf(buf, "%s%s",buf,"\r\n");
		send(s, buf, strlen(buf), 0);
	} else {
		not_implemented(s);
	}
	if (fake) {
		free(fake);
	}
	return (1);
}

int32
_handler(
		 void *varg
		 )
{
	handler_args_t *args = (handler_args_t *)varg;
	int res;

	res = handler(args->s, args->dirname, args->addr,
				args->index_file_name, args->dir_list_flag);
	close(args->s);
	free(args->dirname);
	free(args->index_file_name);
	delete args;
	return (res);
}


void
handle_in_thread(int s, const char *dirname, struct in_addr addr,
			const char *index_file_name, const bool dir_list_flag)
{
	thread_id th;
	handler_args_t *args;

	args = new(handler_args_t);
	args->s = s;
	args->dirname = strdup(dirname);
	args->index_file_name = strdup(index_file_name);
	args->dir_list_flag = dir_list_flag;
	strcpy(args->addr, inet_ntoa(addr));
	
	th = spawn_thread(_handler, "www connection", B_NORMAL_PRIORITY, (void *)args);
	resume_thread(th);

	if (!B_SUCCESS(th)) {
		close(s);
		return;
	} 
}

WebServer::WebServer(void):
	lock("web-dir")
{
	s = -1;
	callback = NULL;
	hits = 0;
	this->dirname = NULL;
	index_file_name = NULL;
}

WebServer::~WebServer(void)
{
	if (s >= 0) {
		close(s);
		s = -1;
	}
}

void
WebServer::ClearHits(void)
{
	hits = 0;
	if (callback) {
		callback->Hit(0);
	}
}

void
WebServer::SetCallback(GetHits *callback)
{
	this->callback = callback;
}

long
listener(void *varg)
{
	WebServer *ws = (WebServer *)varg;
	struct sockaddr_in from;
	int fromlen;
	int s2;
	int s;
	GetHits *callback;

	s = ws->GetSocket();
	for (;;) {
		fromlen = sizeof(from);
		s2 = accept(s, (struct sockaddr *) &from, &fromlen);
		if (s2 < 0) {
			break;
		}
		ws->Hit();
		ws->Lock();
		handle_in_thread(s2, ws->GetDirName(), from.sin_addr,
				ws->GetIndexFileName(), ws->GetDirListFlag());
		ws->Unlock();
	}
	return (0);
}

void
WebServer::Hit(
			   void
			   )
{
	hits++;
	if (callback) {
#if !NOHITS
		callback->Hit(hits);
#endif
	}
}

int
WebServer::Start(int backlog)
{
	struct sockaddr_in mine;
	thread_id th;

	logger->Lock();
	logger->AddToLog(NULL);
	logger->AddToLog("Starting up...");	// Add to log

	// If no directory set, fail at once
	
	if (!dirname) {
		logger->AddToLog(" no web directory, can't start up.");
		return 0;
	}

	// Open the socket.  Try up to five times, five
	// seconds apart

	int i = 5;
	do {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if ((s < 0) && (i!=1)) {
			snooze(5000000);
		}
		else {
			break;
		}
	} while (--i);

	if (s < 0) {
		perror("socket");
		logger->AddToLog(" can't create socket.\n");
		logger->Unlock();
		return (0);
	}
	mine.sin_family = AF_INET;
	mine.sin_port = htons(80);
	mine.sin_addr.s_addr = 0;
	memset(mine.sin_zero, 0, sizeof(mine.sin_zero));
	if (bind(s, (struct sockaddr *)&mine, sizeof(mine)) < 0) {
		perror("bind");
		close(s);
		s = -1;
		logger->AddToLog(" can't bind to socket.\n");
		logger->Unlock();
		return (0);
	}
	if (listen(s, backlog) < 0) {
		perror("listen");
		close(s);
		s = -1;
		logger->AddToLog(" can't listen to socket.\n");
		logger->Unlock();
		return (0);
	}
	th = spawn_thread(listener, "www listener", B_NORMAL_PRIORITY,
					  (void *)this);
	if (!B_SUCCESS(th)) {
		close(s);
		s = -1;
		logger->AddToLog(" can't create listener thread.\n");
		logger->Unlock();
		return (0);
	}
	resume_thread(th);
	logger->AddToLog(" done.\n");
	logger->Unlock();
	return (1);
}

void
WebServer::Stop(void)
{
	logger->Lock();
	logger->AddToLog(NULL);
	logger->AddToLog("Shutting down.\n");
	logger->Unlock();
	if (s >= 0) {
		close(s);
		s = -1;
	}
}
