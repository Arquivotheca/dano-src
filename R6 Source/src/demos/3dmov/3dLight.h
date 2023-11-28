#ifndef _3D_LIGHT_H
#define _3D_LIGHT_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_THING_H
#include "3dThing.h"
#endif

enum {
	B_AMBIENT_LIGHT = 0,
	B_PARALLEL_LIGHT = 1,
	B_RADIAL_LIGHT =   2
};

class B3dLight : public B3dThing {
 public:
	float    power;
	short    type;
	
	B3dLight(char *name, float power, RGBAColor *color);
	virtual ~B3dLight();
};

#endif



