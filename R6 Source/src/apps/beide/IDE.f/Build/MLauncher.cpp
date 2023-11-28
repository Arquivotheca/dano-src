// --------------------------------------------------------------------------- 
/* 
	Run Launcher
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			March 22, 2001 
 
	Run an application with the options from the Run-Prefs Panel
*/ 
// --------------------------------------------------------------------------- 
#include <stdlib.h>
#include <errno.h>

#include <image.h>

#include <Entry.h>
#include <Path.h>
#include <String.h>

#include "IDEApp.h"
#include "MProjectWindow.h"

#include "MLauncher.h"


/* The launch script for running in the Terminal looks like this:

	#!/bin/sh
	cd <application directory>
	<application> [application args]
	[echo 
	echo "Application exited."
	echo -n "Press 'Enter' to close this window."
	read fu]
	rm "$0"
*/

const char* kLaunchScript =
	"#!/bin/sh\n"
	"cd %s\n"
	"%s %s\n"
	"%s\n"
	"rm \"$0\"";

const char* kWaitForExit = 
	"echo\n" 
	"echo \"Application exited.\"\n"
	"echo -n \"Press 'Enter' to close this window.\"\n"
	"read fu";
	
const char* kTermPath =
	"/boot/apps/Terminal";

// todo: do we care if we clobber some unsuspecting file in tmp?
const char* kScriptPath =
		"/boot/var/tmp/BeIDE_run_script.sh";

// ----------------------------------------------------------------------

MLauncher::MLauncher(entry_ref &Ref)
          :fRef(Ref)
{
}

// ----------------------------------------------------------------------

MLauncher::~MLauncher()
{
}
	
// ----------------------------------------------------------------------

status_t
MLauncher::Launch()
{
	status_t result = B_OK;
	
	BEntry e(&fRef, true);
	BPath app(&e);
	result = app.InitCheck();
	
	if( B_OK != result )
	{
		return result;
	}
	
	BString appPath(app.Path());
	result = app.GetParent(&app);

	if( B_OK != result )
	{
		return result;
	}
	
	BString appDir(app.Path());
	
	BMessage msg;
	ssize_t ignore(0);
	msg.AddData(kRunPrefs, kMWPrefs, &ignore, sizeof(ignore));
	IDEApp::GetCurrentProject()->GetData(msg);
	
	const RunPreferences* prefs;
	msg.FindData(kRunPrefs, kMWPrefs, (const void**)&prefs, &ignore);

	BString appArgs(prefs->Args);
	
	uint32 argc = 1;
	BString wait("");

	if( true == prefs->RunInTerminal )
	{	
		wait = kWaitForExit;
		argc = 2;
	}

	FILE* scriptFile = fopen(kScriptPath, "w");

	if( scriptFile )
	{
		fprintf(scriptFile, kLaunchScript, appDir.String(),
		        appPath.String(), appArgs.String(), wait.String());

		fclose(scriptFile);

		const char** argv = new const char*[argc];

		argv[0] = kScriptPath;

		if( true == prefs->RunInTerminal )
		{
			argv[1] = argv[0];
			argv[0] = kTermPath;
		}
		
		result = resume_thread(load_image(argc, argv, SetupEnv(*prefs)));
	
		delete[] argv;

		// What happens to the script you might ask?
		// It deletes itself when it is done!
	}
	else
	{
		result = errno;
	}
	
	return result;
}

// ----------------------------------------------------------------------

const char** MLauncher::SetupEnv(const RunPreferences& Prefs)
{
	fEnviron.SetVar("MALLOC_DEBUG", (BString() << Prefs.MallocDebugLevel).String());
	
	return fEnviron.EnvBlock();
}

// ----------------------------------------------------------------------



