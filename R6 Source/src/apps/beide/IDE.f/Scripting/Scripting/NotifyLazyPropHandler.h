/*	NotifyLazyPropHandler.h
 *	$Id: NotifyLazyPropHandler.h,v 1.1 1996/12/04 11:58:42 hplus Exp elvis $
 *	This handler does the same job as LazyPropHandler, except it notifies another 
 *	object when a change takes place. The other object has to be of a class that 
 *	implements a method:
 *	void NotifyChange(NotifyLazyPropHandler &, Type *)
 *	The Type * argument is for a "cookie" that the object can use to remember what 
 *	the callback was about.
 */

#pragma once

#include "LazyPropHandler.h"


/*	The type T is the type of the object to be called back when the property is changed.
 *	The type U is the type of the magic cookie that is passed to the callback.
  */
template<class T, class U>
class NotifyLazyPropHandler :
	public LazyPropHandler
{
public:
								NotifyLazyPropHandler(
									const char *		name,
									void *				prop,
									int					size,
									unsigned long		type,
									T &					toNotify,
									U					cookie);
protected:

		T &						fToNotify;
		U						fCookie;

		long					DoSet(
									BMessage *			message,
									BMessage * &		reply);
};

template<class T, class U> inline 
NotifyLazyPropHandler<T, U>::NotifyLazyPropHandler(
	const char *		name,
	void *				prop,
	int					size,
	unsigned long		type,
	T &					toNotify,
	U					cookie) :
	LazyPropHandler(name, prop, size, type, true),
	fToNotify(toNotify),
	fCookie(cookie)
{
}

template<class T, class U> inline long 
NotifyLazyPropHandler<T, U>::DoSet(
	BMessage *			message,
	BMessage * &		reply)
{
	long e = LazyPropHandler::DoSet(message, reply);
	if (!e) fToNotify.NotifyChange(*this, fCookie);
	return e;
}
