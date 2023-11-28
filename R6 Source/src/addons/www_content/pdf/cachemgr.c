/*
 * cachemgr.c
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author:  Mike Dewsnap
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */


#include "config.h"
#include "cachemgr.h"


#ifndef CM_DEBUG
#define CM_DEBUG 0
#endif


/*  Use this to calculate a hash key into the cache */

#ifndef CALCHASHIDX
#define CALCHASHIDX(req) hashidx = \
	((req->instCode + req->charCode + req->fontCode) % HASHSZ)
#endif

/*  Use this function to determine if the character we are looking */
/*  for has been found											   */

#ifndef CMPDESC
#define CMPDESC(cur, req, match) match = \
	((cur->charCode == req->charCode) && \
	(cur->fontCode == req->fontCode) && \
	(cur->instCode == req->instCode))
#endif


/* Used to align up memory in the cache */

#define SIZEALIGN(size) size = (((size-1) / STRUCTALIGN) + 1) * STRUCTALIGN


/* local function prototypes */
static void *ff_cm_GetMemory(void *cache, long length);

static void CmInitializeCache(
    void **pCacheContext,
    long len, 
    char *cacheptr);

static chardata_hdr *CmAllocMem(
    FF_CM_Class *pCmGlobals,
    long size);


static chardata_hdr *CmFindChar(
    FF_CM_Class *pCmGlobals,
    char_desc_t *req);



static chardata_hdr *CmMakeChar(
    T2K **theScaler,
	long char_code,
	int8 xFracPenDelta, 
	int8 yFracPenDelta,
	uint8 greyScaleLevel, 
	uint16 cmd,
    uint16 font_code,
	uint32 inst_code,
	int *errCode);

static memory_hdr *CmFreeMem(
    FF_CM_Class *pCmGlobals,
    chardata_hdr * p);






#if CM_DEBUG

void CmCheckCache(
    FF_CM_Class *pCmGlobals);
#endif





/************************************************************************/
/*  This function is used to set up the cache within the space provided */
/*  It then returns a pointer back to the context.						*/
/************************************************************************/

static void CmInitializeCache(
    void **pCacheContext,
    long len, 
    char *cacheptr)
{
FF_CM_Class *pCmGlobals;
int i;
chardata_hdr **hashtemp;



/* Determine if the amount of memory that has been earmarked more the   */
/* cache will be enough to hold everything.								*/

if ((unsigned long)len < (sizeof(FF_CM_Class) + HASHSZ * sizeof(*hashtemp) + sizeof(memory_hdr)))
    {
    pCmGlobals = NULL;
    }
else
    {

	/* There was enough memory so set up the structures and initialize  */
	/* all the pointers used in the cache!!							    */

    pCmGlobals = (FF_CM_Class *)cacheptr;
    hashtemp = pCmGlobals->hashtable = (chardata_hdr **)((char *)cacheptr + 
                                      sizeof(FF_CM_Class));

    for (i =  0; i < HASHSZ; i++)
        *hashtemp++ = NULL;

    pCmGlobals->freelist = (memory_hdr *)hashtemp;    
    pCmGlobals->freelist->len = 
            (long)(len - HASHSZ * sizeof(chardata_hdr *) - sizeof(FF_CM_Class));
    pCmGlobals->cacheSize = (long)pCmGlobals->freelist->len;
    pCmGlobals->freelist->next = pCmGlobals->freelist->prev = NULL;
    pCmGlobals->lruhead = pCmGlobals->lrutail = NULL;
    }

*pCacheContext = (void *)pCmGlobals;

#if CM_DEBUG
	{
	CmCheckCache(pCmGlobals);
	}
#endif
}

/************************************************************************/


/*******************************************************************/
/* Function used for the construction of a new Cache Manager Object */
/* The user of this class must pass in the size allocated for the	*/
/* cache to reside in.  A pointer to the memory area is passed back	*/
/********************************************************************/
#ifdef PALM
/* this is for testing only!!! */
static MemHandle memH;
#endif

