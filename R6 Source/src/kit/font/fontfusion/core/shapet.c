/*
 * SHAPET.c
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#include "syshead.h"

#include "config.h"
#include "dtypes.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "glyph.h"
#include "autogrid.h"
#include "shapet.h"
#include "util.h"

#ifdef ALGORITHMIC_STYLES
void tsi_SHAPET_BOLD_METRICS( register hmtxClass *hmtx, tsiMemObject *mem, short UPEM, F16Dot16 params[] )
{
	register int32 i, limit;
	short moveX, delta;
	F16Dot16 multiplier = params[0];
	
	UNUSED(mem);
	
	
	moveX = (short)(UPEM/12);
	moveX = (short)(util_FixMul( multiplier, moveX ) - moveX);
	delta = (short)(moveX + moveX);
	limit = hmtx->numGlyphs;

	for ( i = 0; i < limit; i++ ) {
		hmtx->aw[i] = (uint16)(hmtx->aw[i] + delta);
	}
}


/*
 * multiplier == 1.0 means do nothing
 */
void tsi_SHAPET_BOLD_GLYPH( GlyphClass *glyph, tsiMemObject *mem, short UPEM, F16Dot16 params[] )
{
	register long ctr, A, B, C, n;
	short xA, xB, xC, yA, yB, yC, orig_xB, orig_yB;
	short line1_pt1_x, line1_pt1_y;
	short line1_pt2_x, line1_pt2_y;
	short line2_pt1_x, line2_pt1_y;
	short line2_pt2_x, line2_pt2_y;
	short rot1Dx, rot1Dy;
	short rot2Dx, rot2Dy;
	short *x, *y;
	
	short clockWiseWinding = 1;
	short maxMove, moveX = 20;
	
	F16Dot16 multiplier = params[0];
	
	moveX = (short)(UPEM/12);
	moveX = (short)(util_FixMul( multiplier, moveX ) - moveX);
	maxMove = (short)(moveX * 2);
	if ( maxMove < 0 ) maxMove = (short)-maxMove;
	
	if ( glyph->contourCount > 0 && moveX != 0 ) {
		n = glyph->ep[glyph->contourCount-1] + 1;
		/* Save the original coordinates. */
		x = (short *)tsi_AllocMem( mem, (n+n) * sizeof(short) );
		y = &x[n];
		for ( A = 0; A < n; A++ ) {
			x[A] = glyph->oox[A];
			y[A] = glyph->ooy[A];
		}
		
		for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
			n = glyph->ep[ctr];
			A = n;
			xB = glyph->oox[A]; yB = glyph->ooy[A];
			B = glyph->sp[ctr];
			orig_xB = glyph->oox[B]; orig_yB = glyph->ooy[B];
			xC = orig_xB; yC = orig_yB;
			for ( ; B <= n; B++ ) {
				xA = xB; yA = yB;
				xB = xC; yB = yC;
			
				C = B + 1; 
				if ( C > n ) {
				 	/* C = glyph->sp[ctr]; */
					xC = orig_xB; yC = orig_yB; /* can't read oox[B] since it has been reset */
				} else {
					xC = glyph->oox[C]; yC = glyph->ooy[C];
				}
				
				line1_pt1_x = xA;	line1_pt1_y = yA;
				line1_pt2_x = xB;	line1_pt2_y = yB;

				line2_pt1_x = xB;	line2_pt1_y = yB;
				line2_pt2_x = xC;	line2_pt2_y = yC;
				
				/* rotate anti-clockwise 90 degrees */
				rot1Dx = (short)-(line1_pt2_y - line1_pt1_y);	/* -dy */
				rot1Dy = (short)(line1_pt2_x - line1_pt1_x);		/* +dx */
				
				rot2Dx = (short)-(line2_pt2_y - line2_pt1_y);	/* -dy */
				rot2Dy = (short)(line2_pt2_x - line2_pt1_x);		/* +dx */

#ifdef SOMEDAY
				if ( !clockWiseWinding ) {
					rot1Dx = (short)-rot1Dx;
					rot1Dy = (short)-rot1Dy;
					rot2Dx = (short)-rot2Dx;
					rot2Dy = (short)-rot2Dy;
				}

#endif
				if ( rot1Dx > 0 ) {
					line1_pt1_x = (short)(line1_pt1_x + moveX);
					line1_pt2_x = (short)(line1_pt2_x + moveX);
				} else if ( rot1Dx < 0 ) {
					line1_pt1_x = (short)(line1_pt1_x - moveX);
					line1_pt2_x = (short)(line1_pt2_x - moveX);
				}

				if ( rot2Dx > 0 ) {
					line2_pt1_x = (short)(line2_pt1_x + moveX);
					line2_pt2_x = (short)(line2_pt2_x + moveX);
				} else if ( rot2Dx < 0 ) {
					line2_pt1_x = (short)(line2_pt1_x - moveX);
					line2_pt2_x = (short)(line2_pt2_x - moveX);
				}
				
				if ( line1_pt2_x == line2_pt1_x ) {
					glyph->oox[B] = line1_pt2_x;
				} else {
					/* compute the intersection of line1 and line2 */
					util_ComputeIntersection( line1_pt1_x, line1_pt1_y, line1_pt2_x, line1_pt2_y,
								 		 line2_pt1_x, line2_pt1_y, line2_pt2_x, line2_pt2_y, &glyph->oox[B], &glyph->ooy[B] );
					{
						long dx, dy, abx, aby, dist;
						
						dx = glyph->oox[B] - xB;
						dy = glyph->ooy[B] - yB;
						
						abx = dx > 0 ? dx : -dx;
						aby = dy > 0 ? dy : -dy;
						dist = abx > aby ? abx + (aby>>1) : aby + (abx>>1);
						if ( dist > maxMove ) {
							dx = dx * maxMove / dist;
							dy = dy * maxMove / dist;
							glyph->oox[B] = (short)(xB + dx);
							glyph->ooy[B] = (short)(yB + dy);
						}
						
					}
				}
				A = B;
			}
		}
		/* Repair the outlines */
		for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
			n = glyph->ep[ctr];
			A = n;
			for ( B = glyph->sp[ctr]; B <= n; B++ ) {
				if ( (x[B] - x[A]) * (glyph->oox[B] - glyph->oox[A])  < 0 ) {
					/* sign flip ! */
					glyph->oox[B] = glyph->oox[A];
				}
				if ( (y[B] - y[A]) * (glyph->ooy[B] - glyph->ooy[A])  < 0 ) {
					/* sign flip ! */
					glyph->ooy[B] = glyph->ooy[A];
				}
				A = B;
			}
		}
		for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
			long sum, count;
			n = glyph->ep[ctr];
			A = n;
			sum = glyph->oox[A]; count = 1;
			for ( B = glyph->sp[ctr]; B <= n; B++ ) {
				if ( x[B] == x[A] ) {
					sum += glyph->oox[B]; count++;
					continue; /*****/
				}
				if ( count > 1 ) {
					sum /= count;
					glyph->oox[A] 	= (short)sum;
					glyph->oox[--B] = (short)sum;
					while( count-- > 2 ) {
						glyph->oox[--B] = (short)sum;
					}
				}
				A = B;
				sum = glyph->oox[A]; count = 1;
			}
		}

		tsi_DeAllocMem( mem,  x );
	}
}

#endif /* ALGORITHMIC_STYLES */


#ifdef SOMEDAY

SHAPETClass *tsi_NewSHAPETClass( tsiMemObject *mem )
{
	register SHAPETClass *t = (SHAPETClass *)tsi_AllocMem( mem, sizeof( SHAPETClass ) );
	t->mem = mem;
	return t; /*****/
}


void tsi_DeleteSHAPETClass( SHAPETClass *t )
{
	if ( t != NULL ) {
		/* tsi_DeAllocMem( t->mem, t->baseAddr ); */
		tsi_DeAllocMem( t->mem, (char *)t );
	}
}
#endif

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/shapet.c 1.4 1999/10/18 17:00:51 jfatal release $
 *                                                                           *
 *     $Log: shapet.c $
 *     Revision 1.4  1999/10/18 17:00:51  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:12:57  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:57:36  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

