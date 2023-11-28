#ifndef _3D_SPHERE_H
#define _3D_SPHERE_H

#include <unistd.h>

#ifndef _3D_FACE_H 
#include "3dFace.h"
#endif
#ifndef _3D_SPHERE_MODEL_H 
#include "3dSphereModel.h"
#endif

class B3dSphere : public B3dFace {
 public:
	B3dSphere(char *name, B3dVector *size, ulong faceCount, ulong flags = 0);
	virtual ~B3dSphere();
	void    SetMapping(struct map_ref *map);
 private:
	B3dVector  mySize;
	ulong      myFace;
};

#endif

