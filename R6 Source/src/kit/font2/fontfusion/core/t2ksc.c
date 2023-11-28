/*
 * T2KSC.c
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
#include "t2ksc.h"
#include "util.h"



/* returns t->free */
static T2KInterSectType *AllocFreeNodes( tsiScanConv * t )
{
	register T2KInterSectType *ptr;
	register long N;
	
	if ( t->freeMemBlockMaxCount == 0 ) {
		N = T2K_SC_FIRST_MEM_BLOCK_BUFFER_SIZE;
		t->freeMemBlockMaxCount = T2K_SC_FREE_BLOCK_BUFFER_SIZE;
		t->freeMemBlocks = t->freeMemBlocksBuffer;
		t->freeMemBlockN = 0;
		t->freeMemBlocks[ 0 ] = ptr = t->firstMemBlockBuffer;
	} else {
		assert( t->free == t->freeEnd );
		/* N  = 256 + t->maxXIndex - t->minXIndex + t->maxYIndex - t->minYIndex; */
		N = 256 + ((t->outlineXMax - t->outlineXMin + t->outlineYMax - t->outlineYMin)>>6);
		t->freeMemBlockN++;
		if ( t->freeMemBlockN >= t->freeMemBlockMaxCount ) {
			t->freeMemBlockMaxCount = t->freeMemBlockN << 1;
			if ( t->freeMemBlocks == t->freeMemBlocksBuffer ) {
				int i;
				t->freeMemBlocks = (T2KInterSectType **)tsi_AllocMem( t->mem, t->freeMemBlockMaxCount * sizeof( T2KInterSectType *) );
				for ( i = 0; i < T2K_SC_FREE_BLOCK_BUFFER_SIZE; i++ ) {
					t->freeMemBlocks[i] = t->freeMemBlocksBuffer[i];
				}
			} else {
				t->freeMemBlocks = (T2KInterSectType **)tsi_ReAllocMem( t->mem, t->freeMemBlocks, t->freeMemBlockMaxCount * sizeof( T2KInterSectType *) );
			}
		}
		t->freeMemBlocks[ t->freeMemBlockN ] = ptr = (T2KInterSectType *)tsi_AllocMem( t->mem, N * sizeof( T2KInterSectType) );
	}
	
	t->free 	= ptr;
	t->freeEnd	= ptr + N;
	return ptr;
}

static void FreeAllNodes( tsiScanConv * t )
{
	long i;
	
	/* Skip index zero since it is statically allocated */
	for ( i = 1; i <= t->freeMemBlockN ; i++) {
		tsi_DeAllocMem( t->mem, (char *)t->freeMemBlocks[ i ] );
	}
	if ( t->freeMemBlocks != t->freeMemBlocksBuffer ) tsi_DeAllocMem( t->mem, (char *)t->freeMemBlocks );
	t->freeMemBlockMaxCount  = 0;
}

#ifdef OLD
static T2KInterSectType *GetNewNode( tsiScanConv * t)
{
	T2KInterSectType *newbie = t->free;
	if ( newbie >= t->freeEnd ) {
		AllocFreeNodes( t );
		newbie = t->free;
	}
	t->free++;
	return newbie; /*****/ 
}
#endif


static void ComputeScanBBox( register tsiScanConv *t )
{
	register long i, first, last;
	register long xmin, xmax;
	register long ymin, ymax;
	register long x,y;
	register T2KInterSectType *next, *node = NULL;
	
	xmin = xmax = (t->outlineXMin + t->outlineXMax) >> 1;
	ymin = ymax = (t->outlineYMin + t->outlineYMax) >> 1;
	
	first = t->minYIndex;
	last  = t->maxYIndex;
	while ( first <= last && (node = t->yEdgeHead[first]) == NULL ) {
		first++;
	}
	if ( node != NULL ) {
		y = (first << 6) + 32;
		if ( y < ymin ) ymin = y;
	}
	while ( first <= last && (node = t->yEdgeHead[last]) == NULL ) {
		last--;
	}
	if ( node != NULL ) {
		y = (last << 6) + 32;
		if ( y > ymax ) ymax = y;
	}
	for ( i = first; i <= last; i++ ) {
		node = t->yEdgeHead[i];
		if ( node != NULL ) {
			x = node->coordinate25Dot6_flag1 >> 1;
			if ( x < xmin ) {
				xmin = x;
			}
			while ( (next = (T2KInterSectType *)node->next) != NULL ) {
				node = next;
			}
			x = node->coordinate25Dot6_flag1 >> 1;
			if ( x > xmax ) {
				xmax = x;
			}
		}
	}
	t->minYIndex = first;
	t->maxYIndex = last;
	
	if ( t->doXEdges ) {	
		first = t->minXIndex;
		last  = t->maxXIndex;
		while ( first <= last && (node = t->xEdgeHead[first]) == NULL ) {
			first++;
		}
		if ( node != NULL ) {
			x = (first << 6) + 32;
			if ( x < xmin ) xmin = x;
		}
		while ( first <= last && (node = t->xEdgeHead[last]) == NULL ) {
			last--;
		}
		if ( node != NULL ) {
			x = (last << 6) + 32;
			if ( x > xmax ) xmax = x;
		}
		t->minXIndex = first;
		t->maxXIndex = last;
		
		for ( i = first; i <= last; i++ ) {
			node = t->xEdgeHead[i];
			if ( node != NULL ) {
				y = node->coordinate25Dot6_flag1 >> 1;
				if ( y < ymin ) {
					ymin = y;
				}
				while ( (next = (T2KInterSectType *)node->next) != NULL ) {
					node = next;
				}
				y = node->coordinate25Dot6_flag1 >> 1;
				if ( y > ymax ) {
					ymax = y;
				}
			}
		}
	}

	t->xminSC = xmin;
	t->xmaxSC = xmax;
	t->yminSC = ymin;
	t->ymaxSC = ymax;
	t->scanBBoxIsComputed = true;
}


static void ComputeOutlineBBox( tsiScanConv *t, long *xPtr, long *yPtr, long lastPoint )
{
	register long i, *ptr, min, max, tmp;
	long direction;
	
	ptr = xPtr; /* First do x */
	for ( direction = 0; direction < 2; direction++ ) {
		min = max = ptr[0];
		for ( i = 1; i <= lastPoint; i++ ) {
			tmp = ptr[i];
			if ( tmp > max ) {
				max = tmp;
			} else if ( tmp < min ) {
				min = tmp;
			}
		}
		if ( direction == 0 ) {
			t->outlineXMin = min;
			t->outlineXMax = max;
			ptr = yPtr; /* Then do y */
		} else {
			t->outlineYMin = min;
			t->outlineYMax = max;
		}
	}
}

#ifdef OLD
static void InsertData( tsiScanConv * t, T2KInterSectType **headPtr, int32 coordinate, uint8 flagBit0 )
{
	register T2KInterSectType *node	= *headPtr;
	register T2KInterSectType *newbie; /*	= GetNewNode( t ); */
	register int32	 coordinate25Dot6_flag1;
	register uint32	 utmp32;
	
	utmp32 = (uint32)coordinate;
	utmp32 += utmp32; /* <<= 1 */
	utmp32 |= flagBit0;
	
	coordinate25Dot6_flag1 = (int32)utmp32;
	
	newbie = t->free;
	if ( newbie >= t->freeEnd ) {
		AllocFreeNodes( t );
		newbie = t->free;
	}
	t->free++;
	newbie->coordinate25Dot6_flag1		= coordinate25Dot6_flag1;
	if ( node == NULL ||  node->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
		/* Insert first in the list */
		newbie->next						= node;
		*headPtr							= newbie;
	} else {
		for(;;) {
			register T2KInterSectType *next = node->next;
			if ( next == NULL || next->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
				/* Insert newbie right after node, and before next */
				newbie->next 	= next;
				node->next		= newbie;
				break; /*****/
			}
			node = next;
		}
	}

}
#endif /* OLD */

#define INTEGER_T2K_SC_LIMIT 46340


