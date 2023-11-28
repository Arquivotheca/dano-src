/* ++++++++++

   FILE:  3dFaceModel.cpp
   REVS:  $Revision: 1.3 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_FACE_MODEL_H
#include "3dFaceModel.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <Debug.h>

B3dFaceModel::B3dFaceModel(B3dFaceModelDesc *desc, ulong flags, ulong OwnFlag) :
B3dModel(desc->pointCount, desc->points, OwnFlag) {
	long        i, j, k, memoIndex, Nnorm;
	long        *PtLeader;
	long        *PtToFace,*curPt;
	long        numFace, offset;
	B3dVector   norm, cur;
	B3dVector   *memo[16];
	B3dFaceDesc *listFace, *face;

// read desc definition
	numFace = desc->faceCount;
	listFace = desc->faces;
	extension = desc->extension;
	if (OwnFlag & B_OWN_FACE_DESC)
		free(desc);
// memorize specific flags status.
	status = flags;
// no vect
	vectorCount = 0;
// auto-norm count
	Nnorm = -1;
	if (flags & B_SMOOTH_MODEL)
		normCount = pointCount;
	else
		normCount = 0;
	for (i=0;i<numFace;i++) {
		if (listFace[i].norm > Nnorm)
			Nnorm = listFace[i].norm;
		else if (listFace[i].norm < 0)
			normCount++;
	}
	Nnorm++;
	normCount += Nnorm;
// allocate Vect and Norm buffer
	vectors = (B3dVector*)malloc(sizeof(B3dVector)*(vectorCount+normCount));
	norms = vectors+vectorCount;
// create face norm vector
	face = listFace;
	for (i=0;i<numFace;i++) {
		if (face->norm < 0) {
			j = Nnorm;
			face->norm = j;
			Nnorm++;
		}
		else
			j = face->norm;
		norms[j] = (points[face->points[2]]-points[face->points[0]])^
			       (points[face->points[1]]-points[face->points[0]]);
		norms[j].Norm();
//		_sPrintf("%d # (%f,%f,%f)\n", i, norms[j].x, norms[j].y, norms[j].z);
		face++;
	}
// create lightPt for smooth object
   if (flags & B_SMOOTH_MODEL) {
   // create Pt to face inverted list
		PtToFace = (long*)malloc(sizeof(long)*6*numFace);
		PtLeader = (long*)malloc(sizeof(long)*pointCount);
		for (i=0;i<pointCount;i++)
			PtLeader[i] = -1;
		curPt = PtToFace;
		face = listFace;
		for (i=0;i<numFace*3;i+=3) {
			curPt[0] = face->norm;
			curPt[2] = face->norm;
			curPt[4] = face->norm;
			curPt[1] = PtLeader[face->points[0]];
			curPt[3] = PtLeader[face->points[1]];
			curPt[5] = PtLeader[face->points[2]];
			PtLeader[face->points[0]] = i*2;
			PtLeader[face->points[1]] = i*2+2;
			PtLeader[face->points[2]] = i*2+4;
			curPt += 6;
			face++;
		}
    // process norm for new lighting pt.
		offset = normCount-pointCount;
		for (i=0;i<pointCount;i++) {
//			memoIndex = 0;
			norm.x = norm.y = norm.z = 0.0;
			j = PtLeader[i];
			while (j >= 0) {
				cur = norms[PtToFace[j]];
//				_sPrintf("(%f,%f,%f)\n", cur.x, cur.y, cur.z);
/*				for (k=0;k<memoIndex;k++)
					if ((*(memo[k]))*cur > 0.9999)
						goto skip;
				if (memoIndex < 16)
					memo[memoIndex++] = norms+PtToFace[j];*/
				norm += cur;
			skip:
				j = PtToFace[j+1];
			}
			norm.Norm();
			norms[i+offset] = norm;
//			_sPrintf("%d -> (%f,%f,%f)\n", i, norm.x, norm.y, norm.z);
		}		
		free(PtToFace);
		free(PtLeader);
	}
// build face definition
	faceCount = numFace;
	if (OwnFlag & B_OWN_FACE_LIST)
		faces = listFace;
	else {
		faces = (B3dFaceDesc*)malloc(sizeof(B3dFaceDesc)*numFace);
		memcpy(faces,listFace,sizeof(B3dFaceDesc)*numFace);
	}
// initialise sort_list table ptr
	sortedList = 0L;
}

B3dFaceModel::~B3dFaceModel() {
	if (sortedList != 0L) free(sortedList);
	free(vectors);
	free(faces);
}

void B3dFaceModel::CalcSort(B3dLensImage *ProjDesc, void **SortDesc) {
}

void B3dFaceModel::Debug() {
	long   i;

	return;
	B3dModel::Debug();
	fprintf(stderr,"NbFace : %d\n",faceCount);
	for (i=0;i<faceCount;i++)
		fprintf(stderr,"%d : [%d,%d,%d] -> %d\n",i,
				faces[i].points[0], faces[i].points[1],
				faces[i].points[2], faces[i].norm);
}
















