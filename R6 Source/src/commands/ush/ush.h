#ifndef _USH_H
#define _USH_H

#include <OS.h>
#include <stdio.h>

#define MAXTOKENS 128
#define MAXDEPTH 128

status_t runfile(FILE *f, int interactive);
status_t execute(int argc, char **argv);
status_t do_run(int argc, char **argv);


typedef struct 
{
	int skip[MAXDEPTH];
	long posn[MAXDEPTH];
	long cur_posn;
	int depth;
	int last_result;
	int interactive;
	FILE *fp;
} context;

typedef struct 
{
	status_t (*func)(int argc, char **argv);
	int minargs;
	const char *name;
	const char *help;
} command;

/* from readline.c */
void readline(int in, int out, const char *prompt, char *buffer);
void init_readline(int in);

#endif
