#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <OS.h>

void
usage(int argc, char **argv)
{
	fprintf(stderr, "%s: a utility to mount a memfs\n", argv[0]);
	fprintf(stderr, "usage: %s <mount point> <size in bytes>[b|kb|mb(default)]\n", argv[0]);
}

int
main(int argc, char **argv)
{
	char *dir;
	size_t size;
	int rc;
	int fd;
	struct stat st;
	char inSize[64];
	int len;
	int mult = 1024*1024;
	
	if (argc < 3) {
		usage(argc, argv);
		return 1;
	}

	dir = argv[1];
	strncpy(inSize,argv[2],sizeof(inSize)-1);
	len = strlen(inSize);
	if (len > 2) {
		if (tolower(inSize[len-1]) == 'b') {
			if (tolower(inSize[len-2]) == 'k') {
				mult = 1024;
				inSize[len-2] = 0;
			} else if (tolower(inSize[len-2]) == 'm') {
				mult = 1024*1024;
				inSize[len-2] = 0;
			} else if(isdigit(inSize[len-2])) {
				mult = 1;
			} else {
				usage(argc, argv);
				return 1;
			}
			inSize[len-1] = 0;
		}
	}
	size = atoi(inSize);
	if(size <= 0) {
		fprintf(stderr, "invalid size\n");
		return 1;
	}
	size *= mult;

	if (stat(dir,&st) < 0) {
		if (mkdir(dir, S_IRWXU|S_IRWXG|S_IRWXO) < 0) {
			fprintf(stderr, "error '%s' creating mount point '%s'\n", strerror(errno), dir);
			return 1;
		}
	}

	rc = mount("memfs",dir,NULL,0,&size,sizeof(size_t));
	if (rc < 0) {
		fprintf(stderr, "error '%s' mounting memfs\n", strerror(errno));
		return 1;
	}

	return rc;
}
