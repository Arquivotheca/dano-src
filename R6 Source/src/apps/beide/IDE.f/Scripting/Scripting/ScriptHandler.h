/*	ScriptHandler.h
 *	$Id: ScriptHandler.h,v 1.1 1996/12/04 11:58:36 hplus Exp elvis $
 *	Simple superclass for "things" that handle script events
 */

#ifndef _SCRIPTHANDLER_H
#define _SCRIPTHANDLER_H

#include "Scripting.h"

#include <SupportDefs.h>

class BLooper;
class Coercions;

class ScriptHandler
{
public:
								ScriptHandler(
									const uint32			id,
									const char *			name,
									BLooper *				looper = NULL);
virtual							~ScriptHandler();

		ScriptHandler *			FindScriptTarget(
									BMessage *				message,
									const char *			parameter,
									bool&					wasRePosted);

virtual	status_t				PerformScriptAction(
									BMessage *				message,
									BMessage * &			reply,
									bool&					wasDeferred);

		uint32					GetID() { return fId; }
virtual	const char *			GetName() { return fName; }
virtual	void					SetName(
									const char *			name);

virtual	ScriptHandler *			Reference();
virtual	void					Done();

virtual	Coercions &				GetCoercions() { return *sCoercions; }

		BLooper *				GetLooper()
								{
									return fLooper;
								}
protected:

		uint32					fId;
		char *					fName;
		BLooper *				fLooper;

		void					FreeName();
		void					AllocName(
									const char * name);

static	Coercions *				sCoercions;

virtual	ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);

		void					PostMessageToNewLooper(
									BMessage *			message,
									BLooper *			looper,
									const char *		parameter,
									int32				index);
};

#endif
