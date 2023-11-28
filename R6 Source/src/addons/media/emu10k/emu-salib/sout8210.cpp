/******************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module sout8210.cpp | 
* This file contains the source code for the E10K1 output device classes.
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  --------------------------------------
* Michael Preston     Dec  5, 1997  Initial development.
*
******************************************************************************/

#include <stdio.h>		/* for sprintf */
#include "datatype.h"
#include "sout8210.h"
#include "emufolks.h"
#include "se8210.h"
#include "sm8210.h"
#include "ip8210.h"
#include "emuerrs.h"
#include "string.h"
#include "emuattr.h"
#include "dbg8210.h"
#include "souttrns.h"

// Macros

#define ADD_BUFFER_TO_LIST(_list, _buffer) \
   { \
      if (_list == NULL) \
         _list = _buffer; \
      else \
      { \
         stSAOutputBuffer *tmp; \
         for (tmp = _list; tmp->pNextBuffer != NULL; \
              tmp = tmp->pNextBuffer); \
         tmp->pNextBuffer = _buffer; \
      } \
   }

#define GET_INST_NOENABLE(list, ret_value) \
   SASE8210InstanceInfo *inst; \
   IDISABLE(); \
   list.Find((SEVOICE)devHandle); \
   if (list.EndOfList()) \
   { \
      IENABLE(); \
      return ret_value; \
   } \
   inst = list.GetCurItem();
#define GET_INST(list, ret_value) \
    GET_INST_NOENABLE(list, ret_value) \
    IENABLE();

#define COMP_RAW_PITCH(dwSampleRate) \
      (((dwSampleRate*0x4000+CLOCK_RATE-1)/CLOCK_RATE)&0xffff)

#define DEBUG_STOP_VOICE \
      DWORD ip = seParameterRead((SEVOICE)devHandle, sepInitialPitch); \
      seParameterWrite((SEVOICE)devHandle, sepInitialPitch, 0)
#define DEBUG_RESTART_VOICE \
      seParameterWrite((SEVOICE)devHandle, sepInitialPitch, ip)

#ifdef DEBUG_INST_VALS
#define STORE_INST_DEBUG_VAL(x, y) \
    { \
      inst->vals[inst->dwNumVals++] = x; \
      inst->vals[inst->dwNumVals++] = y; \
      if (inst->dwNumVals >= 1000) \
          inst->dwNumVals = 0; \
    }
#define STORE_INST_DEBUG_WC(x) \
    { \
      inst->vals[inst->dwNumVals++] = x; \
      inst->vals[inst->dwNumVals++] = L8010SERegRead(0x11a10000, WC)/3; \
      if (inst->dwNumVals >= 1000) \
          inst->dwNumVals = 0; \
    }
#define STORE_INST_DEBUG_PTRREG(x, y) \
    { \
      inst->vals[inst->dwNumVals++] = x; \
      LGRead(0x11a10000, (y), &inst->vals[inst->dwNumVals++]); \
      if (inst->dwNumVals >= 1000) \
          inst->dwNumVals = 0; \
    }
#else
#define STORE_INST_DEBUG_VAL(x, y)
#define STORE_INST_DEBUG_WC(x)
#define STORE_INST_DEBUG_PTRREG(x, y)
#endif

#ifdef DEBUG
#  define INIT_PLAYBACK_MODE TRUE // Change this one for testing
#else
#  define INIT_PLAYBACK_MODE TRUE  // Never change this one!!!
#endif

#ifdef DEBUG_INST_VALS
#include "hal8210.h"
#endif

// Page align addresses
#define PAGE_ALIGN(_addr) (((DWORD)(_addr))&0xfffff000)
#define PAGE_ALIGN_UP(_addr) (((DWORD)(_addr)+SM_PAGE_SIZE-1)&0xfffff000)

// Allocate 136K of page table space for buffering
#define MAX_BUFFER_SIZE 0x10000
#define SM_BUFFER_SIZE (MAX_BUFFER_SIZE+SM_PAGE_SIZE)
#define ZERO_BUFFER_16BIT_POS 0
#define ZERO_BUFFER_8BIT_POS (SM_PAGE_SIZE/2)

// Scale sample rates for 48000 Hz
#define CLOCK_RATE         48000
//#define SCALE_MASTER_CLOCK 11185

// Static variables
#if 0
static const DWORD logMagTable[128] = {
0x00000, 0x02dfc, 0x05b9e, 0x088e6, 0x0b5d6, 0x0e26f, 0x10eb3, 0x13aa2,
0x1663f, 0x1918a, 0x1bc84, 0x1e72e, 0x2118b, 0x23b9a, 0x2655d, 0x28ed5,
0x2b803, 0x2e0e8, 0x30985, 0x331db, 0x359eb, 0x381b6, 0x3a93d, 0x3d081,
0x3f782, 0x41e42, 0x444c1, 0x46b01, 0x49101, 0x4b6c4, 0x4dc49, 0x50191,
0x5269e, 0x54b6f, 0x57006, 0x59463, 0x5b888, 0x5dc74, 0x60029, 0x623a7,
0x646ee, 0x66a00, 0x68cdd, 0x6af86, 0x6d1fa, 0x6f43c, 0x7164b, 0x73829,
0x759d4, 0x77b4f, 0x79c9a, 0x7bdb5, 0x7dea1, 0x7ff5e, 0x81fed, 0x8404e,
0x86082, 0x88089, 0x8a064, 0x8c014, 0x8df98, 0x8fef1, 0x91e20, 0x93d26,
0x95c01, 0x97ab4, 0x9993e, 0x9b79f, 0x9d5d9, 0x9f3ec, 0xa11d8, 0xa2f9d,
0xa4d3c, 0xa6ab5, 0xa8808, 0xaa537, 0xac241, 0xadf26, 0xafbe7, 0xb1885,
0xb3500, 0xb5157, 0xb6d8c, 0xb899f, 0xba58f, 0xbc15e, 0xbdd0c, 0xbf899,
0xc1404, 0xc2f50, 0xc4a7b, 0xc6587, 0xc8073, 0xc9b3f, 0xcb5ed, 0xcd07c,
0xceaec, 0xd053f, 0xd1f73, 0xd398a, 0xd5384, 0xd6d60, 0xd8720, 0xda0c3,
0xdba4a, 0xdd3b4, 0xded03, 0xe0636, 0xe1f4e, 0xe384a, 0xe512c, 0xe69f3,
0xe829f, 0xe9b31, 0xeb3a9, 0xecc08, 0xee44c, 0xefc78, 0xf148a, 0xf2c83,
0xf4463, 0xf5c2a, 0xf73da, 0xf8b71, 0xfa2f0, 0xfba57, 0xfd1a7, 0xfe8df};

static const SHORT logSlopeTable[128] = {
0x5c, 0x5c, 0x5b, 0x5a, 0x5a, 0x59, 0x58, 0x58,
0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x53, 0x53,
0x52, 0x52, 0x51, 0x51, 0x50, 0x50, 0x4f, 0x4f,
0x4e, 0x4d, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4b,
0x4a, 0x4a, 0x49, 0x49, 0x48, 0x48, 0x47, 0x47,
0x47, 0x46, 0x46, 0x45, 0x45, 0x45, 0x44, 0x44,
0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x41, 0x41,
0x41, 0x40, 0x40, 0x40, 0x3f, 0x3f, 0x3f, 0x3e,
0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c,
0x3b, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x39,
0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x38, 0x37,
0x37, 0x37, 0x37, 0x36, 0x36, 0x36, 0x36, 0x35,
0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x34, 0x34,
0x33, 0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x32,
0x32, 0x31, 0x31, 0x31, 0x31, 0x31, 0x30, 0x30,
0x30, 0x30, 0x30, 0x2f, 0x2f, 0x2f, 0x2f, 0x2f};
#endif

LONG SAOutputDefault[saospNumSynthParams] =
{
    0,           // saospGain
    0,           // saospPan
    0,           // saospPitch
    885235594,   // saospFilterCutoff
    0,           // saospFilterQ
    0x80000000,  // saospDelayVolEnv
    0x80000000,  // saospAttackVolEnv
    0x80000000,  // saospHoldVolEnv
    0x80000000,  // saospDecayVolEnv
    65536000,    // saospSustainVolEnv
    0x80000000,  // saospReleaseVolEnv
    0x80000000,  // saospDelayModEnv
    0x80000000,  // saospAttackModEnv
    0x80000000,  // saospHoldModEnv
    0x80000000,  // saospDecayModEnv
    65536000,    // saospSustainModEnv
    0x80000000,  // saospReleaseModEnv
    0x80000000,  // saospDelayModLFO
    -55791973,   // saospFreqModLFO
    0x80000000,  // saospDelayVibLFO
    -55791973,   // saospFreqVibLFO
    0,           // saospModLFOToPitch
    0,           // saospVibLFOToPitch
    0,           // saospModLFOToFilterCutoff
    0,           // saospModLFOToAttenuation
    0,           // saospModEnvToPitch
    0            // saospModEnvToFilterCutoff
};

/****************************************************************************
 * The SASE8210OutputSetup class sets up the empty buffer used by all
 * SASE8210OutputDevice instances.  This class is instantiated once here so
 * the memory will be allocated when the driver is loaded, and freed when
 * the driver is unloaded.
 ****************************************************************************/

static void *pEmptyBuffer;
static DWORD hEmptyBuffer;
static DWORD dwEmptyBuffer;

class SASE8210OutputSetup
{
    public:

    SASE8210OutputSetup()
    {
        if ((pEmptyBuffer = osAllocPages(1, &hEmptyBuffer)) == NULL)
        {
            ASSERT(0);
			return;
        }

        if (osLockVirtualRange(pEmptyBuffer, 1, 1, &dwEmptyBuffer) != SUCCESS)
        {
            ASSERT(0);
            osFreePages(hEmptyBuffer);
			return;
        }

        memset((BYTE *)pEmptyBuffer+ZERO_BUFFER_16BIT_POS, 0, SM_PAGE_SIZE/2);
        memset((BYTE *)pEmptyBuffer+ZERO_BUFFER_8BIT_POS, 0x80, SM_PAGE_SIZE/2);
    }

    ~SASE8210OutputSetup()
    {
		if (pEmptyBuffer) {
			osUnlockVirtualRange((OSVIRTADDR)pEmptyBuffer, 1);
        	osFreePages(hEmptyBuffer);
		}
    }
};

static SASE8210OutputSetup dummy;

/****************************************************************************
 *  SAOutputDevice classes
 ****************************************************************************/

