/* ++++++++++
	$Source: /net/bally/be/rcs/src/boot/all/rom.h,v $
	$Revision: 1.8 $
	Author: herold
	Date:	Oct 14 1993
	Copyright (c) 1994-1995 by Be Incorporated.  All Rights Reserved.

	Structures at the start of a rom image.
+++++ */

#ifndef _ROM_H
#define _ROM_H

#define DATELEN 32

/* -----
   romstart is the layout of the start of a rom image
----- */
typedef struct {
	unsigned long	entry;				/* -> entry point */
	unsigned long	TOC;				/* PEF TOC register */
	unsigned long	serial_num[2];		/* sapce for a serial number (nub only) */
	unsigned long	checksum;			/* image checksum (xcept serial_num) */
	unsigned long	addr;				/* absolute addr of this image */
	unsigned long	size;				/* size of image */
	unsigned long	segtable;			/* offset to rom seg table */
	char			date[DATELEN];		/* build date, in ascii */
} romstart;

/* -----
   romseg is the layout of the entries in the 'rom segment table', which
   identifies all the segments in the rom, where they mey need to be
   copied to, and how much memory to zero after copying.
----- */

typedef struct {
	unsigned long	size;			/* size of segment */
	unsigned long	offset;			/* offset to segment in rom */
	unsigned long	addr;			/* destination addr, or -1 if not moved */
	unsigned long	zsize;			/* # bytes to zero after (size) */
} romseg;


#endif

