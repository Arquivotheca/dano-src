/*
 * @(#)attrmgr.c	2.45 97/12/22

	Manages reading, writing, changing and deletion of PT attributes

	Written by:	Drivin' Team

	Copyright:	(c) 1991-1994 by Eastman Kodak Company, all rights reserved.

	Change History (most recent first):

		 <4>	 1/11/94	sek		Cleaned up warnings
		 <3>	 12/8/93	htr		Add pointer checks to PTGetAttribute,
									 PTSetAttribute, PTGetTags
 */

#include "kcms_sys.h"

#if defined(KPMAC)
	#include <Types.h>
	#include <Memory.h>
	#include <OSUtils.h>
#endif

#include <string.h>

#include "kcmptlib.h"
#include "kcptmgrd.h"
#include "attrib.h"
#include "attrcipg.h"
#include "kcptmgr.h"

#define MAX_DIGITS 10					/* decimal digits in a 32 bit number */
#define NUMBER_OF_ATTRIB_ENTRIES 100	/* initial number of attribute entries*/

#define ADD_TAG 1						/* Add a tag to list */
#define FIND_TAG 2						/* Find tag in list */
#define DELETE_TAG 3					/* Delete tag from list */

typedef struct PTAttribCount_s {
	int32 		num_entries;			/* the current number of tag entries */
	int32 		max_entries;			/* the maximum number of tag entries */
} PTAttribCount_t, FAR *PTAttribCount_p;

typedef struct PTAttribEntry_s {
	int32 		tag;					/* the attribute tag for the PT */
	KcmHandle	string;					/* the attribute string for the PT */
} PTAttribEntry_t, FAR *PTAttribEntry_p, FAR* FAR* PTAttribEntry_h;

/* prototypes */
static PTErr_t LinearScanList ARGS((KcmHandle startAttrList, PTAttribEntry_p attrEntry, int32 mode));
static PTErr_t SetAttribute ARGS((KcmHandle *startAttrH, int32 attrTag, char *attrString));
static PTErr_t AddAttribute ARGS((KcmHandle *startAttrH, int32 attrTag, KcmHandle attrString));
static PTErr_t attrSizeCheck ARGS((kcpindex_t bytes));

KcmHandle copyAttrList (PTAttribEntry_p startAttrListP);


/* do the "PTGetAttribute" function */
PTErr_t
	PTGetAttribute(PTRefNum_t PTRefNum, int32 attrTag, int32 * size,
			char * attrString)
{
	threadGlobals_p	threadGlobalsP;		/* a pointer to the process global */
	kcpindex_t		bytes;
	KcmHandle		startAttrList;
	PTErr_t			errnum = KCP_SUCCESS;

	char KPCPversion[] = KPCP_VERSION;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}

/* Check for valid pointers */
	if (Kp_IsBadWritePtr(size, (u_int32) sizeof(*size))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}
	if (Kp_IsBadWritePtr(attrString, (u_int32) *size)) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}
	if (attrTag == KCM_KCP_VERSION) {		/* special attribute? */
 		bytes = (kcpindex_t)strlen(KPCPversion);

		if (bytes >= (kcpindex_t)*size) {
			/* if the given size was too small copy as much of the string as will fit, */
			strncpy (attrString, KPCPversion, (int) ((*size)-1));

			*(attrString + (*size-1)) = '\0';	/* null-terminate the string */
			errnum =  KCP_ATT_SIZE_TOO_SMALL;
		}
		else {
			strcpy(attrString, KPCPversion);	/* copy the entire attribute */
			errnum = KCP_SUCCESS;
		}
		*size = (int32)strlen(KPCPversion);			/* and size */
	}
	else if (attrTag == KCM_CP_RULES_DIR) {  /* special attribute? */
		/* return the path to the rules directory */
 		bytes = (kcpindex_t)strlen(threadGlobalsP->processGlobalsP->iGP->KCPDataDir);

		if (bytes >= (kcpindex_t)*size) {
			/* if the given size was too small copy as much of the string as will fit, */
			strncpy (attrString, threadGlobalsP->processGlobalsP->iGP->KCPDataDir, (int) ((*size)-1));

			*(attrString + (*size-1)) = '\0';	/* null-terminate the string */
			errnum =  KCP_ATT_SIZE_TOO_SMALL;
		}
		else {
			/* copy the entire attribute */
			strcpy(attrString, threadGlobalsP->processGlobalsP->iGP->KCPDataDir);
			errnum = KCP_SUCCESS;
		}
		*size = (int32) strlen (threadGlobalsP->processGlobalsP->iGP->KCPDataDir);
	}
	else {
		errnum = kcpGetStatus(threadGlobalsP, PTRefNum);

		if ((errnum != KCP_PT_ACTIVE) && (errnum != KCP_PT_INACTIVE) && (errnum != KCP_SERIAL_PT)) {
			errnum = KCP_NOT_CHECKED_IN;
		}
		else {
			startAttrList = getPTAttr(threadGlobalsP, PTRefNum);
			errnum = GetAttribute(startAttrList, attrTag, size, attrString);
		}
	}
