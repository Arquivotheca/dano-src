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
 * hufftbls.c
 *
 * Performs the intialiazation and manipulation of the huffman and run-value
 * tables.
 *
 * Functions:
 *	InitStaticEncodeTables	Sets up the tables necessary for encode
 *	InitStaticDecodeTables	Sets up the tables necessary for decode
 *	GetHuffmanTbls			Return the huffman tables being used
 *	GetRVMapTbls			Return the RV Mapping tables for a given band
 *	PB2Huff					Convert description to a generic huffman table
 *	GetDecHuffmanTable		Return a specific huffman table
 *	GetRVMapTbl				Return a specific RV Mapping table
 *	CreateHuffDecTable		Create a decode table from an encoded description
 *	InitRVMapTbl			Initialize a static RV mapping table
 */
/*#ifdef DEBUG */
/*#include <windows.h> Needed for errhand.h if DEBUG defined */
/*#endif*/

#include <setjmp.h>

#include "datatype.h"
#include "tksys.h"
#include "hufftbls.h"
#include "const.h"
#include "errhand.h"
#include "rvmaptbl.h"		/* contains the RV map tables */

/* array of const run val mapping tables at the picture layer*/
const PCRVMapTblSt RVMapTblSet[NUM_RV_MAP_TBLS] = {
		&MapTbl0, &MapTbl1, &MapTbl2, &MapTbl3, 
		&MapTbl4, &MapTbl5, &MapTbl6, &MapTbl7};

const PCRVMapTblSt pConstDefRVMapTbl = &MapTbl8;

#include "mbhuftbl.h"	/* Huffman table for macroblock data */
#include "bkhuftbl.h"	/* Huffman table for block data */

/**********************************************************/

void CreateHuffDecTable(PCHuffTblSt HuffTbl, 
								PNewDecHuffTblSt pNewDecHuffTbl,
								jmp_buf jbEnv);

U32  inv_bits(U32 Symbol, U8 Len);
void InitRVMapTbl(pRVMapTblSt pRVMapTbl, jmp_buf jbEnv);

/**********************************************************/


/* Initializes all of the Huffman tables and does the
   PB to Regular Huffman conversion 
 */
void InitStaticEncodeTables(HuffTblSt MBHuffTbls[],
							HuffTblSt BlkHuffTbls[],
							PHuffTblSt pDefMBHuffTbl,
							PHuffTblSt pDefBlkHuffTbl,
							jmp_buf jbEnv)
{
	I32 i;

	/* Create the static MB encode Huffman tables from the PB Style descriptors */
	for(i = 0; i < NUM_PB_MB_HUFF_TABLES; i++) {
		PB2Huff(&PBMBHuffTables[i], &MBHuffTbls[i]);
	}
	/* Create the static Block encode Huffman tables from the PB Style descriptors */
	for(i = 0; i < NUM_PB_BLK_HUFF_TABLES; i++) {
		PB2Huff(&PBBlkHuffTables[i], &BlkHuffTbls[i]);
	}

	/* Create the default MB Huffman table */
	PB2Huff(&PBMBHuffTables[NUM_PB_MB_HUFF_TABLES], pDefMBHuffTbl);

	/* Create the default Block Huffman table */
	PB2Huff(&PBBlkHuffTables[NUM_PB_MB_HUFF_TABLES], pDefBlkHuffTbl);

	return;
}

/*  return a set of RV mappping tables static RV mapping tables. */
void InitStaticRVMapTables(RVMapTblSt RVMapTbls[],
						   pRVMapTblSt pDefRVMapTbl,
						   jmp_buf jbEnv)
{
	I32 i;
	for (i = 0; i < NUM_RV_MAP_TBLS; i++) {
		RVMapTbls[i] = *RVMapTblSet[i];
		InitRVMapTbl(&RVMapTbls[i], jbEnv);		
		
	}

	/* Initialize the default Pic RV map table */
	*pDefRVMapTbl = *pConstDefRVMapTbl;

	InitRVMapTbl(pDefRVMapTbl, jbEnv);

}

