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
 * bsutil.h
 *
 * This file contains definitions of bitstream related structures
 * and declaration of function prototypes for bitstream handling
 * functions.
 * Some other basic structures are also defined here: 
 * PlaneSt, BandSt, TransSt, TileSt, MacBlockSt, BlockSt
 *
 * This header depends upon:
 * #include "datatype.h"
 * #include	"mc.h"
 * #include	"hufftbls.h"
 * #include	"qnttbls.h"
 * #include	"mbhuftbl.h"
 * #include	"bkhuftbl.h"
 * #include "ivi5bs.h"
 * #ifdef SIMULATOR
 * #include "encsmdef.h"
 * #endif
 */

#ifdef INCLUDE_NESTING_CHECK
#ifdef __BSUTIL_H__
#pragma message("***** BSUTIL.H Included Multiple Times")
#endif
#endif

#ifndef __BSUTIL_H__
#define __BSUTIL_H__

/* Verify inclusion of required files */
#ifndef __DATATYPE_H__
#pragma message("bsutil.h requires datatype.h")
#endif
#ifndef __MC_H__
#pragma message("bsutil.h requires mc.h")
#endif
#ifndef __HUFFTBLS_H__
#pragma message("bsutil.h requires hufftbls.h")
#endif
#ifndef __QNTTBLS_H__
#pragma message("bsutil.h requires qnttbls.h")
#endif
#ifndef __MBHUFTBL_H__
#pragma message("bsutil.h requires mbhuftbl.h")
#endif
#ifndef __BKHUFTBL_H__
#pragma message("bsutil.h requires bkhuftbl.h")
#endif
#ifndef __IVI5BS_H__
#pragma message("bsutil.h requires ivi5bs.h")
#endif
#ifdef SIMULATOR
#ifndef __ENCSMDEF_H__
#pragma message("bsutil.h requires encsmdef.h for simulator build")
#endif
#endif /* SIMULATOR */

/* READ_BITS takes a pointer, the bit to start reading at,
 * the number of bits to read, and an U32 to store the value read in.  It
 * can read a maximum of 32 bits.
 */
#define READ_BITS(ptr, start, length, value) {\
	I32 offset,index; \
	offset = (start)%8; \
	index  = (start)/8; \
	value  = (U32)(((PU8)ptr)[index++]) >>     offset;  \
	value |= (U32)(((PU8)ptr)[index++]) << ( 8-offset); \
	value |= (U32)(((PU8)ptr)[index++]) << (16-offset); \
	value |= (U32)(((PU8)ptr)[index++]) << (24-offset); \
	value |= (U32)(((PU8)ptr)[index  ]) << (32-offset); \
	value &= (1<<(length))-1; \
}

/* The following are needed for quickly reading some of the useful header info without parsing 
 * the whole bitstream.
 */
#define Position_GOPHDR_SIZE_PRESENT  16
#define Position_GOPHDR_SIZE  24
#define LenStartCode 16
#define Position_FRAME_TYPE   5


/* Bitstream buffer structures */

/*  This structure creates a linked list of buffers to ease the problem
	of memory allocation.  The functions writing bits to the buffers
	make sure that the buffer is not overflowed and allocates a new one
	when necessary.  One parameter to the BitBuffAlloc() function is a
	desired buffer size.  This should be a best guess at a reasonable
	value without having to give some very large value just to make sure
	it is enough.
*/
typedef struct _BitBuffNodeSt {
	I32	BitsInBuff;						/* Number of bits in this buffer */
	PU8 Buff;							/* The bytes in the buffer */
	struct _BitBuffNodeSt *NextNode;	/* Pointer to the next buffer. */
} BitBuffNodeSt, *pBitBuffNodeSt;

/*  Main bit buffer structure */
typedef struct _BitBuffSt {
	PIA_Boolean	Write;				/* Buffer is in write mode */
	I32		BuffSize;			/* Number of bytes allocated to the buffers */
	U8		BitsInByte;			/* Bits left over in the current byte */
	I32		TotalBitsWritten;	/* Total bits written */
	I32		TotalBitsRead;		/* Total bits read */
	PU8		CurrentByte;		/* Pointer to the current byte */
	pBitBuffNodeSt RootNode;	/* Root of the linked list */
	pBitBuffNodeSt CurrentNode;	/* Current element of the linked list */
} BitBuffSt, *pBitBuffSt;

