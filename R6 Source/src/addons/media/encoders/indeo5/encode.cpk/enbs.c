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
 * enbs.c
 * 
 * Encode the bitstream.  Taking the structures containing the
 * compression information, choose the best huffman parameters,
 * then encode.
 *
 * Functions:
 * EncodeBitstream()
 */

#include <string.h>		/* memset and memcpy */
#include <math.h>		/* ceil and log10 */
#include <setjmp.h>
#include "datatype.h"
#include "pia_main.h"

#include "hufftbls.h"	/* for ivi5bs.h */
#include "qnttbls.h"
#include "mc.h"

#include "ivi5bs.h"		/* for bsutil.h */
#include "mbhuftbl.h"
#include "bkhuftbl.h"
#ifdef SIMULATOR
#include "decsmdef.h"
#include "encsmdef.h"
#endif
#include "bsutil.h"
#include "indeo5.h"
#ifdef CMD_LINE_ENC
#include "CmdLParm.h"  
#endif	/*CMD_LINE_ENC*/
#include "pia_enc.h"	


#include "const.h"
#include "enbs.h"
#include "errhand.h"
#include "common.h"
#include "ensyntax.h"
#include "enme.h"
#include "enmesrch.h"

#include "enbspriv.h"	/* Forward declarations, constants */

/* Encodes the bitstream through the sequence of steps shown. */
void EncodeBitstream(PPicBSInfoSt pPicInfo, 
					U8 u8EncodeMode, 
					jmp_buf jbEnv)
{
	if (u8EncodeMode!=ENC_MODE_FINAL) {
		pPicInfo->PicDRC.LastCycles = 0;
	}

	/* Unless this is a repeat frame, 
	 * Encode the transparency plane (if present),
	 * Encode the macroblock and block buffers, and
	 * select the best huffman tables
	 */

	if(pPicInfo->Hdr.PictureType != PIC_TYPE_R) {
		if (pPicInfo->Hdr.GOPHdr.Transparency 
			&& ((u8EncodeMode==ENC_MODE_TEST_WATER) || (u8EncodeMode==ENC_MODE_ALL)) )  {
			/* Transparency Band only needs to encode once */

			/* Update Transparancy Huffman Table Flag */
			/* The huffman table must be invalidated on */
			/* Key frames. */
			if (pPicInfo->Hdr.PictureType == PIC_TYPE_K) {
				pPicInfo->pTransBand->HuffTblUnchanged = FALSE;
			}

			ByteEncodeTransPlane(pPicInfo, jbEnv);
		}
		ByteEncodeBlockBuffs(pPicInfo, u8EncodeMode, jbEnv);

		if (EmptyFrameTest(pPicInfo)) {
			pPicInfo->Hdr.PictureType = PIC_TYPE_R;
		} else {
			ChooseHuffmanTables(pPicInfo, u8EncodeMode,  jbEnv);
		}
	}

	/* Encode all of the buffers.  This also does Huffman encoding of the
	   macro block and block buffers .
	*/	
	EncodeBitBuffs(pPicInfo,u8EncodeMode, jbEnv);

	/* Update Transparancy Huffman Table Flag */
	/* Only do this on the final encode of this */
	/* frame, or it will be incorrect for the */
	/* final encode. */
	if (((pPicInfo->Hdr.PictureType == PIC_TYPE_K) ||
		(pPicInfo->Hdr.PictureType == PIC_TYPE_P)) &&
		(pPicInfo->Hdr.GOPHdr.Transparency) &&
		((u8EncodeMode==ENC_MODE_ALL) || (u8EncodeMode==ENC_MODE_FINAL) ))
		pPicInfo->pTransBand->HuffTblUnchanged = TRUE;


} /* EncodeBitstream() */


/*
 *      Creates a histogram of the statistics for Huffman encoding decisions.
 */
static void ByteEncodeTransPlane(PPicBSInfoSt pPicInfo, 
								 jmp_buf jbEnv)
{
	I32 i;
	I32 iTile;
    pPicHdrSt  pPicInfoHdr = &pPicInfo->Hdr;
	PTransTileSt pTransTile;

	/* Set the histogram values to 0 */
    memset(pPicInfo->pTransBand->BsTransDatHist, 0,
            sizeof(pPicInfo->pTransBand->BsTransDatHist[0]) * MAX_SYMBOLS);

    memset(pPicInfo->pTransBand->BsTransXORDatHist, 0,
            sizeof(pPicInfo->pTransBand->BsTransDatHist[0]) * MAX_SYMBOLS);

	/* Now accumulate the histograms for each tile */
	pTransTile = pPicInfo->pTransBand->Tiles;
	for (iTile = 0; iTile < pPicInfo->pTransBand->NumTiles; iTile++) {
		for (i = 0; i < 256; i++) {
			pPicInfo->pTransBand->BsTransDatHist[i] += pTransTile->RunHistogram[i];
		}
		pTransTile++;
	}

	/* Now accumulate the histograms for each tile */
	pTransTile = pPicInfo->pTransBand->XORTiles;
	for (iTile = 0; iTile < pPicInfo->pTransBand->NumTiles; iTile++) {
		for (i = 0; i < 256; i++) {
			pPicInfo->pTransBand->BsTransXORDatHist[i] += pTransTile->RunHistogram[i];
		}
		pTransTile++;
	}

} /* ByteEncodeTransPlane() */

/*
 *      Encodes the macro block header buffers and block data buffers in bytes
 *      to the corresponding byte buffers.  It also creates histograms of the
 *      statistics for Huffman encoding decisions.
 */
static void ByteEncodeBlockBuffs(PPicBSInfoSt pPicInfo, 
								 U8 u8EncodeMode, 
								 jmp_buf jbEnv)
{
	pPlaneSt   pPlane;
	pBandSt    pBand;
	pPicHdrSt  pPicInfoHdr = &pPicInfo->Hdr;
 	pBandHdrSt pBandHdr;
	I32 iPlane, iBand;

	pPlane = pPicInfo->Planes;

	if (u8EncodeMode!=ENC_MODE_FINAL)	{
		  /* Find the RV histograms */
		CalcRVHist(pPicInfo, jbEnv); 

		/* Set the picture level MB header histogram values to 0
		 * It gets accumulated later on from all the band level histogram 
		 */
		memset(pPicInfo->HuffInfo.BsMBHdrHist, 0,
			   sizeof(pPicInfo->HuffInfo.BsMBHdrHist[0]) * MAX_SYMBOLS);
	}

    for (iPlane = 0; iPlane < pPicInfo->NumPlanes; iPlane++, pPlane++) {
       	pBand = pPlane->Bands;
       	for(iBand = 0; iBand < pPlane->NumBands; iBand++, pBand++) {
       		pBandHdr = &pBand->BandHdr;

			/* Choose the best RV map table at the band level */
			if (u8EncodeMode!=ENC_MODE_FINAL)	{
				BestRVMapTbl(&pBand->HuffInfo.RVMap,   /* return: table itself */
					&pBandHdr->RVMapTblNum,   /* return: Table number (index) */
					&pPicInfo->HuffInfo.RVMapTbls[0], /* input: (candiate A) static table arrry */
					pBand->HuffInfo.RVHist,   /* input: histogram (representing the source) */
					&pPicInfo->HuffInfo.DefRVMapTbl,		/* input: (candiate B) default table */
					pBand->HuffInfo.FRVTbl,	 /* input: forced table ? */
					&pPicInfo->PicDRC, 
					&pBand->BandDRCCntx[PicDRCType(pPicInfoHdr->PictureType,jbEnv)],
					jbEnv);
			}

			if ((u8EncodeMode==ENC_MODE_ALL) || (u8EncodeMode==ENC_MODE_FINAL)) {
				pBand->BandDRCCntx[PicDRCType(pPicInfoHdr->PictureType,jbEnv)].RVMapTblNum 
					= (U8) pBandHdr->RVMapTblNum;
			}

            /*  Byte encode all the tiles in this band : using the RunVal coding 
			 */
			if (u8EncodeMode!=ENC_MODE_FINAL)	{
				BSFormatTiles(pBand->Tiles,
							  pBand->NumTiles,
							  (U8) pPicInfoHdr->PictureType,
							  pBand->pBandDscrptor,
							  pBandHdr,
							  pBand->HuffInfo.BsMBHdrHist, /* output (by product): band histogram */
							  pBand->HuffInfo.BsBlkDatHist, /* output (by product): band histogram */
							  &pBand->HuffInfo.RVMap,	/* using the best table found before */
							  pPicInfoHdr->PicInheritQ,
							  jbEnv);

				/* determine if this band is empty */
				pBandHdr->IsEmpty = EmptyBandTest(pBand->Tiles, pBand->NumTiles);
			}

			if (u8EncodeMode!=ENC_MODE_FINAL)	{
				 /*  Add band histograms to picture histograms for global stats of MB header */
				CombineHistograms(pPicInfo->HuffInfo.BsMBHdrHist,
								  pBand->HuffInfo.BsMBHdrHist);
			}
        } /* band */
    } /* plane */
} /* ByteEncodeBlockBuffs() */

/* Uses the statistics gathered into the histogram arrays to choose the
 * best huffman codebook to use.
 */
static void ChooseHuffmanTables(PPicBSInfoSt pPicInfo, 
								U8 u8EncodeMode,
								jmp_buf jbEnv)
{
    pPlaneSt 	pPlane;
    pBandSt 	pBand;
    pPicHdrSt 	pPicInfoHdr = &pPicInfo->Hdr;
    I32 iPlane, iBand;
	I32 iPBSwitchCost;

	if (u8EncodeMode!=ENC_MODE_FINAL) {
		/* Find the best huffman table for the MB header at the picture level */
		iPBSwitchCost = iMBPBCost;
		
		BestHuffTable(&pPicInfo->HuffInfo.MBHuff, /* return the best Huffman table itself */
			&pPicInfo->Hdr.MBHuffTbl,	/* return the best table index */
			&pPicInfo->HuffInfo.MBHuffTbls[0], /* input (candidate A): static table array */
			pPicInfo->HuffInfo.BsMBHdrHist,	/* input (histogram): representing the source */ 
			&pPicInfo->HuffInfo.DefMBHuffTbl, /* input (candidate B): default table  */
			pPicInfo->HuffInfo.FHTMB, /* forced table or not? */
			&pPicInfo->PicDRCCntx[PicDRCType(pPicInfoHdr->PictureType, jbEnv)].MBHuffTbl, 
										/* input (candidate C): table for last frame */
			iPBSwitchCost, 
			&pPicInfo->PicDRC, 
			1, 0, jbEnv);
	}


	if ((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL))  {
		/* update the last used table for next frame */
	pPicInfo->PicDRCCntx[PicDRCType(pPicInfoHdr->PictureType, jbEnv)].MBHuffTbl.StaticTblNum
		= (U8) pPicInfo->Hdr.MBHuffTbl.StaticTblNum;
	if(pPicInfo->Hdr.MBHuffTbl.StaticTblNum == HuffTblEscape) {
		pPicInfo->PicDRCCntx[PicDRCType(pPicInfoHdr->PictureType, jbEnv)].MBHuffTbl.PBHuff 
			= pPicInfo->Hdr.MBHuffTbl.PBHuff;
	}
	}

		/* Find the best Huffman table for the transparency band */
	if (pPicInfo->Hdr.GOPHdr.Transparency 
		&& ((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_TEST_WATER)) ) {
		TransHuffTable(&pPicInfo->pTransBand->TransHuff,
						&pPicInfo->pTransBand->TransHdr.TransHuffMapTbl,
						(int*)&pPicInfo->pTransBand->TransHdr.UseXOR_RLE,
						(int*)&pPicInfo->pTransBand->HuffTblUnchanged,
						pPicInfo->pTransBand->BsTransDatHist, 
						pPicInfo->pTransBand->BsTransXORDatHist, 
						0, jbEnv);
	}

    pPlane = pPicInfo->Planes;
    for(iPlane = 0; iPlane < pPicInfo->NumPlanes; iPlane++, pPlane++) {
		pBand = pPlane->Bands;
        for(iBand = 0; iBand < pPlane->NumBands; iBand++, pBand++) {

			if (u8EncodeMode!=ENC_MODE_FINAL) {
				/* Choose the best Huffman table for the block data buffer */
				iPBSwitchCost = iBlkPBCost;

				/* Cost of switching Huffman tables depends on whether the
				   RV mapping table has been switched.
				 */
				if(pBand->BandDRCCntx[PicDRCType(pPicInfoHdr->PictureType, jbEnv)].RVMapTblChanged) {
					iPBSwitchCost -= iRVMapChngCost;
				}
				/* Find the best block Huffman table for this band */
				BestHuffTable(&pBand->HuffInfo.BlkHuff, /* return the best Huffman table itself */
					&pBand->BandHdr.BlkHuffTbl,    /* return the best table index */
					&pPicInfo->HuffInfo.BlkHuffTbls[0], 
					pBand->HuffInfo.BsBlkDatHist,
					&pPicInfo->HuffInfo.DefBlkHuffTbl, 
					pBand->HuffInfo.HTBlock,
					&pBand->BandDRCCntx[PicDRCType(pPicInfoHdr->PictureType, jbEnv)].BlkHuffTbl,
					iPBSwitchCost, 
					&pPicInfo->PicDRC, 
					MAX_SYMBOLS,
					MAX_EOB_CODE, jbEnv);
			}


			if ((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL))  {
				/* update the last used table for next frame */
				pBand->BandDRCCntx[PicDRCType(pPicInfoHdr->PictureType, jbEnv)].BlkHuffTbl.StaticTblNum
					= (U8) pBand->BandHdr.BlkHuffTbl.StaticTblNum;
				if(pBand->BandHdr.BlkHuffTbl.StaticTblNum == HuffTblEscape) {
					pBand->BandDRCCntx[PicDRCType(pPicInfoHdr->PictureType, jbEnv)].BlkHuffTbl.PBHuff 
						= pBand->BandHdr.BlkHuffTbl.PBHuff;
				}
			}

            /* Make a swap list of swaps in the Huffman table */
			if (u8EncodeMode!=ENC_MODE_FINAL) {
				if(pBand->HuffInfo.UseRVChangeList) {
					FindRVChangeList(&pBand->HuffInfo.BlkHuff,
						pBand->HuffInfo.BsBlkDatHist, 
						&pBand->BandHdr.RVSwapList,
						&pBand->HuffInfo.RVMap, jbEnv);
				}
				if ((pBand->HuffInfo.UseRVChangeList) && (pBand->BandHdr.RVSwapList.NumChanges>0)) {
						pBand->BandHdr.UseRVHuffMapChangeList = TRUE;
				}	 else {
						pBand->BandHdr.UseRVHuffMapChangeList = FALSE;
				}

				/* For decoder performance reasons the EOB huffman code is restricted to
				   less than MAX_BITS_FOR_EOB.  This restriction is accomplished by
				   choosing a combination of static Huffman tables, and RV mapping
				   tables which guarantees this.  The VLC algorithm also makes sure that
				   the code for EOB for the resulting Huffman table will be <= MAX_BITS_FOR_EOB
				   no matter what the RV mapping table is.
				 */
				 if (pBand->HuffInfo.BlkHuff.HuffSym
			 					[pBand->HuffInfo.RVMap.InvFreqOrdTbl[EOB]].Len
					 > MAX_BITS_FOR_EOB)
					longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_ERROR << TYPE_OFFSET));
			}
		} /* band */
	} /* plane */

} /* ChooseHuffmanTables */

