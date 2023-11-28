/* ++++++++++

   FILE:  3dSpinModel.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_SPIN_MODEL_H
#include "3dSpinModel.h"
#endif
#ifndef _3D_LENS_H
#include "3dLens.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <stdlib.h>

B3dSpinModel::B3dSpinModel(B2dVector *shape, long shape_count, long radius_count, ulong flags):
B3dFaceModel(BuildModel(shape, shape_count, radius_count), flags,
			 B_OWN_POINT_LIST|B_OWN_FACE_LIST|B_OWN_FACE_DESC) {
	long      i;
	float     size;
	
	divider = (float*)malloc(sizeof(float)*shape_count-2.0);
	size = shape[shape_count-1].y - shape[0].y;
	for (i=0;i<shape_count-2;i++)
		divider[i] = (shape[i+1].y-shape[0].y)*size;
	radiusCount = radius_count;
	bandCount = shape_count-1;
}

B3dSpinModel::~B3dSpinModel() {
	free(divider);
}

void B3dSpinModel::CalcSort(B3dLensImage *lensImage, void **SortDesc) {
	long        i, imax;
	float       scal;
	ushort      *cur;
	B3dVector   axis;

	if (sortedList == 0L)
		sortedList = (ushort*)malloc(sizeof(ushort)*faceCount);
	cur = sortedList-1;
	axis = *((B3dVector*)(lensImage->lensPointAt(0))) -
		   *((B3dVector*)(lensImage->lensPointAt(1)));
	scal = *((B3dVector*)(lensImage->lensPointAt(0)))*axis;
	if (scal <= divider[0])
		imax = 0;
	else {
		for (i=bandCount-2;i>0;i--)
			if (scal > divider[i]) break;
		imax = (2*i+1)*radiusCount;
		for (i=0;i<imax;i++)
			*++cur = i;
	}
	for (i=(2*bandCount-2)*radiusCount-1;i>=imax;i--)
		*++cur = i;
	*SortDesc = (void*)sortedList;
}

B3dFaceModelDesc *B3dSpinModel::BuildModel(B2dVector *shape,
										   long shape_count,
										   long radius_count) {
	long             i, j, NbPt, NbFace;
	long             pt0, pt1, ptd0, ptd1, i0, i1;
	float            f, cs, sn;
	float            angle, pas;
	B3dVector        *listPt,*curPt;
	B3dFaceDesc      *listFace;
	B3dFaceModelDesc *desc;

// calculate number of points
	NbPt = 2+(shape_count-2)*radius_count;
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
	curPt[0].x = 0.0;
	curPt[0].y = 0.0;
	curPt[0].z = shape[0].y;
	curPt[1].x = 0.0;
	curPt[1].y = 0.0;
	curPt[1].z = shape[shape_count-1].y;
	curPt += 2;
	pas = (2*3.141592654)/(float)radius_count;
// create pts
	for (angle = 0; angle < 6.283; angle += pas) {
		cs = b_cos(angle);
		sn = b_sin(angle);
		for (i=1;i<shape_count-1;i++) {
			curPt->x = shape[i].x*cs;
			curPt->y = shape[i].x*sn;
			curPt->z = shape[i].y;
			curPt++;
		}
	}
// create faces by band
	for (i=0; i<shape_count-1; i++) {
		if (i == 0) {
			pt0 = 0;
			ptd0 = 0;
		} else {
			pt0 = i+1;
			ptd0 = shape_count-2;
		}
		if (i == shape_count-2) {
			pt1 = 1;
			ptd1 = 0;
		} else {
			pt1 = i+2;
			ptd1 = shape_count-2;
		}
		i0 = pt0;
		i1 = pt1;
		for (j=0;j<radius_count-1;j++) {
			if (ptd0 != 0) {
				listFace->points[0] = i1;
				listFace->points[1] = i0+ptd0;
				listFace->points[2] = i0;
				listFace->norm = -1;
				listFace++;
			}
			if (ptd1 != 0) {
				listFace->points[0] = i1;
				listFace->points[1] = i1+ptd1;
				listFace->points[2] = i0+ptd0;
				listFace->norm = -1;
				listFace++;
			}
			i0 += ptd0;
			i1 += ptd1;
		}
		if (ptd0 != 0) {
			listFace->points[0] = i1;
			listFace->points[1] = pt0;
			listFace->points[2] = i0;
			listFace->norm = -1;
			listFace++;
		}
		if (ptd1 != 0) {
			listFace->points[0] = i1;
			listFace->points[1] = pt1;
			listFace->points[2] = pt0;
			listFace->norm = -1;
			listFace++;
		}
	}
// result
	return desc;
}

















