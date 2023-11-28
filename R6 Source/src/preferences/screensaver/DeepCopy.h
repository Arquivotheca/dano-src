#if ! defined DEEPCOPY_INCLUDED
#define DEEPCOPY_INCLUDED

#include <Directory.h>
#include <Entry.h>

bool DeepCopy(BDirectory &dir, // target dir in here
	const char *targetname,	// target name in here, not null to rename
	entry_ref *ref, // source ref in here
	bool failifexist = true);	// fail if exist

#endif
