/******************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module ip8210.c | 
* This file contains the code for the EMU 8010 interrupt pending manager.
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  -----------------------------------------
* Michael Preston     Sep 30, 1997  Initial development.
*
******************************************************************************/

/****************
* Include files
****************/
#include "dbg8210.h"
#include "ip8210d.h"
#include "os8210.h"
#include "ipsd8210.h"
#include "string.h"

/**********
* Defines
**********/
#define NUM_VOICE_INTERRUPTS 64
#define NUM_INTERRUPTS (IP_ONLOOP + NUM_VOICE_INTERRUPTS)

#define IPID_HEADER 0x49500000
#define IPHANDLE_HEADER 0x49480000

#define MAKE_IPID(_ipindex) (IPID)(IPID_HEADER | (_ipindex & 0xff))
#define IS_VALID_IPID(_ipid) (((_ipid & 0xffffff00) == IPID_HEADER) && \
                              IS_VALID_IPINDEX(GET_IPINDEX(_ipid)))
#define IS_VALID_IPINDEX(_ipindex) ((_ipindex < MAX_CHIPS) && \
                                   (ipState[_ipindex] != NULL))
#define GET_IPINDEX(_ipid) (BYTE)(_ipid & 0xff)
#define MAKE_IPHANDLE(_ipindex, _iptype) \
           (IPHANDLE)(IPHANDLE_HEADER | ((_ipindex & 0xff) << 8) | \
                      (_iptype & 0xff))
#define IS_VALID_IPHANDLE(_iphandle) \
           (((_iphandle & 0xffff0000) == IPHANDLE_HEADER) && \
            IS_VALID_IPINDEX(GET_IPINDEX_FROM_HANDLE(_iphandle)) && \
            (GET_IPTYPE_FROM_HANDLE(_iphandle) < NUM_INTERRUPTS))
#define GET_IPINDEX_FROM_HANDLE(_iphandle) \
           (BYTE)((_iphandle >> 8) & 0xff)
#define GET_IPTYPE_FROM_HANDLE(_iphandle) \
           (BYTE)(_iphandle & 0xff)

#define NUM_INIT_PD 20
#define NUM_MIN_PD  10

/***************
* Enumerations
***************/

/*************
* Structures
*************/

typedef struct _stIPInterruptData {
   DWORD interruptParameter;
   DWORD userParameter;
   ipCallback fHandler;
   ipCallback fISRHandler;
} stIPInterruptData;

typedef struct _stIPPendingData {
   IPID ipID;
   DWORD lowChannelData;
   DWORD highChannelData;
   DWORD globalData;
   struct _stIPPendingData *next;
   BYTE MIDIData[32];
   BYTE numMIDIBytes;
} stIPPendingData;

typedef struct _stIPState {
   HALID id;
   DWORD handler;
   DWORD dwPdListLength;
   stIPPendingData *pdList;
   stIPInterruptData ipData[NUM_INTERRUPTS];
} stIPState;

/**********
* Globals
**********/

static stIPState *ipState[MAX_CHIPS];
static DWORD ipEnableTrans[NUM_INTERRUPTS] =
   {EINTE_SRT, /* IP_SRTLOCKED */
    EINTE_FX,  /* IP_FX8010    */
    0,         /* IP_HOSTMODEM */
    EINTE_PCI, /* IP_PCIERROR  */
    EINTE_VI,  /* IP_VOLINC    */
    EINTE_VD,  /* IP_VOLDEC    */
    EINTE_MU,  /* IP_MUTE      */
    EINTE_MB,  /* IP_MICBUFFER */
    EINTE_AB,  /* IP_ADCBUFFER */
    EINTE_FB,  /* IP_FXBUFFER  */
    EINTE_SS,  /* IP_GPSPDIF   */
    EINTE_CS,  /* IP_CDSPDIF   */
    EINTE_IT,  /* IP_INTTIMER  */
    EINTE_TX,  /* IP_UARTTX    */
    EINTE_RX   /* IP_UARTRX    */
   };
