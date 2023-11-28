
#ifndef _KCPCACHE_H_
#define _KCPCACHE_H_ 1


#include "kcmptdef.h"
#include "kcptmgr.h"
#include "attrib.h"
#include "kcpfut.h"
	

#define KCP_MAX_12BIT (0xfff)
#define KCP_MAX_16BIT (0xffff)

#define KCP_SCALE_OTBL_DATA(data, maxData) \
		data = ((data * maxData) + (FUT_MAX_PEL12/2)) / FUT_MAX_PEL12; \
		if (data > maxData) { \
			data = maxData; \
		}

#define KCP_EXTRACT_COMPONENT(data, dataBits, position) \
	((data >> position) & ((1 << dataBits) -1))

#define KCP_CONVERT_DOWN(data, startBits, endBits) \
	((data + ((1 << (startBits - endBits -1)) - (data >> endBits))) >> (startBits - endBits))

/* the following macro will only work if (startBits - (endBits - startBits)) >= 0
 * which means endBits can be no more than twice startBits
 * Use the macro successively if this condition is not met */
#define KCP_CONVERT_UP(data, startBits, endBits) \
((data << (endBits-startBits)) + ((data >> (startBits - (endBits - startBits)))))

/* image pointers definition */
typedef union imagePtr_u {
	PTImgAddr_t pI;
	KpUInt8_p	p8;
	KpUInt16_p	p16;
	KpUInt32_p	p32;
} imagePtr_t, FAR* imagePtr_p;

/* Evaluation lut definition  */
typedef union evalOLutPtr_u {
	KpUInt8_p	p8;
	KpUInt16_p	p16;
} evalOLutPtr_t, FAR* evalOLutPtr_p; 


#if defined (KCP_EVAL_TH1)

/* Number of input variables for each output channel */
#define TH1_MAX_INPUT_VARS (4)

/* Maximum number of output channels */
#define TH1_MAX_OUTPUT_CHANS (8)

/* The overhead associated with the cache means that below some
	threshold evaluation will be faster using non-caching evaluation */
#define TH1_MIN_EVALS (1500)

/* number of offsets in the interpolation volume */
#define TH1_NUM_OFFSETS (2*2*2*2)

/* #define TH1_FP_GRID_DATA 1	/* undefine to use integer */

#if defined (TH1_FP_GRID_DATA)
	typedef KpFloat32_t ecGridData_t, FAR* ecGridData_p;	/* floating point grid table data */
	typedef KpFloat32_t ecInterp_t, FAR* ecInterp_p;		/* floating point interpolant */
#else
	typedef KpInt16_t ecGridData_t, FAR* ecGridData_p;	/* integer grid table data */
	typedef KpInt32_t ecInterp_t, FAR* ecInterp_p;		/* integer interpolant */
#endif

typedef struct ecItbl_s {
	KpInt32_t	index;	/* index into grid table */
	ecInterp_t	frac;	/* the interpolant */
} ecItbl_t, FAR* ecItbl_p;

typedef struct ecMem_s {
	KpInt32_t		bytes;
	KpGenericPtr_t	P;
	KpHandle_t		H;
} ecMem_t, FAR* ecMem_p;

/* this stuff is used for 4-input evaluation using the same method as TH1 */
#define TH1_4D_COMBINATIONS 64
#define TH1_4D_PENTAHEDROA	24

/*  flags for input lut size  */
#define EVAL_LUT_8BIT  0
#define EVAL_LUT_16BIT 1     /* using 16 bit input data */

typedef struct th1_4dControl_s {
	KpUInt32_t	tvert1, tvert2, tvert3, tvert4;	/* offsets from the base grid point to the pentahedral corners */
	KpUInt32_t	dx, dy, dz, dt;
} th1_4dControl_t, FAR* th1_4dControl_p, FAR* FAR* th1_4dControl_h;

