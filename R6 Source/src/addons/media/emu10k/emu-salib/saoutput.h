/******************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module saoutput.h | 
* This file contains the class definitions required for the Streaming Audio
* output classes.  
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  --------------------------------------
* Michael Preston     Nov 13, 1997  Initial development.
*
******************************************************************************/

#ifndef __SAOUTPUT_H
#define __SAOUTPUT_H

#include "datatype.h"
#include "emuatcls.h"
#include "sacommon.h"
#include "win_mem.h"

class SAOutputClient;

// @enum enSAOutputSynthParam | Synthesis parameters that may be set with
//  SetSynthParam().  All parameters are expressed in SoundFont units.
enum enSAOutputSynthParam
{
    // General parameters
    saospGain,
    saospPan,
    saospPitch,
    saospFilterCutoff,
    saospFilterQ,
    // Volume envelope
    saospDelayVolEnv,
    saospAttackVolEnv,
    saospHoldVolEnv,
    saospDecayVolEnv,
    saospSustainVolEnv,
    saospReleaseVolEnv,
    // Modulation envelope
    saospDelayModEnv,
    saospAttackModEnv,
    saospHoldModEnv,
    saospDecayModEnv,
    saospSustainModEnv,
    saospReleaseModEnv,
    // LFOs
    saospDelayModLFO,
    saospFreqModLFO,
    saospDelayVibLFO,
    saospFreqVibLFO,
    // Modulators
    saospModLFOToPitch,
    saospVibLFOToPitch,
    saospModLFOToFilterCutoff,
    saospModLFOToGain,
    saospModEnvToPitch,
    saospModEnvToFilterCutoff,
//
    saospEndSynthParams
};

#define saospNumSynthParams saospEndSynthParams

// @enum SAOUTPUTERRORTAG | The following list of errors may be set when
//  an error occurs somewhere in the SAOUTPUT
enum SAOUTPUTERRORTAG {
	SAOERR_BAD_SAMPLERATE,
	SAOERR_BAD_CHANNELS,
	SAOERR_BAD_SAMPLESIZE,
	SAOERR_BAD_DEVICEID,
	SAOERR_BAD_HANDLE,
	SAOERR_BAD_BUFFER,
	SAOERR_NO_MEMORY,
	SAOERR_ALREADY_STARTED,
	SAOERR_NOT_STARTED,
	SAOERR_UNKNOWN
};

// Loop macros

#define SA_LOOP_START   0x1
#define SA_LOOP_END     0x2
#define SA_STATIC       0x4
#define SA_LOOP_FOREVER 0xffffffff

#define sapmNoControl            0
#define sapmControlFrequency     1
#define sapmControlFrequencyOnly 1
#define sapmControlAll           2
#define sapmSync                 4

// @struct stSAOutputConfig | Structure describing the configuration
//  of an output device.
//
struct stSAOutputConfig
{
	SAOutputCallback  callback;		// @field The callback to call when a 
									//  buffer has been filled.
	DWORD             dwSampleRate;	// @field The sample rate for the device.
	enSASampleFormat  sampleFormat; // @field The sample format for the data
									// in the buffers.
	BYTE              byNumChans;	// @field The number of channels to play.
    DWORD              playbackMode;
};

/* @struct stSAOutputBuffer | Describes the output data buffer and contains
 *  any associated data.
 */
typedef struct stSAOutputBufferTag {
	void *        virtAddr;     /* @field The virtual address of the base
								 *  of the allocated buffer to fill.  */
   DWORD         dwSize;       /* @field The size of the buffer in bytes. Note that
                                 *  the underlying buffer routines may actually reduce
                                 *  this value to something which is a multiple of the
                                 *  framesize in order to assure that a sample frame
                                 *  doesn't span a buffer boundary. */
   DWORD         dwLoopFlags;  /* @field Loop flags */
   DWORD         dwNumLoops;   /* @field Number of times to loop */
   struct stSAOutputBufferTag * pNextBuffer;  /* @field The next buffer in the data
								 *  buffer chain.  It should be set to NULL
								 *  if only a single buffer is being passed. */
	DWORD         dwUser1;      /* @field This field is available to the
								 *  client for any purpose.  Its contents
								 *  will remain constant from the time this
								 *  buffer is passed to SAOutput until the
								 *  buffer is returned via a callback.  */
	DWORD         dwUser2;      /* @field Available to the client.  */
	DWORD         dwUser3;      /* @field Available to the client.  */
	DWORD         dwReserved;  /* @field Reserved for SAOutput use.  */
} stSAOutputBuffer;


// @class SAOutputDevice | 
//  An abstract class which defines the type signature of a 
//  Streaming Audio Output Device.
class SAOutputDevice : public OmegaClass
{
public:
	friend class SAOutputClient;

	virtual ~SAOutputDevice()=0;

	virtual stSADevCaps *GetDevCaps()=0;
    virtual DWORD GetResourceID()=0;
	virtual DWORD Open(stSAOutputConfig *)=0;
	virtual void Close(DWORD devHandle)=0;	

