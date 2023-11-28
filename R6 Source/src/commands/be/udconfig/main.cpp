//
// Copyright 2000 Be, Incorporate.  All Rights Reserved.
//

#include <stdio.h>

#include "ConfigApp.h"

int
main(int argc, char *argv[])
{
	const char *fname;

	if(argc >= 2) {
		fname = argv[1];
	}
	else {
		fname = NULL;
	}

	ConfigApp app(fname);
	app.Run();
	return B_OK;
}