typedef struct th1Cache_s {
	fut_ptr_t	cachedFut;	/* A pointer to the fut assigned to the cache. */
	KpInt32_t	futRefNum;	/* Unique ID of the fut assigned to the cache */
	KpInt32_t	futModNum;
	KpUInt32_t	iomask;		/* iomask of the evaluation assigned to the cache */
	KpUInt32_t	dataSizeI;	/* input and output data sizes */
	KpUInt32_t	dataSizeO;
	KpUInt32_t	optimizedEval;	/* 1 if using optimized evaluation function */

	KpInt32_t	numInputs;	/* Number of input channels of the fut assigned to the cache. */
	KpInt32_t	numOutputs;	/* Number of output channels of the fut assigned to the cache. */

	ecMem_t		inputLuts;			/* Input lookup tables of the cached fut for a given evaluation */
	KpInt32_t	inputTableEntries;	/* # of entries in each input table */
	
	ecMem_t		gridLuts;	/* grid lookup tables for each channel */

	ecMem_t		outputLuts;	/* Output lookup tables of the cached fut for a given evaluation */

	KpInt32_t	gridOffsets[TH1_NUM_OFFSETS];	/* Offset in grid from base of a cell to next entry in each dimension */

	KpUInt32_p		pentahedron;				/* pentahedron finder */
	th1_4dControl_t	finder[TH1_4D_PENTAHEDROA];	/* finder tables for pentahedron data */

	struct th1Cache_s FAR* FAR* th1CacheSvH;	/* saved handle to this structure, for ease of access */

}	th1Cache_t, FAR* th1Cache_p, FAR* FAR* th1Cache_h;


