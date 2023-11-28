/*
 * Tsimem.h
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

#ifndef __TSIMEM__
#define __TSIMEM__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#include <setjmp.h>

/* #define TRACK_RAM */
#define T2K_FB_GLYPH		0
#define T2K_FB_POINTS		1
#define T2K_FB_IOSTREAM		2
#define T2K_FB_HINTS		3
#define T2K_FB_SC			4
#define T2K_FB_SC_BITMAP	5
#define T2K_FB_FILTER		6
#define T2K_MAX_FAST_BLOCKS 7

typedef struct {
	/* private */
	unsigned long stamp1; 		/* == MAGIC1 */
	long numPointers;			/* Number of allocated memory pointers */
	long maxPointers;			/* current maximum limit on number of pointers */
	void	 **base;
	/* semi-private */
	jmp_buf env;				/* Use the 	tsi_Assert() below */		
#ifdef TRACK_RAM
	long totRAM;
	long maxRAM;
#endif
	/* private */
	void			*fast_base[T2K_MAX_FAST_BLOCKS];
	unsigned long 	fast_size[T2K_MAX_FAST_BLOCKS];
	int				fast_free[T2K_MAX_FAST_BLOCKS];
	
	unsigned long	ii;
	unsigned long state;
	unsigned long stamp2; 		/* == MAGIC2 */
} tsiMemObject;

/* Normally returns 0 in *errCode */
tsiMemObject *tsi_NewMemhandler( int *errCode );
void tsi_DeleteMemhandler( tsiMemObject *t );

void *tsi_AllocMem( register tsiMemObject *t, size_t size );
void *tsi_ReAllocMem( register tsiMemObject *t, void *p, size_t size );
void tsi_DeAllocMem( register tsiMemObject *t, void *p );


void tsi_EmergencyShutDown( tsiMemObject *t );

/* only for use by tsi_Assert */
void tsi_Error( tsiMemObject *t, int errcode );
/* only for internal T2K use */
#define T2K_STATE_ALIVE 0xaa005501
#define T2K_STATE_DEAD	0x5500aaff

/* only for internal T2K use */
void *tsi_FastAllocN(  register tsiMemObject *t, size_t size, int N );
/* void tsi_FastDeAllocN( register tsiMemObject *t, void *p, int N ); */
/* Only free if not a fast pointer */
/* Do a trick with with a do-while(0) statement to avoid an exposed if/else pair! */
#define tsi_FastDeAllocN( t, p, N ) do { if ( p == t->fast_base[N] ) t->fast_free[N] = true; else tsi_DeAllocMem( t, p ); } while(0)
#define tsi_FastReleaseN( t, N ) tsi_DeAllocMem( t, t->fast_base[N] ), t->fast_base[N] = NULL, t->fast_size[N] = 0, t->fast_free[N] = true;
#define tsi_FastSizeN( t, N ) (t->fast_size[(N)])
void tsi_FreeFastMemBlocks( tsiMemObject *t );


/*
#define tsi_Assert( t, cond, errcode ) assert( cond )
*/
#define tsi_Assert( t, cond, errcode ) if ( !(cond) ) tsi_Error( t, errcode )


#define T2K_ERR_MEM_IS_NULL			10000
#define T2K_ERR_TRANS_IS_NULL		10001
#define T2K_ERR_RES_IS_NOT_POS		10002
#define T2K_ERR_BAD_GRAY_CMD		10003
#define T2K_ERR_BAD_FRAC_PEN		10004
#define T2K_ERR_GOT_NULL_GLYPH		10005
#define T2K_ERR_TOO_MANY_POINTS		10006
#define T2K_ERR_BAD_T2K_STAMP		10007
#define T2K_ERR_MEM_MALLOC_FAILED	10008
#define T2K_ERR_BAD_MEM_STAMP		10009
#define T2K_ERR_MEM_LEAK			10010
#define T2K_ERR_NULL_MEM			10011
#define T2K_ERR_MEM_TOO_MANY_PTRS	10012
#define T2K_ERR_BAD_PTR_COUNT		10013
#define T2K_ERR_MEM_REALLOC_FAILED	10014
#define T2K_ERR_MEM_BAD_PTR			10015
#define T2K_ERR_MEM_INVALID_PTR		10016
#define T2K_ERR_MEM_BAD_LOGIC		10017
#define T2K_ERR_INTERNAL_LOGIC		10018
#define T2K_ERR_USE_PAST_DEATH		10019
#define T2K_ERR_NEG_MEM_REQUEST		10020
#define T2K_BAD_CMAP				10021
#define T2K_UNKNOWN_CFF_VERSION		10022
#define T2K_MAXPOINTS_TOO_LOW		10023
#define T2K_EXT_IO_CALLBACK_ERR     10024
#define T2K_BAD_FONT				10025

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __TSIMEM__ */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/tsimem.h 1.8 2000/06/14 21:32:45 reggers release $
 *                                                                           *
 *     $Log: tsimem.h $
 *     Revision 1.8  2000/06/14 21:32:45  reggers
 *     Added T2K_BAD_FONT
 *     Revision 1.7  1999/11/19 01:43:00  reggers
 *     Make non-ram stream error return possible.
 *     Revision 1.6  1999/10/19 16:19:25  shawn
 *     Added '#define T2K_FB_SC_BITMAP 5' statement.
 *     Changed tsi_FastDeAllocN() to use a 'Do {} While(0)' construct.
 *     
 *     
 *     
 *     Revision 1.5  1999/10/18 16:57:34  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.4  1999/10/07 17:43:11  shawn
 *     Added error manifest T2K_MAXPOINTS_TOO_LOW.
 *     Revision 1.3  1999/09/30 15:12:35  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:51  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
