#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define USER
#include "bfs.h"

#include <SupportDefs.h>

int main(int argc, char **argv)
{
	FILE *fp;
	int fd, i;
	off_t len;
	int patch1_location, patch2_location, start, end;
	unsigned char buffer[0x400];

	if (argc != 3) {
		printf("usage: %s infile outfile\n       converts ld86 object to C structure for bfs\n", *argv);
		return 1;
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		printf("error opening %s (%s)\n", argv[1], strerror(fd));
		return fd;
	}

	len = lseek(fd, 0, 2) - 0x20;
	if (len != 0x400) {
		printf("%s is an invalid image (incorrect size)\n", argv[1]);
		return 1;
	}

	if (read_pos(fd, 0x20, buffer, 0x400) < 0x400) {
		printf("error reading %s\n", argv[1]);
		return 1;
	}

	close(fd);

	for (patch1_location=0;patch1_location<0x200;patch1_location++)
		if (*(uint32 *)(buffer+patch1_location) == 0xffb20272)
			break;

	if (patch1_location == 0x200) {
		printf("error finding patch1 location\n");
		return 1;
	}
	patch1_location += 3;

	for (patch2_location=0;patch2_location<0x200;patch2_location++)
		if (*(uint32 *)(buffer+patch2_location) == 0x01020304)
			break;

	if (patch2_location == 0x200) {
		printf("error finding patch2 location\n");
		return 1;
	}

	for (start=0x200;(start<0x400)&&(buffer[start]==0); start++)
		;

	if (start < 0x200 + sizeof(disk_super_block)) {
		printf("%s is an invalid image (start of second part should be at %x, not %x)\n", argv[1], 0x200 + sizeof(disk_super_block), start);
		return 1;
	}

	for (end=0x3ff;(end>=start)&&(buffer[end]==0);end--)
		;

	if ((fp = fopen(argv[2], "wb")) == NULL) {
		printf("error opening %s\n", argv[2]);
		return 1;
	}

	fprintf(fp, "int patch1_offset = 0x%x;\n", patch1_location);
	fprintf(fp, "int patch2_offset = 0x%x;\n", patch2_location);
	fprintf(fp, "\nunsigned char bootsector[] = {\n");
	for (i=0;i<end;i++) {
		fprintf(fp, "0x%02x,", buffer[i]);
		if ((i & 0x0f) == 0x0f) fprintf(fp, "\n");
	}
	fprintf(fp, "0x%02x\n};\n", buffer[i]);

	fclose(fp);

	return 0;
}
