/*
   this is a simple shell utility to list the contents of the index
   directory on a given volume.

   butt simple. and built to stay that way.
*/   

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <AppDefs.h>
#include <fs_index.h>
#include <errno.h>



int
main(int argc, char **argv)
{
	int         fd, type;
	struct stat st;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s index_name\n", argv[0]);
		fprintf(stderr, "\tto view index names, use lsindex\n");
		
		return 5;
	}

	if (stat(".", &st) < 0) {
		fprintf(stderr, "%s: can't open the current directory?\n", argv[0]);
		return 5;
	}

	if (fs_remove_index(st.st_dev, argv[1]) < 0) {
		fprintf(stderr, "failed to remove the index %s: %s\n", argv[1],
				strerror(errno));
		return 5;
	}
	
	return 0;
}