// @func Don't need to do much here.  We define this mostly to allow us to
// keep folks from calling the constructor.
//
SAOutputDevice::~SAOutputDevice()
{
}

// @func Store the SE-specific data that we can get our hands on.
//  Later derived classes will actually do more with this stuff.
SASE8210OutputDevice::SASE8210OutputDevice(HRMID theHrmid, DWORD dwDevNum /*,
                                           stSASE8210OutputConfig *pConfig,
                                           void *pBuffer, DWORD dwBuffer*/)
{
    hrmid = theHrmid;
	seid = hrmGetSEID(hrmid);
	smid = hrmGetSMID(hrmid);
	ipid = hrmGetIPID(hrmid);
	halid = hrmGetHALID(hrmid);
	//devConfig = *pConfig;
	//pEmptyBuffer = pBuffer;
    //dwEmptyBuffer = dwBuffer;
    devCaps.sampleRates = saRateRange;
    devCaps.sampleFormats = saFormatUnsigned8PCM | saFormatSigned16PCM;
    devCaps.dwChannels = 2;
    devCaps.dwFlags = 0;
    devCaps.dwLowSampleRate = 4000;
    devCaps.dwHighSampleRate = 48000;
    sprintf(devCaps.szDevName, "SA Output %c", 'A'+dwDevNum);
    devConfig.dwSendAmountA = 255;
    devConfig.dwSendAmountB = 255;
    devConfig.dwSendAmountC = 0;
    devConfig.dwSendAmountD = 0;
    devConfig.dwSendRoutingA = 0;
    devConfig.dwSendRoutingB = 1;
    devConfig.dwSendRoutingC = 2;
    devConfig.dwSendRoutingD = 3;
}

// @func We don't currently need to do any operations on instance destruction
SASE8210OutputDevice::~SASE8210OutputDevice()
{
}


// @func The HRB class simply maintains a per-instance copy of the device caps.
//  We just return this.
stSADevCaps *
SASE8210OutputDevice::GetDevCaps()
{
	return &devCaps;
}


DWORD
SASE8210OutputDevice::GetResourceID()
{
    return hrmid;
}


DWORD
SASE8210OutputDevice::Open(stSAOutputConfig *pConfig)
{
   SEVOICESETUP *setupArr, **pSetup;
   SASE8210InstanceInfo *inst;
   BYTE i;
   SAOutputSampleData sampdata;

   if ((inst = new SASE8210InstanceInfo) == NULL)
      return 0;
      
   if ((inst->voiceHandle =
       (SEVOICE *)NewAlloc(pConfig->byNumChans*sizeof(SEVOICE))) == NULL)
   {
      delete inst;
      return 0;
   }
      
   if ((setupArr = (SEVOICESETUP *)
                    NewAlloc(pConfig->byNumChans*sizeof(SEVOICESETUP))) == NULL)
   {
      delete inst;
      return 0;
   }

   if ((pSetup = (SEVOICESETUP **)
                  NewAlloc(pConfig->byNumChans*sizeof(SEVOICESETUP *))) == NULL)
   {
      DeleteAlloc(setupArr);
      delete inst;
      return 0;
   }
      
   // Keep track of whether we're synchronizing this channel 
   // with other stuff
   if (pConfig->playbackMode & sapmSync)
       inst->bIsSynchronized = TRUE;

   DWORD dwFlags = 0;
   dwFlags |= (pConfig->byNumChans > 1) ? SEAF_PAIRED : 0;
   dwFlags |= (pConfig->playbackMode & sapmSync) ? SEAF_SYNCABLE : 0;
   if (seVoiceAllocate(seid, 0, SEAP_RESERVED, dwFlags, 
                       pConfig->byNumChans,
                       inst->voiceHandle,
#ifdef DEBUG
                       SASE8210OutputDevice::NotifyCallBack,
#else
                       NULL,
#endif
                       0) != pConfig->byNumChans)
   {
      DeleteAlloc(pSetup);
      DeleteAlloc(setupArr);
      delete inst;
      return 0;
   }

   STORE_INST_DEBUG_WC(0x4f70656e)

   inst->playbackMode = pConfig->playbackMode & (sapmControlFrequency|sapmControlAll);
   inst->callback = pConfig->callback;
   inst->dwSampleRate = pConfig->dwSampleRate;
   switch (pConfig->sampleFormat) {
      case saFormatUnsigned8PCM:  inst->bySampleSize = 1;  break;
      case saFormatSigned16PCM:   inst->bySampleSize = 2;  break;
      default:                    inst->bySampleSize = 2;  break;
   }
   inst->byNumChans = pConfig->byNumChans;
   inst->byFrameSize = inst->byNumChans*inst->bySampleSize;
   
   if (smMemoryAllocate(smid, SM_BUFFER_SIZE*2,
       (inst->bySampleSize == 1 ? SMAF_8BIT_DATA : 0), &inst->smHandle) != SUCCESS)
   {
       seVoiceFree(inst->byNumChans, inst->voiceHandle);
       DeleteAlloc(pSetup);
       DeleteAlloc(setupArr);
       delete inst;
       return 0;
   }

   IDISABLE();
   instList.Insert(inst);
   IENABLE();

   memset(setupArr, 0, inst->byNumChans*sizeof(SEVOICESETUP));
   for (i = 0; i < inst->byNumChans; i++)
   {
      pSetup[i] = &setupArr[i];
      setupArr[i].voice = inst->voiceHandle[i];
      setupArr[i].baseConfig = secfgCustom;

      setupArr[i].dwParamValues[sepPriority] = SEAP_RESERVED;
      setupArr[i].dwParamValues[sepEightBitSample] = (inst->bySampleSize == 1);
      setupArr[i].dwParamValues[sepStereo] = ((inst->byNumChans > 1) ?
                                             SESTMODE_INTERLEAVED :
                                             SESTMODE_MONO);
      setupArr[i].dwParamValues[sepSampleMemoryHandle] = inst->smHandle;
      if (inst->playbackMode != sapmControlAll)
      {
         setupArr[i].dwParamValues[sepDisableEnv] = TRUE;
         setupArr[i].dwParamValues[sepRawPitch] = COMP_RAW_PITCH(inst->dwSampleRate);
         setupArr[i].dwParamValues[sepRawVolume] = 0xffff;
         setupArr[i].dwParamValues[sepRawFilter] = 0xffff;
         setupArr[i].dwParamValues[sepInterpolationROM] =
            ((inst->dwSampleRate == CLOCK_RATE) &&
             (inst->playbackMode == sapmControlFrequency)) ? 0 : 1;
         setupArr[i].dwParamValues[sepEffectsSendA] =
            (((inst->byNumChans == 1) || (i == 0)) ? devConfig.dwSendAmountA : 0);
         setupArr[i].dwParamValues[sepEffectsSendB] =
            (((inst->byNumChans == 1) || (i == 1)) ? devConfig.dwSendAmountB : 0);
      }
      else
      {
         DWORD dest;

         inst->trans.SetFileType("SAOutput");
         inst->trans.SetDefaultParameters();
         sampdata.dwSamplesPerSec = pConfig->dwSampleRate;
         inst->trans.SetSoundParameters(&sampdata, FALSE);
         inst->trans.Translate();

         for (dest = sepDelayModLFO; dest <= sepEffectsSendB; dest++)
             setupArr[i].dwParamValues[dest] =
                inst->xlatapp.remveDestIndex[dest](inst->trans.GetDestParameter(dest));
         setupArr[i].dwParamValues[sepInterpolationROM] = 1;
      }
      if (inst->byNumChans > 1)
      {
          if (i == 0)
             setupArr[i].dwParamValues[sepEffectsSendB] = 0;
          else if (i == 1)
             setupArr[i].dwParamValues[sepEffectsSendA] = 0;
      }
      setupArr[i].dwParamValues[sepRouteEffectsSendA] = devConfig.dwSendRoutingA;
      setupArr[i].dwParamValues[sepRouteEffectsSendB] = devConfig.dwSendRoutingB;
      setupArr[i].dwParamValues[sepRouteEffectsSendC] = devConfig.dwSendRoutingC;
      setupArr[i].dwParamValues[sepRouteEffectsSendD] = devConfig.dwSendRoutingD;
      setupArr[i].dwParamValues[sepPriority] = SEAP_RESERVED;
   }
   
   if (seVoiceSetup(inst->byNumChans, pSetup) != SUCCESS)
      ASSERT(0);
   
   DeleteAlloc(pSetup);
   DeleteAlloc(setupArr);
   
   return inst->voiceHandle[0];
}


BOOL
SASE8210OutputDevice::SetDeviceSendRouting(DWORD dwSend, DWORD dwRouting)
{
    DWORD dwParamID;

    switch (dwSend) {
    case 0:
        devConfig.dwSendRoutingA = dwRouting;
        dwParamID = sepRouteEffectsSendA;
        break;
    case 1:
        devConfig.dwSendRoutingB = dwRouting;
        dwParamID = sepRouteEffectsSendB;
        break;
    case 2:
        devConfig.dwSendRoutingC = dwRouting;
        dwParamID = sepRouteEffectsSendC;
        break;
    case 3:
        devConfig.dwSendRoutingD = dwRouting;
        dwParamID = sepRouteEffectsSendD;
        break;
    default:
        return FALSE;
    };

    IDISABLE();
    for (instList.First(); !instList.EndOfList(); instList.Next())
    {
        SASE8210InstanceInfo *inst;

        inst = instList.GetCurItem();
        seParameterWrite((SEVOICE)inst->voiceHandle[0], dwParamID, dwRouting);
        if (inst->byNumChans > 1)
            seParameterWrite((SEVOICE)inst->voiceHandle[1], dwParamID, dwRouting);
    }
    IENABLE();

    return TRUE;
}

/*
BOOL
SASE8210OutputDevice::SetDeviceSendMaxAmount(BYTE bySend, BYTE byMaxAmount)
{
    DWORD dwParamID;

    switch (bySend) {
    case 0:
        devConfig.bySendAmountA = byMaxAmount;
        dwParamID = sepEffectsSendA;
        break;
    case 1:
        devConfig.bySendAmountB = byMaxAmount;
        dwParamID = sepEffectsSendB;
        break;
    case 2:
        devConfig.bySendAmountC = byMaxAmount;
        dwParamID = sepEffectsSendC;
        break;
    case 3:
        devConfig.bySendAmountD = byMaxAmount;
        dwParamID = sepEffectsSendD;
        break;
    default:
        return FALSE;
    };

    IDISABLE();
    for (instList.First(); !instList.EndOfList(); instList.Next())
    {
        SASE8210InstanceInfo *inst;

        inst = instList.GetCurItem();
        if ((inst->byNumChans == 1) || (bySend == 0))
            seParameterWrite((SEVOICE)inst->voiceHandle[0], dwParamID, byMaxAmount);
        if ((inst->byNumChans > 1) && (bySend == 1))
            seParameterWrite((SEVOICE)inst->voiceHandle[1], dwParamID, byMaxAmount);
    }
    IENABLE();

    return TRUE;
}
*/

