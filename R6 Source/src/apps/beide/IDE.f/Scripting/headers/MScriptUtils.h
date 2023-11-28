// ===========================================================================
//	MScriptUtils.h
// ===========================================================================
//	Copyright 1996 Metrowerks Corporation. All rights reserved.

#ifndef _MSCRIPTUTILS_H
#define _MSCRIPTUTILS_H

#include "IDEConstants.h"

class BMessage;

#include <SupportDefs.h>

	uint32						NewScriptID();

	void						AddErrorToReply(
									BMessage * &	reply,
									status_t		inError = 0,
									const char*		inErrorText = nil);

#endif
