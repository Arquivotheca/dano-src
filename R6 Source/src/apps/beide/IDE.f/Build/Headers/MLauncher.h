// --------------------------------------------------------------------------- 
/* 
	Run Launcher
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			March 22, 2001 
 
	Run an application with the options from the Run-Prefs Panel
*/ 
// --------------------------------------------------------------------------- 
#ifndef MLAUNCHER_H
#define MLAUNCHER_H

#include <SupportDefs.h>

#include "MEnviron.h"
#include "MPrefsStruct.h"

struct entry_ref;

class MLauncher
{
	public:
		MLauncher(entry_ref& Ref);
		~MLauncher();
		
		status_t		Launch();
	
	private:
		const char**	SetupEnv(const RunPreferences& Prefs);
		
	private:
		entry_ref&		fRef;
		MEnviron		fEnviron;
};


#endif

