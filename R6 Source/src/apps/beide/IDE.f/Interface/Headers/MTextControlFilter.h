//========================================================================
//	MTextControlFilter.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MTEXTCONTROLFILTER_H
#define _MTEXTCONTROLFILTER_H

#include <MessageFilter.h>

class BTextControl;


class MTextControlFilter : public BMessageFilter
{
public:
								MTextControlFilter(
									BTextControl*	inControl);

virtual	filter_result			Filter(
									BMessage *message, 
									BHandler **target);

		void					SetControl(
									BTextControl*	inControl);
private:
		BTextControl*		fControl;
};

#endif
