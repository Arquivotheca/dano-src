/* ++++++++++

   FILE:  3dPageModel.cpp
   REVS:  $Revision: 1.3 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_PAGE_MODEL_H
#include "3dPageModel.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <stdlib.h>
#include <Debug.h>

static float angles_max[24] = {
	0.8, 1.12, 1.34, 1.5,
	1.6, 1.67, 1.72, 1.76,
	1.8, 1.8, 1.8, 1.8,
	1.8, 1.8, 1.8, 1.8,
	1.8, 1.8, 1.8, 1.8,
	1.8, 1.8, 1.8, 1.8
};

B3dPageModel::B3dPageModel(float size0, long step0, ulong flags0) :
B3dConvexModel(BuildModel(size0, step0, flags0), flags0, B_OWN_POINT_LIST|B_OWN_FACE_DESC) {
	int     i;

	if (step0 > 24) step0 = 24;
	step0 &= 0x1e;
	step = step0;
	flags = flags0;
	size = size0;
	SetShape(1.0, 0.0, 0.0);
}

B3dPageModel::~B3dPageModel() {
}

B3dFaceModelDesc *B3dPageModel::BuildModel(float size, long step, ulong flags) {
	long             i, j, index, ipt, nb_pt, nb_face;
	float            dx, dy, sx, sy;
	B3dVector        *listPt, *curPt;
	B3dFaceDesc      *listFace, *curFace;
	B3dFaceModelDesc *desc;

	if (step > 24) step = 24;
	step &= 0x1e;
// calculate count of point and faces
	nb_pt = (step+1) * (step+1);
	nb_face = 0;
	if (flags & B_FRONT_FACES)
		nb_face += step * step;
	if (flags & B_REAR_FACES)
		nb_face += step * step;
	nb_face *= 2;
// buffer allocation
	listPt = (B3dVector*)malloc(nb_pt*sizeof(B3dVector));
	listFace = (B3dFaceDesc*)malloc(nb_face*sizeof(B3dFaceDesc));
// create fake points
	curPt = listPt;
	dy = dx = size/(float)step;
	sy = 0.0;
	for (i=0; i<=step; i++) {
		sx = 0.0;
		for (j=0; j<=step; j++) {
			curPt->x = sx;
			curPt->y = sy;
			curPt->z = 0.0;
			sx += dx;
			curPt++;
		}
		sy += dy;
	}
// create faces (real ones).
	curFace = listFace;
	index = 0;
	ipt = step-1;
	for (i=0; i<step; i++) {
		for (j=step-1; j>=0; j--) {
			if (flags & B_FRONT_FACES) {
				curFace[0].points[0] = ipt; 
				curFace[0].points[1] = ipt+step+1;
				curFace[0].points[2] = ipt+1;
				curFace[0].norm = index;
				curFace[1].points[0] = ipt+step+1;
				curFace[1].points[1] = ipt+step+2;
				curFace[1].points[2] = ipt+1;
				curFace[1].norm = index+1;
				curFace += 2;
			}
			if (flags & B_REAR_FACES) {
				curFace[0].points[0] = ipt; 
				curFace[0].points[1] = ipt+1;
				curFace[0].points[2] = ipt+step+1;
				curFace[0].norm = index;
				curFace[1].points[0] = ipt+step+1;
				curFace[1].points[1] = ipt+1;
				curFace[1].points[2] = ipt+step+2;
				curFace[1].norm = index+1;
				curFace += 2;				
			}
			index += 2;
			ipt--;
		}
		ipt+=2*step+1;
	}
// create, fill and return constructor.
	desc = (B3dFaceModelDesc*)malloc(sizeof(B3dFaceModelDesc));
	desc->pointCount = nb_pt;
	desc->faceCount = nb_face;
	desc->faces = listFace;
	desc->points = listPt;
	return desc;
}

void B3dPageModel::SetShape(float alpha, float inertia, float traction) {
	long        i, j, k, hstep, step1;
	float       angle, coeff, c_iner, pas, prev_angle, a0, d_move, move;
	B3dVector   centre[25];
	B3dVector   norm[25];
	B3dVector   *curPt, *curNorm, *Pt, *Norm;
	B3dFaceDesc *curFace;

//	_sPrintf("face:%d point:%d norm:%d\n", faceCount, pointCount, normCount);
	coeff = 1.0/(float)step;
	pas = size*coeff;
	hstep = step/2;
	step1 = step+1;
// build the central curve
	// first point
	centre[0].x = size*0.5;
	centre[0].y = 0.0;
	centre[0].z = 0.0;
	prev_angle = 0.0;
	for (i=0; i<step; i++) {
		c_iner = (float)(step-i)*coeff*inertia;
		if (c_iner > 0.0)
			angle = angles_max[i] * (c_iner + alpha*(1.0-c_iner)); 
		else
			angle = angles_max[i] * (c_iner + alpha*(1.0+c_iner));
	// next point.
		centre[i+1].x = centre[i].x;
		centre[i+1].y = centre[i].y + pas * b_sin(angle);
		centre[i+1].z = centre[i].z + pas * b_cos(angle);
	// previous norm.
		a0 = (angle+prev_angle) * 0.5 - 1.5708;
		norm[i].x = 0.0;
		norm[i].y = b_sin(a0);
		norm[i].z = b_cos(a0);
		prev_angle = angle;
	}
    // last norm
	a0 = angle - 1.5708;
	norm[step].x = 0.0;
	norm[step].y = b_sin(a0);
	norm[step].z = b_cos(a0);
// rebuild points and point's norms.
	curPt = points;
	curNorm = norms+faceCount/2;
// first half, without traction
	for (i=0; i<=hstep; i++) {
		curPt->x = centre[i].x - pas * (float)hstep;
		curPt->y = centre[i].y;
		curPt->z = centre[i].z;
		curNorm[0] = norm[i];
		for (j=1; j<=step; j++) {
			curNorm[j] = curNorm[0];
			curPt[j].x = curPt[j-1].x + pas;
			curPt[j].y = curPt[0].y;
			curPt[j].z = curPt[0].z;
		}
		curPt += step1;
		curNorm += step1;
	}
// the axis is normal
	Pt = curPt+hstep;
	Norm = curNorm+hstep;
	for (i=1; i<= hstep; i++) {
		*Pt = centre[i+hstep];
		*Norm = norm[i+hstep];
		Pt += step1;
		Norm += step1;
	}
// second half, with traction.
	if (traction >= 0.0) {
		for (i=0; i<hstep*step1; i+=step1)
			for (j=hstep-1; j>=0; j--) {
				Pt = curPt+j+i;
				Norm = curNorm+j+i;
				Pt->x = Pt[1].x-pas;
				Pt->y = Pt[1].y;
				Pt->z = Pt[1].z;
				*Norm = Norm[1];
			}
	}
	else {
		d_move = -3.0 * (traction * inertia * pas)/(float)(hstep*hstep);
		move = 0.0;
		for (k=1; k<step; k++) {
			if (k>=hstep)
				if ((move < 0.12) && (move > -0.12))
					move += d_move;
			i = k-hstep;
			if (i<0) i = 0;
			for (;i<hstep ;i++) {
				j = hstep-k+i;
				if (j>=hstep) break;
				Pt = curPt+j+i*step1;
				Norm = curNorm+j+i*step1;
				Pt->x = Pt[1].x-pas;
				Pt->y = Pt[1].y + move*(Norm[1].y + Norm[-step1].y);
				Pt->z = Pt[1].z + move*(Norm[1].z + Norm[-step1].z);
				Norm->x = Norm[1].x + (Pt[1].x + Pt[-step1].x - 2*Pt->x) * move;
				Norm->y = Norm[1].y + (Pt[1].y + Pt[-step1].y - 2*Pt->y) * move;
				Norm->z = Norm[1].z + (Pt[1].z + Pt[-step1].z - 2*Pt->z) * move;
				Norm->Norm();
			}
		}
	}

	if (traction <= 0.0) {
		for (i=0; i<hstep*step1; i+=step1)
			for (j=hstep+1; j<=step; j++) {
				Pt = curPt+j+i;
				Norm = curNorm+j+i;
				Pt->x = Pt[-1].x+pas;
				Pt->y = Pt[-1].y;
				Pt->z = Pt[-1].z;
				*Norm = Norm[-1];
			}
	}
	else {
		d_move = 3.0 * (traction * inertia * pas)/(float)(hstep*hstep);
		move = 0.0;
		for (k=1+hstep; k<step+hstep; k++) {
			if (k>=step)
				move += d_move;
			i = k-step;
			if (i<0) i = 0;
			for (;i<hstep ;i++) {
				j = k-i;
				if (j<=hstep) break;
				Pt = curPt+j+i*step1;
				Norm = curNorm+j+i*step1;
				Pt->x = Pt[-1].x+pas;
				Pt->y = Pt[-1].y + move*(Norm[-1].y + Norm[-step1].y);
				Pt->z = Pt[-1].z + move*(Norm[-1].z + Norm[-step1].z);
				Norm->x = Norm[-1].x + (Pt[-1].x + Pt[-step1].x - 2*Pt->x) * move;
				Norm->y = Norm[-1].y + (Pt[-1].y + Pt[-step1].y - 2*Pt->y) * move;
				Norm->z = Norm[-1].z + (Pt[-1].z + Pt[-step1].z - 2*Pt->z) * move;
				Norm->Norm();
			}
		}
	}
	
// calculate norms of all faces.
	j = 0;
	if (flags & B_FRONT_FACES)
		j += 2;
	if (flags & B_REAR_FACES)
		j += 2;
	curNorm = norms;
	curFace = faces;
	for (i=0; i<faceCount; i+=j) {
		curNorm[0] = (points[curFace->points[2]]-points[curFace->points[1]])^
			(points[curFace->points[1]]-points[curFace->points[0]]);
		curNorm[0].Norm();
		curNorm[1] = (points[curFace[1].points[2]]-points[curFace[1].points[1]])^
			(points[curFace[1].points[1]]-points[curFace[1].points[0]]);
		curNorm[1].Norm();
		curNorm += 2;
		curFace += j;
	}
}