BOOL
SASE8210OutputDevice::SetSendAmount(DWORD devHandle, DWORD dwSend, DWORD dwAmount)
{
    GET_INST(instList, FALSE)

    DWORD dwParamID;

    switch (dwSend) {
    case 0:
        dwParamID = sepEffectsSendA;
        break;
    case 1:
        dwParamID = sepEffectsSendB;
        break;
    case 2:
        dwParamID = sepEffectsSendC;
        break;
    case 3:
        dwParamID = sepEffectsSendD;
        break;
    default:
        return FALSE;
    };

    if (inst->byNumChans == 1)
        seParameterWrite(inst->voiceHandle[0], dwParamID, dwAmount);
	else
        seParameterWrite(inst->voiceHandle[dwSend & 1], dwParamID, dwAmount);

    return TRUE;
}


BOOL
SASE8210OutputDevice::SetSendRouting(DWORD devHandle, DWORD dwSend,
                                     DWORD dwRouting)
{
    DWORD dwParamID;

    GET_INST(instList, FALSE)

    switch (dwSend) {
    case 0:
        dwParamID = sepRouteEffectsSendA;
        break;
    case 1:
        dwParamID = sepRouteEffectsSendB;
        break;
    case 2:
        dwParamID = sepRouteEffectsSendC;
        break;
    case 3:
        dwParamID = sepRouteEffectsSendD;
        break;
    default:
        return FALSE;
    };

    seParameterWrite((SEVOICE)inst->voiceHandle[0], dwParamID, dwRouting);
    if (inst->byNumChans > 1)
        seParameterWrite((SEVOICE)inst->voiceHandle[1], dwParamID, dwRouting);

    return TRUE;
}


BOOL
SASE8210OutputDevice::SetSynthParam(DWORD devHandle, enSAOutputSynthParam param,
                                    LONG lScale)
{
    GET_INST(instList, FALSE)

    return _SetSynthParam(inst, param, lScale);
}


BOOL
SASE8210OutputDevice::_SetSynthParam(SASE8210InstanceInfo *inst,
                                     enSAOutputSynthParam param, LONG lScale)
{
    if (inst->playbackMode != sapmControlAll)
        return FALSE;

    DWORD dest = inst->trans.GetDestination(param);
    inst->trans.SetParameterAndTranslate(param, lScale, FALSE);
    seParameterWrite(inst->voiceHandle[0], dest,
        inst->xlatapp.remveDestIndex[dest](inst->trans.GetDestParameter(dest)));
    if (param == saospPan)
    {
        if (inst->byNumChans > 1)
            seParameterWrite(inst->voiceHandle[1], sepEffectsSendB,
                inst->xlatapp.remveDestIndex[sepEffectsSendB](inst->trans.GetDestParameter(sepEffectsSendB)));
        else
            seParameterWrite(inst->voiceHandle[0], sepEffectsSendB,
                inst->xlatapp.remveDestIndex[sepEffectsSendB](inst->trans.GetDestParameter(sepEffectsSendB)));
    }

    if (param == saospPitch)
        inst->lPitch = lScale;

    return TRUE;
}


BOOL
SASE8210OutputDevice::SetSynthConnection(DWORD devHandle, WORD wSource,
                                         WORD wControl, WORD wDestination,
                                         LONG lScale)
{
    GET_INST(instList, FALSE)
    enSAOutputSynthParam param; // Set from connection block

    return _SetSynthParam(inst, param, lScale);
}

#if 0
BOOL
SASE8210OutputDevice::SetSynthParam(DWORD dwParamID, DWORD dwValue)
{
   BOOL bSupportedParam = TRUE;

   paramValue[dwParamID] = dwValue;
   IDISABLE();
   for (instList.First(); !instList.EndOfList(); instList.Next())
   {
      BOOL bSetParam;
      DWORD dwTmpValue;
      SASE8210InstanceInfo *inst;

      inst = instList.GetCurItem();
      switch(dwParamID) {
      case sepEffectsSendA:
      case sepEffectsSendB:
      case sepEffectsSendC:
      case sepEffectsSendD:
      case sepRouteEffectsSendA:
      case sepRouteEffectsSendB:
      case sepRouteEffectsSendC:
      case sepRouteEffectsSendD:
         bSetParam = TRUE;
         break;
      case sepDelayModLFO:
      case sepFreqModLFO:
      case sepDelayVibLFO:
      case sepFreqVibLFO:
      case sepModLFOToPitch:
      case sepVibLFOToPitch:
      case sepModLFOToFilterFc:
      case sepModLFOToVolume:
      case sepDelayModEnv:
      case sepAttackModEnv:
      case sepHoldModEnv:
      case sepDecayModEnv:
      case sepSustainModEnv:
      case sepReleaseModEnv:
      case sepDelayVolEnv:
      case sepAttackVolEnv:
      case sepHoldVolEnv:
      case sepDecayVolEnv:
      case sepSustainVolEnv:
      case sepReleaseVolEnv:
      case sepInitialFilterFc:
      case sepModEnvToPitch:
      case sepModEnvToFilterFc:
      case sepInitialFilterQ:
      case sepInitialVolume:
      case sepInitialPitch:
         bSetParam = !inst->bOptimalPlayback;
         break;
/*
	  sepDisableEnv
	  sepRawPitch
	  sepRawFilter
	  sepRawVolume
	  sepStartAddrs
	  sepStartloopAddrs
	  sepEndloopAddrs
	  sepCurrentPitch
	  sepFraction
	  sepCurrentVolume
	  sepCurrentFilter
	  sepFilterDelayMemory1
	  sepFilterDelayMemory2
	  sepForceZeroPitch
	  sepInterpolationROM
	  sepEightBitSample
	  sepSampleMemoryHandle
	  sepPriority
	  sepStereo
	  sepOneShot
*/
      default:
         bSetParam = FALSE;
         bSupportedParam = FALSE;
      };

      if (bSetParam)
      {
         inst->paramValue[dwParamID] = dwValue;
         if (dwParamID == sepInitialPitch)
            dwTmpValue = CompSampleRate(inst->dwSampleRate);
         else
            dwTmpValue = dwValue;

         seParameterWrite((SEVOICE)inst->voiceHandle[0], dwParamID,
                          dwTmpValue);
         if (inst->byNumChans > 1)
            seParameterWrite((SEVOICE)inst->voiceHandle[1], dwParamID,
                             dwTmpValue);
      }
   }
   IENABLE();

   return bSupportedParam;
}


BOOL
SASE8210OutputDevice::SetSynthParam(DWORD devHandle, DWORD dwParamID,
                                    DWORD dwValue)
{
   BOOL bSetParam;
   DWORD dwTmpValue, dwTmpParamID;
   SASE8210InstanceInfo *inst;

   IDISABLE();
   instList.Find((SEVOICE)devHandle);
   if (instList.EndOfList())
   {
      IENABLE();
      return FALSE;
   }
   inst = instList.GetCurItem();
   IENABLE();

   switch(dwParamID) {
   case sepEffectsSendA:
   case sepEffectsSendB:
   case sepEffectsSendC:
   case sepEffectsSendD:
   case sepRouteEffectsSendA:
   case sepRouteEffectsSendB:
   case sepRouteEffectsSendC:
   case sepRouteEffectsSendD:
   case sepEightBitSample:
   case sepInitialPitch:
      bSetParam = TRUE;
      break;
   case sepDelayModLFO:
   case sepFreqModLFO:
   case sepDelayVibLFO:
   case sepFreqVibLFO:
   case sepModLFOToPitch:
   case sepVibLFOToPitch:
   case sepModLFOToFilterFc:
   case sepModLFOToVolume:
   case sepDelayModEnv:
   case sepAttackModEnv:
   case sepHoldModEnv:
   case sepDecayModEnv:
   case sepSustainModEnv:
   case sepReleaseModEnv:
   case sepDelayVolEnv:
   case sepAttackVolEnv:
   case sepHoldVolEnv:
   case sepDecayVolEnv:
   case sepSustainVolEnv:
   case sepReleaseVolEnv:
   case sepInitialFilterFc:
   case sepModEnvToPitch:
   case sepModEnvToFilterFc:
   case sepInitialFilterQ:
   case sepInitialVolume:
      bSetParam = !inst->bOptimalPlayback;
      break;
/*
   sepDisableEnv
   sepRawPitch
   sepRawFilter
   sepRawVolume
   sepStartAddrs
   sepStartloopAddrs
   sepEndloopAddrs
   sepCurrentPitch
   sepFraction
   sepCurrentVolume
   sepCurrentFilter
   sepFilterDelayMemory1
   sepFilterDelayMemory2
   sepForceZeroPitch
   sepInterpolationROM
   sepSampleMemoryHandle
   sepPriority
   sepStereo
   sepOneShot
*/
   default:
      bSetParam = FALSE;
   };

   if (bSetParam)
   {
      inst->paramValue[dwParamID] = dwTmpValue = dwValue;
      dwTmpParamID = dwParamID;
      if (dwParamID == sepInitialPitch)
      {
         inst->dwSampleRate = dwValue;
         if (inst->bOptimalPlayback)
         {
            dwTmpParamID = sepRawPitch;
            dwTmpValue = COMP_RAW_PITCH(dwValue);
         }
         else
            dwTmpValue = TransSampleRate(CompSampleRate(dwValue));
      }
      else if (dwParamID == sepEightBitSample)
      {
         inst->bySampleSize = (dwValue ? 1 : 2);
         inst->byFrameSize = inst->byNumChans*inst->bySampleSize;
      }
      else
         dwTmpValue = dwValue;

      if ((inst->byNumChans == 1) || (dwTmpParamID != sepEffectsSendB))
         seParameterWrite((SEVOICE)inst->voiceHandle[0], dwTmpParamID,
                          dwTmpValue);
      if ((inst->byNumChans > 1) && (dwTmpParamID != sepEffectsSendA))
         seParameterWrite((SEVOICE)inst->voiceHandle[1], dwTmpParamID,
                          dwTmpValue);
   }

   return bSetParam;
}
#endif

