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
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

#ifndef __DATATYPE_H__
#define __DATATYPE_H__

#if __BEOS__
#include <ByteOrder.h>
#endif

/* These are the non-platform specific data types needed for the PIA
 * interface.  All data and return types defined are ANSI C compatible.
 * All pointers declared in the PIA headers must use the FAR attribute to
 * support operation in 16 bit environments.
 */

/* These types of data must always be the same
 * number of bits regardless of the platform.
 * Therefore, these generic names are defined
 * to be:
 *
 * U8		8 bits,  unsigned           0 ..        255
 * I8		8 bits,  signed          -128 ..        127
 * U16		16 bits, unsigned           0 ..      65535
 * I16		16 bits, signed        -32768 ..      32767
 * U32		32 bits, unsigned           0 .. 4294967295
 * I32		32 bits, signed   -2147483648 .. 2147483647
 * Sngl		32 bit floating point (4 bytes)
 * Dbl		64 bit floating point (8 bytes)
 * 
 * PU8		Pointers to the above types
 * PI8
 * PU16
 * PI16
 * PU32
 * PI32
 * PSngl
 * PDbl
 */

/* Macros are defined in the following.  They are also commonly
 * defined in some environemnts, so they are undefined here.
 */

#undef FAR		/* FAR, HUGE and NEAR modify pointer types.  For example: */
				/* typedef U8 FAR *pu8;  // a far pointer to unsigned 8. */
                /* FAR means 'use 32 bit address space' */				
#undef HUGE		/* HUGE means 'use 48 bit address space' */
#undef NEAR		/* NEAR means 'use 16 bit address space' */
#undef NULL		/* An invalid pointer */

#undef FALSE	/* Boolean values */
#undef TRUE


#if defined _WINDOWS
	#if !defined _MSC_VER
		/* All Microsoft compilers define this value.  It is checked
		 * because some definitions below may not be portable to other
		 * compilers.  Modify _* to suit your compiler.
		 */
		#error "Only Microsoft compilers supported at this time"
	#endif

	#if defined _WIN32
		/* Win32 definitons for Windows 95 and Windows NT */
		#if defined _M_IX86

			/* definitions for Intel processors compatible with x86
			 * instruction set.
			 */
			#define FAR
			#define HUGE
			#define NEAR

			typedef char			Chr;
			typedef unsigned char	U8;
			typedef signed char		I8;
			typedef unsigned short	U16;
			typedef signed short	I16;
			typedef unsigned long	U32;
			typedef signed long		I32;
			typedef float			Sngl;
			typedef double			Dbl;

		#else /* _M_IX86 */
			#error "Only Intel x86 supported at this time"
		#endif /* _M_IX86 */
	#else /* _WIN32 */

		/* Windows 3.1 definitions */
		#define FAR				_far
		#define HUGE			_huge
		#define NEAR			_near


		typedef unsigned char  U8;
		typedef signed char    I8;
		typedef unsigned short U16;
		typedef signed short   I16;
		typedef unsigned long  U32;
		typedef signed long    I32;
		typedef float          Sngl;
		typedef double         Dbl;

	#endif /* _WIN32 */
#else
	/* UNIX environment */
	#define FAR
	#define HUGE
	#define NEAR

	typedef unsigned char	U8;
	typedef signed char		I8;
	typedef unsigned short	U16;
	typedef signed short	I16;
	typedef unsigned long	U32;
	typedef signed long		I32;
	typedef float			Sngl;
	typedef double			Dbl;
#endif

/* PIA_ENUM is a constant four byte size type that should be used in place of
 * enumerated types because the size of enumerated types depends on the host
 * compiler's data layout scheme.
 */
typedef I32 PIA_ENUM;
typedef PIA_ENUM FAR * PTR_PIA_ENUM;


/* Use PIA_Boolean in the PIA interface for function parameters and 
 * structure members because its size is constant.  PIA_Boolean can
 * take one of two values: TRUE or FALSE.  It is defined this way
 * instead of an enum because the size of an enumerated type is
 * compiler dependent.
 */
typedef enum {
	FALSE  = 0,
	TRUE   = 1
} BOOLEAN_LIST;
typedef	PIA_ENUM		PIA_Boolean;
typedef	PIA_Boolean FAR *	PPIA_Boolean;


/* Below are pointers to the const size data types.
 */
typedef char	FAR *	PChr;		   /* far pointer to a char */
typedef U8		FAR *	PU8;           /* far pointer to unsigned 8-bit value */
typedef PU8     FAR *   PPU8;          /* far pointer to PU8 */
typedef I8		FAR *	PI8;           /* far pointer to signed 8-bit value */
typedef U16		FAR *	PU16;          /* far pointer to unsigned 16 bit value */
typedef I16		FAR *	PI16;          /* far pointer to signed 16 bit value */
typedef U32		FAR *	PU32;          /* far pointer to unsigned 32 bit value */
typedef I32		FAR *	PI32;          /* far pointer to signed 32 bit value */
typedef Sngl	FAR *	PSngl;         /* far pointer to single precision float */
typedef Dbl		FAR *	PDbl;          /* far pointer to double precision float */
typedef I32	   (FAR *	PFunction)();  /* far pointer to function returning signed 32 bit value. */

