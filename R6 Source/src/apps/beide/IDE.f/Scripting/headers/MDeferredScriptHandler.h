//========================================================================
//	MDeferredScriptHandler.h
//	Copyright 1996Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MDEFERREDSCRIPTHANDLER_H
#define _MDEFERREDSCRIPTHANDLER_H

#include "Scripting.h"
#include "ScriptHandler.h"


class MDeferredScriptHandler : public ScriptHandler
{
public:
								MDeferredScriptHandler(
									const uint32			inId,
									const char *			inName,
									BLooper &				inLooper);
virtual							~MDeferredScriptHandler();

virtual	status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);

virtual	void					SendReply(	
									status_t		inError,
									const char*		inErrorText);
protected:

		BMessage*				fMessage;
		BMessage*				fReply;
		BLooper&				fLooper;
};

#endif