void
SASE8210OutputDevice::AddBuffer(DWORD devHandle, stSAOutputBuffer *buffer)
{
    GET_INST(instList, )

    _AddBuffer(inst, buffer);
}


void
SASE8210OutputDevice::_AddBuffer(SASE8210InstanceInfo *inst,
                                 stSAOutputBuffer *buffer)
{
   stSAOutputBuffer *newBuffer, *tmp;
   SMMAP m;
   stIPInfo ipInfo;
   DWORD dwCurLength;
   BOOL bTurnOnInterrupt = FALSE;

   DPRINTF(("OutputDevice::AddBuffer"));

   DPRINTF(("inst = 0x%lx", inst));

   if (inst->pCurBuffer)
       DPRINTF(("   In AddBuffer, pCurBuffer (0x%x) is %ld", inst->pCurBuffer, inst->pCurBuffer->dwUser3));
   else
       DPRINTF(("   In AddBuffer, pCurBuffer is NULL"));

   STORE_INST_DEBUG_WC(0x41646442)

   // Check for zero loops
   if (!inst->bPlaying || (inst->dwPausePos == 0))
   {
      stSAOutputBuffer *retBuffer, *retList = NULL;
      for (tmp = buffer; tmp != NULL;)
      {
         if (!inst->bInZeroLoop && (tmp->dwLoopFlags&SA_LOOP_START) &&
             (tmp->dwNumLoops == 0))
             inst->bInZeroLoop = TRUE;

         if (inst->bInZeroLoop)
         {
            stSAOutputBuffer *retBuffer;
            BOOL bEndLooping = tmp->dwLoopFlags&SA_LOOP_END;
            retBuffer = tmp;
            if (tmp == buffer)
               buffer = buffer->pNextBuffer;
            tmp = tmp->pNextBuffer;
            retBuffer->pNextBuffer = NULL;
            ADD_BUFFER_TO_LIST(retList, retBuffer);
            if (bEndLooping)
               inst->bInZeroLoop = FALSE;
         }
         else
            tmp = tmp->pNextBuffer;
      }

      // Return the buffers
      while (retList != NULL)
      {
         retBuffer = retList;
         retList = retList->pNextBuffer;
         if (retBuffer->dwReserved != 0)
         {
            DeleteAlloc((void *)(retBuffer->dwReserved));
            retBuffer->dwReserved = 0;
         }
         inst->callback(retBuffer);
      }

      // If we removed all the buffer, just return because we have nothing
      // to play
      if (buffer == NULL)
         return;
   }

   // Get physical pages for the buffers
   for (tmp = buffer; tmp != NULL; tmp = tmp->pNextBuffer)
   {
      DPRINTF(("   Adding buffer 0x%lx, %ld", tmp, tmp->dwUser3));
      if (tmp->dwReserved == 0)
      {
         DWORD dwVirtAddr = PAGE_ALIGN((DWORD)tmp->virtAddr);
         DWORD dwNumPages = PAGE_ALIGN_UP(tmp->dwSize + (DWORD)tmp->virtAddr -
                                          dwVirtAddr)/SM_PAGE_SIZE;
         tmp->dwReserved = (DWORD)NewAlloc(dwNumPages*sizeof(OSPHYSADDR));
         osLockVirtualRange((OSVIRTADDR)dwVirtAddr, dwNumPages, dwNumPages,
                            (OSPHYSADDR *)(tmp->dwReserved));
     		osUnlockVirtualRange((OSVIRTADDR)dwVirtAddr, dwNumPages);
      }
   }

   IDISABLE();
   if ((inst->pTmpLastBuffer != NULL) || (inst->pRetLastBuffer != NULL))
   {
       STORE_INST_DEBUG_WC(0x41424c42)

       ASSERT(inst->pCurBuffer == NULL);
       ADD_BUFFER_TO_LIST(inst->bufferList, buffer);
       IENABLE();
       return;
   }

   if (inst->bPlaying)
   {
      // Start playback
      m.dwFlags = SMMAP_PHYSICAL;
      if (inst->pCurBuffer == NULL)
      {
         STORE_INST_DEBUG_WC(0x41423100)

         if ((buffer->dwLoopFlags & SA_STATIC) &&
             (buffer->dwSize < MAX_BUFFER_SIZE*2))
         {
            // Static buffer
            ASSERT(inst->bufferList == NULL);
            ASSERT(buffer->pNextBuffer == NULL);
            ASSERT((buffer->dwLoopFlags & SA_LOOP_START) &&
                   (buffer->dwLoopFlags & SA_LOOP_END));
            ASSERT((buffer->dwNumLoops == 1) ||
                   (buffer->dwNumLoops == SA_LOOP_FOREVER));

            inst->loopState = ensalsStaticLoop;
            if (inst->dwLoopCount == 0)
               inst->dwLoopCount = buffer->dwNumLoops;
            inst->pCurBuffer = buffer;
            inst->bCurBufferInFirstHalf = TRUE;
            newBuffer = NULL;
            m.pBaseAddr = (BYTE *)PAGE_ALIGN((DWORD)inst->pCurBuffer->virtAddr);
            inst->dwCurBufferOffset = (DWORD)inst->pCurBuffer->virtAddr -
                                      (DWORD)m.pBaseAddr;
            m.dwLength = PAGE_ALIGN_UP(inst->pCurBuffer->dwSize +
                                       inst->dwCurBufferOffset);
            m.dwOffset = 0;
            m.pdwPhysicalPageList = (DWORD *)inst->pCurBuffer->dwReserved;
            if (smMemoryMap(inst->smHandle, &m) != SUCCESS)
               ASSERT(0);

            // Setup the loop addresses
            seParameterWrite(inst->voiceHandle[0], sepStartAddrs,
                             (inst->dwPausePos +
                              inst->dwCurBufferOffset)/inst->byFrameSize);
            seParameterWrite(inst->voiceHandle[0], sepEndloopAddrs,
                             (inst->pCurBuffer->dwSize +
                              inst->dwCurBufferOffset)/inst->byFrameSize);
            inst->dwPausePos = 0;

            if (inst->dwLoopCount == SA_LOOP_FOREVER)
               seParameterWrite(inst->voiceHandle[0], sepStartloopAddrs,
                                inst->dwCurBufferOffset/inst->byFrameSize);
            else
            {
               UpdateNextRegion(inst);

               if (inst->bIsSynchronized) {
                   bTurnOnInterrupt = TRUE;
               } else {
                   // Register the interrupt
                   ipInfo.type = IP_ONLOOP;
                   ipInfo.interruptParameter = inst->voiceHandle[0];
                   ipInfo.userParameter = (DWORD)this;
                   ipInfo.fISRHandler = ISRCallback;
                   ipInfo.fHandler = Callback;
                   if (ipRegisterCallback(ipid, &ipInfo, &inst->ipHandle) != SUCCESS)
                      ASSERT(0);
               }
            }
          }
         else
         {
            // Setup the first buffer
            DPRINTF(("   AddBuffer Setup first buffer %ld", buffer->dwUser3));
            inst->pCurBuffer = buffer;
            inst->bufferList = buffer->pNextBuffer;
            newBuffer = NULL;
            inst->pCurBuffer->pNextBuffer = NULL;
            inst->bCurBufferInFirstHalf = TRUE;
         
            m.pBaseAddr = (BYTE *)PAGE_ALIGN((DWORD)inst->pCurBuffer->virtAddr +
                                             inst->dwCurPos);
            inst->dwCurBufferOffset = (DWORD)inst->pCurBuffer->virtAddr +
                                      inst->dwCurPos - (DWORD)m.pBaseAddr;
            if (inst->pCurBuffer->dwSize-inst->dwCurPos > MAX_BUFFER_SIZE)
               dwCurLength = MAX_BUFFER_SIZE;
            else
               dwCurLength = inst->pCurBuffer->dwSize-inst->dwCurPos;
            m.dwLength = PAGE_ALIGN_UP(dwCurLength+inst->dwCurBufferOffset);
            m.dwOffset = 0;
            m.pdwPhysicalPageList = (DWORD *)inst->pCurBuffer->dwReserved +
               inst->dwCurPos/SM_PAGE_SIZE;
            if (smMemoryMap(inst->smHandle, &m) != SUCCESS)
               ASSERT(0);
         
            // Map the next region
            UpdateNextRegion(inst);

            // Setup the loop addresses
            seParameterWrite(inst->voiceHandle[0], sepStartAddrs,
                             (inst->dwPausePos +
                              inst->dwCurBufferOffset)/inst->byFrameSize);
            seParameterWrite(inst->voiceHandle[0], sepEndloopAddrs,
                             (dwCurLength+inst->dwCurBufferOffset)/inst->byFrameSize);
            inst->dwPausePos = 0;

            // Register the interrupt and start the voice
            if (inst->bIsSynchronized) {
                bTurnOnInterrupt = TRUE;
            } else {
                ipInfo.type = IP_ONLOOP;
                ipInfo.interruptParameter = inst->voiceHandle[0];
                ipInfo.userParameter = (DWORD)this;
                ipInfo.fISRHandler = ISRCallback;
                ipInfo.fHandler = Callback;

                if (!inst->bInCallback)
                   if (ipRegisterCallback(ipid, &ipInfo, &inst->ipHandle) != SUCCESS)
                      ASSERT(0);

                bTurnOnInterrupt = FALSE;
            }
         }

         if (seVoiceStart(inst->byNumChans, inst->voiceHandle) != SUCCESS)
            ASSERT(0);

         if (inst->bIsSynchronized && bTurnOnInterrupt) {
             ipInfo.type = IP_ONLOOP;
             ipInfo.interruptParameter = inst->voiceHandle[0];
             ipInfo.userParameter = (DWORD)this;
             ipInfo.fISRHandler = ISRCallback;
             ipInfo.fHandler = Callback;

             if (!inst->bInCallback)
                if (ipRegisterCallback(ipid, &ipInfo, &inst->ipHandle) != SUCCESS)
                    ASSERT(0);

             bTurnOnInterrupt = FALSE;
         }

      }
      else if ((inst->bufferList == NULL) &&
               (inst->dwCurPos+MAX_BUFFER_SIZE >= inst->pCurBuffer->dwSize))
      {
         STORE_INST_DEBUG_WC(0x41423200)

         // If this is the second buffer, remap so we play this one instead
         // of the empty buffer
//         IDISABLE();
         inst->bufferList = buffer;
         newBuffer = NULL;

         UpdateNextRegion(inst);
//         IENABLE();
      }
      else { 
         DPRINTF(("  Still playing, queuing buffer %ld", buffer->dwUser3)); 
         newBuffer = buffer;
      }
   }
   else {
      DPRINTF(("  Not playing, queuing buffer %ld", buffer->dwUser3));
      newBuffer = buffer;
   }

   // Put the rest of the buffers on the list
   if (newBuffer != NULL) {
//      IDISABLE();
      ADD_BUFFER_TO_LIST(inst->bufferList, newBuffer);
//      IENABLE();
   }
   IENABLE();
   DPRINTF(("  At end of AddBuffer, pCurBuffer (0x%x) is %ld", inst->pCurBuffer, inst->pCurBuffer->dwUser3));

   if (inst->bufferList) 
       DPRINTF(("  At end of AddBuffer, first thing on bufferList is %ld", 
                inst->bufferList->dwUser3));
}


