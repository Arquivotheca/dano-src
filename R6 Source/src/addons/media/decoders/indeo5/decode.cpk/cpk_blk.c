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
/*
 * cpk_bkj.c
 *	Block and Macroblock decode routines
 *
 * Functions
 *	DecodeBlockInfo_C	decode macroblock data from the bitstream
 */

#include "datatype.h"
#include "pia_main.h"	/* shared ptr stuff */
#include "cpk_xfrm.h"
#include "cpk_blk.h"
#include "readbits.h"	/* rename to cpk_bits.h */

/* /////////////////////////////////////////////// */

/* Dequantization table configuration parameters.  */
#define MAXQCBITS	13		/* not counting sign */
#define MAXESCBITS	MAXQCBITS
#define LOG2MAXQUANT	8
#define MAXQUANT	1<<LOG2MAXQUANT
#if	(MAXQCBITS == 13) && (LOG2MAXQUANT == 9)
#define PRODUCTS 111218
#elif	(MAXQCBITS == 13) && (LOG2MAXQUANT == 8)
#define PRODUCTS 100124
#elif	(MAXQCBITS == 13) && (LOG2MAXQUANT == 7)
#define PRODUCTS 88922
#elif	(MAXQCBITS == 12) && (LOG2MAXQUANT == 9)
#define PRODUCTS 55370
#elif	(MAXQCBITS == 12) && (LOG2MAXQUANT == 8)
#define PRODUCTS 49944
#elif	(MAXQCBITS == 12) && (LOG2MAXQUANT == 7)
#define PRODUCTS 44406
#elif	(MAXQCBITS == 11) && (LOG2MAXQUANT == 9)
#define PRODUCTS 27438
#elif	(MAXQCBITS == 11) && (LOG2MAXQUANT == 8)
#define PRODUCTS 24842
#elif	(MAXQCBITS == 11) && (LOG2MAXQUANT == 7)
#define PRODUCTS 22134
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 9)
#define PRODUCTS 13500
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 8)
#define PRODUCTS 12306
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 7)
#define PRODUCTS 11010
#elif	(MAXQCBITS == 10) && (LOG2MAXQUANT == 6)
#define PRODUCTS 9658
#elif
#undef PRODUCTS
#error
#endif
#define MULSIZE (PRODUCTS * sizeof(I32))

/* DQTable memory-mapped file name*/
#define DQTABLENAME "IVI50_DQTable"



/* Module global data */
static I32 *multiply[MAXQUANT+1] = { 0 };

#ifndef QT_FOR_MAC
#pragma data_seg(".sdata")	/* following is data shared among instances */
#endif

static U8 PDQ[NQSETS][2][NQLEVS][64] = { 0 }; /* = { 0 } to make shared */
static U8 PSCAN[NZSETS][64] = { 0 };

#ifndef QT_FOR_MAC
#pragma data_seg() 	/* end shared data */
#endif

/*/////////////////////////// */


/* Begin non-reentrant code section */


/*
 * BuildSingleStageDQ()
 *
 * Function: build tables for inverse quantization.
 * One table per quantizer each with one table entry per possible
 * quantized value.  Use (MM..MMM~S - 1), to match escaped values.
 * Each table entry is the bin centered product of the quantized
 * value and the quantizer, plus a noise term.
 *
 * Input: none
 *
 * Output:	-1 error	- memory allocation failure
 *		 0 no error
 *		 multiply[] table initialized.
 *
 */
