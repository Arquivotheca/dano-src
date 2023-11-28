#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/param.h>

#define FORCE_DELETE_FILE  82969


int
main(int argc, char **argv)
{
	int fd, i;
	char *fname, *ptr, dir_buff[MAXPATHLEN];

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		exit(5);
	}

	for(i=1; i < argc; i++) {
		ptr = strrchr(argv[i], '/');
		if (ptr) {
			*ptr = NULL;
			strcpy(dir_buff, argv[i]);
			ptr++;  /* so it points to the file name */
		} else {
			ptr = argv[i];
			strcpy(dir_buff, ".");
		}
		fd = open(dir_buff, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "%s: can't open %s: %s\n", argv[0], argv[i],
					strerror(errno));
			continue;
		}

		printf("forcing removal of: %s/%s\n", dir_buff, ptr);
		ioctl(fd, FORCE_DELETE_FILE, ptr);
		close(fd);
	}

	return 0;
}
