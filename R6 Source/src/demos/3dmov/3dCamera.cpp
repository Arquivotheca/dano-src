/* ++++++++++

   FILE:  3dCamera.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_CAMERA_H
#include "3dCamera.h"
#endif
#ifndef _3D_RADIAL_LENS_H
#include "3dRadialLens.h"
#endif

B3dCamera::B3dCamera(char *name, B3dWorld *theWorld,
					 ushort ProjectorMode, ushort LighterMode) :
B3dThing(name) {
	world = theWorld;
	myProjector = 0L;
	SetProjectorMode(ProjectorMode);
	myLighter = 0L;
	SetLighterMode(LighterMode);
}

B3dCamera::~B3dCamera() {
	if (myProjector != 0L)
		delete myProjector;
	if (myLighter != 0L)
		delete myLighter;
}

long B3dCamera::SetProjectorMode(ushort ProjectorMode) {
	B3dLens   *newProj;
	
	switch (ProjectorMode) {
	case B_RADIAL_LENS:
		newProj = new B3dRadialLens();
		break;
	default:
		return B_ERROR;
	}
	if (myProjector == 0L)
		delete myProjector;
	myProjector = newProj;
	ProjMode = ProjectorMode;
	return B_NO_ERROR;
}


long B3dCamera::SetLighterMode(ushort LighterMode) {
	B3dLighter   *newLight;
	
	switch (LighterMode) {
	case B_STANDARD_LIGHT:
		newLight = new B3dLighter();
		break;
	default:
		return B_ERROR;
	}
	if (myLighter == 0L)
		delete myLighter;
	myLighter = newLight;
	LightMode = LighterMode;
	return B_NO_ERROR;
}

ushort B3dCamera::ProjectorMode() {
	return ProjMode;
}

ushort B3dCamera::LighterMode() {
	return LightMode;
}

bool B3dCamera::GetTouchDesc(short h, short v, B3dTouchDesc *touch) {
	B3dVector    rel, axis;

	myProjector->CalcAxis(h, v, &rel);
	axis = rotation*rel;
	return world->GetTouch(&origin, &axis, touch);
}

void B3dCamera::CalcTouchDesc(short h, short v,
							  B3dVector *origin0, B3dVector *axis0,
							  B3dTouchDesc *touch, B3dMatrix *mat) {
	B3dVector    rel, axis;

	myProjector->CalcAxis(h, v, &rel);
	axis = rotation*rel;
	world->CalcTouch(&origin, &axis, origin0, axis0, touch, mat);
}

void B3dCamera::SetFrame(long Hmin, long Hmax, long Vmin, long Vmax) {
	switch (ProjMode) {
	case B_RADIAL_LENS :
		{
			B3dRadialOptions PPOptions;

		    myProjector->GetOptions(&PPOptions);
			PPOptions.hMin = 0.2+(float)Hmin;
			PPOptions.hMax = 1.8+(float)Hmax;
			PPOptions.vMin = 0.2+(float)Vmin;
			PPOptions.vMax = 1.8+(float)Vmax;
			myProjector->SetOptions(&PPOptions);
		}
		break;
	}
}