ErrOut1:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
	return (errnum);
}


/* do the "PTSetAttribute" function */
PTErr_t 
	PTSetAttribute(PTRefNum_t PTRefNum, int32 attrTag, char * attrString)
{
threadGlobals_p	threadGlobalsP;		/* a pointer to the process global */
PTErr_t 		errnum = KCP_SUCCESS;
KcmHandle		startAttr;
kcpindex_t		i;
char 			nullChar = '\0';
char 			*tempAttrString;

	threadGlobalsP = KCMDloadGlobals();		/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}

	if (attrTag == KCM_KCP_VERSION) {		/* special attribute? */
		errnum = KCP_INVAL_PTA_TAG;			/* may not be set */
	}
	else {
		/* if the string isn't NULL, put it into the list */
 		if (attrString != NULL) {

			/* Check for valid pointer */
			if (Kp_IsBadStringPtr( (void *) attrString, KCM_MAX_ATTRIB_VALUE_LENGTH)) {
				/* Let's give them the benefit of the doubt, only check till we find a NULL */
				for (i = 1; i < KCM_MAX_ATTRIB_VALUE_LENGTH; i++) {
					if (Kp_IsBadStringPtr( (void *) attrString, i)) {
						errnum = KCP_BAD_PTR;
						goto ErrOut1;
					}
					else {
						if (attrString[i] == nullChar) {
							break;
						}
					}
				}
			}
			tempAttrString = attrString;
			while (*tempAttrString != nullChar) {
				if (*tempAttrString == KP_NEWLINE) {
					errnum = KCP_INVAL_PTA_TAG;
					goto ErrOut1;
				}
				tempAttrString++;
			}
		}
		errnum = kcpGetStatus(threadGlobalsP, PTRefNum);
		if ((errnum != KCP_PT_ACTIVE) && (errnum != KCP_PT_INACTIVE) && (errnum != KCP_SERIAL_PT)) {
			errnum = KCP_NOT_CHECKED_IN;
		}
		else {
			startAttr = getPTAttr(threadGlobalsP, PTRefNum);
			errnum = SetAttribute(&startAttr, attrTag, attrString);

		/* set attribute start in case this was the first */
			setPTAttr(threadGlobalsP, PTRefNum, startAttr);
		}
	}

ErrOut1:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
	return (errnum);
}


/* calculate the size needed to store all attributes in a list */
int32
	getAttrSize(KcmHandle startAttrListH)
{
	PTAttribEntry_p startAttrListP;
	PTAttribCount_p tagCount;
	int32			count, numTagEntries, loop;
	KpGenericPtr_t	attrString;

/* a 32 bit decimal number needs MAX_DIGITS characters plus null */
	char numstr[MAX_DIGITS+1];

/* if start attribute is NULL, there are no attributes */
	if (startAttrListH == NULL) {
		return 0;
	}
	count = 0;
	startAttrListP = lockBuffer (startAttrListH);
	tagCount = (PTAttribCount_p)startAttrListP;
	(PTAttribCount_p)startAttrListP++;
	numTagEntries = tagCount->num_entries;
	
	for (loop = 0; loop < numTagEntries; loop++) {
		KpItoa(startAttrListP->tag, numstr);

			/* attribute size is length of attribute tag in ascii,
			 * length of attribute string, plus 2 for equal sign
			 * and newline terminator */
		attrString = lockBuffer (startAttrListP->string);
		count += strlen(numstr) + strlen(attrString) + 2;
		unlockBuffer (startAttrListP->string);
		startAttrListP++;
	}
	unlockBuffer (startAttrListH);
	count++;	/* add a terminating null */

	return count;
}


