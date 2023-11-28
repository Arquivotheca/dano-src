#ifndef _3D_CUBE_H
#define _3D_CUBE_H

#include <unistd.h>

#ifndef _3D_FACE_H 
#include "3dFace.h"
#endif
#ifndef _3D_CUBE_MODEL_H 
#include "3dCubeModel.h"
#endif

class B3dCube : public B3dFace {
 public:
	B3dCube(char *name, B3dVector *size, ulong flags = 0);
	virtual ~B3dCube();
	void    SetMapping(struct map_ref *list_map);
};

#endif