/* Initializes all of the Huffman tables and does the
   PB to Regular Huffman conversion and the Encode Huffman to Decode
   Huffman conversions
*/
void InitStaticDecodeTables(HuffTblSt MBHuffTbl[],
								HuffTblSt BlkHuffTbl[],
							PHuffTblSt pDefMBHuffTbl,
								PHuffTblSt pDefBlkHuffTbl,
							PNewDecHuffTblSt pNewDecMBHuffTbls[],
								PNewDecHuffTblSt pNewDecBlkHuffTbls[],
							PNewDecHuffTblSt pNewDefDecMBHuffTbl, 
								PNewDecHuffTblSt pNewDefDecBlkHuffTbl, 
							jmp_buf jbEnv)
{
	I32 i;	

	/* Create the encode Huffman tables from the PB Style descriptors */
	for(i = 0; i < NUM_PB_MB_HUFF_TABLES; i++) {
		PB2Huff(&PBMBHuffTables[i], &MBHuffTbl[i]);
	}

	/* Create the encode Huffman tables from the PB Style descriptors */
	for(i = 0; i < NUM_PB_BLK_HUFF_TABLES; i++) {
		PB2Huff(&PBBlkHuffTables[i], &BlkHuffTbl[i]);
	}
	
	/* Create the default MB Huffman table */
	PB2Huff(&PBMBHuffTables[NUM_PB_MB_HUFF_TABLES], pDefMBHuffTbl);

	/* Create the default Block Huffman table */
	PB2Huff(&PBBlkHuffTables[NUM_PB_MB_HUFF_TABLES], pDefBlkHuffTbl);

	/* Given the Huffman encode tables create the corresponding
	   decode tables */
	for(i = 0; i < HUFF_TBLS_PER_BAND; i++) {
		CreateHuffDecTable(&MBHuffTbl[i], pNewDecMBHuffTbls[i], jbEnv);
		CreateHuffDecTable(&BlkHuffTbl[i], pNewDecBlkHuffTbls[i],jbEnv);
	}

	/* Also create the default decode Huffman tables */
	CreateHuffDecTable(pDefMBHuffTbl, pNewDefDecMBHuffTbl, jbEnv); 
	CreateHuffDecTable(pDefBlkHuffTbl, pNewDefDecBlkHuffTbl, jbEnv); 

	return;
}

/* Creates a Huffman decode table from an encode table description */
void CreateHuffDecTable(PCHuffTblSt pcHuffTbl, 
						PNewDecHuffTblSt pNewDecHuffTbl,
						jmp_buf jbEnv)
{
	I32 iSymbol;	/* Symbol number to be put in the Huffman decoder table. */
	PCHuffSymbolSt pcHuffSym;	/* Current huffman symbol */

  	/* Loop through each encode symbol and add it to the decode tree. */
	pNewDecHuffTbl->NumSymbols = pcHuffTbl->NumSymbols;
	pcHuffSym = pcHuffTbl->HuffSym;
	for (iSymbol = 0; iSymbol < pcHuffTbl->NumSymbols; iSymbol++, pcHuffSym++) {
		/* Now that we have a symbol, construct all bitstrings of length */
		/* MAX_HUFF_BITLENGTH that have this symbol as a prefix and set */
		/* the Hufftbl entries to this symbol for those strings */
		I32 iExtraBits = MAX_HUFF_BITLENGTH - pcHuffSym->Len;
		I32 iMask = (1 << iExtraBits) - 1;
		U32 uSymbol = pcHuffSym->Symbol;
		U8 u8Length = pcHuffSym->Len;
		pNewDecHuffTbl->BitsUsed[iSymbol] = u8Length;

		while (iMask >= 0) {
			I32 iIndex = (iMask << u8Length) | uSymbol;
			pNewDecHuffTbl->DecHuffSym[iIndex] = iSymbol;
			iMask--;
		}
	} /* for iSymbol */
} /* CreateHuffDecTable() */

