//========================================================================
//	MKeyFilter.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#ifndef _MKEYFILTER_H
#define _MKEYFILTER_H

#include <MessageFilter.h>
#include "MKeyBindingManager.h"

struct KeyBinding;
class BWindow;
class BMenuBar;


class MKeyFilter : public BMessageFilter
{
public:
								MKeyFilter(	
									BWindow* 			inWindow,
									KeyBindingContext	inContext);

virtual	filter_result			Filter(
									BMessage *message, 
									BHandler **target);
private:

	bigtime_t				fPrefixTime;
	BWindow*				fWindow;
	BMenuBar*				fBar;
	KeyBindingContext		fContext;
	int16					fPrefixIndex;
	bool					fIsPrefix;
	
	bool						MessageToBinding(
									BMessage*	inMessage, 
									KeyBinding&	outBinding);

	bool						CommandIsValid(
									CommandT	inCommand);
};


// keyfilter class to be used in windows that have BTextViews in them
// so that edit commands aren't trapped
class MTextKeyFilter : public MKeyFilter
{
public:
								MTextKeyFilter(	
									BWindow* 			inWindow,
									KeyBindingContext	inContext);

virtual	filter_result			Filter(
									BMessage *message, 
									BHandler **target);
};


// keyfilter class for project windows.  Allows command arrow keys to
// be passed to the project view
class MProjectKeyFilter : public MKeyFilter
{
public:
								MProjectKeyFilter(	
									BWindow* 			inWindow,
									KeyBindingContext	inContext);

virtual	filter_result			Filter(
									BMessage *message, 
									BHandler **target);
};

#endif
