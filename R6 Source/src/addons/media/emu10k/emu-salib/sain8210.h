/****************************************************************************
 *  Copyright (C) 1997 E-mu Systems Inc.  All rights reserved.
 ****************************************************************************
 * @doc INTERNAL
 * @module sain8210.h | Contains private class definitions for
 *  the E10K1 hardware's input devices.
 *
 * @iex
 *  Revision History:
 *
 *  Person	Revision	Date		Description
 *  ------	--------	----		-------------------------
 *	JK		0.001		Nov 12, 97	Created.
 *  JK      0.002       Aug 14, 98  Rearranged files so that

 ****************************************************************************/

#ifndef __SAINPUT_PRIVATE_H
#define __SAINPUT_PRIVATE_H

#include "datatype.h"
#include "sacommon.h"
#include "sainput.h"
#include "hrm8210.h"
#include "hrb8210.h"

struct stSAInputConfig;


// @func This function automatically adds all of the
//  appropriate devices to the given SAInputMgr.  It
//  allows upper level code to avoid knowing too much
//  about what devices are provided by particular hardware
//  implementations.
//
extern EMUSTAT SAInput8210AddDevices(SAInputMgr*, HRMID);

// @class A derived class which supports the 8210 Hardware Record
//  Buffer Interface.  It complies to the standard 
//
class SAHRBInputDevice : public SAInputDevice
{
public:
    friend class SA8210SyncHandler;

	SAHRBInputDevice(HRMID, HRBDEVICEVALUE);

	virtual ~SAHRBInputDevice();
	virtual stSADevCaps * GetDevCaps();
    virtual DWORD GetResourceID();
	virtual DWORD Open(stSAInputConfig *) = 0;
	virtual void  Close(DWORD);	

	virtual void AddBuffer(DWORD devHandle, stSAInputBuffer *);
	virtual void Record(DWORD devHandle);
	virtual void Stop(DWORD devHandle, stSAInputBuffer **);
	virtual void Reset(DWORD devHandle, stSAInputBuffer **);
	virtual DWORD GetCurrentSampleFrame(DWORD devHandle);
	virtual BOOL HasBuffersQueued(DWORD devHandle);

protected:
	void            setupDevConfig(stSAInputConfig*, HRBDEVCONFIG*);
	DWORD		    openHRBDevice(HRBID, HRBDEVCONFIG*);
    HRMID           hrmid;
	HRBID           hrbid;
	HRBDEVICEVALUE  hrbdev;
	stSADevCaps     devcaps;
};

// @class This class inherits most of its functionality from its
//  parent, the SAHRBInputDevice.  
class SA8210WaveInputDevice : public SAHRBInputDevice
{
public:
	SA8210WaveInputDevice(HRMID);
	virtual DWORD Open(stSAInputConfig *);
};


// @class This class inherits most of functionality from the 
//  SAHRBInputDevice.  It adds a new constructor and open
//  routine to support the Microphone.
class SA8210MicInputDevice : public SAHRBInputDevice
{
public:
	SA8210MicInputDevice(HRMID);
	virtual DWORD Open(stSAInputConfig *);
};


// @class This supports the FX Recording interface.
//  Theoretically, up to 32 channels can be recorded.
//  (Note, though, that the hardware guys are skeptical
//  that this will actually work.  The chip only has a 
//  32-byte FIFO, which means that in order to record
//  32 channels we need to be able to do a 32-byte burst
//  write every 10 us.  Apparently, this is a pretty tall
//  order given PCI bus utilization.
//
class SA8210FxInputDevice : public SAHRBInputDevice
{
public:
    SA8210FxInputDevice(HRMID);
};


// @class 
class SA8210EightChannelInputDevice : public SA8210FxInputDevice
{
public:
    SA8210EightChannelInputDevice(HRMID);
    virtual DWORD Open(stSAInputConfig *);
};


#endif /* __SAINPUT_PRIVATE_H */
