/*****************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
*****************************************************************************/

/*****************************************************************************
*
* @doc INTERNAL
* @module hrb8210.h | 
*  This file contains the implementation of the Hardware Record Buffer
*  Manager (aka, the HRB).  It includes functions to initialize the manager,
*  open and close devices, and start and stop DMA into memory.
*
* @iex 
* Revision History:
*
* Person            Date			Reason
* ---------------	------------    --------------------------------------
* John Kraft		Feb 24, 98		Modified the code so that we could invoke
*									_transferSamples from the high-priority
*									ISR.  This mostly involved bracketing bits
*									of code with IDISABLE/IENABLE.
* John Kraft		Oct 27, 97		Created.
*
*****************************************************************************/

/******************
 * Include files
 ******************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "datatype.h"
#include "dbg8210.h"
#include "hal8210.h"
#include "os8210.h"
#include "ip8210.h"
#include "itm8210.h"
#include "hrb8210.h"
#include "hrb8210d.h"
#include "hrm8210d.h"

#ifdef DEBUG
static DWORD gsdwOutCount;
#endif

#define FXOUT_NUMCHANS 8
#define FXOUT_CHANNEL_SELECT 0x003CF0000

static DWORD gdwLastIntrTime;

/******************
 * Definitions 
 ******************/

#define HRB_INTR_FREQUENCY 20	/* One intr approx.  every 5 ms */

#define HRB_IN_USE      0xABBADEADL
#define HRB_ID_MAGIC    0xABBAL
#define HRB_OPEN_MAGIC  0xABBA00ABL

#define MAKE_HRBID(_x)     ((HRB_ID_MAGIC << 16) | (_x))
#define GET_STINDEX(_x)     ((_x) & 0xFF)
#define IS_VALID_HRBID(_x)  \
	((((_x) >> 16) == HRB_ID_MAGIC) && (GET_STINDEX(_x) < MAX_CHIPS) && \
	 (hrbStates[GET_STINDEX(_x)].dwInUse != 0))
#define FIND_STATE(_x) \
    (IS_VALID_HRBID(_x) ? &hrbStates[GET_STINDEX(_x)] : NULL)

#define MAKE_HANDLE(_x)  ((HRBHANDLE) (_x))
#define IS_VALID_OPENINFO(_x) \
	(((_x) != 0) && (((stOpenInfo*) (_x))->dwMagic == HRB_OPEN_MAGIC))
#define FIND_OPENINFO(_x) (IS_VALID_OPENINFO(_x) ? ((stOpenInfo*) (_x)) : NULL)

/* @type fpCopyFunc
 *  Defines the function pointer for the copy function used to transfer
 *  samples.
 */
struct openInfoTag;
typedef void (*fpCopyFunc)(struct openInfoTag *poi, BYTE *from, BYTE *to, 
						  DWORD dwNumFrames);

/********************
 * Structures 
 ********************/

/* @struct stOpenInfo |
 *  This structure encapsulates all of the state for a particular
 *  open instance.
 */
typedef struct openInfoTag {
	DWORD          dwMagic;       /* @field  Magic number to insure that
								   *  this really is an OpenInfo struct */
	struct devInfoTag * pdevice;  /* @field The device that this open handle
								   *  refers to.  */
	struct openInfoTag * pNextOpen; /* @field Successor in linked list */
	HRBCALLBACK    callback;      /* @field Callback function to invoke */
	DWORD          dwFrameNum;    /* @field The current sample frame number */
	DWORD          dwNumUnderrun; /* @field The number of sample frames which
								   *  were dropped because no buffers were
								   *  available on the buffer list */
	DWORD          dwLastDevIndex;/* @field Keeps track of the last absolute
								   *  device index from which we transferred.
								   *  This gets updated in _transferSamples
								   *  and initially set in DevStart. */
	HRBBUFFER    * pBufferList;   /* @field The enqueued sample buffers */
	HRBBUFFER    * pFilledBuffers;/* @field A list of buffers which have
								   *  been filled with data */
	DWORD          dwChanSelect;  /* @field bit field specifying which
								   * channels to record */
	BYTE           byNumChans;    /* @field The number of channels to record */
	BYTE           bySampleSize;  /* @field The size of the sample in bytes */
	BOOL           bRecording;    /* @field Flag indicating whether we're
								   *  recording */
    DWORD          dwFlags;       /* @field Recording flags from config structure */
    fpCopyFunc     fpCopyFunc;    /* @field The copy function to be used */

} stOpenInfo;


/* @struct stDeviceInfo | 
 *  Contains all of the information the information the HRB needs
 *  about a particular device and its current state.
 */
typedef struct devInfoTag {
	HALID         halid;          /* @field The HAL ID for the HRB */
	IPID          ipid;           /* @field Interrupt manager ID */
	IPHANDLE      iphandle;       /* @field Interrupt handle */
	OSVIRTADDR	  virtAddr;       /* @field Virtual address of record buffer */
	DWORD         dwMaxSize;      /* @field The largest possible size (in bytes)
	                               *  for the record buffer.  In order to 
	                               *  reduce latency a device may not utilize
	                               *  the entire buffer. */
	DWORD         dwCurrSize;     /* @field The size of the buffer actually
	                               *  being used for recording.  < dwMaxSize */
	enSASampleRate sampleRate;    /* @field Device's current sample rate */
	enSASampleRate supportedSampleRates;  /* @field Bit field of sample 
								   *  rates supported by the device */
	DWORD		   dwHighSampleRate;
	DWORD		   dwLowSampleRate;
	enSASampleFormat supportedFormats;  /* @field Contains the supported formats */
	stOpenInfo *  pOpenList;      /* @field List of open handles */
	DWORD         dwLastIndex;    /* @field The position of the buffer pointer
								   *  the last time we sucked samples out of buffer. */
	enIPType      ipType;         /* @field The interrupt type */
	DWORD         dwSizeReg;      /* @field The register containing the buffer
								   *  size.  Needed to start and stop DMA.  */
	DWORD         dwIndexReg;     /* @field The register containing the buffer
								   *  index.  */
	BYTE          byNumChans;     /* @field Number of channels supported
								   *  by the device */
	BYTE          bySampleSize;   /* @field The largest supported sample
								   *  size for the device */
	BYTE          byNumRefs;      /* @field Number of open handles */
	BYTE          byNumRecording; /* @field A count of how many of the
								   *  open handles are actually in record
								   *  mode right now. */
	BYTE          byDevice;       /* @field Device number (HRBDEVICEVALUE) */
	BYTE          byEncodedSize;  /* @field Value which is actually written */
#ifdef ITM_RECORDING
    ITMID         itmid;          /* @field ID of the Interval Timer Manager. */
    ITMHANDLE     itmhandle;      /* @field Timer used to trigger sample
                                   *  transfers.  */
    ITMSCHEDINFO  itmschedinfo;   /* @field Scheduling info for timer. */
    BOOL          bLowLevelCallbackScheduled;
#endif
} stDeviceInfo;
	
/* @struct stHRBState |
 *  This structure contains data relevant to a particular HRB instance.
 *  One stHRBState structure exists for every E8010 chip in the system.
 */
typedef struct {
	DWORD          dwInUse;       /* @field Flag indicating structure is in use */
	stDeviceInfo   devices[3];    /* @field Device info for Mic, ADC, and FX */
} stHRBState;


/****************************************************************************
 * Global variables 
 ****************************************************************************/

stHRBState  hrbStates[MAX_CHIPS];
WORD        hrbCount;


/****************************************************************************
 * Static function declarations 
 ****************************************************************************/

static void         _computeBufferSize(stDeviceInfo*);
static EMUSTAT      _startDMA(stDeviceInfo *);
static void         _stopDMA(stDeviceInfo *);
static HRBBUFFER *  _transferSamples(stOpenInfo *, DWORD, BOOL);

#ifdef ITM_RECORDING
static void         _intrCallback(DWORD);
static BOOL         _intrISRCallback(ITMSCHEDINFO *pSchedInfo);
#else
static BOOL         _intrCallback(IPHANDLE handle, enIPType type, 
                                  DWORD userParam, unIPReply *pReply);
static BOOL         _intrISRCallback(IPHANDLE handle, enIPType type, 
									 DWORD userParam, unIPReply *pReply);
#endif

static fpCopyFunc   _pickCopyFunc(stOpenInfo *poi);

static HRBBUFFER *  _concatBuffLists(HRBBUFFER *, HRBBUFFER *);

/****************************************************************************
 * Chip Discovery Functions
 ****************************************************************************/

EMUAPIEXPORT EMUSTAT
hrbInit()
{
	hrbCount = 0;
	memset(hrbStates, 0x0, sizeof(hrbStates));

	return SUCCESS;
}


