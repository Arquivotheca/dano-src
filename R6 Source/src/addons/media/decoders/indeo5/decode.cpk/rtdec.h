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
*               Copyright (C) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
/*************************************************************************
 *                                                                       *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                *
 *                                                                       *
 *      This software is supplied under the terms of a license           *
 *      agreement or nondisclosure agreement with Intel Corporation      *
 *      and may not be copied or disclosed except in accordance          *
 *      with the terms of that agreement.                                *
 *                                                                       *
 *************************************************************************/

/*************************************************************************
 * RTDEC.H -- function declarations and definitions needed for rtdec.c
 *************************************************************************/

#ifdef __RTDEC_H__
#pragma message("rtdec.h: multiple inclusion")
#else /* __RTDEC_H__ */
#define __RTDEC_H__

#ifndef __PIA_MAIN_H__
#pragma message("rtdec.h requires pia_main.h")
#endif

#define PASS_PINST
#define MAX_Y_BANDS 4

/* Band Flags */
#define BF_DROPPED 0x1			/* set if this band is dropped */
#define BF_REF_DROPPED 0x2		/* set if this band is dropped on a */
								/* reference frame, to indicate that */
								/* this band must be dropped until the */
								/* next Key frame. */

/* Information for handling scalability and determining when to 
 * drop more (or fewer) bands 
 */
typedef struct _ScaleCntxSt {
/*	Frame Level Scalability Support */
	I32			iStartClock;			/* % time rem. at start of dec */
	I32			iAverageDecodePercent;	/* running avg of frame dec time */
	I32			iAverageDisplayPercent;	/* running avg of frame disp time */
	I32			iAverageVoidPercent;	/* running avg of frame dec time */
	I32			iPercentFramesDecoded;	/* current percent frames decoded */
	I32			iPercentFramesDisplayed;/* current percent frames displayed */
	I32			iFrameScaleLevel;		/* Specifies amount to scale back */

/*	Band Level Scalability Support */
	I32			iYBandPercent[MAX_Y_BANDS]; /* % of time for each band */
	I32			iVUBandPercent[MAX_Y_BANDS];
/*	I32			iComposeNColorConvertPercent; */ /* % time for color convert */
/*	I32			iComposeStartPercent;		  */
	I32			iScaleLevel;			/* Specifies amount to scale back */
	U32			uNYBands;				/* Number of Y bands decoded */
	Boo			bDecode_P;				/* are we currently decoding P's? */
	Boo			bDecode_P2;				/* are we currently decoding P2's? */
	Boo			bDecode_D;				/* are we currently decoding D's? */
} ScaleCntxSt, * pScaleCntxSt;


typedef struct _RtTileSt {
	PointSt		tNTileMBs;	/* Number of Macroblocks in this tile */
	PointSt		tNTileBlocks;	/* Number of Blocks in this tile */
	U32			uTileOffset;
	pBlkInfoSt	pBlockInfo; /* Pointer to blockinfo structure for this */
							/* tile.  YBand0 is treated differently than */
							/* other bands (for inheritance) - each yband0
							/* tile has it't own blockinfost, the rest of */
							/* the tiles share a common blockinfost.  */
	PU8			uYVUOutput;
/*	There is no lower level information for the MacroBlocks or Blocks,
 *	it is stored in the 2 blockinfo structures.
 */
} RtTileSt, * pRtTileSt;

