/* ++++++++++
	$Source: /net/bally/be/rcs/src/commands/psloop.c,v $
	$Revision: 1.14 $
	Author: herold
	Copyright (c) 1994-96 by Be Incorporated.  All Rights Reserved.
+++++ */

#define PS_LOOP

#include "ps.c"

int
main (int argc, char **argv)
{
	for (;;) {
		do_ps (argc, argv);
		snooze (3000000);
	}
	return 0;
}
