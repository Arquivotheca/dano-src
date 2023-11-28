/*
 * beep.cpp
 *
 * produce a beep on the default sound output.
 * command-line tool.
 */

#include <Beep.h>
#include <PlaySound.h>
#include <stdio.h>

int
main(
        int argc,
        char *argv[])
{
	if ((argc > 2) || ((argv[1] != 0) && (argv[1][0] == '-'))) {
		fprintf(stderr, "usage: beep [ eventname ]\n");
		fprintf(stderr, "Event names are found in the Sounds preferences panel.\n");
		return 1;
	}
	wait_for_sound(system_beep(argv[1]));
	return 0;
}
