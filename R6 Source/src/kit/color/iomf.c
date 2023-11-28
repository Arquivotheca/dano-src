/*
  File:         iomf.c          @(#)iomf.c	1.40 06/27/97
  Author:       George Pawle

  This file contains functions to read and write binary matrix fut files.

  All opens, closes, reads and writes are performed with the functions
  Kp_open, Kp_close, Kp_read, and Kp_write, respectively
  to provide an "operating system independent" i/o interface.  These
  functions are implemented differently for each operating system, and are
  defined in the library kcms_sys.

  PROPRIETARY NOTICE: The  software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on their
  designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1993-1995 Eastman Kodak Company.
  As  an  unpublished  work pursuant to Title 17 of the United
  States Code.  All rights reserved.

 */

#include "fut.h"
#include "fut_util.h"                    /* internal interface file */
#include "fut_io.h" 

#define FUT_MATRIX_ZERO (0x0)
#define FUT_MATRIX_ONE (0x10000)
#define GBUFFER_SIZE (MF1_TBL_ENT*2)

static int32 getITblFactors     ARGS((fut_ptr_t, int32, int32_p, int32_p, int32_p, int32_p));
static KpInt32_t calcNextGBufSize ( KpInt32_t curBufSize, KpInt32_p totalBytesRemaining);
static KpInt32_t convertMFutItbls (fut_itbldat_ptr_t src, fut_itbldat_ptr_t des, KpUInt32_t entries);

/*
 * fut_loadMFut loads a matrix fut from the file named "filename", performing the
 * open and close automatically.  It returns a pointer to the loaded fut and the matrix
 * or NULL on error.
 */
fut_ptr_t
	fut_loadMFut (	char_p	filename,
					Fixed_p	matrixP)
{
fut_ptr_t  fut;
ioFileChar	fileProps;
char_p realFileName;

	realFileName = setDefaultFP (&fileProps, filename);

	fut = fut_loadMFut_fp (realFileName, fileProps, matrixP);

	return (fut);
}

/*
 * fut_loadMFut_fp is like fut_loadMFut but
 * includes file properties in a separate function argument.
 */
fut_ptr_t
	fut_loadMFut_fp (	char_p		filename,
						ioFileChar	fileProps,
						Fixed_p		matrixP)
{
fut_ptr_t  fut;
KpFd_t		fd;

	if ( ! Kp_open (filename, "r", &fd, &fileProps) ) {
		return (FUT_NULL);
	}

	fut = fut_readMFut_Kp (&fd, matrixP);

	(void) Kp_close (&fd);

	return (fut);
}


/*
 * fut_readMFut_Kp reads a fut from an open file descriptor, returning a pointer
 * to a newly allocated fut and matrix or NULL on error.
 */
fut_ptr_t
	fut_readMFut_Kp (	KpFd_p	fd,
						Fixed_p	matrixP)
{
fut_ptr_t	fut;
fut_hdr_t	futio;

	if ( ! Kp_read (fd, (fut_generic_ptr_t)&futio.magic, sizeof(int32))) {
		return (FUT_NULL);
	}

	if ((futio.magic == FUT_CIGAM) || (futio.magic == FUT_MAGIC) ) {
		fut = fut_read_fut (fd, &futio);                                        /* read a fut */
		makeIdentityMatrix (matrixP, MF_MATRIX_DIM);
	}
	else {
#if (FUT_MSBF == 0)
		Kp_swab32 ((fut_generic_ptr_t)&futio.magic, 1);
#endif

		switch (futio.magic) {
		case MF1_TBL_ID:        /* 8 bit matrix fut */
		case MF2_TBL_ID:        /* 16 bit matrix fut */
			fut = fut_read_mfut (fd, &futio, matrixP);
			break;

		default:
			return (NULL);          /* unknown type */
		}
	}

	return (fut);
}


/*
 * fut_read_mfut reads a matrix fut in the specified format and stores it in a fut.
 * Returns: 
 * valid fut pointer on success
 * FUT_NULL on error
 */

fut_ptr_t
	fut_read_mfut (	KpFd_p			fd,
					fut_hdr_ptr_t	futioP,
					Fixed_p			matrixP)
{
int32           ret;
fut_ptr_t       fut;

	switch (futioP->magic) {
	case MF1_TBL_ID:        /* 8 bit matrix fut */
	case MF2_TBL_ID:        /* 16 bit matrix fut */
		ret = fut_readMFutHdr (fd, futioP);     /* read the matrix fut header */

		if (ret == 1) {
			ret = fut_readMFutTbls (fd, &fut, futioP, matrixP);      /* read the matrix fut tables */
		} 
		
		if (ret != 1) {
			return (NULL);
		}
			
		break;
		
	default:
		return (NULL);          /* unknown type */
	}

	return (fut);

} /* fut_read_mfut */

/*
 * fut_readMFutHdr reads the header of a matrix fut from
 * an open file descriptor and stores it in a fut I/O header.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_read error
 * -2 to -4 on a table specific error
 */

int32
	fut_readMFutHdr(        KpFd_p      fd,
						fut_hdr_ptr_t   futioP)
{
int32   	ret;
u_int32		dummy;
u_int8  	inVars, outChans, gridDim;

	/* read in the common matrix fut stuff */
	ret = Kp_read (fd, (fut_generic_ptr_t)&dummy, sizeof(u_int32)) &&
		 Kp_read (fd, (fut_generic_ptr_t)&inVars, sizeof(u_int8)) &&
		 Kp_read (fd, (fut_generic_ptr_t)&outChans, sizeof(u_int8)) &&
		 Kp_read (fd, (fut_generic_ptr_t)&gridDim, sizeof(u_int8)) &&
		 Kp_read (fd, (fut_generic_ptr_t)&dummy, sizeof(u_int8));

	futioP->version = 1;      			/* save in fut locations */
	futioP->order = 0;
	if (inVars < 1 || inVars > FUT_NICHAN) {
		return (-2);
	}
	futioP->icode[0] = (int32)inVars;
	if (outChans < 1 || outChans > FUT_NOCHAN) {
		return (-3);
	}
	futioP->icode[1] = (int32)outChans;
	if (gridDim < 2) {
		return (-4);
	}
	futioP->icode[2] = (int32)gridDim;

	return (ret? 1: -1);
	
}       /* fut_readMFutHdr */


/* fut_readMFutTbls reads the tables of a matrix fut
 * from an open file descriptor and puts them into the supplied fut.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_read error
 * -2 to -4 on a table specific error
 */
