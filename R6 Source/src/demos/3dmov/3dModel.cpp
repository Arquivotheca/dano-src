/* ++++++++++

   FILE:  3dModel.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_MODEL_H
#include "3dModel.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <stdlib.h>
#include <string.h>

B3dModel::B3dModel(long numPt, B3dVector *listPt, ulong OwnFlag) {
	long       i;
	float      rayon, Rmax;
	B3dVector  max, min;
	
// points and steric def
	pointCount = numPt;
	if (OwnFlag & B_OWN_POINT_LIST)
		points = listPt;
	else {
		points = (B3dVector*)malloc(sizeof(B3dVector)*numPt);
		memcpy(points, listPt, sizeof(B3dVector)*numPt);
	}
	max = min = points[0];
	Rmax = points[0].Square();
	for (i=1;i<numPt;i++) {
		rayon = points[i].Square();
		if (rayon > Rmax) Rmax = rayon;
		if (points[i].x < min.x) min.x = points[i].x;
		else if (points[i].x > max.x) max.x = points[i].x;
		if (points[i].y < min.y) min.y = points[i].y;
		else if (points[i].y > max.y) max.y = points[i].y;
		if (points[i].z < min.z) min.z = points[i].z;
		else if (points[i].z > max.z) max.z = points[i].z;
	}
	steric.radius = b_sqrt(Rmax)*1.001;
	steric.min = min;
	steric.max = max;
// RefCpt init
	referenceCount = 0;
}

B3dModel::~B3dModel() {
	free(points);
}

void B3dModel::CalcSort(B3dLensImage *ProjDesc, void **SortDesc) {
}

void B3dModel::Debug() {
	long   i, nb;
	
	nb = pointCount;
	fprintf(stderr,"NbPt:%d\n",nb);
	for (i=0;i<nb;i++)
		points[i].Debug();
	nb = vectorCount;
	fprintf(stderr,"NbVect:%d\n",nb);
	for (i=0;i<nb;i++)
	    vectors[i].Debug();
	nb = normCount;
	fprintf(stderr,"NbNorm:%d\n",nb);
	for (i=0;i<nb;i++)
		norms[i].Debug();
	fprintf(stderr,"Steric : radius = %f\n",steric.radius);
	steric.min.Debug();
	steric.max.Debug();
}