/*
** TransHuffTable - build a Huffman table for the transparency band.
**/
static void
TransHuffTable( PHuffTblSt pBestTbl,
			    PHuffMapTblSt pHuffMapTbl,
				PBoo pUseXOR,
				PBoo pHuffTblUnchanged,
				PI32 piHist, 
				PI32 piXORHist, 
				I32 iEOBCode,
				jmp_buf jbEnv)
{
    I32 iThisBitLength = iMax_I32;    /* Current bit length */
	I32 iThisBitLengthXOR = iMax_I32; /* Current bit length XOR */
    I32 iLastBitLength = iMax_I32;    /* Bit length for last Hufftbl */
    I32 iLastBitLengthXOR = iMax_I32; /* Bit length for last Hufftbl XOR*/
	PBHuffTblSt		PBHuff;
	PBHuffTblSt		PBHuffXOR;

	HuffTblSt		CurrBestTbl;
	HuffTblSt		CurrBestTblXOR;

    /* Finds the PB Style descriptor */
	PBHuff = FindExplHuffTable(MAX_SYMBOLS, piHist, MAX_SYMBOLS, iEOBCode,
    				 MAX_TRANS_HUFF_BITLENGTH, MAX_TRANS_PB_GROUPS, jbEnv);

	PBHuffXOR = FindExplHuffTable(MAX_SYMBOLS, piXORHist, MAX_SYMBOLS, iEOBCode,
    				 MAX_TRANS_HUFF_BITLENGTH, MAX_TRANS_PB_GROUPS, jbEnv);


    /* Converts PB Style to generic Huffman */
    PB2Huff(&PBHuff, &CurrBestTbl);
    PB2Huff(&PBHuffXOR, &CurrBestTblXOR);

    /* Total bits to encode using the PB table including the added cost
       of specifying the PB Table. */
    iThisBitLength = Bits2Encode(&CurrBestTbl, piHist) + (PBHuff.NumGroups + 1) 
            	   * PB_Entry_Size;
    iThisBitLengthXOR = Bits2Encode(&CurrBestTblXOR, piXORHist) + (PBHuffXOR.NumGroups + 1) 
            	   * PB_Entry_Size;

	/* See if we can reuse the last huffman table */
	if (pBestTbl->NumSymbols != 0) {
	    /* Total bits to encode using the PB table including 
	    	the added cost of specifying the PB Table. */

	    iLastBitLength = Bits2Encode(pBestTbl, piHist); 

	    iLastBitLengthXOR = Bits2Encode(pBestTbl, piXORHist);

		if (!*pHuffTblUnchanged) { /* If the table has changed, it must be coded */ 
			iLastBitLength += (pHuffMapTbl->PBHuff.NumGroups + 1) * PB_Entry_Size;
			iLastBitLengthXOR += (pHuffMapTbl->PBHuff.NumGroups + 1) * PB_Entry_Size;
		}

		/* Depending on how XOR was used last frame */
		if (*pUseXOR) {
			/* If Non-XOR is enough better than both XOR amounts */
			if ((iThisBitLength + (iThisBitLength >> 4) < iThisBitLengthXOR) && 
				(iThisBitLength + (iThisBitLength >> 4) < iLastBitLengthXOR)) {
				/* Code with non-XOR RLE, and change huffman tables */
				pHuffMapTbl->PBHuff = PBHuff;
				*pBestTbl = CurrBestTbl;
				*pHuffTblUnchanged = FALSE;
				*pUseXOR = FALSE;
			} else {
				/* Otherwise code XOR RLE, and decide whether to change huff tables */
				if (iLastBitLengthXOR > (iThisBitLengthXOR + (iThisBitLengthXOR >> 5))) {
					/* Change Huffman tables */
					pHuffMapTbl->PBHuff = PBHuffXOR;
					*pBestTbl = CurrBestTblXOR;
					*pHuffTblUnchanged = FALSE;
				}
			}
		} else {
			/* If XOR is enough better than both non-XOR amounts */
			if ((iThisBitLengthXOR + (iThisBitLengthXOR >> 4) < iThisBitLength) && 
				(iThisBitLengthXOR + (iThisBitLengthXOR >> 4) < iLastBitLength)) {
				/* Code with XOR RLE, and change huffman tables */
				pHuffMapTbl->PBHuff = PBHuffXOR;
				*pBestTbl = CurrBestTblXOR;
				*pHuffTblUnchanged = FALSE;
				*pUseXOR = TRUE;
			} else {
				/* Otherwise code non-XOR RLE, and decide whether to change huff tables */
				if (iLastBitLength > (iThisBitLength + (iThisBitLength >> 5))) {
					/* Change Huffman tables */
					pHuffMapTbl->PBHuff = PBHuff;
					*pBestTbl = CurrBestTbl;
					*pHuffTblUnchanged = FALSE;
				}
			}
		}
	} else {
		/* Initialize Huffman tables the first time through */

		/* Choose XOR_RLE or regular RLE */
		if (iThisBitLength > iThisBitLengthXOR) {
			pHuffMapTbl->PBHuff = PBHuffXOR;
			pHuffMapTbl->StaticTblNum = HuffTblEscape;
			*pBestTbl = CurrBestTblXOR;
			*pUseXOR = TRUE;
		} else {
			pHuffMapTbl->PBHuff = PBHuff;
			pHuffMapTbl->StaticTblNum = HuffTblEscape;
			*pBestTbl = CurrBestTbl;
			*pUseXOR = FALSE;
		}
	}
} /* TransHuffTable */




/* Using the histogram information passed in it loops through the possible huffman
 * tables and calculates the number of bits needed to represent that histogram using each
 * table.  
 * Among the candidates to choose from are:
 *   * static tables (index # 0-6)
 *   * default static table 
 *   * the table used in the last frame for the same band: could be static, default, or PB style
 *   * the PB style table for this specific band (represented by the histogram here).
 *
 * From all these choices, tt then selects the best huffman table (i.e., the table which can
 * code the histogram with the minimum number of bits, with minimal impact on the decoder
 * performance (More specifically, the table used for last frame is favored if it can encode
 * the current histogram reasonably well, because switching tables between frames involves 
 * penalty for decoder performance.
 * 
 * The best table selected is passed back in pBestTbl and while and puts the corresponding choice in the pMapTbl
 * and the best table passed back in both pBestTbl and pMapTbl.
 */
static void BestHuffTable(PHuffTblSt pBestTbl, 	 /* the best Huffman table is returned here */
						  PHuffMapTblSt pMapTbl, /* the best Huffman table is also returned here */
						  HuffTblSt HuffTbls[HUFF_TBLS_PER_BAND],   /* the static (0-6) tables as candidates */
						  PI32 piHist, 				   /* the histogram representing the source */
						  PCHuffTblSt pcDefTbl,		   /* the default table as one of the candidates */
						  I32 ForcedHuffTbl, 	/* indicate if any table is forced to use */
						  PHuffMapTblSt pLastMapTbl,   /* the table used for last frame is also one candidate */
						  I32 iPBSwitchCost, 		   /* the cost associated with switching tables */
						  pPicDRCSt pPicDRC, 
				/*		  U8 u8EncodeMode, */
						  I32 iMinNumSym, 
						  I32 iEOBCode,
						  jmp_buf jbEnv)
{
    I32 i;                          /* Index */
    I32 iBestTblNum;                /* Number of the best huffman table */
    I32 iBestBitLength = iMax_I32;  /* The smallest bit length */
    I32 iThisBitLength = iMax_I32;  /* Current bit length */
	I32 iNumSymbols = 0;			/* The number of symbols in the histogram */
    PCHuffTblSt pcHuffTbl;      	/* Pointer to Huffman tables */
    PCHuffTblSt pcBestHuffTbl;  	/* The best huffman table to use */
	HuffTblSt TmpHuffTbl;			/* Temporary Huffman table */
	PBHuffTblSt PBHuffTbl;
	I32 iLastNumBits = iMax_I32;
	
	/* Find the largest symbol to be used to make certain that the Huffman
	   table being tested has an adaquate number of symbols. */
	for(i = 0; i < MAX_SYMBOLS; i++) {
		if(piHist[i] > 0) {
			iNumSymbols = i + 1;
		}
	}

	if(ForcedHuffTbl == -1) {
        for(i = 0; i < HUFF_TBLS_PER_BAND; i++) {
            pcHuffTbl = &HuffTbls[i];
			if(pcHuffTbl->NumSymbols < iNumSymbols) {
                    continue;
            }
            /* Calculate bits to encode this distribution with this Huffman table */
            iThisBitLength = Bits2Encode(pcHuffTbl, piHist);

			/* If this table is the same as the last one used then store the
			   number of bits to encode for later analysis.
			 */
			if (pLastMapTbl && (pLastMapTbl->StaticTblNum == i)) {
				iLastNumBits = iThisBitLength;
			}

            /* Keep track of the best choice */
            if(iThisBitLength < iBestBitLength) {
                iBestBitLength = iThisBitLength;
                pcBestHuffTbl = pcHuffTbl;
                iBestTblNum = i;
            }
        }

		if (pcDefTbl && (iNumSymbols <= pcDefTbl->NumSymbols)) {
            /* Calc bits to encode with the default Huffman table */
            iThisBitLength = Bits2Encode(pcDefTbl, piHist);

			if (pLastMapTbl && (pLastMapTbl->StaticTblNum == DefaultHuffTbl)) {
				iLastNumBits = iThisBitLength;  /* Store number of bits */
			}

            
            if (iThisBitLength < iBestBitLength) {	/* Best so far? */
                iBestBitLength = iThisBitLength;
                pcBestHuffTbl = pcDefTbl;
                iBestTblNum = DefaultHuffTbl;
            }
        }
	} else if (ForcedHuffTbl != HuffTblEscape) { /* Represents a forced table */
		pcBestHuffTbl = &HuffTbls[ForcedHuffTbl];
		if (pcBestHuffTbl->NumSymbols < iNumSymbols)
			longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));
        iBestTblNum = ForcedHuffTbl;
        iBestBitLength = 0;      /* To force this as the decision */
    }

    /* Finds the PB Style descriptor */
    PBHuffTbl = FindExplHuffTable(MAX_SYMBOLS, piHist, iMinNumSym, iEOBCode, 
    					MAX_HUFF_BITLENGTH, MAX_PB_GROUPS, jbEnv);

    /* Converts PB Style to generic Huffman */
    PB2Huff(&PBHuffTbl, pBestTbl);

    /* Total bits to encode using the PB table including the added cost
       of specifying the PB Table */
    iThisBitLength = Bits2Encode(pBestTbl, piHist) + (PBHuffTbl.NumGroups + 1) 
            	   * PB_Entry_Size;

	/* Decide if the escape table is the best to use so far. */
	if (iThisBitLength < iBestBitLength) {
		iBestTblNum = HuffTblEscape;
		iBestBitLength = iThisBitLength;
	}

	if (pLastMapTbl) {
		/* If the last table was an escape table then calculate the number
		   of bits to encode the current distribution given the last table.
		 */
		if(pLastMapTbl->StaticTblNum == HuffTblEscape){

			/* Converts PB Style to generic Huffman */
			PB2Huff(&pLastMapTbl->PBHuff, &TmpHuffTbl);

			if(iNumSymbols <= TmpHuffTbl.NumSymbols) {
				/* Total bits to encode using the PB table including the added cost
				   of specifying the PB Table. */
				iLastNumBits = Bits2Encode(&TmpHuffTbl, piHist) 
							 + (pLastMapTbl->PBHuff.NumGroups + 1)
							 * PB_Entry_Size;	
			} else {
				iLastNumBits = iMax_I32;  /* Ensure not use this table */
			}
		}
	}

	if (pLastMapTbl) {
		/* If the wasted bits by using the last chosen Huffman table is small 
		   enough as compared to the dDRC value or if the cycle budget is all
		   used up and there was a last table and the last table was valid then
		   use the last table.
		 */
		if((((iLastNumBits - iBestBitLength) < pPicDRC->dDRC) || 
			((pPicDRC->MaxBudget - pPicDRC->LastCycles) < iPBSwitchCost)) && 
			(pLastMapTbl->StaticTblNum != NoLastTbl) &&
			(iLastNumBits != iMax_I32)) {

			/* Best table is the table used for this band in 
			 * the last frame of this type. 
			 */
			iBestTblNum = pLastMapTbl->StaticTblNum;

			/* If the last table was PB style then copy over the PB table */
			if(iBestTblNum == HuffTblEscape) {
				*pBestTbl = TmpHuffTbl;
				PBHuffTbl = pLastMapTbl->PBHuff;
			} else if(iBestTblNum == DefaultHuffTbl) {
				/* The default table */
				pcBestHuffTbl = pcDefTbl;
			} else { /* The last table was a static table */
				pcBestHuffTbl = &HuffTbls[iBestTblNum];
			}
		} else {  /* Switch tables */
			pPicDRC->LastCycles += iPBSwitchCost; /* add cost of switching */
		}
	}

	if((iBestTblNum == HuffTblEscape) || (ForcedHuffTbl == HuffTblEscape)) {
		pMapTbl->StaticTblNum = (U8) HuffTblEscape;
		pMapTbl->PBHuff = PBHuffTbl;
	} else {
		/* If the PB Style descriptor was the best then pBestTbl already
			 contains this table */
		pMapTbl->StaticTblNum = (U8) iBestTblNum;
		*pBestTbl = *pcBestHuffTbl; /* Copy the best huffman table to pBestTbl */
	}

	/* Update the last table if there is a last context defined and this is
	   the last time the bitstream will be formated for this frame.
	 */
	/*
	if(pLastMapTbl != NULL && 
		((u8EncodeMode == ENC_MODE_ALL) || (u8EncodeMode == ENC_MODE_FINAL)) ) {
		pLastMapTbl->StaticTblNum = (U8) iBestTblNum;
		if(iBestTblNum == HuffTblEscape) {
			pLastMapTbl->PBHuff = PBHuffTbl;
		}
	}
	*/

	return;
} /* BestHuffTable() */


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
	jmp_buf jbEnv) /* Last band context */
{
	I32 i;							/* Index */
	I32 iBestTblNum;				/* Number of the best RV map table */
	I32 iThisBitLength;				/* Est. Bits used for current RV Table */
	I32 iMinBits = iMax_I32;		/* Smallest number of bits */
	PCRVMapTblSt pcMapTbl;			/* Pointer to RV map table */		
	PCRVMapTblSt pcBestRVMapTbl;	/* The best RV map table to use */
	I32 aiRVCHist[MAX_SYMBOLS];		/* Histogram of run val codes */
	I32 iNumEsc;
	I32 iLastNumBits;				/* Number of bits using the last table */
	PBHuffTblSt PBHuffTbl;			/* PB Huffman table used for RV table comparisons */
	HuffTblSt BestTbl;				/* Generic Huffman table used for RV table comparisons */


	if(iForcedRVTbl == -1) {
		for(i = 0; i < NUM_RV_MAP_TBLS; i++) {
			pcMapTbl = &RVMapTbls[i];

			if (pcMapTbl == NULL) {	/* next iteration if not defined */
				continue;
			}

			/* Initialize the histogram to zero */
			memset(aiRVCHist, 0, MAX_SYMBOLS * sizeof(I32));

			/* Find the histogram of the mapped values */
			CalcRVCHist(aiRVCHist, pcMapTbl, aiRVHist);


			/* Finds the PB Style descriptor */
			PBHuffTbl = FindExplHuffTable(MAX_SYMBOLS, aiRVCHist, MAX_SYMBOLS, MAX_EOB_CODE, 
    						   			  MAX_HUFF_BITLENGTH, MAX_PB_GROUPS, jbEnv);

			/* Converts PB Style to generic Huffman */
			PB2Huff(&PBHuffTbl, &BestTbl);

			/* Get the number of escapes */			
			iNumEsc = aiRVCHist[pcMapTbl->InvFreqOrdTbl[ESC]]; 

			/* Total bits to encode using the PB table including the added cost
			   of specifying the PB Table, and Escapes */
			iThisBitLength = Bits2Encode(&BestTbl, aiRVCHist) + (PBHuffTbl.NumGroups + 1) 
						     * PB_Entry_Size + 50 * iNumEsc;
			
			/* If this was the RV mapping table used last time then save the
			   last number of escapes to be the current.
			 */

			if  (i == (I32) pBandDRCCntx->RVMapTblNum) {
				iLastNumBits = iThisBitLength;
			}

			/* Keep track of the best choice.  The best choice in general is
			   the one which minimizes the number of bits;
			 */

			if (iThisBitLength < iMinBits) {
				iMinBits = iThisBitLength;
				pcBestRVMapTbl = pcMapTbl;
				iBestTblNum = i;
			}
		}

		if (pDefRVTbl) {
			memset(aiRVCHist, 0, MAX_SYMBOLS * sizeof(I32));

			/* Find the histogram of the mapped values */
			CalcRVCHist(aiRVCHist, pDefRVTbl, aiRVHist);

			/* Finds the PB Style descriptor */
			PBHuffTbl = FindExplHuffTable(MAX_SYMBOLS, aiRVCHist, MAX_SYMBOLS, MAX_EOB_CODE, 
    						   			  MAX_HUFF_BITLENGTH, MAX_PB_GROUPS, jbEnv);

			/* Converts PB Style to generic Huffman */
			PB2Huff(&PBHuffTbl, &BestTbl);

			/* Get the number of escapes */			
			iNumEsc = aiRVCHist[pDefRVTbl->InvFreqOrdTbl[ESC]]; 

			/* Total bits to encode using the PB table including the added cost
			   of specifying the PB Table, and Escapes */
			iThisBitLength = Bits2Encode(&BestTbl, aiRVCHist) + (PBHuffTbl.NumGroups + 1) 
						     * PB_Entry_Size + 50 * iNumEsc;

			/* If this was the RV mapping table used last time then save the
			   last number of escapes to be the current.
			 */
			if (pBandDRCCntx->RVMapTblNum == DefaultRvMapTbl) {
				iLastNumBits = iThisBitLength;
			}

			if (iThisBitLength < iMinBits) {
				iMinBits = iThisBitLength;
				pcBestRVMapTbl = pDefRVTbl;
				iBestTblNum = DefaultRvMapTbl;
			}

		}
		/* Convert the extra number of escapes given from using the last RV 
		   mapping table to an expected impact in bits and compare to the
		   current dDRC level.  This is done only if there was a previous table.  
		   Also, if the maximum cycle budget is already consumed then it is 
		   forced to pick the old table if there was one.
		 */
		if ((((iLastNumBits - iMinBits) < pPicDRC->dDRC) ||
			 ((pPicDRC->MaxBudget - pPicDRC->LastCycles) < iRVMapChngCost)) && 
			(pBandDRCCntx->RVMapTblNum != NoLastTbl)) {
			/* Set the best table to be the same as the last frame of this
			   frame type.
			 */
			iBestTblNum = pBandDRCCntx->RVMapTblNum;
			if (iBestTblNum < NUM_RV_MAP_TBLS) { /* One of the static set */
				pcBestRVMapTbl = &RVMapTbls[iBestTblNum];
			} else {  /* The default table */
				pcBestRVMapTbl = pDefRVTbl;
			}
			/* A PIA_Boolean used later to determine the cost of a PB switch.
			   Since if the PB table changed for this band then the RV
			   mapping table switch is for free.
			 */
			pBandDRCCntx->RVMapTblChanged = FALSE;
		} else { /* Choose the new RV mapping table */
			pPicDRC->LastCycles += iRVMapChngCost; /* Add cost to switch tables */
			pBandDRCCntx->RVMapTblChanged = TRUE;
		}
	} else {  /* An RV mapping table forced from the command line. */
		pcBestRVMapTbl = &RVMapTbls[iForcedRVTbl];
		iBestTblNum = iForcedRVTbl;
	}

	/* Set the RVMapTblNum to be the best one chosen */
	*pu8RVMapTblNum = (U8) iBestTblNum;

	/* Copy over the Best RV map table */
	*pBestRVTbl = *pcBestRVMapTbl;

	return;
} /* BestRVMapTbl() */


