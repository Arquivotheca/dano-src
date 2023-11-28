/* ++++++++++

   FILE:  3dCubeModel.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_CUBE_MODEL_H
#include "3dCubeModel.h"
#endif

#include <stdlib.h>

static B3dFaceDesc CubeFaces[12] = {
	{ 0, 4, 1, 0 },
	{ 4, 5, 1, 0 },
	{ 3, 7, 0, 1 },
	{ 7, 4, 0, 1 },
	{ 2, 6, 3, 2 },
	{ 6, 7, 3, 2 },
	{ 1, 5, 2, 3 },
	{ 5, 6, 2, 3 },
	{ 7, 6, 4, 4 },
	{ 6, 5, 4, 4 },
	{ 2, 3, 1, 5 },
	{ 3, 0, 1, 5 },
};

static B3dFaceDesc PlateFaces[4] = {
	{ 0, 1, 2, 0 },
	{ 2, 1, 3, 0 },
	{ 0, 2, 1, 1 },
	{ 1, 2, 3, 1 },
};

B3dCubeModel::B3dCubeModel(B3dVector *size, ulong flags) :
B3dConvexModel(BuildModel(size), flags, B_OWN_POINT_LIST|B_OWN_FACE_DESC) {
}

B3dCubeModel::~B3dCubeModel() {
}

B3dFaceModelDesc *B3dCubeModel::BuildModel(B3dVector *size) {
	long             cnt0;
	float            dx, dy, dz;
	B3dVector        *listPt;
	B3dFaceModelDesc *desc;

	cnt0 = 0;
	if (size->x > 0) cnt0++;
	if (size->y > 0) cnt0++;
	if (size->z > 0) cnt0++;
	if (cnt0 == 3) {
		listPt = (B3dVector*)malloc(8*sizeof(B3dVector));
		desc = (B3dFaceModelDesc*)malloc(sizeof(B3dFaceModelDesc));
		desc->pointCount = 8;
		desc->faceCount = 12;
		desc->faces = CubeFaces;
		desc->points = listPt;
		dx = size->x*0.5;
		dy = size->y*0.5;
		dz = size->z*0.5;
		listPt[0].x = -dx;
		listPt[0].y = -dy;
		listPt[0].z = -dz;
		listPt[1].x = +dx;
		listPt[1].y = -dy;
		listPt[1].z = -dz;
		listPt[2].x = +dx;
		listPt[2].y = +dy;
		listPt[2].z = -dz;
		listPt[3].x = -dx;
		listPt[3].y = +dy;
		listPt[3].z = -dz;
		listPt[4].x = -dx;
		listPt[4].y = -dy;
		listPt[4].z = +dz;
		listPt[5].x = +dx;
		listPt[5].y = -dy;
		listPt[5].z = +dz;
		listPt[6].x = +dx;
		listPt[6].y = +dy;
		listPt[6].z = +dz;
		listPt[7].x = -dx;
		listPt[7].y = +dy;
		listPt[7].z = +dz;
	}
	else if (cnt0 == 2) {
		listPt = (B3dVector*)malloc(4*sizeof(B3dVector));
		desc = (B3dFaceModelDesc*)malloc(sizeof(B3dFaceModelDesc));
		desc->pointCount = 4;
		desc->faceCount = 4;
		desc->faces = PlateFaces;
		desc->points = listPt;
		dx = size->x*0.5;
		dy = size->y*0.5;
		dz = size->z*0.5;
		listPt[0].x = -dx;
		listPt[0].y = -dy;
		listPt[0].z = -dz;
		listPt[1].x = +dx;
		listPt[1].y = -dy;
		listPt[1].z = -dz;
		listPt[2].x = +dx;
		listPt[2].y = +dy;
		listPt[2].z = -dz;
		listPt[3].x = -dx;
		listPt[3].y = +dy;
		listPt[3].z = -dz;
	}
	return desc;
}










