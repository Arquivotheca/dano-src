/*
 * T2KSTRK1.c
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

#ifdef ENABLE_T2KS

#include "dtypes.h"
#include "tsimem.h"
#include "t2kstrm.h"
#include "glyph.h"
#include "truetype.h"
#include "t2kstrk1.h"
#include "util.h"

#define MITER_JOIN 0
#define ROUND_JOIN 1
#define BEVEL_JOIN 2

#define BUTT_CAP				0
#define SQUARE_50PERCENT_CAP	1
#define ROUND_CAP				2
#define SQUARE_CAP				3


typedef struct {
	/* private */
	tsiMemObject *mem;
	
	long leftCount, leftMaxCount;
	short *xyLeft;
	long rightCount, rightMaxCount;
	short *xyRight;
	/* rotated vector cache */
	int rot1Dx, rot1Dy, rot1IsForPoint;
	/* public */
} PathClass;

/*
 * Creates a new PathClass.
 */
static PathClass *New_PathClass( tsiMemObject *mem )
{
	PathClass *t;
	
	t  = (PathClass*) tsi_AllocMem( mem, sizeof( PathClass ) );
	
	t->mem				= mem;
	t->leftCount 		= 0;
	t->rightCount		= 0;
	
	t->leftMaxCount	 	= 256;
	t->rightMaxCount	= 256;
	
	t->xyLeft 	= (short *)tsi_AllocMem( mem, sizeof(short) * t->leftMaxCount );
	t->xyRight 	= (short *)tsi_AllocMem( mem, sizeof(short) * t->rightMaxCount );
	
	t->rot1IsForPoint = -1;
	return t; /*****/
}

/*
 * Deletes a PathClass.
 */
static void Delete_PathClass( PathClass *t )
{
	tsi_DeAllocMem( t->mem, t->xyLeft );
	tsi_DeAllocMem( t->mem, t->xyRight );
	tsi_DeAllocMem( t->mem, t );
}

typedef void (*PF_ADD_POINT) ( PathClass *t, short x, short y, short onOff );


/*
 * Adds a point on the left side.
 */
static void AddLeftPoint( register PathClass *t, short x, short y, short onOff )
{
	register long count = t->leftCount;
	register short *xyLeft;
	if ( (t->leftCount = count + 3) > t->leftMaxCount ) {
		t->leftMaxCount = count + (count>>1) + 16;
		t->xyLeft 	= (short *)tsi_ReAllocMem( t->mem, t->xyLeft, sizeof(short) * t->leftMaxCount );
	}
	xyLeft = &t->xyLeft[count];
	xyLeft[ 0 ] = x;
	xyLeft[ 1 ] = y;
	xyLeft[ 2 ] = onOff;
}

/*
 * Adds two points on the left side.
 */
static void Add2LeftPoints( register PathClass *t, short x1, short y1, short onOff1, short x2, short y2, short onOff2 )
{
	register long count = t->leftCount;
	register short *xyLeft;
	if ( (t->leftCount = count + 6) > t->leftMaxCount ) {
		t->leftMaxCount = count + (count>>1) + 16;
		t->xyLeft 	= (short *)tsi_ReAllocMem( t->mem, t->xyLeft, sizeof(short) * t->leftMaxCount );
	}
	xyLeft = &t->xyLeft[count];
	xyLeft[ 0 ] = x1;
	xyLeft[ 1 ] = y1;
	xyLeft[ 2 ] = onOff1;
	xyLeft[ 3 ] = x2;
	xyLeft[ 4 ] = y2;
	xyLeft[ 5 ] = onOff2;
}


/*
 * Adds a point on the right side.
 */
static void AddRightPoint( PathClass *t, short x, short y, short onOff )
{
	long count = t->rightCount;
	register short *xyRight;
	if ( ( t->rightCount = count + 3) > t->rightMaxCount ) {
		t->rightMaxCount = count + (count>>1) + 16;
		t->xyRight 	= (short *)tsi_ReAllocMem( t->mem, t->xyRight, sizeof(short) * t->rightMaxCount );
	}
	xyRight = &t->xyRight[count];
	xyRight[ 0 ] = x;
	xyRight[ 1 ] = y;
	xyRight[ 2 ] = onOff;
}


/*
 * Computes sqrt( dx*dx + dy*dy)
 */
static int Distance( int dx, int dy )
{
	long root, square;
	
	if ( dy == 0 ) {
		if ( dx < 0 ) dx = -dx;
		root = dx;
	} else if ( dx == 0 ) {
		if ( dy < 0 ) dy = -dy;
		root = dy;
	} else {
		if ( dx < 0 ) dx = -dx;
		if ( dy < 0 ) dy = -dy;
		
		root = dx > dy ? dx + (dy>>1) : dy + (dx>>1);

		assert( dx >= 0 && dx < 32768 );
		assert( dy >= 0 && dy < 32768 );
		/* Do a few Newton Raphson iterations.*/
		square = (long)dx*dx + (long)dy*dy;
		root = (( root + square/root) + 1 ) >> 1;
		root = (( root + square/root) + 1 ) >> 1; /* close enough! */
	}
	return root; /*****/
}

/*
 * Computes an approximation to sqrt( dx*dx + dy*dy) quickly.
 */
static int ApproxDistance( int dx, int dy)
{
	if (dy<0) dy = -dy;
	if (dx<0) dx = -dx;
	return (dx > dy ? dx + (dy>>1) : dy + (dx>>1)) ; /*****/
}


/*
 * Computes the instersection of line 1 and line 2 in *x, *y.
 *
 *
 * (1) x2 + dx2 * t2 = x1 + dx1 * t1
 * (2) y2 + dy2 * t2 = y1 + dy1 * t1
 *
 *  1  =>  t1 = ( x2 - x1 + dx2 * t2 ) / dx1
 *  +2 =>  y2 + dy2 * t2 = y1 + dy1/dx1 * [ x2 - x1 + dx2 * t2 ]
 *
 *     => t2 * [dy1/dx1 * dx2 - dy2] = y2 - y1 - dy1/dx1*(x2-x1)
 *     => t2(dy1*dx2 - dy2*dx1) = dx1(y2 - y1) + dy1(x1-x2)
 *     => t2 = [dx1(y2-y1) + dy1(x1-x2)] / [dy1*dx2 - dy2*dx1]
 *     => t2 = [dx1(y2-y1) - dy1(x2-x1)] / [dx2*dy1 - dy2*dx1]
 *     t2 = Num/Denom
 *     =>
 *	    Num   = (y2 - y1) * dx1 - (x2 - x1) * dy1;
 *		Denom = dx2 * dy1 - dy2 * dx1;
 *
 */
static int IsIntersection( short line1_pt1_x, short line1_pt1_y, short line1_pt2_x, short line1_pt2_y,
								 short line2_pt1_x, short line2_pt1_y, short line2_pt2_x, short line2_pt2_y,
								 short *x, short *y )
{
 	long dx1, dy1, dx2, dy2, x1, y1, x2, y2;
	long Num,Denom, t1, t2;
	int result = false;
	
	
	dx1 = line1_pt2_x - line1_pt1_x;	dy1 = line1_pt2_y - line1_pt1_y;
	dx2 = line2_pt2_x - line2_pt1_x;	dy2 = line2_pt2_y - line2_pt1_y;
 
 	x1 = line1_pt1_x; y1 = line1_pt1_y;
 	x2 = line2_pt1_x; y2 = line2_pt1_y;

	Num   = (y2 - y1) * dx1 - (x2 - x1) * dy1;
	Denom = dx2 * dy1 - dy2 * dx1;

	if ( Denom != 0  ) {
		t1 = util_FixDiv( Num, Denom );
	    /* Now reverse the lines. */
	    if ( t1 >= 0 && t1 <= ONE16Dot16 ) {
			Num   = (y1 - y2) * dx2 - (x1 - x2) * dy2;
			Denom = dx1 * dy2 - dy1 * dx2;
			if ( Denom != 0   ) {
				t2 = util_FixDiv( Num, Denom );
				if (  t2 >= 0 && t2 <= ONE16Dot16 ) {
					result = true;
				}
			}
		}
		*x = (short)( x2 + ((util_FixMul( dx2<<2, t1 )+2)>>2) );
		*y = (short)( y2 + ((util_FixMul( dy2<<2, t1 )+2)>>2) );
	} else {
		*x = (short)((line1_pt2_x+line2_pt1_x)/2);
		*y = (short)((line1_pt2_y+line2_pt1_y)/2);
	}
	return result; /*****/
}
		
		
#define SafeFixDiv(a,b)  ((b) != 0 ? util_FixDiv( (a), (b)) : ONE16Dot16 )

