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
*               Copyright (c) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

/*
 * enntryns.c
 *
 * This file handles the encoding of a colorplane and the plane context
 * structure.  
 *
 * Functions:
 *	EncBand				Compress a color plane
 *	EncBandOpen			Open the structure for a plane, allocate memory, init
 *	EncBandClose		Free a structure for a plane
 *
 */

#include <math.h>		/* for log */
#include <setjmp.h>
#include "datatype.h"
#include "matrix.h"

#include "mc.h"			/* for ivi5bs.h */
#include "hufftbls.h"
#include "qnttbls.h"

#include "ivi5bs.h"		/* for bsutil.h */
#include "mbhuftbl.h"
#include "bkhuftbl.h"

#ifdef SIMULATOR
#include "encsmdef.h"
#include "decsmdef.h"
#endif
#include "bsutil.h"

#include "pia_main.h"
#include "indeo5.h"
#ifdef CMD_LINE_ENC
#include "CmdLParm.h"  
#endif	/*CMD_LINE_ENC*/
#include "pia_enc.h"	/* includes datatype.h and pia_main.h */

#include "ensyntax.h"
#include "encoder.h"
#include "enme.h"
#include "enmesrch.h"
#include "enseg.h"
#include "context.h"
#include "enntryns.h"		 
#ifdef SIMULATOR
#include "encsim.h"
#endif

#include "tksys.h"
#include "const.h"
#include "common.h"
#include "errhand.h"

#include "bsdbg.h"

/* --------------  PROTOTYPES FOR PRIVATE ROUTINES  ----------------*/

static void SetTileInfo(I32 iPicTileWidth, I32 iPicTileHeight, I32 iPicWidth,
	I32 iPicHeight, I32 iBandWidth,	I32 iBandHeight, PI32 piTileWidth,
	PI32 piTileHeight, PI32 piNumTiles);

/* Calculates all of the errors needed for block type classification. */
static void	CalcMotionErrors(
	I32 iPictureType,	/* Type of the picture */	
	MatrixSt mRef[2],	/* Forward reference */
	MatrixSt mPicOrig,	/* Target image */
	I32 iNumTiles,		/* Number of tiles */
	pTileSt pTileInfo,	/* Array of tiles in info structures */
	I32 iErrMeasure, 	/* Error measure to use */
	PIA_Boolean bMeErrAvailable); /* Whether or not the ME error is already available */

static void	MBTypeClass(
	Dbl dIntraThresh, 	/* Threshold for Intra/Inter decision */
	Dbl dFW0Thresh,		/* Threshold for FW0 decision */
	Dbl dFWLimit,	    /* Smallest per pixel error that will code with MV */
	I32 iNumTiles,	/* Number of tiles */
	pTileSt pTileInfo);

/* Calculates the intra errors for key frames */
static void	CalcBandIntraErrors(
	MatrixSt mPicOrig,		/* orig pic */
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTileInfo,		/* Pointer to tile info */
	I32 iErrMeasure);			/* Designates MADM or MSE */

/* Calculates the total error */
static Dbl CalcTotalMCErr(
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTileInfo);		/* Pointer to tile info */

static Dbl ErrorVerify(
	MatrixSt mPicOrig,		/* Original picture */
	MatrixSt mComp,			/* Motion compensated picture */
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTileInfo,		/* Pointer to tile info */
	I32 iErrMeasure,		/* Error measure to use */
	jmp_buf jbEvn);

#ifdef SIMULATOR /* Force forward motion estimation */
static void	EncSimForceForwardMotionEstimation(
	I32 iNumTiles,		/* Number of tiles */
	pTileSt pTileInfo);
#endif /* SIMULATOR, Force forward motion estimation */

#ifdef SIMULATOR /* Dump motion type classification */
static void	EncSimDumpMotionTypeClassification(
	I32 iNumTiles,		/* Number of tiles */
	pTileSt pTileInfo);
#endif /* SIMULATOR, Dump motion type classification */


/* The following tables of constants are the correct Intra Block */
/* selection thresholds by quantization level for Nonscalable and */
/* and scalable sequences.  These tables are determined by a process */
/* of evaluating which setting minimizes the datarate for each quant */
/* level over a variety of clips (these were determined using 8 clips). */
/* These tables are therefore not arbitrary and should not be modified */
/* haphazardly, as doing so can seriously impact quality/datarate */
/* performance of the encoder.  Situations in which these tables should */
/* be updated include: new quantization tables, new transforms, major */
/* entropy coding changes, multiple level band decompostion (for the */
/* scalable table, etc. */

static Dbl dNonScalableIntraThresh[24] = {1.7, 0.7, 0.7, 1.0,  
										  0.9, 1.0, 1.1, 0.9,  
										  0.9, 0.9, 0.8, 0.8,  
										  0.9, 0.8, 0.8, 0.9,   
										  0.8, 0.8, 0.7, 0.6,  
										  0.7, 0.7, 0.5, 0.7};

static Dbl dScalableIntraThresh[24] = {1.3,	0.7, 0.9, 0.9,
									   0.8, 0.9, 0.9, 0.9,
									   0.8,	0.9, 1.0, 1.0,
									   1.0, 1.0, 0.9, 1.0,  
									   0.8, 1.0, 1.0, 0.9,
									   0.9, 1.0, 0.9, 0.8};

static Dbl dNonScalableFW0Thresh[24] = {0.99, 0.99, 0.99, 0.99,
									    0.99, 0.99, 0.99, 0.99,
									    0.99, 0.99, 0.99, 0.99,
									    0.99, 0.99, 0.99, 0.99,
									    0.99, 0.99, 0.99, 0.99,
									    0.99, 0.99, 0.99, 0.99};

static Dbl dScalableFW0Thresh[24] = {0.95, 0.93, 0.96, 0.92,
									 0.99, 0.99, 0.97, 0.97, 
									 0.95, 0.99, 0.98, 0.99,
									 0.99, 0.99, 0.99, 0.99,
									 0.99, 0.99, 0.99, 0.99,
									 0.99, 0.99, 0.99, 0.99};

static Dbl dNonScalableFWLimit[24] = {0.0, 0.0, 0.0, 0.0,
									  0.0, 0.0, 0.1, 0.2,
									  0.2, 0.2, 0.2, 0.3,
									  0.3, 0.3, 0.3, 0.3,
									  0.3, 0.3, 0.4, 0.4,
									  0.4, 0.4, 0.5, 0.6};

static Dbl dScalableFWLimit[24] = {1.0,1.5, 1.5, 2.0, 
								   1.5,2.0, 2.0, 2.0,
								   1.5,2.0, 2.5, 2.0,
								   2.5,2.5, 2.5, 2.5,
								   3.0,3.0, 3.5, 3.5,
								   3.5,2.5, 4.0, 4.5};

#ifdef SIMULATOR

/*  This table needs to be Tuned */

static Dbl dTwoLevelScalableIntraThresh[24] = {0.90, 0.90, 0.90, 0.90,
											   0.90, 0.90, 0.90, 0.90,
											   0.90, 0.90, 0.90, 0.90,
											   0.90, 0.90, 0.60, 0.60,
											   0.60, 0.60, 0.60, 0.60, 
											   0.60, 0.60, 0.60, 0.60};

static Dbl dTwoLevelScalableFW0Thresh[24] = {0.95, 0.95, 0.95, 0.95,
											 0.95, 0.95, 0.95, 0.95,
											 0.95, 0.95, 0.95, 0.95,
											 0.95, 0.95, 0.95, 0.95,
											 0.95, 0.95, 0.95, 0.95,
											 0.95, 0.95, 0.95, 0.95};

static Dbl dTwoLevelScalableFWLimit[24] = {4.0, 4.0, 4.0, 4.0,
										   4.0, 4.0, 4.0, 4.0,
										   4.0, 4.0, 4.0, 4.0,
										   4.0, 4.0, 4.0, 4.0,
										   4.0, 4.0, 4.0, 4.0,
										   4.0, 4.0, 4.0, 4.0};
#endif /* SIMULATOR */

