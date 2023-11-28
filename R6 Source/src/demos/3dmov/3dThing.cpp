/* ++++++++++

   FILE:  3dThing.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_THING_H
#include "3dThing.h"
#endif
#ifndef _3D_LINK_H
#include "3dLink.h"
#endif

B3dThing::B3dThing(char *name) {
	long     i;

	for (i=0;i<B_3D_NAME_LENGTH-1;i++)
		myName[i] = name[i];
	myName[B_3D_NAME_LENGTH-1] = 0;
	link = 0L;
	linkCount = 0;
	status = 0;
	MoveTo(&B_VECTOR_NULL, 0L);
	highlightColor.Set(0.0, 0.0, 0.0, 1.0);
}

B3dThing::~B3dThing() {
}

void B3dThing::MoveTo(B3dVector *Origin, B3dMatrix *Rotation) {
	origin = *Origin;
	if (Rotation == 0L) {
		rotation = B_MATRIX_ID;
//		Status &= ~B_ROTATED_OBJECT;
	}
	else {		
		rotation = *Rotation;
//		Status |= B_ROTATED_OBJECT;
	}
}

long B3dThing::LookAt(B3dVector *cible, ulong mode) {
	switch (mode) {
	case B_HORIZONTAL_VIEW :
		{
			float      norm;
			B3dVector  Vx, Vy, Vz;
			B3dMatrix  Mat;
			
			Vz = *cible-origin;
			Vz.Norm();
			Vx.Set(0.0, 1.0, 0.0);
			Vy = Vz^Vx;
			norm = Vy.Norm(TRUE);
			if (norm < 1e-5) {
				Vx.Set(0.0, 0.0, 1.0);
				Vy = Vx^Vz;
				Vy.Norm();
			}
			Vx = Vy^Vz;
			Vx.Norm();
			Mat.Set(Vx, Vy, Vz);
			rotation = Mat;
		}
		break;
	default:
		return B_ERROR;
	}
	return B_NO_ERROR;
}

void B3dThing::LinkTo(B3dLink *Link) {
	if (link != 0L) {
		link->referenceCount--;
		if (link->referenceCount == 0)
			delete link;
	}
	link = Link;
	Link->referenceCount++;
	link->DoLink(this, 0.0, 0.0);
}

bool B3dThing::GetTouch(B3dVector *origin, B3dVector *axis, B3dVector *touch, int *index) {
	return FALSE;
}

void B3dThing::Debug() {
	fprintf(stderr,"Object: %s\n",Name());
	fprintf(stderr,"  Status:%x\n",status);
	origin.Debug();
	rotation.Debug();
}












