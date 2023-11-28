/* ++++++++++

   FILE:  3dAmbientLight.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_AMBIENT_LIGHT_H
#include "3dAmbientLight.h"
#endif

B3dAmbientLight::B3dAmbientLight(char *name, float power, RGBAColor *color) :
B3dLight(name, power, color) {
	type = B_AMBIENT_LIGHT;
}

B3dAmbientLight::~B3dAmbientLight() {
}