/* simulate reading attributes */
PTErr_t 
	readAttributes(threadGlobals_p threadGlobalsP,
						KpFd_p fd,
						int32 size,
						KcmHandle * startAttrH)
{
	char	*idstr;
	PTErr_t	errnum;
	char	*attPtr;
	short	i, j;
	int32	attrTag;

/* need MAX_DIGITS chars for a 32 bit decimal number, plus null */
	char	numstr[MAX_DIGITS+1];
	int		err;

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}
	errnum = KCP_SUCCESS;
	*startAttrH = NULL;						/* assume no attributes to set */

/* if size is zero, there are no attributes to read */
	if (size == 0)
		return KCP_SUCCESS;

	if (size + sizeof(PT_t) > PTCHECKINSIZE)
		return KCP_INVAL_PT_BLOCK;

	idstr = (char *) allocBufferPtr (size);	/* get memory for id string */
	if ( idstr == NULL)
		return KCP_NO_CHECKIN_MEM;

	attPtr = idstr;							/* save the position of idstr */

/* get all the attributes into idstr and convert all newline chars to nulls */
	err = Kp_read (fd, attPtr, size);
	if (err != KCMS_IO_SUCCESS) {
		errnum = KCP_INVAL_PT_BLOCK;
		goto GetOut;
	}
	for (i=0; i< (short) size; i++) {
		if (*attPtr == KP_NEWLINE) {
			*attPtr = '\0';
		}
		attPtr++;
	}
/* now parse the idstr into individual attributes and put them in linked list */
	attPtr = idstr;	/* get beginning position of idstr */
	i = 0;
	do {

	/* first get the tag by reading everything up to the equal sign */
		j = 0;
		while ((*attPtr != '=') && (j<MAX_DIGITS)) {
			if (((*attPtr < '0') || (*attPtr > '9')) && (*attPtr != '-')) {
				goto SkipAttr;		/* non-numeric tag, just ignore it */
			}
			numstr[j++] = *attPtr;
			attPtr++;
		}
		if (j >= MAX_DIGITS) {
			errnum = KCP_INVAL_PTA_TAG;	/* tag value is too large */
			goto SkipAttr;
		}

		attPtr++;			/* to skip over the = sign */
		numstr[j] = '\0';	/* terminate with a null to make it a c string */
		attrTag = KpAtoi(numstr);	/* convert tag to an int for linked list */

	/* ignore attribute with tag of 0 */
		if (attrTag != 0) {

		/* insert this attribute */
			errnum = SetAttribute(startAttrH, attrTag, attPtr);
		}
SkipAttr:
		while (*attPtr != '\0') attPtr++;
		attPtr++; /* to skip over the null */

	/* account for terminating null */
	} while ((attPtr < (idstr + size-1)) && (errnum == KCP_SUCCESS));

GetOut:
	freeBufferPtr((KcmGenericPtr)idstr);		/* discard id string */

	return (errnum);
}