void
SASE8210OutputDevice::Play(DWORD devHandle)
{
   stSAOutputBuffer *tmp;

   DPRINTF(("OutputDevice::Play"));
   GET_INST(instList, )

   STORE_INST_DEBUG_WC(0x506c6179)

   if (inst->bPlaying)
      return;
         
   inst->bPlaying = TRUE;
   if (inst->bufferList != NULL)
   {
      tmp = inst->bufferList;
      inst->bufferList = NULL;
      DPRINTF(("   In Play, invoking AddBuffer to start output"));
      _AddBuffer(inst, tmp);
   }
}


void
SASE8210OutputDevice::Pause(DWORD devHandle)
{
   DPRINTF(("OutputDevice::Pause"));

   DWORD dwTmpPos = seParameterRead((SEVOICE)devHandle, sepStartAddrs);

   GET_INST(instList, )

   inst->bPlaying = FALSE;
   if (inst->pCurBuffer == NULL)
      return;

   seVoiceStop(inst->byNumChans, inst->voiceHandle);
   if ((inst->loopState != ensalsStaticLoop) ||
       (inst->dwLoopCount != SA_LOOP_FOREVER))
   {
      ipUnregisterCallback(inst->ipHandle);
      inst->ipHandle = 0;
   }
   ASSERT(inst->ipHandle == NULL);

   STORE_INST_DEBUG_WC(0x50617573)

   inst->dwPausePos = dwTmpPos*inst->byFrameSize -
                      (inst->dwCurBufferOffset +
                       (inst->bCurBufferInFirstHalf ? 0 : SM_BUFFER_SIZE));
   if (inst->bySampleSize == 1)
   {
       if (inst->dwPausePos >= (DWORD)(61*inst->byFrameSize))
           inst->dwPausePos -= 61*inst->byFrameSize;
       else if (inst->loopState == ensalsStaticLoop)
           inst->dwPausePos += inst->pCurBuffer->dwSize - 61*inst->byFrameSize;
       else
           inst->dwPausePos = 0;
   }
   else
   {
       if (inst->dwPausePos >= (DWORD)(29*inst->byFrameSize))
           inst->dwPausePos -= 29*inst->byFrameSize;
       else if (inst->loopState == ensalsStaticLoop)
           inst->dwPausePos += inst->pCurBuffer->dwSize - 29*inst->byFrameSize;
       else
           inst->dwPausePos = 0;
   }

   inst->pCurBuffer->pNextBuffer = inst->bufferList;
   inst->bufferList = inst->pCurBuffer;
   inst->pCurBuffer = NULL;
}


void
SASE8210OutputDevice::Stop(DWORD devHandle, stSAOutputBuffer **buffer)
{
    GET_INST_NOENABLE(instList, )

    _Stop(inst, buffer);
}

void
SASE8210OutputDevice::_Stop(SASE8210InstanceInfo *inst, stSAOutputBuffer **buffer)
{
   DPRINTF(("OutputDevice::Stop"));

   if ((inst->pCurBuffer != NULL) || (inst->pTmpLastBuffer != NULL) ||
       (inst->pRetLastBuffer != NULL))
   {
      seVoiceStop(inst->byNumChans, inst->voiceHandle);
      if ((inst->loopState != ensalsStaticLoop) ||
          (inst->dwLoopCount != SA_LOOP_FOREVER))
      {
         ipUnregisterCallback(inst->ipHandle);
         inst->ipHandle = 0;
      }
   }
   IENABLE();
   ASSERT(inst->bInCallback || (inst->ipHandle == NULL));

   STORE_INST_DEBUG_WC(0x53746f70)

   inst->dwPausePos = inst->dwPlayPos = inst->dwCurPos = 0;
   inst->bPlaying = FALSE;
   inst->loopState = ensalsNotLooping;
   inst->dwLoopCount = 0;

   stSAOutputBuffer *tmpbuf = NULL, **tmp = &tmpbuf;

   if (inst->pRetLastBuffer != NULL)
   {
       if (inst->pRetLastBuffer->dwReserved != 0)
       {
           DeleteAlloc((void *)(inst->pRetLastBuffer->dwReserved));
           inst->pRetLastBuffer->dwReserved = 0;
       }
       *tmp = inst->pRetLastBuffer;
       tmp  = &(*tmp)->pNextBuffer;
   }

   if (inst->pTmpLastBuffer != NULL)
   {
       if (inst->pTmpLastBuffer->dwReserved != 0)
       {
           DeleteAlloc((void *)(inst->pTmpLastBuffer->dwReserved));
           inst->pTmpLastBuffer->dwReserved = 0;
       }
       *tmp = inst->pTmpLastBuffer;
       tmp  = &(*tmp)->pNextBuffer;
   }

   if (inst->pCurBuffer != NULL)
   {
       if (inst->pCurBuffer->dwReserved != 0)
       {
           DeleteAlloc((void *)(inst->pCurBuffer->dwReserved));
           inst->pCurBuffer->dwReserved = 0;
       }
      *tmp = inst->pCurBuffer;
      tmp = &(*tmp)->pNextBuffer;
   }

   while (inst->pLoopBufferList != NULL)
   {
       if (inst->pLoopBufferList->dwReserved != 0)
       {
           DeleteAlloc((void *)(inst->pLoopBufferList->dwReserved));
           inst->pLoopBufferList->dwReserved = 0;
       }
      *tmp = inst->pLoopBufferList;
      inst->pLoopBufferList = inst->pLoopBufferList->pNextBuffer;
      tmp = &(*tmp)->pNextBuffer;
   }

   while (inst->bufferList != NULL)
   {
       if (inst->bufferList->dwReserved != 0)
       {
           DeleteAlloc((void *)(inst->bufferList->dwReserved));
           inst->bufferList->dwReserved = 0;
       }
      *tmp = inst->bufferList;
      inst->bufferList = inst->bufferList->pNextBuffer;
      tmp = &(*tmp)->pNextBuffer;
   }

   if (buffer != NULL)
       *buffer = tmpbuf;

#ifdef DEBUG
   // Dump all of the buffers on the final list
   if (buffer != NULL) {
      stSAOutputBuffer *pCurrBuff = *buffer;
      while (pCurrBuff) {
         DPRINTF(("  Stopping with buffer %ld", pCurrBuff->dwUser3));
         pCurrBuff = pCurrBuff->pNextBuffer;
      }
   }
#endif

   inst->pRetLastBuffer = NULL;
   inst->pTmpLastBuffer = NULL;
   inst->pCurBuffer = NULL;
   inst->bufferList = NULL;
}

BOOL
SASE8210OutputDevice::HasBuffersQueued(DWORD devHandle)
{
   GET_INST(instList, FALSE)

   if (inst->bPlaying)
      return (inst->pCurBuffer != NULL);
   else
      return (inst->bufferList != NULL);
}

DWORD
SASE8210OutputDevice::GetCurrentSampleFrame(DWORD devHandle)
{
   DWORD ret;

   DPRINTF(("OutputDevice::GetCurrentSampleFrame"));

   GET_INST_NOENABLE(instList, 0)

   if (inst->pCurBuffer) 
       DPRINTF(("  In GCSF, pCurBuffer %ld", inst->pCurBuffer->dwUser3));

   if ((inst->pCurBuffer != NULL) || (inst->pTmpLastBuffer != NULL))
   {
      DWORD pos = seParameterRead((SEVOICE)devHandle, sepStartAddrs);
      if (((inst->loopState == ensalsStaticLoop) &&
           (pos < (DWORD)(MAX_BUFFER_SIZE*2/inst->byFrameSize))) ||
          ((inst->loopState != ensalsStaticLoop) &&
           ((pos < (DWORD)SM_BUFFER_SIZE/inst->byFrameSize) &&
            inst->bCurBufferInFirstHalf) ||
           ((pos >= (DWORD)SM_BUFFER_SIZE/inst->byFrameSize) &&
            !inst->bCurBufferInFirstHalf)))
      {
         // Normal condition
         ret = (LONG)pos +
               ((LONG)inst->dwPlayPos -
                (LONG)inst->dwCurBufferOffset -
                (inst->bCurBufferInFirstHalf ? 0 : SM_BUFFER_SIZE))/
                   inst->byFrameSize;
      }
      else
      {
         // We've looped but the ISR hasn't been called yet
         if (inst->loopState == ensalsStaticLoop)
            ret = (LONG)pos +
               ((LONG)inst->dwPlayPos -
                (MAX_BUFFER_SIZE*2+SM_PAGE_SIZE) -
                (LONG)inst->dwNextBufferOffset)/inst->byFrameSize;
         else if (inst->pCurBuffer == NULL)
            ret = (LONG)pos +
               ((LONG)inst->dwPlayPos -
                (LONG)inst->dwNextBufferOffset -
                (inst->bCurBufferInFirstHalf ? SM_BUFFER_SIZE : 0))/
                   inst->byFrameSize;
         else
            ret = (LONG)pos +
               ((LONG)inst->dwPlayPos +
                ((inst->dwCurPos + MAX_BUFFER_SIZE >= inst->pCurBuffer->dwSize) ?
                 ((LONG)inst->pCurBuffer->dwSize - (LONG)inst->dwCurPos) :
                 MAX_BUFFER_SIZE) -
                (LONG)inst->dwNextBufferOffset -
                (inst->bCurBufferInFirstHalf ? SM_BUFFER_SIZE : 0))/
                   inst->byFrameSize;
      }

      if (inst->bySampleSize == 1)
      {
         if (ret >= 61)
            ret -= 61;
         else if (inst->loopState == ensalsStaticLoop)
            ret += inst->pCurBuffer->dwSize/inst->byFrameSize - 61;
         else
            ret = 0;
      }
      else
      {
         if (ret >= 29)
            ret -= 29;
         else if (inst->loopState == ensalsStaticLoop)
            ret += inst->pCurBuffer->dwSize/inst->byFrameSize - 29;
         else
            ret = 0;
      }
   }
   else
      ret = ((inst->dwPausePos + inst->dwPlayPos)/inst->byFrameSize);

   if ((inst->pTmpLastBuffer != NULL) && (ret > inst->dwPlayPos/inst->byFrameSize))
       ret = inst->dwPlayPos/inst->byFrameSize;
   IENABLE();

   if (inst->pCurBuffer)
       DPRINTF(("  At end of GCSF, pCurBuffer %ld", inst->pCurBuffer->dwUser3));

   if (inst->bufferList)
       DPRINTF(("  At end, bufferList %ld", inst->bufferList->dwUser3));

   DPRINTF(("OutputDevice::GetCurrentSampleFrame returned %ld", ret));
   return ret;
}


