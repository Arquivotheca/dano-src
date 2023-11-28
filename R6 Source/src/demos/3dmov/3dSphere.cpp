/* ++++++++++

   FILE:  3dSphere.cpp
   REVS:  $Revision: 1.4 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_SPHERE_H
#include "3dSphere.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif
#ifndef _3D_SPHERE_MODEL_H 
#include "3dSphereModel.h"
#endif

static ulong default_bitmap = 0xa0a0a000L;
static map_ref default_map = { (void*)&default_bitmap, 0, 0 };

typedef struct {
	long       nb;
	long       iLast;
	long       iFirst;
	float      pas;
	float      angle;
} ConstCircle;

B3dSphere::B3dSphere(char *name, B3dVector *size, ulong NbFace, ulong Flags) :
B3dFace(name) {
	B3dSphereModel *myModel;
	B3dLook        *myLook;   

	mySize = *size;
	myFace = NbFace;
	myModel = new B3dSphereModel(size, NbFace, Flags);
	myLook = new B3dLook("Default Look", myModel);
	flags = B_BASIC_BODY;
	modelCount = 1;
	threshold = -1.0;
	desc.look = myLook;
	SizeTo(1.0, 1.0, 1.0);
}

B3dSphere::~B3dSphere() {
}

void B3dSphere::SetMapping(map_ref *map) {
	long             i, j, n, iPt, NbPt, NbFace;
	float            f, coeff;
	float            *offset;
	map_desc         *desc, *curdesc;
	ConstCircle      *c;

// chose n parameter.
	n = (long)(b_sqrt((3.14/8.0)*(float)myFace)+1.0);
	if (n < 2) n = 2;
	coeff = 3.141592654/(float)n;
// calculate count of segments par circle, count of points and count of faces
	c = (ConstCircle*)malloc(sizeof(ConstCircle)*(n-1));
	for (i=1;i<n;i++)
		c[i].nb = (long)((float)(2*n)*b_sin(coeff*(float)i)+0.5);
	NbPt = 0;
	for (i=2;i<n-1;i++)
		NbPt += c[i].nb;
	NbFace = 2*(NbPt+c[1].nb+c[n-1].nb);
	NbPt += 2;
// allocate buffers
	desc = (map_desc*)malloc(sizeof(map_desc)*NbFace);
	offset = (float*)malloc(sizeof(float)*2*(NbPt+n-3));
	offset[0] = 0.4;
	offset[1] = 0.4;
	offset[2] = 0.6;
	offset[3] = 0.6;
// prepare pts construction, create first Pt.
	coeff = 256.0/(float)(n-2);
	iPt = 2;
	for (i=1; i<n; i++) {
		c[i].pas = (2.0*3.141592654)/(float)c[i].nb;
		c[i].angle = c[i].pas*0.32;
		c[i].iLast = iPt;
		offset[iPt*2+0] = 768.0 - c[i].angle * (256.0/3.141592654);
		offset[iPt*2+1] = coeff*(float)(n-1-i);
		c[i].angle += c[i].pas;
		if ((i > 1) && (i < (n-1)))
			iPt++;
	}
// create face and mapping coordinates
	curdesc = desc;
	while (iPt < NbPt) {
		j = 1;
		f = c[1].angle;
		for (i=2; i<n; i++)
			if (c[i].angle < f) {
				f = c[i].angle;
				j = i;
			}
		if ((j <= 2) || (j == (n-1))) {
			curdesc->p1 = 0;
			curdesc->p2 = 1;
			curdesc->p3 = 2;
			curdesc->bitmap = &default_map;
		}
		else {
			curdesc->p1 = 2*c[j-1].iLast;
			curdesc->p2 = 2*iPt;
			curdesc->p3 = 2*c[j].iLast;
			curdesc->bitmap = map;
		}
		curdesc->status = 0;
		curdesc++;
		if ((j >= (n-2)) || (j == 1)) {
			curdesc->p1 = 0;
			curdesc->p2 = 1;
			curdesc->p3 = 2;
			curdesc->bitmap = &default_map;
		}
		else {
			curdesc->p1 = 2*c[j+1].iLast;
			curdesc->p2 = 2*c[j].iLast;
			curdesc->p3 = 2*iPt;
			curdesc->bitmap = map;
		}
		curdesc->status = 0;
		curdesc++;
		if ((j > 1) && (j < (n-1))) {
			c[j].iLast = iPt;
			offset[iPt*2+0] = 768.0 - c[j].angle * (256.0/3.141592654);
			offset[iPt*2+1] = coeff*(float)(n-1-j);
			iPt++;
		}
		c[j].angle += c[j].pas;
	}
// create the last n+1 points.
	for (i=2; i<n-1; i++) {
		c[i].iFirst = iPt;
		offset[iPt*2+0] = 768.0 - c[i].angle * (256.0/3.141592654);
		offset[iPt*2+1] = coeff*(float)(n-1-i);
		iPt++;		
	}
// close face definition
	for (i=0; i<3; i++) {
		curdesc->p1 = 0;
		curdesc->p2 = 1;
		curdesc->p3 = 2;
		curdesc->bitmap = &default_map;
		curdesc->status = 0;
		curdesc++;
	}
	for (i=2;i<n-2;i++) {
		if (c[i].angle < c[i+1].angle) {
	  		curdesc->p1 = 2*c[i].iLast;
			curdesc->p2 = 2*c[i+1].iFirst;
			curdesc->p3 = 2*c[i+1].iLast;
			curdesc->status = 0;
			curdesc->bitmap = map;
			curdesc++;
			curdesc->p1 = 2*c[i].iLast;
			curdesc->p2 = 2*c[i].iFirst;
			curdesc->p3 = 2*c[i+1].iFirst;
			curdesc->status = 0;
			curdesc->bitmap = map;
			curdesc++;
		}
		else {
	  		curdesc->p1 = 2*c[i].iLast;
			curdesc->p2 = 2*c[i].iFirst;
			curdesc->p3 = 2*c[i+1].iLast;
			curdesc->status = 0;
			curdesc->bitmap = map;
			curdesc++;
			curdesc->p1 = 2*c[i].iFirst;
			curdesc->p2 = 2*c[i+1].iFirst;
			curdesc->p3 = 2*c[i+1].iLast;
			curdesc->status = 0;
			curdesc->bitmap = map;
			curdesc++;
		}
	}
	for (i=0; i<3; i++) {
		curdesc->p1 = 0;
		curdesc->p2 = 1;
		curdesc->p3 = 2;
		curdesc->bitmap = &default_map;
		curdesc->status = 0;
		curdesc++;
	}
// set the look.
	SetMapLook("Map look", B_CORNER_COLOR, offset, desc, 0);
}