/*  Takes a PB style description and converts it to a generic Huffman
	table representation.
*/
void PB2Huff(PCPBHuffTblSt pcPBTbl, PHuffTblSt pHuffTbl)
{
	I32 iPBGroup, iHuffSym = 0;
	I32 i;
	I32 iLastInGroup;
	I32 iGroupStart;
	U32 uCommaCode = 0;

  	/*  Loop through each group adding the symbols of that group to the
		Huffman table.
	*/
	for (iPBGroup = 0; iPBGroup < pcPBTbl->NumGroups; iPBGroup++) {
		/* Last of the symbols in this group */
		iLastInGroup = (1 << pcPBTbl->NumRandBits[iPBGroup]) - 1;

		/* Calculate the comma code for this group */
		uCommaCode = (U32) 0xffffff >> (24 - iPBGroup);

		/* iGroupStart is subtracted from iHuffSym so that the encoded symbols
		   start at 0.
		*/
		iGroupStart = iHuffSym;
		for (i = 0; i <= iLastInGroup; i++, iHuffSym++){
			if (iHuffSym >= MAX_SYMBOLS) {
				break;
			}
			if (iPBGroup != pcPBTbl->NumGroups - 1) {
				/* Because the bits are written to the bitstream from LSB to MSB
				   the bits must be have their order inverted.  For instance 100
				   becomes 001 and 011 becomes 110.
				*/
				pHuffTbl->HuffSym[iHuffSym].Len = (U8) (iPBGroup +
					pcPBTbl->NumRandBits[iPBGroup] + 1);
				pHuffTbl->HuffSym[iHuffSym].Symbol = 
					inv_bits(iHuffSym - iGroupStart, pcPBTbl->NumRandBits[iPBGroup])
						<< (iPBGroup + 1);
			} else {
				/* Because the bits are written to the bitstream from LSB to MSB
				   the bits must be have their order inverted.  For instance 100
				   becomes 001 and 011 becomes 110.
				*/
				pHuffTbl->HuffSym[iHuffSym].Len =
					(U8) (iPBGroup + pcPBTbl->NumRandBits[iPBGroup]);
				pHuffTbl->HuffSym[iHuffSym].Symbol = 
					inv_bits(iHuffSym - iGroupStart, pcPBTbl->NumRandBits[iPBGroup])
						<< iPBGroup;

				if (pHuffTbl->HuffSym[iHuffSym].Len == 0) {
					pHuffTbl->HuffSym[iHuffSym].Len = 1;
				}
			}
			/* Comma code starts in the */
			pHuffTbl->HuffSym[iHuffSym].Symbol |= uCommaCode;
		}
	}	

	pHuffTbl->NumSymbols = iHuffSym;
} /* PB2Huff() */


/* Copies the desired decode huffman table at the address passed in.
*/
void GetDecHuffmanTable(PHuffMapTblSt pHuffMapTbl, /* input: table index or PB descriptor */
				PNewDecHuffTblSt pNewDecHuffTable, 		/* The output huff tbl */
				PNewDecHuffTblSt pNewDefDecHuffTbl,	/* Default table */
				PNewDecHuffTblSt pNewDecHuffTbls[], /* decode static  hufftables */
				jmp_buf jbEnv)
{
	HuffTblSt HuffTbl;

	if ((pHuffMapTbl->StaticTblNum > HuffTblEscape) &&
		(pHuffMapTbl->StaticTblNum != DefaultHuffTbl))
		longjmp(jbEnv,  (HUFFTBLS << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));

	/* Verify that it is a static table and not a PB Style descriptor */
	if (pHuffMapTbl->StaticTblNum != HuffTblEscape){
		if (pHuffMapTbl->StaticTblNum == DefaultHuffTbl) {
			*pNewDecHuffTable = *pNewDefDecHuffTbl;
		} else {
			*pNewDecHuffTable = *pNewDecHuffTbls[pHuffMapTbl->StaticTblNum];
		}
	} else { /* Escape */
		/* Convert the PB style to a Huffman encode table */
		PB2Huff(&pHuffMapTbl->PBHuff, &HuffTbl);

		/* Convert the Huffman encode table to a decode table */
		CreateHuffDecTable(&HuffTbl, pNewDecHuffTable, jbEnv);
	}
}  /* GetDecHuffmanTable() */

