/*
	File:	kcptmgrd.h	@(#)kcptmgrd.h	2.22 11/25/96

	Contains:       Kodak Color Management System interface file.

	Written by:     Drivin' Team

	Copyright:      (c) 1992-1995 Eastman Kodak Company, all rights reserved.

*/

#ifndef KCMCPR_D_H_
#define KCMCPR_D_H_ 1

#if defined (KPMACPPC)
#pragma options align=mac68k	/* make sure these structures are 68k type for backward compatibility */
#endif

/* Note: negative numbers may not be used due to Component Manager requirements */
#define	PTCHECKIN 1				/* Check in a PT */
#define	PTCHECKOUT 2			/* Check out a PT */
#define	PTACTIVATE 3			/* Activate a PT */
#define	PTDEACTIVATE 4			/* DeActivate a PT */
#define	PTGETATTRIBUTE 5		/* Get the PT's attributes */
#define	PTSETATTRIBUTE 6		/* Set the PT's attributes */
#define	PTGETSIZE 7				/* get external PT size */
#define	PTGETPT 8				/* get PT to external memory block */
#define	PTEVAL 9				/* evaluation a PT function */
#define	PTEVALRDY 10			/* is the evaluator ready for another PT ? */
#define	PTEVALCANCEL 11			/* kill an evaluation */
#define	PTCOMBINE 12			/* composition */
#define	PTCHAINVALIDATE 13		/* validate a composition list */
#define	PTCHAININIT 14			/* define a composition list */
#define	PTCHAIN 15				/* compose next PT in list */
#define	PTCHAINEND 16			/* get composed PT */
#define	PTNEWEMPTY 17			/* create a PT with non-initialized tables */
#define	PTEVALUATORS 18			/* number of evaluator boards */
#define	PTPROCESSORRESET 19		/* Reset everything */
#define	PTGETTAGS 20			/* number of attribute tags in PT */
#define	PTGETPTINFO 21			/* get PT info from checkin list */
#define	PToldPT 22				/* make a PT from properly formatted tables */
#define	PTCTEGRIDSIZE 23		/* get size of grid tables on CTE */
#define	PTPROCESSORSETUP 24		/* initialize the cte */
#define	PTINITIALIZE 25			/* initialize the color processor */
#define	PTTERMINATE 26			/* reset when application terminates */
#define	PTGETITBL 27			/* get an input table of a PT */
#define	PTGETGTBL 28			/* get a grid table of a PT */
#define	PTGETOTBL 29			/* get an output table of a PT */
#define	PTAPPLINITIALIZE 30		/* initialize the color processor called from the bridge */
#define	PTNEWEMPTYSEP 31		/* create a separable PT with non-initialized tables */
#define	PTSETHOSTA5	32			/* compatable with filter stuff */
#define	PTGETXFORMSIZE 33		/* get external XFORM size */
#define	PTGETXFORM 34			/* get XFORM to external memory block */
#define	PTEVALDT 35				/* evaluation a PT function */
#define	PTEVALPT 36				/* evaluation a PT function with a PT (not at fut) */
#define	PTGETSIZEF 37			/* get external PTF size */
#define	PTGETPTF 38				/* get PTF to external memory block */
#define	PTNEWMATGAMPT 39		/* Make a PT from a matrix and a set of 1D gamma tables */
#define	PTCHAININITM 40			/* define a composition list with a mode */
#define	PTNEWMATGAMAIPT 41		/* PTNEWMATGAMPT with adaptation and interpolation control */
#define	PTINITGLUE 42			/* initialize the color processor glue */
#define	PTTERMGLUE 43			/* terminate the color processor glue */
#define	PTNEWMONOPT 44			/* Make a gray scale PT from a matrix and a set of 1D gamma tables */
#define	PTGETFLAVOR 45			/* Get the type of image evaluations from the CP */
#define	PTGETRELTOABSPT 46		/* PTGETRELTOABSPT */
#define	PTICMGRIDSIZE 47		/* get size of grid tables on CTE for the ICM */
#define	PTCOLORSPACE 48			/* set the input color space for next pteval */

