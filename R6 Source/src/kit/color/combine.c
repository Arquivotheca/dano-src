/*
 * @(#)combine.c	2.70 98/01/02

	Contains:	execute the KCMS compose function, which is called combine

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1996 by Eastman Kodak Company, all rights reserved.

*/


#include <string.h>

#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "attrib.h"
#include "attrcipg.h"
#include "kcptmgr.h"
#include "fut.h"		/* check get_lab2xyz prototype */
#include "kcpfut.h"		/* check get_lab2xyz prototype */
#include "kcpfchn.h"
#include "makefuts.h"

/* prototypes */
static PTErr_t checkFuT (PTRefNum_t PTRefNum);
static int32 kcpGetColorSpace (PTRefNum_t PTRefNum, int32 AttrId);
static fut_ptr_t do_compose (fut_t FAR*, int32, fut_t FAR*, int32, int32);
static fut_ptr_t do_pre_compose (PTRefNum_t, fut_ptr_t,  PTRefNum_t, fut_ptr_t, int32, int32, int32);
static fut_ptr_t do_post_compose (fut_ptr_t fut1, int32 iomask);


/*----------------------------------------------------------------------
 *  get_fut
 *----------------------------------------------------------------------
 */

static fut_ptr_t
	get_fut (	char_p	fname,
				fut_ptr_t	(*makeAFuT) (int size), int size)
{
KpUInt32_t		retVal;
char			Fullname[256];
fut_ptr_t		futp;
threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (NULL);
	}

/* make fully qualified file name */
	strcpy (Fullname, threadGlobalsP->processGlobalsP->iGP->KCPDataDir);
	strcat (Fullname, fname);
/*	strcat (Fullname, ".fut"); */

										/* try to load it from the file */
	futp = fut_load_fp (Fullname, threadGlobalsP->processGlobalsP->KCPDataDirProps);
	if (futp == NULL) {

										/* The creation of the aux pt file
										   can only be performed by one 
										   process at a time.  To enforce
										   this initialize all opens for
										   write locks the file 			*/

		
		/* create the file data and write it to disk */

		futp = makeAFuT (size);	
		if (futp != NULL) {
			/* If write fails, use the fut.  It may be slower
			   since it is recreated each time but it works */
			retVal = fut_store_fp (futp, Fullname, threadGlobalsP->processGlobalsP->KCPDataDirProps);
		}


	}

	KCMDunloadGlobals();					/* Unlock this apps Globals */
	return futp;
}

/*----------------------------------------------------------------------
 *  get_linlab
 *----------------------------------------------------------------------
 */

fut_ptr_t
	get_linlab (int size)
{
threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
KpInt32_t	PTCubeSize;					/* size of ICC profiles (pts) */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (NULL);
	}
	PTCubeSize = threadGlobalsP->processGlobalsP->iGP->PTCubeSize;

	KCMDunloadGlobals();					/* Unlock this apps Globals */

	if (size == PTCubeSize) {
		return get_fut ("CP23", get_linlab_fut, size);

	} else if (size == SIXTEEN_CUBE) {
		return get_fut ("CP24", get_linlab_fut, size);

	} else {
		return (get_linlab_fut(size));
	}
}

/*----------------------------------------------------------------------
 *  get_lin3d
 *----------------------------------------------------------------------
 */

fut_ptr_t
	get_lin3d (int size)
{
threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
KpInt32_t	PTCubeSize;					/* size of ICC profiles (pts) */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (NULL);
	}
	PTCubeSize = threadGlobalsP->processGlobalsP->iGP->PTCubeSize;

	KCMDunloadGlobals();					/* Unlock this apps Globals */

	if (size == PTCubeSize) {
		return get_fut ("CP12", get_lin3d_fut, size);

	} else if (size == SIXTEEN_CUBE) {
		return get_fut ("CP16", get_lin3d_fut, size);

	} else {
		return (get_lin3d_fut(size));
	}
}

/*----------------------------------------------------------------------
 *  get_lin4d
 *----------------------------------------------------------------------
 */