void
SASE8210OutputDevice::SetCurrentSampleFrame(DWORD devHandle,
                                            DWORD dwNewSampleFrame)
{
   DWORD dwNewPos;

   DPRINTF(("OutputDevice::SetCurrentSampleFrame"));

   GET_INST_NOENABLE(instList, )

   dwNewPos = dwNewSampleFrame*inst->byFrameSize;
   if (inst->pCurBuffer != NULL)
   {

      if ((inst->loopState == ensalsStaticLoop) ||
          ((inst->dwCurPos <= dwNewPos) &&
           ((inst->dwCurPos + MAX_BUFFER_SIZE >= inst->pCurBuffer->dwSize) ?
            inst->pCurBuffer->dwSize : (inst->dwCurPos + MAX_BUFFER_SIZE) >
              dwNewPos)))
      {
         // Loop within the same buffer (or part of a buffer)
         DWORD dwCurPos, dwStartLoopPos, dwEndLoopPos;

         // Get the current loop points
         dwStartLoopPos = seParameterRead((SEVOICE)devHandle, sepStartloopAddrs);
         dwEndLoopPos = seParameterRead((SEVOICE)devHandle, sepEndloopAddrs);

         // Unregister the callback (if necessary)
         if ((inst->loopState != ensalsStaticLoop) ||
             (inst->dwLoopCount != SA_LOOP_FOREVER))
            ipUnregisterCallback(inst->ipHandle);

         // Loop to the new position
         seParameterWrite((SEVOICE)devHandle, sepStartloopAddrs, dwNewSampleFrame);
         dwCurPos = seParameterRead((SEVOICE)devHandle, sepStartAddrs);
         seParameterWrite((SEVOICE)devHandle, sepEndloopAddrs, dwCurPos);

         // Wait for loop
         halWaitWallClockCounts(halid, 2);

         // Reset loop points
         seParameterWrite((SEVOICE)devHandle, sepStartloopAddrs, dwStartLoopPos);
         seParameterWrite((SEVOICE)devHandle, sepEndloopAddrs, dwEndLoopPos);

         // Reregister the callback (if necessary)
         if ((inst->loopState != ensalsStaticLoop) ||
             (inst->dwLoopCount != SA_LOOP_FOREVER))
         {
            stIPInfo ipInfo;

            ipInfo.type = IP_ONLOOP;
            ipInfo.interruptParameter = (SEVOICE)devHandle;
            ipInfo.userParameter = (DWORD)this;
            ipInfo.fISRHandler = ISRCallback;
            ipInfo.fHandler = Callback;
            if (ipRegisterCallback(ipid, &ipInfo, &inst->ipHandle) != SUCCESS)
               ASSERT(0);
         }
      }
      else
      {
         // Loop to a different part of the buffer
         SMMAP m;
         DWORD dwCurPos, dwCurLength;
         stIPInfo ipInfo;

         // Unregister the callback
         ipUnregisterCallback(inst->ipHandle);

         // Remap the new part of the buffer
         inst->dwPlayPos += (dwNewPos - dwNewPos%MAX_BUFFER_SIZE) - inst->dwCurPos;
         inst->dwCurPos = dwNewPos - dwNewPos%MAX_BUFFER_SIZE;
         inst->bCurBufferInFirstHalf = !inst->bCurBufferInFirstHalf;
         m.dwFlags = SMMAP_PHYSICAL;
         m.pBaseAddr = (BYTE *)PAGE_ALIGN((DWORD)inst->pCurBuffer->virtAddr+
                                          inst->dwCurPos);
         inst->dwCurBufferOffset = (DWORD)inst->pCurBuffer->virtAddr+
                                   inst->dwCurPos-(DWORD)m.pBaseAddr;
         if (inst->dwCurPos + MAX_BUFFER_SIZE < inst->pCurBuffer->dwSize)
            dwCurLength = MAX_BUFFER_SIZE;
         else
            dwCurLength = inst->pCurBuffer->dwSize-inst->dwCurPos;
         m.dwLength = PAGE_ALIGN_UP(dwCurLength+inst->dwCurBufferOffset);
         m.pdwPhysicalPageList = (DWORD *)inst->pCurBuffer->dwReserved +
                                 inst->dwCurPos/SM_PAGE_SIZE;
         m.dwOffset = (inst->bCurBufferInFirstHalf ? 0 : SM_BUFFER_SIZE);

         if (smMemoryMap(inst->smHandle, &m) != SUCCESS)
            ASSERT(0);

         // Loop to the new position
         seParameterWrite(inst->voiceHandle[0], sepStartloopAddrs,
                          ((inst->bCurBufferInFirstHalf ? 0 : SM_BUFFER_SIZE) +
                          inst->dwCurBufferOffset + dwNewPos%MAX_BUFFER_SIZE)/
                             inst->byFrameSize);
         dwCurPos = seParameterRead((SEVOICE)devHandle, sepStartAddrs);
         seParameterWrite((SEVOICE)devHandle, sepEndloopAddrs, dwCurPos);

         // Wait for loop
         halWaitWallClockCounts(halid, 2);

         // Update next region
         UpdateNextRegion(inst);
         seParameterWrite((SEVOICE)devHandle, sepEndloopAddrs,
                          ((inst->bCurBufferInFirstHalf ? 0 : SM_BUFFER_SIZE) +
                           dwCurLength+inst->dwCurBufferOffset)/inst->byFrameSize);

         // Reregister the callback
         ipInfo.type = IP_ONLOOP;
         ipInfo.interruptParameter = (SEVOICE)devHandle;
         ipInfo.userParameter = (DWORD)this;
         ipInfo.fISRHandler = ISRCallback;
         ipInfo.fHandler = Callback;
         if (ipRegisterCallback(ipid, &ipInfo, &inst->ipHandle) != SUCCESS)
            ASSERT(0);
      }
   }
   else
   {
      // Just update the pause position
      if (inst->bufferList != NULL)
      {
         if (inst->loopState == ensalsStaticLoop)
            inst->dwPausePos = dwNewPos;
         else
         {
            inst->dwPausePos = dwNewPos%MAX_BUFFER_SIZE;
            inst->dwPlayPos += (dwNewPos - dwNewPos%MAX_BUFFER_SIZE) - inst->dwCurPos;
            inst->dwCurPos = dwNewPos - dwNewPos%MAX_BUFFER_SIZE;
         }
      }
   }

   IENABLE();
}


void
SASE8210OutputDevice::Close(DWORD devHandle)
{
   DPRINTF(("OutputDevice::Close"));

   GET_INST_NOENABLE(instList, )
   IDISABLE();

   _Stop(inst, NULL);
   
   seVoiceFree(inst->byNumChans, inst->voiceHandle);
   if (smMemoryFree(inst->smHandle) != SUCCESS)
      ASSERT(0);

   instList.DeleteCurItem();
   IENABLE();
}


void
SASE8210OutputDevice::SetLoopCount(DWORD devHandle, DWORD dwLoopCount)
{
    GET_INST_NOENABLE(instList, )

   if (inst->loopState == ensalsStaticLoop)
   {
      if (dwLoopCount == SA_LOOP_FOREVER)
      {
         if (inst->dwLoopCount == 1)
         {
            if (inst->pCurBuffer == NULL)
            {
               if (inst->bufferList != NULL)
                   inst->dwLoopCount = SA_LOOP_FOREVER;
            }
            else
            {
               inst->dwLoopCount = SA_LOOP_FOREVER;
               ipUnregisterCallback(inst->ipHandle);
               inst->ipHandle = 0;
               seParameterWrite((SEVOICE)devHandle, sepStartloopAddrs,
                                (inst->dwCurBufferOffset)/inst->byFrameSize);
            }
         }
      }
      else if (dwLoopCount == 1)
      {
         if (inst->dwLoopCount == SA_LOOP_FOREVER)
         {
            if (inst->pCurBuffer == NULL)
            {
               if (inst->bufferList != NULL)
                   inst->dwLoopCount = 1;
            }
            else
            {
               stIPInfo ipInfo;

               inst->dwLoopCount = 1;
               UpdateNextRegion(inst);
               // Register the interrupt
               ipInfo.type = IP_ONLOOP;
               ipInfo.interruptParameter = (SEVOICE)devHandle;
               ipInfo.userParameter = (DWORD)this;
               ipInfo.fISRHandler = ISRCallback;
               ipInfo.fHandler = Callback;
               if (ipRegisterCallback(ipid, &ipInfo, &inst->ipHandle) != SUCCESS)
                  ASSERT(0);
            }
         }
      }
      else
          ASSERT(0);
   }
   else if (inst->loopState != ensalsNotLooping)
   {
      inst->dwLoopCount = dwLoopCount;
      if (((inst->pCurBuffer != NULL) &&
           (inst->pCurBuffer->dwLoopFlags&SA_LOOP_END)) ||
          ((inst->pCurBuffer == NULL) && (inst->bufferList != NULL) &&
           (inst->bufferList->dwLoopFlags&SA_LOOP_END)))
         inst->loopState = ensalsLooping1;

      if (inst->pCurBuffer != NULL)
         UpdateNextRegion(inst);
   }
   IENABLE();
}


