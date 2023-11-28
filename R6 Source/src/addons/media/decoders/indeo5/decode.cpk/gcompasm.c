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
 * GCOMPASM.C -- Global Band Composition Assembly Loop
 *************************************************************************/

#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "gcompasm.h"

#define NEW_COMP1

#ifdef DEBUG
extern uDemoScale; /* from rtdec */
#endif /* DEBUG */

static U8 clip[1024] = { 0 };
static U8 clip10to8[1024] = { 0 };


void ComposeBegin(void) {
	I32 i;
	for (i = 0; i < 1024; i++) {
		if (i & 0x200)			clip[i] = 0 ;
		else if (i & 0x100)		clip[i] = 255;
		else					clip[i] = (U8)i;
	}

/*
	map [-512,-129] to 0
	map [-128,127] = 0x080,0x07f to [0,255]
	map [128,511] to 255
	-512	0x200
	-129	0x37f
	-128	0x380
	127		0x07f
	128		0x080
	511 	0x1ff
*/
	for (i = -512; i <= -129; i++) clip10to8[i&0x3ff] = 0;
	for (i = -128; i <= 127; i++) clip10to8[i&0x3ff] = (U8)i+128;
	for (i = 128; i < 511; i++) clip10to8[i] = 255;
} /* ComposeBegin */

static void
GlobalComp1_Clamp_C(PU8 BandData, U32 BandPitch, PU8 YPlane, U32 YPitch,
	U32 xres, U32 yres) {

	U32 r, c;
	PU8 irptr; PI16 icptr;
	PU8	orptr, ocptr;

	irptr = BandData;
	orptr = YPlane;
	for (r = 0; r < yres; r++) {
		icptr = (PI16)irptr;
		ocptr = orptr;
		for (c = 0; c < xres; c++) {
			*ocptr++ = clip10to8[(*icptr++)&0x3ff];
		}
		orptr += YPitch;
		irptr += BandPitch;
	}
} /* GlobalComp1_Clamp_C */

#define B4 0x4000
static void
GlobalComp4_8x8Clamp_C(PU8 BandData, U32 BandPitch, U32 BandOffset,
	PU8 YPlane, U32 YPitch, U32 xres, U32 yres, U32 fDoBands);