EMUAPIEXPORT EMUSTAT
hrbDiscoverChip(HALID halid, IPID ipid, ITMID itmid, HRBCONFIG *config, HRBID *retID)
{
	WORD           wIndex;    /* Index variable for the hrbStates array */
	stHRBState    *state;     /* Newly initialized state */
	stDeviceInfo  *devinfo;   /* Newly initialized device info */

	for (wIndex = 0; wIndex < MAX_CHIPS; wIndex++) {
		if (hrbStates[wIndex].dwInUse != HRB_IN_USE)
			break;
	}

	if (wIndex == MAX_CHIPS)
		RETURN_ERROR(HRBERR_INIT_FAILED);

	/* Set up the state data */
	state = &hrbStates[wIndex];
    
	state->dwInUse = HRB_IN_USE;
	
	/* Set up the Analog-to-Digital converter input device */
	devinfo = &state->devices[hrbdevADC];
	devinfo->halid     = halid;
	devinfo->ipid      = ipid;
	devinfo->iphandle  = 0;
	devinfo->virtAddr  = config->hbSrcRecord.osVirtAddr;
	devinfo->dwMaxSize =  config->hbSrcRecord.dwSize; 
	devinfo->dwCurrSize = devinfo->dwMaxSize;
	devinfo->supportedSampleRates = (enSASampleRate)
						(saRate48K | saRate44_1K | saRate32K |
						saRate24K | saRate22_05K | saRate16K | 
						saRate11_025K | saRate8K);
	devinfo->sampleRate       = (enSASampleRate) 0;
	devinfo->dwLowSampleRate  = 8000;
	devinfo->dwHighSampleRate = 48000;
	devinfo->supportedFormats = (enSASampleFormat) (saFormatUnsigned8PCM | saFormatSigned16PCM);
	devinfo->pOpenList     = NULL;
	devinfo->dwLastIndex   = 0;
	devinfo->ipType        = IP_ADCBUFFER;
	devinfo->dwSizeReg     = ADCBS;
	devinfo->dwIndexReg    = ADCIDX;
	devinfo->byNumChans    = 2;
	devinfo->bySampleSize  = 2;
	devinfo->byNumRefs     = 0;
	devinfo->byNumRecording = 0;
	devinfo->byDevice      = (BYTE) hrbdevADC;

#ifdef ITM_RECORDING
    devinfo->itmid         = itmid;
#endif

	_computeBufferSize(devinfo);
	LSEPtrWrite(halid, ADCBS, 0);
	LSEPtrWrite(halid, ADCBA, config->hbSrcRecord.osPhysAddr);
    DPRINTF(("ADC SRC virtual 0x%lx physic 0x%lx\n",
            config->hbSrcRecord.osVirtAddr, config->hbSrcRecord.osPhysAddr));
	
	/* Set up the Mono Microphone input device */
	devinfo = &state->devices[hrbdevMic];
	devinfo->halid          = halid;
	devinfo->ipid           = ipid;
	devinfo->iphandle       = 0;
	devinfo->virtAddr       = config->hbMicRecord.osVirtAddr;
	devinfo->dwMaxSize      = config->hbMicRecord.dwSize;
	devinfo->dwCurrSize     = devinfo->dwMaxSize;
	devinfo->supportedSampleRates = saRate8K;
	devinfo->supportedFormats = (enSASampleFormat) (saFormatUnsigned8PCM | saFormatSigned16PCM);
	devinfo->sampleRate     = saRate8K;
	devinfo->dwLowSampleRate  = 8000;
	devinfo->dwHighSampleRate = 8000;
	devinfo->pOpenList      = NULL;
	devinfo->dwLastIndex    = 0;
	devinfo->ipType         = IP_MICBUFFER;
	devinfo->dwSizeReg      = MBS;
	devinfo->dwIndexReg     = MIDX;
	devinfo->byNumChans     = 1;
	devinfo->bySampleSize   = 2;
	devinfo->byNumRefs      = 0;
	devinfo->byNumRecording = 0;
	devinfo->byDevice       = (BYTE) hrbdevMic;

#ifdef ITM_RECORDING
    devinfo->itmid          = itmid;
#endif

	_computeBufferSize(devinfo);
	LSEPtrWrite(halid, MBS, 0);
	LSEPtrWrite(halid, MBA, config->hbMicRecord.osPhysAddr);
    DPRINTF(("Microphone virtual 0x%lx physical 0x%1x\n",
            config->hbMicRecord.osVirtAddr, config->hbMicRecord.osPhysAddr));

	/* Set up the FX Output loopback device */
	devinfo = &state->devices[hrbdevFXOut];
	devinfo->halid            = halid;
	devinfo->ipid             = ipid;
	devinfo->iphandle         = 0;
	devinfo->virtAddr         = config->hbFXRecord.osVirtAddr;
	devinfo->dwMaxSize        = config->hbFXRecord.dwSize;
	devinfo->dwCurrSize       = devinfo->dwMaxSize;
	devinfo->supportedSampleRates = saRate48K;
	devinfo->sampleRate       = saRate48K;
	devinfo->dwLowSampleRate  = 48000;
	devinfo->dwHighSampleRate = 48000;
	devinfo->supportedFormats = (enSASampleFormat)(saFormatUnsigned8PCM | saFormatSigned16PCM);
	devinfo->pOpenList        = NULL;
	devinfo->dwLastIndex      = 0;
	devinfo->ipType           = IP_FXBUFFER;
	devinfo->dwSizeReg        = FXBS;
	devinfo->dwIndexReg       = FXIDX;
	devinfo->byNumChans       = FXOUT_NUMCHANS;
    devinfo->bySampleSize     = 2;
	devinfo->byNumRefs        = 0;
	devinfo->byNumRecording   = 0;
	devinfo->byDevice         = (BYTE) hrbdevFXOut;

#ifdef ITM_RECORDING
    devinfo->itmid            = itmid;
#endif

	_computeBufferSize(devinfo);
	LSEPtrWrite(halid, FXBS, 0x0);
	LSEPtrWrite(halid, FXBA, config->hbFXRecord.osPhysAddr);

    /* This just enables the 8 channels corresponding to the current input
     * devices  */
    LSEPtrWrite(halid, FXWC, FXOUT_CHANNEL_SELECT);

	hrbCount++;
	*retID = MAKE_HRBID(wIndex);
	return SUCCESS;
}


/* @func Deinitialize the chip, cleaning up any data structures along the
 *  way.
 */