/*
 *  Calculates the number of bits needed to encode the buffer represented
 *  by the histogram with the given Huffman table.  Go over each symbol 
 *  and tally in the number of bits for that symbol: the number of that
 * 	type of symbol times the symbol's length.
 */
static I32 Bits2Encode(PCHuffTblSt pcHuffTbl, PI32 piHist)
{
	I32 iTotalBits = 0;
	I32 i;
	PCHuffSymbolSt pcHuffSym;

	pcHuffSym = pcHuffTbl->HuffSym;
	for (i = 0; i < pcHuffTbl->NumSymbols; i++, pcHuffSym++) {
		iTotalBits += piHist[i] * pcHuffSym->Len;
	}
	return iTotalBits;
} /* Bits2Encode */

/* Formats the bitstream at all levels.  The byte buffers previously created
   are Huffman encoded and writen to the Huffman encoded bit buffers.  The
   Huffman tables previously selected are used to do the Huffman encoding.
*/
static void EncodeBitBuffs(PPicBSInfoSt pPicInfo, U8 u8EncodeMode, jmp_buf jbEnv)
{
    pPlaneSt pPlane;
    pBandSt pBand;
    pTileSt pTile;
	PTransTileSt pTransTile;
    pPicHdrSt pPicInfoHdr = &pPicInfo->Hdr;
    pBandHdrSt pBandHdr;
    I32 iplane, iband, iTile;
	U32 uBandDataSize;
	I32 iTmp;
		
    CreatePicHdr(pPicInfo, jbEnv);
	/* If it is a repeated frame then all bit buffers are set to no 
	   data except for the start code.
	 */
	if (pPicInfo->Hdr.PictureType == PIC_TYPE_R) {
		pPlane = pPicInfo->Planes;
		for (iplane = 0; iplane < pPicInfo->NumPlanes; iplane++, pPlane++) {
			pBand = pPlane->Bands;
			for (iband = 0; iband < pPlane->NumBands; iband++, pBand++) {
				/* Format the Band Header */
				pBandHdr = &pBand->BandHdr;
				BitBuffInit(&(pBand->BsBandHdr), jbEnv);
				BitBuffFlush(&(pBand->BsBandHdr));

				pTile = pBand->Tiles;
				for (iTile = 0; iTile < pBand->NumTiles; iTile++, pTile++) {
					/* Sets bit buffer bytes written to zero and set for
					   reading. */
					BitBuffInit(&(pTile->BsTileHdr), jbEnv);
					BitBuffFlush(&(pTile->BsTileHdr));
					BitBuffInit(&(pTile->BsBlockDat), jbEnv);
					BitBuffFlush(&(pTile->BsBlockDat));
					BitBuffInit(&(pTile->BsMacHdr), jbEnv);
					BitBuffFlush(&(pTile->BsMacHdr));
				} /* Empty Tiles */

			} /* band */
		} /* plane */
		/* Now hit the transparency bitbuffs if transparency is on */
		if (pPicInfo->Hdr.GOPHdr.Transparency) {
			PTransSt pTransBand = pPicInfo->pTransBand;
			PTransTileSt pTransTile = pTransBand->Tiles;

			BitBuffInit(&(pTransBand->BsTransHdr), jbEnv);
			BitBuffFlush(&(pTransBand->BsTransHdr));
			
			for (iTile = 0; iTile < pTransBand->NumTiles; iTile++, pTransTile++) {
				BitBuffInit(&(pTransTile->BsTransTileHdr), jbEnv);
				BitBuffFlush(&(pTransTile->BsTransTileHdr));
				BitBuffInit(&(pTransTile->BsTransTileDat), jbEnv);
				BitBuffFlush(&(pTransTile->BsTransTileDat));
			}

			pTransTile = pTransBand->XORTiles;

			for (iTile = 0; iTile < pTransBand->NumTiles; iTile++, pTransTile++) {
				BitBuffInit(&(pTransTile->BsTransTileHdr), jbEnv);
				BitBuffFlush(&(pTransTile->BsTransTileHdr));
				BitBuffInit(&(pTransTile->BsTransTileDat), jbEnv);
				BitBuffFlush(&(pTransTile->BsTransTileDat));
			}
		}
	} else { /* Not a repeat frame */
        pPlane = pPicInfo->Planes;
        for (iplane = 0; iplane < pPicInfo->NumPlanes; iplane++, pPlane++) {
            pBand = pPlane->Bands;
            for (iband = 0; iband < pPlane->NumBands; iband++, pBand++) {
                /* Format the Band Header */
                pBandHdr = &pBand->BandHdr;
                CreateBandHdr(pBand, pPicInfo->Hdr.BandDataSizeFlag, jbEnv);

				if (!pBand->BandHdr.IsDropped) {
					/* at FINAL mode, only checksum	and clamping hints in header may be changed, 
					 * all the tile and below BS are already formated and don't need to encode
					 * again. But there is one exception : if the pPicInfo->Hdr.GOPHdr.IsExpBS 
					 * is set, the tile header includes the block checksums which are available
					 * only at the FINAL mode and hence it needs to encode the tile header again.
					 */
					if (pPicInfo->Hdr.GOPHdr.IsExpBS  ||
						( !pPicInfo->Hdr.GOPHdr.IsExpBS && u8EncodeMode!=ENC_MODE_FINAL)) {
				
#ifdef SIMULATOR /* dump block data sizes */
						pTile = pBand->Tiles;
						for (iTile = 0; iTile < pBand->NumTiles; iTile++, pTile++) {
							pTile->pSimInst = pPicInfo->pSimInst;
						}
#endif /* SIMULATOR, dump block data sizes */
						Tile2BitBuff(pBand->Tiles, pBand->NumTiles,
                            		(U8)pPicInfoHdr->PictureType,
									pBand->pBandDscrptor,
                            		pBandHdr, 
                            		&pPicInfo->HuffInfo.MBHuff,
                            		&pBand->HuffInfo.BlkHuff, 
                            		&pBand->HuffInfo.RVMap, 
									pPicInfo->Hdr.GOPHdr.IsExpBS,
									jbEnv);
					}

					/* Find out the band data size (including both header and tile data) */
					if (pPicInfo->Hdr.BandDataSizeFlag) {
						uBandDataSize = BitBuffSize(&(pBand->BsBandHdr));
						pTile = pBand->Tiles;
						for (iTile = 0; iTile < pBand->NumTiles; iTile++, pTile++) {
							uBandDataSize += BitBuffSize(&(pTile->BsTileHdr));
							uBandDataSize += BitBuffSize(&(pTile->BsMacHdr));
							uBandDataSize += BitBuffSize(&(pTile->BsBlockDat));
						}
						if (uBandDataSize>MaxBandDataSize) {					   
							uBandDataSize = 0;
						}
						/* Update in Band Header */

						pBand->BandHdr.BandDataSize = uBandDataSize;

						/* overwrite in the BS */
						iTmp = BitBuffRead(&(pBand->BsBandHdr), LenBandFlags, jbEnv); 
						BitBuffOverWrite(&(pBand->BsBandHdr), uBandDataSize, 
                					 LenBandDataSize, jbEnv);
						BitBuffFlush(&(pBand->BsBandHdr));
					}

                } else {
                    /* Reset all of the bitbuffers to zero showing no data */
                    pTile = pBand->Tiles;
                    for (iTile = 0; iTile < pBand->NumTiles; iTile++, pTile++) {
                            /* Sets bit buffer bytes written to zero and set for
                               reading. */
                            BitBuffInit(&(pTile->BsTileHdr), jbEnv);
                            BitBuffFlush(&(pTile->BsTileHdr));
                            BitBuffInit(&(pTile->BsBlockDat), jbEnv);
                            BitBuffFlush(&(pTile->BsBlockDat));
                            BitBuffInit(&(pTile->BsMacHdr), jbEnv);
                            BitBuffFlush(&(pTile->BsMacHdr));
                    } /* Empty Tiles */
                }
            } /* band */
        } /* plane */
		if (pPicInfo->Hdr.GOPHdr.Transparency
			&& ((u8EncodeMode==ENC_MODE_ALL) || (u8EncodeMode==ENC_MODE_TEST_WATER)) ) {
			/* Now handle the transparency band */
			CreateTransBandHdr(pPicInfo->pTransBand, pPicInfo->Hdr.BandDataSizeFlag, jbEnv);

			if (pPicInfo->pTransBand->TransHdr.UseXOR_RLE) {
				TransTile2BitBuff(pPicInfo->pTransBand->XORTiles, 
								  pPicInfo->pTransBand->NumTiles,
				 			      &pPicInfo->pTransBand->TransHuff, jbEnv);
			} else {
				TransTile2BitBuff(pPicInfo->pTransBand->Tiles, 
								  pPicInfo->pTransBand->NumTiles,
				 			      &pPicInfo->pTransBand->TransHuff, jbEnv);
			}

			/* Find out the Transparency band data size (including both header and tile data) */
			if (pPicInfo->Hdr.BandDataSizeFlag) {
				uBandDataSize = BitBuffSize(&(pPicInfo->pTransBand->BsTransHdr));
				if (pPicInfo->pTransBand->TransHdr.UseXOR_RLE) {
					pTransTile = pPicInfo->pTransBand->XORTiles;
				} else {
					pTransTile = pPicInfo->pTransBand->Tiles;
				}

				for (iTile = 0; iTile < pPicInfo->pTransBand->NumTiles; iTile++) {
					uBandDataSize += BitBuffSize(&(pTransTile[iTile].BsTransTileHdr));
					uBandDataSize += BitBuffSize(&(pTransTile[iTile].BsTransTileDat));
				}
				if (uBandDataSize>MaxTransDataSize) {
					uBandDataSize = 0;
				}
				/* overwrite in the BS */
				BitBuffOverWrite(&(pPicInfo->pTransBand->BsTransHdr),uBandDataSize, 
							LenTransDataSize, jbEnv);
				BitBuffFlush(&(pPicInfo->pTransBand->BsTransHdr));
			}
		}	
	} /* if PIC_TYPE_R */
} /* EncodedBitBuffs() */

