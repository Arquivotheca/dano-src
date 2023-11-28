/*
 * UTIL.H
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

#ifndef __T2K_UTIL__
#define __T2K_UTIL__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

unsigned char *ReadFileIntoMemory( tsiMemObject *mem, const char *fname, unsigned long *size );
void WriteDataToFile( const char *fname, void *dataIn, unsigned long size );



void util_SortShortArray( short *a, long n );
/* Return mA * mB */
F16Dot16 util_FixMul( F16Dot16 mA, F16Dot16 mB );
/* Return mA / mB */
F16Dot16 util_FixDiv( F16Dot16 mA, F16Dot16 mB );
/* Returns sin( in * pi/180.0 ) valid when in belongs to [0,90] */
F16Dot16 util_FixSin( F16Dot16 in );
/* Returns cos( in * pi/180.0 ) valid when in belongs to [0,90] */
#define util_FixCos( in ) util_FixSin( (90*0x10000 - (in)) )
void TESTFIXDIV( void );

/* 
 * Return mA * mB
 * This is an integer implementation!
 * mA is an integer
 * mB is 2.14
 */
long util_ShortFracMul( long mA, short mB );

F16Dot16 util_FixMul26Dot6( F16Dot16 mA, F16Dot16 mB );

/*
 * Description:		returns sqrt( A*A + B*B );
 * How used:		Call with the data (dx, and dy).
 * Side Effects: 	None.
 * Return value: 	sqrt( A*A + B*B ).
 */
F16Dot16 util_EuclidianDistance( register F16Dot16 A, register F16Dot16 B );

/*
 * Computes the instersection of lie1 and line 2 in *x, *y.
 */
void util_ComputeIntersection( short line1_pt1_x, short line1_pt1_y, short line1_pt2_x, short line1_pt2_y,
                               short line2_pt1_x, short line2_pt1_y, short line2_pt2_x, short line2_pt2_y,
							   short *x, short *y );


#ifdef ENABLE_HASH_CLASS
typedef struct {
	uint16 key;
	uint16 value;
} hashEntry;

typedef int (*FF_STRS_ARE_EQUAL_FUNC_PTR)( void *privptr, char *str, uint16 n);

typedef struct {
	/* private */
	tsiMemObject *mem;
	long x;
	long numItems;
	long maxItems;
	long enum_x;
	long M; /* table size */
	FF_STRS_ARE_EQUAL_FUNC_PTR stringsAreEqual;
	void *privPtr; /* first parameter to getStr */
	hashEntry *hash;
	
	/* public */
} hashClass;

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
hashClass *New_hashClass( tsiMemObject *mem, long maxItems, FF_STRS_ARE_EQUAL_FUNC_PTR stringsAreEqual, void *privPtr );
/*
 * public method for getting a value from a uint16 key
 * returns true if key is found, and false otherwise
 * puts the value in *value if found.
 */
int get_using_uint16_hashClass( hashClass *t, uint16 key, uint16 *value );
/*
 * public method for getting a value from a string key
 * returns true if key is found, and false otherwise
 * puts the value in *value if found.
 */
int get_using_str_hashClass( hashClass *t, char *keystr, uint16 *value );
/*
 * public method for putting a a key+value pair into the hash table.
 * When putting in a string, pass in an uint16 identifier in the key variable which you will later use in
 * the stringsAreEqual to access this string from your string storage.
 */
void put_hashClass( hashClass *t, uint16 key, uint16 value, char *keyStr  );
/*
 * Resets the enumaration capability.
 */
void rewind_enum_hashClass( hashClass *t );
/*
 * First call rewind_enum_hashClass()
 * Then to get all the data keep calling this as long as it returns true.
 * When it returns false it means no more data was available. *key and *value are NOT set.
 * When it returns true it sets *key and *value.
 */
int next_enum_hashClass( hashClass *t, uint16 *key, uint16 *value );
/*
 * The hashClass destructor
 */
void Delete_hashClass( hashClass *t );
/* Do NOT define, unless you are specifically testing this. */
#ifdef ACTICATE_HASHCLASS_TESTUNIT
void TEST__hashClass( tsiMemObject *mem );
#endif
#endif /* ENABLE_HASH_CLASS */


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_UTIL__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/util.h 1.6 2001/04/26 15:24:43 reggers Exp $
 *                                                                           *
 *     $Log: util.h $
 *     Revision 1.6  2001/04/26 15:24:43  reggers
 *     Hash class functionality added, including enumeration. Enabled
 *     now only when ENABLE_HASH_CLASS is enabled. (Sampo).
 *     Revision 1.5  1999/09/30 15:12:42  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.4  1999/07/19 17:00:01  sampo
 *     Removed proto of kern shell sort routine
 *     Revision 1.3  1999/07/16 15:57:35  mdewsnap
 *     Created prototype for util_kernShellSort
 *     Revision 1.2  1999/05/17 15:58:59  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
