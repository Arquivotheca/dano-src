/*//////////////////////////////////////////////////////////////////////*/
/*                                                                      */
/*	Contains Intel proprietary and secret confidential information      */
/*                                                                      */
/*//////////////////////////////////////////////////////////////////////*/
/*//////////////////////////////////////////////////////////////////////*/
/*                                                                      */
/*              INTEL CORPORATION PROPRIETARY INFORMATION               */
/*                                                                      */
/*      This software is supplied under the terms of a license          */
/*      agreement or nondisclosure agreement with Intel Corporation     */
/*      and may not be copied or disclosed except in accordance         */
/*      with the terms of that agreement.                               */
/*                                                                      */
/*//////////////////////////////////////////////////////////////////////*/
/*                                                                      */
/* Copyright (C) 1994-1997 Intel Corp.  All Rights Reserved.            */
/*                                                                      */
/*//////////////////////////////////////////////////////////////////////*/

#if !defined __TSC_H__
#define __TSC_H__

/* 	macros & function prototypes for accessing Pentium(TM) processor
	time stamp counter.
*/

/* Clock speed, in MHz, of CPU */
#define TSC_CLOCK	90

/* Read counter to local variables */
#define GET_TSC( hi, lo ) { \
	__asm __emit 0fh \
    __asm __emit 31h \
    __asm mov lo, eax \
    __asm mov hi, edx \
}

/* Typedefs and prototypes to use pentime functions */

/* cycle and time counts are all 64 bit values. define 64-bit type */
typedef struct _DDWORD {
	U32 lo;
	U32	hi;
} DDWORD;

void init_pentime( void );
void pentime( DDWORD *ptime );
void pencycle( DDWORD *pcycle );
void cycletomicrosec( DDWORD *pTim );
void microsectocycle( DDWORD *pTim );

#endif /* __TSC_H__ */


