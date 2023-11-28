/*	LazyMallocStringProp.h
 *	$Id: LazyMallocStringProp.h,v 1.1 1996/12/04 11:58:45 hplus Exp elvis $
 *	This handler handles the specific case of a string allocated by malloc() that 
 *	can be changed by calling free() and malloc() again. It is a subclass of 
 *	NotifyLazyPropHandler.h to notify a user of a change to the string.
 */

#pragma once

#include "NotifyLazyPropHandler.h"
#include "Coercion.h"


template <class T, class U>
class LazyMallocStringProp :
	public NotifyLazyPropHandler<T, U>
{
public:
								LazyMallocStringProp(
									const char *			name,
									char **					string,
									T &						toNotify,
									U						cookie);

protected:

		char * &				fString;

		long					DoSet(
									BMessage *				message,
									BMessage * &			reply);

};


template<class T, class U> inline 
LazyMallocStringProp<T, U>::LazyMallocStringProp(
	const char *			name,
	char * *				string,
	T &						toNotify,
	U						cookie) :
	NotifyLazyPropHandler<T, U>(
		name, *string, strlen(*string)+1, B_STRING_TYPE, toNotify, cookie),
	fString(*string)
{
}


template<class T, class U> inline long
LazyMallocStringProp<T, U>::DoSet(
	BMessage *				message,
	BMessage * &			/*reply*/)
{
	char *newString = NULL;
	if (!GetCoercions().GetString(newString, message, kDefaultDataName))
		return SCRIPT_BAD_TYPE;
	if (!newString)
		return SCRIPT_MISSING_DATA;
	free(fString);
	fString = newString;
	fProp = fString;
	fSize = strlen(fString)+1;
	fToNotify.NotifyChange(*this, fCookie);
	return B_NO_ERROR;
}
