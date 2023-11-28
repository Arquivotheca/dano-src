/* --------------------------------------------------------------------
	
	format - volume format tool

	Written by: Robert Polic
	
	Copyright 1995 Be, Inc. All Rights Reserved.
	
-------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int
main(int argc, char **argv)
{
	int			err;

	if (argc != 3)
		fprintf (stderr, "wrong number of arguments\n");
	else {
		err = format_device(argv[1], argv[2]);
		if (err < 0)
			fprintf (stderr, "could not format device %s with volume name %s\n", argv[1], argv[2]);
	}
}
