/* ++++++++++

   FILE:  Filter.cpp
   REVS:  $Revision: 1.10 $
   NAME:  peter
   DATE:  Wed Jun  5 15:09:26 PDT 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <Debug.h>

#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif

/*---------------------------------------------------------------*/

BMessageFilter::BMessageFilter(uint32 w, filter_hook func)
{
	fDelivery = B_ANY_DELIVERY;
	fSource = B_ANY_SOURCE;
	what = w;
	fFiltersAny = false;
	fLooper = NULL;
	fFilterFunction = func;
}

/*---------------------------------------------------------------*/

BMessageFilter::BMessageFilter(message_delivery type, message_source source,
	uint32 w, filter_hook func)
{
	fDelivery = type;
	fSource = source;
	what = w;
	fFiltersAny = false;
	fLooper = NULL;
	fFilterFunction = func;
}

/*---------------------------------------------------------------*/

BMessageFilter::BMessageFilter(message_delivery type, message_source source,
	filter_hook func)
{
	fDelivery = type;
	fSource = source;
	what = 0;
	fFiltersAny = true;
	fFilterFunction = func;
	fLooper = NULL;
}

/*---------------------------------------------------------------*/

BMessageFilter::BMessageFilter(const BMessageFilter &filter)
{
	fDelivery = filter.fDelivery;
	fSource = filter.fSource;
	what = filter.what;
	fFiltersAny = filter.fFiltersAny;
	fFilterFunction = filter.fFilterFunction;
	fLooper = NULL;
}

/*---------------------------------------------------------------*/

BMessageFilter::BMessageFilter(const BMessageFilter *filter)
{
	fDelivery = filter->fDelivery;
	fSource = filter->fSource;
	what = filter->what;
	fFiltersAny = filter->fFiltersAny;
	fFilterFunction = filter->fFilterFunction;
	fLooper = NULL;
}

/*---------------------------------------------------------------*/

BMessageFilter &BMessageFilter::operator=(const BMessageFilter &from)
{
	// The 'this' filter continues to filter the same object (if any).
	// If 'this' was previously added to some handler/looper it continues
	// to filter that same object - only the parameters of the
	// filtering change.

	fDelivery = from.fDelivery;
	fSource = from.fSource;
	what = from.what;
	fFiltersAny = from.fFiltersAny;
	fFilterFunction = from.fFilterFunction;
	return *this;
}

/*---------------------------------------------------------------*/

BMessageFilter::~BMessageFilter()
{
}

/*---------------------------------------------------------------*/

filter_result BMessageFilter::Filter(BMessage *, BHandler **)
{
	return B_DISPATCH_MESSAGE;
}

/*---------------------------------------------------------------*/

void BMessageFilter::SetLooper(BLooper *looper)
{
	fLooper = looper;
}

/*---------------------------------------------------------------*/

filter_hook BMessageFilter::FilterFunction() const
{
	return fFilterFunction;
}

/*-------------------------------------------------------------*/

message_delivery BMessageFilter::MessageDelivery() const
	{ return fDelivery; };

/*-------------------------------------------------------------*/

message_source BMessageFilter::MessageSource() const
	{ return fSource; };

/*-------------------------------------------------------------*/

uint32 BMessageFilter::Command() const
	{ return what; };

/*-------------------------------------------------------------*/

bool BMessageFilter::FiltersAnyCommand() const
	{ return fFiltersAny; };

/*-------------------------------------------------------------*/

BLooper *BMessageFilter::Looper() const
	{ return fLooper; };

/*-------------------------------------------------------------*/

void BMessageFilter::_ReservedMessageFilter1() {}
void BMessageFilter::_ReservedMessageFilter2() {}

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