/* ------------------------  OPEN  --------------------------*/
/* Initialize the sequence and return ptr to EncBandCntxSt. 
*/
PEncBandCntxSt EncBandOpen(
	PEncConstInfoSt pCInfo,
	PEncRtParmsSt pRtParms,
	pBandSt pBsBandInfo,
	ppBandSt ppBand0Info,
	PIA_Boolean  bIsExpBS,
	jmp_buf jbEnv)		 
{
	PEncBandCntxSt pBandContext;
	I32 iTile, iTileC, iTileR;	/* Indices to loop through tiles */
	I32 iMacBlock, iMBc, iMBr;	/* Indices to loop through macro blocks */
	I32 iNumMacBlocks; 			/* Number of macro blocks per tile */
	I32 iNumBlocks; 			/* Number of blocks in a macro block */
	I32 iBlockPixels;			/* Number of pixels in a block */
	I32 iBlockC, iBlockR;		/* Index for blocks */
	I32 iCnt = 0; 				/* Misc Counter */
	U8 u8MBSize,u8BlkSize ;				/* Macro block size and block size*/
	pBandHdrSt pBandHdr;
	pBandDscrptSt pBandDscrptor;
	pTileSt	pTileInfo;
	pTileHdrSt pTileInfoHdr;
	pMacBlockSt pMBInfo;
	pBlockSt pBlockDat;
	I32 iTileWidth, iTileHeight, iNumTiles;

	PU8 pu8MemBuf;		 		/* Buffer to divvy memory from */
	I32 iSize = 0;
	I32 iNumBlocksPerMB = 0;
	pBandSt Y_Band0Info = *ppBand0Info;
	
	/* get/setup context */
	pBandContext = (PEncBandCntxSt) SysMalloc(sizeof(EncBandCntxSt), jbEnv);

#ifdef SIMULATOR
	pBandContext->pSimInst = pRtParms->pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */

	pBandContext->Resolution = pRtParms->MeResolution;
	pBandContext->PicMc = pCInfo->Pic;
	pBandContext->ErrMeasure = pRtParms->MeMeasure;
	pBandContext->GlobalQuant = pRtParms->GlobalQuant;	/* BRC init stuff */

	for (iCnt = 0; iCnt < 2; iCnt++)	/* init ref pics  */
		pBandContext->PicRef[iCnt] = pCInfo->Pic;

	/* Open and init ME */
	pBandContext->EncMeCntx = EncMeOpen(pRtParms, jbEnv);

	/* Code to reuse MC info from band 0 of Y plane. Can only reuse
		if there is a 1:1 match of blocks - and planes are same size 
		(this ensures blocks are same size - but could scale) */

	pBandContext->McUseBand0 = FALSE;	/* Default to no McUseBand0 */
	pBandContext->QuantUseBand0 = FALSE;

	/* Calculate block sizes for shift */
	if(pCInfo->Color == COLOR_Y)
		u8MBSize = (U8) pRtParms->YMBSize[pCInfo->BandId];
	else 
		u8MBSize = (U8) pRtParms->VUMBSize[pCInfo->BandId];

	if (pCInfo->Color == COLOR_Y && pCInfo->BandId == 0) {
		Y_Band0Info = pBsBandInfo;
		*ppBand0Info = pBsBandInfo;
	} else if ((pRtParms->McUseBand0) || (pRtParms->QuantUseBand0)) {
		I8 i8Shift;

		/* Calculate what the resolution of this band must be to */
		/* match the resolution and macroblock size of Band 0    */

		/* Verify that there is a 1-to-1 correspondance of MBs */

		i8Shift = ((( u8MBSize) >> 3) -
		         (Y_Band0Info->pBandDscrptor->MacBlockSize >> 3));

		if (i8Shift > 0) {
			if ((pBandContext->PicMc.NumRows==Y_Band0Info->BandRect.h<<i8Shift)&&
				(pBandContext->PicMc.NumCols==Y_Band0Info->BandRect.w<<i8Shift)){
				/* ok, passes all tests */
				pBandContext->McUseBand0 = pRtParms->McUseBand0;	
				pBandContext->QuantUseBand0 = pRtParms->QuantUseBand0;	
			}
		} else {
			if ((pBandContext->PicMc.NumRows ==
								Y_Band0Info->BandRect.h >> (-i8Shift)) &&
				(pBandContext->PicMc.NumCols ==
								Y_Band0Info->BandRect.w >> (-i8Shift))) {
				/* ok, passes all tests */
				pBandContext->McUseBand0 = pRtParms->McUseBand0;		
				pBandContext->QuantUseBand0 = pRtParms->QuantUseBand0;	
			}
		} /* end if (i8Shift > 0) */
	}

	pBandContext->Band0Info = Y_Band0Info; /* Always points to Band 0 Info */

	pBandContext->UseVarQuant = pRtParms->UseVarQuant;
		
	/* alloc arrays */
	/* Rather than do 6 MatAlloc's, do all the allocation together
	 * and handle the divvying and other stuff here.  Tally up how
	 * much to alloc, alloc, then divvy the memory throughout 
	 */
	pBandContext->PicMc.Pitch = (pBandContext->PicMc.NumCols + 7) & ~0x7; 
	iSize += (I32) pBandContext->PicMc.Pitch * 
			(pBandContext->PicMc.NumRows + 7) & ~0x7;

	pBandContext->PicXfrmRes = pBandContext->PicMc;	 /* same dimension */
	iSize += (I32) pBandContext->PicXfrmRes.Pitch * 
			(pBandContext->PicXfrmRes.NumRows + 7) & ~0x7;

	for (iCnt = 0; iCnt < 2; iCnt++)	{
		pBandContext->PicRef[iCnt].Pitch = 
						(pBandContext->PicRef[iCnt].NumCols + 7) & ~0x7;

		iSize += (I32) pBandContext->PicRef[iCnt].Pitch * 
				(pBandContext->PicRef[iCnt].NumRows + 7) & ~0x7;
	}

	pu8MemBuf = (PU8) SysMalloc(iSize * sizeof(I16), jbEnv);

	/* parcel out the memory */

	pBandContext->PicMc.pi16 = (PI16) pu8MemBuf;
	pu8MemBuf += ((pBandContext->PicMc.NumRows + 7) & ~0x7) *
				 (pBandContext->PicMc.Pitch) * sizeof(I16);	

	pBandContext->PicXfrmRes.pi16 = (PI16) pu8MemBuf;
	pu8MemBuf += ((pBandContext->PicXfrmRes.NumRows + 7) & ~0x7) *
				 (pBandContext->PicXfrmRes.Pitch) * sizeof(I16);	

    for (iCnt = 0; iCnt < 2; iCnt++) {
		pBandContext->PicRef[iCnt].pi16 = (PI16) pu8MemBuf;
		pu8MemBuf += ((pBandContext->PicRef[iCnt].NumRows + 7) & ~0x7) *
					 (pBandContext->PicRef[iCnt].Pitch) * sizeof(I16);
	}


	/* Bitstream allocation and Initialization for tiles and band info */

	/*
	 * Determine the tile size using the command line input (RtParams)
	 * and the now known Band size (pCInfo->Pic).  Set the values of
	 * tile size and number of tiles in the Band info structure.
	 */

	SetTileInfo(pRtParms->iaTileSize[0], pRtParms->iaTileSize[1],
		pCInfo->iNumCols, pCInfo->iNumRows, pCInfo->Pic.NumCols,
		pCInfo->Pic.NumRows, &iTileWidth, &iTileHeight, &iNumTiles);

	/* The static tables are all stored at the picture level */
	pBsBandInfo->HuffInfo.HTBlock = pRtParms->HTBlock;
	pBsBandInfo->HuffInfo.FRVTbl = pRtParms->FRVTbl;
	pBsBandInfo->HuffInfo.UseRVChangeList = pRtParms->UseRVChange;

	pBandHdr = &pBsBandInfo->BandHdr;
	pBandDscrptor = pBsBandInfo->pBandDscrptor;
	pBsBandInfo->BandId = pCInfo->BandId;

	for (iCnt = 0; iCnt < NUM_PIC_DRC_TYPES; iCnt++) {
		pBsBandInfo->BandDRCCntx[iCnt].RVMapTblNum = NoLastTbl;
		pBsBandInfo->BandDRCCntx[iCnt].BlkHuffTbl.StaticTblNum = NoLastTbl;
	}

	pBsBandInfo->NumTiles = iNumTiles;
	pBsBandInfo->Tiles = 
		(pTileSt) SysMalloc(pBsBandInfo->NumTiles * sizeof(TileSt), jbEnv);
	BitBuffAlloc(&(pBsBandInfo->BsBandHdr), sizeof(BandHdrSt), jbEnv);
	pBsBandInfo->BandRect.r = pBsBandInfo->BandRect.c = 0;
	pBsBandInfo->BandRect.w = pCInfo->Pic.NumCols;
	pBsBandInfo->BandRect.h = pCInfo->Pic.NumRows;

	pBandHdr->ColorPlane = (U8) pCInfo->Color;
	pBandHdr->BandId = (U8) pCInfo->BandId;
	pBandDscrptor->MVRes = (U8) pBandContext->Resolution;
	pBandHdr->UseChecksum = TRUE;
	pBandHdr->InheritTypeMV = pBandContext->McUseBand0;
	pBandHdr->QuantDeltaUsed = pBandContext->UseVarQuant; 

		/* initialize NumChanges to 0: deternmined by FindRVChangeList() later in enbs.c */ 
	pBandHdr->RVSwapList.NumChanges = 0;
	pBandHdr->UseBandExt = FALSE;


	/* set up plane dependant band encoding parameters:
	/	MBSize, BlockSize, Xform, Scan order, QuantMatrices
	*/
	if( pCInfo->Color == COLOR_Y ) {
 		u8MBSize = pBandDscrptor->MacBlockSize = (U8) pRtParms->YMBSize[pCInfo->BandId];
		u8BlkSize = pBandDscrptor->BlockSize = (U8) pRtParms->YBlockSize[pCInfo->BandId];
		pBandDscrptor->Xform = (U8) pRtParms->YXfrm[pCInfo->BandId];
			/* always use the default scan order */	
		pBandDscrptor->ScanOrder = (U8) xform_to_scan[pBandDscrptor->Xform]; 
		pBandDscrptor->QuantMatrix = (U8) pRtParms->YQuant[pCInfo->BandId];
		/* get quant table */
		InitQuantTables(pBandDscrptor->QuantMatrix,pBsBandInfo->uwQuantTable);
		/* point to scan order */
		pBsBandInfo->pubScan = &(ubScan[pBandDscrptor->ScanOrder][0]);
	} else { /* u or v planes */
		u8MBSize = pBandDscrptor->MacBlockSize = (U8) pRtParms->VUMBSize[pCInfo->BandId];
		u8BlkSize =pBandDscrptor->BlockSize = (U8) pRtParms->VUBlockSize[pCInfo->BandId];
		pBandDscrptor->Xform = (U8) pRtParms->VUXfrm[pCInfo->BandId];
		pBandDscrptor->ScanOrder = (U8) xform_to_scan[pBandDscrptor->Xform];
		pBandDscrptor->QuantMatrix = (U8) pRtParms->VUQuant[pCInfo->BandId];
		/* get quant table */
		InitQuantTables(pBandDscrptor->QuantMatrix,pBsBandInfo->uwQuantTable);
		/* point to scan order */
		pBsBandInfo->pubScan = &(ubScan[pBandDscrptor->ScanOrder][0]);
 	} /* end for u or v planes */

	/* end set up plane dependant band encoding parameters:
	/	MBSize, BlockSize, Xform, Scan order, QuantMatrices
	*/

	/*  Determine the amount of space needed for tiles, blocks and runvalues,
		so that the allocation can be made all at once, and initialize
		the tile information */
	iSize = 0;
	for (iTileR = 0, pTileInfo = pBsBandInfo->Tiles;
			iTileR < (I32)pCInfo->Pic.NumRows;	iTileR += iTileHeight){
		for (iTileC = 0; iTileC < (I32)pCInfo->Pic.NumCols; iTileC += iTileWidth){
			pTileInfoHdr = &pTileInfo->TileHdr;
			pTileInfoHdr->IsEmpty = FALSE;
			pTileInfoHdr->IsTileDataSize = TRUE;
			pTileInfoHdr->TileDataSize = 0;
			pTileInfoHdr->NumBlks = 0;

			BitBuffAlloc(&(pTileInfo->BsTileHdr), sizeof(TileHdrSt), jbEnv);
			BitBuffAlloc(&(pTileInfo->BsMacHdr), 100, jbEnv);	
			BitBuffAlloc(&(pTileInfo->BsPreHuffMbHdr), 100, jbEnv);	
			BitBuffAlloc(&(pTileInfo->BsBlockDat), 100, jbEnv);
			BitBuffAlloc(&(pTileInfo->BsPreHuffBlockDat), 100, jbEnv);

			/* Calculate rectangle information */
			pTileInfo->TileRect.r = (U32) iTileR;
			pTileInfo->TileRect.c = (U32) iTileC;
			pTileInfo->TileRect.h =  iTileHeight - 
				MAX(0, iTileR + iTileHeight - (I32)pCInfo->Pic.NumRows);
			pTileInfo->TileRect.w = iTileWidth - 
				MAX(0, iTileC + iTileWidth - (I32)pCInfo->Pic.NumCols);


			/* The + 1 is to include the EOB code in worst case */
			iBlockPixels = u8BlkSize  * u8BlkSize + 1;

			/* First calculate the number of macro blocks in a row */
			iNumMacBlocks = (pTileInfo->TileRect.h / u8MBSize) + 
				((pTileInfo->TileRect.h % u8MBSize) > 0 ? 1 : 0);

			/* Multiply the number of macro blocks in a row by 
			   the number in a column */
			iNumMacBlocks *= (pTileInfo->TileRect.w / u8MBSize) + 
				((pTileInfo->TileRect.w % u8MBSize) > 0 ? 1 : 0);

			pTileInfo->NumMacBlocks = iNumMacBlocks;
			
			iSize += iNumMacBlocks * sizeof(MacBlockSt);

			for (iMBr=iTileR; iMBr < iTileR + (I32)pTileInfo->TileRect.h; 
					iMBr += u8MBSize){
				for (iMBc=iTileC; iMBc < iTileC + (I32)pTileInfo->TileRect.w; 
						  iMBc += u8MBSize){
					I32 iHeight, iWidth;
					iHeight = u8MBSize - 
						MAX(0, iMBr + u8MBSize - (iTileR + (I32)pTileInfo->TileRect.h));
					iWidth = u8MBSize - MAX(0, iMBc + u8MBSize -(iTileC + (I32)pTileInfo->TileRect.w));

					iNumBlocksPerMB = (((iHeight / u8BlkSize) +
								      ((iHeight % u8BlkSize) > 0 ? 1 : 0)) *
							          ((iWidth / u8BlkSize)+
								      ((iWidth % u8BlkSize) > 0 ? 1 : 0)));

					iSize += iNumBlocksPerMB * sizeof(BlockSt);
					iSize += sizeof(RunValSt) * iBlockPixels * iNumBlocksPerMB;
					pTileInfoHdr->NumBlks += (U32)iNumBlocksPerMB;

				} /* for iMBc*/
			} /* for iMBr */

			if (bIsExpBS) {
				if (pTileInfoHdr->NumBlks >= (U32)(1<<LenNumBlks-1)) {/* overflow the field */
					 pTileInfoHdr->NumBlks = 0;	 /* practically turn it off */
					 pTileInfoHdr->pu16BlkChecksums = NULL;
				} else {
					pTileInfoHdr->pu16BlkChecksums = 
						(PU16)SysMalloc(pTileInfoHdr->NumBlks*sizeof(U16), jbEnv);
				}
			}  else {
				pTileInfoHdr->pu16BlkChecksums = NULL;
			}
			pTileInfo++;
		} /* for iTileC */
	} /* for iTileR */
	pu8MemBuf = (PU8) SysMalloc(iSize, jbEnv);


	/* Initialize the iTiles in the info structures */
	for (iTileR = 0, iTile = 0, pTileInfo = pBsBandInfo->Tiles;
			iTileR < (I32)pCInfo->Pic.NumRows;	iTileR += iTileHeight){
		for (iTileC = 0; iTileC < (I32) pCInfo->Pic.NumCols; iTileC += iTileWidth){
			pTileInfo->MacBlocks = (pMacBlockSt) pu8MemBuf;
			pu8MemBuf += sizeof(MacBlockSt) * pTileInfo->NumMacBlocks;

			/* Initialize macro blocks.  Note: There is no reason to initialize
			   the macro block header information since these values must be
			   initialized from frame to frame.
		     */
			for (iMBr = iTileR, pMBInfo = pTileInfo->MacBlocks, iMacBlock = 0;
					iMBr < iTileR + (I32) pTileInfo->TileRect.h; 
					iMBr += u8MBSize){
				for (iMBc = iTileC; iMBc < iTileC + (I32)pTileInfo->TileRect.w;
						iMBc += u8MBSize){
					/* Calculate rectangle information */
					pMBInfo->MacBlockRect.r = iMBr;
					pMBInfo->MacBlockRect.c = iMBc;
					pMBInfo->MacBlockRect.h = u8MBSize - MAX(0, iMBr + u8MBSize - 
						(iTileR + (I32)pTileInfo->TileRect.h));
					pMBInfo->MacBlockRect.w = u8MBSize- MAX(0, iMBc + u8MBSize - 
						(iTileC + (I32)pTileInfo->TileRect.w));

					/* First calculate the number of blocks in a row */
					iNumBlocks = (pMBInfo->MacBlockRect.h/u8BlkSize) +
								((pMBInfo->MacBlockRect.h%u8BlkSize)> 0 ? 1 : 0);

					/* Multiply the number of blocks in a row by 
					   the number in a column*/
					iNumBlocks *= (pMBInfo->MacBlockRect.w/u8BlkSize)+
						((pMBInfo->MacBlockRect.w % u8BlkSize) > 0 ? 1 : 0);

					pMBInfo->NumBlocks = iNumBlocks;
					pMBInfo->Blocks = (pBlockSt) pu8MemBuf;
					pu8MemBuf += sizeof(BlockSt)*iNumBlocks;

					/* Initialize blocks */
					for (iBlockR = iMBr, pBlockDat = pMBInfo->Blocks;
							iBlockR < iMBr + (I32)pMBInfo->MacBlockRect.h; 
							iBlockR += u8BlkSize){
						for (iBlockC = iMBc; iBlockC < iMBc + (I32) pMBInfo->MacBlockRect.w;
								iBlockC += u8BlkSize){
							/* Calculate rectangle information */
							pBlockDat->BlockRect.r = iBlockR;
							pBlockDat->BlockRect.c = iBlockC;
							pBlockDat->BlockRect.h = u8BlkSize - 
								MAX(0, iBlockR + u8BlkSize - (iMBr + (I32)pMBInfo->MacBlockRect.h));
							pBlockDat->BlockRect.w = u8BlkSize - 
								MAX(0, iBlockC + u8BlkSize - (iMBc + (I32)pMBInfo->MacBlockRect.w));

							pBlockDat->NumRunVals = 0;
							pBlockDat->RunVals = (pRunValSt) pu8MemBuf;
							pu8MemBuf += sizeof(RunValSt) * iBlockPixels;
							pBlockDat++;
						} /* for(iBlockC) */
					} /* for(iBlockR) */
					iMacBlock++;
					pMBInfo++;
				} /* for(iMBc) */
			} /* for(iMBr) */
			iTile++;
			pTileInfo++;
		} /* for(iTileC) */
	} /* for(iTileR) */
	
	if (iTile != pBsBandInfo->NumTiles)
		longjmp(jbEnv,  (ENNTRYNS << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
	
	/* open context to be used */
	pBandContext->EncSegCntx = EncSegOpen(	pCInfo, pRtParms, 
											pBsBandInfo->Tiles, 
											iNumTiles,
											jbEnv);


	
	return pBandContext;
}



/* ------------------------  CLOSE  --------------------------*/
/* End the sequence; free any allocated storage
*/
void EncBandClose(PEncBandCntxSt pBandContext,  pBandSt pBsBandInfo, jmp_buf jbEnv)
{
	pTileSt	pTileInfo;
	pMacBlockSt pMBInfo;
	I32 iTile;				/* Tile index */

	EncSegClose(pBandContext->EncSegCntx, jbEnv);

	/* Because of the edges the number of blocks shown in the NumBlocks
		value of the MacBlockSt may be fewer than originally allocated so
		NumBlocks represents the number originally allocated. */

	pTileInfo = pBsBandInfo->Tiles;
	SysFree((PU8) (pTileInfo->MacBlocks), jbEnv);
	for(iTile = 0; iTile < pBsBandInfo->NumTiles; iTile++, pTileInfo++) {
		pMBInfo = pTileInfo->MacBlocks;
		BitBuffFree(&(pTileInfo->BsTileHdr), jbEnv);
		BitBuffFree(&(pTileInfo->BsBlockDat), jbEnv);
		BitBuffFree(&(pTileInfo->BsPreHuffBlockDat), jbEnv);
		BitBuffFree(&(pTileInfo->BsMacHdr), jbEnv);
		BitBuffFree(&(pTileInfo->BsPreHuffMbHdr), jbEnv);
		if (pTileInfo->TileHdr.pu16BlkChecksums!=NULL) {
			SysFree((PU8) pTileInfo->TileHdr.pu16BlkChecksums, jbEnv);
		}
	}
 	BitBuffFree(&(pBsBandInfo->BsBandHdr), jbEnv);
	SysFree((PU8) pBsBandInfo->Tiles, jbEnv);

	/* This pointer represents the memory for PicMc, mPicX,
	 * and pBandContext->PicRef[]
	 */
	SysFree((PU8)pBandContext->PicMc.pi16, jbEnv);

	EncMeClose(pBandContext->EncMeCntx, jbEnv);
	SysFree((PU8)pBandContext, jbEnv);
}


/* ------------------------  ENCODE  --------------------------*/
/* Encode target pic plane and return reconstructed pic.
This is the entry point for the (non-scalable) coding of a pic plane.
*/
void EncBand(
	PTR_ENC_INST pInst,
	PCPicHdrSt pcPicInfoHdr, /* Header with general pic info */
	pBandSt pBsBandInfo,
	PEncBandCntxSt pBandContext,
	MatrixSt mPicOrig,	/* orig */
	MatrixSt mPicX,		/* Reconstructed Band */
	U8 u8EncodeMode,
	jmp_buf jbEnv )
{
	MatrixSt mRef[2];		/* Reference pictures */
	
	I32 iCnt; 		/* Misc Counter */
	I16 i16Zero=0;
	I32 iTile, iMB;			/* Indices to tiles and macro blocks */
	pMacBlockSt pMBInfo;
	pMacHdrSt pMBInfoHdr;
	pTileSt pTile;
	pcMacBlockSt pB0MBInfo;
	pcTileSt pB0Tile;
	pBandHdrSt pBandHdr = &pBsBandInfo->BandHdr;
	pBandDscrptSt pBandDscrptor = pBsBandInfo->pBandDscrptor;
	U8 u8MVRes = pBandDscrptor->MVRes;
	U8 u8MBSize = pBandDscrptor->MacBlockSize;
	PIA_Boolean bMeErrAvailable;

#if __INTEL__
	volatile Dbl dTotalMCErr, dTotalMCErr2;
#else
	Dbl dTotalMCErr, dTotalMCErr2;
#endif


	/* Initialize the IsDropped flag to false.  The encoder should never drop a 
	 * band unless directed to do so for simulation, datarate or testing reasons.
	 * WARNING: If a band is dropped, that band must also be dropped for each of
	 * the following frames in the GOP.  This encoder does not currently
	 * automatically do this, so and code which set IsDropped to TRUE must 
	 * ensure this constraint is held.  It is NEVER valid to drop Band 0 of the
	 * Y plane.
	 */

	pBandHdr->IsDropped = FALSE;  

	if 	(!pcPicInfoHdr->PicInheritQ) {	
		/* Q-inheritance is off for all bands if picture level is off */
		 pBandHdr->QuantDeltaUsed = pBandHdr->InheritQ = FALSE;
	}  else {
		 pBandHdr->QuantDeltaUsed = pBandContext->UseVarQuant;
		 pBandHdr->InheritQ = pBandContext->QuantUseBand0;
	}

	if ( (u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_TEST_WATER)) {
	  /* if encode mode is test_water or all and it is not an empty band, then 
		 initialize the info header data.  Even if the band is empty then we
		 want the data to be initialized so that it will match the decoder
		 data. */

		pTile = pBsBandInfo->Tiles;
		for (iTile = 0; iTile < pBsBandInfo->NumTiles; iTile++, pTile++) {
			pMBInfo = pTile->MacBlocks;
			pTile->TileHdr.IsEmpty = FALSE;
			for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
				pMBInfoHdr = &pMBInfo->MacHdr;
				pMBInfoHdr->MbType = TYPE_MV_I;
				pMBInfoHdr->Cbp = 0;
				pMBInfoHdr->McVector.r = 0;
				pMBInfoHdr->McVector.c = 0;
			}
		}
	}

	if (!pBandHdr->IsDropped) {
		/* If the band is not set to empty to encode it */
		if ((u8EncodeMode == ENC_MODE_ALL) || 
		    (u8EncodeMode == ENC_MODE_TEST_WATER)) { 
			/* if encode mode is test_water or all and it is not an empty band,
			   then do the Motion Estimation and Pred */

			/* Do motion estimation and form predicted pic */
			if (pcPicInfoHdr->PictureType == PIC_TYPE_K) {
				MatSetFull(pBandContext->PicMc, &i16Zero, jbEnv);	/* clear to zero */
				CalcBandIntraErrors(mPicOrig, pBsBandInfo->NumTiles, 
									pBsBandInfo->Tiles,
									pBandContext->ErrMeasure);
			} else {	
				/* motion analysis - write info to BS */
  				/* should IMF info be inherited from band 0. */
				if (pBandContext->McUseBand0) {
					I8 i8MVShift;/*  controls how inherited motion vectors  are scaled.	*/

					/*			**** WARNING ****
					 * The correct general formula for i8MVShift is :
					 *	i8MVShift = 
					 *		log2(u8MBSize/pBandContext->Band0Info->pBandDscrptor->MacBlockSize))
					 * A more efficient formula	(which is used here )
					 *   i8MVShift = 
					 *		(((u8MBSize) >> 3) - (pBandContext->Band0Info->pBandDscrptor->MacBlockSize >> 3));
					 * exists ONLY if the MBSizes are either 4, or 8 or 16.
					 * 
					 * It does NOT work in general for other combinations of MBSizes.
					 * 
					 * Note: The same formula is used in enntryns.c and parsebs.c. 
					 */

					i8MVShift = (((u8MBSize) >> 3) -
		   				 (pBandContext->Band0Info->pBandDscrptor->MacBlockSize >> 3));

					pTile = pBsBandInfo->Tiles;
					pB0Tile = pBandContext->Band0Info->Tiles;

					for (iTile = 0; iTile < pBsBandInfo->NumTiles;	
						 iTile++, pTile++, pB0Tile++) {

						pMBInfo = pTile->MacBlocks;
						pB0MBInfo = pB0Tile->MacBlocks;

						for (iMB = 0; iMB < pTile->NumMacBlocks;
							 iMB++, pMBInfo++, pB0MBInfo++) {

							pMBInfo->MacHdr.MbType = pB0MBInfo->MacHdr.MbType;

							if (i8MVShift > 0) {
								pMBInfo->MacHdr.McVector.r = 
									pB0MBInfo->MacHdr.McVector.r << i8MVShift;
								pMBInfo->MacHdr.McVector.c = 
									pB0MBInfo->MacHdr.McVector.c << i8MVShift;
							} else {
								pMBInfo->MacHdr.McVector.r = 
									pB0MBInfo->MacHdr.McVector.r >> (-i8MVShift);
								pMBInfo->MacHdr.McVector.c = 
									pB0MBInfo->MacHdr.McVector.c >> (-i8MVShift);
							}

							if (pMBInfo->MacHdr.McVector.r % u8MVRes != 0) {
								pMBInfo->MacHdr.McVector.r = 
									DIV_ROUND(pMBInfo->MacHdr.McVector.r, u8MVRes) * u8MVRes;
							}

							if (pMBInfo->MacHdr.McVector.c % u8MVRes != 0) {									  
								pMBInfo->MacHdr.McVector.c = 
									DIV_ROUND(pMBInfo->MacHdr.McVector.c, u8MVRes) * u8MVRes;
							}

							if ((pMBInfo->MacHdr.McVector.r % u8MVRes != 0) ||
								(pMBInfo->MacHdr.McVector.c % u8MVRes != 0)) {
							
								longjmp(jbEnv,  (ENNTRYNS >> FILE_OFFSET) |
												(__LINE__ >> LINE_OFFSET) |
												(ERR_BAD_PARM >> TYPE_OFFSET));
							}

							for (iCnt = 0; iCnt < MAX_ME_TYPES; iCnt++) {
								/*  Copy all of the vectors over for proper error
									calculation in CalcMotionErrors(). */
								if (i8MVShift > 0) {
									pMBInfo->pMEInfo[iCnt].Vect.r = 
										pB0MBInfo->pMEInfo[iCnt].Vect.r << i8MVShift;
									pMBInfo->pMEInfo[iCnt].Vect.c = 
										pB0MBInfo->pMEInfo[iCnt].Vect.c << i8MVShift;
								} else {
									pMBInfo->pMEInfo[iCnt].Vect.r = 
										pB0MBInfo->pMEInfo[iCnt].Vect.r >> (-i8MVShift);
										pMBInfo->pMEInfo[iCnt].Vect.c = 
										pB0MBInfo->pMEInfo[iCnt].Vect.c >> (-i8MVShift);
								}
	      						if (pMBInfo->pMEInfo[iCnt].Vect.r % u8MVRes != 0) {
							 		pMBInfo->pMEInfo[iCnt].Vect.r = 
							 			DIV_ROUND(pMBInfo->pMEInfo[iCnt].Vect.r, u8MVRes) * u8MVRes;
								}
	      						if (pMBInfo->pMEInfo[iCnt].Vect.c % u8MVRes != 0) {
								    pMBInfo->pMEInfo[iCnt].Vect.c = 
							   			DIV_ROUND(pMBInfo->pMEInfo[iCnt].Vect.c, u8MVRes) * u8MVRes;
								}
								if ((pMBInfo->pMEInfo[iCnt].Vect.r % u8MVRes != 0) ||
									(pMBInfo->pMEInfo[iCnt].Vect.c % u8MVRes != 0)) {									
										longjmp(jbEnv,  (ENNTRYNS >> FILE_OFFSET) |
														(__LINE__ >> LINE_OFFSET) |
														(ERR_BAD_PARM >> TYPE_OFFSET));
								}
							}
						} /* end for each macroblock in this tile */
					} /* end for each tile in this band */
					bMeErrAvailable = FALSE; 
				} /* end if use band 0 vectors for mc */

				else {  /* Inheritance is not on, so do ME */

					for (iCnt = 0; iCnt < 2; iCnt++) {
						 mRef[iCnt] = pBandContext->PicRef[iCnt];
					}
					/* P Frames predict from buffer 1, D and P2 from */
					/* buffer 0 */
					if (pcPicInfoHdr->PictureType == PIC_TYPE_P) {
					/* Forward motion estimation */
						EncMe(pBandContext->EncMeCntx, 
						      mRef[1], 
							  mPicOrig, 
							  pBsBandInfo->NumTiles, 
							  pBsBandInfo->Tiles, 
							  jbEnv);
					} else { /* D and P2 Frames */
						EncMe(pBandContext->EncMeCntx, 
							  mRef[0], 
							  mPicOrig, 
							  pBsBandInfo->NumTiles, 
							  pBsBandInfo->Tiles, 
							  jbEnv);
					}
				
					bMeErrAvailable = TRUE; 

					HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);
  		
 					pTile = pBsBandInfo->Tiles;

 					for (iTile = 0; iTile < pBsBandInfo->NumTiles; iTile++, pTile++) {

						pMBInfo = pTile->MacBlocks;

						for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
							pMBInfo->MacHdr.MbType =  TYPE_MV_FW;
							pMBInfo->MacHdr.McVector =
								pMBInfo->pMEInfo[TYPE_MV_FW].Vect;
						}
					}
				}
				for (iCnt = 0; iCnt < 2; iCnt++) {
					mRef[iCnt] = pBandContext->PicRef[iCnt];
				}
  					
				HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);

				/* Calculate all the errors used in MB type classification */

				CalcMotionErrors(pcPicInfoHdr->PictureType, mRef, mPicOrig,
								 pBsBandInfo->NumTiles, pBsBandInfo->Tiles,
								 pBandContext->ErrMeasure, bMeErrAvailable);
  		
				HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);
				/* Use previously calc'd errors to classify each macro block */
				if (!pBandContext->McUseBand0) {

					Dbl dIntraThresh;  /* Declare Temp Intra Thresh */
					Dbl dFW0Thresh;    /* Declare Temp FW0 Thresh */
					Dbl dFWLimit;	   /* Declare Temp FW0 Thresh */

					/* Decide on the correct Intra Threshold for Block */
					/* Classification.  Depends on Quant Level and */
					/* Scalability Setting. */
					
					if (pcPicInfoHdr->YSubDiv[0] == SUBDIVLEAF) {
						/* If the first Y band subdivision code is */
						/* SUBDIVLEAF, then scalability is off.    */

						dIntraThresh = dNonScalableIntraThresh[pcPicInfoHdr->GlobalQuant];
						dFW0Thresh = dNonScalableFW0Thresh[pcPicInfoHdr->GlobalQuant];
						dFWLimit = dNonScalableFWLimit[pcPicInfoHdr->GlobalQuant];

					} else {
						/* Else, scalability is on. */

	#ifdef SIMULATOR	/* Cover the 2 level and beyond scalability cases. */				
						/* If the 2nd split code is not SUBDIVLEAF, then at */
						/* least two level decomposition is in use. */

						if (pcPicInfoHdr->YSubDiv[1] == SUBDIVLEAF) {
							dIntraThresh = dScalableIntraThresh[pcPicInfoHdr->GlobalQuant];
							dFW0Thresh = dScalableFW0Thresh[pcPicInfoHdr->GlobalQuant];
							dFWLimit = dScalableFWLimit[pcPicInfoHdr->GlobalQuant];

						} else { /*  This case needs to be tuned */
							dIntraThresh = dTwoLevelScalableIntraThresh[pcPicInfoHdr->GlobalQuant];
							dFW0Thresh = dTwoLevelScalableFW0Thresh[pcPicInfoHdr->GlobalQuant];
							dFWLimit = dTwoLevelScalableFWLimit[pcPicInfoHdr->GlobalQuant];
						}
							
	#else   /* SIMULATOR */
						dIntraThresh = dScalableIntraThresh[pcPicInfoHdr->GlobalQuant];
						dFW0Thresh = dScalableFW0Thresh[pcPicInfoHdr->GlobalQuant];
						dFWLimit = dScalableFWLimit[pcPicInfoHdr->GlobalQuant];
	#endif  /* SIMULATOR */
					}

	#ifdef SIMULATOR /* Set intra threshold from the simulation parameters */
					if (pBandContext->pSimInst->SimParms.bIntraThreshSet) {
						dIntraThresh = pBandContext->pSimInst->SimParms.dIntraThresh;
					}

					if (pBandContext->pSimInst->SimParms.bFW0ThreshSet) {
						dFW0Thresh = pBandContext->pSimInst->SimParms.dFW0Thresh;
					}

					if (pBandContext->pSimInst->SimParms.bFWLimitSet) {
						dFWLimit = pBandContext->pSimInst->SimParms.dFWLimit;
					}

	#endif /* SIMULATOR, Set intra threshold from the simulation parameters */

					MBTypeClass(dIntraThresh, 
								dFW0Thresh,
								dFWLimit,
								pBsBandInfo->NumTiles, 
								pBsBandInfo->Tiles);

	#ifdef SIMULATOR /* Force forward motion estimation */
					if (pBandContext->pSimInst->SimParms.bForceForwardME) {
						EncSimForceForwardMotionEstimation(pBsBandInfo->NumTiles,
							pBsBandInfo->Tiles);
					}
	#endif /* SIMULATOR, force forward motion estimation */

	#ifdef SIMULATOR /* Dump motion type classification */
					if (pBandContext->pSimInst->SimDumps.bDumpMotionTypeClassification) {
						EncSimDumpMotionTypeClassification(pBsBandInfo->NumTiles,
							pBsBandInfo->Tiles);
					}
	#endif /* SIMULATOR, Dump motion type classification */

					HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);
				}
				
				/* Create motion compensated estimate */
				pTile = pBsBandInfo->Tiles;

				for (iTile = 0; iTile < pBsBandInfo->NumTiles; iTile++, pTile++) {
						
					pMBInfo = pTile->MacBlocks;
					for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
						pMBInfoHdr = &pMBInfo->MacHdr;
						if (pMBInfoHdr->MbType == TYPE_MV_I) {
							MatSet(pBandContext->PicMc, 
								   pMBInfo->MacBlockRect, 
								   &i16Zero, jbEnv);
						} else {

							/* Do the motion compensation */
							if (pcPicInfoHdr->PictureType == PIC_TYPE_P) { 									
								McRectInterp(mRef[1], 
										pBandContext->PicMc, 
										pMBInfo->MacBlockRect,
										pMBInfoHdr->McVector, jbEnv);
							} else { /* D or P2 Frame */
 								McRectInterp(mRef[0], 
										pBandContext->PicMc, 
										pMBInfo->MacBlockRect,
										pMBInfoHdr->McVector, jbEnv);
							}
						}  /* (pMBInfoHdr->MbType != TYPE_MV_I)  */
					} /* end of for (iMB) */
				} /* end of for (iTile) */	
			}  /* !PIC_TYPE_K */

			/* Calculate the error by summing the errors used for the MB type
			   decision. */

			dTotalMCErr = CalcTotalMCErr(pBsBandInfo->NumTiles, pBsBandInfo->Tiles);

				/* Verify the error by taking the error between the motion compensated
 				   image and the original image. */

			dTotalMCErr2 = ErrorVerify(mPicOrig, pBandContext->PicMc,
									   pBsBandInfo->NumTiles, pBsBandInfo->Tiles,
									   pBandContext->ErrMeasure, jbEnv);

			if (dTotalMCErr != dTotalMCErr2) {
				longjmp(jbEnv,  (ENNTRYNS << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BAD_PARM << TYPE_OFFSET));
			}
			
			HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);
	#ifdef SIMULATOR /* /DUMP_SAD simulator option */
			/* This option prints out the sum of the absolute value of the 
			 * differences between the motion compensated image and the original image.
			 */
			EncDumpSAD(pBandContext->pSimInst,pBandContext->PicMc,mPicOrig);
	#endif /* SIMULATOR, /DUMP_SAD */


	#ifdef SIMULATOR /* option /BANDENGY: collect band energy distribution after motion compensation */
			if (pBandHdr->ColorPlane==0) {
				EncSimBandEnergy((I32)(pBandHdr->BandId), &mPicOrig, &(pBandContext->PicMc), pBandContext->pSimInst,0, FALSE);
			}
	#endif /* SIMULATOR */
		} /* end if encode mode is mode_all or mode_test_water */

		/* Copy over quant values when Quant Inheritance is enabled */
		HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);
		if (u8EncodeMode!=ENC_MODE_FINAL) {
			if (pBandContext->QuantUseBand0) {
				pTile = pBsBandInfo->Tiles;
				pB0Tile = pBandContext->Band0Info->Tiles;

				for (iTile = 0; iTile < pBsBandInfo->NumTiles;
						iTile++, pTile++, pB0Tile++) {
					pMBInfo = pTile->MacBlocks;
					pB0MBInfo = pB0Tile->MacBlocks;
					for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++, pB0MBInfo++) {
						pMBInfo->MacHdr.QuantDelta= pB0MBInfo->MacHdr.QuantDelta;
					}
				}
			}
		}

		/* encode residual */
		EncSeg(pBsBandInfo, 
				   pBsBandInfo->Tiles, 
				   pBandContext->EncSegCntx, 
				   mPicOrig, 
				   pBandContext->PicMc,
				   pBandContext->PicXfrmRes,
				   mPicX, 
				   pBsBandInfo->NumTiles,
				   pBandContext->Band0Info->BandHdr.GlobalQuant,
				   u8EncodeMode,
				   jbEnv);

			/* Add the motion compensation to the reconstructed diff image.  If a 
			   tile is empty then the tile region will be zero in both mPicX and
			   PicMcCopy. */
		if ( (u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL)) {
			MatAddFull(mPicX, pBandContext->PicMc);
		}
	} else { /* Empty band so set the reconstructed band to zero */
		MatSetFull(mPicX, &i16Zero, jbEnv);
	}
			
	if ((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL)) {
		/*
		 * Calculate the motion compensated block checksums and save them in
		 * the TileHdr
		 */
		if (pcPicInfoHdr->GOPHdr.IsExpBS) {
			CalcMcBlkChkSum(pBsBandInfo, mPicX, jbEnv);	 
		}
	    /* if enc mode is all or final, update the references */
		/* if pic !disposable, save ref pictures for future */
		/* P frames predict from buffer 1, P2 and D frames */
		/* predict from buffer 0 */
		if (pcPicInfoHdr->PictureType == PIC_TYPE_K ||
			pcPicInfoHdr->PictureType == PIC_TYPE_P) {
			HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);
			/* K, I and P frames fill both buffer 0 and 1 */
			MatCopyFull(mPicX, pBandContext->PicRef[0], jbEnv);
			MatCopyFull(pBandContext->PicRef[0], 
						pBandContext->PicRef[1], jbEnv);
			pBandContext->PicRefNum[0] = pcPicInfoHdr->PictureNum;
			pBandContext->PicRefNum[1] = pcPicInfoHdr->PictureNum;
		} else if (pcPicInfoHdr->PictureType == PIC_TYPE_P2) {
				  /* P2 Frames only fill buffer 0 */
				  HiveEncodeProgressFunc(pInst, PCT_YIELD, 0);
				  MatCopyFull(mPicX, pBandContext->PicRef[0], jbEnv);
				  pBandContext->PicRefNum[0] = pcPicInfoHdr->PictureNum;
		}

		pBsBandInfo->BandHdr.Checksum = MatCksum(mPicX);

	} /* end if encode mode all or final */	  else {
		pBsBandInfo->BandHdr.Checksum = 0;
	}
}

