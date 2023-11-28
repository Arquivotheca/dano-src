//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: SFModTbl.CPP
//
// Author: Mike Guzewicz
//
// Description: Data structure containing a modulator routing image for 
//              a single sample 
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// MikeGuz             Jun ??, 1996  Initial development.
//
//*****************************************************************************


/*

Theory of operation:
 
Each sample zone which has a unique modulation signature is associated with
one of these tables.

The table is a multiply-linked list of modulator instance structures. A modulator 
instance is defined as the minimum amount of data necessary to represent a single 
source influencing a single destination. This implies that SF2 modulators, which 
use two sources influencing a single destination, become two modulator instances 
in this model.

A modulator instance structure also contains redundant information to facilitate
realtime navigating of the table.

Modulator navigating takes on two forms... 

note-on navigation, where for every given destination you need to know 
what sources are influencing that destination

real-time navigation, where for a given real-time source you need to
know what destinations are being influenced.

Each modulator instance has information about itself, and 4 pointers
to other modulator instance. One pointer tells you where the next modulator
instance sharing the same source AND destination lives. The second pointer tells
you where the modulator instance sharing the same source but DIFFERENT destination
lives. The third pointer tells you where the modulator instance sharing the same
destination but DIFFERENT source lives. The fourth pointer is a dotted-line which
tells you where the secondary source that is tied to a given modulator exists.

note-on navigation uses the third pointer primarily, then the first and fourth if
appropriate.

real-time navigation uses the second pointer primarilay, then the first if
appropriate.

An array of 'first destinations' points to the routing structure of the 
first source which is actually influencing a given destination. That
routing structure in turn points to the routing structure of the 
next source which is influencing a given destination. And so forth.
This is called a 'destination tie'

Similarly, an array of 'first sources' points to the routing structure 
of the first destination which is actually influenced by that source. That
routing structure in turn points to the routing structure of the 
next destination which is being influencing by a given source. And so forth.
This is called a 'source tie'

Note that before source ties and destination ties are navigated,
the list of routing structures for a single given source and destination
must be processed.


                 Destinations

                         First
						   |
						   V
                      ___________
  S		First -----> | Modulator |------>
  o                  |  Instance |  Next mod with
  u                  |           |__ common source
  r                  |           |  |
  c                  |___________|  |
  e                   | | Next mod  |
  s                   | | common src|
                      | | and dest  |.......\ Secondary 
					  | |___________|       /  source mod
					  |
					  V 
			    Next mod with
				common destination


Public Functions:

It is not expected that the client of this table understand any 
of that. Only the maintainer of this code module needs to understand
the details here.

Functions are provided to create and destroy these instances of routing
structures, making or breaking the ties as it happens. 

Functions are provided to facilitate the nuances of the navigation of 
the structures at optimal speeds.

Functions are provided to create the 'default' modulator table.

The client then has the responsibility to create the effect which 
the modulator table dictates.

*/
#ifdef _DEBUG
#  ifndef DEBUG
#    define DEBUG
#  endif
#endif

#include <string.h>
#include "sfmodtbl.h"
#include "se8210.h"

#ifdef DEBUG 
#  include "dbg8210.h"
#else
#  define ASSERT
#endif

#ifdef DEBUG_SFMODTBL
#  include <stdio.h>
#  define LOG_OPEN(a)
#else
#  define LOG_OPEN(a)
#endif

#define MODULENAME "Emu82x0 Modulator Table"

// Memory conservation, only use the EMU80x0 parameters
// which map to real synth params.
#define SFMT_LAST_VOICE_VARIABLE sepCurrentPitch

//********************************************************************
// This class creates and deletes the static default modulator table
// member of the ModulatorTable Class
//********************************************************************
class MakeDefaultModulatorTable
{
  public:
    MakeDefaultModulatorTable(void)
    {
      ModulatorTable *cur = ModulatorTable::def = new ModulatorTable;
      if (cur != NULL)
      {
        cur->SetDefaultModulators();
      }
    }  

    ~MakeDefaultModulatorTable(void)
    {
      ModulatorTable *cur = ModulatorTable::def;
      if (cur != NULL)
      {
        delete cur;
        ModulatorTable::def = NULL;
      }
    }  
};

//********************************************************************
// This static variable causes the above class to be instantiated
// at initialization time and destroyed at destruction time.
//********************************************************************
//
// Mike, uncomment this (and remove this comment) when you need it!
//
ModulatorTable *ModulatorTable::def;
MakeDefaultModulatorTable dmt;

#ifdef DEBUG
DWORD ModulatorTable::mtCount=0;
#endif