EMUAPIEXPORT EMUSTAT
hrbUndiscoverChip(HRBID hrbid)
{
	stHRBState *state = FIND_STATE(hrbid);

	/* XXX We don't use RETURN_ERROR here, since the sound 
	 * coordinator ends up calling a number of the undiscover
	 * functions twice.  */
	if (state == NULL)
		return HRBERR_BAD_HANDLE;

	/* Shut down the devices */
	_stopDMA(&state->devices[hrbdevADC]);
	_stopDMA(&state->devices[hrbdevMic]);
	_stopDMA(&state->devices[hrbdevFXOut]);

	/* Deallocate the state structure itself */
	state->dwInUse = 0;
	hrbCount--;

	return SUCCESS;
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/*****************************************************************************
 * @func Retrieves an array of all of the currently discovered hardware
 *  record buffer manager IDs.
 */
DWORD 
hrbGetHardwareInstances(DWORD dwNumIDs, HRBID *hrbids)
{
	WORD i;
    WORD wFound = 0;

	for (i = 0; i < MAX_CHIPS && wFound < dwNumIDs; i++) {
		if (hrbStates[i].dwInUse) 
			hrbids[wFound++] = MAKE_HRBID(i);
	}

    return hrbCount;
}


/****************************************************************************
 * @func Synthesize a name string for the specified sound engine.
 */

EMUAPIEXPORT EMUSTAT
hrbGetModuleName(HRBID hrbid, DWORD count, char *szName)
{
	char  name[64];		/* Name buffer.  We guarantee in the software 
	                     * spec that the name will never exceed 64 
	                     * chars in length */
	
	DWORD id = GET_STINDEX(hrbid);

	if (!IS_VALID_HRBID(hrbid)) 
		RETURN_ERROR(HRBERR_BAD_HANDLE);

	sprintf(name, "EMU 8210 Host Record Buffer Manager %d", id);
	strncpy(szName, name, (size_t) count);

	return SUCCESS;
}


/****************************************************************************
 * @func Return attribute information.  Right now, this doesn't do much.
 */
EMUAPIEXPORT EMUSTAT
hrbGetModuleAttributes(HRBID seid, HRBATTRIB *attr)
{
	RETURN_ERROR(1);
}


/***************************************************************************
 * @func Open a device.
 */
EMUAPIEXPORT EMUSTAT
hrbOpenDevice(HRBID hrbid, HRBDEVCONFIG *devconfig, HRBHANDLE *retHandle)
{
	stHRBState   *pstate  = FIND_STATE(hrbid);
	stDeviceInfo *pdevice;
	stOpenInfo   *poi;
	
	if (pstate == NULL)
		RETURN_ERROR(HRBERR_BAD_HANDLE);

	/* Check the specified device parameters */
	if (devconfig->device > hrbdevFXOut)
		RETURN_ERROR(HRBERR_BAD_DEVICE)
	else
		pdevice = &pstate->devices[devconfig->device];

	/* Check to see if sample rate is acceptable */
	if (!(devconfig->sampleRate & pdevice->supportedSampleRates))
		RETURN_ERROR(HRBERR_BAD_SAMPLERATE);
	if (pdevice->byNumRefs && (pdevice->sampleRate != devconfig->sampleRate))
		RETURN_ERROR(HRBERR_CANT_CHANGE_SAMPLERATE);

    pdevice->sampleRate = devconfig->sampleRate;

	/* Make sure that the client isn't asking for more channels than the
     * device supports */
	if (pdevice->byNumChans < devconfig->byNumChans)
		RETURN_ERROR(HRBERR_BAD_CHANNELS);

	/* Make sure that valid channels are selected in the channel select
	   bit mask  */
	if ((pdevice->byNumChans < 32) && 
		(devconfig->dwChanSelect > (1UL << pdevice->byNumChans)))
		RETURN_ERROR(HRBERR_BAD_CHANNELS);
	
	/* Count the number of bits set in the channel select mask and insure
	 * that it is either 1 or equal to byNumChans.  */
	if (devconfig->byNumChans > 1) {
		DWORD dwSelectedChans = devconfig->dwChanSelect;
		BYTE  byCount = 0;
		
		while (dwSelectedChans) {
			if (dwSelectedChans & 0x1)
				byCount++;
			dwSelectedChans >>= 1;
		}
		if (byCount != devconfig->byNumChans)
			RETURN_ERROR(HRBERR_BAD_CHANNELS);
	}
	
	/* See if the caller is trying to get a larger sample size than is 
	 * supported by the device (XXX We should probably allow this, since
	 * it is easy enough to just shift the device's sample size up). */
	if (pdevice->bySampleSize < devconfig->bySampleSize)
		RETURN_ERROR(HRBERR_BAD_SAMPLESIZE);

    /* Everything looks good, so let's go ahead and create an OpenInfo
	 * structure for the client.  */
	if ((poi = (stOpenInfo*) osHeapAlloc(sizeof(stOpenInfo), 0)) == NULL)
		RETURN_ERROR(HRBERR_NO_MEM);

	poi->dwMagic       = HRB_OPEN_MAGIC;
	poi->pdevice       = pdevice;
	poi->callback      = devconfig->callback;
	poi->dwFrameNum    = 0;
	poi->dwNumUnderrun = 0;
	poi->pBufferList   = NULL;
	poi->pFilledBuffers= NULL;
	poi->dwChanSelect  = devconfig->dwChanSelect;
	poi->byNumChans    = devconfig->byNumChans;
	poi->bySampleSize  = devconfig->bySampleSize;
	poi->bRecording    = FALSE;
    poi->dwFlags       = devconfig->dwFlags;

	/* Add the new open instance to the device list */
	IDISABLE();
	poi->pNextOpen     = pdevice->pOpenList;
	pdevice->pOpenList = poi;
	ASSERT(pdevice->byNumRefs < 255);
	pdevice->byNumRefs++;
	IENABLE();

    /* Choose the appropriate copy function */
    poi->fpCopyFunc = _pickCopyFunc(poi);

	*retHandle = MAKE_HANDLE(poi);

	return SUCCESS;
}


/***************************************************************************
 * @func Close an open handle to a device.
 */

EMUAPIEXPORT EMUSTAT
hrbCloseDevice(HRBHANDLE hrbh)
{
	stOpenInfo *poi = FIND_OPENINFO(hrbh);
	stOpenInfo *pPrevOpen;
	stOpenInfo *pCurrOpen;
	stDeviceInfo *pdevice;

	if (poi == NULL)
		RETURN_ERROR(HRBERR_BAD_HANDLE);

	pdevice = poi->pdevice;

	/* Switch off any recording that might be going on */
	hrbDevReset(hrbh, NULL);

	/* Remove this open info node from the open list */
	pPrevOpen = NULL;
	pCurrOpen = poi->pdevice->pOpenList;
	ASSERT(pCurrOpen != NULL);

	while (pCurrOpen != NULL) {
		if (pCurrOpen == poi) {
			/* We found ourselves; dequeue us */
			IDISABLE();
			if (pPrevOpen == NULL) 
				pdevice->pOpenList = poi->pNextOpen;
			else
				pPrevOpen->pNextOpen = poi->pNextOpen;

			ASSERT(pdevice->byNumRefs > 0);
			pdevice->byNumRefs--;

			poi->dwMagic = 0;
			IENABLE();

			osHeapFree(poi);
			return SUCCESS;
		} else {
			pPrevOpen = pCurrOpen;
			pCurrOpen = pCurrOpen->pNextOpen;
		}
	}

	/* We shouldn't ever get here, since to do so implies that
	 * the open info wasn't on the device list */
	ASSERT(0);

	osHeapFree(poi);
	return HRBERR_DATA_CORRUPT;
}


/****************************************************************************
 * @func Query various device properties.
 */
EMUAPIEXPORT EMUSTAT
hrbQueryDevice(HRBID hrbid, HRBDEVICEVALUE device, HRBDEVINFO *devinfo)
{
	stHRBState   *pstate = FIND_STATE(hrbid);
	stDeviceInfo *pdi;

	/* Validate the parameters */
	if (pstate == NULL)       RETURN_ERROR(HRBERR_BAD_HANDLE);
	if (device > hrbdevFXOut) RETURN_ERROR(HRBERR_BAD_DEVICE);
	if (devinfo == NULL)      RETURN_ERROR(HRBERR_BAD_DEVINFO);

	pdi = &pstate->devices[device];
	devinfo->sampleRates   = pdi->supportedSampleRates;
	devinfo->sampleFormats = pdi->supportedFormats;
	devinfo->dwChannels    = pdi->byNumChans;
	devinfo->dwFlags       = 0;
	devinfo->dwLowSampleRate  = pdi->dwLowSampleRate;
	devinfo->dwHighSampleRate = pdi->dwHighSampleRate;

	return SUCCESS;
}


/****************************************************************************
 * @func Enqueue a buffer onto an open handle's buffer list.  
 */
EMUAPIEXPORT EMUSTAT
hrbDevAddBuffers(HRBHANDLE hrbh, HRBBUFFER *pbuffer)
{
	stOpenInfo *poi = FIND_OPENINFO(hrbh); /* A gooey Hawaiian food staple */
	HRBBUFFER  *pCurrBuff;
    DWORD       dwFrameSize;

	if (poi == NULL)
		RETURN_ERROR(HRBERR_BAD_HANDLE);

	/* Make sure that the queued buffers are properly initialized */
	pCurrBuff = pbuffer;
	while (pCurrBuff != NULL) {
		pCurrBuff->dwNumFramesRecorded = 0;
		pCurrBuff->dwNumFramesDropped  = 0;
		pCurrBuff->dwFrameNumber       = 0;

		ASSERT(pCurrBuff->virtAddr);
		ASSERT(pCurrBuff->dwSize != 0);

		/* Round the buffer size down to a multiple of the frame size.
         * There is no guarantee that the framesize is a power of two,
         * so we can't get away with bit twiddling here. */
        dwFrameSize = (DWORD) poi->bySampleSize * poi->byNumChans;
		pCurrBuff->dwSize = (pCurrBuff->dwSize / dwFrameSize) * dwFrameSize;

#if 0
		/* Stamp the buffers with a buffer count */
		pCurrBuff->dwUser3 = gsdwInCount;
#endif
		pCurrBuff = pCurrBuff->pNextBuffer;
	}

	/* Make sure that we don't get interrupted */
	IDISABLE();

	if (poi->pBufferList == NULL) {
		poi->pBufferList = pbuffer;
	} else if (poi->pBufferList != pbuffer) {

		/* We need to find the end of the buffer list */
		pCurrBuff = poi->pBufferList;
		while (pCurrBuff->pNextBuffer != NULL) {
			pCurrBuff = pCurrBuff->pNextBuffer;
			if (pCurrBuff == pbuffer)
			  pbuffer = NULL;
		}
		
		pCurrBuff->pNextBuffer = pbuffer;
	}

	IENABLE();

	return SUCCESS;
}


/****************************************************************************
 * @func Determine whether any buffers are currently queued.
 */
EMUAPIEXPORT BOOL
hrbDevHasBuffersQueued(HRBHANDLE hrbh)
{
	stOpenInfo *poi = FIND_OPENINFO(hrbh);
	BOOL  bHasBuffers;

	if (poi == NULL)
		return FALSE;

	bHasBuffers = (poi->pBufferList != NULL);

	return bHasBuffers;
}


/****************************************************************************
 * @func Kick off the recording process for the specified handle.  The
 *  way this works is that we keep track of how many open handles are
 *  currently in recording start.  When the first open handle begins recording
 *  we call _startDMA() to start DMA'ing audio data into memory and schedule
 *  interrupts. 
 *
 *  For channels marked as syncable, the device start takes care
 *  of setting up everything but latching the initial frame index
 *  and marking the device as having started recording.   We do
 *  this as a final step in the hrbDevStartSync().
 */
EMUAPIEXPORT EMUSTAT
hrbDevStart(HRBHANDLE hrbh)
{
	stOpenInfo   *poi = FIND_OPENINFO(hrbh);  /* Pointer to open device info */
	stDeviceInfo *pdevice;                    /* Pointer to device itself */
	EMUSTAT       status;

	if (poi == NULL)
		RETURN_ERROR(HRBERR_BAD_HANDLE);

	pdevice = poi->pdevice;
	poi->dwNumUnderrun = 0;
	poi->dwFrameNum    = 0;

    /* When we're running in sync mode, we don't
     * really do any other work in this routine.  
     * It gets started in hrbDevStartSync().
     */
    if (poi->dwFlags & HRB_CONFIGFLAGS_SYNCABLE)
        return SUCCESS;

	if (pdevice->byNumRecording == 0) {
        poi->dwLastDevIndex = 0;
		status = _startDMA(pdevice);
		if (status != SUCCESS)
			RETURN_ERROR(status);
    } else {
        poi->dwLastDevIndex = LSEPtrRead(pdevice->halid, pdevice->dwIndexReg);
    }

#ifdef DEBUG
	gdwLastIntrTime = L8010SERegRead(pdevice->halid, WC);
#endif

	ASSERT(pdevice->byNumRecording < 255);
	pdevice->byNumRecording++;

    if (!(poi->dwFlags & HRB_CONFIGFLAGS_SYNCABLE))
        poi->bRecording = TRUE;

	return SUCCESS;
}


/****************************************************************************
 * Try and start a set of open instances simultaneously.
 *
 * XXX Warning: This routine currently assumes that all of the open instances
 * passed in the array reference the same device, and that 
 */
EMUAPIEXPORT EMUSTAT
hrbDevStartSync(HRBHANDLE *ahrbh, DWORD dwNumHandles)
{
    stOpenInfo   *poi;
    stDeviceInfo *pdevice;
    DWORD         dwLastDevIndex;
    DWORD         dwNumChans=0;
    DWORD         i;
    EMUSTAT       status;

    if (dwNumHandles == 0)
        return SUCCESS;

    ASSERT(ahrbh);

    for (i = 0; i < dwNumHandles; i++) {

        ASSERT(ahrbh[i]);

        poi = FIND_OPENINFO(ahrbh[i]);
        if (poi == NULL)
            RETURN_ERROR(HRBERR_BAD_HANDLE);

        if (poi->bRecording)
            RETURN_ERROR(HRBERR_ALREADY_STARTED);

        poi->bRecording    = TRUE;
        dwNumChans += poi->byNumChans;
    }

    pdevice = poi->pdevice;


    dwLastDevIndex = LSEPtrRead(pdevice->halid, pdevice->dwIndexReg);

    for (i = 0; i < dwNumHandles; i++) 
        poi->dwLastDevIndex = dwLastDevIndex;

    ASSERT(pdevice->byNumRecording < (255 - dwNumHandles));
    pdevice->byNumRecording += (BYTE) dwNumHandles;

    return SUCCESS;
}


/****************************************************************************
 * @func Shut down recording on a particular open handle.  In order for
 *  this to work correctly, we need to check a couple of places for filled
 *  buffers.  First off, it is possible that the ISR has activated while
 *  we've been running in the driver.  In this case, we need to insure that
 *  we've pulled off the buffers from filled list first.  In addition, we
 *  need to pull out data that may have shown up since the last interrupt
 *  occurred, so we call _transferBuffers and then merge the lists.
 */
EMUAPIEXPORT EMUSTAT
hrbDevStop(HRBHANDLE hrbh, HRBBUFFER **retBufferList)
{
 	stOpenInfo   *poi = FIND_OPENINFO(hrbh);
	stDeviceInfo *pdevice;

	/* Make sure that we don't inadvertantly return a bad handle */
	if (retBufferList)
		*retBufferList = NULL;

	if (poi == NULL)
		RETURN_ERROR(HRBERR_BAD_HANDLE);

	if (poi->bRecording == FALSE)
		return HRBERR_NOT_STARTED;

	pdevice = poi->pdevice;

	IDISABLE(); /* XXX Not sure that we need to lock out intrs here */
	poi->bRecording = FALSE;
	pdevice->byNumRecording--;

	if (pdevice->byNumRecording == 0)
		_stopDMA(pdevice);

	IENABLE();

	/* Clean up the buffer list */
	if (retBufferList == NULL) {
		poi->pBufferList = NULL;
	} else {
		HRBBUFFER *pMoreFilledBuffers;
		DWORD      dwBuffIndex;

		/* Clean out any data which has shown up since the last interrupt */
		dwBuffIndex = LSEPtrRead(pdevice->halid, pdevice->dwIndexReg);
		pMoreFilledBuffers = _transferSamples(poi, dwBuffIndex, TRUE);

		*retBufferList = _concatBuffLists(poi->pFilledBuffers, pMoreFilledBuffers);
		poi->pFilledBuffers = NULL;

	}

	return SUCCESS;
}


/****************************************************************************
 * @func Stop a number of open instances which were originally started
 *  using the synchronous start routine.
 *
 * XXX HACK Warning: Right now, this code only works if all of the open
 * instances in the array refer to the same device.  Also, no provisions
 * are made for allowing users to retrieve their partially filled buffers;
 * the assumption is that in this mode, the user has the callback function
 * set to NULL.
 *
 * ???? Shouldn't we return some indication of where we were when we stopped?
 */
EMUAPIEXPORT EMUSTAT
hrbDevStopSync(HRBHANDLE *ahrbh, DWORD dwNumHandles)
{
    stOpenInfo *poi;
    stDeviceInfo *pdevice;
    DWORD       i;


    if (dwNumHandles == 0)
        return SUCCESS;

    ASSERT(ahrbh);

    IDISABLE();
    for (i = 0; i < dwNumHandles; i++) {

        ASSERT(ahrbh[i]);

        poi = FIND_OPENINFO(ahrbh[i]);
        if (poi == NULL) {
            IENABLE();
            RETURN_ERROR(HRBERR_BAD_HANDLE);
        }

        if (poi->bRecording == FALSE) {
            IENABLE();
            RETURN_ERROR(HRBERR_NOT_STARTED);
        }

        poi->bRecording = FALSE;
        poi->pdevice->byNumRecording--;
    }

    pdevice = poi->pdevice;
    if (pdevice->byNumRecording == 0)
        _stopDMA(pdevice);

    IENABLE();

    /* Clear out the old, dead buffers */
    for (i = 0; i < dwNumHandles; i++) {
        poi = FIND_OPENINFO(ahrbh[i]);
        poi->pBufferList = NULL;
    }

    /* Do we need to clean up any data here */
    return SUCCESS;
}



/****************************************************************************
 * @func Stop recording and return all the buffers.  We first call hrbDevStop
 *  to do the bulk of the work, but we then need to enqueue any remaining 
 *  unfilled buffers onto the end of the returned buffer list.
 */
EMUAPIEXPORT EMUSTAT
hrbDevReset(HRBHANDLE hrbh, HRBBUFFER **retBufferList)
{
	stOpenInfo   *poi = FIND_OPENINFO(hrbh);

	if (poi == NULL)
		RETURN_ERROR(HRBERR_BAD_HANDLE);

    /* Stop the device and retrieve any partially filled buffers. 
	 * We don't need to check the return value, because under any
	 * circumstances we need to dequeue everything from the buffer list
	 * and hand it back. */
	hrbDevStop(hrbh, retBufferList);

	/* Find the end of the ret buffer list */
	if (retBufferList != NULL) {
		*retBufferList = _concatBuffLists(*retBufferList, poi->pBufferList);
	}

	poi->pBufferList = NULL;
	poi->dwFrameNum  = 0;

	return SUCCESS;
}


/****************************************************************************
 * @func Return the current sample count.  We do this by calling the 
 *  _transferSamples routine to actually copy any available samples into
 *  buffers and then update the frame count.  Doing this this way has a
 *  number of advantages.  First, if we encounter an underflow, we're
 *  guaranteed that the position returned will be that of the last sample
 *  written into a buffer, which appears to be the desired value.  Secondly,
 *  we're guaranteed that time will never run backwards, which is surprisingly
 *  difficult using other methods.  The disadvantage of this approach is that
 *  it reduces the sample accuracy of the returned value.  We could ameliorate
 *  this by doing the _transferBuffers in a loop and checking to see whether
 *  the buffer index changed between the current and previous transfers at
 *  the cost of some performance.  
 */
EMUAPIEXPORT DWORD
hrbDevGetCurrentSampleFrame(HRBHANDLE hrbh)
{
	stOpenInfo    *poi = FIND_OPENINFO(hrbh);
	stDeviceInfo  *pdevice;
	DWORD          dwBuffIndex;

	if (poi == NULL) return 0;

	if (poi->bRecording) 
	{
		IDISABLE();

		pdevice = poi->pdevice;
		dwBuffIndex = LSEPtrRead(pdevice->halid, pdevice->dwIndexReg);
		poi->pFilledBuffers = _concatBuffLists(poi->pFilledBuffers, 
				_transferSamples(poi, dwBuffIndex, FALSE));

		IENABLE();

		/* One might be tempted to return the buffers via the callback,
		 * but doing so runs the risk of returning buffers out of order
		 * if we're not really careful.  Safer to let _intrCallback do it.
		 */
	}

	return poi->dwFrameNum;
}


EMUAPIEXPORT EMUSTAT
hrbDevFlushBuffers(DWORD dwNumHandles, HRBHANDLE *ahrbh)
{
    stOpenInfo *poi;
    DWORD i;
    DWORD dwBuffIndex;

    if (dwNumHandles == 0) return SUCCESS;

    poi = FIND_OPENINFO(ahrbh[0]);

    if (poi == NULL) return HRBERR_BAD_HANDLE;
    if (poi->bRecording) {
        IDISABLE();

        dwBuffIndex = LSEPtrRead(poi->pdevice->halid, poi->pdevice->dwIndexReg);
 
        for (i = 0; i < dwNumHandles; i++) {
            poi = FIND_OPENINFO(ahrbh[i]);
            if (poi == NULL) {
                IENABLE();
                return HRBERR_BAD_HANDLE;
            }

            poi->pFilledBuffers = _concatBuffLists(poi->pFilledBuffers,
                _transferSamples(poi, dwBuffIndex, FALSE));
        }

        IENABLE();
    }

    return SUCCESS;
}


/****************************************************************************
 *  Static private functions
 ****************************************************************************/

/* @func Given a pointer to a device, this routine examines the given
 *  buffer size and determines the value that needs to get written into
 *  the device's buffer size register. Since the hardware doesn't support
 *  arbitrary buffer sizes, if the client is attempting to use a size that
 *  isn't supported by hardware we drop the buffer size down to the next
 *  lower size supported by hardware.
 *
 * @parm stDeviceInfo * | pdevice | The device for whom the buffer size is
 *  to be calculated.
 *
 * @rdesc No value is returned, but as a side effect the routine sets the 
 *  <t byEncodedSize> field in the structure.  The <t dwSize> field may also
 *  change depending on whether the requested size is supported.
 */
static void
_computeBufferSize(stDeviceInfo *pdevice)
{
	WORD   wIndex;
	DWORD  size = pdevice->dwCurrSize;

	static DWORD sizes[] = {0, 384, 448, 512, 640, 768, 896, 1024,
		1280, 1536, 1792, 2048, 2560, 3072, 3584, 4096, 5120, 6144,
		7168, 8192, 10240, 12288, 14336, 16384, 20480, 24576, 28672,
		32768, 40960, 49152, 57344, 65536, 0xFFFFFFFF };

	for (wIndex = 0; wIndex < (sizeof(sizes)/sizeof(DWORD)); wIndex++) {
		if (size < sizes[wIndex]) {
			break;
		}
	}

	/* If the closest approximate size would exceed the maximum buffer size
	 * pull back until we find something suitable */
	while (sizes[wIndex] > pdevice->dwMaxSize)
		wIndex--;

	pdevice->dwCurrSize   = sizes[wIndex];
	pdevice->byEncodedSize = (BYTE) wIndex;
}


/* @func Start data flowing in from the device.  We assume that this
 *  routine only gets called when the first open handle goes into record
 *  mode; it must not be called if the device is already DMA'ing.  We
 *  also do any configuration here that depends on how the handles are
 *  open.
 *
 * @parm stDeviceInfo * | pdevice | A pointer to the device being started.
 *
 * @rdesc Returns SUCCESS if DMA starts up successfully; otherwise,
 *	@flag HRBERR_INTR_FAILED | We couldn't set up the interrupt.
 */
static EMUSTAT
_startDMA(stDeviceInfo *pdevice)
{
	stIPInfo  ipinfo;
	DWORD     dwSampsPerSec;
    DWORD dwFrameSize = pdevice->bySampleSize * pdevice->byNumChans;

	/* Note: this makes the assumption that the byNumRecording is
	 * incremented after _startDMA is called */
	ASSERT(pdevice->byNumRecording == 0);

	/* First we need to program the device.  This is unfortunately
	 * very device-specific, so we switch based on the device id */
	if (pdevice->byDevice == hrbdevADC) 
	{
		DWORD rate;
		/* Program the channel selects and sample rate */
		switch (pdevice->sampleRate) {
		case saRate48K:		rate = 0x0;  dwSampsPerSec = 48000;   break; 
		case saRate44_1K:	rate = 0x1;  dwSampsPerSec = 44100;   break; 
		case saRate32K:		rate = 0x2;  dwSampsPerSec = 32000;   break;
		case saRate24K:		rate = 0x3;  dwSampsPerSec = 24000;   break;
		case saRate22_05K:	rate = 0x4;  dwSampsPerSec = 22050;   break;
		case saRate16K:		rate = 0x5;  dwSampsPerSec = 16000;   break;
		case saRate11_025K:	rate = 0x6;  dwSampsPerSec = 11025;   break;
		case saRate8K:		rate = 0x7;  dwSampsPerSec =  8000;   break;
        default:            ASSERT(0);
		}

        /* Select both channels for input (indicated by 0x18) and
         * set the sample rate */
		LSEPtrWrite(pdevice->halid, ADCSR, 0x18 | rate);
	} else if (pdevice->byDevice == hrbdevMic) {
		dwSampsPerSec = 8000;
	} 
	else if (pdevice->byDevice == hrbdevFXOut) {
		dwSampsPerSec = 48000;
	}

#ifdef ITM_RECORDING
    /* Calculate the ideal interrupt rate-- we try and interrupt
     * once every 5 ms to keep latency down to a minimum.  */
    pdevice->bLowLevelCallbackScheduled = FALSE;
    pdevice->itmschedinfo.dwNumerator = dwSampsPerSec / 200;
    pdevice->itmschedinfo.dwDenominator = dwSampsPerSec;
    pdevice->itmschedinfo.dwFlags = ITMSF_RUN_AT_ISR_TIME;
    pdevice->itmschedinfo.fCallback = _intrISRCallback;
    pdevice->itmschedinfo.dwUser1  = (DWORD) pdevice;
    if (itmScheduleCallback(pdevice->itmid, &pdevice->itmschedinfo, 
                              &pdevice->itmhandle) != SUCCESS)
        RETURN_ERROR(HRBERR_INTR_FAILED);
#else
	/* Calculate the optimal size of the buffer */
	pdevice->dwCurrSize = (dwSampsPerSec * dwFrameSize) / (1000 / (HRB_INTR_FREQUENCY * 2));
	DPRINTF(("_startDMA size calculation: dwSampsPerSec %ld requested buffer %ld", dwSampsPerSec, pdevice->dwCurrSize));
	_computeBufferSize(pdevice);
	DPRINTF(("   size returned %ld", pdevice->dwCurrSize));

	/* Enable interrupts for the device */
	ipinfo.type = pdevice->ipType;
	ipinfo.interruptParameter = 0;
	ipinfo.userParameter = (DWORD) pdevice;
	ipinfo.fHandler = _intrCallback;
	ipinfo.fISRHandler = _intrISRCallback;
	if (ipRegisterCallback(pdevice->ipid, &ipinfo, &pdevice->iphandle) != SUCCESS)
		RETURN_ERROR(HRBERR_INTR_FAILED);
#endif

	/* Update some fields for tracking DMA progress */
	pdevice->dwLastIndex  = 0;
    
    /* If we're starting the FX output channels, we need to wait
     * until we're out of the channel processing section for these
     * channels (otherwise, we can end up with only a partial frame
     * of data).  According to specifications, FX out recording 
     * occurs while channels 0x8 through 0xF are being processed,
     * so we wait until the LSB's of the wall clock fall into a 
     * safe window of opportunity.
     */
    if (pdevice->byDevice == hrbdevFXOut) {
        BYTE byWC;
        BYTE byTimeout = 100;

        IDISABLE();
        do {
            byWC = (BYTE) L8010SERegRead(pdevice->halid, WC) & 0x3F;
        } while (byTimeout-- && ((byWC <= 0x10) && (byWC >= 0x18)));
        ASSERT(byTimeout);
    }

	/* Start the DMA engine running */
	LSEPtrWrite(pdevice->halid, pdevice->dwSizeReg, pdevice->byEncodedSize);

    if (pdevice->byDevice == hrbdevFXOut)
        IENABLE();

	return SUCCESS;
}


/* @func Shuts down DMA on the specified device.  This routine should
 *  only be called when all of the open handles are no longer recording
 *  (i.e., when pdevice->byNumRecording == 0).
 *
 * @parm stDeviceInfo * | pdevice | Pointer to device to stop
 */
static void
_stopDMA(stDeviceInfo *pdevice)
{
	/* Shut down DMA */
	LSEPtrWrite(pdevice->halid, pdevice->dwSizeReg, 0x0);

#ifdef ITM_RECORDING
    if (pdevice->itmhandle) {
        itmCancelCallback(pdevice->itmhandle);
        pdevice->itmhandle = 0;
    }
#else
	/* Unregister the interrupt */
	ipUnregisterCallback(pdevice->iphandle);
#endif

}


/* @func This routine is called by the Interrupt Pending manager when one 
 *  of the buffer full or half full interrupts is asserted.  It then
 *  simply walks down the open handle list for the device and provides
 *  each open handle with the opportunity to transfer samples into its
 *  buffers.  The vast majority of the work occurs in the _transferSamples
 *  function.  The parameters are documented in the IP specification, but
 *  the only parameter we actually use is <t dwUserData>, which contains
 *  a pointer to the pdevice whose buffer is filling.
 *
 *  IMPORTANT SAFETY TIP: This routine is called at ISR time, when interrupts
 *  are disabled.  As a result, it has to be fast, it can't reference unlocked
 *  memory, and it can't call anything dangerous.  Anything of that sort needs
 *  to be done in the dispatched callback.  Also note that this routine can
 *  be invoked at any point in time, so we need to be sure that any data 
 *  structures it references are guarded (by disabling interrupts and
 *  introducing spin locks as necessary).
 */

#ifdef ITM_RECORDING
static BOOL
_intrISRCallback(ITMSCHEDINFO *pSchedInfo)
{
    stDeviceInfo *pdevice = (stDeviceInfo*) pSchedInfo->dwUser1;
#else
static BOOL
_intrISRCallback(IPHANDLE iphandle, enIPType iptype, DWORD dwUserData, 
			  unIPReply *ipreply)
{
    stDeviceInfo *pdevice = (stDeviceInfo*) dwUserData;
#endif

	stOpenInfo   *poi;
	DWORD         dwBuffIndex;
    DWORD         dwNow, dwElapsedTime;
    BOOL          bCallLowLevelIntr = FALSE;

	ASSERT(pdevice && (pdevice->byNumRecording > 0));

	/* First figure out how much data we need to transfer */
	dwBuffIndex = LSEPtrRead(pdevice->halid, pdevice->dwIndexReg);

	/* Don't copy partial device frames */
	dwBuffIndex &= ~(pdevice->bySampleSize * pdevice->byNumChans - 1);

#if 0
    dwNow = L8010SERegRead(pdevice->halid, WC);
    if (dwNow < gdwLastIntrTime)
        dwElapsedTime = dwNow + (0x3FFFFFF - gdwLastIntrTime);
    else
	    dwElapsedTime =  dwNow - gdwLastIntrTime;

    /* Convert elapsed time to microseconds */
	dwElapsedTime = dwElapsedTime * 325 / 1000;
	ASSERT(dwElapsedTime < (1000 * HRB_INTR_FREQUENCY * 3 / 2));
    gdwLastIntrTime = dwNow;
#endif

	/* We now just walk down the list of open handles and give them
	 * each a crack at the data. We invoke the callbacks separately to
	 * minimize the impact of a long callback on the data acquisition.
	 */
	poi = pdevice->pOpenList;
	while (poi != NULL) {
		if (poi->bRecording) {
			HRBBUFFER *pNewFilledBufs = _transferSamples(poi, dwBuffIndex, FALSE);

            /* If we're in Looping mode, just set pFilledBuffers to point to
             * the last processed buffer.  */
            if (poi->dwFlags & HRB_CONFIGFLAGS_LOOP)
                poi->pFilledBuffers = pNewFilledBufs;
            else
                poi->pFilledBuffers = _concatBuffLists(poi->pFilledBuffers, pNewFilledBufs);

            /* We don't need to call the low-level interrupt unless someone
             * actually wants to send data back to ring3.  */
            if (poi->callback && poi->pFilledBuffers)
                bCallLowLevelIntr = TRUE;
		}

		poi = poi->pNextOpen;
	}

	/* Update the last processed index */
	pdevice->dwLastIndex = dwBuffIndex;

#ifdef ITM_RECORDING
    if (bCallLowLevelIntr && !pdevice->bLowLevelCallbackScheduled) {
        osScheduleCallback(_intrCallback, (DWORD) pdevice);
        pdevice->bLowLevelCallbackScheduled = TRUE;
    }

    return TRUE;
#else
	/* Advance to the next buffer if we need to dequeue the current	
     * By returning TRUE, we indicate that we'd like the lower-level 
	 * interrupt to be scheduled. */
	return bCallLowLevelIntr;
#endif
}


/* @func Do non-time-critical interrupt processing here.  This routine has
 *  a lot more flexability in terms of the types of things it can do.
 */
#ifdef ITM_RECORDING
static void
_intrCallback(DWORD dwUserData)
#else
static BOOL
_intrCallback(IPHANDLE iphandle, enIPType iptype, DWORD dwUserData, 
			  unIPReply *ipreply)
#endif
{
	stDeviceInfo *pdevice = (stDeviceInfo*) dwUserData;
	HRBBUFFER    *pDequeuedBuffers;
	stOpenInfo   *poi = pdevice->pOpenList;

	DPRINTF(("*** In Dispatched interrupt handler"));

#ifdef ITM_RECORDING
    pdevice->bLowLevelCallbackScheduled = FALSE;
#endif
	while (poi != NULL)
	{
		/* We need to pull off the filled list of buffers while the interrupt
		 * is disabled  */
		if (poi->bRecording && poi->pFilledBuffers) 
		{
			IDISABLE();
			pDequeuedBuffers = poi->pFilledBuffers;
			poi->pFilledBuffers = NULL;
			IENABLE();

			/* Make sure that we call the callback with only one buffer at
			 * a time.  */
            if ((poi->dwFlags & HRB_CONFIGFLAGS_LOOP) && poi->callback) {
                /* Only call the callback once when in callback mode */
                poi->callback(pDequeuedBuffers);
            } else {
                while (pDequeuedBuffers) {
				    HRBBUFFER *pNextBuffer = pDequeuedBuffers->pNextBuffer;
                    ASSERT(poi->callback);
    				poi->callback(pDequeuedBuffers);
    				pDequeuedBuffers = pNextBuffer;
	    		}
            }
		}

		poi = poi->pNextOpen;
	}

#ifndef ITM_RECORDING
	return TRUE;
#endif
}


/* @func Copies all available samples out of the corresponding device's
 *  sample buffer and into the buffers attached to the open handle.  
 *  NOTE: This code is a little tricky, because the frame sizes of the
 *  device ring buffer and the client's buffer are not necessarily 
 *  the same.  There is, however, a one-to-one correspondence between
 *  them for the purposes of doing transfers.  For this reason, we
 *  do most of our calculations in terms of frames.  
 *
 *  The bDequeueCurrent buffer indicates that we want to dequeue the current
 *  buffer regardless of whether it contains any data.  This is required by
 *  the Windows Multimedia spec, which states that Stop always dequeues the
 *  current buffer.
 */

static HRBBUFFER *
_transferSamples(stOpenInfo *poi, DWORD dwBuffIndex, BOOL bDequeueCurrent)
{
	HRBBUFFER    *pCurrBuff;
	HRBBUFFER    *pPrevBuff;
	BYTE         *pbyDevBaseAddr;
	DWORD         dwXferSize;
	DWORD	      dwTotalSize;
	DWORD	      dwNumFrames;
	DWORD	      dwBuffSpaceLeft;
    DWORD         dwBuffFrames;
	WORD	      wIter, wNumIters;
	DWORD         dwBuffFrameSize = poi->byNumChans * poi->bySampleSize;
	stDeviceInfo *pdevice         = poi->pdevice;
    DWORD         dwDevFrameSize  = pdevice->byNumChans * pdevice->bySampleSize;
	fpCopyFunc    copyFunc        = poi->fpCopyFunc;
    DWORD         dwDevFrames;
	DWORD         dwHalfBuff = pdevice->dwCurrSize / 2;

int i = 0;
HRBBUFFER* x;
void* v;
	/* Since we're dealing with a circular buffer, there are going to be times
	 * when the buffer index is less than the last index, which indicates
	 * that the buffer index has wrapped.  In this case, we break the
	 * data to be copied into two contiguous ranges: the range which
	 * runs from the last index to the end of the buffer, and the range
	 * which goes from the base of the buffer to the write pointer index.
	 * The wNumIters variable indicates whether we need to process one
	 * or two buffers.
	 */
	pbyDevBaseAddr = (BYTE*) pdevice->virtAddr + poi->dwLastDevIndex;
	if (dwBuffIndex < poi->dwLastDevIndex) {
		dwXferSize     = pdevice->dwCurrSize - poi->dwLastDevIndex;
		dwTotalSize    = dwXferSize + dwBuffIndex;
		wNumIters      = 2;
	} else {
		dwXferSize     = dwBuffIndex - poi->dwLastDevIndex;
		dwTotalSize    = dwXferSize;
		wNumIters      = 1;
	}

	DPRINTF(("   dwXferSize 0x%lx dwTotalSize %lx", dwXferSize, dwTotalSize));

	pCurrBuff = poi->pBufferList;

	/* Make sure that the buffer's frame number gets initialized */
    if (pCurrBuff)
	  if (pCurrBuff->dwNumFramesRecorded == 0)
		pCurrBuff->dwFrameNumber = poi->dwFrameNum;

	/* Actually process the data in the recording buffers.  We loop here
	 * so that we can process both portions of the data (see comment above).
	 */
	for (wIter = 0; wIter < wNumIters; wIter++) 
	{
		/* Copy the given amount of data into the buffers */
		while (dwXferSize > 0) 
		{
			/* If we've run out of buffers, increment the underrun count and
			 * break out.  */
			if (pCurrBuff == NULL) {
                DPRINTF(("pCurrBuff is NULL.\n"));
				poi->dwNumUnderrun += (dwXferSize / dwDevFrameSize);
				break;
			}

			/* Figure out how much data we can copy into the current buffer */
			dwBuffSpaceLeft = pCurrBuff->dwSize - 
				(pCurrBuff->dwNumFramesRecorded * dwBuffFrameSize);
            dwBuffFrames  = dwBuffSpaceLeft / dwBuffFrameSize;
            dwDevFrames   = dwXferSize / dwDevFrameSize;
			dwNumFrames = ((dwBuffFrames < dwDevFrames) ? dwBuffFrames : dwDevFrames);

            DPRINTF(("Transferring samples: dwBuffSpaceLeft %ld dwXferSize %ld dwNumFrames %ld\n",
                    dwBuffSpaceLeft, dwXferSize, dwNumFrames));

			copyFunc(poi, pbyDevBaseAddr, 
					 (BYTE*) pCurrBuff->virtAddr + 
                         (pCurrBuff->dwNumFramesRecorded * dwBuffFrameSize),
					 dwNumFrames);

			pbyDevBaseAddr += dwNumFrames * dwDevFrameSize;          

			pCurrBuff->dwNumFramesRecorded += dwNumFrames;
			poi->dwFrameNum += dwNumFrames;
            dwXferSize -= (dwNumFrames * dwDevFrameSize);

            DPRINTF(("After copy pCurrBuff->dwNumFramesRecorded %ld\n",
                    pCurrBuff->dwNumFramesRecorded));
			
			/* Move on to the next buffer if necessary */
			ASSERT( (pCurrBuff->dwNumFramesRecorded * dwBuffFrameSize) <= pCurrBuff->dwSize );
			if ((pCurrBuff->dwNumFramesRecorded * dwBuffFrameSize) == pCurrBuff->dwSize) {
				
				/* Update the number of dropped samples */
				pCurrBuff->dwNumFramesDropped = poi->dwNumUnderrun;
				poi->dwNumUnderrun = 0;

				pPrevBuff = pCurrBuff;

                /* This is an odd case; if the loop flag is set and there is
                 * no next buffer, then what we do is set the current buffer
                 * back to the first thing on the bufferlist.  This implies
                 * that we never actually dequeue anything from the buffer list
                 * in this mode.
                 */
                if (poi->dwFlags & HRB_CONFIGFLAGS_LOOP)
                {
                    /* Wrap around back to the first buffer on the list */
                    if (pCurrBuff->pNextBuffer == NULL) {
                        pCurrBuff = poi->pBufferList;
                        DPRINTF((" In LOOP mode, Rewriting the current buffer 0x%x", pCurrBuff));
                    } else {
                        pCurrBuff = pCurrBuff->pNextBuffer;
                        DPRINTF((" In LOOP mode, advancing to buffer 0x%x", pCurrBuff));
                    }

                    /* Reset the number of frames recorded so that we just
                     * overwrite previous data.  If the application hasn't 
                     * already processed them, too bad.  */
                    pCurrBuff->dwNumFramesRecorded = 0;
                }
                else
                {
                    pCurrBuff = pCurrBuff->pNextBuffer;
                    DPRINTF(("Advancing to new buffer: %lx\n", pCurrBuff->dwUser3));
                }

				/* Set the initial frame number for the new buffer */
				if (pCurrBuff)
				  pCurrBuff->dwFrameNumber = poi->dwFrameNum;
			}
		}

		/* Update the base address and transfer size, just in case we're
		 * going to loop again */
		pbyDevBaseAddr   = (BYTE*) pdevice->virtAddr;
		dwXferSize = dwBuffIndex;
	}

	poi->dwLastDevIndex = dwBuffIndex;

	/* Advance to the next buffer if we need to dequeue the current
	 * buffer. */
	if (pCurrBuff && bDequeueCurrent) {
		pPrevBuff = pCurrBuff;
		pCurrBuff = pCurrBuff->pNextBuffer;
	}

	/* This is kind of sneaky.  We know that pCurrBuff is always pointing
	 * to the buffer to be filled.  This implies that all of
	 * the buffers preceeding pCurrBuff must have already been filled with
	 * data (or that the bDequeueCurrent flag is set).  So what we do is 
	 * play some pointer games to dequeue all of the filled buffers and 
	 * update the head of the buffer list.  
     * 
     * If the callback is NULL, this indicates that we're in buffer loop
     * mode.  When running in this mode we never actually want to remove
     * anything from 
	 */
    if (poi->dwFlags & HRB_CONFIGFLAGS_LOOP) {
        if (poi->callback && (pCurrBuff != pPrevBuff))
            return pPrevBuff;
        else
            return NULL;
    } 
    else if (pCurrBuff != poi->pBufferList) {
		HRBBUFFER *pRetBuff = poi->pBufferList;

		poi->pBufferList = pCurrBuff;
		pPrevBuff->pNextBuffer = NULL;

		return pRetBuff;
	}
    else {
        /* We haven't yet reached the end of the buffer; nother to 
         * queue.  */
        return NULL;
    }
}


/* @func Examines the open info structure and selects an appropriate
 *  copy function based on the characteristics of the number of channels
 *  to be copied, destination sample size, etc.  This indirection lets us
 *  arbitrarily optimize the sample copy functions without changing the
 *  top-level code.
 *
 * @parm stOpenInfo * | poi | A pointer to the open info structure.
 *
 * @rdesc Returns a function pointer to the copy function to be used.
 */

static void  _genericCopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames);
static void  _stereo16bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames);
static void  _stereo8bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames);
static void  _mono2ch8bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames);
static void  _mono2ch16bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames);
static void  _mono1ch16bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames);