/* --------------  PRIVATE ROUTINES  ----------------*/
/* Loops through all of the macro blocks in the band calculating the intra
   error value.
*/
static void	CalcBandIntraErrors(
	MatrixSt mPicOrig,		/* orig pic */
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTileInfo,		/* Pointer to tile info */
	I32 iErrMeasure)			/* Designates MADM or MSE */
{
	pMeBlockSt pMEInfo;
	I32 iTile, iMB;			/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	
	/* Loop through the tiles and calculate the intra error */
	pTile = pTileInfo;
	for(iTile = 0; iTile < iNumTiles; iTile++, pTile++) {
		pMBInfo = pTile->MacBlocks;
		for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
			pMEInfo =  &pMBInfo->pMEInfo[TYPE_MV_I];
			pMEInfo->Error = CalcIntraErr(mPicOrig, pMBInfo->MacBlockRect,
				iErrMeasure);
		}
	}
}

/* Loops through all of the macro blocks in the band calculating the total
   average error for the chosen MB types.
*/
static Dbl CalcTotalMCErr(
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTileInfo)		/* Pointer to tile info */
{
	pMeBlockSt pMEInfo;
	I32 iTile, iMB;			/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	Dbl dTotalErr = 0.0;
	I32 iTotalNumMB = 0;
	
	/* Loop through the tiles and calculate the intra error */
	pTile = pTileInfo;
	for (iTile = 0; iTile < iNumTiles; iTile++, pTile++) {
		pMBInfo = pTile->MacBlocks;
		for (iMB = 0; 
			iMB < pTile->NumMacBlocks; 
			iMB++, pMBInfo++, iTotalNumMB++) {

			pMEInfo =  &pMBInfo->pMEInfo[pMBInfo->MacHdr.MbType];
			dTotalErr += pMEInfo->Error;
		}
	}
	dTotalErr = dTotalErr / (Dbl) iTotalNumMB;

	return dTotalErr;

}

