//========================================================================
//	MProjectFileHandler.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPROJECTFILEHANDLER_H
#define _MPROJECTFILEHANDLER_H

#include "LazyScriptHandler.h"

class MSourceFileLine;
class MProjectView;

class MProjectFileHandler : public LazyScriptHandler
{
public:
								MProjectFileHandler(
									MSourceFileLine*	inProjectLine,
									MProjectView&		inProjectView,
									int32				inIndex);

virtual							~MProjectFileHandler();

protected:

		int32					fIndex;
		MSourceFileLine*		fSourceFileLine;
		MProjectView&			fProjectView;

virtual	ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);
virtual	status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);

};

#endif