/******************************************************************************
*
* Function: ModulatorTable::ModulatorTable
*
* Description: Constructor (setup)
*
*****************************************************************************
*/
ModulatorTable::ModulatorTable(void) : OmegaClass(MODULENAME)
{
  
	ClearError();
	
	WORD count;
	
    dwRefCount = 1;

	searchInstance=NULL;
	searchSource = ssEndSupportedSources;
	searchDest   = sepLastVoiceParam;
	
	destModulated = new BOOL[SFMT_LAST_VOICE_VARIABLE];
	if (destModulated==NULL)
	{
		SetError(oscErrorStatus, SF_MEMORYERROR);
		return;
	}
	
	pFirstInstanceThisDest = new modInstance*[SFMT_LAST_VOICE_VARIABLE];
	if (pFirstInstanceThisDest==NULL)
	{
		SetError(oscErrorStatus, SF_MEMORYERROR);
		return;
	}
	memset(pFirstInstanceThisDest, 0, sizeof(modInstance *)*SFMT_LAST_VOICE_VARIABLE);
	
	pFirstInstanceThisSource = new modInstance*[ssEndSupportedSources];
	if (pFirstInstanceThisSource==NULL)
	{
		SetError(oscErrorStatus, SF_MEMORYERROR);
		return;
	}
	memset(pFirstInstanceThisSource, 0, sizeof(modInstance *)*ssEndSupportedSources);
	
	for (count=0; count<ssEndSupportedSources; count++)
	{
		srcModulating[count] = FALSE;
	}
	
	for (count=0; count<SFMT_LAST_VOICE_VARIABLE; count++)
	{
		destModulated[count] = FALSE;
	}
	
	LOG_OPEN(this);
#ifdef DEBUG
	mtCount++;
#endif

}

/******************************************************************************
*
* Function: ModulatorTable::~ModulatorTable
*
* Description: Destructor 
*
*****************************************************************************
*/
ModulatorTable::~ModulatorTable(void)
{
  ZapAllModulators();

  //WORD count;

  if (destModulated != NULL)
	  delete [] destModulated;
  if (pFirstInstanceThisDest != NULL)
	  delete [] pFirstInstanceThisDest;
  if (pFirstInstanceThisSource != NULL)
	  delete [] pFirstInstanceThisSource;

  pFirstInstanceThisDest=NULL;
  pFirstInstanceThisSource=NULL;
#ifdef DEBUG
  mtCount--;
#endif

}

void ModulatorTable::
AddRef(void)
{
    dwRefCount++;
}

DWORD ModulatorTable::
DeleteRef(void)
{
    if (dwRefCount > 0)
        dwRefCount--;
    return dwRefCount;
}

DWORD GetMaxSupportedDestinations(void) {return SFMT_LAST_VOICE_VARIABLE;}

