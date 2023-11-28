/****************************************************************************
 *  Copyright (C) 1997 E-mu Systems Inc. All rights reserved.
 ****************************************************************************
 * @doc INTERNAL
 * @module sain8210.cpp | 
 *  This file contains the source code for the E10K1 devices.
 * 
 * @iex
 *  Revision History:
 *
 *  Person	Version		Date        	Description
 *  ------	-------		------------	------------------------
 *  JK		0.0001		Aug 14, 1998	Transferred from sainput.cpp.
 *
 ****************************************************************************/

#include "datatype.h"
#include "sain8210.h"
#include "emufolks.h"
#include "hrb8210.h"
#include "emuerrs.h"


// @func Standard function for adding expected devices to
//  specified input manager.
EMUSTAT
SAInput8210AddDevices(SAInputMgr *theMgr, HRMID hrmid)
{
    SAInputDevice *pNewWaveIn = new SA8210WaveInputDevice(hrmid);
    if (pNewWaveIn == NULL)
        return SAIERR_NO_MEMORY;
    else
        theMgr->AddDevice(pNewWaveIn);

#if 0
    pNewWaveIn = new SA8210EightChannelInputDevice(hrmid);
    if (pNewWaveIn == NULL)
        return SAIERR_NO_MEMORY;
    else
        theMgr->AddDevice(pNewWaveIn);
#endif

    return SUCCESS;
}



// @func Store the HRB-specific data that we can get our hands on.
//  Later derived classes will actually do more with this stuff.
SAHRBInputDevice::SAHRBInputDevice(HRMID theHrmid, HRBDEVICEVALUE dev)
{
    hrmid  = theHrmid;
	hrbid  = hrmGetHRBID(hrmid);
	hrbdev = dev;

	// Initialize as much of the device capabilities as we can based
	// on the device capabilities
	if (hrbQueryDevice(hrbid, hrbdev, &devcaps) != SUCCESS) {
		SetError(oscErrorStatus, SAIERR_BAD_HANDLE);
	} else {
		ClearError();
	}
}

// @func We don't currently need to do any operations on instance destruction
SAHRBInputDevice::~SAHRBInputDevice()
{
}


// @func The HRB class simply maintains a per-instance copy of the device caps.
//  We just return this.
stSADevCaps *
SAHRBInputDevice::GetDevCaps()
{
	return &devcaps;
}


DWORD
SAHRBInputDevice::GetResourceID()
{
    return hrmid;
}


void
SAHRBInputDevice::AddBuffer(DWORD dwDevHandle, stSAInputBuffer *buffer)
{
	buffer->pNextBuffer = NULL;
	EMUSTAT status = hrbDevAddBuffers(dwDevHandle, buffer);
	if (status != SUCCESS)
		SetError(oscErrorStatus, SAIERR_BAD_HANDLE);
	else
		ClearError();
}


void
SAHRBInputDevice::Record(DWORD dwDevHandle)
{
	EMUSTAT status = hrbDevStart(dwDevHandle);
	if (status != SUCCESS) {
		if (status == HRBERR_BAD_HANDLE)
			SetError(oscErrorStatus, SAIERR_BAD_HANDLE);
		else
			SetError(oscWarningStatus, SAIERR_ALREADY_STARTED);
	} else {
		ClearError();
	}
}


void
SAHRBInputDevice::Stop(DWORD devHandle, stSAInputBuffer **buffer)
{
	EMUSTAT status = hrbDevStop(devHandle, buffer);
	if (status != SUCCESS) {
		switch (status) {
		case HRBERR_BAD_HANDLE:	  SetError(oscErrorStatus, SAIERR_BAD_HANDLE); break;
		case HRBERR_NOT_STARTED:  SetError(oscWarningStatus, SAIERR_NOT_STARTED); break;
		default:                  SetError(oscErrorStatus, SAIERR_UNKNOWN); break;
		}
	} else {
		ClearError();
	}
}


void
SAHRBInputDevice::Reset(DWORD devHandle, stSAInputBuffer **buffer)
{
	EMUSTAT status = hrbDevReset(devHandle, buffer);
	if (status != SUCCESS) {
		switch (status) {
		case HRBERR_BAD_HANDLE:	 SetError(oscErrorStatus, SAIERR_BAD_HANDLE);    break;
		case HRBERR_NOT_STARTED: SetError(oscWarningStatus, SAIERR_NOT_STARTED); break;
		default:                 SetError(oscErrorStatus, SAIERR_UNKNOWN);       break;
		}
	} else {
		ClearError();
	}
}


DWORD
SAHRBInputDevice::GetCurrentSampleFrame(DWORD devHandle)
{
	return hrbDevGetCurrentSampleFrame(devHandle);
}


BOOL
SAHRBInputDevice::HasBuffersQueued(DWORD devHandle)
{
	ClearError();
	return hrbDevHasBuffersQueued(devHandle);
}


void
SAHRBInputDevice::Close(DWORD dwDevHandle)
{
	ClearError();
	hrbCloseDevice(dwDevHandle);
}


