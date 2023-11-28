/* ++++++++++
	db.cpp
	Copyright (C) 1992 Be Labs, Inc.  All Rights Reserved
	A little app for getting other processes into the debugger.

	Modification History (most recent first):
	30 jul 92	rwh	new today
+++++ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <OS.h>
#include <unistd.h>
#include <image.h>

#include <Application.h>

#include "debugger.h"


void	load_and_run (int argc, char **argv);
long	task(void *arg);
extern "C" char	**environ;

//====================================================================

int main(int argc, char **argv)
{	
	int		i;
	long pid;

	if (argc == 1) {
		debugger ("Poke around, type g (go) to continue");
		exit(0);
	}

	/* if argument was a number, stop that pid */
	if (argv[1] && isdigit(argv[1][0])) {
		pid = strtol(argv[1], 0, 0);
		if (debug_thread (pid) < 0)
			printf("%d is not a valid process id\n", pid);
		else
			printf ("pid %d will break into the debugger when it exits the kernel\n", pid);

	/* argument was a name - look for process by that name */
	} else if ((pid = find_thread (argv[1])) >= 0) {

		if (debug_thread (pid) < 0)				/* running */
			printf ("Could not stop '%s'\n", argv[1]);
		else
			printf ("pid %d '%s' will break into the debugger when it exits the kernel\n", pid, argv[1]);
	} else
		/* named process not found: look for object file, run it */
		load_and_run (argc - 1, &argv[1]);

	return 0;
}



//--------------------------------------------------------------------

void
load_and_run (int argc, char **argv)
{
	int			pid;
	char		*fn;
	char		*epath;
	char		*dir;
	char		*path = NULL;
	char		*ebuf = NULL;
	long		result;

	fn = argv[0];
	
	/* first try in current directory */
	pid = load_image (argc, const_cast<const char **>(argv),
		const_cast<const char **>(environ));
	if (pid >= 0)
		goto found;

	/* now try in our search path */

	if (!(epath = (getenv ("PATH"))))
		goto err;
	if (!(ebuf = (char *) malloc (strlen (epath) + 1)))
		goto err;
	strcpy (ebuf, epath);		/* get a copy, cuz we trash it */

	if (!(path = (char *) malloc (strlen (ebuf) + strlen (fn) + 2)))
		goto err;
	argv[0] = path;

	dir = strtok (ebuf, ":");

	while (dir) {
		strcpy (path, dir);
		strcat (path, "/");
		strcat (path, fn);
	
		pid = load_image (argc, const_cast<const char **>(argv),
			const_cast<const char **>(environ));
		if (pid >= 0)
			goto found;

		dir = strtok (NULL, ":");
	}
err:
	printf ("Could not find %s\n", fn);
	goto exit;

found:
	debug_thread (pid);
	resume_thread(pid);
	wait_for_thread (pid, &result);
	printf ("'%s' exited with result %d\n", fn, result);
exit:
	if (ebuf) free (ebuf);
	if (path) free (path);
	return;
}