/* Copies the desired RV map table from the static table array or the default one */
void GetRVMapTbl(U8 u8MapTblNum,	/* input: Map table number */
				 RVMapTblSt  RVMapTbls[],
				 pRVMapTblSt pDefRVMapTbl,
				 pRVMapTblSt pRVMapTable, /* output: place to copy the RV map table */
				 jmp_buf jbEnv)	
{
	if (u8MapTblNum > MapTblEscape && u8MapTblNum != DefaultRvMapTbl)
		longjmp(jbEnv,  (HUFFTBLS << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));

	/* Verify that it is a static table and not a PB Style descriptor */
	if (u8MapTblNum != DefaultRvMapTbl) {
		*pRVMapTable = RVMapTbls[u8MapTblNum];
	} else {
		*pRVMapTable = *pDefRVMapTbl;
	}
}  /* GetRVMapTbl() */

/*  Because the bits in the huffman table are written LSB
	 to MSB so the representation of the codes in the Huffman table
	 have to be inverted.  For instance the code 001 must be 100 and
	 011 to 110.
*/
U32 inv_bits(U32 uSymbol, U8 u8Len)
{
	const U32 uMask = 1;
	U32 uRetSymbol = 0;
	I32 i;

	for(i = 0; i < u8Len; i++, uSymbol >>= 1) {
		uRetSymbol |= (uSymbol & uMask) << (u8Len - i - 1);
	}
	return uRetSymbol;
}

/*	Build static tables for decoding run/values
*	runtbl[] and valtbl[] */
void InitRVMapTbl(pRVMapTblSt pRVMapTbl, jmp_buf jbEnv)
{
	I32 i = 2, k = 1, l = 0, j = 1;
	I32 inv;	/* Used to calculate the inverse values */
	
	pRVMapTbl->runtbl[EOB] = 0;
	pRVMapTbl->runtbl[ESC] = 0;
	pRVMapTbl->valtbl[EOB] = 0;
	pRVMapTbl->valtbl[ESC] = 0;

	if ((pRVMapTbl->NumRVCodes > MAX_SYMBOLS) || 
		(pRVMapTbl->NabsValLen > MAX_NABSVAL_LEN))
		longjmp(jbEnv,  (HUFFTBLS << FILE_OFFSET) |
						(__LINE__ << LINE_OFFSET) |
						(ERR_BAD_PARM << TYPE_OFFSET));
 
	/* Set up the run and val tables */
	while (i < pRVMapTbl->NumRVCodes) {
		if (k > l) {
			l = pRVMapTbl->NAbsVal[j++];
			k = -l;
		}
		pRVMapTbl->runtbl[i] = j-1;
		pRVMapTbl->valtbl[i] = (I16) k;
		
		k++;
		if (!k) {
			k++;
		}
		i++;
	}

	/* Initialize the start table */
	pRVMapTbl->start[0] = 1;
	for (i=1; i < pRVMapTbl->NabsValLen; i++) {
		pRVMapTbl->start[i] = pRVMapTbl->start[i-1] + pRVMapTbl->NAbsVal[i-1] +
			pRVMapTbl->NAbsVal[i];
	}

	/* Create the inverse table */
	for (inv = 0; inv < pRVMapTbl->NumRVCodes; inv++) {
		pRVMapTbl->InvFreqOrdTbl[pRVMapTbl->FreqOrdTbl[inv]] = (U8) inv;
	}

	return;
}
