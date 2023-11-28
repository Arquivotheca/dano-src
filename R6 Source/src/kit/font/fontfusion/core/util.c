/*
 * UTIL.C
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

#include "dtypes.h"
#include "config.h"
#include "tsimem.h"
#include "util.h"



#if 0
/*
 * THIS is ONLY demo code. Do not use!!!
 * You do not want these asserts hitting in a real release.
 */
unsigned char *ReadFileIntoMemory( tsiMemObject *mem, const char *fname, unsigned long *size )
{
	unsigned char *dataPtr;
	int error;
	size_t count;
	FILE *fp;
	
	assert( fname != NULL );
	fp		= fopen(fname, "rb"); assert( fp != NULL );
	error	= fseek( fp, 0L, SEEK_END ); assert( error == 0 );
	*size	= (unsigned long)ftell( fp ); assert( ferror(fp) == 0 );
	error	= fseek( fp, 0L, SEEK_SET ); assert( error == 0 ); /* rewind */
	dataPtr	= (unsigned char *)tsi_AllocMem( mem, sizeof( char ) * *size ); assert( dataPtr != NULL );
	count	= fread( dataPtr, sizeof( char ), *size, fp ); assert( ferror(fp) == 0 && count == *size );
	error	= fclose( fp ); assert( error == 0 );
	return	dataPtr; /******/
}
#endif
#ifdef ENABLE_WRITE
/*
 *  THIS is ONLY demo code. Do not use!!!
 *  You do not want these asserts hitting in a real release.
 */
void WriteDataToFile( const char *fname, void *dataIn, unsigned long size )
{
	int error;
	size_t count;
	FILE *fp	= fopen( fname, "wb" ); assert( fp != NULL );
	count		= fwrite( dataIn, sizeof( char ), size, fp ); assert( ferror(fp) == 0 && count == size );
	error		= fclose( fp ); assert( error == 0 );
}
#endif


/*
 * Slow Bubbles everywhere, do not use for large n values
 */
void util_SortShortArray( short *a, long n )
{
	register short change, tmp;
	register long i;
	
	n--;
	for ( change = 1; change; ) {
		change = 0;
		for ( i = 0; i < n; i++ ) {
			if ( a[i+1] < a[i] ) {
				change = 1;
				/* swap */
				tmp = a[i];
				a[i] = a[i+1];
				a[i+1] = tmp;
			}
		}
	}
}




/* 
 * Return mA * mB
 * This is an integer implementation!
 */
F16Dot16 util_FixMul( F16Dot16 mA, F16Dot16 mB )
{
	uint16 mA_Hi, mA_Lo;
	uint16 mB_Hi, mB_Lo;
	uint32 d1, d2, d3;
	F16Dot16 result;
	int sign;
	
	
	if ( mA < 0 ) {
		mA = -mA;
		sign = -1;
		if ( mB < 0 ) {
			sign = 1;
			mB = -mB;
		}
	} else {
		sign = 1;
		if ( mB < 0 ) {
			sign = -1;
			mB = -mB;
		}
	}
	/*
	result = (F16Dot16)((float)mA/65536.0 * (float)mB);
	result *= sign;
	return result;
	*/

	mA_Hi = (uint16)(mA>>16);
	mA_Lo = (uint16)(mA);
	mB_Hi = (uint16)(mB>>16);
	mB_Lo = (uint16)(mB);
	
	/*
			mB_Hi 	mB_Lo
	  X		mA_Hi 	mA_Lo
	------------------
	d1		d2		d3
	*/
	d3  = (uint32)mA_Lo * mB_Lo;		/* <<  0 */
	d2  = (uint32)mA_Lo * mB_Hi;		/* << 16 */
	d2 += (uint32)mA_Hi * mB_Lo;				/* << 16 */
	d1  = (uint32)mA_Hi * mB_Hi;		/* << 32 */
	
	result 	 = (F16Dot16)((d1 << 16) + d2 + (d3 >> 16));
	result	*= sign;
	return result; /*****/
}

