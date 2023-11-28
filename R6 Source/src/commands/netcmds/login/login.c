/*
 * Copyright (C) 1996 Be Inc.  All Rights Reserved
 */

/*
 * login program: for use mainly by telnetd
 */
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <termios.h>
#include <OS.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pwd.h>


#if DEBUG
#define read log_read
#define write log_write
#else
#define logit (void)
#endif /* DEBUG */

extern char **environ;
extern char *crypt(const char *pass, const char *salt);

static struct passwd *pw = 0;

#define NAMESIZE 32
#define ALARMTIME 60	/* timeout login after 60 seconds */

static int notimeout = 0;

static void
sigalrm(int ignored)
{
	(void)(ignored);
}

static void
echo(int on)
{
	struct termios tc;

	ioctl(0, TCGETA, &tc);
	if (on) {
		tc.c_lflag |= ECHO;
	} else {
		tc.c_lflag &= ~ECHO;
	}
	ioctl(0, TCSETA, &tc);
}

static void
init_tty(void)
{
	struct termios tc;

	ioctl(0, TCGETA, &tc);
	tc.c_lflag |= (ISTRIP | ISIG | ICANON | ECHO);
	tc.c_cflag &= ~CSIZE;
	tc.c_cflag |= (CS7 | PARENB);
	tc.c_oflag |= OPOST;
	ioctl(0, TCSETA, &tc);
}

void
putstr(const char *str)
{
	write(1, str, strlen(str));
}

void
getstr(
	   char *str,
	   int strsize
	   )
{
	char c;
	int len;
	int cnt;

	alarm(ALARMTIME);
	len = 0;
	while ((cnt = read(0, &c, 1)) == 1 && c != '\r' && c != '\n') {
		if (len < strsize) {
			str[len++] = c;
		}
	}
	if (cnt < 0) {
		putstr("Login timed out.\n");
		exit(1);
	}
	str[len] = 0;
	alarm(0);	/* cancel alarm */
}

static
void
print_tty(int h, bool full)
{
	char *tt= ttyname(h);
	char *pt= NULL;
	char *pq= NULL;

	if(!tt) {
		tt= "Unknown tty";
	}

	pt= strdup(tt);
	if(!full      ) { pq= 1+strrchr(pt, '/'); }
	if(!pq || !*pq) { pq= pt;                 }

	write(h, pq, strlen(pq));

	free(pt);
}

static
void
etc_issue(char const *hostname)
{
	int   fd= 0;
	off_t flength;
	char *buff;
	int   nread;
	int   i;
	int   j;

	fd= open("/etc/issue", O_RDONLY, 000);
	if(fd< 0) {
		return;
	}

	flength= lseek(fd, 0L, SEEK_END);
	buff= (char *)malloc(flength+1);
	lseek(fd, 0L, SEEK_SET);

	nread= read(fd, buff, flength);
	buff[nread]= 0;

	i= 0;
	while(i< nread) {
		for(j= i; (j< nread) && (buff[j]!= '%'); j++) {
		}

		write(1, buff+i, j-i);
		if(buff[j]== '%') {
			switch(buff[j+1]) {
				case '%':
					write(1, "%", 1);
					break;
				case 'h':
				case 'H':
					write(1, hostname, strlen(hostname));
					break;
				case 'o':
				case 'O':
					write(1, "BeOS", 4);
					break;
				case 't':
					print_tty(1, false);
					break;
				case 'T':
					print_tty(1, true);
					break;
				default:
					break;
			}
			j++;
		}
		i= j+1;
	}

	free(buff);
	close(fd);
}

int
login(const char *hostname)
{
	char username[33];
	char password[33];
	char *checkpass;
	int failed;

	failed = 0;
	checkpass = NULL;
	echo(1);
	if (*hostname) {
		putstr(hostname);
		putstr(" ");
	}
#if DEBUG
	putstr("login (debug): "); 
#else
	putstr("login: "); 
#endif
	getstr(username, 32);
	if (username[0] == 0) {
		exit(1);
	}
	if ((pw = getpwnam(username)) == NULL) {
		logit("failed because username doesn't match: ");
		logit(username);
		logit("\n");
		failed++;
	} else {
		checkpass = pw->pw_passwd;		
	}
	
	putstr("Password: ");
	echo(0);
	getstr(password, 32);
	echo(1);
	putstr("\n");
	if (checkpass && *checkpass) {
		if (strcmp(crypt(password, checkpass), checkpass) != 0) {
			logit("failed because had password and crypt failed\n");
			failed++;
		}
	}
		
	if (failed) {
		putstr("Password incorrect. Wait a second...");
		fflush(stdout);
		sleep(1);
		putstr("\n");
	}
	return (!failed);
}

