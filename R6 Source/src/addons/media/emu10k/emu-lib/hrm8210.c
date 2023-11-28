
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* Filename: hrm8210.c
*
* Description: EMU8210 Hardware Resource Manager functions 
*
* Visible Routines: emuinit
*
* [Local Routines:] NONE
*
* History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  --------------------------------------- 
*  0.002		JK		 Sep 24, 97	  Changed to use the 8210 name scheme.
*  0.001        MG       Jun 02, 97   Initial version
*
******************************************************************************
*/

#ifndef __HRM8210_C
#define __HRM8210_C

/************
* Includes
************/

#include "dbg8210.h"
#include "hrm8210d.h"
#include "fxsystem.h"
#include "fxconfig.h"
#include "se8210d.h"
#include "sm8210d.h"
#include "mxr8210d.h"
#include "ip8210d.h"
#include "itm8210d.h"
#include "hrb8210d.h"
#include "hal8210.h"  /* EMU8010 Register specific stuff */ 
#include "emuerrs.h" 

#include <string.h>

#define MAX_TIMEOUT 0x1000
#define ALL_STATES 0xFFFFFFFF
#define RESET_BYTE 0xFF

#define FXCHIPREV "EMU8010_A"


/*********************
 * Private functions
 ********************/
 
typedef struct stHRMStateTag
{ 
	DWORD dwUserHWNum;
	HALID halid;  
	SEID  seid;
	SMID  smid;
	HRBID hrbid;
	IPID  ipid;
	MXRID mxrid;
    ITMID itmid;
	HRMFLAGS discFlags;
	HRMFLAGS mxrFlags;
	FXID    fxid;
    DWORD dwHWRevision;
	fxChipHandle fxch;
} stHRMState;
 
stHRMState  hrmStates[MAX_CHIPS]; 
BOOL bHRMInit=FALSE;

#define HRMIDQUALIFIER     0xB00B1E50
#define HRMIDQUALIFIERMASK 0xFFFFFFF0

#define HRM_MAKE_ID(a)    ((a) |  HRMIDQUALIFIER)
#define HRM_IS_ID(a)      (((a) & HRMIDQUALIFIERMASK)==HRMIDQUALIFIER)
#define HRM_MAKE_INDEX(a) ((a) & ~HRMIDQUALIFIER)

#define GETHRMSTATE(a) (stHRMState *)(((WORD)(a)>=MAX_CHIPS) ? NULL : &hrmStates[a])
#define GETACTIVEHRMSTATE(a) (stHRMState *)(((a)>=hrmCount) ? NULL : &hrmStates[a])

/*HRMFLAGS hrmpGetDiscoveredFlags(HALID id)
{
		BOOL bCheck=FALSE;
		HRMFLAGS hCheck=0;
		
		bCheck = halIsDiscovered(id, HRMDISC_SOUND_ENGINE);
		if (bCheck) hCheck |= HRMDISC_SOUND_ENGINE;

		bCheck = halIsDiscovered(id, HRMDISC_JOYSTICK);
		if (bCheck) hCheck |= HRMDISC_JOYSTICK;

		bCheck = halIsDiscovered(id, HRMDISC_HOST_MODEM);
		if (bCheck) hCheck |= HRMDISC_HOST_MODEM;

		return hCheck;
}*/

void hrmpResetAllSubIDs(HRMID id)
{
	WORD start, end, count;
	if (id==ALL_STATES)
	{
		start=0; 
		end=MAX_CHIPS;
	}
	else
	{
		start=(WORD)HRM_MAKE_INDEX(id);
		end=start+1;
	}

	for (count=start; count<end; count++)
	{
		stHRMState *state=&hrmStates[count];	
		state->seid=HRM_INVALID_ID;
		state->fxid=(FXID)HRM_INVALID_ID;
		state->smid=HRM_INVALID_ID;
		state->ipid=HRM_INVALID_ID;
		state->hrbid=HRM_INVALID_ID;
        state->itmid=HRM_INVALID_ID;
	}
}
	