/*
 *           p2
 *          *   *
 *         *       *
 *        *           *
 *       *               *
 *      *                   *
 *     *                       p3
 *    *
 *   *
 *  p1
 *
 * Joins the lines p1-p2 and p2-p3 while stroking the path p1,p2,p3.
 */
static void lineJoin( PathClass *t, int joinType, int x1, int y1, int x2, int y2, int onOff2, int pointNum2, int x3, int y3, int r  )
{
	int rot1Dx, rot1Dy, rot2Dx, rot2Dy;
	F16Dot16 length, mul;
	short xLeft, yLeft, xRight, yRight;
	short xOutside, yOutside;
	long area;
	int miterLimit = r + r;
	PF_ADD_POINT AddOutsidePoint;
	int iLeft, iRight;
	int /* shallow, */posArea, maxArea;
	long direction;
	int d1 = ApproxDistance( x2-x1, y2-y1 );
	int d2 = ApproxDistance( x3-x2, y3-y2 );
	
	/* Computes the area of the parallelogram made up of (p2-p1) * (p3-p2) */
	area = (long)(x2-x1)*(y3-y2) - (long)(y2-y1)*(x3-x2); /* area > 0 => left turn, otherwise right turn */
	posArea = area;
	if ( posArea< 0 ) posArea = -posArea;
	maxArea = d1*d2;
	
	/* shallow = posArea < (maxArea>>2); */
	direction = ((long)(x2-x1)*(x3-x2) + (long)(y2-y1)*(y3-y2));
	
	if ( ( posArea < (maxArea>>3) && direction > 0 ) || ( !onOff2  && posArea < (maxArea>>2) && direction < 0 ) ) {
		rot1Dx = -(y3 - y1);	/* -dy */
		rot1Dy =   x3 - x1;		/* +dx */
		
		length = Distance( rot1Dx+rot1Dx, rot1Dy+rot1Dy );
		mul = SafeFixDiv( r+r, length);
		rot1Dx = util_FixMul( rot1Dx, mul );
		rot1Dy = util_FixMul( rot1Dy, mul );

		length = Distance( rot1Dx+rot1Dx, rot1Dy+rot1Dy );
		mul = SafeFixDiv( r+r, length);
		rot1Dx = util_FixMul( rot1Dx, mul );
		rot1Dy = util_FixMul( rot1Dy, mul );
		
		AddLeftPoint(  t, (short)(x2+rot1Dx), (short)(y2+rot1Dy), (short)onOff2 );
		AddRightPoint( t, (short)(x2-rot1Dx), (short)(y2-rot1Dy), (short)onOff2 );
		return; /*****/
	}

	/* A turn */
	/* Normalize to length r */
	/* Rotate anti-clockwise 90 degrees. */
	if ( pointNum2 == t->rot1IsForPoint ) {
		rot1Dx = t->rot1Dx;
		rot1Dy = t->rot1Dy;
	} else {
		rot1Dx = -(y2 - y1);	/* -dy */
		rot1Dy =   x2 - x1;		/* +dx */

		length = Distance( rot1Dx+rot1Dx, rot1Dy+rot1Dy );
		mul = SafeFixDiv( r+r, length);
		rot1Dx = util_FixMul( rot1Dx, mul );
		rot1Dy = util_FixMul( rot1Dy, mul );

		length = Distance( rot1Dx+rot1Dx, rot1Dy+rot1Dy );
		mul = SafeFixDiv( r+r, length);
		rot1Dx = util_FixMul( rot1Dx, mul );
		rot1Dy = util_FixMul( rot1Dy, mul );
	}
	
	/* Rotate anti-clockwise 90 degrees. */
	rot2Dx = -(y3 - y2);	/* -dy */
	rot2Dy =   x3 - x2;		/* +dx */
	length = Distance( rot2Dx+rot2Dx, rot2Dy+rot2Dy );

	mul = SafeFixDiv( r+r, length);
	rot2Dx = util_FixMul( rot2Dx, mul );
	rot2Dy = util_FixMul( rot2Dy, mul );
	length = Distance( rot2Dx+rot2Dx, rot2Dy+rot2Dy );
	mul = SafeFixDiv( r+r, length);
	rot2Dx = util_FixMul( rot2Dx, mul );
	rot2Dy = util_FixMul( rot2Dy, mul );

	/* Cache these values for increased speed */
	t->rot1Dx = rot2Dx; t->rot1Dy = rot2Dy;
	t->rot1IsForPoint = pointNum2+1;

	iLeft  = IsIntersection( (short)(x1+rot1Dx), (short)(y1+rot1Dy), (short)(x2+rot1Dx), (short)(y2+rot1Dy),
							  (short)(x2+rot2Dx), (short)(y2+rot2Dy), (short)(x3+rot2Dx), (short)(y3+rot2Dy), &xLeft,  &yLeft );
	iRight = IsIntersection( (short)(x1-rot1Dx), (short)(y1-rot1Dy), (short)(x2-rot1Dx), (short)(y2-rot1Dy),
							  (short)(x2-rot2Dx), (short)(y2-rot2Dy), (short)(x3-rot2Dx), (short)(y3-rot2Dy), &xRight, &yRight );

	{

		if ( area > 0 ) {
			/* A left turn. */
			xOutside = xRight;
			yOutside = yRight;
			rot1Dx = -rot1Dx;
			rot1Dy = -rot1Dy;
			rot2Dx = -rot2Dx;
			rot2Dy = -rot2Dy;
			/* Add the inside point */
			if ( iLeft || !onOff2 ) {
				AddLeftPoint( t, xLeft, yLeft, (short)onOff2 );
			} else {
				/* Oh, well do an inside bevel joint. */
				Add2LeftPoints( t, (short)(x2-rot1Dx), (short)(y2-rot1Dy), (short)onOff2,
								   (short)(x2-rot2Dx), (short)(y2-rot2Dy), (short)onOff2  );
			}
			AddOutsidePoint = AddRightPoint;
		} else {
			/* A right turn. */
			xOutside = xLeft;
			yOutside = yLeft;
			/* Add the inside point */
			/* AddRightPoint( t, xRight, yRight, (short)onOff2 ); */
			if ( iRight || !onOff2 ) {
				AddRightPoint( t, xRight, yRight, (short)onOff2 );
			} else {
				/* Oh, well do an inside bevel joint. */
				AddRightPoint( t, (short)(x2-rot1Dx), (short)(y2-rot1Dy), (short)onOff2  );
				AddRightPoint( t, (short)(x2-rot2Dx), (short)(y2-rot2Dy), (short)onOff2  );
			}
			AddOutsidePoint = AddLeftPoint;
		}
								  
		if ( joinType == MITER_JOIN || onOff2 == 0 ) {
			/* if within the miter limit and the angle is not too shallow */
			if ( Distance( xOutside - x2, yOutside - y2 ) <= miterLimit || onOff2 == 0 ) {
				AddOutsidePoint( t, xOutside, yOutside, (short)onOff2);
			} else {
				/* Revert to a Bevel Joint */
				assert( onOff2 != 0 );
				AddOutsidePoint( t, (short)(x2+rot1Dx), (short)(y2+rot1Dy), 1 );
				AddOutsidePoint( t, (short)(x2+rot2Dx), (short)(y2+rot2Dy), 1 );
			}
		} else if ( joinType == ROUND_JOIN ) {
			short xRot, yRot, p0x, p0y, p1x, p1y, p2x, p2y, dx, dy;
			length = Distance( dx = (short)(xOutside - x2), dy = (short)(yOutside - y2 ));
			mul = SafeFixDiv( r, length);
			dx = (short)util_FixMul( dx, mul );
			dy = (short)util_FixMul( dy, mul );
			p0x  = (short)(x2 + dx);
			p0y  = (short)(y2 + dy);
			xRot = (short)-dy;
			yRot =  dx;
			util_ComputeIntersection( (short)(x1+rot1Dx), (short)(y1+rot1Dy), (short)(x2+rot1Dx), (short)(y2+rot1Dy),
									  (short)(p0x), (short)(p0y), (short)(p0x+xRot), (short)(p0y+yRot), &p1x, &p1y );
			util_ComputeIntersection( (short)(x2+rot2Dx), (short)(y2+rot2Dy), (short)(x3+rot2Dx), (short)(y3+rot2Dy),
									  (short)(p0x), (short)(p0y), (short)(p0x+xRot), (short)(p0y+yRot), &p2x, &p2y );
			AddOutsidePoint( t, (short)(x2+rot1Dx), (short)(y2+rot1Dy), 1 );
			/* OLD: AddOutsidePoint( t, xOutside, yOutside, 0 ); */
			AddOutsidePoint( t, p1x, p1y, 0 );
			AddOutsidePoint( t, p2x, p2y, 0 );
			AddOutsidePoint( t, (short)(x2+rot2Dx), (short)(y2+rot2Dy), 1 );
		} else if ( joinType == BEVEL_JOIN ) {
			AddOutsidePoint( t, (short)(x2+rot1Dx), (short)(y2+rot1Dy), 1 );
			AddOutsidePoint( t, (short)(x2+rot2Dx), (short)(y2+rot2Dy), 1 );
		} else {
			;
			assert(false );
		}
	}		
}



