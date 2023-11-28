/* ++++++++++

   FILE:  3dSpin.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_SPIN_H
#include "3dSpin.h"
#endif
#ifndef _3D_SPIN_MODEL_H 
#include "3dSpinModel.h"
#endif

B3dSpin::B3dSpin(char *name,
				 B2dVector *shape,
				 long shape_count,
				 long radius_count ,
				 ulong Flags) :
B3dFace(name) {
	B3dSpinModel   *myModel;
	B3dLook        *myLook;   

	myModel = new B3dSpinModel(shape, shape_count, radius_count, Flags);
	myLook = new B3dLook("Default Look", myModel);
	flags = B_BASIC_BODY;
	modelCount = 1;
	threshold = -1.0;
	desc.look = myLook;
	SizeTo(1.0, 1.0, 1.0);
}

B3dSpin::~B3dSpin() {
}









