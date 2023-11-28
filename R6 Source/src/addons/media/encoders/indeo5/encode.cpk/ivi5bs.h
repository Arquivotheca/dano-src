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

#ifdef INCLUDE_NESTING_CHECK
#ifdef __IVI5BS_H__
#pragma message("***** IVI5BS.H Included Multiple Times")
#endif
#endif

#ifndef __IVI5BS_H__
#define __IVI5BS_H__

/* --------------------- THE BITSTREAM HEADERS --------------------- */	

/*  The following header represents the bitstream header for the Picture
	level of the bitstream.
*/

/* Header file dependency list:
 include "datatype.h"
 include "mc.h"
 include "hufftbls.h"
 include "qnttbls.h"
*/

/* Verify inclusion of required files */
#ifndef __DATATYPE_H__
#pragma message("ivi5bs.h requires datatype.h")
#endif
#ifndef __MC_H__
#pragma message("ivi5bs.h requires mc.h")
#endif
#ifndef __HUFFTBLS_H__
#pragma message("ivi5bs.h requires hufftbls.h")
#endif
#ifndef __QNTTBLS_H__
#pragma message("ivi5bs.h requires qnttbls.h")
#endif

typedef  U16 QuantTableSt[2][24][64];
typedef QuantTableSt *pQuantTableSt;

typedef struct _BandDscrptSt {
	U8		MVRes;			/* Specifies MV resolution ie) 1/2 pel or 1 pel */
	U8		MacBlockSize;	/* Size of macroblocks - 2 bits */
	U8		BlockSize;		/* Size of blocks - 2 bits */
	U8 		Xform;				/* Transform type */
	U8		ScanOrder;		/* Scan Order Index for default scan orders*/
	U8		QuantMatrix;	/* Quantization matrix set index for default quant set */
} BandDscrptSt, * pBandDscrptSt;
typedef const BandDscrptSt * PCBandDscrptSt;

typedef struct _TransDscrptSt {
	U8		BitDepth;		/* Bit Depth of Transparency Band (currently always 1) */
	PIA_Boolean	UseTransColor;	/* The following trans color is valid */
	U32     uTransXRGB;		/* Representative transparency color */
} TransDscrptSt, * pTransDscrptSt;
typedef const TransDscrptSt * PCTransDscrptSt;

typedef struct _ToolStampSt {
	U8		ToolIndex;
	U8		ToolBuildNum;
	PIA_Boolean ChainBit;
	struct _ToolStampSt *  pNextTool;
} ToolStampSt, * pToolStampSt;
typedef const ToolStampSt * PCToolStampSt;

typedef struct _VersionSt {
	U8		PlatformId;
	U8		BuildNum;
	U8		EncoderId;
	U8		BSVersion;
	PIA_Boolean ChainBit;
	pToolStampSt  pToolStamp;
} VersionSt, * pVerstionSt;
typedef const VersionSt * PCVersionSt;


typedef struct _GOPHdrSt {
	/* The following 8 Booleans are the 8 bits in the GOPFlags */
	PIA_Boolean GOPHdrSizeFlag;  /* Is GOPHdrSize present or not */
	PIA_Boolean IsYVU12;			 /* Is YVU12 (true) or YVU9 (false) */
	PIA_Boolean IsExpBS;		 /* Is Experiemental BS or not? */
	PIA_Boolean Transparency;    /* Is there transparency band or not? */
	PIA_Boolean SkipBitBand;     /* Is there skip bit band? */
	PIA_Boolean UseKeyFrameLock; /* Is the sequence locked (protected) by Access key or not? */
	PIA_Boolean Tiling;			 /* Is tiling used or not? */
	PIA_Boolean Padding;		 /* Is Padding used at the end of the frame or not? */

	U32		GOPHdrSize;		/* optional: GOP Header size in bytes */

	PIA_Boolean bKeyProvided;    /* Access Key provided by the app? */
	U32		AccessKey;			/* 32 bit access key, valid only when bKeyProvided = TRUE */
	U32		FrameLock;		/* optional: The lockword hashed from the access key:
							 * It will be written into the BS 
							 */
	PIA_Boolean bKeyMatched;

	U16		TileWidth;      
	U16		TileHeight;      /* optional: The Tile is either the picture itself 
							  * or smaller square:
							  */
	U8		YDecompLevel;   /* Decomposition level of Y plane */
	U8		VUDecompLevel;  /* Decomposition level of V and U planes */
	U16		PictureWidth;	/* Width of picture - 16 bit */
	U16		PictureHeight;  /* Height of the picture - 16 bit */
	pBandDscrptSt  pBandDscrpts;  /* Band Descriptors */
	pTransDscrptSt pTransDscrpt;  /* Transparency band descriptor */
	
	VersionSt Version;		/* Version number info */

	/* The following are derived from the above */
	U8		VUSubVertical;
	U8		VUSubHorizontal;

} GOPHdrSt, * pGOPHdrSt;
typedef const GOPHdrSt * PCGOPHdrSt;