/*
 *
 *
 *          ***********************
 *                                  *
 *                                   *
 *                                    *
 *                                X    *
 *                                    *
 *                                   *
 *                                  *
 *          ***********************
 *                                ^ [x,y]
 *                               
 *                         
 *                                *-----> [dx,dy]  
 *
 * Draws an endcap.
 *
 */
static void EndCap( PathClass *t, int capType, int x, int y, int dx, int dy, int r )
{
	int xr, yr, rot1Dx, rot1Dy;
	F16Dot16 length, mul;

	/* Normalize to length r */
	length = Distance( dx+dx, dy+dy );
	mul = SafeFixDiv( r+r, length);
	dx = util_FixMul( dx, mul );
	dy = util_FixMul( dy, mul );
	length = Distance( dx+dx, dy+dy );
	mul = SafeFixDiv( r+r, length);
	dx = util_FixMul( dx, mul );
	dy = util_FixMul( dy, mul );


	xr = x + dx;
	yr = y + dy;
	/* Rotate anti-clockwise 90 degrees. */
	rot1Dx = -dy;	/* -dy */
	rot1Dy =  dx;   /* +dx */
	
	
	if ( capType == SQUARE_CAP ) {
		Add2LeftPoints( t, (short)(xr + rot1Dx), (short)(yr + rot1Dy), 1,
						   (short)(xr - rot1Dx), (short)(yr - rot1Dy), 1 );
	} else if ( capType == BUTT_CAP ) {
		Add2LeftPoints( t, (short)(x  + rot1Dx), (short)(y  + rot1Dy), 1,
		    			   (short)(x  - rot1Dx), (short)(y  - rot1Dy), 1 );
	} else if ( capType == SQUARE_50PERCENT_CAP ) {
		xr = x + ((dx+1)>>1);
		yr = y + ((dy+1)>>1);
		Add2LeftPoints( t, (short)(xr  + rot1Dx), (short)(yr  + rot1Dy), 1,
						   (short)(xr  - rot1Dx), (short)(yr  - rot1Dy), 1 );
	} else if ( capType == ROUND_CAP ) {
		Add2LeftPoints( t, (short)(x  + rot1Dx), (short)(y  + rot1Dy), 1,
						   (short)(xr + rot1Dx), (short)(yr + rot1Dy), 0 );
		Add2LeftPoints( t, (short)(xr - rot1Dx), (short)(yr - rot1Dy), 0,
						   (short)(x  - rot1Dx), (short)(y  - rot1Dy), 1 );
	}
}


/*
 * NOTE: Could be moved into GLYPH.c and also be used by glyph_AddPoint()
 */
static void glyph_EnsureSpaceForNPoints( GlyphClass *glyph, int N )
{
if ( N  > glyph->pointCountMax ) {
	register int limit, i;
	register short *oox, *ooy;
	register uint8 *onCurve;
	register short *ooxOld, *ooyOld;
	register uint8 *onCurveOld;
	F26Dot6 *memBase;
	/* ReAllocate */
	memBase = glyph->x; ooxOld = glyph->oox; ooyOld = glyph->ooy; onCurveOld = glyph->onCurve;
	limit = N + SbPtCount;
	AllocGlyphPointMemory( glyph, limit );
	oox = glyph->oox; ooy = glyph->ooy; onCurve = glyph->onCurve;

	limit = glyph->pointCount + SbPtCount; /* Changed 10/12/99 Rob & Sampo */
	for ( i = 0; i < limit; i++ ) {
		oox[i]          = ooxOld[i];
		ooy[i]          = ooyOld[i];
		onCurve[i]      = onCurveOld[i];
		}
	tsi_FastDeAllocN( glyph->mem, memBase, T2K_FB_POINTS); /* contains all the old point data */
	}
}

/*
 * Computes the Manhattan distance between two points on the left and right side of the path.
 */
static long myDist( PathClass *path, long leftIndex, long rightIndex )
{
	long dx = path->xyLeft[leftIndex]   - path->xyRight[rightIndex];
	long dy = path->xyLeft[leftIndex+1] - path->xyRight[rightIndex+1];
	if ( dx < 0 ) dx = -dx;
	if ( dy < 0 ) dy = -dy;
	
	return dx + dy; /*****/
}

/*
 * Returns true if a heuristic determines that a closed path has overextended itself through its
 * "focal point" or "center of gravity", thereby becoming 180 degrees out of phase.
 */
static int IsOverExtended( PathClass *path )
{
	long dist_0   = 0;
	long dist_180 = 0;
	long leftIndex, rightIndex, rightMidIndex;
	long leftCountDiv2 = path->leftCount >> 1;
	
	rightMidIndex = (path->rightCount + 1 ) / 2;
	rightMidIndex = (rightMidIndex+rightMidIndex+3)/6;
	rightMidIndex = rightMidIndex + rightMidIndex + rightMidIndex;
	
	for ( leftIndex = 0; leftIndex < path->leftCount; leftIndex += 3) {
		/* Map the leftIndex to a rightIndex which is a multiple of 3. */
		rightIndex = (leftIndex * path->rightCount + leftCountDiv2) / path->leftCount;
		rightIndex = (rightIndex+rightIndex+3)/6;
		rightIndex = rightIndex + rightIndex + rightIndex;
		if ( rightIndex >= path->rightCount ) {
			rightIndex = path->rightCount - 3;
		}
		/* Compute the zero phase "in sync" distance */
		dist_0 += myDist( path, leftIndex, rightIndex );
		rightIndex = rightIndex + rightMidIndex;
		if ( rightIndex >= path->rightCount ) {
			rightIndex = rightIndex - path->rightCount;
		}
		/* Compute the 180 degree phase "out of sync" distance */
		dist_180 += myDist( path, leftIndex, rightIndex );
	}
	return dist_180 <= dist_0; /*****/
}

/*
 * Merges a path into a GlyphClass.
 */