static DWORD ipPendingTrans[EINT_SRT+1] =
   {0,
    0,
    0,            /* Channel loop bits */
    0,
    0,
    0,
    IP_ONLOOP,    /* EINT_CL */
    IP_UARTRX,    /* EINT_RX */
    IP_UARTTX,    /* EINT_TX */
    IP_INTTIMER,  /* EINT_IT */
    IP_CDSPDIF,   /* EINT_CS */
    IP_GPSPDIF,   /* EINT_SS */
    IP_FXBUFFER,  /* EINT_FH */
    IP_FXBUFFER,  /* EINT_FF */
    IP_ADCBUFFER, /* EINT_AH */
    IP_ADCBUFFER, /* EINT_AF */
    IP_MICBUFFER, /* EINT_MH */
    IP_MICBUFFER, /* EINT_MF */
    IP_MUTE,      /* EINT_MU */
    IP_VOLDEC,    /* EINT_VD */
    IP_VOLINC,    /* EINT_VI */
    IP_PCIERROR,  /* EINT_PCI */
    IP_HOSTMODEM, /* EINT_HM */
    IP_FX8010,    /* EINT_FX */
    IP_SRTLOCKED  /* EINT_SRT */
   };

/**********************
* Function prototypes
**********************/

BOOL ipServiceInterrupt(IPSVCHANDLE serviceID, IPCBHANDLE *);
void ipServiceCallback(IPCBHANDLE callbackID);
void _ipUnregInt(HALID id, BYTE ipType);


/**************
* IP8210 Code
**************/

EMUSTAT ipInit(void)
{
   BYTE i;
   DWORD mask;

   /* Clear the state variables */
   memset(ipState, 0, sizeof(stIPState *)*MAX_CHIPS);

   /* Setup the interrupt translation tables */
   for (i = IP_ONLOOP, mask = 1; i < IP_ONLOOP+32; i++, mask <<= 1)
   {
      ipEnableTrans[i] = mask;
      ipEnableTrans[i+32] = mask;
   }

   return SUCCESS;
}

EMUSTAT ipDiscoverChip(DWORD halID, stIPConfig *pIPConfig, IPID *ipID)
{
   WORD ipCount, n;
   stIPPendingData *tmp;
   IPID id;

   for (ipCount = 0; (ipCount < MAX_CHIPS) && (ipState[ipCount] != NULL);
        ipCount++);
   if ((ipCount >= MAX_CHIPS) || (pIPConfig == NULL))
      return IP_INVALID_PARAM;

   id = MAKE_IPID(ipCount);
   if (!ipsdSetupInterrupt(pIPConfig->opaque32BitHandler, (IPSVCHANDLE)id,
                           ipServiceInterrupt, ipServiceCallback))
      return IP_SYSTEM_ERROR;

   if ((ipState[ipCount] = (stIPState *)osLockedHeapAlloc(sizeof(stIPState))) == NULL)
      return IP_MEM_ALLOC_FAILED;
   ipState[ipCount]->id = (HALID)halID;
   ipState[ipCount]->handler = pIPConfig->opaque32BitHandler;
   ipState[ipCount]->pdList = NULL;
   for (n = 0; n < NUM_INIT_PD; n++)
   {
      if ((tmp = (stIPPendingData *)osLockedHeapAlloc(sizeof(stIPPendingData))) == NULL)
      {
         osLockedHeapFree(ipState[ipCount], sizeof(stIPState));
         ipState[ipCount] = NULL;
         return IP_MEM_ALLOC_FAILED;
      }
      memset(tmp, 0, sizeof(stIPPendingData));
      tmp->next = ipState[ipCount]->pdList;
      ipState[ipCount]->pdList = tmp;
   }
   ipState[ipCount]->dwPdListLength = NUM_INIT_PD;

   memset(ipState[ipCount]->ipData, 0, NUM_INTERRUPTS*sizeof(stIPInterruptData));
   *ipID = id;

   /* Initialize the interrupt registers */
   L8010SERegWrite(halID, (WORD)INTE, 0);
   LSEPtrWrite(halID, CLIEL, 0);
   LSEPtrWrite(halID, CLIEH, 0); 
   
   return SUCCESS;
}

