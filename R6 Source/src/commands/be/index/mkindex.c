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
#include <TypeConstants.h>
#include <fs_index.h>
#include <errno.h>


int
str_to_type(char *str)
{
	if (strcasecmp(str, "int") == 0)
		return B_INT32_TYPE;
	else if (strcasecmp(str, "llong") == 0)
		return B_INT64_TYPE;
	else if (strcasecmp(str, "string") == 0)
		return B_STRING_TYPE;
	else if (strcasecmp(str, "ascii") == 0)
		return B_STRING_TYPE;
	else if (strcasecmp(str, "text") == 0)
		return B_STRING_TYPE;
	else if (strcasecmp(str, "float") == 0)
		return B_FLOAT_TYPE;
	else if (strcasecmp(str, "double") == 0)
		return B_DOUBLE_TYPE;
	else
		return 0;
}

void
usage(char *pname)
{
	fprintf(stderr, "Usage: %s [-t type] index_name\n", pname);
	fprintf(stderr, "\twhere index_name is an attribute name and type\n");
	fprintf(stderr, "\tis one of int, llong, string, float or double.\n");
		
	exit(5);
}

int
main(int argc, char **argv)
{
	int         fd, type = B_STRING_TYPE, arg_index = 1;
	struct stat st;

    if (argc == 1)
		usage(argv[0]);

	if (stat(".", &st) < 0) {
		fprintf(stderr, "%s: can't open the current directory?\n", argv[0]);
		return 5;
	}

	if (strcmp(argv[arg_index], "-t") == 0) {
		if (argv[arg_index+1] == NULL)
			usage(argv[0]);
		
		type = str_to_type(argv[arg_index+1]);
		if (type == 0) {
			fprintf(stderr, "%s: attribute type %s is not valid\n", argv[0],
					argv[arg_index+1]);
			fprintf(stderr, "\tTry one of: int, llong, string, float or double\n");
			return 1;
		}

		arg_index += 2;
	}

	if (arg_index >= argc) {
		usage(argv[0]);
	}


	/* XXXdbg -- what about setting the flags (allow duplicates, etc) */

	if (fs_create_index(st.st_dev, argv[arg_index], type, 0) < 0) {
		fprintf(stderr, "failed to create the index %s: %s\n", argv[1],
				strerror(errno));
		return 5;
	}
	
	return 0;
}
