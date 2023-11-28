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
#include "datatype.h"
#include "const.h"


/*
 * PicHdr and GOPHdr constants
 */

/* Start Code */
U8 const LenPicStartCode = 5;
U8 const LenPictureType = 3;
U8 const LenPictureNum = 8;

/* GOP Header */
	 /* Define the GOPFLags */
U8 const LenGOPFlags = 8;

U8 const GOPFLags_GOPHdrSizeFlag	=	(1UL<<0);
U8 const GOPFLags_IsYVU12			=	(1UL<<1);
U8 const GOPFLags_IsExpBS			=	(1UL<<2);
U8 const GOPFLags_Transparency		=	(1UL<<3);
U8 const GOPFLags_SkipBitBand		=	(1UL<<4);
U8 const GOPFLags_UseKeyFrameLock	=	(1UL<<5);
U8 const GOPFLags_Tiling			=	(1UL<<6);
U8 const GOPFLags_Padding			=	(1UL<<7);

U8 const LenGOPHdrSize	= 16;
U32 const MaxGOPHdrSize = (1<<16)-1;
U8 const LenFrameLock = 32;

const U8 LenTileSize = 2;

const U8 LenYDecompLevel = 2;
const U8 LenVUDecompLevel = 1;

U8 const NumComPicSizes = 12;
U8 const PicSizeEscCode = 15;
U8 const LenExpPicDim = 13;
U8 const LenPicSize = 4;
const struct _PicSize ComPicSizes[] = {
										480, 640,
										240, 320,
										120, 160,
										480, 704,
										240, 352,
										288, 352,
										144, 176,
										180, 240,
										240, 640,
										240, 704,
										60,   80,
										72,   88
									  };
	/* Band Descriptors in GOPHdr */
const U8	MVResIntegral		= 0x0;
const U8	MVResHalf			= 0x1;
const U8	LenMVRes			= 1;

const U8	LenBlockSize		= 2;

const U8	DefaultTransform = 0;
const U8    ExpTransform=1;
const U8	LenTransform = 1;
const U8    LenExpTransform = 2;

const U8    LenScanOrder =1;
const U8    DefaultScanOrder = 0;

const U8	DefaultQuantMatrix = 0;
const U8    LenQuantMatrix =1;

	/* Transparency Band Descriptor in GOPHdr */
const U8	TransBitDepth = 0;  /* 1 bit per pixel, no multi-bit alpha channel yet */
const U8	LenTransBitDepth		 = 3;
const U8	HasTransColor		 = 1;
const U8	LenHasTransColor		 = 1;		
const U8	LenTransColor		 = 8;

	/* Version Info */
const U8	LenPlatformId = 8;
const U8	LenBuildNum = 8;
const U8	LenEncoderId = 3;
const U8	LenBSVersion = 4;
const U8	LenChainBit = 1;

const U8	LenToolIndex = 7;
const U8	LenToolBuildNum = 8;

/* Pic Header */
	 /* Define the PicFLags */
U8 const LenPicFlags = 8;

U8 const PicFlags_DataSizeFlag	=	(1UL<<0);
U8 const PicFlags_SideBitStream	=	(1UL<<1);
U8 const PicFlags_PictureInheritTypeMV  =	(1UL<<2);
U8 const PicFlags_PictureInheritQ		=	(1UL<<3);
U8 const PicFlags_UseChecksum			=	(1UL<<4);
U8 const PicFlags_UsePicExt				=	(1UL<<5);
U8 const PicFlags_NonDefMBHuffTab		=	(1UL<<6);
U8 const PicFlags_BandDataSizeFlag		=	(1UL<<7);

U8 const LenPicDataSize = 24;
U32 const MaxPicDataSize = (1<<24)-1;
U8 const LenPicChecksum = 16;
U8 const LenExtNumBytes = 8;
U8 const LenExtData = 8;
U8 const LenHuffTbl = 3;

U8 const LenClamping = 1;
U8 const Clamping = 1;
U8 const NotClamping = 0;

const U32 DWordSize = 4;

/*
 * Constant for clamping limits on the input data
 */
const I16 MaxClampIn = 240;
const I16 MinClampIn = 16;

/*
 * Constant for clamping limits on the output data
 */
const I16 MaxClamp = 255;
const I16 MinClamp = 0;