#if 0
static void
fork_exec(
		  const char *fname,
		  const char **argv,
		  const char **env
		  )
{
	thread_id tid;
	int		argc;

	argv[0] = fname;
	for(argc=1; argv[argc]; argc++)
		;
	tid = load_image(argc, argv, env);
	if (tid >= B_NO_ERROR) {
		resume_thread(tid);
		ioctl(0, 'pgid', tid);
	}
}
#endif 

int
main(int argc, char **argv)
{
	char hostname[NAMESIZE + 1];
	const char *Argv[3];

	if (argc > 1 && strcmp(argv[1], "-n") == 0) {
		notimeout++;
	}
#if !BEOS_MULTIUSER
	if (!getenv("USER")) {
		putenv("USER=root");
	}
	if (!getenv("GROUP")) {
		putenv("GROUP=wheel");
	}
	if (!getenv("HOME")) {
		putenv("HOME=/boot/home");
	}
#endif
	if (!getenv("SHELL")) {
		putenv("SHELL=/bin/sh");
	}
	if (!getenv("PATH")) {
		putenv("PATH=/bin:/boot/apps:/boot/preferences");
	}
	if (!notimeout) {
		signal(SIGALRM, sigalrm);
	}
	init_tty();
	gethostname(hostname, sizeof(hostname));
	etc_issue(hostname);
	if (notimeout) {
		while (!login(hostname));
	} else {
		if (!login(hostname) && 
			!login(hostname) &&
			!login(hostname)) {
			putstr("Too many failed logins. Goodbye!\n");
			exit(1);
		}
	}
#if (BEOS_MULTIUSER)
	if (pw) {
		char tmp[1024];

		sprintf(tmp,"USER=%s",pw->pw_name);
		putenv(tmp);

		sprintf(tmp,"LOGNAME=%s",pw->pw_name);
		putenv(tmp);

		sprintf(tmp,"HOME=%s",pw->pw_dir);
		putenv(tmp);

		putenv("TERM=vt100");
	
		setuid(pw->pw_uid);
		setgid(pw->pw_gid);

	} else {
		// Brian : do these need to be set here?  
		char userenv[64];
		char groupenv[64];
		char homeenv[64];
		
		
		strcat(userenv,"USER=");
		strcat(userenv,pw->pw_name);
		putenv(userenv);
		
		// Brian : default group and home for the old-style
		// authentication are ??
		strcat(groupenv,"GROUP=");
		strcat(groupenv,"users");
		putenv(groupenv);
		
		strcat(homeenv,"HOME=");
		strcat(homeenv,"/boot/home");

		setuid(0);
		setgid(0);
		
	}
#endif // !(BEOS_MULTIUSER)
	Argv[0] = "sh";
	Argv[1] = "-login";
	Argv[2] = NULL;
	ioctl(0, 'pgid', getpid());
	chdir(getenv("HOME"));
	execve("/bin/sh", (char **)Argv, environ);
	exit(0);
}

#if DEBUG
#undef read
#undef write

static FILE *f;

int
logit(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	if (f == NULL) {
		f = fopen("login.out", "w");
	}
	if (f) {
		vfprintf(f, format, args);
		fflush(f);
	}
	va_end(args);
}

static void
showbuf(const char *buf, int len)
{
	int i;
	char line[80];
	char c;

	for (i = 0; i < len; i++) {
		c = buf[i];
		if (c >= ' ' && c <= '~') {
			sprintf(&line[(i % 25) * 3], " %c ", c);
		} else {
			sprintf(&line[(i % 25) * 3], "x%02x", c);
		}
		if ((i % 25) == 24) {
			logit("%s\n", line);
		}
	}
	if ((i % 25) != 0) {
		logit("%s\n", line);
	}
}


int
log_read(int fd, char *buf, int len)
{
	int ret;

	ret = read(fd, buf, len);
	logit("read(%d, %d) = %d\n", fd, len, ret);
	showbuf(buf, len);
	return (ret);
}


int
log_write(int fd, char *buf, int len)
{
	int ret;

	ret = write(fd, buf, len);
	logit("write(%d, %d) = %d\n", fd, len, ret);
	showbuf(buf, len);
	return (ret);
}
#endif /* DEBUG */
