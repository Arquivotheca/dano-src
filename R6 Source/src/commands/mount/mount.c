/*
 *
 * Copyright (C) 1997 by Be, Inc.  All Rights Reserved.
 *
 * Unix-compatible mount command
 *
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>


void
usage(const char *myname)
{
	fprintf(stderr, "usage: mount [-ro] [-t fstype] device directory\n", myname);
	exit(1);
}

int
main(int argc, char **argv)
{
	char *myname = argv[0];
	char *fstype = "bfs";
	char *dirname;
	char *devname;
	int status;
	int flags = 0;
    struct stat st;

	for (argv++, argc--; argc > 0 && **argv == '-'; argv++, argc--) {
		if (strcmp(*argv, "-t") == 0) {
			if (argc < 2) {
				usage(myname);
			}
			fstype = argv[1];
			argv++, argc--;
		} else if (!strcmp(*argv, "-ro")) {
			flags = B_MOUNT_READ_ONLY;
		} else {
			usage(myname);
		}
	}
	if (argc != 2) {
		usage(myname);
	}
	devname = argv[0];
	dirname = argv[1];
	if (stat(dirname, &st) < 0) {
		fprintf(stderr, "mount: The mount point %s is not accessible\n", dirname);
        exit(5);
    }

	status = mount(fstype, dirname, devname, flags, NULL, 0);
	if (status < 0) {
		fprintf(stderr, "mount: %s\n", strerror(errno));
		exit(1);
	}
	exit(0);
}