static void glyph_MergePathIntoGlyph( GlyphClass *glyph, PathClass *path, int open )
{
	int i, limit, point, count/* , oldPoinCount*/;
	register short *oox, *ooy;
	register uint8 *onCurve;
	int ctr = glyph->contourCount++;

   	/* oox = glyph->oox; ooy = glyph->ooy; onCurve = glyph->onCurve;*/
	/* point = glyph->sp[ctr]; */
	/*count = glyph->ep[ctr] - point + 1;*/
	/*oldPoinCount = glyph->pointCount;*/
	glyph->sp[ctr] = glyph->pointCount;

	
	/* Set count to the number of points in the stroked path */
	count = (path->leftCount + path->rightCount)/3;

	glyph_EnsureSpaceForNPoints( glyph, glyph->pointCount + count );
	oox = glyph->oox; ooy = glyph->ooy; onCurve = glyph->onCurve;

	limit = path->leftCount;
	point = glyph->sp[ctr];
	for ( i = 0; i < limit; ) {
		oox[point]		= path->xyLeft[i++];
		ooy[point]		= path->xyLeft[i++];
		onCurve[point]	= (uint8)path->xyLeft[i++];
		point++;
	}
	assert( i == limit );
	if ( open ) {
		limit = path->rightCount;
		for ( i = limit-1; i >= 0; ) {
			onCurve[point]	= (uint8)path->xyRight[i--];
			ooy[point]		= path->xyRight[i--];
			oox[point]		= path->xyRight[i--];
			point++;
		}
		assert( i == -1 );
		glyph->ep[ctr] = (short)(point-1);
	} else {
		int isOverExtended = IsOverExtended( path );
		glyph->ep[ctr] = (short)(point-1);
		ctr = glyph->contourCount++;
		glyph->pointCount  = (short)point;
		glyph->sp[ctr] = glyph->pointCount;

		limit = path->rightCount;
		if ( isOverExtended ) {
			/* In this unusual case we flip the path direction to create a black area */
			for ( i = 0; i < limit; ) {
				oox[point]		= path->xyRight[i++];
				ooy[point]		= path->xyRight[i++];
				onCurve[point]	= (uint8)path->xyRight[i++];
				point++;
			}
			assert( i == limit );
		} else {
			for ( i = limit-1; i >= 0; ) {
				onCurve[point]	= (uint8)path->xyRight[i--];
				ooy[point]		= path->xyRight[i--];
				oox[point]		= path->xyRight[i--];
				point++;
			}
			assert( i == -1 );
		}
	
		glyph->ep[ctr] = (short)(point-1);
	}
	
	glyph->pointCount  = (short)point;
	glyph->ep[ctr] = (short)(point-1);

	assert( glyph->pointCount <= glyph->pointCountMax );
	assert( glyph->contourCount <= glyph->contourCountMax );
}


/*
 * radius   possible values:   2,3,4,...
 * joinType possible values:   MITER_JOIN, ROUND_JOIN, BEVEL_JOIN 
 * capType  possible values:   BUTT_CAP, ROUND_CAP, SQUARE_CAP, SQUARE_50PERCENT_CAP 
 * open     possible values:   true, or false
 */
static void glyph_StrokeCtr( GlyphClass *glyph, int ctr, GlyphClass *glyph2, int radius, int joinType, int capType, int open )
{
	int point, count;
	int x1,y1,x2,y2;
	PathClass *path  = New_PathClass( glyph->mem );
	int overlap = 0;
	 
	
	point = glyph->sp[ctr];
	count = glyph->ep[ctr] - point + 1;
	
	if ( count > 1 ) {
		int pointNum2;
		x1 = glyph->oox[point];
		y1 = glyph->ooy[point];
		if ( open && x1 == glyph->oox[point + count -1] && y1 == glyph->ooy[point + count -1] ) {
			open = false;
			overlap = 1;
		}
		point++;
		x2 = glyph->oox[point];
		y2 = glyph->ooy[point];
		point++;
		
		
		
		if ( open ) {
			EndCap( path, capType, x1, y1, x1-x2, y1-y2, radius );
		} else {
			int point3 = glyph->ep[ctr]-overlap;
			int x0 = glyph->oox[point3];
			int y0 = glyph->ooy[point3];
			
			pointNum2 = glyph->sp[ctr];
			lineJoin( path, joinType, x0, y0, x1, y1, glyph->onCurve[pointNum2], pointNum2, x2,  y2, radius );
		}

	
		for ( ; point <= glyph->ep[ctr];  point++ ) {
			int x3,y3;
			x3 = glyph->oox[point];
			y3 = glyph->ooy[point];
			
			
			pointNum2 = point-1;
			lineJoin( path, joinType, x1, y1, x2, y2, glyph->onCurve[pointNum2], pointNum2, x3,  y3, radius );
			x1 = x2;
			y1 = y2;
			x2 = x3;
			y2 = y3;
		}
		
		if ( open ) {
			EndCap( path, capType, x2, y2, x2-x1, y2-y1, radius );
		} else {
			int point3 = glyph->sp[ctr]+overlap;
			int x3 = glyph->oox[point3];
			int y3 = glyph->ooy[point3];
			
			pointNum2 = glyph->ep[ctr];
			lineJoin( path, joinType, x1, y1, x2, y2, glyph->onCurve[pointNum2], pointNum2, x3,  y3, radius );
		}
		/* Rebuild the glyph from the path. */
		glyph_MergePathIntoGlyph( glyph2, path, open );
	}
	Delete_PathClass( path );
}

static void YShiftGlyph( GlyphClass *glyph, register int shift )
{
	register int i, y, endPoint;
	register short *ooy = glyph->ooy;

	if ( glyph->contourCount > 0 ) {
		endPoint = glyph->ep[ glyph->contourCount-1 ];
		for ( i = 0; i <= endPoint; i++ ) {
			y = ooy[i];
			y += shift;
			ooy[i] = (short)y;
		}
	}
}

/*
 * This provides "native" hint processing for the stroke format.
 */
static void ApplyHintsToStrokeGlyph( GlyphClass *glyph, F16Dot16 xScale, F16Dot16 yScale, int radius, int alwaysOn )
{
	register int ptA, ptB, endPoint, i1;
	register short zA, zB, dz;
	register short *ooz, pixSize;
	int pass, ctr;
	F16Dot16 scale;
	F26Dot6 strokeDiameter, alignment;
#define kUsageBufferSize 32
#define kUsageBufferMask 127
	uint32 usageBuffer[kUsageBufferSize];
	register uint8 *usage;

	assert( sizeof( uint32) >= 4 * sizeof(uint8) ); /* We count on being able to access 64 bytes in here */
	for ( pass = 0; pass < 2; pass++ ) {
		if ( pass == 0 ) {      
			/* Do y */
			strokeDiameter = util_FixMul( radius+radius, scale = yScale );
#ifdef OLD
			if ( strokeDiameter >= 1*64+16 ) continue; /*****/
#endif
			pixSize = (short)(util_FixDiv( 64, yScale ) );
			ooz = glyph->ooy;
		} else {
			/* Do x */
			strokeDiameter = util_FixMul( radius+radius, scale = xScale);
#ifdef OLD
			if ( strokeDiameter >= 1*64+16 ) continue; /*****/
#endif
			pixSize = (short)(util_FixDiv( 64, xScale ) );
			ooz = glyph->oox;
		}
		alignment = strokeDiameter + 32;
		if ( alignment < 64 ) alignment = 64;
		alignment = alignment & 64 ? 32 : 0;
		if ( !alwaysOn ) {
			register uint32 *usage32 = usageBuffer;
			for ( ptA = 0; ptA < kUsageBufferSize; ) {
				usage32[ptA++] = 0;
 				usage32[ptA++] = 0;
				usage32[ptA++] = 0;
				usage32[ptA++] = 0;
			}
		}
		usage = (uint8 *)usageBuffer;
		for ( ctr = 0; ctr < glyph->contourCount; ctr++ ) {
			endPoint = glyph->ep[ ctr ];
			ptA = glyph->sp[ ctr ];
			zA = ooz[ptA];
			for ( ptB = ptA+1; ptB <= endPoint; ) {
				zB = ooz[ptB];
				dz = (short)(zA - zB);
				if ( dz <= 1 && dz >= -1 ) {
					long z2, z1;
        
					z1 = util_FixMul( zA, scale );  /* scale to pixel space */
#ifdef OLD
					z2 = (z1 & ~63) + 32;           /* round to closest half-pixel */
#endif
					/* round to correct alignment,  alignment is either 0 or 32 */
					if ( alignment == 0 ) {
						z2 = ((z1+32) & ~63) /* + 0 */;
					} else {
						z2 = (z1 & ~63) + 32;
					}
					i1 = (int)(z2 >> 6);
					if ( !alwaysOn && (usage[i1 & kUsageBufferMask] || usage[(i1-1) & kUsageBufferMask] || usage[(i1+1) & kUsageBufferMask]) ) {
						;
					} else {
						register short delta = (short)(z2-z1);
						/* This is equivalent to zA += util_FixDiv(z2-z1, scale); but it avoids util_FixDiv :-) */
						if ( delta >= 0 ) {
							zA = (short)(zA + ((( delta) * pixSize + 32) >> 6));
						} else {
							zA = (short)(zA - (((-delta) * pixSize + 32) >> 6));
						}
						ooz[ptA] = zA;
						ooz[ptB] = zA;
					}
					usage[i1 & kUsageBufferMask] = 1;
				}
				ptA = ptB;
				zA  = zB;
				ptB++;
			}
		}
	}
}