typedef void (*evalTh1Proc_t) (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
typedef evalTh1Proc_t FAR* evalTh1Proc_p;
typedef void (*formatFunc_t) (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);

typedef struct evalControl_s {
	threadGlobals_p	threadGlobalsP;
	#if defined (KCP_EVAL_TH1)
	evalTh1Proc_t	evalFunc;
	th1Cache_p		th1CacheP;
	#endif
	KpUInt32_t		compatibleDataType;
	KpUInt32_t		optimizedEval;
	KpInt32_t		imageLines, imagePels;
	imagePtr_t		inputData[FUT_NICHAN], outputData[FUT_NOCHAN];
	KpInt32_t		inPelStride[FUT_NICHAN], inLineStride[FUT_NICHAN], outPelStride[FUT_NOCHAN], outLineStride[FUT_NOCHAN];

	/* needed only if not compatible */
	KpInt32_t		nFuts;
	futEvalInfo_p	futEvalList;
	formatFunc_t	formatFuncI, formatFuncO;
	KpInt32_t		nInputs, nOutputs;
	KpUInt32_t		evalDataTypeI, evalDataTypeO;
	KpInt32_t		tempPelStride[FUT_NOCHAN];
	PTErr_t			errnum;
} evalControl_t, FAR* evalControl_p, FAR* FAR* evalControl_h;

#if !defined(KPNONANSIC)

th1Cache_p	th1AllocCache (KpHandle_t FAR*);
void		th1FreeCache (KpHandle_t);
th1Cache_p	th1LockCache (th1Cache_h);
void		th1UnLockCache (th1Cache_p);
PTErr_t		kcpCheckTh1Cache (KpHandle_t, KpInt32_t, KpInt32_t, KpUInt32_t, KpUInt32_t, evalControl_p);
PTErr_t		kcpInitTh1Cache (fut_ptr_t, KpInt32_t, evalControl_p);

void evalTh1gen (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o1d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o1d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o2d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o2d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o3d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o3d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1iB24oB24 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1iL24oL24 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1iQDoQD (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1iQDo3 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3oQD (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o4d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o4d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o5d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o5d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o6d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o6d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o7d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o7d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o8d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i3o8d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o1d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o1d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o2d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o2d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o3d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o3d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o3QD (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1iB32oB32 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1iL32oL32 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o4d8 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);
void evalTh1i4o4d16 (imagePtr_p, KpInt32_p, imagePtr_p, KpInt32_p, KpInt32_t, th1Cache_p);

#endif

/* Macros used by 3-input functions */

#if defined (TH1_FP_GRID_DATA)
#define TH1_3D_TMP_DATA ecGridData_t	tvert0Data, tvert1Data, tvert2Data;
#else
#define TH1_3D_TMP_DATA KpInt32_t	mullacc, tvertData;
#endif


#define TH1_STORE_DATA8(chan) \
	*outp##chan## = prevRes##chan##;	/* write to buffer */ \
	outp##chan## = (KpUInt8_p)outp##chan## + outStride##chan##;	/* next location */

#define TH1_STORE_DATA16(chan) \
	*outp##chan## = prevRes##chan##;	/* write to buffer */ \
	outp##chan## = (KpUInt16_p)((KpUInt8_p)outp##chan## + outStride##chan##);	/* next location */

#define TH1_EVAL_INIT \
ecItbl_p	inLut0 = th1Cache->inputLuts.P; \
KpUInt8_p	baseP, gridBase = (KpUInt8_p)th1Cache->gridLuts.P; \
KpUInt32_t	i, data0, data1, data2; \
KpInt32_t	tResult, tvert1, tvert2, tvert3; \
ecInterp_t	Xf, Yf, Zf; \
KpInt32_t	inStride0 = inStride[0], inStride1 = inStride[1], inStride2 = inStride[2];

#define TH1_3D_VARIABLES ecInterp_t	xf, yf, zf; \
KpUInt32_t	a001, a010, a011, a100, a101, a110;

#define TH1_3D_GRID_OFFSETS_INIT \
	a001 = th1Cache->gridOffsets[1];	/* copy grid offsets into locals */ \
	a010 = th1Cache->gridOffsets[2]; \
	a011 = th1Cache->gridOffsets[3]; \
	a100 = th1Cache->gridOffsets[4]; \
	a101 = th1Cache->gridOffsets[5]; \
	a110 = th1Cache->gridOffsets[6]; \
	tvert3 = th1Cache->gridOffsets[7];

#define TH1_3D_INIT \
TH1_EVAL_INIT \
TH1_3D_TMP_DATA \
TH1_3D_VARIABLES \
TH1_3D_GRID_OFFSETS_INIT

#define TH1_3D_INITD8 \
KpUInt8_p	inp0 = inp[0].p8, inp1 = inp[1].p8, inp2 = inp[2].p8; \
KpUInt8_p	outLut0 = (KpUInt8_p)th1Cache->outputLuts.P; \
KpUInt32_t	thisColor, prevColor = 0xffffffff; \
 \
	TH1_3D_INIT

#define TH1_3D_INITD16 \
KpUInt16_p	inp0 = inp[0].p16, inp1 = inp[1].p16, inp2 = inp[2].p16; \
ecItbl_p	inLut1, inLut2; \
KpUInt16_p	outLut0 = (KpUInt16_p)th1Cache->outputLuts.P; \
KpUInt32_t	ColorPart1, prevPart1 = 0, prevPart2 = 0xffffffff, dataMask;  \
 \
	TH1_3D_INIT \
 \
	inLut1 = inLut0 + th1Cache->inputTableEntries; \
	inLut2 = inLut1 + th1Cache->inputTableEntries; \
 \
	if (th1Cache->dataSizeI == KCM_USHORT_12) {	/* set up data mask to prevent input table memory access violations */ \
		dataMask = 0xfff; \
	} \
	else { \
		dataMask = 0xffff; \
	}


#define TH1_3D_GETDATAD8 \
	data0 = *inp0; 								/* get channel 0 input data */ \
	inp0 += inStride0; \
	data1 = *inp1; 								/* get channel 1 input data */ \
	inp1 += inStride1; \
	data2 = *inp2; 								/* get channel 2 input data */ \
	inp2 += inStride2; \
 \
	thisColor = (data0 << 16) | (data1 << 8) | (data2);	/* calc this color   */ 


#define TH1_3D_GETDATAD16 \
	data0 = *inp0; 								/* get channel 0 input data */ \
	data0 &= dataMask; \
	inp0 = (KpUInt16_p)((KpUInt8_p)inp0 + inStride0); \
	data1 = *inp1; 								/* get channel 1 input data */ \
	data1 &= dataMask; \
	inp1 = (KpUInt16_p)((KpUInt8_p)inp1 + inStride1); \
	data2 = *inp2; 								/* get channel 2 input data */ \
	data2 &= dataMask; \
	inp2 = (KpUInt16_p)((KpUInt8_p)inp2 + inStride2); \
\
	ColorPart1 = (data0 << 16) | (data1);	/* calc this color */

	
#define TH1_3D_FINDTETRAD8    \
\
	prevColor = thisColor;    \
\
	baseP = gridBase; \
	baseP += inLut0[(0*FUT_INPTBL_ENT) + data0].index; 	/* pass input data through input tables */ \
	Xf = inLut0[(0*FUT_INPTBL_ENT) + data0].frac; \
	baseP += inLut0[(1*FUT_INPTBL_ENT) + data1].index; \
	Yf = inLut0[(1*FUT_INPTBL_ENT) + data1].frac; \
	baseP += inLut0[(2*FUT_INPTBL_ENT) + data2].index; \
	Zf = inLut0[(2*FUT_INPTBL_ENT) + data2].frac; \
\
	TH1_3D_CALCTETRA


#define TH1_3D_FINDTETRAD16 \
\
	prevPart1 = ColorPart1;  \
	prevPart2 = data2;  \
\
	baseP = gridBase; \
	baseP += inLut0[data0].index;		/* pass input data through input tables */ \
	Xf = inLut0[data0].frac; \
	baseP += inLut1[data1].index; \
	Yf = inLut1[data1].frac; \
	baseP += inLut2[data2].index; \
	Zf = inLut2[data2].frac; \
\
	TH1_3D_CALCTETRA 	


#define	TH1_3D_CALCTETRA   \
	/* find the tetrahedron in which the point is located */ \
	if (Xf > Yf) { \
		if (Yf > Zf) {	/* AHEG */ \
			xf = Xf;		/* unchanged order */ \
			yf = Yf; \
			zf = Zf; \
\
			tvert1 = a110; \
			tvert2 = a100; \
		} \
		else { \
			zf = Yf;		/* y into z */ \
			tvert1 = a101; \
			if (Xf > Zf) {	/* AHEF */ \
				xf = Xf;		/* x does not change */ \
				yf = Zf;		/* z into y */ \
\
				tvert2 = a100; \
			} \
			else {			/* AHBF */ \
				xf = Zf;		/* z into x */ \
				yf = Xf;		/* x into y */ \
\
				tvert2 = a001; \
			} \
		} \
	} \
	else { \
		if (Yf > Zf) { \
			xf = Yf;		/* y into x */ \
			tvert2 = a010; \
			if (Xf > Zf) {	/* AHCG */ \
				yf = Xf;		/* x into y */ \
				zf = Zf;		/* z into z */ \
\
				tvert1 = a110; \
			} \
			else {			/* AHCD */ \
				yf = Zf;		/* z into y */ \
				zf = Xf;		/* x into z */ \
\
				tvert1 = a011; \
			} \
		} \
		else {			/* AHDB */ \
			xf = Zf;		/* z into x */ \
			yf = Yf;		/* y into y */ \
			zf = Xf;		/* x into z */ \
\
			tvert1 = a011; \
			tvert2 = a001; \
		} \
	}

#if defined (TH1_FP_GRID_DATA)

#define TH1_3D_TETRAINTERP \
	tvert2Data = *(ecGridData_p)(baseP + tvert1); \
	tvert1Data = zf * (*(ecGridData_p)(baseP + tvert3) - tvert2Data);	/* (H - tvert1) * z */ \
 \
	tvert0Data = *(ecGridData_p)(baseP + tvert2); \
	tvert1Data += (yf * (tvert2Data - tvert0Data));					/* (tvert1 - tvert2) * y */ \
 \
	tvert2Data = *(ecGridData_p)(baseP + 0); \
	tvert1Data += (xf * (tvert0Data - tvert2Data));					/* (tvert2 - A) * x */ \
 \
	tResult = tvert2Data + tvert1Data + 0.49999;					/* A + (mults) */

#else

#define TH1_3D_TETRAINTERP \
	tResult = (KpInt32_t)*(ecGridData_p)(baseP + tvert1); \
	mullacc = zf * ((KpInt32_t)*(ecGridData_p)(baseP + tvert3) - tResult);	/* (tvert3 - tvert1) * z */ \
 \
	tvertData = (KpInt32_t)*(ecGridData_p)(baseP + tvert2); \
	mullacc += (yf * (tResult - tvertData));					/* (tvert1 - tvert2) * y */ \
 \
	tResult = (KpInt32_t)*(ecGridData_p)(baseP + 0); \
	mullacc += (xf * (tvertData - tResult));					/* (tvert2 - A) * x */ \
 \
	tResult += ((mullacc + (1 << 15)) >> 16);					/* A + (mults) */

#endif

#define TH1_3D_TETRAINTERP_AND_OLUT(chan) \
	baseP += sizeof (ecGridData_t); \
	TH1_3D_TETRAINTERP		/* tetrahedral interpolation for this channel */ \
	prevRes##chan## = outLut0[(##chan##*4096)+tResult];
	

/* Macros used by 4-input functions */

#if defined (TH1_FP_GRID_DATA)
#define TH1_4D_TMP_DATA TH1_3D_TMP_DATA ecGridData_t	tvert3Data, tvert4Data;
#else
#define TH1_4D_TMP_DATA TH1_3D_TMP_DATA
#endif


#define TH1_4D_INIT \
TH1_EVAL_INIT \
TH1_4D_TMP_DATA \
KpUInt32_t	data3; \
KpInt32_t	tvert4; \
ecInterp_t	interp[4], Tf; \
KpInt32_t	inStride3 = inStride[3]; \
KpUInt32_t	index; \
th1_4dControl_p	pentaInfo, finderP; \
KpUInt32_p	pentahedronP; \
 \
	pentahedronP = th1Cache->pentahedron; \
	finderP = &th1Cache->finder[-1];	/* -1 due to Matlab indexing starting at 1 */

#define TH1_4D_INITD8 \
KpUInt8_p	inp0 = inp[0].p8, inp1 = inp[1].p8, inp2 = inp[2].p8, inp3 = inp[3].p8; \
KpUInt8_p	outLut0 = (KpUInt8_p)th1Cache->outputLuts.P; \
KpUInt32_t	thisColor, prevColor; \
 \
	TH1_4D_INIT \
	prevColor = (~(*inp0)) << 24; 			/* make sure cache is not valid */

#define TH1_4D_INITD16 \
KpUInt16_p	inp0 = inp[0].p16, inp1 = inp[1].p16, inp2 = inp[2].p16, inp3 = inp[3].p16; \
ecItbl_p	inLut1, inLut2, inLut3; \
KpUInt16_p	outLut0 = (KpUInt16_p)th1Cache->outputLuts.P; \
KpUInt32_t	thisColor1, thisColor2, prevColor1, prevColor2 = 0, dataMask; \
 \
	TH1_4D_INIT \
 \
	inLut1 = inLut0 + th1Cache->inputTableEntries; \
	inLut2 = inLut1 + th1Cache->inputTableEntries; \
	inLut3 = inLut2 + th1Cache->inputTableEntries; \
 \
	if (th1Cache->dataSizeI == KCM_USHORT_12) {	/* set up data mask to prevent input table memory access violations */ \
		dataMask = 0xfff; \
	} \
	else { \
		dataMask = 0xffff; \
	} \
 \
	prevColor1 = (~(*inp0)) << 16; 			/* make sure cache is not valid */

#define TH1_STORE_DATA8(chan) \
	*outp##chan## = prevRes##chan##;	/* write to buffer */ \
	outp##chan## = (KpUInt8_p)outp##chan## + outStride##chan##;	/* next location */

#define TH1_STORE_DATA16(chan) \
	*outp##chan## = prevRes##chan##;	/* write to buffer */ \
	outp##chan## = (KpUInt16_p)((KpUInt8_p)outp##chan## + outStride##chan##);	/* next location */

#define TH1_4D_GETDATA8 \
	data0 = *inp0; 					/* get channel 0 input data */ \
	inp0 += inStride0; \
	data1 = *inp1; 					/* get channel 1 input data */ \
	inp1 += inStride1; \
	data2 = *inp2; 					/* get channel 2 input data */ \
	inp2 += inStride2; \
	data3 = *inp3; 					/* get channel 2 input data */ \
	inp3 += inStride3; \
\
	thisColor = (data0 << 24) | (data1 << 16) | (data2 << 8) | (data3);	/* calc this color */

#define TH1_4D_GETDATA16 \
	data0 = *inp0; 					/* get channel 0 input data */ \
	data0 &= dataMask; \
	inp0 = (KpUInt16_p)((KpUInt8_p)inp0 + inStride0); \
	data1 = *inp1; 					/* get channel 1 input data */ \
	data1 &= dataMask; \
	inp1 = (KpUInt16_p)((KpUInt8_p)inp1 + inStride1); \
	data2 = *inp2; 					/* get channel 2 input data */ \
	data2 &= dataMask; \
	inp2 = (KpUInt16_p)((KpUInt8_p)inp2 + inStride2); \
	data3 = *inp3; 					/* get channel 2 input data */ \
	data3 &= dataMask; \
	inp3 = (KpUInt16_p)((KpUInt8_p)inp3 + inStride3); \
\
	thisColor1 = (data0 << 16) | (data1);	/* calc this color */  \
    thisColor2 = (data2 << 16) | (data3);

#define TH1_4D_FINDTETRAD8 \
	prevColor = thisColor;  \
\
	baseP = gridBase; \
	baseP += inLut0[(0*FUT_INPTBL_ENT) + data0].index; 	/* pass input data through input tables */ \
	Xf = inLut0[(0*FUT_INPTBL_ENT) + data0].frac; \
	baseP += inLut0[(1*FUT_INPTBL_ENT) + data1].index; \
	Yf = inLut0[(1*FUT_INPTBL_ENT) + data1].frac; \
	baseP += inLut0[(2*FUT_INPTBL_ENT) + data2].index; \
	Zf = inLut0[(2*FUT_INPTBL_ENT) + data2].frac; \
	baseP += inLut0[(3*FUT_INPTBL_ENT) + data3].index; \
	Tf = inLut0[(3*FUT_INPTBL_ENT) + data3].frac; \
\
	TH1_4D_CALCTETRA

#define TH1_4D_FINDTETRAD16 \
	prevColor1 = thisColor1;  \
	prevColor2 = thisColor2;  \
\
	baseP = gridBase; \
	baseP += inLut0[data0].index;		/* pass input data through input tables */ \
	Xf = inLut0[data0].frac; \
	baseP += inLut1[data1].index; \
	Yf = inLut1[data1].frac; \
	baseP += inLut2[data2].index; \
	Zf = inLut2[data2].frac; \
	baseP += inLut3[data3].index; \
	Tf = inLut3[data3].frac; \
\
	TH1_4D_CALCTETRA

#define TH1_4D_CALCTETRA \
	/* find the pentahedron in which the point is located */ \
	/* this builds a binary number based of the possible comparisons of the 4 interpolants */ \
	index = 0; \
	if (Xf > Yf) { \
		index += (1<<5); \
	} \
	if (Zf > Tf) { \
		index += (1<<4); \
	} \
	if (Xf > Zf) { \
		index += (1<<3); \
	} \
	if (Yf > Tf) { \
		index += (1<<2); \
	} \
	if (Yf > Zf) { \
		index += (1<<1); \
	} \
	if (Xf > Tf) { \
		index += (1<<0); \
	} \
\
	pentaInfo = finderP + pentahedronP[index];	/* get pentahedron info */ \
\
	tvert1 = pentaInfo->tvert1;	/* offsets from the base grid point to the pentahedral corners */ \
	tvert2 = pentaInfo->tvert2; \
	tvert3 = pentaInfo->tvert3; \
	tvert4 = pentaInfo->tvert4; \
\
	interp[pentaInfo->dx] = Xf;	/* re-dorder the interpolants */ \
	interp[pentaInfo->dy] = Yf; \
	interp[pentaInfo->dz] = Zf; \
	interp[pentaInfo->dt] = Tf; \
	Tf = interp[0]; \
	Zf = interp[1]; \
	Yf = interp[2]; \
	Xf = interp[3];

#if defined (TH1_FP_GRID_DATA)

#define TH1_4D_TETRAINTERP \
	tvert0Data = *(ecGridData_p)(baseP + 0); \
	tvert1Data = *(ecGridData_p)(baseP + tvert1); \
	tvert2Data = *(ecGridData_p)(baseP + tvert2); \
	tvert3Data = *(ecGridData_p)(baseP + tvert3); \
	tvert4Data = *(ecGridData_p)(baseP + tvert4); \
\
	tResult = 0.49999 + tvert0Data \
			+ ((tvert1Data - tvert0Data) * Xf) \
			+ ((tvert2Data - tvert1Data) * Yf) \
			+ ((tvert3Data - tvert2Data) * Zf) \
			+ ((tvert4Data - tvert3Data) * Tf);

#else

#define TH1_4D_TETRAINTERP \
	tvertData = (KpInt32_t)*(ecGridData_p)(baseP + tvert3); \
	mullacc = Tf * ((KpInt32_t)*(ecGridData_p)(baseP + tvert4) - tvertData);	/* (tvert4 - tvert3) * t */ \
 \
	tResult = (KpInt32_t)*(ecGridData_p)(baseP + tvert2); \
	mullacc += (Zf * (tvertData - tResult));					/* (tvert3 - tvert2) * z */ \
 \
	tvertData = (KpInt32_t)*(ecGridData_p)(baseP + tvert1); \
	mullacc += (Yf * (tResult - tvertData));					/* (tvert2 - tvert1) * y */ \
 \
	tResult = (KpInt32_t)*(ecGridData_p)(baseP + 0); \
	mullacc += (Xf * (tvertData - tResult));					/* (tvert1 - tvert0) * x */ \
 \
	tResult += ((mullacc + (1 << 15)) >> 16);					/* tvert0 + (mullacc) */
#endif

#define TH1_4D_TETRAINTERP_AND_OLUT(chan) \
	baseP += sizeof (ecGridData_t); \
	TH1_4D_TETRAINTERP		/* tetrahedral interpolation for this channel */ \
	prevRes##chan## = outLut0[(##chan##*4096)+tResult];


#endif


void pass8in (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format555to8 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format565to8 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format8to12 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void pass16in (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format16to12 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format555to12 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format565to12 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format10to12 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void pass8out (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format8to555 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format8to565 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format12to8 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void pass16out (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format12to16 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format12to555 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format12to565 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);
void format12to10 (KpInt32_t, KpInt32_t, imagePtr_p, KpInt32_p, imagePtr_p);

#endif	/* _KCPCACHE_H_ */

