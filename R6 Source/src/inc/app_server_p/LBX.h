//--------------------------------------------------------------------
//	
//	LBX.h
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2001 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _LBX_H_
#define _LBX_H_

#include <SupportDefs.h>

/* Shared part for all revisions */
typedef struct {
	uchar		signature[4]; 		// always 'LBX'
	uint8		status;				// bit 0 : 0 is regular, 1 is rotated.
									// bit 1-3 : custom per revision
									// bits 4-7 : revision ID, 0 or 1 for now
	uint8		bitmap_count;		// count of bitmap [range: 1 to 255].
} LBX_header;

typedef struct {
	uint16		width;				// dimensions
	uint16		height;
	uint16		cmap_index;			// cmap index in the cmap table
	uint16		scan_index;			// index of the first scan ref in scan tables
	int16		similar_delta[2];	// similarity offsets in the scanline table
} LBX_bitmap_header;


/*---------------------------------------------------------------------------
  File structure (revision ID 0) :
  - LBX header					(LBX_header0)
  - LBX bitmap header array		(LBX_bitmap_header0)
  - name list					(C strings)
  - cmap buffer 				(uint16)
  - scanline buffer 			(2 or 3 bytes)
  - scanbitmap buffer 			(uint8)
  - pixel buffer 				(raw)
---------------------------------------------------------------------------*/
  
typedef struct {
	uchar		signature[4]; 		// always 'MBCP'
	uint8		status;				// bit 0 : 0 is regular, 1 is rotated.
									// bit 1 : 0 for 2 bytes scanline ref
									//				uint16		offset;
									//		   1 for 3 bytes scanline ref
									//				uint24		offset;
									// bits 2-3 : 0 for now, reserved.
									// bits 4-7 : revision ID, 1 for now 
	uint8		bitmap_count;		// count of bitmap [range: 1 to 255].
	uint16		cmap_count;			// count of entries in the cmap table
	uint32		scanline_count;		// count of entries in scanline ref tables
} LBX_header0;

typedef struct {
	uint16		width;				// dimensions
	uint16		height;
	uint16		cmap_index;			// cmap index in the cmap table
	uint16		scan_index;			// index of the first scan ref in scan tables
	int16		similar_delta[2];	// similarity offsets in the scanline table
} LBX_bitmap_header0;

/*---------------------------------------------------------------------------
  File structure (revision ID 1) :
  - LBX header					(LBX_header1)
  - LBX bitmap header array		(LBX_bitmap_header1)
  - name list					(C strings)
  - cmap buffer 				(uint16)
  - scanline compressed buffer 	(raw)
  - pixel buffer 				(raw)
---------------------------------------------------------------------------*/

typedef struct {
	uchar		signature[4]; 		// always 'MBCP'
	uint8		status;				// bit 0 : 0 is regular, 1 is rotated.
									// bit 1 : 0 is regular names, 1 compressed
									// bits 2-3 : 0 for now, reserved.
									// bits 4-7 : revision ID, 1 for now 
	uint8		bitmap_count;		// count of bitmap [range: 1 to 255].
	uint16		cmap_count;			// count of entries in the cmap table
	uint32		scanline_length;	// length of the scanline compressed buffer
} LBX_header1;

typedef struct {
	uint16		width;				// dimensions
	uint16		height;
	uint16		cmap_index;			// cmap index in the cmap table
	uint16		scan_index;			// index of the first scan ref in scan tables
	int16		similar_delta[2];	// similarity offsets in the scanline table
	uint32		scan_offset;		// offset in the scanline compressed buffer
	uint32		pixel_offset;		// offset in the pixel table
} LBX_bitmap_header1;

#endif
