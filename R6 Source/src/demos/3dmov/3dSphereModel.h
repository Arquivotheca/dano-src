#ifndef _3D_SPHERE_MODEL_H
#define _3D_SPHERE_MODEL_H

#ifndef _3D_CONVEX_MODEL_H
#include "3dConvexModel.h"
#endif

class B3dSphereModel : public B3dConvexModel {
 public:
	B3dSphereModel(B3dVector *size, ulong faceCount, ulong flags);
	virtual ~B3dSphereModel();
 private:
	static B3dFaceModelDesc *BuildModel(B3dVector *size, ulong faceCount);
};

#endif

 