fut_ptr_t
	get_lin4d (int size)
{
threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
KpInt32_t	PTCubeSize;					/* size of ICC profiles (pts) */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (NULL);
	}
	PTCubeSize = threadGlobalsP->processGlobalsP->iGP->PTCubeSize;

	KCMDunloadGlobals();					/* Unlock this apps Globals */

	if (size == PTCubeSize) {
		return get_fut ("CP13", get_lin4d_fut, size);

	} else if (size == SIXTEEN_CUBE) {
		return get_fut ("CP17", get_lin4d_fut, size);

	} else {
		return (get_lin4d_fut(size));
	}
}

/*----------------------------------------------------------------------
 *  get_lab2xyz
 *----------------------------------------------------------------------
 */

fut_ptr_t
	get_lab2xyz (int size)
{
threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
KpInt32_t	PTCubeSize;					/* size of ICC profiles (pts) */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (NULL);
	}
	PTCubeSize = threadGlobalsP->processGlobalsP->iGP->PTCubeSize;

	KCMDunloadGlobals();					/* Unlock this apps Globals */

	if (size == PTCubeSize) {
		return get_fut ("CP14", get_lab2xyz_fut, size);

	} else if (size == SIXTEEN_CUBE) {
		return get_fut ("CP18", get_lab2xyz_fut, size);

	} else {
		return (NULL);
	}
}

/*----------------------------------------------------------------------
 *  get_xyz2lab
 *----------------------------------------------------------------------
 */

fut_ptr_t
	get_xyz2lab (int size)
{
threadGlobals_p	threadGlobalsP;			/* a pointer to the process global */
KpInt32_t	PTCubeSize;					/* size of ICC profiles (pts) */

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (NULL);
	}
	PTCubeSize = threadGlobalsP->processGlobalsP->iGP->PTCubeSize;

	KCMDunloadGlobals();					/* Unlock this apps Globals */

	if (size == PTCubeSize) {
		return get_fut ("CP20", get_xyz2lab_fut, size);

	} else if (size == SIXTEEN_CUBE) {
		return get_fut ("CP21", get_xyz2lab_fut, size);

	} else {
		return (NULL);
	}
}

/*----------------------------------------------------------------------
 *  kcpGetColorSpace
 *----------------------------------------------------------------------
 */

