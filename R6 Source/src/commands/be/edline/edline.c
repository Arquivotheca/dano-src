
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#if !defined(bool)
#define bool unsigned char
#endif
#if !defined(false)
#define false 0
#endif
#if !defined(true)
#define true 1
#endif

int curline = 1;	//	one-based line numbering
int nlines;
char **lines;	//	the first line is NULL for convenience
bool dirty;
bool running = true;
bool interactive;
char filename[1024];

extern void help(char *, int, int);
extern void escape(char *, int, int);
extern void copy_lines(char *, int, int);
extern void delete_lines(char *, int, int);
extern void goto_line(char *, int, int);
extern void insert_lines(char *, int, int);
extern void list_lines(char *, int, int);
extern void quit(char *, int, int);
extern void read_lines(char *, int, int);
extern void write_lines(char *, int, int);


static bool
checkline(
	int n)
{
	if (n < 1)
	{
		fprintf(stderr, "%d: lines start at 1\n", n);
		return false;
	}
	if (n > nlines+1)
	{
		fprintf(stderr, "%d: there are only %d lines\n", n, nlines);
		return false;
	}
	return true;
}

static bool
insert_line(
	const char * line,
	int atline)
{
	char ** nl;
	char * d;
	int ix;

	nl = (char **)realloc(lines, (nlines+2)*sizeof(char *));
	if (!nl)
	{
		fprintf(stderr, "out of memory");
		return false;
	}
	lines = nl;
	d = strdup(line);
	if (!d)
	{
		fprintf(stderr, "out of memory");
		return false;
	}
	dirty = true;
	for (ix=nlines+1; ix>atline; ix--)
	{
		lines[ix] = lines[ix-1];
	}
	lines[atline] = d;
	nlines++;
	return true;
}

static bool
checkfile(
	const char * path)
{
	struct stat st;
	if ((stat(path, &st) < 0) || !S_ISREG(st.st_mode))
	{
		fprintf(stderr, "%s: no such file\n", path);
		return false;
	}
	return true;
}

struct cmd
{
	char			letter;
	const char *	help;
	void			(*func)(char *, int, int);
}
commands[] = {
	{ '?', "? -- help; ? x -- help on x", help },
	{ '!', "!cmd -- run cmd in a sub-shell", escape },
	{ 'c', "c x [y] -- copy x lines from [y/current line] to current line", copy_lines },
	{ 'd', "d [x [y]/a] -- delete [x/all/1] lines from [y/current line]", delete_lines },
	{ 'g', "g x -- make line x current line", goto_line },
	{ 'i', "i [x] -- insert text at line [x/current line]", insert_lines },
	{ 'l', "l [x [y]] -- list [x/11] lines of text from [y/current line-5]", list_lines },
	{ 'q', "q -- quit", quit },
	{ 'r', "r filename -- insert a file at current line", read_lines },
	{ 'w', "w [filename] -- write the file out", write_lines },
};

void help(char * str, int x, int y)
{
	int ix;
	for (ix=0; ix<sizeof(commands)/sizeof(commands[0]); ix++)
	{
		if (!*str || (*str == commands[ix].letter))
		{
			fprintf(stderr, "%c: %s\n", commands[ix].letter, commands[ix].help);
			if (*str) return;
		}
	}
	if (*str)
	{
		fprintf(stderr, "'%c': unknown command. Use '?' for help.\n", *str);
	}
}

void escape(char * str, int x, int y)
{
	if (!*str)
	{
		fprintf(stderr, "null command\n");
		return;
	}
	if (system(str) < 0)
	{
		fprintf(stderr, "%s: returned with error status\n", str);
	}
}

void copy_lines(char *str, int count, int toline)
{
	if (count < 1) count = 1;
	if (toline < 1) toline = curline;
	fprintf(stderr, "copy_lines(): not implemented\n");
}

void delete_lines(char *str, int count, int fromline)
{
	int top;
	int ix;

	if (str[0] == 'a')
	{
		for (ix=1; ix<=nlines; ix++)
		{
			free(lines[ix]);
		}
		nlines = 0;
		return;
	}
	if (count < 1) count = 1;
	if (fromline < 1) fromline = curline;

	if (!checkline(fromline)) return;
	dirty = true;
	for (ix=fromline; ix<fromline+count; ix++)
	{
		free(lines[ix]);
	}
	for (ix=fromline; ix<nlines+1-count; ix++)
	{
		lines[ix] = lines[ix+count];
	}
	nlines -= count;
}

