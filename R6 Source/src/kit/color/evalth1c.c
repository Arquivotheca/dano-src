/*
 * @(#)evalth1c.c	1.18 98/01/13
*
*	(c) Copyright 1996-1997 Eastman Kodak Company
*	All rights reserved.
*
*	tetrahedral interpolation with cache within the color processor
*
*	Author:				 G. Pawle
*
*******************************************************************************/

#include "fut_util.h" 
#include "attrib.h"
#include "kcpfut.h"
#include "kcpcache.h"

#if defined (KCP_EVAL_TH1)


static KpUInt32_t th1CheckFut (KpHandle_t, KpInt32_t, KpUInt32_p, KpUInt32_p);
static void th1Itbl2InLut (fut_itbldat_ptr_t, KpInt32_t, KpInt32_t, ecItbl_p);
static void th1Itbl2In12tbl (fut_itbl_ptr_t, fut_itbldat_ptr_t);
static void th1Itbl2In16tbl (fut_itbl_ptr_t, fut_itbldat_ptr_t);
static void th1Otbl2Out8Lut (fut_otbldat_ptr_t, KpUInt8_p);
static void th1Otbl2Out12Lut (fut_otbldat_ptr_t, KpUInt16_p);
static void th1Otbl2Out16Lut (fut_otbldat_ptr_t, KpUInt16_p);
static void th1FreeCacheLutMem (th1Cache_p);
static void freeEcMem (ecMem_p);
static void nullEcMem (ecMem_p);
static void unlockEcMem (ecMem_p);
static void allocEcMem (ecMem_p, KpInt32_t);
static evalTh1Proc_t getTh1EvalFuncGen (void);
static evalTh1Proc_t getTh1EvalFuncOpt (KpHandle_t, KpInt32_t, KpInt32_t, KpUInt32_t, KpUInt32_t, KpUInt32_t);
static PTErr_t kcpInitTh1Luts (KpInt32_t, KpInt32_t, fut_gtbl_ptr_t, fut_itbl_ptr_t FAR*, fut_gtbldat_ptr_t FAR*, fut_otbldat_ptr_t FAR*, th1Cache_p			th1Cache);
static void expandITblMEP (fut_itbldat_ptr_t, KpUInt32_t, fut_itbldat_ptr_t, KpUInt32_t);


/* constant tables for finding pentahedral volumes and the corresponding coefficients and multipliers */

static KpUInt32_t	pentahedronBase[TH1_4D_COMBINATIONS] = { 24,  0, 23,  0,  0,  0, 12,  0,  0,  0, 22,  0,
															  0,  0,  9,  8, 18,  0,  0,  0, 17, 16, 11, 10,
															  0,  0,  0,  0,  0,  0,  0,  7, 21,  0,  0,  0,
															  0,  0,  0,  0, 20,  6, 19,  3,  0,  0,  0,  2,
															 15, 14,  0,  0,  0, 13,  0,  0,  0,  5,  0,  0,
															  0,  4,  0,  1};
static KpUInt32_t	bceiBase[TH1_4D_PENTAHEDROA] =		{ 9,  9,  9,  9,  9,  9,  5,  5,  5,  5, 5, 5,  3,  3,  3,  3, 3, 3,  2,  2,  2,  2, 2, 2};
static KpUInt32_t	dfgjkmBase[TH1_4D_PENTAHEDROA] =	{13, 13, 10, 11, 11, 10, 13, 13,  6,  7, 7, 6, 11, 11,  4,  7, 7, 4, 10, 10,  4,  6, 6, 4};
static KpUInt32_t	hlnoBase[TH1_4D_PENTAHEDROA] =		{15, 14, 14, 15, 12, 12, 15, 14, 14, 15, 8, 8, 15, 12, 12, 15, 8, 8, 14, 12, 12, 14, 8, 8};
static KpUInt32_t	dxBase[TH1_4D_PENTAHEDROA] =		{ 3,  3,  3,  3,  3,  3,  2,  2,  1,  1, 0, 0,  2,  2,  1,  1, 0, 0,  2,  2,  1,  1, 0, 0};
static KpUInt32_t	dyBase[TH1_4D_PENTAHEDROA] =		{ 2,  2,  1,  1,  0,  0,  3,  3,  3,  3, 3, 3,  1,  0,  0,  2, 2, 1,  1,  0,  0,  2, 2, 1};
static KpUInt32_t	dzBase[TH1_4D_PENTAHEDROA] =		{ 1,  0,  0,  2,  2,  1,  1,  0,  0,  2, 2, 1,  3,  3,  3,  3, 3, 3,  0,  1,  2,  0, 1, 2};
static KpUInt32_t	dtBase[TH1_4D_PENTAHEDROA] =		{ 0,  1,  2,  0,  1,  2,  0,  1,  2,  0, 1, 2,  0,  1,  2,  0, 1, 2,  3,  3,  3,  3, 3, 3};


