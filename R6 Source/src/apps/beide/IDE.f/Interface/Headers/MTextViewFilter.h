//========================================================================
//	MTextControlFilter.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#pragma once

#include <MessageFilter.h>


class MTextControlFilter : public BMessageFilter
{
public:
								MTextControlFilter();
								~MTextControlFilter();

virtual	filter_result			Filter(
									BMessage *message, 
									BHandler **target);

};