/*
 * @(#)doevalsw.c	2.71 97/12/22

	Contains:	This module contains most of the internals for
				ptevalsw.c and was split out so it could be
				included in a 32-bit evaluation DLL for Windows.
				It is intended to be portable to the Macintosh
					Created by PGT, January 2, 1992

	Written by:	The KCMS Team

 *********************************************************************
 *    COPYRIGHT (c) 1992-1997 Eastman Kodak Company
 *    As  an  unpublished  work pursuant to Title 17 of the United
 *    States Code.  All rights reserved.
 *********************************************************************
 */

#include "kcms_sys.h"
#include "fut.h"
#include "fut_util.h"
#include "kcptmgr.h"
#include "kcpmgru.h"
#include "kcpfut.h"
#include "kcpcache.h"
#if defined (KCP_THREAD_MP)
#include "sithread.h"
#endif

static PTErr_t evalImage (evalControl_p);
static KpUInt32_t format_analyze (imagePtr_p, KpInt32_p, KpUInt32_t);
static KpUInt32_t getEvalDataType (KpUInt32_t);

static formatFunc_t getFormatFuncI (KpUInt32_t, KpUInt32_t);
static formatFunc_t getFormatFuncO (KpUInt32_t, KpUInt32_t);


PTErr_t
	PTEvalSeq (	threadGlobals_p	threadGlobalsP,
					KpInt32_t		nFuts,
					futEvalInfo_p	futEvalList,
					PTEvalDTPB_p	evalDef,
					PTProgress_t	progress)
{
PTErr_t			errnum = KCP_SUCCESS, errnum1;
KpUInt32_t		futmask, ifmt, ofmt;
KpInt32_t		i1, i2, nInputs, nOutputs, numEvals, numWaitLoops, theTempPelStride, tasksStarted;
KpInt32_t		linesPerTask, mainLines, imageStride;
KpHandle_t		futH;
fut_ptr_t		fut;
evalControl_t	evalControl[KCP_MAX_PROCESSORS];
KpHandle_t		taskEvalCache[KCP_MAX_PROCESSORS];
evalControl_p	evalControlP;
th1Cache_h		saveH;
#if defined (KCP_MACPPC_MP)
OSStatus		theStatus;
KpUInt32_t		action, taskStatus;
#elif defined (KCP_THREAD_MP)
KpThread_t		thread[KCP_MAX_PROCESSORS];
#endif
KpInt32_t		taskCount = threadGlobalsP->processGlobalsP->iGP->numProcessors;

	/* Do not do anything if the input FuTs or evaluation parameters are NULL pointers */
	if ((nFuts == 0) || (nFuts > MAX_PT_CHAIN_SIZE) || (futEvalList == NULL) || (evalDef == NULL)) {
		return   KCP_INVAL_EVAL;
	}

	/* verify input and output data formats and set size of each component of the evaluation */
	evalControl[0].evalDataTypeI = getEvalDataType (evalDef->dataTypeI);
	evalControl[0].evalDataTypeO = getEvalDataType (evalDef->dataTypeO);

	if ((evalControl[0].evalDataTypeI == KCM_UNKNOWN) || (evalControl[0].evalDataTypeO == KCM_UNKNOWN)) {
		errnum = KCP_INVAL_DATA_TYPE;
		goto GetOut;
	}

	/* Initialize the evaluation control structure for the main task */
	evalControl[0].threadGlobalsP = threadGlobalsP;
	evalControl[0].imageLines = evalDef->nLines;
	evalControl[0].imagePels = evalDef->nPels;
	evalControl[0].nFuts = nFuts;
	evalControl[0].futEvalList = futEvalList;
	
	for (i1 = 0; i1 < FUT_NICHAN; i1++) {
		evalControl[0].inputData[i1].pI = NULL;
		evalControl[0].inPelStride[i1] = 0;
		evalControl[0].inLineStride[i1] = 0;
	}

	for (i1 = 0; i1 < FUT_NOCHAN; i1++) {
		evalControl[0].outputData[i1].pI = NULL;
		evalControl[0].outPelStride[i1] = 0;
		evalControl[0].outLineStride[i1] = 0;
	}

	/* set up input address/stride arrays */
	for (i1 = 0, nInputs = 0; i1 < evalDef->nInputs; i1++) {
		if (evalDef->input[i1].addr != NULL) {
			evalControl[0].inputData[nInputs].pI = evalDef->input[i1].addr;
			evalControl[0].inPelStride[nInputs] = evalDef->input[i1].pelStride;
			evalControl[0].inLineStride[nInputs] = evalDef->input[i1].lineStride;
			nInputs++;
		}
	}

	evalControl[0].nInputs = nInputs;

	/* set up output address/stride arrays */
	for (i1 = 0, nOutputs = 0; i1 < evalDef->nOutputs; i1++) {
		if (evalDef->output[i1].addr != NULL) {
			evalControl[0].outputData[nOutputs].pI = evalDef->output[i1].addr;
			evalControl[0].outPelStride[nOutputs] = evalDef->output[i1].pelStride;
			evalControl[0].outLineStride[nOutputs] = evalDef->output[i1].lineStride;
			nOutputs++;
		}
	}

	evalControl[0].nOutputs = nOutputs;

	/* initialize flags and set up evaluation caches if needed */		
	if ((nFuts == 1) &&		/* if just one fut (i.e. not serial) */
		  (((evalDef->dataTypeI == KCM_UBYTE) && (evalDef->dataTypeO == KCM_UBYTE))	/* and i/o precisions match */
		|| ((evalDef->dataTypeI == KCM_USHORT_12) && (evalDef->dataTypeO == KCM_USHORT))
		|| ((evalDef->dataTypeI == KCM_USHORT) && (evalDef->dataTypeO == KCM_USHORT_12))
		|| ((evalDef->dataTypeI == KCM_USHORT_12) && (evalDef->dataTypeO == KCM_USHORT_12))
		|| ((evalDef->dataTypeI == KCM_USHORT) && (evalDef->dataTypeO == KCM_USHORT)))) {
		
		evalControl[0].compatibleDataType = 1;		/* data already in format compatible with evaluation functions */

		ifmt = format_analyze (evalControl[0].inputData, evalControl[0].inPelStride, evalControl[0].evalDataTypeI);   
		ofmt = format_analyze (evalControl[0].outputData, evalControl[0].outPelStride, evalControl[0].evalDataTypeO); 

	   	numEvals = evalDef->nPels * evalDef->nLines;
	}
	else {
		evalControl[0].compatibleDataType = 0;		/* data is not in compatible format */

		/* set up temporary buffer pel strides */
		if ((evalControl[0].evalDataTypeI == KCM_UBYTE) && (evalControl[0].evalDataTypeO == KCM_UBYTE)) {
			theTempPelStride = 1;		/* 1 byte temporary data */
			evalControl[0].evalDataTypeI = KCM_UBYTE;
		}
		else {
			theTempPelStride = 2;		/* 2 byte temporary data */
			evalControl[0].evalDataTypeI = KCM_USHORT_12;
		}

		for (i1 = 0; i1 < FUT_NOCHAN; i1++) {
			evalControl[0].tempPelStride[i1] = theTempPelStride;
		}

		ifmt = FMT_COMPATIBLE;					/* this means temp I/O buffers MUST be set up to planar arrays */
		ofmt = FMT_COMPATIBLE; 
		evalControl[0].evalDataTypeO = evalControl[0].evalDataTypeI;	/* in and out are the same */

		/* get the input reformatting function */
		evalControl[0].formatFuncI = getFormatFuncI (evalDef->dataTypeI, 
											evalControl[0].evalDataTypeI);	

		/* and the output reformatting function */
		evalControl[0].formatFuncO = getFormatFuncO (evalControl[0].evalDataTypeO, 
												evalDef->dataTypeO);	
		
		if (nFuts == 1) {
		   	numEvals = evalDef->nPels * evalDef->nLines;
		}
		else {
		   	numEvals = FUT_EVAL_BUFFER_SIZE;
		}
	}

	futH = futEvalList[0].futH;				/* get the first fut */
	futmask = futEvalList[0].evalMask;		/* and its mask */

#if defined (KCP_EVAL_TH1)
	for (i1 = 0; i1 < KCP_MAX_PROCESSORS; i1++) {
		taskEvalCache[i1] = NULL;
	}
	
	evalControl[0].th1CacheP = th1AllocCache (&threadGlobalsP->evalTh1Cache);	/* set up the main evaluation cache */
	if (evalControl[0].th1CacheP == NULL) {
		goto GetOut;
	}

	errnum = kcpCheckTh1Cache (futH, futmask, numEvals, ifmt, ofmt, &evalControl[0]);	/* see if it's set up */
	if (evalControl[0].evalFunc == NULL) {
		goto GetOut;
	}
#endif

	if ((errnum != KCP_SUCCESS) || (evalControl[0].optimizedEval != 1)) {		/* must lock futs */
		for (i1 = 0; i1 < nFuts; i1++) {        
			fut = fut_lock_fut (futEvalList[i1].futH);
			futEvalList[i1].futP = fut;
			if (fut == NULL) {
			}
		}		
	}

	if (errnum != KCP_SUCCESS) {	/* must re-initialize the cache */
	#if defined (KCP_EVAL_TH1)
		errnum = kcpInitTh1Cache (futEvalList[0].futP, futmask, &evalControl[0]);  /* Initialize the cache */
		if (errnum != KCP_SUCCESS) {
			goto GetOut;
		}
	#endif
	}

/* keep just one global cache of optimized lookup tables */
/* this single cache can be shared by all tasks, but only as read-only memory */
/* if the tasks need separate optimized luts - which should not happen - then */
/* they need to switch to the non-optimized luts mode.  Otherwise, the memory demands are simply too great */

	linesPerTask = (evalDef->nLines + taskCount -1) / taskCount;	/* calc lines each task will process */
	mainLines = evalDef->nLines - (linesPerTask * (taskCount -1));	/* this makes main task the smallest */
	
	evalControl[0].imageLines = mainLines;	/* insert lines for main task */
	
	for (i1 = 1; i1 < taskCount; i1++) {
		evalControl[i1] = evalControl[0];		/* copy main task's evaluation control into this task */

		evalControl[i1].th1CacheP = th1AllocCache (&taskEvalCache[i1]);	/* set up the task evaluation caches */
		if (evalControl[i1].th1CacheP == NULL) {
			goto GetOut;
		}

		saveH = evalControl[i1].th1CacheP->th1CacheSvH;	/* preserve the handle! */

		*evalControl[i1].th1CacheP = *evalControl[0].th1CacheP;	/* copy main task's evaluation cache into this task */

		evalControl[i1].th1CacheP->th1CacheSvH = saveH;	/* restore the handle */

		evalControl[i1].threadGlobalsP = NULL;	/* so that only main task uses progress call-back */

		/* calc start address for this task */
		for (i2 = 0; i2 < nInputs; i2++) {		/* set up input image addresses */
			imageStride = evalControl[0].inLineStride[i2] * evalControl[i1 -1].imageLines;	/* lines done by previous task */
			evalControl[i1].inputData[i2].p8 = evalControl[i1 -1].inputData[i2].p8 + imageStride;
		}

		for (i2 = 0; i2 < nOutputs; i2++) {		/* set up output image addresses */
			imageStride = evalControl[0].outLineStride[i2] * evalControl[i1 -1].imageLines;	/* lines done by previous task */
			evalControl[i1].outputData[i2].p8 = evalControl[i1 -1].outputData[i2].p8 + imageStride;
		}

		evalControl[i1].imageLines = linesPerTask;	/* insert lines for this task */
	}

	/* Initialize the progress, calling it no more than 1% of the time. */
	numWaitLoops = (evalDef->nLines + 99) / 100;
	initProgress (threadGlobalsP, numWaitLoops, progress);

	errnum = doProgress (threadGlobalsP, 0);	/* always send 0 at start */
	if (errnum != KCP_SUCCESS) {
		goto GetOut;
	}

	/* send evaluation command to each task */
	tasksStarted = 0;	/* need to wait for each started task to keep queues synchronized */
						/* otherwise, the completion queue for a task started this time */
						/* might be erroneously seen as the completion for a task started next time */

	for (i1 = 1; i1 < taskCount; i1++) {
		evalControlP = &evalControl[i1];
/*		#if defined (KCP_NEVER_DEFINED)	/* for testing */
#if defined (KCP_MACPPC_MP)
			theStatus = MPNotifyQueue (threadGlobalsP->taskListP[i1].fromMain, (void *)kEvaluate, (void *)evalControlP, 0);
			if (theStatus != noErr) {
/*	DebugStr ("\pMPNotifyQueue error");	/* for testing */
				errnum = KCP_NOT_COMPLETE;
				break;
			}
#elif defined (KCP_THREAD_MP)
		thread[0] = KpThreadCreate ((KpThrStartFunc)evalImage, evalControlP, NULL, 0, NULL);
#endif
		tasksStarted++;		/* count tasks started */
	}

	/* evaluate the main task image */
	errnum1 = evalImage (&evalControl[0]);
	if ((errnum == KCP_SUCCESS) && (errnum1 != KCP_SUCCESS)) {
		errnum = errnum1;		/* save the first error */
	}

	/* wait for all tasks to finish */
#if defined (KCP_THREAD_MP)
	KpThreadWait (thread, taskCount-1, THREAD_WAIT_ALL,
									THREAD_TIMEOUT_INFINITE, NULL);
	for (i1 = 0; i1 < taskCount-1; i1++) {
		KpThreadDestroy (thread[i1]);
	}
#else	
	for (i1 = 1; i1 < tasksStarted+1; i1++) {
/*		#if defined (KCP_NEVER_DEFINED)	/* for testing */
#if defined (KCP_MACPPC_MP)
		theStatus = MPWaitOnQueue (threadGlobalsP->taskListP[i1].toMain, (void **)&action,
										(void **)&taskStatus, NULL, kDurationForever);
		if (theStatus == noErr) {
			errnum1 = (PTErr_t)taskStatus;		/* copy to avoid enum size problems */
		}
		else {
/*	DebugStr ("\pMPWaitOnQueue error");	/* for testing */
			errnum1 = KCP_NOT_COMPLETE;
		}
#else
		evalControlP = &evalControl[i1];
		errnum1 = evalImage (evalControlP);	/* evaluate the sub-image */
#endif
		if ((errnum == KCP_SUCCESS) && (errnum1 != KCP_SUCCESS)) {
/*	DebugStr ("\pError in completion loop");	/* for testing */
			errnum = errnum1;		/* save the first error */
		}
	}
#endif

GetOut:
	for (i1 = 0; i1 < nFuts; i1++) {        
		fut_unlock_fut (futEvalList[i1].futP);	/* unlock all the futs */
	}		

#if defined (KCP_EVAL_TH1)
	if (errnum == KCP_SUCCESS) {
		for (i1 = 1; i1 < taskCount; i1++) {
			if (evalControl[i1].errnum != KCP_SUCCESS) {
				errnum = evalControl[i1].errnum; /* did any thread fail? */
				break;
			}
		}
	}
	th1UnLockCache (evalControl[0].th1CacheP);	/* unlock the cache */

	for (i1 = 1; i1 < taskCount; i1++) {
		freeBuffer (taskEvalCache[i1]);	/* do not use th1FreeCache - it frees the LutMem */
	}
	
#endif
	
	if (errnum == KCP_SUCCESS) {
		initProgress (threadGlobalsP, numWaitLoops, progress);	/* always send 100 at end */
		errnum = doProgress (threadGlobalsP, 100);
	}

	return errnum;
}