/* get color space */
static int32
	kcpGetColorSpace (	PTRefNum_t	PTRefNum,
					int32		AttrId)
{
	PTErr_t	errnum;
	int32	attrSize;
	char	attribute [KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	char	cie_attrib [KCM_MAX_ATTRIB_VALUE_LENGTH+1];

	attrSize = sizeof (attribute) - 1;
	errnum = PTGetAttribute (PTRefNum, AttrId, &attrSize, attribute);
	if (errnum != KCP_SUCCESS)
		return KCM_UNKNOWN;

	KpItoa(KCM_CIE_LAB, cie_attrib);
	if (0 == strcmp (attribute, cie_attrib))
		return KCM_CIE_LAB;

	KpItoa(KCM_CIE_XYZ, cie_attrib);
	if (0 == strcmp (attribute, cie_attrib))
		return KCM_CIE_XYZ;

	return KCM_UNKNOWN;
}


/* do compose, taking into account possibly differant colorspaces */
static fut_ptr_t
	do_compose (fut_ptr_t		fut2,
					int32		in_of_fut2,
					fut_ptr_t	fut1,
					int32		out_of_fut1,
					int32		iomask)
{
fut_ptr_t	fut1new = FUT_NULL;
fut_ptr_t	futmatch = FUT_NULL;
fut_ptr_t	futR = FUT_NULL;

/* create color space matching fut, if needed */
	if ((out_of_fut1 == KCM_CIE_XYZ) && (in_of_fut2 == KCM_CIE_LAB)) {
		futmatch = get_xyz2lab ((int) SIXTEEN_CUBE);
	} else if ((out_of_fut1 == KCM_CIE_LAB) && (in_of_fut2 == KCM_CIE_XYZ)) {
		futmatch = get_lab2xyz ((int) SIXTEEN_CUBE);
	} else {
		return fut_comp (fut2, fut1, iomask);
	}

	if (futmatch != FUT_NULL) {
/* change fut1's color space to match fut2's color space */
		fut1new = fut_comp (futmatch, fut1, iomask);
		fut_free (futmatch);

		if (fut1new != FUT_NULL) {
/* now combine the two futs */
			futR = fut_comp (fut2, fut1new, iomask);
			fut_free (fut1new);
		}
	}
/* return the result */
	return futR;
}

/**************************************************/
/* pre compose to set the grid size appropriately */
/**************************************************/
static fut_ptr_t
	do_pre_compose (PTRefNum_t	PTRefNum2,
						 fut_ptr_t	fut2,
						 PTRefNum_t	PTRefNum1,
						 fut_ptr_t	fut1,
						 int32		iomask,
						 int32		newLUTDims,
						 int32		mode)
{
int32		OutSpace, InSpace1, InSpace2;
int32		LUTDims1, NumInVars1, NumOutVars1, status;
int32		LUTDims2, NumInVars2, NumOutVars2;
fut_ptr_t	fut1new;
fut_ptr_t	fut2new;
fut_ptr_t	futR;

	InSpace1 = kcpGetColorSpace (PTRefNum1, KCM_SPACE_IN);
	OutSpace = kcpGetColorSpace (PTRefNum1, KCM_SPACE_OUT);
	InSpace2 = kcpGetColorSpace (PTRefNum2, KCM_SPACE_IN);

	status = fut_mfutInfo (fut1, &LUTDims1, &NumInVars1, &NumOutVars1);
	if ((mode & PT_COMBINE_LARGEST) != 0) {
		/* pre compose with the largest grid dimension */
		fut_mfutInfo (fut2, &LUTDims2, &NumInVars2, &NumOutVars2);
		if (NumInVars1 == FOUR_CLR) {
			/* if cmyk input, limit the results to the size of the first or... */
			if (LUTDims2 > SIXTEEN_CUBE) {	/* SIXTEEN_CUBE which ever is greater */
				LUTDims2 = SIXTEEN_CUBE;
			}
		}
		if (newLUTDims < LUTDims1) {
			newLUTDims = LUTDims1;
		}
		if (newLUTDims < LUTDims2) {
			newLUTDims = LUTDims2;
		}
	}
	if ( ((status != 1) || (LUTDims1 != newLUTDims)) && (NumInVars1 <= FOUR_CLR)) {
		switch (NumInVars1) {
			case THREE_CLR:
				if (InSpace1 == KCM_CIE_LAB) {
					fut2new = get_linlab ((int) newLUTDims);
				} else {
					fut2new = get_lin3d ((int) newLUTDims);
				}
				break;

			case FOUR_CLR:
				fut2new = get_lin4d ((int) newLUTDims);
				break;

			default:
				fut2new = NULL;
				break;
		} /* switch */

		if (NULL == fut2new)
			return NULL;

		fut1new = fut_comp (fut1, fut2new, iomask);
		fut_free (fut2new);
		if (NULL == fut1new) {
			return (NULL);
		}

										/* now that the fut is the right size, do the
											actual composition									*/

		futR = do_compose (fut2, InSpace2, fut1new, OutSpace, iomask);
		fut_free (fut1new);
	} /* if status */
	
	else {
		futR = do_compose (fut2, InSpace2, fut1, OutSpace, iomask);
	}

	return futR;

} /* do_pre_compose */


PTErr_t
	PTCombine (	int32		mode,
				PTRefNum_t	PTRefNum1,
				PTRefNum_t	PTRefNum2,
				PTRefNum_p	PTRefNumR)
{
	threadGlobals_p	threadGlobalsP;				/* a pointer to the process global */
	KcmGenericPtr	PTHdr;
	KcmHandle	PTHdr1, PTHdr2, PTHdrR, PTAttr, PTData1, PTData2, PTDataR;
	fut_ptr_t	fut1, fut2, futR;
	PTErr_t		errnum = KCP_FAILURE, errnum1 = KCP_FAILURE, errnum2 = KCP_FAILURE;
	int32		iomask, compType;
	int32		OutSpace, InSpace;
	PTRefNum_t	labxyzPTRefNum, PTRefNumSerialR;
	int32		attrSize;
	char		attribute1 [KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	char		attribute2 [KCM_MAX_ATTRIB_VALUE_LENGTH+1];

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}

/* Check for valid PTRefNumP */
	if (Kp_IsBadWritePtr(PTRefNumR, sizeof(*PTRefNumR))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut3;
	}

	*PTRefNumR = 0;
	PTHdr = NULL;
	PTDataR = PTHdrR = NULL;
	fut1 = fut2 = futR = FUT_NULL;
	compType = mode & PT_COMBINE_TYPE;	/* and composition type */
	
/* must currently be active */
	errnum1 = PTGetPTInfo (PTRefNum1, (PTAddr_t**)&PTHdr1,
							(PTAddr_t**)&PTAttr, (PTAddr_t**)&PTData1);
	if (errnum1 == KCP_PT_ACTIVE) {
		errnum = checkFuT(PTRefNum1);
		if (errnum != KCP_SUCCESS)
			goto ErrOut3;
	}
	if (errnum1 != KCP_PT_INACTIVE) {
		/* must also currently be active */
		errnum2 = PTGetPTInfo (PTRefNum2, (PTAddr_t**)&PTHdr2, (PTAddr_t**)&PTAttr, (PTAddr_t**)&PTData2);
		if (errnum2 == KCP_PT_ACTIVE) {
			errnum = checkFuT(PTRefNum2);
			if (errnum != KCP_SUCCESS) {
				goto ErrOut3;
			}
		}
		
		if ((errnum1 != KCP_PT_INACTIVE) && (errnum2 != KCP_PT_INACTIVE)) {
			
			if ((errnum1 == KCP_SERIAL_PT) || (errnum2 == KCP_SERIAL_PT)) {		/* must be for serial evaluation */
				attrSize = sizeof (attribute1) - 1;
				errnum =  PTGetAttribute (PTRefNum1, KCM_CLASS, &attrSize, attribute1);
				if (errnum != KCP_SUCCESS) {
					attribute1[0] = 0;
				}
				
				attrSize = sizeof (attribute2) - 1;
				errnum =  PTGetAttribute (PTRefNum2, KCM_CLASS, &attrSize, attribute2);
				if (errnum != KCP_SUCCESS) {
					attribute2[0] = 0;
				}
				
				/* Don't add chainning rules to serial pt list! */
				if ((strcmp(attribute1, KCM_AUX_RULE_CLASS_S) != 0) && (strcmp(attribute2, KCM_AUX_RULE_CLASS_S) != 0)) {

					/* create color space matching fut, if needed */
					OutSpace = kcpGetColorSpace (PTRefNum1, KCM_SPACE_OUT);
					InSpace = kcpGetColorSpace (PTRefNum2, KCM_SPACE_IN);

					if ((OutSpace == KCM_CIE_XYZ) && (InSpace == KCM_CIE_LAB)) {
						futR = get_xyz2lab (SIXTEEN_CUBE);
					}
					else {
						if ((OutSpace == KCM_CIE_LAB) && (InSpace == KCM_CIE_XYZ)) {
							futR = get_lab2xyz (SIXTEEN_CUBE);
						}
					}

					if (futR == NULL) {
						errnum = addSerialPT (threadGlobalsP, PTRefNum1, PTRefNum2, PTRefNumR);
					}
					else {
						PTDataR = (KcmHandle)fut_unlock_fut(futR);	/* unlock the fut */
						errnum = registerPT(threadGlobalsP, NULL, NULL, &labxyzPTRefNum);
						if (errnum != KCP_SUCCESS) {
							goto ErrOut3;
						}
						
						makeActive (threadGlobalsP, labxyzPTRefNum, PTDataR);
						makeSerial (threadGlobalsP, labxyzPTRefNum);
						
						errnum = addSerialPT (threadGlobalsP, PTRefNum1, labxyzPTRefNum, &PTRefNumSerialR);
						if (errnum != KCP_SUCCESS) {
							goto ErrOut3;
						}
						
						errnum = PTCheckOut (labxyzPTRefNum);		/* leave just enough for serial evaluation */
						if (errnum != KCP_SUCCESS) {
							goto ErrOut3;
						}
										
						errnum = addSerialPT (threadGlobalsP, PTRefNumSerialR, PTRefNum2, PTRefNumR);

						if (errnum == KCP_SUCCESS) {					
							errnum = PTCheckOut (PTRefNumSerialR);
						}
					}
				}
				else {
					errnum = registerPT(threadGlobalsP, NULL, NULL, &PTRefNumSerialR);
					if (errnum != KCP_SUCCESS) {
						goto ErrOut3;
					}
					
					makeSerial (threadGlobalsP, PTRefNumSerialR);
					
					/* Don't add chaining rules to serial pt list! */
					if (strcmp(attribute1, KCM_AUX_RULE_CLASS_S) != 0) {
						errnum = addSerialPT (threadGlobalsP, PTRefNumSerialR, PTRefNum1, PTRefNumR);
					}
					else {
						errnum = addSerialPT (threadGlobalsP, PTRefNumSerialR, PTRefNum2, PTRefNumR);
					}
				
					if (errnum == KCP_SUCCESS) {					
						errnum = PTCheckOut (PTRefNumSerialR);
					}
				}

				if (errnum != KCP_SUCCESS) {
					goto ErrOut3;
				}
			}
			
			if ((PTData1 != NULL) && (PTData2 != NULL)) {	/* compose PT data */
				/* define iomask */
				iomask = fut_iomask ("([xyzt])");
				if ((mode & PT_COMBINE_CUBIC) != 0) {
					iomask |= FUT_ORDER (FUT_CUBIC);		/* use cubic interpolation for composition evaluations */
				}

				/* get buffer for resultant info header */
				PTHdr = allocBufferPtr (sizeof(fut_hdr_t));

				if (PTHdr == NULL) {
					errnum = KCP_NO_CHECKIN_MEM;
					diagWindow("PTCombine: allocBufferPtr failed", errnum);
					goto ErrOut3;
				}

				fut1 = fut_lock_fut((KpHandle_t)PTData1);	/* get the fut addresses */
				if (fut1 == FUT_NULL) {
					errnum = KCP_FAILURE;
					diagWindow("PTCombine: fut_lock_fut 1 failed", errnum);
					goto ErrOut;
				}
				fut2 = fut_lock_fut((KpHandle_t)PTData2);
				if (fut2 == FUT_NULL) {
					errnum = KCP_FAILURE;
					diagWindow("PTCombine: fut_lock_fut 2 failed", errnum);
					goto ErrOut1;
				}

				switch (compType) {
				case PT_COMBINE_STD:
					futR = fut_comp(fut2, fut1, iomask);
					if (futR == FUT_NULL) {
						errnum = KCP_FAILURE;
						goto ErrOut2;
					}
					break;

#if !defined (KP_NO_IOTBL_COMBINE)
				case PT_COMBINE_ITBL:
					futR = fut_comp_itbl(fut2, fut1, iomask);
					if (futR == FUT_NULL) {
						errnum = KCP_FAILURE;
						goto ErrOut2;
					}
					break;
			
				case PT_COMBINE_OTBL:
					futR = fut_comp_otbl(fut2, fut1, iomask);	
					if (futR == FUT_NULL) {
						errnum = KCP_FAILURE;
						goto ErrOut2;
					}
					break;
#endif
						
				case PT_COMBINE_PF_8:
do_combine_pf8:
					futR = do_pre_compose (PTRefNum2, fut2, PTRefNum1, fut1, iomask,
									threadGlobalsP->processGlobalsP->iGP->PTCubeSize, mode);
					if (futR == FUT_NULL) {
						errnum = KCP_FAILURE;
						goto ErrOut2;
					}
					break;

				case PT_COMBINE_PF_16:
					if (threadGlobalsP->processGlobalsP->iGP->PTCubeSize != EIGHT_CUBE) {
						goto do_combine_pf8;
					}
					futR = do_pre_compose (PTRefNum2, fut2,
									PTRefNum1, fut1, iomask, SIXTEEN_CUBE, mode);
					if (futR == FUT_NULL) {
						errnum = KCP_FAILURE;
						goto ErrOut2;
					}
					break;

				case PT_COMBINE_PF:
					if (threadGlobalsP->processGlobalsP->iGP->PTCubeSize != EIGHT_CUBE) {
						goto do_combine_pf8;
					}
					if ((mode & PT_COMBINE_LARGEST) != 0) {
						futR = do_pre_compose (PTRefNum2, fut2, PTRefNum1,
												fut1, iomask, TWO_CUBE, mode);
					} else {
						OutSpace = kcpGetColorSpace (PTRefNum1, KCM_SPACE_OUT);
						InSpace = kcpGetColorSpace (PTRefNum2, KCM_SPACE_IN);
						futR = do_compose (fut2, InSpace, fut1, OutSpace, iomask);
					}
					if (futR == FUT_NULL) {
						errnum = KCP_FAILURE;
						goto ErrOut2;
					}
					break;

				default:
					errnum = KCP_BAD_COMP_MODE;
					diagWindow("PTCombine: unknown actual composition type", errnum);
					goto ErrOut2;
				}

				if (!fut_io_encode (futR, PTHdr)) {	/* make the info header */
					errnum = KCP_FAILURE;
					diagWindow("PTCombine: fut_io_encode failed", errnum);
					goto ErrOut2;
				}
					
				PTDataR = (KcmHandle)fut_unlock_fut(fut2);	/* unlock the futs */
				if (PTDataR == NULL) {
					errnum = KCP_FAILURE;
					diagWindow("PTCombine: fut_unlock_fut fut2 failed", errnum);
					fut_unlock_fut(futR);
					goto ErrOut1;
				}
				PTDataR = (KcmHandle)fut_unlock_fut(fut1);
				if (PTDataR == NULL) {
					errnum = KCP_FAILURE;
					diagWindow("PTCombine: fut_unlock_fut fut1 failed", errnum);
					fut_unlock_fut(futR);
					goto ErrOut;
				}
				if ( (futR != FUT_NULL) && (futR->idstr != NULL) ) {
					fut_free_idstr (futR->idstr);
					futR->idstr = NULL;
				}
				PTDataR = (KcmHandle)fut_unlock_fut(futR);
				if (PTDataR == NULL) {
					errnum = KCP_FAILURE;
					diagWindow("PTCombine: fut_unlock_fut futR failed", errnum);
					goto ErrOut;
				}

				PTHdrR = unlockBufferPtr(PTHdr);	/* unlock the header buffer */
				if (PTHdrR == NULL) {
					errnum = KCP_FAILURE;
					diagWindow("PTCombine: unlockBufferPtr failed", errnum);
					goto ErrOut3;
				}
				PTHdr = NULL;						/* indicate that this is unlocked */

			/* checkin and activate */
				if (*PTRefNumR == 0) {
					errnum = PTNewPT(PTHdrR, PTDataR, PTRefNumR);
					if (errnum != KCP_SUCCESS) {
						diagWindow("PTCombine: PTNewPT failed", errnum);
						goto ErrOut;
					}
				} else {
					makeActive (threadGlobalsP, *PTRefNumR, PTDataR);
					makeSerial (threadGlobalsP, *PTRefNumR);
					errnum = TpGenerateAttr(threadGlobalsP, *PTRefNumR); /* generate constant attributes */
					if (errnum != KCP_SUCCESS) {
							(void) PTCheckOut(*PTRefNumR);	/* clean up if error */
					}
					errnum = setPTHdr (threadGlobalsP, *PTRefNumR, PTHdrR);
					if (errnum != KCP_SUCCESS) {		
						goto ErrOut;
					}
				}
			}	/* end of NOT serial evaluation */
			
			/* propagate attributes */
			errnum = ComposeAttr(threadGlobalsP, PTRefNum1, PTRefNum2, compType, *PTRefNumR);
			if (errnum != KCP_SUCCESS) {		
				goto ErrOut;
			}

			/* propagate FuT attributes */
			errnum = ComposeAttrFut(threadGlobalsP, PTRefNum1, PTRefNum2, *PTRefNumR);
			if (errnum != KCP_SUCCESS) {		
				goto ErrOut;
			}
		}
	}

ErrOut3:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
	return (errnum);
	
ErrOut2:
	fut_unlock_fut(fut2);
ErrOut1:
	fut_unlock_fut(fut1);
				
ErrOut:
	if (PTHdr != NULL) {
		freeBufferPtr(PTHdr);
	} 
	else if (PTHdrR != NULL) {
		freeBuffer(PTHdrR);
	}
	if (futR == FUT_NULL) {
		fut_free(futR);
	}
	goto ErrOut3;
}


/* see if a PT is a FuT */
static PTErr_t
	checkFuT (	PTRefNum_t	PTRefNum)
{
PTErr_t	errnum;
int32	attrSize;
char	attribute[KCM_MAX_ATTRIB_VALUE_LENGTH+1];

	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum = PTGetAttribute(PTRefNum, KCM_TECH_TYPE, &attrSize, attribute);
	if (errnum == KCP_SUCCESS) {		
		if ((strcmp(attribute, KCM_FUT_S) != 0) && (strcmp(attribute, KCM_SERIAL_FUT_S) != 0)) {
			errnum = KCP_NOT_FUT;			/* PT is not FuT technology */
		}
	}
	
	return (errnum);
}








