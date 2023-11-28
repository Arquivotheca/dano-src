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

#if !defined __RTENC_H__
#define __RTENC_H__

#include "brc.h"

#if COM_INTFC == 1
	#include <microsft.h>
#endif

/* global RTE var indicating CPU type on which executing */
extern PIA_Boolean bCPUIsKlamath;

/** MACROS *******************************************************/

/* max supported resolutions */
#define MAX_X_RES	640		
#define MAX_Y_RES	480
/* Y plane blocks are 8x8 */
/* NOTE: DO NOT CHANGE WITHOUT CHANGING CORRESPONDING DEF IN rtenc.inc */
#define YBLKSIZE	8
/* UV plane blocks are 4x4 */
/* NOTE: DO NOT CHANGE WITHOUT CHANGING CORRESPONDING DEF IN rtenc.inc */
#define UVBLKSIZE	4		

#define BandPitch	320		/* band data has fixed pitch */

/* Block types */
/* NOTE: DO NOT CHANGE WITHOUT CHANGING CORRESPONDING DEF IN rtenc.inc */
#define BiPredIntra	0
#define BiPredPrev	1		/* forward prediction, from previous frame */
#define BiPredNext	2		/* backward prediction, from next frame */
#define BiPredAve	3


/** STRUCTURES ***************************************************/

/*  Tool Information */
typedef struct _ToolStampSt {
	U8		ToolIndex;
	U8		ToolBuildNum;
	PIA_Boolean ChainBit;
	struct _ToolStampSt *  pNextTool;
} ToolStampSt, * pToolStampSt;
typedef const ToolStampSt * PCToolStampSt;

/*  Version Information */
typedef struct _VersionSt {
	U8		PlatformId;
	U8		BuildNum;
	U8		EncoderId;
	U8		BSVersion;
	PIA_Boolean ChainBit;
	pToolStampSt  pToolStamp;
} VersionSt, * pVersionSt;
typedef const VersionSt * PCVersionSt;

/*	Block Level Information */
/* NOTE: DO NOT CHANGE WITHOUT CHANGING CORRESPONDING DEF IN rtenc.inc */
typedef struct _BlkInfoSt {
	U32			BlkAddrOffset;	/* address offset of block in motion buffer */
	U32			MEFirstState;	/* start state for ME search */
	U8			NumPoints;		/* how many actual pels in the block */
	U8			Prediction;		/* block prediction type */
	I8			OffR, OffC;		/* motion vectors if not intra */
	I32			QDelta;			/* quant delta */
	U32			IntraSAD;		/* sum of absolute diffs from mean */
	U32			WtIntraSAD;		/* weighted (by NumPoints) block IntraSAD */
	I32			MVOffset;		/* motion estimation offset				*/
	U8			EmptyBlock;		/* Empty Block Flags					*/

#ifdef DEBUG 
#ifdef INTERSAD
	U32			InterSAD;	/* sum of absolute diffs for best match */
#endif
#endif

} BlkInfoSt, * pBlkInfoSt;

/* Band Level Information */
/* NOTE: DO NOT CHANGE WITHOUT CHANGING CORRESPONDING DEF IN rtenc.inc */
typedef struct _BandInfo {
	U32			NRows, NCols;
	U32			BlockSize;		/* 4 (4x4) or 8 (8x8) */
	U32			Xform;			/* one of XFORM_* */
	U32			GlobalQuant;
	PIA_Boolean		UseChecksum;	/* TRUE if checksum written to band header */
	U16			Checksum;		/* on the reconstructed band */
} BandInfoSt, * pBandInfoSt;

/* Frame Level Information */
typedef struct _FrmInfoSt {
	U32			FrameType;		/* one of PIC_TYPE_* */
	pBandInfoSt	BandInfo;
	PI16		YBandData;		/* start of Y bands buffer */
	PU8			VBandData;		/* start of V band buffer */
	PU8			UBandData;		/* start of U band buffer */			
	PI16		BandData[6];	
	PU8			MotionBand;
	U32			MBOffset;		/* from allocated MotionBand pointer to 4K boundary */
	pBlkInfoSt	BlockInfo;
} FrmInfoSt, * pFrmInfoSt;

/* bit rate control structure */
typedef struct _BRCInfoSt {
	U32 FrameSize[BRC_AVG_PERIOD];	/* frame sizes in bytes */
	U32 FrameSizeIndex;				/* next entry to use */
	U32 TotalBytes;					/* sum of FrameSize entries */
	U32 InputTargetBytes;			/* stored input requested target */
	U32 ReqBytesPerFrame;			/* initialized at startup */
	U32 AdjustMarginBytes;			/* in bytes -- the difference between actual
									 * bytes per frame is compared with this
									 * to determine if adjustment is required.
									 */
	U32 KeyAdjust;					/* quant level decrease for key frames */
} BRCInfoSt, * pBRCInfoSt;