static I32
BuildSingleStageDQ2(void){

	I32 *p, s;
	int i, j, n, t, r;
	PIA_Boolean bDQExists;

	/* Get pointer to shared DQTable, if present. Create and initialize
	 * shared DQTable if it is not present.
	 */

	 /*
	 * Warning: The memory allocated for the dequant table is sharable, identified
	 * in all instances by the text string used in the following call. If
	 * the content of the table changes in any way the text string needs to also
	 * be changed to avoid attempting to share the table with an older version
	 * of the codec.
	 * Note: DQTable is 1 dword larger than required to allow one dword prior
	 * to multiply[1] to be accessed by inverse quantization (blockdec.c) as
	 * a performance optimization.
	 */
	p = (I32 *)HiveAllocSharedPtr(MULSIZE+4, (unsigned char*)DQTABLENAME, &bDQExists);
	
	if ( p == NULL ) return -1;
	multiply[0]  = p;		/* for memory free */
	p++;					/* skip past first dword (see note above) */
	
	if ( bDQExists )
	{	/* DQTable already initialized, just need to init multiply[] */
		for(i = 1; i <= MAXQUANT; i++){
			multiply[i] = p;
			p += ((1<<MAXESCBITS)/i) << 1;
		}
	}
	else
	{	/* Init both DQTable and multiply[] */
		for(i = 1; i <= MAXQUANT; i++){
			multiply[i] = p;
			r = i>>1;		/* q/2 */
#ifdef PREBUGFIX2_5
			n = i&1;
#else
			if(i == 1) n = 0;
			else n = i&1;
#endif
			t = 0;
			for(j=0; j < (1<<MAXESCBITS)/i; j++){
				t += i;
				s = t + r - n;
				*(p+0) = s;
				*(p+1) =  -s;
				/* *(p+16/sizeof(*p)) = *(p+32/sizeof(*p)); */
				p += 2;
			}
		}
	}

	if((p - multiply[1]) > MULSIZE) return -1;
	return 0;
} /* BuildSingleStageDQ2 */


/* Begin non-reentrant code */

/* Windows 3.1 port needs this helper */
/* ActiveMovie on NT needs this helper */

#ifndef QT_FOR_MAC
#pragma data_seg(".sdata")	/* following is data shared among instances */
#endif

volatile static int non_rommable_init_happenned = 0;

#ifndef QT_FOR_MAC
#pragma data_seg()	/* end shared data */
#endif

/* lew: move this stuff to common directory */
extern const au16QuantScale[6][2][24];
extern const U8 au8BaseTables[6][2][64];
extern U8 ubScan[4][64];

static 
void InitPSCAN(void) {
	I32 set, i;
	for (set = 0; set < NZSETS; set++) {
		for (i = 0; i < 64; i++) {
			PSCAN[set][i] = ubScan[set][i];
		}
	}
} /* InitPSCAN */

static
void InitPDQ(void) {

	I32 set;
	I32 i, j, k, q; 

	for (set = 0; set < NQSETS; set++) {
		if (set < 5) { /* Y plane set? */
			for (i = 0; i < 2; i++) {			/* For Inter/Intra */
				for (j=0; j < NQLEVS; j++) {
					for (k = 0; k < 64; k++) {	/* Set each element */
						q = (au8BaseTables[set][i][k] *
							 au16QuantScale[set][i][j]) >> 8;
						if (q < 1)   q = 1;
						if (q > 255) q = 255;
						PDQ[set][i][j][k] = (U8) q;
					}
				}
			}
		} else { /* UV plane set (6) */
			for (i = 0; i < 2; i++) {			/* For Inter/Intra */
				for (j=0; j < NQLEVS; j++) {
					for (k = 0; k < 16; k++) {	/* Set each element */
						q = (au8BaseTables[set][i][k] *
							 au16QuantScale[set][i][j]) >> 8;
						if (q < 1)   q = 1;
						if (q > 255) q = 255;
						PDQ[set][i][j][k] = (U8) q;
					} /* for k */
					for (; k < 64; k++) {
						PDQ[set][i][j][k] = 0;
					}
				} /* for j */
			} /* for i */
		} /* end if Y plane set else UV plane set */
	}
} /* InitPDQ */

/* ConvertScan4to8()
 * This changes a 4x4 block scan order table to match the decoders
 * sparsely populated 8x8 block approach.  Any scan order table with
 * the 63rd and 64th entries equal is a 4x4 table, with only the first
 * 16 entries valid.  This is all that needs to be done to the PSCAN
 * table if it is presented as a mapping of runlength along the scan
 * path to block location of the coefficient.
 */
static
void
ConvertScan4to8(void){

	int i, j;

	for(i = 0; i < NZSETS; i++)
		if(PSCAN[i][62] == PSCAN[i][63])
			for(j = 0; j < 16; j++)
				PSCAN[i][j] += PSCAN[i][j] & 0x0C;
} /* ConvertScan4to8 */

/* Change the quantization tables for 4x4 blocks to match the 4x4 in 8x8
 * model for the decoder.
 * Determine that such tables are present by examining the last element,
 * if it is 0, then only the first 16 entries of the table are valid.
 */
