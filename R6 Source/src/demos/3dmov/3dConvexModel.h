#ifndef _3D_CONVEX_MODEL_H
#define _3D_CONVEX_MODEL_H

#ifndef _3D_FACE_MODEL_H
#include "3dFaceModel.h"
#endif

class B3dConvexModel : public B3dFaceModel {
 public:
	B3dConvexModel(B3dFaceModelDesc *desc, ulong flags, ulong OwnFlag = 0);
	virtual void CalcSort(B3dLensImage *lensImage, void **sortDesc);
};

#endif

 