static DWORD _findFirstSet(DWORD value);

static fpCopyFunc
_pickCopyFunc(stOpenInfo *poi)
{
	DWORD dwFirst = _findFirstSet(poi->dwChanSelect);

    if ((poi->byNumChans == 2) && ((poi->dwChanSelect >> dwFirst) == 0x3)) {
        /* Handle the stereo 8-bit and 16-bit cases */
        if (poi->bySampleSize == 1)
            return _stereo8bitcopy;
        else
            return _stereo16bitcopy;
    }
    else if ((poi->bySampleSize == 1) && ((poi->dwChanSelect >> dwFirst) == 0x3)) {
        /* Handle 2 -> 1 mono mixdowns when the source channels are contiguous */
        if (poi->bySampleSize == 1)
            return _mono2ch8bitcopy;
        else
            return _mono2ch16bitcopy;
    }
    else if ((poi->byNumChans == 1) && ((poi->dwChanSelect >> dwFirst) == 0x1)) {
        /* Handle mono transfers -- Used mostly for ASIO */
        if (poi->bySampleSize == 1)
            return _genericCopy;
        else
            return _mono1ch16bitcopy;
    }
	else
		return _genericCopy;
}


/* @func Performs the slowest but most general kind of sample copy.
 *  genericCopy can handle pretty much any type of transfer, but it
 *  pays for its generality with the overhead of a number of conditional
 *  tests.
 *
 *  NOTE: This code assumes that the raw device always provides 16-bit
 *  samples.  If this ever changes, this code will need to change.
 */
