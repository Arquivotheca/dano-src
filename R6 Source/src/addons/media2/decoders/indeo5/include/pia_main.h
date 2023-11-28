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

/*
 *  pia_main.h
 *
 *  DESCRIPTION:
 *  The main PIA file.  PIA is the platform independent architecture for
 *  Indeo.  In order to build the CODEC on an arbitrary host, the CODEC 
 *  should only need to #include datatype.h, pia_main.h, and a
 *  module specific PIA header file (pia_cin.h, pia_enc.h, pia_dec.h
 *  and pia_cout.h)
 *
 *  Tabs set to 4
 */

#ifndef __PIA_MAIN_H__
#define __PIA_MAIN_H__

/* This file is dependent on the following files:
#include "datatype.h"
*/

#if !defined __DATATYPE_H__
#error "DataType.h required for Pia_Main.h"
#endif

#if __MWERKS__
#define __cdecl
#endif

/*********************************************************************
 *
 * Macros for the PIA interface
 */

/* A codec can support only a limited number of color formats.
 */
#define MAX_NUMBER_OF_COLOR_FORMATS		16


/* An 8-bit color palette has 2^8 color entries.
 */
#define MAX_NUMBER_OF_PALETTE_ENTRIES	256   


/*********************************************************************
 *
 * Enumerated types for the PIA interface
 */

/* COLOR_FORMAT
 * The COLOR_FORMAT_LIST enumeration defines the possible external and
 * internal image data formats.  Use COLOR_FORMAT for PIA structures.
 */
typedef enum {
	CF_UNDEFINED,			/* uninitialized color format */
	CF_QUERY,				/* Query the PIA module for the format */
	CF_GRAY_1,				/* Black and white */
	CF_GRAY_2,				/* 4 level gray scale */
	CF_GRAY_4,				/* 16 level gray scale */
	CF_GRAY_8,				/* 256 level gray scale */
	CF_CLUT_8,				/* 8-bit color lookup table */
	CF_XRGB16_1555,			/* 16-bit color 5-bits each: red, green, blue */
	CF_RGB16_565,			/* 16-bit color variant */
	CF_RGB16_655,			/* 16-bit color variant */
	CF_RGB16_664,			/* 16-bit color variant */
	CF_RGB24,				/* 24-bit color, 8-bits each: red, green, blue */
	CF_BGR24,				/* 24-bit color, 8-bits	each: blue, green, red */
	CF_IF09,				/* hardware accelerated color conversion. */
	CF_PLANAR_YVU9_8BIT,	/* YVU9 color format */
	CF_YVU9 = CF_PLANAR_YVU9_8BIT,
	CF_PLANAR_YVU12_8BIT,	/* YVU12 color format */
	CF_YV12 = CF_PLANAR_YVU12_8BIT,
	CF_IV50,				/* Indeo 5.0 bitstream */
	CF_XBGR32,
	CF_RGBX32,
	CF_BGRX32,
	CF_XRGB32,

#if !defined CMD_LINE_DEC
	CF_IYUV,				/* Y, U, V */
	CF_I420 = CF_IYUV,
	CF_YVU12= CF_IYUV,
	CF_CLPL, /* we think this is YVU12 */
	CF_YUY2,
	CF_YUYV = CF_YUY2,
	CF_Y211 = CF_YUY2,	/* difference is subtle, interpretation only */
	CF_UYVY,
#endif /* !CMD_LINE_DEC */

	MAX_CF
} COLOR_FORMAT_LIST;
typedef PIA_ENUM COLOR_FORMAT;


/*	ENVIRONMENT MODE
 *	The Environment Mode enumeration is placed in each module 
 *  to indicate the desired operation.
 */
typedef enum {
	EM_UNKNOWN,				/* uninitialized value */
	EM_STEPPING,			/* frame by frame mode - no dropping */
	EM_PREROLL,				/* decode, but do not display */
	EM_NORMAL,				/* normal - frames may be dropped */
	EM_HURRYUP,				/* hurried - do as little work as possible */
	MAX_EM
} ENVIRONMENT_MODE_LIST;
typedef PIA_ENUM ENVIRONMENT_MODE;


/* KEY_FRAME_RATE_KIND
 * Encoders support different kinds of key frame rate control.  The following 
 * enumeration is used to identify the mechanism.  Use KEY_FRAME_RATE_KIND
 * in PIA defined structures.
 */
