/* ++++++++++

   FILE:  3dLight.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_LIGHT_H
#include "3dLight.h"
#endif

B3dLight::B3dLight(char *name, float Power, RGBAColor *Color) : B3dThing(name) {
	highlightColor = *Color;
	power = Power;
}

B3dLight::~B3dLight() {
}
