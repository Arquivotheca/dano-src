#ifndef _3D_LIGHT_PROC_H
#define _3D_LIGHT_PROC_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_AMBIENT_LIGHT_H 
#include "3dAmbientLight.h"
#endif
#ifndef _3D_PARALLEL_LIGHT_H 
#include "3dParallelLight.h"
#endif
//#ifndef _3D_RADIAL_LIGHT_H 
//#include "3dRadialLight.h"
//#endif

class B3dLightProc {
 public:
	virtual ulong  Calc32(B3dVector *point, B3dVector *norm, float sign);
};

#endif