static void CreateGOPHdr(PPicBSInfoSt pBsPicInfo, jmp_buf jbEnv)
{
	pGOPHdrSt pGOPHdr = &(pBsPicInfo->Hdr.GOPHdr);
	pBitBuffSt	pBuff = pBsPicInfo->BsGOPHdr;
	pBandDscrptSt pDescriptor;
	U8 u8GOPFlags=0;		 /* one byte GOPFlags */
	U8 u8TileSize;
	U8 u8MVRes;
	U8 u8BlkSize;
	U8 u8YDescriptorNum;
	U8 u8VUDescriptorNum;
	U8 u8Xfrm, u8DefXfrm, u8DefQuant;
    U32 i,k;                /* temp index */ 
	U32 uBandId;
    I32 iTmp;

    BitBuffInit(pBuff, jbEnv);

		/* One byte of GOPFlags, consisting of 8 bits of Boolean */
	if (pGOPHdr->GOPHdrSizeFlag) {
		u8GOPFlags = u8GOPFlags | GOPFLags_GOPHdrSizeFlag;
	}
	if (pGOPHdr->IsYVU12) {
		u8GOPFlags = u8GOPFlags | GOPFLags_IsYVU12;
	}
	if (pGOPHdr->IsExpBS) {
		u8GOPFlags = u8GOPFlags | GOPFLags_IsExpBS;
	}
	if (pGOPHdr->Transparency) {
		u8GOPFlags = u8GOPFlags | GOPFLags_Transparency;
	}
	if (pGOPHdr->SkipBitBand) {
		u8GOPFlags = u8GOPFlags | GOPFLags_SkipBitBand;
	}
	if (pGOPHdr->UseKeyFrameLock && pBsPicInfo->Hdr.UseChecksum) {	
		u8GOPFlags = u8GOPFlags | GOPFLags_UseKeyFrameLock;
	}  else {
		pGOPHdr->UseKeyFrameLock = FALSE;
	}

	if (pGOPHdr->Tiling) {
		u8GOPFlags = u8GOPFlags | GOPFLags_Tiling;
	}
	if (pGOPHdr->Padding) {
		u8GOPFlags = u8GOPFlags | GOPFLags_Padding;
	}
	BitBuffWrite(pBuff, (U32)u8GOPFlags, LenGOPFlags, jbEnv);

	if (pGOPHdr->GOPHdrSizeFlag) {

	/* Space is set aside here for the data size.  The actual value is written
  	 *	into this space after all of the bitstream formating is done.
	 */
		BitBuffWrite(pBuff, pGOPHdr->GOPHdrSize, LenGOPHdrSize, jbEnv);
	}
			
	if (pGOPHdr->UseKeyFrameLock && pBsPicInfo->Hdr.UseChecksum) {	
		U32 LockWord;

		/* Calculate lock value from the key */
		LockWord  = ((U32) pBsPicInfo->Hdr.PicChecksum) + (pGOPHdr->AccessKey & 0xff00L) + 1;
		LockWord *= (pGOPHdr->AccessKey >> 16) + ((pGOPHdr->AccessKey&0xffL)<<8) + 1;
		LockWord /= (LockWord % ((pGOPHdr->AccessKey & 0xffL) + 1)) + 1;
		LockWord *= (LockWord % (((U32) pBsPicInfo->Hdr.PicChecksum & 0xff00) + 1)) + 1;
		pGOPHdr->FrameLock = LockWord;
		BitBuffWrite(pBuff, pGOPHdr->FrameLock, LenFrameLock, jbEnv);
	}

	if (pGOPHdr->Tiling) {	/* Tile Size */
		if (pGOPHdr->TileWidth!=pGOPHdr->TileHeight) {
		/* Legal Tile size : squares only */
			u8TileSize = TileSizeResvd;
		}
		switch (pGOPHdr->TileWidth) {
			case 64:   u8TileSize= TileSize64;
				break;
			case 128:  u8TileSize = TileSize128;
				break;
			case 256:  u8TileSize = TileSize256;
				break;
			default:   u8TileSize = TileSizeResvd;
				break;
		}
		if (u8TileSize==TileSizeResvd) {
			goto error;
		} else {
			BitBuffWrite(pBuff, u8TileSize, LenTileSize, jbEnv);
		}
	}	/* end of if (Tiling) */

	/* Y and VU band decomposition levels */
	if (pGOPHdr->YDecompLevel>MaxYDecompLevels) { /* illegal */
		goto error;
	} else {
		BitBuffWrite(pBuff, pGOPHdr->YDecompLevel, LenYDecompLevel, jbEnv);
	}
	BitBuffWrite(pBuff, pGOPHdr->VUDecompLevel, LenVUDecompLevel, jbEnv);

    /* picture size */
    for (k=0; k < NumComPicSizes; k++) {
		if (pGOPHdr->PictureHeight == ComPicSizes[k].Height &&
            pGOPHdr->PictureWidth == ComPicSizes[k].Width) 	 {
				break;
		}
	}
    if (k == NumComPicSizes) {              /* escape */
        BitBuffWrite(pBuff, PicSizeEscCode, LenPicSize, jbEnv);      /* write escape */
        BitBuffWrite(pBuff, pGOPHdr->PictureHeight, LenExpPicDim, jbEnv);
        BitBuffWrite(pBuff, pGOPHdr->PictureWidth, LenExpPicDim, jbEnv);
    } else 	 {
     	BitBuffWrite(pBuff, k, LenPicSize, jbEnv);
	}

	/*  Band Descriptors */

	u8YDescriptorNum = (3*pGOPHdr->YDecompLevel+1) ;
	u8VUDescriptorNum = 3*pGOPHdr->VUDecompLevel+1;

	pDescriptor = pGOPHdr->pBandDscrpts;

	for (i=0;i<(U8)(u8YDescriptorNum+u8VUDescriptorNum);i++) {

		/* Write out MVRes field -- it's in 16ths of a pel */
		switch (pDescriptor->MVRes) {
			case INTEGER_PEL_RES:
				u8MVRes = MVResIntegral;
				break;
			case HALF_PEL_RES:
				u8MVRes = MVResHalf;
				break;
			default:
				goto error;
				break;
		}
		BitBuffWrite(pBuff, u8MVRes, LenMVRes, jbEnv);

		/* Write out the MacroBlockSize field */
		switch( pDescriptor->MacBlockSize ) {
			case 16:
				if (pDescriptor->BlockSize == 8) {
					u8BlkSize = BlockSize16_8;
				} else 	{
					goto error;
				}
				break;
			case  8:
				if (pDescriptor->BlockSize == 8) {
					u8BlkSize = BlockSize8_8;
				} else {
					if (pDescriptor->BlockSize == 4) {
						u8BlkSize = BlockSize8_4;
					} else {
						goto error;
					}
				}
				break;
			case  4:
				if (pDescriptor->BlockSize == 4) {
					u8BlkSize = BlockSize4_4;
				} else 	{
					goto error;
				}
				break;
			default:
				goto error;
				break;
		}
		BitBuffWrite(pBuff, u8BlkSize, LenBlockSize, jbEnv);

		/* write out the transform */
		if (i<u8YDescriptorNum) { /* For Y bands */
			uBandId = i;
			u8DefXfrm = Default_Y_Xfrm[uBandId];
			u8DefQuant = Default_Y_Quant[pGOPHdr->YDecompLevel][uBandId];
		} else { /* For V and U bands */
			uBandId = i - u8YDescriptorNum;
			if (pGOPHdr->IsYVU12) {
				u8DefXfrm = Default_VU12_Xfrm[uBandId];
				u8DefQuant = Default_VU12_Quant[pGOPHdr->VUDecompLevel][uBandId];
			}  else {
				u8DefXfrm = Default_VU9_Xfrm[uBandId];
				u8DefQuant = Default_VU9_Quant[pGOPHdr->VUDecompLevel][uBandId];
			}
		}

		if (pDescriptor->Xform == u8DefXfrm) { 
			BitBuffWrite(pBuff, DefaultTransform, LenTransform, jbEnv);
		} else { /* Explicit transform 1+2 follows */
				BitBuffWrite(pBuff, ExpTransform, LenTransform, jbEnv);
				switch (pDescriptor->Xform) {
				case XFORM_SLANT_8x8:
				case XFORM_SLANT_4x4: u8Xfrm = Slant2D;
									  break;
				case XFORM_SLANT_1x8: u8Xfrm = SlantRow;
									  break;
				case XFORM_SLANT_8x1: u8Xfrm = SlantColumn;
									  break;
				case XFORM_NONE_8x8:  u8Xfrm = NoneXfrm;
									  break;
				BitBuffWrite(pBuff, u8Xfrm, LenExpTransform, jbEnv);
			}
		}

		/* write out the scan order */
		if 	(pDescriptor->ScanOrder == xform_to_scan [pDescriptor->Xform]) {
 			BitBuffWrite(pBuff, DefaultScanOrder, LenScanOrder, jbEnv);
		} else { /* error */	  
			goto error;
		}

		/* write out the quant matrix */
		if (pDescriptor->QuantMatrix == u8DefQuant) {
			BitBuffWrite(pBuff, DefaultQuantMatrix, LenQuantMatrix, jbEnv);
		} else { /* error */	  
			goto error;
		}

		pDescriptor++;
	} /* end of for (i) */

	if (pGOPHdr->Transparency) { /* Transparency Descriptor */
		/* Bit depth of transparency:0 for bit depth of 1 (the only supported one in Indeo 5.0) */
		BitBuffWrite(pBuff, TransBitDepth, LenTransBitDepth, jbEnv);
		/* Write out the Transparent Color if it's present */
		if (pGOPHdr->pTransDscrpt->UseTransColor) {
			BitBuffWrite(pBuff, HasTransColor, LenHasTransColor, jbEnv);
			BitBuffWrite(pBuff, (pGOPHdr->pTransDscrpt->uTransXRGB&0x00FF0000)>>16,
						 LenTransColor, jbEnv);
			BitBuffWrite(pBuff, (pGOPHdr->pTransDscrpt->uTransXRGB&0x0000FF00)>>8, 
						 LenTransColor, jbEnv);
			BitBuffWrite(pBuff, (pGOPHdr->pTransDscrpt->uTransXRGB&0x000000FF), 
							 LenTransColor, jbEnv);
		} else {
			BitBuffWrite(pBuff, 0, LenHasTransColor, jbEnv);
		}
	}

	/* Byte align the version info */
    BitBuffByteAlign(pBuff, jbEnv);

	/*  version info: 24 bits ,  and no tool info follows*/
	BitBuffWrite(pBuff, pGOPHdr->Version.PlatformId, LenPlatformId, jbEnv);
	BitBuffWrite(pBuff, pGOPHdr->Version.BuildNum, LenBuildNum, jbEnv);
	BitBuffWrite(pBuff, pGOPHdr->Version.EncoderId, LenEncoderId, jbEnv);
	BitBuffWrite(pBuff, pGOPHdr->Version.BSVersion, LenBSVersion, jbEnv);
	BitBuffWrite(pBuff, pGOPHdr->Version.ChainBit, LenChainBit, jbEnv);

    BitBuffByteAlign(pBuff, jbEnv);
	BitBuffFlush(pBuff);

	if (pGOPHdr->GOPHdrSizeFlag) { /* update the GOP Header Size if the falg is on */
			/* Calculate the size of the actual header. */
		pGOPHdr->GOPHdrSize = (U32) BitBuffSize(pBuff);
		if (pGOPHdr->GOPHdrSize > MaxGOPHdrSize) {	/*  overflow */
			pGOPHdr->GOPHdrSize = 0;
		}
			 /* skip the GOPFlags to the right position */
		iTmp = BitBuffRead(pBuff, LenGOPFlags, jbEnv); 
		BitBuffOverWrite(pBuff, (U32)pGOPHdr->GOPHdrSize, LenGOPHdrSize, jbEnv);
		BitBuffFlush(pBuff); /* Reset the buffer for reading */
	}

	return;
error:
	longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
									(__LINE__ << LINE_OFFSET) |
									(ERR_BAD_PARM << TYPE_OFFSET));
}


/* Takes the information in the info structure for the picture header
   and formats it for the bitstream.
*/
static void CreatePicHdr(PPicBSInfoSt pBsPicInfo, jmp_buf jbEnv)
{
        pBitBuffSt pBuff;       /* output bit buffer */
        pPicHdrSt pHdr;         /* pic header info (input) */
		U8 u8PicFlags=0;
		U32 i;

        pHdr = &(pBsPicInfo->Hdr);

		/* write the 2 bytes of Start Code bit buffer */
		pBuff = pBsPicInfo->BsStartCode;	  
        BitBuffInit(pBuff, jbEnv);

        BitBuffWrite(pBuff, pHdr->PicStartCode, LenPicStartCode, jbEnv);
        BitBuffWrite(pBuff, pHdr->PictureType, LenPictureType, jbEnv);
        BitBuffWrite(pBuff, pHdr->PictureNum, LenPictureNum, jbEnv);
		
		BitBuffFlush(pBuff);

		/* write the GOP Header bit buffer for key frames */
		if(pHdr->PictureType == PIC_TYPE_K) { /* Key frame: encode GOP Header */
			CreateGOPHdr(pBsPicInfo, jbEnv);
		}	else { /* set the GOP header bit buffer to no data */
			pBuff = pBsPicInfo->BsGOPHdr;	  
			BitBuffInit(pBuff, jbEnv);
			BitBuffFlush(pBuff);
		}

		if(pHdr->PictureType == PIC_TYPE_R) { /* Repeat frame: no Picture Header */
			pBuff = pBsPicInfo->BsPicHdr;	  
			BitBuffInit(pBuff, jbEnv);
			BitBuffFlush(pBuff);
		}  else {

			/* Now write the picture Header bit buffer */
			pBuff = pBsPicInfo->BsPicHdr;	  
			BitBuffInit(pBuff, jbEnv);

				/* one byte of Picture Flags */
			if (pHdr->DataSizeFlag) {
				u8PicFlags |= PicFlags_DataSizeFlag;
			}
			if (pHdr->SideBitStream) {
				u8PicFlags |= PicFlags_SideBitStream;
			}
			if (pHdr->PicInheritTypeMV) {
				u8PicFlags |= PicFlags_PictureInheritTypeMV;
			}
			if (pHdr->PicInheritQ) {
				u8PicFlags |= PicFlags_PictureInheritQ;
			}
			if (pHdr->UseChecksum) {
				u8PicFlags |= PicFlags_UseChecksum;
			}
			if (pHdr->UsePicExt) {
				u8PicFlags |= PicFlags_UsePicExt;
			}
			if (pHdr->MBHuffTbl.StaticTblNum == DefaultHuffTbl) {
				pHdr->NonDefMBHuffTab = FALSE;
			} else {
				pHdr->NonDefMBHuffTab = TRUE;
				u8PicFlags |= PicFlags_NonDefMBHuffTab;
			}
			/* Band Data Size *must* be on for Key Frames when frame lock is used */
			if ((pHdr->BandDataSizeFlag) ||
			    ((pHdr->PictureType == PIC_TYPE_K) && (pBsPicInfo->Hdr.GOPHdr.UseKeyFrameLock))) {
				u8PicFlags |= PicFlags_BandDataSizeFlag;
			}

			BitBuffWrite(pBuff, u8PicFlags, LenPicFlags, jbEnv);

			if (pHdr->DataSizeFlag) {  
				/* optional: DataSize 
				 * It is a place holder here.
				 * It needs to be overwritten at the end of current frame encoding 
				 */
				BitBuffWrite(pBuff, pHdr->DataSize, LenPicDataSize, jbEnv);
			}

			if (pHdr->UseChecksum) {  /* optional: Checksum */
				BitBuffWrite(pBuff, pHdr->PicChecksum, LenPicChecksum, jbEnv);
			}

			if (pHdr->UsePicExt) {  /* optional: extension */
				pBSExtSt  pExt = pHdr->pPicExt; 
				while (pExt->NumBytes>0) {
					BitBuffWrite(pBuff, pExt->NumBytes, LenExtNumBytes, jbEnv);
					for (i=0; i<pExt->NumBytes; i++) {
						BitBuffWrite(pBuff, (U32)(*((pExt->pExtData)+i)), LenExtData, jbEnv);
					}
					pExt = pExt->pNextExt;
				}
				BitBuffWrite(pBuff, 0, LenExtNumBytes, jbEnv);  /* end of the link */

			}

			if (pHdr->NonDefMBHuffTab) { /* optional: non-default MB Huffman table */
				FormatHuffman(pBuff, &pHdr->MBHuffTbl, jbEnv);
			}

			if (pHdr->YClamping) {
				BitBuffWrite(pBuff, Clamping, LenClamping, jbEnv);
			} else {
				BitBuffWrite(pBuff, NotClamping, LenClamping, jbEnv);
			}
			if (pHdr->VClamping) {
				BitBuffWrite(pBuff, Clamping, LenClamping, jbEnv);
			} else {
				BitBuffWrite(pBuff, NotClamping, LenClamping, jbEnv);
			}
			if (pHdr->UClamping) {
				BitBuffWrite(pBuff, Clamping, LenClamping, jbEnv);
			} else {
				BitBuffWrite(pBuff, NotClamping, LenClamping, jbEnv);
			}
 
			BitBuffByteAlign(pBuff, jbEnv);
			BitBuffFlush(pBuff);
		} /* end of else {} */
} /* CreatePicHdr() */


