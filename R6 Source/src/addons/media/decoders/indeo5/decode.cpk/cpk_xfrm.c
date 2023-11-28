/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
/*
 * cpk_xfrm.c
 *	Transformation functions.  All the functions in this file are inverse
 *	transformations used in decode.  The Inv____ routines perform an actual
 *	transformation on all the input data, while the Smear____ routines are
 *	just a special case of the corresponding Inv____ routine where only a
 *	DC coeficient is present.  This is in the case of an empty intra block.
 *
 * Functions
 *	InvSlant8x8
 *	InvSlant1x8
 *	InvSlant8x1
 *	InvNada8x8
 *	InvSlant4x4
 *	InvSlant1x4
 *	InvSlant4x1
 *	InvNada4x4
 *	SmearSlant8x8
 *	SmearSlant1x8
 *	SmearSlant8x1
 *	SmearNada8x8
 *	SmearSlant4x4
 *	SmearSlant1x4
 *	SmearSlant4x1
 *	SmearNada4x4
 */

#include "datatype.h"

#define BT_PITCH	0x0000ffe0	/* pitch indicator */

/*////////////////////////////////////////////////////////////////////////////*/

#define bfly(x,y) t1 = x-y; x += y; y = t1;

/* This is a reflection using a,b = 1/2, 5/4, more precision, rounding */
#define treflect(s1,s2)			\
{	int t;						\
	t  =  (s1*5 + s2*2 + 2)>>2;	\
	s2 = (s1*2 - s2*5 + 2)>>2;	\
	s1 = t;						\
}

/*////////////////////////////////////////////////////////////////////////////*/


#define SlantPart1		\
	bfly(r1,r4);		\
	bfly(r2,r3);		\
	bfly(r5,r8);		\
	bfly(r6,r7);

#define ExpSlantPart2	\
	bfly(r1,r2)			\
	treflect(r4,r3)		\
	bfly(r5,r6)			\
	treflect(r8,r7)

#define SlantPart3		\
	bfly(r1,r5);		\
	bfly(r2,r6);		\
	bfly(r7,r3);		\
	bfly(r4,r8);

/* This is a reflection of r4,r5 using a8,b8 = 7/8, 1/2, more precise & round */
#define ExpSlantPart4			\
	{ int t;					\
	t  =  (r4*4 + r5*7 + 4)>>3;	\
	r5 =  (r4*7 - r5*4 + 4)>>3;	\
	r4 = t;						\
	}



/*////////////////////////////////////////////////////////////////////////////*/


static __inline void
ColInvSlant1d8 (PI32 p)
{
	I32 r1,r2,r3,r4,r5,r6,r7,r8;
	I32 t1;

	/* read in r1 through r8 from the row in the correct order */
	r1 = p[0];
	r4 = p[8];
	r8 = p[16];
	r5 = p[24];
	r2 = p[32];
	r6 = p[40];
	r3 = p[48];
	r7 = p[56];

	ExpSlantPart4 SlantPart3 ExpSlantPart2 SlantPart1

	/* write out r1 through r8 back to the row in the correct order */
	p[0]  = r1;
	p[8]  = r2;
	p[16] = r3;
	p[24] = r4;
	p[32] = r5;
	p[40] = r6;
	p[48] = r7;
	p[56] = r8;
	return;
}


static __inline void
RowInvSlant1d8 (PI32 p, PI16 d)
{
	I32 r1,r2,r3,r4,r5,r6,r7,r8;
	I32 t1;

	/* read in r1 through r8 from the row in the correct order */
	r1 = p[0];
	r4 = p[1];
	r8 = p[2];
	r5 = p[3];
	r2 = p[4];
	r6 = p[5];
	r3 = p[6];
	r7 = p[7];

	ExpSlantPart4 SlantPart3 ExpSlantPart2 SlantPart1

	/* write out r1 through r8 back to the row in the correct order */
	d[0] = (r1+1) >> 1;
	d[1] = (r2+1) >> 1;
	d[2] = (r3+1) >> 1;
	d[3] = (r4+1) >> 1;
	d[4] = (r5+1) >> 1;
	d[5] = (r6+1) >> 1;
	d[6] = (r7+1) >> 1;
	d[7] = (r8+1) >> 1;

	return;
}

