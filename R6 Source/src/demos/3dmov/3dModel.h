#ifndef _3D_MODEL_H
#define _3D_MODEL_H

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _3D_LIGHTER_H
#include "3dLighter.h"
#endif

enum {
	B_BASIC_MODEL = 0x0001,
	B_OWN_POINT_LIST = 0x00000001
};

class B3dLensImage;

class B3dModel {
 public:
	ulong      referenceCount;
	long       pointCount;
	long       vectorCount;
	long       normCount;
	B3dVector  *points;
	B3dVector  *vectors;
	B3dVector  *norms;
	B3dSteric  steric;
	
	B3dModel(long numPt, B3dVector *points, ulong OwnFlag = 0);
	virtual ~B3dModel();
	virtual void CalcSort(B3dLensImage *lensImage, void **sortDesc);
	virtual void Debug();
};

#endif

 









