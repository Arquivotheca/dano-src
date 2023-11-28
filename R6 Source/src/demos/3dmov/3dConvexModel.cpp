/* ++++++++++

   FILE:  3dConvexModels.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_CONVEX_MODEL_H
#include "3dConvexModel.h"
#endif

B3dConvexModel::B3dConvexModel(B3dFaceModelDesc *desc, ulong flags, ulong OwnFlag) :
B3dFaceModel(desc, flags, OwnFlag) {
}

void B3dConvexModel::CalcSort(B3dLensImage *lensImage, void **sortDesc) {
	long        i;

	if (sortedList == 0L) {
		sortedList = (ushort*)malloc(sizeof(ushort)*faceCount);
		for (i=0;i<faceCount;i++)
			sortedList[i] = i;
	}
	*sortDesc = (void*)sortedList;
}