EMUSTAT ipUndiscoverChip(IPID ipID)
{
   BYTE ipIndex;
   stIPPendingData *tmppd;
   HALID id;

   if (!IS_VALID_IPID(ipID))
      return IP_INVALID_PARAM;

   ipIndex = GET_IPINDEX(ipID);
   id = ipState[ipIndex]->id;

   /* Clear the interrupt registers */
   L8010SERegWrite(id, (WORD)INTE, 0);
   LSEPtrWrite(id, CLIEL, 0);
   LSEPtrWrite(id, CLIEH, 0);

   ipsdSetupInterrupt(ipState[ipIndex]->handler, (IPSVCHANDLE)id,
                      NULL, NULL);

   while (ipState[ipIndex]->pdList != NULL)
   {
      tmppd = ipState[ipIndex]->pdList;
      ipState[ipIndex]->pdList = tmppd->next;
      osLockedHeapFree(tmppd, sizeof(stIPPendingData));
   }

   osLockedHeapFree(ipState[ipIndex], sizeof(stIPState));
   ipState[ipIndex] = NULL;

   return SUCCESS;
}

DWORD ipGetHardwareInstances(DWORD count, IPID *ipIDs)
{
	WORD ipCount, curCount;

   if (ipIDs == NULL)
      return 0;

   for (ipCount = curCount = 0; (ipCount < MAX_CHIPS) && (curCount < count);
        ipCount++)
      if (ipState[ipCount] != NULL)
         ipIDs[curCount++] = MAKE_IPID(ipCount);

    return curCount;
}

EMUSTAT ipGetModuleName(IPID ipID, DWORD count, CHAR *szName)
{
   return SUCCESS;
}

EMUSTAT ipGetModuleAttributes(IPID ipID, stIPAttrib *attrib)
{
   return SUCCESS;
}

EMUAPIEXPORT EMUSTAT ipRegisterCallback(IPID ipID, stIPInfo *info, IPHANDLE *handle)
{
   BYTE ipIndex, ipType, byVoice;
   stIPInterruptData *ipData;
   DWORD dwIntData;
   HALID id;


   if (info->type == IP_ONLOOP)
      byVoice = (BYTE)seGetVoiceIndex((SEVOICE)info->interruptParameter);

   if ((!IS_VALID_IPID(ipID)) ||
       (info->type >= lastInterruptType) ||
       ((info->type == IP_ONLOOP) && (byVoice >= NUM_VOICE_INTERRUPTS)) ||
       (info->fHandler == NULL))
      return IP_INVALID_PARAM;

   ipIndex = GET_IPINDEX(ipID);
   ipType = (BYTE)info->type;
   if (ipType == IP_ONLOOP)
      ipType += byVoice;

   IDISABLE();
   ipData = &(ipState[ipIndex]->ipData[ipType]);

   if (ipData->fHandler != NULL)
   {
      IENABLE();
      return IP_ALREADY_SET;
   }

   ipData->interruptParameter = info->interruptParameter;
   ipData->userParameter = info->userParameter;
   ipData->fHandler = info->fHandler;
   ipData->fISRHandler = info->fISRHandler;
   IENABLE();

   *handle = MAKE_IPHANDLE(ipIndex, ipType);

   /* Enable the interrupt */
   id = ipState[ipIndex]->id;
   if (ipType == IP_INTTIMER)
   {
      /* Set the interval timer frequency */
      W8010SERegWrite(id, TIMR, (WORD)info->interruptParameter);
   }

   if (ipType < IP_ONLOOP)
   {
      dwIntData = L8010SERegRead(id, INTE);
      dwIntData |= ipEnableTrans[ipType];
      L8010SERegWrite(id, INTE, dwIntData);
   }
   else if (ipType < IP_ONLOOP+32)
   {
      dwIntData = LSEPtrRead(id, CLIEL);
      dwIntData |= ipEnableTrans[ipType];
      LSEPtrWrite(id, CLIEL, dwIntData);
   }
   else
   {
      dwIntData = LSEPtrRead(id, CLIEH);
      dwIntData |= ipEnableTrans[ipType];
      LSEPtrWrite(id, CLIEH, dwIntData);
   }

   return SUCCESS;
}

