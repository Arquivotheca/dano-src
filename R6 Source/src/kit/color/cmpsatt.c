/*
 * @(#)cmpsatt.c	2.21 97/12/22

	Contains:	Attribute propagation when composing PTs

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1996 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

		 <7>	 1/12/94	sek		Cleaned up warnings
		 <6>	 11/5/91	gbp		add some more propagation rules
		 <5>	10/31/91	gbp		change KCM_UCR to KCM_PRT_UCR and KCM_GCR
									to KCM_PRT_GCR.
		 <4>	 9/18/91	gbp		remove GenerateAttr()
		 <3>	 9/18/91	gbp		KCM_INPUT_LINEARIZED should be input,
									not output; use
									diagWindow()
		 <2>	 9/12/91	gbp		fix constant attribute generation
									(cannot use addresses in global memory!!!!)
		 <1>	  9/5/91	gbp		first checked in
*/


#include "kcmsos.h"

#include <string.h>

#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "attrib.h"
#include "attrcipg.h"
#include "kcptmgr.h"
#include "kcpfchn.h"


#define ATTR_LIST_END 0

/* attribute structure */
typedef struct attr_s {
	int32 tag;
	int32 value;
} attr_t;


/* prototypes */
static PTErr_t moveAttr ARGS((KcmHandle startR1AttrList, KcmHandle startR2AttrList,
	KcmAttribute attrTag, PTRefNum_t PTRefNumR));
static PTErr_t setPVstr ARGS((PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
	PTRefNum_t PTRefNumR));
static PTErr_t setCOMPstate ARGS((PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
	PTRefNum_t PTRefNumR));
static PTErr_t doPropRule07 ARGS((PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
	PTRefNum_t PTRefNumR));
static void getTime ARGS((char*	attrStr));
static PTErr_t getCurrentDate ARGS((struct kpTm *Date));
static PTErr_t convertTimeToStr ARGS((struct kpTm *time, char *str));
static void addIntStr ARGS((int32 integer, char *str));
static PTErr_t generateAttr ARGS((PTRefNum_t PTRefNumR));
static PTErr_t setInLinearized ARGS((PTRefNum_t PTRefNumR));
static PTErr_t setOutLinearized ARGS((PTRefNum_t PTRefNumR));


/* attributes propagated from the first("Input") PT */
static int32 propRule02[] = {
	KCM_SPACE_IN,
	KCM_MEDIUM_IN,
	KCM_MEDIUM_DESC_IN,
	KCM_MEDIUM_PRODUCT_IN,
	KCM_ILLUM_TYPE_IN,
	KCM_MEDIUM_SENSE_IN,
	KCM_WHITE_POINT_IN,
	KCM_KCP_INPUT_WT_UPVP,
	KCM_DEVICE_PHOSPHOR_IN,
	KCM_DEVICE_LINEARIZED_IN,
	KCM_PRIMARIES_1_IN,
	KCM_PRIMARIES_2_IN,
	KCM_PRIMARIES_3_IN,
	KCM_PRIMARIES_4_IN,
	KCM_PRIMARIES_5_IN,
	KCM_PRIMARIES_6_IN,
	KCM_PRIMARIES_7_IN,
	KCM_PRIMARIES_8_IN,
	KCM_GAMMA_RED_IN,
	KCM_GAMMA_GREEN_IN,
	KCM_GAMMA_BLUE_IN,
	KCM_SENSE_INVERTIBLE_IN,
	KCM_CHAN_NAME_1_IN,
	KCM_CHAN_NAME_2_IN,
	KCM_CHAN_NAME_3_IN,
	KCM_CHAN_NAME_4_IN,
	KCM_CHAN_NAME_5_IN,
	KCM_CHAN_NAME_6_IN,
	KCM_CHAN_NAME_7_IN,
	KCM_CHAN_NAME_8_IN,
	KCM_ICC_COLORSPACE_IN,
	ATTR_LIST_END};

