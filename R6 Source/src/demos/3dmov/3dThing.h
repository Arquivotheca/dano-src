#ifndef _3D_THING_H
#define _3D_THING_H

#include <unistd.h>

#ifndef _3D_DEFS_H 
#include "3dDefs.h"
#endif
#ifndef _RGBA_COLOR_H 
#include "RGBAColor.h"
#endif
#ifndef _3D_LINK_H 
#include "3dLink.h"
#endif

enum {
	B_HORIZONTAL_VIEW
};

class B3dThing {
	friend class B3dBody;

 public:
	ulong     status;
	ulong     linkCount;
	B3dLink   *link;
	B3dVector origin;
	B3dMatrix rotation;
	B3dSteric steric;
	RGBAColor highlightColor;
	
	B3dThing(char *name);
	virtual ~B3dThing();
	inline void    MoveTo(B3dVector *origin);
	inline void    MoveTo(float new_x, float new_y, float new_z);
	void           MoveTo(B3dVector *origin, B3dMatrix *rotation);
	inline void    LinkTo(B3dVector *axis,
						  float speed,
						  long mode = B_WORLD_REFERENCE,
						  B3dVector *origin = 0L);
	inline void    LinkTo(float speed_alpha,
						  float speed_theta,
						  float speed_phi,
						  B3dVector *origin = 0L);
	inline void    LinkTo(B3dThing *from, B3dVector *to, B3dMatrix *to_mat, float speed);
	void           LinkTo(B3dLink *link);
	long           LookAt(B3dVector *target, ulong mode = B_HORIZONTAL_VIEW);
	inline long    LookAt(B3dThing *target, ulong mode = B_HORIZONTAL_VIEW);
	virtual void   Debug();
	virtual bool   GetTouch(B3dVector *origin, B3dVector *axis, B3dVector *touch, int *index);
	inline const char *Name();
 private:
	char           myName[B_3D_NAME_LENGTH];
};

void B3dThing::MoveTo(float new_x, float new_y, float new_z) {
	origin.x = new_x;
	origin.y = new_y;
	origin.z = new_z;	
}

void B3dThing::MoveTo(B3dVector *origin2) {
	origin = *origin2;
}

long B3dThing::LookAt(B3dThing *target, ulong mode) {
	return LookAt(&target->origin, mode);
}

const char *B3dThing::Name() {
	return myName;
}

void B3dThing::LinkTo(B3dVector *p_axis, float p_speed,
					  long p_mode, B3dVector *p_origin) {
	LinkTo((B3dLink*)new B3dLink0(p_axis, p_speed, p_mode, p_origin));
}

void B3dThing::LinkTo(float speed_alpha, float speed_theta,
					  float speed_phi, B3dVector *origin) {
	LinkTo((B3dLink*)new B3dLink1(speed_alpha, speed_theta, speed_phi, origin));
}

void B3dThing::LinkTo(B3dThing *from, B3dVector *to, B3dMatrix *to_mat, float speed) {
	LinkTo((B3dLink*)new B3dLink2(from, to, to_mat, speed));
}

#endif








