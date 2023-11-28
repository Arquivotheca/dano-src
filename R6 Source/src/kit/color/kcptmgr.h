/*
 * @(#)kcptmgr.h	2.130 97/12/22

	Contains:	header file for KCMS system processor

	Written by:	Drivin' Team

	Copyright (c) 1992-1997 by Eastman Kodak Company, all rights reserved.
 */

#ifndef _KCMSYS_H_
#define _KCMSYS_H_ 1

/* !!!! change 'vers' in KPCPversion.r !!!! */
/* THIS MUST NOT GO TO 6 (count em 6) chars or the precision api will barf !!! */
#define KPCP_VERSION "3.43"		/* color processor version number */


/* #define DIAGWIND 1 */		/* do this for the alpha release only!!! */

#include "kcms_sys.h"
#include "kcmptlib.h"
#include "fut.h"

#if defined (KPMAC)
#include <Components.h>
#endif

#if defined (KCP_MACPPC_MP)
#include <MP.h>
#endif

/* chaining constants */
#define MAX_RULE_COMPONENT 3	/* max # of rule components */
#define MAX_PT_CHAIN_SIZE 20	/* maximum # of PTs in a chain */
#define MAX_ICOMP 20			/* maximum # of input PT composition groups */
#define MAX_OCOMP 20			/* maximum # of output PT composition groups */
#define NOT_CHECKED_IN 0		/* show this pt is not checked in */
#define IS_CHECKED_IN 1			/* show this pt is checked in */
#define NOT_SERIAL_PT 2			/* show this is not a serial pt */
#define IS_SERIAL_PT 3			/* show this is a serial pt */

/* the dimensions of the futs */ 
#define EIGHT_CUBE	8

/* the maximum number of processors active at one time (including main processor) */ 
#define KCP_MAX_PROCESSORS	4

/* PT format */
typedef struct PT_s {
	KpInt32_t	magic;			/* for technology id and byte swapping */
	KpInt32_t	version;		/* allow for future modifications */
	KpInt32_t	attrSize;		/* # bytes of attribute information */
	char	hdr[488];		/* private header info */
							/* followed by attribute data */
							/* followed by technology data */
} PT_t;

#if defined (KCP_COMP_2)
/* composition rules */
typedef struct composeRule_s {
	char *start[MAX_RULE_COMPONENT+1];	/* start rules */
	char *finish[MAX_RULE_COMPONENT+1];	/* finish rules */
} composeRule_t, FAR* composeRule_p;

#endif

/* PT table definition */
typedef struct PTTable_s {
	struct PTTable_s **next;
	struct PTTable_s **prev;

	PTRefNum_t	refNum;					/* the reference number for the PT */
	KcmHandle	hdr;					/* address of the PT header */
	KcmHandle	attrBase;				/* address of the PT attributes */
	KcmHandle	data;					/* address of the PT data */
	KpUInt32_t	checkInFlag;			/* is this PT checked in? */
	KpUInt32_t	inUseCount;				/* number of links to this PT */
	KpUInt32_t	serialPTflag;			/* is this a serial PT? */
	KpUInt32_t	serialCount;			/* # of PTs in the serial chain */
	PTRefNum_t	serialDef[MAX_PT_CHAIN_SIZE]; /* list of PTs for serial eval */
} PTTable_t, FAR* PTTable_p;

#if defined (KPMAC)
typedef PTErr_t (FAR PASCAL *PTRelay_t) (long savedA5, long savedA4, PTProgress_t progressFunc, KpInt32_t perCent);
#endif

#if defined (KCP_MACPPC_MP)
#define	kEvaluate (1)
#define	kTerminate (2)
#define	kComplete (3)
#define	kErrMP (4)

typedef struct taskControl_s {
	MPTaskID	ID;				/* ID of this task */
	MPQueueID	fromMain;		/* messages from main task to others */
	MPQueueID	toMain;			/* messages from others to main task */
	MPQueueID	termination;	/* termination queue for this task */
} taskControl_t, FAR* taskControl_p;

#endif

/* structures for explicit global handling */

/* globals which are common to all components and which are set up at system startup time */
typedef struct initializedGlobals_s {
	KpInt32_t		SWalways;  					/* init = 0: default is to try to load the CTE driver. */
	char		KCPDataDir [256];			/* fully qualified path to CP data directory */
	evalList_t	evalList[EVAL_LIST_MAX];	/* available evaluators */

	KpInt32_t	PTCubeSize;					/* size of ICC profiles (pts) */

#if defined (KPMAC)
	PTRelay_t	callBackRelay;				/* progress call back relay for PPC->68K */
#endif

#if defined(KCP_COMP_2)
	char_p		composeRuleDB;						/* composition rule data base */
	composeRule_t composeRule[MAX_ICOMP][MAX_OCOMP]; /* composition rule base */
#endif

#if defined (KPWIN)
	KpModuleId		moduleId;
	KpInstance		instance;
	KpModuleId		appModuleId;
#endif

	KpInt32_t		numProcessors;
	
} initializedGlobals_t, FAR* initializedGlobals_p;