/* attributes propagated from the second("Output") PT */
static int32 propRule03[] = {
	KCM_SPACE_OUT,
	KCM_MEDIUM_OUT,
	KCM_MEDIUM_DESC_OUT,
	KCM_MEDIUM_PRODUCT_OUT,
	KCM_ILLUM_TYPE_OUT,
	KCM_MEDIUM_SENSE_OUT,
	KCM_PRT_UCR,
	KCM_PRT_GCR,
	KCM_PRT_BLACK_SHAPE,
	KCM_PRT_BLACKSTART_DELAY,
	KCM_PRT_LINE_RULINGS,
	KCM_PRT_SCREEN_ANGLES,
	KCM_DMAX_OUT,
	KCM_WHITE_POINT_OUT,
	KCM_KCP_OUTPUT_WT_UPVP,
	KCM_DEVICE_PHOSPHOR_OUT,
	KCM_DEVICE_LINEARIZED_OUT,
	KCM_DEVICE_MFG_OUT,
	KCM_DEVICE_MODEL_OUT,
	KCM_DEVICE_UNIT_OUT,
	KCM_DEVICE_SETTINGS_OUT,
	KCM_PRIMARIES_1_OUT,
	KCM_PRIMARIES_2_OUT,
	KCM_PRIMARIES_3_OUT,
	KCM_PRIMARIES_4_OUT,
	KCM_PRIMARIES_5_OUT,
	KCM_PRIMARIES_6_OUT,
	KCM_PRIMARIES_7_OUT,
	KCM_PRIMARIES_8_OUT,
	KCM_GAMMA_RED_OUT,
	KCM_GAMMA_GREEN_OUT,
	KCM_GAMMA_BLUE_OUT,
	KCM_DENSITY_FILTER,
	KCM_25_DOTGAIN,
	KCM_50_DOTGAIN,
	KCM_75_DOTGAIN,
	KCM_COMPRESSION_OUT,
	KCM_SENSE_INVERTIBLE_OUT,
	KCM_CHAN_NAME_1_OUT,
	KCM_CHAN_NAME_2_OUT,
	KCM_CHAN_NAME_3_OUT,
	KCM_CHAN_NAME_4_OUT,
	KCM_CHAN_NAME_5_OUT,
	KCM_CHAN_NAME_6_OUT,
	KCM_CHAN_NAME_7_OUT,
	KCM_CHAN_NAME_8_OUT,
	KCM_ICC_COLORSPACE_OUT,
	ATTR_LIST_END};

/* attributes set to current time */
static int32 propRule04[] = {
	KCM_CREATE_TIME,
	ATTR_LIST_END};

/* attributes which are the same for all PTs, values */
static attr_t propRule05a[] = {
	{KCM_CLASS, KCM_COMPOSED_CLASS},
	{ATTR_LIST_END, 0 /* NULL */} };

/* use either "input" or "output" if in just one PT */
static attr_t propRule07[] = {
	{KCM_EFFECT_TYPE, KCM_EFFECT_MULTIPLE},	/* else if in both and same, use the common value */
	{ATTR_LIST_END, 0 /* NULL */} };		/* else if in both and different, use the default value */
																					/* else do not write either */

/* attributes propagated from the 1st(Input) PT with 2nd(Output) as backup */
static int32 propRule11[] = {
	KCM_DEVICE_MFG_IN,
	KCM_DEVICE_MODEL_IN,
	KCM_DEVICE_UNIT_IN,
	KCM_DEVICE_SETTINGS_IN,
	ATTR_LIST_END};

/* attributes propagated from the 2nd(Output) PT with 1st(Input) as backup */
/* these were moved to propRule03 March 1, 1993 */
/* static int32 propRule12[] = {
	ATTR_LIST_END}; */

/* attributes propagated from the 2nd(Output) PT with 1st(Input) as backup, and
	2nd attribute in list from 1st PT as back backup*/