#define	NULL	0

/* These types of data do not need to be the same size from platform to
 * platform. The only concern is that the values represented by these types
 * fit into the smallest bit size of any platform we port to. As new platforms
 * are added, please update this table:
 *                  WIN3.x UNIX  WINNT (win32)
 * NaturalInt:      16     32    32
 * NaturalUnsigned: 16     32    32
 * 
 * NaturalInt is used in places where speed is important.  Instead of using
 * 'int' use NaturalInt so that the code can be tested with TEST_16_BITS and
 * TEST_32_BITS macros.
 *
 * These pointers are naturally platform specific, and what they point to is
 * allowed to be platform specific also.
 *
 *               These are far pointers
 * PNaturalInt
 * PNaturalUnsigned
 *
 * For those who complain about long identifiers, these data types are spelled
 * out specifically to avoid confusion with the fixed length data types and to
 * let the reader who is unfamiliar with our coding practices know that these
 * data types are different on each platform. The intent is to be obvious and
 * obnoxious.
 *
 * In order to facilitate testing, define TEST_16_BITS
 * which will force all natural data types to 16 bits.
 */

#if defined TEST_16_BITS
	typedef I16				NaturalInt;
	typedef U16				NaturalUnsigned;
#elif defined TEST_32_BITS
	typedef I32				NaturalInt;
	typedef U32				NaturalUnsigned;
#else
	typedef signed int		NaturalInt;
	typedef unsigned int	NaturalUnsigned;
#endif /* TEST_16_BITS */

/* pointers */
typedef NaturalInt FAR *       PNaturalInt;
typedef NaturalUnsigned FAR *  PNaturalUnsigned;


/* logical types */
typedef NaturalInt		NaturalBoolean;
typedef NaturalBoolean	Boo;
typedef Boo FAR *		PBoo;



/* BGR_ENTRY
 * The BGR_ENTRY structure is used to define a pixel's color 
 * in blue, green, red order.  This is typically used in an
 * array for palletized color output.
 */
typedef struct {
	U8 u8B;				/* Blue component */
	U8 u8G;				/* Green component */
	U8 u8R;				/* Red component */
	U8 u8Reserved;		/* Extra information for each color.  Could be Alpha channel. */
} BGR_ENTRY;
typedef BGR_ENTRY FAR * PTR_BGR_ENTRY;

typedef void FAR *  GLOBAL_HANDLE;
typedef void NEAR * LOCAL_HANDLE;

typedef void NEAR * MUTEX_HANDLE;

typedef void FAR  * PVOID_GLOBAL;
typedef void NEAR * NPVOID_LOCAL;
typedef void HUGE * HPVOID;  
typedef void FAR *	PINSTANCE;


/* define useful macros */

#define ABS(a)				((a) < 0 ? (-(a)) : (a))
#define ARSHIFT(a, b)		((a) >> (b))
#define DIV2(x)				((x)>0?(x)>>1:-(-(x))>>1)
#define DIV_ROUND(x,d)		((x)>0?((x)+(d)/2)/(d):-(((-(x))+(d)/2)/(d)))
#define MAX(a, b)			((a) > (b) ? (a) : (b))
#define MIN(a, b)			((a) < (b) ? (a) : (b))

/* DimensionSt defines the image's dimensions.
 */
typedef struct {
	U32 h;
	U32 w;
} DimensionSt;
typedef DimensionSt FAR * PDimensionSt;


/* PointSt defines a point residing at a row and column where (0,0) is the upper left
 * corner, pictorially.
 */
typedef struct {
	U32 r;
	U32	c;
} PointSt, FAR* PPointSt;


/* RectSt is a PointSt and a dimenstionSt which defines am image's
 * rectangle.
 */
typedef struct {
	U32 r;			/* row, column of upper left-hand corner */
	U32 c;
	U32 h;			/* height, width */
	U32 w;
} RectSt, FAR* PRectSt;


/* MatrixSt is a dimensionSt plus pitch and data which defines a rectangular
 * region where the distance from one row to the next is the Pitch.
 * Pitch may be positive or negative depending on the orientation of
 * the image in memory (-:BottomUp or +:TopDown).
 * Example:
 *
 *  PMatrixST z;
 *  PI16 p = z->pi16 + z->Pitch;
 *
 *  p now points to 'the second row'.
 */

typedef struct {
	U32 NumRows;	/* dimensions of matrix (data - cf pitch) */
	U32 NumCols;
	I32 Pitch;		/* spacing between rows; ABS(Pitch) >= NumCols */
	PI16 pi16;
} MatrixSt, FAR* PMatrixSt;
typedef const MatrixSt FAR* PCMatrixSt;


#endif /* __DATATYPE_H__ */