// @func Set up an HRBDEVCONFIG structure based on the data
//  provided in an stSAInputConfig structure.
//
void
SAHRBInputDevice::setupDevConfig(stSAInputConfig *config,
								 HRBDEVCONFIG *devconfig)
{
	devconfig->device       = hrbdev;
	devconfig->callback     = config->callback;
	devconfig->sampleRate   = config->sampleRate;
	devconfig->byNumChans   = config->byNumChans;
    devconfig->dwFlags      = 0;

    if (config->dwFlags & SAIC_LOOP_FLAG)
        devconfig->dwFlags |= HRB_CONFIGFLAGS_LOOP;

    if (config->dwFlags & SAIC_SYNC_FLAG)
        devconfig->dwFlags |= HRB_CONFIGFLAGS_SYNCABLE;

	// Figure out the actual number of bytes per sample
	switch (config->sampleFormat) {
	case saFormatUnsigned8PCM:
		devconfig->bySampleSize = 1;
		break;
	case saFormatSigned16PCM:
		devconfig->bySampleSize = 2;
		break;

	default:
		// This is a sleazy hack; we know that this will 
		// be invalid, and we want the HRB code to bitch
		// so that the error code gets set correctly later.
		devconfig->bySampleSize = 80;
	}

	// When we're in mono mode we want to mix down both channels
	// into a mono signal rather than favor the left or right channel.
	devconfig->dwChanSelect = 0x3;
}


// @func Performs the common case open handling code for our subclasses.
//  In particular, it parses out the errors that can be returned by 
//  hrbOpenDevice.
//
HRBHANDLE
SAHRBInputDevice::openHRBDevice(HRBID hrbid, HRBDEVCONFIG *devconfig)
{
	HRBHANDLE hrbh;
	EMUSTAT status = hrbOpenDevice(hrbid, devconfig, &hrbh);

	if (status != SUCCESS) {
		SAINPUTERRORTAG error;
		switch (status) {
		case HRBERR_BAD_HANDLE:		error = SAIERR_BAD_HANDLE;	break;
		case HRBERR_BAD_DEVICE:		error = SAIERR_BAD_DEVICEID; break;
		case HRBERR_BAD_SAMPLERATE:	error = SAIERR_BAD_SAMPLERATE; break;
		case HRBERR_BAD_CHANNELS:	error = SAIERR_BAD_CHANNELS; break;
		case HRBERR_BAD_SAMPLESIZE: error = SAIERR_BAD_SAMPLESIZE; break;
		case HRBERR_NO_MEM:			error = SAIERR_NO_MEMORY; break;
		default:					error = SAIERR_UNKNOWN; break;
		}

		SetError(oscErrorStatus, error);
		return 0;
	} else {
		return hrbh;
	}
}


/***************************************************************************
 * SA8210WaveInputDevice method definitions
 *
 * NOTE:  We inherit most of the device dispatch routines from our parent
 * class, SAHRBInputDevice.  The only things we override are the constructor.
 ***************************************************************************/

SA8210WaveInputDevice::SA8210WaveInputDevice(HRMID hrmid)
  : SAHRBInputDevice(hrmid, hrbdevADC)
{
	// Stuff the device name
	strcpy(devcaps.szDevName, "APS Wave In");
}


DWORD
SA8210WaveInputDevice::Open(stSAInputConfig *config)
{
	ClearError();

	// Copy the fields which map directly
	HRBDEVCONFIG devconfig;
	setupDevConfig(config, &devconfig);

	return openHRBDevice(hrbid, &devconfig);
}


/****************************************************************************
 *  SA8210MicInputDevice -- Provides support for the 8KHz Mono microphone
 *  input.
 ****************************************************************************/

SA8210MicInputDevice::SA8210MicInputDevice(HRMID hrmid)
  : SAHRBInputDevice(hrmid, hrbdevMic)
{
	// Stuff the device name
	strcpy(devcaps.szDevName, "Microphone");
}


DWORD
SA8210MicInputDevice::Open(stSAInputConfig *config)
{
	ClearError();

	// Copy the fields which map directly
	HRBDEVCONFIG devconfig;
	setupDevConfig(config, &devconfig);

	return openHRBDevice(hrbid, &devconfig);
}


/****************************************************************************
 *  SA8210FxInputDevice -- Provides support for the 48KHz FX Output
 *  bus.  
 ****************************************************************************/

SA8210FxInputDevice::SA8210FxInputDevice(HRMID hrmid)
  : SAHRBInputDevice(hrmid, hrbdevFXOut)
{
    // Stuff the device name
    strcpy(devcaps.szDevName, "APS FX Output Record");
}


/****************************************************************************
 *  SA8210EightChannelInputDevice -- Provides a simplified eight-channel
 *  interface which is used by ASIO.
 ****************************************************************************/

SA8210EightChannelInputDevice::SA8210EightChannelInputDevice(HRMID hrmid)
  : SA8210FxInputDevice(hrmid)
{
    // Stuff the device name
    strcpy(devcaps.szDevName, "APS 8 Channel In");
    devcaps.dwChannels = 8;
}


DWORD
SA8210EightChannelInputDevice::Open(stSAInputConfig *config)
{
    ClearError();

    if (config->byNumChans == 0 || config->byNumChans > 8) {
        SetError(SAIERR_BAD_CHANNELS);
        return 0;
    }

    // Make sure that no more than numchans bits are set in the
    // channel select.
    DWORD dwNumBitsSet = 0;
    DWORD dwBitSet = config->dwChanSelect;
    while (dwBitSet) {
        if (dwBitSet & 0x1)
            dwNumBitsSet++;
        dwBitSet >>=1;
    }

    if ((dwNumBitsSet != config->byNumChans) ||
        (config->dwChanSelect >= 0x100))
    {    
        SetError(SAIERR_BAD_CHANNELS);
        return 0;
    }
    
    HRBDEVCONFIG devconfig;
    setupDevConfig(config, &devconfig);

    devconfig.dwChanSelect  = config->dwChanSelect;

    return openHRBDevice(hrbid, &devconfig);
}