/*
 * glyph1 is the input GlyphClass;
 * radius   possible values:   2,3,4,...
 * joinType possible values:   MITER_JOIN, ROUND_JOIN, BEVEL_JOIN 
 * capType  possible values:   BUTT_CAP, ROUND_CAP, SQUARE_CAP, SQUARE_50PERCENT_CAP 
 * open     possible values:   true, or false
 */
GlyphClass *ff_glyph_StrokeGlyph( GlyphClass *glyph1, int radius, int joinType, int capType, int open )
{
	int ctr, pointCount1, pointCount2;
	GlyphClass *glyph2;
	
	glyph2 = New_EmptyGlyph( glyph1->mem, 0, 0, 0, 0 );

	glyph2->contourCountMax = glyph1->contourCount;
	/* if ( !open ) glyph2->contourCountMax += glyph2->contourCountMax; */
	glyph2->contourCountMax = (short)(glyph2->contourCountMax + glyph2->contourCountMax);
	
	if ( glyph2->contourCountMax <= T2K_CTR_BUFFER_SIZE ) {
		glyph2->sp = glyph2->ctrBuffer;
		glyph2->ep = &glyph2->sp[T2K_CTR_BUFFER_SIZE];
	} else {
		glyph2->sp = (short *) tsi_AllocMem( glyph2->mem, sizeof(short) * 2 * glyph2->contourCountMax);
		glyph2->ep   = &glyph2->sp[glyph2->contourCountMax];
	}
	glyph2->sp[0] = glyph2->ep[0] = 0;
	glyph2->contourCount = 0;
	
	/* If not open then the contourCount would change, and the below loop would have to be fixed. */
	for ( ctr = 0; ctr < glyph1->contourCount; ctr++ ) {
		glyph_StrokeCtr( glyph1, ctr, glyph2, radius, joinType, capType, open );
	}
	pointCount1 =  glyph1->pointCount;
	pointCount2 =  glyph2->pointCount;
	
	glyph2->ooy[pointCount2 + 0] = glyph1->ooy[pointCount1 + 0];
	glyph2->oox[pointCount2 + 0] = glyph1->oox[pointCount1 + 0];
	
	glyph2->ooy[pointCount2 + 1] = glyph1->ooy[pointCount1 + 1];
	glyph2->oox[pointCount2 + 1] = glyph1->oox[pointCount1 + 1];

#if SbPtCount >= 4
	glyph2->ooy[pointCount2 + 2] = glyph1->ooy[pointCount1 + 2];
	glyph2->oox[pointCount2 + 2] = glyph1->oox[pointCount1 + 2];
	
	glyph2->ooy[pointCount2 + 3] = glyph1->ooy[pointCount1 + 3];
	glyph2->oox[pointCount2 + 3] = glyph1->oox[pointCount1 + 3];
#endif
	Delete_GlyphClass( glyph1 );
	
	return glyph2; /*****/
}

/****************************************************************************/
/************* BELOW WE HAVE STROKE FONT READING/PARSING CODE ***************/
/****************************************************************************/

#ifdef ENABLE_WRITE
/*
 *
 */
int Write2Numbers( uint16 arrIn[], OutputStream *out )
{
	int more, pass;
	uint8 bitCount, mask, val = 0;
	uint16 arr[2];
	
	arr[0] = arrIn[0];
	arr[1] = arrIn[1];
	
	for ( pass = 0; true; pass++ ) {
		more = false; val = 0;
		bitCount = 4; mask = (uint8)((1 << bitCount) - 1);
		val <<= bitCount; val |= arr[0] & mask; arr[0] >>= bitCount; more |= (arr[0] != 0);
		
		bitCount = 3; mask = (uint8)((1 << bitCount) - 1);
		val <<= bitCount; val |= arr[1] & mask; arr[1] >>= bitCount; more |= (arr[1] != 0);
		
		val <<= 1; val |= (more ? 1 : 0 );
		
		WriteUnsignedByte( out, val );
		
		if ( !more ) {
			break; /*****/
		} 
	}
	return pass + 1; /******/
}
#endif

/*
 *
 */
void ff_Read2Numbers( InputStream *in, uint16 arr[] )
{
	int i,N, more;
	uint8 aData[8], aCode;
	uint8 bitCount, mask;
	
	arr[0] = 0;
	arr[1] = 0;
	
	N = 0;
	for ( i = 0; i < 8; i++ ) {
		aCode = ReadUnsignedByteMacro( in );
		more = aCode & 1; aCode >>= 1;
		aData[i] = aCode;
		N++;
		if ( !more) break;
	}
	for ( i = N-1; i >= 0; i-- ) {
		bitCount = 3; mask = (uint8)((1 << bitCount) - 1);
		arr[1] <<= bitCount; arr[1] |= aData[i] & mask; aData[i] >>= bitCount;
		bitCount = 4; mask = (uint8)((1 << bitCount) - 1);
		arr[0] <<= bitCount; arr[0] |= aData[i] & mask; aData[i] >>= bitCount;
	}
}


#ifdef ENABLE_WRITE
/*
 *
 */
int Write4Numbers( uint16 arrIn[], OutputStream *out )
{
	int more, pass;
	uint8 bitCount, mask, val = 0;
	uint16 arr[4];
	
	arr[0] = arrIn[0];
	arr[1] = arrIn[1];
	arr[2] = arrIn[2];
	arr[3] = arrIn[3];
	
	for ( pass = 0; true; pass++ ) {
		more = false; val = 0;
		bitCount = 1; mask = (uint8)((1 << bitCount) - 1);
		val <<= bitCount; val |= arr[0] & mask; arr[0] >>= bitCount; more |= (arr[0] != 0);
		
		bitCount = 1; mask = (uint8)((1 << bitCount) - 1);
		val <<= bitCount; val |= arr[1] & mask; arr[1] >>= bitCount; more |= (arr[1] != 0);
		
		bitCount = 2; mask = (uint8)((1 << bitCount) - 1);
		val <<= bitCount; val |= arr[2] & mask; arr[2] >>= bitCount; more |= (arr[2] != 0);
		
		bitCount = 3; mask = (uint8)((1 << bitCount) - 1);
		val <<= bitCount; val |= arr[3] & mask; arr[3] >>= bitCount; more |= (arr[3] != 0);
		
		val <<= 1; val |= (more ? 1 : 0 );
		
		WriteUnsignedByte( out, val );
		
		if ( !more ) {
			break; /*****/
		} 
	}
	return pass + 1; /******/
}
#endif


/*
 *
 */
void ff_Read4Numbers( InputStream *in, uint16 arr[] )
{
	int i,N, more;
	uint8 aData[8], aCode;
	uint8 bitCount, mask;
	
	arr[0] = 0;
	arr[1] = 0;
	arr[2] = 0;
	arr[3] = 0;
	
	N = 0;
	for ( i = 0; i < 8; i++ ) {
		aCode = ReadUnsignedByteMacro( in );
		more = aCode & 1; aCode >>= 1;
		aData[i] = aCode;
		N++;
		if ( !more) break;
	}
	for ( i = N-1; i >= 0; i-- ) {
		bitCount = 3; mask = (uint8)((1 << bitCount) - 1);
		arr[3] <<= bitCount; arr[3] |= aData[i] & mask; aData[i] >>= bitCount;
		bitCount = 2; mask = (uint8)((1 << bitCount) - 1);
		arr[2] <<= bitCount; arr[2] |= aData[i] & mask; aData[i] >>= bitCount;
		bitCount = 1; mask = (uint8)((1 << bitCount) - 1);
		arr[1] <<= bitCount; arr[1] |= aData[i] & mask; aData[i] >>= bitCount;
		bitCount = 1; mask = (uint8)((1 << bitCount) - 1);
		arr[0] <<= bitCount; arr[0] |= aData[i] & mask; aData[i] >>= bitCount;
	}
}


