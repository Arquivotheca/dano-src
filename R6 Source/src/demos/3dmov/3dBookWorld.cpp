/* ++++++++++

   FILE:  3dBookWorld.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_BOOK_WORLD_H 
#include "3dBookWorld.h"
#endif
#ifndef _3D_BODY_H 
#include "3dBody.h"
#endif
#ifndef _3D_LENS_H 
#include "3dLens.h"
#endif
#ifndef _3D_CAMERA_H 
#include "3dCamera.h"
#endif
#ifndef _3D_RENDERED_H 
#include "3dRenderer.h"
#endif
#ifndef _3D_LINK_H 
#include "3dLink.h"
#endif

#ifndef _3D_FACE_H 
#include "3dFace.h"
#endif

#include <stdio.h>
#include <Debug.h>

B3dBookWorld::B3dBookWorld(char *name, B3dUniverse *universe) :
B3dEthericWorld(name, universe) {
}

B3dBookWorld::~B3dBookWorld() {
}

long B3dBookWorld::SortThings(B3dVector *camera, B3dMatrix *rotation) {
	long      i, j, cnt, index;
	Node      *node;
	float     z, tx, ty, tz, cx, cy, cz;
	B3dVector *pt;

// get depth for each object
	if (ObjUsed < 0) return 0;
	tx = camera->x;
	ty = camera->y;
	tz = camera->z;
	cx = rotation->m13;
	cy = rotation->m23;
	cz = rotation->m33;
	cnt = 0;
	index = ObjUsed;
	node = ObjList+index;
	while (TRUE) {
		pt = &node->obj->origin;
		z = cx*(pt->x-tx) + cy*(pt->y-ty) + cz*(pt->z-tz);
		DepthList[cnt] = z;
		OrderList[cnt] = index;			
		cnt++;
		index = node->next;
		if (index < 0) break;
		node = ObjList+index;
	}
	return cnt;
}

void B3dBookWorld::RenderHook(B3dCamera *camera, B3dRenderer *renderer) {
	long			ClipState, cnt, i;
	float			radius;
	B3dVector		Offset0, Tr2;
	B3dMatrix		Rot2, rot;
	B3dLighter		*lighter;
	B3dLens		*projector;
	B3dBody		*LOD;
	B3dLightProc	*lightProc;
	
	lighter = camera->Lighter();
	projector = camera->Lens();
	rot /= camera->rotation;
	Offset0 = rot*camera->origin;
	cnt = SortThings(&Offset0, &rot);
	lighter->CollectLight(this, &rot, &camera->origin);
	for (i=0;i<cnt;i++) {
		LOD = (B3dBody*)ObjList[OrderList[i]].obj;
		lightProc = lighter->SelectLight(LOD);
		Rot2 = rot * LOD->rotation;
		Tr2 = rot * LOD->origin - Offset0;
		ClipState = projector->CheckSteric(&LOD->steric, &Rot2, &Tr2, &radius);
		if (ClipState != B_CLIPPED_OUT) {
			while ((radius > LOD->threshold) && (LOD->modelCount > 1))
				LOD = LOD->desc.bodies[1];
			if (LOD->modelCount == 1)
// BACO - Problem child - locks when we call this next function:
				LOD->desc.look->Draw(ClipState, &Rot2, &Tr2, 
					projector, renderer, lightProc);
			else
				LOD->desc.looks[0]->Draw(ClipState, &Rot2, &Tr2,
					projector, renderer, lightProc);
		}
	}
}








