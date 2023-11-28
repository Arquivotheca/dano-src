/*	prefs.cpp	*/

#include "prefs.h"

#include <stdio.h>
#include <string.h>
#include <FindDirectory.h>
#include <Path.h>
#include <ctype.h>

static prefs def_prefs = {
	true
};

prefs g_prefs;

static const char * ini_name = "SHOWIMAG.INI";	/* this is a joke :-) */


static bool
get_bool(
	const char * val)
{
	const char * trues[] = {
		"yes", "on", "true", NULL
	};
	const char **ptr = trues;
	while (*ptr) {
		if (!strcasecmp(val, *ptr)) {
			return true;
		}
		ptr++;
	}
	return false;
}

static void
got_value(
	const char * var,
	const char * val)
{
	if (!strcmp(var, "dither")) {
		g_prefs.dither = get_bool(val);
	}
	else {
		fprintf(stderr, "unknown variable %s in %s file\n", var, ini_name);
	}
}

void
init_prefs()
{
	g_prefs = def_prefs;
	BPath dir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dir) < B_OK) {
		dir.SetTo("/tmp");
	}
	dir.Append(ini_name);
	FILE * f = fopen(dir.Path(), "r");
	if (f) {
		char line[1024];
		char word[256];
		char value[768];
		while (true) {
			char * lptr = line;
			line[0] = 0;
			fgets(line, 1024, f);
			if (!line[0]) 
				break;
			while (*lptr && isspace(*lptr)) lptr++;
			if (lptr[0] == '#' || lptr[0] == '[' || !lptr[0])
				continue; /* comment */
			int elem = sscanf(lptr, "%255[^= \t\n\r] = %767[^\n\r]", word, value);
			if (elem == 0) 
				continue;
			else if (elem == 1)
				fprintf(stderr, "syntax error in %s: '%s'", dir.Path(), line);
			else
				got_value(word, value);
		}
		fclose(f);
	}
}


void
save_prefs()
{
	BPath dir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dir) < B_OK) {
		dir.SetTo("/tmp");
	}
	dir.Append(ini_name);
	FILE * f = fopen(dir.Path(), "w");
	if (!f) 
		return;
	fprintf(f, "[SHOWIMAGE_v1]\n");
	fprintf(f, "dither = %s\n", g_prefs.dither ? "yes" : "no");
	fclose(f);
}