FF_CM_Class *FF_CM_New(long sizeofCache, int *errCode)
{
	FF_CM_Class *pCmGlobals = NULL;
	tsiMemObject *mem = NULL;
	char *cacheptr = NULL;

	/* Create the memory object that will hold the cache */
	mem = tsi_NewMemhandler( errCode );
	assert( *errCode == 0 );
	
	if (!*errCode)
	{
		/* Now actually allocate the memory for the cache */
#ifdef PALM
		memH = MemHandleNew((UInt32)sizeofCache);
		if (memH)
			cacheptr = (char *)MemHandleLock(memH);
#else
		cacheptr = (char *)tsi_AllocMem( mem, (size_t)sizeofCache );
#endif		
		if (cacheptr)
		{
			/* Using this memory block go and assemble the cache */
			/* initialize pointers, setup data structures, etc.  */
		
			CmInitializeCache((void **)&pCmGlobals, sizeofCache, cacheptr);
		
			/* Save this pointer to the cache context within the global */
			/* cache structure itself -- wild huh?					*/
		
			pCmGlobals->mem = mem;
			pCmGlobals->BitmapFilter = NULL;
			pCmGlobals->filterParamsPtr = NULL;
		}
		else
		{
			*errCode = T2K_ERR_MEM_MALLOC_FAILED;
		}
	}
    /* Return the self pointer back to the creating application */
	return (pCmGlobals);
}

/*********************************************************************/




/*********************************************************************/
/* Function used as a destructor for the cache manager object.  It   */
/* cleans up after itself.											 */
/*********************************************************************/

void FF_CM_Delete(FF_CM_Class *theCache, int *errCode)
{
	tsiMemObject *tempmem = theCache->mem;

	*errCode = 0;
#ifdef PALM
	MemHandleFree(memH);
#else
	tsi_DeAllocMem( theCache->mem, (void *)theCache);
#endif
	tsi_DeleteMemhandler( tempmem);
}

/*********************************************************************/


void FF_CM_SetFilter(
						FF_CM_Class *theCache,		/* Cache class pointer returned from FF_CM_New() */
						uint16 theFilterTag,		/* numeric tag for identifying filtering done
														by theFilterFunc() */
						FF_T2K_FilterFuncPtr BitmapFilter,	/* function pointer to T2K filter spec */
						void *filterParamsPtr)				/* Pointer to optional parameter block for filter function */
{

	theCache->BitmapFilter = BitmapFilter;
	theCache->filterParamsPtr = filterParamsPtr;
	theCache->FilterTag = theFilterTag;
}
/*********************************************************************/
/* Function called to try and either retrieve a particular character */
/* from the cache or actually construct a character by making calls  */
/* to the Core.														 */
/*********************************************************************/



