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

#ifdef __cplusplus
extern "C" {				/* Assume C declarations for C++ */
#endif	/* __cplusplus */

/* Sequence Type */
#define Default			0	/* KPPPP ... K		30->KFI	*/
#define SequenceA		1	/* KDPDP ... K		30->15->KFI		*/
#define	SequenceB		2	/* KDP2DP ... K		30->15->7.5->KFI*/
#define SequenceC		3	/* KDP2DP2DP ... K  30->15->5->KFI  */

/* Tile and Block Defines */
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


/* Various RT encoder-specific constants */
extern const U32 NoError;				/* useful return value */
extern const U32 NUM_FRAME_BUFS;		/* for allocating frame buffers */
extern const U32 NUM_MOTION_BUFS;		/* for allocating Motion buffers */
extern const U32 UVSUBSAMPLE;			/* YVU9 */
extern const U32 BRCInitGlobalQuant;	/* initial global quant when bit rate control used */

/*
 * PicHdrSt and GOPHdr Constants
 * ToDo:  Add Picture Start Code 
 */
extern const U8 PicStartCode;
extern const U8 LenPicStartCode;
extern const U8 LenPictureType;
extern const U8 LenPictureNum;

/* GOP Header */
extern const U8 LenGOPFlags ;
/* Define the GOPFLags */
extern const U8 GOPFLags_GOPHdrSizeFlag;	
extern const U8 GOPFLags_IsYVU12;	
extern const U8 GOPFLags_IsExpBS;	
extern const U8 GOPFLags_Transparency;			
extern const U8 GOPFLags_SkipBitBand;		
extern const U8 GOPFLags_UseKeyFrameLock;		
extern const U8 GOPFLags_Tiling;				
extern const U8 GOPFLags_Padding;			

extern const U8 LenGOPHdrSize;
extern const U32 MaxGOPHdrSize;
extern const U8 LenLockword1;

extern const U8 LenTileSize;

extern const U8 MaxYDecompLevel;
extern const U8 LenYDecompLevel;
extern const U8 LenVUDecompLevel;

extern const U8 NumComPicSizes;
extern const U8 PicSizeEscCode ;
extern const U8 LenExpPicDim;
extern const U8 LenPicSize;
extern const struct _PicSize { I32 Height, Width; }	ComPicSizes[];

/* Band Descriptors in GOPHdr */
extern const U8	MVResIntegral;
extern const U8	MVResHalf;
extern const U8	LenMVRes;

extern const U8	LenBlockSize;

extern const U8	DefaultTransform;
extern const U8	ExpTransform;
extern const U8	LenTransform;
extern const U8	LenExpTransform;

extern const U8 LenScanOrder;
extern const U8 DefaultScanOrder;
extern const U8	ExpScanOrder;
extern const U8 LenExpScanOrderEntry44;
extern const U8 LenExpScanOrderEntry88;

extern const U8	DefaultQuantMatrix;
extern const U8	ExpQuantMatrix;
extern const U8	LenQuantMatrix;
extern const U8	LenExpQuantEntry;

/*
 * Constants used in Band Descriptors.
 */
extern const U32 BlockSzLarge;
extern const U32 BlockSzMedium;
extern const U32 BlockSzSmall;
extern const U8	BlockSzBitValLen;

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


/*
 * Symbolic name for the bit stream version number
 */
extern const U8 uDebVersion;

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
extern const U8 BandFlags_BandDroppedFlag;
extern const U8 BandFlags_InheritTypeMV;
extern const U8 BandFlags_QuantDeltaUsed;
extern const U8 BandFlags_InheritQ;
extern const U8 BandFlags_UseRVChangeList;
extern const U8 BandFlags_UseBandExt;
extern const U8 BandFlags_RVMappingTable;
extern const U8 BandFlags_BlkHuffTable;

extern const U8 LenBandFlags;
extern const U8	LenBandChecksumFlag;
extern const U8	UseBandChecksum;
extern const U8	NotUseBandChecksum;
extern const U8	LenBandChecksum;
extern const U8	LenBandRVSwapNum;
extern const U8	LenBandRVSwapEntry;
extern const U8 LenRVMapTbl;
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

extern const U8 LenMBEmpty;
extern const U8 MBEmpty;
extern const U8 MBNotEmpty;
extern const U8 LenMbType;

extern const U8 Small_TD_Width;
extern const U8 Max_Small_TD_Size;
extern const U8 Large_TD_Width;
extern const U32 Max_Large_TD_Size;
		
/*
 * Symbolic names for other stuff
 */
extern const U8	HuffTblEscape;
extern const U8	MapTblEscape;
extern const U8	DefaultHuffTbl;
extern const U8	DefaultPicHuffTblNum;
extern const U8	DefaultRvMapTbl;
extern const U8	NoLastTbl;
extern const I32	numberOfPlanes;
extern const U8	MacroBlockTypeBitLen;
extern const U8	PB_Entry_Size;

extern const U8	TileSizePicSize;
extern const I16 TileSizeTable[];

extern const U8	EOB;
extern const U8	TileSizeEscape;


/* Values for Huffman/fixed Length combination in bitstream */
extern const U8 FixedLenIndic;	/* Number of bits to distinguish between FL and Huff */
extern const U8 IndicatorLen;	/* Indicator as to whether its FL or Huff */
extern const U8 FixLenLen;		/* Number of bits to describe the len of a FL code */
extern const U8 HuffCodeIndic;	/* Indicator that this will be Huffman coded */
extern const U8 HuffLen;		/* Length of values to be Huffman coded */

/*
 * Constants and definitions for the Tile structure.
 */
extern const U8 K_Tiles;
extern const U8 Tile_Empty_Width;
extern const U8 K_MB;
extern const U8 MB_Empty_Width;

/* ColorPlane field */
extern const U8 BSColorPlaneSize;
extern const U8 COLOR_Y;
extern const U8 COLOR_V;
extern const U8 COLOR_U;

	
/* picture type */
extern const U32 NumPicTypes;
extern const U32 PicTypeK;
extern const U32 PicTypeP;
extern const U32 PicTypeP2;
extern const U32 PicTypeD; 
extern const U32 PicTypeR;

extern const U32 FrameTypeAuto; 	/* Internally selected Frame type*/
 
#ifdef __cplusplus
}		/* End of extern "C" { */	
#endif	/* __cplusplus */


#endif /* __CONST_H__ */