EMUAPIEXPORT EMUSTAT ipUnregisterCallback(IPHANDLE handle)
{
   BYTE ipIndex, ipType;
   stIPInterruptData *ipData;

   if (!IS_VALID_IPHANDLE(handle))
      return IP_INVALID_PARAM;

   ipIndex = GET_IPINDEX_FROM_HANDLE(handle);
   ipType = GET_IPTYPE_FROM_HANDLE(handle);

   IDISABLE();
   ipData = &(ipState[ipIndex]->ipData[ipType]);
   if (ipData->fHandler == NULL)
   {
      IENABLE();
      return IP_INVALID_PARAM;
   }

   _ipUnregInt(ipState[ipIndex]->id, ipType);
   memset(ipData, 0, sizeof(stIPInterruptData));
   IENABLE();

   return SUCCESS;
}

IPID ipGetIPID(IPHANDLE handle)
{
   if (!IS_VALID_IPHANDLE(handle))
      return (IPID)0;
   return MAKE_IPID(GET_IPINDEX_FROM_HANDLE(handle));
}


EMUSTAT ipGetIPHandleInfo(IPHANDLE handle, stIPInfo *info)
{
   BYTE ipIndex, ipType;
   stIPInterruptData *ipData;

   if ((info == NULL) || !IS_VALID_IPHANDLE(handle))
      return IP_INVALID_PARAM;

   ipIndex = GET_IPINDEX_FROM_HANDLE(handle);
   ipType = GET_IPTYPE_FROM_HANDLE(handle);

   ipData = &(ipState[ipIndex]->ipData[ipType]);

   if (ipData->fHandler == NULL)
      return IP_INVALID_PARAM;

   if (ipType < IP_ONLOOP)
      info->type = (enIPType)ipType;
   else
      info->type = IP_ONLOOP;
   info->interruptParameter = ipData->interruptParameter;
   info->userParameter = ipData->userParameter;
   info->fHandler = ipData->fHandler;
   info->fISRHandler = ipData->fISRHandler;

   return SUCCESS;
}


/* Handle an high-priority interrupt.  The returned boolean
 * indicates whether we serviced the interrupt or not, and
 * is used to facilitate the sharing of interrupts with 
 * other devices.  piph points to a DWORD into which
 * the new IPCBHANDLE is to be written.  It is possible that
 * we'll return NULL, which indicates that we don't want to
 * schedule a callback.
 */

#if __BEOS__
#include <OS.h>
#include <unistd.h>
#include "emu10k_driver.h"
#endif