void FF_CM_RenderGlyph(
		FF_CM_Class *theCache,
		uint32 font_code,
		T2K **theScaler,
		long char_code,
		int8 xFracPenDelta, 
		int8 yFracPenDelta,
		uint8 greyScaleLevel, 
		uint16 cmd,
		int *errCode)
{
	T2K *tScaler;

	/* Place holder from the cache find function */
	chardata_hdr *tempReturnChar;
	
	/* Must create a seach structure for the character we are interested in */
	char_desc_t	request, *prequest;
	uint32 inst_code;

	tScaler = *theScaler;
	/* This instance code is made up of 4 bits each from the subpixel position */
	/* fields as well as any filter tag that has been set up in the scaler we  */
	/* are using.															   */

	inst_code  = (uint32)((((xFracPenDelta & 0x2F) << 14) | ((yFracPenDelta & 0x2F) << 8) | 
					(theCache->FilterTag & 0xFF)));


	/* Load up the request and go look for the character */
	request.fontCode = font_code;
	request.charCode = (uint16)char_code;
	request.instCode = inst_code;
	prequest = &request;

	tempReturnChar = CmFindChar(theCache, prequest);

	if (tempReturnChar != NULL)
	{
		/* We came back with the character in the cache so now the proper */
		/* fields in the scaler structure must be set up to deliver back  */
		/* to the calling application.  This longbit field is used to     */
		/* determine whether or not the stored image is made up as long   */
		/* or short data.  The appropriate image data pointer in the      */
		/* scaler structure must be set based on this flag.				  */

#if CM_DEBUG
		printf("Char found in the Cache!!\n");
#endif

		if (tempReturnChar->cacheSpecs.longbit == 1)
			tScaler->baseARGB = (uint32 *)tempReturnChar + 
					sizeof(chardata_hdr) + 
					sizeof(char_desc_t);
		else
			tScaler->baseAddr = (uint8 *)tempReturnChar + 
					sizeof(chardata_hdr) + 
					sizeof(char_desc_t);

		tScaler->internal_baseAddr = false; /* The address is from the cache and not internal to the T2K core ! */

		/* Now all the other information which was stored about the character */
		/* is retrieved and plugged into the current scaler structure         */

		tScaler->horizontalMetricsAreValid = tempReturnChar->cacheSpecs.horizontalMetricsAreValid;
		tScaler->xAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.xAdvanceWidth16Dot16;
		tScaler->yAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.yAdvanceWidth16Dot16;
		tScaler->xLinearAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.xLinearAdvanceWidth16Dot16;
		tScaler->yLinearAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.yLinearAdvanceWidth16Dot16;
		tScaler->fTop26Dot6 = tempReturnChar->cacheSpecs.fTop26Dot6;
		tScaler->fLeft26Dot6 = tempReturnChar->cacheSpecs.fLeft26Dot6;
		tScaler->verticalMetricsAreValid = tempReturnChar->cacheSpecs.verticalMetricsAreValid;
		tScaler->vert_xAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.vert_xAdvanceWidth16Dot16;
		tScaler->vert_yAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.vert_yAdvanceWidth16Dot16;
		tScaler->vert_xLinearAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.vert_xLinearAdvanceWidth16Dot16;
		tScaler->yLinearAdvanceWidth16Dot16 = tempReturnChar->cacheSpecs.yLinearAdvanceWidth16Dot16;
		tScaler->vert_fTop26Dot6 = tempReturnChar->cacheSpecs.vert_fTop26Dot6;
		tScaler->vert_fLeft26Dot6 = tempReturnChar->cacheSpecs.vert_fLeft26Dot6;
		tScaler->width = tempReturnChar->cacheSpecs.width;
		tScaler->height = tempReturnChar->cacheSpecs.height;
		tScaler->rowBytes = tempReturnChar->cacheSpecs.rowBytes;
		tScaler->embeddedBitmapWasUsed = tempReturnChar->cacheSpecs.embeddedBitmapWasUsed;

		/* Currently returning a pointer to the character in the cache -- */
		/* this might change since all the important data has been placed */
		/* into the scaler at this point.								  */

		*errCode = 0;
	}
	else
	{
	 /* Did not find the character in the cache then the character must be */
	 /* created.  First, plug the current cache manager handle into the    */
	 /* valid scaler structure.  Then make the call to the internal make   */
	 /* character routine passing through all the information the app sent */
	 /* down regarding this character.									   */

#if CM_DEBUG
		printf("Have to make the character, then put in cache!\n");
#endif

	 tScaler->theCache = theCache;
	 tScaler->GetCacheMemory = (FF_GetCacheMemoryPtr)ff_cm_GetMemory;
	 FF_Set_T2K_Core_FilterReference( tScaler, theCache->BitmapFilter, theCache->filterParamsPtr );
	 tempReturnChar = CmMakeChar(theScaler, char_code, 
							xFracPenDelta, 
							yFracPenDelta,
							greyScaleLevel, 
							cmd,
							font_code,
							inst_code,
							errCode);

	 tScaler = *theScaler;

	 /* Clean up the scaler structure -- remove the cache pointer */
	 tScaler->theCache = NULL;
	 tScaler->GetCacheMemory = NULL;
	 FF_Set_T2K_Core_FilterReference( tScaler, NULL, NULL );


	} 
}

/*********************************************************************/


/*********************************************************************/
/*  Function callback called by the core to make requests for the    */
/*  memory needed to store bitmap characters in the cache.			 */
/*  Once the RenderGlyph call is made to the core the space needed   */
/*  to hold the bitmap is calculated.  This is a request to the cache*/
/*  for enough memory to store the image.  A pointer to the header   */
/*  block for the character is also set for use when the flow of     */
/*  control returns back to the cache manager make char routine in   */
/*  to store all the other associated data involving the character.  */
/*********************************************************************/


