/* ++++++++++

   FILE:  3dLink.c
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_LINK_H
#include "3dLink.h"
#endif
#ifndef _3D_THING_H
#include "3dThing.h"
#endif

B3dLink::B3dLink() {
	referenceCount = 0;
}

B3dLink::~B3dLink() {
}

void B3dLink::DoLink(B3dThing *Object, double time0, double time1) {
}

B3dLink0::B3dLink0(B3dVector *axis,
				   float speed,
				   long mode,
				   B3dVector *origin) {
	Mode = mode;
	Axis = *axis;
	Speed = speed;
	Origin = origin;
}

B3dLink0::~B3dLink0() {
}

void B3dLink0::DoLink(B3dThing *Object, double time0, double time1) {
	float     delta;
	B3dMatrix rot;
	B3dVector v;
	
	delta = (time1-time0)*1e-6;
	rot.Set(&Axis, Speed*delta);
	if (Mode == B_WORLD_REFERENCE)
		Object->rotation = rot * Object->rotation;
	else if (Mode == B_THING_REFERENCE)
		Object->rotation *= rot;
	if (Origin != 0L) {
		v = Object->origin-(*Origin);
		delta = v.Norm(TRUE);
		Object->origin = ((rot*v)*delta) + (*Origin);
	}
	if (((long)delta & 7) == 0)
		Object->rotation.Norm();
}

B3dLink1::B3dLink1(float speed_alpha,
				   float speed_theta,
				   float speed_phi,
				   B3dVector *origin) {
	Speed[0] = speed_alpha;
	Speed[1] = speed_theta;
	Speed[2] = speed_phi;
	Origin = origin;
	Port.x = 1.278e20;
	Pos[0] = Pos[1] = Pos[2] = 0.0;
}

B3dLink1::~B3dLink1() {
}

void B3dLink1::DoLink(B3dThing *Object, double time0, double time1) {
	long      i;
	float     delta;

	if (Port.x == 1.278e20) {
		if (Origin != 0L)
			Port = Object->origin-(*Origin);
		else
			Port.x = 0;
	}
	delta = (time1-time0)*1e-6;
	for (i=0;i<3;i++) {
		Pos[i] += delta*Speed[i];
		if (Pos[i] < 0.0) Pos[i] += 2*3.141592654;
		else if (Pos[i] > 2*3.141592654) Pos[i] -= 2*3.141592654;
	}
	Object->rotation.Set(Pos[0], Pos[1], Pos[2]);
	if (Origin != 0L)
		Object->origin = Object->rotation*Port + (*Origin);
}

B3dLink2::B3dLink2(B3dThing *from, B3dVector *to, B3dMatrix *to_mat, float speed) {
	float     length;
	B3dMatrix rot;
	
	Move = (*to)-from->origin;
	length = Move.Norm(TRUE);
	Move *= speed;
	term = length/speed;
	rot /= from->rotation;
	rot *= (*to_mat);
	rot.GetAxialRotateZ(&Axis, &Speed);
	Speed /= term;
}

B3dLink2::~B3dLink2() {
}

void B3dLink2::DoLink(B3dThing *Object, double time0, double time1) {
	long      i;
	float     delta;
	B3dMatrix rot;

	if (term <= 0.0) return;
	delta = (time1-time0)*1e-6;
	if (delta < term) delta = term;
	term -= delta;
	Object->origin += Move*delta;
	rot.Set(&Axis, Speed*delta);
	Object->rotation = rot * Object->rotation;
}
















