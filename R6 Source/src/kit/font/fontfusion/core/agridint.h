/*
 * Agridint.h
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

#ifdef ENABLE_AUTO_GRIDDING_CORE

#ifndef __T2K_AGRINDINT__
#define __T2K_AGRINDINT__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#define F26Dot6 long
#define int32 	long
#define int16 	short
#define uint8 	unsigned char
#define uint16 	unsigned short
#define uint32 	unsigned long


#define SHORTMAX 32767
#define SHORTMIN -32768
#define	kMaxLong		0x7FFFFFFF

#define TWO_PERCENT_OF_THE_EM (hData->unitsPerEm/50)
#define ONE_PERCENT_OF_THE_EM (hData->unitsPerEm/100)
#define TEN_PERCENT_OF_THE_EM (hData->unitsPerEm/10)

#define CVT_EXIT_MARKER 9999


#define HEIGHT_ENTRY_SIZE 2

/* We can lower this later, but for now keep same as for StingRay */
/* #define MAX_DOUBLE_HEIGHTS	19 */
/* We can lower this later, but for now keep same as for StingRay */
#define MAX_HEIGHTS				67

#define MAX_SINGLE_HEIGHTS		(MAX_HEIGHTS-LC_L_DOT_BOTTOM_HEIGHT)
#define NUMNORMVALUES 10
#define USER_TMPS 3
#define FIRST_USER_CVT MAX_HEIGHTS
/* 70 */
#define FIRSTTMP (FIRST_USER_CVT + USER_TMPS)
/* used by function 18 and tp_MIRP*/
/* 71 */
#define SECONDTMP (FIRSTTMP + 1)
/* 72 */
#define THIRDTMP (FIRSTTMP + 2)
#define USED_TMPS		7
#define RESERVED_TMPS	8  
#define NUMTEMPS (USED_TMPS+RESERVED_TMPS)
#define GLOBALNORMSTROKEINDEX THIRDTMP
#define FREEBASE (FIRSTTMP + NUMTEMPS + 24 )
/*
	CVT ALLOCATION
	0 -66 heights
	67-69 user values
	70-71 compiler
	72-87 weights


*/



/********* Start Internal data types *********/
/* The caller of the external API can NEVER read or write inside these */

/* 2.14 */
#define SMALLFRAC		int32
#define SMALLFRAC_ONE	0x4000
#define SMALLFRACVECMUL( dx1, dx2, dy1, dy2 ) (( (((SMALLFRAC)(dx1))*((SMALLFRAC)(dx2))) + (((SMALLFRAC)(dy1))*((SMALLFRAC)(dy2))) ) >> 14)


#define MAX_CVT_ENTRIES	96

typedef struct {
	uint8 type;
	uint8 direction;
	uint8 forwardTo;
	uint8 priority;
	int16 from;
	int16 to;
} tp_LinkType;

#define T2K_H_ZONES 5