/******************************************************************************
*
* Function: ModulatorTable::NewModulator
*
* Description: Create a new routing, detect whether it should be an
*              new routing instance or an addition to a previous 
*              routing instance, and tie the new instance with the
*              rest of the table.
*
*****************************************************************************
*/
modInstance * ModulatorTable::
NewModulator(enSupportedSources src,
       enSupportedSourceType srcType,
       WORD dest, 
       LONG amount,
       enSupportedTransforms xform,
       enSupportedSources asrc,
       enSupportedSourceType asrcType,
       BOOL addTo)
{
  SEVOICEPARAM vvDest = (SEVOICEPARAM)dest;
  LONG addAmount;
  modInstance *currInstance;
  modInstance *aInstance = NULL;
  ClearError();

  if (vvDest >= SFMT_LAST_VOICE_VARIABLE)         return NULL;
  if (xform >= stEndSupportedTransforms) return NULL;
  
  // Not handling DC as a realtime modulator
  if ((src >= ssEndSupportedSources) && (asrc >= ssEndSupportedSources)) return NULL;

  // If we get an amount source without a source, this is exactly equivilant to a
  // source without an amount source. The code is geared to either 2 sources or a
  // source without an amount source. So, switch 'em.
  else if ((src >= ssEndSupportedSources) && (asrc < ssEndSupportedSources))
  {
	  src=asrc;
	  asrc=ssEndSupportedSources;
  }

  // Test.
  //if (vvDest==sepInitialVolume)
//	  amount=(amount*10)/4;

  // If there already exists a modulator of this type in this table,
  // return that one. This call REPLACES the previous modulator!
  currInstance = GetIdenticalInstance(src, srcType, vvDest, xform, asrc, asrcType);
  if (currInstance == NULL)
  {
    // If you are 'setting' a modulator with amount 0, and if the mod
    // does not yet exist, don't make it exist at all. This conserves
    // memory. NOTE: if you change  this line, you will break other
    // memory conservation code below! Understand the whole function
    // before manipulating!!!
    if ((addTo == FALSE) && (amount == 0))
    {
      return NULL;
    }
    // Otherwise, create one here.
    currInstance = _NewInstance(src, vvDest);
    if (currInstance == NULL)
    {
      return NULL;
    }
  }
  else
  {
    if (addTo == TRUE) 
    {
      // If you are 'adding' a modulator with amount 0 to an already
      // existing modulator, do nothing else. This conserves memory.
      if (amount == 0)
      {
        return currInstance;
      }

      // If you are 'adding' a modulator with an amount equal to the 
      // negative of the existing modulator, you will be modifying 
      // the existing modulator to have an amount 0. So might as well 
      // Zap it instead. This conserves memory.
      else if (amount == -(currInstance->amount))
      {
        ZapModulator(src, vvDest, currInstance);
        return NULL;
      }
    }
    // Here we have an identical modulator, whose amount is affected by
    // this call. So set its amount source instance here.
    aInstance = currInstance->pAsrcInstance;
  }

  if (addTo == FALSE)
  {
    // If you are 'setting' a modulator with amount 0, and that
    // modulator already exists, zap it here because we don't need it
    // anymore. We know the modulator existed previously because this
    // section of the code would not have been executed otherwise. See
    // above. This conserves memory.
    if (amount == 0)
    {
      ZapModulator(src, vvDest, currInstance);
      return NULL;
    }
    addAmount = 0;
  }

  else
  {
    addAmount = currInstance->amount;
  }

  currInstance->srcType = srcType;
  currInstance->amount = amount + addAmount;
  currInstance->asrc = asrc;
  currInstance->asrcType = asrcType;
  currInstance->xform = xform;
  currInstance->pAsrcInstance = aInstance;  

  if (asrc == ssEndSupportedSources)
    return currInstance;

  else if (aInstance != NULL)
  { 
    return currInstance;
  }

  // In this case, we KNOW we have a brand new amount source instance 
  // with an amount which is affected by this call. Otherwise the
  // code above would have trapped this condition.

  // So all circumvention codeing regarding identical
  // modulators and zero'ed modulators are not used.

  aInstance = _NewInstance(asrc, vvDest, TRUE);

  if (aInstance == NULL)
  {
    // Allocation error... wipe out that modulator!
    if (addTo == FALSE)
      ZapModulator(src, vvDest, currInstance);
    else
    {
      currInstance->amount -= amount;
      if (currInstance->amount == 0L)
		ZapModulator(src, vvDest, currInstance);
    }
    SetError(oscFatalStatus, SF_MEMORYERROR);
    return NULL;
  }

  aInstance->srcType = asrcType;
  aInstance->amount = amount + addAmount;
  aInstance->asrc = src;
  aInstance->asrcType = srcType;
  aInstance->xform = xform;
  aInstance->pAsrcInstance = currInstance;  

  // MG - currInstance->isASrc was NOT set properly
  // above because the aInstance was created here. So, set that
  // variable now.
  currInstance->pAsrcInstance=aInstance;

  ASSERT((DWORD)currInstance->pNextInstanceThisSourceAndDest!=0xdddddddd);
  ASSERT((DWORD)aInstance->pNextInstanceThisSourceAndDest!=0xdddddddd);

  return currInstance;
  
}

/******************************************************************************
*
* Function: ModulatorTable::GetInstance
*
* Description: Given a source and a destination, return the instance list
*
*****************************************************************************
*/
inline modInstance * ModulatorTable::
GetInstance(enSupportedSources src, SEVOICEPARAM dest)
{
  if ((src >= ssEndSupportedSources) || (dest >= SFMT_LAST_VOICE_VARIABLE))
    return NULL;

  // Note moving through the source navigation paths because those are always
  // connected.
  modInstance *i = pFirstInstanceThisSource[src];
  while (i!=NULL)
  {
	if (i->dest==dest)
		break;
	if (i->src>src) 
		return NULL;
	i=i->pNextInstanceThisSource;
  }

  return i;
     
}

/******************************************************************************
*
* Function: ModulatorTable::GetSurroundingInstance
*
* Description: Given a source and a destination, return the instances that 
*              do or would exist previous to the combination. If any is NULL,
*			   that means combination woudl be the head. 
*
*****************************************************************************
*/
inline void ModulatorTable::
_GetPreviousInstances(enSupportedSources src, SEVOICEPARAM dest, modInstance **pFromSource,
						 modInstance **pFromDest)
{
	*pFromDest=NULL;
	*pFromSource=NULL; 

	if ((pFirstInstanceThisDest[dest]==NULL)||(pFirstInstanceThisDest[dest]->src==src))
		*pFromDest=NULL;
	else
	{
		*pFromDest = pFirstInstanceThisDest[dest];
		while (*pFromDest!=NULL)  
		{ 
			if ((*pFromDest)->pNextInstanceThisDest == NULL)
				break;
			if ((*pFromDest)->pNextInstanceThisDest->src >= src) 
				break; 
			*pFromDest=(*pFromDest)->pNextInstanceThisDest;
		}
	}
	
	if ((pFirstInstanceThisSource[src]==NULL)||pFirstInstanceThisSource[src]->dest==dest)
		*pFromSource=NULL;
	else
	{
		*pFromSource = pFirstInstanceThisSource[src];
		while (*pFromSource!=NULL)  
		{ 
			if ((*pFromSource)->pNextInstanceThisSource == NULL)
				break;
			if ((*pFromSource)->pNextInstanceThisSource->dest >= dest) 
				break; 
			*pFromSource=(*pFromSource)->pNextInstanceThisSource;
		}
	}
	
	return;
     
}


