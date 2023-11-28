/****************************************************************************
 *  Copyright (C) 1997 E-mu Systems Inc. All rights reserved.
 ****************************************************************************
 * @doc INTERNAL
 * @module sainput.cpp | 
 *  This file contains the source code for the Streaming Audio Input class.
 * 
 * @iex
 *  Revision History:
 *
 *  Person	Version		Date        	Description
 *  ------	-------		------------	------------------------
 *	JK		0.0002		Dec 02, 1997	Added the Microphone device 
 *  JK		0.0001		Nov 12, 1997	Created.
 *
 ****************************************************************************/

#include "datatype.h"
#include "sainput.h"
#include "emufolks.h"
#include "emuerrs.h"

static storeAttributes sainputAttrs =
{
  "SAInputMgr",
  {
    {'B', 2, 0, 0},
    copyright,
    {
      engineerName[eeJohnK],
      engineerName[eeEnd  ]
    }
  }
};

/****************************************************************************
 *  SAInputMgr class public definitions 
 ****************************************************************************/

// @func Constructor for the streaming audio input manager.  This
//  creates a new array and then statically fills it with classes.
//  Whenever we want to add a new device, we need to construct an
//  instance of it here and store it in the devices array.
//
SAInputMgr::SAInputMgr()
  : EmuModuleAttributes(&sainputAttrs)
{
	ClearError();

	_dwNumDevs=0;
    _dwNumDevEntries = 0;
	_devices=NULL;
}

// @func Input manager destructor
SAInputMgr::~SAInputMgr()
{
	ClearError();
	for (WORD w = 0; w < _dwNumDevs; w++) {
		delete _devices[w];
	}

    delete [] _devices;

    _devices = NULL;
	_dwNumDevs = 0;
    _dwNumDevEntries = 0;
}


// Add a new device.
//
void
SAInputMgr::AddDevice(SAInputDevice *pNewInputDev)
{
    if (_dwNumDevEntries == _dwNumDevs) {
        // We need to reallocate the array
        SAInputDevice **pNewDevs = new SAInputDevice*[_dwNumDevEntries + 8];
        if (pNewDevs == NULL) {
            SetError(oscErrorStatus, SAIERR_NO_MEMORY);
            return;
        }

        for (DWORD i = 0; i < _dwNumDevs; i++)
            pNewDevs[i] = _devices[i];

        _dwNumDevEntries += 8;

        if (_devices)
            delete[] _devices;

        _devices = pNewDevs;
    }

    _devices[_dwNumDevs++] = pNewInputDev;
}


// @func Retrieve a dev caps structure from the underlying device.  We require
//   that users of the DevCaps not modify any of the data in the structure, and
//   as a result we can simply pass them a pointer to the internal devcaps structure.
//
stSADevCaps *
SAInputMgr::GetDevCaps(DWORD dwDevNum)
{
	ClearError();

	if (dwDevNum > _dwNumDevs) {
		SetError(oscErrorStatus, SAIERR_BAD_DEVICEID);
		return NULL;
	}

	return _devices[dwDevNum]->GetDevCaps();
}


// @func Retrieve a specified device pointer.
//
SAInputDevice *
SAInputMgr::GetDevice(DWORD dwDevNum)
{
    ClearError();

    if (dwDevNum >= _dwNumDevs) {
        SetError(oscErrorStatus, SAIERR_BAD_DEVICEID);
        return NULL;
    }

    return _devices[dwDevNum];
}


// @func Create a new device client.  What we do here is ask the appropriate
//  device to open an instance of itself and return us a device handle.  We
//  then give the device pointer and the device handle to a newly constructed
//  instance of SAInputClient.
//
SAInputClient *
SAInputMgr::CreateDevClient(DWORD dwDevNum, stSAInputConfig *devConfig)
{
	ClearError();

	if (dwDevNum >= _dwNumDevs) {
		SetError(oscErrorStatus, SAIERR_BAD_DEVICEID);
		return NULL;
	}

	DWORD dwDevHandle = _devices[dwDevNum]->Open(devConfig);
	if (dwDevHandle == 0) {
		PropagateError(_devices[dwDevNum]);
		return NULL;
	} else {

		// Derive the sample rate
		DWORD dwSampleRate;
		switch (devConfig->sampleRate) {
		case saRate96K:    dwSampleRate = 96000; break;
		case saRate48K:	   dwSampleRate = 48000; break;
		case saRate44_1K:  dwSampleRate = 44100; break;
		case saRate32K:    dwSampleRate = 32000; break;
		case saRate24K:    dwSampleRate = 24000; break;
		case saRate22_05K: dwSampleRate = 22050; break;
		case saRate22K:    dwSampleRate = 22000; break;
		case saRate16K:    dwSampleRate = 16000; break;
		case saRate11_025K:dwSampleRate = 11025; break;
		case saRate11K:    dwSampleRate = 11000; break;
		case saRate8K:     dwSampleRate = 8000;  break;
		default:           dwSampleRate = 0;     break;
		}

		BYTE bySampleSize;
		switch (devConfig->sampleFormat) {
		case saFormatUnsigned8PCM: bySampleSize = 1;  break;
		case saFormatSigned8PCM:   bySampleSize = 1;  break;
		case saFormatSigned16PCM:  bySampleSize = 2;  break;
		case saFormatUnsigned16PCM:bySampleSize = 2;  break;
		}

		return new SAInputClient(_devices[dwDevNum], dwDevHandle, 
								 dwSampleRate, devConfig->byNumChans,
								 bySampleSize);
	}
}


/****************************************************************************
 *  SAInputDevice classes
 ****************************************************************************/

// @func Doesn't actually do anything
SAInputDevice::~SAInputDevice()
{
}