typedef struct {
	unsigned long magic0xA5A0F5A5; /* For basic sanity checking */
    short numberOfContours;	/* Number of contours in the character */
    short *startPoint; 		/* should be set to point at the same field in the ag_ElementType structure */
    short *endPoint;		/* should be set to point at the same field in the ag_ElementType structure */
	unsigned char *onCurve; /* should be set to point at the same field in the ag_ElementType structure */
	int16 *oox; /* should be set to point at the same field in the ag_ElementType structure */
	int16 *ooy; /* should be set to point at the same field in the ag_ElementType structure */
	
	short isFigure; /* Set to true for 0..9 and false otherwise */

	
#ifdef ENABLE_ALL_AUTO_HANDG_CODE
	int16 *nextPt; 			/* Is set so that nextPt[A] == the next point number on the contour A is on */
	int16 *prevPt;			/* Is set so that nextPt[A] == the previous point number on the contour A is on */
	int16 *searchPoints;	/* Just used by ag_FindPointPairs to cache important pointnumbers. This caching makes things go faster */

	/* Begin Local hint data base */
	uint16 *flags;			/* Contains various bit-flags for all the points */
	int16 *realX;			/* The real x-coordinate ON the outline for all the points */
	int16 *realY;			/* The real y-coordinate ON the outline for all the points */
	/* Black space */
	int16 *forwardAngleOrthogonalPair; 	/* forwardAngleOrthogonalPair[B] and B make a potential black distance based on the forward angle */
	int16 *backwardAngleOrthogonalPair; /* backwardAngleOrthogonalPair[B] and B make a potential black distance based on the backward angle */
	int16 *pointToLineMap;

	SMALLFRAC *cos_f;	/* Contains the x projection of the forward tangent for all points multiplied by 16384 */
	SMALLFRAC *sin_f;	/* Contains the y projection of the forward tangent for all points multiplied by 16384 */
	SMALLFRAC *cos_b;	/* Contains the x projection of the backward tangent for all points multiplied by 16384 */
	SMALLFRAC *sin_b;	/* Contains the y projection of the backward tangent for all points multiplied by 16384 */
	
	int linkCount;		/* The number of links that we have */
	int maxLinks;		/* The maximum allowed number of links */
	tp_LinkType *links;	/* The actual links */
	/* End Local hint data base */
#endif /* ENABLE_ALL_AUTO_HANDG_CODE */

	/* PRIVATE, for internal use by the auto-gridder */
	F26Dot6 cvt[MAX_CVT_ENTRIES];	/* scaled control values */
	short ocvt[MAX_CVT_ENTRIES];	/* unscaled control values */
	short unitsPerEm;				/* Units per Em for the fonts */
	long xPixelsPerEm;				/* Pixels per em in the x direction */
	long yPixelsPerEm;				/* Pixels per em in the y direction */
	ag_FontCategory fontType;		/* The type of font we have */
	
	/* private for ag_T2KFastYAG */
	F26Dot6 oy1, y1, y1Shift;
	F26Dot6 oy2, y2, y2Shift;
	F26Dot6 oy3, y3, y3Shift;
	F26Dot6 oy4, y4, y4Shift;
	int16  ooy1, ooy2, ooy3, ooy4;
	int32 mul[ T2K_H_ZONES ];
	int32 div[ T2K_H_ZONES ];
	int32 add[ T2K_H_ZONES ];
	int32 fmul[ T2K_H_ZONES ];
	int yBounds[T2K_H_ZONES];
	int fFastYAGDisabled;
		
	int maxPointCount;				/* The maximum number of points as set by ag_HintInit */
	/* our internal copy */
	ag_GlobalDataType gData;
	/* maximum profile info */
	ag_HintMaxInfoType maxInfo;
	
#ifdef ENABLE_ALL_AUTO_HANDG_CODE
	
	/* PRIVATE, for internal use by the auto-gridder, and allocated internally */
	F26Dot6	*ox;		/* ox[pointCount] Scaled Unhinted Points */
	F26Dot6	*oy;		/* oy[pointCount] Scaled Unhinted Points */
	uint8	*f;  		/* f[pointCount]Internal flags, one byte for every point */
	
	/* The instruction data */
	long   ttCodeBaseMaxLength; /* The current allocated length */
	uint8 *ttCodeBase;			/* The start of the allocated memory */
	uint8 *ttcode;				/* The first free location */
	/* The data for the stack */
	long   ttDataBaseMaxElements; /* The current allocated length */
	short *ttDataBase;			  /* The start of the allocated memory */
	short *ttdata;				  /* The first free location */
	/* The TrueType hint fragment */
	long   hintFragmentMaxLength; /* The current allocated length */
	uint8 *hintFragment;		  /* The start of the allocated memory */
	long   hintFragmentLength;	  /* The first free location */
	/* The TT graphics state */
	short RP0, RP1, RP2;		/* These mirror the TrueType local graphic state variables RP0,RP1, and RP2 */
#endif /* ENABLE_ALL_AUTO_HANDG_CODE */

#define STORE_maxMul			8
#define STORE_minMul			9
#define STORE_multiplier		10
#define STORE_mulRepeatCount	11
#define STORE_error				12
#define STORE_return			13
#define MAXSTORAGE				16

	long  storage[MAXSTORAGE];

	short inX;	/* Set to true when we operate in the x direction and false otherwise */
	short inY;	/* Set to true when we operate in the y direction and false otherwise */
	char cvtHasBeenSetUp; /* Boolean indicating if the cvt has been initialized */
	char hintInfoHasBeenSetUp; /* Boolean indicating if the the hint info has been initialized by ag_SetHintInfo */
	tsiMemObject *mem;
	int strat98;
	unsigned long magic0x0FA55AF0; /* For basic sanity checking */
} ag_DataType;
/********** End Internal data types *********/
#define ag_pixelSize 64

/* cos ( 5.0 ) * 16384 = 16322 */
#define ag_COS_5_DEG 16322
/* cos(10.0) * 16384 = 16135 */
#define ag_COS_10_DEG 16135
/* cos(15.0) * 16384 = 15826  */
#define ag_COS_15_DEG 15826
/* cos( 30.0 ) * 16384 = 14189 */
#define ag_COS_30_DEG 14189
/* sin ( 2.5 ) * 16384 = 715 */
#define ag_SIN_2_5_DEG 715

/* type */
#define INC_HEIGHT	1
/* #define INC_WIDTH	2  this is unused */
#define INC_LINK	3

/* direction */
#define INC_XDIR 1
#define INC_YDIR 2

/* Bit flags */
#define X_IMPORTANT	0x0001
#define Y_IMPORTANT	0x0002
#define XEX			0x0004
#define YEX			0x0008
#define X_TOUCHED	0x0010
#define Y_TOUCHED	0x0020	
#define HEIGHT		0x0040
#define INFLECTION	0x0080
#define CORNER		0x0100
#define X_ROUND		0x0200
#define Y_ROUND		0x0400
/* Added 95/12/4 --Sampo */
#define IN_XF		0x0800
#define IN_YF		0x1000
/* Added 96/04/08 --Sampo */
#define IN_XB		0x2000
#define IN_YB		0x4000