/*
 * Constants and definitions for the Trans Tile structure
 */
const U8 TransTileEmpty = 1;
const U8 TransTileNotEmpty = 0;
const U8 LenTransTileEmpty = 1;
const U8 LenTransTileInitState = 1;

const U8 DefTransHuffTbl = 0;
const U8 NotDefTransHuffTbl = 1;
const U8 LenDefTransHuffTbl = 1;
const U8 LenUseTransTileDataSize	= 1;
const U8 UseTransTileDataSize = 1;
const U8 NoTransTileDataSize = 0;
/*
 * Constants and definitions for the Tile structure
 */

const U8 LenTileEmpty = 1;
const U8 TileEmpty = 1;
const U8 TileNotEmpty = 0;
const U8 LenTileDataSize = 1;
const U8 UseTileDataSize = 1;
const U8 NotUseTileDataSize = 0;

const U8 LenNumBlks = 16;
const U8 LenBlkChecksum = 16;

const U8 LenMBEmpty = 1;
const U8 MBEmpty = 1;
const U8 MBNotEmpty = 0;
const U8 LenMbType=1;

const U8 Small_TD_Width  = 8;
const U8 Max_Small_TD_Size = 254; /* 2^8 - 2 (because of escape) */
const U8 Large_TD_Width  = 24;
const U32 Max_Large_TD_Size = (1<<24)-1; /* 2^24 - 1*/


/*
 * Symbolic names for the Transparency Band Header Size fields
 */
const U8	LenTransDataSize		 = 24;
const U32   MaxTransDataSize = (1<<24)-1;  /* 2^24 -1 */
const U8	LenDirtyRectDim			 = 16;	
const U8	LenUseXOR_RLE			 = 1;	
const U8	LenNumDirtyRects		 = 3;		

/*
 * Symbolic names for the Band Header fields
 */
const U8 LenBandDataSize = 24;
const U32 MaxBandDataSize = (1<<24)-1;  /* 2^24 -1 */
 /* BandFlags */
U8 const BandFlags_BandDroppedFlag	=	(1UL<<0);
U8 const BandFlags_InheritTypeMV	=	(1UL<<1);
U8 const BandFlags_QuantDeltaUsed	=	(1UL<<2);
U8 const BandFlags_InheritQ			=	(1UL<<3);
U8 const BandFlags_UseRVChangeList	=	(1UL<<4);
U8 const BandFlags_UseBandExt   	=	(1UL<<5);
U8 const BandFlags_RVMappingTable	=	(1UL<<6);
U8 const BandFlags_BlkHuffTable		=	(1UL<<7);

U8 const	LenBandFlags = 8;
const U8	LenBandChecksumFlag = 1;
const U8	UseBandChecksum = 1;
const U8	NotUseBandChecksum = 0;
const U8	LenBandChecksum		= 16;
const U8	LenBandRVSwapNum		= 8;
const U8	LenBandRVSwapEntry		= 8;
const U8    LenRVMapTbl = 3;
const U8	LenBandGlobalQuant = 5;

/*
 * More constants
 */
const U8	HuffTblEscape			= 0x07;
const U8	MapTblEscape			= 0x07;
const U8	DefaultHuffTbl			= 0xff;
const U8	DefaultRvMapTbl			= 0xff;
const U8	NoLastTbl				= 0xfe;
/*const U8	DefaultPicHuffTblNum	= 0;*/
const I32	numberOfPlanes			= 3;
const U8	MacroBlockTypeBitLen	= 6;
const U8	PB_Entry_Size			= 4;

/* Values for Huffman/fixed Length combination in bitstream */
const U8 FixedLenIndic  = 1;	/* Number of bits to distinguish between FL and Huff */
const U8 IndicatorLen	= 1;	/* Indicator as to whether its FL or Huff */
const U8 FixLenLen		= 3;	/* Number of bits to describe the len of a FL code */
const U8 HuffCodeIndic  = 0;	/* Indicator that this will be Huffman coded */
const U8 HuffLen		= 8;	/* Length of values to be Huffman coded */


/* For skip Bit Band */
const U8 LenSkipDataEmpty=1;
const U8 SkipDataEmpty = 1;
const U8 SkipDataNotEmpty = 0;	 