/*
 * Compress the Transparency Band header structure into it's bit buffer
 */
static void 
CreateTransBandHdr(PTransSt pTransBand, PIA_Boolean UseBandDataSize, jmp_buf jbEnv) 
{
	pTransHdrSt  pTransHdr;               /* the Band's Header pointer */
	pBitBuffSt   pbBuf;                   /* the Band's bit buffer pointer */
	I32 i;

	/* Create some short cuts */
	pbBuf      = &pTransBand->BsTransHdr;
	pTransHdr = &pTransBand->TransHdr;

	/* Ensure the buffer pointers are initialized */
	BitBuffInit(pbBuf, jbEnv);

	/* place holder for TransDataSize */
	if (UseBandDataSize) {
		BitBuffWrite(pbBuf, 0, LenTransDataSize, jbEnv);
	}

	/* Number of dirty rectangles - dirty rectangles fully enclose those */
	/* parts of the transparency band which contain non-transparent pixels */
	BitBuffWrite(pbBuf, pTransHdr->NumDirtyRects, LenNumDirtyRects, jbEnv);


	/* Write out the dirty rectangles */
	for (i=0;i < pTransHdr->NumDirtyRects; i++) {
		BitBuffWrite(pbBuf, pTransHdr->DirtyRect.c, LenDirtyRectDim, jbEnv);
		BitBuffWrite(pbBuf, pTransHdr->DirtyRect.r, LenDirtyRectDim, jbEnv);
		BitBuffWrite(pbBuf, pTransHdr->DirtyRect.w, LenDirtyRectDim, jbEnv);
		BitBuffWrite(pbBuf, pTransHdr->DirtyRect.h, LenDirtyRectDim, jbEnv);
	}

	/* Write out the UseXOR bit */
	BitBuffWrite(pbBuf, pTransHdr->UseXOR_RLE, LenUseXOR_RLE, jbEnv);

	/* Write out the chosen Huffman tables */
	if (pTransBand->HuffTblUnchanged) {
		BitBuffWrite(pbBuf, DefTransHuffTbl, LenDefTransHuffTbl, jbEnv);
	} else {
		FormatTransHuffman(pbBuf, &pTransBand->TransHdr.TransHuffMapTbl, jbEnv);
	}
    
	/* Align and flush the bit buffer */
    BitBuffByteAlign(pbBuf, jbEnv);
	BitBuffFlush(pbBuf);
	return;
}


/*
 * Format the Band Header -- Code and Data follows
 */

/*
 * Compress the Band header structure into it's bit buffer
 */
static void CreateBandHdr(pBandSt pBsBandInfo, PIA_Boolean UseBandDataSize, jmp_buf jbEnv)
{
	U32 		uFixedBitVal; 		/* fixed part of an n+m bit value */
	pBandHdrSt 	pbHdr;				/* shortcut to Band's Header pointer */
	pBitBuffSt	pbBuf;				/* shortcut to Band's bit buffer pointer */
	U8			u8BandFlags=0;
	U32			i;

	pbBuf = &pBsBandInfo->BsBandHdr;  
	pbHdr = &pBsBandInfo->BandHdr;

	BitBuffInit(pbBuf, jbEnv); /* Ensure the buffer pointers are initialized */
	if(pbHdr->IsDropped) {	 /* write the BandFlags byte only */
		u8BandFlags |= BandFlags_BandDroppedFlag;
        BitBuffWrite(pbBuf, u8BandFlags, LenBandFlags, jbEnv);
	}	else {	 /* format rest of header only if not empty */
        /*
         * insert the place holder for the Band Data Size field.
		 * It includes both the band header and tile data for this band.
		 * It needs to be updated later.
         */

		if (pbHdr->InheritTypeMV) {
			u8BandFlags |= BandFlags_InheritTypeMV;
		}
		if (pbHdr->QuantDeltaUsed) {
			u8BandFlags |= BandFlags_QuantDeltaUsed;
		}
		if (pbHdr->InheritQ) {
			u8BandFlags |= BandFlags_InheritQ;
		}
		if (pbHdr->UseRVHuffMapChangeList) {
			u8BandFlags |= BandFlags_UseRVChangeList;
		}
		if (pbHdr->UseBandExt) {
			u8BandFlags |= BandFlags_UseBandExt;
		}
		if (pbHdr->RVMapTblNum == DefaultRvMapTbl ) {
			pbHdr->NonDefRVMapTab = FALSE;
		} else {
			pbHdr->NonDefRVMapTab = TRUE;
			u8BandFlags |= BandFlags_RVMappingTable;
		}
		if (pbHdr->BlkHuffTbl.StaticTblNum  == DefaultHuffTbl ) {
			pbHdr->NonDefBlkHuffTab = FALSE;
		} else {
			pbHdr->NonDefBlkHuffTab = TRUE;
			u8BandFlags |= BandFlags_BlkHuffTable;
		}

        BitBuffWrite(pbBuf, u8BandFlags, LenBandFlags, jbEnv);

		if (UseBandDataSize) {	 /* place holder */
			BitBuffWrite(pbBuf, 0, LenBandDataSize, jbEnv);
		}

		if (pbHdr->UseRVHuffMapChangeList) {
			/* Write out the RV Huffman Map Change list table */
			I32 i;
			PI32 piEntry;

			/* Write out the table entries -- this will not be done if the
			 * tables are inherited
			 */
			BitBuffWrite(pbBuf, pbHdr->RVSwapList.NumChanges, LenBandRVSwapNum, jbEnv);
			for (i = 0, piEntry = pbHdr->RVSwapList.ChangeList;
				 i < pbHdr->RVSwapList.NumChanges; i++) {

				uFixedBitVal = (U32) *piEntry++;
				BitBuffWrite(pbBuf, uFixedBitVal, LenBandRVSwapEntry, jbEnv);
				uFixedBitVal = (U32) *piEntry++;
				BitBuffWrite(pbBuf, uFixedBitVal, LenBandRVSwapEntry, jbEnv);
			}
		}

		if (pbHdr->NonDefRVMapTab) {
            /* Write the static table number to the bitstream */
            BitBuffWrite(pbBuf, pbHdr->RVMapTblNum, LenRVMapTbl, jbEnv);
		}

        /* Write out the chosen Huffman table if it not the default one  */
        if (pbHdr->NonDefBlkHuffTab) {
			FormatHuffman(pbBuf, &pbHdr->BlkHuffTbl, jbEnv);
		}
        if (pbHdr->UseChecksum == TRUE) { /* Write out the Checksum field	*/
			BitBuffWrite(pbBuf, UseBandChecksum, LenBandChecksumFlag, jbEnv);
			BitBuffWrite(pbBuf, pbHdr->Checksum, LenBandChecksum, jbEnv);
		}  else {
			BitBuffWrite(pbBuf, NotUseBandChecksum, LenBandChecksumFlag, jbEnv);
		}
        BitBuffWrite(pbBuf, pbHdr->GlobalQuant, LenBandGlobalQuant, jbEnv);

		if (pbHdr->UseBandExt) { /* optional: extension */
			pBSExtSt  pExt = pbHdr->pBandExt; 
			BitBuffByteAlign(pbBuf, jbEnv);  /* This field must be byte aligned */
			while (pExt->NumBytes>0) {
				BitBuffWrite(pbBuf, pExt->NumBytes, LenExtNumBytes, jbEnv);
				for (i=0; i<pExt->NumBytes; i++) {
					BitBuffWrite(pbBuf, (U32)(*((pExt->pExtData)+i)), LenExtData, jbEnv);
				}
				pExt = pExt->pNextExt;
			}
			BitBuffWrite(pbBuf, 0, LenExtNumBytes, jbEnv);  /* end of the link */
		}

	}

	/* Pad the entire structure out to a byte boundary */
	BitBuffByteAlign(pbBuf, jbEnv);  
	BitBuffFlush(pbBuf);

	return;
}

/* This function detects and marks bands with all empty tiles */
static PIA_Boolean EmptyBandTest(pTileSt pBsTileInfo, I32 iNumTiles)
{
	PIA_Boolean bRet = TRUE;  /* assume empty band first unless there is a nonempty tile */
	pTileHdrSt	pTileHdr;    		/* Pointer to the tile header */
    I32 iTile;                      /* Index to loop through tiles */

    for (iTile = 0; iTile < iNumTiles; iTile++) {
        pTileHdr = &(pBsTileInfo[iTile].TileHdr);
		if (!pTileHdr->IsEmpty) { /* one non empty tile is found*/
			bRet = FALSE;
			break;
		}
	}
	return(bRet);
}

/* This function determines if all of the bands are empty (have empty tiles) */
static PIA_Boolean EmptyFrameTest(PPicBSInfoSt pPicInfo) {

	PIA_Boolean bRet = TRUE;  /* assume empty frame first unless there is a nonempty band */
	pPlaneSt   pPlane;
	pBandSt    pBand;
	pBandHdrSt pBandHdr;
	I32 iPlane, iBand;

	pPlane = pPicInfo->Planes;

	/* Frame can only be empty if it's reference picture is the frame just */
	/* before it.  (Ref picture 255 is always changed to Ref picture 0) */

	if (pPicInfo->Hdr.PictureNum != pPicInfo->Hdr.RefPictureNum + 1) {
		bRet = FALSE;
	}

	/* If Transparency is on, then can't be an empty frame */
	if (pPicInfo->Hdr.GOPHdr.Transparency) {
		bRet = FALSE;
	}

	for (iPlane = 0; iPlane < pPicInfo->NumPlanes; iPlane++, pPlane++) {
  		pBand = pPlane->Bands;
   		for (iBand = 0; iBand < pPlane->NumBands; iBand++, pBand++) {
	       	pBandHdr = &pBand->BandHdr;
			if (!pBandHdr->IsEmpty)
				bRet = FALSE;
		}
	}

	return(bRet);
}

/*
 * Function to write the Tile, macro block and block information to the
 * corresponding Bit buffers in preparation for Huffman coding.  At this
 * stage for the MB level both Huffman coded and and fixed length codes
 * combined in a bitbuffer with a code to show which is which.
 */