int32
	fut_readMFutTbls (      KpFd_p              fd,
						fut_ptr_t fut_far*      futP,
						fut_hdr_ptr_t           futioP,
						Fixed_p                         matrix)
{
int32   	ret, i1, ndim, dimTbl[FUT_NICHAN], nchan;
fut_ptr_t   fut;
u_int16		iTblEntries = 0, oTblEntries = 0;

	ndim = futioP->icode[0];         /* copy for appearances */
	nchan = futioP->icode[1];
	for (i1 = 0; i1 < ndim; i1++) {
		dimTbl[i1] = futioP->icode[2];
	}

	fut = fut_new_empty (ndim, dimTbl, nchan);      /* make a fut for the matrix fut */
	if (fut == FUT_NULL) {
		return (0);             /* no luck */
	}

	ret = fut_readMFutMTbls (fd, matrix);   /* read the matrix tables */

	if (ret == 1) {
		switch (futioP->magic) {
		case MF1_TBL_ID:
			break;

		case MF2_TBL_ID:
			ret = Kp_read (fd, (fut_generic_ptr_t)&iTblEntries, sizeof(u_int16));
			if (ret != 1) {
				return (-1);
			}
			ret = Kp_read (fd, (fut_generic_ptr_t)&oTblEntries, sizeof(u_int16));
			if (ret != 1) {
				return (-1);
			}
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
			Kp_swab16 ((fut_generic_ptr_t)&iTblEntries, 1);
			Kp_swab16 ((fut_generic_ptr_t)&oTblEntries, 1);
#endif
			break;

		default:
			return (-2);    /* unknown type */
		}

		ret = fut_readMFutITbls (fd, fut, futioP->magic, (int32)iTblEntries);       /* read the input tables */

		if (ret == 1) {
			ret = fut_readMFutGTbls (fd, fut, futioP->magic);    /* read the grid tables */

			if (ret == 1) {
				ret = fut_readMFutOTbls (fd, fut, futioP->magic, (int32)oTblEntries); /* read the output tables */
			}
		}
	}

	if (ret == 1) {
		*futP = fut;
	} else {
		fut = fut_free (fut);           /* if error, free fut */
	}

	return (ret);   /* error, abort */

}       /* fut_readMFutTbls */


/*
 * fut_readMFutMTbls reads the matrix tables of a matrix fut from
 * an open file descriptor stores it in a matrix array.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_read error
 * -2 to -4 on a table specific error
 */

int32
	fut_readMFutMTbls(      KpFd_p   fd,
						Fixed_p matrix)
{
int32   ret;
Fixed_t lMatrix[MF_MATRIX_DIM * MF_MATRIX_DIM];
Fixed_p lMatrixP;

	if (matrix != NULL) {
		lMatrixP = matrix;                              /* return matrix data */
	}
	else {
		lMatrixP = (Fixed_p)&lMatrix;   /* discard matrix data */
	}

	ret = Kp_read (fd, (fut_generic_ptr_t) lMatrixP,
					sizeof(Fixed_t) * MF_MATRIX_DIM * MF_MATRIX_DIM);

#if (FUT_MSBF == 0)                                             /* swap bytes if necessary */
		Kp_swab32((fut_generic_ptr_t)lMatrixP, MF_MATRIX_DIM * MF_MATRIX_DIM);
#endif

	return (ret ? 1 : -1);
	
}       /* fut_readMFutMTbls */


/* fut_readMFutITbls reads the input tables of a 'fut'
 * to an open file descriptor in the specified format.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_read error
 * -2 to -4 on a table specific error
 */
int32
	fut_readMFutITbls (KpFd_p    		fd,
						fut_ptr_t       fut,
						int32           MFutType,
						int32			iTblEntries)
{
u_int16 tmpTbl[MF2_MAX_TBL_ENT+1];        /* local so we do not have memory allocation problems */
int32   i1, i2, ret, iTableInt, exShift, denominator, dataOver;
int32   iTblSize, index;
u_int8_p        xf3dataP;
u_int16_p       xf4dataP;
fut_itbldat_t	iData;
fut_itbldat_ptr_t iDataP;
fut_itbldat_ptr_t newTblPtr, newTbl2Ptr;
int32   		tmpData, tmpData1, tmpData2, iTableInterp, iTableIndex;
KpUInt32_t		imask;

	switch (MFutType) {
	case MF1_TBL_ID:
		iTblEntries = MF1_TBL_ENT;                  	/* has constant size */
		iTblSize = sizeof(u_int8) * iTblEntries;
		break;

	case MF2_TBL_ID:
		if ((iTblEntries < MF2_MIN_TBL_ENT) || (iTblEntries > MF2_MAX_TBL_ENT)) {
			return (0);
		}

		if (iTblEntries > MF1_TBL_ENT) {
			/* make new itbls */
			imask = FUT_IMASK(fut->iomask.in);
			for (i1 = 0; i1 < FUT_NICHAN; i1++) {
				if ( (imask & FUT_BIT(i1)) == 0 )
					continue;

				/* allocate the new big itbl */
				newTblPtr = fut_alloc_itbldat (ITBLMODE12BIT);
				if (newTblPtr == 0) {
					return (0);
				}

				newTbl2Ptr = newTblPtr + (FUT_INPTBL_ENT+1);

				/* free the old itbl */
				fut_mfree ((fut_generic_ptr_t)fut->itbl[i1]->tbl, "i");

				/* fill the fut structure */
				fut->itbl[i1]->tbl = newTblPtr;
				fut->itbl[i1]->tbl2 = newTbl2Ptr;
				fut->itbl[i1]->tblHandle = getHandleFromPtr((fut_generic_ptr_t)newTblPtr);
				fut->itbl[i1]->tblFlag = iTblEntries;
			}
		}

		iTblSize = iTblEntries * (int32)sizeof(int16); /* convert to bytes */
		break;

	default:
		return (-2);    /* unknown type */
	}

	ret = getITblFactors (fut, MFutType, &iTableInt, &denominator, &exShift, &dataOver);    
	if (ret != 1) {
		return (ret);
	}
	
	/* convert each input table to required precision */
	for (i1 = 0; (fut->itbl[i1] != NULL) && (i1 < FUT_NICHAN); i1++) {
			
		ret = Kp_read (fd, (fut_generic_ptr_t)tmpTbl, iTblSize);   /* read the input table */
		if (ret != 1) {
			return (-1);
		}
		
		if (MFutType == MF2_TBL_ID) {
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
			for (i2 = 0, xf4dataP = (u_int16_p) tmpTbl; i2 < iTblEntries; i2++, xf4dataP++) {
				Kp_swab16 ((fut_generic_ptr_t)xf4dataP, 1);
			}
#endif
			xf4dataP = (u_int16_p) tmpTbl;
			xf4dataP[iTblEntries] = xf4dataP[iTblEntries-1];
		}
		iDataP = fut->itbl[i1]->tbl;    /* get the address of the table data */
		xf3dataP = (u_int8_p) tmpTbl;
		xf4dataP = (u_int16_p) tmpTbl;

		for (i2 = 0, index = 0; i2 < FUT_INPTBL_ENT; i2++, index += iTblEntries-1) {
			
			switch (MFutType) {
			case MF1_TBL_ID:
#if (MF1_TBL_ENT != FUT_INPTBL_ENT)
*** table size mismatch!!!
#endif
				iData = (int32)xf3dataP[i2]; /* convert to 32 bit size */
				break;

			case MF2_TBL_ID:
				iTableIndex = index / (FUT_INPTBL_ENT-1);
				tmpData = (int32) xf4dataP[iTableIndex];
				tmpData1 = (int32) xf4dataP[iTableIndex+1];

				iTableInterp = index - (iTableIndex * (FUT_INPTBL_ENT-1));      /* interpolant */
			
				tmpData2 = (((tmpData1 - tmpData) * iTableInterp) / (FUT_INPTBL_ENT-1));
				iData = tmpData + tmpData2;
				break;

			default:
				return (-2);
			}


			iData <<= exShift;              /* position the data */
			iData *= denominator;   /* scale by multiplying by denominator */
			iData >>= iTableInt;    /* and dividing by numerator */

			*iDataP++ = iData;              /* store each entry */
		}
		*iDataP++ = iData;             		/* repeat last entry (257) */

		/* if big itbls */
		if (fut->itbl[i1]->tblFlag > 0) {
			iDataP = fut->itbl[i1]->tbl2;    /* get the address of the table data */
			xf3dataP = (u_int8_p) tmpTbl;
			xf4dataP = (u_int16_p) tmpTbl;

			for (i2 = 0, index = 0; i2 < FUT_INPTBL_ENT2; i2++, index += iTblEntries-1) {
			
				switch (MFutType) {
				case MF1_TBL_ID:
#if (MF1_TBL_ENT != FUT_INPTBL_ENT)
*** table size mismatch!!!
#endif
					iData = (int32)xf3dataP[i2]; /* convert to 32 bit size */
					break;

				case MF2_TBL_ID:
					iTableIndex = index / (FUT_INPTBL_ENT2-1);
					tmpData = (int32) xf4dataP[iTableIndex];
					tmpData1 = (int32) xf4dataP[iTableIndex+1];

					iTableInterp = index - (iTableIndex * (FUT_INPTBL_ENT2-1));      /* interpolant */
			
					tmpData2 = (((tmpData1 - tmpData) * iTableInterp) / (FUT_INPTBL_ENT2-1));
					iData = tmpData + tmpData2;
					break;
				}


				iData <<= exShift;              /* position the data */
				iData *= denominator;   /* scale by multiplying by denominator */
				iData >>= iTableInt;    /* and dividing by numerator */

				*iDataP++ = iData;              /* store each entry */
			}
			*iDataP++ = iData;             		/* repeat last entry (4097) */
		}
	}

	return (1);             /* success */
	
}       /* fut_readMFutITbls */