/********************* getTh1EvalFuncGen ***************************/
/* Get a general evaluation function.
   This works for any FuT.
   Returns function address if available.
*/


static evalTh1Proc_t
	getTh1EvalFuncGen (void)
{
evalTh1Proc_t	func;

	func = evalTh1gen;

	return (func);
}


/********************* getTh1EvalFuncOpt ***************************/
/* See if this evaluation has been optimized.
   Returns function address if it has.
*/


evalTh1Proc_t
	getTh1EvalFuncOpt (	KpHandle_t	futH,
						KpInt32_t	iomask,
						KpInt32_t	nEvals,
						KpUInt32_t	ifmt,
						KpUInt32_t	ofmt,
						KpUInt32_t	evalDataType)
{
KpUInt32_t	standardFut, numInputs, numOutputs, totalEvals;
evalTh1Proc_t	func;

/*	return NULL;	/* return null to force non-optimized evaluation */

	standardFut = th1CheckFut (futH, iomask, &numInputs, &numOutputs);
	if (standardFut != 1) {
		return (NULL);			/* this fut can not be cached */
	}

	totalEvals = numOutputs * nEvals;
	
	if (totalEvals < TH1_MIN_EVALS) {					/* verify sufficient # of evaluations */
		if ((ifmt != FMT_QD) && (ofmt != FMT_QD)) {		/* still need to use iQDoQD for QD */
			return (NULL);
		}
	}

	switch (evalDataType) {
	case KCM_UBYTE:
		switch (numInputs) {	
		case 3:
			switch (numOutputs) {
				case 1:
					func = evalTh1i3o1d8;
					break;

				case 2:
					func = evalTh1i3o2d8;
					break;

				case 3:
					func = evalTh1i3o3d8;
				
		#if defined (KPMAC)
					if (ifmt == FMT_QD) {
						if (ofmt == FMT_QD) {
							func = evalTh1iQDoQD;
						}
						else {
							func = evalTh1iQDo3;
						}
					}
					else {
						if (ofmt == FMT_QD) {
							func = evalTh1i3oQD;
						}
						else {
		#endif
							if ((ifmt == FMT_BIGENDIAN24) && (ofmt == FMT_BIGENDIAN24)) {
								func = evalTh1iB24oB24;
							}
							else {
								if ((ifmt == FMT_LITTLEENDIAN24) && (ofmt == FMT_LITTLEENDIAN24)) {
									func = evalTh1iL24oL24;
								}
							}
		#if defined (KPMAC)
						}
					}
		#endif
					break;

				case 4:
					func = evalTh1i3o4d8;
					break;	  

				case 5:
					func = evalTh1i3o5d8;
					break;

				case 6:
					func = evalTh1i3o6d8;
					break;

				case 7:
					func = evalTh1i3o7d8;
					break;

				case 8:
					func = evalTh1i3o8d8;
					break;

				default:
					func = NULL;
					break;
			}
			
			break;
			
		case 4:
			switch (numOutputs) {
				case 1:
					func = evalTh1i4o1d8;
					break;

				case 2:
					func = evalTh1i4o2d8;
					break;

				case 3:
					func = evalTh1i4o3d8;

		#if defined (KPMAC)
					if (ofmt == FMT_QD) {
						func = evalTh1i4o3QD;
					}
		#endif
					break;

				case 4:
					func = evalTh1i4o4d8;

					if ((ifmt == FMT_BIGENDIAN32) && (ofmt == FMT_BIGENDIAN32)) {
						func = evalTh1iB32oB32;
					}
					else {
						if ((ifmt == FMT_LITTLEENDIAN32) && (ofmt == FMT_LITTLEENDIAN32)) {
							func = evalTh1iL32oL32;
						}
					}
					
					break;	  

				default:
					func = NULL;
					break;
			}
			break;

			default:
				func = NULL;
				break;
		}
		break;

	case KCM_USHORT_12:
	case KCM_USHORT:
		switch (numInputs) {
		case 3:
			switch (numOutputs) {
				case 1:
					func = evalTh1i3o1d16;
					break;

				case 2:
					func = evalTh1i3o2d16;
					break;

				case 3:
					func = evalTh1i3o3d16;
					break;

				case 4:
					func = evalTh1i3o4d16;
					break;	  

				case 5:
					func = evalTh1i3o5d16;
					break;

				case 6:
					func = evalTh1i3o6d16;
					break;

				case 7:
					func = evalTh1i3o7d16;
					break;

				case 8:
					func = evalTh1i3o8d16;
					break;

				default:
					func = NULL;
					break;
			}
			break;
			
		case 4:
			switch (numOutputs) {
				case 1:
					func = evalTh1i4o1d16;
					break;

				case 2:
					func = evalTh1i4o2d16;
					break;

				case 3:
					func = evalTh1i4o3d16;
					break;

				case 4:
					func = evalTh1i4o4d16;
					break;	  

				default:
					func = NULL;
					break;
			}
			break;

		default:
			func = NULL;
			break;
		}
		break;

	default:
		func = NULL;
		break;
	}

	return (func);
}


