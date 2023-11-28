/*	WindowPropHandler.h
 *	$Id: WindowPropHandler.h,v 1.1 1996/12/04 11:58:44 hplus Exp elvis $
 *	Properties of a standard window
 */

#pragma once

#include "LazyScriptHandler.h"
#include "NotifyLazyPropHandler.h"


class WindowPropHandler :
	public LazyScriptHandler
{
public:
								WindowPropHandler(
									const char *	name,
									BWindow *		window);
								~WindowPropHandler() { }

		void					NotifyChange(
									NotifyLazyPropHandler<WindowPropHandler, long> &
															notifier,
									long					what);
protected:

		char *					fName;
		bool					fVisible;
		bool					fActive;

		BWindow *				fWindow;

		ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);

		status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);
		status_t				DoSet(
									BMessage *			message,
									BMessage * &		reply);
		status_t				DoGet(
									BMessage *			message,
									BMessage * &		reply);

};
