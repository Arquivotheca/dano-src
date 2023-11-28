/*
 * strkconv.c
 * Font Fusion Copyright (c) 2000 all rights reserved by Bitstream Inc.
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
#include "strkconv.h"
#include "util.h"


#ifdef ENABLE_STRKCONV

#define INTEGER_T2K_SC_LIMIT 46340

/* #define AUTO_SHARPEN2 */
#define AUTO_SHARPEN2

/* Only define if testing */
/* #define BOUNDS_CHECK_ON */

/*
 *
 */
static void DrawLine( ffStrkConv *t, F26Dot6 x0, F26Dot6 y0, F26Dot6 x2, F26Dot6 y2, int end0, int end2, long xRadius, long yRadius )
{
	F26Dot6 xA, xB, yA, yB, center, dist, lim;
	F26Dot6 limA, limB;
	int endA, endB;
	long h1 = t->ymax - t->ymin - 1;
	long xmin = t->xmin;
	long ymin = t->ymin;
#ifdef BOUNDS_CHECK_ON
	long xmax = t->xmax;
	long ymax = t->ymax;
#endif
	long rowBytes = t->rowBytes;
	register uint8 *ptr, *baseAddr = t->baseAddr;
	register uint8 pixv,pixv2, pixo, *ptrB;
	int mostlyX;
	F26Dot6 absDx, absDy;
	
	absDx = x2 - x0; if ( absDx < 0 ) absDx = -absDx;
	absDy = y2 - y0; if ( absDy < 0 ) absDy = -absDy;
	absDx = (absDx * yRadius );
	absDy = (absDy * xRadius );
	
	mostlyX = absDx >= absDy;
    /* Supress insignificant compiler warnings on some compilers. */
    xA = xB = yA = yB = 0;
    endA = endB = 0;
    pixv2 = 0;	
	if ( mostlyX ) {
		/* Do X Edges */
		if ( x0 < x2 ) {
			xA = x0; yA = y0; endA = end0;
			xB = x2; yB = y2; endB = end2;
		} else if ( x0 > x2 ) {
			xA = x2; yA = y2; endA = end2;
			xB = x0; yB = y0; endB = end0;
		}
		
		dist = xB - xA;

		limB = xB;
		limA = (xA + 31) & ~63;
		if ( absDx >= absDy && absDx > 0 ) {
			lim		= xB + xRadius;
			center	= (xA + 31 - xRadius) & ~63;
		} else {
			lim		= limB;
			center	= limA;
		}
		center += 32;
		
		/* This code is never intended to be used at high sizes (>700ppem), where dist >= INTEGER_T2K_SC_LIMIT */
		/* For these high sizes we would need to compute: */
		/* y = (yA + util_FixMul( yStep, util_FixDiv( center - xA, dist ) )); */
		assert( dist+xRadius+xRadius < INTEGER_T2K_SC_LIMIT );
		
		if ( center <= lim ) {
			F26Dot6 z, y, yStep = yB - yA;
			z = yStep * (center - xA);
			yStep <<= 6;
#ifdef BOUNDS_CHECK_ON
if ( (center>>6) < xmin ) {
	SysBeep(10);
}
if ( (lim>>6) >= xmax ) {
	SysBeep(10);
}
#endif
			do {
				y		 = (yA + z / dist);
				/* Process (center,y ) */

				{
					int done;
					long y1, y2, x;
					y1 = y - yRadius;
					y2 = y + yRadius;
#ifdef BOUNDS_CHECK_ON
if ( (y1>>6) < ymin ) {
	SysBeep(10);
}
if ( (y2>>6) >= ymax ) {
	SysBeep(10);
}
#endif

					x = (center>>6) - xmin;
					
#ifdef REVERSE_SC_Y_ORDER
					ptr  = &baseAddr[( ((y1>>6) - ymin) ) * rowBytes + x];
					ptrB = &baseAddr[( ((y2>>6) - ymin) ) * rowBytes + x];
#else
					ptr  = &baseAddr[( h1 - ((y1>>6) - ymin) ) * rowBytes + x];
					ptrB = &baseAddr[( h1 - ((y2>>6) - ymin) ) * rowBytes + x];
#endif
					if ( ptr == ptrB ) {
						pixv = (uint8)(y2-y1);
						pixv = (uint8)(pixv+pixv);
						done = true;
					} else {
						pixv = (uint8)(63 - ( y1 & 63 ));
						pixv = (uint8)(pixv+pixv);
						pixv2 = (uint8)( y2 & 63 );
						pixv2 = (uint8)(pixv2+pixv2);
						done = false;
#ifdef AUTO_SHARPEN2
						if ( pixv >= pixv2 ) {
							pixv2 = (uint8)((pixv2+1) >> 1);
							pixv += pixv2;
							if ( pixv > 126 ) {
								pixv2 = (uint8)(pixv2 + pixv - 126);
								pixv = 126;
							}
						} else {
							pixv = (uint8)((pixv+1) >> 1);
							pixv2 += pixv;
							if ( pixv2 > 126 ) {
								pixv += (uint8)(pixv + pixv2 - 126);
								pixv2 = 126;
							}
						}
#endif /* AUTO_SHARPEN2 */
					}
					if ( (center < limA && !endA) || (center > limB && !endB) ) { /* We are in an extended region */
						for (;;) {
							pixo = *ptr ;
							if ( pixo == 0 ) {
								*ptr = (uint8)(0x80 | pixv);
							} else {
								pixo &= 0x7f;
								if ( pixo > pixv ) pixv = pixo;
								*ptr = pixv;
							}
							if ( done ) break; /*****/
							done = true;
							/*
							pixv = (uint8)( y2 & 63 );
							pixv = (uint8)(pixv+pixv);
							*/
							pixv = pixv2;
							for ( ;; ) {
								#ifdef REVERSE_SC_Y_ORDER
									ptr += rowBytes;
								#else
									ptr -= rowBytes;
								#endif
								if ( ptr == ptrB ) break; /*****/
								pixo = *ptr;
								*ptr = (uint8)(pixo == 0 ? (0x80 | 126) : 126);
							}
						}
					
					} else {
						for (;;) {
							pixo = *ptr ;
							if ( pixo >= 128 ) {
								pixo = (uint8)(pixo - 128);
								if ( pixo > pixv ) pixv = pixo;
								*ptr = pixv;
							} else {
								if ( pixv > pixo ) *ptr = pixv;
							}
							if ( done ) break; /*****/
							done = true;
							/*
							pixv = (uint8)( y2 & 63 );
							pixv = (uint8)(pixv+pixv);
							*/
							pixv = pixv2;
							for ( ;; ) {
								#ifdef REVERSE_SC_Y_ORDER
									ptr += rowBytes;
								#else
									ptr -= rowBytes;
								#endif
								if ( ptr == ptrB ) break; /*****/
								*ptr = 126;
							}
						}
					}
				}
				z 		+= yStep;
				center	+=  64;
			} while( center <= lim );
		}
	}
	if ( ( !mostlyX) ) {
		/* Do Y Edges */
		if ( y0 < y2 ) {
			xA = x0; yA = y0; endA = end0;
			xB = x2; yB = y2; endB = end2;
		} else if ( y0 > y2 ) {
			xA = x2; yA = y2; endA = end2;
			xB = x0; yB = y0; endB = end0;
		}

		dist = yB - yA;

		limB = yB;
		limA = (yA + 31) & ~63;
		if ( absDy > absDx ) {
			lim 	= yB + yRadius;
			center	= (yA + 31 - yRadius) & ~63;
		} else {
			lim		= limB;
			center	= limA;
		}
		center += 32;
		/* This code is never intended to be used at high sizes (>700ppem), where dist >= INTEGER_T2K_SC_LIMIT */
		/* For these high sizes we would need to compute: */
		/* x = (xA + util_FixMul( xStep, util_FixDiv( center - yA, dist ) )); */
		assert( dist+yRadius+yRadius < INTEGER_T2K_SC_LIMIT );
		if ( center <= lim ) {
			F26Dot6 z, x, xStep = xB - xA;
			z = xStep * (center - yA); xStep <<= 6;
#ifdef BOUNDS_CHECK_ON
if ( (center>>6) < ymin ) {
	SysBeep(10);
}
if ( (lim>>6) >= ymax ) {
	SysBeep(10);
}
#endif
		
			do {
				x		 = (xA + z / dist);
				
				/* Process ( x, center ) */
				{
					int done;
					long x1, x2;
					x1 = x - xRadius;
					x2 = x + xRadius;

#ifdef BOUNDS_CHECK_ON
if ( (x1>>6) < xmin ) {
	SysBeep(10);
}
if ( (x2>>6) >= xmax ) {
	SysBeep(10);
}
#endif

#ifdef REVERSE_SC_Y_ORDER
					ptr = &baseAddr[( ((center>>6) - ymin) ) * rowBytes - xmin];
#else
					ptr = &baseAddr[( h1 - ((center>>6) - ymin) ) * rowBytes - xmin];
#endif

					x = (x1>>6);
					/* if ( x < xmin ) x = xmin; */
					
					/* (x1>>6) == (x2>>6) */
					if ( ((x1 ^ x2) & (~63)) == 0 ) {
						pixv = (uint8)(x2-x1);
						pixv = (uint8)(pixv+pixv);
						done = true;
					} else {
						pixv = (uint8)(63 - ( x1 & 63 ));
						pixv = (uint8)(pixv+pixv);
						pixv2 = (uint8)( x2 & 63 );
						pixv2 = (uint8)(pixv2+pixv2);
						done = false;
#ifdef AUTO_SHARPEN2
						if ( pixv >= pixv2 ) {
							pixv2 = (uint8)((pixv2+1) >> 1);
							pixv += pixv2;
							if ( pixv > 126 ) {
								pixv2 = (uint8)(pixv2 + pixv - 126);
								pixv = 126;
							}
						} else {
							pixv = (uint8)((pixv+1) >> 1);
							pixv2 += pixv;
							if ( pixv2 > 126 ) {
								pixv = (uint8)(pixv + pixv2 - 126);
								pixv2 = 126;
							}
						}
#endif /* AUTO_SHARPEN2 */
					}
						
					if ( (center < limA && !endA) || (center > limB && !endB) ) { /* We are in an extended region */
						for (;;) {
							pixo = ptr[x] ;
							if ( pixo == 0 ) {
								ptr[x] = (uint8)(0x80 | pixv);
							} else {
								pixo &= 0x7f;
								if ( pixo > pixv ) pixv = pixo;
								ptr[x] = pixv;
							}
							
							if ( done ) break; /*****/
							done = true;
							/*
							pixv = (uint8)( x2 & 63 );
							pixv = (uint8)(pixv+pixv);
							*/
							pixv = pixv2;
							x2 >>= 6;
							for ( x = x+1; x < x2; x++ ) {
								pixo = ptr[x];
								ptr[x] = (uint8)(pixo == 0 ? (0x80 | 126) : 126);
							}
						}
					} else {
						for (;;) {
							pixo = ptr[x] ;
							if ( pixo >= 128 ) {
								pixo = (uint8)(pixo - 128);
								if ( pixo > pixv ) pixv = pixo;
								ptr[x] = pixv;
							} else {
								if ( pixv > pixo ) ptr[x] = pixv;
							}
						
							if ( done ) break; /*****/
							done = true;
							/*
							pixv = (uint8)( x2 & 63 );
							pixv = (uint8)(pixv+pixv);
							*/
							pixv = pixv2;
							x2 >>= 6;
							for ( x = x+1; x < x2; x++ ) {
								ptr[x] = 126;
							}
							
						}
					}
				}
				z 		+= xStep;
				center	+=  64;
			} while( center <= lim );
		}
	}
}