/* fut_readMFutGTbls reads the grid tables of a 'fut'
 * from an open file descriptor.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_read error
 * -2 to -4 on a table specific error
 */
int32
	fut_readMFutGTbls (KpFd_p                fd,
						fut_ptr_t       fut,
						int32           MFutType)
{
u_int16  tmpTbl[GBUFFER_SIZE];           /* local to avoid memory allocation problems */
int32			i1, i2, ret = -1, outputChans, readBytes;
int32			inCount, gTblEntries, totalGSize;
unsigned long   tmpData;
u_int8_p        xf3dataP;
u_int16_p       xf4dataP;
u_int16         gData;
fut_gtbldat_ptr_t gDataP[FUT_NOCHAN];
fut_chan_ptr_t chan;

	if ( ! IS_FUT(fut) ) {
		return (0);
	}

	for (outputChans = 0; outputChans < FUT_NOCHAN; outputChans++) {
		if ((chan = fut->chan[outputChans]) == FUT_NULL_CHAN) {
			break;
		}
		gDataP[outputChans] = chan->gtbl->tbl;  /* get each grid table pointer */
	}

	gTblEntries = (fut->chan[0]->gtbl->tbl_size) / (int32)sizeof (fut_gtbldat_t);  /* get the # of entries in the table in the table */
	totalGSize = gTblEntries * outputChans;

	switch (MFutType) {
	case MF1_TBL_ID:
		totalGSize *= sizeof(u_int8);
		break;

	case MF2_TBL_ID:
		totalGSize *= sizeof(u_int16);
		break;

	default:
		return (-2);    /* unknown type */
	}

	readBytes = GBUFFER_SIZE;               /* count bytes read into buffer */
	inCount = readBytes;                    /* force reset 1st time */
	xf3dataP = (u_int8_p) tmpTbl;
	xf4dataP = (u_int16_p) tmpTbl;
	
	for (i1 = 0; i1 < gTblEntries; i1++) {
		for (i2 = 0; i2 < outputChans; i2++) {
			if (inCount == readBytes) {
				inCount = 0;    /* reset for next time */
				xf3dataP = (u_int8_p) tmpTbl;
				xf4dataP = (u_int16_p) tmpTbl;

				totalGSize -= readBytes;
				if (totalGSize <= 0) {
					readBytes += totalGSize;
				}
				ret = Kp_read (fd, (fut_generic_ptr_t)tmpTbl, readBytes);
				if (ret != 1) {
					return (-1);
				}
			}

			switch (MFutType) {
			case MF1_TBL_ID:
				tmpData = (unsigned long) *xf3dataP++;
				tmpData <<= (FUT_GRD_BITS - MF1_TBL_BITS);

				inCount += sizeof(u_int8);                      /* count bytes read from buffer */
				
				break;

			case MF2_TBL_ID:
				gData = (u_int16) *xf4dataP++;
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
				Kp_swab16 ((fut_generic_ptr_t)&gData, 1);
#endif
				tmpData = (unsigned long)gData; /* do not round since noise is added on output */
				tmpData >>= (MF2_TBL_BITS - FUT_GRD_BITS);

				inCount += sizeof(u_int16);                     /* count bytes in buffer */

				break;

			default:
				return (-2);    /* unknown type */
			}

			*(gDataP[i2])++ = (fut_gtbldat_t)tmpData;                       /* store each grid table entry */
			}
	}

	return (ret);
	
}       /* fut_readMFutGTbls */



/* fut_readMFutOTbls reads the output tables of a matrix fut
 * to an open file descriptor in the specified format.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_read error
 * -2 to -4 on a table specific error
 */
