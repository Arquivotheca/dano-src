/* ++++++++++

   FILE:  3dFaceLook.cpp
   REVS:  $Revision: 1.4 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _3D_FACE_LOOK_H
#include "3dFaceLook.h"
#endif
#ifndef _3D_FACE_MODEL_H
#include "3dFaceModel.h"
#endif
#ifndef _3D_RENDERER_H
#include "3dRenderer.h"
#endif
#ifndef _3D_LENS_H
#include "3dLens.h"
#endif
#ifndef _3D_FACE_H
#include "3dFace.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <Debug.h>

B3dMapFaceLook::B3dMapFaceLook(char      *name,
							   B3dModel  *Model,
							   float     *new_pt_refs,
							   map_desc  *new_map_list) :
B3dLook(name, Model) {
	pt_refs = new_pt_refs;
	map_list = new_map_list;
	curClip = 0;
}

B3dMapFaceLook::~B3dMapFaceLook() {
}

void *B3dMapFaceLook::ClipHook(void *look1, float rel1, void *look2, float rel2) {
	long     r, g, b, r1, g1, b1, val;
	float    fact;
	ulong    col1, col2;
	map_look *look;
	float    x0, y0, dx, dy;

	fact = rel1/(rel1-rel2);
	look = LookClip+curClip;
	curClip = (curClip+1)&7;
	col1 = ((map_look*)look1)->color;
	col2 = ((map_look*)look2)->color;
	x0 = ((map_look*)look1)->x_map;
	r1 = (col1>>24)&0xff;
	y0 = ((map_look*)look1)->y_map;
	g1 = (col1>>16)&0xff;
	b1 = (col1>>8)&0xff;
	dx = ((map_look*)look2)->x_map - x0;
	r = ((col2>>24)&0xff)-r1;
	dy = ((map_look*)look2)->y_map - y0;
	g = ((col2>>16)&0xff)-g1;
	b = ((col2>>8)&0xff)-b1;
	val = (long)(1024.0*fact);
	r1 = (r1+((r*val)>>10))&0xff;
	look->y_map = y0 + dy*fact;
	g1 = (g1+((g*val)>>10))&0xff;
	look->x_map = x0 + dx*fact;
	b1 = (b1+((b*val)>>10))&0xff;
	look->color = (r1<<24)|(g1<<16)|(b1<<8);
	return look;
}

B3dMap1FaceLook::B3dMap1FaceLook(char      *name,
								 B3dModel  *Model,
								 float     *pt_refs,
								 map_desc  *map_list) :
								 B3dMapFaceLook(name, Model, pt_refs, map_list) {
}

B3dMap1FaceLook::~B3dMap1FaceLook() {
}

void B3dMap1FaceLook::Draw(ulong         flags,
						   B3dMatrix     *Rot,
						   B3dVector     *Trans,
						   B3dLens       *Proj,
						   B3dRenderer   *Buf,
						   B3dLightProc  *lightProc) {
	void         *bits;
	long         row, i, nb, index;
	map_look     look1, look2, look3;
	ushort       *mySortDesc;
	float        sign;
	B3dVector    *norm;
	B_MAP        draw;
	B3dFaceDesc  *face, *face_ref;
	map_desc     *desc;
	map_ref      *ref;
	B3dLensPoint *p1, *p2, *p3;
	B3dLensImage proj;
	
	bits = Buf->bits;
	row = Buf->bytesPerRow;
	draw = (B_MAP)((Buf->RenderHooks())[B_RENDER_MAP]);
	
	face_ref = ((B3dFaceModel*)model)->faces;
	
	sign = 1.0;

	if (flags == B_CLIPPED_IN) {
		Proj->See(model, Rot, Trans, B_SEE_POINT|B_SEE_NORM, &proj);
		model->CalcSort(&proj, (void**)&mySortDesc);
		for (i=((B3dFaceModel*)model)->faceCount; i>0; i--) {
			index = *mySortDesc++;
			face = face_ref+index;
			desc = map_list+index;
			norm = proj.lensVectorAt(face->norm);
			p1 = (B3dLensPoint*)proj.lensPointAt(face->points[0]);

			if ((*norm) * (*p1) < 0.0) {
				p2 = (B3dLensPoint*)proj.lensPointAt(face->points[1]);
				p3 = (B3dLensPoint*)proj.lensPointAt(face->points[2]);
				look1.color =
					lightProc->Calc32(p1, norm, sign);
				look2.color =
					lightProc->Calc32(p2, norm, sign);
				look3.color =
					lightProc->Calc32(p3, norm, sign);
				ref = desc->bitmap;
//				_sPrintf("from1 : %x [%x]\n", ref->buf, ref);
				(draw)(p1->h, p1->v, look1.color,
					   pt_refs[desc->p1], pt_refs[desc->p1+1], p1->inv_z,
					   p2->h, p2->v, look2.color, 
					   pt_refs[desc->p2], pt_refs[desc->p2+1], p2->inv_z,
					   p3->h, p3->v, look3.color, 
					   pt_refs[desc->p3], pt_refs[desc->p3+1], p3->inv_z,
					   bits, row,
					   ref->buf, ref->size_h, ref->size_v);
			}
		}
	}
	else {
		Proj->See(model, Rot, Trans, B_SEE_POINT|B_SEE_NORM|B_SEE_CLIP, &proj);
		model->CalcSort(&proj, (void**)&mySortDesc);
		for (i=((B3dFaceModel*)model)->faceCount;i>0;i--) {
			index = *mySortDesc++;
			face = face_ref+index;
			desc = map_list+index;
			norm = proj.lensVectorAt(face->norm);
			p1 = (B3dLensPoint*)proj.lensPointAt(face->points[0]);

			if ((*norm) * (*p1) < 0.0) {
				p2 = (B3dLensPoint*)proj.lensPointAt(face->points[1]);
				p3 = (B3dLensPoint*)proj.lensPointAt(face->points[2]);
				look1.color =
					lightProc->Calc32(p1, norm, sign);
				look1.x_map = pt_refs[desc->p1];
				look1.y_map = pt_refs[desc->p1+1];
				look2.color =
					lightProc->Calc32(p2, norm, sign);
				look2.x_map = pt_refs[desc->p2];
				look2.y_map = pt_refs[desc->p2+1];
				look3.color =
					lightProc->Calc32(p3, norm, sign);
				look3.x_map = pt_refs[desc->p3];
				look3.y_map = pt_refs[desc->p3+1];
				ref = desc->bitmap;
//				_sPrintf("from1 : %x [%x]\n", ref->buf, ref);
				Proj->ClipMap(p1, p2, p3, &look1, &look2, &look3,
							  draw, this, bits, row, ref);
			}
		}
	}
}

B3dMap2FaceLook::B3dMap2FaceLook(char      *name,
								 B3dModel  *Model,
								 float     *pt_refs,
								 map_desc  *map_list) :
B3dMapFaceLook(name, Model, pt_refs, map_list) {
	int     i, nb;

	nb = ((B3dFaceModel*)model)->pointCount;	
	BufferLook = (ulong*)malloc(sizeof(ulong)*nb);
}

B3dMap2FaceLook::~B3dMap2FaceLook() {
	free(BufferLook);
}

void B3dMap2FaceLook::Draw(ulong         flags,
						   B3dMatrix     *Rot,
						   B3dVector     *Trans,
						   B3dLens       *Proj,
						   B3dRenderer   *Buf,
						   B3dLightProc  *lightProc) {
	void         *bits;
	long         row, i, nb, off, index;
	map_look     look1, look2, look3;
	ushort       *mySortDesc;
	float        scal, sign;
	B3dVector    *norm;
	ulong        *buf, *col1, *col2, *col3;
	B_MAP        draw;
	B3dFaceDesc  *face, *face_ref;
	map_desc     *desc;
	map_ref      *ref;
	B3dLensPoint *p1, *p2, *p3;
	B3dLensImage proj;

	bits = Buf->bits;
	row = Buf->bytesPerRow;
	draw = (B_MAP)(Buf->RenderHooks()[B_RENDER_MAP]);

	face_ref = ((B3dFaceModel*)model)->faces;

	nb = ((B3dFaceModel*)model)->pointCount;	
	buf = BufferLook;
	for (;nb>0;nb--) *buf++ = 0xfedcba87;
	buf = BufferLook;

	off = ((B3dFaceModel*)model)->normCount
		-((B3dFaceModel*)model)->pointCount;

	sign = 1.0;
	
	if (flags == B_CLIPPED_IN) {
		Proj->See(model, Rot, Trans, B_SEE_POINT|B_SEE_NORM, &proj);
		model->CalcSort(&proj, (void**)&mySortDesc);
		for (i=((B3dFaceModel*)model)->faceCount;i>0;i--) {
			index = *mySortDesc++;
	
			face = face_ref+index;
			desc = map_list+index;
			norm = proj.lensVectorAt(face->norm);
			p1 = (B3dLensPoint*)proj.lensPointAt(face->points[0]);
			scal = (*norm) * (*p1);
			if (desc->status & B_INVERT_NORM)
				sign = -1.0;
			else
				sign = 1.0;
			
			if (scal*sign < 0.0) {
				p2 = (B3dLensPoint*)proj.lensPointAt(face->points[1]);
				p3 = (B3dLensPoint*)proj.lensPointAt(face->points[2]);
				col1 = buf+face->points[0];
				if (*col1 == 0xfedcba87)
					*col1 = lightProc->Calc32(p1,
											  proj.lensVectorAt(face->points[0]+off),
											  sign);
				col2 = buf+face->points[1];
				if (*col2 == 0xfedcba87)
					*col2 = lightProc->Calc32(p2,
											  proj.lensVectorAt(face->points[1]+off),
											  sign);
				col3 = buf+face->points[2];
				if (*col3 == 0xfedcba87)
					*col3 = lightProc->Calc32(p3,
											  proj.lensVectorAt(face->points[2]+off),
											  sign);
				ref = desc->bitmap;
//				_sPrintf("from2 : %x [%x]\n", ref->buf, ref);
				(draw)(p1->h, p1->v, *col1,
					   pt_refs[desc->p1], pt_refs[desc->p1+1], p1->z,
					   p2->h, p2->v, *col2, 
					   pt_refs[desc->p2], pt_refs[desc->p2+1], p2->z,
					   p3->h, p3->v, *col3, 
					   pt_refs[desc->p3], pt_refs[desc->p3+1], p3->z,
					   bits, row,
					   ref->buf, ref->size_h, ref->size_v);
			}
		}
	}
	else {
		Proj->See(model, Rot, Trans, B_SEE_POINT|B_SEE_NORM|B_SEE_CLIP, &proj);
		model->CalcSort(&proj, (void**)&mySortDesc);
		for (i=((B3dFaceModel*)model)->faceCount;i>0;i--) {
			index = *mySortDesc++;
			face = face_ref+index;
			desc = map_list+index;
			norm = proj.lensVectorAt(face->norm);
			p1 = (B3dLensPoint*)proj.lensPointAt(face->points[0]);
			scal = (*norm) * (*p1);
			if (desc->status & B_INVERT_NORM)
				sign = -1.0;
			else
				sign = 1.0;
			
			if (scal*sign < 0.0) {
				p2 = (B3dLensPoint*)proj.lensPointAt(face->points[1]);
				p3 = (B3dLensPoint*)proj.lensPointAt(face->points[2]);
				col1 = buf+face->points[0];
				if (*col1 == 0xfedcba87)
					*col1 = lightProc->Calc32(p1,
											  proj.lensVectorAt(face->points[0]+off),
											  sign);
				look1.color = *col1;
				look1.x_map = pt_refs[desc->p1];
				look1.y_map = pt_refs[desc->p1+1];
				col2 = buf+face->points[1];
				if (*col2 == 0xfedcba87)
					*col2 = lightProc->Calc32(p2,
											  proj.lensVectorAt(face->points[1]+off),
											  sign);
				look2.color = *col2;
				look2.x_map = pt_refs[desc->p2];
				look2.y_map = pt_refs[desc->p2+1];
				col3 = buf+face->points[2];
				if (*col3 == 0xfedcba87)
					*col3 = lightProc->Calc32(p3,
											  proj.lensVectorAt(face->points[2]+off),
											  sign);
				look3.color = *col3;
				look3.x_map = pt_refs[desc->p3];
				look3.y_map = pt_refs[desc->p3+1];
				ref = desc->bitmap;
//				_sPrintf("from2 : %x [%x]\n", ref->buf, ref);
				Proj->ClipMap(p1, p2, p3, &look1, &look2, &look3,
							  draw, this, bits, row, ref);
			}
		}
	}
}