typedef struct _RtBandSt {
	PointSt		tBandSize;		/* size of the band after splitting */
	U32			uBandOffset;
	U32			uPitch;			/* pitch of band data */
	PointSt		tTileSize;
	U32			uDataSize;
	U32			uMVRes;			/* MV Resolution */
	U32			uCheckSum;		/* Band Checksum */
	U32			uMBSize;		/* Size of Macroblocks in this band */
	U32			uBlockSize;		/* Size of Macroblocks in this band */
	Boo			bQDelta;		/* non-zero if deltaq is on */
	Boo			bInheritQ;		/* Non-zero if inherit Quant is on */
	Boo			bInheritTypeMV;	/* Non-zero if inherit Type & MV is on */
	U32			uBandFlags;		/* flags for this band - see BF_xxx */
	U8			u8BandFlags;	/* flags for this band - see BF_xxx */
	U8			_unused[3];		/* for alignment */
	U32			uGlobalQuant;	/* Global Quantization for this band */
	SetCntxSt	SetCntx;		/* Context to set up block data decode */
	BlkCntxSt	BlkCntx[4];
	U32			uBlockHuffTab;
	U32			uRVMapTable;
	U32			uRVChangeList;
	pRtTileSt	pTile;			/* Pointer to tile structures for this band */
#ifdef C_PORT_KIT
	U8			rvswap[128];
#endif /* C_PORT_KIT */
} RtBandSt, * pRtBandSt;

typedef struct _RtPlaneSt {		/* 3 PlaneSt, but only 2 BandSt */
	RectSt		rVisible;		/* Visible portion of this plane */
	RectSt		rDecoded;		/* Decoded portion of this plane */
	U32			uPlaneOffset;	/* Offset for the bands of this plane */
	U32			uNBands;		/* Guess. */
	pRtBandSt	pBand;			/* Info for each band */
	PU8			pOutput;		/* YVU Output location */
	U32			uOutputPitch;	/* Pitch in YVU Output */
} RtPlaneSt, * pRtPlaneSt;

typedef struct _FrameSt {
	RectSt		rColorConvert;	/* Rectangle to color convert */
	RectSt		rDecodeRect;	/* clipping */
	RectSt		rViewRect;	   	/* clipping */

	RectSt		rBoundRect;		/* Transparency Bounding Rect */
	Boo			bValidBoundingRect;

	Boo			bUseAccessKey;
	U32			uAccessKey;
	Boo			bKeyFailure;
	U8			u8ScrambledData[64];	/* must 'rescramble' before return */
	U32			uScrambleLength;
	PU8			pu8ScrambleStart;

	Boo			bValidTransBitMask;
	TRANSPARENCY_KIND	htkTransparencyKind;

/* // TBD: move this to rtcout.h */
	PU8			puLumaTable;
	PU8			puChromaTable;
	PU8			puContrastTable;
	PU8			puBrightTable;

	Boo			bUpdateLuminance;	
	Boo			bUpdateChrominance;
/* // */

	U32			uFrameType;		/* K, P, P2, D, R */
	U8			u8FrameNumber;	/* Frame number from bitstream */

	U8			u8GOPFlags;		/* GOP Flags */
	U8			u8PicFlags;		/* Picture Header Flags */
	U8			u8BSVersion;	/* Bitstream Version for GOP  */

	PointSt		tFrameSize;		/* BS specified frame size */
	PointSt		tVUFrameSize;	/* subsampled BS specified frame size */
	PointSt		tTileSize;
	PointSt		tNTiles;

	U32			uDataSize;		/* # bytes - from picture header */
	U32			uFrameCheckSum;
	U32			uDecoderHints;

	PU8			vu8DecodeTiles;
	PU8			pPlane3Storage;
	Boo			uValidCurrent;
	Boo			uValidForward;
	Boo			uValidBackward;
	Boo			uValidDisplay;
	PU8			pFrameCurrent;
	PU8			pFrameForward;
	PU8			pFrameBackward;
	PU8			pFrameDisplay;
	Boo			bInitForward;	/* buffer initialized to decode P frames? */
	Boo			bInitBackward;	/* buffer initialized to decode B frames? */
	U32			uBSize;			/* band storage buffer size in bytes */
	Boo			bInheritQ;
	Boo			bInheritTypeMV;
	U32			uFlags;			/* flags */

	PU8			pLDMaskStorage;	/* pointer to private ld mask */
	PU8			pLDMask;		/* pointer to current ld mask */
	PU8			pTranspMaskCurrent;	/* for "other" frame type masks */
	PU8			pTranspMask;	/* pointer to private transparency mask */
	PIA_Boolean		bDidXparDec;	/* was there a sucessful xparency decode? */

	PIA_Boolean		bDirtyUpdate;	/* was there a bounding box present? */
	U32			uMaskStride;	/* initialized by plane decoder */
	PU8			pMaskStorage;	/* unaligned storage to optimize mask handling*/
	U32			uMaskSize;		/* size of LD & Transp Masks */
	BGR_ENTRY	uTransColor;	/* default 0 */

	RtPlaneSt	Plane[3];
	HuffTabSt	MBHuffTabList[13];	/* 0-7, Default, EscK, EscP, EscP2, EscD */
	pHuffTabSt	MBHuffTab;
	U32			uBlockHuffTab;
	U32			uRVMapTable;
	PU8			ComposeScratch;
	pBlkInfoSt	pBlockInfoBand0;	/* storage for all band 0 tiles */
	pBlkInfoSt	pBlockInfo;			/* scratch for all non band 0 tiles */
	PU8			YVUAlignedOutput;	/* Cache line aligned copy of what the */
	PU8			YVUOutputStorage;	/* allocator returned, may be misaligned */
	XparCntx	XCntx;
	XparSetCntx	XSCntx;
	ScaleCntxSt	ScaleCntx;
	PBDESCRIPTOR_TYPE	MBHuffTab8Descriptor[17];
	PBDESCRIPTOR_TYPE	MBHuffTab9Descriptor[17];
	PBDESCRIPTOR_TYPE	MBHuffTab10Descriptor[17];
	PBDESCRIPTOR_TYPE	MBHuffTab11Descriptor[17];
	PBDESCRIPTOR_TYPE	MBHuffTab12Descriptor[17];
/*	Descriptor to transmit picture level default block huffman table to
	the current band: */
	PBDESCRIPTOR_TYPE	BHuffTab8Descriptor[17];
} RTDecInst, *pRTDecInst;