static void BSFormatTiles (
        pTileSt pBsTileInfo,
        I32 iNumTiles,
        U8 u8PictureType, 
		pBandDscrptSt pBandDscrptor,
        pBandHdrSt pBandHdr,
        PI32 piBsMBHdrHist,
        PI32 piBsBlkDatHist,
        pRVMapTblSt pRVMapTbl,  /* Run val mapping table */
        PIA_Boolean bPicInheritQ,   /* Picture level inherit Q is on */
		jmp_buf jbEnv)
{
    pBitBuffSt	pBsPreHuffBlockDat; /* Pointer to the pre Huffman buffer */
    pBitBuffSt	pBsPreHuffMbHdr;    /* Pointer to the pre Huffman MB header */
    pMacHdrSt	pMacHdr;            /* Pointer to a macro block header */
    pMacBlockSt pMacBlocks; 		/* Pointer to the macro blocks */
    pTileHdrSt	pTileHdr;    		/* Pointer to the tile header */
    I32 iMcVal, iThisQ;             /* Intermediate values for bitstream formating */
    I32 iTile;                      /* Index to loop through tiles */
    I32 iMacBlock;                  /* Index to loop through macro blocks */
    I32 iBlock;                              /* Index to loop through blocks */
    McVectorSt LastMcVector;/* Encode difference from last McVector */
    U8 u8CbpMask = 1;
    /* Size of the Cbp written to the bitstream */
    U8 u8CbpLen = pBandDscrptor->MacBlockSize > pBandDscrptor->BlockSize ? 4 : 1;
    U8 u8LengthOfCode;
	PIA_Boolean bUseQuantDelta = pBandHdr->QuantDeltaUsed; 

    memset(piBsMBHdrHist,  0, sizeof(piBsMBHdrHist [0]) * MAX_SYMBOLS);
    memset(piBsBlkDatHist, 0, sizeof(piBsBlkDatHist[0]) * MAX_SYMBOLS);

    for (iTile = 0; iTile < iNumTiles; iTile++) {
        pTileHdr = &(pBsTileInfo[iTile].TileHdr);
        pBsPreHuffBlockDat = &(pBsTileInfo[iTile].BsPreHuffBlockDat);
        pBsPreHuffMbHdr = &(pBsTileInfo[iTile].BsPreHuffMbHdr);
        pMacBlocks = pBsTileInfo[iTile].MacBlocks;

        /* Init the bit buffers */
        BitBuffInit(pBsPreHuffBlockDat, jbEnv);
        BitBuffInit(pBsPreHuffMbHdr, jbEnv);

		/* Check for Empty Tile, by looping through all macroblocks first. */
			
		/* If this is band 0 Y plane, and Inherit Q is on, then can't be empty tile */

        if (!((pBandHdr->BandId == 0) && (pBandHdr->ColorPlane == COLOR_Y) && bPicInheritQ)) {
					
			pTileHdr->IsEmpty = TRUE;   /* Start out assuming it's TRUE */

			/* Now walk through the macroblocks, looking for a reason to make it FALSE. */
            for (iMacBlock = 0; iMacBlock < pBsTileInfo[iTile].NumMacBlocks; iMacBlock++) {

                pMacHdr = &(pMacBlocks[iMacBlock].MacHdr); /* This macro block header */

                /*
                 * A macro block has a strict definition of emptiness.  It is
                 * empty if it is a motion compensated vector with a 0,0 fwd mc
                 * vector and Cbp comes out as no pixels to process
                 */
				if (pBandHdr->InheritTypeMV) { /* inherit MV: then no need to check the MV  */
					if (!((pMacHdr->Cbp == 0) && (pMacHdr->MbType == TYPE_MV_FW))) {
						pTileHdr->IsEmpty = FALSE;   /* Tile isn't empty. */
						break;
					}
				} else { /* no inheritance: check MV as well */
					if (!((pMacHdr->Cbp == 0) && (pMacHdr->MbType == TYPE_MV_FW) &&
						(pMacHdr->McVector.r == 0) && (pMacHdr->McVector.c == 0))) {
									
						pTileHdr->IsEmpty = FALSE;   /* Tile isn't empty. */
						break;
					}
				}
			} /* end of for (iMacBlock) */
        } else {
			pTileHdr->IsEmpty = FALSE;
		}

        if (!pTileHdr->IsEmpty) {
            LastMcVector.r = LastMcVector.c = 0; /* Initial value of last McVector */

            /* Now loop through the macroblocks and write out the headers */
            for (iMacBlock = 0; iMacBlock < pBsTileInfo[iTile].NumMacBlocks;
                 iMacBlock++) {
                pMacHdr = &(pMacBlocks[iMacBlock].MacHdr); 

                /*
                 * A macro block is a macroblock with no encoded data in it. If MV is not
				 * inherited, then it is empty if and only if it is a motion compensated
				 * vector with a 0,0 fwd mc vector and Cbp comes out as no pixels to process.
				 * If MV is inherited, then it is empty if and only if it is a motion compensated
				 * vector which may or may not be zero, we don't care ) and Cbp comes out
				 * as no pixels to process. 
                 */
				if (pBandHdr->InheritTypeMV) { /* inherit MV: then no need to check the MV  */
					if ((pMacHdr->Cbp == 0) && (pMacHdr->MbType == TYPE_MV_FW)) {
	 					pMacHdr->IsEmpty = TRUE;   /* MB is empty. */
					} else {
						pMacHdr->IsEmpty = FALSE;   /* MB is not empty. */
					}
				} else { /* no inheritance: check MV as well */
					if ((pMacHdr->Cbp == 0) && (pMacHdr->MbType == TYPE_MV_FW) &&
						(pMacHdr->McVector.r == 0) && (pMacHdr->McVector.c == 0)) {
						pMacHdr->IsEmpty = TRUE;   /* MB is empty. */
					} else {
						pMacHdr->IsEmpty = FALSE;   /* MB is not empty. */
					}
				}
                if (pMacHdr->IsEmpty == TRUE) {

                    /* 1 bit tile empty indicator for each empty macro block */
                    u8LengthOfCode = LenMBEmpty;
                    BitBuffWrite(pBsPreHuffMbHdr, FixedLenIndic, IndicatorLen, jbEnv);
                    BitBuffWrite(pBsPreHuffMbHdr, u8LengthOfCode, FixLenLen, jbEnv);
                    BitBuffWrite(pBsPreHuffMbHdr, MBEmpty, u8LengthOfCode, jbEnv);

                    /* If it is band 0 and the Y color plane and the QuantDelta is used
					 * and inherit Q is on, then write the quantization value even if
					 * this macro block is empty so that it can be inherited by other bands.
                     */
                    if ((pBandHdr->BandId == 0) && (pBandHdr->ColorPlane == COLOR_Y)
						&& bPicInheritQ) {
                       
                        iThisQ = pMacHdr->QuantDelta - pBandHdr->GlobalQuant;
                        if (iThisQ > 8 || iThisQ < -8)
							longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
											(__LINE__ << LINE_OFFSET) |
											(ERR_BAD_PARM << TYPE_OFFSET));
                        iThisQ = tounsigned(iThisQ);
                        BitBuffWrite(pBsPreHuffMbHdr, HuffCodeIndic, 
                        			IndicatorLen, jbEnv);
                        BitBuffWrite(pBsPreHuffMbHdr, iThisQ, HuffLen, jbEnv);
                    }
                } else { /* This macro block is not empty */

                    /* Write a 1 bit tile empty indicator for a
                       non empty macro block */
                    u8LengthOfCode = LenMBEmpty;
                    BitBuffWrite(pBsPreHuffMbHdr, FixedLenIndic, IndicatorLen, jbEnv);
                    BitBuffWrite(pBsPreHuffMbHdr, u8LengthOfCode, FixLenLen, jbEnv);
                    BitBuffWrite(pBsPreHuffMbHdr, MBNotEmpty, u8LengthOfCode, jbEnv);

					/* Write type field if not inherited */
					if (!pBandHdr->InheritTypeMV) {
						/* Unidirectional delta frame */					

						if (u8PictureType == PIC_TYPE_P ||
						    u8PictureType == PIC_TYPE_P2 ||
							u8PictureType == PIC_TYPE_D) {
							
							u8LengthOfCode = 1;
							BitBuffWrite(pBsPreHuffMbHdr, FixedLenIndic, 
										IndicatorLen, jbEnv);
							BitBuffWrite(pBsPreHuffMbHdr, u8LengthOfCode, 
										FixLenLen, jbEnv);
							BitBuffWrite(pBsPreHuffMbHdr, pMacHdr->MbType, 
										u8LengthOfCode, jbEnv);
						} 
					} /* end if !InheritTypeMV */

                    /* Write the Cbp to the bitstream */
                    BitBuffWrite(pBsPreHuffMbHdr, FixedLenIndic, IndicatorLen, jbEnv);
                    BitBuffWrite(pBsPreHuffMbHdr, u8CbpLen, FixLenLen, jbEnv);
                    BitBuffWrite(pBsPreHuffMbHdr, pMacHdr->Cbp, u8CbpLen, jbEnv);

					if (bUseQuantDelta)  { /* If the QuantDelta is used */
						/* If Cbp == 0 Or this band inherits Q from the band 0 of Y, then there 
						 * is no Q value.  One exception is
						 * if it is band 0 in the Y plane and QuantDelta is used and inherited,
						 * then even if the Cbp is
						 * zero a Q value still needs to be written out so that
						 * subsequent bands can inherit it.
						 */
						if ((pMacHdr->Cbp != 0 && !pBandHdr->InheritQ) ||
                    		((pBandHdr->BandId == 0) &&
                    		(pBandHdr->ColorPlane == COLOR_Y) &&
                    		bPicInheritQ)) {
                   
							iThisQ = pMacHdr->QuantDelta - pBandHdr->GlobalQuant;
							if (iThisQ > 8 || iThisQ < -8)
								longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
												(__LINE__ << LINE_OFFSET) |
												(ERR_BAD_PARM << TYPE_OFFSET));
							iThisQ = tounsigned(iThisQ);
							BitBuffWrite(pBsPreHuffMbHdr, HuffCodeIndic, 
                        				IndicatorLen, jbEnv);
							BitBuffWrite(pBsPreHuffMbHdr, iThisQ, HuffLen, jbEnv);
						}
					}

                    /* If not inheriting Type and MV and it is not an intra MB
                       then write out the motion vector(s).
                    */
                    if (!pBandHdr->InheritTypeMV && pMacHdr->MbType != TYPE_MV_I) {

                        iMcVal = tounsigned((pMacHdr->McVector.r - LastMcVector.r) 
                                          / pBandDscrptor->MVRes);

                        BitBuffWrite(pBsPreHuffMbHdr, HuffCodeIndic, 
                        			IndicatorLen, jbEnv);
                        BitBuffWrite(pBsPreHuffMbHdr, iMcVal, HuffLen, jbEnv);

                        LastMcVector.r = pMacHdr->McVector.r;

                        iMcVal = tounsigned((pMacHdr->McVector.c - LastMcVector.c) 
                                          / pBandDscrptor->MVRes);

                        BitBuffWrite(pBsPreHuffMbHdr, HuffCodeIndic, 
                        			IndicatorLen, jbEnv);
                        BitBuffWrite(pBsPreHuffMbHdr, iMcVal, HuffLen, jbEnv);

                        LastMcVector.c = pMacHdr->McVector.c;

                    }
                    /* else don't encode motion vectors */

                    /* Write block data to bit buff based on cbp */

					/* The test that follows is special case code to handle 
					 * when a  macroblock is over half off the right 
					 * edge of the frame.  This results in a macroblock 
					 * with only two blocks...  In this case, the Blockflag 
					 * below has to be bumped from 1 to 4 (instead of 2) 
					 * so that the CBP for the next block is correct. 
					 */

                    u8CbpMask = 1;
                    for (iBlock = 0; iBlock < pMacBlocks[iMacBlock].NumBlocks;
                         iBlock++, u8CbpMask <<= 1) {

                        if (pMacHdr->Cbp & u8CbpMask) 
                            RVCodes2BitBuff(pBsPreHuffBlockDat,
                                    		&pMacBlocks[iMacBlock].Blocks[iBlock], 
                                    		pRVMapTbl, jbEnv);
						if (pMacBlocks[iMacBlock].MacBlockRect.w <= pBandDscrptor->BlockSize) 
							u8CbpMask <<= 1;
                    }
                } /* end if Macroblock not empty */
            } /* for each macroblock */

            BitBuffFlush(pBsPreHuffMbHdr);
            BitBuffFlush(pBsPreHuffBlockDat);

            /* Add the statistics of these buffers to the band histograms */
            BitBuffHist(pBsPreHuffMbHdr, piBsMBHdrHist, jbEnv);
            BitBuffHist(pBsPreHuffBlockDat, piBsBlkDatHist, jbEnv);
        } /* if(!pTileHdr->IsEmpty) */
    } /* Tile */
} /* BSFormatTiles() */


/*
 * Function to write the transparency tile header and run information to the
 * corresponding Bit buffers.
 */
static void TransTile2BitBuff(PTransTileSt pBsTransTileInfo, I32 iNumTiles,
							  PHuffTblSt pTransHuffTbl, jmp_buf jbEnv)
{
    pBitBuffSt pBsTransTileHdr;  /* Pointer to the tile header bit buff */
    pBitBuffSt pBsTransTileDat;  /* Pointer to the block header bit buff */
    pBitBuffSt pBsPreHuffDat;    /* Pointer to the transparency runs byte buff */
    PTransTileHdrSt pTransTileHdr;    /* Pointer to the tile header */
    I32 iNumTileEmpties;          /* Keep track of empty tiles */
    I32 iTile;                    /* Index to loop through macro blocks */

	for (iTile = 0, iNumTileEmpties = 0; iTile < iNumTiles; iTile++) {
        pTransTileHdr   = &(pBsTransTileInfo[iTile].TransTileHdr);
        pBsTransTileHdr = &(pBsTransTileInfo[iTile].BsTransTileHdr);
        pBsTransTileDat = &(pBsTransTileInfo[iTile].BsTransTileDat);
        pBsPreHuffDat   = &(pBsTransTileInfo[iTile].BsPreHuffDat);

        BitBuffInit(pBsTransTileHdr, jbEnv); /* Init the bit buffers */
        BitBuffInit(pBsTransTileDat, jbEnv);

        if (pTransTileHdr->IsEmpty) {
            iNumTileEmpties++;
        } else {
			pTransTileHdr->TileDataSize = 0;

			/* See if there were empty tiles before this one which isn't
			   and write the empty code to the bitstream for each empty set */
			while(iNumTileEmpties > 0) {
				/* Write a 1 bit tile empty indicator for each empty tile */
				BitBuffWrite(pBsTransTileHdr, TransTileEmpty, LenTransTileEmpty, jbEnv);
				pTransTileHdr->TileDataSize += LenTransTileEmpty;

				/* Now write the fill value (1 bit) for the empty tile.  This
				   fill bit value must come from the corresponding empty tile
				   even though the data gets put in the current tile header.
				 */
				BitBuffWrite(pBsTransTileHdr,
					pBsTransTileInfo[iTile - iNumTileEmpties]
									.TransTileHdr.InitialState,
					LenTransTileInitState, jbEnv);
				pTransTileHdr->TileDataSize += LenTransTileInitState;

				iNumTileEmpties--;
			}
			/* Write a 1 bit tile not empty indicator */
			BitBuffWrite(pBsTransTileHdr, TransTileNotEmpty, LenTransTileEmpty, jbEnv);
			pTransTileHdr->TileDataSize += LenTransTileEmpty;

			/* Write out the initial state of the tile */
			BitBuffWrite(pBsTransTileDat, pTransTileHdr->InitialState,
				LenTransTileInitState, jbEnv);
	
			/* Huffman encode the transparency run byte buffers */
			HuffEncBitBuff( pBsTransTileDat, pBsPreHuffDat, 
							pTransHuffTbl, NULL, jbEnv);

			/* Byte align and reset the buffers for reading */
			BitBuffByteAlign(pBsTransTileDat, jbEnv);
			BitBuffFlush(pBsTransTileDat);

			/* Write out the TileDataSize field */
			if (pTransTileHdr->IsTileDataSize) {
				BitBuffWrite(pBsTransTileHdr, UseTransTileDataSize, LenUseTransTileDataSize, jbEnv);
				/* TileDataSize is in bits at this point */
				pTransTileHdr->TileDataSize += 1;
				pTransTileHdr->TileDataSize += 8 * BitBuffSize(pBsTransTileDat);
				if ((  pTransTileHdr->TileDataSize + Small_TD_Width) 
					<= (U32)Max_Small_TD_Size * 8) {
					pTransTileHdr->TileDataSize =
						(pTransTileHdr->TileDataSize + Small_TD_Width + 7) / 8;
					BitBuffWrite(pBsTransTileHdr, 
								 pTransTileHdr->TileDataSize, 
								 Small_TD_Width, jbEnv);
				} else if ((  pTransTileHdr->TileDataSize 
							+ Large_TD_Width 
							+ Small_TD_Width) 
						 <= Max_Large_TD_Size * 8) {
					pTransTileHdr->TileDataSize = 
						( pTransTileHdr->TileDataSize + Large_TD_Width 
						+ Small_TD_Width + 7)/ 8;
					BitBuffWrite(pBsTransTileHdr, 
								 Max_Small_TD_Size + 1, 
								 Small_TD_Width, jbEnv);
					BitBuffWrite(pBsTransTileHdr, 
								 pTransTileHdr->TileDataSize, 
								 Large_TD_Width, jbEnv);
				} else { /* overflow the whole field: set to zero then */
					pTransTileHdr->TileDataSize = 0;
					BitBuffWrite(pBsTransTileHdr, 
								 pTransTileHdr->TileDataSize, 
								 Small_TD_Width, jbEnv);
				}
			} else { /* Tile size not present explicitly */
				BitBuffWrite(pBsTransTileHdr, NoTransTileDataSize, LenUseTransTileDataSize, jbEnv);
			}
			BitBuffByteAlign(pBsTransTileHdr, jbEnv);
			BitBuffFlush(pBsTransTileHdr);
        } /* end if tile empty ... else ... */
    }
    
    if (iNumTileEmpties > 0) {
        /* A run of empties finished the band */
        while (iNumTileEmpties > 0) {
            /* Write a 1 bit tile empty indicator for each empty tile */
            BitBuffWrite(pBsTransTileHdr, TransTileEmpty, LenTransTileEmpty, jbEnv);

			/* Now write the fill value (1 bit) for the empty tile.  This
			   fill bit value must come from the corresponding empty tile
			   even though the data gets put in the current tile header.
			 */
			BitBuffWrite(pBsTransTileHdr,
				pBsTransTileInfo[iTile - iNumTileEmpties].TransTileHdr.InitialState, 
				LenTransTileInitState, jbEnv);
			pTransTileHdr->TileDataSize += LenTransTileInitState;

            iNumTileEmpties--;
        }
        BitBuffByteAlign(pBsTransTileHdr, jbEnv);
        BitBuffFlush(pBsTransTileHdr);
    }

} /* TransTile2BitBuff() */

/*
 * Function to write the iTile, macro block and block information to the
 * corresponding Bit buffers.
 */