static void drawLine( register tsiScanConv *t, long x0, long y0, long x2, long y2 )
{
	long xA, xB, yA, yB, center, dist;
	uint8 flagBit0;
	register T2KInterSectType **head;
	register uint32	 utmp32;
#ifdef ENABLE_MORE_TT_COMPATIBILITY
	long distAdd = 0;	
#else /* ENABLE_MORE_TT_COMPATIBILITY */
	/* Ensure we go on odd coordinates to avoid singularities
	   with multiple points at pixelcenters */
	if ( (x0 & 63) == 32 ) x0--;
	if ( (y0 & 63) == 32 ) y0--;
	if ( (x2 & 63) == 32 ) x2--;
	if ( (y2 & 63) == 32 ) y2--;
#endif /* ENABLE_MORE_TT_COMPATIBILITY */
	
    /* Supress insignificant compiler warnings on some compilers. */
    xA = xB = yA = yB = dist = 0;
    flagBit0 = 0;if ( t->doXEdges ) {
	/* Do X Edges */
	if ( x0 < x2 ) {
		xA = (long)x0; yA = (long)y0;
		xB = (long)x2; yB = (long)y2;
		flagBit0 = IS_POS_T2K_EDGE;
		dist = xB - xA;
		center = (xA + 31) & ~63;
		center += 32;
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		if ( ((x0 & 63) == 32) && t->prevX0 < x0 ) {
			center += 64; /* Not a local extrema => skip the first vertex */
		}
#endif
	} else if ( x0 > x2 ) {
		xA = (long)x2; yA = (long)y2;
		xB = (long)x0; yB = (long)y0;
		flagBit0 = 0;
		dist = xB - xA;
		center = (xA + 31) & ~63;
		center += 32;
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		if ( ((x0 & 63) == 32) && t->prevX0 > x0 ) {
			xB--; /* Not a local extrema => skip the first vertex */
		}
#endif
	} else {
		center = 1; xB = 0; dist = 0;
	}
	/* from A to B */

	if ( center <= xB ) {
		head = &(t->xEdgeHead[center>>6]);
		/* if ( dist != 0 ) */ {
			long y, yStep = yB - yA;
			
			if ( t->couldOverflowIntegerMath && (dist >= INTEGER_T2K_SC_LIMIT || yStep >= INTEGER_T2K_SC_LIMIT || yStep <= -INTEGER_T2K_SC_LIMIT) ) {
				/* Numbers too big for simple integer math */
				do {
					/* InsertData( t, head++,  yA + util_FixMul( yStep, util_FixDiv( center - xA, dist ) ), flagBit0 ); */
					utmp32 = (uint32)(yA + util_FixMul( yStep, util_FixDiv( center - xA, dist ) ));
{ /* COPY OF COMMON InsertData() CODE BLOCK, since inline is faster than a function call */
	T2KInterSectType **headPtr = head++;
	register T2KInterSectType *node	= *headPtr;
	register T2KInterSectType *newbie; /*	= GetNewNode( t ); */
	register int32	 coordinate25Dot6_flag1;
	
	utmp32 += utmp32; /* <<= 1 */
	utmp32 |= flagBit0;
	coordinate25Dot6_flag1 = (int32)utmp32;
	if ( (newbie = t->free) >= t->freeEnd ) {
		newbie = AllocFreeNodes( t );
	}
	t->free++;
	newbie->coordinate25Dot6_flag1		= coordinate25Dot6_flag1;
	if ( node == NULL ||  node->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
		/* Insert first in the list */
		newbie->next = node; *headPtr = newbie;
	} else {
		for(;;) {
			register T2KInterSectType *next = (T2KInterSectType *)node->next;
			if ( next == NULL || next->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
				/* Insert newbie right after node, and before next */
				newbie->next = next; node->next = newbie; break; /*****/
			}
			node = next;
		}
	}
}
					center += 64;
				} while ( center <= xB );
			} else {
				/* y = yB * (center - xA) + yA * (xB - center); */
				y = yStep * (center - xA);
				/* y = yA + (yB - yA) * (center - xA) / dist */
				yStep <<= 6;
				do {
					/* InsertData( t, head++,  yA + y / dist, flagBit0 ); */
					utmp32 = (uint32)(yA + y / dist);
{ /* COPY OF COMMON InsertData() CODE BLOCK, since inline is faster than a function call */
	T2KInterSectType **headPtr = head++;
	register T2KInterSectType *node	= *headPtr;
	register T2KInterSectType *newbie; /*	= GetNewNode( t ); */
	register int32	 coordinate25Dot6_flag1;
	
	utmp32 += utmp32; /* <<= 1 */
	utmp32 |= flagBit0;
	coordinate25Dot6_flag1 = (int32)utmp32;
	if ( (newbie = t->free) >= t->freeEnd ) {
		newbie = AllocFreeNodes( t );
	}
	t->free++;
	newbie->coordinate25Dot6_flag1		= coordinate25Dot6_flag1;
	if ( node == NULL ||  node->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
		/* Insert first in the list */
		newbie->next = node; *headPtr = newbie;
	} else {
		for(;;) {
			register T2KInterSectType *next = (T2KInterSectType *)node->next;
			if ( next == NULL || next->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
				/* Insert newbie right after node, and before next */
				newbie->next = next; node->next = newbie; break; /*****/
			}
			node = next;
		}
	}
}
					y		+=  yStep;
					center	+= 64;
				} while ( center <= xB );
			}
		}
#ifdef NOT_NEEDED_ANYMORE		
		else {
			/* InsertData( t, head,  yA, flagBit0 ); */
			utmp32 = (uint32)(yA);
{ /* COPY OF COMMON InsertData() CODE BLOCK, since inline is faster than a function call */
	T2KInterSectType **headPtr = head++;
	register T2KInterSectType *node	= *headPtr;
	register T2KInterSectType *newbie; /*	= GetNewNode( t ); */
	register int32	 coordinate25Dot6_flag1;
	
	utmp32 += utmp32; /* <<= 1 */
	utmp32 |= flagBit0;
	coordinate25Dot6_flag1 = (int32)utmp32;
	if ( (newbie = t->free) >= t->freeEnd ) {
		newbie = AllocFreeNodes( t );
	}
	t->free++;
	newbie->coordinate25Dot6_flag1		= coordinate25Dot6_flag1;
	if ( node == NULL ||  node->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
		/* Insert first in the list */
		newbie->next = node; *headPtr = newbie;
	} else {
		for(;;) {
			register T2KInterSectType *next = (T2KInterSectType *)node->next;
			if ( next == NULL || next->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
				/* Insert newbie right after node, and before next */
				newbie->next = next; node->next = newbie; break; /*****/
			}
			node = next;
		}
	}
}
			;
		}
#endif /* NOT_NEEDED_ANYMORE */

	}
}	
	/* Do Y Edges */
	if ( y0 < y2 ) {
		yA = (long)y0; xA = (long)x0;
		yB = (long)y2; xB = (long)x2;
		flagBit0 = IS_POS_T2K_EDGE;
		dist = yB - yA;
		center = (yA + 31) & ~63;
		center += 32;
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		if ( ((y0 & 63) == 32)  && t->prevY0 < y0  ) {
			center += 64; /* Not a local extrema => skip the first vertex */
		} 
		distAdd =  ( xB < xA ) ? 0 : dist-1;  /* truncate towards black area */
#endif
	} else if ( y0 > y2 ) {
		yA = (long)y2; xA = (long)x2;
		yB = (long)y0; xB = (long)x0;
		flagBit0 = 0;
		dist = yB - yA;
		center = (yA + 31) & ~63;
		center += 32;
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		if ( ((y0 & 63) == 32) && t->prevY0 > y0 ) {
			yB--; /* Not a local extrema => skip the first vertex */
		} 
		distAdd =  ( xB < xA ) ? dist-1 : 0;  /* truncate towards black area */
#endif
	} else {
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		center = (y0 + 31) & ~63;
		center += 32;
		dist = 0;
		distAdd = 0;
#else
		center = 1; yB = 0;
#endif
	}
	/* from A to B */
	
	if ( center <= yB ) {
		head = &(t->yEdgeHead[center>>6]);
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		if ( dist != 0 )
#endif
		{
			long x, xStep = xB - xA;

			if ( t->couldOverflowIntegerMath && (dist >= INTEGER_T2K_SC_LIMIT || xStep >= INTEGER_T2K_SC_LIMIT || xStep <= -INTEGER_T2K_SC_LIMIT) ) {
				/* Numbers too big for simple integer math */
				do {
					/* InsertData( t, head++,  xA + util_FixMul( xStep, util_FixDiv( center - yA, dist ) ), flagBit0 ); */
					utmp32 = (uint32)(xA + util_FixMul( xStep, util_FixDiv( center - yA, dist ) ) );
{ /* COPY OF COMMON InsertData() CODE BLOCK, since inline is faster than a function call */
	T2KInterSectType **headPtr = head++;
	register T2KInterSectType *node	= *headPtr;
	register T2KInterSectType *newbie; /*	= GetNewNode( t ); */
	register int32	 coordinate25Dot6_flag1;
	
	utmp32 += utmp32; /* <<= 1 */
	utmp32 |= flagBit0;
	coordinate25Dot6_flag1 = (int32)utmp32;
	if ( (newbie = t->free) >= t->freeEnd ) {
		newbie = AllocFreeNodes( t );
	}
	t->free++;
	newbie->coordinate25Dot6_flag1		= coordinate25Dot6_flag1;
	if ( node == NULL ||  node->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
		/* Insert first in the list */
		newbie->next = node; *headPtr = newbie;
	} else {
		for(;;) {
			register T2KInterSectType *next = (T2KInterSectType *)node->next;
			if ( next == NULL || next->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
				/* Insert newbie right after node, and before next */
				newbie->next = next; node->next = newbie; break; /*****/
			}
			node = next;
		}
	}
}
					center += 64;
				} while ( center <= yB );
			} else {
				/* x = xB * (center - yA) + xA * (yB  - center); */
				x = xStep * (center - yA);
				xStep <<=  6;
				do {
					/* InsertData( t, head++,  xA + x / dist, flagBit0 ); */
#ifdef ENABLE_MORE_TT_COMPATIBILITY
					if ( x < 0 ) {
						utmp32 = (uint32)(xA - (-x+distAdd)/ dist);
					} else {
						utmp32 = (uint32)(xA + (x+distAdd)/ dist);
					}
#else
					utmp32 = (uint32)(xA + x / dist);
#endif
{ /* COPY OF COMMON InsertData() CODE BLOCK, since inline is faster than a function call */
	T2KInterSectType **headPtr = head++;
	register T2KInterSectType *node	= *headPtr;
	register T2KInterSectType *newbie; /*	= GetNewNode( t ); */
	register int32	 coordinate25Dot6_flag1;
	
	utmp32 += utmp32; /* <<= 1 */
	utmp32 |= flagBit0;
	coordinate25Dot6_flag1 = (int32)utmp32;
	if ( (newbie = t->free) >= t->freeEnd ) {
		newbie = AllocFreeNodes( t );
	}
	t->free++;
	newbie->coordinate25Dot6_flag1		= coordinate25Dot6_flag1;
	if ( node == NULL ||  node->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
		/* Insert first in the list */
		newbie->next = node; *headPtr = newbie;
	} else {
		for(;;) {
			register T2KInterSectType *next = (T2KInterSectType *)node->next;
			if ( next == NULL || next->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
				/* Insert newbie right after node, and before next */
				newbie->next = next; node->next = newbie; break; /*****/
			}
			node = next;
		}
	}
}
					x 		+= xStep;
					center	+= 64;
				} while ( center <= yB );
			}
		}
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		/* dist == 0 here, so y0 == y2, turn on row of pixels exactly on outline here!  */	
		else if ( (y0 & 63) == 32 && x0 != x2 ) {
for ( dist = 0; dist < 2; dist++ ) {
			/* InsertData( t, head,  xA, flagBit0 ); */
			if ( dist == 0 ) {
				utmp32 = (uint32) (x0 < x2 ? x0 : x2 );
				flagBit0 = IS_POS_T2K_EDGE;
			} else {
				utmp32 = (uint32) (x0 > x2 ? x0 : x2 );
				flagBit0 = 0;
			}
{ /* COPY OF COMMON InsertData() CODE BLOCK, since inline is faster than a function call */
	T2KInterSectType **headPtr = head;
	register T2KInterSectType *node	= *headPtr;
	register T2KInterSectType *newbie; /*	= GetNewNode( t ); */
	register int32	 coordinate25Dot6_flag1;
	
	utmp32 += utmp32; /* <<= 1 */
	utmp32 |= flagBit0;
	coordinate25Dot6_flag1 = (int32)utmp32;
	if ( (newbie = t->free) >= t->freeEnd ) {
		newbie = AllocFreeNodes( t );
	}
	t->free++;
	newbie->coordinate25Dot6_flag1		= coordinate25Dot6_flag1;
	if ( node == NULL ||  node->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
		/* Insert first in the list */
		newbie->next = node; *headPtr = newbie;
	} else {
		for(;;) {
			register T2KInterSectType *next = (T2KInterSectType *)node->next;
			if ( next == NULL || next->coordinate25Dot6_flag1 > coordinate25Dot6_flag1 ) {
				/* Insert newbie right after node, and before next */
				newbie->next = next; node->next = newbie; break; /*****/
			}
			node = next;
		}
	}
}

}
		}
#endif /* ENABLE_MORE_TT_COMPATIBILITY */

	}
#ifdef ENABLE_MORE_TT_COMPATIBILITY	
	if ( x0 != x2 ) t->prevX0 = x0;
	if ( y0 != y2 ) t->prevY0 = y0;
#endif
	
	return ; /********/

	
}