/*
 *
 */
static void DrawCurve( ffStrkConv *t, F26Dot6 x0, F26Dot6 y0, F26Dot6 x1, F26Dot6 y1, F26Dot6 x2, F26Dot6 y2, int end0, int end2, long xRadius, long yRadius )
{
	long dZ, error, count, midX, midY;
	long Arr[16*7], *wp;

	wp = &Arr[0];
	/*
	dX		= ((x0+x2+1)>>1) - ((x0 + x1 + x1 + x2 + 2) >> 2); if ( dX < 0 ) dX = -dX;
	dY		= ((y0+y2+1)>>1) - ((y0 + y1 + y1 + y2 + 2) >> 2); if ( dY < 0 ) dY = -dY;
	*/
	dZ		= ((x0 - x1 - x1 + x2 ) >> 2); if ( dZ < 0 ) dZ = -dZ;
	error 	= dZ;
	dZ		= ((y0 - y1 - y1 + y2 ) >> 2); if ( dZ < 0 ) dZ = -dZ;
	if ( dZ > error ) error = dZ;
	for ( count = 0; error > 8; count++ ) {
		error >>= 2; /* We could solve this directly instead... */
	}
	for (;;) {
		if ( count > 0 ) {
			midX = ((x0 + x1 + x1 + x2 + 2) >> 2);
			midY = ((y0 + y1 + y1 + y2 + 2) >> 2);
			count--;
			/* InnerDrawParabola( t, x0, y0, ((x0+x1+1)>>1), ((y0+y1+1)>>1), midX, midY, count); */
			
			*wp++ = midX;
			*wp++ = midY;
			*wp++ = ((x1+x2+1)>>1);
			*wp++ = ((y1+y2+1)>>1);
			*wp++ = x2;
			*wp++ = y2;
			*wp++ = count;
			/* InnerDrawParabola( t, midX, midY, ((x1+x2+1)>>1), ((y1+y2+1)>>1), x2, y2, count); */
			x1 = ((x0+x1+1)>>1);
			y1 = ((y0+y1+1)>>1);
			x2 = midX;
			y2 = midY;
			continue; /*****/
		} else {
			DrawLine( t, x0, y0, x2, y2, end0, end2 && wp <= Arr, xRadius, yRadius );
			end0 = false;
		}
		if ( wp > Arr ) {
			count	= *(--wp);
			y2 		= *(--wp);
			x2		= *(--wp);
			y1		= *(--wp);
			x1		= *(--wp);
			y0		= *(--wp);
			x0		= *(--wp);
			
			continue; /*****/
		}
		break; /*****/
	}
}