#ifdef ENABLE_WRITE
/*
 *
 */
void WriteLowUnsignedNumber( OutputStream *out, unsigned long n )
{
	unsigned char value;
	
	for(;;) {
		if ( n >= 240 ) {
			value = (unsigned char)(240 + (n&0x0f));
			n >>= 4;
			WriteUnsignedByte( out, value );
		} else {
			value = (unsigned char)(n);
			WriteUnsignedByte( out, value );
			break; /*****/
		}
	}
}
#endif

/*
 *
 */
static unsigned long ReadLowUnsignedNumber( InputStream *in, void *entropycoder )
{
	unsigned char value;
	unsigned long n = 0;
	unsigned long shift = 0;
#ifdef SOME_DAY_BITLEVEL
	SCODER *sc = (SCODER *)entropycoder;
#else
	UNUSED(entropycoder);
#endif 

	
	for (;;) {
#ifdef SOME_DAY_BITLEVEL
		if ( sc != NULL ) {
			SCODER_ReadSymbol( sc, in );
		} else {
			value = ReadUnsignedByteMacro( in );
		}
#else
		value = ReadUnsignedByteMacro( in );
#endif
		if ( value >= 240 ) {
			n |= ((unsigned long)value-240) << shift;
			shift += 4;
		} else {
			n |= ((unsigned long)value) << shift;
			break; /*****/
		}
	}
	return n; /*****/
}

/*
 *
 */
static void AllocatePointSpaceIfNeeded( GlyphClass *t, long N )
{
	register short *oox, *ooy;
	register short *ooxOld, *ooyOld;
	register uint8 *onCurve;
	register uint8 *onCurveOld;
	F26Dot6 *memBase;
	int i, limit;
	
	if ( N > t->pointCountMax ) { 
		/* ReAllocate */
		memBase = t->x; ooxOld = t->oox; ooyOld = t->ooy; onCurveOld = t->onCurve;
		AllocGlyphPointMemory( t, N + 32 );
		oox = t->oox; ooy = t->ooy; onCurve = t->onCurve;
		
		limit = t->pointCount + SbPtCount;
		for ( i = 0; i < limit; i++ ) {
			oox[i]		= ooxOld[i];
			ooy[i]		= ooyOld[i];
			onCurve[i]	= onCurveOld[i];
		}
		tsi_FastDeAllocN( t->mem, memBase, T2K_FB_POINTS); /* contains all the old point data */
	}
}

/*
 *
 */
static void AllocateContourSpaceIfNeeded( GlyphClass *t, long N )
{
	/* UNUSED(t); UNUSED(N); */
	glyph_AllocContours( t, (short)N );
}

/*
 * Caller has to do this "Delete_InputStream( stream, NULL );"
 * after the caller is done with the stream.
 */
static InputStream *GetGlyphStream( sfntClass *font, long index, uint32 *lengthPtr )
{
	InputStream *stream = NULL;
	slocClass *sloc;
	sfnt_DirectoryEntry *dirEntry  = GetTableDirEntry_sfntClass( font, tag_GlyphData );
	unsigned long length = 0;
	
	sloc = font->sloc;
	
	if ( dirEntry != NULL && sloc != NULL && index >= 0 && index < font->numGlyphs ) {
		unsigned long position = (uint32)Tell_InputStream( font->in );
		unsigned long offset1 = FF_SLOC_MapIndexToOffset( sloc, font->in, index + 0);
		unsigned long offset2 = FF_SLOC_MapIndexToOffset( sloc, font->in, index + 1);
		length = offset2 - offset1;
	
		assert( offset2 >= offset1 );
		Seek_InputStream( font->in, position );
		stream = New_InputStream2( font->mem, font->in, dirEntry->offset + offset1, length, T2K_FB_IOSTREAM, NULL );
	}
	*lengthPtr = length;
	return stream; /*****/
}

/*
 * The ffhmClass constructor
 */
ffhmClass *FF_New_ffhmClass( tsiMemObject *mem, InputStream *in )
{
	register int32 i, N, version;
	ffhmClass *t = NULL;
	
	version					= ReadInt32( in );
	if ( version >= 0x00010000 && version < 0x00020000 ) {
		t 					= (ffhmClass *) tsi_AllocMem( mem, sizeof( ffhmClass ) );
		t->mem				= mem;
		t->version			= version;
		
		t->N				= N = ReadInt32( in );
		t->gIndex			= (uint16 *)tsi_AllocMem( mem, sizeof( uint16 ) * (N+N) );
		t->aw				= &t->gIndex[ N ];
		
		t->defaultWidth 	= (uint16)ReadInt16( in );
		t->reserved 		= (uint16)ReadInt16( in );
		
		for ( i = 0; i < N; i++ ) {
			t->gIndex[i] = (uint16)ReadInt16( in );
		}
		for ( i = 0; i < N; i++ ) {
			t->aw[i] 	= (uint16)ReadInt16( in ); 
		}
	}
	
	return t; /*****/
}

/*
 * The ffhmClass Get-Advance-Width method
 */
uint16 FF_GetAW_ffhmClass( void *param1, register uint16 index )
{
	register ffhmClass *t = (ffhmClass *)param1;
	register long low, mid, high;
	register uint16 midIndex, value, *gIndex = t->gIndex;

	value = t->defaultWidth;
	low = 0;
	high = t->N - 1;
	/* Do a binary search */
	do {
		mid = (low + high) >> 1;
		midIndex = gIndex[mid];
		if ( index > midIndex ) {
			low = mid+1;
		} else if ( index < midIndex ) {
			high = mid-1;
		} else { /* index == midIndex */
			value = t->aw[mid];
			break; /*****/
		}
	} while ( high >= low );
	return value; /*****/
}


/*
 * The ffhmClass destructor
 */
void FF_Delete_ffhmClass( ffhmClass *t )
{
	if ( t != NULL ) {
		assert( t->gIndex != NULL );
		tsi_DeAllocMem( t->mem, t->gIndex );
		/* No need to deallocate t->aw since it points &t->gIndex[N] */
		tsi_DeAllocMem( t->mem, t );
	}
}

#ifdef ENABLE_WRITE
/*
 * Writes out the ffhm image into the out stream
 */
void FF_Write_ffhmClass( OutputStream *out, int32 numGlyphs, uint16 *aw, uint16 defaultWidth )
{
	register int32 i, N = 0;
	uint32 pos0, pos1;
	
	pos0 	= Tell_OutputStream( out );
	for ( i = 0; i < numGlyphs; i++ ) {
		if ( aw[i] != defaultWidth ) N++;
	}
	
	WriteInt32( out, 0x00010000 );
	WriteInt32( out, N );
	printf("ffhm: N = %d, defaultWidth = %d\n", N, defaultWidth );
	WriteInt16( out, (short)defaultWidth );
	WriteInt16( out, 0 ); /* reserved */

	for ( i = 0; i < numGlyphs; i++ ) {
		if ( aw[i] != defaultWidth ) {
			WriteInt16( out, (short)i );
		}
	}
	for ( i = 0; i < numGlyphs; i++ ) {
		if ( aw[i] != defaultWidth ) {
			WriteInt16( out, (short)aw[i] );
		}
	}
	pos1 	= Tell_OutputStream( out );
	/* Now test the table!!! This is the automatic biuilt in unit test for the ffhm class */
	{
		int errCode;
		ffhmClass *ffhm;
		InputStream *in = New_InputStream3( out->mem,  GET_POINTER( out ) + pos0, pos1-pos0, &errCode );
		
		printf("Starting ffhm class unit test...\n");
		assert( errCode == 0 );
		ffhm = FF_New_ffhmClass( out->mem, in );
	
		for ( i = 0; i < numGlyphs; i++ ) {
			uint16 result = FF_GetAW_ffhmClass( ffhm, (uint16)i );
			assert( result == aw[i] );
		}
		
		FF_Delete_ffhmClass( ffhm );
		Delete_InputStream( in, &errCode);
		assert( errCode == 0 );
		printf("ffhm class unit test OK!!!\n");
	}
}
#endif





