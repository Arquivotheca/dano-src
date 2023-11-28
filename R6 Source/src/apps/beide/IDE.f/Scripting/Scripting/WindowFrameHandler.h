/*	WindowHandlers.h
 *	$Id: WindowFrameHandler.h,v 1.1 1996/12/04 11:58:40 hplus Exp elvis $
 *	Standard handler used to manipulate a window frame. This is a little nicer than 
 *	the default NotifyLazyPropHandler for use with a BRect because it allows you 
 *	to descend into each part of the rect (left, top, right, bottom)
 */

#ifndef _WINDOWFRAMEHANDLER_H
#define _WINDOWFRAMEHANDLER_H

#include "LazyScriptHandler.h"

class WindowFrameHandler :
	public LazyScriptHandler
{
public:
								WindowFrameHandler(
									const char *		inName,
									BWindow *			inWindow);
								~WindowFrameHandler() { }

protected:

		float *					fPtr;
		BRect					frame;
		BWindow *				window;

		ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);

		status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);

		long					DoGet(
									BMessage *				message,
									BMessage * &			reply);
		long					DoSet(
									BMessage *				message,
									BMessage * &			reply);
};

#endif