static void
GlobalComp4_8x8Clamp_C(PU8 BandData, U32 BandPitch, U32 BandOffset,
	PU8 YPlane, U32 YPitch, U32 xres, U32 yres, U32 fDoBands) {

	U32 r,c;
	PU8	yrowptr, ycolptr0, ycolptr1;
	PU8 browptr, bcolptr;

	I32 prevrowhalf, nextrowhalf;
	I32 prevrowwhole, nextrowwhole;

	I16 B0e,B0f,B0h,B0i;
	I16 B1b,B1c,B1e,B1f,B1h,B1i;
	I16 B2d,B2e,B2f,B2g,B2h,B2i;
	I16 B3a,B3b,B3c,B3d,B3e,B3f,B3g,B3h,B3i;

	I16 Zero;

	Zero = 0;
	
	browptr = BandData;
	yrowptr = YPlane;

	prevrowhalf = 0;
	nextrowhalf = nextrowwhole = prevrowwhole = BandPitch;

	B0e = B0f = B0h = B0i = Zero;
	B1b = B1c = B1e = B1f = B1h = B1i = Zero;
	B2d = B2e = B2f = B2g = B2h = B2i = Zero;
	B3a = B3b = B3c = B3d = B3e = B3f = B3g = B3h = B3i = Zero;

	for (r = 0; r < yres; r+=2) {

		I32 A,B,C,D;

		bcolptr = browptr;
		ycolptr0 = yrowptr;
		ycolptr1 = yrowptr + YPitch;

		if (r + 2 >= yres) {
			nextrowhalf = 0;
			nextrowwhole = 0-BandPitch;		/* maybe move this out of loop */
		}

	/*	a b c */
	/*	d(e)f */
	/*	g h i */

		if (fDoBands >= 1) {
			B0f = *((PI16)bcolptr);
			B0i = *((PI16)(bcolptr+nextrowhalf));
		}

		bcolptr += BandOffset;
		if (fDoBands >= 2) {
			B1c = *((PI16)(bcolptr+prevrowhalf));
			B1f = *((PI16)bcolptr);
			B1i = *((PI16)(bcolptr+nextrowwhole));
		}

		bcolptr += BandOffset;
		if (fDoBands >= 3) {
			B2f = *((PI16)bcolptr);
			B2i = *((PI16)(bcolptr+nextrowhalf));

			B2e = B2f;
			B2h = B2i;
		}

		bcolptr += BandOffset;
		if (fDoBands >= 4) {
			B3c = *((PI16)(bcolptr+prevrowhalf));
			B3f = *((PI16)bcolptr);
			B3i = *((PI16)(bcolptr+nextrowwhole));

			B3b = B3c;
			B3e = B3f;
			B3h = B3i;
		}


		bcolptr += 2 - 3*BandOffset;
		for (c = 2; c < xres; c+=2) {
			PU8 bcoloffset;

			bcoloffset = bcolptr;

		/*	a b c */
		/*	d(e)f */
		/*	g h i */

		/*	Shift future values to current */
			B0e = B0f;
			B0h = B0i;

			B1b = B1c;
			B1e = B1f;
			B1h = B1i;

			B2d = B2e;
			B2e = B2f;
			B2g = B2h;
			B2h = B2i;

			B3a = B3b;
			B3b = B3c;
			B3d = B3e;
			B3e = B3f;
			B3g = B3h;
			B3h = B3i;

		/*	a b c */
		/*	d(e)f */
		/*	g h i */

			A = B = C = D = 0;

			if (fDoBands >= 1) {
				B0f = *((PI16)bcoloffset);
				B0i = *((PI16)(bcoloffset+nextrowhalf));

				A += B0e<<4;
				B += (B0e+B0f)<<3;

				C += (B0e+B0h)<<3;
				D += (B0e+B0f+B0h+B0i)<<2;
			}

			bcoloffset += BandOffset;
			if (fDoBands >= 2) {
				B1c = *((PI16)(bcoloffset+prevrowhalf));
				B1f = *((PI16)bcoloffset);
				B1i = *((PI16)(bcoloffset+nextrowwhole));

				A += (B1b+B1e)<<3;
				B += (B1b+B1e+B1c+B1f)<<2;
				C += (B1b-6*B1e+B1h)<<2;
				D += (B1b-6*B1e+B1h+B1c-6*B1f+B1i)<<1;
			}

			bcoloffset += BandOffset;
			if (fDoBands >= 3) {
				B2f = *((PI16)bcoloffset);
				B2i = *((PI16)(bcoloffset+nextrowhalf));

				A += (B2d+B2e)<<3;
				B += (B2d-6*B2e+B2f)<<2;
				C += (B2d+B2e+B2g+B2h)<<2;
				D += (B2d-6*B2e+B2f+B2g-6*B2h+B2i)<<1;
			}

			bcoloffset += BandOffset;
			if (fDoBands >= 4) {
				B3c = *((PI16)(bcoloffset+prevrowhalf));
				B3f = *((PI16)bcoloffset);
				B3i = *((PI16)(bcoloffset+nextrowwhole));

				A += (B3a+B3b+B3d+B3e)<<2;
				B += (B3a-6*B3b+B3c+B3d-6*B3e+B3f)<<1;
				C += (B3a-6*B3d+B3g+B3b-6*B3e+B3h)<<1;
				D += (B3a-6*B3b+B3c-6*B3d+36*B3e-6*B3f+B3g-6*B3h+B3i);
			}

		/*	a b c */
		/*	d(e)f */
		/*	g h i */

/*			packsswb AB0, AB1 */
/*			packsswb CD0, CD1 */

			*ycolptr0++ = clip[(128+(A>>6))&0x3ff];
			*ycolptr0++ = clip[(128+(B>>6))&0x3ff];
/*			movq [ycolptr], AB0 */

			*ycolptr1++ = clip[(128+(C>>6))&0x3ff];
			*ycolptr1++ = clip[(128+(D>>6))&0x3ff];
/*			movq [ycolptr+ypitch], CD0 */

/*			add ycolptr, 8 */
			bcolptr += 2;
/*			add bcolptr, ? */
		}

	/*	Shift future values to current, with wrap */

		B0e = B0f;
		B0h = B0i;

		B1b = B1c;
		B1e = B1f;
		B1h = B1i;

		B2d = B2e;
		B2e = B2f;
			B2f = B2d;
		B2g = B2h;
		B2h = B2i;
			B2i = B2g;

		B3a = B3b;
		B3b = B3c;
			B3c = B3a;
		B3d = B3e;
		B3e = B3f;
			B3f = B3d;
		B3g = B3h;
		B3h = B3i;
			B3i = B3g;

	/*	Now finish last 2 elements */
		A = B0e<<4;
		B = (B0e+B0f)<<3;
		C = (B0e+B0h)<<3;
		D = (B0e+B0f+B0h+B0i)<<2;

		A += (B1b+B1e)<<3;
		B += (B1b+B1e+B1c+B1f)<<2;
		C += (B1b-6*B1e+B1h)<<2;
		D += (B1b-6*B1e+B1h+B1c-6*B1f+B1i)<<1;

		A += (B2d+B2e)<<3;
		B += (B2d-6*B2e+B2f)<<2;
		C += (B2d+B2e+B2g+B2h)<<2;
		D += (B2d-6*B2e+B2f+B2g-6*B2h+B2i)<<1;

		A += (B3a+B3b+B3d+B3e)<<2;
		B += (B3a-6*B3b+B3c+B3d-6*B3e+B3f)<<1;
		C += (B3a-6*B3d+B3g+B3b-6*B3e+B3h)<<1;
		D += (B3a-6*B3b+B3c-6*B3d+36*B3e-6*B3f+B3g-6*B3h+B3i);

		*ycolptr0++ = clip[(128+(A>>6))&0x3ff];
		*ycolptr0++ = clip[(128+(B>>6))&0x3ff];
		*ycolptr1++ = clip[(128+(C>>6))&0x3ff];
		*ycolptr1++ = clip[(128+(D>>6))&0x3ff];

	/*	Update pointers */

		prevrowhalf = prevrowwhole = 0-BandPitch;	/* maybe move this out of loop */
		browptr += BandPitch;
		yrowptr += 2*YPitch;
	}
}	/* GlobalComp4_8x8Clamp_C */
#undef B4 /*0x4000 */