/*
 * Creates a GlyphClass object for the glyph corresponding to index.
 */
GlyphClass *ff_New_GlyphClassT2KS( sfntClass *font, GlyphClass *glyph, long index, uint16 *aWidth, uint16 *aHeight, void *model, int depth )
{
	register long i, iOut, j;
	tsiMemObject *mem = font->mem;
	register short *oox = NULL, *ooy = NULL;
	register uint8 *onCurve;
	register int pointCount = 0;
	register short xmin, ymax;
	uint16 arr[4], num0011, numXX11, numXXXX, numCTRS;
	short stmp;
	register InputStream *in;
	uint32 glyphLength;
#ifdef SOME_DAY_BITLEVEL
	void *entropycoder;
		OrionModelClass *orion = (OrionModelClass *)model;
		long			switchPos, numSwitches;
		unsigned char  *switchData = NULL;
#endif
	
	
	UNUSED(model);
	
	
 	
 	in = GetGlyphStream( font, index, &glyphLength );
 	if ( glyphLength == 0 ) {
		if ( glyph == NULL ) {
			glyph = New_EmptyGlyph( mem, 0, 0, 0, 0 );
			oox = glyph->oox; ooy = glyph->ooy;
			pointCount = 0;
		}
 	} else {
	 	assert( in != NULL );
		ff_Read4Numbers( in, arr );
		
		num0011 = arr[0];
		numXX11 = arr[1];
		numXXXX = arr[2];
		numCTRS = arr[3];
		
		if ( depth > 0 && (num0011!=0 || numXX11 != 0 || numXXXX != 0 ) ) {
			/* printf("index = %d\n", index ); */
			assert( false );
			num0011 = numXXXX = numXXXX = 0;
		}

		/* xOffset == yOffset == 0, xScale == yScale == ONE16Dot16 */
		for ( i = 0; i < num0011; i++ ) {
			uint16 aWidth, aHeight;
			long radIndex;
			
			radIndex =  (long)ReadLowUnsignedNumber( in, NULL );
			glyph = ff_New_GlyphClassT2KS( font, glyph, radIndex, &aWidth, &aHeight, model, depth+1 );
		}
		/* xOffset == *, yOffset == *, xScale == yScale == ONE16Dot16 */
		for ( i = 0; i < numXX11; i++ ) {
			uint16 aWidth, aHeight;
			int16 dx, dy;
			int onCurve, countA, countB;
			long radIndex;;
			
			onCurve = ReadDeltaXYValue( in, &dx, &dy );
			assert( onCurve == 0  || onCurve == 1 );
			dx = (int16)((dx + dx) + onCurve);
		
			countA = glyph == NULL ? 0 : glyph->pointCount;
			radIndex =  (long)ReadLowUnsignedNumber( in, NULL );
			glyph = ff_New_GlyphClassT2KS( font, glyph, radIndex, &aWidth, &aHeight, model, depth+1 );
			assert( glyph != NULL );
			countB = glyph->pointCount;
			oox = glyph->oox; ooy = glyph->ooy;
			for ( j = countA; j < countB; j++ ) {
				oox[j] = (short)(oox[j] + dx);
				ooy[j] = (short)(ooy[j] + dy);
			}
		}
		/* xOffset == *, yOffset == *, xScale == *, yScale == * */
		for ( i = 0; i < numXXXX; i++ ) {
			uint16 aWidth, aHeight;
			int16 dxO, dyO, dxS, dyS, xScale4Dot12, yScale4Dot12;
			int onCurve, countA, countB;
			long radIndex;
			F16Dot16 xMul, yMul;
			
			onCurve = ReadDeltaXYValue( in, &dxO, &dyO );
			assert( onCurve == 0  || onCurve == 1 );
			dxO = (int16)((dxO + dxO) + onCurve);

			onCurve = ReadDeltaXYValue( in, &dxS, &dyS );
			assert( onCurve == 0  || onCurve == 1 );
			dxS = (int16)((dxS + dxS) + onCurve);
			xMul = util_FixDiv(256 + dxS, 256); xScale4Dot12 = (int16)((xMul + (1 << 3)) >> 4);
			yMul = util_FixDiv(256 + dyS, 256); yScale4Dot12 = (int16)((yMul + (1 << 3)) >> 4);
		
			countA = glyph == NULL ? 0 : glyph->pointCount;
			radIndex =  (long)ReadLowUnsignedNumber( in, NULL );
			glyph = ff_New_GlyphClassT2KS( font, glyph, radIndex, &aWidth, &aHeight, model, depth+1 );
			assert( glyph != NULL );
			countB = glyph->pointCount;
			oox = glyph->oox; ooy = glyph->ooy;
			for ( j = countA; j < countB; j++ ) {
				oox[j] = (int16)((((long)oox[j] * xScale4Dot12  + (1 << 11) ) >> 12) + dxO);
				ooy[j] = (int16)((((long)ooy[j] * yScale4Dot12  + (1 << 11) ) >> 12) + dyO);
			}
		}

		if ( glyph == NULL ) {
			glyph = New_EmptyGlyph( mem, 0, 0, 0, 0 );
		}
		 oox = glyph->oox; ooy = glyph->ooy;
		
		AllocateContourSpaceIfNeeded( glyph, glyph->contourCount + numCTRS );
		
#	ifdef SOME_DAY_BITLEVEL
		entropycoder = NULL;
		if ( orion != NULL ) {
			entropycoder = orion->ep;
		}
#	endif	

		stmp = glyph->pointCount;
		for ( i = 0; i < numCTRS; ) {
			uint16 arr[2];
			ff_Read2Numbers( in, arr );
		
		    glyph->sp[glyph->contourCount]	= stmp;
		    glyph->ep[glyph->contourCount]	= (short)(arr[0] + stmp);
		 	stmp = (short)(glyph->ep[glyph->contourCount] + 1);
		 	glyph->contourCount++; i++;
		 	if ( i >= numCTRS ) break; /*****/
		    glyph->sp[glyph->contourCount]	= stmp;
		    glyph->ep[glyph->contourCount]	= (short)(arr[1] + stmp);
		 	stmp = (short)(glyph->ep[glyph->contourCount] + 1);
		 	glyph->contourCount++; i++;
		 	
		}
		assert( glyph->contourCount <= glyph->contourCountMax );
		pointCount = stmp - glyph->pointCount;
		
		iOut = glyph->pointCount;
		if (pointCount > 0  ) {
			short x, y;
			
			AllocatePointSpaceIfNeeded( glyph, glyph->pointCount+pointCount );
			oox = glyph->oox; ooy = glyph->ooy; onCurve = glyph->onCurve;
			assert( iOut+pointCount  <= glyph->pointCountMax );
			glyph->hintLength = 0;
			
	#ifdef SOME_DAY_BITLEVEL
			if ( orion != NULL ) {
				numSwitches = (pointCount+7)/8;;
				switchData = (unsigned char *) tsi_AllocMem( mem, numSwitches * sizeof( unsigned char ) );
				for ( switchPos = 0; switchPos < numSwitches; switchPos++ ) {
					switchData[ switchPos ] = SCODER_ReadSymbol( orion->control, in ); /* Initialize */
				}
				orion->OrionState = ORION_STATE_0 % orion->num_eb1; /* Initialize */;
			}
			switchPos = 0;

	#endif
			

			x = y = 0;
			xmin = 0x7fff;
			ymax = -0x7fff;
			for ( i = 0; i < pointCount; i++ ) {
				short dx, dy;
	#ifdef SOME_DAY_BITLEVEL
				if ( orion != NULL ) {
					if ( switchData[ switchPos>>3 ] & (1 << (switchPos&7)) ) {
						unsigned char index;
						int i256 = orion->OrionState << 8;
						/* copy */
						index		= SCODER_ReadSymbol( orion->copy[ orion->OrionState ], in );
						/* printf("%d, %d\n",orion->OrionState, index ); */
						i256       += index;
						dx 			= orion->dx[i256];
						dy 			= orion->dy[i256];
						onCurve[iOut]	= (unsigned char)orion->onCurve[i256];
						/* printf("C dx = %d, dy = %d, onCurve = %d\n", dx, dy, (int)onCurve[i] ); */
					} else {
						/* literal */
						/* REVERSE dy & dx for CJK for smaller size */
						onCurve[iOut] = (unsigned char)ReadOrionDeltaXYValue( in, orion, &dy, &dx);
						/* printf("L dx = %d, dy = %d, onCurve = %d\n", dx, dy, (int)onCurve[i] ); */
					}
					switchPos++;
					Set_OrionState( orion, dx, dy, (char)onCurve[i] );
				} else {
					/* REVERSE dy & dx for CJK for smaller size */
					onCurve[iOut] = (unsigned char)ReadDeltaXYValue( in, &dy, &dx );
				}

	#else
				/* REVERSE dy & dx for CJK for smaller size */
				onCurve[iOut] = (unsigned char)ReadDeltaXYValue( in, &dy, &dx );
	#endif
				x = (short)(x + dx);
				y = (short)(y + dy);
				if ( x < xmin ) xmin = x;
				if ( y > ymax ) ymax = y;
				oox[iOut] = x;
				ooy[iOut] = y;
				iOut++;
			}
			glyph->xmin = xmin;
			glyph->ymax = ymax;
	#ifdef SOME_DAY_BITLEVEL
			tsi_DeAllocMem( mem, switchData );
	#endif		
			assert( xmin >= -200 );
			assert( ymax <= 500);
		}
		pointCount = iOut;
	}

	assert( oox != NULL );
	assert( pointCount <= glyph->pointCountMax );
	{
		uint16 aw, ah, upem = GetUPEM( font );
		int16 lsb, tsb;

		aw = ah = upem;
		
		assert( font->GetAWFuncPtr1 != NULL );
		aw = font->GetAWFuncPtr1( font->GetAWParam1, (uint16)index );
#ifdef OLD
		if ( font->ffhm != NULL ) {
			aw = FF_GetAW_ffhmClass( font->ffhm, (uint16)index );
		}
#endif
		lsb = glyph->xmin;
		tsb = (short)(upem - glyph->ymax);
		ooy[pointCount + 0] = 0;
		oox[pointCount + 0] = (short)(glyph->xmin - lsb);
		
		ooy[pointCount + 1] = 0;
		oox[pointCount + 1] = (short)(oox[pointCount + 0] + aw);
		
	#if SbPtCount >= 4
		{
			long xMid = (oox[pointCount + 0] + oox[pointCount + 1]) >> 1;
			ooy[pointCount + 2] = (short)(glyph->ymax + tsb);
			oox[pointCount + 2] = (short)xMid;
			
			ooy[pointCount + 3] = (short)(ooy[pointCount + 2] - ah);
			oox[pointCount + 3] = (short)xMid;
		}
	#else
	tsb;ah;
	#endif
		*aWidth = aw;
		*aHeight = ah;
	}
	glyph->pointCount = (short)pointCount;
	
	Delete_InputStream( in, NULL );
	
	if ( depth == 0 ) {
		int radius;
		int joinType = MITER_JOIN;  /* MITER_JOIN, ROUND_JOIN, BEVEL_JOIN */
		int capType  = SQUARE_CAP;	/* BUTT_CAP, ROUND_CAP, SQUARE_CAP, SQUARE_50PERCENT_CAP */

		assert( font->ffst != NULL );
		radius = font->ffst->minRadius + util_FixMul( font->currentCoordinate[0], font->ffst->maxRadius - font->ffst->minRadius );
		/* Integralize the stroke diameter for monochrome output. */
		if ( font->greyScaleLevel == 0 && font->yScale > 0 ) {
			/* Compute the stroke diameter. */
			long diameter = util_FixMul( radius+radius, font->yScale );
			/* Integralize it to en even number of pixels. */
			diameter += 32; diameter >>= 6; diameter <<= 6;
			/* Force the stroke diameter to be at least one pixel wide. */
			if ( diameter < 64 ) diameter = 64;
			/* Compute the radius in the original domain. */
			radius = util_FixDiv( diameter/2, font->yScale );
			/* If the radius is too small due to rounding, then increase it. */
			while ( util_FixMul( radius+radius, font->yScale ) < diameter ) {
				radius++;
			}
		}
		YShiftGlyph( glyph, radius ); /* Maintain baseline alignment */
        if ( font->useNativeHints )  ApplyHintsToStrokeGlyph( glyph, font->xScale, font->yScale, radius, font->greyScaleLevel == 0 );
		if ( font->strokeGlyph ) {
			glyph = ff_glyph_StrokeGlyph( glyph, radius, joinType, capType, true);
		}
		if ( font->maxp->maxPoints < glyph->pointCount ) {
			font->maxp->maxPoints = (uint16)glyph->pointCount;
		}
		if ( font->maxp->maxContours < glyph->contourCount ) {
			font->maxp->maxContours = (uint16)glyph->contourCount;
		}
	}
	return glyph; /*****/
}