BOOL ipServiceInterrupt(IPSVCHANDLE serviceID, IPCBHANDLE *piph)
{
   stIPPendingData *pd, tmpPendingData;
   BYTE ipIndex;
   HALID id;
   DWORD globalData, lowChannelData=0, highChannelData=0, tmpGlobalData;
   DWORD mask, pos;
   BOOL bScheduleCallback = FALSE;
#if __BEOS__
   emu_int_data data;
   int fd;
#endif

#ifdef DEBUG
   DWORD dwTmpPtr;
#endif

   if (!IS_VALID_IPID((IPID)serviceID)) {
      *piph = 0;
      return FALSE;
   }
   
   ipIndex = GET_IPINDEX((IPID)serviceID);
   id = ipState[ipIndex]->id;

#ifdef DEBUG
   dwTmpPtr = L8010SERegRead(id, PTRREG);
#endif

#if __BEOS__
   fd = halGetUserHardwareID(id);
   if (ioctl(fd, EMU_GET_INTERRUPT_DATA, &data, sizeof(data)) < 0) {
	 printf("EMU_GET_INTERRUPT_DATA failed!  fd = %d\n", fd);
	 return (IPCBHANDLE) 0;
   }
   globalData = data.global_data;
   lowChannelData = data.low_channel_data;
   highChannelData = data.high_channel_data;
#else
   globalData = tmpGlobalData = L8010SERegRead(id, INTP);
   if (globalData & EINTP_CL)
   {
      lowChannelData = LSEPtrRead(id, CLIPL);
      highChannelData = LSEPtrRead(id, CLIPH);
   }
#endif

   /* Read the interrupt pending registers */
   pd = &tmpPendingData;
   pd->globalData = globalData;
   pd->lowChannelData = lowChannelData;
   pd->highChannelData = highChannelData; 

   /* Read the MIDI receive data if there is any */
   pd->numMIDIBytes = 0;

#if __BEOS__
   while((pd->numMIDIBytes < data.midi_bytes) && (pd->numMIDIBytes < 32)) {
      pd->MIDIData[pd->numMIDIBytes] = data.midi_data[pd->numMIDIBytes];
      ++pd->numMIDIBytes;
   }
#else
   /* If there's more than 16 bytes in the FIFO, that means that
      more data sneaked in since we started reading.  If there are
      more than a couple of extra bytes, there's something seriously
      wrong! */
   while((tmpGlobalData & EINTP_RX) && (pd->numMIDIBytes < 32))
   {
      pd->MIDIData[pd->numMIDIBytes++] = B8010SERegRead(id, MUDTA);
      tmpGlobalData = L8010SERegRead(id, INTP);
   }

   if (tmpGlobalData & EINTP_TX)
   {
      DWORD dwIntData = L8010SERegRead(id, INTE);
      dwIntData &= ~ipEnableTrans[IP_UARTTX];
      L8010SERegWrite(id, INTE, dwIntData);
      tmpGlobalData &= ~EINTP_TX;
   }

   /* Clear the interrupt pending registers */
   if (tmpGlobalData & EINTP_CL)
   {
      LSEPtrWrite(id, CLIPL, pd->lowChannelData);
      LSEPtrWrite(id, CLIPH, pd->highChannelData);
   }
   L8010SERegWrite(id, INTP, tmpGlobalData);
#endif

   /* If it doesn't look like this interrupt belongs to us, return. */
   if (!(pd->globalData || pd->lowChannelData || pd->highChannelData))
   {
#ifdef DEBUG
      L8010SERegWrite(id, PTRREG, dwTmpPtr);
#endif
      *piph = 0;
      return FALSE;
   }

   /* Invoke the high priority ISRS, if any */
   for (mask = EINTP_CL, pos = EINT_CL; pos <= EINT_SRT; mask <<= 1, pos++)
   {
      if (pd->globalData & mask)
      {
         unIPReply repl;
         stIPInterruptData *ipData;
         BYTE ipType;
		 BOOL bCallDispatch;

         memset(&repl, 0, sizeof(unIPReply));
         if (pos == EINT_CL)
         {
            /* Process interrupt on loop callbacks */
            BYTE byCurChannel, byHighChannel = (BYTE)(pd->globalData&0x3f);

            for (byCurChannel = 0; byCurChannel <= byHighChannel; byCurChannel++)
            {
               ipType = IP_ONLOOP + byCurChannel;
               if (((byCurChannel < 32) &&
                    (pd->lowChannelData & ipEnableTrans[ipType])) ||
                   ((byCurChannel >= 32) &&
                    (pd->highChannelData & ipEnableTrans[ipType])))
			   {
                  /* Do the callbacks */
				  bCallDispatch = TRUE;
                  ipData = &ipState[ipIndex]->ipData[ipType];
                  if (ipData->fISRHandler)
                  {
                     repl.voiceHandle = ipData->interruptParameter;
				     if (!ipData->fISRHandler(MAKE_IPHANDLE(ipIndex, ipType),
							                  IP_ONLOOP, 
										      ipData->userParameter, &repl))
					    bCallDispatch = FALSE;
				  }
                 
                  if (!bCallDispatch) {
				     /* Don't invoke the lower level interrupt handler */
					 if (byCurChannel < 32) 
						 pd->lowChannelData &= ~ ipEnableTrans[ipType];
					 else
						 pd->highChannelData &= ~ipEnableTrans[ipType];
                  } else {
                      bScheduleCallback = TRUE;
                  }
			   }
            }
         }
         else
         {
            /* Process other callbacks */
            ipType = (BYTE)ipPendingTrans[pos];

            /* Setup reply structure if necessary */
            if ((pos == EINT_FH) || (pos == EINT_AH) || (pos == EINT_MH))
               repl.bIsHalfBuffer = TRUE;
            else if (pos == EINT_RX)
            {
               repl.MIDIData.numMIDIBytes = pd->numMIDIBytes;
               memcpy(&repl.MIDIData.MIDIData, pd->MIDIData,
                      (size_t)repl.MIDIData.numMIDIBytes);
            }

            /* Do the callbacks */
			bCallDispatch = TRUE;
            ipData = &ipState[ipIndex]->ipData[ipType];
            if (ipData->fISRHandler)
            {
			   if (!ipData->fISRHandler(MAKE_IPHANDLE(ipIndex, ipType),
                                        (enIPType)ipType, ipData->userParameter, &repl))
			      bCallDispatch = FALSE;
            }

			if (!bCallDispatch) 
				pd->globalData &= ~mask;
            else
                bScheduleCallback = TRUE;

         } /* end else */
      } /* end if */
   } /* end for */

   pd->ipID = (IPID)serviceID;

#ifdef DEBUG
   L8010SERegWrite(id, PTRREG, dwTmpPtr);
#endif

   if (bScheduleCallback) {
       stIPPendingData *retpd;

       /* Try and grab a pending IP structure off the free list */
       retpd = ipState[ipIndex]->pdList;
       if (retpd == NULL) {
#if DEBUG
           dprintf("WARNING -- Ran out of IP structures.");
#endif
           *piph = (IPCBHANDLE) 0;
       } else {
           /* Dequeue the IP structure from the free list */
           ipState[ipIndex]->pdList = ipState[ipIndex]->pdList->next;
           ipState[ipIndex]->dwPdListLength--;

           /* Copy the temporary data into the returning data */
           *retpd = *pd;
           retpd->next = NULL;
           *piph = (IPCBHANDLE)retpd;
       }
   }
   else {
       /* We don't want to schedule a callback, so we just
        * set the handle to 0.  
        */
       *piph = (IPCBHANDLE) 0;
   }

   return TRUE;
}