/******************************************************************************/
/* See if this fut is compatible with the cache.
   Returns 1 if it can, also returns # of inputs and outputs.
   Check for the following:
	1) Trilinear or default interpolation order
	2) Shared itbls
*/

static KpUInt32_t
	th1CheckFut (	KpHandle_t	futH,
					KpInt32_t	iomask,
					KpUInt32_p	numInputsP,
					KpUInt32_p	numOutputsP)
{
KpInt32_t 	imask, omask, i, nIn = 0, o, nOut;
fut_chan_ptr_t	futChan;
fut_ptr_t		futHdr;

	imask = (KpInt32_t)FUT_IMASK(iomask);
	omask = (KpInt32_t)FUT_OMASK(iomask);
	
	/* check the interpolation order */
	if ((FUT_ORDMASK(iomask) != FUT_LINEAR) && (FUT_ORDMASK(iomask) != FUT_DEFAULT)) {
		goto NotThis;		/* Wrong interpolation order. Don't use this caching method */
	}

	futHdr = KCP_DEREFERENCE(fut_ptr_t, futH);			/* get pointer to fut header */

	/* find number of output channels */
	for (nOut = 0, o = 0; o < FUT_NOCHAN; o++) {
		if (omask & FUT_BIT(o)) {
			for (nIn = 0, i = 0; i < FUT_NICHAN; i++) {	/* make sure input tables are shared */
				if (imask & FUT_BIT(i)) {
				
					futChan = KCP_DEREFERENCE(fut_chan_ptr_t, futHdr->chanHandle[o]);	/* get pointer to chan */

					if (futChan->itblHandle[i] != futHdr->itblHandle[i]) {
						goto NotThis;		/* Input tables not shared. Don't use this caching method */
					}
					nIn++;
				}
			}
			nOut++;
		}
	}

	*numInputsP = nIn;
	*numOutputsP = nOut;
	
	return 1;		/* use this caching method */


NotThis:
	return 0;		/* do not use this caching method */
}


/******************************************************************************/

PTErr_t
	kcpCheckTh1Cache (	KpHandle_t	futH,
						KpInt32_t	iomask,
						KpInt32_t	nEvals,
						KpUInt32_t	ifmt,
						KpUInt32_t	ofmt,
						evalControl_p	evalControlP)
{
th1Cache_p	th1Cache = evalControlP->th1CacheP;
KpUInt32_t	ioDataMask, imask, omask;
fut_ptr_t	futHdr;

	/* get the function to use */
	evalControlP->evalFunc = 
		getTh1EvalFuncOpt (futH, iomask, 
				nEvals, ifmt, ofmt, 
				evalControlP->evalDataTypeI);	

	if (evalControlP->evalFunc != NULL) {
		evalControlP->optimizedEval = 1;
	}
	else {
		evalControlP->optimizedEval = 0;

		/* use general eval function */
		evalControlP->evalFunc = 
			getTh1EvalFuncGen ();	
	}
	
	imask = (KpInt32_t)FUT_IMASK(iomask);
	omask = (KpInt32_t)FUT_OMASK(iomask);
	ioDataMask = FUT_OUT(omask) | FUT_IN(imask);	/* strip all but I/O */

	futHdr = KCP_DEREFERENCE(fut_ptr_t, futH);				/* get pointer to fut header */

	/* If the cached futRefNum, futModNum, and iomask match the new fut, the cache is valid. */
	if ((th1Cache->futRefNum == futHdr->refNum)
	 && (th1Cache->futModNum == futHdr->modNum)
	 && (th1Cache->dataSizeI == evalControlP->evalDataTypeI)
	 && (th1Cache->dataSizeO == evalControlP->evalDataTypeO)
	 && (th1Cache->optimizedEval == evalControlP->optimizedEval)
	 && (th1Cache->iomask == ioDataMask)
	 && (th1Cache->inputLuts.H != NULL)) {
		return KCP_SUCCESS;		/* cache is valid */
	}

	return KCP_FAILURE;		/* cache is not valid */
}


/******************************************************************************/

