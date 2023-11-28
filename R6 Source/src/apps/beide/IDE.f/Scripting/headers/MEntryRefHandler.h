//========================================================================
//	MEntryRefHandler.h
//	Copyright 1996 -97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MENTRYREFHANDLER_H
#define _MENTRYREFHANDLER_H

#include "LazyScriptHandler.h"

class BList;
struct entry_ref;

class MEntryRefHandler : public LazyScriptHandler
{
public:
								MEntryRefHandler(
									const char *		inName,
									BList *				inList);
								MEntryRefHandler(
									const char *		inName,
									entry_ref			inRef);

virtual							~MEntryRefHandler();

		status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);

protected:

		BList *					fList;

		ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);

		long					DoGet(
									BMessage *				message,
									BMessage * &			reply);
		long					DoSet(
									BMessage *				message,
									BMessage * &			reply);
};

#endif
