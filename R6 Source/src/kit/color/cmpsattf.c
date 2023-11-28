/*
 * @(#)cmpsattf.c	2.9 97/12/22
 *

	Contains:	FuT specific attribute propagation when composing PTs

	Written by:	Drivin' Team

	Copyright:	(c) 1991 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

		 <4>	 1/12/94	sek		Cleaned up warnings
		 <3>	 11/5/91	gbp		add default PT arg to moveAttrLIst
		 <2>	10/31/91	gbp		remove previous revision history
		 <1>	10/29/91	gbp		first checked in
*/

#include "kcmsos.h"
#include "kcmptlib.h"
#include "attrib.h"
#include "attrcipg.h"
#include "kcptmgr.h"
#include "kcpfchn.h"

#define ATTR_LIST_END 0

/* attributes which are propagated from the first("Input") PT */
static int32 propIAttrF[] = {
	KCM_IN_CHAIN_CLASS,
	KCM_IN_CHAIN_CLASS_2,
	ATTR_LIST_END};

/* attributes which are propagated from the second("Output") PT */
static int32 propOAttrF[] = {
	KCM_OUT_CHAIN_CLASS,
	KCM_OUT_CHAIN_CLASS_2,
	ATTR_LIST_END};


/* use PTRefNum1 and PTRefNum2 to propagate attributes to PTRefNumR */
PTErr_t ComposeAttrFut(threadGlobals_p threadGlobalsP,
						PTRefNum_t PTRefNum1,
						PTRefNum_t PTRefNum2,
						PTRefNum_t PTRefNumR)
{
PTErr_t	errnum = KCP_SUCCESS;
KpInt8_t	strInSpace[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
KpInt8_t	strOutSpace[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
KpInt32_t	inspace, outspace, attrSize;

	/* get 1st PT */
	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum = PTGetAttribute(PTRefNum1, KCM_SPACE_OUT, &attrSize, strOutSpace);
	if (errnum == KCP_SUCCESS) {
		outspace = KpAtoi(strOutSpace);

		/* get 2nd PT */
		attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
		errnum = PTGetAttribute(PTRefNum2, KCM_SPACE_IN, &attrSize, strInSpace);

		if (errnum == KCP_SUCCESS) {
			inspace = KpAtoi(strInSpace);

			if ((outspace == KCM_UNKNOWN) && (inspace != KCM_UNKNOWN)) {
				errnum = moveAttrList(threadGlobalsP, PTRefNum2, 0, propIAttrF, PTRefNumR);
				if (errnum == KCP_SUCCESS) {		
					errnum = moveAttrList(threadGlobalsP, PTRefNum2, 0, propOAttrF, PTRefNumR);
				}
				return (errnum);
			}
			else {
				if ((outspace != KCM_UNKNOWN) && (inspace == KCM_UNKNOWN)) {
					errnum = moveAttrList(threadGlobalsP, PTRefNum1, 0, propIAttrF, PTRefNumR);
					if (errnum == KCP_SUCCESS) {		
						errnum = moveAttrList(threadGlobalsP, PTRefNum1, 0, propOAttrF, PTRefNumR);
					}
					return (errnum);
				}
			}
		}
	}

/* propagate "input" attributes */
	errnum = moveAttrList(threadGlobalsP, PTRefNum1, 0, propIAttrF, PTRefNumR);
	if (errnum == KCP_SUCCESS) {		

	/* propagate "output" attributes */
		errnum = moveAttrList(threadGlobalsP, PTRefNum2, 0, propOAttrF, PTRefNumR);
	}
	return (errnum);
}


