/* ----------------
 * Debug server glue program.
 * 
 * FILE:	$Source: /net/bally/be/rcs/src/commands/debug_glue.c,v $
 * REVS:	$Revision: 1.4 $
 * NAME:	$Author: mani $
 * DATE:	$Date: 1997/02/14 22:14:35 $
 * STATE:  	$State: Exp $
 *
 * Copyright (c) 1997 by Be Incorporated.	All Rights Reserved.
 ---------------- */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <OS.h>
#include <syslog.h>

static long transfer_to_debug (void *data);
static long transfer_from_debug (void *data);
static int debuginfd, debugoutfd;

static void usage(const char* appname)
{
	fprintf (stderr, "usage:\t%s key sem_id\nor:\t%s -app [to debug an app_server crash]\n", appname, appname);
	exit (1);
}

int main (int argc, const char *argv[], const char *env[])
{
	char key[32];
	char buf[50];
	int     ret;
	sem_id  debug_glue_sem;

	if (argc == 2)
	{
		// when launched with no args, we create a port with a well-known name and
		// read the key string & sem_id through that port.  This protocol is used by the
		// debug_server when the app_server crashes, to let someone telnet into the
		// box and run debug_glue in their telnet session.
		status_t err;
		int32 code;
		port_id server_info_port;

		if (strcmp(argv[1], "-app"))
		{
			usage(argv[0]);
		}

		server_info_port = create_port(4, "debug_glue handoff");
		err = read_port(server_info_port, &code, &key, sizeof(key));
		if (err > 0)
		{
			read_port(server_info_port, &code, &debug_glue_sem, sizeof(debug_glue_sem));
		}
		close_port(server_info_port);
		if (err < 0)
		{
			fprintf(stderr, "error 0x%08x reading key & sem_id from debug_server\n", err);
			return err;
		}
	}
	else if (argc == 3)
	{
		strcpy(key, argv[1]);
		debug_glue_sem = atoi (argv[2]);
	}
	else
	{
		usage(argv[0]);
	}

	sprintf (buf, "/pipe/debug_out_%s", key);
	if ((debugoutfd = open (buf, O_CREAT | O_RDONLY, 0777)) < 0) {
		perror ("opening debug_server output pipe");
		exit (2);
	}

	sprintf (buf, "/pipe/debug_in_%s", key);
	if ((debuginfd = open (buf, O_CREAT | O_WRONLY, 0777)) < 0) {
		perror ("opening debug_server input pipe");
		close (debugoutfd);
		exit (3);
	}

	if (release_sem (debug_glue_sem) < B_NO_ERROR) {
		close (debugoutfd);
		close (debuginfd);
		exit (4);
	}

	/* XXX - these threads should be replaced with select/poll
	 * when those things get done.
	 */

	resume_thread (spawn_thread (transfer_to_debug, "debug_thread input", B_NORMAL_PRIORITY, (void *) NULL));

	rename_thread (find_thread (NULL), "debug_thread output");
	transfer_from_debug (NULL);

	// not actually reached, but avoids a warning
	return 0;
}


static long
transfer_to_debug (void *data)
{
	int n;
	char buf[1000];
	
	while (1) {
		n = read (STDIN_FILENO, buf, 1000);
		if (n <= 0) {
			if (n < 0)
				perror ("read");
/*			syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Cleaning up from read\n");
			syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Sending exit cmd\n");*/
			write (debuginfd, "exit\n", 5);
			break;
		}
		n = write (debuginfd, buf, n);
		if (n <= 0) {
			if (n < 0)
				perror ("write");
/*			syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Cleaning up from write\n");*/
			break;
		}
	}

	close (debugoutfd);
	close (debuginfd);
/*	syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Before exit\n");*/
	exit_thread (5);
	return 5;		// avoids a warning
}


static long
transfer_from_debug (void *data)
{
	int n;
	char buf[1000];
	
	while (1) {
		n = read (debugoutfd, buf, 1000);
/*		syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Read %d bytes\n", n);*/
		if (n <= 0) {
			if (n < 0)
				perror ("read");
/*			syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Cleaning up from read\n");*/
			break;
		}
		n = write (STDOUT_FILENO, buf, n);
/*		syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Wrote %d bytes\n", n);*/
		if (n <= 0) {
			if (n < 0)
				perror ("write");
/*			syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Cleaning up from write\n");*/
			write (debuginfd, "exit\n", 5);
			break;
		}
	}

/* this doesn't work
 	syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Sending exit cmd\n");
 	write (debuginfd, "exit\n", 5);
	syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Finished sending exit cmd\n");
*/	
	close (STDIN_FILENO);
	close (debugoutfd);
	close (debuginfd);
/*	syslog (LOG_USER|LOG_DEBUG|LOG_PID, "Before exit\n");*/
	exit (5);
}

