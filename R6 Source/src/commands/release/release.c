/* ++++++++++
	$Source: /net/bally/be/rcs/src/commands/release.c,v $
	$Revision: 1.3 $
	$Author: btaylor $
	$Date: 1997/02/26 17:23:57 $
	Copyright (c) 1994-1997 by Be Incorporated.  All Rights Reserved.

	release a semaphore from the command line.
+++++ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>

static void
usage(void)
{
	printf("usage: release [-f] [-c count] semaphore_number\n");
	printf("warning: this can be hazardous...\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int sem;
	char *end;
	char *sem_name;
	sem_info seminfo;
	thread_info thinfo;
	status_t status;
	int count = 1;
	int force = 0;

	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		if (strcmp(argv[0], "-f") == 0) {
			force++;
		} else if (strcmp(argv[0], "-c") == 0) {
			if (argc < 2) {
				usage();
			}
			count = atoi(argv[1]);
			argc--, argv++;
		} else {
			usage();
		}
	}
		
	if (argc != 1) {
		usage();
	}
	sem_name = argv[0];
	sem = strtol(argv[0], &end, 0);
	if (end != sem_name + strlen (sem_name)) {
		printf("bad number syntax: '%s'\n", sem_name);
		usage();
	}
	printf("releasing semaphore %d\n", sem);

	if (force) {
		get_thread_info(find_thread(NULL), &thinfo);
		get_sem_info(sem, &seminfo);
		set_sem_owner(sem, thinfo.team);
	}
	status = release_sem_etc(sem, count, 0);
	if (force) {
		set_sem_owner(sem, seminfo.team);
	}
	if (status < B_OK) {
		fprintf(stderr, "release_sem failed: %s\n", strerror(status));
		exit(1);
	}
	exit(0);
}
