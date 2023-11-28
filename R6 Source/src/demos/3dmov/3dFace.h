#ifndef _3D_FACE_H
#define _3D_FACE_H

#include <unistd.h>

#ifndef _3D_BODY_H 
#include "3dBody.h"
#endif
#ifndef _3D_FACE_MODEL_H 
#include "3dFaceModel.h"
#endif
#ifndef _3D_FACE_LOOK_H 
#include "3dFaceLook.h"
#endif

enum {
	B_INVERT_NORM = 0x0001
};

class B3dFace : public B3dBody {
 public:
	B3dFace(char *name);
	virtual ~B3dFace();
	virtual bool GetTouch(B3dVector *origin, B3dVector *axis, B3dVector *touch, int *index);
	virtual long SetMapLook(char     *name,
							long     mode,
							float    *pt_refs,
							map_desc *map_list,
							ulong    status = 0L);
};

#endif







