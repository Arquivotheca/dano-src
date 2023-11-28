#ifndef _3D_PAGE_MODEL_H
#define _3D_PAGE_MODEL_H

#ifndef _3D_MODEL_H
#include "3dModel.h"
#endif
#ifndef _3D_CONVEX_MODEL_H
#include "3dConvexModel.h"
#endif

enum {
	B_FRONT_FACES = 0x0010,
	B_REAR_FACES = 0x0020
};

class B3dPageModel : public B3dConvexModel {
 public:
	B3dPageModel(float size, long step, ulong flags);
	virtual ~B3dPageModel();
	void    SetShape(float alpha, float inertia, float traction);
	long   step;
	ulong  flags;
	float  size;
	
 private:
	static B3dFaceModelDesc *BuildModel(float size, long step, ulong flags);
};

#endif

 