/* Macro to return and end of buffer indicator */

#define EOBUFF(x) (x->TotalBitsWritten <= x->TotalBitsRead)

/*
 * Bit buffer access functions
 */

/*
 * Allocate space for the bit buffer structure, initialize the pointers
 * and counters
 */
extern void	BitBuffAlloc(pBitBuffSt pBitBuff, I32 iInitBuffSize, jmp_buf jbEnv);

/*
 * Initialize a bit buffer structure:  reset pointers and set it up
 * for witing.
 */
extern void	BitBuffInit(pBitBuffSt pBitBuff, jmp_buf jbEnv);

/*
 * Free all nodes allocated to the bit buffer, as well as the bit buffer
 * itself.  Used in conjuntion with BitBuffInit.
 */
extern void	 BitBuffFree(pBitBuffSt pBitBuff, jmp_buf jbEnv);

/*
 * Appends Count bits of Value to the bitbuffer.  More nodes are allocated
 * if required.  The internal counters are advanced in the bit buffer.
 */
extern void	BitBuffWrite(pBitBuffSt pBitBuff, U32 uValue, U8 u8Count, 
						jmp_buf jbEnv);

/*
 * Write out n+m bits to the bit buffer.
 *
 * The "n" part is always written.
 * The "m" part is optional, and is only written if it's length > 0
 */

extern void NpMBitBuffWrite(pBitBuffSt pBuff, U32 uFixedBits, U8 u8FixedLen,
                U32 uVarBits, U8 u8VarLen, jmp_buf jbEnv);

/*
 * Writes Count bits of Value to the bitbuffer.
 * It is assumed that the bit buffer has been positioned by a prior read.
 * The internal counters are advanced in the bit buffer.
 */
extern void	BitBuffOverWrite(pBitBuffSt pBitBuff, U32 uValue, U8 u8Count, 
			jmp_buf jbEnv);

/*
 * "Rewind" the bit buffer.  Used after a write operation to allow
 * bit buffer reads.
 */
extern void	BitBuffFlush(pBitBuffSt pBitBuff);

/*
 * Read Count bits out of the bit buffer.
 * The internal counters are advanced in the bit buffer.
 * Fatal error if trying to read more bits than remain in the buffer.
 */
extern U32	BitBuffRead(pBitBuffSt pBitBuff, U8 u8Count, jmp_buf jbEnv);

/*
 * Read a full byte from the bit buffer as a byte (versus 1 bit at a time).
 * The internal counters are advanced in the bit buffer.
 * Fatal error if trying to read more bits than remain in the buffer.
 */
extern U8	BitBuffByteRead(pBitBuffSt pBitBuff, jmp_buf jbEnv);

/*
 * Unravels a bit buffer and its nodes into contiguous bytes of
 * memory.  The destination location is assumed to be large enough.
 */
extern U32 BitBuff2Mem(PU8 pu8MemTo, pBitBuffSt pBuffFrom, jmp_buf jbEnv);

/*
 * Advances the internal pointers of a "write to" bit biffer so that an
 * integral number of bytes is used.  The next write to the bit buffer
 * will be on a byte boundary.
 */
extern void	BitBuffByteAlign(pBitBuffSt pBitBuff, jmp_buf jbEnv);

/*
 * Advances the internal pointers of a "read from" bit biffer so that an
 * integral number of bytes is used.  The next read from the bit buffer
 * will be from a byte boundary.
 */
extern void	BitBuffSkip2ByteAlign(pBitBuffSt pBitBuff, jmp_buf jbEnv);

/* Returns the number of bytes in the buffer */
extern U32	BitBuffSize(pBitBuffSt pBuff);

/*
 * Calculate the histogram of the bit buffer.
 */
extern void BitBuffHist(pBitBuffSt pPreHuffBuff, PI32 piHist, jmp_buf jbEnv);

/* Pads to the next DWord and then adds an extra DWord at the end */
void SkipPad2DWord(pBitBuffSt pBitBuff, jmp_buf jbEnv);


/* Huffman read function */
U32 HuffRead(pBitBuffSt pBitBuff, 
			 PNewDecHuffTblSt pNewDecHuffTbl, 
			 jmp_buf jbEnv);

