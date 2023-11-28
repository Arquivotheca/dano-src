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
 * This file contains the code to support the debug info (block Checksums) that can
 * be written in the Indeo 5 bitstream if the Experimental Flag bit is on in the GOP Header.
 *
 */


#include	<setjmp.h>
#include    "datatype.h"
#include 	"pia_main.h" 
#include 	"matrix.h"
#include	"errhand.h"
 /* The following needed for bsutil.h */
#include	"mc.h"
#include	"hufftbls.h"
#include	"qnttbls.h"
#include	"mbhuftbl.h"
#include	"bkhuftbl.h"
#include "ivi5bs.h"

#ifdef SIMULATOR
#include "encsmdef.h"
#endif

#include 	"bsutil.h"
#include	"bsdbg.h"


/*
 * Forward, local references
 */
static U16 MatRectChkSum(RectSt rect, MatrixSt PicX, jmp_buf jbEnv);

/*
 * Calculate a checksum for only a block, specified by the rect, of a matrix.
 * Return the 16-bit checksum.
 */
static U16 MatRectChkSum(RectSt rect, MatrixSt PicX, jmp_buf jbEnv)
{
	MatrixSt	temp;
	U16			ret;

	SetMatrixRect(&temp, PicX, rect, jbEnv);
	ret = MatCksum(temp);
	return(ret);
}

/*
 * Calculate the motion compensated block checksums and save them in
 * the debug info
 */
void CalcMcBlkChkSum(pBandSt pBand, MatrixSt PicX, jmp_buf jbEnv)
{
	I32 tileNum, mbkNum, blkNum;		/* for loop indices */ 
	U32 uNumBlks;
	pTileSt		pTile;					/* short hand notation */
	pTileHdrSt pTileHdr;
	pMacBlockSt	pMBlk;					/* short hand notation */
	pBlockSt	pBlk;					/* short hand notation */

	/*
	 * Iterate over all of the blocks in the band
	 */
	for (tileNum = 0; tileNum < pBand->NumTiles; tileNum++) {
		pTile = &pBand->Tiles[tileNum];
		pTileHdr = &pTile->TileHdr;

		if ((pTileHdr->pu16BlkChecksums!=NULL) && (pTileHdr->NumBlks>0)) {

			uNumBlks = 0;
			for (mbkNum = 0; mbkNum < pTile->NumMacBlocks; mbkNum++) {
				pMBlk = &pTile->MacBlocks[mbkNum];

				for (blkNum = 0; blkNum < pMBlk->NumBlocks; blkNum++) {
					pBlk = &pMBlk->Blocks[blkNum];
					/*
					 * Use the rect for each block to map into the PicX
					 * and calculate the checksum
					 */
					pTileHdr->pu16BlkChecksums[uNumBlks++] 
						= MatRectChkSum(pBlk->BlockRect, PicX, jbEnv);
					if (uNumBlks > pTileHdr->NumBlks) { /* Should not happen */
						 longjmp(jbEnv, (BSDBG << FILE_OFFSET) |
										(__LINE__ << LINE_OFFSET) |
										(ERR_BITSTREAM));
					}
				}	/* end for (blkNum...) */
			}	/* end for (mbkNum...) */
		} /* end if () */
	}	/* end for (tileNum...) */
	return;
}


/*
 * Calculate the motion compensated block checksums and save them in
 * the debug info
 */
void CheckMcBlkChkSum(pBandSt pBand, MatrixSt PicX, jmp_buf jbEnv)
{
	I32 tileNum, mbkNum, blkNum;		/* for loop indices */ 
	pTileSt		pTile;					/* short hand notation */
	pTileHdrSt pTileHdr;
	pMacBlockSt	pMBlk;					/* short hand notation */
	pBlockSt	pBlk;					/* short hand notation */
	U16 u16McChkSum;
	U32 uNumBlks;
	/*
	 * Iterate over all of the blocks in the band
	 */
	for (tileNum = 0; tileNum < pBand->NumTiles; tileNum++) {
		pTile = &pBand->Tiles[tileNum];
		pTileHdr = &pTile->TileHdr;

		if ((pTileHdr->pu16BlkChecksums!=NULL) && (pTileHdr->NumBlks>0)) {

			uNumBlks = 0;
			for (mbkNum = 0; mbkNum < pTile->NumMacBlocks; mbkNum++) {
				pMBlk = &pTile->MacBlocks[mbkNum];

				for (blkNum = 0; blkNum < pMBlk->NumBlocks; blkNum++) {
					pBlk = &pMBlk->Blocks[blkNum];
					/*
					 * Use the rect for each block to map into the PicX
					 * and calculate the checksum
					 */
					 u16McChkSum = MatRectChkSum(pBlk->BlockRect, PicX, jbEnv);
					 if(u16McChkSum != pTileHdr->pu16BlkChecksums[uNumBlks++]) {
						 longjmp(jbEnv, (BSDBG << FILE_OFFSET) |
										(__LINE__ << LINE_OFFSET) |
										(ERR_BITSTREAM));
					 }
					if (uNumBlks > pTileHdr->NumBlks) { /* Should not happen */
						 longjmp(jbEnv, (BSDBG << FILE_OFFSET) |
										(__LINE__ << LINE_OFFSET) |
										(ERR_BITSTREAM));
					}
				}	/* end for (blkNum...) */
			}	/* end for (mbkNum...) */
		} /* end of if () */
	}	/* end for (tileNum...) */
	return;
}