static void drawParabola( register tsiScanConv *t, long x0, long y0, long x1, long y1, long x2, long y2 )
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
	dZ = t->maxError;
	for ( count = 0; error > dZ; count++ ) {
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
			drawLine( t, x0, y0, x2, y2 );
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

#ifdef T1_OR_T2_IS_ENABLED

static void draw3rdDegreeBezierInner( register tsiScanConv *t, long x0, long y0, long x1, long y1, long x2, long y2, long x3, long y3, long error )
{
	long dZ, count, midX, midY;
	long Arr[16*9], *wp;

	wp = &Arr[0];
	/* Midpoint 50% of line 0-3 to parametric midpoint 0.5 of the curve. */
	/* dZ = (z0+3*z1+3*z2+z3)/8 - (4*(z0+z3)/8), => */
	dZ		= (( 3*(x1+x2-x0-x3) + 4) >> 3); if ( dZ < 0 ) dZ = -dZ;
	if ( error < dZ ) error = dZ;
	dZ		= (( 3*(y1+y2-y0-y3) + 4) >> 3); if ( dZ < 0 ) dZ = -dZ;
	if ( error < dZ ) error = dZ;
	dZ = t->maxError;
	for ( count = 0; error > dZ; count++ ) {
		error >>= 2; /* We could solve this directly instead... */
	}
	
	
	for (;;) {
		if ( count > 0 ) {
			long xB, yB, xC, yC;
			
			midX = ((x0 + 3*(x1+x2) + x3 + 4) >> 3);
			midY = ((y0 + 3*(y1+y2) + y3 + 4) >> 3);
			count--;
			xC = (x2+x3+1)>>1;
			yC = (y2+y3+1)>>1;
			xB = (xC + ((x1+x2+1)>>1) + 1 ) >> 1;
			yB = (yC + ((y1+y2+1)>>1) + 1 ) >> 1;
			/* mid, B, C, 3  */
			
			*wp++ = midX;
			*wp++ = midY;
			*wp++ = xB;
			*wp++ = yB;
			*wp++ = xC;
			*wp++ = yC;
			*wp++ = x3;
			*wp++ = y3;
			*wp++ = count;

			/* 0, B, C, mid */
			xB = (x0+x1+1)>>1;
			yB = (y0+y1+1)>>1;
			xC = (xB + ((x1+x2+1)>>1) + 1 ) >> 1;
			yC = (yB + ((y1+y2+1)>>1) + 1 ) >> 1;
			
			
			x1 = xB;
			y1 = yB;
			x2 = xC;
			y2 = yC;
			x3 = midX;
			y3 = midY;
			continue; /*****/
		} else {
			drawLine( t, x0, y0, x3, y3 );
		}
		if ( wp > Arr ) {
			count	= *(--wp);
			y3 		= *(--wp);
			x3		= *(--wp);
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

/* Recurse just one level to avoid inflection that could throw off the error and subdivision calculations */
static void draw3rdDegreeBezierOuter( register tsiScanConv *t, long x0, long y0, long x1, long y1, long x2, long y2, long x3, long y3 )
{
	long midX, midY;
	long xB, yB, xC, yC;
	long dZ, error;

	/* Midpoint 50% of line 0-3 to parametric midpoint 0.5 of the curve. */
	/* dZ = (z0+3*z1+3*z2+z3)/8 - (4*(z0+z3)/8), => */
	dZ		= (( 3*(x1+x2-x0-x3) + 4) >> 3); if ( dZ < 0 ) dZ = -dZ;
	error 	= dZ;
	dZ		= (( 3*(y1+y2-y0-y3) + 4) >> 3); if ( dZ < 0 ) dZ = -dZ;
	if ( error < dZ ) error = dZ;
	/* since we are subdividing at least once we can divide the error with 4 */
	error = ( error + 2 ) >> 2;
	
	midX = ((x0 + 3*(x1+x2) + x3 + 4) >> 3);
	midY = ((y0 + 3*(y1+y2) + y3 + 4) >> 3);

	/* 0, B, C, mid */
	xB = (x0+x1+1)>>1;
	yB = (y0+y1+1)>>1;
	xC = (xB + ((x1+x2+1)>>1) + 1 ) >> 1;
	yC = (yB + ((y1+y2+1)>>1) + 1 ) >> 1;
	draw3rdDegreeBezierInner( t, x0, y0, xB, yB, xC, yC, midX, midY, error );

	/* mid, B, C, 3  */
	xC = (x2+x3+1)>>1;
	yC = (y2+y3+1)>>1;
	xB = (xC + ((x1+x2+1)>>1) + 1 ) >> 1;
	yB = (yC + ((y1+y2+1)>>1) + 1 ) >> 1;
	draw3rdDegreeBezierInner( t, midX, midY, xB, yB, xC, yC, x3, y3, error );
}
#endif /* T1_OR_T2_IS_ENABLED */

#ifdef T1_OR_T2_IS_ENABLED
static void Make3rdDegreeEdgeList( register tsiScanConv *t )
{
    int startPoint, lastPoint, ctr;
    int ptA, ptB;
	long Ax, Bx, Ay, By;
	register long *x = t->x;
	register long *y = t->y;
	long pointCount;
	register char *onCurve = t->onCurve;
	
	for ( ctr = 0; ctr < t->numberOfContours; ctr++ ) {
		ptA = startPoint = t->startPoint[ctr];
		lastPoint = t->endPoint[ctr];
		pointCount = lastPoint - startPoint + 1;

#ifdef ENABLE_MORE_TT_COMPATIBILITY	
		ptB = lastPoint;
#endif
		while ( !onCurve[ ptA ] ) {
#ifdef ENABLE_MORE_TT_COMPATIBILITY	
			ptB = ptA;
#endif
			ptA = (ptA + 1);
			assert ( ptA <= lastPoint );
		}
		assert( onCurve[ ptA ] );
		Ax  = x[ ptA ];	Ay  = y[ ptA ];
		
#ifdef ENABLE_MORE_TT_COMPATIBILITY	
		if ( (Ax & 63) == 32 ) {
			t->prevX0 = x[ptB]; /* we only need to set this if the above if statement is true */
			if ( !onCurve[ ptB ] ) {
				x[ ptA ] = --Ax; /* sidestep the singularity */
			} else if ( t->prevX0 == Ax ) {
				if ( ptB != startPoint && onCurve[ ptB-1 ] ) t->prevX0 = x[ptB-1]; 
				if ( t->prevX0 == Ax ) x[ ptA ] = --Ax; /* sidestep the singularity */
			}
		}
		if ( (Ay & 63) == 32 ) {
			t->prevY0 = y[ptB]; /* we only need to set this if the above if statement is true */
			if ( !onCurve[ ptB ] ) {
				y[ ptA ] = --Ay; /* sidestep the singularity */
			} else if ( t->prevY0 == Ay ) {
				if ( ptB != startPoint && onCurve[ ptB-1 ] ) t->prevY0 = y[ptB-1]; 
				if ( t->prevY0 == Ay ) y[ ptA ] = --Ay; /* sidestep the singularity */
			}
		}
#endif /* ENABLE_MORE_TT_COMPATIBILITY */
		
		while ( pointCount > 0) {
			ptB = (ptA + 1);
			if ( ptB > lastPoint ) ptB = startPoint;
			Bx  = x[ ptB ];	By  = y[ ptB ];
			if ( onCurve[ ptB ] ) {
				/* A straight line. */
				drawLine( t, Ax, Ay, Bx, By );
				ptA = ptB; Ax = Bx; Ay = By;
				pointCount--;
			} else {
				long Cx, Dx, Cy, Dy;
				int ptC, ptD;
				/* A 3rd degree bezier. */
				ptC = (ptB + 1);
				if ( ptC > lastPoint ) ptC = startPoint;
				ptD = (ptC + 1);
				if ( ptD > lastPoint ) ptD = startPoint;
				
				assert( !onCurve[ ptC ] );
				assert( onCurve[ ptD ] );
				Cx  = x[ ptC ];	Cy  = y[ ptC ];
				Dx  = x[ ptD ];	Dy  = y[ ptD ];
				draw3rdDegreeBezierOuter( t, Ax, Ay, Bx, By, Cx, Cy, Dx, Dy );
				ptA = ptD; Ax = Dx; Ay = Dy;
				pointCount -= 3;
			}
		}
	}
}
#endif /* T1_OR_T2_IS_ENABLED */


static void Make2ndDegreeEdgeList( register tsiScanConv *t )
{
    int startPoint, lastPoint, ctr;
    int ptA, ptB, ptC;
	long Ax, Bx, Cx, Ay, By, Cy;
	register long *x = t->x;
	register long *y = t->y;
	register char *onCurve = t->onCurve;

	ctr = 0;
SC_startContour:
	while ( ctr < t->numberOfContours ) {
		ptA = startPoint = t->startPoint[ctr];
		lastPoint = t->endPoint[ctr];

		if ( onCurve[ ptA ] ) {
			Ax  = x[ ptA ];	Ay  = y[ ptA ];
			ptB = -1; Bx = By = 0;
#ifdef ENABLE_MORE_TT_COMPATIBILITY	
			if ( (Ax & 63) == 32 ) {
				t->prevX0 = x[lastPoint]; /* we only need to set this if the above if statement is true */
				if ( !onCurve[ lastPoint ] ) {
					x[ ptA ] = --Ax; /* sidestep the singularity */
				} else if ( t->prevX0 == Ax ) {
					if ( lastPoint != startPoint && onCurve[ lastPoint-1 ] ) t->prevX0 = x[lastPoint-1]; 
					if ( t->prevX0 == Ax ) x[ ptA ] = --Ax; /* sidestep the singularity */
				}
			}
			if ( (Ay & 63) == 32 ) {
				t->prevY0 = y[lastPoint]; /* we only need to set this if the above if statement is true */
				if ( !onCurve[ lastPoint ] ) {
					y[ ptA ] = --Ay; /* sidestep the singularity */
				} else if ( t->prevY0 == Ay ) {
					if ( lastPoint != startPoint && onCurve[ lastPoint-1 ] ) t->prevY0 = y[lastPoint-1]; 
					if ( t->prevY0 == Ay ) y[ ptA ] = --Ay; /* sidestep the singularity */
				}
			}
#endif /* ENABLE_MORE_TT_COMPATIBILITY */
		} else {
			Bx  = x[ ptA ]; By  = y[ ptA ];
			ptB = lastPoint;

			Ax  = x[ ptB ]; Ay = y[ ptB ];
			if ( !onCurve[ ptB ] ) {
				Ax = (Ax + Bx + 1) >> 1;
				Ay = (Ay + By + 1) >> 1;
#ifdef ENABLE_MORE_TT_COMPATIBILITY	
				if ( (Ax & 63) == 32 ) {
					Ax--; x[ptA]--;x[ptB]--;/* sidestep the singularity */
				}
				if ( (Ay & 63) == 32 ) {
					Ay--; y[ptA]--;y[ptB]--;/* sidestep the singularity */
				}
#endif /* ENABLE_MORE_TT_COMPATIBILITY */
			}
#ifdef ENABLE_MORE_TT_COMPATIBILITY	
			else {
				if ( (Ax & 63) == 32 ) {
					x[ ptB ] = --Ax; /* sidestep the singularity */
				}
				if ( (Ay & 63) == 32 ) {
					y[ ptB ] = --Ay; /* sidestep the singularity */
				}
			}
#endif /* ENABLE_MORE_TT_COMPATIBILITY */
			ptB = ptA; ptA = lastPoint; /* SWAP A, B */
		}

		for (;;) {
/* SC_AOnBOff: */
			while ( ptB >= 0 ) {	
				ptC = (ptB + 1); /* ptC = NEXTPT( ptB, ctr ); */
				if ( ptC > lastPoint ) ptC = startPoint;
				Cx = x[ ptC ];	Cy = y[ ptC ];
				if ( ! onCurve[ ptC ]) {
					register long tmpX = ((Bx + Cx + 1) >> 1);
					register long tmpY = ((By + Cy + 1) >> 1);
					drawParabola( t, Ax, Ay, Bx, By, tmpX, tmpY );
					if ( ptC == startPoint ) {ctr++; goto SC_startContour;}	/********************** continue SC_startContour */
					Ax = tmpX; Ay = tmpY;
					Bx = Cx; By = Cy; ptA = ptB; ptB = ptC;
					continue; /********************** continue SC_AOnBOff */
				}
				drawParabola( t, Ax, Ay, Bx, By, Cx, Cy );
				if ( ptC == startPoint ) {ctr++; goto SC_startContour;}	/********************** continue SC_startContour */
				ptA = ptC; Ax = Cx; Ay = Cy;
				break; /***********************/
			}
/* SC_AOn:	*/		
			for (;;) {
				ptB = (ptA + 1); /* ptB = NEXTPT( ptA, ctr ); */
				if ( ptB > lastPoint ) ptB = startPoint;
				Bx = x[ ptB ];	By = y[ ptB ];
				if ( onCurve[ ptB ] ) {
					drawLine( t, Ax, Ay, Bx, By );
					if ( ptB == startPoint ) {ctr++; goto SC_startContour;}		/********************** continue SC_startContour */
					ptA = ptB; Ax = Bx; Ay = By;
					continue;  /********************** continue SC_AOn */
				}
				if ( ptB == startPoint ) {ctr++; goto SC_startContour;}			/**********************continue SC_startContour */
				break; /***********************/
			}
		}
	}
} /* end Make2ndDegreeEdgeList() */

/* Changed the error threshold from 4 to 2 for B&W since the letter OCQ in Verdana.ttf 10point at 96dpi had extra pixels. */
#define SUB_DIVISION_THRESHOLD_BW	2
#define SUB_DIVISION_THRESHOLD_GRAY 4

tsiScanConv *tsi_NewScanConv( tsiMemObject *mem, short numberOfContours, short *startPtr, short *endPtr,
				              long *xPtr, long *yPtr, char *onCurvePtr, uint8 greyScaleLevel,
				              char curveType, char xDropOutControl, char yDropOutControl,
				              int smart_droput, int include_stubs, F26Dot6 oneHalfFUnit )
{
	/* register tsiScanConv *t = (tsiScanConv *)tsi_AllocMem( mem, sizeof( tsiScanConv ) ); */
	/* register tsiScanConv *t = buffer; */
	register tsiScanConv *t = (tsiScanConv *)tsi_FastAllocN( mem, sizeof( tsiScanConv ), T2K_FB_SC );
	t->mem = mem;

	t->smartDropout = (char)(smart_droput ? true : false);  /* best quality setting is true  */
	t->includeStubs = (char)(include_stubs ? true : false); /* best quality setting is false */

#ifdef ENABLE_MORE_TT_COMPATIBILITY		
	if ( !t->includeStubs ) {
		/* We need to detect stubs, and we need both directions for that! */
		if ( xDropOutControl || yDropOutControl ) {
			xDropOutControl = yDropOutControl = true;
		}
	}
#endif

	t->greyScaleLevel = greyScaleLevel;

	/* t->maxError = SUB_DIVISION_THRESHOLD; */
	t->maxError = greyScaleLevel > 0 ? SUB_DIVISION_THRESHOLD_GRAY : SUB_DIVISION_THRESHOLD_BW;
	if ( oneHalfFUnit > t->maxError ) t->maxError = oneHalfFUnit;
	if ( t->maxError > 16 ) t->maxError = 16;

	t->xDropOutControl	= xDropOutControl;
	t->yDropOutControl	= yDropOutControl;
	t->doXEdges 		= (char)(yDropOutControl || greyScaleLevel > 0);
	
	t->weDidXDropouts = false;
	t->weDidYDropouts = false;

	t->numberOfContours	= numberOfContours;
	t->startPoint		= startPtr;
	t->endPoint			= endPtr;
	
	t->x				= xPtr;
	t->y				= yPtr;
	
	t->onCurve			= onCurvePtr;
	
	t->scanBBoxIsComputed = false; /* Initialize */

	{
		register long i, min, max, tmp;
		long direction;
		
		
		ComputeOutlineBBox( t, xPtr, yPtr, endPtr[ numberOfContours - 1 ] );
		t->couldOverflowIntegerMath = false;
		if ( (t->outlineXMax - t->outlineXMin) >= INTEGER_T2K_SC_LIMIT ||
			 (t->outlineYMax - t->outlineYMin) >= INTEGER_T2K_SC_LIMIT ) {
			t->couldOverflowIntegerMath = true; 
		}
		
		{
			register T2KInterSectType **base;
		
			max = t->outlineXMax;  /* First do x */
			min = t->outlineXMin; 
			for ( direction = 0; direction < 2; direction++ ) {
				max += 64; max >>= 6;
				min -= 64; min >>= 6;
				tmp		= max - min + 1;
				if ( direction == 0 && !t->doXEdges ) {
					base = NULL;
				} else {
					if ( tmp <= T2K_SC_BASE_BUFFER_SIZE ) {
						base = direction == 0 ? t->xBaseBuffer : t->yBaseBuffer;
					} else {
						base	= (T2KInterSectType **)tsi_AllocMem( mem, tmp * sizeof( T2KInterSectType * ) );
					}
					for ( i = 0; i < tmp; i++ ) {
						base[i] = NULL;
					}
				}
				if ( direction == 0 ) {
					t->xBase		= base;
					t->xEdgeHead	= base != NULL ? &base[-min] : NULL; /* The condition avoids dereferencing a NULL pointer, which is illegal on some platforms even if we never use the results. */
					t->minXIndex	= min;
					t->maxXIndex	= max;
					
					min = t->outlineYMin;   /* Then do y */
					max = t->outlineYMax; 
				} else {
					t->yBase		= base;
					t->yEdgeHead	= &base[-min];
					t->minYIndex	= min;
					t->maxYIndex	= max;
				}
			}
		}
	}
	t->freeMemBlockMaxCount = 0;
	AllocFreeNodes( t );
	
	
	t->baseAddr 	= NULL;

	if ( curveType == 3 ) {
		;
#ifdef T1_OR_T2_IS_ENABLED
		Make3rdDegreeEdgeList( t );
#endif
	} else {
		Make2ndDegreeEdgeList( t );
	}
	return t; /*****/
}

#ifdef ENABLE_MORE_TT_COMPATIBILITY		
/*
 * Returns Min(2, intersectionCount );
 */
static long CountInterSections( register T2KInterSectType *node, long lim1, long lim2, register int count )
{
	if ( node != NULL ) {
		register long x1, x2;
		register T2KInterSectType *next = (T2KInterSectType *)node->next;
		assert( next != NULL );
		lim1 -= 4;
		lim2 += 4;
		do {
			/* draw from node to next */
			x1 = node->coordinate25Dot6_flag1 >> 1;
			x2 = next->coordinate25Dot6_flag1 >> 1;
	
			if ( x1 > lim2 ) 					break; /*****/
			/* Now we know that x1 <= lim2 */
			if ( x1 >= lim1 && ++count >= 2 )	break; /*****/
			if ( x2 > lim2 )					break; /*****/
			/* Now we know that x2 <= lim2 */
			if ( x2 >= lim1 && ++count >= 2 )	break; /*****/
			
			node = (T2KInterSectType *)next->next;
			if ( node == NULL ) 				break; /*****/
			next = (T2KInterSectType *)node->next;
		} while ( next != NULL );
	}
	return count; /*****/
}

static int IsStub(T2KInterSectType **yEdgeHead, T2KInterSectType **xEdgeHead, long x1, long x2, long yB, long ymin, long ymax )
{
	register int count;
	register long yA = yB-1;
	register long yC = yB+1;
	register long yB32 = (yB<<6)+32;
	
	/* assert( CountInterSections( yEdgeHead[yB],  (x1<<6)+32, (x2<<6)+32, 0 ) >= 2 ); */
	
	count  = yC >= ymax >= ymax ? 0 : CountInterSections( yEdgeHead[yC],  (x1<<6)+32, (x2<<6)+32, 0 );
	if ( count < 2 ) {
		count = CountInterSections( xEdgeHead[x1], yB32, (yC<<6)+32, count );
		if ( count < 2 ) {
			count = CountInterSections( xEdgeHead[x2], yB32, (yC<<6)+32, count );
		}
	}
	if ( count < 2 ) return true; /*****/
	
	count  = yA < ymin ? 0 : CountInterSections( yEdgeHead[yA],  (x1<<6)+32, (x2<<6)+32, 0 );
	if ( count < 2 ) {
		count = CountInterSections( xEdgeHead[x1], (yA<<6)+32, yB32, count );
		if ( count < 2 ) {
			count = CountInterSections( xEdgeHead[x2], (yA<<6)+32, yB32, count );
		}
	}
	if ( count < 2 ) return true; /*****/
	
	return false; /**** Not a stub!!! */
}
#endif /* ENABLE_MORE_TT_COMPATIBILITY	*/	

/*
 * 
 */
void MakeBits( register tsiScanConv *t, char xWeightIsOne, char omitBitMap, FF_GetCacheMemoryPtr funcptr, void *theCache,
 				int bitRange255, uint8 *remapBits )
{
	register long i , N;
	long xmin, xmax;
	long ymin, ymax;
	long w, h, rowBytes;
	register unsigned char *baseAddr;
	register uint8 *ptr;
	long xmid, ymid;
	uint8 greyScaleLevel = t->greyScaleLevel;
#ifdef USE_NON_ZERO_WINDING_RULE
	int windingCount;
#endif

	UNUSED(xWeightIsOne);
	if ( !t->scanBBoxIsComputed ) {
		ComputeScanBBox( t );
	} 
	xmin = t->xminSC;
	xmax = t->xmaxSC;
	ymin = t->yminSC;
	ymax = t->ymaxSC;
	xmid = (xmin+xmax) >> 1;
	ymid = (ymin+ymax) >> 1;

	
	/* Min is inclusive the max is not */
	t->fLeft26Dot6 = xmin;
	t->left		= xmin = ( xmin + 0  ) >> 6;
	t->right	= xmax = ( xmax + 64 + 0 ) >> 6;
	t->fTop26Dot6 = ymax + 64;
	t->top		= ymin = ( ymin + 0  ) >> 6;
	t->bottom	= ymax = ( ymax + 64 + 0 ) >> 6;
	
	/* System.out.println("ymin =  " + ymin + " ymax = " + ymax ); */
	/* printf("ymin = %d, ymax = %d\n", ymin, ymax); */


	w = xmax - xmin;
	h = ymax - ymin;
	
	if ( greyScaleLevel == 0 ) {
		#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
			rowBytes = (w+31) / 32;
			rowBytes <<= 2;
		#else
			rowBytes = (w+7) / 8;
		#endif
	} else {
		#ifdef MAKE_SC_ROWBYTES_A_4BYTE_MULTIPLE
			rowBytes = (w+3)/4;
			rowBytes <<= 2;
		#else
			rowBytes = w;
		#endif
	}
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


	{
		register uint32 *lptr = (uint32*)baseAddr;
		register long lim = N >> 2;
		
		for ( i = 0; i < lim; i++ ) {
			lptr[i] = 0L;
		}
		for ( i <<=  2; i < N; i++ ) {
			baseAddr[i] = 0;
		}
	}
	if ( t->minYIndex <= t->maxYIndex && t->maxYIndex >=  ymax  ) {
		/* assert( false ); FIX for color font */
		t->maxYIndex = ymax-1;
	}

	/* Begin Gray scale scan conversion */
	if ( greyScaleLevel > 0 ) {
#ifndef REVERSE_SC_Y_ORDER
		long h1 = h - 1;
#endif
		
		i = t->minYIndex;
#ifdef REVERSE_SC_Y_ORDER
		ptr = &baseAddr[ /* k = */ ( (i - ymin) ) * rowBytes]; /* y == i */
#else
		ptr = &baseAddr[ /* k = */ ( h1 - (i - ymin) ) * rowBytes]; /* y == i */
#endif
		ptr -= xmin; /* so that we do not have to subtract xmin from x1 and x2 below */
#ifdef REVERSE_SC_Y_ORDER
		for ( ; i <= t->maxYIndex; i++, ptr += rowBytes  ) {
#else
		for ( ; i <= t->maxYIndex; i++, ptr -= rowBytes  ) {
#endif
			T2KInterSectType *node, *next;
			long x1, x2, xA, xB;
			
			node = t->yEdgeHead[i];
			if ( node == NULL ) continue; /*****/
			next = (T2KInterSectType *)node->next;
			assert( next != NULL );
#ifdef USE_NON_ZERO_WINDING_RULE
			windingCount = 0;
#endif
			do {
				/* draw from node to next left to right */
				x1 = node->coordinate25Dot6_flag1;
				x2 = next->coordinate25Dot6_flag1;
#ifdef USE_NON_ZERO_WINDING_RULE
				windingCount += (( x1 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				windingCount += (( x2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				while ( windingCount != 0 ) {
					next = (T2KInterSectType *)next->next;
					x2 = next->coordinate25Dot6_flag1;
					windingCount += (( x2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				}
#endif				
				x1 >>= 1; xA = x1; x1 >>= 6; 
				x2 >>= 1; xB = x2; x2 >>= 6; 
				if ( x1 == x2 ) {
					ptr[x1] = (unsigned char)(ptr[x1] + ((((xB & 63) - (xA & 63))) << 1));
				} else {
					ptr[x1] = (unsigned char)(ptr[x1] + (((63 - (xA & 63)) ) << 1));
					x1++;
					if ( x1 < x2 ) {
						/* if ( x1 & 1 ) ptr[x1++] = (unsigned char)126; */
#ifdef MY_CPU_WRITES_WORDS_SLOW
						for ( ; x1 < x2; x1++ ) {
							ptr[x1] = (unsigned char)126;
						}
#else
						if ( (long)&ptr[x1] & 1 ) ptr[x1++] = (unsigned char)126;
						for ( ; x1 < x2; x1 += 2 ) {
							*((uint16 *)&ptr[x1]) = (uint16)( (126 << 8) | 126 ); /* 126 + 126 */
						}
#endif
					}
					ptr[x2] = (unsigned char)((((xB & 63))) << 1);
				}
				node = (T2KInterSectType *)next->next;
				if( node == NULL ) break; /*****/
				next = (T2KInterSectType *)node->next;
			} while ( next != NULL );
		}
		for ( i = t->minXIndex; i <= t->maxXIndex; i++  ) {
			T2KInterSectType *node, *next;
			long y1, y2, x, k, yA, yB;
			uint8 otherSample, thisSample = 0;
			uint8 *otherPtr = NULL;
			
			node = t->xEdgeHead[i];
			if ( node == NULL ) continue; /*****/
			next = (T2KInterSectType *)node->next;
			assert( next != NULL );
#ifdef USE_NON_ZERO_WINDING_RULE
			windingCount = 0;
#endif
			do {
				/* draw from node to next bottom to top */
				y1 = node->coordinate25Dot6_flag1;
				y2 = next->coordinate25Dot6_flag1;
#ifdef USE_NON_ZERO_WINDING_RULE
				windingCount += (( y1 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				windingCount += (( y2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				while ( windingCount != 0 ) {
					next = (T2KInterSectType *)next->next;
					y2 = next->coordinate25Dot6_flag1;
					windingCount += (( y2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				}
#endif				
				y1 >>= 1; yA = y1; y1 >>= 6;
				y2 >>= 1; yB = y2; y2 >>= 6;
				
				x  = i; x -= xmin;
#ifdef REVERSE_SC_Y_ORDER
				k = ( (y1 - ymin) ) * rowBytes + x; /* y = y1; */
#else
				k = ( h1 - (y1 - ymin) ) * rowBytes + x; /* y = y1; */
#endif
				
					if ( otherPtr != NULL && otherPtr != &baseAddr[k] ) {
						otherSample = *otherPtr;
						{
							long w1 = 63 - thisSample;
							long w2 = 63 - otherSample;
							w1 = w1 * w1; 	w2 = w2 * w2;
							w1 = 3970 - w1; w2 = 3970 - w2;
							*otherPtr = (unsigned char)( (w1 * thisSample + w2 * otherSample) /( w1 + w2) );
						}
						thisSample = 0;
					}
				
				if ( y1 == y2 ) {
					/* baseAddr[k] = (unsigned char)(baseAddr[k] + ((((yB & 63) - (yA & 63)) * fac ) >> 6)); */
					thisSample = (unsigned char)(thisSample + ((((yB & 63) - (yA & 63)) ) << 1));
					otherPtr = &baseAddr[k];
				} else {
					/* baseAddr[k] = (unsigned char)(baseAddr[k] + (((64 - (yA & 63)) * fac ) >> 6)); */			
					thisSample = (unsigned char)(thisSample + (((63 - (yA & 63)) ) << 1));			
					otherPtr = &baseAddr[k];
					
					
					{
						otherSample = *otherPtr;
						{
							long w1 = 63 - thisSample;
							long w2 = 63 - otherSample;
							w1 = w1 * w1; 	w2 = w2 * w2;
							w1 = 3970 - w1; w2 = 3970 - w2;
							*otherPtr = (unsigned char)( (w1 * thisSample + w2 * otherSample) /( w1 + w2) );
						}
						thisSample = 0;
					}
					
					/* k2  = ( h1 - (y2 - ymin) ) * rowBytes + x; */
#ifdef REVERSE_SC_Y_ORDER
					x += ( (y2 - ymin) ) * rowBytes; /* use x as k2 */ /* y  = y2; */
#else
					x += ( h1 - (y2 - ymin) ) * rowBytes; /* use x as k2 */ /* y  = y2; */
#endif
					thisSample = (unsigned char)(thisSample + ((((yB & 63)) ) << 1));
					otherPtr = &baseAddr[x];
				}
				node = (T2KInterSectType *)next->next;
				if( node == NULL ) break; /*****/
				next = (T2KInterSectType *)node->next;
			} while ( next != NULL );
					if ( otherPtr != NULL ) {
						otherSample = *otherPtr;
						{
							long w1 = 63 - thisSample;
							long w2 = 63 - otherSample;
							w1 = w1 * w1; 	w2 = w2 * w2;
							w1 = 3970 - w1; w2 = 3970 - w2;
							*otherPtr = (unsigned char)( (w1 * thisSample + w2 * otherSample) /( w1 + w2) );
						}
					}
		}
		if ( bitRange255 ) {
			/* Assumes T2K_BLACK_VALUE == 126 */
			for ( i = 0; i < N; i++ ) {
				register int tmp8 = baseAddr[i];	/* Read the value */
				tmp8 = tmp8 + tmp8 + (tmp8>>5);		/* Map [0-126] to [0,255] */
				baseAddr[i] = (uint8)tmp8;			/* Write out the remapped value. */
			}
		} else if ( remapBits != NULL ) {
			for ( i = 0; i < N; i++ ) {
				baseAddr[i] = remapBits[ baseAddr[i] ]; /* Read, remap with the lookup table, and write out. */
			}
		}
		return; /*****/
	}
	
	
	/* Begin monchrome scan conversion */
	i = t->maxYIndex;
	
#ifdef REVERSE_SC_Y_ORDER
	/* h-1 - normal */
	ptr = &baseAddr[ /* k = */ ( (i - ymin) ) * rowBytes ]; /* y == i */
	for ( ; i >= t->minYIndex; i--, ptr -= rowBytes  ) { /* minYIndex maxYIndex */
#else
	ptr = &baseAddr[ /* k = */ ( h - 1 - (i - ymin) ) * rowBytes ]; /* y == i */
	for ( ; i >= t->minYIndex; i--, ptr += rowBytes  ) { /* minYIndex maxYIndex */
#endif
		register T2KInterSectType *node, *next;
		register long x1, x2, b1, b2;


		node = t->yEdgeHead[i];
		if ( node == NULL ) continue; /*****/
		next = (T2KInterSectType *)node->next;
		assert( next != NULL );
#ifdef USE_NON_ZERO_WINDING_RULE
		windingCount = 0;
#endif
		do {
			/* draw from node to next */
			/*
			x1 = node->coordinate25Dot6_flag1 >> 1;
			x2 = next->coordinate25Dot6_flag1 >> 1;
			x1 = (x1 + 32) >> 6;
			x2 = (x2 + 32) >> 6;
			*/
			x1 = node->coordinate25Dot6_flag1;
			x2 = next->coordinate25Dot6_flag1;
#ifdef USE_NON_ZERO_WINDING_RULE
			windingCount += (( x1 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
			windingCount += (( x2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
			while ( windingCount != 0 ) {
				next = (T2KInterSectType *)next->next;
				x2 = next->coordinate25Dot6_flag1;
				windingCount += (( x2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
			}
#endif				
			x1 = (x1 + 62) >> 7; /* 62 so that we include the left  side if on a pixel center */
			x2 = (x2 + 64) >> 7; /* 64 so that we include the right side if on a pixel center */
			
			if ( x1 < x2 ) {
				x1 -= xmin;
				x2 -= (xmin + 1);/* Make x2 inclusive */
				b1 = x1 >> 3;
				b2 = x2 >> 3; 
				
				/* assert( k + b1 >= 0 );assert( k + b2 < N ); */
				
				if ( b1 == b2 ) {
					ptr[ b1 ] |= (unsigned char)(0x00ff >> (x1 & 0x07)) & (unsigned char)(0xff80 >> (x2 & 0x07));
				} else {
					ptr[ b1 ] |= (unsigned char)(0x00ff >> (x1 & 0x07));
					b1++;
#ifdef MY_CPU_WRITES_WORDS_SLOW
					for ( ; b1 < b2; b1++ ) {
						ptr[ b1 ] = (unsigned char)0xff;
					}
#else
					if ( ((long)&ptr[b1]) & 1 ) ptr[b1++] = (unsigned char)0xff;
					for ( ; b1 < b2; b1 += 2 ) {
						*((uint16 *)&ptr[b1]) = (uint16)( 0xffff ); /* 2 0xff bytes*/
					}
#endif					
					ptr[ b2 ] = (unsigned char)(0xff80 >> (x2 & 0x07));
				}
			}
			node = (T2KInterSectType *)next->next;
			if( node == NULL ) break; /*****/
			next = (T2KInterSectType *)node->next;
		} while ( next != NULL );
	}
	

	/* x dropout control */
	if ( t->xDropOutControl ) {
		register int weDidXDropouts = false;
		/* old: for ( i = t->minYIndex; i <= t->maxYIndex; i++  ) */
		/* new: for ( i = t->minYIndex + 1; i < t->maxYIndex; i++  )  ) */
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		for ( i = t->minYIndex; i <= t->maxYIndex; i++  )
#else
		for ( i = t->minYIndex+1; i < t->maxYIndex; i++  )
#endif
		{
			T2KInterSectType *node, *next;
			long x1, x2, y, x, k;
			
			node = t->yEdgeHead[i];
			if ( node == NULL ) continue; /*****/
			next = (T2KInterSectType *)node->next;
			if ( next == NULL ) continue; /*****/
#ifdef USE_NON_ZERO_WINDING_RULE
			windingCount = 0;
#endif
			do {
				/* draw from node to next */
				x1 = node->coordinate25Dot6_flag1;
				x2 = next->coordinate25Dot6_flag1;
#ifdef USE_NON_ZERO_WINDING_RULE
				windingCount += (( x1 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				windingCount += (( x2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				while ( windingCount != 0 ) {
					next = (T2KInterSectType *)next->next;
					x2 = next->coordinate25Dot6_flag1;
					windingCount += (( x2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				}
#endif				
				x1 >>=1;
				x2 >>=1;				
				if ( (x2 - x1) < 64 ) {
					y = i;
#ifdef OLD
					x1 = (x1 + 0) >> 6;
					x2 = (x2 + 0) >> 6;
					x1 -= xmin;
					x2 -= xmin;
#endif
					x1 = (x1 + x2 - 64) >> 7;
				    x1 -= xmin;
				    x2 = x1 + 1;
				    if ( x1 < 0 )	x1 = 0;
				    if ( x2 >= w )	x2 = w-1;
#ifdef REVERSE_SC_Y_ORDER
					k = (  (y - ymin) ) * rowBytes;
#else
					k = ( h - 1 - (y - ymin) ) * rowBytes;
#endif
					if ( (baseAddr[k + (x1>>3)] & (unsigned char)( 0x80 >> (x1 & 0x07))) == 0 && 
					     (baseAddr[k + (x2>>3)] & (unsigned char)( 0x80 >> (x2 & 0x07))) == 0 ) {
						/* dropout */
						x  = (node->coordinate25Dot6_flag1 >> 1);
						x2 = (next->coordinate25Dot6_flag1 >> 1);
#ifdef ENABLE_MORE_TT_COMPATIBILITY		
						if ( t->includeStubs || !IsStub(t->yEdgeHead, t->xEdgeHead, ((x - 32) >> 6), ((x2 + 32) >> 6),  y, ymin, ymax ) ) {
							if ( t->smartDropout ) {
#endif
								if ( x > xmid ) { /* create a slight preference to go towards the center */
									x = (x + x2 - 2)>>1;
								} else {
									x = (x + x2 + 1)>>1;
								}
#ifdef ENABLE_MORE_TT_COMPATIBILITY		
							}
#endif
							x = (x+0) >> 6;
							x -= xmin;
							/* System.out.println("x-drop at " + x + "," + y ); */
							baseAddr[k + (x>>3)] |= (unsigned char)( 0x80 >> (x & 0x07) );
							weDidXDropouts = true;
#ifdef ENABLE_MORE_TT_COMPATIBILITY		
						}
#endif
					}
				} 
				node = (T2KInterSectType *)next->next;
				if( node == NULL ) break; /*****/
				next = (T2KInterSectType *)node->next;
			} while ( next != NULL );
		}
		t->weDidXDropouts |= weDidXDropouts;
	}

	/* y dropout control */
	if ( t->yDropOutControl ) {
		register int weDidYDropouts = false;
		/* old: for ( i = t->minXIndex ; i <= t->maxXIndex ; i++  ) */
		/* new: for ( i = t->minXIndex+1 ; i < t->maxXIndex ; i++  ) */
#ifdef ENABLE_MORE_TT_COMPATIBILITY
		for ( i = t->minXIndex; i <= t->maxXIndex; i++  )
#else
		for ( i = t->minXIndex+1; i < t->maxXIndex; i++  )
#endif
		{
			T2KInterSectType *node, *next;
			long y1, y2, y, x, k1, k2;
			
			node = t->xEdgeHead[i];
			if ( node == NULL ) continue; /*****/
			next = (T2KInterSectType *)node->next;
			if ( next == NULL ) continue; /*****/
#ifdef USE_NON_ZERO_WINDING_RULE
			windingCount = 0;
#endif
			do {
				/* draw from node to next */
				y1 = node->coordinate25Dot6_flag1;
				y2 = next->coordinate25Dot6_flag1;
#ifdef USE_NON_ZERO_WINDING_RULE
				windingCount += (( y1 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				windingCount += (( y2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				while ( windingCount != 0 ) {
					next = (T2KInterSectType *)next->next;
					y2 = next->coordinate25Dot6_flag1;
					windingCount += (( y2 & IS_POS_T2K_EDGE ) << 1) - 1; /* +- one */
				}
#endif				
				y1 >>=1;
				y2 >>=1;				
				if ( (y2 - y1) < 64 ) {
					x = i;
					x -= xmin;
#ifdef OLD
					y1 = (y1 + 0) >> 6;
					y2 = (y2 + 0) >> 6;
#endif
					
					y1 = (y1 + y2 - 64) >> 7;
					y2 = y1 + 1;
					if ( y1 < ymin )  y1 = ymin;
					if ( y2 >= ymax ) y2 = ymax-1;
									    
#ifdef REVERSE_SC_Y_ORDER
					k1 = (  (y1 - ymin) ) * rowBytes;
					k2 = (  (y2 - ymin) ) * rowBytes;
#else					
					k1 = ( h - 1 - (y1 - ymin) ) * rowBytes;
					k2 = ( h - 1 - (y2 - ymin) ) * rowBytes;
#endif
					if ( (baseAddr[k1 + (x>>3)] & (unsigned char)( 0x80 >> (x & 0x07))) == 0 && 
					     (baseAddr[k2 + (x>>3)] & (unsigned char)( 0x80 >> (x & 0x07))) == 0 ) {
						/* dropout */
						y  = (node->coordinate25Dot6_flag1 >> 1);
						y2 = (next->coordinate25Dot6_flag1 >> 1);
#ifdef ENABLE_MORE_TT_COMPATIBILITY		
						if ( t->includeStubs || !IsStub(t->xEdgeHead, t->yEdgeHead, ((y - 32) >> 6), ((y2 + 32) >> 6), x+xmin, xmin, xmax ) ) {
							if ( t->smartDropout ) {
#endif
								if ( y > ymid ) { /* create a slight preference to go towards the center */
									y = (y + y2 - 2)>>1;
								} else {
									y = (y + y2 + 1)>>1;
								}
#ifdef ENABLE_MORE_TT_COMPATIBILITY		
							}
#endif
							y = (y+0) >> 6;
							/* System.out.println("y-drop at " + x + "," + y ); */
#ifdef REVERSE_SC_Y_ORDER
							k1 = ( (y - ymin) ) * rowBytes;
#else					
							k1 = ( h - 1 - (y - ymin) ) * rowBytes;
#endif
							baseAddr[k1 + (x>>3)] |= (unsigned char)( 0x80 >> (x & 0x07) );
							weDidYDropouts = true;
#ifdef ENABLE_MORE_TT_COMPATIBILITY		
						}
#endif
					}
				} 
				node = (T2KInterSectType *)next->next;
				if( node == NULL ) break; /*****/
				next = (T2KInterSectType *)node->next;
			} while ( next != NULL );
		}
		t->weDidYDropouts |= weDidYDropouts;
	}
}


void tsi_DeleteScanConv( tsiScanConv *t )
{
	if ( t != NULL ) {
		if ( t->baseAddr != NULL) tsi_FastDeAllocN( t->mem, t->baseAddr, T2K_FB_SC_BITMAP );
		FreeAllNodes( t );
		if ( t->xBase != t->xBaseBuffer ) tsi_DeAllocMem( t->mem, (char *)t->xBase );
		if ( t->yBase != t->yBaseBuffer ) tsi_DeAllocMem( t->mem, (char *)t->yBase );
		tsi_FastDeAllocN( t->mem, (char *)t, T2K_FB_SC );
	}
}







/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2ksc.c 1.16 2000/11/02 21:57:37 reggers Exp $
 *                                                                           *
 *     $Log: t2ksc.c $
 *     Revision 1.16  2000/11/02 21:57:37  reggers
 *     Silence insignificant warnings.
 *     Revision 1.15  2000/06/13 14:44:30  reggers
 *     Another casting repair after Borland STRICT fix.
 *     Revision 1.14  2000/05/11 13:36:11  reggers
 *     Improved logic for decide how to break down qubics into
 *     straight-line segments. (Sampo)
 *     Revision 1.13  2000/03/24 20:54:57  reggers
 *     Changed 6 int's to long's in MakeBits for systems with 2 byte integers
 *     Revision 1.12  2000/02/25 17:46:15  reggers
 *     STRICT warning cleanup.
 *     Revision 1.11  1999/12/23 21:09:15  reggers
 *     MORE_COMPATIBILITY fixes.
 *     Revision 1.10  1999/12/10 16:50:40  reggers
 *     Fixed new scan converter bugs.
 *     Revision 1.9  1999/12/09 21:17:38  reggers
 *     Sampo: multiple TrueType compatibility enhancements (scan convereter).
 *     Revision 1.8  1999/11/17 21:34:49  reggers
 *     more pixels at pure black (@=126), and fewer with near-white
 *     Revision 1.7  1999/10/19 16:26:00  shawn
 *     Added '#ifndef REVERSE_SC_Y_ORDER'.
 *     
 *     Revision 1.6  1999/10/18 17:01:48  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.5  1999/09/30 15:10:45  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.4  1999/09/20 18:31:44  reggers
 *     Changes for MY_CPU_WRITES_WORDS_SLOW.
 *     Revision 1.2  1999/05/17 15:58:25  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

