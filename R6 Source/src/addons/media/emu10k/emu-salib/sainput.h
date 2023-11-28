/****************************************************************************
 *   Copyright (C) 1997 E-mu Systems Inc.  All rights reserved
 ****************************************************************************
 * @doc INTERNAL
 * @module sainput.h | 
 *  This file contains the class definitions required for the Streaming Audio
 *  input classes.  
 * 
 * @iex
 *  Revision History:
 *
 *  Person	Version		Date        	Description
 *  ------	-------		------------	------------------------
 *  JK		0.0001		Nov 12, 1997	Created.
 *
 ****************************************************************************
 * @doc EXTERNAL
 *  This document describes the enumerations, structures, and classes 
 *  associated with the support of the input and/or recording of incoming
 *  streams of audio data.  
 *
 * @contents1 Contents
 * @subindex Public Classes
 *
 ****************************************************************************/

#ifndef __SAINPUT_H
#define __SAINPUT_H

#include "datatype.h"
#include "emuatcls.h"
#include "sacommon.h"
#include "win_mem.h"

#define PropagateError PropogateError

class SAInputClient;
class SASync;


// @enum SAINPUTERRORTAG | The following list of errors may be set when
//  an error occurs somewhere in the SAINPUT
enum SAINPUTERRORTAG {
	SAIERR_BAD_SAMPLERATE,
	SAIERR_BAD_CHANNELS,
	SAIERR_BAD_SAMPLESIZE,
	SAIERR_BAD_DEVICEID,
	SAIERR_BAD_HANDLE,
	SAIERR_BAD_BUFFER,
	SAIERR_NO_MEMORY,
	SAIERR_ALREADY_STARTED,
	SAIERR_NOT_STARTED,
	SAIERR_UNKNOWN
};




typedef void (*SAInputCallback)(stSAInputBuffer*);


#define SAIC_LOOP_FLAG   0x1
#define SAIC_SYNC_FLAG   0x2

// @struct stSAInputConfig |
// @struct stSAInputConfig | Structure describing the configuration
//  of an input device.
//
struct stSAInputConfig
{
	SAInputCallback   callback;	    // @field The callback to call when a 
                                    //  buffer has been filled.  When running
                                    //  in looping mode (see below) the
                                    //  callback may be set to NULL.
	enSASampleRate    sampleRate;   // @field The sample rate for the device.
	enSASampleFormat  sampleFormat; // @field The sample format for the data
                                    // in the buffers.
	BYTE              byNumChans;	// @field The number of channels to record.
    DWORD             dwChanSelect; // Specifies which channels to record from
    DWORD             dwFlags;      // @field Control flags for input.
                                    //  Currently, the only supported
                                    //  flag is SAIC_LOOP_FLAG.  When
                                    //  This flag is set, the system
                                    //  will just cycle through all of
                                    //  the queued buffers, filling them with
                                    //  data.  When it reaches the last
                                    //  queued buffer, it loops back to the
                                    //  first buffer queued.  When a 
                                    //  buffer is completed, the callback
                                    //  routine will be invoked with a pointer
                                    //  to that buffer.  Note, however,
                                    //  that in this mode the application
                                    //  must not queue any additional buffers
                                    //  once recording has been started.
};


// @class SAInputDevice | 
//  An abstract class which defines the type signature of a 
//  Streaming Audio Input Device.
class SAInputDevice : public OmegaClass
{
public:
	friend class SAInputClient;

	virtual ~SAInputDevice();

	virtual stSADevCaps *   GetDevCaps() = 0;
    virtual DWORD           GetResourceID() = 0;

	virtual DWORD Open(stSAInputConfig *) = 0;
	virtual void  Close(DWORD) = 0;	