/* 
 * Return mA * mB
 * This is an integer implementation!
 */
F16Dot16 util_FixMul26Dot6( F16Dot16 mA, F16Dot16 mB )
{
	uint16 mA_Hi, mA_Lo;
	uint16 mB_Hi, mB_Lo;
	uint32 d1, d2, d3;
	F16Dot16 result;
	int sign;
	
	
	if ( mA < 0 ) {
		mA = -mA;
		sign = -1;
		if ( mB < 0 ) {
			sign = 1;
			mB = -mB;
		}
	} else {
		sign = 1;
		if ( mB < 0 ) {
			sign = -1;
			mB = -mB;
		}
	}
	/*
	result = (F16Dot16)((float)mA/65536.0 * (float)mB);
	result *= sign;
	return result;
	*/

	mA_Hi = (uint16)(mA>>16);
	mA_Lo = (uint16)(mA);
	mB_Hi = (uint16)(mB>>16);
	mB_Lo = (uint16)(mB);
	
	/*
			mB_Hi 	mB_Lo
	  X		mA_Hi 	mA_Lo
	------------------
	d1		d2		d3
	*/
	d3  = (uint32)(mA_Lo * mB_Lo);		/* <<  0 */
	d2  = (uint32)(mA_Lo * mB_Hi);		/* << 16 */
	d2 += mA_Hi * mB_Lo;				/* << 16 */
	d1  = (uint32)(mA_Hi * mB_Hi);		/* << 32 */
	
	/* shift up by 10 since mB is 26.6 and not 16.16 */
	result 	 = (F16Dot16)((d1 << (16+10)) + (d2<<10) + (d3 >> (16-10)));
	result	*= sign;
	return result; /*****/
}



#ifdef OLD
static long ShortFracMulTest(long a, short b)
{
        int negative = 0;
        uint16 al, ah;
        uint32 lowlong, midlong, hilong;

        if (a < 0) { a = -a; negative = 1; }
        if (b < 0) { b = (short)-b; negative ^= 1; }

        al = (uint16)a; ah = (uint16)(a >> 16);

        midlong = (unsigned long)(ah * b);
        hilong = midlong & 0xFFFF0000;
        midlong <<= 16;
        midlong += 1 << 13;
        lowlong = (al * b) + midlong;
        if (lowlong < midlong)
                hilong += 0x10000;

        midlong = (lowlong >> 14) | (hilong << 2);
        return (long)(negative ? -(int32)midlong : midlong);
}
#endif


/* 
 * Return mA * mB
 * This is an integer implementation!
 * mA is an integer
 * mB is 2.14
 */
long util_ShortFracMul( long mA, short mB )
{
	uint16 mA_Hi, mA_Lo;
	uint16 mB_Lo;
	uint32 d2, d3;
	F16Dot16 result;
	int sign;
	
	/* result2 = ShortFracMulTest(mA, mB ); */
	
	mB_Lo = (uint16)(mB);
	if ( mA < 0 ) {
		mA = -mA;
		sign = -1;
		if ( mB < 0 ) {
			sign = 1;
			mB_Lo = (uint16)-mB;
		}
	} else {
		sign = 1;
		if ( mB < 0 ) {
			sign = -1;
			mB_Lo = (uint16)-mB;
		}
	}

	mA_Hi = (uint16)(mA>>16);
	mA_Lo = (uint16)(mA);
	/* mB_Hi = (uint16)(mB>>16); mB_Hi = 0 */
	
	/*
	   		mA_Hi 	mA_Lo
	  X		0    	mB_Lo
	------------------
	d1		d2		d3
	*/
	d3  = (uint32)(mA_Lo * mB_Lo);		/* <<  0 */
	d2  = (uint32)mA_Hi * mB_Lo;				/* << 16 */
	/* d1 == 0 */
	
	
	result 	 = (F16Dot16)((d2<<2) + ((d3+8192) >> (16-2))); /* shift up by 2 since mB is 2.14 and not 0.16 */
	result	*= sign;

	/* assert ( result2 == result || result2-1 == result || result2+1 == result ); */
	return result; /*****/
}