void hrmpFreeHRMState(HRMID id)
{
	if (id==ALL_STATES)
	{
	  memset(hrmStates, RESET_BYTE, sizeof(hrmStates)); 
	}
	else if (!HRM_IS_ID(id))
		return;
	else
		memset(&hrmStates[HRM_MAKE_INDEX(id)], RESET_BYTE, sizeof(stHRMState)); 

	hrmpResetAllSubIDs(id);
}

stHRMState *hrmpGetHRMState(HALID id)
{
	if (!HRM_IS_ID(id)) return NULL; 
	return &hrmStates[HRM_MAKE_INDEX(id)];
}

stHRMState *hrmpGetActiveHRMStateHWID(DWORD dwHardwareID, HALID *id)
{

	WORD count;
	*id = 0xFFFFFFFF;
	for (count=0; count<MAX_CHIPS; count++)
	{
		if (hrmStates[count].dwUserHWNum == dwHardwareID)
		{
			*id = HRM_MAKE_ID(count);
			return &hrmStates[count];
		}
	} 
	return NULL;
}


stHRMState *hrmpGetNextAvailableHRMState(HRMID *id)
{
	WORD count;
	for (count=0; count<MAX_CHIPS; count++)
	{
		if (hrmStates[count].dwUserHWNum == 0xFFFFFFFF)
		{
			memset(&hrmStates[count], 0, sizeof(stHRMState)); 
			hrmStates[count].halid = HRM_INVALID_ID;
			*id = HRM_MAKE_ID(count);
			return &hrmStates[count];
		}
	}
	return NULL;
}

