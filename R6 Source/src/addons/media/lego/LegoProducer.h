#ifndef _LEGO_PRODUCER_H
#define _LEGO_PRODUCER_H

/* ++++++++++

   FILE:  LegoProducer.h
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1995-1998 by Be Incorporated.  All Rights Reserved.
	
	
+++++ */

#include "BufferProducer.h"
#include "Controllable.h"
#include "legoaddon.h"
#include <Locker.h>

class BLegoProducer : public BBufferProducer, public BControllable
{
public:
				BLegoProducer(BLegoAddOn*, dev_spec* spec,
							  char* name, int32 id, status_t* status);
	virtual 	~BLegoProducer();

	status_t HandleMessage(int32 message, const void * data, size_t size);
	virtual status_t 	DoRun();

	// From BMediaNode
	virtual	BMediaAddOn* AddOn(long *) const;	/* Who instantiated you -- or NULL for app class */
	virtual	port_id ControlPort() const;
	virtual void	HandleBadMessage(int32 code, const void * data, size_t size);
	
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

protected:
	// Stuff from BMediaNode
	virtual	void Start(bigtime_t performance_time);
	virtual	void Stop(bigtime_t performance_time, bool immediate);
	virtual	void TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);

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

	virtual void	ConstructControlWeb();
	virtual void	SetTimeSource(BTimeSource * time_source);
	virtual void	NodeRegistered();

private:
			status_t	BLegoProducer_Initialize();
	virtual void		Flush(BBuffer*, int32 used, bigtime_t capture_time);

	dev_spec*			fDevSpec;


	thread_id 			fRunThreadID;
	bool				fRunThreadKeepGoing;
	static int32		RunThread(void*);	
	
	thread_id			fServiceThreadID;
	bool				fServiceThreadKeepGoing;
	static int32		ServiceThread(void*);
	bigtime_t			fSchedLatency;
	
	port_id				fControlPort;
	media_format		fCaptureFormat;
	media_source		fCaptureSource;
	media_destination	fCaptureDestination;
	BLegoAddOn			*fAddOn;
	
	int					fd;
	sem_id				fReadCompletion;
	int32				fReadsPending;

	// Stuff related to buffer production
	BLocker				mLock;
	BBufferGroup*		fBufferGroup;
	BBufferGroup*		fDefaultBufferGroup;

	bool				mRunning;
	bigtime_t			mStartTime;
	bigtime_t			mStopTime;

	int32				fInputSize;
	audio_buffer_header* fInputHeader;
	char*				fInputBuffer;
	int32				fNextBuffer;

	double				fUsecPerByte;
	bigtime_t			fCaptureTime;
	int32				fInputPointer;
	int32				fChangeTag;
	bool				fDisableOutput;

	bigtime_t			fDataChangeTime;
	int32				fInternalID;

	char				fName[B_MEDIA_NAME_LENGTH];
	void				MakeName();
};

#endif
