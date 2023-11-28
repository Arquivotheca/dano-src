/* ++++++++++

   FILE:  3dBody.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_BODY_H
#include "3dBody.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <stdlib.h>
#include <string.h>

B3dBody::B3dBody(char *name) : B3dThing(name) {
	threshold = -1.0;
}

B3dBody::~B3dBody() {
	long       i;
	
	if (flags & B_BASIC_BODY) {
		if (modelCount == 1) {
			if ((--desc.look->referenceCount) == 0)
				delete desc.look;
		}
		else {
			if ((--desc.looks[0]->referenceCount) == 0)
				delete desc.looks[0];
		}
	}
	else
		delete desc.bodies[0];
	for (i=1;i<modelCount;i++)
		delete desc.bodies[i];
	if (modelCount > 1)
		free((void*)desc.bodies);
}

B3dBody *B3dBody::Clone(char *name, long cloneFlag) {
	int         i;
	B3dBody     *obj;

	if (name == 0L) name = &myName[0];
	obj = new B3dBody(name);
	memcpy((void*)obj, (void*)this, sizeof(B3dBody));

	if (cloneFlag & B_CLONE_LINK) {
		if (link != 0L)
			link->referenceCount++;
	}
	else obj->link = 0L;
		
	if (flags & B_BASIC_BODY) {
		if (modelCount == 1)
			desc.look->referenceCount++;
		else
			desc.looks[0]->referenceCount++;
	}
	else
		obj->desc.bodies[0] = desc.bodies[0]->Clone();
	for (i=1;i<modelCount;i++)
		obj->desc.bodies[i] = desc.bodies[i]->Clone();
	return obj;
}

void B3dBody::SwitchBody(B3dBody *newBody) {
	ushort      cflags;
	ushort      cmodelCount;
	B3dVector   cscaler;
	float       cthreshold;
	B3dBodyDesc cdesc;

	cflags = flags;
	cmodelCount = modelCount;
	cscaler = scaler;
	cthreshold = threshold;
	cdesc = desc;
	flags = newBody->flags;
	modelCount = newBody->modelCount;
	scaler = newBody->scaler;
	threshold = newBody->threshold;
	desc = newBody->desc;
	newBody->flags = cflags;
	newBody->modelCount = cmodelCount;
	newBody->scaler = cscaler;
	newBody->threshold = cthreshold;
	newBody->desc = cdesc;
}

long B3dBody::SetFlatLook(char *name,
						  long mode,
						  RGBAColor *color,
						  RGBAColor *colorList,
						  ulong status) {
	return B_ERROR;
}

bool B3dBody::GetTouch(B3dVector *origin, B3dVector *axis, B3dVector *touch, int *index) {
	return FALSE;
}

void B3dBody::SizeTo(float size_x, float size_y, float size_z) {
//	float       min, max, d2;
//	BSteric     *gab;

	if (modelCount == 1)
		steric = desc.look->model->steric;
/*
	Resize.x = size_x;
	Resize.y = size_y;
	Resize.z = size_z;
	gab = &ObjectList.Obj->myModel->Steric;
	Steric.PtMin.x = min = gab->PtMin.x*size_x;
	Steric.PtMax.x = max = gab->PtMax.x*size_x;
	if (min+max < 0)
		d2 = min*min;
	else
		d2 = max*max;
	Steric.PtMin.y = min = gab->PtMin.y*size_y;
	Steric.PtMax.y = max = gab->PtMax.y*size_y;
	if (min+max < 0)
		d2 += min*min;
	else
		d2 += max*max;
	Steric.PtMin.z = min = gab->PtMin.z*size_z;
	Steric.PtMax.z = max = gab->PtMax.z*size_z;
	if (min+max < 0)
		d2 += min*min;
	else
		d2 += max*max;
	Steric.Radius = d3_sqrt(d2);
*/
}

B3dBody *tie_bodies(B3dBody *complex, B3dBody *simple, float threshold_size) {
	B3dLook     *look;

	if (simple->modelCount != 1) return 0L;
	look = simple->desc.look;
	memcpy(simple, complex, sizeof(B3dBody));
	simple->modelCount = 2;
	simple->flags = B_BASIC_BODY;
	simple->desc.bodies = (B3dBody**)malloc(sizeof(B3dBody*)*2);
	simple->desc.looks[0] = look;
	simple->desc.bodies[1] = complex;
	simple->threshold = threshold_size*0.65;
	return simple;
}




