hrmpDiscoverSoundEngine(stHRMState *state, HRMCHIPCONFIG *conf)
{
	EMUSTAT stat;
	SECHIPCONFIG seconf;
	SMCONFIG smconf;
	stIPConfig ipconf; 
	HRBCONFIG hrbconf;

	state->seid=HRM_INVALID_ID;
	state->fxid=(FXID)HRM_INVALID_ID;
	state->smid=HRM_INVALID_ID;
	state->ipid=HRM_INVALID_ID;
	state->hrbid=HRM_INVALID_ID;

	do 
	{
		memset(&ipconf, 0, sizeof(ipconf));
		ipconf.opaque32BitHandler = conf->dwHardwareInterruptID;
		stat = ipDiscoverChip(state->halid, &ipconf, &state->ipid);
		if (stat != SUCCESS) break;

        /* Discover the Interval Timer Manager; must be done after
         * the IP manager.  */
        stat = itmDiscoverChip(state->halid, state->ipid, &state->itmid);
        if (stat != SUCCESS) break;

      /* SE must be initialized after IP - MRP 3/11/98*/
		memset(&seconf, 0, sizeof(seconf));
		seconf.bInitSoundEngine = conf->bInitSoundEngine;
        seconf.itmid = state->itmid;
        seconf.dwHWRevision = conf->dwHWRevision;
		stat = seDiscoverChip(state->halid, &seconf, &state->seid);
		if (stat != SUCCESS) break;
 
		memset(&smconf, 0, sizeof(smconf));
		smconf.smPageTable = conf->smPageTable;
		smconf.dwSampleMemorySize = conf->totalSystemPhysRam;
		stat = smDiscoverChip(state->halid, &smconf, &state->smid);
		if (stat != SUCCESS) break;

		state->fxch.halid = state->halid;
		state->fxch.ipid = state->ipid;
		stat = fxSystemDiscoverChip(FXCHIPREV, conf->dwHWRevision, conf->fxXTRAM.osPhysAddr, conf->fxXTRAM.dwSize/2, (ULONG)(&state->fxch), &state->fxid);
		if (stat != SUCCESS) break;

		/* Note that since this depends on the IP, it must be initialized after the IP is */
		hrbconf.hbSrcRecord = conf->hbSRCRecord;
		hrbconf.hbMicRecord = conf->hbMicRecord;
		hrbconf.hbFXRecord  = conf->hbFXRecord;
		stat = hrbDiscoverChip(state->halid, state->ipid, state->itmid, 
                               &hrbconf, &state->hrbid);
		if (stat != SUCCESS) break;

		/* Here's a hack.  Since MIDI isn't abstracted, and we
		 * need to initialize it, we'll just do it here and look
		 * the other way.  EL
		 *
                 * First write a 0xFF to the MUCMD register to guarantee
                 * that the hardware is in RESET mode.
                 *
                 * Next, write a 0x3f to the MUCMD register to set MIDI port
		 * from RESET to Enter_UART_Mode
		 */
                B8010SERegWrite(state->halid, MUCMD, 0xff);
		B8010SERegWrite(state->halid, MUCMD, 0x3f);
		/* Flush the MIDI in FIFO */
		{
			BYTE b;
			b = B8010SERegRead( state->halid, MUSTA );
			while( !(b & MUSTA_IRDYN) ) {
				B8010SERegRead(state->halid, MUDTA);
				b = B8010SERegRead( state->halid, MUSTA );
			}
		}

		/* Since these hacky, undefined tasks always fall on my lap, I am
		 * starting to look like an anarchist rebel.  I'm not.  I like order
		 * just as much as the next guy.  But, the joystick isn't abstracted
		 * either, and it needs a simple initialization.  So, once again,
		 * I'll just do it here.  I won't tell anybody if you don't.  EL
		 */
		/* Note 4/2/98 EL:  For Rev 3 board and up, we use the external
		 * joystick clock.
		 */
		L8010SERegWrite( state->halid, HC, 
					     L8010SERegRead( state->halid, HC ) & (~HC_JSTK) ); 

		return SUCCESS;
	
	} while (0);
 
	/* Clean up time, clean up time, no more time to play */
	if (state->hrbid != HRM_INVALID_ID)
		hrbUndiscoverChip(state->hrbid);
	if(state->fxid != HRM_INVALID_ID)
		fxSystemUndiscoverChip(state->fxid, state->halid);
	if (state->smid != HRM_INVALID_ID)
		smUndiscoverChip(state->smid);
	if (state->seid != HRM_INVALID_ID)
		seUndiscoverChip(state->seid);
    if (state->itmid != HRM_INVALID_ID)
        itmUndiscoverChip(state->itmid);
    if (state->ipid != HRM_INVALID_ID)
		ipUndiscoverChip(state->ipid);

	state->seid=HRM_INVALID_ID;
	state->fxid=(FXID)HRM_INVALID_ID;
	state->smid=HRM_INVALID_ID;
	state->ipid=HRM_INVALID_ID;
    state->itmid=HRM_INVALID_ID;
	state->hrbid=HRM_INVALID_ID;

	state->discFlags &= (~HRMDISC_SOUND_ENGINE);
	return stat;

}

/******************************************************************************
*
* Function: hrmInit
*
* Description: Initialize the EMU8010 and related hardware  
*
* Parameters: none
*
*******************************************************************************
*/

EMUAPIEXPORT 
EMUSTAT hrmInit(void) 
{
	if (bHRMInit==TRUE) return SUCCESS;

	hrmpFreeHRMState(ALL_STATES);

	halInit();
	smInit();
	seInit();
	mxrInit();		
	ipInit();
    itmInit();
	fxSystemInitialize();

	hrbInit();

	bHRMInit=TRUE;

	return SUCCESS;
}


