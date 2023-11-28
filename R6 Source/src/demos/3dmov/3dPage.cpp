/* ++++++++++

   FILE:  3dPage.cpp
   REVS:  $Revision: 1.3 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_PAGE_H
#include "3dPage.h"
#endif
#ifndef _3D_LOOK_H
#include "3dLook.h"
#endif
#ifndef _3D_PAGE_MODEL_H 
#include "3dPageModel.h"
#endif

#include <Debug.h>

static ulong default_bitmap = 0xffffff00L;
static map_ref default_map = { (void*)&default_bitmap, 0, 0 };

B3dPage::B3dPage(char *name, float size, long step, ulong Flags) : B3dFace(name) {
	B3dPageModel   *myModel;
	B3dLook        *myLook;   

	myModel = new B3dPageModel(size, step, Flags);
	myLook = new B3dLook("Default Look", myModel);
	flags = B_BASIC_BODY;
	modelCount = 1;
	threshold = -1.0;
	desc.look = myLook;
	SizeTo(1.0, 1.0, 1.0);
}

B3dPage::~B3dPage() {
}

void B3dPage::SetMapping(map_ref *list_map, map_ref *list_number) {
	int          i, j, imin, imax, index, step;
	ulong        flags;
	float        coeff;
	float        *offset;
	map_desc     *d2, *d;
	B3dPageModel *model;

	model = (B3dPageModel*)(desc.look->model);
	step = model->step;
	flags = model->flags;
	d2 = (map_desc*)malloc(sizeof(map_desc)*model->faceCount);
	offset = (float*)malloc(sizeof(float)*(4*(step-1)*(step-1)+9));
	
	offset[0] = 0.4;
	offset[1] = 0.4;
	offset[2] = 0.6;
	offset[3] = 0.6;
	offset[4] = 0.0;
	offset[5] = 16.0;
	offset[6] = 16.0;
	offset[7] = 0.0;
	offset[8] = 0.0;

	index = 9;
	coeff = 256.0/(float)(step-2);
	for (i=step-2; i>=0; i--)
		for (j=0; j<step-1; j++) {
			offset[index++] = coeff*(float)i; 
			offset[index++] = coeff*(float)j;
		}
	for (i=0; i<step-1; i++)
		for (j=0; j<step-1; j++) {
			offset[index++] = coeff*(float)i; 
			offset[index++] = coeff*(float)j;
		}
	index = 9+2*(step-1)*(step-1);

	imin = step/2-1;
	imax = step/2+1;
	d = d2;
	for (i=0; i<step; i++)
		for (j=0; j<step; j++) {
			if ((i == 0) || (i == step-1) || (j == 0) || (j == step-1)) {
				if ((j == step-1) && (i >= imin) && (i <= imax)) {
					if (flags & B_FRONT_FACES) {
						d->p1 = 5;
						d->p2 = 4;
						d->p3 = 6;
						d->status = 0;
						d->bitmap = list_number+i-imin;
						d++;
						d->p1 = 4;
						d->p2 = 7;
						d->p3 = 6;
						d->status = 0;
						d->bitmap = list_number+i-imin;
						d++;
					}
					if (flags & B_REAR_FACES) {
						d->p1 = 4;
						d->p2 = 7;
						d->p3 = 5;
						d->status = B_INVERT_NORM;
						d->bitmap = list_number+i-imin+2;
						d++;
						d->p1 = 5;
						d->p2 = 7;
						d->p3 = 6;
						d->status = B_INVERT_NORM;
						d->bitmap = list_number+i-imin+2;
						d++;
					}
				}
				else {
					if (flags & B_FRONT_FACES) {
						d->p1 = 0;
						d->p2 = 1;
						d->p3 = 2;
						d->status = 0;
						d->bitmap = &default_map;
						d++;
						d->p1 = 0;
						d->p2 = 1;
						d->p3 = 2;
						d->status = 0;
						d->bitmap = &default_map;
						d++;
					}
					if (flags & B_REAR_FACES) {
						d->p1 = 0;
						d->p2 = 2;
						d->p3 = 1;
						d->status = B_INVERT_NORM;
						d->bitmap = &default_map;
						d++;
						d->p1 = 0;
						d->p2 = 2;
						d->p3 = 1;
						d->status = B_INVERT_NORM;
						d->bitmap = &default_map;
						d++;
					}
				}
			}
			else {
				if (flags & B_FRONT_FACES) {
					d->p1 = 9+2*((i-1)*(step-1)+(j+0));
					d->p2 = 9+2*((i+0)*(step-1)+(j+0));
					d->p3 = 9+2*((i-1)*(step-1)+(j-1));
					d->status = 0;
					d->bitmap = list_map;
					d++;
					d->p1 = 9+2*((i+0)*(step-1)+(j+0));
					d->p2 = 9+2*((i+0)*(step-1)+(j-1));
					d->p3 = 9+2*((i-1)*(step-1)+(j-1));
					d->status = 0;
					d->bitmap = list_map;
					d++;
				}
				if (flags & B_REAR_FACES) {
					d->p1 = index+2*((i-1)*(step-1)+(j+0));
					d->p2 = index+2*((i-1)*(step-1)+(j-1));
					d->p3 = index+2*((i+0)*(step-1)+(j+0));
					d->status = B_INVERT_NORM;
					d->bitmap = list_map+1;
					d++;
					d->p1 = index+2*((i+0)*(step-1)+(j+0));
					d->p2 = index+2*((i-1)*(step-1)+(j-1));
					d->p3 = index+2*((i+0)*(step-1)+(j-1));
					d->status = B_INVERT_NORM;
					d->bitmap = list_map+1;
					d++;
				}
			}
		}

//	_sPrintf("Set corner color\n");
	SetMapLook("Map look", B_CORNER_COLOR, offset, d2, 0);
}

