int32
	fut_readMFutOTbls (KpFd_p       	fd,
						fut_ptr_t       fut,
						int32           MFutType,
						int32			oTblEntries)
{
#define OBUFFER_SIZE (MF2_MAX_TBL_ENT+1)
u_int16  tmpTbl[OBUFFER_SIZE];   /* local so we do not have memory allocation problems */
int32   i1, ret, outputChans, oTblSize, tmpData, tmpData1, tmpData2, iTableInterp, iTableIndex, index;
u_int8_p        xf3dataP;
u_int16_p       xf4dataP;
fut_otbldat_ptr_t oDataP;
fut_chan_ptr_t chan;

	if (! IS_FUT(fut)) {
		return (0);
	}

	switch (MFutType) {
	case MF1_TBL_ID:
		oTblEntries = MF1_TBL_ENT;                   /* has constant size */
		oTblSize = oTblEntries * (int32)sizeof (u_int8);
				
		break;

	case MF2_TBL_ID:
		if ((oTblEntries < MF2_MIN_TBL_ENT) || (oTblEntries > MF2_MAX_TBL_ENT)) {
			return (-2);                                    /* illegal size */
		}
		
		oTblSize = oTblEntries * (int32)sizeof (u_int16);

		break;

	default:
		return (-2);                                    /* unknown type */
	}
	
	for (outputChans = 0; outputChans < FUT_NOCHAN; outputChans++) {
		if ((chan = fut->chan[outputChans]) == FUT_NULL_CHAN) {
			return (1);             /* all done */
		}

		ret = Kp_read (fd, (fut_generic_ptr_t)tmpTbl, oTblSize);   /* get each table */
		if (ret != 1) {
			return (-1);
		}

		switch (MFutType) {
		case MF1_TBL_ID:
				xf3dataP = (u_int8_p) tmpTbl;
				xf3dataP[oTblEntries] = xf3dataP[oTblEntries-1];        /* duplicate last point for easier interpolation */
				
			break;

		case MF2_TBL_ID:        /* do this first so that table is properly swapped for interpolation */
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
			for (i1 = 0, xf4dataP = (u_int16_p) tmpTbl; i1 < oTblEntries; i1++, xf4dataP++) {
				Kp_swab16 ((fut_generic_ptr_t)xf4dataP, 1);
			}
#endif
			xf4dataP = (u_int16_p) tmpTbl;
			xf4dataP[oTblEntries] = xf4dataP[oTblEntries-1];

			break;

		default:
			return (-2);                                    /* unknown type */
		}

		oDataP = chan->otbl->tbl;       /* get the address of the table data */
		
		for (i1 = 0, index = 0; i1 < FUT_OUTTBL_ENT; i1++, index += oTblEntries-1) {
			iTableIndex = index / (FUT_OUTTBL_ENT-1);
			
			switch (MFutType) {
			case MF1_TBL_ID:
				tmpData = ((int32) xf3dataP[iTableIndex]) << (MF2_TBL_BITS - MF1_TBL_BITS);
				tmpData1 = ((int32) xf3dataP[iTableIndex+1]) << (MF2_TBL_BITS - MF1_TBL_BITS);
				
				break;

			case MF2_TBL_ID:
				tmpData = (int32) (xf4dataP[iTableIndex] 
							& ((FUT_BIT(FUT_GRD_BITS) -1) << FUT_OUT_FRACBITS));
				tmpData1 = (int32) (xf4dataP[iTableIndex+1]
							& ((FUT_BIT(FUT_GRD_BITS) -1) << FUT_OUT_FRACBITS));
				break;

			default:
				return (-2);
			}

			iTableInterp = index - (iTableIndex * (FUT_OUTTBL_ENT-1));      /* interpolant */
			
			tmpData2 = (((tmpData1 - tmpData) * iTableInterp) / (FUT_OUTTBL_ENT-1));
			tmpData += tmpData2;
			tmpData += FUT_BIT (MF2_TBL_BITS - FUT_GRD_BITS -1);
			tmpData >>= (MF2_TBL_BITS - FUT_GRD_BITS);
			if (tmpData > FUT_GRD_MAXVAL) {
				tmpData = FUT_GRD_MAXVAL;
			}
			*oDataP++ = (fut_otbldat_t) tmpData;
		}
	}

	return (1);             /* success */
	
}       /* fut_readMFutOTbls */

/* fut_storeMFut stores fut to the file named "filename", performing the
 * open and close automatically.  Returns 1 on success, 0 or negative
 * on error.
 */
int32
	fut_storeMFut (	fut_ptr_t	fut,
					Fixed_p		matrix,
					int32		MFutType,
					int32		oTblSize,
					char_p		filename)
{
int32 ret;
ioFileChar	fileProps;
char_p realFileName;

	realFileName = setDefaultFP (&fileProps, filename);

	ret = fut_storeMFut_fp (fut, matrix, MFutType, oTblSize, realFileName, fileProps);

	return (ret);
}



/* fut_storeMFut_fp is like fut_storeMFut but
 * includes file properties in a separate function argument.
 * Returns 1 on success, 0 or negative on error.
 */
int32
	fut_storeMFut_fp (	fut_ptr_t	fut,
						Fixed_p		matrix,
						int32		MFutType,
						int32		oTblSize,
						char_p		filename,
						ioFileChar	fileProps)
{
KpFd_t		fd;
int32 ret;

	if ( ! Kp_open (filename, "w", &fd, &fileProps) ) {
		return (-1);
	}

	ret = fut_writeMFut_Kp (&fd, fut, matrix, MFutType, oTblSize);

	(void) Kp_close (&fd);

	return (ret);
}



/*
 * fut_writeMFut_Kp writes a fut in the specified matrix fut format
 * to an open file descriptor.
 * Returns: 
 * 1 on success
 * 0 on invalid fut error
 * -1 on header, id string, or Kp_write error
 * -2 to -5 on a table specific error
 */
int32
	fut_writeMFut_Kp (  KpFd_p           fd,
					fut_ptr_t       fut,
					Fixed_p         matrix,
					int32           MFutType,
					int32           oTblSize)
{
int32   ret = 1;

						/* check for valid fut */
	if ( ! IS_FUT(fut) ) {
		return (0);
	}

	ret = fut_writeMFutHdr (fd, fut, MFutType);     /* write the common matrix fut part */

	if (ret == 1) {
		ret = fut_writeMFutTbls (fd, fut, matrix, MFutType, oTblSize);  /* write out the matrix fut tables */
	}

	return ( ret );

} /* fut_writeMFut_Kp */


/*
 * fut_writeMFutHdr writes the common parts of a matrix fut from a fut
 * to an open file descriptor.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -2 to -4 on a table specific error
 */