EMUAPIEXPORT 
EMUSTAT hrmDiscoverChip(HRMID *id, HRMCHIPCONFIG *conf)
{

	EMUSTAT stat; 
	HRMFLAGS hDone, hDo, hDid;
	MXRCONFIG mxrConfig;

	stHRMState *state;
	state = hrmpGetActiveHRMStateHWID(conf->dwUserHardwareID, id);
	if (state == NULL)
	{
		state = hrmpGetNextAvailableHRMState(id);
		if (state==NULL)
		{
			return 1;
		}
		state->dwUserHWNum = conf->dwUserHardwareID;
        state->dwHWRevision = conf->dwHWRevision;
	}

	/* hDid are the functions which were discovered before this call */
	if (state->halid != HRM_INVALID_ID)
	{
		hDid = state->discFlags; 
	}
	else
	{
		hDid = 0;
	}

	/* hDo are the functions which now need to be discovered,
	   given the above were already discovered. */
	hDo = conf->hrmDiscoverFlags & (~hDid);

	if ((stat=halDiscoverChip(&state->halid, conf)) != SUCCESS)
	{
		hrmpFreeHRMState(*id);
		*id = HRM_INVALID_ID;
		return stat;
	}

	/* hDone are the functions which are discovered as of this
	   call. */
	state->discFlags = hDone=hDid|hDo;

	do 
	{
		if (hDo & HRMDISC_SOUND_ENGINE)
		{
			/* If this routine fails, all is undiscovered, and state->discFlags 
			   bit for sound engine is reset. */
			stat = hrmpDiscoverSoundEngine(state, conf);	
			if (stat != SUCCESS) break; 
		}

		if (((hDone & conf->hrmMixerFlags) == conf->hrmMixerFlags) &&
			((hDid  & conf->hrmMixerFlags) != conf->hrmMixerFlags))
		{
			memset(&mxrConfig, 0, sizeof(mxrConfig));
			mxrConfig.mixerID    = conf->dwMixerID;
			mxrConfig.bInitMixer = conf->bInitSoundEngine;

			stat = mxrDiscoverChip(state->halid, &mxrConfig, &state->mxrid);
			if (stat != SUCCESS) break;
			state->mxrFlags = conf->hrmMixerFlags;
		}

		/* Don't lose this line!*/
	    return stat;
	} while (0);

	/* Error condition */
	hrmUndiscoverChip(*id, conf->hrmDiscoverFlags);
	*id = HRM_INVALID_ID;
	return stat;

}

#define SET_STAT(a,b) if (a!=SUCCESS) {\
						if (b==SUCCESS) { \
							b=a;}} 

EMUAPIEXPORT
EMUSTAT hrmUndiscoverChip(HRMID id, DWORD flags)
{
	HRMFLAGS hCheck;
	EMUSTAT stat, firstStat;
	stHRMState *state = hrmpGetHRMState(id);
	if (state == NULL) return 1;
	
	hCheck=state->discFlags;
	
	stat=firstStat=SUCCESS;

	if (flags & HRMDISC_SOUND_ENGINE)
	{ 
		if (state->hrbid != HRM_INVALID_ID)
		{
			stat=hrbUndiscoverChip(state->hrbid);
			SET_STAT(stat, firstStat);
			state->hrbid=HRM_INVALID_ID;
		}

		if (state->fxid != (FXID)HRM_INVALID_ID)
		{
			stat=fxSystemUndiscoverChip(state->fxid, (ULONG)(&state->fxch));
			SET_STAT(stat, firstStat);
			state->fxid=(FXID)HRM_INVALID_ID;
		}

		/* Note undiscover sound engine BEFORE sample memory manager */
		if (state->seid != HRM_INVALID_ID)
		{
			stat=seUndiscoverChip(state->seid);
			SET_STAT(stat, firstStat);
			state->seid=HRM_INVALID_ID;
		}

        /* Sound Engine uses itm, so undiscover se first */
        if (state->itmid != HRM_INVALID_ID)
        {
            stat=itmUndiscoverChip(state->itmid);
            SET_STAT(stat, firstStat);
            state->itmid=HRM_INVALID_ID;
        }

        /* Itm depends on ip, so undiscover itm first */
        if (state->ipid != HRM_INVALID_ID)
		{
			stat=ipUndiscoverChip(state->ipid);
			SET_STAT(stat, firstStat);
			state->ipid=HRM_INVALID_ID;
		}

		if (state->smid != HRM_INVALID_ID)
		{
			stat=smUndiscoverChip(state->smid);
			SET_STAT(stat, firstStat);
			state->smid=HRM_INVALID_ID;
		}

	}

	/* If ALL PCI functions which use the mixer are undiscovered, 
	   free the mixer instance.*/
	
	/* 
	   MG If the last PCI function using the mixer is undiscovered,
	   then undiscover the PCI function. 
	*/
	if ((flags & state->mxrFlags) == state->mxrFlags) 
	{
		if (state->mxrid != HRM_INVALID_ID)
		{
			stat=mxrUndiscoverChip(state->mxrid);
			SET_STAT(stat, firstStat);
			state->mxrid=HRM_INVALID_ID;
		}
	}
	else
	{
		state->mxrFlags &= ~flags;
	}

	/* Free the portion of the HAL instance that is undiscovered. */
	halUnDiscoverChip(state->halid, flags);
	hCheck &= ~flags;
	state->discFlags=hCheck;

	/* If ALL PCI functions are undiscovered, 
	   free the hardware resource manager instance.*/
	if (hCheck == 0)
		hrmpFreeHRMState(id);

	return firstStat;
}