	// Device function dispatch routines; one of these exists for each
	// of the client's functions.
	virtual void AddBuffer(DWORD devHandle, stSAInputBuffer *) = 0;
	virtual void Record(DWORD devHandle) = 0;
	virtual void Stop(DWORD devHandle, stSAInputBuffer **) = 0;
	virtual void Reset(DWORD devHandle, stSAInputBuffer **) = 0;
	virtual DWORD GetCurrentSampleFrame(DWORD devHandle) = 0;
	virtual BOOL HasBuffersQueued(DWORD devHandle) = 0;
};


// @class The SAInputMgr manages all of the input devices associated with
//  a particular board.  It provides methods for determining how many 
//  input devices are available, for determining the capabilities of a 
//  particular input device, and for opening a particular device.
// 
class SAInputMgr : public EmuModuleAttributes
{
public:
	// @cmember Allocate a new instance of the SAInputMgr class
	void *operator new (size_t size)  { return NewAlloc(size); };

	// @cmember Deallocate an instance of the SAInputMgr class
	void operator delete(void *ptr)   { DeleteAlloc(ptr); }

	// @cmember Construct an instance of the Streaming Audio Input Device
	//  Manager.
	// 
	// @parm HRMID | hrmID | The HRMID of the board whose input devices are
	//  to be managed.
	SAInputMgr();

	// @cmember Destroy an instance of the Streaming Audio Input Device
	//  Manager.
	// 
	~SAInputMgr();

    // @cmember Add a new device to the manager.  In general, only
    //  a device-specific function, like SAInput8210AddDevices,
    //  should call this routine.
    //
    void AddDevice(SAInputDevice*);

	// @cmember Returns the number of input devices managed.  These devices
	//  may later be referenced by calling subsequent function with a DWORD
	//  value between 0 and N -1 (inclusive), where N is the number of
	//  devices managed.
	// 
	// @rdesc  Returns the number of devices managed.
	DWORD  GetNumDevs()  { return _dwNumDevs; }

    // @cmember Retrieve a particular device pointer.
    SAInputDevice *GetDevice(DWORD dwDevNum);

	// @cmember Returns a pointer to the stSADevCaps stucture for a 
	//  specified device.  The returned pointer references data which is
	//  private to the device, and the call must neither modify the data
	//  in the DevCaps structure nor attempt to free it.
	//
	// @parm DWORD | dwDevNum | The device number whose capabilities are
	//  to be returned.
	// @rdesc  Returns a pointer to the device's DevCaps structure.
    stSADevCaps *GetDevCaps(DWORD dwDevNum);

	// @cmember Returns a pointer to a newly opened instance of the specified
	//  device.  This instance can be used to actually read audio data in
	//  from a device.  The caller is responsible for destroying the 
	//  device instance when done.
	// 
	// @parm DWORD | dwDevNum | The number of the device to open.
	// @parm stSAInputConfig * | devConfig | The device configuration.  This
	//  describes the sample rate, sample format, and number of channels
	//  to be used when reading from the device.  
	//
	// @rdesc Returns a pointer to an SAInputInstance instance.  
	SAInputClient *CreateDevClient(DWORD dwDevNum, stSAInputConfig *devConfig);

private:
	DWORD		   _dwNumDevs;
    DWORD          _dwNumDevEntries;
	SAInputDevice **_devices;
};


// @class An SAInputClient object contains all of the device state needed
//  to actually start recording from a device.  In many ways, an
//  SAInputInstance is similar to a File Descriptor.  
class SAInputClient : public OmegaClass
{
public:
	// Constructor and destructors may only be called by our friend,
	// SAInputDevice.  
	SAInputClient(SAInputDevice *psaid, DWORD dev, DWORD sampleRate, BYTE numChans,
		          BYTE sampleSize): OmegaClass("SAInputClient") {
		ClearError();
		pInputDevice = psaid; dwDevHandle = dev;
		dwSampleRate = sampleRate;
		byNumChans   = numChans;
		bySampleSize = sampleSize;
		byFrameSize  = byNumChans * bySampleSize;
	}

	~SAInputClient() {
		ClearError();
		pInputDevice->Close(dwDevHandle);
		pInputDevice = NULL;
		dwDevHandle = 0;
	}

