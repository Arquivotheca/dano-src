/* ++++++++++
	unchop.c
	Copyright (C) 1992 Be Inc.  All Rights Reserved.
	Piece a chopped up file back together.

	Modification History (most recent first):
	20 aug 92	rwh	new today
+++++ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* buffer size must be less than max DOS integer, hence 31 instead of 32 */
#define BUFSIZE (31*1024)

static char buf[BUFSIZE];
static int seq = 0;
static char name_buf[256];

/* ----------
	chunk_file_name returns the next chunk file name;
----- */
char *
chunk_file_name (name)
	char *name;
{
	strcpy (name_buf, name);
	sprintf (name_buf + strlen(name), "%.2d", seq++);
	return name_buf;
}

main(argc, argv)
	int argc;
	char **argv;
{
	char *prog, *file;
	long n_bytes, n;
	char *cname;
	FILE *f, *fc;


	/* parse command line options */

	prog = argv[0];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s file\n", prog);
		fprintf (stderr, "Concatenates files named file00, file01... into file\n");
		return -1;
	}

	file = argv[1];

	
	/* check for 1st chunk file */
	cname = chunk_file_name(file);
	if (!(fc = fopen (cname, "rb"))) {	/* file doesn't exist.. */
		fprintf(stderr, "No chunk files present (%s)\n", cname);
		return -1;
	}
		
	/* create merged file */
	if (!(f = fopen (file, "wb"))) {
		fprintf (stderr, "Problem creating %s - aborting\n", file);
		fclose (fc);
		return -1;
	}

	for (;;) {
		/* get chunk file size */

		if (fseek (fc, 0L, SEEK_END)) {
			fprintf (stderr, "Error seeking in the file %s\n", cname);
			goto close_exit;
		}

		if ((n_bytes = ftell (fc)) == -1) {
			fprintf (stderr, "Error ftell-ing the file %s\n", cname);
			goto close_exit;
		}

		if (fseek (fc, 0L, SEEK_SET)) {
			fprintf (stderr, "Error seeking in the file %s\n", cname);
			goto close_exit;
		}

		while (n_bytes) {
			n = (n_bytes > BUFSIZE) ? BUFSIZE : n_bytes;
			if (fread (buf, 1, n, fc) != n) {
				fprintf (stderr, "Error reading %s\n", cname);
				goto close_exit;
			}
			if (fwrite (buf, 1, n, f) != n) {
				fprintf (stderr, "Error writing %s\n", file);
				goto close_exit;
			}
			n_bytes -= n;
		}
		fclose (fc);

		cname = chunk_file_name(file);
		if (!(fc = fopen (cname, "rb")))/* no more chunks? */
			break;
	}
	fclose (f);
	return 0;

close_exit:
	fclose (fc);
	fclose(f);
	return -1;
}
	
	

	

