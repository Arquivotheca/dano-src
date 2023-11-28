/******************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module saoutdev.h | 
* This module contains private class definitions for the Streaming Audio Output
* manager.  
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  --------------------------------------
* Michael Preston     Nov 13, 1997  Initial development.
*
******************************************************************************/

#ifndef __SAOUTPUT_PRIVATE_H
#define __SAOUTPUT_PRIVATE_H

#include "datatype.h"
#include "sacommon.h"
#include "saoutput.h"
#include "se8210.h"
#include "sm8210.h"
#include "ip8210.h"
#include "hrm8210.h"
#include "hal8210.h"
#include "llist.h"
#include "omega.h"
#include "win_mem.h"
#include "xlat8000.h"
#include "transe8k.h"

#ifdef DEBUG
//#define DEBUG_INST_VALS
#endif

enum enSALoopState {
   ensalsNotLooping, ensalsLooping1, ensalsLooping2, ensalsStaticLoop
};

struct stSAOutputConfig;
typedef struct stSAOutputBufferTag stSAOutputBuffer;

class stSASE8210OutputConfig
{
	public:
	
	void *operator new (size_t size)  { return NewAlloc(size); }
	void operator delete(void *ptr)   { DeleteAlloc(ptr); }
	
	DWORD dwSendAmountA;  // @field Amount for FX send A.
	DWORD dwSendAmountB;  // @field Amount for FX send B.
	DWORD dwSendAmountC;  // @field Amount for FX send C.
	DWORD dwSendAmountD;  // @field Amount for FX send D.
	DWORD dwSendRoutingA; // @field Routing for FX send A.
	DWORD dwSendRoutingB; // @field Routing for FX send B.
	DWORD dwSendRoutingC; // @field Routing for FX send C.
	DWORD dwSendRoutingD; // @field Routing for FX send D.
};

class SASE8210InstanceInfo
{
	public:
	
	void *operator new (size_t size)  { return NewAlloc(size); }
	void operator delete(void *ptr)   { DeleteAlloc(ptr); }

    SASE8210InstanceInfo() : trans(&xlatapp)
	{voiceHandle = NULL;
    ipHandle = 0;
	 bufferList = NULL;
	 pCurBuffer = NULL;
    pRetBufferList = NULL;
    pLoopBufferList = NULL;
    pRetLastBuffer = NULL;
	pTmpLastBuffer = NULL;
	 bCurBufferInFirstHalf = TRUE;
	 bPlaying = FALSE;
	 dwPausePos = 0;
    dwCurPos = 0;
    dwPlayPos = 0;
    bInCallback = FALSE;
    loopState = ensalsNotLooping;
    bInZeroLoop = FALSE;
    lPitch = 0;
#ifdef DEBUG_INST_VALS
    dwNumVals = 0;
    memset(vals, 0, 1000*sizeof(DWORD));
#endif
   }
	
	~SASE8210InstanceInfo() 
	{if (voiceHandle != NULL)
	    DeleteAlloc(voiceHandle);}
	 
	SEVOICE *voiceHandle;
	SAOutputCallback callback;
	stSAOutputBuffer *bufferList;
	stSAOutputBuffer *pCurBuffer;
    stSAOutputBuffer *pRetLastBuffer;
    stSAOutputBuffer *pTmpLastBuffer;
    stSAOutputBuffer *pRetBufferList;
    stSAOutputBuffer *pLoopBufferList;
	SMHANDLE smHandle;
	IPHANDLE ipHandle;
	BOOL bCurBufferInFirstHalf;
	BOOL bPlaying;
	DWORD dwPausePos;
	DWORD dwSampleRate;
	DWORD dwCurPos;
    DWORD dwCurBufferOffset;
    DWORD dwNextBufferOffset;
    DWORD dwPlayPos;
    DWORD dwRealStartloopAddr;
    BOOL bInCallback;
    enSALoopState loopState;
    DWORD dwLoopCount;
    BOOL bInZeroLoop;
    BOOL bIsSynchronized;
    BOOL bIsStartingSync;
	BYTE byNumChans;
	BYTE bySampleSize;
	BYTE byFrameSize;
    DWORD playbackMode;
    Emu8000TransApp xlatapp;
    Emu8000Translator trans;
    LONG lPitch;
#ifdef DEBUG_INST_VALS
    DWORD dwNumVals;
    DWORD vals[1000];
#endif

   static void dealloc(void *ptr) {delete (SASE8210InstanceInfo *)ptr;}
   static void *ccons(void *ptr)
      {return new SASE8210InstanceInfo(*(SASE8210InstanceInfo *)ptr);}