static void *ff_cm_GetMemory(void *cache, long length)
{
	chardata_hdr *tempReturnChar;


	/* Actually make the request to see what we got on hand!! */

	tempReturnChar = CmAllocMem((FF_CM_Class *)cache ,
				(long)(length + sizeof(chardata_hdr) + sizeof(char_desc_t)));

	if (tempReturnChar != NULL)
	{
		/* Had enough room to service the request.  Actually save the */
		/* pointer to the space in the cache global struct			  */

		((FF_CM_Class *)cache)->imagedata = (uint8 *)tempReturnChar + 
					sizeof(chardata_hdr) + 
					sizeof(char_desc_t);
		
		/* Send back the image pointer as a void */
		return((void *)((uint8 *)tempReturnChar + sizeof(chardata_hdr) + sizeof(char_desc_t)));
	

	}

	/* Could not swing this request- therefore send back a null */

	else return NULL;

}

/*********************************************************************/




/************************************************************** 
*
*    CmReinitCache
*
*       Flush cache contents and recreate cache data structures
*
***************************************************************/


void FF_CM_Flush(
    FF_CM_Class *cache, int *errCode)
{
int i;
chardata_hdr **hashtemp;

*errCode = 0;
if (cache == NULL)
	{
	*errCode = T2K_ERR_NULL_MEM;
    return;
    }

hashtemp = cache->hashtable = (chardata_hdr **)((char *)cache + 
                                      sizeof(FF_CM_Class));

for (i =  0; i < HASHSZ; i++)
    *hashtemp++ = NULL;

cache->freelist = (memory_hdr *)hashtemp;    
cache->freelist->len = cache->cacheSize;
cache->freelist->next = cache->freelist->prev = NULL;
cache->lruhead = cache->lrutail = NULL;


#if CM_DEBUG
CmCheckCache(cache);
#endif
}

/************************************************************** 
*
*    CmCheckCache
*
*       Check the integrity of the cache data structures
*		Calculates how much memory is used from the free list
*		Checks usage list forwards and backwards
*		Checks each character list forwards and backwards
*		Checks total character list memory usage
*
***************************************************************/
#if CM_DEBUG

static void CmCheckCache(
    FF_CM_Class *pCmGlobals)
{
int	error = 0;
long freeSize;
long usageSize1, usageSize2;
long charSize = 0;
memory_hdr *pMem;
chardata_hdr *pChar;
short i;

/* Exit if cache is not available */
if (pCmGlobals == NULL)
	return;
	
/* Get memory size of usage list in forward direction */
usageSize1 = 0;
for (pChar = pCmGlobals->lruhead;
	pChar != NULL;
	pChar = pChar->lrunext)
	{
	usageSize1 += pChar->len;
	}

/* Get memory size of usage list in reverse direction */
usageSize2 = 0;
for (pChar = pCmGlobals->lrutail;
	pChar != NULL;
	pChar = pChar->lruprev)
	{
	usageSize2 += pChar->len;
	}
	
/* Get memory size of character lists */
charSize = 0;
for (i =  0; i < HASHSZ; i++)
	{
	for (pChar = pCmGlobals->hashtable[i];
		pChar != NULL;
		pChar = pChar->next)
		{
		charSize += pChar->len;
		}
	}

/* Get size of free list */
freeSize = 0;
for (pMem = pCmGlobals->freelist;
	pMem != NULL;
	pMem = pMem->next)
   {
   freeSize += pMem->len;
   }

/* Check sizes */
if (usageSize1 != usageSize2)
	error |= 1;
if (charSize != usageSize1)
	error |= 2;
if (charSize + freeSize != pCmGlobals->cacheSize)
	error |= 4;
if (error != 0)
	error |= 8;
		
if (error)
	printf("CmCheckCache: ***** error = 0x%x\n", (int)error);
}
#endif

/************************************************************** 
*
*    CmFindChar
*
*       See if requested character is already in cache
*		if found, updates the usage list and returns the character.
*	    Returns BTSNULL if the character is not found.
*
***************************************************************/


