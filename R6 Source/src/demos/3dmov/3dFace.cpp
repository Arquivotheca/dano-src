/* ++++++++++

   FILE:  3dFace.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_FACE_H
#include "3dFace.h"
#endif
#ifndef _3D_FACE_MODEL_H 
#include "3dFaceModel.h"
#endif
#ifndef _3D_FACE_LOOK_H 
#include "3dFaceLook.h"
#endif

#include <Debug.h>

B3dFace::B3dFace(char *name) : B3dBody(name) {
}

B3dFace::~B3dFace() {
}

long B3dFace::SetMapLook(char      *name,
						 long      mode,
						 float     *pt_refs,
						 map_desc  *map_list,
						 ulong     status) {
	B3dLook   *myLook;

	if (modelCount > 1) return B_ERROR;
	switch (mode) {
	case B_FACE_COLOR:
//		_sPrintf("Face_color\n");
		myLook = new B3dMap1FaceLook(name, desc.look->model, pt_refs, map_list);
		break;
	case B_CORNER_COLOR:
//		_sPrintf("Corner_color\n");
		myLook = new B3dMap2FaceLook(name, desc.look->model, pt_refs, map_list);
		break;
	default :
		return B_ERROR;
	}
	delete desc.look;
	desc.look = myLook;
//	myLook->model->Debug();
	return B_NO_ERROR;
}

bool B3dFace::GetTouch(B3dVector *origin, B3dVector *axis, B3dVector *touch, int *index_face) {
	long           i;
	float          scal, scal_min, val1, sign;
	B3dFaceDesc    *face;
	B3dVector      delta, proj;
	B3dVector      *p1, *p2, *p3;
	B3dVector      *norm;
	B3dFaceModel   *model;
	B3dBody        *obj;
	map_desc       *desc;
	
	scal_min = 2e20;
	obj = this;
	while (obj->modelCount > 1)
		obj = obj->desc.bodies[1];
	model = (B3dFaceModel*)(obj->desc.look->model);
	desc = ((B3dMapFaceLook*)(obj->desc.look))->map_list;
	for (i=0; i<model->faceCount; i++) {
		face = model->faces+i;
		p1 = model->points+face->points[0];
		norm = model->norms+face->norm;
		if (desc[i].status & B_INVERT_NORM)
			sign = -1.0;
		else
			sign = 1.0;
		delta = (*p1)-(*origin);
		val1 = delta*(*norm);
		if (val1*sign >= 0.0) continue;
		scal = delta*(*axis);
		if (scal <= 0.0) continue;
		scal = val1/((*axis)*(*norm));
		if (scal > scal_min) continue;
		proj = (*origin)+(*axis)*scal;
		p2 = model->points+face->points[1];
		p3 = model->points+face->points[2];
		if (((proj-(*p1))^((*p2)-(*p1)))*(*norm)*sign < 0.0) continue;
		if (((proj-(*p2))^((*p3)-(*p2)))*(*norm)*sign < 0.0) continue;
		if (((proj-(*p3))^((*p1)-(*p3)))*(*norm)*sign < 0.0) continue;
		scal_min = scal;
		*index_face = i;
		*touch = proj;
	}
	return (scal_min < 2e20);
}