static void _genericCopy(stOpenInfo *poi, BYTE *from, BYTE *to, 
						 DWORD dwNumFrames)
{
	DWORD	dwFrame;
	DWORD   dwSampIndex;
	DWORD   dwChanBits;
	LONG    lSamp;
	SHORT  *pshSource;
	BYTE   *pbyDest;
    BYTE    byNumChanBits;

	pshSource = (SHORT*) from;
	pbyDest   = (BYTE*) to;

	/* Count the number of channel bits set */
	dwChanBits    = poi->dwChanSelect;
	byNumChanBits = 0;
	while (dwChanBits) {
		if (dwChanBits & 0x1)
			byNumChanBits++;
		dwChanBits >>=1;
	}

	for (dwFrame = 0; dwFrame < dwNumFrames; dwFrame++) {

		dwSampIndex = 0;
		lSamp      = 0;
		dwChanBits = poi->dwChanSelect;

		/* We iterate over all of the selected channels, extracting the
		 * appropriate sample from the sample frame and processing it
		 * appropriately.  */
		while (dwChanBits) {

			/* Find the next selected channel */
			while (dwChanBits & !(dwChanBits & 0x1)) {
				dwChanBits >>= 1;
				dwSampIndex++;
			}

			/* Mix and clamp */
			if (poi->byNumChans == 1) {
				lSamp += (LONG) pshSource[dwSampIndex];
			} else {
				/* Just store the sample in the frame */
				lSamp = (LONG) pshSource[dwSampIndex];

				if (poi->bySampleSize == 1) {
					lSamp >>= 8;
					*pbyDest++ = (BYTE) (0x80 + (CHAR) lSamp);
				} else {
					*((WORD*) pbyDest) = (WORD) lSamp;
					pbyDest += 2;
				}
			}

			/* Move to the next channel */
			dwChanBits >>= 1;
			dwSampIndex++;
		}

		/* Now that we've mixed down the selected channels into a single
		 * sample, we write the sample out to the destination buffer.  */
		if (poi->byNumChans == 1) {
			if (poi->bySampleSize == 1) {
                lSamp /= byNumChanBits;
				lSamp >>= 8;
                ASSERT(lSamp >= -128 && lSamp < 128);
				*pbyDest++ = (BYTE) (0x80 + (CHAR) lSamp);
			} else {
				*((WORD*) pbyDest) = (WORD) (lSamp / byNumChanBits);
				pbyDest += 2;
			}
		}

		/* Move to the next input frame */
		pshSource += poi->pdevice->byNumChans;
	}
}