static
void
MapQuant4to8(){


	int i, j, k, m;

	for(i = 0; i < NQSETS; i++)
		if(PDQ[i][0][0][63] == 0)
			for(j = 0; j < 2; j++)
				for(k = 0; k < NQLEVS; k++)
					if(PDQ[i][j][k][63] == 0)
						for(m = 15; m >= 4; m--)
							PDQ[i][j][k][m+(m&~3)] =
									PDQ[i][j][k][m];
} /* MapQuant4to8 */


I32
InitDecodeTables2(void){
	/* Notes:
	 * We may be reinit'd without reload of .data.  Some changes
	 * which are made to .sdata after load are destructive if done
	 * multiple times.  We avoid this by checking the value of a
	 * .sdata item which is statically initialized (by the loader).
	 * The 'ConvertScan4to8()' and 'MapQuant4to8()' functions are
	 * among those which are controlled in this way.  The processor
	 * type detection stores it's output in .bss or .sdata, and as
	 * such it doesn't need to be rerun if .sdata isn't reloaded.
	 * Therefore it is also controlled in the same way.
	 * The 'InitPaletteConfiguration()' and 'BuildSingleStageDQ()'
	 * routines both are un-done by a deinit, and redone at init.
	 */

	if(non_rommable_init_happenned == 0){

		non_rommable_init_happenned = 0xDEADBEEF;
		InitPSCAN();
		InitPDQ();
		ConvertScan4to8();
		MapQuant4to8();

	} /* end if(not_initd) */

	if(BuildSingleStageDQ2() != 0)
		return -1;

/*	moved to coloroutstartup */
/*	InitPaletteConfiguration(); */

	return 0;
} /* InitDecodeTables2 */

I32
DeInitDecodeTables2(void){

	I32 errors;
/*	moved to coloroutshutdown */
/*	extern void DeInitPaletteConfiguration(); */

	errors = 0;

	if(multiply[0]!=NULL)
		if (HiveFreeSharedPtr(multiply[0]) == PIA_S_ERROR)
			errors = 1;
	multiply[0] = NULL;

/*	moved to coloroutshutdown */
/*	DeInitPaletteConfiguration(); */
	return(errors);
} /* DeInitDecodeTables2 */
/* End non-reentrant code */

/* //////////////////////////// */

static I32
ContextMatches2(BLOCKCONTEXT *context, SETCONTEXT *proposed) {

	int i;
	SETCONTEXT *current;

	if(!context->initialized) {
		return 0;
	}

	current = &context->setinfo_pair[context->currently_set];

	if( (current->transform_token != proposed->transform_token) )
		return 0;

	if(current->quant_token != proposed->quant_token) {
		return 0;
	}

	if(current->scan_token != proposed->scan_token) {
		return 0;
	}

	if(current->huffman != proposed->huffman){
		if(current->huffman[0] != proposed->huffman[0])
			return 0;

		for(i = 1; i <= current->huffman[0]; i++)
			if(current->huffman[i] != proposed->huffman[i])
				return 0;
	}

/*rvcodebookmatches */
	if(current->nabsval != proposed->nabsval){
		for(i = 0; current->nabsval[i] != 0; i++)
			if(current->nabsval[i] != proposed->nabsval[i])
				return 0;

		if(proposed->nabsval[i] != 0) return 0;
	}

/*	hint - optimize this loop in assembly */
	for(i = 0; i < 256; i++)
		if(current->invfreq[i] != proposed->invfreq[i])
			return 0;



/*	hint - optimize this loop in assembly */
	for(i = 0; i <= *proposed->bs_rvswaplist * 2; i++) {
		context->bs_rvswaplist[i] = proposed->bs_rvswaplist[i];
	}

	return 1;
}	/* ContextMatches2 */


static I32
ScanMatches2(BLOCKCONTEXT *context, U8 **runtoloc){

	SETCONTEXT *current, *last;

	current = &context->setinfo_pair[context->currently_set];
	last = &context->setinfo_pair[!context->currently_set];

	*runtoloc = PSCAN[current->scan_token];
	if(!context->initialized) {
		return(0);
	}
	if(last->scan_token != current->scan_token) {
		return(0);
	}
	return(1);
}	/* ScanMatches2 */

static I32
QuantMatches2(BLOCKCONTEXT *context){

	SETCONTEXT *current, *last;

	current = &context->setinfo_pair[context->currently_set];
	last = &context->setinfo_pair[!context->currently_set];

	context->dequant.setnumber = current->quant_token;

	if(!context->initialized) {
		return 0;
	}
	if(context->dequant.setnumber != last->quant_token) {
		return 0;
	}
	return 1;

} /* QuantMatches2() */