/******************************************************************************
*
* Function: ModulatorTable::SetInstance
*
* Description: Given a source and a destination, set the instance to the
*              given list.
*
*****************************************************************************
*/
/*inline void ModulatorTable::
SetInstance(enSupportedSources src, SEVOICEPARAM dest, modInstance *i)
{
  if ((src >= ssEndSupportedSources) || (dest >= sepLastVoiceParam))
    return;
  else
    instance[src][dest] = i;
}*/

/******************************************************************************
*
* Function: ModulatorTable::GetIdenticalInstance
*
* Description: Given a routing description, determine if there is already
*              an identical routing description in the list and if so
*              return a pointer to the instance of that routing.
*
*****************************************************************************
*/
modInstance * ModulatorTable::
GetIdenticalInstance(enSupportedSources src, enSupportedSourceType srcType,
		     SEVOICEPARAM dest, enSupportedTransforms xform,
		     enSupportedSources asrc, enSupportedSourceType asrcType)
{
  modInstance *instance;
  instance = GetInstance(src, dest);

  while (instance != NULL)
  {
	  // Explanation: If the src is 'no source', then the source type is irrelevant
	  // you still have an identical modulator, even if the source types don't match. 
	  // Same for the 'asrc'
    if (((src==ssEndSupportedSources)||(instance->srcType  == srcType))  &&
	     (instance->xform    == xform)    &&
	     (instance->asrc     == asrc)     &&
	    ((asrc==ssEndSupportedSources)||(instance->asrcType == asrcType)))
      break;
    instance=instance->pNextInstanceThisSourceAndDest;
  }
  return instance;
}

/******************************************************************************
*
* Function: ModulatorTable::NewInstance
*
* Description: Given a source/destination slot, create a new routing
*              instance and connect it to any/all common src and dest instances
*
*****************************************************************************
*/
modInstance * ModulatorTable::
_NewInstance(enSupportedSources src, SEVOICEPARAM dest, BOOL isASrc)
{
  modInstance *newInstance;

  if ((newInstance = new modInstance) == NULL)
  {
    SetError(oscFatalStatus, SF_MEMORYERROR);
    return NULL;
  }

  destModulated[dest] = TRUE;
  srcModulating[src]  = TRUE;

  memset(newInstance, 0, sizeof(modInstance));
  newInstance->src=src;
  newInstance->dest=dest;
  newInstance->isASrc=isASrc;
  newInstance->srcType = syEndSupportedSourceTypes;
  newInstance->xform = stEndSupportedTransforms;
  newInstance->asrc = ssEndSupportedSources;
  newInstance->asrcType = syEndSupportedSourceTypes;
  

  modInstance *currInstance = GetInstance(src, dest);
  
  // This means nothing exists at this slot. Therefore the tie to other slots
  // from the source and destination side need to happen here.
  if (currInstance==NULL) 
  {
	  modInstance *pFromDest, *pFromSource;
	  _GetPreviousInstances(src, dest, &pFromSource, &pFromDest);
	  
	  // NOTE: do NOT connect common destinations for amount sources!
	  if (isASrc==FALSE)
	  {
		  if (pFromDest==NULL)
		  { 
		      newInstance->pNextInstanceThisDest = pFirstInstanceThisDest[dest];
			  pFirstInstanceThisDest[dest]       = newInstance;
		  }
		  else
		  {
			  newInstance->pNextInstanceThisDest = pFromDest->pNextInstanceThisDest;
			  pFromDest  ->pNextInstanceThisDest = newInstance;
		  }
	  }

	  if (pFromSource==NULL)
	  { 
		  newInstance->pNextInstanceThisSource = pFirstInstanceThisSource[src];
		  pFirstInstanceThisSource[src]        = newInstance;
	  }
	  else
	  {
		  newInstance->pNextInstanceThisSource = pFromSource->pNextInstanceThisSource;
		  pFromSource->pNextInstanceThisSource = newInstance;
	  }
	  return newInstance;
  }
				
  // This means a modulator instance already exists at this src/dest combination.
  // Tie the new one to the existing one.
  modInstance *lastInst = NULL; // MG change this from NULL

  // Sources go to the front of the list.
  if (isASrc == FALSE)
  {
    while (currInstance != NULL)
    {
      if (currInstance->isASrc == TRUE)
		break;
      lastInst = currInstance;
      currInstance = currInstance->pNextInstanceThisSourceAndDest;
    }

    if (lastInst != NULL)
    {
      lastInst->pNextInstanceThisSourceAndDest = newInstance;
      newInstance->pNextInstanceThisSourceAndDest = currInstance;
    }
	else
	{
		// MG If original list only has ASRC's in it, make this entry the first
		newInstance->pNextInstanceThisSourceAndDest=currInstance;  

		// Now, connect 
		modInstance *pFromDest, *pFromSource;
		_GetPreviousInstances(src, dest, &pFromSource, &pFromDest);
		
		
		if (pFromDest==NULL)
		{ 
			newInstance->pNextInstanceThisDest = pFirstInstanceThisDest[dest];
			pFirstInstanceThisDest[dest]       = newInstance;
		}
		else
		{
			newInstance->pNextInstanceThisDest = pFromDest->pNextInstanceThisDest;
			pFromDest  ->pNextInstanceThisDest = newInstance;
		}
		
		
		if (pFromSource==NULL)
		{ 
			newInstance->pNextInstanceThisSource = pFirstInstanceThisSource[src];
			pFirstInstanceThisSource[src]        = newInstance;
		}
		else
		{
			newInstance->pNextInstanceThisSource = pFromSource->pNextInstanceThisSource;
			pFromSource->pNextInstanceThisSource = newInstance;
		}
	}
  }

  // ASources go to the back of the list.
  else
  {

    while (currInstance != NULL)
    {
      lastInst = currInstance;
      currInstance = currInstance->pNextInstanceThisSourceAndDest;
    }

    if (lastInst != NULL)
      lastInst->pNextInstanceThisSourceAndDest = newInstance; 
  }

  return newInstance;
}
 