/* 
 * Return mA / mB
 * This is an integer implementation!
 * It is actually 32% faster than the old floating point implementation on even a 604 PPC !!! :-)
 */
F16Dot16 util_FixDiv( F16Dot16 mA, F16Dot16 mB )
{
#ifdef NEWWAY
/* this is not activated yet: needs extensive testing of delta hinted fonts RJE 10/25/00 */
	int sign;
	uint32 high16, low16;
	F16Dot16 Q;
	uint32 tmp16;
	
	if ( mA < 0 ) {
		mA = -mA;
		sign = -1;
		if ( mB < 0 ) {
			sign = 1;
			mB = -mB;
		}
	} else {
		sign = 1;
		if ( mB < 0 ) {
			sign = -1;
			mB = -mB;
		}
	}
	
	high16 = (uint32)mA / (uint32)mB;
	low16  = (uint32)mA % (uint32)mB;
	/* assert( high16 * mB + low16 == mA ); */
	/* mA / mB = high16 + low16/mB :-) !!! */
	high16 <<= 16;
	while ( low16 > 0xffff ) {
		low16 >>= 1;
		mB >>= 1;
		/* mB is always > low16, and low16 > 0xffff/2 => mB is never zero because of this loop */
	}
	low16  <<= 16;
	tmp16 = low16;
	low16 = tmp16 / mB;
    tmp16 = tmp16 % mB;
    if ((tmp16+tmp16) > (uint32)mB ) {
    	low16++;
    }	
	Q = (F16Dot16)(high16 + low16);
	Q *= sign;
	return Q; /*****/
#else /* Original way: slight rounding error */
	int sign;
	uint32 high16, low16;
	F16Dot16 Q;

	if ( mA < 0 ) {
		mA = -mA;
		sign = -1;
		if ( mB < 0 ) {
			sign = 1;
			mB = -mB;
		}
	} else {
		sign = 1;
		if ( mB < 0 ) {
			sign = -1;
			mB = -mB;
		}
	}

	high16 = (uint32)mA / (uint32)mB;
	low16  = (uint32)mA % (uint32)mB;
	/* assert( high16 * mB + low16 == mA ); */
	/* mA / mB = high16 + low16/mB :-) !!! */
	high16 <<= 16;
	while ( low16 > 0xffff ) {
		low16 >>= 1;
		mB >>= 1;
		/* mB is always > low16, and low16 > 0xffff/2 => mB is never zero because of this loop */
	}
	low16  <<= 16;
	low16   /= mB;
	
	Q = (F16Dot16)(high16 + low16);
	Q *= sign;
	return Q; /*****/
#endif
}

#ifdef OLD
/*
 * Returns sin( in * pi/180.0 )
 * Valid for input ranges between 0 and 90 degrees.
 * Approximates the sinus function with just 7 FixMuls :-)
 * The max error is about 0.002640 for in = 90.0
 */
F16Dot16 util_FixSinOLD( F16Dot16 in )
{
	F16Dot16 node1pow1, node3;  /* working variables */
	F16Dot16 node1pow2;
	F16Dot16 node1pow3;
	F16Dot16 out;
	
	if ( in > 90 * 0x10000 ) in = 90 * 0x10000;
	else if ( in < 0 ) in = 0;

	node1pow1   = -113041 + util_FixMul( 2512, in);
	node1pow2	= util_FixMul(node1pow1,node1pow1);
	node1pow3	= util_FixMul(node1pow1,node1pow2);
	
	node3    = 14830
			   + util_FixMul(68243, node1pow1)
			   - util_FixMul(14871, node1pow2)
	           - util_FixMul( 2280, node1pow3);
	
	out =  41697 + util_FixMul( 20249, node3 );
	if ( out < 0 ) out = 0;
	return out; /*****/
}
#endif