static void Tile2BitBuff(pTileSt pBsTileInfo,
						 I32 iNumTiles,
						 U8 u8PictureType,
						 pBandDscrptSt pBandDscrptor,
						 pBandHdrSt pBandHdr,
						 PHuffTblSt pMBHuffTbl,
						 PHuffTblSt pBHuffTbl,
						 pRVMapTblSt pRVMapTbl,
						 PIA_Boolean bIsExpBS,
						 jmp_buf jbEnv)
{
	pBitBuffSt pBsTileHdr;  /* Pointer to the tile header bit buff */
	pBitBuffSt pBsBlockDat; /* Pointer to the block header bit buff */
	pBitBuffSt pBsPreHuffBlockDat;  /* Pointer to the block header byte buff */
	pBitBuffSt pBsMacHdr;   /* Pointer to the macro block header bit buff */
	pBitBuffSt pBsPreHuffMbHdr;     /* Pointer to macro block header byte buff */
	pTileHdrSt pTileHdr;    /* Pointer to the tile header */
	pMacBlockSt pMacBlocks; /* Pointer to the macro blocks */
	I32 iNumTileEmpties;             /* Keep track of empty tiles */
	I32 iTile;                               /* Index to loop through macro blocks */
	McVectorSt LastMcVector;/* Encode difference from last McVector */
	U8 CbpMask = 1;
	PIA_Boolean IsCbp = pBandDscrptor->MacBlockSize > pBandDscrptor->BlockSize;

	for (iTile = 0, iNumTileEmpties = 0; iTile < iNumTiles; iTile++) {
        pTileHdr = &(pBsTileInfo[iTile].TileHdr);
        pBsTileHdr = &(pBsTileInfo[iTile].BsTileHdr);
        pBsBlockDat = &(pBsTileInfo[iTile].BsBlockDat);
        pBsPreHuffBlockDat = &(pBsTileInfo[iTile].BsPreHuffBlockDat);
        pBsMacHdr = &(pBsTileInfo[iTile].BsMacHdr);
        pBsPreHuffMbHdr = &(pBsTileInfo[iTile].BsPreHuffMbHdr);
        pMacBlocks = pBsTileInfo[iTile].MacBlocks;

        /* Init the bit buffers */
        BitBuffInit(pBsTileHdr, jbEnv);
        BitBuffInit(pBsBlockDat, jbEnv);
        BitBuffInit(pBsMacHdr, jbEnv);

        if (pTileHdr->IsEmpty) {
            iNumTileEmpties++;
        } else {
            pTileHdr->TileDataSize = 0;

            /* See if there were empty tiles before this one which isn't
               and write the empty code to the bitstream for each empty set */
            while (iNumTileEmpties > 0) {
                /* Write a 1 bit tile empty indicator for each empty tile */
                BitBuffWrite(pBsTileHdr, TileEmpty, LenTileEmpty, jbEnv);
                iNumTileEmpties--;
                pTileHdr->TileDataSize += LenTileEmpty;
            }
            /* Write a 1 bit tile not empty indicator */
            BitBuffWrite(pBsTileHdr, TileNotEmpty, LenTileEmpty, jbEnv);

            pTileHdr->TileDataSize += LenTileEmpty;

             /* Initial value of last McVector */
            LastMcVector.r = LastMcVector.c = 0;

			/* Huffman encode the macro block and block byte buffers */
			HuffEncBitBuff(pBsMacHdr, pBsPreHuffMbHdr, pMBHuffTbl, NULL, jbEnv);
			HuffEncBitBuff(pBsBlockDat,pBsPreHuffBlockDat,pBHuffTbl,pRVMapTbl,jbEnv);

#ifdef SIMULATOR /* dump block data sizes */
			if (pBsTileInfo->pSimInst->SimDumps.bDumpBlockDataSize) {
				SimPrintf(pBsTileInfo->pSimInst->pSFile, 
					pBsTileInfo->pSimInst->pSFile->uPrintFlags,
					"BlockDataSize: ","BlockPreHuff %8d PostHuff %8d MBPreHuff %8d MBPostHuff %8d\n",
					pBsPreHuffBlockDat->TotalBitsWritten,
					pBsBlockDat->TotalBitsWritten,
					pBsPreHuffMbHdr->TotalBitsWritten,
					pBsMacHdr->TotalBitsWritten);
			}

#endif /* SIMULATOR, dump block data sizes */

            /* Byte align and reset the buffers for reading */
            BitBuffByteAlign(pBsMacHdr, jbEnv);
            BitBuffFlush(pBsMacHdr);
            BitBuffByteAlign(pBsBlockDat, jbEnv);
            BitBuffFlush(pBsBlockDat);

            /* Write out the TileDataSize field */
			if (pTileHdr->IsTileDataSize) {
				BitBuffWrite(pBsTileHdr, UseTileDataSize, LenTileDataSize, jbEnv);
				/* TileDataSize is in bits at this point */
				pTileHdr->TileDataSize += LenTileDataSize;
				pTileHdr->TileDataSize += 8 * (BitBuffSize(pBsMacHdr) 
									   + BitBuffSize(pBsBlockDat));
				if (bIsExpBS) {
					pTileHdr->TileDataSize += (LenNumBlks + pTileHdr->NumBlks * LenBlkChecksum);
				}
				if (   (pTileHdr->TileDataSize + Small_TD_Width) 
					<= (U32) Max_Small_TD_Size * 8) {
					
					pTileHdr->TileDataSize =
						(pTileHdr->TileDataSize + Small_TD_Width + 7) / 8;
					BitBuffWrite(pBsTileHdr, pTileHdr->TileDataSize, 
								 Small_TD_Width, jbEnv);
				} else if ((pTileHdr->TileDataSize + Large_TD_Width + Small_TD_Width) 
					   <= Max_Large_TD_Size * 8) {
					pTileHdr->TileDataSize = (pTileHdr->TileDataSize 
											+ Large_TD_Width 
											+ Small_TD_Width + 7) / 8;
					BitBuffWrite(pBsTileHdr, Max_Small_TD_Size + 1, 
								Small_TD_Width, jbEnv);
					BitBuffWrite(pBsTileHdr, pTileHdr->TileDataSize, 
								Large_TD_Width, jbEnv);
				} else { /* overflow the whole field: write zero instead */
					pTileHdr->TileDataSize = 0;
					BitBuffWrite(pBsTileHdr, pTileHdr->TileDataSize, 
								 Small_TD_Width, jbEnv);
				}
			} else { 
				BitBuffWrite(pBsTileHdr, NotUseTileDataSize, LenTileDataSize, jbEnv);
            }

			if (bIsExpBS) { /* write the Block Checksums to BS */
				U32 i;
				BitBuffWrite(pBsTileHdr, pTileHdr->NumBlks, LenNumBlks, jbEnv);
				for (i=0; i<pTileHdr->NumBlks; i++) {
					BitBuffWrite(pBsTileHdr, pTileHdr->pu16BlkChecksums[i], LenBlkChecksum, jbEnv);
				}
			}

            BitBuffByteAlign(pBsTileHdr, jbEnv);
            BitBuffFlush(pBsTileHdr);
        } /* End if IsEmpty... else ... */
	} /* end for each tile */

	if (iNumTileEmpties > 0) { /* A run of empties finished the band */
        while(iNumTileEmpties > 0) {
            /* Write a 1 bit tile empty indicator for each empty tile */
            BitBuffWrite(pBsTileHdr, TileEmpty, LenTileEmpty, jbEnv);
            iNumTileEmpties--;
        }
        BitBuffByteAlign(pBsTileHdr, jbEnv);
        BitBuffFlush(pBsTileHdr);
	}
} /* Tile2BitBuff() */


/* Writes the run value codes in the block to the bit buffer.
*/
static void RVCodes2BitBuff(
        pBitBuffSt pBsDat,
        pBlockSt pBsInfoBlock,
        const pRVMapTblSt cpRVMapTbl,  /* Run val mapping table */
		jmp_buf jbEnv)
{
    I32 iRunVal;
    pRunValSt pThisRunVal;
    I32 iVal;

    pThisRunVal = pBsInfoBlock->RunVals;
    for (iRunVal = 0; iRunVal < pBsInfoBlock->NumRunVals; 
		 iRunVal++, pThisRunVal++) {

        pThisRunVal->RVCode = RunVal2RVCode(pThisRunVal->Run, 
        									pThisRunVal->Val, 
        									cpRVMapTbl);
        /* See of code is an escape code */
        if(pThisRunVal->RVCode != cpRVMapTbl->InvFreqOrdTbl[ESC]){
            BitBuffWrite(pBsDat, HuffCodeIndic, IndicatorLen, jbEnv);
            BitBuffWrite(pBsDat, pThisRunVal->RVCode, HuffLen, jbEnv);
        } else {
            /* Write an escape code to the bitstream. */
            BitBuffWrite(pBsDat, HuffCodeIndic, IndicatorLen, jbEnv);
            BitBuffWrite(pBsDat, pThisRunVal->RVCode, HuffLen, jbEnv);

            /* Write the run to the bitstream */
            BitBuffWrite(pBsDat, HuffCodeIndic, IndicatorLen, jbEnv);
            BitBuffWrite(pBsDat, pThisRunVal->Run - 1, HuffLen, jbEnv);

            iVal = tounsigned(pThisRunVal->Val);
            if (iVal > 0x3FFF) 
				longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BITSTREAM << TYPE_OFFSET));

            /* First 6 bits in val Lo */
            BitBuffWrite(pBsDat, HuffCodeIndic, IndicatorLen, jbEnv);
            BitBuffWrite(pBsDat, iVal & 0x3f, HuffLen, jbEnv);

            /* Remaining 8 bits in valHi */
            BitBuffWrite(pBsDat, HuffCodeIndic, IndicatorLen, jbEnv);
            BitBuffWrite(pBsDat, iVal >> 6, HuffLen, jbEnv);
        }
    }
} /* RVCodes2BitBuff() */

/* given a run and a value it returns an RV code. */
static I16 RunVal2RVCode(
        U8 u8Run,
        I32 iValue,
        PCRVMapTblSt pcRVMapTbl)  /* Run val mapping table */
{
    I16 i16RetRVCode;

    if (u8Run == EOB)
        i16RetRVCode = pcRVMapTbl->InvFreqOrdTbl[EOB];
    else { /* Output a run-val pair. */
        if (   (u8Run < pcRVMapTbl->NabsValLen) 
            && (ABS(iValue) <= pcRVMapTbl->NAbsVal[u8Run])) {
           
            /* Emit the special code for this run-val pair */
            if ( iValue > 0 ) 
                iValue += (I32) pcRVMapTbl->start[u8Run] - 1;
            else 
                iValue += (I32) pcRVMapTbl->start[u8Run];
            i16RetRVCode = pcRVMapTbl->InvFreqOrdTbl[iValue];
        } else { /* Emit an escaped run-val pair */
            i16RetRVCode = pcRVMapTbl->InvFreqOrdTbl[ESC];
        }
    }
    return i16RetRVCode;
} /* RunVal2RVCode() */


/* 
 * Determine codes for the next huffman group.
 * It returns the length of codes for this group.
 */ 
static I32 FindNextHuffGroup(I32 iFreqLeft, I32 iSymsLeft, PI32 piHist, PI32 piCum)
{
	static Dbl Pc = 0.38196601125010510; /* (3 - sqrt(5)) / 2 */
	I32 iCutoff = (I32)(iFreqLeft * Pc);
	I32 k = 0;
	I32 iCodeLen = -1; /* Num bits in code for G0 */
	I32 iNumSymbols = 1; /* 2^n = # symbols encoded */

	*piCum = 0;
	while (*piCum <= iCutoff) {
		iCodeLen++;	
		for ( ; k < MIN(iNumSymbols, iSymsLeft); k++) 
			*piCum += piHist[k];
		iNumSymbols *= 2;	
	}
	return iCodeLen;
}

/* This routine calculates the SVLC code using ALG1. The args specify
   the # of symbols and their distribution.  The algorithm constrains
   both the maximum code bitlength and the maximum number of groups. 
   Those constraints are currently hard defines.  A minimum number of
   symbols in the table is also sent in so that the resulting Huffman
   code is guaranteed to have at least that many symbols in it.  Also
   there is a constraint on the maximum bit length of the EOB code.  
   The worst case of EOB code's from all of the RV mapping tables is
   passed in and this code is forced to be within the maximum EOB
   bitlength constraint.
*/
static PBHuffTblSt FindExplHuffTable(
	I32 iMaxNumSym,	/* Maximum number of symbols allowed */
	PI32 piHist,	/* The frequencies of the symbols */
	I32 iMinNumSym,	/* Minimum number of symbols in the table.  This is used
					   to make sure that there are 256 symbols in block tables */
	I32 iEOBCode,	/* The maximum code for EOB in worst case (0 for MB tables) */
	I32 iMaxHuffBitLength, /* The maximum number of bits in a Huffman code */
	I32 iMaxPBGroups, /* Maximum number of allowed PB groups */
	jmp_buf jbEnv)	/* Jump buffer for exception handling */
{
	I32 iFreqLeft;	/* The sum of the histogram values left to encode */
	I32 k;			/* Symbol index */
	I32 iGroup;		/* The current group number  */
	I32 iSymbol, iCumCount;	/*   1st symbol; total "count" */
	U8 u8Bit;		/* # FLC bits */
	PBHuffTblSt table;	/* the SVLC table calculated and returned */
	I32 iNumSym = 0;		/* The total number of symbols there are to encode */
	I32 iSymsLeft = 0;	/* The number of symbols left to encode */
	I32 iTotFreq;		/* The sum of all the histogram values */

	I32 iBitLenOfEOB;	/* The bit length of the EOB code */
	I32 iEOBGroup;		/* Group containing the EOB code */
	I32 iEOBFreq;		/* Frequency of the EOB code */
	PIA_Boolean bSubFromHist = FALSE;	/* Signals that a value from iMinNumSym was
									   added to the histogram and needs to be
									   subtracted off at the end.
									 */
	const I32 ciEOBFreqStep = 10;  /* Amount to increment the EOB frequency in 
									 order to maintain the EOB code 
									 <= MAX_BITS_FOR_EOB.
								   */

	/* This makes certain that at least iMinNumSym symbols are in the 
	   dynamic Huffman table.  This allows the forcing of the dynamic
	   Huffman tables to have at least this many entries in it.  For the block
	   layer this is forced to be 256 so that this table encompasses all of 
	   the symbols possible.  This allows reuse of the table in the next
	   frame for decode rate control reasons.
	 */
	if (piHist[iMinNumSym - 1] == 0) {
		piHist[iMinNumSym - 1] = 1;
		bSubFromHist = TRUE;
	}

	iTotFreq = 0;
	for (k=0; k < iMaxNumSym; k++) {
		if (piHist[k] > 0) {
			iNumSym = k + 1;
		}
		iTotFreq += piHist[k];	/* message length */
	}

	/* Store the EOB code frequency for restoration after this loop */
	iEOBFreq = piHist[iEOBCode];
	do {
		iFreqLeft = iTotFreq;
		iGroup = iSymbol = 0;

		/* The -1 signals that the group which the EOB code belongs to has not
		   yet been found.
		 */
		iEOBGroup = -1;
		while (iFreqLeft > 0) {	/* loop until all symbols accounted for */
			/* This assures that the total number of groups is not greater than
			   the maximum number allowed.
			 */
			if(iGroup == iMaxPBGroups - 1) {
				table.NumRandBits[iGroup] = (U8) ceil(LOG2(iSymsLeft));
				iGroup++;
				break;					
			}
			u8Bit = (U8) FindNextHuffGroup( iFreqLeft, iNumSym-iSymbol, 
								 	piHist+iSymbol, &iCumCount);
			iFreqLeft -= iCumCount;

			/* Verify that the symbols will fit within the maximum bit length
			   constraint.  Adjust the current bit length if the number of
			   symbols the remaining PB descriptors can represent is less than
			   the number left.
			*/
			iSymsLeft = iNumSym - (iSymbol + (1 << u8Bit));
			/* Here 1 << (iMaxHuffBitLength - iGroup - 1) is the total number 
			   of sumbols which can be represented with the rest of the groups
			   given the maximum bit length constraint.
			 */
			k = (iSymbol + (1 << u8Bit));
			while ((iSymsLeft > 1 << (iMaxHuffBitLength - iGroup - 1)) &&
				   (iSymsLeft > 0)) {
				/* Add one bit to the current group in order to decrese the
				   number of remaining symbols so that they will fit into
				   the remaining groups.
				 */
				u8Bit++;
				iSymsLeft = iNumSym - (iSymbol + (1 << u8Bit));
				for  (; k < iNumSym && k < (iSymbol + (1 << u8Bit)); k++) {
					iFreqLeft -= piHist[k];				
				}
			}
			/* Set the number of bits in the group to the final value including the
			   "crowding" of the group done above in case of the maximum bit length
			   constraint.
			 */
			table.NumRandBits[iGroup] = u8Bit;
			iSymbol += (1 << u8Bit);	/* 2^n = # symbols in this group */

			/* If these conditions are true then this is the group which
			   contains the EOB code.
			 */
			if(iSymbol > iEOBCode && iEOBGroup == -1) {
				iEOBGroup = iGroup;
			}

			iGroup++;
		}

		/* This error checking really should be unnecessary since code above 
		   assures that this assumption is correct.  However, just to make 
		   sure we haven't missed anything.
		 */
		if (iGroup > MAX_PB_GROUPS)
			longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_BITSTREAM << TYPE_OFFSET));

		table.NumGroups = iGroup;

		/* Calculate the bitlength of the EOB code */
		iBitLenOfEOB = iEOBGroup + table.NumRandBits[iEOBGroup] + 1;

		/* If the EOB code is in the last group then decrement the bitlength */
		if(iEOBGroup == (table.NumGroups - 1)) {
			iBitLenOfEOB--;
		}

		/* Keep increasing the frequency of the EOB code until it's bitlength is
		   <= MAX_BITS_FOR_EOB.  This is only needed sometimes and general
		   works in 1 or 2 iterations.
		 */
		piHist[iEOBCode] += ciEOBFreqStep;	

		/* The total frequency also needs to be increased */
		iTotFreq += ciEOBFreqStep;
	} while (iBitLenOfEOB > MAX_BITS_FOR_EOB);

	/* If a value was added to the histogram in order to guarantee the
	   iMinNumSym number of symbols then subtract it off here so that the
	   histogram remains correct.
	 */
	if(bSubFromHist == TRUE) {
		piHist[iMinNumSym - 1] = 0;
	}

	/* Restore the correct original frequency of the EOB code */
	piHist[iEOBCode] = iEOBFreq;
	return table;
}

