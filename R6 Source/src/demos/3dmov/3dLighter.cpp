/* ++++++++++

   FILE:  3dLighter.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_LIGHTER_H
#include "3dLighter.h"
#endif
#ifndef _3D_AMBIENT_LIGHT_H
#include "3dAmbientLight.h"
#endif
#ifndef _3D_PARALLEL_LIGHT_H
#include "3dParallelLight.h"
#endif
#ifndef _3D_WORLD_H
#include "3dWorld.h"
#endif
//#ifndef _DITHER_SERVER_H
//#include "DitherServer.h"
//#endif

#include <stdio.h>

long lighter_thread(void *data);

B3dLighter::B3dLighter() {
	LP.ambientColor.red = 0.0;
	LP.ambientColor.green = 0.0;
	LP.ambientColor.blue = 0.0;
	LP.parallelLightCount = 0;
	LP.radialLightCount = 0;
}

B3dLighter::~B3dLighter() {
}

void B3dLighter::CollectLight(B3dWorld *world, B3dMatrix *Rotation, B3dVector *Origin) {
	long             i, nb;
	float            power;
	B3dLight         *light;
	B3dLightProcDesc *desc;
	
	nb = world->LightCount();
	LP.ambientColor.red = 0.0;
	LP.ambientColor.green = 0.0;
	LP.ambientColor.blue = 0.0;
	LP.parallelLightCount = 0;
	LP.radialLightCount = 0;
	for (i=0;i<nb;i++) {
		light = world->GetNthLight(i);
		switch (light->type) {
		case B_AMBIENT_LIGHT:
			power = light->power;
			LP.ambientColor.red = light->highlightColor.red*power;
			LP.ambientColor.green = light->highlightColor.green*power;
			LP.ambientColor.blue = light->highlightColor.blue*power;
			break;
		case B_PARALLEL_LIGHT:
			if (LP.parallelLightCount < B_MAX_PARALLEL_LIGHT_COUNT) {
				desc = LP.parallelProcs+LP.parallelLightCount;
				LP.parallelLightCount++;
				power = light->power;
				desc->color.red = light->highlightColor.red*power;
				desc->color.green = light->highlightColor.green*power;
				desc->color.blue = light->highlightColor.blue*power;
				desc->vector = (*Rotation)*(light->rotation.X());
				desc->vector.Norm();
			}
			break;
		}
	}
}

B3dLightProc *B3dLighter::SelectLight(B3dThing *object) {
	LP.SetHighlightColor(&LP_copy, &object->highlightColor);
	return &LP_copy;
}

void B3dLightProc_1::SetHighlightColor(B3dLightProc_1 *dup, RGBAColor *color) {
	long       i, nb;
	
	dup->ambientColor = ambientColor^(*color);
	nb = parallelLightCount;
	dup->parallelLightCount = nb;
	for (i=0;i<nb;i++)
		parallelProcs[i].AdjustColor(dup->parallelProcs+i, color);
	nb = radialLightCount;
	dup->radialLightCount = nb;
	for (i=0;i<nb;i++)
		radialProcs[i].AdjustColor(dup->radialProcs+i, color);
}

void B3dLightProcDesc::AdjustColor(B3dLightProcDesc *dup, RGBAColor *Color) {
	dup->color = color^(*Color);
	dup->vector = vector;
}

ulong B3dLightProc_1::Calc32(B3dVector *point, B3dVector *norm, float sign) {
	long             i, nb;
	float            r, g, b;
	float            coeff;
	ulong            Rcolor;
	B3dLightProcDesc *desc;
	
	r = ambientColor.red;
	g = ambientColor.green;
	b = ambientColor.blue;
	nb = parallelLightCount;
	if (nb > 0) {
		desc = parallelProcs;
		do {
			coeff = (*norm)*desc->vector;
			coeff *= sign;
			if (coeff < 0.0) {
				r -= desc->color.red*coeff;
				g -= desc->color.green*coeff;
				b -= desc->color.blue*coeff;
			}
			desc++;
		} while (--nb > 0);
	}
	if (r > 1.0)
		Rcolor = 0xff000000;
	else
		Rcolor = (ulong)(r*255.0)<<24;
	if (g > 1.0)
		Rcolor |= 0x00ff0000;
	else
		Rcolor |= (ulong)(g*255.0)<<16;
	if (b > 1.0)
		Rcolor |= 0x0000ff00;
	else
		Rcolor |= (ulong)(b*255.0)<<8;
	return Rcolor;
}