typedef struct _BSExtSt	{
	U8		NumBytes;
	PU8		pExtData;
	struct _BSExtSt *pNextExt;  /* link to the next field */
} BSExtSt, *pBSExtSt;
typedef const BSExtSt *PCBSExtSt;

/*  The following header represents the bitstream header for the GOP and picture
	level of the bitstream.
*/
typedef struct _PicHdrSt {

	U8		PicStartCode;  /* fixed 5-bit pattern */
	U8		PictureType;	   /* 3-bit type: K, D, R, P2, P */
	U8		PictureNum;	   /* 8-bit picture number within the GOP */
	U8		RefPictureNum; /* 8-bit picture number of reference frame for this pic */

	GOPHdrSt	GOPHdr;			/* GOP Header */

	/* The following 7 Booleans are the 7 bits in the PicFlags */
	PIA_Boolean	DataSizeFlag;	/* Is DataSize present or not? */
	PIA_Boolean	SideBitStream;  /* Is there side bitstream at the end of the frame ? */
	PIA_Boolean	PicInheritTypeMV;    /* Is there at least one band inheriting BlockType (inter/intra)
								  * and MV from band 0? 
								  */
	PIA_Boolean	PicInheritQ;		/* Is there at least one band inheriting QuantDelta
								 *  from band 0? 
								 */
	PIA_Boolean	UseChecksum;		/* Is there Picture checksum present or not? */
	PIA_Boolean	UsePicExt;			/* Is there picture extensions present or not? */
	PIA_Boolean	NonDefMBHuffTab;	/* use the default macroblock Huffman table or not? */
	PIA_Boolean BandDataSizeFlag;   /* Band Data Sizes are on for all band headsers */

	U32		DataSize;		/* optional: Size of the compressed frame in bytes*/
	U16		PicChecksum;	/* optional: Error detection checksum of rec. picture */
	pBSExtSt	pPicExt;			/* Picture extensions */
	HuffMapTblSt MBHuffTbl;	/* Macro block Huffman table */
	PIA_Boolean YClamping;		/* Y plane needs clamping or not */
	PIA_Boolean VClamping;		/* V plane needs clamping or not */
	PIA_Boolean UClamping;		/* U plane needs clamping or not */

	/* The following are needed for decoder only */
	pQuantTableSt pQuantTables[NUM_QUANT_TABLES];

	PU8	YSubDiv;   /* derived from YDecompLevel */
	PU8 VUSubDiv;  /* derived from VUDecompLevel */
	U8  GlobalQuant; /* not needed in BS but needed for prefiltering */
							 

} PicHdrSt, *pPicHdrSt;
typedef const PicHdrSt *PCPicHdrSt;

/*  The following header represents the bitstream header for the Band
	level of the bitstream.
*/

typedef struct _BandHdrSt {
	U8		ColorPlane;		/* Color plane indicator */
	U8		BandId;			/* Id of the band from 0 */
	PIA_Boolean IsEmpty;		/* PIA_Boolean indicating a band with all empty tiles */
							/* This does *not* go in the bitstream, it is only */
							/* used to determine if a whole frame should be marked */
							/* empty (a repeat frame) because nothing has changed. */

	PIA_Boolean IsDropped;		/* PIA_Boolean indicating a dropped band */
	U32 	BandDataSize;	/* Size of this band in the bitsream - including the band hdr
							 * and the tile data 
							 */
		/* The following 8 Booleans are for the BandFlags */
	PIA_Boolean InheritTypeMV;	/* Inherit Intra/Inter, and MV from band 0 of the Y */
	PIA_Boolean QuantDeltaUsed; /* Use the QuantDelta or not: */
	PIA_Boolean	InheritQ;		/* Inherit Q values from band 0 of the Y */
	PIA_Boolean UseChecksum;	/* TRUE -> band check sum is present in header */
	PIA_Boolean UseRVHuffMapChangeList;  /* Is there RVHuffMapChangeList in the hdr or not?*/
	PIA_Boolean UseBandExt;		/* Is there Band Ext or not? */
	PIA_Boolean NonDefRVMapTab;  /* Use the default RunVal Mapping Table or not?*/
	PIA_Boolean NonDefBlkHuffTab; /* Use the default block Huffman table or not */

	U16		Checksum;		/* optional: Band Checksum */
	RVChangeListSt RVSwapList;	/* optional: Huffman map table 1 + var */
	pBSExtSt	pBandExt;		/* optional: Band extension */
	U8		RVMapTblNum;	/* optional: RV code mapping table number */
	HuffMapTblSt BlkHuffTbl;	/* Block Huffman table */
	U8		GlobalQuant;	/* Global Quant. level for this band */
} BandHdrSt, *pBandHdrSt;
typedef const BandHdrSt *pcBandHdrSt;

