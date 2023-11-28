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
*               Copyright (c) 1994-1997 Intel Corp.				        *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

#ifdef INCLUDE_NESTING_CHECK
#ifdef __HUFFTBLS_H__
#pragma message("***** HUFFTBLS.H Included Multiple Times")
#endif
#endif

#ifndef __HUFFTBLS_H__
#define __HUFFTBLS_H__

#define EOB 0
#define ESC 1
#define PRINTVAL FALSE
#define HUFF_TBLS_PER_BAND 7
#define NUM_RV_MAP_TBLS 8
#define MAX_SYMBOLS 256

/* Maximum number of PB groups for transparency encoding */
#define MAX_TRANS_PB_GROUPS 7

/* Maximum allowed bit length for Huffman transparency codes */
#define MAX_TRANS_HUFF_BITLENGTH 12

/* This is 15 because it is the largest value which fits in 4 bits */
#define MAX_PB_GROUPS 15

/* Maximum allowed bit length for the Huffman codes */
#define MAX_HUFF_BITLENGTH 13

/* Need 2**MAX_HUFF_BITLENGTH for faster hufftables */
#define TWO_TO_MAXBITS	8192	
#define MAXBITS_MASK    TWO_TO_MAXBITS - 1		/* Mask off MAXBITS */

/* Hard constraint on the maximum number of bits allowed for EOB */
#define MAX_BITS_FOR_EOB 10
#define MAX_RVCHANGES 512
#define HUFF_TYPE_BLK 	0
#define HUFF_TYPE_MB 	1
#define MAX_NABSVAL_LEN 66	/* 64 + 2 (Max for 8x8 blocks) */

/* Huffman symbol structure */
typedef struct _HuffSymbolSt {
	U8 Len;
	U32 Symbol;	
} HuffSymbolSt, *PHuffSymbolSt;
typedef const HuffSymbolSt *PCHuffSymbolSt;

/* Huffman encode table definition */
typedef struct _HuffTblSt {
	I32 NumSymbols;
	HuffSymbolSt HuffSym[MAX_SYMBOLS];
} HuffTblSt, *PHuffTblSt;
typedef const HuffTblSt *PCHuffTblSt;

/* Huffman decode table element */
typedef struct _DecHuffSymbolSt {
	struct {
		I32 Sym;
		PIA_Boolean IsSym;
	} Node[2];
} DecHuffSymbolSt, *PDecHuffSymbolSt;

/* Huffman decode table */
typedef struct _NewDecHuffTblSt {
	I32 NumSymbols;
	U8 BitsUsed[MAX_SYMBOLS];
	U32 DecHuffSym[TWO_TO_MAXBITS];
} NewDecHuffTblSt, *PNewDecHuffTblSt;


/* Structure for the PB Huffman style descriptors */
typedef struct _PBHuffTblSt{
	I32 NumGroups;
	U8 NumRandBits[MAX_PB_GROUPS];
} PBHuffTblSt, *PPBHuffTblSt;
typedef const PBHuffTblSt *PCPBHuffTblSt;

/* Structure for the RVHuffChangeList */
typedef struct _RVChangeListSt {
	I32 NumChanges;
	I32 ChangeList[MAX_RVCHANGES];
} RVChangeListSt, *PRVChangeListSt;

/* Keeps track of the desired Huffman table */
typedef struct _HuffMapTblSt {
	U8 StaticTblNum;
	PBHuffTblSt PBHuff;
} HuffMapTblSt, *PHuffMapTblSt;

/* Run val mapping tables */
typedef struct _RVMapTblSt {
	I32 NabsValLen;
	U8 NAbsVal[MAX_NABSVAL_LEN];
	I32 NumRVCodes;
	U8 FreqOrdTbl[MAX_SYMBOLS];
	U8 InvFreqOrdTbl[MAX_SYMBOLS];  
	U8 start[MAX_NABSVAL_LEN];
	U8 runtbl[MAX_SYMBOLS];
	I16 valtbl[MAX_SYMBOLS];
} RVMapTblSt, *pRVMapTblSt;
typedef const RVMapTblSt *PCRVMapTblSt;

void InitStaticEncodeTables(HuffTblSt MBHuffTbls[],
							HuffTblSt BlkHuffTbls[],
							PHuffTblSt pDefMBHuffTbl,
							PHuffTblSt pDefBlkHuffTbl,
							jmp_buf jbEnv);

void InitStaticRVMapTables(RVMapTblSt RVMapTbls[],
						   pRVMapTblSt pDefRVMapTbl,
						   jmp_buf jbEnv);

void InitStaticDecodeTables(HuffTblSt MBHuffTbl[],
								HuffTblSt BlkHuffTbl[],
							PHuffTblSt pDefMBHuffTbl,
								PHuffTblSt pDefBlkHuffTbl,
							PNewDecHuffTblSt pNewDecMBHuffTbls[],
								PNewDecHuffTblSt pNewDecBlkHuffTbls[],
							PNewDecHuffTblSt pNewDefDecMBHuffTbl, 
								PNewDecHuffTblSt pNewDefDecBlkHuffTbl, 
							jmp_buf jbEnv);


void PB2Huff(PCPBHuffTblSt PBTbl, PHuffTblSt HuffTbl);
void GetDecHuffmanTable(PHuffMapTblSt pHuffMapTbl, /* input: table index or PB descriptor */
				PNewDecHuffTblSt pNewDecHuffTable, 		/* The output huff tbl */
				PNewDecHuffTblSt pNewDefDecHuffTbl,	/* Default table */
				PNewDecHuffTblSt pNewDecHuffTbls[], /* decode static  hufftables */
				jmp_buf jbEnv);

/* Copies the desired RV map table at the address passed in.
*/
void GetRVMapTbl(U8 u8MapTblNum,	/* input: Map table number */
				 RVMapTblSt  RVMapTbls[],
				 pRVMapTblSt pDefRVMapTbl,
				 pRVMapTblSt pRVMapTable, /* output: place to copy the RV map table */
				 jmp_buf jbEnv);	

void CreateHuffDecTable( 
	PCHuffTblSt HuffTbl, 
	PNewDecHuffTblSt NewDecHuffTbl,
	jmp_buf jbEnv);

void InitRVMapTbl(
	 pRVMapTblSt pRVMapTbl, jmp_buf jbEnv);

#endif /* __HUFFTBLS_H__ */