/**********************************************************************/

#if defined (KCP_MACPPC_MP)

OSStatus
	evalTaskMac (	void*	taskParameter)
{
PTErr_t			errnum;
KpUInt32_t		finished, action;
taskControl_p	taskControl;
evalControl_p	evalControlP;
OSStatus		theStatus;

/*	DebugStr ("\pevalTaskMac");	/* for testing */
	
	taskControl = (taskControl_p)taskParameter;

	finished = 0;

	while (finished == 0) {
		theStatus = MPWaitOnQueue(taskControl->fromMain, (void **)&action, (void **)&evalControlP, NULL, kDurationForever);
		if (theStatus != noErr) {
			action = kErrMP;		/* MP error */
		}

		switch	(action) {
			case kEvaluate:
				errnum = evalImage (evalControlP);		/* evaluate the image */

				break;
				
			case kTerminate:
				errnum = KCP_SUCCESS;
				finished = 1;
				break;
				
			case kErrMP:
				errnum = KCP_NOT_COMPLETE;
				finished = 1;
				break;
				
			default:
				errnum = KCP_FAILURE;
				break;
			}

		theStatus = MPNotifyQueue(taskControl->toMain, (void *)kComplete, (void *)errnum, NULL);
		if (theStatus != noErr) {
			errnum = KCP_NOT_COMPLETE;
			finished = 1;
		}
	}

	return	(noErr);
}