/* write attributes to external memory */
PTErr_t
	writeAttributes (threadGlobals_p threadGlobalsP,
							KpFd_p fd,
							KcmHandle startAttrListH)
{
	PTAttribEntry_p startAttrListP;
	PTAttribCount_p tagCount;
	KpGenericPtr_t	attrString;
	PTErr_t			errnum = KCP_PT_HDR_WRITE_ERR;
	int32			numTagEntries, loop;

/* need MAX_DIGITS chars for a 32 bit decimal number, plus = sign */
	char	numstr[MAX_DIGITS+1];
	char 	equal = '=';
	char 	newline = KP_NEWLINE;
	char 	nullChar = '\0';

/* if threadGlobalsP is NULL return zero */
	if (threadGlobalsP == NULL) {
		return KCP_NO_THREAD_GLOBAL_MEM;
	}
	if (startAttrListH != NULL) {
		if ((sizeof(PT_t) + getAttrSize(startAttrListH)) > PTCHECKINSIZE) {
			return KCP_NO_CHECKIN_MEM;
		}

		startAttrListP = lockBuffer (startAttrListH);
		tagCount = (PTAttribCount_p)startAttrListP;
		(PTAttribCount_p)startAttrListP++;
		numTagEntries = tagCount->num_entries;
	
		for (loop = 0; loop < numTagEntries; loop++) {
			KpItoa(startAttrListP->tag, numstr);		/* convert tag to an ascii string */

	/* write the tag, an equal sign, and the attribute string to the PT block */
			attrString = lockBuffer (startAttrListP->string);
			if ( (Kp_write (fd, numstr, (KpInt32_t)strlen(numstr)) != KCMS_IO_SUCCESS)   ||
	    		   (Kp_write (fd, &equal, 1) != KCMS_IO_SUCCESS)   ||
	    		   (Kp_write (fd, attrString, (KpInt32_t)strlen(attrString)) != KCMS_IO_SUCCESS)  ||
	    		   (Kp_write (fd, &newline, 1) != KCMS_IO_SUCCESS) )  {
				unlockBuffer (startAttrListP->string);
				errnum = KCP_PT_BLOCK_TOO_SMALL;
				goto GetOut;
			}
			errnum = KCP_SUCCESS;
			unlockBuffer (startAttrListP->string);
			startAttrListP++;
		}

	/* terminate with a null character */
		if (Kp_write (fd, &nullChar, 1) != KCMS_IO_SUCCESS)
			errnum = KCP_PT_BLOCK_TOO_SMALL;
		else
			errnum = KCP_SUCCESS;
	}

GetOut:
	unlockBuffer (startAttrListH);
	return  (errnum);
}

/* return all the attribute tags of a given PT */
PTErr_t 
	PTGetTags(PTRefNum_t PTRefNum, int32 * nTags, int32 * tagArray)
{
	threadGlobals_p threadGlobalsP;
	KcmHandle 		startAttrListH;
	PTAttribEntry_p startAttrListP;
	PTAttribCount_p tagCount;
	int32			totalEntries, numTags, loop;
	PTErr_t			errnum;

	threadGlobalsP = KCMDloadGlobals();			/* Setup this apps Globals */
	if (threadGlobalsP == NULL) {
		return (KCP_NO_THREAD_GLOBAL_MEM);
	}

	errnum = kcpGetStatus(threadGlobalsP, PTRefNum);

	if ((errnum != KCP_PT_ACTIVE) && (errnum != KCP_PT_INACTIVE) && (errnum != KCP_SERIAL_PT)) {
		errnum = KCP_NOT_CHECKED_IN;
		goto ErrOut1;
	}

/* Check for valid pointers */
	if (Kp_IsBadWritePtr(nTags, sizeof(*nTags))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}
	if (tagArray != NULL && Kp_IsBadWritePtr(tagArray, (u_int32) (*nTags * sizeof(*tagArray)))) {
		errnum = KCP_BAD_PTR;
		goto ErrOut1;
	}
	numTags = *nTags;	/* save the given number of tags */
	*nTags = 0;

	startAttrListH = getPTAttr(threadGlobalsP, PTRefNum);
	startAttrListP = lockBuffer (startAttrListH);
	tagCount = (PTAttribCount_p)startAttrListP;
	(PTAttribCount_p)startAttrListP++;
	totalEntries = tagCount->num_entries;
	
	for (loop = 0; loop < totalEntries; loop++) {

		/* add tags to the array until they are exhausted or the given
		 * number is reached.  When the number has been reached, continue
		 * to count tags but stop adding to array */
		if ((*nTags <= numTags) && (tagArray != NULL)) {
			*tagArray++ = startAttrListP->tag;
		}

		(*nTags)++;							/* return the actual number of tags */
		startAttrListP++;
	}
	unlockBuffer (startAttrListH);
	errnum = KCP_SUCCESS;
	
ErrOut1:
	KCMDunloadGlobals();					/* Unlock this apps Globals */
	return (errnum);
}

