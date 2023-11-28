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
#ifdef __CONST_H__
#pragma message("***** CONST.H Included Multiple Times")
#endif
#endif

#ifndef __CONST_H__
#define __CONST_H__

/* TODO:These constants may be moved to another separate version header file ?*/
#define OFFLINE_ENCODER 0  /* Offline Encoder Id */
/*#define QUICK_COMPRESSOR 1 /* ? */
/*#define REAL_TIME_ENCODER 2 /* ? */

#define PREBETA_BS 0
#define BETA_BS 1
#define INDEO5_1_BS 2
#define BS_VERSION INDEO5_1_BS	   /* BS version # */

#define TileSize64		0 
#define TileSize128		1
#define TileSize256		2
#define TileSizeResvd	3

#define 	BlockSize16_8	0
#define 	BlockSize8_8	1
#define 	BlockSize8_4	2
#define 	BlockSize4_4	3

#define 		Slant2D		0
#define 		SlantRow	1
#define 		SlantColumn 2
#define 		NoneXfrm	3

/*
 * PicHdrSt and GOPHdr constants
 */
extern U8 const LenPicStartCode;
extern U8 const LenPictureType;
extern U8 const LenPictureNum;

extern U8 const LenGOPFlags ;
	 /* Define the GOPFLags */
extern U8 const GOPFLags_GOPHdrSizeFlag;	
extern U8 const GOPFLags_IsYVU12;	
extern U8 const GOPFLags_IsExpBS;	
extern U8 const GOPFLags_Transparency;			
extern U8 const GOPFLags_SkipBitBand;		
extern U8 const GOPFLags_UseKeyFrameLock;		
extern U8 const GOPFLags_Tiling;				
extern U8 const GOPFLags_Padding;			

extern U8 const LenGOPHdrSize;
extern U32 const MaxGOPHdrSize;
extern U8 const LenFrameLock;

extern const U8 LenTileSize;

extern const U8 MaxYDecompLevel;
extern const U8 LenYDecompLevel;
extern const U8 LenVUDecompLevel;

extern U8 const NumComPicSizes;
extern U8 const PicSizeEscCode ;
extern U8 const LenExpPicDim;
extern U8 const LenPicSize;
extern const struct _PicSize { I32 Height, Width; }	ComPicSizes[];

/* Band Descriptors in GOPHdr */
extern const U8	MVResIntegral;
extern const U8	MVResHalf;
extern const U8	LenMVRes;

extern const U8	LenBlockSize;

extern const U8		DefaultTransform;
extern const U8		ExpTransform;
extern const U8		LenTransform;
extern const U8		LenExpTransform;

extern const U8    LenScanOrder;
extern const U8    DefaultScanOrder;

extern const U8		DefaultQuantMatrix;
extern const U8		LenQuantMatrix;

/* Transparency Band Descriptor in GOPHdr */
extern const U8	TransBitDepth;
extern const U8	LenTransBitDepth;
extern const U8	HasTransColor;
extern const U8	LenHasTransColor;		
extern const U8	LenTransColor;

	/* Version Info */
extern const U8	LenPlatformId;
extern const U8	LenBuildNum;
extern const U8	LenEncoderId;
extern const U8	LenBSVersion;
extern const U8	LenChainBit;

extern const U8	LenToolIndex;
extern const U8	LenToolBuildNum;

/* Pic Header */
	 /* Define the PicFLags */
extern U8 const LenPicFlags;

extern U8 const PicFlags_DataSizeFlag;
extern U8 const PicFlags_SideBitStream;
extern U8 const PicFlags_PictureInheritTypeMV;
extern U8 const PicFlags_PictureInheritQ;
extern U8 const PicFlags_UseChecksum;
extern U8 const PicFlags_UsePicExt;
extern U8 const PicFlags_NonDefMBHuffTab;
extern U8 const PicFlags_BandDataSizeFlag;


extern U8 const LenPicDataSize;
extern U32 const MaxPicDataSize;
extern U8 const LenPicChecksum;
extern U8 const LenExtNumBytes;
extern U8 const LenExtData;
extern U8 const LenHuffTbl;

