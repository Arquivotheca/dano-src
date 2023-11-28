/*
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved 
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Alert.h>
#include "pmapp.h"
#include "pmprefs.h"
#include "hardcoded.h"


PMApplication *my_app;
PMLog *logger;

int
main(
	 int argc, 
	 char **argv
	 )
{	
	char *dirname = NULL;
	bool hide = false;

	for (int i=1; i<argc; i++) {
		if (strcmp(argv[i], "-d") == 0) {
			i++;
			dirname = argv[i];
		}
		else if (strcmp(argv[i], "-w") == 0) {
			hide = true;
		}
		else {
			fprintf(stderr, "usage: %s [-d directory] [-w]\n", argv[0]);
			exit(5);
		}
	}
	
	PMPrefs settings("PoorMan Settings");
	my_app = new PMApplication(&settings, dirname, hide);
	my_app->Run();
	delete my_app;
	return 0;
}

PMApplication::PMApplication(PMPrefs *settings, char *dirname, bool hide)
	: BApplication(SIGNATURE)
{
	win = new PMWindow(settings, dirname, hide);
}

void PMApplication::AboutRequested(void) {
	BAlert *alert;
	
	alert = new BAlert(NULL, "Poor Man's Web Server\n" \
							 "Copyright " B_UTF8_COPYRIGHT " 1996-99 Be Incorporated\n" \
							 "All rights reserved.", \
					   OK);
	alert->Go();
}

void PMApplication::MessageReceived(BMessage *message) {
	switch(message->what) {
		case GET_PM_WINDOW:
			{
				BMessage reply(GET_PM_WINDOW);
				reply.AddPointer("window", win);
				message->SendReply(&reply);
			}
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}