/****************************************************************************** * 
* Function: ModulatorTable::FindSourceGivenASource
* 
* Description: Destination ties do not account for ASrc settings, since
*              that would cause a modulation to happen twice. So... this
*              function tells you what the ASrc was when navigating 
*              destination ties.
* 
***************************************************************************** 
*/ 
modInstance *ModulatorTable::
_FindSourceGivenASource(enSupportedSources, SEVOICEPARAM, modInstance *inst)
{
  if (inst) return inst->pAsrcInstance;
  return NULL;
}

/****************************************************************************** *                               
* Function: ModulatorTable::FindASourceGivenSource
*  
* Description: Destination ties do not account for ASrc settings, since
*              that would cause a modulation to happen twice. So... this 
*              function tells you what the Source was when navigating 
*              destination ties.
*  
*****************************************************************************  
*/  
modInstance *ModulatorTable::
_FindASourceGivenSource(enSupportedSources, SEVOICEPARAM dest, modInstance *inst)
{

  modInstance *s = GetInstance(inst->asrc, dest);
  while (s != NULL)
  {
    if (s->pAsrcInstance == inst)
      break;
    s=s->pNextInstanceThisSourceAndDest;
  }
  return s;
}

/****************************************************************************** *                               
* Function: ModulatorTable::FindASourceInstance
*  
* Description: Destination ties do not account for ASrc settings, since
*              that would cause a modulation to happen twice. So... this 
*              function tells you what the ASrc was when navigating 
*              destination ties.
*  
*****************************************************************************  
*/  
modInstance *ModulatorTable ::
_FindASourceInstance(enSupportedSources src, SEVOICEPARAM dest, modInstance *inst)
{
  if ((inst == NULL) ||
      ((inst->pAsrcInstance == NULL) && (inst->asrc == ssEndSupportedSources)))
    return NULL;

  if (inst->isASrc == TRUE)
    return _FindSourceGivenASource(src, dest, inst);
  else
    return _FindASourceGivenSource(src, dest, inst);
}