/*
 * Sets up an inverse quantization table accessed by runlength.
 *
 * The output table is a function of the quantizer set selected,
 * the scan order specified, and the prescaling information which
 * is indicated for the specified transform.
 *
 */
static I32
QuantToScanOrder2(BLOCKCONTEXT *context){

	int i, j, k;
	U8 *scanorder;

#ifdef __MEANTIME_BLKTABS__
	STARTCLOCK
#endif /* __MEANTIME_BLKTABS__ */

	if(multiply[0]==NULL)
		return -1;

	scanorder = PSCAN[context->setinfo_pair[context->currently_set].scan_token];

#ifdef USENOASM
	/* OPTIMIZE THIS TO AVOID STARTUP STUTTERS */
	for(i = 0; i < 2; i++){
		for(j = 0; j < NQLEVS; j++){
			context->dequant.level[i][j] =
				context->dequant.set[i][j];
			for(k = 0; k < 64; k++){
				context->dequant.set[i][j][k] =
		multiply[PDQ[context->dequant.setnumber][i][j][scanorder[k]]];
			}
		}
	}
#else /* USENOASM */
	for(i = 0; i < 2; i++){
		for(j = 0; j < NQLEVS; j++){
			I32 **this_dqset;
			PU8 this_pdq;
			this_dqset = context->dequant.set[i][j];
			this_pdq = PDQ[context->dequant.setnumber][i][j];
			context->dequant.level[i][j] = this_dqset;
			for(k = 0; k < 64; k++){
				this_dqset[k]=multiply[this_pdq[scanorder[k]]];
			}
		}
	}
#endif /* USENOASM */

#ifdef __MEANTIME_BLKTABS__
	STOPCLOCK
	if(iTimingFrame != -1){
		frametimes[iTimingFrame].qscan += elapsed;
		frametimes[iTimingFrame].num_qscan++;
	}
#endif /* __MEANTIME_BLKTABS__ */

	return 0;
} /* QuantToScanOrder2 */

static I32
RVCodeBookMatches2(SETCONTEXT *old, SETCONTEXT *new) {

	int i;

	if(old->nabsval != new->nabsval){
		for(i = 0; old->nabsval[i] != 0; i++)
			if(old->nabsval[i] != new->nabsval[i])
				return 0;
		if(new->nabsval[i] != 0) return 0;
	}

/* hint: optimize this loop in assembly */
	for(i = 0; i < 256; i++) {
		if(old->invfreq[i] != new->invfreq[i]) {
			return 0;
		}
	}

	return 1;
}


