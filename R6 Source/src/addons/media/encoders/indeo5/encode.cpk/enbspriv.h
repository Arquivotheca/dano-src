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

/*
 * enbspriv.h
 *
 * Private declarations for enbs.c
 *
 * Dependencies:
 * 		datatype.h
 *		hufftbls.h
 *		bsutil.h
 *
 */

#ifndef __ENBSPRIV_H__
#define __ENBSPRIV_H__

#define LOG2(x) (log10(x) / log10(2.0)) /* Log to the base 2 */
#define MAX_EOB_CODE 42

const I32 iRVMapChngCost = 55000;
const I32 iMBPBCost = 12000;
const I32 iBlkPBCost = 105000;
const I32 iMax_I32 = 2147483647;

static void EncodeBitBuffs(PPicBSInfoSt pPicInfo,U8 u8EncodeMode, jmp_buf jbEnv);
static void CreatePicHdr(PPicBSInfoSt pBsPicInfo, jmp_buf jbEnv);
static void CreateGOPHdr(PPicBSInfoSt pBsPicInfo, jmp_buf jbEnv);
static void CreateBandHdr(pBandSt pBsBandInfo, PIA_Boolean UseBandDataSize, jmp_buf jbEnv);
static void 
CreateTransBandHdr(PTransSt pTransBand, PIA_Boolean UseBandDataSize, jmp_buf jbEnv); 

/* Encode the transparency band information */
static void TransTile2BitBuff(PTransTileSt pBsTransTileInfo, I32 iNumTiles,
							  PHuffTblSt pTransHuffTbl, jmp_buf jbEnv);


static void Tile2BitBuff(pTileSt pBsTileInfo,
						 I32 iNumTiles,
						 U8 u8PictureType,
						 pBandDscrptSt pBandDscrptor,
						 pBandHdrSt pBandHdr,
						 PHuffTblSt pMBHuffTbl,
						 PHuffTblSt pBHuffTbl,
						 pRVMapTblSt pRVMapTbl,
						 PIA_Boolean bIsExpBS,
						 jmp_buf jbEnv);

/* Writes the run value codes in the block to the bit buffer.
*/
static void RVCodes2BitBuff(
        pBitBuffSt pBsDat,
        pBlockSt pBsInfoBlock,
        const pRVMapTblSt cpRVMapTbl, /* Run val mapping table */
		jmp_buf jbEnv);

/* given a run and a value it returns an RV code. */
static I16 RunVal2RVCode(
        U8 u8Run,
        I32 iValue,
        PCRVMapTblSt pcRVMapTbl); /* Run val mapping table */

static void ByteEncodeBlockBuffs(PPicBSInfoSt pPicInfo, 
								U8 u8EncodeMode,
								jmp_buf jbEnv);

static PIA_Boolean EmptyBandTest(pTileSt pBsTileInfo, I32 iNumTiles);
static PIA_Boolean EmptyFrameTest(PPicBSInfoSt pPicInfo);

/*
 * Function to write the Tile, macro block and block information to the
 * corresponding Bit buffers.
 */
static void BSFormatTiles (
        pTileSt BsTileInfo,
        I32 NumTiles,
        U8 PictureType,
		pBandDscrptSt pBandDscrptor,
        pBandHdrSt BandHdr,
        PI32 BsMBHdrHist,
        PI32 BsBlkDatHist,
        pRVMapTblSt pRVMapTbl,  /* Run val mapping table */
        PIA_Boolean bPicInheritQ,   /* Picture level inherit Q is on */
		jmp_buf jbEnv);

static void ChooseHuffmanTables(PPicBSInfoSt PicInfo, 
								U8 u8EncodeMode,
								jmp_buf jbEnv);

/* Chooses the best Huffman table  */
static void BestHuffTable(PHuffTblSt pBestTbl, 	 /* the best Huffman table is returned here */
						  PHuffMapTblSt pMapTbl, /* the best Huffman table is also returned here */
						  HuffTblSt HuffTbls[],   /* the static (0-6) tables as candidates */
						  PI32 piHist, 				   /* the histogram representing the source */
						  PCHuffTblSt pcDefTbl,		   /* the default table as one of the candidates */
						  I32 ForcedHuffTbl, 	/* indicate if any table is forced to use */
						  PHuffMapTblSt pLastMapTbl,   /* the table used for last frame is also one candidate */
						  I32 iPBSwitchCost, 		   /* the cost associated with switching tables */
						  pPicDRCSt pPicDRC, 
						  I32 iMinNumSym, 
						  I32 iEOBCode,
						  jmp_buf jbEnv);
/* Chooses Huffman table for transparency band */
static void
TransHuffTable( PHuffTblSt pBestTbl,
			    PHuffMapTblSt pHuffMapTbl,
				PBoo UseXOR,
				PBoo HuffTblUnchanged,
				PI32 piHist, 
				PI32 piXORHist, 
				I32 iEOBCode,
				jmp_buf jbEnv);

/* Using the histogram information passed in it loops through the possible RV
 * tables and chooses the best one.
 */
static void BestRVMapTbl(
	pRVMapTblSt pBestRVTbl,		/* Contains the best RV table */
	PU8 pu8RVMapTblNum,			/* Set to the number of the best RV table */
	RVMapTblSt RVMapTbls[],     /* Set of RV tables to choose from */
	I32 aiRVHist[MAX_NABSVAL_LEN][MAX_SYMBOLS],		/* RV histogram */
	pRVMapTblSt pDefRVTbl,		/* Default RV table */
	I32 iForcedRVTbl,			/* Forced RV table number */
	pPicDRCSt pPicDRC,			/* Picture level DRC values */
	pBandDRCCntxSt pBandDRCCntx,
	jmp_buf jbEnv); /* Last band context */

/* Calculate the RV histograms */

static void     CalcRVHist(PPicBSInfoSt pPicInfo, jmp_buf jbEnv);

/* Calculate the best RV change list */	
static void FindRVChangeList(PHuffTblSt pBestTbl,
							 PI32 piHist,
							 PRVChangeListSt pRVChgLst, 
							 pRVMapTblSt pRVMapTbl, 
							 jmp_buf jbEnv);

static I32 FindNextHuffGroup(I32 iFreqLeft, I32 iSymsLeft, PI32 piHist, PI32 piCum);
static PBHuffTblSt FindExplHuffTable(I32 iMaxNumSym, PI32 piHist, I32 iMinNumSym,
	I32 iEOBCode, I32 iMaxHuffBitLength, I32 iMaxPBGroups, jmp_buf jbEnv);
static I32 Bits2Encode(PCHuffTblSt pcHuffTbl, PI32 piHist);
static void CombineHistograms(PI32 piDest, PI32 piSrc);
static void FormatHuffman(pBitBuffSt pBuff, PHuffMapTblSt pMapTbl, jmp_buf jbEnv);
static void FormatTransHuffman(pBitBuffSt pBuff, PHuffMapTblSt pMapTbl, jmp_buf jbEnv);


/* Write escape sub sample to the bitstream */
static void WriteEscSubSamp(pBitBuffSt pBuff, U8 u8SubSample, jmp_buf jbEnv);

static void	CalcRVCHist(
	PI32 piRVCHist,					 /* Run val code histogram output */
	PCRVMapTblSt pcRvMapTbl,					 /* Current map table */
	I32	aiRVHist[MAX_NABSVAL_LEN][MAX_SYMBOLS]); /* RV Code Histogram */

static void ByteEncodeTransPlane(PPicBSInfoSt pPicInfo, 
								 jmp_buf jbEnv);

#endif /* __ENBSPRIV_H__ */
