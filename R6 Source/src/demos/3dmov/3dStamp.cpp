/* ++++++++++

   FILE:  3dStamp.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_STAMP_H
#include "3dStamp.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif
#ifndef _3D_STAMP_MODEL_H 
#include "3dStampModel.h"
#endif

#include <Debug.h>

static ulong default_bitmap = 0xffffff00L;
static map_ref default_map = { (void*)&default_bitmap, 0, 0 };

B3dStamp::B3dStamp(char *name, float size, long step, ulong Flags) : B3dFace(name) {
	int            i, k, index;
	B3dStampModel  *myModel;
	B3dLook        *myLook;   
	BRect          rect;
	BPoint         point;
	
	myModel = new B3dStampModel(size, step, Flags);
	myLook = new B3dLook("Default Look", myModel);
	flags = B_BASIC_BODY;
	modelCount = 1;
	threshold = -1.0;
	desc.look = myLook;
	SizeTo(1.0, 1.0, 1.0);

	step0 = step;
	size0 = size;
}

B3dStamp::~B3dStamp() {
}

void B3dStamp::SetMapping(map_ref *list_map) {
	int           i, nb_pt, nb_face;
	float         coeff;
	float         *offset;
	map_desc      *d2, *d;
	B3dVector     *curPt;
	B3dFaceDesc   *curFace;
	B3dStampModel *model;

	model = (B3dStampModel*)(desc.look->model);
	nb_pt = model->pointCount;
	nb_face = model->faceCount;
	curPt = model->points;
	curFace = model->faces;
	d2 = (map_desc*)malloc(sizeof(map_desc)*nb_face);
	offset = (float*)malloc(sizeof(float)*4*nb_pt);

	coeff = 256.0/size0;
	for (i=0; i<nb_pt; i++) {
		offset[2*i] = 128.0 - coeff*(curPt->y);
		offset[2*i+1] = 128.0 - coeff*(curPt->x);
		curPt++;
	}
	curPt = model->points;
	for (i=nb_pt; i<2*nb_pt; i++) {
		offset[2*i] = 128.0 + coeff*(curPt->y);
		offset[2*i+1] = 128.0 - coeff*(curPt->x);
		curPt++;
	}
	
// create face definition
	d = d2;
	for (i=0; i<nb_face; i+=1) {
		d[0].p1 = curFace[0].points[0]*2; 
		d[0].p2 = curFace[0].points[1]*2; 
		d[0].p3 = curFace[0].points[2]*2; 
		if (i&1) {
			d[0].status = 0;
			d[0].p1 += nb_pt*2;
			d[0].p2 += nb_pt*2;
			d[0].p3 += nb_pt*2;
		}
		else
			d[0].status = B_INVERT_NORM;
		d[0].bitmap = list_map+(i&1);
		d += 1;
		curFace += 1;
	}

	SetMapLook("Map look", B_CORNER_COLOR, offset, d2, 0);
}

