/* @func Given two lists, concatenate the lists together and return
 *  a pointer to the concatenated list.  This function deals with either
 *  or both of the buffer pointers being NULL.
 */
static HRBBUFFER *
_concatBuffLists(HRBBUFFER *pFirstBuffList, HRBBUFFER *pSecondBuffList)
{
	if (pFirstBuffList == NULL)
		return pSecondBuffList;

	if (pSecondBuffList == NULL) {
		return pFirstBuffList;
	} else {
		/* Move to the end of the first buffer list */
		HRBBUFFER *pLastBuff = pFirstBuffList;

		while (pLastBuff->pNextBuffer != NULL)
			pLastBuff = pLastBuff->pNextBuffer;

		pLastBuff->pNextBuffer = pSecondBuffList;

		return pFirstBuffList;
	}
}


/* @func  Find the bit index of the first bit set in the given word. */
static DWORD 
_findFirstSet(DWORD value)
{
	DWORD first = 0;

	while (!(value & 0x1)) {
		value >>= 1;
		first++;
	}

	return first;
}


/* Copy a single channel from a sample frame into a monophonic output buffer.
 * This is way useful for many multitrack apps, which tend to like to work
 * in monophonic mode.
 */
static void
_mono1ch16bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames)
{
    DWORD dwSourceStride = poi->pdevice->byNumChans;
    SHORT *pshFrom = (SHORT*) from;
    SHORT *pshTo   = (SHORT*) to;

    /* Move to the appropriate offset in the sample frame */
    pshFrom += _findFirstSet(poi->dwChanSelect);

    while (dwNumFrames) {
        *pshTo = *pshFrom;
        pshTo++;
        pshFrom += dwSourceStride;
        dwNumFrames--;
    }
}