typedef enum {
	KEY_UNDEFINED,					/* uninitialized value */
	KEY_AVERAGE,					/* key frame at interval on average */
	KEY_MAXIMUM_FRAMES_BETWEEN,		/* maximum interval between key frames */
	KEY_FIXED,						/* fixed interval between frames */
	KEY_NONE,						/* no key frames (except the first one ). */
	MAX_KEY_KIND
} KEY_FRAME_RATE_KIND_LIST;
typedef PIA_ENUM KEY_FRAME_RATE_KIND;


/* OUTPUT_DESTINATION
 * The OUTPUT_DESTINATION enumeration is used to specify the location where 
 * the image should be displayed.  Use it in structures defined in PIA.
 */
typedef enum {
	DEST_UNDEFINED,					/* uninitialized value */
	DEST_QUERY,						/* query for the desired location */
	DEST_SLIDE_BANK_VIDEOHW,		/* video hardware */
	DEST_BUFFER,					/* contiguous memory */
	DEST_BITMAP_ARRAY,				/* array of bitmaps */
	MAX_DEST
} OUTPUT_DESTINATION_LIST;
typedef PIA_ENUM OUTPUT_DESTINATION;


#if 0

Add this at sometime in the future?

/* OUTPUT_GOAL
 * The OUTPUT_GOAL enumeration is used to further constrain the output color 
 * converter.
 */
typedef enum {
	GOAL_UNDEFINED,					/* uninitialized value */
	GOAL_BEST_OVERALL,				/* balance speed and quality */
	GOAL_BEST_SPEED,				/* emphasize speed of operation */
	GOAL_BEST_QUALITY,				/* emphasize picture quality over speed */
	MAX_GOAL
} OUTPUT_GOAL_LIST;
typedef PIA_ENUM OUTPUT_GOAL;

#endif


/* PIA_RETURN_STATUS
 * The PIA_RETURN_STATUS enumeration defines function exit status. 
 * A high level description of each would be helpful here.  For example:
 */
typedef enum {
	PIA_S_UNDEFINED,		/* No function should ever return this value. */
	PIA_S_OK,				/* No error occurred. */
	PIA_S_ERROR,			/* An internal error occurred during processing. */
	PIA_S_INSTANCE_FATAL,	/* A fatal error which disables this instance */
	PIA_S_GLOBAL_FATAL,		/* A fatal error which disables all instances */
	PIA_S_DONT_DRAW,		/* Do not draw this frame. */
	PIA_S_OUT_OF_MEMORY,
	PIA_S_BAD_CONTROL_VALUE,	/* Bad input value */
	PIA_S_UNSUPPORTED_CONTROL,	/* unsupported control requested */
	PIA_S_BAD_IMAGE_DIMENSIONS,
	PIA_S_BAD_SETUP_VALUE,
	PIA_S_BAD_COLOR_FORMAT,
	PIA_S_BAD_DESTINATION,
	PIA_S_BAD_DIRECTION,
	PIA_S_UNSUPPORTED_FUNCTION,
	PIA_S_BAD_COMPRESSED_DATA,
	PIA_S_CHECKSUM_ERROR,
	PIA_S_MISSING_KEY_FRAME,
	PIA_S_TIMEOUT,
	PIA_S_KEY_FAILURE,
	PIA_S_TBD,				/* Internal use only.  Redefine before release. */
	PIA_S_SKIP_TO_KEY,
	MAX_PIA_S
} PIA_RETURN_STATUS_LIST;
typedef PIA_ENUM PIA_RETURN_STATUS;   /* used as a function return type */


/* TRANSPARENCY_KIND 
 * Transparency can be handled in different ways.  This type enumerates those
 * ways.
 */
typedef enum {
	TK_UNDEFINED,				/* uninitialized value */
	TK_SELECTIVE_UPDATE,		/* blit using the transparency mask */
	TK_IGNORE,					/* Fill entire retangle */
	MAX_TK
} TRANSPARENCY_KIND_LIST;
typedef PIA_ENUM TRANSPARENCY_KIND;
typedef TRANSPARENCY_KIND FAR * PTRANSPARENCY_KIND;


/* CPU
 * Cpu refers to the family of the type of processor where the stream was
 * encoded, is being encoded or being decoded.
 */
typedef enum {
	CPU_UNKNOWN			= 0,     /* An unknown family of processor. */
	CPU_80386			= 3,     /* 80386 */
	CPU_80486			= 4,     /* 80486 */
	CPU_P5				= 5,	 /* Pentium */
	CPU_P6				= 6,	 /* Pentium Pro */
	CPU_P7				= 7,	 /* ? */

	CPU_PPC601			= 11,    /* Power PC 601 */
	CPU_PPC603			= 13,
	CPU_PPC604			= 14,
	CPU_PPC620			= 17,

	MAX_CPUS
} CPU_LIST;
typedef PIA_ENUM CPU;