PTErr_t
	kcpInitTh1Cache (	fut_ptr_t	fut,
						KpInt32_t	iomask,
						evalControl_p	evalControlP)
{

th1Cache_p			th1Cache = evalControlP->th1CacheP;
KpInt32_t			theSizef, i, dimx, dimy, dimz, numInputs, nOutputChans, interleaveFactor;
KpUInt32_t			ioDataMask, imask, omask, index;
fut_itbl_ptr_t		iTblsP [TH1_MAX_INPUT_VARS];
fut_chan_ptr_t		aChan;
fut_gtbl_ptr_t		aGTbl = NULL;
fut_otbldat_ptr_t	oTblsP [TH1_MAX_OUTPUT_CHANS];
fut_gtbldat_ptr_t	gTblsP [TH1_MAX_OUTPUT_CHANS];
PTErr_t				PTErr;

	th1Cache->cachedFut = fut;
	
	imask = (KpInt32_t)FUT_IMASK(iomask);
	omask = (KpInt32_t)FUT_OMASK(iomask);
	ioDataMask = FUT_OUT(omask) | FUT_IN(imask);	/* strip all but I/O */

	/* Update cache state info */
	th1Cache->futRefNum = fut->refNum;
	th1Cache->futModNum = fut->modNum;
	th1Cache->dataSizeI = evalControlP->evalDataTypeI;
	th1Cache->dataSizeO = evalControlP->evalDataTypeO;
	th1Cache->optimizedEval = evalControlP->optimizedEval;
	th1Cache->iomask = ioDataMask;
	
	/* Find number of input variables */
	numInputs = 0;
	for (i = 0; i < TH1_MAX_INPUT_VARS ; i++) {
		if (imask & FUT_BIT(i)) {
			iTblsP[numInputs] = fut->itbl[i];		/* make a list of input tables */
			numInputs++;
		}
	}
	th1Cache->numInputs = numInputs;

	/* Find number of output channels */
	nOutputChans = 0;
	for (i = 0; i < TH1_MAX_OUTPUT_CHANS; i++) {
		if (omask & FUT_BIT(i)) {
			aChan = fut->chan[i];	/* this output cahnnel is being evaluated */
			
			gTblsP[nOutputChans] = aChan->gtbl->tbl;	/* make a list of gtbls */
			oTblsP[nOutputChans] = aChan->otbl->tbl;	/* and otbls */

			aGTbl = aChan->gtbl;			/* shared input tables, so any gtbl will do */

			nOutputChans++;
		}
	}
	th1Cache->numOutputs = nOutputChans;

	if (evalControlP->optimizedEval != 1) {
		interleaveFactor = 1;				/* using fut gtbls */
		th1FreeCacheLutMem (th1Cache);		/* so free lookup table memory */
		PTErr = KCP_SUCCESS;
	}
	
	else {	/* optimized evaluation, set up input, grid, and output tables */
		interleaveFactor = nOutputChans;		/* this many output channels are interleaved */

		PTErr = kcpInitTh1Luts (numInputs, nOutputChans, aGTbl, iTblsP, gTblsP, oTblsP, th1Cache);
		if (PTErr == KCP_NO_MEMORY) {
			/* switch to general eval function */
			evalControlP->evalFunc = 
				getTh1EvalFuncGen ();	

			if (evalControlP->evalFunc == NULL) {
				return KCP_INVAL_EVAL;
			}

			evalControlP->optimizedEval = 0;				/* not optimized */
			interleaveFactor = 1;				/* using fut gtbls */
			PTErr = KCP_SUCCESS;
		}
	}

	/* set up offsets in grid */
	switch (numInputs) {
	case 1:
		dimx = 0;	/* not used */
		dimy = 0;
		dimz = 0;
		break;

	case 2:
		dimx = 0;	/* not used */
		dimy = 0;
		dimz = aGTbl->size[1];
		break;

	case 3:
		dimx = 0;	/* not used */
		dimy = aGTbl->size[1];
		dimz = aGTbl->size[2];
		break;

	case 4:
		dimx = aGTbl->size[1];
		dimy = aGTbl->size[2];
		dimz = aGTbl->size[3];
		break;

	default:
		return (KCP_NOT_COMPLETE);
	}		
		
	th1Cache->gridOffsets[0] = 0;														/* offset to 0000 */
	th1Cache->gridOffsets[1] = 1;														/* offset to 0001 */
	th1Cache->gridOffsets[2] = dimz;													/* offset to 0010 */
	th1Cache->gridOffsets[3] = th1Cache->gridOffsets[2] + 1;							/* offset to 0011 */
	th1Cache->gridOffsets[4] = dimy * th1Cache->gridOffsets[2];							/* offset to 0100 */
	th1Cache->gridOffsets[5] = th1Cache->gridOffsets[4] + 1;							/* offset to 0101 */
	th1Cache->gridOffsets[6] = th1Cache->gridOffsets[4] + th1Cache->gridOffsets[2];		/* offset to 0110 */
	th1Cache->gridOffsets[7] = th1Cache->gridOffsets[6] + 1;							/* offset to 0111 */
	th1Cache->gridOffsets[8] = dimx * th1Cache->gridOffsets[4];							/* offset to 1000 */
	th1Cache->gridOffsets[9] = th1Cache->gridOffsets[8] + 1;							/* offset to 1001 */
	th1Cache->gridOffsets[10] = th1Cache->gridOffsets[8] + th1Cache->gridOffsets[2];	/* offset to 1010 */
	th1Cache->gridOffsets[11] = th1Cache->gridOffsets[10] + 1;							/* offset to 1011 */
	th1Cache->gridOffsets[12] = th1Cache->gridOffsets[8] + th1Cache->gridOffsets[4];	/* offset to 1100 */
	th1Cache->gridOffsets[13] = th1Cache->gridOffsets[12] + 1;							/* offset to 1101 */
	th1Cache->gridOffsets[14] = th1Cache->gridOffsets[8] + th1Cache->gridOffsets[4] + th1Cache->gridOffsets[2];	/* offset to 1110 */
	th1Cache->gridOffsets[15] = th1Cache->gridOffsets[14] + 1;							/* offset to 1111 */

	theSizef = interleaveFactor * sizeof (ecGridData_t);
	for (i = 0; i < TH1_NUM_OFFSETS; i++) {
		th1Cache->gridOffsets[i] *= theSizef;	/* adjust for # and size of channels in interleaved grid tables */
	}

	th1Cache->pentahedron = pentahedronBase;	/* initialize pentahedron finder */
	
	for (i = 0; i < TH1_4D_PENTAHEDROA; i++) {
		index = bceiBase[i] -1;
		th1Cache->finder[i].tvert1 = th1Cache->gridOffsets[index];	/* grid offset to bcei corner */
		index = dfgjkmBase[i] -1;
		th1Cache->finder[i].tvert2 = th1Cache->gridOffsets[index];	/* grid offset to dfgjkm corner */
		index = hlnoBase[i] -1;
		th1Cache->finder[i].tvert3 = th1Cache->gridOffsets[index];	/* grid offset to hlno corner */
		th1Cache->finder[i].tvert4 = th1Cache->gridOffsets[15];			/* grid offset to bcei corner */
		th1Cache->finder[i].dx = dxBase[i];							/* dx multiplier */
		th1Cache->finder[i].dy = dyBase[i];							/* dy multiplier */
		th1Cache->finder[i].dz = dzBase[i];							/* dz multiplier */
		th1Cache->finder[i].dt = dtBase[i];							/* dt multiplier */
	}
	
	return PTErr;
}


