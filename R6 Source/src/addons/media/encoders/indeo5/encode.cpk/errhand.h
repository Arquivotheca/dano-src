/*
 *
 *               INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *    This listing is supplied under the terms of a license agreement
 *      with INTEL Corporation and may not be copied nor disclosed
 *        except in accordance with the terms of that agreement.
 *
 *************************************************************************
 *
 *               Copyright (C) 1994-1997 Intel Corp.
 *                         All Rights Reserved.
 *
 */

/*	ERRHAND.H
 *
 *	This file contains the #defines necessary for the error handling --
 *	codes for each file in the project that uses a longjmp to handle
 *	an error.
 */

#ifdef INCLUDE_NESTING_CHECK
#ifdef __ERRHAND_H__

#pragma message("***** ERRHAND.H Included Multiple Times")

#endif
#endif

#ifndef __ERRHAND_H__
#define __ERRHAND_H__

/* In the 32-bit field of information, the first 16 are used for
 * line number information, the next 8 for the file code, and the
 * final 8 for the type of error.  These offsets specify the number
 * of bits to shift the entries by in composing the entire field.
 */
#define TYPE_OFFSET	24
#define FILE_OFFSET 16
#define LINE_OFFSET  0
#define TYPE_MASK 0xFF000000
#define FILE_MASK 0x00FF0000
#define LINE_MASK 0x0000FFFF

/* Codes for each file --
 *		any modifications need to be to the array of file names
 *		in encpia.c as well, as it is indexed by these values
 *		NUMFILES needs to match up with the number of the bottom
 *		file listed (+1 for item 0 ).
 */
#define BIDISM		0x00
#define BSDBG		0x01
#define BSUTIL		0x02
#define	ENBS		0x03
#define ENMESRCH 	0x04
#define ENNTRY		0x05
#define ENNTRYNS	0x06
#define ENWAV		0x07
#define	HUFFTBLS	0x08
#define	MATRIX		0x09
#define MC			0x0A
#define TKSYS		0x0B
#define ENCPIA		0x0C
#define ENCTRANS	0x0D
#define ENSEG		0x0E
#define CDEC		0x0F
#define DENTRYNS	0x10
#define DEWAV		0x11
#define DEXFRM		0x12
#define PARSEBS		0x13

#define NUMFILES 	20
#define NAMELENGTH 	9

#define ERR_NULL_PARM 		0x00
#define	ERR_BAD_PICTYPE 	0x01
#define ERR_BAD_PARM		0x02
#define ERR_BAD_VECT		0x03
#define ERR_BITSTREAM		0x04
#define ERR_FRAMETYPE		0x05
#define ERR_BUFFSIZE		0x06
#define ERR_NO_RV_SWAP		0x07
#define ERR_ERROR			0xFF

#ifdef DEBUG

extern char gac8Files[NUMFILES][NAMELENGTH];	
#endif



#endif 