/* HIVE_CONTROLS are switches that express functionality over and above 
 * rectangular playback.  Every control flag is associated with one or
 * more functions which implement that control.  Every PIA module defines
 * supported and suggested controls.  
 */
typedef enum {
	HIVE_CONTROLS_VALID				= (1UL<<31)

} HIVE_CONTROLS_LIST;
typedef PIA_ENUM HIVE_CONTROLS;


/* ENVIRONMENT_INFO
 * The ENVIRONMENT_INFO structure contains information about the
 * environment the CODEC module is operating in.  The HIVE layer of
 * code fills in this structure.
 */
typedef struct {
	PINSTANCE		pInstance;			/* pointer to top level instance data */

	U32				uPlatformId;		/* Assigned by Intel */
	U32				uHiveBuildNumber;

	CPU				eFamily;			/* CPU */
	U32				uModel;				/* version of the CPU. Information is specific to the family
	                                       of CPU.  An example of the model information is P54 vs P55 */
	U32				uStepping;			/* fab version */
	PIA_Boolean		bMMXavailable;		/* MMX instructions are allowed and available */

	PIA_Boolean		bHasMSR;			/* Model Specific Registers */
	PIA_Boolean		bHasTSC;			/* Time stamp counter */
	PIA_Boolean		bHasPMC;			/* What is PMC? */

	U32				uTimerScale;        /* HiveReadTime() / uTimerScale always equals the time in
										   milliseconds */
	PIA_Boolean		bInterlaceInput;    /* Input stream was from an interlaced device */
	PIA_Boolean		bInterlaceOutput;   /* Output stream to interlaced device */

	HIVE_CONTROLS	hcSupportedControls;
	HIVE_CONTROLS	hcSuggestedControls;

} ENVIRONMENT_INFO;
typedef ENVIRONMENT_INFO FAR * PTR_ENVIRONMENT_INFO;


/*********************************************************************
 *
 * Structures for the PIA interface
 *
 *********************************************************************/

/* BGR_PALETTE
 * The BGR_PALETTE structure is used to specify a palette.  An initialized
 * BGR_PALETTE will have the BGR_PALETTE_TAG in the tag field.
 * It is the job of the xxx (e.g. client application) to set the tag field.
 */
typedef struct {
	U16				u16Tag;
	U16				u16NumberOfEntries;
	PTR_BGR_ENTRY	pbgrTable;
} BGR_PALETTE;
typedef BGR_PALETTE FAR * PTR_BGR_PALETTE;
#define BGR_PALETTE_TAG	0x5250


/* COLOR_RANGE
 * The COLOR_RANGE structure is used to define a range of colors.
 * An initialized COLOR_RANGE will have the COLOR_RANGE_TAG in the tag field.
 */
typedef struct {
	U16				u16Tag;
	U8				u8BlueLow;
	U8				u8BlueHigh;
	U8				u8GreenLow;
	U8				u8GreenHigh;
	U8				u8RedLow;
	U8				u8RedHigh;
} COLOR_RANGE;
typedef COLOR_RANGE FAR * PTR_COLOR_RANGE;
#define COLOR_RANGE_TAG	0x7227


/* TRANSPARENCY_MASK
 * A TRANSPARENCY_MASK structure is used to describe the transparency
 * mask in terms of pixels size and buffer size.  It is assumed to have
 * the same height and width as the frame it came from.
 */
typedef struct {
	U16				u16Tag;
	COLOR_FORMAT	cfPixelSize;		/* One of CF_GRAY_* */
	U32				uMaskSize;			/* total size of data */
	PU8				pu8TransBuffer;
	U32				uStride;			/* Buffer width */
} TRANSPARENCY_MASK;
typedef TRANSPARENCY_MASK FAR * PTR_TRANSPARENCY_MASK;
typedef PTR_TRANSPARENCY_MASK FAR * PTR_PTR_TRANSPARENCY_MASK;
#define TRANSPARENCY_MASK_TAG	0x7654


/* SUPPORTED_ALGORITHMS
 * The SUPPORTED_ALGORITHMS structure is used by the decoder to specify the 
 * supported algorithms.  This allows a decoder to support a number of 
 * different backward compatible algorithms.  For example, the HQV 3.2 decoder 
 * also support 3.1 and 3.0.  An initialized structure will have the
 * SUPORTED_ALGORITHMS_TAG in the u16tag member.
 */
