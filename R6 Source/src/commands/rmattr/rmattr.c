#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fs_attr.h>
#include <AppDefs.h>
#include <errno.h>


int
main(int argc, char **argv)
{
	int fd, arg_index, err = 0;
	char *attr_name, *fname;

	

	arg_index = 1;
	attr_name = argv[arg_index++];
	if (arg_index >= argc)
		goto args_error;

	for(; arg_index < argc; arg_index++) {
		fname = argv[arg_index];

		fd = open(fname, O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "%s: can't open file %s to remove attribute\n",
					argv[0], fname);
			continue;
		}

		
		err = fs_remove_attr(fd, attr_name);
		if (err < 0) {
			fprintf(stderr, "%s: error removing attribute %s from %s: %s\n",
					argv[0], attr_name, fname, strerror(errno));
		}

		close(fd);
	}

	return 0;

 args_error:
	fprintf(stderr, "usage: %s attr filename1 [filename2...]\n", argv[0]);
	fprintf(stderr, "\tattr is the name of an attribute of the file\n");
	return 1;
}