typedef struct processGlobals_s {
	KpInt32_t				threadCount;		/* number of threads in a process */
	KpCriticalFlag_t		PTcriticalSection;	/* this is the critical section object for thread access */
	PTTable_p FAR*			PTTableRootH;		/* handle to root for PT linked list */
	ioFileChar				KCPDataDirProps;	/* file properties for CP data files */
	initializedGlobals_p	iGP;				/* initialized globals */
} processGlobals_t, FAR* processGlobals_p;

typedef struct threadGlobals_s {
	processGlobals_p	processGlobalsP;			/* a pointer to the process global */
	KpInt32_t			threadUseCount;				/* number of users in a single thread */
	KpInt32_t			compMode;					/* composition mode for entire chain */
	KpInt32_t			rulesKey;					/* key to apply chaining rules */
	KpInt32_t			chainLength;				/* # of PTs in the chain */
	KpInt32_t		 	chainIndex;					/* current position in chain */
	PTRefNum_t			chainDef[MAX_PT_CHAIN_SIZE]; /* list of PTs to chain */
	PTRefNum_t			currentPT;					/* current result of PT chain */
	char_h 				finishRules;				/* finish rules list */
	kcpindex_t 			iComp;						/* input chain rule of first PT in chain */
	kcpindex_t 			oComp;						/* output chain rule of last PT in chain */
	kcpindex_t 			inSense;					/* input sense of first PT in chain */
	kcpindex_t 			outSense;					/* output sense of last PT in chain */
	KpHandle_t			evalTh1Cache;				/* evaluation cache for tetrahedral */
	KpInt32_t 			loopStart;
	KpInt32_t			loopCount;
	PTProgress_t		progressFunc;
	KpInt32_t			currPasses;
	KpInt32_t			totalPasses;
	KpInt32_t			lastProg100;
	PTProgress_t		apiProgressFunc;
#if defined (KPMAC)
	long 				gHostA4;
	long 				gHostA5;
#endif

#if defined (KCP_MACPPC_MP)
	taskControl_p		taskListP;
#endif
} threadGlobals_t, FAR* threadGlobals_p;

/* color processor globals */
#ifndef KCP_GLOBAL
#define KCP_GLOBAL extern
#endif

KCP_GLOBAL	KpThreadMemHdl_t	theRootID;


/* function prototypes */
void		kcpFreeCache (threadGlobals_p);

PTErr_t		doActivate ARGS((PTRefNum_t PTRefNum, KpInt32_t mBlkSize, PTAddr_t PTAddr, KpInt32_t crcMode));
PTErr_t		doGetSizeF ARGS((threadGlobals_p threadGlobalsP, 
						PTRefNum_t PTRefNum, PTType_t format, int32_p mBlkSize, KpInt32_t crcMode));

PTErr_t		initPTTable ARGS((threadGlobals_p threadGlobalsP));
PTErr_t		freeApplPT ARGS((threadGlobals_p threadGlobalsP));
PTErr_t		registerPT ARGS((threadGlobals_p threadGlobalsP, KcmHandle PTHdr,
							KcmHandle PTAttr, PTRefNum_p PTRefNumP));
PTErr_t		kcpGetStatus ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
KcmHandle	getPTHdr ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
PTErr_t		setPTHdr ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum,
							KcmHandle hdr));
KcmHandle	getPTAttr ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
void		setPTAttr ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum,
							KcmHandle attrBase));
KcmHandle	getPTData ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
PTErr_t 	resolvePTData (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum,
							KpInt32_p SerialCount, PTRefNum_p SerialData);
KpInt32_t		getCheckInSize ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
void		makeActive ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum,
							KcmHandle PTData));
void		makeSerial ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
PTErr_t 	freeSerialData (threadGlobals_p threadGlobalsP, PTRefNum_t  PTRefNum);
PTErr_t		makeInActive ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
PTErr_t	 	makeCheckedOut (threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum);
void		deletePTTable ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum));
PTErr_t 	addSerialPT (threadGlobals_p threadGlobalsP, PTRefNum_t  CurrentPTRefNum,
						PTRefNum_t  SerialPTRefNum, PTRefNum_p	ResultPTRefNum);

