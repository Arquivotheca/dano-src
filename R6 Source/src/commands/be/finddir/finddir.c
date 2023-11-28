/*
 * finddir.c
 *
 *   A command line interface to the find_directory(...) library call.
 *   It's not pretty, but it works.
 *
 *   by Nathan Schrenk (nschrenk@be.com) 08/09/1999
 */

#include <FindDirectory.h>
#include <StorageDefs.h>
#include <fs_info.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	const char *name;
	int32 code;
} finddir_entry;

const finddir_entry directories[] = {
	{ "B_DESKTOP_DIRECTORY",			B_DESKTOP_DIRECTORY },
	{ "B_TRASH_DIRECTORY",				B_TRASH_DIRECTORY },
	{ "B_BEOS_DIRECTORY",				B_BEOS_DIRECTORY },
	{ "B_BEOS_SYSTEM_DIRECTORY",		B_BEOS_SYSTEM_DIRECTORY },
	{ "B_BEOS_ADDONS_DIRECTORY",		B_BEOS_ADDONS_DIRECTORY },
	{ "B_BEOS_BOOT_DIRECTORY",			B_BEOS_BOOT_DIRECTORY },
	{ "B_BEOS_FONTS_DIRECTORY",			B_BEOS_FONTS_DIRECTORY },
	{ "B_BEOS_LIB_DIRECTORY",			B_BEOS_LIB_DIRECTORY },
	{ "B_BEOS_SERVERS_DIRECTORY",		B_BEOS_SERVERS_DIRECTORY },
	{ "B_BEOS_APPS_DIRECTORY",			B_BEOS_APPS_DIRECTORY },
	{ "B_BEOS_BIN_DIRECTORY",			B_BEOS_BIN_DIRECTORY },
	{ "B_BEOS_ETC_DIRECTORY",			B_BEOS_ETC_DIRECTORY },
	{ "B_BEOS_ETC_DIRECTORY",			B_BEOS_ETC_DIRECTORY },
	{ "B_BEOS_DOCUMENTATION_DIRECTORY",	B_BEOS_DOCUMENTATION_DIRECTORY },
	{ "B_BEOS_PREFERENCES_DIRECTORY",	B_BEOS_PREFERENCES_DIRECTORY },
	{ "B_BEOS_TRANSLATORS_DIRECTORY",	B_BEOS_TRANSLATORS_DIRECTORY },
	{ "B_BEOS_MEDIA_NODES_DIRECTORY",	B_BEOS_MEDIA_NODES_DIRECTORY },
	{ "B_BEOS_SOUNDS_DIRECTORY",		B_BEOS_SOUNDS_DIRECTORY },
	{ "B_COMMON_DIRECTORY",				B_COMMON_DIRECTORY },
	{ "B_COMMON_SYSTEM_DIRECTORY",		B_COMMON_SYSTEM_DIRECTORY },
	{ "B_COMMON_ADDONS_DIRECTORY",		B_COMMON_ADDONS_DIRECTORY },
	{ "B_COMMON_BOOT_DIRECTORY",		B_COMMON_BOOT_DIRECTORY },
	{ "B_COMMON_FONTS_DIRECTORY",		B_COMMON_FONTS_DIRECTORY },
	{ "B_COMMON_LIB_DIRECTORY", 		B_COMMON_LIB_DIRECTORY },
	{ "B_COMMON_SERVERS_DIRECTORY",		B_COMMON_SERVERS_DIRECTORY },
	{ "B_COMMON_BIN_DIRECTORY",			B_COMMON_BIN_DIRECTORY },
	{ "B_COMMON_ETC_DIRECTORY",			B_COMMON_ETC_DIRECTORY },
	{ "B_COMMON_DOCUMENTATION_DIRECTORY",	B_COMMON_DOCUMENTATION_DIRECTORY },
	{ "B_COMMON_SETTINGS_DIRECTORY",	B_COMMON_SETTINGS_DIRECTORY },
	{ "B_COMMON_DEVELOP_DIRECTORY",		B_COMMON_DEVELOP_DIRECTORY },
	{ "B_COMMON_LOG_DIRECTORY",			B_COMMON_LOG_DIRECTORY },
	{ "B_COMMON_SPOOL_DIRECTORY",		B_COMMON_SPOOL_DIRECTORY },
	{ "B_COMMON_TEMP_DIRECTORY",		B_COMMON_TEMP_DIRECTORY },
	{ "B_COMMON_VAR_DIRECTORY",			B_COMMON_VAR_DIRECTORY },
	{ "B_COMMON_TRANSLATORS_DIRECTORY", B_COMMON_TRANSLATORS_DIRECTORY },
	{ "B_COMMON_MEDIA_NODES_DIRECTORY", B_COMMON_MEDIA_NODES_DIRECTORY },
	{ "B_COMMON_SOUNDS_DIRECTORY",		B_COMMON_SOUNDS_DIRECTORY },
	{ "B_USER_DIRECTORY",				B_USER_DIRECTORY },
	{ "B_USER_CONFIG_DIRECTORY",		B_USER_CONFIG_DIRECTORY },
	{ "B_USER_ADDONS_DIRECTORY",		B_USER_ADDONS_DIRECTORY },
	{ "B_USER_BOOT_DIRECTORY",			B_USER_BOOT_DIRECTORY },
	{ "B_USER_FONTS_DIRECTORY",			B_USER_FONTS_DIRECTORY },
	{ "B_USER_LIB_DIRECTORY",			B_USER_LIB_DIRECTORY },
	{ "B_USER_SETTINGS_DIRECTORY",		B_USER_SETTINGS_DIRECTORY },
	{ "B_USER_DESKBAR_DIRECTORY",		B_USER_DESKBAR_DIRECTORY },
	{ "B_USER_PRINTERS_DIRECTORY",		B_USER_PRINTERS_DIRECTORY },
	{ "B_USER_TRANSLATORS_DIRECTORY",	B_USER_TRANSLATORS_DIRECTORY },
	{ "B_USER_MEDIA_NODES_DIRECTORY",	B_USER_MEDIA_NODES_DIRECTORY },
	{ "B_USER_SOUNDS_DIRECTORY",		B_USER_SOUNDS_DIRECTORY },
	{ "B_APPS_DIRECTORY",				B_APPS_DIRECTORY },
	{ "B_PREFERENCES_DIRECTORY",		B_PREFERENCES_DIRECTORY },
	{ "B_UTILITIES_DIRECTORY",			B_UTILITIES_DIRECTORY },
	{ NULL, -1 }
};