int32
	fut_writeMFutHdr (      KpFd_p           fd,
						fut_ptr_t       fut,
						int32           MFutType)
{
u_int32	dummy = 0;
u_int8  inputChans, outputChans, LUTDimensions;
int32   inputChansT, outputChansT, LUTDimensionsT, ret;

	ret = fut_mfutInfo (fut, &LUTDimensionsT, &inputChansT, &outputChansT);
	if (ret != 1) {
		return (ret);
	}

	LUTDimensions = (u_int8)LUTDimensionsT; /* type conversion */
	inputChans = (u_int8)inputChansT;
	outputChans = (u_int8)outputChansT;
	
#if (FUT_MSBF == 0)                                             /* swap bytes if necessary */
	Kp_swab32 ((fut_generic_ptr_t)&MFutType, 1);   
#endif

	/* write out the common matrix fut stuff */
	ret = Kp_write (fd, (fut_generic_ptr_t)&MFutType, sizeof(int32)) &&
		 Kp_write (fd, (fut_generic_ptr_t)&dummy, sizeof(u_int32)) &&
		 Kp_write (fd, (fut_generic_ptr_t)&inputChans, sizeof(u_int8)) &&
		 Kp_write (fd, (fut_generic_ptr_t)&outputChans, sizeof(u_int8)) &&
		 Kp_write (fd, (fut_generic_ptr_t)&LUTDimensions, sizeof(u_int8)) &&
		 Kp_write (fd, (fut_generic_ptr_t)&dummy, sizeof(u_int8));

	return (ret ? 1 : -1);

}       /* fut_writeMFutHdr */


/* fut_writeMFutTbls writes the tables of a 'fut'
 * to an open file descriptor in the specified format.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -2 to -4 on a table specific error
 */
int32
	fut_writeMFutTbls(	KpFd_p           fd,
						fut_ptr_t       fut,
						Fixed_p         matrix,
						int32           MFutType,
						int32           oTblEntries)
{
int32   ret;
u_int16	itbl_ents, otbl_ents;
KpUInt32_t	entries;

	if ( ! IS_FUT(fut) )
		return (0);

	ret = fut_writeMFutMTbls (fd, matrix);  /* write the input tables */

	if (ret == 1) {
		switch (MFutType) {
		case MF1_TBL_ID:
			break;

		case MF2_TBL_ID:
			ret = fut_getItblFlag (fut, &entries);
			if (ret == 0) {
				return (-1);
			}

			/* write the # of input table entries */
			if (entries > MF1_TBL_ENT)
				itbl_ents = (KpInt16_t)entries;
			else
				itbl_ents = MF1_TBL_ENT;

#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
			Kp_swab16 ((fut_generic_ptr_t)&itbl_ents, 1);
#endif
			ret = Kp_write (fd, (fut_generic_ptr_t)&itbl_ents, sizeof(u_int16));
			if (ret != 1) {
				return (-1);
			}
			
			otbl_ents = (u_int16)oTblEntries;
		
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
			Kp_swab16 ((fut_generic_ptr_t)&otbl_ents, 1);
#endif
			ret = Kp_write (fd, (fut_generic_ptr_t)&otbl_ents, sizeof(u_int16));
			if (ret != 1) {
				return (-1);
			}
			break;

		default:
			return (-2);                                    /* unknown type */
		}
		
		ret = fut_writeMFutITbls (fd, fut, MFutType);   /* write the input tables */

		if (ret == 1) {
			ret = fut_writeMFutGTbls (fd, fut, MFutType);   /* write the grid tables */

			if (ret == 1) {
				ret = fut_writeMFutOTbls (fd, fut, MFutType, oTblEntries); /* write the output tables */
			}
		}
	}

	return (ret);
	
}       /* fut_writeMFutTbls */


/*
 * fut_writeMFutMTbls writes the matrix tables
 * to an open file descriptor of a matrix fut.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -2 to -4 on a table specific error
 */
int32
	fut_writeMFutMTbls(     KpFd_p   fd,
						Fixed_p matrix)
{
int32   i1, ret;
Fixed_t lMatrix[MF_MATRIX_DIM * MF_MATRIX_DIM];
Fixed_p	lMatrixP;

	if (matrix != NULL) {
		for (i1 = 0; i1 < MF_MATRIX_DIM * MF_MATRIX_DIM; i1++) {
			lMatrix[i1] = matrix[i1];               /* copy the matrix */
		}
	}
	else {  /* create an identity matrix when there is no matrix */
		lMatrixP = lMatrix;
		makeIdentityMatrix (lMatrixP, MF_MATRIX_DIM);
	}
	
#if (FUT_MSBF == 0)                                             /* swap bytes if necessary */
		Kp_swab32((fut_generic_ptr_t)lMatrix, MF_MATRIX_DIM * MF_MATRIX_DIM);
#endif

	/* write out the matrix */
	ret = Kp_write (fd, (fut_generic_ptr_t) lMatrix,
						sizeof(Fixed_t) * MF_MATRIX_DIM * MF_MATRIX_DIM);

	return (ret ? 1 : -1);

}       /* fut_writeMFutMTbls */


/* fut_writeMFutITbls writes the input tables of a 'fut'
 * to an open file descriptor in the specified format.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -2 to -4 on a table specific error
 */