void
SASE8210OutputDevice::SetSampleRate(DWORD devHandle, DWORD dwSampleRate)
{
    GET_INST(instList, )

    if (inst->playbackMode == sapmNoControl)
        return;

    inst->dwSampleRate = dwSampleRate;
    if (inst->playbackMode == sapmControlFrequency)
    {
        seParameterWrite(inst->voiceHandle[0], sepRawPitch,
                         COMP_RAW_PITCH(inst->dwSampleRate));
        if (inst->byNumChans > 1)
            seParameterWrite(inst->voiceHandle[1], sepRawPitch,
                             COMP_RAW_PITCH(inst->dwSampleRate));
    }
    else
    {
        SAOutputSampleData sampdata;

        sampdata.dwSamplesPerSec = inst->dwSampleRate;
        inst->trans.SetSoundParameters(&sampdata, FALSE);
        inst->trans.SetParameterAndTranslate(saospPitch, inst->lPitch, FALSE);
        seParameterWrite(inst->voiceHandle[0], sepInitialPitch,
            inst->xlatapp.remveDestIndex[sepInitialPitch](inst->trans.GetDestParameter(sepInitialPitch)));
        if (inst->byNumChans > 1)
            seParameterWrite(inst->voiceHandle[1], sepInitialPitch,
                inst->xlatapp.remveDestIndex[sepInitialPitch](inst->trans.GetDestParameter(sepInitialPitch)));
    }
}


DWORD
SASE8210OutputDevice::GetSampleRate(DWORD devHandle)
{
    GET_INST(instList, 0)

    return inst->dwSampleRate;
}


BYTE 
SASE8210OutputDevice::GetFrameSize(DWORD devHandle)
{
    GET_INST(instList, 0)

    return inst->byFrameSize;
}


BYTE 
SASE8210OutputDevice::GetNumChans(DWORD devHandle)
{
    GET_INST(instList, 0)

    return inst->byNumChans;
}


BYTE 
SASE8210OutputDevice::GetSampleSize(DWORD devHandle)
{
    GET_INST(instList, 0)
   
    return inst->bySampleSize;
}

#if 0
DWORD
SASE8210OutputDevice::CompSampleRate(DWORD dwSampleRate)
{
   DWORD ret;
   WORD wIntPart = (WORD)(paramValue[sepInitialPitch] >> 16)&0xffff;
   WORD wFracPart = (WORD)paramValue[sepInitialPitch]&0xffff;

   ret = (dwSampleRate*wIntPart + dwSampleRate*wFracPart/0x10000);
   if (ret > 191985)
      ret = 191985;

   return ret;
}


WORD
SASE8210OutputDevice::TransSampleRate(DWORD dwSampleRate)
{
   DWORD sampleRate;
   WORD count;
   
   sampleRate = dwSampleRate * SCALE_MASTER_CLOCK; /* Scale 44100 or 48000 to 0x200009b8 */
   for (count = 31; count > 0; count --)
   {
      if (sampleRate & 0x80000000)     /* Detect leading "1" */
         return (WORD)((((LONG)(count-15)<<20) +
                logMagTable[0x7f&(sampleRate>>24)] +
                (0x7f&(sampleRate>>17)) *
                logSlopeTable[0x7f&(sampleRate>>24)]) >> 8);
      sampleRate = sampleRate << 1;
   }
   return 0;
}
#endif

BOOL
SASE8210OutputDevice::ISRCallback(IPHANDLE ipHandle, enIPType type,
                                  DWORD userParam, unIPReply *pReply)
{
   SASE8210InstanceInfo *inst;
   SASE8210OutputDevice *outdev = (SASE8210OutputDevice *)userParam;
   stSAOutputBuffer *retBuffer;
   DWORD devHandle = (DWORD)pReply->voiceHandle;
   DWORD dwCurLength;
   DWORD dwCurPos, dwEndLoopPos;

   DPRINTF(("OutputDevice::ISRCallback"));

   // Figure out which instance we're in
   outdev->instList.Find((SEVOICE)devHandle);
   if (outdev->instList.EndOfList())
      return FALSE;      
   inst = outdev->instList.GetCurItem();

   DPRINTF(("inst = 0x%lx", inst));
   if (inst->pCurBuffer)
      DPRINTF(("   In ISR, pCurBuff (0x%lx) = %ld", inst->pCurBuffer, inst->pCurBuffer->dwUser3));

   STORE_INST_DEBUG_WC(0x49535243)
   STORE_INST_DEBUG_VAL(0x54797065, type)
   STORE_INST_DEBUG_PTRREG(0x43410000, QKBCA|((devHandle+8)&0x3f))
   STORE_INST_DEBUG_PTRREG(0x454c0000, SDL|((devHandle+8)&0x3f))
   STORE_INST_DEBUG_PTRREG(0x534c0000, SCSA|((devHandle+8)&0x3f))

   // If we're already in the zero buffer, go to the callback to stop the voice
   if (inst->pTmpLastBuffer != NULL)
   {
      DPRINTF(("  In ISR, pRetLastBuffer is NULL.  Returning."));
      STORE_INST_DEBUG_WC(0x49433100)

	  inst->pRetLastBuffer = inst->pTmpLastBuffer;
	  inst->pTmpLastBuffer = NULL;
      return TRUE;
   }

   // Check to see if we've played through the entire current buffer
   inst->dwCurPos += MAX_BUFFER_SIZE;
   if ((inst->loopState == ensalsStaticLoop) ||
       (inst->dwCurPos >= inst->pCurBuffer->dwSize))
   {
      STORE_INST_DEBUG_WC(0x49433200)

      inst->dwPlayPos += inst->pCurBuffer->dwSize -
                         (inst->dwCurPos - MAX_BUFFER_SIZE);
      inst->dwCurPos = 0;
      // Set up for the next buffer
      retBuffer = inst->pCurBuffer;
      retBuffer->pNextBuffer = NULL;

      // Check for looping
      if (inst->loopState != ensalsNotLooping)
      {
         // First, update the loop count
         if (retBuffer->dwLoopFlags&SA_LOOP_END)
            inst->dwLoopCount--;

         // Then, put the buffer on the end of the loop buffer list
         ADD_BUFFER_TO_LIST(inst->pLoopBufferList, retBuffer);

         // Next, update the current buffer
         if ((inst->loopState == ensalsLooping1) ||
             (inst->loopState == ensalsStaticLoop))
         {
            inst->pCurBuffer = inst->bufferList;
            if (inst->bufferList != NULL) {
               DPRINTF(("  In ISR, ensalsLooping1 advanced to %ld", inst->pCurBuffer->dwUser3));
               inst->bufferList = inst->bufferList->pNextBuffer;
            }

            // Update the loop state if necessary
            if ((retBuffer->dwLoopFlags&SA_LOOP_END) && (inst->dwLoopCount == 0))
            {
               if (inst->loopState != ensalsStaticLoop)
                  inst->loopState = ensalsNotLooping;
               retBuffer = inst->pLoopBufferList;
               inst->pLoopBufferList = NULL;
            }
            else
            {
               retBuffer = NULL;
               if ((inst->pCurBuffer->dwLoopFlags&SA_LOOP_END) &&
                   (inst->dwLoopCount > 1))
                  inst->loopState = ensalsLooping2;
            }
         }
         else
         {
            retBuffer = NULL;
            inst->pCurBuffer = inst->pLoopBufferList;
            if (inst->pLoopBufferList != NULL)
               inst->pLoopBufferList = inst->pLoopBufferList->pNextBuffer;

            // Update the loop state if necessary
            if ((inst->pCurBuffer->dwLoopFlags&SA_LOOP_END) &&
                (inst->dwLoopCount == 1))
               inst->loopState = ensalsLooping1;
         }
      }
      else
      {
         inst->pCurBuffer = inst->bufferList;
         if (inst->bufferList != NULL) {
            DPRINTF(("  In ISR, advancing to buffer %ld", inst->pCurBuffer->dwUser3));
            inst->bufferList = inst->bufferList->pNextBuffer;
         }
      }

      if (inst->pCurBuffer == NULL)
      {
         DPRINTF(("  In ISR, pCurBuffer is now NULL.  Setting up one shot."));
         STORE_INST_DEBUG_WC(0x49433300)

         // Set the loop points so we get interrupted in 64 samples so we
         //    play through the entire cache, then stop
         dwCurPos = seParameterRead((SEVOICE)devHandle, sepStartAddrs);
         seParameterWrite((SEVOICE)devHandle, sepStartloopAddrs, dwCurPos + 72);
         seParameterWrite((SEVOICE)devHandle, sepEndloopAddrs, dwCurPos + 64);
         seParameterWrite((SEVOICE)devHandle, sepOneShot, 1);
         inst->pTmpLastBuffer = retBuffer;
         return FALSE;
      }
      else if (retBuffer != NULL)
      {
         STORE_INST_DEBUG_WC(0x49433400)
         DPRINTF(("  In ISR, adding buffer %ld to pRetBufferList", retBuffer->dwUser3));

         // Add the buffer to the return list
         ADD_BUFFER_TO_LIST(inst->pRetBufferList, retBuffer);
      }
   }
   else
   {
      STORE_INST_DEBUG_WC(0x49433500)

      // We're not done with the current buffer
      inst->dwPlayPos += MAX_BUFFER_SIZE;
      retBuffer = NULL;
   }
   
   // Map the next region
   inst->dwCurBufferOffset = inst->dwNextBufferOffset;
   inst->bCurBufferInFirstHalf = !(inst->bCurBufferInFirstHalf);
   if (inst->dwCurPos+MAX_BUFFER_SIZE < inst->pCurBuffer->dwSize)
      dwCurLength = MAX_BUFFER_SIZE;
   else
      dwCurLength = inst->pCurBuffer->dwSize - inst->dwCurPos;
   outdev->UpdateNextRegion(inst);

   // Update the loop addresses
   dwEndLoopPos = ((inst->bCurBufferInFirstHalf ? 0 : SM_BUFFER_SIZE) +
                   dwCurLength + inst->dwCurBufferOffset)/inst->byFrameSize;
   seParameterWrite((SEVOICE)devHandle, sepEndloopAddrs, dwEndLoopPos);

   // Check to see if we missed an interrupt.  This means that somebody was
   // holding off hardware interrupts for too long.  This is BAD!!!  We have
   // to reprogram the loop address to recover.
   dwCurPos = seParameterRead((SEVOICE)devHandle, sepStartAddrs);
   if (dwCurPos > dwEndLoopPos)
   {
      seParameterWrite((SEVOICE)devHandle, sepEndloopAddrs, dwCurPos+8);
#ifdef DEBUG
      dprintf("SAOutput interrupt missed - dwCurPos = %d, dwEndLoopPos = %d",
              dwCurPos, dwEndLoopPos);
#endif
   }

   return (retBuffer != NULL);
}

