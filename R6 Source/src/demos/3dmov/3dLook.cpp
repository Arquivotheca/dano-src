/* ++++++++++

   FILE:  3dLooks.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif

RGBAColor RGBADefaultColor = { 0.5, 0.5, 0.5, 0.0 };

B3dLook::B3dLook(char *Name, B3dModel *Model) {
	long       i;

// name
	for (i=0;i<B_3D_NAME_LENGTH-1;i++)
		name[i] = Name[i];
	name[B_3D_NAME_LENGTH-1] = 0;
// refs
	model = Model;
	Model->referenceCount++;
	referenceCount = 1;
}

B3dLook::~B3dLook() {
	if ((--model->referenceCount) == 0)
		delete model;
}

void B3dLook::Draw(ulong         flags,
				   B3dMatrix     *Rot,
				   B3dVector     *Trans,
				   B3dLens       *Proj,
				   B3dRenderer   *Buf,
				   B3dLightProc  *lightProc) {
}

void *B3dLook::ClipHook(void *look1, float rel1, void *look2, float rel2) {
	if (rel1 > rel2)
		return look1;
	else
		return look2;
}

















