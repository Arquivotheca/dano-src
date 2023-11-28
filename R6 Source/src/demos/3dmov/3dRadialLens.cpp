/* ++++++++++

   FILE:  3dRadialLens.cpp
   REVS:  $Revision: 1.1 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <stdlib.h>
#include <string.h>

#ifndef _3D_RADIAL_LENS_H
#include "3dRadialLens.h"
#endif
#ifndef _3D_MATH_LIB_H
#include "3dMathLib.h"
#endif

#include <Debug.h>

B3dRadialLens::B3dRadialLens() {
	long     i;

	maxVector = 512;
	maxPt = 512;
	lensVectors = (B3dVector*)malloc(maxVector*sizeof(B3dVector));
	lensPoints = (B3dRadialPoint*)malloc(maxPt*sizeof(B3dRadialPoint));
}

B3dRadialLens::~B3dRadialLens() {
	free((void*)lensPoints);
	free((void*)lensVectors);
}

void B3dRadialLens::CheckListSize(long nbVector, long nbPt) {
	if (nbVector > maxVector) {
		free((void*)lensVectors);
		do maxVector *= 2;
		while (maxVector < nbVector);
		lensVectors = (B3dVector*)malloc(maxVector*sizeof(B3dVector));
	}
	if (nbPt > maxPt) {
		free((void*)lensPoints);
		do maxPt *= 2;
		while (maxPt < nbPt);
		lensPoints = (B3dRadialPoint*)malloc(maxPt*sizeof(B3dRadialPoint));
	}
}

void B3dRadialLens::GetOptions(void *options) {
	memcpy((void*)options,(void*)&Options,sizeof(B3dRadialOptions));
}

void B3dRadialLens::SetOptions(void *options) {
	memcpy((void*)&Options,(void*)options,sizeof(B3dRadialOptions));
	Options.CheckOptions();
}

void B3dRadialOptions::CheckOptions() {
	float   Zoom;

	Zoom = 1.0/zoom;
	OffsetH = -(float)(long)((hMin+hMax)*0.5);
	OffsetV = -(float)(long)((vMin+vMax)*0.5);
	XZMin = (hMin+OffsetH)*Zoom;
	XZMax = (hMax+OffsetH)*Zoom;
	YZMin = (vMin+OffsetV)*Zoom;
	YZMax = (vMax+OffsetV)*Zoom;
	NormXZMin = b_sqrt(1.0+XZMin*XZMin);
	NormXZMax = b_sqrt(1.0+XZMax*XZMax);
	NormYZMin = b_sqrt(1.0+YZMin*YZMin);
	NormYZMax = b_sqrt(1.0+YZMax*YZMax);
//	fprintf(stderr,"View : [%d,%d,%d,%d]\n",(long)Hmin,(long)Hmax,(long)Vmin,(long)Vmax);
//	fprintf(stderr,"Zoom : %f\n",Zoom);
//	fprintf(stderr,"       [%f,%f,%f,%f]\n",XZMin,XZMax,YZMin,YZMax);
//	fprintf(stderr,"       [%f,%f,%f,%f]\n",NormXZMin,NormXZMax,NormYZMin,NormYZMax);
}

void B3dRadialLens::CalcAxis(short h, short v, B3dVector *axis) {
	float    x, y;

	x = (h+Options.OffsetH)/Options.zoom;
	y = (v+Options.OffsetV)/Options.zoom;
	axis->Set(x, y, 1.0);
	axis->Norm();
}

long B3dRadialLens::CheckSteric(B3dSteric *Gab, B3dMatrix *Rotation,
								B3dVector *Trans, float *Radius) {
	long        ObjectState;
	float		vrX,vrY,v2X,v2Y;

// Visible Radius approximation for LOD management
	if (Radius != 0L) {
		if (Trans->z > 2.0*Gab->radius)
			*Radius = (Gab->radius*Options.zoom)/Trans->z;
		else
			*Radius = 1e5;
	}
//	fprintf(stderr,"Cube centr09e:");
//	Trans.Debug();
//	fprintf(stderr,"Radius:%f\n",Gab->Radius);
//	Gab->PtMin.Debug();
//	Gab->PtMax.Debug();
	// check steric clipping
	ObjectState = B_CLIPPED_IN;
	vrX = Trans->x - Trans->z*Options.XZMin;
	v2X = Options.NormXZMin*Gab->radius;
//	fprintf(stderr,"%f < %f < %f\n",-v2X,vrX,v2X);
	if ((vrX+v2X) < 0.0) return B_CLIPPED_OUT;
	if ((vrX-v2X) < 0.0) ObjectState = B_UNCLIPPED;
	vrX = Trans->z*Options.XZMax - Trans->x;
	v2X = Options.NormXZMax*Gab->radius;
//	fprintf(stderr,"%f < %f < %f\n",-v2X,vrX,v2X);
	if ((vrX+v2X) < 0.0) return B_CLIPPED_OUT;
	if ((vrX-v2X) < 0.0) ObjectState = B_UNCLIPPED;
	vrY = Trans->y - Trans->z*Options.YZMin;
	v2Y = Options.NormYZMin*Gab->radius;
//	fprintf(stderr,"%f < %f < %f\n",-v2Y,vrY,v2Y);
	if ((vrY+v2Y) < 0.0) return B_CLIPPED_OUT;
	if ((vrY-v2Y) < 0.0) ObjectState = B_UNCLIPPED;
	vrY = Trans->z*Options.YZMax - Trans->y;
	v2Y = Options.NormYZMax*Gab->radius;
//	fprintf(stderr,"%f < %f < %f\n",-v2Y,vrY,v2Y);
	if ((vrY+v2Y) < 0.0) return B_CLIPPED_OUT;
	if ((vrY-v2Y) < 0.0) ObjectState = B_UNCLIPPED;
	return ObjectState;
}
	
void B3dRadialLens::See(B3dModel     *Model,
						B3dMatrix    *Rotation,
						B3dVector    *Trans,
						long         status,
						B3dLensImage *ProjDesc) {
	char		   Visible;
	long           i,nbPt,nbVector;
	float		   offH,offV;
	float		   divi,Hmin,Hmax,Vmin,Vmax,zoom,memoH,memoV;
	float		   vrX,vrY,vrZ,v2X,v2Y,v2Z;
	B3dVector      *vector,*vect;
	B3dRadialPoint *pt;

	// prefetch critical values
	offH = Options.OffsetH;
	offV = Options.OffsetV;
	zoom = Options.zoom;
	// check buffer size
	if (status & B_SEE_POINT)
		nbPt = Model->pointCount;
	else
		nbPt = 0;
	if (status & B_SEE_NORM)
		nbVector = Model->normCount;
	else
		nbVector = 0;
	if (status & B_SEE_VECTOR)
		nbVector += Model->vectorCount;
	CheckListSize(nbVector,nbPt);
	// project point
	if (status & B_SEE_POINT) {
		vector = Model->points;
		pt = (B3dRadialPoint*)lensPoints;
		if (!(status & B_SEE_CLIP)) {
			for (;nbPt>0;nbPt--) {
				// do translation/rotation
				vrX = vector->x;
				vrY = vector->y;
				vrZ = vector->z;
				v2X = Rotation->m11*vrX+Trans->x;
				v2Y = Rotation->m12*vrX+Trans->y;
				v2Z = Rotation->m13*vrX+Trans->z;
				v2X += Rotation->m21*vrY;
				v2Y += Rotation->m22*vrY;
				v2Z += Rotation->m23*vrY;
				v2X += Rotation->m31*vrZ;
				v2Y += Rotation->m32*vrZ;
				v2Z += Rotation->m33*vrZ;
				pt->x = v2X;
				pt->y = v2Y;
				pt->z = v2Z;
				// do projection
				divi = zoom/v2Z;
				memoH = v2X*divi-offH;
				memoV = v2Y*divi-offV;
				pt->inv_z = divi;
				pt->h = (long)memoH;
				pt->v = (long)memoV;
				vector++;
				pt++;
			}
		}
		else {
			Hmin = Options.hMin;
			Hmax = Options.hMax;
			Vmin = Options.vMin;
			Vmax = Options.vMax;
//			fprintf(stderr,"NbPt:%d\n",nbPt);
//			fprintf(stderr,"Offset:%f %f\n",offH,offV);
//			fprintf(stderr,"[%f,%f] [%f,%f]\n",Hmin,Hmax,Vmin,Vmax);
			for (;nbPt>0;nbPt--) {
				// do translation/rotation
				vrX = vector->x;
				vrY = vector->y;
				vrZ = vector->z;
				v2X = Rotation->m11*vrX+Trans->x;
				v2Y = Rotation->m12*vrX+Trans->y;
				v2Z = Rotation->m13*vrX+Trans->z;
				v2X += Rotation->m21*vrY;
				v2Y += Rotation->m22*vrY;
				v2Z += Rotation->m23*vrY;
				v2X += Rotation->m31*vrZ;
				v2Y += Rotation->m32*vrZ;
				v2Z += Rotation->m33*vrZ;
				pt->x = v2X;
				pt->y = v2Y;
				pt->z = v2Z;
				// can it be projected ?
				if (v2Z > 1e-12) {
					// do projection
					divi = zoom/v2Z;
					memoH = v2X*divi-offH;
					memoV = v2Y*divi-offV;
					pt->h = (long)memoH;
					pt->v = (long)memoV;
					// check horizontal overflow
					if (memoH < Hmin) {
						Visible = B_CLIP_HMIN;
						goto test_next;
					}
					else if (memoH > Hmax) {
						Visible = B_CLIP_HMAX;
						goto test_next;
					}
					else if (memoV < Vmin) {
						Visible = B_CLIP_VMIN;
						goto bad_test;
					}
					else if (memoV > Vmax) {
						Visible = B_CLIP_VMAX;
						goto bad_test;
					}
					pt->visible = 0;
					goto good_test;
				test_next:
					if (memoV < Vmin)
						Visible |= B_CLIP_VMIN;
					else if (memoV > Vmax)
						Visible |= B_CLIP_VMAX;
				bad_test:
					pt->visible = Visible;
				good_test:
					pt->inv_z = divi;
				}
				else
					pt->visible = B_CLIP_ZMIN;
				vector++;
				pt++;
			}
		}
	}
	if (status & (B_SEE_VECTOR|B_SEE_NORM)) {
		// project vector
		if (status & B_SEE_VECTOR)
			vector = Model->vectors;
		else
			vector = Model->norms;
		vect = lensVectors;
//		fprintf(stderr,"Nbvect: %d\n",nbVector);
		for (;nbVector>0;nbVector--) {
			// do rotation
			vrX = vector->x;
			vrY = vector->y;
			vrZ = vector->z;
			v2X = Rotation->m11*vrX;
			v2Y = Rotation->m12*vrX;
			v2Z = Rotation->m13*vrX;
			v2X += Rotation->m21*vrY;
			v2Y += Rotation->m22*vrY;
			v2Z += Rotation->m23*vrY;
			v2X += Rotation->m31*vrZ;
			v2Y += Rotation->m32*vrZ;
			v2Z += Rotation->m33*vrZ;
			vect->x = v2X;
			vect->y = v2Y;
			vect->z = v2Z;
			vector++;
			vect++;
		}
//		fprintf(stderr,"Vector end:%x\n",(long)vect);
	}
	ProjDesc->lensPoints = lensPoints;
	ProjDesc->lensPointSize = sizeof(B3dRadialPoint);
	ProjDesc->lensVectors = lensVectors;
//	fprintf(stderr,"Pt  :%x\n",(long)ProjDesc->ProjPtList);
//	fprintf(stderr,"Size:%x\n",(long)ProjDesc->ProjPtSize);
//	fprintf(stderr,"Vect:%x\n",(long)ProjDesc->ProjVectList);
}

void B3dRadialLens::ClipMap(B3dLensPoint *p1,
							B3dLensPoint *p2,
							B3dLensPoint *p3,
							map_look     *look1,
							map_look     *look2,
							map_look     *look3,
							B_MAP        draw,
							B3dLook      *looker,
							void         *bits,
							long         row,
							map_ref      *ref) {
	long      i;
	char	  MaskOr,MaskAnd;

//	fprintf(stderr,"Clip: (%d,%d) (%d,%d) (%d,%d)\n",p1->H,p1->V,p2->H,p2->V,p3->H,p3->V);
	MaskAnd = MaskOr = ((B3dRadialPoint*)p1)->visible;
	MaskAnd &= ((B3dRadialPoint*)p2)->visible;
	MaskOr |= ((B3dRadialPoint*)p2)->visible;
	MaskAnd &= ((B3dRadialPoint*)p3)->visible;
	MaskOr |= ((B3dRadialPoint*)p3)->visible;
// triangle visible
	if (MaskOr == 0) {
		(draw)(p1->h, p1->v, look1->color, look1->x_map, look1->y_map, p1->inv_z,
			   p2->h, p2->v, look2->color, look2->x_map, look2->y_map, p2->inv_z,
			   p3->h, p3->v, look3->color, look3->x_map, look3->y_map, p3->inv_z,
			   bits, row,
			   ref->buf, ref->size_h, ref->size_v);
		return;
	}
// triangle invisible
	if (MaskAnd != 0)
		return;
// triangle a clipper
	ClipPt[0] = (B3dRadialPoint*)p1;
	ClipPt[1] = (B3dRadialPoint*)p2;
	ClipPt[2] = (B3dRadialPoint*)p3;
	ClipLook[0] = (void*)look1;
	ClipLook[1] = (void*)look2;
	ClipLook[2] = (void*)look3;
	ClipNbPt = 3;
	curClipList = ClipList;
	if (MaskOr & (B_CLIP_HMIN|B_CLIP_ZMIN)) {
		ClipNorm.x = 1.0;
		ClipNorm.y = 0.0;
		ClipNorm.z = -Options.XZMin;
		ClipPlan(looker);
	}
	if (MaskOr & (B_CLIP_HMAX|B_CLIP_ZMIN)) {
		ClipNorm.x = -1.0;
		ClipNorm.y = 0.0;
		ClipNorm.z = Options.XZMax;
		ClipPlan(looker);
	}
	if (MaskOr & (B_CLIP_VMIN|B_CLIP_ZMIN)) {
		ClipNorm.x = 0.0;
		ClipNorm.y = 1.0;
		ClipNorm.z = -Options.YZMin;
		ClipPlan(looker);
	}
	if (MaskOr & (B_CLIP_VMAX+B_CLIP_ZMIN)) {
		ClipNorm.x = 0.0;
		ClipNorm.y = -1.0;
		ClipNorm.z = Options.YZMax;
		ClipPlan(looker);
	}
// draw result
//	fprintf(stderr,"NbPt clip:%d\n",ClipNbPt);
	if (ClipNbPt >= 3) {
		p1 = ClipPt[0];
		p3 = ClipPt[1];
		look1 = (map_look*)ClipLook[0];
		look3 = (map_look*)ClipLook[1];
		for (i=2;i<ClipNbPt;i++) {
			p2 = p3;
			p3 = ClipPt[i];
			look2 = look3;
			look3 = (map_look*)ClipLook[i];
//			fprintf(stderr,"Draw: (%d,%d) (%d,%d) (%d,%d)\n",
//					p1->H,p1->V,p2->H,p2->V,p3->H,p3->V);
			(draw)(p1->h, p1->v, look1->color, look1->x_map, look1->y_map, p1->inv_z,
				   p2->h, p2->v, look2->color, look2->x_map, look2->y_map, p2->inv_z,
				   p3->h, p3->v, look3->color, look3->x_map, look3->y_map, p3->inv_z,
				   bits, row,
				   ref->buf, ref->size_h, ref->size_v);
		}		
	}
}

void B3dRadialLens::ClipPlan(B3dLook *looker) {
	long			i, j, k, m;
	void            *CopyClipLook[7];
	void            *newLook1, *newLook2;
	void    		**newClipLook;
	float			ProjVal[10];
	B3dRadialPoint  *CopyClipList[7];
	B3dRadialPoint  *p;
	B3dRadialPoint  **newClipPt;
	
	// scan for first visible point
	for (i=0;i<ClipNbPt;i++) {
		p = ClipPt[i];
		if ((ProjVal[i] = (*p)*ClipNorm) > 0.0) {
			// i is the index of a visible point. Look for next hidden point
			for (j=i+1; j<ClipNbPt; j++) {
				p = ClipPt[j];
				if ((ProjVal[j] = (*p)*ClipNorm) <= 0.0) {
					// j is the index of the first hidden point
					CalculeNewPt(ClipPt[j-1], ProjVal[j-1],
								 ClipPt[j], ProjVal[j],
								 curClipList);
					newLook1 = looker->ClipHook(ClipLook[j-1], ProjVal[j-1],
												ClipLook[j], ProjVal[j]);
					// look for last hidden point
					for (k=j+1;k<ClipNbPt;k++) {
						p = ClipPt[k];
						if ((ProjVal[k] = (*p)*ClipNorm) > 0.0) {
							// hidden part from j to k-1
							CalculeNewPt(ClipPt[k-1], ProjVal[k-1],
										 ClipPt[k], ProjVal[k],
										 curClipList+1);
							newLook2 = looker->ClipHook(ClipLook[k-1], ProjVal[k-1],
														ClipLook[k], ProjVal[k]);
							newClipPt = ClipPt+j;
							newClipLook = ClipLook+j;
							for (m=k;m<ClipNbPt;m++) {
								CopyClipList[m] = ClipPt[m];
								CopyClipLook[m] = ClipLook[m];
							}
							*(newClipPt++) = curClipList;
							*(newClipPt++) = curClipList+1;
							*(newClipLook++) = newLook1;
							*(newClipLook++) = newLook2;
							for (m=k;m<ClipNbPt;m++) {
								*(newClipPt++) = CopyClipList[m];
								*(newClipLook++) = CopyClipLook[m];
							}
							curClipList += 2;
							ClipNbPt += 2-k+j;
							return;
						}
					}
					// hidden part from j to i-1, looping between extremities
					if (i == 0) {
						// hidden part from j to ClipNbPt-1
						CalculeNewPt(ClipPt[ClipNbPt-1], ProjVal[ClipNbPt-1],
									 ClipPt[0], ProjVal[0],
									 curClipList+1);
						newLook2 = looker->ClipHook(ClipLook[ClipNbPt-1], ProjVal[ClipNbPt-1],
													ClipLook[0], ProjVal[0]);
						newClipPt = ClipPt+j;
						newClipLook = ClipLook+j;
						*(newClipPt++) = curClipList;
						*(newClipPt++) = curClipList+1;
						*(newClipLook++) = newLook1;
						*(newClipLook++) = newLook2;
						curClipList += 2;
						ClipNbPt = j+2;
						return;
					}
					// hidden part from j to ClipNbPt-1 and from 0 to i-1
					else {
						CalculeNewPt(ClipPt[i-1], ProjVal[i-1],
									 ClipPt[i], ProjVal[i],
									 curClipList+1);
						newLook2 = looker->ClipHook(ClipLook[i-1], ProjVal[i-1],
													ClipLook[i], ProjVal[i]);
						newClipPt = ClipPt;
						newClipLook = ClipLook;
						for (m=i;m<j;m++) {
							*(newClipPt++) = ClipPt[m];
							*(newClipLook++) = ClipLook[m];
						}
						*(newClipPt++) = curClipList;
						*(newClipPt++) = curClipList+1;
						*(newClipLook++) = newLook1;
						*(newClipLook++) = newLook2;
						curClipList += 2;
						ClipNbPt = j-i+2;
						return;
					}
				}
			}
			// all point are visible
			if (i == 0) return;
			// the last hidden point is i-1. The hidden part is from 0 to i-1.
			CalculeNewPt(ClipPt[ClipNbPt-1], ProjVal[ClipNbPt-1],
						 ClipPt[0], ProjVal[0],
						 curClipList);
			newLook1 = looker->ClipHook(ClipLook[ClipNbPt-1], ProjVal[ClipNbPt-1],
										ClipLook[0], ProjVal[0]);
			CalculeNewPt(ClipPt[i-1], ProjVal[i-1],
						 ClipPt[i], ProjVal[i],
						 curClipList+1);
			newLook2 = looker->ClipHook(ClipLook[i-1], ProjVal[i-1],
										ClipLook[i], ProjVal[i]);
			newClipPt = ClipPt;
			newClipLook = ClipLook;
			for (m=i;m<ClipNbPt;m++) {
				*(newClipPt++) = ClipPt[m];
				*(newClipLook++) = ClipLook[m];
			}
			*(newClipPt++) = curClipList;
			*(newClipPt++) = curClipList+1;
			*(newClipLook++) = newLook1;
			*(newClipLook++) = newLook2;
			curClipList += 2;
			ClipNbPt += 2-i;
			return;
		}
	}
	// hidden face
	ClipNbPt = 0;
}

void B3dRadialLens::ClipPlan2() {
	long			i, j, k, m;
	float			ProjVal[10];
	B3dRadialPoint  *CopyClipList[7];
	B3dRadialPoint  *p;
	B3dRadialPoint  **newClipPt;
	
//	ClipNorm.Debug();
//	ClipPt[0]->Debug();
//	ClipPt[1]->Debug();
//	ClipPt[2]->Debug();
	// scan for first visible point
	for (i=0;i<ClipNbPt;i++) {
		p = ClipPt[i];
		if ((ProjVal[i] = (*p)*ClipNorm) > 0.0) {
//			fprintf(stderr,"ProjVal[%d] = %f\n",i,ProjVal[i]);
			// i is the index of a visible point. Look for next hidden point
			for (j=i+1; j<ClipNbPt; j++) {
				p = ClipPt[j];
				if ((ProjVal[j] = (*p)*ClipNorm) <= 0.0) {
					// j is the index of the first hidden point
//					fprintf(stderr,"ProjVal[%d] = %f\n",j,ProjVal[j]);
					CalculeNewPt(ClipPt[j-1], ProjVal[j-1],
								 ClipPt[j], ProjVal[j],
								 curClipList);
					// look for last hidden point
					for (k=j+1;k<ClipNbPt;k++) {
						p = ClipPt[k];
						if ((ProjVal[k] = (*p)*ClipNorm) > 0.0) {
							// hidden part from j to k-1
							CalculeNewPt(ClipPt[k-1], ProjVal[k-1],
										 ClipPt[k], ProjVal[k],
										 curClipList+1);
							newClipPt = ClipPt+j;
							for (m=k;m<ClipNbPt;m++) {
								CopyClipList[m] = ClipPt[m];
							}
							*(newClipPt++) = curClipList;
							*(newClipPt++) = curClipList+1;
							for (m=k;m<ClipNbPt;m++)
								*(newClipPt++) = CopyClipList[m];
							curClipList += 2;
							ClipNbPt += 2-k+j;
							return;
						}
					}
					// hidden part from j to i-1, looping between extremities
					if (i == 0) {
						// hidden part from j to ClipNbPt-1
						CalculeNewPt(ClipPt[ClipNbPt-1], ProjVal[ClipNbPt-1],
									 ClipPt[0], ProjVal[0],
									 curClipList+1);
						newClipPt = ClipPt+j;
						*(newClipPt++) = curClipList;
						*(newClipPt++) = curClipList+1;
						curClipList += 2;
						ClipNbPt = j+2;
						return;
					}
					// hidden part from j to ClipNbPt-1 and from 0 to i-1
					else {
						CalculeNewPt(ClipPt[i-1], ProjVal[i-1],
									 ClipPt[i], ProjVal[i],
									 curClipList+1);
						newClipPt = ClipPt;
						for (m=i;m<j;m++)
							*(newClipPt++) = ClipPt[m];
						*(newClipPt++) = curClipList;
						*(newClipPt++) = curClipList+1;
						curClipList += 2;
						ClipNbPt = j-i+2;
						return;
					}
				}
//				fprintf(stderr,"ProjVal[%d] = %f\n",j,ProjVal[j]);
			}
			// all point are visible
			if (i == 0) return;
			// the last hidden point is i-1. The hidden part is from 0 to i-1.
			CalculeNewPt(ClipPt[ClipNbPt-1], ProjVal[ClipNbPt-1],
						 ClipPt[0], ProjVal[0],
						 curClipList);
			CalculeNewPt(ClipPt[i-1], ProjVal[i-1],
						 ClipPt[i], ProjVal[i],
						 curClipList+1);
			newClipPt = ClipPt;
			for (m=i;m<ClipNbPt;m++)
				*(newClipPt++) = ClipPt[m];
			*(newClipPt++) = curClipList;
			*(newClipPt++) = curClipList+1;
			curClipList += 2;
			ClipNbPt += 2-i;
			return;
		}
//		fprintf(stderr,"ProjVal[%d] = %f\n",i,ProjVal[i]);
	}
	// hidden face
	ClipNbPt = 0;
}

void B3dRadialLens::CalculeNewPt(B3dRadialPoint *pt1, float rel1,
								 B3dRadialPoint *pt2, float rel2,
								 B3dRadialPoint *newPt) {	
	float	   base, divi, vX, vY, vZ, memoH, memoV;
	
	base = 1.0/(rel1-rel2);
	rel1 *= base;
	rel2 *= base;
	vX = pt2->x*rel1 - pt1->x*rel2;
	vY = pt2->y*rel1 - pt1->y*rel2;
	vZ = pt2->z*rel1 - pt1->z*rel2;
	newPt->x = vX;
	newPt->y = vY;
	newPt->z = vZ;
	if (vZ > 0.0) {
		divi = Options.zoom/vZ;
		memoH = vX*divi-Options.OffsetH;
		memoV = vY*divi-Options.OffsetV;
		newPt->h = (long)memoH;
		newPt->v = (long)memoV;
		newPt->inv_z = divi;
	}
}

