/*
 *
 */
static void ParseStrokes( ffStrkConv *t, long xRadius, long yRadius, char omitBitMap, FF_GetCacheMemoryPtr funcptr, void *theCache, int bitRange255, uint8 *remapBits  )
{
	short ctr, point, lastPoint;
	long xmin, xmax;
	long ymin, ymax;
	register long tmp;
	register long *x = t->x;
	register long *y = t->y;
	uint8 *baseAddr;
	long w, h, rowBytes, N;
	
	lastPoint = t->endPoint[t->numberOfContours-1];
	
	xmin = xmax = x[0];
	ymin = ymax = y[0];
	for ( point = 1; point <= lastPoint; point++ ) {
		if ( (tmp=x[point]) > xmax ) {
			xmax = tmp;
		} else if ( tmp < xmin ) {
			xmin = tmp;
		}
		if ( (tmp=y[point]) > ymax ) {
			ymax = tmp;
		} else if ( tmp < ymin ) {
			ymin = tmp;
		}
	}
	/* We need to expand by radius * 2.0 */
	xmin -= xRadius*2+0;
	xmax += xRadius*2+0;
	ymin -= yRadius*2+0;
	ymax += yRadius*2+0;

	/* Min is inclusive the max is not */
	t->fLeft26Dot6 = xmin;
	t->left		= xmin = ( xmin + 0  ) >> 6;
	t->right	= xmax = ( xmax + 64 + 0 ) >> 6;
	t->fTop26Dot6 = ymax + 64;
	t->top		= ymin = ( ymin + 0  ) >> 6;
	t->bottom	= ymax = ( ymax + 64 + 0 ) >> 6;
	
	t->xmin = xmin;
	t->xmax = xmax;
	t->ymin = ymin;
	t->ymax = ymax;

	w = xmax - xmin;
	h = ymax - ymin;
	
	#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
		rowBytes = (w+3) & ~3;
	#else
		rowBytes = w;
	#endif

	t->rowBytes = rowBytes;
	t->baseAddr = NULL;
	
	if ( omitBitMap ) return; /*****/
	N = rowBytes * h;
	baseAddr = NULL;
	t->internal_baseAddr = false;
	if ( funcptr != NULL ) {
		baseAddr = (unsigned char*) funcptr( theCache, N * sizeof( unsigned char ) ); /* can return NULL */
	}
	if ( baseAddr == NULL ) {
		baseAddr = (unsigned char*) tsi_FastAllocN( t->mem, N * sizeof( unsigned char ), T2K_FB_SC_BITMAP );
		t->internal_baseAddr = true;
	}
	t->baseAddr = baseAddr;
	
	/* Clear the memory. */
	{
		register uint32 *lptr = (uint32*)baseAddr;
		register long i, lim = N >> 2;
		
		for ( i = 0; i < lim; i++ ) {
			lptr[i] = 0L;
		}
		for ( i <<=  2; i < N; i++ ) {
			baseAddr[i] = 0L;
		}
	}
	
	{
		register char *onCurve = t->onCurve;

		for ( ctr = 0; ctr < t->numberOfContours; ctr++ ) {
			F26Dot6 x0, y0, x2, y2;
			int openContour, end0, end2;
			lastPoint = t->endPoint[ctr];
			point = t->startPoint[ctr];
			x0 = x[point];
			y0 = y[point];
			
			openContour = x0 != x[lastPoint] || y0 != y[lastPoint];
			end0 = openContour;
			end2 = false;
			for ( point++; point <= lastPoint; point++ ) {
				x2 = x[point];
				y2 = y[point];
				if ( point >= lastPoint ) end2 = openContour;
				if ( onCurve[point] ) {
					DrawLine( t, x0, y0, x2, y2, end0, end2, xRadius, yRadius );

					x0 = x2;
					y0 = y2;
				} else {
					F26Dot6 x4, y4;
					
					point++;
					x4 = x[point];
					y4 = y[point];
					
					if ( !onCurve[point] ) {
						x4 = (x4 + x2 + 1) >> 1;
						y4 = (y4 + y2 + 1) >> 1;
						point--;
					}
					DrawCurve(  t, x0, y0, x2, y2, x4, y4, end0, end2, xRadius, yRadius );
#ifdef OLD
						x2 = ( x0 + x2 + x2 + x4 + 2 ) >> 2;
						y2 = ( y0 + y2 + y2 + y4 + 2 ) >> 2;
						DrawLine( t, x0, y0, x2, y2, end0, false, xRadius, yRadius );
						DrawLine( t, x2, y2, x4, y4, false, end2, xRadius, yRadius );
#endif
					x0 = x4;
					y0 = y4;
				}
				end0 = false;
			}
		}
	}
	/* Do final processing */
	{
		register long i;
		register uint8 tmp8;
		
		if ( bitRange255 ) {
			/* assumes black is 126 */
			for ( i = 0; i < N; i++ ) {
				tmp8 = baseAddr[i];								/* Read the value */
				if ( tmp8 >= 128 ) {
					baseAddr[i] = 0;							/* Not in the intersection of two strokes, => clear */
				} else {
					tmp8 = (uint8)(tmp8 + tmp8 + (tmp8>>5));	/* Map [0-126] to [0,255] */
					baseAddr[i] = (uint8)tmp8;					/* Write out the remapped value. */
				}
			}
		} else if ( remapBits != NULL ) {
			for ( i = 0; i < N; i++ ) {
				tmp8 = baseAddr[i];								/* Read the value */
				if ( tmp8 >= 128 ) {
					baseAddr[i] = remapBits[ 0 ]; 				/* Not in the intersection of two strokes, => clear */
				} else {
					baseAddr[i] = remapBits[ tmp8 ]; 			/* Read, remap with the lookup table, and write out. */
				}
			
			}
		} else {
			for ( i = 0; i < N; i++ ) {
				if ( baseAddr[i] >= 128 ) {
					baseAddr[i] = 0;							/* Not in the intersection of two strokes, => clear */
				}
			}
		}
	}
}


