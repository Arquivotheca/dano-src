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
/*************************************************************************
 *                                                                       *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                *
 *                                                                       *
 *      This software is supplied under the terms of a license           *
 *      agreement or nondisclosure agreement with Intel Corporation      *
 *      and may not be copied or disclosed except in accordance          *
 *      with the terms of that agreement.                                *
 *                                                                       *
 *************************************************************************/

/*************************************************************************
 * MCASM.C -- Assembly code for MMX style motion compensation
 *************************************************************************/
/*; BIASING: 1 for ia classic input, 0 for mmx input/output */
/*;	0 means the input data is in a normal signed word format */
/*;	1 means the input data has a bias added to it */
/*#define BIASING */
#define IMUL_LOOKUP

#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"

#define SIMDSIZE 2 /* work on 2 elements at a time for this version */
#define SIMDBIAS 0x40004000 /* old ia bias */

#define MV2X1(x) ((I32)(((U32)x)&0xff)-0x80)
#define MV2Y1(x) ((I32)((((U32)x)>>8)&0xff)-0x80)
#define MV2X2(x) ((I32)((((U32)x)>>16)&0xff)-0x80)
#define MV2Y2(x) ((I32)((((U32)x)>>24)&0xff)-0x80)

#define int_int 0
#define int_half 1
#define half_int 2
#define half_half 3


#if B_HOST_IS_BENDIAN
#define MISLOAD32(x)  ((*((PU16)((PU8)(x)+0)))<<16)|(*(PU16)((PU8)(x)+2))
#else /* B_HOST_IS_BENDIAN */
#define MISLOAD32(x) ((*((PU16)((PU8)(x)+2)))<<16)|(*(PU16)((PU8)(x)+0))
#endif /* B_HOST_IS_BENDIAN */

void AddMotionToDelta_C(U32 uFrameType, pBlockInfoSt pBlockInfo, U32 uNBlocks,
	U32 uBlockSize, U32 uMVRes,
	PU32 currbase, PU32 fwdbase, PU32 bkwdbase) {

	U32 flags;
	U32 motion_vectors;
	U32 curr;

#if 0 /* old bidir code */
	U32 uTempStorage[8*8/2];
#endif /*0 old bidir code */

	I32 i;

	if (!uFrameType) return;		/* only process PDB frames */

	bkwdbase = (PU32)((U32)bkwdbase - (U32)currbase);
	fwdbase = (PU32)((U32)fwdbase - (U32)currbase);

	i = -(I32)uNBlocks;

/*	Here's what we're doing:
		if !bidir goto process_first
		if coded curryptr = temp_storage
		goto
		processfirst
 */
loopstart:
	if (i < 0) { /* foreach block */
		U32 curryptr, currxptr;
		U32 predyptr, predxptr;
		U32 t, tt;
		U8 r,c, vector_type;
		I32 mv_x, mv_y;
		U32 pitch, currpitch, f;

		flags = pBlockInfo[uNBlocks + i].flags; i++;

		pitch = flags;
		pitch &= BT_PITCH;
		currpitch = pitch;
		f = flags;
		f &= (BT_FWD|BT_BKWD);
		f &= (BT_FWD|BT_BKWD);
		if (f == 0) goto loopstart; /* jz loopstart */
		motion_vectors = pBlockInfo[uNBlocks + i - 1].motion_vectors;
		curryptr = predyptr = curr = (U32)pBlockInfo[uNBlocks + i - 1].curr;
		mv_x = MV2X1(motion_vectors);
		mv_y = MV2Y1(motion_vectors);

		if (flags & BT_FWD) {
			predyptr += (U32)fwdbase;
		}
		else {
			predyptr += (U32)bkwdbase;
			mv_x = -mv_x;
			mv_y = -mv_y;
		}

		if (uMVRes) {
			vector_type = (U8)((mv_x & uMVRes) | ((mv_y & uMVRes) << 1));
			mv_x >>= 1;
			mv_y >>= 1;
		}
		else {
			vector_type = 0;
		}

		predyptr += (I32)mv_y * pitch;
		predyptr += (I32)mv_x * 2;

#if 0 /* old bidir code */
		if (f == (BT_FWD|BT_BKWD)) goto mc_averaged_start;
	/*	mc_process_first: */
#endif /*0 old bidir code */

#define CBIAS 0x80008000

/*	add 2 16 bit signed integers within 1 32 bit integer. */
/*	valid for 14 bit precision only */
#define ADDP(a,b) ( ( ( (U32)((a) ^ CBIAS) + (U32)((b) ^ CBIAS) ) - (U32)CBIAS ) ^ CBIAS)

/*	divide 2 16 bit signed integers by 2 within 1 32 bit integer. */
/*	valid for 14 bit precision only */
#define DIVP2(a) ( (U32)((a) & 0x80008000) | (((a) & 0xfffefffe) >> 1))

/*	divide 2 16 bit signed integers by 4 within 1 32 bit integer. */
/*	valid for 14 bit precision only */
#define DIVP4(a) ( (U32)((a) & 0xc000c000) | (((a) & 0xfffcfffc) >> 2))

		if (!(flags & BT_CODED)) goto mc_single_noncoded_start;
	/*	mc_single_coded: */
		if (vector_type == int_int) {
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					U32 t;
					t = MISLOAD32(predxptr);
					*(PU32)currxptr = ADDP(t, *(PU32)currxptr);
#else /* B_HOST_IS_BENDIAN */
					*(PU32)currxptr = ADDP(*(PU32)predxptr, *(PU32)currxptr);
#endif /* B_HOST_IS_BENDIAN */
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += pitch;
			}
		}
		else if (vector_type == int_half) {
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					t = MISLOAD32(predxptr);
					tt = MISLOAD32(predxptr+2);
					t = ADDP(t, tt);
#else /* B_HOST_IS_BENDIAN */
					t = ADDP(*(PU32)predxptr, *(PU32)(predxptr+2));
#endif /* B_HOST_IS_BENDIAN */
					*(PU32)currxptr = ADDP(DIVP2(t), *(PU32)currxptr);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += pitch;
			}
		}
		else if (vector_type == half_int) {
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					t = MISLOAD32(predxptr);
					tt = MISLOAD32(predxptr+pitch);
					t = ADDP(t, tt);
#else /* B_HOST_IS_BENDIAN */
					t = ADDP(*(PU32)predxptr, *(PU32)(predxptr+pitch));
#endif /* B_HOST_IS_BENDIAN */
					*(PU32)currxptr = ADDP(DIVP2(t), *(PU32)currxptr);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += pitch;
			}
		}
		else { /* half_half */
#if B_HOST_IS_BENDIAN
			if (predyptr & 2) {	/* predicted block is mis-aligned */
#endif /* B_HOST_IS_BENDIAN */
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					t = MISLOAD32(predxptr);
					t = ADDP(*(PU32)(predxptr+2), t);
					tt = MISLOAD32(predxptr+pitch);
					tt = ADDP(*(PU32)(predxptr+pitch+2), tt);
#else /* B_HOST_IS_BENDIAN */
					t = ADDP(*(PU32)predxptr, *(PU32)(predxptr+2));
					tt = ADDP(*(PU32)(predxptr+pitch), *(PU32)(predxptr+pitch+2));
#endif /* B_HOST_IS_BENDIAN */
					t = ADDP(t, tt);
					*(PU32)currxptr = ADDP(DIVP4(t), *(PU32)currxptr);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += pitch;
			} /* for r */
#if B_HOST_IS_BENDIAN
			}
			else {	/* predicted block is aligned */
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
					t = MISLOAD32(predxptr+2);
					t = ADDP(*(PU32)predxptr, t);
					tt = MISLOAD32(predxptr+pitch+2);
					tt = ADDP(*(PU32)(predxptr+pitch), tt);

					t = ADDP(t, tt);
					*(PU32)currxptr = ADDP(DIVP4(t), *(PU32)currxptr);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += pitch;
			} /* for r */
			}
