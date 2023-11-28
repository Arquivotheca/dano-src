/* ++++++++++
	FILE:	shutdown.cpp
	REVS:	$Revision: 1.9 $
	NAME:	herold
	DATE:	Mon Mar 18 11:57:49 PST 1996
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Messenger.h>
#include <os_p/priv_syscalls.h>
#include <app_p/roster_private.h>

//+extern "C" void _shutdown_(bool);

//====================================================================

int main(long argc, char* argv[])
{
	BMessenger	roster("application/x-vnd.Be-ROST", -1, NULL);
	int			error = B_NO_ERROR;
	int         reboot = 0, i, delay = 0, quick = 0;
	status_t res;

	for(i=1; i < argc; i++) {
		if (strcmp(argv[i], "-r") == 0) {
			reboot = 1;
		} else if (strcmp(argv[i], "-d") == 0 && argv[i+1]) {
			delay = strtol(argv[i+1], NULL, 0);
			if (delay < 0)
				delay = 0;
			i++;
		} else if (strcmp(argv[i], "-q") == 0) {
			quick++;
		} else {
			fprintf(stderr, "usage: %s [-r] [-q] [-d time]\n", argv[0]);
			exit(5);
		}
	}

	if (delay)
		sleep(delay);

	if (quick) {
		_kshutdown_(reboot);
		fprintf(stderr, "Shutdown failed!\n");
		exit(1);
	}

	if (reboot)
		res = roster.SendMessage(CMD_REBOOT_SYSTEM);
	else
		res = roster.SendMessage(CMD_SHUTDOWN_SYSTEM);

	if (res != 0)
        printf("Failed to send message to roster server: %s\n", strerror(res));

	return(error);
}
