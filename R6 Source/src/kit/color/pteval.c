/*
 * @(#)pteval.c	2.45 97/12/22

	Contains:	Handle function evaluations using PTs

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1997 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

		 <3>	 1/11/94	sek		Cleaned up warnings
		 <2>	12/ 7/93	htr		Added checks for valid ptrs in PTEvalDT
			12/7/93		gbp		call PT_eval_cteDT instead of PT_eval_cte
 */

#include "attrib.h"
#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "kcptmgr.h"
#include "kcpmgru.h"
#include "kcpfut.h"
#include "fut.h"
#include "fut_util.h"

#if defined (KCP_ACCEL)
#include "ctelib.h"

#define	FASTER_IN_SW 8000
#endif

static KpUInt32_t getImageSizeAndStart (KpInt32_t, KpInt32_t, KpInt32_t, KpInt32_t, PTImgAddr_t, PTImgAddr_t*);
static PTErr_t setupEvalList (KpUInt32_t, futEvalInfo_p, PTEvalDTPB_p, KpUInt32_p);
static PTErr_t getDataBytes (KpInt32_t,	KpUInt32_p);


PTErr_t
	 PTEval (	PTRefNum_t		PTRefNum,
				PTEvalPB_p		evalDef,
				PTEvalTypes_t	evalID,
				int32			devNum,
				int32			aSync,
				opRefNum_p		opRefNum,
				PTProgress_t	progress)
{
PTEvalDTPB_t	evalDTDef;
PTCompDef_t 	imageInput[FUT_NICHAN];
PTCompDef_t 	imageOutput[FUT_NOCHAN];
KpInt32_t		i;
PTErr_t			PTErr;

	/* Check for valid PTEvalPB_p */
	if (Kp_IsBadReadPtr(evalDef, (KpUInt32_t)sizeof(*evalDef))) {
		return (KCP_BAD_PTR);
	}

	if (Kp_IsBadReadPtr(evalDef->input,
						evalDef->nInputs * (KpUInt32_t)sizeof(*evalDef->input))) {
		return (KCP_BAD_PTR);
	}

	if (Kp_IsBadReadPtr(evalDef->output,
					evalDef->nOutputs * (KpUInt32_t)sizeof(*evalDef->output))) {
		return (KCP_BAD_PTR);
	}

	evalDTDef.nPels = evalDef->nPels;
	evalDTDef.nLines = evalDef->nLines;
	evalDTDef.nInputs = evalDef->nInputs;
	evalDTDef.input = imageInput;
	evalDTDef.dataTypeI = KCM_UBYTE;
	
	if (evalDTDef.nInputs > FUT_NICHAN) {
		return (KCP_INVAL_IN_VAR);
	}
	
	for (i = 0; i < evalDTDef.nInputs; i++) {
			evalDTDef.input[i] = evalDef->input[i];
	}
	evalDTDef.nOutputs = evalDef->nOutputs;
	evalDTDef.output = imageOutput;
	evalDTDef.dataTypeO = KCM_UBYTE;
	for (i = 0; i < evalDTDef.nOutputs; i++) {
			evalDTDef.output[i] = evalDef->output[i];
	}

	PTErr = PTEvalDT(PTRefNum, &evalDTDef, evalID, devNum, aSync, opRefNum, progress);
	return (PTErr);
}


