
#include "prefs.h"
#include <FindDirectory.h>
#include <Path.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <Screen.h>

prefs_struct prefs;

static FILE * get_file(const char * mode)
{
	char path[1024];
	if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path, 1024)) {
		strcpy(path, "/boot/home/config/settings");
	}
	strcat(path, "/tv_settings");
	FILE * f = fopen(path, mode);
	return f;
}

void get_prefs()
{
	FILE * f = get_file("r");
	BScreen scrn;
	prefs.x = scrn.Frame().left+48;
	prefs.y = scrn.Frame().bottom-180;
	if (!f) {
		return;
	}
	char str[256];
	while (1) {
		char * ptr;
		str[0] = 0;
		fgets(str, 256, f);
		if (!str[0]) break;
		for (ptr=str; *ptr && isspace(*ptr); ptr++)
			;
		if (sscanf(ptr, "x = %f", &prefs.x) == 1)
			continue;
		if (sscanf(ptr, "y = %f", &prefs.y) == 1)
			continue;
		if (*ptr && *ptr != '#') {
			fprintf(stderr, "error in settings file: %s\n", ptr);
		}
	}
	fclose(f);
	if (prefs.x < scrn.Frame().left) {
		prefs.x = scrn.Frame().left+10;
	}
	if (prefs.x > scrn.Frame().right-10) {
		prefs.x = scrn.Frame().right-10;
	}
	if (prefs.y < scrn.Frame().top+10) {
		prefs.y = scrn.Frame().top+10;
	}
	if (prefs.y > scrn.Frame().bottom) {
		prefs.y = scrn.Frame().bottom-10;
	}
}


void put_prefs()
{
	FILE * f = get_file("w");
	if (f) {
		fprintf(f, "x = %g\ny = %g\n", prefs.x, prefs.y);
		fclose(f);
	}
}

