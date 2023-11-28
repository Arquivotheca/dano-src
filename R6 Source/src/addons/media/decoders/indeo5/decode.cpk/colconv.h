/*
 *
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *    This listing is supplied under the terms of a license agreement
 *      with INTEL Corporation and may not be copied nor disclosed
 *        except in accordance with the terms of that agreement.
 *
 *
 *
 *               Copyright (c) 1994-1995 Intel Corporation.
 *                         All Rights Reserved.
 *
 */

/*************************************************************************
 *	colconv.h			
 * 
 *  Macro definitions used in color converters; endian-dependency
 *  is focussed in these as much as possible.
 *
 *************************************************************************/

#include <ByteOrder.h>

/* Bit Masks are endian-independent */
/* Mask to AND with a nibble to extract information on a pel */
#define PEL0_BITMASK 8
#define PEL1_BITMASK 4
#define PEL2_BITMASK 2
#define PEL3_BITMASK 1

#if B_HOST_IS_LENDIAN  /* Little endian */

/* Mask to AND with 32 bits to extract a pel */
#define PEL0_BYTEMASK 0x000000FF
#define PEL1_BYTEMASK 0x0000FF00
#define PEL2_BYTEMASK 0x00FF0000
#define PEL3_BYTEMASK 0xFF000000
/* Amount to shift to that byte to access it as an 8-bit value */
#define PEL0_SHIFT 0
#define PEL1_SHIFT 8
#define PEL2_SHIFT 16
#define PEL3_SHIFT 24

#else /* Big Endian */

/* Mask to AND with 32 bits to extract a pel */
#define PEL3_BYTEMASK 0x000000FF
#define PEL2_BYTEMASK 0x0000FF00
#define PEL1_BYTEMASK 0x00FF0000
#define PEL0_BYTEMASK 0xFF000000
/* Amount to shift to that byte to access it as an 8-bit value */
#define PEL3_SHIFT 0
#define PEL2_SHIFT 8
#define PEL1_SHIFT 16
#define PEL0_SHIFT 24

#endif