/* Takes an uncompressed byte buffer and Huffman encodes it placing
   the Huffman symbols in the Huffman bit buffer.
*/
void HuffEncBitBuff(pBitBuffSt pBitBuff, pBitBuffSt pPreHuffBuff,
		PHuffTblSt pHuffTbl, pRVMapTblSt pRVMapTbl, jmp_buf jbEnv);

I32 tosigned(I32 x);
I32 tounsigned(I32 x);

/* Keeps track of the context of each band to evaluate the need for a 
   decoder rebuild */
typedef struct _BandDRCCntxSt {
	HuffMapTblSt BlkHuffTbl;	/* Block Huffman table */
	U8		RVMapTblNum;	/* RV code mapping table number */
	PIA_Boolean RVMapTblChanged;	/* PIA_Boolean to signal that this table changed */
} BandDRCCntxSt, *pBandDRCCntxSt;

/* Keeps track of the context of picture level changes to evaluate the need
   for a decoder rebuild */
typedef struct _PicDRCCntxSt {
	HuffMapTblSt MBHuffTbl;	/* Macro block Huffman table */
} PicDRCCntxSt, *pPicDRCCntxSt;

/* Defines values governing the control of the decode rate */
typedef struct _PicDRCSt {
	PIA_Boolean	bShowDRC;		/* Switch to show DRC info in the output window */
	I32 CycleBudget;		/* The decode cycle budget */
	I32 TargetBudget;		/* The target cycle budget for each frame */
	I32 MaxBudget;			/* Never go over this budget */
	I32 LastCycles;			/* Number of cycles used up for the last frame */
	Dbl	dDRC;				/* Control CycleBudget level */
	Dbl DeltaTA;			/* Gain for modification of dDRC */
} PicDRCSt, *pPicDRCSt;


/* Huffman structure definitions */

typedef struct _MeBlockSt {   /* Describes the analysis of a block */
	Dbl        Error;    /* error after MC */
	McVectorSt Vect;  /* initial/final displ - units of 1/16 pel */
} MeBlockSt, *pMeBlockSt;

typedef struct _PicHuffInfoSt {
	/* static encoder table arrays */
	HuffTblSt  MBHuffTbls[NUM_PB_MB_HUFF_TABLES];
	HuffTblSt  BlkHuffTbls[NUM_PB_BLK_HUFF_TABLES];
	RVMapTblSt RVMapTbls[NUM_RV_MAP_TBLS];
	/* default encoder table */
	HuffTblSt  DefMBHuffTbl;
	HuffTblSt  DefBlkHuffTbl;
	RVMapTblSt DefRVMapTbl;
	/* static and default decoder tables */
	PNewDecHuffTblSt pNewDefDecMBHuffTbl;
	PNewDecHuffTblSt pNewDefDecBlkHuffTbl;
	PNewDecHuffTblSt pNewDecMBHuffTbls[HUFF_TBLS_PER_BAND];
	PNewDecHuffTblSt pNewDecBlkHuffTbls[HUFF_TBLS_PER_BAND];

	/* Picture level MB Header tables */

	/* The Chosen macro block huffman table : a pointer is not enough here.
	 * Because the Huffman table can be custom made table transmitted through BS, so
	 * it can potentially be different from any static table.
	 */
	HuffTblSt  MBHuff;		/* Chosen macro block huffman table */

	I32		   FHTMB;  			/* Forced  Huffman table */

	/* The Chosen decoder macro block huffman table : a pointer is not enough here.
	 * Because the Huffman table can be custom made table transmitted through BS, so
	 * it can potentially be different from any static table.
	 */
	NewDecHuffTblSt NewDecMBHuff;		/* Chosen decoder macro block huffman table */

	I32		   BsMBHdrHist[MAX_SYMBOLS];	/* Buffer histogram for the whole picture */
} PicHuffInfoSt, *pPicHuffInfoSt;