BOOL
SASE8210OutputDevice::Callback(IPHANDLE ipHandle, enIPType type,
                               DWORD userParam, unIPReply *pReply)
{
   SASE8210OutputDevice *outdev = (SASE8210OutputDevice *)userParam;
   DWORD devHandle = (DWORD)pReply->voiceHandle;
   stSAOutputBuffer *retBuffer;

   DPRINTF(("OutputDevice::Callback started"));

   GET_INST(outdev->instList, FALSE)

   DPRINTF(("inst = 0x%lx", inst));

   if (inst->pCurBuffer) 
       DPRINTF(("  In Callback, pCurBuffer is %ld", inst->pCurBuffer->dwUser3));

   STORE_INST_DEBUG_WC(0x4342636b)

   inst->bInCallback = TRUE;
   // If we've run out of buffers, stop the voice
   if (inst->pRetLastBuffer != NULL)
   {
      STORE_INST_DEBUG_WC(0x43423100)

      DPRINTF(("  pRetLastBuffer (%ld)  is not null, stopping voice.",
              inst->pRetLastBuffer->dwUser3)); 

      IDISABLE();
      // Add the buffer to the return list
      ADD_BUFFER_TO_LIST(inst->pRetBufferList, inst->pRetLastBuffer);
      inst->pRetLastBuffer = NULL;

      // Stop the voice
      seVoiceStop(inst->byNumChans, inst->voiceHandle);
      seParameterWrite((SEVOICE)devHandle, sepOneShot, 0);
      IENABLE();

      // Restart the voice if somone's added a buffer
      if (inst->bufferList != NULL)
      {
         stSAOutputBuffer *tmp = inst->bufferList;
         inst->bufferList = NULL;
         DPRINTF(("  In Callback, calling AddBuffer to restart voice"));
         outdev->AddBuffer(devHandle, tmp);
      }
   }

   // Return the buffers
   if (inst->callback != NULL)
   {
      IDISABLE();
      while (inst->pRetBufferList != NULL)
      {
         STORE_INST_DEBUG_WC(0x43423200)

         retBuffer = inst->pRetBufferList;
         inst->pRetBufferList = retBuffer->pNextBuffer;
         IENABLE();
		 retBuffer->pNextBuffer = NULL;
         if (retBuffer->dwReserved != 0)
         {
            DeleteAlloc((void *)(retBuffer->dwReserved));
            retBuffer->dwReserved = 0;
         }
	 DPRINTF(("  In Callback, returning buffer %ld", retBuffer->dwUser3));
         inst->callback(retBuffer);
         IDISABLE();
      }
      IENABLE();
   }
   inst->bInCallback = FALSE;

   DPRINTF(("OutputDevice::Callback finished"));

   if ((inst->pCurBuffer != NULL) || (inst->pTmpLastBuffer != NULL) ||
       (inst->pRetLastBuffer != NULL))
      return TRUE;
   else
   {
      inst->ipHandle = 0;
      return FALSE;
   }
}

void
SASE8210OutputDevice::UpdateNextRegion(SASE8210InstanceInfo *inst)
{
   SMMAP m;

   // Set the loop info
   if ((inst->loopState == ensalsNotLooping) &&
       (inst->pCurBuffer->dwLoopFlags&SA_LOOP_START))
   {
      STORE_INST_DEBUG_WC(0x554e5231)

      inst->dwLoopCount = inst->pCurBuffer->dwNumLoops;
      if ((inst->pCurBuffer->dwLoopFlags&SA_LOOP_END) &&
          (inst->dwLoopCount > 1))
         inst->loopState = ensalsLooping2;
      else
         inst->loopState = ensalsLooping1;
   }

   // Now map the next region
   if (inst->loopState == ensalsStaticLoop)
   {
      // Setup the zero buffer for one shot static buffers
      m.pBaseAddr = (BYTE *)pEmptyBuffer;
      inst->dwNextBufferOffset = (inst->bySampleSize == 1 ?
               ZERO_BUFFER_8BIT_POS : ZERO_BUFFER_16BIT_POS);
      m.dwLength = SM_PAGE_SIZE;
      m.pdwPhysicalPageList = &dwEmptyBuffer;
      m.dwOffset = MAX_BUFFER_SIZE*2+SM_PAGE_SIZE;

      seParameterWrite(inst->voiceHandle[0], sepStartloopAddrs,
                       (m.dwOffset + inst->dwNextBufferOffset)/inst->byFrameSize);
   }
   else if (inst->dwCurPos+MAX_BUFFER_SIZE < inst->pCurBuffer->dwSize)
   {
      // More to play from the current buffer
      STORE_INST_DEBUG_WC(0x554e5232)

      m.pBaseAddr = (BYTE *)PAGE_ALIGN((DWORD)inst->pCurBuffer->virtAddr+
                                       inst->dwCurPos+MAX_BUFFER_SIZE);
      inst->dwNextBufferOffset = (DWORD)inst->pCurBuffer->virtAddr+
                                 inst->dwCurPos+MAX_BUFFER_SIZE-
                                 (DWORD)m.pBaseAddr;
      if (inst->dwCurPos + MAX_BUFFER_SIZE*2 < inst->pCurBuffer->dwSize)
         m.dwLength = PAGE_ALIGN_UP(MAX_BUFFER_SIZE+inst->dwNextBufferOffset);
      else
         m.dwLength = PAGE_ALIGN_UP(inst->pCurBuffer->dwSize-inst->dwCurPos-
                                    MAX_BUFFER_SIZE+inst->dwNextBufferOffset);
      m.pdwPhysicalPageList = (DWORD *)inst->pCurBuffer->dwReserved +
         (inst->dwCurPos+MAX_BUFFER_SIZE)/SM_PAGE_SIZE;
   }
   else if (inst->loopState == ensalsLooping2)
   {
      // Next buffer is in the loop buffer list
      // Current buffer is almost done, so set up next buffer
      STORE_INST_DEBUG_WC(0x554e5233)

      if (inst->pLoopBufferList == NULL)
      {
         // If loop buffer list is NULL, we're looping on a single buffer
         m.pBaseAddr = (BYTE *)PAGE_ALIGN(inst->pCurBuffer->virtAddr);
         inst->dwNextBufferOffset = (DWORD)inst->pCurBuffer->virtAddr -
                                    (DWORD)m.pBaseAddr;
         if (inst->pCurBuffer->dwSize > MAX_BUFFER_SIZE)
            m.dwLength = PAGE_ALIGN_UP(MAX_BUFFER_SIZE+inst->dwNextBufferOffset);
         else
            m.dwLength = PAGE_ALIGN_UP(inst->pCurBuffer->dwSize+
                                       inst->dwNextBufferOffset);
         m.pdwPhysicalPageList = (DWORD *)inst->pCurBuffer->dwReserved;
      }
      else
      {
         // Use the buffer at the head of the loop buffer list
         m.pBaseAddr = (BYTE *)PAGE_ALIGN(inst->pLoopBufferList->virtAddr);
         inst->dwNextBufferOffset = (DWORD)inst->pLoopBufferList->virtAddr -
                              (DWORD)m.pBaseAddr;
         if (inst->pLoopBufferList->dwSize > MAX_BUFFER_SIZE)
            m.dwLength = PAGE_ALIGN_UP(MAX_BUFFER_SIZE+inst->dwNextBufferOffset);
         else
            m.dwLength = PAGE_ALIGN_UP(inst->pLoopBufferList->dwSize+
                                       inst->dwNextBufferOffset);
         m.pdwPhysicalPageList = (DWORD *)inst->pLoopBufferList->dwReserved;
      }
   }
   else if (inst->bufferList != NULL)
   {
      // Current buffer is almost done, so set up next buffer
      DPRINTF(("   In UpdateNextRegion, setting up next buffer %ld", 
               inst->bufferList->dwUser3));
      STORE_INST_DEBUG_WC(0x554e5234)

      m.pBaseAddr = (BYTE *)PAGE_ALIGN(inst->bufferList->virtAddr);
      inst->dwNextBufferOffset = (DWORD)inst->bufferList->virtAddr -
                           (DWORD)m.pBaseAddr;
      if (inst->bufferList->dwSize > MAX_BUFFER_SIZE)
         m.dwLength = PAGE_ALIGN_UP(MAX_BUFFER_SIZE+inst->dwNextBufferOffset);
      else
         m.dwLength = PAGE_ALIGN_UP(inst->bufferList->dwSize+
                                    inst->dwNextBufferOffset);
      m.pdwPhysicalPageList = (DWORD *)inst->bufferList->dwReserved;
   }
   else
   {
      // No more buffers, so setup empty buffer
      DPRINTF(("   In UpdateNextRegion, next buffer is NULL."));
      STORE_INST_DEBUG_WC(0x554e5235)

      m.pBaseAddr = (BYTE *)pEmptyBuffer;
      inst->dwNextBufferOffset = (inst->bySampleSize == 1 ?
               ZERO_BUFFER_8BIT_POS : ZERO_BUFFER_16BIT_POS);
      m.dwLength = SM_PAGE_SIZE;
      m.pdwPhysicalPageList = &dwEmptyBuffer;
   }
   m.dwFlags = SMMAP_PHYSICAL;
   if (inst->loopState != ensalsStaticLoop)
      m.dwOffset = (inst->bCurBufferInFirstHalf ? SM_BUFFER_SIZE : 0);
   smMemoryMap(inst->smHandle, &m); /* != SUCCESS ??? */

   // Update the start loop address
   if (inst->loopState != ensalsStaticLoop)
      seParameterWrite(inst->voiceHandle[0], sepStartloopAddrs,
                       ((inst->bCurBufferInFirstHalf ? SM_BUFFER_SIZE : 0) +
                       inst->dwNextBufferOffset)/inst->byFrameSize);
}

#ifdef DEBUG
void
SASE8210OutputDevice::NotifyCallBack(DWORD ptr, SEVOICE voiceHandle,
                                     SENOTIFYEVENT why)
{
    ASSERT(0);
}
#endif


DWORD
SASE8210OutputDevice::GetSEVOICE(DWORD devHandle, DWORD dwChanNum)
{
    GET_INST(instList, 0);

    return inst->voiceHandle[dwChanNum];
}




/***************************************************************************
 * SA8210WaveOutputDevice method definitions
 *
 * NOTE:  We inherit most of the device dispatch routines from our parent
 * class, SASE8210OutputDevice.  The only things we override are the
 * constructor.
 ***************************************************************************/
/*
static stSASE8210OutputConfig SA8210WaveOutputConfig=
{255, 255, 0, 0, 0, 1, 2, 3};

SA8210WaveOutputDevice::SA8210WaveOutputDevice(HRMID hrmid, void *pBuffer,
                                               DWORD dwBuffer)
  : SASE8210OutputDevice(hrmid, &SA8210WaveOutputConfig, pBuffer, dwBuffer)
{
	// Stuff the device name
	strcpy(SASE8210DevCaps.szDevName, "APS Wave Out");
}
*/