typedef struct PTCheckInParam_s {
	PTRefNum_long_ptr_t     PTRefNum;               /* PT reference number */
	PTAddr_t                PTAddr;                 /* PT address */
} PTCheckInParam_t, FAR* PTCheckInParam_long_ptr_t;

typedef struct PTActivateParam_s {
	PTRefNum_t      PTRefNum;       /* PT reference number */
	int32           mBlkSize;       /* size of the PT in external memory */
	PTAddr_t        PTAddr;         /* PT address */
} PTActivateParam_t, FAR* PTActivateParam_long_ptr_t;

typedef struct PTDeActivateParam_s {
	PTRefNum_t      PTRefNum;           /* PT reference number */
} PTDeActivateParam_t, FAR* PTDeActivateParam_long_ptr_t;

typedef struct PTCheckOutParam_s {
	PTRefNum_t      PTRefNum;           /* PT reference number */
} PTCheckOutParam_t, FAR* PTCheckOutParam_long_ptr_t;

typedef struct PTGetPTInfoParam_s {
	PTRefNum_t              PTRefNum;   /* PT reference number */
	PTAddr_t FAR* FAR*      PTHdr;  /* PT header */
	PTAddr_t FAR* FAR*      PTAttr; /* PT attributes */
	PTAddr_t FAR* FAR*      PTData; /* PT data */
} PTGetPTInfoParam_t, FAR* PTGetPTInfoParam_long_ptr_t;

typedef struct PTGetAttributeParam_s {
	PTRefNum_t      PTRefNum;           /* PT reference number */
	int32           attributeTag;
	int32_long_ptr  size;
	char_long_ptr   attribute;
} PTGetAttributeParam_t, FAR* PTGetAttributeParam_long_ptr_t;

typedef struct PTSetAttributeParam_s {
	PTRefNum_t      PTRefNum;               /* PT reference number */
	int32           attributeTag;
	char_long_ptr   attribute;
} PTSetAttributeParam_t, FAR* PTSetAttributeParam_long_ptr_t;

typedef struct PTGetTagsParam_s {
	PTRefNum_t      PTRefNum;   /* PT reference number */
	int32_long_ptr  nTags;
	int32_long_ptr  tagArray;
} PTGetTagsParam_t, FAR* PTGetTagsParam_long_ptr_t;

typedef struct PTGetSizeParam_s {
	PTRefNum_t      PTRefNum;       /* PT reference number */
	int32_long_ptr  mBlkSize;       /* size of the PT in external memory */
} PTGetSizeParam_t, FAR* PTGetSizeParam_long_ptr_t;

typedef struct PTGetSizeFParam_s {
	PTRefNum_t      PTRefNum;       /* PT reference number */
	PTType_t		PTType;			/* Type of format to store PT */
	int32_long_ptr  mBlkSize;       /* size of the PT in external memory */
} PTGetSizeFParam_t, FAR* PTGetSizeFParam_long_ptr_t;

typedef struct PTGetPTParam_s {
	PTRefNum_t      PTRefNum;       /* PT reference number */
	int32           mBlkSize;       /* size of the PT in external memory */
	PTAddr_t        PTAddr;         /* external PT address */
} PTGetPTParam_t, FAR* PTGetPTParam_long_ptr_t;

typedef struct PTGetPTFParam_s {
	PTRefNum_t      PTRefNum;       /* PT reference number */
	PTType_t		PTType;			/* Type of format to store PT */
	int32           mBlkSize;       /* size of the PT in external memory */
	PTAddr_t        PTAddr;         /* external PT address */
} PTGetPTFParam_t, FAR* PTGetPTFParam_long_ptr_t;

typedef struct PTEvalRdyParam_s {
	opRefNum_t      opRefNum;				/* reference # for this operation */
	int32_long_ptr	progress;				/* evaluation progress */
} PTEvalRdyParam_t, FAR* PTEvalRdyParam_long_ptr_t;