/* Note that both of the mono routines assume that we are mixing two contiguous
 * samples down to mono.  As a result, they don't work for mono downmixes of
 * either arbitrary number of channels or two discontiguous channels.  Sorry.
 */
static void
_mono2ch8bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames)
{
    DWORD dwSourceStride = (poi->pdevice->bySampleSize * poi->pdevice->byNumChans);

    /* Get to the first sample */
	from += (_findFirstSet(poi->dwChanSelect) * poi->pdevice->bySampleSize);

    ASSERT(dwNumFrames);
	while (dwNumFrames) {
		SHORT shSampLeft  = *((SHORT*) from) >> 9;
		SHORT shSampRight = *((SHORT*) (from + 2)) >> 9;

		*to++ = (BYTE) (0x80 + shSampLeft + shSampRight);
		from += dwSourceStride;
		dwNumFrames--;
	}
}

static void
_mono2ch16bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames)
{
	SHORT *shTo = (SHORT*) to;
    DWORD dwSourceStride = (poi->pdevice->bySampleSize * poi->pdevice->byNumChans);

	from += (_findFirstSet(poi->dwChanSelect) * poi->pdevice->bySampleSize);

    ASSERT(dwNumFrames);
    while (dwNumFrames) {
		/* We do the two shifts separately to avoid overflow in later add */
		SHORT shSampLeft  = *((SHORT*) from) >> 1;
		SHORT shSampRight = *((SHORT*) (from + 2)) >> 1;

		*shTo++   = (shSampLeft + shSampRight);
		from     += dwSourceStride;
		dwNumFrames--;
	}
}