/*
 * Returns sin( in * pi/180.0 )
 * Valid for input ranges between 0 and 90 degrees.
 * Approximates the sin function with just 9 multiplies and one divide.
 * The max error is = 0.000063 ( => Roughly 14 out of 16 bits are correct )
 * for this genetically (!) evolved function.
 */
F16Dot16 util_FixSin( F16Dot16 in )
{
	F16Dot16 node1pow1;  /* working variables */
	F16Dot16 node1pow2;
	F16Dot16 node1pow3;
	F16Dot16 node1pow4;
	F16Dot16 node1pow5;
	F16Dot16 out;
	
	if ( in > 90 * 0x10000 ) in = 90 * 0x10000;
	else if ( in < 0 ) in = 0; /* 0.0 -- 90.0 */
	
	node1pow1   = in;
	node1pow1  /= 90; /* => +0 --- 1.0 */
	node1pow1  -= 0x8000; /* -0.5 .. 0.5 */

	node1pow2	= (node1pow1 * node1pow1) >> 15; /* 0.5 -- 0.5 */
	node1pow3	= (node1pow1 * node1pow2) >> 15; /* 0.5 -- 0.5 */
	node1pow4	= (node1pow2 * node1pow2) >> 15; /* 0.5 -- 0.5 */
	node1pow5	= (node1pow2 * node1pow3) >> 15; /* 0.5 -- 0.5 */

	out    	 = 46343 +  
				+ (( node1pow1 *  18198 ) >> 14)
				+ (( node1pow2 * -14284 ) >> 15)
				+ (( node1pow3 *  -3742 ) >> 15)
				+ (( node1pow4 *    711 ) >> 15)
				+ (( node1pow5 *    114 ) >> 15);
				
	if ( out > 0x10000 ) out = 0x10000;
	return out; /*****/
}


#ifdef OLDOLD

/* Return N / D */
static F16Dot16 util_FixDivOld( F16Dot16 N, F16Dot16 D )
{
	F16Dot16 Q;
	/* I think we need to eliminate this floating point math and make our own FixDiv instead */
	/* Q = (long)(((float)N / (float)D ) * (float)ONE16Dot16); */
	Q = (long)(((double)N / (double)D ) * (double)ONE16Dot16);
	return Q; /*****/
	
}

void TESTFIXDIV( void )
{
	F16Dot16 N, D, Q1, Q2, Q3;
	float fQ;
	long tick1, tick2, testCount;
	
	
	tick1 = TickCount();
	for ( N = 0; N < 1000; N++ ) {
		for ( D = 0; D < 4000; D++ ) {
			Q2 = util_FixMul( N, D );
		}
	}
	tick2 = TickCount();
	printf("util_FixMul ticks = %d\n", tick2-tick1 );
	tick1 = TickCount();
	for ( N = 0; N < 1000; N++ ) {
		for ( D = 0; D < 4000; D++ ) {
			Q2 = util_FixDiv( N, D );
		}
	}
	tick2 = TickCount();
	printf("util_FixDiv ticks = %d\n", tick2-tick1 );
	tick1 = TickCount();
	for ( N = 0; N < 1000; N++ ) {
		for ( D = 0; D < 4000; D++ ) {
			Q2 = util_FixDivOld( N, D );
		}
	}
	tick2 = TickCount();
	printf("util_FixDivOld ticks = %d\n", tick2-tick1 );
	
	printf("Testing FixDiv\n");
	testCount = 0;
	for ( N = -20000000L; N <= 20000000L; N += 1003 ) {
		for ( fQ = 0.01; fQ < 100.0;  fQ *= 1.01 ) {
			Q1 = ONE16Dot16 * fQ;
			D = N / fQ;
			if ( D == 0 ) continue; /*****/
			Q2 = util_FixDiv( N, D );
			Q3 = util_FixDivOld( N, D );
			if ( false && Q2 != Q3 ) {
				printf("%d / %d : Q1 = %d, Q2 = %d\, Q3 = %d\n", N, D, Q1, Q2, Q3 );
			}
			assert( Q2 >= Q3 -2 && Q2 <= Q3 + 2 );

			Q1 = ONE16Dot16 * (-fQ);
			D = N / (-fQ);
			Q2 = util_FixDiv( N, D );
			Q3 = util_FixDivOld( N, D );
			if ( false && Q2 != Q3 ) {
				printf("%d / %d : Q1 = %d, Q2 = %d\, Q3 = %d\n", N, D, Q1, Q2, Q3 );
			}
			assert( Q2 >= Q3 -2 && Q2 <= Q3 + 2 );
			testCount += 2;
		}
	}
	printf("OK FixDiv, did %d tests\n", testCount );
}
#endif /* OLDOLD */