	// @cmember Enqueues a buffer for the reception of data.  Enqueuing
	//  a buffer makes it Enqueuing a buffer makes it available to
	//  receive sample data once recording starts on the specified device. 
    //  Buffers are returned either incrementally as they are filled with 
    //  data via the callback or all at once when <f hrbDevReset> is called.
    //  The client may enqueue multiple buffers with a single call by linking
    //  the buffers together using the <t pNextBuffer> field in the buffer
    //  structures.  If only a single buffer is being passed, the client must
    //  ensure that the <t pNextBuffer> field is set to NULL.
	//
	// @parm stSAInputBuffer * | psaib | A pointer to a buffer which will
	//  receive data.
	void AddBuffer(stSAInputBuffer *psaib) { 
		pInputDevice->AddBuffer(dwDevHandle, psaib);
		PropagateError(pInputDevice);
	}

	// @cmember This routine begins filling any buffers on the queue with 
	// incoming sample data from the selected device.  If no buffers have 
	// been queued, the system will just throw away audio data until the 
	// client enqueues the first buffer with <f AddBuffer>.  
	//
	void Record() {
		pInputDevice->Record(dwDevHandle);
		PropagateError(pInputDevice);
	}

	// @cmember Stops input, resets the sample counter, and dequeues all 
	// buffers from the queue.  The dequeued buffers will be returned via 
	// the <t retBuffers> pointer.  A client should call this routine when 
	// finished reading samples for a period of time.  This routine can be 
	// called either when the device is started or when it is paused.
	//
	// @parm stSAInputBuffer ** | retBufferList | A pointer to the stSAInputBuffer
	// pointer which will recieve the address of the head of the dequeued 
	// buffer list.  If retBuffer is NULL, the buffers will be dequeued but 
	// the head will not be returned; in this case, the HRB manager assumes 
	// that the client has retained pointers to the buffers.
	void Reset(stSAInputBuffer **ppsaib) {
		pInputDevice->Reset(dwDevHandle, ppsaib);
		PropagateError(pInputDevice);
	}

	// @cmember Stops reading samples and hands back to the client a pointer 
	//	to the head of a list of buffers which have been filled with data 
	//  but have not yet been returned to the client.  Any unfilled buffers 
	//  will remain on the queue, and the sample counter is _not_ reset.  
	//  This function provides a convenient way of pausing input temporarily.
	void Stop(stSAInputBuffer **ppsaib) {
		pInputDevice->Stop(dwDevHandle, ppsaib);
		PropagateError(pInputDevice);
	}

	// @cmember Returns the sample number of sample most recently DMA'ed into
	//  memory.  This can be used to provide a rough correspondence between
	//  the current time and a sample number.  This function may only be
	//  called if the device has been started.
	//
	// @rdesc Returns the sample number.  If the device has not yet been 
	//  started or is paused, the sample number number of the next sample to 
	// be read will be returned.
	DWORD GetCurrentSampleFrame() {
		return pInputDevice->GetCurrentSampleFrame(dwDevHandle);
	}

	// @cmember Attempts to close the device.  If the device still has
	//  buffers enqueued, the close fails.
	BOOL HasBuffersQueued() {
		return pInputDevice->HasBuffersQueued(dwDevHandle);
	}

	DWORD GetSampleRate() { return dwSampleRate; }
	BYTE  GetFrameSize()  { return byFrameSize;  }
	BYTE  GetNumChans()   { return byNumChans;   }
	BYTE  GetSampleSize() { return bySampleSize; }
    SAInputDevice *GetDevice() { return pInputDevice; }
    DWORD GetDeviceHandle() { return dwDevHandle; }

protected:
	SAInputDevice *pInputDevice;
	DWORD          dwDevHandle;
	DWORD          dwSampleRate;
	BYTE		   byFrameSize;
	BYTE           byNumChans;
	BYTE           bySampleSize;
};


#endif /* __SAINPUT_H */
