#ifndef _EMU_CONSUMER_H
#define _EMU_CONSUMER_H

/* ++++++++++

   FILE:  EmuConsumer.h
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1995-1998 by Be Incorporated.  All Rights Reserved.
	
	
+++++ */

#include <TimedEventQueue.h>
#include <MediaEventLooper.h>
#include <BufferConsumer.h>
#include <Controllable.h>
#include <TimeSource.h>
#include <Autolock.h>
#include "emuaddon.h"
#include "sout8210.h"

#define NO_SYNC 0
#define SYNCING 1
#define SYNCED 2

class BEmuConsumer : public BMediaEventLooper,
					 public BBufferConsumer,
					 public BControllable,
					 public BTimeSource
{
public:
				BEmuConsumer(BEmuAddOn* addon, dev_spec* spec,
							  char* name, int32 id, status_t* status);
	virtual 	~BEmuConsumer();

	status_t HandleMessage(int32 message, const void * data, size_t size);

	// From BMediaNode
	virtual	BMediaAddOn* AddOn(long *) const;
	
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
	// From BMediaEventLooper
	virtual void ControlLoop();
	virtual void SetRunMode(run_mode mode);
	virtual	void HandleEvent(const media_timed_event* event,
							 bigtime_t lateness,
							 bool realTimeEvent);

	virtual status_t DoBuffer(const media_timed_event* event,
							  BBuffer* ibuf,
							  bigtime_t lateness);

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

	// Stuff from BTimeSource
	virtual	status_t TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved);

	virtual void	ConstructControlWeb();
	virtual void	SetTimeSource(BTimeSource * time_source);
	virtual	void	NodeRegistered();
	virtual status_t DeleteHook(BMediaNode*);

private:
			status_t	BEmuConsumer_Initialize();
			status_t	SetupEmu();

	dev_spec*			fDevSpec;
	media_format		fPlayFormat;
	media_destination	fPlayDestination;
	media_source		fPlaySource;
	BEmuAddOn*			fAddOn;
	sem_id				fWriteCompletion;

	BLocker				mLock;

	char*				fOutputBuffer;
	stSAOutputBuffer	fOutputBuffers[N_OUTPUT_BUFFERS];
	int32				fNextBuffer;

	bigtime_t			fLastPublishedPerf;
	bigtime_t			fLastPublishedReal;
	float				fDrift;

	int32				fPerfSyncSample;
	bigtime_t			fPerfSyncStart;
	bigtime_t			fDataChangeTime;

	SAOutputMgr*		fOutputMgr;
	SAOutputDevice*		fOutputDevice;
	SAOutputClient*		fClient;
	bool				fPlaying;

	int32				fInternalID;
	int32				fProducerStatus;

	char				fName[B_MEDIA_NAME_LENGTH];
	void				MakeName();
};

#endif