extern U8 const LenClamping;
extern U8 const Clamping;
extern U8 const NotClamping;

extern const U32 DWordSize;

#endif /* __CONST_H__ */

/*
 * Constant for clamping limits on the input data
 */
extern const I16 MaxClampIn;
extern const I16 MinClampIn;

/*
 * Constant for clamping limits on the output data
 */
extern	const I16 MaxClamp;
extern	const I16 MinClamp;
																              
/* band header fields */

extern const U8 LenBandDropped;
extern const U8 LenBandDataSize;
extern const U32 MaxBandDataSize;

/* BandFlags */
extern U8 const BandFlags_BandDroppedFlag;
extern U8 const BandFlags_InheritTypeMV;
extern U8 const BandFlags_QuantDeltaUsed;
extern U8 const BandFlags_InheritQ;
extern U8 const BandFlags_UseRVChangeList;
extern U8 const BandFlags_UseBandExt;
extern U8 const BandFlags_RVMappingTable;
extern U8 const BandFlags_BlkHuffTable;

extern U8 const LenBandFlags;
extern const U8	LenBandChecksumFlag;
extern const U8	UseBandChecksum;
extern const U8	NotUseBandChecksum;
extern const U8	LenBandChecksum;
extern const U8	LenBandRVSwapNum;
extern const U8	LenBandRVSwapEntry;
extern const U8    LenRVMapTbl;
extern const U8	LenBandGlobalQuant;

/*
 * Symbolic names for the Tile and Block fields
 */

extern const U8 LenTileEmpty;
extern const U8 TileEmpty;
extern const U8 TileNotEmpty;
extern const U8 LenTileDataSize;
extern const U8 UseTileDataSize;
extern const U8 NotUseTileDataSize;

extern const U8 LenNumBlks;
extern const U8 LenBlkChecksum;

extern const U8 LenMBEmpty;
extern const U8 MBEmpty;
extern const U8 MBNotEmpty;
extern const U8 LenMbType;

extern	const U8 TransTileEmpty;
extern	const U8 TransTileNotEmpty;
extern	const U8 LenTransTileEmpty;
extern	const U8 LenTransTileInitState;

extern	const U8 DefTransHuffTbl;
extern	const U8 NotDefTransHuffTbl;
extern	const U8 LenDefTransHuffTbl;

extern	const U8	LenUseTransTileDataSize;
extern	const U8	UseTransTileDataSize;
extern const U8    NoTransTileDataSize;

extern	const U8	Small_TD_Width;
extern	const U8	Max_Small_TD_Size;
extern	const U8	Large_TD_Width;
extern	const U32	Max_Large_TD_Size;

/*
 * Symbolic names for transparency band header fields 
 */
extern const U8		LenTransDataSize;
extern const U32	MaxTransDataSize;  
extern const U8		LenDirtyRectDim;		
extern const U8	    LenUseXOR_RLE;
extern const U8		LenNumDirtyRects;		
		
/*
 * Symbolic names for other stuff
 */
extern	const U8	HuffTblEscape;
extern	const U8	MapTblEscape;
extern	const U8	DefaultHuffTbl;
extern	const U8	DefaultPicHuffTblNum;
extern 	const U8	DefaultRvMapTbl;
extern	const U8	NoLastTbl;
extern	const I32	numberOfPlanes;
extern	const U8	MacroBlockTypeBitLen;
extern 	const U8	PB_Entry_Size;

/* Values for Huffman/fixed Length combination in bitstream */
extern	const U8 FixedLenIndic;	/* Number of bits to distinguish between FL and Huff */
extern	const U8 IndicatorLen;	/* Indicator as to whether its FL or Huff */
extern	const U8 FixLenLen;		/* Number of bits to describe the len of a FL code */
extern	const U8 HuffCodeIndic;	/* Indicator that this will be Huffman coded */
extern	const U8 HuffLen;		/* Length of values to be Huffman coded */

/* Skip Bit Band */
extern	const U8 LenSkipDataEmpty;
extern	const U8 SkipDataEmpty;
extern	const U8 SkipDataNotEmpty;	 