/* Loops through all of the macro blocks in the band calculating the total
   average error for the chosen MB types.
*/
static Dbl ErrorVerify(
	MatrixSt mPicOrig,		/* Original picture */
	MatrixSt mComp,			/* Motion compensated picture */
	I32 iNumTiles,			/* Number of tiles */
	pTileSt pTileInfo,		/* Pointer to tile info */
	I32 iErrMeasure,		/* Error measure to use */
	jmp_buf jbEnv)
{
	I32 iTile, iMB;			/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	Dbl dTotalErr = 0.0;
	I32 iTotalNumMB = 0;
	volatile Dbl dThisErr;
	McVectorSt vZero = {0,0};	/* Zero vector for error calculations */
	
	/* Loop through the tiles and calculate the intra error */
	pTile = pTileInfo;
	for(iTile = 0; iTile < iNumTiles; iTile++, pTile++) {

		pMBInfo = pTile->MacBlocks;
		for (iMB = 0; 
			 iMB < pTile->NumMacBlocks; 
			 iMB++, pMBInfo++, iTotalNumMB++) {
			if (pMBInfo->MacHdr.MbType == TYPE_MV_I) {
				dThisErr = CalcIntraErr(mPicOrig, pMBInfo->MacBlockRect,
					iErrMeasure);
			} else {
				dThisErr = EncMeRectErr(&mComp, &mPicOrig, 
										pMBInfo->MacBlockRect,
										iErrMeasure, vZero);
			}
			/* Verify that all of the errors are the same */
			if (dThisErr != pMBInfo->pMEInfo[pMBInfo->MacHdr.MbType].Error)
				longjmp(jbEnv,  (ENNTRYNS << FILE_OFFSET) |
					(__LINE__ << LINE_OFFSET) |
					(ERR_BAD_PARM << TYPE_OFFSET));


			dTotalErr += dThisErr;
		}
	}
	dTotalErr = dTotalErr / (Dbl) iTotalNumMB;

	return dTotalErr;

}

