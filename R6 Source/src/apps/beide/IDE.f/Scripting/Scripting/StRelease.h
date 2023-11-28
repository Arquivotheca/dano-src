/*	StRelease.h
 *	$Id: StRelease.h,v 1.1 1996/12/04 11:58:42 hplus Exp elvis $
 *	Release a handler when done. Note that it DOESN'T Reference() the handler; 
 *	if you want that, use StRelease(handler->Reference()) (or another class)
 */

#pragma once

#include "ScriptHandler.h"

class StRelease {
public:
								StRelease(
									ScriptHandler *		handler)
								{
									fHandler = handler;
								}
								~StRelease()
								{
									if (fHandler)
										fHandler->Done();
								}
protected:
		ScriptHandler *			fHandler;
};