void usage(int argc, char *argv[])
{
	fprintf(stderr, "usage:	%s [ -v volume ] directory_which\n", argv[0]);
	fprintf(stderr, "        -v <file>   use the specified volume for directory\n");
	fprintf(stderr, "                    constants that are volume-specific.\n");
	fprintf(stderr, "                    <file> can be any file on that volume.\n");
	fprintf(stderr, "                    defaults to the boot volume.\n");
	fprintf(stderr, " For a description of recognized directory_which constants,\n");
	fprintf(stderr, " see the find_directory(...) documentation in the Be Book.\n");
}

int main(int argc, char *argv[])
{
	int argIndex = 1;
	int i = 0;
	char *dirWhich;
	char *volFile = "/boot";
	int32 dirWhichConstant = -1;
	char path[B_PATH_NAME_LENGTH];
	dev_t vol;

	if (argc != 2 && argc != 4) {
		usage(argc, argv);
		exit(-1);
	}
	
	if (!strcmp("-v", argv[1])) {
		volFile = argv[2];
		argIndex = 3;
	}

	vol = dev_for_path(volFile);
	dirWhich = argv[argIndex];
	
	while (directories[i].name != NULL) {
		if (!strcmp(directories[i].name, dirWhich)) {
			dirWhichConstant = directories[i].code;
			break;
		}
		i++;
	}
	
	if (dirWhichConstant != -1) {
		if (find_directory(dirWhichConstant, vol, false, path, B_PATH_NAME_LENGTH) == B_OK)
		{
			printf("%s\n", path);
		} else {
			fprintf(stderr, "%s: unexpected find_directory() error!\n", argv[0]);
			exit(-3);
		}
	} else {
		fprintf(stderr, "%s: unrecognized directory_which constant \'%s\'\n", argv[0], dirWhich);
		exit(-4);
	}

	return 0;
}