typedef struct {
	U16				u16Tag;
	U16				u16NumberOfAlgorithms;
	COLOR_FORMAT	eList[MAX_NUMBER_OF_COLOR_FORMATS];
} SUPPORTED_ALGORITHMS;
typedef SUPPORTED_ALGORITHMS FAR * PTR_SUPPORTED_ALGORITHMS;
#define SUPPORTED_ALGORITHMS_TAG 0x5341

/* PLANAR_IO 
 * The PLANAR_IO structure is used to communicate PLANAR data between
 * components of the CODEC.
 */
typedef struct {
/*	YVU Data */
	PU8			pu8Y;				/* ptr to Y plane data.					*/
	PU8			pu8V;				/* ptr to V plane data.					*/
	PU8			pu8U;				/* ptr to U plane data.					*/
	U32			uYPitch;			/* y pitch.								*/
	U32			uVUPitch;			/* u & v pitch							*/
/*	Color Convert Info */
	U32			uCCFlags;			/* various flags to be decided.			*/
	RectSt		rCCRect;			/* color convert rectangle.				*/

/*	Forground Transparency */
	PU8			pu8FGTransMask;		/* app defined forground trans mask		*/
	U32			uFGTransPitch;	/* app defined forground mask pitch		*/
/*	Internal Transparency */
	PU8			pu8LDMask;			/* internal local decode mask for cc	*/
	PU8			pu8TransMask;		/* internal transparency mask 			*/
	U32			uTransPitch;		/* internal transparency mask pitch		*/
	U32			uMaskSize;			/* internal mask total data size		*/
	BGR_ENTRY	bgrXFill;			/* transparency fill color				*/
/*	Exposed Transparency */
	PU8			pu8ExpTransMask;	/* exposed transparency mask			*/
	U32			uExpTransPitch;	/* exposed transparency mask pitch		*/
/*	Note: both forground and exposed mask pitches will require new vfw_spec */
/*	support */
}	PLANAR_IO;
typedef PLANAR_IO FAR * PTR_PLANAR_IO;

/**************************************************************
 * H I V E  C A L L B A C K  F U N C T I O N S.               *
 **************************************************************/
/* The callback functions defined using reserved names
 */

/* Local memory */
extern NPVOID_LOCAL			HiveLocalAllocPtr( U32 uSize, PIA_Boolean bZeroInit);
	/* Allocate uSize bytes of local heap space, and optionally zero it.
	 * If all local memory is exhausted returns (NPLOCAL_PTR) 0.
	 */

extern PIA_RETURN_STATUS	HiveLocalFreePtr( NPVOID_LOCAL );
	/* Release local heap space that was originally allocated with
	 * HiveLocalAllocPtr().
	 */

extern PIA_RETURN_STATUS	HiveLocalPtrCheck( NPVOID_LOCAL vlpPtr, U32 uLength );
	/* Validate that vlpPtr pointers to a valid buffer of length uLength.
	 * This function ensures that user memory is valid in order to prevent a GPF.
	 */


/* Global memory */
extern PVOID_GLOBAL			HiveGlobalAllocPtr( U32 uSize, PIA_Boolean bZeroInit );
	/* Allocate uSize bytes of global heap space, and optionally zero it.
	 * If all global memory is exhausted returns (PVOID_GLOBAL) 0.
	 */

extern PIA_RETURN_STATUS	HiveGlobalFreePtr( PVOID_GLOBAL );
	/* Release global heap space that was originally allocated with
	 * HiveGlobalAllocPtr().
	 */

extern PIA_RETURN_STATUS	HiveGlobalPtrCheck( PVOID_GLOBAL vgpPtr, U32 uLength );
	/* Validate that vgpPtr pointers to a valid buffer of length uLength.
	 * This function ensures that user memory is valid in order to prevent a GPF.
	 */

/* Shared memory */
extern PVOID_GLOBAL HiveAllocSharedPtr(U32 uBytes, PU8 pc8Name, PPIA_Boolean pbExists);
extern PIA_RETURN_STATUS HiveFreeSharedPtr(PVOID_GLOBAL vgpPtr);