/****************************************************************************** *                               
* Function: ModulatorTable::ZapModInstance
*  
* Description: Free the memory used to store a Mod Instance, restoring
*              all ties when doing so.
*
* Notes: DON'T CALL THIS FUNCTION unless you REALLY know what you are doing!
*        It exists to simplify the code in ZapModulator().
*  
*****************************************************************************  
*/  
void ModulatorTable::
_ZapModInstance(enSupportedSources src, SEVOICEPARAM dest, modInstance *inst)
{

  if ((inst == NULL) || (src >= ssEndSupportedSources) || (dest >= SFMT_LAST_VOICE_VARIABLE))
    return;

  modInstance *firstInst = GetInstance(src, dest);

  if (firstInst==NULL) return;

  modInstance *pPrevSource;
  modInstance *pPrevDest;
  _GetPreviousInstances(src, dest, &pPrevSource, &pPrevDest);

  if (inst == firstInst)
  {
	  // In this case, the modulator to be zapped is the first in a src/dest list
	  if (inst->pNextInstanceThisSourceAndDest == NULL)
	  {
		  // In this case, this is only one instance in the src/dest list

		  // - Here, taking care of the common destination traversal path

		  if (pPrevDest==NULL)
		  {
			  // In this case, the current instance is either an amount source or
			  // is the first in the common destination path.

			  // MG old code below, I don't think it is necessary

			  //if (inst->isASrc==TRUE) 
			  //{
				  // OK, it is an amount source.

				  // In this case, where the current is an amount source AND the head points
			      // nowhere, there are no further destinations in this dest list
			//	  if (pFirstInstanceThisDest[dest]==NULL)
			//		{
					    // Actually, this should never happen.
			//		    ASSERT(0);
			//			destModulated[dest]=FALSE;
			//		}
			//  }

			  
			  //else 
			  // If the current instance is an amount soruce, there is nothing more to do here. Go to the
			  // source chain

			  // MG end old code

			  if (inst->isASrc==FALSE)
			  {
				  // OK, it is the first in the common destination path.
				  pFirstInstanceThisDest[dest]=inst->pNextInstanceThisDest;
				  if (inst->pNextInstanceThisDest==NULL)
					{
					// In this case, where the current is NOT an amount source AND the next
					// destination is nowhere, there are no further destinations in this dest
					// list.
						destModulated[dest]=FALSE;
					}
			  }
		  }
 
		  else 
		  { 
			  // In this case, this is somewhere within the common destination chain.
			  // But, if the current instance is an ASRC, then the destination chain will not be
			  // hooked up to us. So, ignore 
			  if (inst->isASrc==FALSE)
			  {
				  pPrevDest->pNextInstanceThisDest=inst->pNextInstanceThisDest; 
			  }
			  else
			  {
				  // Just making sure this never happens, where dest chain is hooked to an amount source
				  ASSERT(pPrevDest->pNextInstanceThisDest!=inst);
				  ;
			  } 

		  }
		  
		  // - Here, taking care of the common source traversal path. 
		  
		  // No amount source variable here, so it is MUCH easier.

		  if (pPrevSource==NULL)
		  {
			  pFirstInstanceThisSource[src]=inst->pNextInstanceThisSource;
			  if (inst->pNextInstanceThisSource==NULL)
			  {
					srcModulating[src]=FALSE;
			  }
		  }

		  else
			  pPrevSource->pNextInstanceThisSource=inst->pNextInstanceThisSource;
		  
		  // MG Never disconnect source from dest chain if amount source
		  // MG legacy code, not sure yet if it applies here...
		  //if (inst->isASrc==FALSE)
		  //	_DisconnectSourceCommonDest(src, dest);
	  }
	  else
	  { 
	      // In this case, the modulator to be zapped is the first in a src/dest list.
		  // We therefore have to get the next guy down in the src/dest list to take over
		  // the common source and common destination chain.

		  // First handle the common source chain.

		  // If the previous instance is NULL, that must mean the the first instance
		  // with this source is a pointer to the current instance. Have that point
		  // to the next instance on the src/dest chain
		  if (pPrevSource==NULL)
		  {
			  ASSERT(pFirstInstanceThisSource[src]==inst);
			  pFirstInstanceThisSource[src]=inst->pNextInstanceThisSourceAndDest;
		  }

		  // Otherwise, have the previous point to the next guy on the src/dest chain.
		  else
			  pPrevSource->pNextInstanceThisSource=inst->pNextInstanceThisSourceAndDest;

		  // Now, there is a new head of the src/dest combination, so it has to take over the
		  // role of pointing to the common dest as well as common source tree.
		
		  inst->pNextInstanceThisSourceAndDest->pNextInstanceThisSource = inst->pNextInstanceThisSource;

		  // OK, now, if the next guy on the chain is an amount source, do NOT connect the 
		  // common destination chain. This is to avoid double-caluclation on note-on.
		  // Connect to the next instance with common src/dest, if that instance is not an amount source.
		  // Otherwise, just leave the previous instance pointing to what next destination it was.
		  if (inst->pNextInstanceThisSourceAndDest->isASrc==FALSE)
		  {		  
			  if (pPrevDest==NULL)
			  {
				  ASSERT(pFirstInstanceThisDest[dest]==inst);
				  pFirstInstanceThisDest[dest]=inst->pNextInstanceThisSourceAndDest; 
			  }

			  else
					pPrevDest->pNextInstanceThisDest=inst->pNextInstanceThisSourceAndDest;
		  }

		  // 
		  // Now, there is a new head of the src/dest combination, so it has to take over the
		  // role of pointing to the common dest as well as common source/dest tree.
		  // Connect up the destination chain to the next modulator with common src/dest IF
		  // that modulator is not an amount source. Otherwise, connect up the destination chain
		  // to the next modulator with common dest. 
		  else  
		  {  

		  	if (pPrevDest==NULL) 
			{
				// We know 'inst' is the first in a src/dest chain.
				// We know there is a next 'inst' in the src/dest chain.
				// We know that the NEXT inst in the src/dest chain IS an 
				// amount source.

				if (inst->isASrc==FALSE)
				{
					// We know that the instance to be zapped is NOT amount source.
 
					// Therefore the head had better NOT be null. 
					ASSERT(pFirstInstanceThisDest[dest]!=NULL);

					// In fact, if the head is not NULL, but the previous is NULL, then the head 
					// had better be the current instance.
					ASSERT(pFirstInstanceThisDest[dest]==inst);

					// In this case, set the destination chain to the next common destination,
					// NOT the next common src/dest pair!
					pFirstInstanceThisDest[dest]=inst->pNextInstanceThisDest;
				}

				// If the current is an amount source, we need do nothing in this chain.
 
			}
		  	else
			{
				// If the next common src/dest instance of the PREVIOUS destinatoin is an amount 
				// source, connect up to the current instance's common DEST chain. Otherwise, 
				// connect up to the current instance's common src/dest chain.

				// We know the next instance in the 'inst' src/dest chain IS an amount source.
				// Therefore, connect common destination only!
				//if (inst->pNextInstanceThisSourceAndDest->isASrc==TRUE)
				if (inst->isASrc==FALSE)
					pPrevDest->pNextInstanceThisDest=inst->pNextInstanceThisDest;
				//else
		  		//  pPrevDest->pNextInstanceThisDest=inst->pNextInstanceThisSourceAndDest;
			}
			  
		   }
	  }
  }
  
  else
  {
	  // In this case, the instance is somewhere within a src/dest chain, and is NOT the head
	  // of the src/dest 
	  modInstance *lastInst = firstInst;
	  while (lastInst != NULL)
	  {
		  if (lastInst->pNextInstanceThisSourceAndDest == inst)
		  {
			  lastInst->pNextInstanceThisSourceAndDest = inst->pNextInstanceThisSourceAndDest;
			  break;
		  }
		  else 
			  lastInst = lastInst->pNextInstanceThisSourceAndDest;
	  }
  }

  // Here if this instance has an ASrc, then the ASrc which is pointing to 
  // this instance should be set to NULL before deleting the instance.
  if (inst->pAsrcInstance != NULL)
  {
	  inst->pAsrcInstance->pAsrcInstance = NULL;
	  inst->pAsrcInstance->asrc = ssEndSupportedSources;
  }
  delete inst;
}

