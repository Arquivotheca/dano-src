/*	ViewPropHandler.h
 *	$Id: ViewPropHandler.h,v 1.1 1996/12/04 11:58:43 hplus Exp elvis $
 *	Generic ScriptHandler referring to a BView
 */

#pragma once


#include "LazyScriptHandler.h"
#include "NotifyLazyPropHandler.h"


class BView;


class ViewPropHandler :
	public LazyScriptHandler
{
public:
								ViewPropHandler(
									BView *				view);
								~ViewPropHandler() { }

		void					NotifyChange(
									NotifyLazyPropHandler<ViewPropHandler, long> &
														prop,
									long				what);

protected:

		BView *					fView;
		BRect					fFrame;
		rgb_color				fColor;

		ScriptHandler *			GetSubHandler(
									const char *			propertyName,
									EForm					form,
									const SData &			data);
};