PTErr_t
	PTEvalDT (	PTRefNum_t		PTRefNum,
				PTEvalDTPB_p	evalDef,
				PTEvalTypes_t	evalID,
				int32			devNum,
				int32			aSync,
				opRefNum_p		opRefNum,
				PTProgress_t	progress)
{
threadGlobals_p threadGlobalsP;
PTErr_t			errnum;
KcmHandle		PTData;
PTEvalDTPB_t	lEvalDef;
PTCompDef_t		thisInput[FUT_NICHAN], thisOutput[FUT_NOCHAN];
PTRefNum_t		PTList[MAX_PT_CHAIN_SIZE], aPTRefNum;
fut_ptr_t		futHdr;
futEvalInfo_t	futEvalList[MAX_PT_CHAIN_SIZE];
futEvalInfo_p	listStart;
KpInt32_t		theSerialCount, i1, i2, i3, i4, nFuts, PTcount, j, size;
KpUInt32_t		tempMemNeeded;
#if defined (KCP_ACCEL)
PTEvalTypes_t	evaluator;
KpInt32_t		i, output_var, numEvals;
KpInt8_t		strInSpace[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
KpInt32_t		attrSize, inspace;
#endif
PTImgAddr_t		addr, oAddr, startAddr;

	threadGlobalsP = KCMDloadGlobals();			/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		errnum = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut2;
	}

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum);		/* must be an active PT */

	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_SERIAL_PT)) {
#if defined (KCP_ACCEL)
		errnum = GetEval(evalID, &evaluator, threadGlobalsP->processGlobalsP->iGP->evalList);	/* get a evaluator */
		if (errnum == KCP_SUCCESS) {
#endif

/* Check for valid callback address... */
			if ((progress != NULL) && (Kp_IsBadCodePtr((OsiProcPtr_t)progress))) {
				errnum = KCP_BAD_CALLBACK_ADDR;
				goto ErrOut1;
			}

/*  and valid opRefNum address */
			if (Kp_IsBadWritePtr (opRefNum, (KpUInt32_t)sizeof(*opRefNum))) {
				errnum = KCP_BAD_PTR;
				goto ErrOut1;
			}

/*  and valid PTEvalDTPB_p */
			if (Kp_IsBadReadPtr(evalDef, (KpUInt32_t)sizeof(*evalDef))) {
				errnum = KCP_BAD_PTR;
				goto ErrOut1;
			}

			if (Kp_IsBadReadPtr(evalDef->input,
							evalDef->nInputs * (KpUInt32_t)sizeof(*evalDef->input))) {
				errnum = KCP_BAD_PTR;
				goto ErrOut1;
			}

			if (Kp_IsBadReadPtr(evalDef->output,
							evalDef->nOutputs * (KpUInt32_t)sizeof(*evalDef->output))) {
				errnum = KCP_BAD_PTR;
				goto ErrOut1;
			}

			/* Check all of the data input addresses for validity */
			for (j = 0; j < evalDef->nInputs; j++) {
				addr = evalDef->input[j].addr;
				if (addr != NULL) {
					size = getImageSizeAndStart (evalDef->nPels, evalDef->input[j].pelStride,
										evalDef->nLines, evalDef->input[j].lineStride,
										addr, &startAddr); 

					if (Kp_IsBadHugeReadPtr (startAddr, size)) {
						errnum = KCP_BAD_PTR;
						goto ErrOut1;
					}
				}
			}

			/* Check all of the data output addresses for validity */
			for (j = 0; j < evalDef->nOutputs; j++) {
				addr = evalDef->output[j].addr;
				if (addr != NULL) {
					size = getImageSizeAndStart (evalDef->nPels, evalDef->output[j].pelStride,
										evalDef->nLines, evalDef->output[j].lineStride,
										addr, &startAddr); 

					if (Kp_IsBadHugeWritePtr (startAddr, size)) {
						errnum = KCP_BAD_PTR;
						goto ErrOut1;
					}
				}
			}

			*opRefNum = 1;	/* use constant, since everything is synchronous now */

			for (i4 = 0; i4 < MAX_PT_CHAIN_SIZE; i4++) {
				futEvalList[i4].futP = NULL;			/* null addresses in case of error */
				futEvalList[i4].futH = NULL;
			}

			/* get the list of PTs which we must actually evaluate */
			errnum = resolvePTData (threadGlobalsP, PTRefNum, &theSerialCount, PTList);

			/* set up list of futs through which the image is evaluated */
			for (i4 = 0; i4 < theSerialCount; i4++) {
				aPTRefNum = PTList[i4];							/* get the PT reference number */
				PTData = getPTData(threadGlobalsP, aPTRefNum);	/* get the transform data */
				futHdr = KCP_DEREFERENCE(fut_ptr_t, PTData);

				futHdr->refNum = aPTRefNum;						/* insert unique id */

				futEvalList[i4].futH = PTData;					/* add HANDLE to list */
				futEvalList[i4].nOutputs = 0;					/* # outputs unknown */
				futEvalList[i4].evalMask = 0;					/* mask unknown */
			}
			
			futEvalList[theSerialCount -1].nOutputs = evalDef->nOutputs;	/* last one is known! */
			
			/* set up the local evaluation structures */
			lEvalDef.nPels = evalDef->nPels;
			lEvalDef.nLines = evalDef->nLines;
			lEvalDef.nInputs = evalDef->nInputs;
			lEvalDef.dataTypeI = evalDef->dataTypeI;
			lEvalDef.input = thisInput;
			for (i1 = 0; i1 < evalDef->nInputs; i1++) {
				lEvalDef.input[i1].pelStride = evalDef->input[i1].pelStride;
				lEvalDef.input[i1].lineStride = evalDef->input[i1].lineStride;
				lEvalDef.input[i1].addr = evalDef->input[i1].addr;
			} 
			lEvalDef.nOutputs = evalDef->nOutputs;
			lEvalDef.dataTypeO = evalDef->dataTypeO;
			lEvalDef.output = thisOutput;
			for (i1 = 0, i2 = 0; i1 < evalDef->nOutputs; i1++) {
				oAddr = evalDef->output[i1].addr;
				if (oAddr != NULL) {				/* pack to front */
					lEvalDef.output[i2].pelStride = evalDef->output[i1].pelStride;
					lEvalDef.output[i2].lineStride = evalDef->output[i1].lineStride;
					lEvalDef.output[i2].addr = oAddr;
				
					i2++;
				}
			} 

			/* initialize the evaluation list */
			errnum = setupEvalList (theSerialCount, futEvalList, evalDef, &tempMemNeeded);
			if (errnum != KCP_SUCCESS) {
				goto ErrOut1;
			}

			/* if temporary memory is not needed, */
			/* then evaluate a full image at a time until */
			/* the image has been processed through all futs */
			/* if temporary memory is needed, */
			/* then this level processes the image just once */
			/* and a lower level processes the image through all futs */
			if (tempMemNeeded == 0) {
				PTcount = theSerialCount;
			}
			else {
				PTcount = 1;
			}
			initProgressPasses (threadGlobalsP, PTcount, progress);

			/* process the image through each fut in the list */ 
			for (i2 = 0; i2 < PTcount; i2++) {

				/* set up the output data addresses */
				if (i2 == (PTcount -1)) {
					lEvalDef.nOutputs = evalDef->nOutputs;			/* last section, use actual output */
					for (i3 = 0; i3 < lEvalDef.nOutputs; i3++) {
						lEvalDef.output[i3].pelStride = evalDef->output[i3].pelStride;
						lEvalDef.output[i3].lineStride = evalDef->output[i3].lineStride;
						lEvalDef.output[i3].addr = evalDef->output[i3].addr;
					}
				}
				else {
					lEvalDef.nOutputs = futEvalList[i2].nOutputs;	/* outputs to evaluate this time */
				}
				
#if defined (KCP_ACCEL)
				/* if there are less than FASTER_IN_SW evaluations, it's faster to do it in software. */
				output_var = 0;
				for (i = 0; i < lEvalDef.nOutputs; i++) {
					if (lEvalDef.output[i].addr != NULL) {
						output_var++;
					}
				}
				
				numEvals = lEvalDef.nPels * lEvalDef.nLines * output_var;
				
				if ((numEvals < FASTER_IN_SW) || (tempMemNeeded == 1)) {
					evaluator = KCP_EVAL_SW;
				}
				
				switch (evaluator) {
				case KCP_EVAL_SW:		/* evaluate in software */
		
software_evaluation:
#endif

					/* serial PT? */
					if (tempMemNeeded == 1) {
						nFuts = theSerialCount;		/* this many to evaluate */
						listStart = futEvalList;
					}
					else {							/* just evaluate the next fut */
						nFuts = 1;
						listStart = &futEvalList[i2];
					}

					errnum = PTEvalSeq (threadGlobalsP, nFuts, listStart, &lEvalDef, progress);
					if (errnum != KCP_SUCCESS) {
						goto ErrOut1;
					}
	
#if defined (KCP_ACCEL)
					break;
	
				case KCP_EVAL_CTE:	/* evaluate using the NFE */
					aPTRefNum = PTList[i2];							/* get the PT reference number */

					attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
					errnum = PTGetAttribute(aPTRefNum, KCM_SPACE_IN, &attrSize, strInSpace);

					if (errnum != KCP_SUCCESS) {
						goto software_evaluation;
					}
					inspace = KpAtoi(strInSpace);
		/*			PTColorSpace_cte(inspace);	*/

					PTData = getPTData(threadGlobalsP, aPTRefNum);	/* get the transform data */

					errnum = PT_eval_cteDT (threadGlobalsP,
											PTData, aPTRefNum, &lEvalDef, devNum, progress, aSync);
											
					if ((errnum == KCP_CTE_GRID_TOO_BIG) || (errnum == KCP_CTE_NOT_ATTEMPTED)) {
						goto software_evaluation;
					}
					
					break;
	
				default:
					errnum = KCP_INVAL_EVAL;
	
					break;
				}
#else
				aSync += 0;  /* this is to humor the PC 32-bit compiler  */
				devNum += 0; /* these are unreferenced formal parameters */
				evalID += 0;
#endif
						
				/* output for this PT is input for next PT */
				i3 = lEvalDef.nOutputs;
				if (i3 > FUT_NICHAN) {
					i3 = FUT_NICHAN;	/* clamp at max */
				}
				lEvalDef.dataTypeI = lEvalDef.dataTypeO;

				lEvalDef.nInputs = i3;
				for (i3 = 0; i3 < lEvalDef.nInputs; i3++) {
					lEvalDef.input[i3].addr = lEvalDef.output[i3].addr;
					lEvalDef.input[i3].pelStride = lEvalDef.output[i3].pelStride;
					lEvalDef.input[i3].lineStride = lEvalDef.output[i3].lineStride;
				}
			}

#if defined (KCP_ACCEL)
		}
#endif
	}

