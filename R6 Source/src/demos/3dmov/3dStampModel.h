#ifndef _3D_STAMP_MODEL_H
#define _3D_STAMP_MODEL_H

#ifndef _3D_MODEL_H
#include "3dModel.h"
#endif
#ifndef _3D_CONVEX_MODEL_H
#include "3dConvexModel.h"
#endif

#define   RES_MAX 30

class B3dStamp;

class B3dStampModel : public B3dFaceModel {
 public:
	B3dStampModel(float size, long step, ulong flags);
	virtual ~B3dStampModel();
	void    SetShape(double time);
	virtual void CalcSort(B3dLensImage *lensImage, void **sortDesc);
	
 private:
	long    offset[RES_MAX];
	long    count[RES_MAX];
	long    step;
	long    radius_count;
	ulong   *keys;
	ulong   *tri1;
	ulong   *tri2;
	float   *radius;
	float   *radius2z;
	ushort  *index_radius;
	ushort  *table_norms;
	
	static B3dFaceModelDesc *BuildModel(float size, long step, ulong flags);
};

#endif

 