/****************************************************************************** *                               
* Function: ModulatorTable::ZapModulator
*  
* Description: Free the memory used for all instances of a source modulating
*              a destination
*  
*****************************************************************************  
*/  
void ModulatorTable::
ZapModulator(enSupportedSources src, SEVOICEPARAM dest, modInstance *i)
{
  if (i==NULL) return;
  _ZapModInstance(i->asrc, dest, _FindASourceInstance(src, dest, i));
  _ZapModInstance(src, dest, i);
}

/****************************************************************************** *                               
* Function: ModulatorTable::ZapAllModulators
*  
* Description: Clean up the whole table.
*  
*****************************************************************************  
*/  
void ModulatorTable::
ZapAllModulators(void)
{
  WORD currSrc  = ssKeyNumber;
  WORD currDest = sepDelayModLFO;

  for (currDest = sepDelayModLFO; currDest < SFMT_LAST_VOICE_VARIABLE; currDest++)
  {
    for (currSrc = ssKeyNumber; currSrc < ssEndSupportedSources; currSrc++)
    {
      modInstance *killInstance;
      while ((killInstance =
	     GetInstance((enSupportedSources)currSrc, (SEVOICEPARAM)currDest))

		 != NULL)
      {
			ZapModulator((enSupportedSources)currSrc,
		     (SEVOICEPARAM)     currDest,
		     killInstance);
      }
    }
  }
}

/****************************************************************************** *                               
* Function: ModulatorTable::FirstModThisSource
*  
* Description: Public Navigation Function
*              Find the first modulator and destination which is influenced
*              by a given source. For realtime control navigation.
*  
*****************************************************************************  
*/  
modInstance * ModulatorTable::
FirstModThisSource(enSupportedSources src, SEVOICEPARAM& dest)
{ 
	searchInstance = pFirstInstanceThisSource[src];
	if (searchInstance==NULL)
		dest=sepLastVoiceParam;
	else
		dest=searchInstance->dest;
	return searchInstance;
}

/****************************************************************************** *                               
* Function: ModulatorTable::NextModThisSource
*  
* Description: Public Navigation Function
*              Find the next modulator and destination which is influenced
*              by a given source. For realtime control navigation.
*  
*****************************************************************************  
*/  
modInstance * ModulatorTable::
NextModThisSource(SEVOICEPARAM &dest, BOOL retInst)
{
  if (searchInstance == NULL) return NULL;

  if ((retInst == TRUE) && (searchInstance->pNextInstanceThisSourceAndDest != NULL))
    return searchInstance = searchInstance->pNextInstanceThisSourceAndDest;

  searchInstance = searchInstance->pNextInstanceThisSource;
  if (searchInstance!=NULL)
	  dest=searchInstance->dest;
  else
	  dest=sepLastVoiceParam;
  return searchInstance;

}