typedef struct PTEvalParam_s {
	PTRefNum_t            PTRefNum; /* PT reference number */
	PTEvalPB_long_ptr_t   evalDef;  /* address of evaluation parameter block */
	PTEvalTypes_t         evalID;   /* evaluator ID */
	int32                 devNum;   /* evaluator # */
	int32                 aSync;    /* synchronous/asynchronous flag */
	opRefNum_long_ptr_t   opRefNum; /* reference # for this operation */
	PTProgress_t          progress; /* progress callback function */
#if defined(KPMAC)
	long					hostA5;
	long					hostA4;
#endif
} PTEvalParam_t, FAR* PTEvalParam_long_ptr_t;

typedef struct PTEvalDTParam_s {
	PTRefNum_t            PTRefNum; /* PT reference number */
	PTEvalDTPB_long_ptr_t   evalDef;  /* address of evaluation parameter block */
	PTEvalTypes_t         evalID;   /* evaluator ID */
	int32                 devNum;   /* evaluator # */
	int32                 aSync;    /* synchronous/asynchronous flag */
	opRefNum_long_ptr_t   opRefNum; /* reference # for this operation */
	PTProgress_t          progress; /* progress callback function */
#if defined(KPMAC)
	long					hostA5;
	long					hostA4;
#endif
} PTEvalDTParam_t, FAR* PTEvalDTParam_long_ptr_t;

typedef struct PTEvalCancelParam_s {
	opRefNum_t      opRefNum;          /* reference # for this operation */
} PTEvalCancelParam_t, FAR* PTEvalCancelParam_long_ptr_t;

typedef struct PTCombineParam_s {
	int32                   mode;      /* compose mode */
	PTRefNum_t              PTRefNum1; /* PT reference number, PT 1 */
	PTRefNum_t              PTRefNum2; /* PT reference number, PT 2 */
	PTRefNum_long_ptr_t     PTRefNumR; /* PT reference number, resultant PT */
} PTCombineParam_t, FAR* PTCombineParam_long_ptr_t;

typedef struct PTChainValidateParam_s {
	int32                   nPT;       /* number of PT's */
	PTRefNum_long_ptr_t     PTList;    /* list of PT's */
	int32_long_ptr          index;     /* index of PT at which error occurred */
} PTChainValidateParam_t, FAR* PTChainValidateParam_long_ptr_t;

typedef struct PTChainInitParam_s {
	int32                   nPT;       /* number of PT's */
	PTRefNum_long_ptr_t     PTList;    /* list of PT's */
	int32                   validate;  /* validate/no validate flag */
	int32_long_ptr          index;     /* index of PT at which error occurred */
} PTChainInitParam_t, FAR* PTChainInitParam_long_ptr_t;

typedef struct PTChainInitMParam_s {
	int32                   nPT;       /* number of PT's */
	PTRefNum_long_ptr_t     PTList;    /* list of PT's */
	int32                   compMode;  /* composition mode */
	int32                   rulesKey;  /* key to enable chainning rules */
} PTChainInitMParam_t, FAR* PTChainInitMParam_long_ptr_t;

typedef struct PTChainParam_s {
	PTRefNum_t      PTRefNum;          /* reference # of next PT to chain */
} PTChainParam_t, FAR* PTChainParam_long_ptr_t;

typedef struct PTChainEndParam_s {
	PTRefNum_long_ptr_t     PTRefNum;  /* reference # of resultant PT */
} PTChainEndParam_t, FAR* PTChainEndParam_long_ptr_t;

typedef struct PTEvaluatorsParam_s {
	int32_long_ptr          nEval;          /* number of evaluator types */
	evalList_long_ptr_t     evalList;       /* type and number of type */
} PTEvaluatorsParam_t, FAR* PTEvaluatorsParam_long_ptr_t;

typedef struct PTNewEmptyParam_s {
	int32								ndim;					/* number of dimensions for each channel */
	int32_long_ptr 			dim;					/* list of dimension sizes */
	int32								nchan;				/* number of output channels */
	PTRefNum_long_ptr_t	PTRefNum;			/* returned PT reference number */ 
} PTNewEmptyParam_t, FAR* PTNewEmptyParam_long_ptr_t;