pRTDecInst RTDecompressBegin();

PIA_RETURN_STATUS RTDecompress(
	pRTDecInst		pCntx,					/* decoder instance */
	PU8				pInput,					/* input bitstream */
	U32				InputLen,				/* input bitstream length */
	DimensionSt		dImageDim,				/* frame size */

	U32				uFrameStartTime,		/* timer at start of frame */
	U32				uFrameTargetTime,		/* allocated time per frame */
	U32				uFrameScale,			/* scale of time units to get ms */

	PU8				pTranspMaskForground,	/* if outside world has xparancy */
											/* & wants to share, here it is */
											/* (null means no mask) */
	PU32			puTranspPitchForground,	/* pitch of forground mask */
											/* (0 means use default) */
	PU8				pTransMaskExternal,		/* non-zero pointer if xpar wanted*/
	PU32			puTransPitchExternal,	/* pitch of external mask*/
											/* (0 means use default) */
	PPIA_Boolean		pbTransMaskExtSet,		/* did we do what was requested? */

	PRectSt			rColorConvert,			/* How much to color convert */
	U32				uRTFlags,				/* run time flags */
	COLOR_FORMAT	cfOutputFormat,			/* current output color format */
	I32				dmDecMode				/* current decode mode */
);

PIA_RETURN_STATUS RTDecompressFrameEnd(pRTDecInst pCntx,
	I32 dmDecMode,
	U32 uFrameTargetTime);

PIA_RETURN_STATUS RTDecompressEnd(pRTDecInst pCntx);

/*	RTFlags values */
#define RT_DONT_DROP_FRAMES		0x01
#define RT_DONT_DROP_QUALITY	0x02
#define RT_REDRAW_LAST_FRAME	0x04
#define RT_CLUMPED_FRAME		0x08	/* This is a (bidir) clumped frame */

extern const PBDESCRIPTOR_TYPE BlockStaticHuff[9][17];

#endif /* __RTDEC_H__ */