/* free the memory used for an attribute list */
PTErr_t 
	freeAttributes(KcmHandle startAttrListH)
{
	PTAttribEntry_p startAttrListP;
	PTAttribCount_p tagCount;
	int32			totalEntries, loop;

	if (startAttrListH != NULL) {
		startAttrListP = lockBuffer (startAttrListH);
		tagCount = (PTAttribCount_p)startAttrListP;
		(PTAttribCount_p)startAttrListP++;
		totalEntries = tagCount->num_entries;

	/* scan through the list until all the attributes are freed */
		for (loop = 0; loop < totalEntries; loop++) {
			freeBuffer (startAttrListP->string);
			startAttrListP++;
		}
		tagCount->num_entries = 0;
		unlockBuffer (startAttrListH);
	}	
	return (KCP_SUCCESS);
}

/* scan each entry in the linked list until a matching tag is found */
static PTErr_t
	LinearScanList(KcmHandle startAttrListH, PTAttribEntry_p attrEntry, int32 mode)
{
	PTAttribEntry_p startAttrListP;
	PTAttribCount_p tagCount;
	int32			totalEntries, loop;
	PTErr_t			errnum = KCP_SUCCESS;

	if (startAttrListH == NULL) {
		return (KCP_NO_ATTR_MEM);
	}
	startAttrListP = lockBuffer (startAttrListH);
	tagCount = (PTAttribCount_p)startAttrListP;
	(PTAttribCount_p)startAttrListP++;
	totalEntries = tagCount->num_entries;

	for (loop = 0; loop < totalEntries; loop++) {
		if (startAttrListP->tag == attrEntry->tag) {
			break;
		}
		startAttrListP++;
	}
	switch(mode) {
		case ADD_TAG:
			if (loop == totalEntries) {
				tagCount->num_entries++;			/* must be a new entry (not an over write) */
			} else {
				freeBuffer (startAttrListP->string); /* free previous tag string */
			}
			*startAttrListP = *attrEntry;
			break;

		case FIND_TAG:
			if (loop != totalEntries) {
				attrEntry->string = startAttrListP->string;
			}
			break;

		case DELETE_TAG:
			if (loop != totalEntries) {
				freeBuffer (startAttrListP->string);
				for (; loop < totalEntries-1; loop++) {
					*startAttrListP = *(startAttrListP+1);
					startAttrListP++;
				}
				startAttrListP->tag = 0;
				startAttrListP->string = NULL;
				tagCount->num_entries--;
			}
			break;

		default:
			errnum = KCP_FAILURE;
	}
	unlockBuffer (startAttrListH);

	return (errnum);
}


/* search for an attribute with a given tag */
PTErr_t
	GetAttribute(KcmHandle startAttr, int32 attrTag,
				int32 * attrStrSize, char * attrString)
{
	PTAttribEntry_t attrEntry;
	KpGenericPtr_t	tmpAttrString;
	PTErr_t			errnum;
	kcpindex_t		bytes;

	attrEntry.tag = attrTag;
	attrEntry.string = NULL;
	LinearScanList(startAttr, &attrEntry, FIND_TAG);

	if (attrEntry.string == NULL) { /* no matching tag was found in the list */
		errnum = KCP_INVAL_PTA_TAG;
	}
	else {
		tmpAttrString = lockBuffer (attrEntry.string);
 		bytes = (kcpindex_t) strlen(tmpAttrString);

		if (bytes >= (kcpindex_t)*attrStrSize) {
			/* if the given size was too small copy as much of the string as will fit, */
			strncpy (attrString, tmpAttrString, (int) ((*attrStrSize)-1));

			*(attrString + (*attrStrSize-1)) = '\0';	/* null-terminate the string */
			errnum =  KCP_ATT_SIZE_TOO_SMALL;
		}
		else {
			strcpy(attrString, tmpAttrString);	/* copy the entire attribute */
			errnum = KCP_SUCCESS;
		}
		unlockBuffer (attrEntry.string);
		*attrStrSize = bytes;	/* send back the actual size of the attribute string */
	}
	return errnum;
}