ErrOut1:
	KCMDunloadGlobals();					/* Unlock this app's Globals */
ErrOut2:
	return (errnum);
}


/* set up the fut evaluation list */
/* Determine whether or not temporary memory is needed */
static PTErr_t
	setupEvalList (	KpUInt32_t		numPTs,
					futEvalInfo_p	futEvalList,
					PTEvalDTPB_p	evalDef,
					KpUInt32_p		tempMemNeeded)
{
PTErr_t		errnum = KCP_SUCCESS;
KpInt32_t	i1, i2, i3;
KpUInt32_t	finalOutputs = 0, nOutputs, maxOutputs, futmask = 0;
KpUInt32_t	imask, omask, futMaskBase, thisOmask, thisImask;
KpUInt32_t	sizeInData, sizeOutData;
fut_ptr_t		futHdr;
fut_chan_ptr_t	futChan;

	/* initialize fut I/O mask base */
	futMaskBase = FUT_ORDER (FUT_DEFAULT);
	if ((evalDef->dataTypeI == KCM_USHORT_12) || (evalDef->dataTypeO == KCM_USHORT_12)
		|| (evalDef->dataTypeI == KCM_USHORT) || (evalDef->dataTypeO == KCM_USHORT)
		|| (evalDef->dataTypeI == KCM_R10G10B10) || (evalDef->dataTypeO == KCM_R10G10B10)) {
		futMaskBase |= FUT_12BITS;
	}

	/* calculate final output mask */
	if ((evalDef->dataTypeO == KCM_USHORT_555) || (evalDef->dataTypeO == KCM_USHORT_565) || (evalDef->dataTypeO == KCM_R10G10B10)) {
		if (evalDef->input[0].addr != NULL) {
			futmask = FUT_OUT(FUT_BIT(0) | FUT_BIT(1) | FUT_BIT(2));
			finalOutputs = 3;
		}
	}
	else {
		futmask = 0;
		finalOutputs = 0;
		for (i1 = 0; i1 < evalDef->nOutputs; i1++) {
			if (evalDef->output[i1].addr != NULL) {
				futmask |= FUT_OUT(FUT_BIT(i1));
				finalOutputs++;
			}
		}
	}
	
	/* does the fut have those outputs? */
	futHdr = KCP_DEREFERENCE(fut_ptr_t, futEvalList[numPTs -1].futH);	/* get pointer to fut header */

	omask = futHdr->iomask.out;
	if ((omask & FUT_OMASK(futmask)) != FUT_OMASK(futmask)) {
		errnum = KCP_INVAL_EVAL;
		goto ErrOut;
	}

	nOutputs = finalOutputs;	/* int # outputs of fut at the end of the list */
	maxOutputs = 0;				/* none yet */
	
	for (i1 = (numPTs -1); i1 >= 0; i1--) {
		if (nOutputs > maxOutputs) {		/* remember max # outputs needed */
			maxOutputs = nOutputs;
		}

		/* get inputs required for this fut from outputs required from this fut */
		thisOmask = FUT_OMASK(futmask);				/* get the output mask */

		futHdr = KCP_DEREFERENCE(fut_ptr_t, futEvalList[i1].futH);	/* get pointer to fut header */

		thisImask = 0;
		
		for (i3 = 0; i3 < FUT_NOCHAN; i3++) {
			if (thisOmask & FUT_BIT(i3)) {			/* is this output channel needed? */

				futChan = KCP_DEREFERENCE(fut_chan_ptr_t, futHdr->chanHandle[i3]);	/* get pointer to chan */

				thisImask |= (KpUInt32_t)FUT_CHAN_IMASK(futChan);	/* include inputs required for this chan */
			}
		}

		futmask |= FUT_IMASK(thisImask) | futMaskBase;

		futEvalList[i1].nOutputs = nOutputs;	/* store # outputs for this fut */
		futEvalList[i1].evalMask = futmask;		/* and the evaluation I/O mask */
		
		/* set up outputs for previous fut */
/*		futmask = FUT_OUT(FUT_IMASK(futmask));*/	/* which are inputs to this fut */
		futmask = FUT_IMASK(futmask);
		futmask <<= 7;			/* do it this way because of MicroSoft bug! */
		futmask <<= 1;
		nOutputs = 0;
		for (i2 = FUT_OMASK(futmask); i2 != 0; i2 >>= 1) {	/* count # of outputs */
			if ((i2 & 1) != 0) {
				nOutputs++;
			}
		}		
	}

	/* calculate input mask for the first fut */
	if ((evalDef->dataTypeI == KCM_USHORT_555) || (evalDef->dataTypeI == KCM_USHORT_565) || (evalDef->dataTypeI == KCM_R10G10B10)) {
		if (evalDef->input[0].addr != NULL) {
			futmask = FUT_IN(FUT_BIT(0) | FUT_BIT(1) | FUT_BIT(2));
		}
	}
	else {
		futmask = 0;
		for (i1 = 0; i1 < evalDef->nInputs; i1++) {
			if (evalDef->input[i1].addr != NULL) {
				futmask |= FUT_IN(FUT_BIT(i1));
			}
		}
	}
	
	/* does the first fut have the required inputs? */
	futHdr = KCP_DEREFERENCE(fut_ptr_t, futEvalList[0].futH);	/* get pointer to fut header */
	imask = futHdr->iomask.in;
	if ((imask & FUT_IMASK(futmask)) != imask) {
		errnum = KCP_INVAL_EVAL;
		goto ErrOut;
	}

	/* Is temporary memory required? */
	/* If just one PT, then temporary data memory is not needed */
	/* if more than one PT (serial evaluation), then the number and size of the outputs */
	/* determines whether or not temporary memory is needed */
	
	errnum = getDataBytes (evalDef->dataTypeI, &sizeInData);	/* get input data size */
	if (errnum != KCP_SUCCESS) {
		goto ErrOut;
	}

	errnum = getDataBytes (evalDef->dataTypeO, &sizeOutData);	/* get output data size */
	if (errnum != KCP_SUCCESS) {
		goto ErrOut;
	}

	if (numPTs == 1) {
		(*tempMemNeeded) = 0;		/* not serial evaluation, no temporary data memory needed */
	}
	else {						/* serial evaluation */
								/* does this require temporary memory? */
		if ((maxOutputs > finalOutputs) || (sizeInData > sizeOutData)) {
			(*tempMemNeeded) = 1;				/* yes */
		}
		else {
			(*tempMemNeeded) = 0;				/* no */
		}
	}
	
ErrOut:
	return errnum;
}


