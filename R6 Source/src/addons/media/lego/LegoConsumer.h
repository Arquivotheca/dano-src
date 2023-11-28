#ifndef _LEGO_CONSUMER_H
#define _LEGO_CONSUMER_H

/* ++++++++++

   FILE:  LegoConsumer.h
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1995-1998 by Be Incorporated.  All Rights Reserved.
	
	
+++++ */

#include <TimedEventQueue.h>
#include <Locker.h>
#include "BufferConsumer.h"
#include "Controllable.h"
#include "TimeSource.h"
#include "legoaddon.h"

#define NO_SYNC 0
#define SYNCING 1
#define SYNCED 2

class BLegoConsumer : public BBufferConsumer,
					public BControllable,
					public BTimeSource
{
public:
				BLegoConsumer(BLegoAddOn* addon, dev_spec* spec,
							  char* name, int32 id, status_t* status);
	virtual 	~BLegoConsumer();

	status_t HandleMessage(int32 message, const void * data, size_t size);
	virtual status_t 	DoRun();

	// From BMediaNode
	virtual	BMediaAddOn* AddOn(long *) const;	/* Who instantiated you -- or NULL for app class */
	virtual	port_id ControlPort() const;
	
	// From BBufferConsumer
	virtual	void BufferReceived(BBuffer * buffer);
	virtual	status_t AcceptFormat(const media_destination &dest, media_format * format);
	virtual	status_t GetNextInput(int32 * cookie, media_input * out_input);
	virtual	void DisposeInputCookie(int32 cookie);
	virtual	status_t FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format & format);

	// From BControllable
	virtual	status_t	GetParameterValue(int32 id, bigtime_t * last_change, void * value,size_t * ioSize);
	virtual	void 		SetParameterValue(int32 id, bigtime_t when, const void * value, size_t size);
	virtual	status_t 	StartControlPanel(BMessenger * out_messenger);

protected:
	// Stuff from BMediaNode
	virtual	void Start(bigtime_t performance_time);
	virtual	void Stop(bigtime_t performance_time, bool immediate);
	virtual void Seek(bigtime_t media_time, bigtime_t performance_time);
	virtual	void TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);

	// From BBufferConsumer
	virtual	void Disconnected(		/* be sure to call BBufferConsumer::Connected()! */
				const media_source &producer,
				const media_destination &where);
	virtual	status_t Connected(		/* be sure to call BBufferConsumer::Connected()! */
				const media_source &producer,	/* here's a good place to request buffer group usage */
				const media_destination &where,
				const media_format & with_format,
				media_input * out_input);
	virtual	void ProducerDataStatus(
				const media_destination &for_whom,
				int32 status,
				bigtime_t at_media_time);
	virtual	status_t GetLatencyFor(
				const media_destination &what,
				bigtime_t * out_latency,
				media_node_id * out_timesource);

	virtual void	ConstructControlWeb();
	virtual void	SetTimeSource(BTimeSource * time_source);
	virtual void	NodeRegistered();
	
	// Stuff from BTimeSource
	virtual	status_t TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved);
		
private:
			status_t	BLegoConsumer_Initialize();

	dev_spec*			fDevSpec;
	thread_id 			fRunThreadID;
	bool				fRunThreadKeepGoing;
	static int32		RunThread(void*);	
	
	thread_id			fServiceThreadID;
	bool				fServiceThreadKeepGoing;
	static int32		ServiceThread(void*);

	
	port_id				fControlPort;
	thread_id 			fControlPortThread;
	media_format		fPlayFormat;
	media_destination	fPlayDestination;
	media_source		fPlaySource;
	BLegoAddOn			*fAddOn;

	int					fd;
	sem_id				fBufferReceivedSem;
	sem_id				fWriteCompletion;
	bool				fWritePending;
	bool				mExtraTiming;
	
	// Stuff related to buffer consumption
	BLocker				mLock;
	bool				mRunning;
	bool				mStarting;
	bool				mStopping;
	bool				mSeeking;
	bigtime_t			mStartTime;
	bigtime_t			mStopTime;
	bigtime_t			mSeekTo;
	bigtime_t			mSeekAt;
	BTimedEventQueue	mQueue;

	audio_buffer_header* fOutputHeader;
	char*				fOutputBuffer;
	bool				fOutputZero[N_OUTPUT_BUFFERS];
	int32				fNextBuffer;

	int32				fSampleRate;
	size_t				fOutputBufSize;
	bigtime_t			fMyLatency;
	bigtime_t			fSchedLatency;
	bigtime_t			fLastPublishedPerf;
	bigtime_t			fLastPublishedReal;
	float				fDrift;

	int32				fPerfSync;
	int32				fPerfSyncBufferNumber;
	bigtime_t			fPerfSyncStart;
	bigtime_t			fPerfSyncDelta;
	bigtime_t			fDataChangeTime;

	bool				fSendZero;
	bigtime_t			fNextBufferTime;
	int32				fInternalID;

	char				fName[B_MEDIA_NAME_LENGTH];
	void				MakeName();
};

#endif