int32
	fut_writeMFutITbls (    KpFd_p           fd,
							fut_ptr_t       fut,
							int32           MFutType)
{
#if (MF1_TBL_ENT != FUT_INPTBL_ENT)
*** table size mismatch!!!
#endif

KpUInt16_t *tmpTbl = NULL;
fut_itbldat_t *tmpTbl2 = NULL;
int32   i1, i2, ret, iTableInt, exShift, denominator, dataOver, iTblBytes;
u_int8_p        xf3dataP;
u_int16 xf4data;
u_int16_p       xf4dataP;
fut_itbldat_t iData;
fut_itbldat_ptr_t iDataP;
KpInt32_t	retErr;
KpUInt32_t	maxEntry, theFlag;

	ret = getITblFactors (fut, MFutType, &iTableInt, &denominator, &exShift, &dataOver);    
	if (ret != 1) {
		return (ret);
	}

	exShift -= 1;           /* -1 for rounding first */
	dataOver <<= 1;         /* ditto for overflow bit */
	
	switch (MFutType) {
		case MF1_TBL_ID:
			maxEntry = MF1_TBL_ENT;
			iTblBytes = sizeof(KpUInt8_t) * maxEntry;
			break;

		case MF2_TBL_ID:
			retErr = fut_getItblFlag (fut, &theFlag);
			if (retErr == 0) {
				return (0);
			}

			if (theFlag > MF1_TBL_ENT)
				maxEntry = theFlag;
			else
				maxEntry = MF1_TBL_ENT;

			iTblBytes = sizeof(KpUInt16_t) * maxEntry;
			break;

		default:
			return (-2);    /* unknown type */
	}

	/* allocate a temp buffer */
	tmpTbl = (KpUInt16_p)allocBufferPtr (iTblBytes);
	if (tmpTbl == NULL) {
		return (0);
	}

	/* allocate a second temp buffer if needed */
	if (maxEntry > MF1_TBL_ENT) {
		tmpTbl2 = (fut_itbldat_t *)allocBufferPtr ((maxEntry+1)*sizeof(fut_itbldat_t));
		if (tmpTbl2 == NULL) {
			ret = 0;
			goto GetOut;
		}
	}

	/* convert each input table to required precision */
	for (i1 = 0; (fut->itbl[i1] != NULL) && (i1 < FUT_NICHAN); i1++) {
		if (maxEntry > MF1_TBL_ENT) {
			retErr = convertMFutItbls (fut->itbl[i1]->tbl2, tmpTbl2, maxEntry);
			if (retErr == 0) {
				ret = 0;
				goto GetOut;
			}

			iDataP = tmpTbl2;
		}
		else {
			iDataP = fut->itbl[i1]->tbl;

		}
		
		xf3dataP = (KpUInt8_p) tmpTbl;
		xf4dataP = tmpTbl;

		for (i2 = 0; i2 < (KpInt32_t)maxEntry; i2++) {
			iData = *iDataP++;                                      /* get each entry */
			iData <<= iTableInt;                            /* scale by multiplying by numerator */
			iData /= denominator;                           /* and dividing by denominator */
			iData >>= exShift;                                      /* extract the desired bits */
			iData++;                                                        /* round */
			if (iData == dataOver) {
				iData = dataOver -1;                    /* overflow, clamp at max */
			}

			iData >>= 1;            /* shift off rounding bit */                    
			
			switch (MFutType) {
			case MF1_TBL_ID:
				*xf3dataP++ = (u_int8)iData;    /* convert to 8 bit size */
				break;

			case MF2_TBL_ID:
				xf4data = (u_int16)iData;               /* convert to 16 bit size */
#if (FUT_MSBF == 0)                                                             /* swap bytes if necessary */
				Kp_swab16 ((fut_generic_ptr_t)&xf4data, 1);
#endif
				*xf4dataP++ = xf4data;
			}
		}

		ret = Kp_write (fd, (fut_generic_ptr_t)tmpTbl, iTblBytes); /* write the input table */
		if (ret != 1) {
			return (-1);    /* error, abort */
		}

	}

GetOut:
	/* free the temp buffers */
	if (tmpTbl != NULL) {
		freeBufferPtr (tmpTbl);
	}

	if (tmpTbl2 != NULL) {
		freeBufferPtr (tmpTbl2);
	}

	return (ret);
	
}       /* fut_writeMFutITbls */


/* fut_writeMFutGTbls writes the grid tables of a 'fut'
 * to an open file descriptor in the specified format.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -2 to -4 on a table specific error
 */
int32
	fut_writeMFutGTbls (    KpFd_p           fd,
							fut_ptr_t       fut,
							int32           MFutType)
{
u_int8  tmpTbl[GBUFFER_SIZE];   /* local so we do not have memory allocation problems */
int32   i1, i2, ret = -1, outputChans;
int32   outCount, outBytes, gTblEntries, totalGSize, seed;
u_int8_p        xf3dataP;
u_int16_p       xf4dataP;
u_int16         gData;
fut_gtbldat_ptr_t gDataP[FUT_NOCHAN];
fut_chan_ptr_t chan;

	if ( ! IS_FUT(fut) )
		return (0);

/*      seed = (int32) fut;     */        /* use fut pointer as seed for random table generator */
	seed = (int32) 123456789;               /* use fut pointer as seed for random table generator */
	
	for (outputChans = 0; outputChans < FUT_NOCHAN; outputChans++) {
		if ((chan = fut->chan[outputChans]) == FUT_NULL_CHAN) {
			break;
		}
		gDataP[outputChans] = chan->gtbl->tbl;  /* get each grid table pointer */
	}

	gTblEntries = (fut->chan[0]->gtbl->tbl_size) / (int32)sizeof (fut_gtbldat_t);  /* get the # of entries in the table in the table */
	totalGSize = gTblEntries * outputChans;

	switch (MFutType) {
	case MF1_TBL_ID:
		totalGSize *= sizeof (u_int8);
		break;

	case MF2_TBL_ID:
		totalGSize *= sizeof (u_int16);
		break;

	default:
		return (-2);    /* unknown type */
	}


	outCount = 0;                                   /* count bytes written to buffer */
	outBytes = calcNextGBufSize (GBUFFER_SIZE, &totalGSize);	/* set up for first write */

	xf3dataP = (u_int8_p) tmpTbl;
	xf4dataP = (u_int16_p) tmpTbl;
	
	for (i1 = 0; i1 < gTblEntries; i1++) {
		for (i2 = 0; i2 < outputChans; i2++) {
			gData = *(gDataP[i2])++;                    /* get each grid table entry */
	
			switch (MFutType) {
			case MF1_TBL_ID:
				gData >>= (FUT_GRD_BITS - MF1_TBL_BITS -1);
				gData++;                                /* round */
				gData >>= 1;
				if (gData == MF1_TBL_MAXVAL +1) {
					gData = MF1_TBL_MAXVAL;              /* overflow, clamp at max */
				}
				*xf3dataP++ = (u_int8)gData;
				outCount += sizeof(u_int8);               /* count bytes in buffer */
				
				break;

			case MF2_TBL_ID:
				seed = gData >> 2;
				gData <<= (MF2_TBL_BITS - FUT_GRD_BITS);
				gData = (u_int16)(gData + (u_int16)(seed & (FUT_BIT(MF2_TBL_BITS - FUT_GRD_BITS) -1)));   /* noise in low bits */
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
				Kp_swab16 ((fut_generic_ptr_t)&gData, 1);
#endif
				*xf4dataP++ = gData;
				outCount += sizeof(u_int16);            /* count bytes in buffer */
			}

			if (outCount == outBytes) {
				outCount = 0;                                           /* reset bytes written */
				xf3dataP = (u_int8_p) tmpTbl;
				xf4dataP = (u_int16_p) tmpTbl;

				ret = Kp_write (fd, (fut_generic_ptr_t)tmpTbl, outBytes);
				if (ret != 1) {
					return (-1);
				}

				outBytes = calcNextGBufSize (outBytes, &totalGSize);	/* set up for next time */
			}
		}
	}

	return (ret);
	
}       /* fut_writeMFutGTbls */


