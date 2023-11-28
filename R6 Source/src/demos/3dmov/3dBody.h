#ifndef _3D_BODY_H
#define _3D_BODY_H

#include <unistd.h>

#ifndef _3D_THING_H 
#include "3dThing.h"
#endif
#ifndef _3D_MODEL_H 
#include "3dModel.h"
#endif
#ifndef _3D_LOOK_H 
#include "3dLook.h"
#endif

class B3dBody;

enum {
	B_BASIC_BODY = 0x0001,

	B_CLONE_LINK = 0x0001
};

typedef union {
	B3dLook    *look;
	B3dLook    **looks;
	B3dBody    **bodies;
} B3dBodyDesc;

class B3dBody : public B3dThing {
 public:
	ushort      flags;
	ushort      modelCount;
	B3dVector   scaler;
	float       threshold;
	B3dBodyDesc desc;

	B3dBody(char *name);
	virtual        ~B3dBody();
	virtual long   SetFlatLook(char *name,
							   long mode,
							   RGBAColor *color,
							   RGBAColor *colorList = 0L,
							   ulong status = B_SMOOTH_SHADE);
	virtual bool   GetTouch(B3dVector *origin, B3dVector *axis, B3dVector *touch, int *index);
	void           SwitchBody(B3dBody *newBody);
	B3dBody        *Clone(char *name = 0L, long cloneFlag = 0);
	inline void    SizeTo(B3dVector *size);
	void           SizeTo(float size_x, float size_y, float size_z);
};

void B3dBody::SizeTo(B3dVector *origin) {
	SizeTo(origin->x, origin->y, origin->z);
}

B3dBody *tie_bodies(B3dBody *complex, B3dBody *simple, float threshold_size);

#endif







