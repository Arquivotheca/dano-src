//	main.cpp

#include "ShowApp.h"
#include "TranslationKit.h"
#include "prefs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ResourceStrings.h>
#include <Alert.h>


bool inhibit_first_panel = false;

BResourceStrings g_strings;	/* from application */

int
main(
	int		argc,
	char *	argv[])
{
	int errs = 0;
	int hits = 0;
	char * errstr = 0;

	init_prefs();
	ShowApp app("application/x-vnd.Be-ShowImage");
	for (int ix=1; ix<argc; ix++) {
		entry_ref ref;
		BEntry entry(argv[ix]);
		if (entry.InitCheck() || !entry.Exists()) {
			printf(g_strings.FindString(27), argv[ix]);
			errs++;
			if (errstr) {
				strcat(errstr, ", ");
				errstr = (char *)realloc(errstr, strlen(errstr)+strlen(argv[ix])+5);
			}
			else {
				const char * s = g_strings.FindString(38);
				errstr = (char *)malloc(strlen(s)+strlen(argv[ix])+5);
				strcpy(errstr, s);
			}
			if (errstr) strcat(errstr, argv[ix]);
			continue;
		}
		entry.GetRef(&ref);
		inhibit_first_panel = true;
		BMessage *msg = new BMessage(B_REFS_RECEIVED);
		msg->AddRef("refs", &ref);
		app.PostMessage(msg);
		hits++;
	}
	if (errstr) {
		(new BAlert("", errstr, "Sorry"))->Go();
		free(errstr);
	}
	app.Run();
	save_prefs();
	return 0;
}


