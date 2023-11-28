#include <AppKit.h>
#include <string.h>

#include <InterfaceDefs.h>

int
main(int argc, char **argv)
{
	BApplication *bapp;
	bool doit = true;

	bapp = new BApplication("application/x-vnd.Be-ffm");

	if (argv[1]) {
		if (strcasecmp(argv[1], "yes") == 0 || strcasecmp(argv[1], "on") == 0)
			doit = true;
		else
			doit = false;
	}

	set_focus_follows_mouse(doit);

	return 0;
}