void InvSlant8x8(PI32 src, PU8 dst, U32 flags) {
	U32		i;
	U32		pitch = flags & BT_PITCH;

	/* 2d slant transform */
	/* Inverse slant transform the columns */
	for (i = 0; i < 8; i++) {
		ColInvSlant1d8 (src);
		src++;
	}
	src -= 8;
	/* now do the rows */
	for (i = 0; i < 8; i++) {
		RowInvSlant1d8 (src, (PI16)dst);
		dst = (PU8) ((U32)dst + pitch);
		src += 8;
	}
	return;
}


/*////////////////////////////////////////////////////////////////////////////*/

void InvSlant1x8(PI32 src, PU8 dst, U32 flags) {
	U32		i;
	U32		pitch = flags & BT_PITCH;

	for (i = 0; i < 8; i++) {
		RowInvSlant1d8 (src, (PI16)dst);
		dst = (PU8)((U32)dst + pitch);
		src += 8;
	}
	return;
}

/*////////////////////////////////////////////////////////////////////////////*/

static __inline void
ColInvSlant1d8_2 (PI32 p, PI16 d, U32 pitch)
{
	I32 r1,r2,r3,r4,r5,r6,r7,r8;
	I32 t1;

	/* read in r1 through r8 from the row in the correct order */

	r1 = p[0];
	r4 = p[8];
	r8 = p[16];
	r5 = p[24];
	r2 = p[32];
	r6 = p[40];
	r3 = p[48];
	r7 = p[56];

	ExpSlantPart4 SlantPart3 ExpSlantPart2 SlantPart1

	/* write out r1 through r8 back to the row in the correct order */
	pitch /= 2;

	d[pitch*0]	= (r1+1) >> 1;
	d[pitch*1]	= (r2+1) >> 1;
	d[pitch*2]	= (r3+1) >> 1;
	d[pitch*3]	= (r4+1) >> 1;
	d[pitch*4]	= (r5+1) >> 1;
	d[pitch*5]	= (r6+1) >> 1;
	d[pitch*6]	= (r7+1) >> 1;
	d[pitch*7]	= (r8+1) >> 1;

	return;
}


void InvSlant8x1(PI32 src, PU8 dst, U32 flags) {
	U32		i;
	U32		pitch = flags & BT_PITCH;

	for (i = 0; i < 8; i++) {
		ColInvSlant1d8_2 (src, (PI16)dst, pitch);
		src++;
		dst+=2;
	}

	return;
}

/*////////////////////////////////////////////////////////////////////////////*/

void InvNada8x8(PI32 src, PU8 dst, U32 flags) {
	U8		i,j;
	PI32	srcrow, srccol;
	PI16	dstrow, dstcol;
	U32		pitch = flags & BT_PITCH;

	srcrow = src;
	dstrow = (PI16)dst;
	for (i = 0; i < 8; i++) {
		srccol = srcrow;
		dstcol = dstrow;
		for (j = 0; j < 8; j++) {
			dstcol[j] = (I16)srccol[j];
		}
		srcrow += 8;
		dstrow = (PI16)((U32)dstrow + pitch);
	}
	return;
}



/*////////////////////////////////////////////////////////////////////////////*/




static __inline void
ColInvSlant1d4 (PI32 p)
{
	I32 r1,r2,r3,r4;
	I32 t1;

	r1 = p[0];
	r4 = p[8];
	r2 = p[16];
	r3 = p[24];

	bfly(r1,r2); treflect(r4,r3);	/* part2 */
	bfly(r1,r4); bfly(r2,r3); 		/* part1 */

	p[0] = r1;
	p[8] = r2;
	p[16] = r3;
	p[24] = r4;
	return;
}

static __inline void
RowInvSlant1d4 (PI32 p, PI16 d)
{
	I32 r1,r2,r3,r4;
	I32 t1;

	r1 = p[0];
	r4 = p[1];
	r2 = p[2];
	r3 = p[3];

	bfly(r1,r2); treflect(r4,r3);	/* part2 */
	bfly(r1,r4); bfly(r2,r3); 		/* part1 */

	d[0] = (r1+1) >> 1;
	d[1] = (r2+1) >> 1;
	d[2] = (r3+1) >> 1;
	d[3] = (r4+1) >> 1;

	return;
}


void InvSlant4x4(PI32 src, PU8 dst, U32 flags) {
	U32		i;
	U32		pitch = flags & BT_PITCH;

	/* 2d slant transform */
	/* Inverse slant transform the columns */
	for (i = 0; i < 4; i++) {
		ColInvSlant1d4 (src);
		src++;
	}
	src -= 4;
	/* now do the rows */
	for (i = 0; i < 4; i++) {
		RowInvSlant1d4 (src, (PI16)dst);
		dst = (PU8)((U32)dst + pitch);
		src += 8;
	}

	return;
}

