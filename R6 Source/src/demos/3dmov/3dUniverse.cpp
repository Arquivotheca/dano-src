/* ++++++++++

   FILE:  3dUniverse.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_UNIVERSE_H 
#include "3dUniverse.h"
#endif
#ifndef _3D_WORLD_H 
#include "3dWorld.h"
#endif

B3dUniverse::B3dUniverse(char *name) {
	long     i;
	
// name
	for (i=0;i<B_3D_NAME_LENGTH-1;i++)
		Name[i] = name[i];
	Name[B_3D_NAME_LENGTH-1] = 0;
	Timer = 1.0;
	Time0 = system_time();;
	Time1 = 0.0;
}

B3dUniverse::~B3dUniverse() {
}

void B3dUniverse::AddWorld(B3dWorld *World) {
}

long B3dUniverse::RemoveWorld(B3dWorld *World) {
	return B_ERROR;
}

double B3dUniverse::Time() {
	double  t0, time;

	t0 = system_time();
	time = t0-Time0;
	Time0 = t0;
	time *= Timer;
	time += Time1;
	Time1 = time;
	return time;
}

void B3dUniverse::SetTime(double time) {
	Time0 = system_time();
	Time1 = time;
}

void B3dUniverse::SetTimeMode(long mode, float timer) {
	switch (mode) {
	case B_REAL_TIME:
		Timer = 1.0;
		break;
	case B_FROZEN_TIME:
		Timer = 0.0;
		break;
	case B_EXPANDED_TIME:
		Timer = timer;
		break;
	}
}