/* set up input, grid, and output lookup tables for optimized evaluation */

static PTErr_t
	kcpInitTh1Luts (	KpInt32_t			numInputs,
						KpInt32_t			nOutputChans,
						fut_gtbl_ptr_t		aGTbl,
						fut_itbl_ptr_t FAR*	iTblsP,
						fut_gtbldat_ptr_t FAR*	gTblsP,
						fut_otbldat_ptr_t FAR*	oTblsP,
						th1Cache_p			th1Cache)
{
KpInt32_t			theSizef, i, j, inputTableEntries, srcITblSize, gridTblSize;
KpUInt32_t			gridTblBytes, totalInputLutBytes, totalGridTableBytes, totalOutputLutBytes;
fut_itbl_ptr_t		theITbl;
fut_itbldat_ptr_t	iTblDat, srcITblDat;
ecItbl_p			iLut;
ecGridData_p		interleavedGridP;
ecGridData_t		tmp;
evalOLutPtr_t		oLut0;
PTErr_t				PTErr = KCP_SUCCESS;

	/* Input Tables */
	switch (th1Cache->dataSizeI) {
	case KCM_UBYTE:		
		inputTableEntries = FUT_INPTBL_ENT;
		break;
		
	case KCM_USHORT_12:		
		inputTableEntries = FUT_INPTBL_ENT2;
		break;
		
	case KCM_USHORT:		
		inputTableEntries = FUT_INPTBL_ENT3;
		break;
		
	default:
		goto ErrOut1;
	}
	th1Cache->inputTableEntries = inputTableEntries;

	totalInputLutBytes = inputTableEntries * sizeof (ecItbl_t) * numInputs;	/* size needed for input tables */

	allocEcMem (&th1Cache->inputLuts, totalInputLutBytes);	/* allocate necessary memory for the input tables */
	if (th1Cache->inputLuts.H == NULL) {
		goto ErrOut2;
	}
	
	/* set up the input table for each variable
	/* this is done in 2 steps:
	/* expand (if necessary) the current input table to the size needed for the input data
	/* convert that table into a lut for evaluation
	 */
	
	theSizef = nOutputChans * sizeof (ecGridData_t);
	for (i = numInputs-1; i >= 0 ; i--) {
	
		theITbl = iTblsP[i];						/* input table to convert */
		iLut = (ecItbl_p)th1Cache->inputLuts.P + (i * th1Cache->inputTableEntries);	/* converted table */
		
		if (inputTableEntries == FUT_INPTBL_ENT) {	/* convert 256 entry table  */
			iTblDat = theITbl->tbl;
		}
		else {		/* need 4096 or 65536 entry table */
			if (theITbl->tblFlag == 0) {			/* use largest table available */
				srcITblDat = theITbl->tbl;
				srcITblSize = FUT_INPTBL_ENT;
			}
			else {
				srcITblDat = theITbl->tbl2;
				srcITblSize = FUT_INPTBL_ENT2;
			}

			if (srcITblSize == inputTableEntries) {
				iTblDat = srcITblDat;				/* already the right size */
			}
			else {	/* expand to needed size */
					/* because an entry of the eval lut is larger than an entry of the input table data, */
					/* the end of the eval lut can be used as temporary memory to expand the input table data. */
				
				iTblDat = (fut_itbldat_ptr_t)(iLut + inputTableEntries);	/* end of lut */
				iTblDat -= inputTableEntries;								/* back up to start of expanded table */

				expandITblMEP (srcITblDat, srcITblSize, iTblDat, inputTableEntries);
			}			
		}

		th1Itbl2InLut (iTblDat, inputTableEntries, theSizef, iLut);		/* convert the input table to an eval lut */
		
		theSizef *= theITbl->size;
	}

	/* Grid Tables */
	gridTblBytes = aGTbl->tbl_size;

	/* is memory already allocated and the right size for the interleaved grid tables? */
	gridTblSize = gridTblBytes / sizeof (fut_gtbldat_t);
	totalGridTableBytes = gridTblSize * nOutputChans * sizeof (ecGridData_t);	/* size needed for this fut's cache */

	allocEcMem (&th1Cache->gridLuts, totalGridTableBytes);	/* allocate necessary memory for the input tables */
	if (th1Cache->gridLuts.H == NULL) {
		goto ErrOut2;
	}
	
	/* interleave the grid tables */
	interleavedGridP = th1Cache->gridLuts.P;
	for (i = 0; i < gridTblSize; i++) {
		for (j = 0; j < nOutputChans; j++) {
			tmp = *gTblsP[j]++;
			*interleavedGridP++ = tmp;
		}
	}
	
	/* Output Tables */
	if (th1Cache->dataSizeO == KCM_UBYTE)  {		/* 8 bit data  */
		totalOutputLutBytes = sizeof(KpUInt8_t);
	}
	else {
		totalOutputLutBytes = sizeof(KpUInt16_t);
	}

	totalOutputLutBytes *= FUT_OUTTBL_ENT * nOutputChans;		/* size needed for this fut's cache */

	allocEcMem (&th1Cache->outputLuts, totalOutputLutBytes);	/* allocate necessary memory for the input tables */
	if (th1Cache->outputLuts.H == NULL) {
		goto ErrOut2;
	}	

	/* set up the output table for each channel */
	oLut0.p8 = (KpUInt8_p)th1Cache->outputLuts.P;
	for (i = 0; i < nOutputChans; i++) {
		switch (th1Cache->dataSizeO) {
		case KCM_UBYTE:		
			th1Otbl2Out8Lut (oTblsP[i], &oLut0.p8[i * FUT_OUTTBL_ENT]);
			break;
			
		case KCM_USHORT_12:		
			th1Otbl2Out12Lut (oTblsP[i], &oLut0.p16[i * FUT_OUTTBL_ENT]);
			break;
			
		case KCM_USHORT:		
			th1Otbl2Out16Lut (oTblsP[i], &oLut0.p16[i * FUT_OUTTBL_ENT]);
			break;
			
		default:
			goto ErrOut1;
		}
	}

	return PTErr;


ErrOut1:
	PTErr = KCP_INVAL_EVAL;
	goto ErrOut;
	
ErrOut2:	
	PTErr = KCP_NO_MEMORY;

ErrOut:	
	th1FreeCacheLutMem (th1Cache);		/* free whatever may have been allocated */
	return PTErr;
}