/* Sequence Level information */
typedef struct RTContext_ {
	U32			nBandsY;		/* number of bands for Y */
	pFrmInfoSt	FrameInfo;		/* pointer to an array of FrmInfoSts */
	pVersionSt	pVersion;
	pBRCInfoSt	pBRCInfo;
	U32			nSrcRows;
	U32			nSrcCols;
	U32			SrcPitch;
	U32			CurrFrame;		/* used to select correct frame info */
	U32			PrevFrame;
	U32			KPRefBuff;
	U32			KPP2RefBuff;
	I32			Free1Buff;
	U32			FrameSequence;
	PU8			MotionBand[4];	/* used to store the 4 offsets for motion bands */
	PU8			RVBuf;			/* buffer for RV codes */
	PU8			YUVBuf;			/* temp buffer for YUV input */
	U32			KFIMax;			/* key frame interval control */
	U32			KFICount;
	U32			BrcEnable;		/* if TRUE enables bit rate control */
	U32			GlobalQuant;	/* picture level global quant */
	U32			PictureNum;		/* Picture number in decode order */
    U32         ScalabilityOn;  /* Scalability On Flag */                    
} RTContext, *pRTCntx;

/* prototypes for the ASM functions used in the RT encoder */

void EncYBands ( pBandInfoSt pBandInfo, pBlkInfoSt BlkInfo,
	PI16 pCurrBandData, PI16 pPrevBandData, PU8 *pRVBuf, U32 band,
	U32 NextFrameIsKey );
void EncYBandNoXform ( pBandInfoSt pBandInfo, pBlkInfoSt BlkInfo,
	PI16 pCurrBandData, PI16 pPrevBandData, PU8 *pRVBuf, U32 band,
	U32 NextFrameIsKey );
void EncUVBands ( pBandInfoSt pBandInfo, pBlkInfoSt BlkInfo, PU8 pUV,
	PU8 pCurrBandData, PU8 pPrevBandData, PU8 *pRVBuf, PU8 YUVBuf,
	U32 NextFrameIsKey );
void EncYPlane ( pBandInfoSt pBandInfo, pBlkInfoSt BlkInfo, PU8 pY,
	PI16 pCurrBandData, PU8 pPrevBandData, PU8 *pRVBuf, PU8 YUVBuf,
	U32 NextFrameIsKey );
U32 Haar2dQuantRV (PU8 pQMatrix0, U32 NonIntraFlag0, PU8 pQMatrix1,
	U32 NonIntraFlag1, PI16 pIn, PU8* ppOut, PIA_Boolean DoPair);
void FwdGlblXfrm (PU8 pY,U32 nRows,U32 nCols,U32 Pitch,PI16 pB0,PU8 pM);
void FwdGlblXfrmKl (PU8 pY,U32 nRows,U32 nCols,U32 Pitch,PI16 pB0,PU8 pM);
void SSCopyYPlane (PU8 pY,U32 nRows,U32 nCols,U32 Pitch,PU8 pM);
void VLCEncode0(PU8 pIn, PU8* ppOut, U32 FirstEncRule, U32 Len);
U32 MotionEstimation( pBlkInfoSt BlkInfo, I32 MBOffset, PU8 MotionBand,
        U32 NoSearch, U32 ClassifyRatio, PU32 pMotionDetected );

/* other prototypes */
void QuantDeltas( U32 NumBlocks, pBlkInfoSt BlockInfo, U32 CumIntraGrad,
	U32 GlobalQuant );
void BRCInit( pBRCInfoSt pBRCInfo, U32 DataRate );
void BRCInitIfRequired( pBRCInfoSt pBRCInfo, U32 DataRate ); 
U32 BRCAdjustGlobalQuant( pBRCInfoSt pBRCInfo, U32 FrameNum, U32 GlobalQuant,
	U32 KFI, U32 FrameType);
void BRCUpdate( pBRCInfoSt pBRCInfo, U32 ThisFrameSize );

#ifdef __cplusplus
extern "C" {				/* Assume C declarations for C++ */
#endif	/* __cplusplus */

/* rtenc function prototypes */
#if COM_INTFC == 1
pRTCntx RTCompressBegin(U32 nRows, U32 nCols, U32 KFIMax,U32 TargetBytes, 
	PIA_Boolean ScalabilityOn, IMalloc* pIMalloc);
U32 RTCompress(pRTCntx pCntx, PU8 Y, PU8 V, PU8 U, U32 KFIMax, PU32 FrameType,
	U32 BrcQualVal, PIA_Boolean FasterEncode,  PU8 bsOut, PU32 ImageSize );
void RTCompressEnd(pRTCntx pCntx, IMalloc* pIMalloc);
void RTCompressReallyBegin(pRTCntx pCntx, U32 TargetBytes);
#else
pRTCntx RTCompressBegin(U32 nRows, U32 nCols, U32 KFIMax,U32 TargetBytes, 
	PIA_Boolean ScalabilityOn);
U32 RTCompress(pRTCntx pCntx, PU8 Y, PU8 V, PU8 U, U32 KFIMax, U32 dwFrameRate, 
			   PU32 FrameType, U32 BrcQualVal, PIA_Boolean FasterEncode,  PU8 bsOut, PU32 ImageSize );
void RTCompressEnd(pRTCntx pCntx);
void RTCompressReallyBegin(pRTCntx pCntx, U32 TargetBytes);

#endif

#if defined __cplusplus
}
#endif

#endif /* __RTENC_H__ */

 