/* Calculates all of the errors for MB type classification.  It may be better 
   to do averaging during motion estimation and find the forward and backward 
   vectors which give you the best results when their compensated values are
   averaged.
*/
static void	CalcMotionErrors(
	I32 iPictureType,	/* Type of the picture */	
	MatrixSt mRef[2],	/* Forward reference */
	MatrixSt mPicOrig,	/* Target image */
	I32 iNumTiles,		/* Number of tiles */
	pTileSt pTileInfo,	/* Array of tiles in info structures */
	I32 iErrMeasure,	/* Error measure to use */
	PIA_Boolean bMeErrAvailable)  /* Whether or not the Motion Estimation Error is already 
							   * available. If TRUE, avoid the recalculation.
							   */
{
	I32 iTile, iMB;			/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	McVectorSt vZero;		/* Zero vector for error calculations */
	RectSt rMBRect;

	vZero.r = vZero.c = 0;	
	/* Loop through the tiles and calculate the various errors */
	pTile = pTileInfo;
	for(iTile = 0; iTile < iNumTiles; iTile++, pTile++) {
		pMBInfo = pTile->MacBlocks;
		for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
			rMBRect = pMBInfo->MacBlockRect;

			/* Calculate the Intra error for all cases */
			pMBInfo->pMEInfo[TYPE_MV_I].Error = 
				CalcIntraErr(mPicOrig, rMBRect, iErrMeasure);
							
			if (iPictureType != PIC_TYPE_K) {
						/* P Frames predict from buffer 1, D and P2 from */
						/* buffer 0 */
						if (iPictureType == PIC_TYPE_P) {
							if (!bMeErrAvailable)  {
								pMBInfo->pMEInfo[TYPE_MV_FW].Error = EncMeRectErr(&mRef[1],
									&mPicOrig, rMBRect, iErrMeasure,
									pMBInfo->pMEInfo[TYPE_MV_FW].Vect);
							}
	
							pMBInfo->pMEInfo[TYPE_MV_FW0].Error = EncMeRectErr(&mRef[1],
								&mPicOrig, rMBRect, iErrMeasure, vZero);
						} else { /* D and P2 Frames */
							if (!bMeErrAvailable)  {
								pMBInfo->pMEInfo[TYPE_MV_FW].Error = EncMeRectErr(&mRef[0],
									&mPicOrig, rMBRect, iErrMeasure,
									pMBInfo->pMEInfo[TYPE_MV_FW].Vect);
							}

							pMBInfo->pMEInfo[TYPE_MV_FW0].Error = EncMeRectErr(&mRef[0],
								&mPicOrig, rMBRect, iErrMeasure, vZero);
						}

			}
		}
	}
}