/******************************************************************************/

/* Expand an input table using the Map End Points method */
/* source and destination table sizes when multiplied MUST fit a KpUInt32_t */

static void
	expandITblMEP (	fut_itbldat_ptr_t	srcITblDat,
					KpUInt32_t			srcITblSize,
					fut_itbldat_ptr_t	dstITblDat,
					KpUInt32_t			dstITblSize)
{
KpInt32_t			interp, fracHalf;
KpUInt32_t			sIMax, dIMax, fracBits, fracOne, fracMask;
KpUInt32_t			i, multMax, index, sPosition;
fut_itbldat_ptr_t	dstPtr;
fut_itbldat_t		base, next, interpData;

	/* use linear interpolation for table expansion */
	dstPtr = dstITblDat;
	sIMax = srcITblSize -1;	/* maximum index for source table */
	dIMax = dstITblSize -1;	/* maximum index for destination table */
	
	multMax = sIMax * dIMax;	/* calc maximum # of fractional bits, using KpUInt32_t */

	for (fracBits = 0; (multMax & 0x80000000) == 0; multMax <<= 1, fracBits++) {}

	fracOne = 1 << fracBits;	/* 1.0 with this # of fractional bits */
	fracHalf = fracOne >> 1;	/* value for rounding */
	fracMask = fracOne -1;		/* fractional bits mask */

	sIMax <<= fracBits;			/* scale up by # of fractional bits */
		
	for (i = 0; i < dstITblSize; i++) {
		sPosition = (i * sIMax) / dIMax;
		index = sPosition >> fracBits;
		interp = sPosition & fracMask;
		
		base = srcITblDat[index];		/* base table data */
		next = srcITblDat[index +1];	/* next table data */
        interpData = base + ((((next - base) * interp) + fracHalf) >> fracBits);

		dstPtr[i] = interpData;
	}
}