/* Global memory by handle */
extern GLOBAL_HANDLE		HiveGlobalAllocHandle( U32 uSize, PIA_Boolean bZeroInit );
	/* Allocate uSize bytes of global heap space, and optionally zero it.
	 * This function returns a handle not a true pointer.  Use
	 * HiveGlobalLockHandle() to retreive a true pointer to the buffer.
	 * If all local memory is exhausted returns (GLOBAL_HANDLE) 0.
	 */

extern PVOID_GLOBAL			HiveGlobalLockHandle( GLOBAL_HANDLE );
	/* Locks global memory referenced by ghHandle into main memory.  Prevents
	 * memory from being moved or paged out.  Must be paired with
	 * HiveGlobalUnlockHandle().
	 */

extern PIA_RETURN_STATUS	HiveGlobalUnlockHandle( GLOBAL_HANDLE );
	/* Unlocks global memory referenced by ghHandle.  Allows the memory to
	 * be moved or paged out.  Must be paired with HiveGlobalLockHandle().
	 */

extern PIA_RETURN_STATUS	HiveGlobalFreeHandle( GLOBAL_HANDLE );
	/* Release global heap space that was originally allocated with
	 * HiveGlobalAllocHandle().  All locks on this data must be released.
	 */


/* Shared global memory */
extern PVOID_GLOBAL			HiveAllocSharedPtr( U32 uSize, PU8 pc8Name, PPIA_Boolean pbExists );
	/* Allocates uSize bytes of global memory that is shared amoung all
	 * instances of the codec.  The memory is known by a name, pc8Name.
	 * If already allocated by another instance, pbExists will be set to TRUE.
	 */

extern PIA_RETURN_STATUS	HiveFreeSharedPtr( PVOID_GLOBAL );
	/* Release global heap space that was originally allocated with
	 * HiveAllocSharedPtr().
	 */


/* Critical sections */
extern MUTEX_HANDLE			HiveCreateMutex( PU8 pc8Name);
	/* Create a mutex globally known by the name, pc8Name.
	 */

extern PIA_RETURN_STATUS	HiveFreeMutex( MUTEX_HANDLE muHandle );
	/* Free the mutex created by HiveCreateMutex().
	 */

extern PIA_RETURN_STATUS	HiveBeginCriticalSection( MUTEX_HANDLE muHandle, U32 uTimeout );
	/* Attempt to enter a critical section.  The Mutex, muHandle, must have
	 * been created by HiveCreateMutex().  The function fails after waiting
	 * iTimeout msec.
	 */

extern PIA_RETURN_STATUS	HiveEndCriticalSection( MUTEX_HANDLE muHandle );
	/* Allow another thread ato enter the critical section.  Call only after
	 * successfully entering the critical section with
	 * HiveBeginCriticalSection().
	 */


/* Misc */
extern U32					HiveReadTime( void );
	/* Reads the system clock and returns the current time.  The granularity
	 * of this clock is varible, and may change from system to system. To
	 * obtain elapsed time in milliseconds:
	 *
	 *      U32 StartTime = HiveReadTime();
	 *
	 *      <do something>
	 *
	 *		elapsed_milliseconds = (HiveReadTime() - StartTime) / uTimerScale
	 *
	 * (uTimerScale is stored in the ENVIRONMENT_INFO struct.)
	 */

extern void HiveReadTimeInit(PIA_Boolean has_tsc);
	/* Tells HiveReadTime to use the Pentium TSC timer if it is available (indicated by
	 * the has_tsc parameter) else (if has_tsc is 0), HiveReadTime() will use a fallback
	 * timer that has AT LEAST millisecond	granularity.
	 */

extern void	__cdecl	HiveShowWarningMessage( PChr );
	/* Immediately displays a nul terminated string to the user.  This function
	 * is meant to be exercised only in debug builds.  
	 */

extern void __cdecl	HivePrintF( PChr, ... );
	/* Formats and prints a nul terminated string to the debug output device.
	 * Formats like standard C printf.  This can also stream to a file.
	 */

extern void	__cdecl	HivePrintString( PChr );
	/* Prints a nul terminated string to the debug output device.  This can also
	 * stream to a file.
	 */

extern void	__cdecl HivePrintHexInt( U32 uValue );
	/* Prints a hexadecimal number to the debug output device.  This can also
	 * stream to a file.
	 */

extern void	__cdecl HivePrintDecInt( I32 iValue );
	/* Prints a nul signed interger number to the debug output device.  This
	 * can also stream to a file.
	 */

#else  /* __PIA_MAIN_H__ */
#error "PIA_MAIN.H already included."
#endif /* __PIA_MAIN_H__ */
