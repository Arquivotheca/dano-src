
#ifndef _SMOOVED_H
#define _SMOOVED_H

#include <Messenger.h>

#define SmooveDSignature "application/x-vnd.Be-smooved"

enum SmooveDMessages {
	bmsgListFlunkies = 'lfln',		/* List available flunkies */
	bmsgPassToFlunky = 'pfln',		/* Pass a message to a named flunky */
	bmsgFetchFlunky =  'ffln'		/* Fetch a messenger for a named flunky */
};

extern BMessenger call_flunky(const char *flunkyName);

#endif
