
#ifndef MAIN_H
#define MAIN_H

#include <OS.h>

#define BEINSTALL_MODE_FILE 0
#define BEINSTALL_MODE_FILES 1
#define BEINSTALL_MODE_DIRECTORY 2

#define NO_MODE_OVERRIDE ((unsigned int)-1)

struct beinstall_options {
	int install_mode;
	unsigned int pmode;
	int use_pmode;
	unsigned int powner;
	int use_powner;
	unsigned int pgroup;
	int use_pgroup;
	int directories_if_needed;
	int dereference_links;
	int ignore_attributes;
	int preserve_timestamps;
	int recursive;
	int force;
	int verbose;
	char **sources;
	char *dest;
	int default_pmode;
};

extern const char *argv0;

extern void error(const char *text);

/*** in copyattr.c ***/
extern int copyattr_start(struct beinstall_options *opt);

/* copy_posix_attr is also used by create_dir when the directory already exists */
status_t
copy_posix_attr(
	const char *	source,
	const char *	destination,
	struct beinstall_options *opt,
	unsigned int mode);

/*** in create_dir.c ***/

int
create_directory(
	const char * path,
	struct beinstall_options *opt,
	unsigned int mode,
	int make_parent);

#endif
