/* ++++++++++

   FILE:  3dLens.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_LENS_H
#include "3dLens.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <Debug.h>

void B3dLens::GetOptions(void *options) {
	return; // so we are not optimized out of existence
}

void B3dLens::SetOptions(void *options) {
	return; // so we are not optimized out of existence
}

void B3dLens::See(B3dModel     *Model,
				  B3dMatrix    *Rotation,
				  B3dVector    *Trans,
				  long         status,
				  B3dLensImage *ProjDesc) {
	return; // so we are not optimized out of existence
}

void B3dLens::CalcAxis(short h, short v, B3dVector *axis) {
	axis->Set(1.0, 0.0, 0.0);
}

long B3dLens::CheckSteric(B3dSteric *Gab, B3dMatrix *Rotation,
						  B3dVector *Trans, float *Radius) {
	return B_CLIPPED_OUT;
}

void B3dLens::ClipMap(B3dLensPoint *p1,
					  B3dLensPoint *p2,
					  B3dLensPoint *p3,
					  map_look     *look1,
					  map_look     *look2,
					  map_look     *look3,
					  B_MAP        draw,
					  B3dLook      *looker,
					  void         *bits,
					  long         row,
					  map_ref      *ref) {
	return; // so we are not optimized out of existence
}