/* returns the # of bytes per datum for a given data type */
static PTErr_t
	getDataBytes (	KpInt32_t	dataType,
					KpUInt32_p	dataTypeSize)
{
	switch (dataType) {
	case KCM_USHORT_555:
	case KCM_USHORT_565:
	case KCM_R10G10B10:
		(*dataTypeSize) = 0;	/* less than 1 byte */
		break;

	case KCM_UBYTE:
		(*dataTypeSize) = 1;	/* 1 byte per datum */
		break;

	case KCM_USHORT_12:
	case KCM_USHORT:
		(*dataTypeSize) = 2;	/* 2 bytes per datum */
		break;

	default:
		return KCP_INVAL_DATA_TYPE;
	}

	return (KCP_SUCCESS);
}


/* calculate the exact bytes required for a given image */
static KpUInt32_t
	getImageSizeAndStart (	KpInt32_t		nPels,
							KpInt32_t		pelStride,
							KpInt32_t		nLines,
							KpInt32_t		lineStride,
							PTImgAddr_t		addr,
							PTImgAddr_t*	startAddr)
{
KpInt32_t size;
 
	if (nPels == 1) {
		pelStride = 1;
	}

	if (nLines == 1) {
		lineStride = 1;
	}

	size = ((nLines -1) * lineStride) + ((nPels -1) * pelStride) +1;
	
	if (size < 0) {
		size = -size;
		*startAddr = (KpUInt8_p)addr - size;
	}
	else {
		*startAddr = addr;
	}

	return size;
}


#if defined(KCP_ACCEL)
PTErr_t
	PTEvalRdy (	opRefNum_t	opRefNum,
				int32_p		progress)
{
threadGlobals_p threadGlobalsP;
PTErr_t			PTErr;

/*	This stuff is here just to appease the compiler gods. */
KpUInt32_t	dummy;

	threadGlobalsP = KCMDloadGlobals();			/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		PTErr = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut1;
	}

	if(opRefNum)
		dummy = *progress;
		
	KCMDunloadGlobals();					/* Unlock this apps Globals */
	PTErr = KCP_SUCCESS;

ErrOut1:
	return(PTErr);
}


PTErr_t
	PTEvalCancel (	opRefNum_t opRefNum)
{
threadGlobals_p threadGlobalsP;
PTErr_t			PTErr;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		PTErr = KCP_NO_THREAD_GLOBAL_MEM;
		goto ErrOut1;
	}

	if (opRefNum) {}	/* This is here just to appease the compiler gods. */
	
	KCMDunloadGlobals ();					/* Unlock this apps Globals */
	PTErr = KCP_INVAL_OPREFNUM;

ErrOut1:
	return(PTErr);
}
#endif