/*
 * The stroke-converter constructor
 */
ffStrkConv *ff_NewStrkConv( tsiMemObject *mem, short numberOfContours, short *startPtr, short *endPtr,
							long *xPtr, long *yPtr, char *onCurvePtr )
{

	register ffStrkConv *t = (ffStrkConv *)tsi_FastAllocN( mem, sizeof( ffStrkConv ), T2K_FB_SC );
	t->mem = mem;

	t->baseAddr = NULL;

	t->numberOfContours	= numberOfContours;
	t->startPoint		= startPtr;
	t->endPoint			= endPtr;
	
	t->x				= xPtr;
	t->y				= yPtr;
	
	t->onCurve			= onCurvePtr;
	
	return t; /*****/
}

/*
 * This uses approximations to gain SPEED at low sizes, with the idea that the approximations will not be visible at low sizes.
 * Useful sizes may be sizes up to about 36 ppem.
 *
 */
void MakeStrkBits( ffStrkConv *t, char omitBitMap, FF_GetCacheMemoryPtr funcptr, void *theCache, int bitRange255, uint8 *remapBits, long xRadius, long yRadius  )
{
	ParseStrokes( t, xRadius, yRadius, omitBitMap, funcptr, theCache, bitRange255, remapBits  );
}


/*
 * The stroke-converter destructor
 */
void ff_DeleteStrkConv( ffStrkConv *t )
{
	if ( t != NULL ) {
		if ( t->baseAddr != NULL) tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP );
		tsi_FastDeAllocN( t->mem, (char *)t, T2K_FB_SC );
	}
}

#endif /*  ENABLE_STRKCONV */
