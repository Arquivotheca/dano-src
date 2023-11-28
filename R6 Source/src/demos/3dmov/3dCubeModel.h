#ifndef _3D_CUBE_MODEL_H
#define _3D_CUBE_MODEL_H

#ifndef _3D_CONVEX_MODEL_H
#include "3dConvexModel.h"
#endif

class B3dCubeModel : public B3dConvexModel {
 public:
	B3dCubeModel(B3dVector *size, ulong flags);
	virtual ~B3dCubeModel();
 private:
	static B3dFaceModelDesc *BuildModel(B3dVector *size);
};

#endif

 