#ifdef SIMULATOR /* Force forward motion estimation */
/* This function will force all macroblocks to be forward predicted.
 * This function is useful for evaluating new simulation options since
 * it removes the effect of the macroblock classification.  If the 
 * macroblock classification is far out of tune for one option, the 
 * comparison may not be fair.  This function may make the comparison
 * more useful since both options will use the same macroblock types.
 * This function is not intended for use in released codecs.
 */
static void	EncSimForceForwardMotionEstimation(
	I32 iNumTiles,		/* Number of tiles */
	pTileSt pTileInfo)
{
	pMeBlockSt pMEInfo;
	I32 iTile, iMB;		/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	I32	iBestType;

	/* Loop through the tiles and calculate the intra error */
	pTile = pTileInfo;
	for (iTile = 0; iTile < iNumTiles; iTile++, pTile++) {
		pMBInfo = pTile->MacBlocks;
		for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
			pMEInfo =  pMBInfo->pMEInfo;

			/* Force forward motion estimation */
			iBestType = TYPE_MV_FW;
			pMBInfo->MacHdr.McVector = pMEInfo[iBestType].Vect;
			pMBInfo->MacHdr.MbType = (U8) iBestType;

		}
	}
}
#endif /* SIMULATOR, Force forward motion estimation */

#ifdef SIMULATOR /* Dump motion type classification */
/* This function will dump the results of the macroblock classification.
 * As with all the statistic dumping options, this function is
 * not intended for use in released codecs.
 */