void InvSlant1x4(PI32 src, PU8 dst, U32 flags) {
	return;
}
void InvSlant4x1(PI32 src, PU8 dst, U32 flags) {
	return;
}

void InvNada4x4(PI32 src, PU8 dst, U32 flags) {
	U8		i,j;
	PI32	srcrow, srccol;
	PI16	dstrow, dstcol;
	U32		pitch;

	pitch = flags & BT_PITCH;
	srcrow = src;
	dstrow = (PI16)dst;
	for (i = 0; i < 4; i++) {
		srccol = srcrow;
		dstcol = dstrow;
		for (j = 0; j < 4; j++) {
			dstcol[j] = (I16)srccol[j];
		}
		srcrow += 4;
		dstrow = (PI16)((U32)dstrow + pitch);
	}
	return;
}


/*////////////////////////////////////////////////////////////////////////////*/

void SmearSlant8x8(PI32 src, PU8 dst, U32 flags) {
	U32		i,j;
	U32		pitch = flags & BT_PITCH;
	PI16	col;
	I16		fill = (src[0]+1) >> 1;

	/* 2d slant transform with only a dc coefficient */
	for (i = 0; i < 8; i++) {
		col = (PI16)dst;
		for (j = 0; j < 8; j++) {
			*col++ = fill;
		}
		dst += pitch;
	}
	return;
}

void SmearSlant1x8(PI32 src, PU8 dst, U32 flags) {
	U32		i,j;
	U32		pitch = flags & BT_PITCH;
	PI16	col;
	I16		fill = (src[0]+1) >> 1;

	/* 1d row slant transform with only a dc coefficient */
	col = (PI16)dst;
	for (j = 0; j < 8; j++) {
		*col++ = fill;
	}
	dst += pitch;
	for (i = 1; i < 8; i++) {
		col = (PI16)dst;
		for (j = 0; j < 8; j++) {
			*col++ = 0;
		}
		dst += pitch;
	}

	return;
}

void SmearSlant8x1(PI32 src, PU8 dst, U32 flags) {
	U32		i,j;
	U32		pitch = flags & BT_PITCH;
	PI16	col;
	I16		fill = (src[0]+1) >> 1;

	/* 1d col slant transform with only a dc coefficient */
	for (i = 0; i < 8; i++) {
		col = (PI16)dst;
		*col++ = fill;

		for (j = 1; j < 8; j++) {
			*col++ = 0;
		}
		dst += pitch;
	}
	return;
}

void SmearNada8x8 (PI32 src, PU8 dst, U32 flags) {
	U32		i,j;
	U32		pitch = flags & BT_PITCH;
	PI16	col;
	I16		fill = (I16)src[0];

	col = (PI16)dst;

	col[0] = fill;

	/* nada transform with only a dc coefficient */
	for (i = 0; i < 8; i++) {
		col = (PI16)dst;
		for (j = !i; j < 8; j++) {
			col[j] = 0;
		}
		dst += pitch;
	}
	return;
}

void SmearSlant4x4(PI32 src, PU8 dst, U32 flags) {
	U32		i,j;
	U32		pitch = flags & BT_PITCH;
	PI16	col;
	I16		fill = (src[0]+1) >> 1;

	/* 2d slant transform with only a dc coefficient */
	for (i = 0; i < 4; i++) {
		col = (PI16)dst;
		for (j = 0; j < 4; j++) {
			*col++ = fill;
		}
		dst += pitch;
	}
	return;
}

void SmearSlant1x4(PI32 src, PU8 dst, U32 flags) {
	return;
}

void SmearSlant4x1(PI32 src, PU8 dst, U32 flags) {
	return;
}

void SmearNada4x4 (PI32 src, PU8 dst, U32 flags) {
	U32		i,j;
	U32		pitch = flags & BT_PITCH;
	PI16	col;
	I16		fill = (I16)src[0];

	col = (PI16)dst;

	col[0] = fill;
	
	/* nada transform with only a dc coefficient */
	for (i = 0; i < 4; i++) {
		col = (PI16)dst;
		for (j = !i; j < 4; j++) {

			col[j] = 0;
		}
		dst += pitch;
	}
	return;
}