static PTErr_t
	SetAttribute(KcmHandle * startAttrListH, int32 attrTag,
				char * attrString)
{
	PTErr_t			errnum = KCP_SUCCESS;
	PTAttribEntry_t attrEntry;
	kcpindex_t		bytes;
	KcmHandle		ourStringH;
	KpGenericPtr_t	ourStringP;

	if (startAttrListH == NULL) {
		return (KCP_NO_ATTR_MEM);
	}
	attrEntry.tag = attrTag;
	attrEntry.string = NULL;
	
/* only delete the old attribute if we're not going to replace it with a new one! */
/*	it's faster this way! (only one search) */

	if (attrString == NULL) {
		LinearScanList(*startAttrListH, &attrEntry, DELETE_TAG);	/* get rid of old attribute */
		return (KCP_SUCCESS);
	}
	if (*attrString == 0) {
		LinearScanList(*startAttrListH, &attrEntry, DELETE_TAG);	/* get rid of old attribute */
		return (KCP_SUCCESS);
	}
	bytes = (kcpindex_t)strlen(attrString);

	errnum = attrSizeCheck(bytes);	/* must have valid string size */
	if (errnum != KCP_SUCCESS) {
		return errnum;
	}
	if (attrTag == 0) {
		return KCP_INVAL_PTA_TAG;
	}
	/* get memory for the attribute */
	ourStringH = allocBufferHandle ((KpInt32_t)(bytes+1));
	if (ourStringH == NULL) {
		return (KCP_NO_ATTR_MEM);
	}
	ourStringP = lockBuffer (ourStringH);
	strcpy (ourStringP, attrString);
	unlockBuffer (ourStringH);
	attrEntry.string = ourStringH;

	/* insert into list */
	errnum = AddAttribute(startAttrListH, attrTag, ourStringH);
	if (errnum != KCP_SUCCESS) {
		freeBuffer(ourStringH);	/* release the attribute's memory */
	}
	return (errnum);
}

/* make sure that an attribute string is within "reasonable" limits */
static 
	PTErr_t attrSizeCheck(kcpindex_t bytes)
{
	if ((bytes <= 0) || (bytes > KCM_MAX_ATTRIB_VALUE_LENGTH+1))
		return KCP_ATTR_TOO_BIG;		/* not a valid string size */
	else
		return KCP_SUCCESS;
}


/* add an attribute to an attribute list */
static PTErr_t 
	AddAttribute(KcmHandle *startAttrH, int32 attrTag, KcmHandle attrString)
{
	PTAttribEntry_t attrib;
	PTAttribCount_p tagCount, oldTagCount;
	KcmHandle 		attrTable;
	
	attrib.tag = attrTag;								/* insert tag */
	attrib.string = attrString;							/* insert string */

	if (*startAttrH == NULL) {							/* if there are no attributes at all */
		/* get memory for the attribute */
		tagCount = allocBufferPtr ((KpInt32_t) (sizeof(PTAttribCount_t) +
					(sizeof(PTAttribEntry_t) * NUMBER_OF_ATTRIB_ENTRIES)));
		if (tagCount == NULL) {
			return (KCP_NO_ATTR_MEM);
		}
		
		tagCount->num_entries = 0;							/* no entries yet. */
		tagCount->max_entries = NUMBER_OF_ATTRIB_ENTRIES;	/* maximum number of entries. */

		attrTable = getHandleFromPtr (tagCount);
		*startAttrH = attrTable;
	}
	else {
		oldTagCount = lockBuffer (*startAttrH);
		if (oldTagCount->num_entries == oldTagCount->max_entries) {
			tagCount = reallocBufferPtr (oldTagCount, (KpInt32_t) (sizeof(PTAttribCount_t) +
						(sizeof(PTAttribEntry_t) * (oldTagCount->max_entries + NUMBER_OF_ATTRIB_ENTRIES))));
			if (tagCount == NULL) {
				return (KCP_NO_ATTR_MEM);
			}

			tagCount->max_entries += NUMBER_OF_ATTRIB_ENTRIES;

			attrTable = getHandleFromPtr (tagCount);
			*startAttrH = attrTable;
		}
	}

	LinearScanList(*startAttrH, &attrib, ADD_TAG);			/* add to end of list */

	return (KCP_SUCCESS);
}

