/* ++++++++++
	csum.c
	Copyright (C) 1992 Be Labs, Inc.  All Rights Reserved
	A quitck and dirty file checksum.

	Modification History (most recent first):
	06 apr 92	rwh	new today
+++++ */

#include <stdio.h>
#include <stdlib.h>

int csum(char* fileName, char* sumStr);

/* ----------
   checksum computes a chesksum of the passed area.  The checksum is the
   ones complement of the sum of all the longwords in the area.
----- */

static unsigned long
checksum (unsigned char *buf, unsigned long size)
{
	unsigned long temp;
	unsigned long sum = 0;

	while (size) {
		temp = 0;
		if (size > 3) {
			temp = *buf++;
			temp = (temp << 8) + *buf++;
			temp = (temp << 8) + *buf++;
			temp = (temp << 8) + *buf++;
			size -= 4;
		} else {
			while (size) {
				temp = (temp << 8) + *buf++;
				size -= 1;
			}
		}
		sum += temp;
	}

	return sum;
}

int 
csum(char* fileName, char* sumStr)
{
	FILE		*f;		/* file id */
	unsigned long	size;		/* size of file */
	unsigned long	sum = 0;	/* accumulated checksum */
	size_t		count;		/* amt to read each pass */
	static unsigned char buf[512*30];/* buffer holding file */

	if (!(f = fopen (fileName, "rb"))) {	/* file doesn't exist.. */
		fprintf(stderr, "Could not open %s\n", fileName);
		return -1;
	}
	
	/* get file size */

	if (fseek (f, 0L, SEEK_END)) {
		fprintf (stderr, "Error seeking in the file %s\n", fileName);
		goto close_exit;
	}

	if ((size = ftell (f)) == -1) {
		fprintf (stderr, "Error ftell-ing the file %s\n", fileName);
		goto close_exit;
	}

	if (size == 0) {
		fprintf (stderr, "Nothing in the file %s\n", fileName);
		goto close_exit;
	}

	rewind(f);

	while (size) {
		count = sizeof(buf);
		if (size < count)
			count = size;

		if (fread (buf, 1, count, f) != count) {
			fprintf (stderr, "Error reading in the file %s\n", fileName);
			goto close_exit;
		}
		sum += checksum (buf, count);
		size -= count;
	}

	sprintf(sumStr, "%.8lx\n", sum ^ -1);
	fclose(f);
	return 0;

close_exit:
	fclose(f);
	return -1;
}