I32	CheckSetContext2(
	 pBlkCntxSt		context/*pBlkCntx */
	,pSetCntxSt		proposed/*pSetCntx */
	)
{
	extern CTRANSFORM ctransforms[];
	extern I32 BuildRVTables();
	extern I32 PBCodeBookMatches();
	extern I32 PrivateBuildPBHuffTab();

	int i, scan_match, quant_match, rvcodebookhit, pbcodebookhit;
	U32 cflags;
	I32 token;
	U8 *runtoloc;

	if(context == NULL) return -1;		/* ERROR */

	if(ContextMatches2(context, proposed)) {	/* NO CHANGE */
		return 0;
	}

	context->currently_set = !context->currently_set && context->initialized;
	context->setinfo_pair[context->currently_set] = *proposed;

	token = context->setinfo_pair[context->currently_set].transform_token;

	context->ctransform=ctransforms[token].cinvert;
	context->csmear=ctransforms[token].csmear;
	context->ctransform_flags = ctransforms[token].flags;
	cflags = ctransforms[token].flags;

	context->bs_rvswaplist = context->setinfo_pair[context->currently_set].bs_rvswaplist;

	if(!(scan_match = ScanMatches2(context, &runtoloc))){	/* NEW ZAG ? */

		U8 swapped_columns[64];

		/* r0r1 r2r3 r4r5 r6r7 to r0r1 r4r5 r2r3 r6r7 for NIB to band */
		if(cflags & TT_SWAP){	/* particular to block size 8 */
			for(i = 0; i < 64; i++) swapped_columns[i] = runtoloc[i];
			for(i = 0; i < 8; i++){
				int j;
				int which2, which3, which4, which5;
				U8 t2, t3;

				which2 = which3 = which4 = which5 = -1;
				for(j = 0; j < 64; j++){
					int candidate;
					candidate = swapped_columns[j] - i*8;
					if((candidate >= 2)&&(candidate <= 5)){
						switch(candidate){
							case 2:
								which2 = j; break;
							case 3:
								which3 = j; break;
							case 4:
								which4 = j; break;
							case 5:
								which5 = j; break;
						}
					}
				}
				if((which2|which3|which4|which5)&0x80000000) return -1;
				t2 = swapped_columns[which2];
				t3 = swapped_columns[which3];
				swapped_columns[which2] = swapped_columns[which4];
				swapped_columns[which3] = swapped_columns[which5];
				swapped_columns[which4] = t2;
				swapped_columns[which5] = t3;
			}
			runtoloc = swapped_columns;
		}

		if(cflags & TT_3236){
			for(i = 0; i < 64; i++)
				context->czag[i] = runtoloc[i] + runtoloc[i]/8;
		}
		else{
			for(i = 0; i < 64; i++)
				context->czag[i] = runtoloc[i];
		}

	}
	quant_match = QuantMatches2(context);

	if(!scan_match || !quant_match) {
		if(QuantToScanOrder2(context) != 0)	{			/* NEW QUANT */
			return -1;
		}
	}

	rvcodebookhit = 0;

	if(context->initialized) {
		if(RVCodeBookMatches2(&context->setinfo_pair[!context->currently_set],
							 &context->setinfo_pair[context->currently_set])) {
			rvcodebookhit = 1;
		}
	}

	if(rvcodebookhit == 0) {
		context->rvcodes.nabsval = context->setinfo_pair[context->currently_set].nabsval;
		context->rvcodes.invfreq = context->setinfo_pair[context->currently_set].invfreq;
		context->rvcodes.runtab = context->run;
		context->rvcodes.valtab = context->val;
		if(BuildRVTables(&context->rvcodes) != 0) {
			return -1;
		}
		context->escape = context->rvcodes.esc;
	}

	pbcodebookhit = 0;

	if(context->initialized)	/* possible PB main table hit? */
		if(PBCodeBookMatches(context->setinfo_pair[!context->currently_set].huffman, context->setinfo_pair[context->currently_set].huffman))
			pbcodebookhit = 1;

	if(pbcodebookhit == 0){
		context->hufftab = &context->hufftab_static;		/* NEW HUFF */
		context->hufftab->descriptor = context->setinfo_pair[context->currently_set].huffman;
		context->hufftab->maintable = context->hufftab->mainstatic;

#ifdef __MEANTIME_BLOCKDEC__  /* elides times for huffman rebuilds */
		STOPCLOCK
		if(iTimingFrame != -1){
			frametimes[iTimingFrame].checkctxt += elapsed;
			frametimes[iTimingFrame].num_checkctxt++;
		}
#endif
		if(PrivateBuildPBHuffTab(context->hufftab) != 0)
			return -1;
	}

#ifdef __MEANTIME_BLOCKDEC__
	STARTCLOCK
#endif

	context->initialized = 1;


	return 1;
}	/* CheckSetContext2 */


I32 PBCodeBookMatches(PBDESCRIPTOR_TYPE *have, PBDESCRIPTOR_TYPE *want) {

	int rows;

	if(have == want)
		return 1;

	rows = (int) *want;


	if(*have++ != *want++)
		return 0;

	while(rows--)
		if(*have++ != *want++)
			return 0;

	return 1;
} /* PBCodeBookMatches */