KpInt32_t getAttrSize ARGS((KcmHandle attrBase));
PTErr_t GetAttribute ARGS((KcmHandle startAttr, KpInt32_t attrTag,
				KpInt32_t *attrStrSize, char *attrString));
PTErr_t readAttributes ARGS((threadGlobals_p threadGlobalsP, KpFd_p fd, KpInt32_t size,
							KcmHandle FAR* attrBaseP));
PTErr_t writeAttributes ARGS((threadGlobals_p threadGlobalsP, KpFd_p fd, KcmHandle attrBase));
PTErr_t freeAttributes ARGS((KcmHandle startHand));
PTErr_t copyAllAttr ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t fromPTRefNum,
							PTRefNum_t toPTRefNum));

PTErr_t initList ARGS((evalList_p));
PTErr_t AddEvaluator ARGS((PTEvalTypes_t evaltype, evalList_p evalList));
PTErr_t GetEval ARGS((PTEvalTypes_t reqEval, PTEvalTypes_p useEval, evalList_p evalList));
PTErr_t	getResizeAuxPT(threadGlobals_p threadGlobalsP, PTRefNum_t PTRefNum,
					   PTRefNum_p auxPTRefNum);

PTErr_t	TpReadHdr ARGS((threadGlobals_p threadGlobalsP, KpFd_p, KcmHandle FAR*, PTType_p));
PTErr_t	TpWriteHdr ARGS((threadGlobals_p threadGlobalsP, KpFd_p, PTType_t, KcmHandle, KpInt32_t));
PTErr_t	TpCompareHdr ARGS((threadGlobals_p threadGlobalsP, KcmHandle, KcmHandle));
PTErr_t	TpFreeHdr ARGS((threadGlobals_p threadGlobalsP, KcmHandle));
PTErr_t	TpReadData ARGS((threadGlobals_p threadGlobalsP, KpFd_p, PTType_t,
							PTRefNum_t, KcmHandle, KcmHandle FAR*));
PTErr_t	TpWriteData ARGS((threadGlobals_p threadGlobalsP, KpFd_p, PTType_t,
							KcmHandle, KcmHandle));
PTErr_t	TpFreeData ARGS((threadGlobals_p threadGlobalsP, KcmHandle));
KpInt32_t	TpGetDataSize ARGS((threadGlobals_p threadGlobalsP, KcmHandle, KcmHandle, PTType_t));
PTErr_t TpGenerateAttr ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t));
PTErr_t TpSetImplicitAttr ARGS((threadGlobals_p threadGlobalsP, PTRefNum_t));
PTErr_t TpCalCrc ARGS((threadGlobals_p threadGlobalsP, KcmHandle, KcmHandle, int32_p));

PTErr_t PT_eval_sw ARGS((threadGlobals_p threadGlobalsP, KcmHandle, PTRefNum_t,
								PTEvalDTPB_p, PTProgress_t));

void KCPChainSetup ARGS((initializedGlobals_p iGblP));

void clearChain ARGS((threadGlobals_p threadGlobalsP));

KpInt32_t	CollisionCheck ARGS((void)); 		/* check space between stack and heap */


#if defined DIAGWIND
void ClrGErr ARGS((void));
void diagWindow ARGS((char_p errStr, int errnum));
#else
#define ClrGErr()
#define diagWindow(errStr, errnum)
#endif

threadGlobals_p KCMDloadGlobals (void);
void			KCMDunloadGlobals (void);

threadGlobals_p	KpInitThread (void);
PTErr_t			KpTermThread (threadGlobals_p threadGlobalsP);
PTErr_t			KpTermProcess (threadGlobals_p threadGlobalsP);

#endif

KpInt32_t	KCMDtakeDown (void);
KpInt32_t	KCPappTakeDown (void);
PTErr_t	KCMDTerminate (void);
KpInt32_t	KCMDsetup (KpGenericPtr_t FAR* IGPtr);
KpInt32_t	KCPappSetup (void* IGPtr);
void	KCPInitIGblP(KpGenericPtr_t FAR* IGPtr, initializedGlobals_p IGblP);
PTErr_t	PTTerminatePlatform(threadGlobals_p	threadGlobalsP);

#if defined (KCP_MACPPC_MP)
OSStatus evalTaskMac (void*);
void	KCPInitializeMP (threadGlobals_p);
void	KCPTerminateMP (threadGlobals_p);
#endif

#if defined (KPMAC)
PTErr_t	KCMDcommand (int16, KpGenericPtr_t);
KpInt32_t	CanDoSelector (int16);
KpInt32_t	GetVersion (void);
void	kcpGetFPU ();
#endif