static KpInt32_t
	calcNextGBufSize ( KpInt32_t curBufSize, KpInt32_p totalBytesRemaining)
{
KpInt32_t	bytesToWrite;

	*totalBytesRemaining -= curBufSize;		/* bytes to write after this buffer is written */
	if (*totalBytesRemaining <= 0) {
		bytesToWrite = *totalBytesRemaining + curBufSize;	/* last buffer to write, adjust size */
	}
	else {
		bytesToWrite = curBufSize;			/* not last buffer to write */
	}

	return bytesToWrite;
}

/* fut_writeMFutOTbls writes the output tables of a 'fut'
 * to an open file descriptor in the specified format.
 *
 * Returns: 
 * 1 on success
 * -1 on Kp_write error
 * -2 to -4 on a table specific error
 */
int32
	fut_writeMFutOTbls (    KpFd_p           fd,
							fut_ptr_t       fut,
							int32           MFutType,
							int32           oTblEntries)
{
u_int16  tmpTbl[OBUFFER_SIZE];   /* local so we do not have memory allocation problems */
int32   		i1, ret, outputChans, oTblBytes, index, oTableIndex;
int32   		tmpData, tmpData1, tmpData2, oTableInterp, seed;
u_int8_p        xf3dataP;
u_int16         xf4data;
u_int16_p       xf4dataP;
fut_otbldat_ptr_t oDataP;
fut_chan_ptr_t chan;

	seed = (int32) 987654321;       /* use fut pointer as seed for random table generator */

	if (! IS_FUT(fut)) {
		return (0);
	}

	switch (MFutType) {
	case MF1_TBL_ID:
		oTblEntries = MF1_TBL_ENT;           /* has constant size */
		oTblBytes = oTblEntries * (int32)sizeof (u_int8);
		break;

	case MF2_TBL_ID:
		if ((oTblEntries < MF2_MIN_TBL_ENT) || (oTblEntries > MF2_MAX_TBL_ENT)) {
			return (0);                                     /* illegal size */
		}
		
		oTblBytes = oTblEntries * (int32)sizeof (u_int16);
		break;

	default:
		return (-2);                                    /* unknown type */
	}

	for (outputChans = 0; outputChans < FUT_NOCHAN; outputChans++) {
		if ((chan = fut->chan[outputChans]) == FUT_NULL_CHAN) {
			return (1);             /* all done */
		}
		
		xf3dataP = (u_int8_p) tmpTbl;
		xf4dataP = (u_int16_p) tmpTbl;
				
		if ((chan->otbl == NULL) || (chan->otbl->tbl == NULL)) {        /* write a linear output table */
			for (i1 = 0; i1 < oTblEntries; i1++) {
				switch (MFutType) {
				case MF1_TBL_ID:
					*xf3dataP++ = (u_int8) i1;
				
					break;

				case MF2_TBL_ID:
					tmpData = i1 * FUT_MAX_PEL12;
					tmpData /= oTblEntries;
					if (tmpData > FUT_MAX_PEL12) {
						tmpData = FUT_MAX_PEL12;
					}
					seed = tmpData >> 2;
					tmpData <<= (MF2_TBL_BITS - FUT_GRD_BITS);
					tmpData += (u_int16)(seed & (FUT_BIT(MF2_TBL_BITS - FUT_GRD_BITS) -1));      /* noise in low bits */
					xf4data = (u_int16)tmpData;
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
					Kp_swab16 ((fut_generic_ptr_t)&xf4data, 1);
#endif
					*xf4dataP++ = xf4data;
				}
			}
		}
		else {
			oDataP = chan->otbl->tbl;               /* get the output table */
				
			for (i1 = 0, index = 0; i1 < oTblEntries; i1++, index += FUT_OUTTBL_ENT-1) {
				oTableIndex = index / (oTblEntries-1);
			

				tmpData = oDataP[oTableIndex];
				if (oTableIndex < (FUT_OUTTBL_ENT-1)) {
					tmpData1 = oDataP[oTableIndex+1];
				} else {
					tmpData1 = oDataP[oTableIndex];
				}
				oTableInterp = index - (oTableIndex * (oTblEntries-1));      /* interpolant */
			
				tmpData2 = (((tmpData1 - tmpData) * oTableInterp) / (FUT_OUTTBL_ENT-1));
				tmpData += tmpData2;
			
				switch (MFutType) {
				case MF1_TBL_ID:
					tmpData += FUT_BIT(FUT_GRD_BITS - MF1_TBL_BITS -1);     /* round */
					if ((tmpData & FUT_BIT(FUT_GRD_BITS)) != 0) {
						tmpData = FUT_BIT(FUT_GRD_BITS) -1;                     /* clamp at max */
					}
					tmpData >>= FUT_GRD_BITS - MF1_TBL_BITS;
					
					*xf3dataP++ = (u_int8)tmpData;
				
				break;

				case MF2_TBL_ID:
					if (tmpData > FUT_GRD_MAXVAL) {
						tmpData = FUT_GRD_MAXVAL;
					}
					seed = tmpData >> 2;
					tmpData <<= (MF2_TBL_BITS - FUT_GRD_BITS);
					tmpData += (u_int16)(seed & (FUT_BIT(MF2_TBL_BITS - FUT_GRD_BITS) -1));      /* noise in low bits */
					xf4data = (u_int16)tmpData;
#if (FUT_MSBF == 0)                             /* swap bytes if necessary */
					Kp_swab16 ((fut_generic_ptr_t)&xf4data, 1);
#endif
					*xf4dataP++ = xf4data;
				}
			}
		}

		ret = Kp_write (fd, (fut_generic_ptr_t)tmpTbl, oTblBytes);
		if (ret != 1) {
			return (-1);
		}
	}

	return (1);             /* success */
	
}       /* fut_writeMFutOTbls */

/* get all of the factors needed to do matrix fut input table I/O
	the input table scaling factor is
	iTableInt = # bits needed to represent (grid size - 1)
	numerator = 2^iTableInt
	denominator = grid size - 1
	the extraction shift for 2^iTableInt table is iTableInt + FUT_INP_FRACBITS - tblBits
 */
	

static int32
	getITblFactors (        fut_ptr_t       fut,
						int32           MFutType,
						int32_p         iTableInt,
						int32_p         denominator,
						int32_p         exShift,
						int32_p         dataOver)
{
int32   gridSize, tblBits, intBits;

	if ( ! IS_FUT(fut) )
		return (0);

	gridSize = fut->itbl[0]->size -1;
	*denominator = gridSize;

	for (intBits = 0; intBits < FUT_NICHAN; intBits++) {
		if (gridSize == 0) {
			break;
		}
		
		gridSize >>= 1; /* keep checking */
	}

	*iTableInt = intBits;

	switch (MFutType) {
	case MF1_TBL_ID:
		tblBits = MF1_TBL_BITS;

		break;

	case MF2_TBL_ID:
		tblBits = MF2_TBL_BITS;

		break;

	default:
		return (-2);    /* unknown type */
	}

	*exShift = *iTableInt + FUT_INP_FRACBITS - tblBits;
	*dataOver = FUT_BIT(tblBits);
	
	return (1);
}