static chardata_hdr *CmFindChar(
    FF_CM_Class *pCmGlobals,
    char_desc_t *prequest)
{
unsigned long hashidx;
chardata_hdr *current;
chardata_hdr *temp;
char_desc_t *curreq;

#if CM_DEBUG
CmCheckCache(pCmGlobals);
#endif

CALCHASHIDX(prequest);

for (current = pCmGlobals->hashtable[hashidx];
	current != NULL;
	current = current->next)
	{
	int match;

	curreq = (char_desc_t *)((char *)current + sizeof (chardata_hdr));
	CMPDESC(curreq, prequest, match); 
	if (match)
		{
		/* Move the character to the head of the usage list */
		if (current != pCmGlobals->lruhead)
			{
			/* Disconnect character from usage list */
			if (current == pCmGlobals->lrutail)
				{
	    		pCmGlobals->lrutail = current->lruprev;
	    		}
			else
				{
	    		current->lrunext->lruprev = current->lruprev;
	    		}
			current->lruprev->lrunext = current->lrunext;
			
			/* Insert character at head of usage list */
			temp = pCmGlobals->lruhead;
			current->lrunext = temp;
			temp->lruprev = current;
			pCmGlobals->lruhead = current;
			current->lruprev = NULL;
			}
		break;
	   	}
	}

#if CM_DEBUG
CmCheckCache(pCmGlobals);
#endif

return current; 
}

/************************************************************** 
*
*     CmMakeChar
*
*         Add requested character to cache
*
***************************************************************/


static chardata_hdr *CmMakeChar(
    T2K **theScaler,
	long char_code,
	int8 xFracPenDelta, 
	int8 yFracPenDelta,
	uint8 greyScaleLevel, 
	uint16 cmd,
    uint16 font_code,
	uint32 inst_code,
	int *errCode)
{
unsigned long hashidx;
chardata_hdr *temp; 
char_desc_t *curreq;



T2K *tScaler = *theScaler;

	
/*  Extract the cache pointer out of the current valid scaler */

FF_CM_Class *tempCache = (FF_CM_Class *)tScaler->theCache;
void *theFM = tScaler->theFM;

#if CM_DEBUG
CmCheckCache(tempCache);
#endif


/*  Initialize the pointers in the global region of the cache */
tempCache->imagedata = NULL;


/* Call the core to make the charcter -- pass in the info that was */
/* sent along.													   */
	if (theFM != NULL)
		tScaler->FMRenderGlyph(theFM, font_code, (void **)theScaler, 
							  char_code, xFracPenDelta, yFracPenDelta, 
							  greyScaleLevel, cmd, errCode );
    else
	T2K_RenderGlyph( tScaler, char_code, xFracPenDelta,yFracPenDelta,
		greyScaleLevel, cmd, errCode );


tScaler = *theScaler;
tempCache = (FF_CM_Class *)tScaler->theCache;

/* If all went well the core called the memory request callback -- this */
/* pointer to image data would have been filled if everything went OK   */

if (tempCache->imagedata == NULL)
    return NULL;
    

/* We have a character so we have to place associated info in the cache */
/* Add the character description to the current spot in the cache       */

curreq = (char_desc_t *)((char *)tempCache->current_char + sizeof(chardata_hdr));

curreq->charCode = (uint16)char_code;
curreq->fontCode = font_code;
curreq->instCode = inst_code;


/* Fill in all the other associated information in the cache header from the */
/* values that the core placed in the current scaler.					     */

tempCache->current_char->cacheSpecs.horizontalMetricsAreValid = tScaler->horizontalMetricsAreValid;
tempCache->current_char->cacheSpecs.xAdvanceWidth16Dot16 = tScaler->xAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.yAdvanceWidth16Dot16 = tScaler->yAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.xLinearAdvanceWidth16Dot16 = tScaler->xLinearAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.yLinearAdvanceWidth16Dot16 = tScaler->yLinearAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.fTop26Dot6 = tScaler->fTop26Dot6;
tempCache->current_char->cacheSpecs.fLeft26Dot6 = tScaler->fLeft26Dot6;
tempCache->current_char->cacheSpecs.verticalMetricsAreValid = tScaler->verticalMetricsAreValid;
tempCache->current_char->cacheSpecs.vert_xAdvanceWidth16Dot16 = tScaler->vert_xAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.vert_yAdvanceWidth16Dot16 = tScaler->vert_yAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.vert_xLinearAdvanceWidth16Dot16 = tScaler->vert_xLinearAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.yLinearAdvanceWidth16Dot16 = tScaler->yLinearAdvanceWidth16Dot16;
tempCache->current_char->cacheSpecs.vert_fTop26Dot6 = tScaler->vert_fTop26Dot6;
tempCache->current_char->cacheSpecs.vert_fLeft26Dot6 = tScaler->vert_fLeft26Dot6;
tempCache->current_char->cacheSpecs.width = tScaler->width;
tempCache->current_char->cacheSpecs.height = tScaler->height;
tempCache->current_char->cacheSpecs.rowBytes = tScaler->rowBytes;
tempCache->current_char->cacheSpecs.embeddedBitmapWasUsed = tScaler->embeddedBitmapWasUsed;

/*  So that we can correctly set the appropriate pointer in the scaler */
/*  set the longbit flag for later use.								   */

if (tScaler->baseAddr != NULL)
	tempCache->current_char->cacheSpecs.longbit = 0;
else tempCache->current_char->cacheSpecs.longbit = 1;



/* Insert new char at head of hash list */
CALCHASHIDX(curreq);
temp = tempCache->hashtable[hashidx];
tempCache->hashtable[hashidx] = tempCache->current_char;
tempCache->current_char->next = temp;
tempCache->current_char->prev = NULL;
if (temp != NULL)
    temp->prev = tempCache->current_char;
    
/* Insert new char at head of usage list */
temp = tempCache->lruhead;
if (temp != NULL)
    temp->lruprev = tempCache->current_char;
tempCache->lruhead = tempCache->current_char;
tempCache->current_char->lrunext = temp;
tempCache->current_char->lruprev = NULL;
if (tempCache->lrutail == NULL)
   tempCache->lrutail = tempCache->current_char;

#if CM_DEBUG
CmCheckCache(tempCache);
#endif

return tempCache->current_char;
}