typedef struct PTNewEmptySepParam_s {
	int32								nchan;				/* number of output channels */
	int32_long_ptr 			dim;					/* list of dimension sizes */
	PTRefNum_long_ptr_t	PTRefNum;			/* returned PT reference number */ 
} PTNewEmptySepParam_t, FAR* PTNewEmptySepParam_long_ptr_t;

typedef struct PTGetItblParam_s {
	PTRefNum_t			PTRefNum;			/* PT reference number */ 
	int32						ochan;				/* output channel selector */
	int32						ichan;				/* input channel selector */
	KcmHandle FAR*	itblDat;			/* returned table address */
} PTGetItblParam_t, FAR* PTGetItblParam_long_ptr_t;

typedef struct PTGetGtblParam_s {
	PTRefNum_t      PTRefNum;		/* PT reference number */
	int32 					ochan;			/* output channel */
	int32_long_ptr	nDim;				/* returned # dimensions in grid table */
	int32_long_ptr	dimList;		/* returned list of dimension sizes */
	KcmHandle FAR*	gtblDat;		/* returned table address */
} PTGetGtblParam_t, FAR* PTGetGtblParam_long_ptr_t;

typedef struct PTGetOtblParam_s {
	PTRefNum_t   		PTRefNum;		/* PT reference number */
	int32 					ochan;		/* output channel */
	KcmHandle FAR*	otblDat;			/* returned table address */
} PTGetOtblParam_t, FAR* PTGetOtblParam_long_ptr_t;

typedef struct PTApplInitializeParam_s {
	KcmHandle	instanceTable;		/* handle to bridge's instanceTable */
	int32		instanceIndex;			/* current position in instanceTable_h */
} PTApplInitializeParam_t, FAR* PTApplInitializeParam_ptr_t;

typedef struct PTNewMatGamPTParam_s {
	FixedXYZColor_p		rXYZ;
	FixedXYZColor_p		gXYZ;
	FixedXYZColor_p		bXYZ;
	ResponseRecord_p	rTRC;
	ResponseRecord_p	gTRC;
	ResponseRecord_p	bTRC;
	u_int32				gridsize;
	bool				invert;
	PTRefNum_p			thePTRefNumP;
} PTNewMatGamPTParam_t, FAR* PTNewMatGamPTParam_long_ptr_t;

typedef struct PTNewMatGamAIPTParam_s {
	FixedXYZColor_p		rXYZ;
	FixedXYZColor_p		gXYZ;
	FixedXYZColor_p		bXYZ;
	ResponseRecord_p	rTRC;
	ResponseRecord_p	gTRC;
	ResponseRecord_p	bTRC;
	KpUInt32_t			gridsize;
	bool				invert;
	newMGmode_p			newMGmode;
	PTRefNum_p			thePTRefNumP;
} PTNewMatGamAIPTParam_t, FAR* PTNewMatGamAIPTParam_p;

typedef struct PTNewMonoPTParam_s {
	ResponseRecord_p	grayTRC;
	KpUInt32_t			gridsize;
	bool				invert;
	PTRefNum_p			thePTRefNumP;
} PTNewMonoPTParam_t, FAR* PTNewMonoPTParam_p;

typedef struct PTGetRelToAbsPTParam_s {
	KpInt32_t			RelToAbsMode;
	PTRelToAbs_p		PTRelToAbs;
	PTRefNum_p			PTRefNumPtr;
} PTGetRelToAbsPTParam_t, FAR* PTGetRelToAbsPTParam_p;

typedef struct PTGetFlavorParam_s {
	KpInt32_t			*kcpFlavor;
} PTGetFlavorParam_t, FAR* PTGetFlavorParam_p;

#if defined (KPMACPPC)
#pragma options align=reset	/* set alignment back to original in case of succeeding #includes */
#endif

#endif  /* KCMCPR_D_H_ */