static int32 propRule13[] = {
	KCM_SIM_MEDIUM_OUT,
	KCM_MEDIUM_OUT,
	KCM_SIM_MEDIUM_DESC_OUT,
	KCM_MEDIUM_DESC_OUT,
	KCM_SIM_MEDIUM_SENSE_OUT,
	KCM_MEDIUM_SENSE_OUT,
	KCM_SIM_MEDIUM_PRODUCT_OUT,
	KCM_MEDIUM_PRODUCT_OUT,
	KCM_SIM_UCR,
	KCM_PRT_UCR,
	KCM_SIM_GCR,
	KCM_PRT_GCR,
	KCM_SIM_ILLUM_TYPE_IN,
	KCM_ILLUM_TYPE_IN,
	KCM_SIM_ILLUM_TYPE_OUT,
	KCM_ILLUM_TYPE_OUT,
	KCM_SIM_WHITE_POINT_OUT,
	KCM_WHITE_POINT_OUT,
	KCM_SIM_BLACK_SHAPE,
	KCM_PRT_BLACK_SHAPE,
	KCM_SIM_BLACKSTART_DELAY,
	KCM_PRT_BLACKSTART_DELAY,
	KCM_SIM_LINE_RULINGS,
	KCM_PRT_LINE_RULINGS,
	KCM_SIM_SCREEN_ANGLE,
	KCM_PRT_SCREEN_ANGLES,
	KCM_SIM_DMAX_OUT,
	KCM_DMAX_OUT,
	KCM_SIM_DEVICE_MFG_OUT,
	KCM_DEVICE_MFG_OUT,
	KCM_SIM_DEVICE_MODEL_OUT,
	KCM_DEVICE_MODEL_OUT,
	KCM_SIM_DEVICE_UNIT_OUT,
	KCM_DEVICE_UNIT_OUT,
	KCM_SIM_DEVICE_SETTINGS_OUT,
	KCM_DEVICE_SETTINGS_OUT,
	KCM_SIM_PRIMARIES_1_OUT,
	KCM_PRIMARIES_1_OUT,
	KCM_SIM_PRIMARIES_2_OUT,
	KCM_PRIMARIES_2_OUT,
	KCM_SIM_PRIMARIES_3_OUT,
	KCM_PRIMARIES_3_OUT,
	KCM_SIM_PRIMARIES_4_OUT,
	KCM_PRIMARIES_4_OUT,
	KCM_SIM_PRIMARIES_5_OUT,
	KCM_PRIMARIES_5_OUT,
	KCM_SIM_PRIMARIES_6_OUT,
	KCM_PRIMARIES_6_OUT,
	KCM_SIM_PRIMARIES_7_OUT,
	KCM_PRIMARIES_7_OUT,
	KCM_SIM_PRIMARIES_8_OUT,
	KCM_PRIMARIES_8_OUT,
	KCM_SIM_COMPRESSION_OUT,
	KCM_COMPRESSION_OUT,
	KCM_SIM_CHAN_NAME_1_OUT,
	KCM_CHAN_NAME_1_OUT,
	KCM_SIM_CHAN_NAME_2_OUT,
	KCM_CHAN_NAME_2_OUT,
	KCM_SIM_CHAN_NAME_3_OUT,
	KCM_CHAN_NAME_3_OUT,
	KCM_SIM_CHAN_NAME_4_OUT,
	KCM_CHAN_NAME_4_OUT,
	KCM_SIM_CHAN_NAME_5_OUT,
	KCM_CHAN_NAME_5_OUT,
	KCM_SIM_CHAN_NAME_6_OUT,
	KCM_CHAN_NAME_6_OUT,
	KCM_SIM_CHAN_NAME_7_OUT,
	KCM_CHAN_NAME_7_OUT,
	KCM_SIM_CHAN_NAME_8_OUT,
	KCM_CHAN_NAME_8_OUT,
	ATTR_LIST_END};


/* use PTRefNum1 and PTRefNum2 to propagate attributes to PTRefNumR */
PTErr_t ComposeAttr(threadGlobals_p threadGlobalsP,
					PTRefNum_t PTRefNum1,
					PTRefNum_t PTRefNum2,
					int32 mode,
					PTRefNum_t PTRefNumR)
{
	PTErr_t	errnum = KCP_SUCCESS;
	char	strInSpace[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	char	strOutSpace[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	int32	inspace, outspace = 0, attrSize;

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
				errnum = copyAllAttr(threadGlobalsP, PTRefNum2, PTRefNumR);
				return (errnum);
				
			}
			else {
				if ((outspace != KCM_UNKNOWN) && (inspace == KCM_UNKNOWN)) {
					errnum = copyAllAttr(threadGlobalsP, PTRefNum1, PTRefNumR);
					return (errnum);
				}
			}
		}
	}
	
/* set up the KCM_PRODUCT_VERSION string */
	errnum = setPVstr(PTRefNum1, PTRefNum2, PTRefNumR);

/* set up the KCM_COMPOSITION_STATE */
	errnum = setCOMPstate(PTRefNum1, PTRefNum2, PTRefNumR);

/* propagate "input" attributes */
	if (errnum == KCP_SUCCESS)
		errnum = moveAttrList(threadGlobalsP, PTRefNum1, 0, propRule02, PTRefNumR);

/* propagate "output" attributes */
	if (errnum == KCP_SUCCESS)
		errnum = moveAttrList(threadGlobalsP, PTRefNum2, 0, propRule03, PTRefNumR);

/* generate "constant" attributes */
	if (errnum == KCP_SUCCESS)
		errnum = generateAttr(PTRefNumR);