EMUAPIEXPORT
BOOL hrmIsDiscovered(HRMID id, HRMFLAGS flags)
{
	/*return halIsDiscovered(hrmGetHALID(id), flags);*/
	stHRMState *state=hrmpGetHRMState(id);
	if (state==NULL) return FALSE;
	if ((flags&state->discFlags)==flags) return TRUE;
	return FALSE;
}

EMUAPIEXPORT 
DWORD hrmGetHardwareInstances(DWORD nInstances, HRMID *id)
{ 
	DWORD count, countFound, countSet;
	stHRMState *state;

	for (count = 0, countFound=0, countSet=0; count!=MAX_CHIPS; count++)
	{ 
		state = &hrmStates[count];

		if (state->dwUserHWNum != 0xFFFFFFFFL)
		{
			if ((id != NULL) && (countFound < nInstances))
			{
				countSet++;
				id[countFound]=HRM_MAKE_ID(count);
			}
			countFound++;
		}
	}
	if ((id != NULL) && (countSet < nInstances))
	  memset(&id[countFound], 0, 
                 (size_t) (nInstances-countSet)*sizeof(HRMID));
 
	return countFound;
}

#define RETURNGETID(a) {stHRMState *state = hrmpGetHRMState(id);\
						if (state==NULL) return (DWORD)HRM_INVALID_ID;\
						return (DWORD)state->a;\
						}

EMUAPIEXPORT 
DWORD hrmGetFXID(HRMID id) 
{ 
	RETURNGETID(fxid); 
}

EMUAPIEXPORT 
DWORD hrmGetSEID(HRMID id) 
{	
	RETURNGETID(seid); 
}

EMUAPIEXPORT 
DWORD hrmGetSMID(HRMID id) 
{	
	RETURNGETID(smid);
}

EMUAPIEXPORT 
DWORD hrmGetIPID(HRMID id) 
{	
	RETURNGETID(ipid);
}

EMUAPIEXPORT 
DWORD hrmGetHALID(HRMID id) 
{ 
	RETURNGETID(halid);
}

EMUAPIEXPORT 
DWORD hrmGetMXRID(HRMID id) 
{ 
	RETURNGETID(mxrid);
}

EMUAPIEXPORT
DWORD hrmGetHRBID(HRMID id)
{
	RETURNGETID(hrbid);
}

EMUAPIEXPORT
DWORD hrmGetITMID(HRMID id)
{
    RETURNGETID(itmid);
}


EMUAPIEXPORT 
EMUSTAT
hrmWaitWallClockCounts(HRMID id, DWORD wait)
{
	return halWaitWallClockCounts(hrmGetHALID(id), wait);
}

EMUAPIEXPORT
DWORD
hrmGetHWRevision(HRMID id)
{
    RETURNGETID(dwHWRevision);
}

#endif