	// Device function dispatch routines; one of these exists for each
	// of the client's functions.
	virtual void AddBuffer(DWORD devHandle, stSAOutputBuffer *)=0;
	virtual void Play(DWORD devHandle)=0;
	virtual void Pause(DWORD devHandle)=0;
	virtual void Stop(DWORD devHandle, stSAOutputBuffer **)=0;
    virtual void SetLoopCount(DWORD devHandle, DWORD dwLoopCount)=0;
    virtual BOOL HasBuffersQueued(DWORD devHandle)=0;
	virtual DWORD GetCurrentSampleFrame(DWORD devHandle)=0;
	virtual void SetCurrentSampleFrame(DWORD devHandle, DWORD dwCurSampleFrame)=0;
    virtual void SetSampleRate(DWORD devHandle, DWORD dwSampleRate)=0;
	virtual DWORD GetSampleRate(DWORD devHandle)=0;
	virtual BYTE GetFrameSize(DWORD devHandle)=0;
	virtual BYTE GetNumChans(DWORD devHandle)=0;
	virtual BYTE GetSampleSize(DWORD devHandle)=0;
    virtual BOOL SetSynthParam(DWORD devHandle, enSAOutputSynthParam param,
                               LONG lScale)=0;
    virtual BOOL SetSynthConnection(DWORD devHandle, WORD wSource, WORD wControl,
                                    WORD wDestination, LONG lScale)=0;
    virtual BOOL SetSendAmount(DWORD devHandle, DWORD dwSend, DWORD dwAmount)=0;
    virtual BOOL SetSendRouting(DWORD devHandle, DWORD dwSend, DWORD dwRouting)=0;

    virtual BOOL SetDeviceSendRouting(DWORD dwSend, DWORD dwRouting)=0;
    //virtual BOOL SetDeviceSendMaxAmount(BYTE bySend, BYTE byMaxAmount)=0;
};


// @class The SAOutputMgr manages all of the output devices associated with
//  a particular board.  It provides methods for determining how many 
//  output devices are available, for determining the capabilities of a 
//  particular output device, and for opening a particular device.
// 
class SAOutputMgr : public EmuModuleAttributes
{
public:
	friend class SAOutputClient;

	// @cmember Allocate a new instance of the SAOutputMgr class
	void *operator new (size_t size)  { return NewAlloc(size); }

	// @cmember Deallocate an instance of the SAOutputMgr class
	void operator delete(void *ptr)   { DeleteAlloc(ptr); }

	// @cmember Construct an instance of the Streaming Audio Output Device
	//  Manager.
	// 
	// @parm HRMID | hrmID | The HRMID of the board whose output devices are
	//  to be managed.
	SAOutputMgr();

	// @cmember Destroy an instance of the Streaming Audio Output Device
	//  Manager.
	// 
	~SAOutputMgr();

    // @cmember Give the output manager a new device to manage.
    void AddDevice(SAOutputDevice*);

	// @cmember Returns the number of output devices managed.  These devices
	//  may later be referenced by calling subsequent function with a DWORD
	//  value between 0 and N -1 (inclusive), where N is the number of
	//  devices managed.
	// 
	// @rdesc  Returns the number of devices managed.
	DWORD            GetNumDevs()  { return _dwNumDevs; }

    // @cmember Return a particular device pointer.
    SAOutputDevice *GetDevice(DWORD dwDevNum);

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
	//  device.  This instance can be used to actually play audio data
	//  through a device.  The caller is responsible for destroying the 
	//  device instance when done.
	// 
	// @parm DWORD | dwDevNum | The number of the device to open.
	// @parm stSAOutputConfig * | devConfig | The device configuration.  This
	//  describes the sample rate, sample format, and number of channels
	//  to be used when reading from the device.  
	//
	// @rdesc Returns a pointer to an SAOutputInstance instance.  
	SAOutputClient *CreateDevClient(DWORD dwDevNum, stSAOutputConfig *devConfig);

    BOOL SetDeviceSendRouting(DWORD dwDevNum, DWORD dwSend, DWORD dwRouting);
    // Do we want (or need) this?
    //BOOL SetDeviceSendMaxAmount(DWORD dwDevNum, BYTE bySend, BYTE byMaxAmount);

private:
	DWORD		   _dwNumDevs;
    DWORD          _dwNumDevEntries;
	SAOutputDevice **_devices;
};


// @class An SAOutputClient object contains all of the device state needed
//  to actually start playing to a device.  In many ways, an
//  SAOutputInstance is similar to a File Descriptor.  
class SAOutputClient : public OmegaClass
{
public:
	// Constructor and destructors may only be called by our friend,
	// SAOutputDevice.  
	SAOutputClient(SAOutputDevice *psaod, DWORD dev) :
	   OmegaClass("SAOutputClient")
	{
		ClearError();
		pOutputDevice = psaod;
		dwDevHandle = dev;
		//dwSampleRate = sampleRate;
		//byNumChans   = numChans;
		//bySampleSize = sampleSize;
		//byFrameSize  = byNumChans * bySampleSize;
	}

	~SAOutputClient()
	{
		ClearError();
		pOutputDevice->Close(dwDevHandle);
		pOutputDevice = NULL;
		dwDevHandle = 0;
	}

