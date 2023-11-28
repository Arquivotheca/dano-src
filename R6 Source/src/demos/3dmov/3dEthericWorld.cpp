/* ++++++++++

   FILE:  3dEthericWorld.cpp
   REVS:  $Revision: 1.3 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_ETHERIC_WORLD_H 
#include "3dEthericWorld.h"
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

B3dEthericWorld::B3dEthericWorld(char *name, B3dUniverse *universe) :
B3dWorld(name, universe) {
	long    i;

	ListCount = 64;
	ObjList = (Node*)malloc(sizeof(Node)*ListCount);
	ObjFree = 0;
	ObjUsed = -1;
	for (i=0;i<ListCount;i++) {
		ObjList[i].next = i+1;
		ObjList[i].obj = 0L;
	}
	ObjList[ListCount-1].next = -1;
	OrderList = (long*)malloc(sizeof(long)*ListCount);
	DepthList = (float*)malloc(sizeof(float)*ListCount);
}

B3dEthericWorld::~B3dEthericWorld() {
	long     i;
	
	for (i=0;i<ListCount;i++)
		if (ObjList[i].obj != 0L)
			RemoveThing(i);
	free(ObjList);
	free(OrderList);
	free(DepthList);
}

void B3dEthericWorld::ExtendList() {
	long     i;
	Node     *newList;
	
	newList = (Node*)malloc(sizeof(Node)*ListCount*2);
	memcpy(newList, ObjList, sizeof(Node)*ListCount);
	for (i=ListCount;i<2*ListCount;i++)
		newList[i].next = i+1;
	newList[2*ListCount-1].next = -1;
	ObjFree = ListCount;
	free (ObjList);
	free(OrderList);
	free(DepthList);
	ObjList = newList;
	ListCount *= 2;
	OrderList = (long*)malloc(sizeof(long)*ListCount);
	DepthList = (float*)malloc(sizeof(float)*ListCount);
}

void B3dEthericWorld::AddThing(B3dThing *thing, ulong flags) {
	long       index;

	if (ObjFree < 0) ExtendList();
	index = ObjFree;
	ObjFree = ObjList[index].next;
	ObjList[index].obj = thing;
	ObjList[index].next = ObjUsed;
	ObjUsed = index;
}

void B3dEthericWorld::RemoveThing(long id) {
	long     index, next;

	index = ObjUsed;
	if (index == id) {
		ObjUsed = ObjList[index].next;
		ObjList[index].next = ObjFree;
		ObjFree = index;
	}
	else while (ObjList[index].next >= 0) {
		if (ObjList[index].next == id) {
			next = ObjList[id].next;
			ObjList[index].next = next;
			ObjList[id].next = ObjFree;
			ObjFree = id;
			return;
		}
		index = ObjList[index].next; 
	}
}

B3dThing *B3dEthericWorld::GetThing(long id) {
	if ((id>=0) && (id<ListCount))
		return ObjList[id].obj;
	else
		return 0L;
}

long B3dEthericWorld::GetThingId(char *name) {
	long          i, j;
	const char    *oName;
	
	for (i=0;i<ListCount;i++) {
		if (ObjList[i].obj == 0L) continue;
		oName = ObjList[i].obj->Name();
		j = -1;
		do {
			j++;
			if (name[j] != oName[j]) goto next_i;
		} while (name[j] != 0);
		return i;
	next_i:;
	}
	return -1;
}

long B3dEthericWorld::GetNthThingId(long index) {
	long    i;

	i = ObjUsed;
	for (;index>0;index--)
		if (ObjList[i].next >= 0) i = ObjList[i].next;
		else return -1;
	return i;
}

void B3dEthericWorld::CheckNewThings() {
}

long B3dEthericWorld::SortThings(B3dVector *camera, B3dMatrix *rotation) {
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
// sort depth (using stupid n2 algorythm)
	for (i=0;i<cnt-1;i++)
		for (j=0;j<cnt-1;j++)
			if (DepthList[j] < DepthList[j+1]) {
				index = OrderList[j];
				z = DepthList[j];
				OrderList[j] = OrderList[j+1];
				DepthList[j] = DepthList[j+1];
				OrderList[j+1] = index;
				DepthList[j+1] = z;
			}
	return cnt;
}

void B3dEthericWorld::UpdateThings() {	
	long          i;
	B3dThing      *obj;

	for (i=0;i<ListCount;i++) {
		if (ObjList[i].obj == 0L) continue;
		obj = ObjList[i].obj;
		if (obj->link != 0L)
			obj->link->DoLink(obj, currentFrameTime, prevFrameTime);
	}
}

void B3dEthericWorld::RenderHook(B3dCamera *camera, B3dRenderer *renderer) {
	long          ClipState, cnt, i;
	float         radius;
	B3dVector     Offset0, Tr2;
	B3dMatrix     Rot2, rot;
	B3dLighter    *lighter;
	B3dLens       *projector;
	B3dBody       *LOD;
	B3dLightProc  *lightProc;

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
				LOD->desc.look->Draw(ClipState, &Rot2, &Tr2,
									 projector, renderer, lightProc);
			else
				LOD->desc.looks[0]->Draw(ClipState, &Rot2, &Tr2,
										 projector, renderer, lightProc);
		}
	}
}

bool B3dEthericWorld::GetTouch(B3dVector *origin, B3dVector *axis, B3dTouchDesc *touch) {
	long          i;
	int           index_face;
	float         d, pos, pos_min;
	B3dVector       ecart, orig0, axis0, pouf, pouf0;
	B3dMatrix       rot;
	B3dThing   *obj;

	pos_min = 1e20;
	for (i=0;i<ListCount;i++) {
		if (ObjList[i].obj == 0L) continue;
		obj = ObjList[i].obj;
		ecart = (*origin)-obj->origin;
		pos = ecart*(*axis);
		if (pos > 0) continue;
		d = (ecart^(*axis)).Length();
		if (d < obj->steric.radius) {
			rot /= obj->rotation;
			orig0 = rot*ecart;
			axis0 = rot*(*axis);
			if (obj->GetTouch(&orig0, &axis0, &pouf0, &index_face)) {
				pouf = (obj->rotation*pouf0)+obj->origin;
				ecart = pouf-(*origin);
				pos = ecart*(*axis);
				if (pos < pos_min) {
					touch->index_face = index_face;
					touch->side = pouf0*axis0;
					touch->touch = pouf;
					touch->obj = obj;
					pos_min = pos;
				}
			}
		}
	}
	return (pos_min < 1e20);
}