/* if composition mode is input set input attribute to "is linearized" */
	if (errnum == KCP_SUCCESS && mode == PT_COMBINE_ITBL)
		errnum = setInLinearized(PTRefNumR);

/* if composition mode is output set output attribute to "is linearized" */
	if (errnum == KCP_SUCCESS && mode == PT_COMBINE_OTBL)
		errnum = setOutLinearized(PTRefNumR);

/* generate "rule 7" attributes */
	if (errnum == KCP_SUCCESS)
		errnum = doPropRule07(PTRefNum1, PTRefNum2, PTRefNumR);

/* propagate "input" attributes */
	if (errnum == KCP_SUCCESS)
		errnum = moveAttrList(threadGlobalsP, PTRefNum1, PTRefNum2, propRule11, PTRefNumR);

/* propagate "simulate" attributes */
	if (errnum == KCP_SUCCESS)
		errnum = moveAttrList(threadGlobalsP, PTRefNum2, PTRefNum1, propRule13, PTRefNumR);

	return (errnum);

}


/* create the KCM_PRODUCT_VERSION string */
static PTErr_t setPVstr(PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
						PTRefNum_t PTRefNumR)
{
char strProdVerDflt[] = "00.00.00";
char strProdVer[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
PTErr_t errnum;
int32 attrSize;

/* try 1st PT */
	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum = PTGetAttribute(PTRefNum1,
					KCM_PRODUCT_VERSION, &attrSize, strProdVer);
	if (errnum != KCP_SUCCESS) {

	/* try 2nd PT */
		attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
		errnum = PTGetAttribute(PTRefNum2,
						KCM_PRODUCT_VERSION, &attrSize, strProdVer);
		if (errnum != KCP_SUCCESS) {

		/* neither, use default */
			strcpy(strProdVer, strProdVerDflt);
		}
	}

	strProdVer[6] = '0';	/* insert '00' for composed PT quality */
	strProdVer[7] = '0';

/* write to destination PT */
	errnum = PTSetAttribute(PTRefNumR, KCM_PRODUCT_VERSION, strProdVer);

	return (errnum);
}


/* propagate a list of attributes */
PTErr_t moveAttrList(threadGlobals_p threadGlobalsP,
					PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
					int32 * attrList, PTRefNum_t PTRefNumR)
{
	PTErr_t 	errnum = KCP_SUCCESS;
	KcmHandle	startR1AttrList = NULL, startR2AttrList = NULL;
	int 		i1;
	int32		attrSize;
	char		attribute[KCM_MAX_ATTRIB_VALUE_LENGTH+1];

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum1);
	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_PT_INACTIVE) || (errnum == KCP_SERIAL_PT)) {
		startR1AttrList = getPTAttr(threadGlobalsP, PTRefNum1);
	}
	
	errnum = kcpGetStatus(threadGlobalsP, PTRefNum2);
	if ((errnum == KCP_PT_ACTIVE) || (errnum == KCP_PT_INACTIVE) || (errnum == KCP_SERIAL_PT)) {
		startR2AttrList = getPTAttr(threadGlobalsP, PTRefNum2);
	}
	
	for (i1 = 0; attrList[i1] != ATTR_LIST_END; i1++) {

	/* move each attribute */
		errnum = moveAttr(startR1AttrList, startR2AttrList, attrList[i1], PTRefNumR);

	/* don't worry if its not there */
		if (errnum == KCP_INVAL_PTA_TAG) {
			if (attrList != propRule13) {
				errnum = KCP_SUCCESS;	/* most PTs will not have all attributes */
			} else {
				attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
				/* read attribute */
				errnum = GetAttribute(startR2AttrList, attrList[i1+1], &attrSize, attribute);
				if (errnum == KCP_SUCCESS) {
					/* write to destination PT */
					errnum = PTSetAttribute(PTRefNumR, attrList[i1], attribute);
				} else if (errnum == KCP_INVAL_PTA_TAG) {
					/* don't worry if its not there */
					errnum = KCP_SUCCESS;	/* most PTs will not have all attributes */
				}
			}
		}
		if (errnum != KCP_SUCCESS) {
			return (errnum);
		}
		if (attrList == propRule13) {
			/* slip over output attribute. */
			i1++;
		}
	}

	return (errnum);
}