/******************************************************************************/

/* Convert fut itbl to input table in special format */

static void
	th1Itbl2InLut (	fut_itbldat_ptr_t	futLut,
					KpInt32_t			nEntries,
					KpInt32_t			sizef,
					ecItbl_p			evalLut)
{
KpInt32_t	i, index, v;

	/* Convert the itbl value to convenient values. */
	for (i = 0; i < nEntries; i++) {
		v = futLut[i];						/* get fut lut value */
		
		index = v >> FUT_INP_FRACBITS;		/* get fut integer index value */
		evalLut[i].index = index * sizef;	/* position index for offset into fut grid */

#if defined (TH1_FP_GRID_DATA)
		evalLut[i].frac = (ecInterp_t)(v & 0xffff)/65535.0;	 /* get fut fractional value */
#else
		{
		KpUInt32_t	frac;
			frac = v & 0xffff;	 				/* get fut fractional value */
			evalLut[i].frac = frac;					/* in 2 steps to avoid sign extension */
		}
#endif
	}
}


/******************************************************************************/

/* convert a 12 bit lut to an 8 bit lut */

static void
	th1Otbl2Out8Lut (	fut_otbldat_ptr_t	oTbl,
						KpUInt8_p			outLut)
{
KpInt32_t	i;

	if (oTbl != FUT_NULL_OTBLDAT) {
		for (i = 0; i < FUT_OUTTBL_ENT; i++) {
			outLut[i] = (u_int8) FUT_OTBL_NINT (oTbl[i]);
		}
	}
	else {
		for (i = 0; i < FUT_OUTTBL_ENT; i++) {
			outLut[i] = (u_int8) FUT_OTBL_NINT (i);
		}
	}
}


/******************************************************************************/

/* convert a 12 bit lut to an 12 bit lut */

static void
	th1Otbl2Out12Lut (	fut_otbldat_ptr_t	oTbl,
						KpUInt16_p			outLut)
{
KpInt32_t	i, oTblData;

	for (i = 0; i < FUT_OUTTBL_ENT; i++) {
		if (oTbl != FUT_NULL_OTBLDAT) {
			oTblData = oTbl[i];				/* just copy */
		}
		else {
			oTblData = i;
		}
		
		KCP_SCALE_OTBL_DATA(oTblData, KCP_MAX_12BIT)

		outLut[i] = (KpUInt16_t) oTblData;
	}
}


/******************************************************************************/
/*    convert a 12 bit output lut to a 16 bit output (evaluation) lut         */

static void
	th1Otbl2Out16Lut (	fut_otbldat_ptr_t	oTbl,
						KpUInt16_p			outLut)
{
KpInt32_t	i, oTblData;

	for (i = 0; i < FUT_OUTTBL_ENT; i++) {
		if (oTbl != FUT_NULL_OTBLDAT) {
			oTblData = oTbl[i];
		}
		else {
			oTblData = i;
		}
		
		KCP_SCALE_OTBL_DATA(oTblData, KCP_MAX_16BIT)
		
		outLut[i] = (KpUInt16_t) oTblData;
	}
}


/******************************************************************************/
/* Allocate memory for all evaluation cache buffers */