static void	EncSimDumpMotionTypeClassification(
	I32 iNumTiles,		/* Number of tiles */
	pTileSt pTileInfo)
{
	pMeBlockSt pMEInfo;
	I32 iTile, iMB;		/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	/* Counter variables, initialize */
	U32 uCountTotal = 0, 
		uCountI = 0, 
		uCountFW = 0, 
		uCountFW0 = 0;

	/* Loop through the tiles and increment the counters */
	pTile = pTileInfo;
	for (iTile = 0; iTile < iNumTiles; iTile++, pTile++) {
		pMBInfo = pTile->MacBlocks;
		for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
			pMEInfo =  pMBInfo->pMEInfo;

			/* Increment counters */
			uCountTotal++;
			switch(pMBInfo->MacHdr.MbType) {
			case TYPE_MV_I:
				uCountI++;
				break;
			case TYPE_MV_FW:
				uCountFW++;
				break;
			case TYPE_MV_FW0:
				uCountFW0++;
				break;
			default: /* should not happen */
				break;
			}
		}
	}
	/* Print output */
	SimPrintf(pTileInfo->pSimInst->pSFile, pTileInfo->pSimInst->pSFile->uPrintFlags, 
		"MOTION_TYPE_CLASS:","Total %5d Intra %5d Forward %5d ForwardZero %5d\n",
		uCountTotal, uCountI, uCountFW, uCountFW0);

}
#endif /* SIMULATOR, Dump motion type classifcation */