/* Adds the source histogram to the destination histogram */
static void CombineHistograms(PI32 piDest, PI32 piSrc)
{
    I32 i;

    for(i = 0; i < MAX_SYMBOLS; i++, piDest++, piSrc++) {
        *piDest += *piSrc;
    }
} /* CombineHistograms() */



/* Takes the input trans huffman choice and formats it for the bitstream.
*/
static void FormatTransHuffman( pBitBuffSt pBuff, 
								PHuffMapTblSt pMapTbl, 
								jmp_buf jbEnv)
{
	U32 uFixedBitVal;

	BitBuffWrite(pBuff, NotDefTransHuffTbl, LenDefTransHuffTbl, jbEnv);

	/* If the static table number is an escape then write out the PB style
	   descriptor in the bitstream. */
	if(pMapTbl->StaticTblNum == HuffTblEscape) {
		I32 i;
		PU8 pu8Entry;

		/* Verify that the number of groups will fit into 4 bits */
		if (pMapTbl->PBHuff.NumGroups & ~0x0f)
			longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));

		/* Write the number of groups in the descriptor */
		BitBuffWrite(pBuff, pMapTbl->PBHuff.NumGroups, PB_Entry_Size, jbEnv);
		for (i = 0, pu8Entry = pMapTbl->PBHuff.NumRandBits;
			 i < pMapTbl->PBHuff.NumGroups; i++, pu8Entry++) {
			uFixedBitVal = (U32) *pu8Entry;

			/* Verify that this value will fit into 4 bits */
			if (uFixedBitVal & ~0x0f)
				longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BITSTREAM << TYPE_OFFSET));

			BitBuffWrite(pBuff, uFixedBitVal, PB_Entry_Size, jbEnv);
		}
	}
} /* FormatTransHuffman() */

/* Takes the input huffman choice and formats it for the bitstream.
*/
static void FormatHuffman(pBitBuffSt pBuff, PHuffMapTblSt pMapTbl, jmp_buf jbEnv)
{
    U32	uFixedBitVal;	/* fixed part of an n+m bit value */

	/* This is only necessary for non-default  table */
    if (pMapTbl->StaticTblNum == DefaultHuffTbl) return;
	
    /* Write the static table number to the bitstream */
    BitBuffWrite(pBuff, pMapTbl->StaticTblNum, LenHuffTbl, jbEnv); 

    /* If the static table number is an escape then write out the PB style
       descriptor in the bitstream. */
    if (pMapTbl->StaticTblNum == HuffTblEscape) {
        I32 i;
        PU8 pu8Entry;

        /* Verify that the number of groups will fit into 4 bits */
        if (pMapTbl->PBHuff.NumGroups & ~0x0f)
			longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
							(__LINE__ << LINE_OFFSET) |
							(ERR_BAD_PARM << TYPE_OFFSET));

        /* Write the number of groups in the discriptor */
        BitBuffWrite(pBuff, pMapTbl->PBHuff.NumGroups, PB_Entry_Size, jbEnv);
        for (i = 0, pu8Entry = pMapTbl->PBHuff.NumRandBits;
             i < pMapTbl->PBHuff.NumGroups; i++, pu8Entry++) {
	        uFixedBitVal = (U32) *pu8Entry;

	        /* Verify that this value will fit into 4 bits */
	        if (uFixedBitVal & ~0x0f)
				longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
								(__LINE__ << LINE_OFFSET) |
								(ERR_BITSTREAM << TYPE_OFFSET));
	        BitBuffWrite(pBuff, uFixedBitVal, PB_Entry_Size, jbEnv);
        }
    }
} /* FormatHuffman() */

/*
 * Find the best RV Change list.
 */
static void FindRVChangeList(
        PHuffTblSt pHuffTbl,        /* Table used for Huffman encoding */
        PI32 piHist,                /* The histogram of the data to encode */
		PRVChangeListSt RVChgLst,	/* Pointer to the RV change list */
		pRVMapTblSt pRVMapTbl,		/* RV Map table to swap */
		jmp_buf jbEnv)
{
	I32 i, j;
	PHuffSymbolSt pHuffSym = pHuffTbl->HuffSym;
	I32 iTmpFreq;
	I32 iMaxBitsSaved = 1;
	I32 iBitsSaved;
	I32 iBesti, iBestj = 0;
	const I32 iSwapOverHead = 16;
	const I32 iMaxRVChanges = 61;
	I32 piTmpHist[MAX_SYMBOLS];
	U8 au8TmpFreqOrdTbl[MAX_SYMBOLS];	/* Temporary Freq Order Table */
	I32 iEob, iEsc;
	I32 iMaxRVCode;

	memcpy(piTmpHist, piHist, MAX_SYMBOLS * sizeof(I32));
	memcpy(au8TmpFreqOrdTbl, pRVMapTbl->FreqOrdTbl, MAX_SYMBOLS * sizeof(U8));

	iEob = pRVMapTbl->InvFreqOrdTbl[EOB];
	iEsc = pRVMapTbl->InvFreqOrdTbl[ESC];
	iMaxRVCode = MIN(pRVMapTbl->NumRVCodes, pHuffTbl->NumSymbols);

	/* init the rv numchanges = 0 */
	RVChgLst->NumChanges = 0;

	/* Loop through and determine if values are worth swapping or not.  During
	   this stage neither EOB or ESC can be swapped.  Swapping EOB or ESC causes
	   a hit in decoder performance and is not worth the cost.  
	 */
	while ((iMaxBitsSaved > 0) && (RVChgLst->NumChanges < iMaxRVChanges)) {
		for (i = 0, iMaxBitsSaved = 0; i < iMaxRVCode; i++) {
			if (i == iEob || i == iEsc) /* Don't swap EOB or Escapes */
				continue;

			for (j = i + 1; j < iMaxRVCode; j++) {
				if (j == iEob || j == iEsc) /* Don't swap EOB or Escapes */
					continue;

				/* Calculate the number of bits saved by making this swap */
				iBitsSaved =  (piTmpHist[i] - piTmpHist[j]) 
							* (pHuffSym[i].Len - pHuffSym[j].Len);
				
				if(iBitsSaved > iMaxBitsSaved) { /* Save the best one */
					iBestj = j;
					iBesti = i;
					iMaxBitsSaved = iBitsSaved;
				}
			}
		}
		iMaxBitsSaved -= iSwapOverHead;
		if (iMaxBitsSaved > 0) {
			/* Swap the histogram value */
			iTmpFreq = piTmpHist[iBesti];
			piTmpHist[iBesti] = piTmpHist[iBestj];
			piTmpHist[iBestj] = iTmpFreq;

			/* Swap the temporary FreqOrdTbl. */
			iTmpFreq = au8TmpFreqOrdTbl[iBesti];
			au8TmpFreqOrdTbl[iBesti] = au8TmpFreqOrdTbl[iBestj];
			au8TmpFreqOrdTbl[iBestj] = (U8) iTmpFreq;

			/* Correct the InvFreqOrdTbl */
			pRVMapTbl->InvFreqOrdTbl[au8TmpFreqOrdTbl[iBesti]] = (U8) iBesti;
			pRVMapTbl->InvFreqOrdTbl[au8TmpFreqOrdTbl[iBestj]] = (U8) iBestj;

			/* Add the swap values to the list */
			RVChgLst->ChangeList[2 * RVChgLst->NumChanges] = iBesti; 
			RVChgLst->ChangeList[2 * RVChgLst->NumChanges + 1] = iBestj; 
			RVChgLst->NumChanges++;
		}
	}
}



/* Function to loop through all of the block data and calculate the RV code
   histograms.
*/
static void
CalcRVHist(PPicBSInfoSt pPicInfo, jmp_buf jbEnv)
{
	pPlaneSt pPlane;
	pBandSt pBand;
	pTileSt pTile;
	pMacBlockSt pMb;
	pBlockSt pBlock;
	pRunValSt pRunVal;
	I32 iValue;			/* Value for the histogram */
	I32 iPlane, iBand, iTile, iMb, iBlock, iRv; /* Various indices */
	U8 u8Cbp;

	pPlane = pPicInfo->Planes;
	
	for (iPlane = 0; iPlane < pPicInfo->NumPlanes; iPlane++, pPlane++) {
		pBand = pPlane->Bands;
		for (iBand = 0; iBand < pPlane->NumBands; iBand++, pBand++) {
			/* Set the band level histogram to zero */
			memset(pBand->HuffInfo.RVHist[0], 0, sizeof(pBand->HuffInfo.RVHist));

			pTile = pBand->Tiles;
			for (iTile = 0; iTile < pBand->NumTiles; iTile++, pTile++) {
				pMb = pTile->MacBlocks;
				for (iMb = 0; iMb < pTile->NumMacBlocks; iMb++, pMb++) {
					pBlock = pMb->Blocks;
					u8Cbp = pMb->MacHdr.Cbp;
					for (iBlock = 0; iBlock < pMb->NumBlocks; iBlock++, pBlock++,
						 u8Cbp >>= 1) {
						pRunVal = pBlock->RunVals;
											
						if (!(u8Cbp & 0x01)) {
							/* The following if statement handles the case 
							 * for 16x16 macroblocks where if the macroblock 
							 * width is 8 or less, only the left most two 
							 * blocks are in the macroblock.  In this case, 
							 * the CBP needs to be shifted twice between the 
							 * first and second blocks to Skip over the CBP 
							 * bit for the phantom upper right corner block. 
							 */
							if (pMb->MacBlockRect.w <= 8) 
								u8Cbp >>= 1;	
							continue;
						}

						if (pMb->MacBlockRect.w <= 8) 
							u8Cbp >>= 1;	

						for (iRv = 0; iRv < pBlock->NumRunVals; iRv++, pRunVal++) {
							if (pRunVal->Run >= MAX_NABSVAL_LEN)
								longjmp(jbEnv,  (ENBS << FILE_OFFSET) |
												(__LINE__ << LINE_OFFSET) |
												(ERR_BAD_PARM << TYPE_OFFSET));

							/* These will have to be escapes */
							iValue = pRunVal->Val;
							iValue = iValue > 0 ? (2*iValue)-1 : 2*(-iValue-1);
							if(iValue >= MAX_SYMBOLS - 1) {
								pBand->HuffInfo.RVHist
									[MAX_NABSVAL_LEN - 1][MAX_SYMBOLS - 1]++;
							} else {
								if(iValue < 0) 
									iValue = 0;
								pBand->HuffInfo.RVHist[pRunVal->Run][iValue]++;
							}
						}
					} /* block */
				} /* macro block */
			} /* tile */
			
		} /* band */
	} /* plane */
}


/* Write escape sub sample to the bitstream */
static void WriteEscSubSamp(pBitBuffSt pBuff, U8 u8SubSample, jmp_buf jbEnv)
{
	U8 u8BitVal;

	BitBuffWrite(pBuff, 3, 2, jbEnv); 	/* write escape */
	switch(u8SubSample) {
		case 1:
			u8BitVal = 0;
			break;
		case 2:
			u8BitVal = 1;
			break;
		case 4:
			u8BitVal = 2;
			break;
		case 8:
			u8BitVal = 3;
			break;
		default:
			/* Error condition */
			longjmp(jbEnv, (ENBS << FILE_OFFSET) |
					(__LINE__ << LINE_OFFSET) |
					(ERR_BAD_PARM << TYPE_OFFSET));
			break;
	}
	/* Write two bit value */
	BitBuffWrite(pBuff, u8BitVal, 2, jbEnv);
}

/* Find the Histogram info given this distribution or RV pairs */
static void	CalcRVCHist(
	PI32 piRVCHist,		/* Run val code histogram output */
	PCRVMapTblSt pcRvMapTbl,		/* Current map table */
	I32	aiRVHist[MAX_NABSVAL_LEN][MAX_SYMBOLS]) /* RV Code Histogram */
{
	I32 iRVCode;
	I32 iRun, iVal;		/* Indices to loop through RV histogram table */
	I32 iCurVal;		/* Restored value */

	/* Loop through all run val pairs and map then using the given
	   RV map table and create a histogram of the mapped values */
	for (iRun = 0; iRun < MAX_NABSVAL_LEN; iRun++) {
		for( iVal = 0; iVal < MAX_SYMBOLS; iVal++) {
			if (aiRVHist[iRun][iVal] != 0) {
				/* Get the run val code */
				iCurVal = (iVal % 2) ? (iVal + 1) / 2 : -(iVal / 2 + 1);
				iRVCode = (I32) RunVal2RVCode((U8) iRun, iCurVal, pcRvMapTbl);
				piRVCHist[iRVCode] += aiRVHist[iRun][iVal];
			}
		}
	}	
}

/* Returns which DRC context the picture type belongs to. */
I32 PicDRCType(I32 iPictureType, jmp_buf jbEnv)
{
	I32 iDRCType;	/* Storage for the picture DRC type */

	/* Find the DRC type given the picture type */
	switch(iPictureType) {
		case PIC_TYPE_K:
			iDRCType = DRC_TYPE_INTRA;			
			break;
		case PIC_TYPE_P:
		case PIC_TYPE_P2:
 			iDRCType = DRC_TYPE_PRED;			
			break;
		case PIC_TYPE_D:
 			iDRCType = DRC_TYPE_DELTA;			
			break;
		case PIC_TYPE_R:
			iDRCType = DRC_TYPE_NODRC;			
			break;

		default:
			longjmp(jbEnv, (ENBS << FILE_OFFSET) |
					(__LINE__ << LINE_OFFSET) |
					(ERR_BAD_PARM << TYPE_OFFSET));
			break;
	}	
           
	return iDRCType;
}