U32 DecodeBlockInfo_C(
	 U32			uFrameType /*&b->BlkCntx[pCntx->uFrameType]*/
	,pHuffTabSt		pHuff

	,pBlockInfoSt	pBlockInfo /*t->pBlockInfo*/
	,pBlockInfoSt	pBlockInfoYband0 /*t->pBlockInfo*/
	,PU8			pBandPtr
	,PointSt		uNMBlocks
	,U32			uMBSize
	,U32			uBlockSize		/* blocksize of current band */
	,U32			uPitch			/* pitch of current band */
	,U32			uMVRes
	,Boo			bInheritTypeMV	/* Inheritance: Type & MV */
	,Boo			bInheritQ		/* Inheritance: Quant */
	,Boo			bQuantDeltaUsed
	,U32			uGlobalQuant
	,U32			uB0GlobalQuant
	,Boo			bQWithCbp0
	,U32			uB0MBSize
	,DQTABLE		*sQuant
	,U32			bYB0_4BperMB	/* true if 4 blocks per mb in yb0 */
	,PU8			InPtr/*	InPtr + sumBytesRead*/
	) {

	bitbufst	p;

	Boo			bYBand0 = (pBlockInfo == pBlockInfoYband0);
	I32			iMVShift =	(I32)(uMBSize - uB0MBSize)>>3;
	U32			sBaseFlag[2];

	U32			mby, mbx;		/* macroblock loop indices */
	U32			type;			/* type of current mb (intra=0,inter=1) */
	U32			cbp;			/* coded block pattern of current mb */

	U32			uBlockInc, uBlockRowInc;
	PU8			rowptr, colptr;

	U8			Last_r = 0x80;
	U8			Last_c = 0x80;

/*	code begin... */

	readbitsinit2(&p, InPtr);

	type = 0;	/* initialize mb type to intra */
	/*	macroblock bits: */
	/*	e	empty bit */
	/*	t	type bit if inter */
	/*	c	cbp bit */

	if (uBlockSize == 4) {
		sBaseFlag[0] = uPitch | BT_FWD | BT_4x4;
		sBaseFlag[1] = uPitch | BT_INTRA | BT_4x4;
	}
	else {
		sBaseFlag[0] = uPitch | BT_FWD;
		sBaseFlag[1] = uPitch | BT_INTRA;
	}

	uBlockInc = uBlockSize*2;
	uBlockRowInc = uPitch*uBlockSize;
	rowptr = pBandPtr;
	for (mby = 0; mby < uNMBlocks.r; mby++) {
	colptr = rowptr;
	for (mbx = 0; mbx < uNMBlocks.c; mbx++) {
		U32		quant;

#ifdef DEBUG
		BlockInfoSt Exp_BlockInfo[4];
		Exp_BlockInfo[0] = pBlockInfo[0];
		if (uBlockSize != uMBSize) {
			Exp_BlockInfo[1] = pBlockInfo[1];
			Exp_BlockInfo[2] = pBlockInfo[2];
			Exp_BlockInfo[3] = pBlockInfo[3];
		}
#endif /* DEBUG */

	/*	read	cte */
		if (readbits2(&p, 1)) {
		/*	macroblock is empty */
			cbp = 0;

			if (bQWithCbp0) {
				quant = readbits2h(&p, pHuff);
				quant = TO_SIGNED(quant);
			}
			else {
				quant = 0;
			}
			pBlockInfo->flags = ((quant + NQLEVS) << BT_LOG2_QMASK)
				| sBaseFlag[0];

			quant = CLIP(quant + uGlobalQuant,0,23);

			pBlockInfo->quant =
				sQuant->level[0][quant];

			if (bInheritTypeMV) {
				U32 mv;
				mv = pBlockInfoYband0->motion_vectors;
				switch (iMVShift) {
					case 0:
						pBlockInfo->motion_vectors = mv;
						break;
					case -1:
					case 1:
						pBlockInfo->motion_vectors = SCALE_2(mv);
						break;
					case -2:
					case 2:
						pBlockInfo->motion_vectors = SCALE_4(mv);
						break;
				}
			}
			else {
				pBlockInfo->motion_vectors = VECTOR_ZERO;
			}

		}
		else {
			U32		intra;
		/*	type */
			if (bInheritTypeMV) {
				intra = (pBlockInfoYband0->flags & BT_INTRA) >> BT_LOG2_INTRA;
			}
			else {
				if (uFrameType) {
					intra = !readbits2(&p, 1);
				}
				else {
					intra = 1;
				}
			}
			pBlockInfo->flags = sBaseFlag[intra];

		/*	cbp */
			if (uBlockSize != uMBSize) {
				cbp = readbits2(&p, 4);
			}
			else {
				cbp = readbits2(&p, 1);
			}

		/*	quant */
			if (bQuantDeltaUsed) {
				if (bInheritQ) {
					quant = (pBlockInfoYband0->flags>>BT_LOG2_QMASK)-NQLEVS;
				}
				else {
					if (bQWithCbp0 || cbp) {
						quant = readbits2h(&p, pHuff);
						quant = TO_SIGNED(quant);
					}
					else {
						quant = 0;
					}
				}
			}
			else {
				quant = 0;
			}

			pBlockInfo->flags |= ((quant + NQLEVS) << BT_LOG2_QMASK);

			quant = CLIP(quant + uGlobalQuant,0,23);

			pBlockInfo->quant =
				sQuant->level[intra][quant];

		/*	mv */
			if (pBlockInfo->flags & BT_INTRA) {
				pBlockInfo->motion_vectors = VECTOR_ZERO;
			}
			else {
				if (bInheritTypeMV) {
					U32 mv;
					mv = pBlockInfoYband0->motion_vectors;
					switch (iMVShift) {
						case 0:
							pBlockInfo->motion_vectors = mv;
							break;
						case -1:
						case 1:
							pBlockInfo->motion_vectors = SCALE_2(mv);
							break;
						case -2:
						case 2:
							pBlockInfo->motion_vectors = SCALE_4(mv);
							break;
					}
				}
				else {
					U8 r,c;

					r = (U8)readbits2h(&p, pHuff);
					c = (U8)readbits2h(&p, pHuff);

					r = (U8)(Last_r + TO_SIGNED(r));
					c = (U8)(Last_c + TO_SIGNED(c));

					pBlockInfo->motion_vectors = (r << 8) | c;
					Last_r = r;
					Last_c = c;
				}
			}

		}
		pBlockInfo[0].curr = colptr;
		if (uBlockSize != uMBSize) {
			pBlockInfo[1].curr = pBlockInfo->curr + uBlockInc;
			pBlockInfo[2].curr = pBlockInfo->curr + uBlockRowInc;
			pBlockInfo[3].curr = pBlockInfo[2].curr + uBlockInc;

			pBlockInfo[1].flags = pBlockInfo->flags | ((cbp >> 1) & 1) << BT_LOG2_CODED | BT_DUAL_HI;
			pBlockInfo[2].flags = pBlockInfo->flags | ((cbp >> 2) & 1) << BT_LOG2_CODED;
			pBlockInfo[3].flags = pBlockInfo->flags | ((cbp >> 3) & 1) << BT_LOG2_CODED | BT_DUAL_HI;
			pBlockInfo[0].flags = pBlockInfo->flags | ((cbp >> 0) & 1) << BT_LOG2_CODED;

			pBlockInfo[1].motion_vectors = pBlockInfo->motion_vectors;
			pBlockInfo[2].motion_vectors = pBlockInfo->motion_vectors;
			pBlockInfo[3].motion_vectors = pBlockInfo->motion_vectors;

			pBlockInfo[1].quant = pBlockInfo->quant;
			pBlockInfo[2].quant = pBlockInfo->quant;
			pBlockInfo[3].quant = pBlockInfo->quant;

#ifdef DEBUG
		{
			int i;
			for (i = 0; i < 4; i++) {
				if (pBlockInfo[i].curr != Exp_BlockInfo[i].curr) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
				}
				if (pBlockInfo[i].quant != Exp_BlockInfo[i].quant) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
				}
				if (Exp_BlockInfo[i].flags & BT_CODED) {
					if (pBlockInfo[i].flags != Exp_BlockInfo[i].flags) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
					}
				}
				else {
					if (pBlockInfo[i].flags != Exp_BlockInfo[i].flags) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
					}
				}
				if (Exp_BlockInfo[i].flags & BT_FWD)
				if (i) {
					if (pBlockInfo[i].motion_vectors != Exp_BlockInfo[i].motion_vectors) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
					}
				}
				else {
					if (pBlockInfo[i].motion_vectors != Exp_BlockInfo[i].motion_vectors) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
					}
				}
			}
		}