#endif


/**********************************************************************/

static PTErr_t
	evalImage (	evalControl_p	eC)
{
PTErr_t			errnum = KCP_SUCCESS;
KpInt32_t		i1, i2, i3, np, imask;
KpUInt32_t		futmask;
KpHandle_t		futH;
KpInt32_t		linecnt, numPels, linecnt100times;

imagePtr_t		iLineData[FUT_NICHAN], oLineData[FUT_NOCHAN];
imagePtr_t		itempData[FUT_NOCHAN], otempData[FUT_NOCHAN];
imagePtr_p		itempDataP, otempDataP, tempDataP;
imagePtr_t		aPtr[FUT_NOCHAN];

KpUInt16_t		xi[FUT_EVAL_BUFFER_SIZE], yi[FUT_EVAL_BUFFER_SIZE], zi[FUT_EVAL_BUFFER_SIZE], ti[FUT_EVAL_BUFFER_SIZE];
KpUInt16_t		ui[FUT_EVAL_BUFFER_SIZE], vi[FUT_EVAL_BUFFER_SIZE], wi[FUT_EVAL_BUFFER_SIZE], si[FUT_EVAL_BUFFER_SIZE];
KpUInt16_t		xo[FUT_EVAL_BUFFER_SIZE], yo[FUT_EVAL_BUFFER_SIZE], zo[FUT_EVAL_BUFFER_SIZE], to[FUT_EVAL_BUFFER_SIZE];
KpUInt16_t		uo[FUT_EVAL_BUFFER_SIZE], vo[FUT_EVAL_BUFFER_SIZE], wo[FUT_EVAL_BUFFER_SIZE], so[FUT_EVAL_BUFFER_SIZE];

	itempData[0].p16 = xi;
	itempData[1].p16 = yi;
	itempData[2].p16 = zi;
	itempData[3].p16 = ti;
	itempData[4].p16 = ui;
	itempData[5].p16 = vi;
	itempData[6].p16 = wi;
	itempData[7].p16 = si;
	otempData[0].p16 = xo;
	otempData[1].p16 = yo;
	otempData[2].p16 = zo;
	otempData[3].p16 = to;
	otempData[4].p16 = uo;
	otempData[5].p16 = vo;
	otempData[6].p16 = wo;
	otempData[7].p16 = so;
	
	linecnt100times = eC->imageLines * 100;

/* Loop over output picture rows */
	for (linecnt = 0; linecnt < linecnt100times; linecnt+=100) {

		errnum = doProgress (eC->threadGlobalsP, linecnt / eC->imageLines);
		if (errnum != KCP_SUCCESS) {
			goto GetOut;
		}

		/* evaluate a line of data */
		if (eC->compatibleDataType == 1) {
#if defined (KCP_EVAL_TH1)
			((evalTh1Proc_t)(eC->evalFunc)) (eC->inputData, eC->inPelStride, eC->outputData, eC->outPelStride, eC->imagePels, eC->th1CacheP);
#endif
		}
		else {	/* somewhere there must be some special format data or it's a serial evaluation */
				/* do in small groups of size FUT_EVAL_BUFFER_SIZE */
				/* initialize data pointers for this line */
			for (i3 = 0; i3 < eC->nInputs; i3++) {
				iLineData[i3] = eC->inputData[i3];
			}

			for (i3 = 0; i3 < eC->nOutputs; i3++) {
				oLineData[i3] = eC->outputData[i3];
			}

			np = FUT_EVAL_BUFFER_SIZE;
			for ( numPels = eC->imagePels; numPels > 0; numPels -= FUT_EVAL_BUFFER_SIZE ) {

				if ( numPels < FUT_EVAL_BUFFER_SIZE ) {     /* last group will be smaller */
					np = numPels;
				}

				for (i1 = 0; i1 < FUT_NICHAN; i1++) {		/* set up temp destination pointers */
					aPtr[i1] = itempData[i1];
				}

				((formatFunc_t)(eC->formatFuncI)) (eC->nInputs, np, iLineData, eC->inPelStride, aPtr);	/* format the input data */
				
				itempDataP = otempData;	/* initialize to swapped */ 
				otempDataP = itempData;

				for (i1 = 0; i1 < eC->nFuts; i1++) {       /* evaluate each fut in the list */
#if defined (KCP_EVAL_TH1)
					futH = eC->futEvalList[i1].futH;		/* get the next fut */
					futmask = eC->futEvalList[i1].evalMask;	/* and the mask for it */

					if (eC->nFuts != 1) {	/* only first can be set up, so must set up eval function here */
						errnum = kcpCheckTh1Cache (futH, futmask, np, eC->evalDataTypeI, eC->evalDataTypeO, eC);	/* see if it's set up */
						if ((evalTh1Proc_t)eC->evalFunc == NULL) {
							goto GetOut;
						}

						if (errnum != KCP_SUCCESS) {	/* must re-initialize the cache */
							errnum = kcpInitTh1Cache (eC->futEvalList[i1].futP, futmask, eC);  /* Initialize the cache */
							if (errnum != KCP_SUCCESS) {
								goto GetOut;
							}
						}
					}

					tempDataP = otempDataP;		/* swap buffers */
					otempDataP = itempDataP;
					itempDataP = tempDataP;
					
					if (eC->optimizedEval == 1) {
						for (i2 = 0; i2 < FUT_NICHAN; i2++) {		/* set up temp destination pointers */
							aPtr[i2] = itempDataP[i2];
						}
					}
					else {	/* may have wierd input combinations, so must use correct input array ordering */
						imask = (KpInt32_t)FUT_IMASK(futmask);

						for (i2 = 0, i3 = 0; i2 < TH1_MAX_INPUT_VARS ; i2++) {
							if (imask & FUT_BIT(i2)) {
								aPtr[i2] = itempDataP[i3];	/* make a list of input tables */
								i3++;
							}
							else {
								aPtr[i2].p8 = NULL;			/* not used */
							}
						}
					}

					((evalTh1Proc_t)(eC->evalFunc)) (aPtr, eC->tempPelStride, otempDataP, eC->tempPelStride, np, eC->th1CacheP);
#endif
				}

				for (i1 = 0; i1 < FUT_NOCHAN; i1++) {		/* set up temp source pointers */
					aPtr[i1] = otempDataP[i1];
				}

				((formatFunc_t)(eC->formatFuncO)) (eC->nOutputs, np, aPtr, eC->outPelStride, oLineData);	/* format the output data */

			}	/* walk thru the line */
		}	/* else simple data */

		/* move inputs to the next line */
		for (i3 = 0; i3 < eC->nInputs; i3++) {
			eC->inputData[i3].p8 += eC->inLineStride[i3];
		}

		/* move outputs to the next line */
		for (i3 = 0; i3 < eC->nOutputs; i3++) {
			eC->outputData[i3].p8 += eC->outLineStride[i3];
		}
	} /* linecnt */

GetOut:
	eC->errnum = errnum;	/* stick it here for MP threads */
	return	(errnum);
}