typedef struct _TransHdrSt {
	U8		ColorPlane;		/* Color plane indicator (Always 11 binary) */
	U32		TransDataSize;
	U8		NumDirtyRects;	/* Number of dirty rects in the dirty rect list (currently 0 or 1) */
	RectSt	DirtyRect;		/* Bounding rectangle of nontransparent region if 1 above */
	PIA_Boolean UseXOR_RLE;		/* Use XOR Based RLE to encode, otherwise use normal RLE */
	PIA_Boolean	ExpTransHuffTbl;	/* use the following Explicit Huff Table */
	HuffMapTblSt TransHuffMapTbl; 
} TransHdrSt, *pTransHdrSt;

/*  The following header represents the bitstream header for the Tile
	level of the bitstream.
*/
typedef struct _TileHdrSt {
	PIA_Boolean IsEmpty;		/* Is this tile empty? */
	PIA_Boolean IsTileDataSize;		/* Is there a TileDataSize field? */
	U32		TileDataSize;	/* Size of the tile in bytes - 1 + 24 bits */
	U32		NumBlks;
	PU16	pu16BlkChecksums;
} TileHdrSt, *pTileHdrSt;

/*  The following header represents the bitstream header for the Transparency 
	Tile level of the bitstream.
*/
typedef struct _TransTileHdrSt {
	PIA_Boolean IsEmpty;		/* Is this tile empty? */
	U32		InitialState;	/* Initial state or fill value if empty. */
	PIA_Boolean IsTileDataSize;		/* Is there a TileDataSize field? */
	U32		TileDataSize;	/* Size of the tile in bytes - 1 + 24 bits */
} TransTileHdrSt, *PTransTileHdrSt;

/*  The following header represents the bitstream header for the macro
	block level of the bitstream.
*/
typedef struct _MacHdrSt {
	PIA_Boolean		IsEmpty;	/* Is this macroblock empty? */
	U8			MbType;		/* Macro block type i.e.) fwd, bkwd, av or intra */
	U8			Cbp;		/* Coded block pattern */
	I32 		QuantDelta;		/* Quantization value for this macroblock */
	McVectorSt 	McVector;	/* Motion compensation vector */
} MacHdrSt, *pMacHdrSt;


#define VERSION_NUMBER 0
#define START_CODE 0x1f	  
#define	SUBDIVHORZ 0x00
#define	SUBDIVVERT 0x01
#define	SUBDIVQUAD 0x02
#define	SUBDIVLEAF 0x03
#define SUBDIVEND 0xFF
 
#define NUM_PIC_TYPES 5
#define PIC_TYPE_K 0
#define PIC_TYPE_P 1
#define PIC_TYPE_P2 2   /*  Second level reference frame */
#define PIC_TYPE_D 3
#define PIC_TYPE_R 4	/* Repeat last frame */

/* DRC Frame type groups */
#define NUM_PIC_DRC_TYPES 4

/* Indicates the PIC_TYPE to DRC_TYPE groupings */
#define DRC_TYPE_INTRA 0	/* Includes PIC_TYPE_K */
#define DRC_TYPE_PRED 1		/* Includes PIC_TYPE_P and PIC_TYPE_P2*/
#define DRC_TYPE_DELTA 2	/* Includes PIC_TYPE_D */
#define DRC_TYPE_NODRC 3	/* Includes PIC_TYPE_R */

/* color indices used with DataOffset, etc
*/
#define COLOR_Y 0
#define COLOR_V 1
#define COLOR_U 2
#define COLOR_T 3	/* Transparency color */

/* Values for Macroblock Type flags.  These are not arbitrary, they 
   need to range from 0 to MAX_ME_TYPES - 1
 */
#define MAX_ME_TYPES 3

#define TYPE_MV_I	0x00
#define TYPE_MV_FW	0x01
#define TYPE_MV_FW0 0x02

#endif
