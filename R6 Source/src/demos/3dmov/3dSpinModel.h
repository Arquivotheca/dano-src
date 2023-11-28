#ifndef _3D_SPIN_MODEL_H
#define _3D_SPIN_MODEL_H

#ifndef _3D_FACE_MODEL_H
#include "3dFaceModel.h"
#endif
#ifndef _2D_VECTOR_H
#include "2dVector.h"
#endif

class B3dSpinModel : public B3dFaceModel {
 public:
	float      *divider;
	long       radiusCount;
	long       bandCount;
	
	B3dSpinModel(B2dVector *shape, long shapeCount, long radiusCount, ulong flags);
	virtual ~B3dSpinModel();
	virtual void CalcSort(B3dLensImage *lensImage, void **sortDesc);
 private:
	static B3dFaceModelDesc *BuildModel(B2dVector *shape, long shapeCount, long radiusCount);
};

#endif

 












