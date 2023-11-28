/* ++++++++++
	hd.c
	Copyright (C) 1992 Be Labs, Inc.  All Rights Reserved
	A hex dump utility.

	Modification History (most recent first):
	18 jun 92	rwh	new today
+++++ */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE (512*16*4)

static char buf[BUFSIZE];		/* file buffer */

/* ----------

	Printable

	returns a printable translation of the passed character.

----- */

unsigned char
Printable(unsigned char c)
{
	return (c >= ' ' && c <= '~') ? c : '.';
}


/* ----------

	DisplayLine

	Display a line of memory.

----- */


void
DisplayLine(off_t offset, char *loc, long count, int num_bytes)
{
	int 		i;
	char		b[100];

	if (count > 16)
		count = 16;

	printf ("%.8Lx   ", offset);
	for (i=0; i < count; i++) {
		printf ("%.2x", loc[i] & 0xff);
		if (((i+1) % num_bytes) == 0)
			printf (" ");
		if (num_bytes == 1 && i == 7)
			printf(" ");
	}

	i = 3*16 + ((count > 7) ? 1 : 2) - 3*count;

	memset (b, ' ', i);
	b[i] = 0;
	printf ("%s",b);
	for (i = 0; i < count; i++)
		printf("%c", Printable(loc[i]));
	printf ("\n");
}


void
usage(char *prog)
{
	fprintf(stderr, "Usage:  %s [-n N]  [file]\n", prog);
	fprintf(stderr, "\t-n expects a number between 1 and 16 and specifies\n");
	fprintf(stderr, "\t   the number of bytes between spaces.\n");
	fprintf(stderr, "\n\tIf no file is specified, input is read from stdin\n");

	exit(0);
}


main(int argc, char **argv)
{
	FILE	*f = NULL;		/* file id */
	off_t	offset = 0;		/* offset in file */
	char	*bp;			/* -> into file buffer */
	int		i, n;			/* n == bytes left in buffer */
    int     amt_read;
	char	line_buf [16];	/* current run, or last line */
	char	*last;			/* -> line to compare to for runs */
	int		in_run;			/* flag: in a run of equal lines */
	int     num_bytes = 1;  /* # of bytes between spaces */

	for(i=1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
			usage(argv[0]);
		else if (strcmp(argv[i], "-n") == 0) {
			if (argv[i+1] && isdigit(argv[i+1][0])) {
				num_bytes = strtoul(argv[i+1], NULL, 0);
				if (num_bytes <= 0 || num_bytes > 16) {
					fprintf(stderr, "num bytes %d not valid, must be 1..16\n",
							num_bytes);
					num_bytes = 1;
				}
				i++;
			} else {
				fprintf(stderr, "%s: -n option needs a numeric argument\n",
						argv[0]);
				exit(5);
			}
		} else if (!(f = fopen (argv[i], "rb"))) {	/* file doesn't exist.. */
			fprintf(stderr, "Could not open %s\n", argv[i]);
			return -1;
		}
	}
		
	if (argc == 1) {
		f = stdin;
	}
		

	while (1) {
		n = BUFSIZE;
		
		if ((amt_read = fread (buf, 1, n, f)) < 0) {
			fprintf (stderr, "Error reading file %s: %s\n", argv[1],
					 strerror(errno));
			goto close_exit;			
		}

		if (amt_read == 0) {
			if(ferror(f))
				fprintf (stderr, "Error reading file %s: %s\n", argv[1],
						 strerror(errno));
			break;
		}

		n = amt_read;
		/* if first line, init to 'not in run' */
		if (!offset) {
			line_buf[0] = buf[0] + 1;	/* ensure no match */
			last = line_buf;
		}
			
		for (bp = buf; n > 0; bp += 16, offset += 16, n -= 16) {

			/* always show last line */
			if (n <= 16 && !in_run) {
				DisplayLine (offset, bp, n, num_bytes);
				offset += n;
				break;
			}
			
			if (!memcmp(last, bp, 16)) {	
				if (in_run)
					continue;
				else {
					in_run = 1;
					memcpy (line_buf, bp, 16);
					last = line_buf;
					printf ("...\n");
				}
			} else {
				DisplayLine (offset, bp, 16L, num_bytes);
				last = bp;
				in_run = 0;
			}
		}
		/* preserve last line of buffer if not in a run */
		if (!in_run) {
			memcpy (line_buf, bp, 16);
			last = line_buf;
		}
	}

close_exit:
	fclose(f);
	return -1;
}