static void	MBTypeClass(
	Dbl dIntraThresh, 	/* Threshold for Intra/Inter decision */
	Dbl dFW0Thresh,		/* Threshold for FW0 decision */
	Dbl dFWLimit,	    /* Smallest per pixel error that will code with MV */
	I32 iNumTiles,		/* Number of tiles */
	pTileSt pTileInfo)
{
	pMeBlockSt pMEInfo;
	I32 iTile, iMB;		/* Indices to loop through tiles and MB's */
	pTileSt pTile;
	pMacBlockSt pMBInfo;
	Dbl dMinError;	/* Minimum Error */
	I32	iBestType;

	/* Loop through the tiles and calculate the intra error */
	pTile = pTileInfo;
	for (iTile = 0; iTile < iNumTiles; iTile++, pTile++) {
		pMBInfo = pTile->MacBlocks;
		for (iMB = 0; iMB < pTile->NumMacBlocks; iMB++, pMBInfo++) {
			pMEInfo =  pMBInfo->pMEInfo;
			/* Set the minimum error to forward to start with for all motion 
			   predicted frames.
			*/

			dMinError = pMEInfo[TYPE_MV_FW0].Error;
			iBestType = TYPE_MV_FW0;


			if ((dMinError - pMEInfo[TYPE_MV_FW].Error > dFWLimit) &&
				 (pMEInfo[TYPE_MV_FW].Error < dMinError * dFW0Thresh)) {
				dMinError = pMEInfo[TYPE_MV_FW].Error;
				iBestType = TYPE_MV_FW;
			}

			/* Compare the best inter error with the intra error */
			if (pMEInfo[TYPE_MV_I].Error < dIntraThresh * dMinError) {
				pMBInfo->MacHdr.McVector.r = pMBInfo->MacHdr.McVector.c = 0;
				pMBInfo->MacHdr.MbType = TYPE_MV_I;
			} else {

				/* Copy the motion vectors to the info structures */
				if (iBestType == TYPE_MV_FW0) {
					iBestType = TYPE_MV_FW;
					pMBInfo->MacHdr.McVector.r = 0;
					pMBInfo->MacHdr.McVector.c = 0;
					pMEInfo[TYPE_MV_FW].Vect.r = 0;
					pMEInfo[TYPE_MV_FW].Vect.c = 0;
					pMEInfo[TYPE_MV_FW].Error = pMEInfo[TYPE_MV_FW0].Error;
				}
				else {
					pMBInfo->MacHdr.McVector = pMEInfo[iBestType].Vect;
				}

				pMBInfo->MacHdr.MbType = (U8) iBestType;
			}
		}
	}
}

/* Determine the tile size and number for the band given picture and
 * band sizes.
 */

static void SetTileInfo(I32 iPicTileWidth,
						I32 iPicTileHeight,
						I32 iPicWidth,
						I32 iPicHeight,
						I32 iBandWidth,
						I32 iBandHeight,
						PI32 piTileWidth,
						PI32 piTileHeight,
						PI32 piNumTiles)
{
	I32 iNumWTiles;		/* number of tiles across width */
	I32 iNumHTiles;		/* number of tiles along height */
	I32	iFactor;		/* number to divide UserTileSize by for subDivs */

	/*
	 * Check for special user value
	 */	
	if (iPicTileWidth == 0) {
		/*
		 * 0 => use the corresponding dimension of the band for the tile
		 */
		iPicTileWidth = iBandWidth;
	} else {
		/*
		 * Scale the TileSize to reflect the breaking the picture into
		 * bands.  The ration of picture size to band size can be used
		 * to compute the effect of the subdivision splitting done by
		 * openBands.
		 */ 
		iFactor = iPicWidth / iBandWidth;
		iPicTileWidth /= iFactor;

		/*
		 * Ensure that the tile size is not greater than the band size
		 */
		iPicTileWidth = MIN(iPicTileWidth, iBandWidth); 
	}

	/* 
	 * Repeat for the other dimension (without the commentary)
	 */
	if (iPicTileHeight == 0) {
		iPicTileHeight = iBandHeight;
	} else {
		iFactor = iPicHeight / iBandHeight;
		iPicTileHeight /= iFactor;
		iPicTileHeight = MIN(iPicTileHeight, iBandHeight); 
	}

	/*
	 * Save the tile size in the band info structure
	 */	
	*piTileWidth = iPicTileWidth;
	*piTileHeight = iPicTileHeight;

	/*
	 * Compute how many tiles can go across and down the band.
	 * Round up to nearest whole tile
	 * Save the tile size in the band info structure
	 */
	iNumWTiles = (iBandWidth + (iPicTileWidth - 1)) / iPicTileWidth;
	iNumHTiles = (iBandHeight + (iPicTileHeight - 1)) / iPicTileHeight;
	*piNumTiles = iNumWTiles * iNumHTiles;
	return;
}