/*
 * Description:		returns sqrt( A*A + B*B );
 * How used:		Call with the data (dx, and dy).
 * Side Effects: 	None.
 * Return value: 	sqrt( A*A + B*B ).
 */
F16Dot16 util_EuclidianDistance( register F16Dot16 A, register F16Dot16 B )
{
	F16Dot16 root;
		
	if ( A < 0 ) A = -A;
	if ( B < 0 ) B = -B;
	
	if ( A == 0 ) {
		return B; /*****/
	} else if ( B == 0 ) {
		return A; /*****/
	} else {
		root	= A > B ? A + (B>>1) : B + (A>>1); /* Do an initial approximation, in root */

		/* Ok, now enter the Newton Raphson iteration sequence */
		root = (root + util_FixMul( A, util_FixDiv( A, root) ) + util_FixMul( B, util_FixDiv( B, root) ) + 1) >> 1; 
		root = (root + util_FixMul( A, util_FixDiv( A, root) ) + util_FixMul( B, util_FixDiv( B, root) ) + 1) >> 1; 
		root = (root + util_FixMul( A, util_FixDiv( A, root) ) + util_FixMul( B, util_FixDiv( B, root) ) + 1) >> 1; 
		/* Now the root should be correct, so get out of here! */
#ifdef OLD
	F16Dot16 square;
		square	= util_FixMul( A, A) + FixMul( B, B);

		do {
			/* root = ((old_root = root) + util_FixDiv( square, root ) + 1 ) >> 1; we may have an overflow in square, so do it as below instead */
printf("root = %d\n", root );
			root = ((old_root = root) + util_FixMul( A, util_FixDiv( A, root) ) + util_FixMul( B, util_FixDiv( B, root) ) + 1) >> 1; 
		} while (old_root != root );
		assert( util_FixMul( root, root ) < util_FixMul( square, 65536+65 ) ); 
		assert( util_FixMul( root, root ) > util_FixMul( square, 65536-65 ) );
#endif
		return root; /*****/
	}
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
void util_ComputeIntersection( short line1_pt1_x, short line1_pt1_y, short line1_pt2_x, short line1_pt2_y,
								 short line2_pt1_x, short line2_pt1_y, short line2_pt2_x, short line2_pt2_y,
								 short *x, short *y )
{
 	long dx1, dy1, dx2, dy2, x1, y1, x2, y2;
	long Num,Denom, t;
	
	
	dx1 = line1_pt2_x - line1_pt1_x;	dy1 = line1_pt2_y - line1_pt1_y;
	dx2 = line2_pt2_x - line2_pt1_x;	dy2 = line2_pt2_y - line2_pt1_y;
 
 	x1 = line1_pt1_x; y1 = line1_pt1_y;
 	x2 = line2_pt1_x; y2 = line2_pt1_y;

	Num   = (y2 - y1) * dx1 - (x2 - x1) * dy1;
	Denom = dx2 * dy1 - dy2 * dx1;

	if ( Denom != 0 ) { /* 3/19/98 changed 0.0 to 0 ---Sampo */
		t = util_FixDiv( Num, Denom );
		*x = (short)( x2 + util_FixMul( dx2, t ) );
		*y = (short)( y2 + util_FixMul( dy2, t ) );
	} else {
		*x = (short)((line1_pt2_x+line2_pt1_x)/2);
		*y = (short)((line1_pt2_y+line2_pt1_y)/2);
	}
}



#ifdef ENABLE_HASH_CLASS
/* hashClass thoughts:
 * The performance could probably be improved slighly by using double hashing instead of the
 * current linear probing. We would have to make M a real prime, + compute a second increment
 * to use for the proble sequence. 14/19/2001 ---Sampo
 */

/*
 * Private method.
 */
static int IsPrimeLike( register long i )
{
	if ( (i & 1) == 0 )  return false;
	assert( i > 7 );
	if ( i % 3  == 0 )   return false;
	if ( i % 5  == 0 )   return false;
	if ( i % 7  == 0 )   return false;
	if ( i > 11 ) {
		if ( i % 11  == 0 )   return false;
		if ( i > 13 ) {
			if ( i % 13  == 0 )   return false;
		}
	}
	return true;
}

/*
 * The hashClass constructor
 * mem: self-explanatory
 * maxItems: The maximum number of items you will ever store in the hash table
 * stringsAreEqual: Set to NULL if you use uint16 to uint16 key value pairs, but if you
 *                  use string to unti16 key value pairs, then this is the callback function which
 *                  will be used to see if a string is equal to string n.
 * privPtr: If (getStr != NULL) then this will be passed to getStr as the first parameter.
 * NOTE: This implemenation does not allow you to associate 0xffff with 0xffff since
 * this value pair is used as an internal marker. All other 2**32-1 pairs are OK however :-)
 */
hashClass *New_hashClass( tsiMemObject *mem, long maxItems, FF_STRS_ARE_EQUAL_FUNC_PTR stringsAreEqual, void *privPtr )
{
	long i, M;
	hashClass *t = (hashClass *) tsi_AllocMem( mem, sizeof( hashClass ) );
	
	t->mem			  = mem;
	t->numItems		  = 0;
	t->maxItems		  = maxItems;
	t->enum_x		  = 0;
	t->stringsAreEqual= stringsAreEqual;
	t->privPtr		  = privPtr;
	assert( maxItems > 1 );
	
	i				= maxItems + (maxItems+maxItems+3)/3; /* 1.67, for a max load factor of 0.6 */
	i				= (i + 2) >> 1;
	i				= i + i + 3; /* make an odd number that is somehwat primelike */
	while ( !IsPrimeLike( i ) ) i += 2; /* go to the next odd number */
	
	t->M			= M = i;
	assert( i > maxItems + (maxItems>>1) );
	
	
	t->hash = (hashEntry *)tsi_AllocMem( mem, t->M * sizeof( hashEntry ) );
	for ( i = 0; i < M; i++ ) {
		t->hash[i].key   = 0xffff;
		t->hash[i].value = 0xffff;
	}
	return t; /*****/
}


/*
 * private method
 */
static long ConvertStrToKey( char *s, long M )
{
	register long h;
	
	for ( h = 0; *s != 0; s++ ) {
		h = ((h<<8) + *s) % M;
	}
	return h;
}

/*
 * public method for getting a value from a uint16 key
 * returns true if key is found, and false otherwise
 * puts the value in *value if found.
 */
int get_using_uint16_hashClass( hashClass *t, uint16 key, uint16 *value )
{
	long M = t->M;
	long x = key % M;
	register hashEntry *hash = t->hash;

	while ( hash[x].key != 0xffff || hash[x].value != 0xffff )  {
		if ( hash[x].key  == key ) {
			t->x   = x;
			*value = hash[x].value;
			return true; /*****/
		}
		x = (x + 1) % M;
	}
	t->x = x;
	*value = 0xffff;
	return false; /*****/
}


/*
 * public method for getting a value from a string key
 * returns true if key is found, and false otherwise
 * puts the value in *value if found.
 */
int get_using_str_hashClass( hashClass *t, char *keystr, uint16 *value )
{
	long M = t->M;
	long x;
	register hashEntry *hash = t->hash;

	assert( t->stringsAreEqual != NULL );
	x = ConvertStrToKey( keystr, M );
	while ( hash[x].key != 0xffff || hash[x].value != 0xffff )  {
		if ( t->stringsAreEqual( t->privPtr, keystr, hash[x].key ) ) {
			t->x   = x;
			*value = hash[x].value;
			return true; /*****/
		}
		x = (x + 1) % M;
	}
	t->x = x;
	*value = 0xffff;
	return false; /*****/
}

/*
 * public method for putting a a key+value pair into the hash table.
 * When putting in a string, pass in an uint16 identifier in the key variable which you will later use in
 * the stringsAreEqual to access this string from your string storage.
 */
void put_hashClass( hashClass *t, uint16 key, uint16 value, char *keyStr  )
{
	register hashEntry *hash = t->hash;
	uint16 oldValue;
	int found;
	
	assert( !( key == 0xffff && value == 0xffff) ); /* This is the one and only key to value we can not map */
	if ( keyStr != NULL ) {
		assert( t->stringsAreEqual != NULL );
		found = get_using_str_hashClass( t, keyStr, &oldValue );
	} else {
		found = get_using_uint16_hashClass( t, key, &oldValue );
	}
	
	if ( found ) {
		hash[t->x].value = value; /* override the old value */
	} else {
		/* It does not exist, so we create a new entry */
		assert( hash[t->x].key   == 0xffff );
		assert( hash[t->x].value == 0xffff );
		hash[t->x].key	= key;
		hash[t->x].value = value;
		t->numItems++;
		assert( t->numItems <= t->maxItems );
	}
}	
	
/*
 * Resets the enumaration capability.
 */
void rewind_enum_hashClass( hashClass *t )
{
	assert( t != NULL );
	t->enum_x		  = 0;
}

/*
 * First call rewind_enum_hashClass()
 * Then to get all the data keep calling this as long as it returns true.
 * When it returns false it means no more data was available. *key and *value are NOT set.
 * When it returns true it sets *key and *value.
 */
int next_enum_hashClass( hashClass *t, uint16 *key, uint16 *value )
{
	register hashEntry *hash = t->hash;
	register long M = t->M;
	register long x = t->enum_x;
	int found = false;
	
	while ( x < M ) {
		if ( hash[x].key == 0xffff && hash[x].value == 0xffff ) {
			x++;
			continue; /*****/
		} else {
			*key   = hash[x].key;
			*value = hash[x++].value;
			found  = true;
			break; /*****/
		}
	}
	t->enum_x = x;
	return found; /*****/
}


/*
 * The hashClass destructor
 */
void Delete_hashClass( hashClass *t )
{
	if ( t != NULL ) {
		tsi_DeAllocMem( t->mem, t->hash );
		tsi_DeAllocMem( t->mem, t );
	}
}

#ifdef ACTICATE_HASHCLASS_TESTUNIT
/*
 * The hashClass DEBUG UNIT TEST
 */

#define HASH_TEST_BUFFER_SIZE 1000
#define HASH_TEST_BUFFER_ENTRY 5
static char buffer[HASH_TEST_BUFFER_SIZE*HASH_TEST_BUFFER_ENTRY];
static char buffer_initialized = false;

static char *TEST_STR( void *privptr, uint16 key )
{
	privptr;

	assert( key >= 0 && key < HASH_TEST_BUFFER_SIZE );
	if (!buffer_initialized) {
		long i;
		for ( i = 0; i < HASH_TEST_BUFFER_SIZE; i++ ) {
			char *p = &buffer[i*HASH_TEST_BUFFER_ENTRY];
			sprintf(p, "%d", i );
		}
		buffer_initialized = 1;
	}
	
	return &buffer[key*HASH_TEST_BUFFER_ENTRY];
}

static int StringsAreEqual( void *privptr, char *str, uint16 n )
{
	privptr;
	return (strcmp( str, TEST_STR(NULL, n) ) == 0);
}


void TEST__hashClass( tsiMemObject *mem )
{
	uint16 i, key, value, value2;
	int found;
	hashClass *t;
	char *str;
	
	/* TEST #1 */
	t = New_hashClass( mem, 1000, NULL, NULL );
	
	for ( i = 0; i < 1000; i++ ) {
		key = (uint16)(117 + i * 3);
		value = (uint16)(i+i+3);
		put_hashClass( t, key, value, NULL );
	}
	for ( i = 0; i < 1000; i++ ) {
		key = (uint16)(117 + i * 3);
		value  = (uint16)(i+i+3);
		found = get_using_uint16_hashClass( t, key, &value2 );
		assert( found );
		assert( value == value2 );
		found = get_using_uint16_hashClass( t, (uint16)(key-1), &value2 );
		assert( !found );
		found = get_using_uint16_hashClass( t, (uint16)(key+1), &value2 );
		assert( !found );
	}
	printf("hashClass: OK, passed uint16 ->uint16 testing!\n");
	
	Delete_hashClass( t );
	
	/* TEST #2 */
	t = New_hashClass( mem, 1000, StringsAreEqual, NULL );
	
	for ( i = 0; i < 1000; i++ ) {
		key = i;
		value = (uint16)(i+i+3);
		if ( i != 500 ) put_hashClass( t, key, value, TEST_STR(NULL, key) );
	}
	for ( i = 0; i < 1000; i++ ) {
		key = i;
		value  = (uint16)(i+i+3);
		str    = TEST_STR( NULL, key );
		
		found = get_using_str_hashClass( t, str, &value2 );
		if ( i == 500 ) {
			assert( !found );
		} else {
			assert( found );
			assert( value == value2 );
		}
	}
	printf("hashClass: OK, passed str ->uint16 testing!\n");
	
	Delete_hashClass( t );

}
#endif /* ACTICATE_HASHCLASS_TESTUNIT */
#endif /* ENABLE_HASH_CLASS */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/util.c 1.16 2001/05/04 21:04:18 reggers Exp $
 *                                                                           *
 *     $Log: util.c $
 *     Revision 1.16  2001/05/04 21:04:18  reggers
 *     Moved a hash only service routine within the conditional manifest.
 *     Revision 1.15  2001/05/03 17:19:56  reggers
 *     Warning cleanup.
 *     Revision 1.14  2001/04/26 15:24:34  reggers
 *     Hash class functionality added, including enumeration. Enabled
 *     now only when ENABLE_HASH_CLASS is enabled.
 *     Revision 1.13  2000/10/25 19:41:20  reggers
 *     Patched in unimplemented correction of rounding error in FixDiv():
 *     to be tested later on delta hinted fonts.
 *     Revision 1.12  2000/05/17 14:08:07  reggers
 *     Fix for 2 byte integer environments
 *     Revision 1.11  2000/04/06 16:26:26  reggers
 *     #if 0'd ReadFileIntoMemory sample function.
 *     Revision 1.10  1999/12/23 22:03:24  reggers
 *     New ENABLE_PCL branches. Rename any 'code' and 'data' symbols.
 *     Revision 1.9  1999/12/09 22:07:07  reggers
 *     Sampo: multiple TrueType compatibility enhancements (scan converter)
 *     Revision 1.7  1999/10/18 17:03:01  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.6  1999/09/30 15:12:45  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.5  1999/07/19 16:59:50  sampo
 *     Removed kern shell sort routine
 *     Revision 1.4  1999/07/16 19:29:59  sampo
 *     Shell sort mystery fix.
 *     Revision 1.3  1999/07/16 15:57:15  mdewsnap
 *     created util_kernShellSort routine
 *     Revision 1.2  1999/05/17 15:58:56  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