/****************************************************************************** *                               
* Function: ModulatorTable::NextModThisSource
*  
* Description: Public Navigation Function
*              Find the first modulator and source which influences
*              a given destination. For note-on navigation.
*  
*****************************************************************************  
*/  
modInstance * ModulatorTable::
FirstModThisDest(SEVOICEPARAM dest, enSupportedSources& src)
{
   searchInstance=pFirstInstanceThisDest[dest];
   if (searchInstance==NULL)
	   src=ssEndSupportedSources;
   else
		src=searchInstance->src;
   return searchInstance;
}

/******************************************************************************
*                               
* Function: ModulatorTable::NextModThisSource
*  
* Description: Public Navigation Function
*              Find the next modulator and source which influences
*              a given destination. For note-on navigation.
*
*****************************************************************************
*/
modInstance * ModulatorTable::
NextModThisDest(enSupportedSources &src, BOOL retInst)
{
  if (searchInstance == NULL) return NULL;

  if ((retInst == TRUE) && (searchInstance->pNextInstanceThisSourceAndDest != NULL))
    return searchInstance = searchInstance->pNextInstanceThisSourceAndDest;

  searchInstance = searchInstance->pNextInstanceThisDest;
  if (searchInstance==NULL)
	   src=ssEndSupportedSources;
   else
		src=searchInstance->src;
   return searchInstance;
}

#define NEWMODBREAKIFBAD(a,b,c,d,e,f,g) \
	NewModulator((a),(b),(c),(d),(e),(f),(g)); \
	if (IsBad()) {break;}

#define NEWMODBREAKIFBAD1(a,b,c,d,e,f,g,h,i) \
	NewModulator((a),(b),(c),(d),(e),(f),(g),(h),(i)); \
	if (IsBad()) {break;}

/******************************************************************************
*                               
* Function: ModulatorTable::SetDefaultModulators
*  
* Description: Public Loadtime Function
*              Take a modulator table and make it look like General MIDI
*              would have it.
*
*****************************************************************************
*/
void ModulatorTable::
SetDefaultModulators(void)
{
  ZapAllModulators();

  do
  {
	  // Scale tune 100 cents
	  NEWMODBREAKIFBAD(ssKeyNumber, syPositiveBipolar, sepInitialPitch, 6400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);

	  // Velocity to volume
	  NEWMODBREAKIFBAD1(ssKeyOnVelocity, syNegativeUnipolar, slConcave, sepInitialVolume, 960, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes, slLinear);
	  
	  // Velocity to filter cutoff
	  //NewModulator(ssKeyOnVelocity, syNegativeUnipolar, sepInitialFilterFc, -2400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  NEWMODBREAKIFBAD1(ssKeyOnVelocity, syNegativeUnipolar, slLinear, sepInitialFilterFc, -2400, stLinear, ssKeyOnVelocity, syNegativeUnipolar, slSwitch);
	  
	  // Pitch wheel
	  NEWMODBREAKIFBAD(ssPitchWheel, syPositiveBipolar, sepInitialPitch, 12700, stLinear, ssPitchBendSensitivity, syPositiveUnipolar);
	  
	  // Channel pressure
	  NEWMODBREAKIFBAD(ssChanPressure, syPositiveUnipolar, sepVibLFOToPitch, 50, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Mod wheel
	  NEWMODBREAKIFBAD(ssCC1, syPositiveUnipolar, sepVibLFOToPitch, 50, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Volume
	  NEWMODBREAKIFBAD1(ssCC7, syNegativeUnipolar, slConcave, sepInitialVolume, 960, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes, slLinear);
	  
	  // Pan (A, B=1-A)
	  NEWMODBREAKIFBAD(ssCC10, syPositiveBipolar, sepEffectsSendA, 500, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  NEWMODBREAKIFBAD(ssCC10, syPositiveBipolar, sepEffectsSendB, 500, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Expression
	  NEWMODBREAKIFBAD1(ssCC11, syNegativeUnipolar, slConcave, sepInitialVolume, 960, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes, slLinear);
	  
	  // Effects Send 1 (D)
	  NEWMODBREAKIFBAD(ssCC91, syPositiveUnipolar, sepEffectsSendD, 200, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Effects Send 2 (C)
	  NEWMODBREAKIFBAD(ssCC93, syPositiveUnipolar, sepEffectsSendC, 200, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Harmonic Content
	  // NEWMODBREAKIFBAD(ssCC71, syPositiveBipolar, sepInitialFilterQ, 220, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Release
	  // NEWMODBREAKIFBAD(ssCC72, syPositiveBipolar, sepReleaseModEnv, 14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  // NEWMODBREAKIFBAD(ssCC72, syPositiveBipolar, sepReleaseVolEnv, 14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Attack
	  // NEWMODBREAKIFBAD(ssCC73, syPositiveBipolar, sepAttackModEnv, 14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  // NEWMODBREAKIFBAD(ssCC73, syPositiveBipolar, sepAttackModEnv, 14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Brightness
	  // NEWMODBREAKIFBAD(ssCC74, syPositiveBipolar, sepInitialFilterFc, -14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	
	  return;

  } while(0);
 
  ZapAllModulators();

};