th1Cache_p
	th1AllocCache (	KpHandle_t FAR* cacheH)
{
th1Cache_h newTh1CacheH;
th1Cache_p th1Cache;

	if (*cacheH == NULL) {				/* set up the evaluation cache */
		newTh1CacheH = (th1Cache_h) allocBufferHandle (sizeof(th1Cache_t));
		if (newTh1CacheH == NULL) {
			return NULL;
		}

		th1Cache = (th1Cache_p) lockBuffer ((fut_handle)newTh1CacheH);
	 	th1Cache->th1CacheSvH = newTh1CacheH;

		th1Cache->futRefNum = 0;					/* invalidate the cache */

		nullEcMem (&th1Cache->inputLuts);			/* null current input */
		nullEcMem (&th1Cache->gridLuts);			/* grid */
		nullEcMem (&th1Cache->outputLuts);			/* and output lookup table memory */

		th1Cache->inputTableEntries = 0;			

		unlockBuffer ((fut_handle)newTh1CacheH);	/* return unlocked */

		*cacheH = (KpHandle_t)newTh1CacheH;
	}
	
	th1Cache = th1LockCache ((th1Cache_p FAR*)*cacheH);	/* get the cache */

	return th1Cache;
}


/* allocate necessary memory for the input tables */
static void
	allocEcMem (ecMem_p		theCacheMemP,
				KpInt32_t	bytesNeeded)
{
		/* is memory already allocated and the right size for the input tables? */
	if (theCacheMemP->bytes != bytesNeeded) {
		freeEcMem (theCacheMemP);							/* free current allocation */
		
		theCacheMemP->H = allocBufferHandle (bytesNeeded);	/* get new memory */
		if (theCacheMemP->H != NULL) {
			theCacheMemP->bytes = bytesNeeded;
			theCacheMemP->P = (ecItbl_p)lockBuffer (theCacheMemP->H);
		}
	}
}


/******************************************************************************/
/* Deallocate all allocated memory and null out handles. */
/* Note: will deallocate properly if only some of the memory got allocated. */

void
	th1FreeCache (	KpHandle_t	th1CacheH)
{
th1Cache_p th1Cache;

	if (th1CacheH != NULL) {
		th1Cache = th1LockCache ((th1Cache_h)th1CacheH);

		th1FreeCacheLutMem (th1Cache);			/* free lookup table memory */

		freeBufferPtr ((fut_generic_ptr_t) th1Cache);	/* free the cache data structure */
	}
}


static void
	th1FreeCacheLutMem (	th1Cache_p th1Cache)
{
	freeEcMem (&th1Cache->inputLuts);	/* free current input */
	freeEcMem (&th1Cache->gridLuts);	/* grid */
	freeEcMem (&th1Cache->outputLuts);	/* and output lookup table memory */
}


/* free a cache's memory allocation */
static void
	freeEcMem (ecMem_p	theCacheMemP)
{
	if (theCacheMemP->H != NULL) {
		freeBuffer (theCacheMemP->H);
	}
	nullEcMem (theCacheMemP);
}


static void
	nullEcMem (ecMem_p	theCacheMemP)
{
	theCacheMemP->H = NULL;
	theCacheMemP->P = NULL;
	theCacheMemP->bytes = 0;
}


/******************************************************************************/

th1Cache_p
	th1LockCache (	th1Cache_h	theCacheH)
{
th1Cache_p th1Cache;

	if (theCacheH == NULL) {
		return NULL;		 /* invalid cache handle */
	}

	/* Lock all the handles and set static pointers */
	th1Cache = (th1Cache_p) lockBuffer ((fut_handle)theCacheH);

	/* need to re-initialize? */
	if (th1Cache->inputLuts.P == NULL) {

		th1Cache->inputLuts.P = (ecMem_p)lockBuffer (th1Cache->inputLuts.H);		
		th1Cache->gridLuts.P = (ecMem_p)lockBuffer (th1Cache->gridLuts.H);
		th1Cache->outputLuts.P = (ecMem_p)lockBuffer (th1Cache->outputLuts.H);
	}

	return th1Cache;	/* success */
}


/******************************************************************************/
/* Unlock all the handles and set static pointers to NULL */

void
	th1UnLockCache (	th1Cache_p th1Cache)
{
th1Cache_h	theCacheH;

	if (th1Cache != NULL) {
		unlockEcMem (&th1Cache->inputLuts);		/* unlock input */
		unlockEcMem (&th1Cache->gridLuts);		/* grid */
		unlockEcMem (&th1Cache->outputLuts);	/* and output lookup table memory */
		
		theCacheH = th1Cache->th1CacheSvH;
		(void) unlockBuffer ((KpHandle_t)theCacheH);
	}
}


/* unlock a cache's memory allocation */
static void
	unlockEcMem (ecMem_p	theCacheMemP)
{
	(void) unlockBuffer (theCacheMemP->H);
	theCacheMemP->P = NULL;
}


#endif /* #if defined (KCP_EVAL_TH1) */