/**********************************************************************/

static KpUInt32_t
	getEvalDataType (KpUInt32_t dataType)
{
KpUInt32_t	evalDataType;

	switch (dataType) {
	case KCM_USHORT_555:
	case KCM_USHORT_565:
	case KCM_UBYTE:
		evalDataType = KCM_UBYTE;
		break;
		
	case KCM_R10G10B10:
	case KCM_USHORT_12:
		evalDataType = KCM_USHORT_12;
		break;
		
	case KCM_USHORT:
		evalDataType = KCM_USHORT;
		break;

	default:
		evalDataType = KCM_UNKNOWN;
	}
	
	return evalDataType;
}


/**********************************************************************/

static formatFunc_t
	getFormatFuncI (	KpUInt32_t		dataTypeI,
						KpUInt32_t		dataTypeO)
{
formatFunc_t	formatFuncI;

	switch (dataTypeO) {
	case KCM_UBYTE:
		switch (dataTypeI) {
		case KCM_UBYTE:
			formatFuncI = pass8in;
			break;
			
		case KCM_USHORT_555:
			formatFuncI = format555to8;
			break;
			
		case KCM_USHORT_565:
			formatFuncI = format565to8;
			break;
			
		case KCM_R10G10B10:		/* would be 12->8, should not happen */
		case KCM_USHORT_12:		/* would be 12->8, should not happen */
		case KCM_USHORT:		/* would be 16->8, should not happen */
		default:
			formatFuncI = NULL;
		}
		break;
		
	case KCM_USHORT_12:
		switch (dataTypeI) {
		case KCM_UBYTE:
			formatFuncI = format8to12;
			break;
			
		case KCM_USHORT_12:
			formatFuncI = pass16in;
			break;
			
		case KCM_USHORT:
			formatFuncI = format16to12;
			break;
			
		case KCM_USHORT_555:
			formatFuncI = format555to12;
			break;
			
		case KCM_USHORT_565:
			formatFuncI = format565to12;
			break;
			
		case KCM_R10G10B10:
			formatFuncI = format10to12;
			break;
			
		default:
			formatFuncI = NULL;
			break;
		}
		
		break;

	default:
		formatFuncI = NULL;
		break;
	}
	
	return (formatFuncI);
}