/************************************************************** 
*
*    CmAllocMem
*
*        allocate memory from free list for new character with 
*		 specified size in bytes
*
***************************************************************/


static chardata_hdr *CmAllocMem(
    FF_CM_Class *pCmGlobals,
    long len)
{
memory_hdr *curseg;
chardata_hdr *temp;
unsigned long hashidx;

#if CM_DEBUG
CmCheckCache(pCmGlobals);
#endif

/* Round size up to alignment granularity */
SIZEALIGN(len);

/* Scan free list for a block that is big enough */
for (curseg = pCmGlobals->freelist;
	curseg != NULL;
	curseg = curseg->next)
	{
	if (curseg->len >= (len + (long)sizeof(memory_hdr)))
    	{
    	goto L1;
    	} 
	}

#if CM_DEBUG
   printf("CACHE::: Have to free up some characters!!\n");
#endif

/* Free least recently used blocks until a big enough block has been assembled */
while (pCmGlobals->lrutail != NULL)
	{
	temp = pCmGlobals->lrutail;

	/* Remove the freed character from its character list */
    if (temp->prev == NULL)
	    {
	    CALCHASHIDX(((char_desc_t *)((unsigned char*)temp + sizeof(chardata_hdr)))) ;
	    pCmGlobals->hashtable[hashidx] = temp->next;
	    }
	else
	    {
	    temp->prev->next = temp->next;
	    }
	if (temp->next != NULL)
	    {
	    temp->next->prev = temp->prev;
	    }

	/* Remove the discarded character from the usage list */
	pCmGlobals->lrutail = temp->lruprev;
	if (temp->lruprev == NULL)
		{
	    pCmGlobals->lruhead = NULL;
	    }
	else
		{
		pCmGlobals->lrutail->lrunext = NULL;
		}

	/* Link the free block into the free list */
	curseg = CmFreeMem(pCmGlobals, temp);

	/* Test if new free memory block is large enough */
	if (curseg->len >= (len + (long)sizeof(memory_hdr)))
		{
       	goto L1;
       	} 
	}

return NULL;

L1:	/* Allocate space from free block */
temp = (chardata_hdr *)((char *)curseg + curseg->len - len);
temp->len = len;
curseg->len -= len;

pCmGlobals->current_char = temp;

return temp;
}