void goto_line(char *str, int nuline, int y)
{
	if (nuline < 1) nuline = curline;
	if (!checkline(nuline)) return;
	curline = nuline;
}

void insert_lines(char *str, int atline, int y)
{
	char line[1024];

	if (atline < 1) atline = curline;
	if (!checkline(atline)) return;
	curline = atline;

	while (true)
	{
		if (interactive)
		{
			fprintf(stdout, "%d:", curline);
			fflush(stdout);
		}
		line[0] = 0;
		fgets(line, 1024, stdin);
		if (!line[0]) break;
		if (!insert_line(line, curline)) break;
		curline++;
	}
	fprintf(stdout, "<EOT>\n");
	fflush(stdout);
}

void list_lines(char *str, int count, int fromline)
{
	if (count < 1) count = 11;
	if (fromline < 1) fromline = curline-5;
	if (fromline < 1) fromline = 1;

	if (!checkline(fromline)) return;
	while ((fromline <= nlines) && (count-- > 0))
	{
		fprintf(stdout, "%d%c%s", fromline, (fromline == curline) ? '@' : ':', lines[fromline]);
		fromline++;
	}
}

void quit(char *str, int x, int y)
{
	if (!dirty || (str[0] == '!'))
	{
		running = false;
	}
	else
	{
		fprintf(stderr, "warning: File is dirty. Use q! to override.\n");
	}
}

void read_lines(char *str, int x, int y)
{
	FILE * f;
	char line[1024];

	if (!checkfile(str))
	{
		return;
	}
	f = fopen(str, "r");
	if (!f)
	{
		fprintf(stderr, "%s: cannot open\n", str);
		return;
	}
	if (nlines == 0)
	{
		strcpy(filename, str);
	}
	while (true)
	{
		line[0] = 0;
		fgets(line, 1024, f);
		if (!line[0]) break;
		if (!insert_line(line, curline)) break;
		curline++;
	}
	fclose(f);
	dirty = true;
}

void write_lines(char *str, int x, int y)
{
	FILE * f;
	int ix;

	if (!*str) str = filename;
	if (!*str)
	{
		fprintf(stderr, "error: No file name.\n");
		return;
	}
	f = fopen(str, "w");
	if (f == NULL)
	{
		fprintf(stderr, "%s: cannot write.\n", str);
		return;
	}
	for (ix=1; ix<=nlines; ix++)
	{
		fprintf(f, "%s", lines[ix]);
	}
	fclose(f);
	if (str != filename) strcpy(filename, str);
	dirty = false;
}



int
main(
	int argc,
	char * argv[])
{
	char line[1024];
	lines = (char **)malloc(sizeof(char *));
	lines[0] = NULL;	//	we don't use the first line
	if ((argc > 2) || ((argc == 2) && (argv[1][0] == '-'))) {
		fprintf(stderr, "usage: edline [filename]\n");
		return 1;
	}
	if (argv[1]) {
		read_lines(argv[1], 0, 0);
		curline = 1;
		dirty = false;
	}
	interactive = isatty(0);
	while (running) {
		int ix;
		char * s = &line[1];
		int a = 0, b = 0;
		char * e;

		line[0] = 0;
		if (interactive)
		{
			fprintf(stdout, "edline> ");
			fflush(stdout);
		}
		fgets(line, 1024, stdin);
		if (line[0] == '\n') continue;
		if (!line[0])
		{
			fprintf(stderr, "end of input -- quitting\n");
			break;
		}
		while (*s && isspace(*s)) s++;
		e = s+strlen(s)-1;
		if (*e == '\n') *e = 0;
		sscanf(s, " %d %d", &a, &b);
		for (ix=0; ix<sizeof(commands)/sizeof(commands[0]); ix++)
		{
			if (commands[ix].letter == line[0])
			{
				(*commands[ix].func)(s, a, b);
				goto cmddone;
			}
		}
		fprintf(stderr, "%c: unknown command. Use ? for help.\n", line[0]);
cmddone:
		;
	}
	return 0;
}
