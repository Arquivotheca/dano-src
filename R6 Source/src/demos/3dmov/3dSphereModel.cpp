/* ++++++++++

   FILE:  3dSphereModel.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_SPHERE_MODEL_H
#include "3dSphereModel.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <stdlib.h>

typedef struct {
	long       nb;
	long       iFirst;
	long       iLast;
	float      pas;
	float      angle;
	float      rayon;
	float      z;
} ConstCircle;

B3dSphereModel::B3dSphereModel(B3dVector *size, ulong NbFace, ulong flags) :
B3dConvexModel(BuildModel(size, NbFace), flags,
			   B_OWN_POINT_LIST|B_OWN_FACE_LIST|B_OWN_FACE_DESC) {
}

B3dSphereModel::~B3dSphereModel() {
}

B3dFaceModelDesc *B3dSphereModel::BuildModel(B3dVector *size, ulong NbFace) {
	long             i, j, iPt, n, NbPt;
	float            f, coeff;
	float            dx, dy, dz;
	float            sx, sy, sz;
	B3dVector        *listPt,*curPt;
	ConstCircle      *c;
	B3dFaceDesc      *listFace;
	B3dFaceModelDesc *desc;

// chose n parameter.
	n = (long)(b_sqrt((3.14/8.0)*(float)NbFace)+1.0);
	if (n < 2) n = 2;
	coeff = 3.141592654/(float)n;
// calculate number of segments par circle
	c = (ConstCircle*)malloc(sizeof(ConstCircle)*(n+1));
	c[0].nb = c[n].nb = 0;
	for (i=1;i<n;i++)
		c[i].nb = (long)((float)(2*n)*b_sin(coeff*(float)i)+0.5);
// calculate number of points
	NbPt = 2;
	for (i=0;i<=n;i++)
		NbPt += c[i].nb;
// calculate number of faces
	NbFace = 2*NbPt-4;
// allocate buffers
	listPt = (B3dVector*)malloc(NbPt*sizeof(B3dVector));
	listFace = (B3dFaceDesc*)malloc(NbFace*sizeof(B3dFaceDesc));
	desc = (B3dFaceModelDesc*)malloc(sizeof(B3dFaceModelDesc));
	desc->pointCount = NbPt;
	desc->faceCount = NbFace;
	desc->faces = listFace;
	desc->points = listPt;
// prepare pts construction, create first Pt.
	curPt = listPt;
	curPt[0].z = 0.5;
	curPt[0].x = curPt[0].y = 0.0;
	c[0].iFirst = c[0].iLast = 0;
	c[0].angle = 0.0;
	curPt[1].z = -0.5;
	curPt[1].x = curPt[1].y = 0.0;
	c[n].iFirst = c[n].iLast = 1;
	c[n].angle = 0.0;
	curPt += 2;
	iPt = 2;
	for (i=1;i<n;i++) {
		c[i].z = 0.5*b_cos(coeff*(float)i);
		c[i].rayon = 0.5*b_sin(coeff*(float)i);
		c[i].pas = (2.0*3.141592654)/(float)c[i].nb;
		c[i].angle = c[i].pas*0.32;
		c[i].iFirst = c[i].iLast = iPt;
		curPt->x = c[i].rayon*b_cos(c[i].angle);
		curPt->y = c[i].rayon*b_sin(c[i].angle);
		curPt->z = c[i].z;
		c[i].angle += c[i].pas;
		curPt++;
		iPt++;
	}
// create pts and faces
	while (iPt < NbPt) {
		j = 1;
		f = c[1].angle;
		for (i=2;i<n;i++)
			if (c[i].angle < f) {
				f = c[i].angle;
				j = i;
			}
		listFace->points[0] = c[j-1].iLast;
		listFace->points[1] = iPt;
		listFace->points[2] = c[j].iLast;
		listFace->norm = -1;
		listFace++;
		listFace->points[0] = c[j+1].iLast;
		listFace->points[1] = c[j].iLast;
		listFace->points[2] = iPt;
		listFace->norm = -1;
		listFace++;
		c[j].iLast = iPt;
		curPt->x = c[j].rayon*b_cos(c[j].angle);
		curPt->y = c[j].rayon*b_sin(c[j].angle);
		curPt->z = c[j].z;		
		c[j].angle += c[j].pas;
		curPt++;
		iPt++;
	}
// close face definition
	for (i=0;i<n;i++) {
		if (c[i].angle < c[i+1].angle) {
	  		listFace->points[0] = c[i].iLast;
			listFace->points[1] = c[i+1].iFirst;
			listFace->points[2] = c[i+1].iLast;
			listFace->norm = -1;
			listFace++;
			if (c[i].iLast != c[i].iFirst) {
				listFace->points[0] = c[i].iLast;
				listFace->points[1] = c[i].iFirst;
				listFace->points[2] = c[i+1].iFirst;
				listFace->norm = -1;
				listFace++;
			}
		}
		else {
	  		listFace->points[0] = c[i].iLast;
			listFace->points[1] = c[i].iFirst;
			listFace->points[2] = c[i+1].iLast;
			listFace->norm = -1;
			listFace++;
			if (c[i+1].iLast != c[i+1].iFirst) {
				listFace->points[0] = c[i].iFirst;
				listFace->points[1] = c[i+1].iFirst;
				listFace->points[2] = c[i+1].iLast;
				listFace->norm = -1;
				listFace++;
			}
		}
	}
// resize pt list
	sx = size->x;
	sy = size->y;
	sz = size->z;
	curPt = listPt;
	for (i=0;i<NbPt;i++) {
		curPt->x *= sx;
		curPt->y *= sy;
		curPt->z *= sz;
		curPt++;
	}
// free temporary buffer
	free(c);
// result
	return desc;
}



