void ipServiceCallback(IPCBHANDLE callbackID)
{
   stIPPendingData *pd = (stIPPendingData *)callbackID;
   BYTE ipIndex;
   DWORD n;
   DWORD mask, pos;
   stIPPendingData *newPdList, *lastNewPd;
   DWORD dwNewPdListLength;
#ifdef DEBUG
   DWORD dwTmpPtr;
#endif

   if ((pd == NULL) || (!IS_VALID_IPID(pd->ipID)))
      return;

   ipIndex = GET_IPINDEX(pd->ipID);

#ifdef DEBUG
   dwTmpPtr = L8010SERegRead(ipState[ipIndex]->id, PTRREG);
#endif

   /* Check global interrupts */
   for (mask = EINTP_CL, pos = EINT_CL; pos <= EINT_SRT; mask <<= 1, pos++)
   {
      if (pd->globalData & mask)
      {
         unIPReply repl;
         stIPInterruptData *ipData;
         BYTE ipType;

         memset(&repl, 0, sizeof(unIPReply));
         if (pos == EINT_CL)
         {
            /* Process interrupt on loop callbacks */
            BYTE byCurChannel, byHighChannel = (BYTE)(pd->globalData&0x3f);

            for (byCurChannel = 0; byCurChannel <= byHighChannel; byCurChannel++)
            {
               ipType = IP_ONLOOP + byCurChannel;
               if (((byCurChannel < 32) &&
                    (pd->lowChannelData & ipEnableTrans[ipType])) ||
                   ((byCurChannel >= 32) &&
                    (pd->highChannelData & ipEnableTrans[ipType])))
               {
                  /* Do the callback */
                  ipData = &ipState[ipIndex]->ipData[ipType];
                  repl.voiceHandle = ipData->interruptParameter;
                  if (ipData->fHandler)
                     if (!ipData->fHandler(MAKE_IPHANDLE(ipIndex, ipType),
                                           IP_ONLOOP, ipData->userParameter, &repl))
                     {
                        memset(ipData, 0, sizeof(stIPInterruptData));
                     }
               }
            }
         }
         else
         {
            /* Process other callbacks */
            ipType = (BYTE)ipPendingTrans[pos];

            /* Setup reply structure if necessary */
            if ((pos == EINT_FH) || (pos == EINT_AH) || (pos == EINT_MH))
               repl.bIsHalfBuffer = TRUE;
            else if (pos == EINT_RX)
            {
               repl.MIDIData.numMIDIBytes = pd->numMIDIBytes;
               memcpy(&repl.MIDIData.MIDIData, pd->MIDIData,
                      (size_t)repl.MIDIData.numMIDIBytes);
            }

            /* Do the callbacks */
            ipData = &ipState[ipIndex]->ipData[ipType];
            if (ipData->fHandler)
               if (!ipData->fHandler(MAKE_IPHANDLE(ipIndex, ipType),
                                     (enIPType)ipType, ipData->userParameter, &repl))
               {
                  memset(ipData, 0, sizeof(stIPInterruptData));
               }
         }

         /* Unregister the interrupt if necessary */
         if (ipState[ipIndex]->ipData[ipType].fHandler == NULL)
            _ipUnregInt(ipState[ipIndex]->id, ipType);
      }
   }

   /* Return pending data structure to the list */
   memset(pd, 0, sizeof(stIPPendingData));
   //pd->next = ipState[ipIndex]->pdList;
   newPdList = lastNewPd = pd;
   dwNewPdListLength = 1;

   /* Check to make sure we have enough pending data structures on the list */
//   for (n = 0, pd = ipState[ipIndex]->pdList; pd != NULL; n++, pd = pd->next);

   n = ipState[ipIndex]->dwPdListLength;
   while (n < NUM_MIN_PD)
   {
      if ((pd = (stIPPendingData *)osLockedHeapAlloc(sizeof(stIPPendingData))) == NULL)
         return;
      memset(pd, 0, sizeof(stIPPendingData));
      pd->next = newPdList;
      newPdList = pd;
      n++;
      dwNewPdListLength++;
   }
   /* Disable interrupts briefly to update the pointer */
   IDISABLE();
   lastNewPd->next = ipState[ipIndex]->pdList;
   ipState[ipIndex]->pdList = newPdList;
   ipState[ipIndex]->dwPdListLength += dwNewPdListLength;
   IENABLE();

   /* Reenable MIDI transmit interrupt if necessary */
   if (ipState[ipIndex]->ipData[IP_UARTTX].fHandler != NULL)
   {
      DWORD dwIntData = L8010SERegRead(ipState[ipIndex]->id, INTE);
      dwIntData |= ipEnableTrans[IP_UARTTX];
      L8010SERegWrite(ipState[ipIndex]->id, INTE, dwIntData);
   }

#ifdef DEBUG
   L8010SERegWrite(ipState[ipIndex]->id, PTRREG, dwTmpPtr);
#endif
}

void _ipUnregInt(HALID id, BYTE ipType)
{
   /* Disable the interrupt */
   DWORD dwIntData;

   if (ipType < IP_ONLOOP)
   {
      dwIntData = L8010SERegRead(id, INTE);
      dwIntData &= ~ipEnableTrans[ipType];
      L8010SERegWrite(id, INTE, dwIntData);
   }
   else if (ipType < IP_ONLOOP+32)
   {
      dwIntData = LSEPtrRead(id, CLIEL);
      dwIntData &= ~ipEnableTrans[ipType];
      LSEPtrWrite(id, CLIEL, dwIntData);
   }
   else
   {
      dwIntData = LSEPtrRead(id, CLIEH);
      dwIntData &= ~ipEnableTrans[ipType];
      LSEPtrWrite(id, CLIEH, dwIntData);
   }
}
