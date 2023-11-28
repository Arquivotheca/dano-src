#ifndef _EMU_PRODUCER_H
#define _EMU_PRODUCER_H

/* ++++++++++

   FILE:  EmuProducer.h
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1995-1998 by Be Incorporated.  All Rights Reserved.
	
	
+++++ */

#include <TimedEventQueue.h>
#include <MediaEventLooper.h>
#include <BufferProducer.h>
#include <Controllable.h>
#include <TimeSource.h>
#include <Autolock.h>
#include "emuaddon.h"
#include "sain8210.h"

class audio_buffer_header;

struct buf_spec
{
  int32		sample;
  bool		pending;
};
  
class BEmuProducer : public BMediaEventLooper,
					 public BBufferProducer,
					 public BControllable,
					 public BTimeSource
{
public:
				BEmuProducer(BEmuAddOn*, dev_spec* spec,
							  char* name, int32 id, status_t* status);
	virtual 	~BEmuProducer();

	// From BMediaNode
	virtual	BMediaAddOn* AddOn(long *) const;
	virtual status_t HandleMessage(int32 message, const void * data, size_t size);
	
	//	BBufferProducer
	virtual	status_t FormatSuggestionRequested(media_type type,int32 quality,media_format * format);
	virtual	status_t FormatProposal(const media_source & output, media_format * format);
	virtual	status_t FormatChangeRequested(
				const media_source & source,
				const media_destination & destination,
				media_format * io_format,
				int32 * out_change_count);

	virtual	status_t GetNextOutput(int32 * cookie, media_output * out_destination);
	virtual	status_t DisposeOutputCookie(int32 cookie);
	virtual	status_t SetBufferGroup(const media_source &for_source, BBufferGroup * group);
	virtual	status_t VideoClippingChanged(
				const media_source &for_source,
				int16 num_shorts,
				int16 * clip_data,
				const media_video_display_info & display,
				int32 * out_from_change_count);

	// From BControllable
	virtual	status_t	GetParameterValue(int32 id, bigtime_t * last_change, void * value,size_t * ioSize);
	virtual	void 		SetParameterValue(int32 id, bigtime_t when, const void * value, size_t size);
	virtual	status_t 	StartControlPanel(BMessenger * out_messenger);

	virtual status_t DoBuffer(stSAInputBuffer* sabuf);

protected:
	// Stuff from BMediaEventLooper
	virtual void ControlLoop();
	virtual	void HandleEvent(const media_timed_event* event,
							 bigtime_t lateness,
							 bool realTimeEvent);

	// BBufferProducer
	virtual status_t GetLatency(bigtime_t* out_latency);
	virtual	status_t PrepareToConnect(
				const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name);
	virtual	void Connect(
				status_t error, 
				const media_source & source,
				const media_destination & destination,
				const media_format & format,
				char * out_name);
	virtual	void Disconnect(const media_source &what, const media_destination &where);
	virtual	void LateNoticeReceived(
				const media_source & what,
				bigtime_t how_much,
				bigtime_t performance_time);
	virtual	void EnableOutput(
				const media_source & what,
				bool enabled,
				int32 * change_tag);

	// Stuff from BTimeSource
	virtual void SetRunMode(run_mode mode);
	virtual	status_t TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved);

	virtual void	ConstructControlWeb();
	virtual void	SetTimeSource(BTimeSource * time_source);
	virtual	void	NodeRegistered();
	virtual status_t DeleteHook(BMediaNode*);

private:
			status_t	BEmuProducer_Initialize();
 			status_t	SetupEmu();
			status_t	SetBufGroup(BBufferGroup* group);

	dev_spec*			fDevSpec;
	media_format		fCaptureFormat;
	media_source		fCaptureSource;
	media_destination	fCaptureDestination;
	int32				fCaptureSize;
	BEmuAddOn			*fAddOn;
	bool				fStopping;
	bigtime_t			fStopTime;

	// Stuff related to buffer production
	BLocker				mLock;
	BBufferGroup*		fBufferGroup;
	stSAInputBuffer		fInputBuffers[N_INPUT_BUFFERS];
	buf_spec			fBufSpec[N_INPUT_BUFFERS];
	int32				fInputBufSize;
	int32				fSampleRate;
	float				fUsecPerFrame;

	bigtime_t			fLastPublishedPerf;
	bigtime_t			fLastPublishedReal;
	float				fDrift;
	int32				fNextSample;
	bigtime_t			fDataChangeTime;
	int32				fPlaybackSample;
	int32				fPlaybackSync;

	SAInputMgr*			fInputMgr;
	SAInputDevice*		fInputDevice;
	SAInputClient*		fClient;
	bool				fCapturing;

	int32				fInternalID;
	int32				fChangeTag;
	bool				fDisableOutput;

	char				fName[B_MEDIA_NAME_LENGTH];
	void				MakeName();
};

#endif
