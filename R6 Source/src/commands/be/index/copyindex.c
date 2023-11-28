/*
   this is a simple shell utility to copy the contents of the index
   directory of a given volume to another volume.

   just to ease my migration to EXP
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


static
void
usage(char const *prog)
{
	fprintf(stderr, "Usage: %s [-v] src_path dst_path\n");
}

static
char *
type_to_str(char *buff, uint32 type)
{
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
	int     fd;
	bool   verbose_mode = false;
	char   *progname;
	char   *src;
	char   *dst;
	struct stat st_src;
	struct stat st_dst;
	DIR    *dp_src;
	DIR    *dp_dst;
	index_info ii_src;
	index_info ii_dst;
	struct dirent *de;
	char   tbuff_src[64];
	char   tbuff_dst[64];
	struct tm *tm;

	progname= argv[0];

	if (argv[1] && strcmp(argv[1], "-v") == 0) {
		verbose_mode = 1;
		argv++;
		argc--;
	}

	if(argc< 3) {
		usage(progname);
		return 1;
	}

	src= argv[1];
	dst= argv[2];

	if(stat(src, &st_src) < 0) {
		fprintf(stderr, "%s: can't open the source path?\n", argv[0]);
		return 5;
	}

	if(stat(dst, &st_dst) < 0) {
		fprintf(stderr, "%s: can't open the destination path?\n", argv[0]);
		return 5;
	}

	dp_src = fs_open_index_dir(st_src.st_dev);
	if (dp_src == NULL) {
		fprintf(stderr, "%s: device %d has no index directory!\n", argv[0],
				st_src.st_dev);
		return 5;
	}

	dp_dst = fs_open_index_dir(st_dst.st_dev);
	if (dp_dst == NULL) {
		fprintf(stderr, "%s: device %d has no index directory!\n", argv[0],
				st_dst.st_dev);
		return 5;
	}

	while((de = fs_read_index_dir(dp_src)) != NULL) {
		if (fs_stat_index(st_src.st_dev, de->d_name, &ii_src) < 0) {
			printf("can't stat index %s\n", de->d_name);
			continue;
		}

		if (fs_stat_index(st_dst.st_dev, de->d_name, &ii_dst) >= 0) {
			if (ii_src.type != ii_dst.type) {
				printf(
					"WARNING: index %s(%s) already exist in target volume but different type (%s),"
					"skipping\n",
					de->d_name,
					type_to_str(tbuff_src, ii_src.type),
					type_to_str(tbuff_dst, ii_dst.type)
				);
				continue;
			}
		} else {
			if(verbose_mode) {
				printf("creating: %s\n", de->d_name);
			}
			fs_create_index(st_dst.st_dev, de->d_name, ii_src.type, 0);
		}
	}

	fs_close_index_dir(dp_src);
	fs_close_index_dir(dp_dst);
	
	return 0;
}