/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/
/***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** *****/


/*
 * Sets default values.
 */
static void Set_ffstDefaults( ffstClass *t  )
{
	t->numAxes 		= 1;
	t->minRadius 	= 2;
	t->maxRadius 	= 10;
	t->gIndexFirstRoman = 0;
	t->gIndexLastRoman 	= 0;
}

#ifdef ENABLE_WRITE
/*
 * int gIndexFirstRoman = 0;
 * int gIndexLastRoman  = 0;
 */
void FF_Write_ffstClass( OutputStream *out, int gIndexFirstRoman, int gIndexLastRoman )
{
	WriteInt16( out, 1); /* majorVersion*/
	WriteInt16( out, 0); /* minorVersion*/

	WriteInt16( out, 0); WriteInt16( out, 1);  /* numAxes */
	WriteInt16( out, 1); WriteInt16( out, 2);  /* minRadius */
	WriteInt16( out, 2); WriteInt16( out, 10); /* maxRadius */
	WriteInt16( out, 3); WriteInt16( out, (int16)gIndexFirstRoman); /* gIndexFirstRoman */
	WriteInt16( out, 4); WriteInt16( out, (int16)gIndexLastRoman); /* gIndexLastRoman */
}

#endif /* ENABLE_WRITE */

/*
 * Creates a ffstClass.
 */
ffstClass *FF_New_ffstClass( tsiMemObject *mem, InputStream *in, unsigned long length )
{
	ffstClass *t = (ffstClass *) tsi_AllocMem( mem, sizeof( ffstClass ) );
	t->mem						= mem;
	
	t->majorVersion	= 0;
	t->minorVersion = 0;
	Set_ffstDefaults( t );
	
	if ( in != NULL ) {
		t->majorVersion				= ReadInt16( in );
		t->minorVersion				= ReadInt16( in );
		
		if ( t->majorVersion == 1 ) {
			unsigned long i;
			for ( i = 4; i < length; i += 4) {
				int16 selector = ReadInt16( in );
				int16 value    = ReadInt16( in );
				
				switch( selector ) {
				case 0: t->numAxes 		= value; break;
				case 1: t->minRadius 	= value; break;
				case 2: t->maxRadius	= value; break;
				case 3: t->gIndexFirstRoman	= value; break;
				case 4: t->gIndexLastRoman	= value; break;
				default: break;
				}
			}
		}
	}
	return t; /*****/
}

/*
 * Deletes a ffstClass.
 */
void FF_Delete_ffstClass( ffstClass *t )
{
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, t );
	}
}


#endif /* ENABLE_T2KS */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: $
 *                                                                           *
 *     $Log:  $
 *                                                                           *
******************************************************************************/