	// @cmember Enqueues a buffer for the streaming of data.  Enqueuing a
    //  buffer makes it available to stream sample data once playing starts
    //  on the specified device.  Buffers are returned either incrementally
    //  as they are used via the callback or all at once when <f Stop> is
    //  called.  The client may enqueue multiple buffers with a single call
    //  by linking the buffers together using the <t pNextBuffer> field in
    //  the buffer structures.  If only a single buffer is being passed, the
    //  client must ensure that the <t pNextBuffer> field is set to NULL.
	//
	// @parm stSAOutputBuffer * | psaib | A pointer to a buffer which will
	//  be played.
	void AddBuffer(stSAOutputBuffer *psaob)
	{ 
		pOutputDevice->AddBuffer(dwDevHandle, psaob);
		PropogateError(pOutputDevice);
	}

	// @cmember This routine begins playing any buffers on the queue with 
	// on the selected device.  If no buffers have been queued, the system
	// will just play zero audio data until the client enqueues the first
	// buffer with <f AddBuffer>.  
	//
	void Play()
	{
		pOutputDevice->Play(dwDevHandle);
		PropogateError(pOutputDevice);
	}

	// @cmember Stops output, but does not dequeue buffers from the queue.
	void Pause()
	{
		pOutputDevice->Pause(dwDevHandle);
		PropogateError(pOutputDevice);
	}

    // @cmember Stops output and dequeues all buffers from the queue.  The
    //  dequeued buffers will be returned via the <t ppsaob> pointer.  A
    //  client should call this routine when finished playing samples for a
    //  period of time.  This routine can be called either when the device
    //  is started or when it is paused.
	//
	// @parm stSAOutputBuffer ** | ppsaob | A pointer to the stSAOutputBuffer
    //  pointer which will recieve the address of the head of the dequeued
    //  buffer list.  If retBuffer is NULL, the buffers will be dequeued but
    //  the head will not be returned; in this case, the SAOutput manager
    //  assumes that the client has retained pointers to the buffers.
    void Stop(stSAOutputBuffer **ppsaob)
	{
		pOutputDevice->Stop(dwDevHandle, ppsaob);
		PropogateError(pOutputDevice);
	}

	void SetLoopCount(DWORD dwLoopCount)
	{
		pOutputDevice->SetLoopCount(dwDevHandle, dwLoopCount);
		PropogateError(pOutputDevice);
	}

   BOOL HasBuffersQueued()
   {
		return pOutputDevice->HasBuffersQueued(dwDevHandle);
   }

	// @cmember Returns the sample number of sample most recently DMA'ed into
	//  memory.  This can be used to provide a rough correspondence between
	//  the current time and a sample number.  This function may only be
	//  called if the device has been started.
	//
	// @rdesc Returns the sample number.  If the device has not yet been 
	//  started or is paused, the sample number number of the next sample to 
	// be read will be returned.
	DWORD GetCurrentSampleFrame()
	{
		return pOutputDevice->GetCurrentSampleFrame(dwDevHandle);
	}

	void SetCurrentSampleFrame(DWORD dwCurSampleFrame)
	{
		pOutputDevice->SetCurrentSampleFrame(dwDevHandle, dwCurSampleFrame);
		PropogateError(pOutputDevice);
	}

    void SetSampleRate(DWORD dwSampleRate)
    {
	    pOutputDevice->SetSampleRate(dwDevHandle, dwSampleRate);
		PropogateError(pOutputDevice);
    }

	DWORD GetSampleRate()
	{
	   return pOutputDevice->GetSampleRate(dwDevHandle);
	}
	
	BYTE  GetFrameSize()  
	{
	   return pOutputDevice->GetFrameSize(dwDevHandle);
	}
	
	BYTE  GetNumChans()   
	{
	   return pOutputDevice->GetNumChans(dwDevHandle);
	}
	
	BYTE  GetSampleSize() 
	{
	   return pOutputDevice->GetSampleSize(dwDevHandle);
	}

    BOOL SetSynthParam(enSAOutputSynthParam param, LONG lScale)
    {
        return pOutputDevice->SetSynthParam(dwDevHandle, param, lScale);
    }

    BOOL SetSynthConnection(WORD wSource, WORD wControl, WORD wDestination,
                            LONG lScale)
    {
        return pOutputDevice->SetSynthConnection(dwDevHandle, wSource, wControl,
                                                 wDestination, lScale);
    }

    BOOL SetSendAmount(DWORD dwSend, DWORD dwAmount)
    {
		return pOutputDevice->SetSendAmount(dwDevHandle, dwSend, dwAmount);
    }

    BOOL SetSendRouting(DWORD dwSend, DWORD dwRouting)
    {
		return pOutputDevice->SetSendRouting(dwDevHandle, dwSend, dwRouting);
    }

    SAOutputDevice * GetDevice()
    {
        return pOutputDevice;
    }

    DWORD GetDeviceHandle()
    { 
        return dwDevHandle;
    }

protected:
   SAOutputDevice *pOutputDevice;
	DWORD           dwDevHandle;
};

#endif /* __SAOUTPUT_H */
