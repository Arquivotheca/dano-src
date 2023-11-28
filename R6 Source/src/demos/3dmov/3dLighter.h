#ifndef _3D_LIGHTER_H
#define _3D_LIGHTER_H

#include <InterfaceDefs.h>

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
#ifndef _3D_LIGHT_PROC_H 
#include "3dLightProc.h"
#endif

enum {
	B_MAX_PARALLEL_LIGHT_COUNT = 4,
	B_MAX_RADIAL_LIGHT_COUNT =   4
};

class B3dLightProcDesc {
 public:
	RGBAColor   color;
	B3dVector   vector;

	void AdjustColor(B3dLightProcDesc *dup, RGBAColor *color);
};

class B3dLightProc_1 : public B3dLightProc {
 public:
	long             parallelLightCount;
	long             radialLightCount;
	RGBAColor        ambientColor;

	B3dLightProcDesc parallelProcs[B_MAX_PARALLEL_LIGHT_COUNT];
	B3dLightProcDesc radialProcs[B_MAX_RADIAL_LIGHT_COUNT];

	void           SetHighlightColor(B3dLightProc_1 *dup, RGBAColor *color);
	virtual ulong  Calc32(B3dVector *point, B3dVector *norm, float sign);
};

class B3dWorld;

class B3dLighter {
 public:
	B3dLighter();
	virtual ~B3dLighter();

	virtual void CollectLight(B3dWorld *world, B3dMatrix *rotation, B3dVector *origin);
	virtual B3dLightProc *SelectLight(B3dThing *thing);
 private:
	B3dLightProc_1    LP;
	B3dLightProc_1    LP_copy;
};

#endif















