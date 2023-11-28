/* alert.cpp */
/* command-line tool to display a textual dialog */
/* Copyright (c) 1997 Be, Incorporated. */
/* Written by Jon Watte */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Application.h>
#include <Alert.h>


static void
usage()
{
	fprintf(stderr, "usage: alert [ <type> ] [ --modal ] [ --help ] text [ button1 [ button2 [ button3 ]]]\n");
	fprintf(stderr, "<type> is --empty | --info | --idea | --warning | --stop\n"); 
	fprintf(stderr, "--modal makes the alert system modal and shows it in all workspaces.\n");
	fprintf(stderr, "If any button argument is given, exit status is button number (starting with 0)\n");
	fprintf(stderr, "and 'alert' will print the title of the button pressed to stdout.\n");
	exit(-1);
}


struct a_opt {
	const char * opt;
	alert_type type;
} s_types[] = {
	{ "--empty", B_EMPTY_ALERT },
	{ "--info", B_INFO_ALERT },
	{ "--idea", B_IDEA_ALERT },
	{ "--warning", B_WARNING_ALERT },
	{ "--stop", B_STOP_ALERT },
	{ NULL, B_EMPTY_ALERT }
};

int
main(
	int argc,
	char * argv[])
{
	bool modal = false;
	alert_type type = B_INFO_ALERT;
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		usage();
	}
	if (argc >= 2 && !strcmp(argv[1], "--modal")) {
		modal = true;
		argc--;
		argv++;
		if (argc == 1) {
			usage();
		}
	}
	int len = strlen(argv[1]);
	struct a_opt * ptr = s_types;
	bool found = false;
	while (ptr->opt != NULL) {
		if (!strncmp(ptr->opt, argv[1], len)) {
			if (found) {
				fprintf(stderr, "ambiguous option: %s\n", argv[1]);
				exit(-1);
			}
			found = true;
			type = ptr->type;
		}
		ptr++;
	}
	if (found) {
		argv++;
		argc--;
	}
	/* re-do the argument checks */
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		usage();
	}
	if (argc >= 2 && !strcmp(argv[1], "--modal")) {
		modal = true;
		argc--;
		argv++;
	}
	BApplication app("application/x-vnd.Be-cmd-alert");
	BAlert * alrt = new BAlert("Alert", argv[1], (argv[2] ? argv[2] : "OK") ,
		(argc > 3 ? argv[3] : NULL), (argc > 4 ? argv[4] : NULL), 
		B_WIDTH_AS_USUAL, type);
	if (modal) {
		alrt->SetWorkspaces(B_ALL_WORKSPACES);
		alrt->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
	}
	int ix = alrt->Go();
	app.PostMessage(B_QUIT_REQUESTED);
	app.Run();
	if (argv[2]) {
		printf("%s\n", argv[2+ix]);
	}
	return ix;
}