#endif /* B_HOST_IS_BENDIAN */
		}
		goto loopstart;
	mc_single_noncoded_start:
			/*	EMPTY: copy pred to current */
		if (vector_type == int_int) {
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					t = MISLOAD32(predxptr);
					*(PU32)currxptr = t;
#else /* B_HOST_IS_BENDIAN */
					*(PU32)currxptr = *(PU32)predxptr;
#endif /* B_HOST_IS_BENDIAN */
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += currpitch;
			}
		}
		else if (vector_type == int_half) {
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					t = MISLOAD32(predxptr);
					tt = MISLOAD32(predxptr+2);
					t = ADDP(t, tt);
#else /* B_HOST_IS_BENDIAN */
					t = ADDP(*(PU32)predxptr, *(PU32)(predxptr+2));
#endif /* B_HOST_IS_BENDIAN */
					*(PU32)currxptr = DIVP2(t);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += currpitch;
			}
		}
		else if (vector_type == half_int) {
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					t = MISLOAD32(predxptr);
					tt = MISLOAD32(predxptr+pitch);
					t = ADDP(t, tt);
#else /* B_HOST_IS_BENDIAN */
					t = ADDP(*(PU32)predxptr, *(PU32)(predxptr+pitch));
#endif /* B_HOST_IS_BENDIAN */
					*(PU32)currxptr = DIVP2(t);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += currpitch;
			}
		}
		else { /* half_half */
#if B_HOST_IS_BENDIAN
			if (predyptr & 2) {	/* predicted block is mis-aligned */
#endif /* B_HOST_IS_BENDIAN */
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
#if B_HOST_IS_BENDIAN
					t = MISLOAD32(predxptr);
					t = ADDP(*(PU32)(predxptr+2), t);
					tt = MISLOAD32(predxptr+pitch);
					tt = ADDP(*(PU32)(predxptr+pitch+2), tt);
#else /* B_HOST_IS_BENDIAN */
					t = ADDP(*(PU32)predxptr, *(PU32)(predxptr+2));
					tt = ADDP(*(PU32)(predxptr+pitch), *(PU32)(predxptr+pitch+2));
#endif /* B_HOST_IS_BENDIAN */
					t = ADDP(t, tt);
					*(PU32)currxptr = DIVP4(t);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += currpitch;
			} /* for r */
#if B_HOST_IS_BENDIAN
			}
			else {	/* predicted block is aligned */
			for (r = 0; r < uBlockSize; r++) {
				predxptr = predyptr;
				currxptr = curryptr;
				for (c = 0; c < uBlockSize; c += SIMDSIZE) {
					t = MISLOAD32(predxptr+2);
					t = ADDP(*(PU32)predxptr, t);
					tt = MISLOAD32(predxptr+pitch+2);
					tt = ADDP(*(PU32)(predxptr+pitch), tt);

					t = ADDP(t, tt);
					*(PU32)currxptr = DIVP4(t);
					predxptr += SIMDSIZE*2;
					currxptr += SIMDSIZE*2;
				}
				predyptr += pitch;
				curryptr += currpitch;
			} /* for r */
			} /* else */
#endif /* B_HOST_IS_BENDIAN */
		}
		goto loopstart;

	} /* block loop */
}
