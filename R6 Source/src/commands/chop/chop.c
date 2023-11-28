/* ++++++++++
	chop.c
	Copyright (C) 1992-3 Be Incorporated.  All Rights Reserved.
	Chop up a file.

	Modification History (most recent first):
	20 aug 92	rwh	new today
+++++ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CHUNK_SIZE	1400

/* buffer size must be less than max DOS integer, hence 31 instead of 32 */
#define BUFSIZE (31*1024)

static char buf[BUFSIZE];
static int seq = 0;

/* ----------
	create_chunk_file creates a new file with the passed name postpended
	with a 2 digit sequence number.
----- */
FILE *
create_chunk_file (name)
	char *name;
{
	FILE *f;

	strcpy (buf, name);
	sprintf (buf + strlen(name), "%.2d", seq++);
	if (f = fopen (buf, "rb")) {
		fprintf (stderr, "%s already exists - aborting\n", buf);
		seq -= 1;	/* don't remove existing one */
		fclose (f);
		return NULL;
	}
	if (!(f = fopen (buf, "wb"))) {
		fprintf (stderr, "Problem creating %s - aborting\n", buf);
		return NULL;
	}
	return f;
}

/* ----------
	remove_chunk_files removes the chunk files created so far.
----- */
void
remove_chunk_files (name)
	char *name;
{
	while (--seq >= 0) {
		strcpy (buf, name);
		sprintf (buf + strlen(name), "%.2d", seq);
		remove (buf);
	}
}
	

main(argc, argv)
	int argc;
	char **argv;
{
	char *prog, *file;
	long chunk_size = DEFAULT_CHUNK_SIZE;
	long chunk_amt;
	int n;
	long n_bytes;
	FILE *f, *fc;


	/* parse command line options */

	prog = *argv++;
	argc--;

	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {
		case 'n':
			if (!(chunk_size = strtol (*++argv, 0, 0))) {
				fprintf (stderr, "Invalid chunk size\n");
				argc = 0;	/* signal an error */
			} else
				argc--;
			break;
		default:
			fprintf (stderr, "Invalid option '%s'\n", prog);
			argc = 0;	/* signal an error */
			break;
		} /* switch */
		argv++;
		argc--;
	}
				
	if (argc != 1) {
		fprintf(stderr, "Usage: %s [-n kbyte_per_chunk] file\n", prog);
		fprintf (stderr, "Splits file into smaller files named file00, file01...\n");
		fprintf (stderr, "Default split size is %dk\n", DEFAULT_CHUNK_SIZE);
		return -1;
	}

	file = argv[0];
	printf ("Chopping up %s into %d kbyte chunks\n", file, chunk_size);
	chunk_size *= 1024;

	if (!(f = fopen (file, "rb"))) {	/* file doesn't exist.. */
		fprintf(stderr, "Could not open %s\n", file);
		return -1;
	}
		
	/* get file size */

	if (fseek (f, 0L, SEEK_END)) {
		fprintf (stderr, "Error seeking in the file %s\n", file);
		goto close_exit;
	}

	if ((n_bytes = ftell (f)) == -1) {
		fprintf (stderr, "Error ftell-ing the file %s\n", file);
		goto close_exit;
	}

	if (n_bytes <= chunk_size) {
		fprintf (stderr, "File is already small enough\n");
		goto close_exit;
	}

	if (fseek (f, 0L, SEEK_SET)) {
		fprintf (stderr, "Error seeking in the file %s\n", file);
		goto close_exit;
	}

	while (n_bytes) {
		if (!(fc = create_chunk_file (file)))
			goto remove_exit;

		chunk_amt = (n_bytes > chunk_size) ? chunk_size : n_bytes;
		n_bytes -= chunk_amt;

		while (chunk_amt > 0) {
			n = (chunk_amt > BUFSIZE) ? BUFSIZE : chunk_amt;
			if (fread (buf, 1, n, f) != n) {
				fprintf (stderr, "Error reading the file\n");
				goto close_remove_exit;
			}
			if (fwrite (buf, 1, n, fc) != n) {
				fprintf (stderr, "Error writing in the chunk file\n");
				goto close_remove_exit;
			}
			chunk_amt -= n;
		}
		fclose (fc);
	}
	fclose (f);
	return 0;

close_remove_exit:
	fclose (fc);

remove_exit:
	remove_chunk_files(file);

close_exit:
	fclose(f);
	return -1;
}
	
	

	