typedef struct _BandHuffInfoSt {  /* Band level: block tables */
	HuffTblSt	BlkHuff;			/* Chosen block huffman table */

	/* The Chosen decoder decoder block huffman table : a pointer is not enough here.
	 * Because the Huffman table can be custom made table transmitted through BS, so
	 * it can potentially be different from any static table.
	 */
	NewDecHuffTblSt NewDecBlkHuff;		/* Chosen decoder block huffman table */

	/* The Chosen Run value mapping table : a pointer is not enough here.
	 * Because the RV table must start from a static table or a default table, but 
	 * the RV Change list can potentially change that table into a unique table. 
	 * So we actually need the space to hold that table.
	 */
	RVMapTblSt	RVMap;			/* Chosen Run value mapping table */

	I32			BsMBHdrHist[MAX_SYMBOLS];	/* MB header histogram for this band */
	I32			BsBlkDatHist[MAX_SYMBOLS];  /* Block data Histogram for this band */
	I32			RVHist[MAX_NABSVAL_LEN][MAX_SYMBOLS];  /* RV Code Histogram for this band */
	I32 		HTBlock;			/* Forced Huffman encoding table */
	I32 		FRVTbl;					/* Forced values for RV code table */
	PIA_Boolean		UseRVChangeList;		/* It is ok to use an RV change list */
} BandHuffInfoSt, *pBandHuffInfoSt;

/* Bitstream structure information */
typedef struct _RunValSt {
	I16 RVCode;
	U8	Run;
	I16	Val;
} RunValSt, *pRunValSt;  
 	    
typedef struct _TransRVSt {
	U8 InitState;
	PU8 pRunDat;
} TransRVSt;  

/*  This structure defines the information in a block of data */
typedef struct _BlockSt {
	RectSt		BlockRect;			/* Rectangle of the band this block covers */
	I32			NumRunVals;			/* The number of run values */
	pRunValSt	RunVals;			/* Array of run values */
} BlockSt, *pBlockSt;

/*  This structure defines the information in a macro block. */
typedef struct _MacBlockSt {
	RectSt		MacBlockRect;		/* The rectangle this block covers */
	MacHdrSt	MacHdr;				/* Macro block header information */
	MeBlockSt	pMEInfo[MAX_ME_TYPES]; /* Information gathered through ME */
	I32			NumBlocks;			/* Number of blocks in this macro block */
	pBlockSt	Blocks;			/* Array of the blocks in this macro block */
} MacBlockSt, *pMacBlockSt;
typedef const MacBlockSt *pcMacBlockSt;

/* This structure defines the information for a tile */
typedef struct _TileSt {
	RectSt		TileRect;			/* Rectangle this tile covers */
	TileHdrSt	TileHdr;			/* Tile header information */
	BitBuffSt	BsTileHdr;			/* Huff Bit buffer for the tile header info */
	BitBuffSt	BsMacHdr;			/* Huff Bit buffer for the header info */
	BitBuffSt	BsBlockDat;			/* An array of run val pairs */
	BitBuffSt	BsPreHuffMbHdr;		/* Not Huffman encoded values */
	BitBuffSt	BsPreHuffBlockDat;	/* Not Huffman encoded run val pairs */
	I32		 	NumMacBlocks;		/* Number of macro blocks in this tile */
	pMacBlockSt	MacBlocks;			/* Macro blocks belonging to this tile */
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */
} TileSt, *pTileSt;
typedef const TileSt *pcTileSt;

/* This structure defines the information for a transparency tile */
typedef struct _TransTileSt {
	RectSt		TileRect;			/* Rectangle this trans tile covers */
	TransTileHdrSt	TransTileHdr;	/* Trans Tile header information */
	BitBuffSt	BsTransTileHdr;		/* Bit buffer for the tile header info */
	BitBuffSt	BsTransTileDat;		/* Huffman encoded runs */
	BitBuffSt	BsPreHuffDat;		/* Not Huffman encoded runs */
	TransRVSt   TransRV;			/* transparency runval structure */
	I32			NumRuns;			/* Number of runs in this tile */
	I32			RunHistogram[256];	/* Histogram of runs in this tile */
} TransTileSt, *PTransTileSt;
typedef const TransTileSt *PCTransTileSt;

