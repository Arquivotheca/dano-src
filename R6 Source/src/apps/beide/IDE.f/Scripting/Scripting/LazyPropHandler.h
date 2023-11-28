//	LazyPropHandler.h

#ifndef _LAZYPROPHANDLER_H
#define _LAZYPROPHANDLER_H

#include "LazyScriptHandler.h"


/*	LazyPropHandler can be used to reference actual members within your objects.
 *	It provides by default getting and optional setting of long, short, char, float, 
 *	double, rect, point and rgb_color types.
 *	You can use it for strings and ASCII only for read-only properties unless you 
 *	subclass to do the right thing for the set case. Pass the actual string pointer in.
 */
class LazyPropHandler :
	public LazyScriptHandler
{
public:
								LazyPropHandler(
									const char *		name,
									void *				prop,
									int					size,
									unsigned long		type,
									bool				modify = true);
								~LazyPropHandler();

		void					SetHandler(
									ScriptHandler *		h);

		status_t				PerformScriptAction(
									BMessage *			message,
									BMessage * &		reply,
									bool&				wasDeferred);

protected:

static	long					sId;

		void *					fProp;
		int						fSize;
		unsigned long			fType;
		bool					fModifiable;
		ScriptHandler *			fHandler;

virtual	long					DoSet(
									BMessage *			message,
									BMessage * &		reply);
virtual	long					DoGet(
									BMessage *			message,
									BMessage * &		reply);
//	should really have virtuals for all common verbs here
};

#endif
