/* ++++++++++

   FILE:  3dParallelLight.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_PARALLEL_LIGHT_H
#include "3dParallelLight.h"
#endif

B3dParallelLight::B3dParallelLight(char *name, float power,
								   RGBAColor *color, B3dVector *Direction) :
B3dLight(name, power, color) {
	rotation.X() = *Direction;
	if ((rotation.X().x > 0.5) || (rotation.X().x < -0.5))
		rotation.Y() = rotation.X().VectorielY();
	else
		rotation.Y() = rotation.X().VectorielX();
	rotation.Norm();
	type = B_PARALLEL_LIGHT;
}

B3dParallelLight::~B3dParallelLight() {
}








