/******************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module saoutput.cpp | 
* This file contains the source code for the Streaming Audio Output class.  
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  --------------------------------------
* Michael Preston     Dec  5, 1997  Initial development.
*
******************************************************************************/

#include "datatype.h"
#include "saoutput.h"
#include "emufolks.h"
#include "emuerrs.h"
#include "string.h"
#include "emuattr.h"
#include "dbg8210.h"

static storeAttributes saoutputAttrs =
{
  "SAOutputMgr",
  {
    {'B', 2, 0, 0},
    copyright,
    {
      engineerName[eeMikeP],
      engineerName[eeEnd  ]
    }
  }
};


/****************************************************************************
 *  SAOutputMgr class public definitions 
 ****************************************************************************/

// @func Constructor for the streaming audio output manager.  This
//  creates a new array and then statically fills it with classes.
//  Whenever we want to add a new device, we need to construct an
//  instance of it here and store it in the devices array.
//
SAOutputMgr::SAOutputMgr()
  : EmuModuleAttributes(&saoutputAttrs)
{
	ClearError();

    _dwNumDevs = 0;
    _dwNumDevEntries = 0;
    _devices = NULL;
}

// @func Input manager destructor
SAOutputMgr::~SAOutputMgr()
{
	ClearError();
    if (_dwNumDevs==0) return;

	for (WORD w = 0; w < _dwNumDevs; w++)
		delete _devices[w];

    delete [] _devices;
    _devices=NULL;

	_dwNumDevs = 0;                              
    _dwNumDevEntries = 0;

}


// @func Add a new device
void
SAOutputMgr::AddDevice(SAOutputDevice *pNewDev)
{
    if (_dwNumDevEntries == _dwNumDevs) {
        // We need to reallocate the array
        SAOutputDevice **pNewDevs = new SAOutputDevice*[_dwNumDevEntries + 8];
        if (pNewDevs == NULL) {
            SetError(oscErrorStatus, SAOERR_NO_MEMORY);
            return;
        }

        for (DWORD i = 0; i < _dwNumDevs; i++)
            pNewDevs[i] = _devices[i];

        _dwNumDevEntries += 8;

        if (_devices)
            delete[] _devices;

        _devices = pNewDevs;
    }

    _devices[_dwNumDevs++] = pNewDev;
}


// @func Retrieve a dev caps structure from the underlying device.  We require
//   that users of the DevCaps not modify any of the data in the structure, and
//   as a result we can simply pass them a pointer to the internal devcaps structure.
//
stSADevCaps *
SAOutputMgr::GetDevCaps(DWORD dwDevNum)
{
	ClearError();

	if (dwDevNum >= _dwNumDevs)
	{
		SetError(oscErrorStatus, SAOERR_BAD_DEVICEID);
		return NULL;
	}

   if (_devices[dwDevNum] == NULL)
      return NULL;

	return _devices[dwDevNum]->GetDevCaps();

}


// @func Retrieve the device pointer for a specified device.
//
SAOutputDevice *
SAOutputMgr::GetDevice(DWORD dwDevNum)
{
    ClearError();

    if (dwDevNum >= _dwNumDevs) {
        SetError(oscErrorStatus, SAOERR_BAD_DEVICEID);
        return NULL;
    }

    return _devices[dwDevNum];
}


// @func Create a new device client.  What we do here is ask the appropriate
//  device to open an instance of itself and return us a device handle.  We
//  then give the device pointer and the device handle to a newly constructed
//  instance of SAInputClient.
//
SAOutputClient *
SAOutputMgr::CreateDevClient(DWORD dwDevNum, stSAOutputConfig *devConfig)
{
	ClearError();

	if (dwDevNum >= _dwNumDevs)
	{
		SetError(oscErrorStatus, SAOERR_BAD_DEVICEID);
		return NULL;
	}

	DWORD dwDevHandle = _devices[dwDevNum]->Open(devConfig);
	if (dwDevHandle == 0)
	{
		PropogateError(_devices[dwDevNum]);
		return NULL;
	}
	else
	{
		return new SAOutputClient(_devices[dwDevNum], dwDevHandle);
	}
}

BOOL
SAOutputMgr::SetDeviceSendRouting(DWORD dwDevNum, DWORD dwSend, DWORD dwRouting)
{
    if (dwDevNum >= _dwNumDevs)
        return FALSE;
    return _devices[dwDevNum]->SetDeviceSendRouting(dwSend, dwRouting);
}
/*
BOOL
SAOutputMgr::SetDeviceSendMaxAmount(DWORD dwDevNum, BYTE bySend, BYTE byMaxAmount)
{
    if (dwDevNum >= _dwNumDevs)
        return FALSE;
    return _devices[dwDevNum]->SetDeviceSendMaxAmount(bySend, byMaxAmount);
}
*/
