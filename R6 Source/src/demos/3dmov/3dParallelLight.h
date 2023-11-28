#ifndef _3D_PARALLEL_LIGHT_H
#define _3D_PARALLEL_LIGHT_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_LIGHT_H
#include "3dLight.h"
#endif

class B3dParallelLight : public B3dLight {
 public:
	B3dParallelLight(char *name, float power, RGBAColor *color, B3dVector *direction);
	virtual ~B3dParallelLight();
};

#endif
