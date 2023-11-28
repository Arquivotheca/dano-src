#ifndef _3D_AMBIENT_LIGHT_H
#define _3D_AMBIENT_LIGHT_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_LIGHT_H
#include "3dLight.h"
#endif

class B3dAmbientLight : public B3dLight {
 public:
	B3dAmbientLight(char *name, float power, RGBAColor *color);
	virtual ~B3dAmbientLight();
};

#endif