/**********************************************************************/

static formatFunc_t
	getFormatFuncO (	KpUInt32_t		dataTypeI,
						KpUInt32_t		dataTypeO)
{
formatFunc_t	formatFuncO;

	switch (dataTypeI) {
	case KCM_UBYTE:
		switch (dataTypeO) {
		case KCM_UBYTE:			/* compatible */
			formatFuncO = pass8out;
			break;
			
		case KCM_USHORT_555:
			formatFuncO = format8to555;
			break;
			
		case KCM_USHORT_565:
			formatFuncO = format8to565;
			break;
			
		case KCM_R10G10B10:		/* would be 8->12, should not happen */
		case KCM_USHORT_12:		/* would be 8->12, should not happen */
		case KCM_USHORT:		/* would be 8->16, should not happen */
		default:
			formatFuncO = NULL;
			break;
		}
		break;
		
	case KCM_USHORT_12:
		switch (dataTypeO) {
		case KCM_UBYTE:
			formatFuncO = format12to8;
			break;
			
		case KCM_USHORT_12:
			formatFuncO = pass16out;
			break;
			
		case KCM_USHORT:
			formatFuncO = format12to16;
			break;
			
		case KCM_USHORT_555:
			formatFuncO = format12to555;
			break;
			
		case KCM_USHORT_565:
			formatFuncO = format12to565;
			break;
			
		case KCM_R10G10B10:
			formatFuncO = format12to10;
			break;
			
		default:
			formatFuncO = NULL;
			break;
		}
		
		break;

	default:
		formatFuncO = NULL;
		break;
	}
	
	return (formatFuncO);
}


