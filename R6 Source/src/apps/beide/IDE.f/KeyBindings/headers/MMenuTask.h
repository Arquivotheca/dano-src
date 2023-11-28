//========================================================================
//	MMenuTask.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#pragma once

#include "MThread.h"


class MMenuTask : public MThread 
{
public:
 								MMenuTask(
									BMenuBar&	inMenuBar); 
								~MMenuTask();

virtual	status_t				Execute();

private:
	
		BMenuBar&			fMenuBar;
		uint32				fModifiers;

virtual	void					LastCall();
};
