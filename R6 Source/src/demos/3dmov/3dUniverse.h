#ifndef _3D_UNIVERSE_H
#define _3D_UNIVERSE_H

#ifndef _3D_DEFS_H
#include "3dDefs.h"
#endif

enum {
	B_REAL_TIME,
	B_FROZEN_TIME,
	B_EXPANDED_TIME
};

class B3dWorld;

class B3dUniverse {
 public:
	B3dUniverse(char *name);
	virtual ~B3dUniverse();
	void AddWorld(B3dWorld *world);
	long RemoveWorld(B3dWorld *world);
	double Time();
	void SetTime(double time);
	void SetTimeMode(long mode, float timer);
 private:
	char     Name[B_3D_NAME_LENGTH];
	double   Time0, Time1;
	float    Timer;
};

#endif
















