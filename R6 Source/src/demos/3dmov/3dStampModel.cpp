/* ++++++++++

   FILE:  3dStampModel.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_STAMP_MODEL_H
#include "3dStampModel.h"
#endif
#ifndef _3D_STAMP_H
#include "3dStamp.h"
#endif
#ifndef _3D_LENS_H
#include "3dLens.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <stdlib.h>
#include <Debug.h>

B3dStampModel::B3dStampModel(float size0, long step0, ulong flags0) :
B3dFaceModel(BuildModel(size0, step0, flags0), flags0,
			 B_OWN_POINT_LIST|B_OWN_FACE_DESC|B_OWN_FACE_LIST) {
	int    i, j, k, index;
	long   *r_off;
	float  r, coeff;
	
	step = step0;
	
	keys = (ulong*)malloc(sizeof(ulong)*faceCount/2);
	tri1 = (ulong*)malloc(sizeof(ulong)*faceCount/2);
	tri2 = (ulong*)malloc(sizeof(ulong)*faceCount/2);
	
	index = 0;
	for (i=0; i<step; i++) {
		offset[i] = index;
		count[i] = 2*(step+i)+1;
		index += count[i];
	}
	for (i=step; i<2*step; i++) {
		offset[i] = index;
		count[i] = 2*(3*step-i)-1;
		index += count[i];
	}
// init in order A.	
	for (i=0; i<faceCount/2; i++)
		keys[i] = i<<18;
// marquage A.
	for (i=0; i<2*step; i++)
		for (j=offset[i]; j<offset[i]+count[i]; j++)
			keys[j] |= i;
// marquage B.
	for (i=0; i<step; i++) {
		k = count[i]-1;
		for (j=0; j<=k; j++)
			keys[offset[i]+k-j] |= (j>>1)<<6;
	}
	for (i=step; i<2*step; i++) {
		k = count[i]-1;
		for (j=0; j<=k; j++)
			keys[offset[i]+j] |= (2*step-1-(j>>1))<<6;
	}
// marquage C.
	for (i=0; i<step; i++) {
		k = count[i]-1;
		for (j=0; j<=k; j++)
			keys[offset[i]+j] |= (j>>1)<<12;
	}
	for (i=step; i<2*step; i++) {
		k = count[i]-1;
		for (j=0; j<=k; j++)
			keys[offset[i]+k-j] |= (2*step-1-(j>>1))<<12;
	}

// radius effect management
	radius = (float*)malloc(sizeof(float)*(step*step+40));
	index_radius = (ushort*)malloc(sizeof(float)*pointCount);
	radius_count = 0;
	coeff = 2.0/size0;
	for (i=0; i<pointCount; i++) {
		r = b_sqrt(points[i].x*points[i].x + points[i].y*points[i].y)*coeff;
		for (j=0; j<radius_count; j++)
			if ((radius[j] < (r+1e-5)) && (radius[j] > (r-1e-5)))
				break;
		if (j == radius_count)
			radius[radius_count++] = r;
		index_radius[i] = j;
	}
	radius2z = (float*)malloc(sizeof(float)*radius_count);
	
// calculate the huge norm tables
	table_norms = (ushort*)malloc(sizeof(ushort)*6*pointCount);
	for (i=0; i<pointCount*6; i++)
		table_norms[i] = 0xffff;
	r_off = (long*)malloc(sizeof(long)*pointCount);
	for (i=0; i<pointCount; i++)
		r_off[i] = 6*i;
	for (i=0; i<faceCount; i+=2)
		for (j=0; j<3; j++)
			table_norms[r_off[faces[i].points[j]]++] = i>>1;
	free(r_off);
}

B3dStampModel::~B3dStampModel() {
	free(keys);
	free(tri1);
	free(tri2);
	free(radius);
	free(radius2z);
	free(index_radius);
	free(table_norms);
}

B3dFaceModelDesc *B3dStampModel::BuildModel(float size, long step, ulong flags) {
	long             i, j, k, index, ipt, ipt2, nb_pt, nb_face;
	long             begin[50];
	float            dx, dy, sx, sy;
	B3dVector        *listPt, *curPt;
	B3dFaceDesc      *listFace, *curFace;
	B3dFaceModelDesc *desc;

// calculate pt and face count.
	nb_pt = 3*step*(step+1)+1;
	nb_face = 12*step*step;
// buffer allocation
	listPt = (B3dVector*)malloc(nb_pt*sizeof(B3dVector));
	listFace = (B3dFaceDesc*)malloc(nb_face*sizeof(B3dFaceDesc));
// create points.
	curPt = listPt;
	dy = size/(2.0*(float)step);
	dx = dy*0.866;
	index = 0;
	begin[0] = 0;
	for (i=-step; i<=step; i++) {
		if (i < 0) k = 2*step+i;
		else k = 2*step-i;
		index += k+1;
		begin[i+step+1] = index;
		sx = dx * (float)i;
		sy = -0.5 * dy * (float)k;
		for (j=0; j<=k; j++) {
			curPt->x = sx;
			curPt->y = sy;
			curPt->z = 0.0;
			curPt++;
			sy += dy; 
		}
	}
// create face definition
	curFace = listFace;
	index = 0;
	for (i=-step; i<0; i++) {
		ipt = begin[i+step];
		ipt2 = begin[i+step+1];
		k = begin[i+step+2] - ipt - 2;
		for (j=0; j<k; j++) {
			if (j & 1) {
				curFace[0].points[0] = ipt; 
				curFace[0].points[1] = ipt+1;
				curFace[0].points[2] = ipt2+1;
				curFace[1].points[0] = ipt; 
				curFace[1].points[1] = ipt2+1;
				curFace[1].points[2] = ipt+1;
				ipt++;
				ipt2++;
			}
			else {
				curFace[0].points[0] = ipt; 
				curFace[0].points[1] = ipt2+1;
				curFace[0].points[2] = ipt2;
				curFace[1].points[0] = ipt; 
				curFace[1].points[1] = ipt2;
				curFace[1].points[2] = ipt2+1;
			}
			curFace[0].norm = index;
			curFace[1].norm = index;
			curFace += 2;
			index++;
		}
	}
	for (i=0; i<step; i++) {
		ipt = begin[i+step];
		ipt2 = begin[i+step+1];
		k = begin[i+step+2] - ipt - 2;
		for (j=0; j<k; j++) {
			if (j & 1) {
				curFace[0].points[0] = ipt+1; 
				curFace[0].points[1] = ipt2+1;
				curFace[0].points[2] = ipt2;
				curFace[1].points[0] = ipt+1; 
				curFace[1].points[1] = ipt2;
				curFace[1].points[2] = ipt2+1;
				ipt++;
				ipt2++;
			}
			else {
				curFace[0].points[0] = ipt; 
				curFace[0].points[1] = ipt+1;
				curFace[0].points[2] = ipt2;
				curFace[1].points[0] = ipt; 
				curFace[1].points[1] = ipt2;
				curFace[1].points[2] = ipt+1;
			}
			curFace[0].norm = index;
			curFace[1].norm = index;
			curFace += 2;
			index++;
		}
	}
	
// create, fill and return constructor.
	desc = (B3dFaceModelDesc*)malloc(sizeof(B3dFaceModelDesc));
	desc->pointCount = nb_pt;
	desc->faceCount = nb_face;
	desc->faces = listFace;
	desc->points = listPt;
	return desc;
}

void B3dStampModel::SetShape(double time) {
	int         i, j;
	long        h, v;
	ulong       val;
	ushort      *t_norms;
	B3dVector   vect;
	B3dVector   *curNorm;
	B3dFaceDesc *curFace;

	for (i=0; i<radius_count; i++)
		radius2z[i] = 0.16*b_cos(radius[i]*10.0-time*3e-6)/(1.0+3.0*radius[i]);
	for (i=0; i<pointCount; i++)
		 points[i].z = radius2z[index_radius[i]];
	
// calculate norms of all faces.
	curNorm = norms;
	curFace = faces+1;
	for (i=0; i<faceCount; i+=2) {
		curNorm[0] = (points[curFace->points[2]]-points[curFace->points[1]])^
			(points[curFace->points[1]]-points[curFace->points[0]]);
		curNorm[0].Norm();
		curNorm++;
		curFace += 2;
	}
// calculate norms at all points
	t_norms = table_norms;
	for (i=0; i<pointCount; i++) {
		vect = norms[t_norms[0]]+norms[t_norms[1]];
		for (j=2; j<6; j++) {
			if (t_norms[j] == 0xffff)
				break;
		}
	    vect.Norm();
		*curNorm++ = vect;
		t_norms += 6;
	}
}

void B3dStampModel::CalcSort(B3dLensImage *lensImage, void **sortDesc) {
	long        pos[RES_MAX];
	long        i, j, Pa, Pb, Pc, P_temp, s_temp, P1, P2, P3;
	long        P[3], s[3], s1, s2, s3;
	float       val[3];
	float       Na, Nb, Nc, Px, Py, val_temp;
	ushort      *list;
	B3dVector   Vx, Vy, Va, Vb, Vc;
	B3dVector   *p0, *p1, *p2;
	
	if (sortedList == 0L)
		sortedList = (ushort*)malloc(sizeof(ushort)*faceCount);
// look for coordonate of the camera in the 3 band systems.
	p0 = (B3dVector*)(lensImage->lensPointAt(0));
	p1 = (B3dVector*)(lensImage->lensPointAt(1));
	p2 = (B3dVector*)(lensImage->lensPointAt(2*step+4));
	Vy = *p0 - *p1;
	Vx = (*p0 - *p2)*0.5;
	
	Va = Vx;
	Vb = (Vx*0.5) + (Vy*(-0.75));
	Vc = (Vx*0.5) + (Vy*0.75);
	Na = Va.Norm(TRUE);
	Nb = Vb.Norm(TRUE);
	Nc = Vc.Norm(TRUE);
	
	p0 = (B3dVector*)(lensImage->lensPointAt((pointCount-1)/2));
	Pa = (long)(((*p0)*Va)/Na + (float)step - 0.5);
	if (Pa > (2*step-1))
		Pa = 2*step-1;
	if (Pa < 0)
		Pa = 0;
	P[0] = Pa;
	s[0] = 0;
	if (Va.z > 0.0)
		val[0] = Va.z;
	else
		val[0] = -Va.z; 
	Pb = (long)(((*p0)*Vb)/Nb + (float)step - 0.5);
	if (Pb > (2*step-1))
		Pb = 2*step-1;
	if (Pb < 0)
		Pb = 0;
	P[1] = Pb;
	s[1] = 6;
	if (Vb.z > 0.0)
		val[1] = Vb.z;
	else
		val[1] = -Vb.z; 
	Pc = (long)(((*p0)*Vc)/Nc + (float)step - 0.5);
	if (Pc > (2*step-1))
		Pc = 2*step-1;
	if (Pc < 0)
		Pc = 0;
	P[2] = Pc;
	s[2] = 12;
	if (Vc.z > 0.0)
		val[2] = Vc.z;
	else
		val[2] = -Vc.z; 
	for (i=0; i<2; i++)
		for (j=0; j<2; j++) {
			if (val[j] > val[j+1]) {
				P_temp = P[j];
				s_temp = s[j];
				val_temp = val[j];
				P[j] = P[j+1];
				s[j] = s[j+1];
				val[j] = val[j+1];
				P[j+1] = P_temp;
				s[j+1] = s_temp;
				val[j+1] = val_temp;
			}
		}
	
	P1 = P[0];
	P2 = P[1];
	P3 = P[2];
	s1 = s[0];
	s2 = s[1];
	s3 = s[2];
	
	for (i=0; i<2*step; i++)
		pos[i] = offset[i];
	for (i=0; i<faceCount/2; i++)
		tri1[pos[(keys[i]>>s1)&0x3f]++] = keys[i];

	for (i=0; i<2*step; i++)
		pos[i] = offset[i];
	for (i=2*step-1; i>P1; i--)
		for (j=offset[i]; j<offset[i]+count[i]; j++)
			tri2[pos[(tri1[j]>>s2)&0x3f]++] = tri1[j];
	for (i=0; i<=P1; i++)
		for (j=offset[i]; j<offset[i]+count[i]; j++)
			tri2[pos[(tri1[j]>>s2)&0x3f]++] = tri1[j];

	for (i=0; i<2*step; i++)
		pos[i] = offset[i];
	for (i=2*step-1; i>P2; i--)
		for (j=offset[i]; j<offset[i]+count[i]; j++)
			tri1[pos[(tri2[j]>>s3)&0x3f]++] = tri2[j];
	for (i=0; i<=P2; i++)
		for (j=offset[i]; j<offset[i]+count[i]; j++)
			tri1[pos[(tri2[j]>>s3)&0x3f]++] = tri2[j];

	list = sortedList;
	for (i=2*step-1; i>P3; i--)
		for (j=offset[i]; j<offset[i]+count[i]; j++) {
			*list++ = (tri1[j]>>17)&0x7ffe;
			*list++ = list[-1]+1;
		}
	for (i=0; i<=P3; i++)
		for (j=offset[i]; j<offset[i]+count[i]; j++) {
			*list++ = (tri1[j]>>17)&0x7ffe;
			*list++ = list[-1]+1;
		}
	
	*sortDesc = (void*)sortedList;
}