#endif /* DEBUG */

			pBlockInfo+=4;
			colptr += uBlockInc*2;
		}
		else {
			pBlockInfo->flags |= cbp << BT_LOG2_CODED;

#ifdef DEBUG
			if (pBlockInfo->curr != Exp_BlockInfo[0].curr) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
			}
			if (pBlockInfo->flags != Exp_BlockInfo[0].flags) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
			}
			if (pBlockInfo->quant != Exp_BlockInfo[0].quant) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
			}
			if (uFrameType)
			if (pBlockInfo->motion_vectors != Exp_BlockInfo[0].motion_vectors) {
#ifndef QT_FOR_MAC
					__asm nop;
#else
					HivePrintF("Whoa\n");
#endif
			}
#endif /* DEBUG */

			pBlockInfo++;
			colptr += uBlockInc;
		}

		if (bYB0_4BperMB) {
			pBlockInfoYband0 += 4;
		}
		else {
			pBlockInfoYband0++;
		}
	}	/* mbx loop */
	if (uBlockSize != uMBSize) {
		rowptr += uBlockRowInc*2;
	}
	else {
		rowptr += uBlockRowInc;
	}
	}	/* mby loop */

	return bytesread(InPtr, &p);
}	/* DecodeBlockInfo_C */




U32 DecodeBlockData_C(
	 pBlkCntxSt		pBlkCntx
	,U32			init
	,PU8			InPtr
	,pBlockInfoSt	pBlockInfo /*t->pBlockInfo*/
	,U32			uNBlocks
	,PU8			rvswap
) {
	bitbufst	p;
	I32			b;
	U32			esc, eob, code;
	pHuffTabSt	pHuff;
	I32			srcb[64], pred = 0;
	I32			indx;
	PU8			runtbl, zagtbl;
	PI32		valtbl;
	Boo			bDCDPCM;
	pfTransform	transform;
	pfTransform	smear;

/*	code begin... */

	pHuff		= pBlkCntx->hufftab;
	eob			= pBlkCntx->rvcodes.eob;
	esc			= pBlkCntx->rvcodes.esc;
	runtbl		= pBlkCntx->rvcodes.runtab;
	valtbl		= pBlkCntx->rvcodes.valtab;
	zagtbl		= pBlkCntx->czag;
	transform	= pBlkCntx->ctransform;
	smear		= pBlkCntx->csmear;

	bDCDPCM = pBlkCntx->ctransform_flags & TT_DCPCM;

	readbitsinit2(&p, InPtr);

/*todo move this code outside tile loop */
	for (b = 0; b < rvswap[0]; b++) {
		register U8		s1, s2;
		register U32	t1, t2;

		s1 = rvswap[1+2*b];
		s2 = rvswap[2+2*b];

		t1 = runtbl[s1];
		t2 = runtbl[s2];
		runtbl[s1] = (U8)t2;
		runtbl[s2] = (U8)t1;

		t1 = valtbl[s1];
		t2 = valtbl[s2];
		valtbl[s1] = t2;
		valtbl[s2] = t1;
	}

	for (b = 0; b < (I32)uNBlocks; b++) {

		indx = -1;
		if (pBlockInfo->flags&BT_CODED) {
			PI32	quanttbl;

		/*	can make this smaller for 4x4 blocks */
			{	U8 z;
				for (z = 0; z < 64; z++) srcb[z] = 0;
			}

			quanttbl = (PI32)(pBlockInfo->quant);
			while ((code = readbits2h(&p,pHuff)) != eob) {
				U8		run, z;
				I32		val;
				PI32	q;
				
				if (code == esc) {
					run = (U8)readbits2h(&p,pHuff); /* run - 1 */
					val = readbits2h(&p,pHuff);
					val |= readbits2h(&p,pHuff) << 6;
					z = zagtbl[indx+=run+1];
					q = (PI32)quanttbl[indx];
					val = q[val-1];
				}
				else {
					run = runtbl[code];
					val = valtbl[code];
					z = zagtbl[indx+=run];
					q = (PI32)quanttbl[indx];
					val = q[val-1];
				}
				srcb[z] = val;
			}
			/* opt: put flag in blockinfo for this case */
			if (bDCDPCM && (pBlockInfo->flags&BT_INTRA)) {
				srcb[0] = pred += srcb[0];
			}
			transform(&srcb[0], (PU8)pBlockInfo->curr, pBlockInfo->flags);
			/* transform the block here */
		}	/* if coded */
		else {	/* if not coded (empty) */
			if (pBlockInfo->flags&BT_INTRA) {
				smear(&pred, (PU8)pBlockInfo->curr, pBlockInfo->flags);
			}
		}	/* if not coded (empty) */

		pBlockInfo++;
	}	/* for b */

/*todo move this code outside tile loop */
	for (b = rvswap[0]-1; b >= 0; b--) {
		register U8		s1, s2;
		register U32	t1, t2;

		s1 = rvswap[1+2*b];
		s2 = rvswap[2+2*b];

		t1 = runtbl[s1];
		t2 = runtbl[s2];
		runtbl[s1] = (U8)t2;
		runtbl[s2] = (U8)t1;

		t1 = valtbl[s1];
		t2 = valtbl[s2];
		valtbl[s1] = t2;
		valtbl[s2] = t1;
	}

	return bytesread(InPtr, &p);
}	/* DecodeBlockData_C */