void Compose (pRTDecInst pCntx, RectSt rComposeRect, U32 fDoBands) {
	I32 plane;
	PU8 base;

	if (pCntx->uValidDisplay) {
	for (plane = 2; plane > -1; plane--) {
		pRtPlaneSt	p;
		pRtBandSt	b;
		U32			xres, yres;
		Boo 		bAllBandsDecoded;
		RectSt		r;
		PU8			pOutput;
		U32			uBMask, uPMask;	/* Band & Plane masks */
		U32			BandOffset;

		p = &pCntx->Plane[plane];
		b = &p->pBand[0];

		bAllBandsDecoded = TRUE;

		for (xres = 0; xres < p->uNBands; xres++) {
			if (p->pBand[xres].uBandFlags & (BF_DROPPED|BF_REF_DROPPED)) {
				bAllBandsDecoded = FALSE;
				break;
			} /* if flags */
		} /* for xres */

	/*	New Optimized composition */
		r = rComposeRect;
		if (plane) {
	/*	compute rect of subsampled output plane, assume YVU9 */
			r.h = (r.r+r.h+3) / 4;
			r.w = (r.c+r.w+3) / 4;
			r.r /= 4;
			r.c /= 4;
			r.h -= r.r;
			r.w -= r.c;
		}

	/*	 */
		if (b->uBlockSize == 4) {
		/*	4x4 blocks: need to align input on a 4x4 DIB boundary
			(0 mod 16), which corresponds to aligning output on
			a 0 mod 8 boundary
		 */
			uPMask = ~0xfU;
			uBMask = ~0x1fU;
		}
		else {
		/*	8x8 blocks: need to align input on a 8x8 DIB boundary
			(0 mod 32),	which corresponds to aligning output on
			a 0 mod 16 boundary
		 */
			uPMask = ~0xfU;
			uBMask = ~0x1fU;
		}
		/*	If the plane is banded, the 1/4 image size band data
			combined with the word storage make the plane width
			& band width the same in bytes.  Thus, the plane mask
			needs to be the more restrictive to reflect this.
		 */
		if (p->uNBands == 4) {
			uPMask <<= 1;
			r.h += r.r & 1;
			r.r &= ~1;
		}

	/*	Compute offset into plane, taking into account that */
	/*	horizontal offset must start on a block boundary */
		pOutput = p->pOutput + r.r * p->uOutputPitch + (r.c&uPMask);
		xres = (r.c+r.w+(~uPMask)-(r.c&uPMask))&uPMask;
		yres = r.h;

		if (p->uNBands == 4) {
		/*	adjust for word input & byte output */
			r.r /= 2;
		} /* p->uNBands == 4 */
		else {
		/*	adjust for word input & byte output */
			r.c *= 2;
		}

	/*	Comput offset into bands, starting on a block boundary */
		base = pCntx->pFrameDisplay;
		base += p->uPlaneOffset + b->uBandOffset + b->pTile[0].uTileOffset;

		base += r.c&uBMask;
		base += r.r*b->uPitch;


		BandOffset = b->uPitch * ((b->tBandSize.r+0xf) & ~0xf) + BANDDELTA;

		if (p->uNBands == 4) {
			GlobalComp4_8x8Clamp_C(base, b->uPitch, BandOffset,
				pOutput, p->uOutputPitch, xres, yres, fDoBands);
		}
		else if (p->uNBands == 1) {
			GlobalComp1_Clamp_C(base, b->uPitch,
				pOutput, p->uOutputPitch, xres, yres);
		}

	} /* for plane */
	} /* if valid display */

} /* Compose */

