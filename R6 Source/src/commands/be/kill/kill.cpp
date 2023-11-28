#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <OS.h>

#ifndef _ROSTER_H
#include <Roster.h>
#endif

typedef struct name_val
{
	char *name;
	int   val;
}name_val;

name_val signals[] =
{
	{ "HUP",     SIGHUP   },
	{ "INT",     SIGINT   },
	{ "QUIT",    SIGQUIT  },
	{ "ILL",     SIGILL   },
	{ "CHLD",    SIGCHLD  },
	{ "ABRT",    SIGABRT  },
	{ "PIPE",    SIGPIPE  },
	{ "FPE",     SIGFPE   },
	{ "KILL",    SIGKILL  },
	{ "STOP",    SIGSTOP  },
	{ "SEGV",    SIGSEGV  },
	{ "CONT",    SIGCONT  },
	{ "TSTP",    SIGTSTP  },
	{ "ALRM",    SIGALRM  },
	{ "TERM",    SIGTERM  },
	{ "TTIN",    SIGTTIN  },
	{ "TTOU",    SIGTTOU  },
	{ "USR1",    SIGUSR1  },
	{ "USR2",    SIGUSR2  },
	{ "WINCH",   SIGWINCH },
	{ "KILLTHR", SIGKILLTHR }
};

static void
list_signals(void)
{
	int i; 
	int len = 0;
	for (i = 0; i < sizeof(signals)/sizeof(name_val); i++) {
		printf("%s ", signals[i].name);
		len += strlen(signals[i].name) + 1;
		if (len > 70) {
			len = 0;
			putchar('\n');
		}	
	}
	putchar('\n');
}

int
convert_signame(char *str)
{
	int i, len=strlen(str);

	if (isdigit(*str)) {   /* then it's just a number */
		return atoi(str);
	}

	for(i=0; i < sizeof(signals)/sizeof(name_val); i++)
		if (strncasecmp(str, signals[i].name, len) == 0)
			break;

	if (i < sizeof(signals)/sizeof(name_val))
		return signals[i].val;
	else
		return -1;
}

int
is_number(char *str)
{
	if (*str == '-') {
		str++;
		if (*str == '\0')
			return 0;
	}
	
	while(*str && isdigit(*str++))
		;

	return (*str == '\0');   /* at the end of the string, it was a number */
}

int
main(int argc, char **argv)
{
	thread_id   thid;
	thread_info	tinfo;
	app_info	ainfo;
	char       *pname = argv[0];
	int         sig = SIGTERM;

	if (argc < 2) {
		fprintf(stderr, "Usage : kill [-SIGNAME] pid1 [pid2 ...]\n");
		fprintf(stderr, "\twhere SIGNAME is one of INT, HUP, KILL, etc\n");
		exit(1);
	}
	for (argc--, argv++; argc > 0; argc--, argv++) {
		
		if (argv[0][0] == '-') {
			if (strcmp(&argv[0][1], "l") == 0) {
				list_signals();
				exit(0);
			}
			sig = convert_signame(&argv[0][1]);
			if (sig < 0) {
				fprintf(stderr, "%s: bad signal name %s\n", pname, argv[0]);
				exit(1);
			}

			argv++;  argc--;
			if (argc <= 0)
				continue;
		}

		if (is_number(argv[0]))
			thid = strtoul(argv[0], 0, 0);
		else {
			thid = find_thread(argv[0]);
			if (thid == B_NAME_NOT_FOUND) {
				fprintf(stderr, "No such thread: %s\n", argv[0]);
				continue;
			}
		}

		status_t err = send_signal(thid, sig);
		switch (err) {
		case B_OK:
			break;
		case B_BAD_THREAD_ID:
			fprintf(stderr, "kill: No such thread: %d\n", thid);
			break;
		case B_BAD_VALUE:
			fprintf(stderr, "kill: Bad signal: %d\n", sig);
			break;
		default:
			fprintf(stderr, "kill: %s\n", strerror(err));
			break;
		}
	}

	exit(0);
	return 0;
}