/* This structure defines the information for a band */
typedef struct _BandSt {
	I32 		BandId;				/* Id number representing the band */
	RectSt		BandRect;			/* Mainly for width and height of the band */
	BandHdrSt	BandHdr;			/* Band header info */
	pBandDscrptSt pBandDscrptor;    /* Band Descriptor in the GOP header */
	BitBuffSt	BsBandHdr;			/* Bit buffer for the band header */
	BandHuffInfoSt	HuffInfo;			/* All Huffman and RV code information */
	U16			uwQuantTable[2][24][64]; /* complete quant table set to be used for this band */
	pQuantTableSt puwQuantTable;
	PU8			pubScan;			/* scan order to be used for the encoding of this band */
	U8 			pu8SpecialScan[64];	/* Memory for non-standard Scan orders */
	I32			NumTiles;			/* Number of tiles in this band */
	pTileSt 	Tiles;				/* Tiles belonging to this band */
	PIA_Boolean		DropBand;			/* TRUE -> skip write of band to bitstream */
	BandDRCCntxSt BandDRCCntx[NUM_PIC_DRC_TYPES];	/* Last context for each picture type */
} BandSt, *pBandSt, **ppBandSt;
typedef const BandSt *pcBandSt;

/* This structure defines the information for a plane */
typedef struct _PlaneSt {
	I32		PlaneId;				/* Id number representing this plane */
	I32		NumBands;				/* Number of bands in which it's divided */
	PU8		SubDiv;					/* 2 bit codes representing the band subdiv */
	pBandSt	Bands;					/* Bands in this plane */
	PIA_Boolean	Clamped;				/* TRUE -> clamping done to this frame */
	U8		GlobalQuant;	/* Global Quant. level for this plane */
} PlaneSt, *pPlaneSt;
typedef const PlaneSt *pcPlaneSt;

/* This structure defines the information for a transparency band */
typedef struct _TransSt {
	pTransDscrptSt pTransDscrptor;  /* Transparency band descriptor in the GOP Header */
	TransHdrSt	TransHdr;			/* Transparency Band header info */
	BitBuffSt	BsTransHdr;			/* Bit buffer for the Transparency Band header */
	I32			BsTransDatHist[MAX_SYMBOLS];  /* transparency data Histogram */
	I32			BsTransXORDatHist[MAX_SYMBOLS];  /* XOR transparency data Histogram */
	HuffTblSt 	TransHuff;			/* Chosen transparency band huffman table */
	NewDecHuffTblSt NewDecTransHuff;/* New style decoder huffman table */
	I32			NumTiles;			/* Number of tiles in trans band */
	I32			NumWTiles;			/* Number of tiles across */
	PTransTileSt	Tiles;			/* Tiles belonging to trans band */
	PTransTileSt 	XORTiles;		/* RLE_XOR Tiles belonging to this band */
	PIA_Boolean		HuffTblUnchanged;	/* This flag indicates that the huffman */
									/* table used is unchanged since the */
									/* last K or P (but not P2) frame, allowing it to */
									/* be omitted from the BS. */
} TransSt, *PTransSt;
typedef const TransSt *pcTransSt;

/* This structure defines the bitstream information for a picture */
typedef struct _PicBSInfoSt {
	PicHdrSt	Hdr;			/* Picture header info */
	pBitBuffSt	BsPicHdr;		/* Bit buffer for the picture header */	
	pBitBuffSt   BsStartCode;    /* Bit Buffer for the 2 bytes of start code */
	pBitBuffSt   BsGOPHdr;       /* Bit Buffer for the GOP Header */
	PicHuffInfoSt	HuffInfo;		/* All picture level Huffman information */
	NaturalInt	NumPlanes;		/* Number of planes in this picture */
	pPlaneSt	Planes;		/* Planes in this picture -- Always 3 */
	PicDRCSt	PicDRC;			/* Picture level decode rate control values */
	PicDRCCntxSt PicDRCCntx[NUM_PIC_DRC_TYPES];	/* Last context for each picture type */
	PTransSt	pTransBand;    	/* Transparency Plane Structure, this struct is */ 
#ifdef SIMULATOR
	PTR_ENC_SIM_INST pSimInst;  /* Pointer to simulator Instance */
#endif /* SIMULATOR */
} PicBSInfoSt, *PPicBSInfoSt; 

typedef const PicBSInfoSt *pcPicBSInfoSt;
/*
 * Buffer returned on encode and used on decode.
 */

typedef struct _BitStrmBuf {
	U32	bufSize;		/* the size of the buffer */
	PU8	bufPtr;			/* the address of the buffer */
} BitStrmBuf, *pBitStrmBuf;

#endif