/* Stereo copy routines.  These routines only work if the two channels are
 * contiguous. */
static void
_stereo8bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames)
{
	SHORT *pshFrom;
	DWORD dwStride = (poi->pdevice->bySampleSize * poi->pdevice->byNumChans) / 2;

	pshFrom = (SHORT*) (from + (_findFirstSet(poi->dwChanSelect) * poi->pdevice->bySampleSize));

    ASSERT(dwNumFrames);
	while (dwNumFrames) {
		*to++ = (BYTE) (0x80 + (*pshFrom >> 8));
		*to++ = (BYTE) (0x80 + (*(pshFrom+1) >> 8));

		pshFrom += dwStride;
		dwNumFrames--;
	}
}


static void
_stereo16bitcopy(stOpenInfo *poi, BYTE *from, BYTE *to, DWORD dwNumFrames)
{
	DWORD *pdwFrom;
	DWORD *pdwTo;
	DWORD  dwStride = (poi->pdevice->bySampleSize * poi->pdevice->byNumChans) / 4;

	pdwFrom = (DWORD*) (from + (_findFirstSet(poi->dwChanSelect) * poi->pdevice->bySampleSize));
	pdwTo   = (DWORD*) to;

    ASSERT(dwNumFrames);
	while (dwNumFrames) {
		*pdwTo++ = *pdwFrom;
		pdwFrom += dwStride;
		dwNumFrames--;
	}
}


/****************************************************************************
 * Private exported functions 
 ****************************************************************************/
EMUAPIEXPORT void
hrbpInvokeCallback(HRBID hrbid, HRBDEVICEVALUE device)
{
	stHRBState   *pstate  = FIND_STATE(hrbid);
	stDeviceInfo *pdevice;
	enIPType      iptype;

	if (pstate == NULL)
		return;

	pdevice = &pstate->devices[device];

	switch (device) {
	case hrbdevADC:   iptype = IP_ADCBUFFER;              break;
	case hrbdevMic:   iptype = IP_MICBUFFER;              break;
    case hrbdevFXOut: iptype = IP_FXBUFFER;               break;
	}

#ifndef ITM_RECORDING
	_intrCallback(pdevice->ipid, iptype, (DWORD) pdevice, NULL);
#endif
}