/*
 * fut_mfutInfo returns matrix fut information of a fut.
 *
 * Returns: 
 *  1   on success
 *  0   invalid fut
 * -1   grid dimensions too large
 * -2   grid dimensions are not the same
 * -3   input channels not contiguous and starting at 0 or too many inputs
 * -4   output channels not contiguous and starting at 0 or too many outputs
 */
int32
	fut_mfutInfo (  fut_ptr_t       fut,                            /* get info of this fut */
					int32_p         LUTDimensionsP,         /* # points in each grid dimension */
					int32_p         inputChansP,            /* # of input channels */
					int32_p         outputChansP)           /* # of output channels */
{
int32   LUTDimensions, inputChans, outputChans;
int32   iomask, imask, omask;
int32   ret = 1;                                /* assume success */

	if ( ! IS_FUT(fut) ) {
		return (0);
	}

/* get the # of input channels, # of output channels, and the grid dimensions */
	iomask = fut_iomask_to_int(fut->iomask);        /* get the fut's input/output mask */

	/* input tables must be common and in first n contiguous input channels */
	imask = FUT_IMASK(iomask);                                      /* get the fut's input mask */
	LUTDimensions = fut->itbl[0]->size;                     /* initialize the size */

	if (LUTDimensions > MF_GRD_MAXDIM) {
		ret = -1;                               /* this fut can not be made into a matrix fut */
	}
	
	for (inputChans = 0; inputChans < FUT_NICHAN; inputChans++, imask >>= 1) {
		if ((fut->itbl[inputChans] == FUT_NULL_ITBL) || ((imask & 1) == 0)) {
			break;
		}
		if (LUTDimensions != fut->itbl[inputChans]->size) {     /* sizes must be the same */
			if (ret == 1) {
				ret = -2;                       /* this fut can not be made into a matrix fut */
			}
		}
	}

	if ((imask != 0) || (inputChans > FUT_NICHAN)) {
		if (ret == 1) {
			ret = -3;                               /* this fut can not be made into a matrix fut */
		}
	}
	
	/* output tables must be in first n contiguous output channels */
	omask = FUT_OMASK(iomask);              /* get the fut's output mask */
	outputChans = 0;

	for (outputChans = 0; outputChans < FUT_NOCHAN; outputChans++, omask >>= 1) {
		if ((fut->chan[outputChans] == FUT_NULL_CHAN) || ((omask & 1) == 0)) {
			break;
		}
	}

	if ((omask != 0) || (outputChans > FUT_NOCHAN)) {
		if (ret == 1) {
			ret = -4;                               /* this fut can not be made into a matrix fut */
		}
	}

	*LUTDimensionsP = LUTDimensions;                /* return info */
	*inputChansP = inputChans;
	*outputChansP = outputChans;

	return (ret);
	
}       /* fut_mfutInfo */



/* make an identity matrix */
void
	makeIdentityMatrix (	Fixed_p matrix,
							int32   matrixSize)
{
int32   i1, i2, i3;

	for (i1 = 0, i3 = 0; i1 < matrixSize; i1++) {
		for (i2 = 0; i2 < matrixSize; i2++, i3++) {
			if (i1 == i2) {
				matrix[i3] = FUT_MATRIX_ONE;
			}
			else {
				matrix[i3] = FUT_MATRIX_ZERO;
			}
		}
	}
}


/* return 1 if the matrix is an identity matrix, 0 if not */
int32
	isIdentityMatrix (	Fixed_p matrix,
						int32   matrixSize)
{
int32   i1, i2, i3;

	for (i1 = 0, i3 = 0; i1 < matrixSize; i1++) {
		for (i2 = 0; i2 < matrixSize; i2++, i3++) {
			if (i1 == i2) {
				if (matrix[i3] != FUT_MATRIX_ONE) {
					return (0);
				}
			}
			else {
				if (matrix[i3] != FUT_MATRIX_ZERO) {
					return (0);
				}
			}
		}
	}

	return (1);
}

KpInt32_t fut_getItblFlag (fut_ptr_t fut,
						   KpUInt32_p theFlag)
{
	KpUInt32_t imask, itblFlags = 0;
	KpUInt32_t firstFlag = 0, found = 0, inputChans = 0;
	kcpindex_t	i1;

	if ( ! IS_FUT(fut) || (theFlag == NULL)) {
		return (0);
	}

	/* test the flag state of each itbl */
	imask = FUT_IMASK(fut->iomask.in);
	for (i1 = 0; i1 < FUT_NICHAN; i1++) {
		if ( (imask & FUT_BIT(i1)) == 0 )
			continue;
		if (found == 0) {
			/* save the first itbl flag */
			firstFlag = fut->itbl[i1]->tblFlag;
			found++;
		}

		itblFlags += fut->itbl[i1]->tblFlag;
		inputChans++;
	}

	/* test to see if the flags in each ichan match */
	if (inputChans == 0) {
		return (0);
	}

	*theFlag = itblFlags / inputChans;

	if (*theFlag != firstFlag) {
		return (0);
	}

	return (1);
}

static
KpInt32_t convertMFutItbls (fut_itbldat_ptr_t srcPtr,
					   fut_itbldat_ptr_t desPtr,
					   KpUInt32_t entries)
{
	KpInt32_t   i1, index, iTableInterp, iTableIndex;
	KpInt32_t	tmpData, tmpData1, tmpData2;
	fut_itbldat_t	iData;
	fut_itbldat_ptr_t iDataP, iDataP2;

	if ((srcPtr == NULL) || (desPtr == NULL)) {
		return (0);
	}

	iData = 0;			/* this is to fix pc warning */
	iDataP = desPtr;    /* get the address of the table data */
	iDataP2 = srcPtr;

	for (i1 = 0, index = 0; i1 < (KpInt32_t)entries; i1++, index += MF2_MAX_TBL_ENT-1) {

		iTableIndex = index / (entries-1);
		tmpData = iDataP2[iTableIndex];
		tmpData1 = iDataP2[iTableIndex+1];

		iTableInterp = index - (iTableIndex * (entries-1));      /* interpolant */
			
		tmpData2 = (((tmpData1 - tmpData) * iTableInterp) / (entries-1));
		iData = tmpData + tmpData2;

		*iDataP++ = iData;              /* store each entry */
	}
	*iDataP++ = iData;             		/* repeat last entry (entries+1) */

	return (1);
}

