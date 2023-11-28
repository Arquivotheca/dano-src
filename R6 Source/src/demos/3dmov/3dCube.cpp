/* ++++++++++

   FILE:  3dCube.cpp
   REVS:  $Revision: 1.3 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_CUBE_H
#include "3dCube.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif
#ifndef _3D_CUBE_MODEL_H 
#include "3dCubeModel.h"
#endif

#include <Debug.h>

B3dCube::B3dCube(char *name, B3dVector *size, ulong Flags) : B3dFace(name) {
	B3dCubeModel   *myModel;
	B3dLook        *myLook;   

	myModel = new B3dCubeModel(size, Flags);
	myLook = new B3dLook("Default Look", myModel);
	flags = B_BASIC_BODY;
	modelCount = 1;
	threshold = -1.0;
	desc.look = myLook;
	SizeTo(1.0, 1.0, 1.0);
}

B3dCube::~B3dCube() {
}

void B3dCube::SetMapping(map_ref *list_map) {
	int        i;
	map_desc   *desc;
	float      *offset;
	
	desc = (map_desc*)malloc(sizeof(map_desc)*12);
	offset = (float*)malloc(sizeof(float)*2*4);
	
	offset[0] = 0.0;
	offset[1] = 0.0;
	offset[2] = 256.0;
	offset[3] = 0.0;
	offset[4] = 256.0;
	offset[5] = 256.0;
	offset[6] = 0.0;
	offset[7] = 256.0;

	for (i=0; i< 12; i+=2) {
		desc[i+0].p1 = 0;
		desc[i+0].p2 = 2;
		desc[i+0].p3 = 6;
		desc[i+0].status = 0;
		desc[i+0].bitmap = list_map+(i/2);
//		_sPrintf("Face : %x [%x]\n", desc[i+0].bitmap->buf, desc[i+0].bitmap);
		desc[i+1].p1 = 2;
		desc[i+1].p2 = 4;
		desc[i+1].p3 = 6;
		desc[i+1].status = 0;
		desc[i+1].bitmap = list_map+(i/2);
//		_sPrintf("Face : %x [%x]\n", desc[i+1].bitmap->buf, desc[i+1].bitmap);
	}

	SetMapLook("Map look", B_FACE_COLOR, offset, desc, 0);
}