/**********************************************************************/

/* format_analyze determines the format of the input or output data */
static KpUInt32_t
	format_analyze (imagePtr_p		p,
					KpInt32_p		s,
					KpUInt32_t		dataType)
{
KpUInt32_t	fmt = FMT_GENERAL; /* assume general format */
KpUInt32_t	t, stride;
#if defined(KPMAC)	/* must allow for word R/W frame buffers */
KpUInt8_p	base;
#endif

	/* determine if all the strides are the same (or zero) and if so,
	 * what that stride is.  If any non-zero stride differs from any
	 * other non-zero stride, return with FMT_GENERAL.  Otherwise, we
	 * continue on to check for pixel interleaved formats.
	 *
	 * NOTE: the following "if" statement, although cryptic, really works!!
	 *       (And it does so with a maximum of 9 "tests").
	 */
	stride = s[0];
	if ( ((t=s[1]) != 0  &&  t != (stride = stride ? stride : t)) ||
	     ((t=s[2]) != 0  &&  t != (stride = stride ? stride : t)) ||
	     ((t=s[3]) != 0  &&  t != (stride = stride ? stride : t)) ) {

		return (fmt);
	}

	/* If we get to here, all non-zero strides are the same.
	 * Now, check for various special case formats. */
	switch (stride) {
		case 1:
			if (dataType == KCM_UBYTE) {
				fmt = FMT_COMPATIBLE;	/* all defined arrays have strides of 1 */
			}
			break;

		case 2:
			if ((dataType == KCM_USHORT_12) || (dataType == KCM_USHORT)) {
				fmt = FMT_COMPATIBLE;	/* all defined arrays have strides of 2 */
			}
			break;

		case 3:
			if (((KpUInt8_p)p[0].p8+1 == p[1].p8) && ((KpUInt8_p)p[1].p8+1 == p[2].p8) && (p[3].p8 == NULL)) {
				fmt = FMT_BIGENDIAN24;
			}
			else {
				if (((KpUInt8_p)p[0].p8-1 == p[1].p8) && ((KpUInt8_p)p[1].p8-1 == p[2].p8) && (p[3].p8 == NULL)) {
					fmt = FMT_LITTLEENDIAN24;
				}
			}
			break;

		case 4:     /* check for 32-bit formats */

#if defined(KPMAC)	/* must allow for word R/W frame buffers */

#define BASE(x) ((KpUInt8_p)((int32)(x) & 0xfffffffc))

			base = BASE(p[0].p8);
			if ((base+1 == p[0].p8) && (p[0].p8+1 == p[1].p8) && (p[1].p8+1 == p[2].p8) && (p[3].p8 == NULL)) {
				fmt = FMT_QD;
			}
#endif
			if (((KpUInt8_p)p[0].p8+1 == p[1].p8) && ((KpUInt8_p)p[1].p8+1 == p[2].p8) && ((KpUInt8_p)p[2].p8+1 == p[3].p8)) {
				fmt = FMT_BIGENDIAN32;
			}
			else {
				if (((KpUInt8_p)p[0].p8-1 == p[1].p8) && ((KpUInt8_p)p[1].p8-1 == p[2].p8) && ((KpUInt8_p)p[2].p8-1 == p[3].p8)) {
					fmt = FMT_LITTLEENDIAN32;
				}
			}
			break;

		default:    /* use general format */
			break;
	}

	return (fmt);

} /* format_analyze */


/**********************************************************************/

void
	kcpFreeCache (	threadGlobals_p threadGlobalsP)
{
#if defined (KCP_EVAL_TH1)
	th1FreeCache ((KpHandle_t)threadGlobalsP->evalTh1Cache);
	threadGlobalsP->evalTh1Cache != NULL;
#endif
}

