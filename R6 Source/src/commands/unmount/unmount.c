/* --------------------------------------------------------------------
	
	unmount - volume unmount tool

	Written by: Robert Polic
	
	Copyright 1995 Be, Inc. All Rights Reserved.
	
-------------------------------------------------------------------- */

#include <stdio.h>
#include <errno.h>


int
main(int argc, char **argv)
{
	int		err;

	if (argc != 2) {
		fprintf(stderr, "usage: unmount path\n");
		exit(1);
	}
	err = unmount(argv[1]);
	if (err) {
		fprintf(stderr, "unmount: %s\n", strerror(errno));
		if (errno == B_BUSY)
			exit(2);
		else
			exit(3);
	}
	return 0;
}