   static int match(void *m, void *mm)
      {return (((SASE8210InstanceInfo *)m)->voiceHandle[0] == (SEVOICE)mm);}
};

class SASE8210InstanceInfoList : public UnorderedLinkedList
{
	public:
	
	void *operator new (size_t size)  { return NewAlloc(size); }
	void operator delete(void *ptr)   { DeleteAlloc(ptr); }

   SASE8210InstanceInfoList() : UnorderedLinkedList(SASE8210InstanceInfo::dealloc,
                                                    SASE8210InstanceInfo::ccons) {}

   void Insert(SASE8210InstanceInfo* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   SASE8210InstanceInfo *GetCurItem() 
      {return (SASE8210InstanceInfo *)UnorderedLinkedList::GetCurItem();}

   void Find(SEVOICE voiceHandle)
     {FindFuncClass::Find(*(LinkedList *)this, (void *)voiceHandle,
                          SASE8210InstanceInfo::match);}
};



// @class A derived class which supports the 8210 Sound Engine
//  Interface.  It complies to the standard 
//
class SASE8210OutputDevice : public SAOutputDevice
{
public:
	SASE8210OutputDevice(HRMID, DWORD /*, stSASE8210OutputConfig *,
                         void *, DWORD*/);

	virtual ~SASE8210OutputDevice();
	virtual stSADevCaps *GetDevCaps();
    virtual DWORD GetResourceID();
	virtual DWORD Open(stSAOutputConfig *);
	virtual void Close(DWORD devHandle);

	virtual void AddBuffer(DWORD devHandle, stSAOutputBuffer *);
	virtual void Play(DWORD devHandle);
	virtual void Pause(DWORD devHandle);
	virtual void Stop(DWORD devHandle, stSAOutputBuffer **);
    virtual void SetLoopCount(DWORD devHandle, DWORD dwLoopCount);
    virtual BOOL HasBuffersQueued(DWORD devHandle);
	virtual DWORD GetCurrentSampleFrame(DWORD devHandle);
	virtual void SetCurrentSampleFrame(DWORD devHandle, DWORD dwCurSampleFrame);
    virtual void SetSampleRate(DWORD devHandle, DWORD dwSampleRate);
	virtual DWORD GetSampleRate(DWORD devHandle);
	virtual BYTE GetFrameSize(DWORD devHandle);
	virtual BYTE GetNumChans(DWORD devHandle);
	virtual BYTE GetSampleSize(DWORD devHandle);
    virtual BOOL SetSynthParam(DWORD devHandle, enSAOutputSynthParam param,
                               LONG lScale);
    virtual BOOL SetSynthConnection(DWORD devHandle, WORD wSource, WORD wControl,
                                    WORD wDestination, LONG lScale);
    virtual BOOL SetSendAmount(DWORD devHandle, DWORD dwSend, DWORD dwAmount);
    virtual BOOL SetSendRouting(DWORD devHandle, DWORD dwSend, DWORD dwAmount);

    virtual BOOL SetDeviceSendRouting(DWORD dwSend, DWORD dwRouting);
    //virtual BOOL SetDeviceSendMaxAmount(BYTE bySend, BYTE byMaxAmount);

    virtual DWORD GetSEVOICE(DWORD devHandle, DWORD dwChannel);

protected:
    BOOL _SetSynthParam(SASE8210InstanceInfo *inst, enSAOutputSynthParam param,
                        LONG lScale);
	void _AddBuffer(SASE8210InstanceInfo *inst, stSAOutputBuffer *);
	void _Stop(SASE8210InstanceInfo *inst, stSAOutputBuffer **);
	static BOOL ISRCallback(IPHANDLE, enIPType, DWORD, unIPReply *);
	static BOOL Callback(IPHANDLE, enIPType, DWORD, unIPReply *);
	void UpdateNextRegion(SASE8210InstanceInfo *inst);
#ifdef DEBUG
    static void NotifyCallBack(DWORD ptr, SEVOICE voiceHandle,
                               SENOTIFYEVENT why);
#endif

    HRMID hrmid;
	SEID seid;
	SMID smid;
	IPID ipid;
    HALID halid;
	stSASE8210OutputConfig devConfig;
	SASE8210InstanceInfoList instList;
    stSADevCaps devCaps;
};


// @class This class inherits most of its functionality from its
//  parent, the SASE8210OutputDevice.  
/*
class SA8210WaveOutputDevice : public SASE8210OutputDevice
{
public:
	SA8210WaveOutputDevice(HRMID, void *, DWORD);
};
*/

// @func Add standard devices to specified output mgr.
//extern EMUSTAT SAOutput8210AddDevices(SAOutputMgr*, HRMID);

#endif /* __SAOUTPUT_PRIVATE_H */