/* move an attribute from one PT to another */
static PTErr_t moveAttr(KcmHandle startR1AttrList, KcmHandle startR2AttrList,
						KcmAttribute attrTag, PTRefNum_t PTRefNumR)
{
	PTErr_t errnum = KCP_FAILURE;
	int32 attrSize;
	char attribute[KCM_MAX_ATTRIB_VALUE_LENGTH+1];

	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;

/* read attribute */
	if (startR1AttrList != NULL) {
		errnum = GetAttribute(startR1AttrList, attrTag, &attrSize, attribute);
	}

	if ((errnum != KCP_SUCCESS) && (startR2AttrList != NULL)) {
		attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;

	/* try for default attribute */
		errnum = GetAttribute(startR2AttrList, attrTag, &attrSize, attribute);
	}

	if (errnum == KCP_SUCCESS) {

	/* write to destination PT */
		errnum = PTSetAttribute(PTRefNumR, attrTag, attribute);
	}

	return (errnum);
}


/* Generate attributes which must be in each PT */
static PTErr_t generateAttr(PTRefNum_t PTRefNumR)
{
	PTErr_t errnum;
	char attrStr[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	int i1;

	errnum = KCP_SUCCESS;
	for (i1 = 0; propRule04[i1] != ATTR_LIST_END; i1++) {
		getTime(attrStr);

	/* write time to destination PT */
		errnum = PTSetAttribute(PTRefNumR, propRule04[i1], attrStr);
		if (errnum != KCP_SUCCESS)
			return (errnum);
	}

	for (i1 = 0; propRule05a[i1].tag != ATTR_LIST_END; i1++) {
		KpItoa(propRule05a[i1].value, attrStr);

	/* write to destination PT */
		errnum = PTSetAttribute(PTRefNumR, propRule05a[i1].tag, attrStr);
		if (errnum != KCP_SUCCESS)
			return (errnum);
	}

	return (errnum);
}


/* Generate Rule 7 attributes, which use a default if the attribute is
 * present in both PTs but with different values */
PTErr_t doPropRule07(PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
					 PTRefNum_t PTRefNumR)
{
	PTErr_t errnum;
	char attrStr[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	kcpindex_t i1;

	errnum = KCP_SUCCESS;
	for (i1 = 0; propRule07[i1].tag != ATTR_LIST_END; i1++) {
		errnum = chkPropRule07(propRule07[i1].tag, PTRefNum1, PTRefNum2, attrStr);
		if (errnum == KCP_AP_RULE7_FAILURE) {
			KpItoa(propRule07[i1].value, attrStr);		/* use default */
			errnum = KCP_SUCCESS;
		}
		if (errnum == KCP_SUCCESS) {

		/* write to destination PT */
			errnum = PTSetAttribute(PTRefNumR, propRule07[i1].tag, attrStr);
			if (errnum != KCP_SUCCESS) {
				return (errnum);
			}
		}

	/* do not worry if attribute is not present */
		else if (errnum == KCP_INVAL_PTA_TAG) {
			errnum = KCP_SUCCESS;
		}
	}

	return (errnum);
}


/* check an attribute for Rule 7 consistency.
 * If the attribute is in neither PT, return KCP_INVAL_PTA_TAG.
 * If the attribute is in only 1 of the PTs or is in both PTs and is the same,
 * return KCP_SUCCESS and the value of the attribute.
 * else return KCP_AP_RULE7_FAILURE.
 */
PTErr_t chkPropRule07(int32 attrTag, PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
					  char * attrVal)
{
	char	attrStr1[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	char	attrStr2[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	PTErr_t	errnum, errnum1, errnum2;
	int32	attrSize;

	errnum = KCP_SUCCESS;

/* get attribute from 1st PT */
	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum1 = PTGetAttribute(PTRefNum1, attrTag, &attrSize, attrStr1);

/* get attribute from 2nd PT */
	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum2 = PTGetAttribute(PTRefNum2, attrTag, &attrSize, attrStr2);

	if (errnum1 == KCP_SUCCESS) {
		if (errnum2 == KCP_SUCCESS) {
		  if (strcmp(attrStr1, attrStr2) == 0) {
				strcpy(attrVal, attrStr1);	/* attribute values are the same,
											 * return value */
			}
			else {
				errnum = KCP_AP_RULE7_FAILURE;	/* attribute values are not the
												 * same, return failure */
			}
		}
		else {
			strcpy(attrVal, attrStr1);	/* attribute only in 1st PT,
										 * return value */
		}
	}
	else {
		if (errnum2 == KCP_SUCCESS) {
			strcpy(attrVal, attrStr2);	/* attribute only in 2nd PT,
										 * return value */
		}
		else {
			errnum = KCP_INVAL_PTA_TAG;	/* not in either PT,
										 * return failure */
		}
	}

	return (errnum);
}


/* get the current date and time in attribute format */
static void getTime(char * attrStr)
{
	struct kpTm Date;

	(void) getCurrentDate (&Date);	/* get the current date and time */
	(void) convertTimeToStr(&Date, attrStr);	/* convert to string */
}


/* This function returns the current date and time
 * Platform dependancies will need to be added.
 */
static PTErr_t getCurrentDate (struct kpTm * Date)
{
	struct kpTm time_now;

	KpGetLocalTime( &time_now );

	Date->year  = time_now.year;
	Date->mon   = time_now.mon;
	Date->mday  = time_now.mday;
	Date->hour  = time_now.hour;
	Date->min   = time_now.min;
	Date->sec   = time_now.sec;
	Date->wday  = time_now.wday;
	Date->yday  = time_now.yday;
	Date->isdst = time_now.isdst;

	return KCP_SUCCESS;
}


/* This function converts a date and time structure to a string */
/*   The date format used in PTs is yyyy:mm:dd:hh:mm:ss */
static PTErr_t convertTimeToStr(struct kpTm * time, char * str)
{
	KpItoa((int32)time->year, str);	/* start with year */
	addIntStr((int32)time->mon, str);	/* month */
	addIntStr((int32)time->mday, str);	/* day */
	addIntStr((int32)time->hour, str);	/* hour */
	addIntStr((int32)time->min, str);	/* minute */
	addIntStr((int32)time->sec, str);	/* second */

	return KCP_SUCCESS;
}


/* insert delimiter ":", convert integer to ascii,
 * and append it to given string */
static void addIntStr(int32 integer, char * str)
{
	char intStr[20];		/* really large just in case */

	strcat( str, ":" );			/* insert delimiter */
	KpItoa(integer, intStr);	/* convert integer to ascii */
	strcat( str, intStr);		/* append to input string */
}


static PTErr_t setCOMPstate(PTRefNum_t PTRefNum1, PTRefNum_t PTRefNum2,
							PTRefNum_t PTRefNumR)
{
PTErr_t	errnum = KCP_SUCCESS;
KpInt8_t	strInSpace[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
KpInt8_t	strOutSpace[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
KpInt8_t	attrStr[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
KpInt32_t	inspace, outspace, compState;
KpInt32_t	attrSize;

	compState = KCM_COMPOSED;	/* assume composed state */
	
	/* try 1st PT */
	attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
	errnum = PTGetAttribute(PTRefNum1, KCM_SPACE_IN, &attrSize, strInSpace);

	if (errnum == KCP_SUCCESS) {
		inspace = KpAtoi(strInSpace);

		/* try 2nd PT */
		attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
		errnum = PTGetAttribute(PTRefNum2, KCM_SPACE_OUT, &attrSize, strOutSpace);

		if (errnum == KCP_SUCCESS) {
			outspace = KpAtoi(strOutSpace);

			if (inspace == KCM_RCS || outspace == KCM_RCS) {
				compState = KCM_SINGLE;							/* partial fut */
			}
		}
	}
	
	KpItoa(compState, attrStr);	/* convert to ASCII */

	errnum = PTSetAttribute(PTRefNumR, KCM_COMPOSITION_STATE, attrStr);	/* write to destination PT */

	return (errnum);
}

/* if composition mode is input set input attribute to "is linearized" */
static PTErr_t setInLinearized(PTRefNum_t PTRefNumR)
{
	PTErr_t errnum = KCP_SUCCESS;
	char attrStr[KCM_MAX_ATTRIB_VALUE_LENGTH+1];

	KpItoa(KCM_IS_LINEARIZED, attrStr);

	/* write to destination PT */
	errnum = PTSetAttribute(PTRefNumR, KCM_DEVICE_LINEARIZED_IN, attrStr);

	return (errnum);
}

/* if composition mode is output set output attribute to "is linearized" */
static PTErr_t setOutLinearized(PTRefNum_t PTRefNumR)
{
	PTErr_t errnum = KCP_SUCCESS;
	char attrStr[KCM_MAX_ATTRIB_VALUE_LENGTH+1];

	KpItoa(KCM_IS_LINEARIZED, attrStr);

	/* write to destination PT */
	errnum = PTSetAttribute(PTRefNumR, KCM_DEVICE_LINEARIZED_OUT, attrStr);

	return (errnum);
}

