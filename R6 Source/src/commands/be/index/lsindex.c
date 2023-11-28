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
#include <time.h>


char *
type_to_str(uint32 type)
{
	static char buff[128];

	if (type == B_STRING_TYPE) {
		strcpy(buff, "Text");
	} else if (type == B_INT32_TYPE) {
		strcpy(buff, "Int-32");
	} else if (type == B_INT64_TYPE) {
		strcpy(buff, "Int-64");
	} else if (type == B_FLOAT_TYPE) {
		strcpy(buff, "Float");
	} else if (type == B_DOUBLE_TYPE) {
		strcpy(buff, "Double");
	} else {
		sprintf(buff, "Unknown type (0x%x)", type);
	}

	return &buff[0];
}


int
main(int argc, char **argv)
{
	int     fd, long_mode = 0;
	char   *to_stat;
	struct stat st;
	DIR    *dp;
	index_info ii;
	struct dirent *de;
	char   tbuff[64];
	struct tm *tm;

	if (argv[1] && strcmp(argv[1], "-l") == 0) {
		long_mode = 1;
		argv++;
		argc--;
	}

	if(argv[1]) {
		to_stat= argv[1];
	} else {
		to_stat= ".";
	}

	if (stat(to_stat, &st) < 0) {
		fprintf(stderr, "%s: can't open the current directory?\n", argv[0]);
		return 5;
	}

	dp = fs_open_index_dir(st.st_dev);
	if (dp == NULL) {
		fprintf(stderr, "%s: device %d has no index directory!\n", argv[0],
				st.st_dev);
		return 5;
	}

	while((de = fs_read_index_dir(dp)) != NULL) {
		if (long_mode) {
			if (fs_stat_index(st.st_dev, de->d_name, &ii) < 0) {
				printf("can't stat index %s\n", de->d_name);
				continue;
			}
			
			tm = localtime(&ii.modification_time);
			strftime(tbuff, sizeof(tbuff), "%m/%d/%y %I:%M %p", tm);

			printf("%16s  %s  %8Ld %s\n", type_to_str(ii.type), tbuff,ii.size, 
				   de->d_name);
		} else {
			printf("%s\n", de->d_name);
		}
	}

	fs_close_index_dir(dp);
	
	return 0;
}