/************************************************************** 
*
*    CmFreeMem
*
*        free unused memory.
*		 Retuns pointer to free block after it is combined with
*		 any adjacent free blocks.
*		 
*
***************************************************************/


static memory_hdr *CmFreeMem(
    FF_CM_Class *pCmGlobals,
    chardata_hdr * p)
{
memory_hdr *curseg;
memory_hdr *freemem, *prev, *next; 

/* Find interval in the free list to insert freed memory segment */
freemem = (memory_hdr *)p;
for (prev = NULL, next = pCmGlobals->freelist;
	next != NULL;
    prev = next, next = next->next)
    {
	if (freemem < next)
		{
		break;
		}
	}

/* Combine freed memory with adjacent free memory if possible */
if ((prev != NULL) &&
	((memory_hdr *)((char *)prev + prev->len) == freemem))
	{
	if ((next != NULL) &&
		((memory_hdr *)((char *)freemem + freemem->len) == next))
		{
		/* Combine freed memory with both previous and next blocks */
		prev->len += freemem->len + next->len;
		prev->next = next->next;
		if (next->next != NULL)
			next->next->prev = prev;
	    }
	else
		{
		/* Combine freed memory with previous block */
		prev->len += freemem->len;
		}
	curseg = prev;
	}
else
	{
	if (prev == NULL)
		{
		pCmGlobals->freelist = freemem;
		}
	else
		{
		prev->next = freemem;
		}

	if ((next != NULL) &&
		((memory_hdr *)((char *)freemem + freemem->len) == next))
		{
		/* Combine freed memory with next block */
	    freemem->len += next->len;
	    freemem->prev = next->prev;
	    freemem->next = next->next;
	    if (next->next != NULL)
	    	next->next->prev = freemem;
	    }
	else
		{
		/* Insert freed memory as separate free memory block */
		freemem->prev = prev;
		freemem->next = next;
		if (next != NULL)
			next->prev = freemem;
		}
	curseg = freemem;
	}
	
#if CM_DEBUG
CmCheckCache(pCmGlobals);
#endif

return curseg;
}

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/CacheManager/rcs/cachemgr.c 1.24 2000/05/31 15:38:26 reggers release $
 *                                                                           *
 *     $Log: cachemgr.c $
 *     Revision 1.24  2000/05/31 15:38:26  reggers
 *     Removed unintentional trigraph in a comment
 *     Revision 1.23  2000/05/30 20:54:45  reggers
 *     Testing changes for PALM.
 *     Set internal_baseAddr appropriately ALWAYS.
 *     Revision 1.22  1999/10/18 20:26:46  shawn
 *     Replaced a BTSBOOL declaration with int.
 *     
 *     Revision 1.21  1999/10/18 16:43:55  mdewsnap
 *     Changed include files to lower case.
 *     Revision 1.20  1999/09/30 13:15:59  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.19  1999/07/30 19:43:51  mdewsnap
 *     Added errCode to CmMakeChar -- remover error check for
 *     no glyph.
 *     Revision 1.18  1999/07/23 17:51:54  sampo
 *     More warning cleanup.
 *     Revision 1.17  1999/07/19 19:40:39  sampo
 *     Error/warning cleanup
 *     Revision 1.16  1999/07/13 21:26:21  sampo
 *     Corrected errCode checking in the New function.
 *     Revision 1.15  1999/07/09 21:02:08  sampo
 *     Corrected the filter spec for FF_CM_SetfilterTag
 *     Revision 1.14  1999/07/08 21:11:02  sampo
 *     Cleanup. Added error checking to FF_CM_New(). Made internal
 *     functions into statics. Renamed FF_CM_Destroy to FF_CM_Delete.
 *     Revision 1.13  1999/07/08 16:11:33  sampo
 *     Added header and footer. Cleanup up errant #ifdef's -> #if's
 *     Allow cachmgr failure to get memory, but T2K success to NOT flag
 *     an error.
 *                                                                           *
 *     AUTHOR: Mike Dewsnap                                                 *
******************************************************************************/
/* EOF cachemgr.c */