short ag_GetCvtNumber( ag_DataType *hData, short doX, short doY, short doD, long dist );
int16 ag_Height( register ag_DataType *hData, int pt );
int16 ag_Abs16( register int16 z );
short ag_GetXMaxCvtVal( ag_DataType *hData );
short ag_GetYMaxCvtVal( ag_DataType *hData );
F26Dot6 ag_ModifyHeightGoal( ag_DataType *hData, int16 cvtNumber, F26Dot6 currentValue );
F26Dot6 ag_ModifyWeightGoal( F26Dot6 goal, F26Dot6 currentValue );


void ag_SVTCA_X( ag_DataType *hData );
void ag_SVTCA_Y( ag_DataType *hData );

void ag_MDAPX( ag_DataType *hData, register ag_ElementType* elem, short round, int point );
void ag_MDAPY( ag_DataType *hData, register ag_ElementType* elem, short round, int point );

void ag_MIAPX( ag_DataType *hData, register ag_ElementType* elem, short round, int point, short cvtNumber);
void ag_MIAPY( ag_DataType *hData, register ag_ElementType* elem, short round, int point, short cvtNumber);

void ag_MDRPX( ag_DataType *hData, register ag_ElementType* elem, short cvtNumber, short move, short minDist, short round, char c1, char c2, int ptA, int ptB );
void ag_MDRPY( ag_DataType *hData, register ag_ElementType* elem, short cvtNumber, short move, short minDist, short round, char c1, char c2, int ptA, int ptB );

void ag_MoveDirectRelativePointInPositiveDirection( ag_DataType *hData, register ag_ElementType* elem, short cvtNumber, int from, int to, short doX );
void ag_MoveDirectRelativePointInNegativeDirection( ag_DataType *hData, register ag_ElementType* elem, short cvtNumber, int from, int to, short doX );

void ag_BiDirectionalLink( ag_DataType *hData, register ag_ElementType* elem, short cvtNumber, short minDist, int from, int to, short doX );
void ag_BiDirectionalLinkWithCvt( ag_DataType *hData, register ag_ElementType* elem, short cvtNumber, int from, int to, short doX );

void ag_IPPointX( ag_DataType *hData, register ag_ElementType* elem, int A, int B, int C );
void ag_IPPointY( ag_DataType *hData, register ag_ElementType* elem, int A, int B, int C );


void ag_XSmooth( register ag_DataType *hData, register ag_ElementType *elem );
void ag_YSmooth( register ag_DataType *hData, register ag_ElementType *elem );

void ag_IF( register ag_DataType *hData, ag_ElementType *elem, int16 storeIndex );
void ag_ELSE( register ag_DataType *hData, ag_ElementType *elem );
void ag_EIF( register ag_DataType *hData, ag_ElementType *elem );
void ag_JMPR( register ag_DataType *hData, ag_ElementType *elem, long positionOfTarget );

void ag_LINK( ag_DataType *hData, register ag_ElementType* elem, int16 *ooz, int16 doX, int16 doY, short minDist, short round, char c1, char c2, int from, int to );
void ag_ADJUST( ag_DataType *hData, register ag_ElementType* elem, int16 doX, int16 doY, int16 anchor, int16 from, int16 to );
void ag_ASSURE_AT_LEAST_EQUAL( ag_DataType *hData, ag_ElementType* elem, int16 doX, int16 prev, int16 point );
void ag_ASSURE_AT_MOST_EQUAL( ag_DataType *hData, ag_ElementType* elem, int16 doX, int16 prev, int16 point );
void ag_ASSURE_AT_MOST_EQUAL2( ag_DataType *hData, register ag_ElementType* elem, int16 doX, int16 prev, int16 point1, int16 point2 );

void ag_ADJUSTSPACING( register ag_DataType *hData, register ag_ElementType* elem, int32 lsbPoint, int32 minPoint, int32 maxPoint, int32 rsbPoint );

void ag_INIT_STORE( ag_DataType *hData );
void AG_CHECK_AND_TWEAK( ag_DataType *hData, ag_ElementType* elem, int16 doX, int16 cvtNumber, int16 ptA );

#ifdef ENABLE_AUTO_HINTING
	int ag_CheckArrayBounds( ag_DataType *hData );
	void ag_MovePushDataIntoInstructionStream( ag_DataType *hData, long ttcodePosition0, long ttdataPosition0 );
#endif /* ENABLE_AUTO_HINTING */

int ag_DoGlyphProgram97( ag_ElementType* elem, ag_DataType *hData );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /*  __T2K_AGRINDINT__ */

#endif /* ENABLE_AUTO_GRIDDING_CORE */



/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/agridint.h 1.5 1999/10/19 16:21:07 shawn release $
 *                                                                           *
 *     $Log: agridint.h $
 *     Revision 1.5  1999/10/19 16:21:07  shawn
 *     Removed the UNUSED() macro from function prototype statements.
 *     
 *     Revision 1.4  1999/10/18 16:57:49  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:10:58  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:56:06  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/

