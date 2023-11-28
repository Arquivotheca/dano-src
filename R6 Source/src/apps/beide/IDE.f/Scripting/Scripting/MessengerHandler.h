/*	MessengerHandler.h
 *	$Id: MessengerHandler.h,v 1.1 1996/12/04 11:58:41 hplus Exp elvis $
 *	Handles a "messenger" property of a handler
 */

#ifndef _MESSENGERHANDLER_H
#define _MESSENGERHANDLER_H

#include "LazyScriptHandler.h"


class BHandler;


class MessengerHandler :
	public LazyScriptHandler
{
public:
								MessengerHandler(
									const char *		name,
									BHandler *			handler);
								~MessengerHandler() { }

protected:

		BHandler *				fHandler;

virtual	status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);

virtual	long					DoGet(
									BMessage *				message,
									BMessage * &			reply);

};

#endif