PTErr_t 
	copyAllAttr (threadGlobals_p threadGlobalsP,
						PTRefNum_t fromPTRefNum,
						PTRefNum_t toPTRefNum)
{
	KcmHandle 		fromAttrListH, newAttrListH, oldAttrListH;
	PTAttribEntry_p fromAttrListP;
	PTErr_t			errnum, errnum1;
	char			attrStr[KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	int32 			attrSize;

	fromAttrListH = getPTAttr(threadGlobalsP, fromPTRefNum);
	fromAttrListP = lockBuffer (fromAttrListH);

	newAttrListH = copyAttrList(fromAttrListP);
	unlockBuffer (fromAttrListH);

	if (newAttrListH != NULL) {
		oldAttrListH = getPTAttr(threadGlobalsP, toPTRefNum);
		if (oldAttrListH != NULL) {
			freeAttributes(oldAttrListH);
			freeBuffer (oldAttrListH);
		}
	/* set attribute start */
		setPTAttr(threadGlobalsP, toPTRefNum, newAttrListH);
		errnum = TpGenerateAttr(threadGlobalsP, toPTRefNum); /* generate constant attributes */
		attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
		errnum1 = GetAttribute(fromAttrListH, KCM_PRODUCT_VERSION, &attrSize, attrStr);
		if (errnum1 == KCP_SUCCESS) {
		/* write to destination PT */
			attrStr[6] = '0';	/* insert '00' for composed PT quality */
			attrStr[7] = '0';
			errnum = SetAttribute(&newAttrListH, KCM_PRODUCT_VERSION, attrStr);
			if (errnum == KCP_SUCCESS) {
				attrSize = KCM_MAX_ATTRIB_VALUE_LENGTH;
				errnum1 = GetAttribute(fromAttrListH, KCM_RAW, &attrSize, attrStr);
				if (errnum1 == KCP_SUCCESS) {
					errnum = SetAttribute(&newAttrListH, KCM_RAW, attrStr);
				}
			}
		}
	} else {
		errnum = KCP_NO_ATTR_MEM;
	}
	return (errnum);
}

KcmHandle
copyAttrList(PTAttribEntry_p startAttrListP)
{
	PTAttribCount_p tagCount, newTagCount;
	PTAttribEntry_p attribP, NewAttribP;
	KcmHandle 		tempStartAttrH, newStringH;
	KpGenericPtr_t	orgStringP, newStringP;
	kcpindex_t		bytes;
	int32			loop;

	tagCount = (PTAttribCount_p)startAttrListP;
	tempStartAttrH = allocBufferHandle ((KpInt32_t)((sizeof(PTAttribEntry_t) * 
						tagCount->max_entries) + sizeof(PTAttribCount_t)));
	if (tempStartAttrH == NULL) {
		return (NULL);
	}
	newTagCount = lockBuffer (tempStartAttrH);
	newTagCount->num_entries = tagCount->num_entries;						
	newTagCount->max_entries = tagCount->max_entries;
	attribP = (PTAttribEntry_p)tagCount + 1;
	NewAttribP = (PTAttribEntry_p)newTagCount + 1;
	for (loop = 0; loop < newTagCount->num_entries; loop++) {
		NewAttribP->tag = attribP->tag;
		orgStringP = lockBuffer (attribP->string);
 		bytes = (kcpindex_t)strlen(orgStringP);
	/* get memory for the attribute */
		newStringH = allocBufferHandle ((KpInt32_t)(bytes+1));
		if (newStringH == NULL) {
			newTagCount->num_entries = loop;						
			unlockBuffer (attribP->string);
			unlockBuffer (tempStartAttrH);
			freeAttributes(tempStartAttrH);
			freeBuffer (tempStartAttrH);
			return (NULL);
		}
		newStringP = lockBuffer (newStringH);
		strcpy (newStringP, orgStringP);
		unlockBuffer (attribP->string);
		unlockBuffer (newStringH);
		NewAttribP->string = newStringH;
		
		NewAttribP++;
		attribP++;
	}

	unlockBuffer (tempStartAttrH);
	return (tempStartAttrH);
}



