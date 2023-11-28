//========================================================================
//	MLinkObj.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MLINKOBJ_H
#define _MLINKOBJ_H

#include "MCompileTool.h"

class MProjectView;


class MLinkObj : public MCompileTool
{
public:
 								MLinkObj(
									const char * 	compiler,
									BList&		 	args,
									bool 			IDEAware,
									MProjectView& 	inProjectView);

		/*
		 *	These two should be implemented to do whatever's right for the message
		 */
virtual	status_t				DoMessage(const ErrorNotificationMessage& message);
virtual status_t				DoMessage(const ErrorNotificationMessageShort& message);

virtual	void					DoStatus(
									bool objProduced,
									status_t errCode);
virtual	void					DoStatus(
									const CompilerStatusNotification& inRec)
								{
									MCompileTool::DoStatus(inRec);
								}

virtual status_t				ParseMessageText(
									const char*	inText);

private:

		MProjectView&			fProjectView;
};

#endif
