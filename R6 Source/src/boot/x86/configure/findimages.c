#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int find_gzip_header(char *buffer, int buffersize, const char *name)
{
	int i, len;

	len = strlen(name) + 1;

	for (i=0;i<buffersize-len-10;i++)
		if (	!memcmp(buffer + i, "\x1f\x8b\x08\x08", 4) &&
				!memcmp(buffer + i + 10, name, len))
			return i;

	return -1;
}

int main(int argc, char **argv)
{
	int fd, error, i;
	struct stat st;
	char *buffer;

	if (argc != 2) {
		fprintf(stderr, "requires exactly one argument\n");
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Unable to open %s (%s)\n", argv[1], strerror(errno));
		return 1;
	}

	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "Unable to stat %s (%s)\n", argv[1], strerror(errno));
		return 1;
	}

	buffer = malloc(st.st_size);
	if (!buffer) {
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	error = read(fd, buffer, st.st_size);
	close(fd);

	if (error < st.st_size) {
		fprintf(stderr, "Error reading from %s\n", argv[1]);
		return 1;
	}

	if ((i = find_gzip_header(buffer, st.st_size, "images")) < 0) {
		fprintf(stderr, "Could not find image pack in %s\n", argv[1]);
		return 1;
	}

	printf("%d\n", i);

	return 0;
}
