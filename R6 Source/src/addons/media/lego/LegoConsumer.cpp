/* ++++++++++

   FILE:  LegoConsumer.cpp
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

+++++ */

#include "LegoConsumer.h"

#include <byteorder.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <R3MediaDefs.h>
#include <scheduler.h>
#include <assert.h>
#include <Autolock.h>

#include "ParameterWeb.h"
#include "tr_debug.h"
#include "Buffer.h"
#include "MediaAddOn.h"
#include "sound.h"
#include "sonic_vibes.h"
#include "mixer_i586.h"

//#define TRACE printf
//#define DEBUG_DO_MESSAGE	1
//#define DEBUG_SERVICE_RUN	1
//#define DEBUG_ITERATE_FORMATS	1
#define BUFFERS(x...) //printf
#define DUMP_FILE 0
#define TEST_TONE 0

#if !DEBUG
#define printf(x...)
#endif

#if 0
#define PUBLISH_TIME(PT, RT, D) { \
	printf("PublishTime(%Ld, %Ld, %f)\n", PT, RT, (float)(D)); \
	PublishTime(PT, RT, D); }
#else
#define PUBLISH_TIME(PT, RT, D) { \
	PublishTime(PT, RT, D); }
#endif

#define X(N) fprintf(stderr,"<%d>\n",(N));
#define PERR(EXP) { \
	status_t err = (EXP); \
	if (err < 0) \
	fprintf (stderr, "BLegoConsumer: " #EXP " == 0x%x (%s)\n", err, strerror(err)); \
}

#define IOCTL(FD,OP,AD,SZ) (ioctl(FD,OP,AD,SZ) < 0 ? errno : B_OK)

#define DEFAULT_TIMEOUT 6000000L
#define TOTAL_SIZE (fOutputBufSize + sizeof(audio_buffer_header))
#define CHUNK_SIZE(FORMAT) (fOutputBufSize / 4 * ((FORMAT).u.raw_audio.format & 0xf) * ((FORMAT).u.raw_audio.channel_count))
#define STARTING_DRIFT 1.0
#define TOTAL_LATENCY (2 * fSchedLatency + fOutputBufSize / 10);
#define DEFAULT_SCHED_LATENCY 1000

#if DUMP_FILE
int dump_file = -1;
int dump_cnt = 0;
#endif

//extern "C"
//void convertBufferFloatToShort(int16* dest, float* src, int32 count, float scale);

enum {
  BLC_CD_LEVEL = 'BLC ',
  BLC_CD_MUTE,
  BLC_LINE_IN_LEVEL,
  BLC_LINE_IN_MUTE,
  BLC_DAC_LEVEL,
  BLC_DAC_MUTE,
  BLC_LOOP_LEVEL,
  BLC_LOOP_MUTE,
  BLC_SPEAKER_LEVEL,
  BLC_SPEAKER_MUTE,
  BLC_SAMPLE_RATE,
  BLC_NULL_PARAMS
};


BLegoConsumer::BLegoConsumer(BLegoAddOn *addon, dev_spec* spec,
							 char* name, int32 id, status_t* status)
	: BMediaNode(name),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	fRunThreadID(B_BAD_THREAD_ID),
	fServiceThreadID(B_BAD_THREAD_ID),
	fControlPort(-1),
	fAddOn(addon),
	mQueue(),
	fInternalID(id)
{
  mExtraTiming = true;
  fDevSpec = spec;
  fSchedLatency = DEFAULT_SCHED_LATENCY;
  fOutputBufSize = fDevSpec->preferred_buffer_size;
  fMyLatency = TOTAL_LATENCY;
  status_t err = BLegoConsumer_Initialize();
  if (*status)
	*status = err;
}


BLegoConsumer::~BLegoConsumer()
{
	close_port(fControlPort);

	if (fServiceThreadID != B_BAD_THREAD_ID) {
		fServiceThreadKeepGoing = false;
		int32 st;
		wait_for_thread(fServiceThreadID, &st);
		fServiceThreadID = B_BAD_THREAD_ID;
	}

	if (fRunThreadID != B_BAD_THREAD_ID) {
		fRunThreadKeepGoing = false;
		int32 st;
		wait_for_thread(fRunThreadID, &st);
		fRunThreadID = B_BAD_THREAD_ID;
	}

	fDevSpec->close();
}

void
BLegoConsumer::SetTimeSource(BTimeSource * time_source)
{
	BMediaNode::SetTimeSource(time_source);
}

status_t
BLegoConsumer::BLegoConsumer_Initialize()
{
	// Now setup our service port
	fControlPort = create_port(10, "BLegoConsumer service port");

	// Let the system know we do physical output
	AddNodeKind(B_PHYSICAL_OUTPUT);
	
	fd = fDevSpec->fd;
	sv_handshake data;
	mExtraTiming = false;
	if (ioctl(fd, SV_SECRET_HANDSHAKE, &data, sizeof(data)) >= 0) {
		mExtraTiming = true;
	}

	// Create a completion semaphore
	fWriteCompletion = create_sem(N_OUTPUT_BUFFERS - 1, "output completion");
	fWritePending = false;
	ioctl(fd, SOUND_SET_PLAYBACK_COMPLETION_SEM,
		  &fWriteCompletion, sizeof(fWriteCompletion));

	sound_setup p;
	status_t err = IOCTL(fd, SOUND_GET_PARAMS, &p, sizeof(p)); 
	if (err < 0) {
	  fprintf(stderr, "BLegoConsumer::BLegoConsumer_Initialize"
			  "- SOUND_GET_PARAMS returned: %lx\n", err);
	  p.sample_rate = kHz_44_1;
	}
	else {
	  // print out sound params
	  printf("  Sample Rate: %d\n", p.sample_rate);
	  printf("  Play Format: %d\n", p.playback_format);
	  // unmute DAC
	  p.left.dac_mute = 0;
	  p.right.dac_mute = 0;
	  ioctl(fd, SOUND_SET_PARAMS, &p, sizeof(p));
	}
		
	fPlayFormat.type = B_MEDIA_RAW_AUDIO;
	fPlayFormat.u.raw_audio.frame_rate = sr_enum_to_float(p.sample_rate);
	if (fPlayFormat.u.raw_audio.frame_rate == 0)
	  fPlayFormat.u.raw_audio.frame_rate = 44100;
	fPlayFormat.u.raw_audio.channel_count = 2;
	fPlayFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	fPlayFormat.u.raw_audio.byte_order
	  = (B_HOST_IS_BENDIAN ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN);
	fPlayFormat.u.raw_audio.buffer_size = CHUNK_SIZE(fPlayFormat);

	// Set the play and record semaphores
	fBufferReceivedSem = create_sem(0,"buffer_received");
	fSampleRate = (int32) fPlayFormat.u.raw_audio.frame_rate;
	mRunning = false;
	mStarting = false;
	mStopping = false;
	mSeeking = false;
	mStartTime = 0;		/* when to start in performance time */
	mStopTime = 0;		/* when to stop in performance time */
	mSeekTo = 0;
	mSeekAt = 0;

	fLastPublishedReal = 0;
	fDrift = STARTING_DRIFT;
	fPerfSync = NO_SYNC;
	fDataChangeTime = 0;

	fPlaySource = media_source::null;
	fPlayDestination.port = fControlPort;
	fPlayDestination.id = 0;
	
	for (int i = 0; i < N_OUTPUT_BUFFERS; i++)
	  fOutputZero[i] = false;
	fNextBuffer = 0;

	MakeName();

	return B_OK;
}

void
BLegoConsumer::NodeRegistered() {
	ConstructControlWeb();
	fServiceThreadKeepGoing = true;
	fServiceThreadID = spawn_thread(ServiceThread, "Lego Service Thread",
		B_REAL_TIME_PRIORITY, this);
	fSchedLatency = estimate_max_scheduling_latency(fServiceThreadID);
	resume_thread(fServiceThreadID);
}

/******************
	BMediaNode
*******************/

BMediaAddOn*
BLegoConsumer::AddOn(int32* flavor_id) const
{	
	// Who instantiated you -- or NULL for app class
	*flavor_id = fInternalID;
	return fAddOn;
}

port_id
BLegoConsumer::ControlPort() const
{
	//TRACE("BLegoConsumer::ControlPort: %d\n", fControlPort);
	return fControlPort;
}

status_t 
BLegoConsumer::HandleMessage(int32 code, const void *data, size_t size)
{
#ifdef DEBUG_DO_MESSAGE
	printf("BLegoConsumer::HandleMessage - BEGIN (0x%x)\n", code);
#endif

	status_t status = BMediaNode::HandleMessage(code, data, size);
	if (status == B_OK)
		goto handled;
#ifdef DEBUG_DO_MESSAGE
		printf("Status from BMediaNode::HandleMessage: 0x%x\n", status);
#endif
	status = BTimeSource::HandleMessage(code, data, size);
	if (status == B_OK)
		goto handled;
	status = BBufferConsumer::HandleMessage(code, data, size);
	if (status == B_OK)
		goto handled;
	status = BControllable::HandleMessage(code, data, size);
	if (status == B_OK)
		goto handled;

	HandleBadMessage(code, data, size);

handled:
#ifdef DEBUG_DO_MESSAGE
	printf("BLegoConsumer::HandleMessage - END\n");
#endif

	return status;
}


/*********************
	BBufferConsumer
**********************/

status_t
BLegoConsumer::FormatChanged(const media_source & /*producer*/,const media_destination & /*consumer*/, 
	int32 /*from_change_count*/, const media_format & /*format*/)
{
	return B_ERROR;
}

status_t
BLegoConsumer::AcceptFormat(const media_destination &/*dest*/, media_format * format)
{
	//printf("BLegoConsumer::FormatSuggestionRequested\n");
	media_format suggestion = fPlayFormat;
	bool bad = false;

	if (format->type != B_MEDIA_NO_TYPE)
	{
		if (format->type != B_MEDIA_RAW_AUDIO) {
			*format = suggestion;
			return B_MEDIA_BAD_FORMAT;
		}
			
		if (format->u.raw_audio.frame_rate != fPlayFormat.u.raw_audio.frame_rate &&
				format->u.raw_audio.frame_rate != media_raw_audio_format::wildcard.frame_rate)
			bad = true;

		if (format->u.raw_audio.channel_count != fPlayFormat.u.raw_audio.channel_count &&
				format->u.raw_audio.channel_count != media_raw_audio_format::wildcard.channel_count)
			bad = true;

		if (format->u.raw_audio.format == media_raw_audio_format::wildcard.format)
			suggestion.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
		else if (format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT
					&& format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_FLOAT)	
			bad = true;
		else
		  suggestion.u.raw_audio.format = format->u.raw_audio.format;

		if (format->u.raw_audio.byte_order != fPlayFormat.u.raw_audio.byte_order &&
				format->u.raw_audio.byte_order != media_raw_audio_format::wildcard.byte_order)
			bad = true;

		suggestion.u.raw_audio.buffer_size = CHUNK_SIZE(suggestion);

		if (format->u.raw_audio.buffer_size != suggestion.u.raw_audio.buffer_size &&
			format->u.raw_audio.buffer_size != media_raw_audio_format::wildcard.buffer_size) {
			printf("BLegoConsumer::AcceptFormat(): rejecting buffer size %d\n",
					format->u.raw_audio.buffer_size);
			bad = true;
		}
	}
	
	*format = suggestion;
	return bad ? B_MEDIA_BAD_FORMAT : B_OK;
}

status_t
BLegoConsumer::GetNextInput(int32* cookie, media_input* out_input)
{
	printf("BLegoConsumer::GetNextInput(%d) - BEGIN: fd = %d\n", *cookie, fd);
	if (fd < 0 || *cookie)
		return B_MEDIA_BAD_DESTINATION;

	BAutolock lock(&mLock);
	out_input->node = Node();
	out_input->destination = fPlayDestination;
	out_input->source = fPlaySource;	
	out_input->format = fPlayFormat;
	out_input->format.u.raw_audio.buffer_size = CHUNK_SIZE(fPlayFormat);
//	sprintf(out_input->name, "BLegoConsumer Input %d", *cookie);
	strcpy(out_input->name, fName);
	*cookie = 1;

	return B_OK;
}

void
BLegoConsumer::DisposeInputCookie(int32 /*cookie*/)
{
	// Don't do anything special since we didn't allocate
	// anything on the cookie's behalf.
}

void
BLegoConsumer::ProducerDataStatus(const media_destination &/*for_whom*/, int32 status, bigtime_t at_media_time)
{
	at_media_time = at_media_time;

	if (status == B_DATA_AVAILABLE)
		fSendZero = false;
	if (status == B_DATA_NOT_AVAILABLE)
		fSendZero = true;
}

status_t
BLegoConsumer::GetLatencyFor(const media_destination &,
	bigtime_t * out_latency, media_node_id * out_timesource)
{
	*out_timesource = TimeSource()->ID();
	*out_latency = fMyLatency;
	return B_OK;
}

void
BLegoConsumer::BufferReceived(BBuffer* buffer)
{
	BUFFERS("BLegoConsumer::BufferReceived(%x) - BEGIN\n", buffer);

#if DUMP_FILE
	if (dump_file > 0 && dump_cnt < 1000) {
		printf("%x:%x\n", buffer->Data(), buffer->Header()->size_used);
		write(dump_file, buffer->Data(), buffer->Header()->size_used);
		dump_cnt++;
	}
	else if (dump_cnt > 999) {
		close(dump_file);
		dump_file = -1;
		dump_cnt = 0;
	}
#endif

	media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
		buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
	mQueue.AddEvent(event);
	//	converted from PushBuffer
//	mQueue.PushEvent(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER, buffer, 0, 0);
	release_sem(fBufferReceivedSem);
}

status_t
BLegoConsumer::Connected(		/* be sure to call BBufferConsumer::Connected()! */
	const media_source &producer,	/* here's a good place to request buffer group usage */
	const media_destination &/*where*/,
	const media_format & with_format,
	media_input * out_input)
{
	printf("BLegoConsumer::Connected()\n");
//	BBufferConsumer::Connected(producer, where, with_format, out_input);

	BAutolock lock(&mLock);
	fPlaySource = producer;
	out_input->node = Node();
	out_input->source = fPlaySource;
	out_input->destination = fPlayDestination;
	out_input->format = with_format;
	fPlayFormat = with_format;

//	sprintf(out_input->name, "BLegoConsumer hookedup");
	strcpy(out_input->name, fName);
#if DUMP_FILE
	if (dump_file < 0) {
		dump_file = open("/tmp/dumped_data.raw", O_RDWR | O_CREAT | O_TRUNC, 0666);
	}
#endif
	return B_OK;
}


void
BLegoConsumer::Disconnected(const media_source &/*producer*/, const media_destination &/*where*/)
{
	printf("BLegoConsumer::Disconnected()\n");
	Stop(TimeSource()->Now(), true);

	BAutolock lock(&mLock);

	mQueue.FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
//	while (BBuffer* buf = mQueue.PopFirstBuffer(NULL))
//		buf->Recycle();
//	BBuffer * buf = NULL;
//	bigtime_t time;
//	int32 what;
//	int64 data;
//	uint32 cleanup;
//	while (mQueue.PopEvent(&time, &what, (void**)&buf, &cleanup, &data) == B_OK) {
//		if (!buf) {
//			break;
//		}
//		buf->Recycle();
//		buf = NULL;
//	} 

	fPlaySource = media_source::null;
}


/*********************
	BMediaNode
**********************/

void
BLegoConsumer::Start(bigtime_t performance_time)		/* be sure to call BMediaNode::Start()! */
{
	printf("BLegoConsumer::Start(%f)\n", performance_time/1000000.0);
	BMediaNode::Start(performance_time);
	BAutolock lock(&mLock);

	if (!mRunning || mStopping)
	{
		for (int i = 0; i < N_OUTPUT_BUFFERS; i++) {
		  if (!fDevSpec->fOutputHeaders[i]) {
			int32 size = (TOTAL_SIZE + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
			char aname[40];
			const char * np = fDevSpec->path;
			if (strlen(np) > 18) {
				np += strlen(np)-18;
			}
			//	don't use more than 3 hex digits worth of buffers...
			sprintf(aname, "be:_LegO;%.18s;%x", np, i);
			aname[31] = 0;
			fDevSpec->fOutputAreas[i] = find_area(aname);
			if (fDevSpec->fOutputAreas[i] >= 0) {
				area_info ai;
#if B_BEOS_VERSION > B_BEOS_VERSION_MAUI
#warning This part assumes kernel addresses are globally readable and live < 0x80000000
#warning (And if you can pick out this warning from a complete rebuild, you have great eyes)
#endif
				if (get_area_info(fDevSpec->fOutputAreas[i], &ai) ||
						((ulong)ai.address > 0x80000000) ||
						(ai.size < (size_t)size)) {
					goto make_new;
				}
				fDevSpec->fOutputHeaders[i] = reinterpret_cast<audio_buffer_header *>(ai.address);
			}
			else {
make_new:
				fDevSpec->fOutputAreas[i] = create_area (aname,
										   (void**) &fDevSpec->fOutputHeaders[i],
										   B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS,
										   B_READ_AREA | B_WRITE_AREA);
			}
			if (fDevSpec->fOutputAreas[i] < 0)
			  fDevSpec->fOutputHeaders[i] = NULL;
		  }
		  if (!fDevSpec->fOutputHeaders[i])
			fprintf(stderr, "BLegoConsumer::Start - create_area(%ld) failed\n", TOTAL_SIZE);
		  fDevSpec->fOutputHeaders[i]->reserved_1 = TOTAL_SIZE;
		}

		mStartTime = performance_time;
		mStarting = true;

		// Start run thread
		fRunThreadKeepGoing = true;
		fRunThreadID = spawn_thread(RunThread, "Lego Consumer Run Thread", B_REAL_TIME_PRIORITY, this);
		resume_thread(fRunThreadID);
		fSchedLatency = estimate_max_scheduling_latency(fRunThreadID);

		fMyLatency = TOTAL_LATENCY;
		fSendZero = false;
		fNextBufferTime = 0;
	}
}

int32 BLegoConsumer::RunThread(void *_castToConsumer)
{
	BLegoConsumer *consumer = (BLegoConsumer*) _castToConsumer;
	while (consumer->fRunThreadKeepGoing && consumer->DoRun() == B_OK)
		;
		
	return 0;
}

int32 BLegoConsumer::ServiceThread(void *_castToConsumer)
{
	BLegoConsumer *consumer = (BLegoConsumer*) _castToConsumer;

	while (consumer->fServiceThreadKeepGoing) {
		int32 code = 0;
		char message_buffer[B_MEDIA_MESSAGE_SIZE];
		status_t err = read_port(consumer->ControlPort(), &code, message_buffer,
			sizeof(message_buffer));
		if (err == B_INTERRUPTED)
			continue;
	
		if (err < B_OK)
			break;
		
		consumer->HandleMessage(code, message_buffer, err);
	}
	
	return 0;
}


void
BLegoConsumer::Stop(bigtime_t performance_time, bool immediate)
{
	//printf("BLegoConsumer::Stop()\n");
	BMediaNode::Stop(performance_time, immediate);
	mLock.Lock();

	if (!mStopping && (mRunning || mStarting))
	{
		if(immediate) {
			mStopTime = TimeSource()->Now();
		}
		else {
			mStopTime = performance_time;
		}

		if (fRunThreadID != B_BAD_THREAD_ID) {
			fRunThreadKeepGoing = false;
			release_sem(fBufferReceivedSem);

			mLock.Unlock();

			int32 st;
			wait_for_thread(fRunThreadID, &st);

			mLock.Lock();

			fRunThreadID = B_BAD_THREAD_ID;
		}

		fOutputHeader = NULL;
		mStopping = true;
	}
	mLock.Unlock();
}


void
BLegoConsumer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
  printf("BLegoConsumer::Seek(%f, %f)\n", media_time/1000000.0,performance_time/1000000.0);
  BMediaNode::Seek(media_time, performance_time);
  BAutolock lock(&mLock);

  if (mRunning) {
	mSeekTo = media_time;
	mSeekAt = performance_time;
	mSeeking = true;
  }
  else {
	PUBLISH_TIME(media_time, performance_time, 0);
	BroadcastTimeWarp(performance_time, media_time);
  }
}


void
BLegoConsumer::TimeWarp(bigtime_t real_time, bigtime_t performance_time)
{
	BMediaNode::TimeWarp(real_time, performance_time);
	BAutolock lock(&mLock);

	mQueue.FlushEvents(performance_time, BTimedEventQueue::B_BEFORE_TIME, false,
		BTimedEventQueue::B_HANDLE_BUFFER);

	/*** Hmm... ***/
//	mQueue.FlushBefore(performance_time, 0);
	//	flush queue
//	BBuffer * buf = NULL;
//	bigtime_t time;
//	int32 what;
//	int64 data;
//	uint32 cleanup;
//	while (mQueue.PopEvent(&time, &what, (void**)&buf, &cleanup, &data) == B_OK) {
//		if (!buf) {
//			break;
//		}
//		if (time >= performance_time) {
//			//	put it back in
//			mQueue.PushEvent(time, what, buf, cleanup, data);
//			break;
//		}
//		buf->Recycle();
//		buf = NULL;
//	}
}

status_t 
BLegoConsumer::TimeSourceOp(const time_source_op_info &op, void */*_reserved*/)
{
	switch(op.op)
	{
		case BTimeSource::B_TIMESOURCE_START:
			Start(op.real_time);
			break;
			
		case BTimeSource::B_TIMESOURCE_STOP:
			Stop(op.real_time, false);
			break;
			
		case BTimeSource::B_TIMESOURCE_STOP_IMMEDIATELY:
			Stop(op.real_time, true);
			break;
			
		case BTimeSource::B_TIMESOURCE_SEEK:
			Seek(op.performance_time, op.real_time);
			break;
		default:
			return B_ERROR;
	}
	return B_OK;
}

status_t
BLegoConsumer::DoRun()
{
	int32 frame_size = 2 * (fPlayFormat.u.raw_audio.format & 0xf);

	BTimeSource* ts = TimeSource();
	bigtime_t now;
	bigtime_t then;
	bigtime_t ptime;
	int32 isize;
	int32 total_size;
	BBuffer* ibuf;
	char* src=""; // initializing to prevent compiler warning
	bigtime_t timeout;
	bool has_bufs;
	status_t err = B_OK;

	{
		BAutolock lock(&mLock);

		// figure out when next buffer is due to play
		has_bufs = mQueue.HasEvents();	//	converted from TBQ
//		ptime = (has_bufs ? mQueue.NextEventTime() : fNextBufferTime);
		ptime = (has_bufs ? mQueue.FirstEventTime() : fNextBufferTime);
		now = ts->RealTime();
		then = ts->RealTimeFor(ptime, fMyLatency);

		timeout = then - now;
//printf("ptime = %Ld\n", ptime);
//printf("fMyLatency = %Ld\n", fMyLatency);
//printf("now = %Ld\n", now);
//printf("then = %Ld\n", then);
if (mStarting) printf("mStartTime = %Ld\n", mStartTime);
if (mSeeking) printf("mSeekAt = %Ld\n", mSeekAt);
if (mStopping) printf("mStopTime = %Ld\n", mStopTime);

		bool alive = ts->IsRunning();
		if (!alive || !mRunning || (!has_bufs && !fSendZero))
			timeout = 1000000000000000LL; /* long time (31 years) */
	
		if (mRunning && mStopping && (!mSeeking || mSeekAt >= mStopTime)) {
			if (timeout > mStopTime - now)
				timeout = mStopTime - now;
			if (timeout < fSchedLatency) {
				mRunning = false;
				mStopping = false;
				if (mSeeking) {
				  PUBLISH_TIME(mSeekTo, mSeekAt, 0);
				  BroadcastTimeWarp(mSeekAt, mSeekTo);
				}
				else
				  PUBLISH_TIME(PerformanceTimeFor(mStopTime), mStopTime, 0);
				mSeeking = false;
			}
		}
		else if (mRunning && mSeeking) {
			if (timeout > mSeekAt - now)
				timeout = mSeekAt - now;
			if (timeout < fSchedLatency) {
				PUBLISH_TIME(mSeekTo, mSeekAt, fDrift);
				BroadcastTimeWarp(mSeekAt, mSeekTo);
				fLastPublishedReal = 0;
				fPerfSync = NO_SYNC;
				mSeeking = false;
			}
		}
		else if (!mRunning && mStarting) {
			if (timeout > mStartTime - now)
				timeout = mStartTime - now;
			if (timeout < fSchedLatency) {
				bigtime_t pt = PerformanceTimeFor(mStartTime);
				//printf("** %Ld  %Ld **\n", pt, mStartTime);				
				PUBLISH_TIME(pt, mStartTime, STARTING_DRIFT);
				BroadcastTimeWarp(mStartTime, pt);
				fLastPublishedReal = 0;
				fPerfSync = NO_SYNC;
				mStarting = false;
				mRunning = true;
			}
		}
		//printf("< %Ld  %Ld >\n", now%1000000, timeout);
	}

	//printf("timeout = %Ld\n", timeout);

	if (timeout == 1000000000000000LL) {
	  err = acquire_sem_etc(fBufferReceivedSem, 1, B_TIMEOUT, 1000000);
	  if (err == B_OK)
		timeout = 0;
	  else if ((err != B_TIMED_OUT) && (err != B_WOULD_BLOCK))
		return err;
	}

	if (timeout > 0)
	{
		if (mExtraTiming && timeout > 5000000LL) {
			timeout = 5000000LL;
		}
		// Block on buffer received semaphore, or until timeout.
		// if it was a timeout, then just loop again.
		//printf("BLegoConsumer::PlayRun - acquiring buffer received with Timeout: %Lx\n", timeout);

		err = acquire_sem_etc(fBufferReceivedSem, 1, B_TIMEOUT, timeout);
		if (err != B_OK)
			if ((err == B_TIMED_OUT) || (err == B_WOULD_BLOCK)) {
				//printf("BLegoConsumer::PlayRun - timed out: %Ldus\n", timeout);
			}
			else {
				//printf("BLegoConsumer::PlayRun - acquired buffer received: 0x%x\n", err);
				return err;
			}
	}
	if (fd < 0) {
		snooze(20000);
	  return B_OK;
	}

	if(!fRunThreadKeepGoing) {
		return B_OK;
	}

	has_bufs = mQueue.HasEvents();	//	from TBQ
	if (has_bufs)
//		fNextBufferTime = mQueue.NextEventTime();	//	from TBQ
		fNextBufferTime = mQueue.FirstEventTime();	//	from TEQ

//printf("%d %Ld\n", has_bufs, fNextBufferTime/1000%1000);
	bool sending_zero = fSendZero && !has_bufs;
	if (!has_bufs && !sending_zero) {
		if (mExtraTiming && (fPerfSync == SYNCED) && (err == B_TIMED_OUT)) {
			sv_handshake data;
			if (ioctl(fd, SV_SECRET_HANDSHAKE, &data, sizeof(data)) >= 0) {
//				printf("Secret Handshake done\n");
				bigtime_t clock = data.wr_total * 250000LL / fSampleRate;
//				fPerfSyncDelta = fPerfSyncStart - clock;
				ptime = clock + fPerfSyncDelta;
				bigtime_t rtime = data.wr_time;
				if (rtime && fLastPublishedReal) {
					if (rtime != fLastPublishedReal) {
						float drift = ((float) (ptime - fLastPublishedPerf)
							/ (rtime - fLastPublishedReal));
						if (drift > 0.5 && drift < 2.0) {
							fDrift = fDrift * 0.98 + drift * 0.02;
						}
					}
				}
				//printf("< %f  %f >\n", fPerfSyncDelta / 1000000.0, clock/1000000.0);
				fLastPublishedPerf = ptime;
				fLastPublishedReal = rtime;
				//bigtime_t t1 = system_time();
				//printf("< %f  %f  %Ld >\n", t1 / 1000000.0, rtime / 1000000.0, rtime - t1);
				//		BAutolock lock(&mLock);
				if (system_time() - rtime < 1000000) {
					PUBLISH_TIME(ptime, rtime, fDrift);
				}
			}
		}
		else {
			snooze(3000);
			BUFFERS("BLegoConsumer::PlayRun - Timed out, but no buffer to play\n");
		}
		return B_OK;
	}

	now = ts->RealTime();
	then = ts->RealTimeFor(fNextBufferTime, 0);
	if (then - 2 * fMyLatency > now) {
		printf("BLegoConsumer::PlayRun - buffer is %Ldus early\n", then - now);
		printf("^then = %Ld\n", then);
		printf("^now = %Ld\n", now);
		return B_OK;
	}

	if (sending_zero) {
		ibuf = NULL;
		isize = fPlayFormat.u.raw_audio.buffer_size;
	}
	else {
		media_timed_event event;
		mQueue.RemoveFirstEvent(&event);
		ibuf = (BBuffer *)event.pointer;
//		ibuf = mQueue.PopFirstBuffer(NULL);
//		bigtime_t time;
//		uint32 cleanup;
//		int32 what;
//		int64 data;
//		ibuf = NULL;
//		(void)mQueue.PopEvent(&time, &what, (void **)&ibuf, &cleanup, &data);
		src = (char*) ibuf->Data();
		isize = ibuf->Header()->size_used;
	}
	total_size = sizeof(audio_buffer_header) + 4 * isize / frame_size;

	//printf("BLegoConsumer::PlayRun - buffer size = %d\n", isize);

//! rounding errors etc
	if (!sending_zero) {
		if (isize > (int32)CHUNK_SIZE(fPlayFormat)) {
		  printf("BLegoConsumer::PlayRun - input buffer too large %d\n", isize);
		  fNextBufferTime += isize / frame_size * 1000000LL / fSampleRate;
		  ibuf->Recycle();
		  return B_OK;
		}
		if (then < now) {
		  printf("<%Ldus late>\n", now - then);
		  fNextBufferTime += isize / frame_size * 1000000LL / fSampleRate;
		  ibuf->Recycle();
		  return B_OK;
		}
	}
	
	//printf("BLegoConsumer::PlayRun - write and recycle (%Lx)\n", mLast);

	fOutputHeader = fDevSpec->fOutputHeaders[fNextBuffer];
	fOutputBuffer = (char*)(fOutputHeader + 1);
	assert((char *)&fOutputHeader[1] == (char *)fOutputBuffer);
	fOutputHeader->reserved_1 = total_size;

	if (sending_zero) {
	  if (!fOutputZero[fNextBuffer])
		memset(fOutputBuffer, 0, fOutputBufSize);
	}
	else if (fPlayFormat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_FLOAT) {
	  int32 n = isize / sizeof(float);
	  convertBufferFloatToShort((int16*) fOutputBuffer, (float*) src, n, 32767.0);
	  if (TEST_TONE) {
		int16* buf = (int16*) fOutputBuffer;
		for (int i = 0; i < n / 2; i++) {
		  int32 x = (i + 16) & 127;
		  x = (x < 64 ? x - 32 : 96 - x);
		  x = x * 128;
		  buf[2 * i] = buf[2 * i + 1] = x;
		}
	  }
	}
	else
	  memcpy(fOutputBuffer, src, isize);

	fOutputZero[fNextBuffer] = sending_zero;

	if (fPerfSync == NO_SYNC) {
	  fPerfSync = SYNCING;
	  fPerfSyncBufferNumber = fNextBuffer;
	  fPerfSyncStart = fNextBufferTime;
	  bigtime_t wait = then - ts->RealTime();
	  if (wait > 0 && wait <= fMyLatency)
		snooze (wait);
	}
	fNextBuffer = (fNextBuffer + 1) % N_OUTPUT_BUFFERS;
	fNextBufferTime += isize / frame_size * 1000000 / fSampleRate;

//printf(">> %Ld %Ld\n", system_time()%1000000, ptime%1000000);
//bigtime_t t5 = system_time();
	err = IOCTL(fd, SOUND_UNSAFE_WRITE, fOutputHeader, fOutputHeader->reserved_1);
	if (err == B_OK)
	  fWritePending = true;
	else if (err == B_RESOURCE_UNAVAILABLE)
	  err = B_OK;
	else
	  printf("BLegoConsumer::PlayRun - Result of UNSAFE WRITE %x\n", errno);
//printf("<%d>\n",(int)(system_time()-t5));
	if (ibuf)
		ibuf->Recycle();
//printf("< %d %d >\n", (int)((then - now)/1000),(int)((then - ts->RealTime())/1000));

	// wait for completion
	if (fWritePending) {
	  fWritePending = false;
	  err = acquire_sem_etc(fWriteCompletion, 1, B_TIMEOUT, DEFAULT_TIMEOUT);
	  if (err == B_OK) {
		int32 bufNumber = (fNextBuffer + N_OUTPUT_BUFFERS - 2) % N_OUTPUT_BUFFERS;
		bigtime_t clock = fDevSpec->fOutputHeaders[bufNumber]->sample_clock;
		if (fPerfSync == SYNCING)
		  if (fPerfSyncBufferNumber != bufNumber) {
			return B_OK;
		  }
		  else {
			fPerfSyncDelta = fPerfSyncStart - clock;
			fPerfSync = SYNCED;
		  }
		ptime = clock + fPerfSyncDelta;
		bigtime_t rtime = fDevSpec->fOutputHeaders[bufNumber]->time;
		if (rtime && fLastPublishedReal)
		  if (rtime != fLastPublishedReal) {
			float drift = ((float) (ptime - fLastPublishedPerf)
						   / (rtime - fLastPublishedReal));
			if (drift > 0.5 && drift < 2.0)
			  fDrift = fDrift * 0.98 + drift * 0.02;
			//fDrift = 1.0;
		  }
//printf("< %f  %f >\n", fPerfSyncDelta / 1000000.0, clock/1000000.0);
		fLastPublishedPerf = ptime;
		fLastPublishedReal = rtime;
//bigtime_t t1 = system_time();
//printf("< %d  %d  %d >\n", (int)(t1/1000),(int)(rtime/1000),(int)(rtime-t1));
//		BAutolock lock(&mLock);
		if (system_time() - rtime < 1000000)
		  PUBLISH_TIME(ptime, rtime, fDrift);
	  }
	  else {
		//printf("BLegoConsumer::PlayRun - Result of WAITING (0x%x)\n", err);
	  }
	}

	return err;
}


/*********************
	BControllable
**********************/

#define QUIET 0.0
#define LOUD 11.0
#define STEP(N) ((LOUD - QUIET) / (N))
#define MCP MakeContinuousParameter
#define MDP(ID, NAME, TYPE) MakeDiscreteParameter(ID, B_MEDIA_RAW_AUDIO, NAME, TYPE);
#define MNP(NAME, TYPE) MakeNullParameter(null_id++, B_MEDIA_RAW_AUDIO, NAME, TYPE);

static float
GetP(uint8 x, int s)
{
  return QUIET + (x << (8 - s)) * LOUD / 255.0;
}

static uint8
SetP(float x, int s)
{
  return (uint8) ((x - QUIET) * 255.0 / LOUD) >> (8 - s);
}

void
BLegoConsumer::ConstructControlWeb()
{
  platform_type platform = B_AT_CLONE_PLATFORM;
  system_info si;
  if (get_system_info(&si) == B_NO_ERROR)
	platform = si.platform_type;

  BParameterWeb* web = new BParameterWeb;
  BParameterGroup* grp = web->MakeGroup("Output");

  if (fDevSpec->is_null_device) {
	grp->MakeNullParameter(1, B_MEDIA_RAW_AUDIO, "This device produces silence.", B_GENERIC);
	SetParameterWeb(web);
	return;
  }
  int null_id = BLC_NULL_PARAMS;
  /*
  BDiscreteParameter* rate = grp->MDP(BLC_SAMPLE_RATE, "Sample Rate",
									  B_FRAME_RATE);
  rate->AddItem(kHz_8_0, "8kHz");
  rate->AddItem(kHz_11_025, "11kHz");
  rate->AddItem(kHz_16_0, "16kHz");
  rate->AddItem(kHz_22_05, "22kHz");
  rate->AddItem(kHz_32_0, "32kHz");
  rate->AddItem(kHz_44_1, "44kHz");
  rate->AddItem(kHz_48_0, "48kHz");
  */
  BParameterGroup * out_grp=NULL; // only for B_BEBOX_PLATFORM
  if (platform == B_BEBOX_PLATFORM)
	out_grp = grp->MakeGroup("Line Out");

  if (platform != B_MAC_PLATFORM) {
	BParameterGroup * cd_grp = grp->MakeGroup("CD");
	BParameterGroup * line_grp = grp->MakeGroup("Line");
	BParameterGroup * dac_grp = grp->MakeGroup("Mixer");

	BNullParameter* cd_in = cd_grp->MNP("CD", B_WEB_PHYSICAL_INPUT);
	BNullParameter* line_in = line_grp->MNP("Line", B_WEB_PHYSICAL_INPUT);
	BNullParameter* dac = dac_grp->MNP("Mixer", B_WEB_DAC_CONVERTER);
	//	BNullParameter* mix_point = out_grp->MNP("Outputs", B_GENERIC);

	BDiscreteParameter* cd_mute = cd_grp->MDP(BLC_CD_MUTE, "Mute", B_MUTE);
	cd_mute->AddItem(0, "Thru");
	cd_mute->AddItem(1, "Mute");
	BContinuousParameter* cd_out = cd_grp->MCP(BLC_CD_LEVEL, B_MEDIA_RAW_AUDIO, "Volume",
											B_GAIN, "", QUIET, LOUD, STEP(32));
	cd_out->SetChannelCount(2);
	cd_mute->AddInput(cd_in);
	cd_out->AddInput(cd_mute);
	//	mix_point->AddInput(cd_out);

	BDiscreteParameter* line_mute = line_grp->MDP(BLC_LINE_IN_MUTE, "Mute", B_MUTE);
	line_mute->AddItem(0, "Thru");
	line_mute->AddItem(1, "Mute");
	BContinuousParameter* line_in_out = line_grp->MCP(BLC_LINE_IN_LEVEL, B_MEDIA_RAW_AUDIO,
												 "Volume", B_GAIN, "",
												 QUIET, LOUD, STEP(32));
	line_in_out->SetChannelCount(2);
	line_mute->AddInput(line_in);
	line_in_out->AddInput(line_mute);
	//	mix_point->AddInput(line_in_out);

	BDiscreteParameter* dac_mute = dac_grp->MDP(BLC_DAC_MUTE, "Mute", B_MUTE);
	dac_mute->AddItem(0, "Thru");
	dac_mute->AddItem(1, "Mute");
	BContinuousParameter* dac_out = dac_grp->MCP(BLC_DAC_LEVEL, B_MEDIA_RAW_AUDIO, "Volume",
											 B_GAIN, "", QUIET, LOUD, STEP(64));
	dac_out->SetChannelCount(2);
	dac_mute->AddInput(dac);
	dac_out->AddInput(dac_mute);
	//	mix_point->AddInput(dac_out);

	if (platform == B_AT_CLONE_PLATFORM) {
		BParameterGroup * mic_grp = grp->MakeGroup("Mic");
	  BNullParameter* mic_in = mic_grp->MNP("Mic", B_WEB_PHYSICAL_INPUT);
	  BDiscreteParameter* mic_mute = mic_grp->MDP(BLC_LOOP_MUTE, "Mute", B_MUTE);
	  mic_mute->AddItem(0, "Thru");
	  mic_mute->AddItem(1, "Mute");
	  BContinuousParameter* mic_out = mic_grp->MCP(BLC_LOOP_LEVEL, B_MEDIA_RAW_AUDIO,
											   "Volume", B_GAIN, "",
											   QUIET, LOUD, STEP(64));
	  mic_mute->AddInput(mic_in);
	  mic_out->AddInput(mic_mute);
	  //	  mix_point->AddInput(mic_out);
	}
	if (platform == B_BEBOX_PLATFORM) {
	  BNullParameter* spkr = out_grp->MNP("Speaker", B_WEB_PHYSICAL_OUTPUT);
	  BDiscreteParameter * spkr_mute = out_grp->MDP(BLC_SPEAKER_MUTE, "Mute", B_MUTE);
	  spkr_mute->AddItem(0, "Thru");
	  spkr_mute->AddItem(1, "Mute");
	  BContinuousParameter* spkr_out = out_grp->MCP(BLC_SPEAKER_LEVEL, B_MEDIA_RAW_AUDIO,
												"Volume", B_GAIN, "",
												QUIET, LOUD, STEP(64));
	  spkr_out->AddInput(spkr_mute);
	  spkr->AddInput(spkr_out);
	  //	  spkr_mute->AddInput(mix_point);
	}

	//	BNullParameter* out = out_grp->MNP("Line Out", B_WEB_PHYSICAL_OUTPUT);
	//	out->AddInput(mix_point);
  }
	else
	{	// platform == B_MAC_PLATFORM
		//	must fix this up for new web...
	BDiscreteParameter* monitor = grp->MDP(BLC_LOOP_MUTE, "Input Monitor Mute",
										   B_MUTE);
	monitor->AddItem(0, "Thru");
	monitor->AddItem(1, "Mute");

	BParameterGroup * dac_grp = grp->MakeGroup("Line Out");
	BDiscreteParameter* dac_mute = dac_grp->MDP(BLC_DAC_MUTE, "Mute", B_MUTE);
	dac_mute->AddItem(0, "Thru");
	dac_mute->AddItem(1, "Mute");
	BContinuousParameter* dac_out = dac_grp->MCP(BLC_DAC_LEVEL, B_MEDIA_RAW_AUDIO,
											 "Volume", B_GAIN, "",
											 QUIET, LOUD, STEP(64));
	dac_out->SetChannelCount(2);
	dac_out->AddInput(dac_mute);

	BParameterGroup * spkr_grp = grp->MakeGroup("Speaker");
	BDiscreteParameter* spkr_mute = spkr_grp->MDP(BLC_SPEAKER_MUTE, "Mute", B_MUTE);
	spkr_mute->AddItem(0, "Thru");
	spkr_mute->AddItem(1, "Mute");
	BContinuousParameter* spkr_out = spkr_grp->MCP(BLC_SPEAKER_LEVEL, B_MEDIA_RAW_AUDIO,
											  "Volume", B_GAIN, "",
											  QUIET, LOUD, STEP(64));
	spkr_out->AddInput(spkr_mute);
  }

  SetParameterWeb(web);
}

status_t
BLegoConsumer::GetParameterValue(int32 id, bigtime_t* last_change,
								 void* value, size_t* ioSize)
{
  sem_id lock = fDevSpec->lock;
  sound_setup* setup = &fDevSpec->setup;
  float* float_val = (float*) value;
  int32* int_val = (int32*) value;
  size_t size = *ioSize;
  //printf("BLegoConsumer::GetParameterValue - %d  Size: %d\n", id, size);

  if (fDevSpec->is_null_device) return B_BAD_INDEX;

  if (size < 4)
	return B_BAD_VALUE;
  status_t err = acquire_sem(lock);
  if (err != B_OK)
	return err;
  if (IOCTL(fd, SOUND_GET_PARAMS, &fDevSpec->setup, sizeof(sound_setup)) < 0) {
	release_sem(lock);
	return B_ERROR;
  }

  *last_change = fDataChangeTime;
  *ioSize = 4;
	
  switch(id) {
  case BLC_CD_LEVEL:
	if (size < 2 * sizeof(float))
	  err = B_BAD_VALUE;
	else {
	  float_val[0] = GetP(31 - setup->left.aux1_mix_gain, 5);
	  float_val[1] = GetP(31 - setup->right.aux1_mix_gain, 5);
	  *ioSize = 8;
	}
	break;
  case BLC_CD_MUTE:
	*int_val = !!setup->left.aux1_mix_mute;
	break;
  case BLC_LINE_IN_LEVEL:
	if (size < 2 * sizeof(float))
	  err = B_BAD_VALUE;
	else {
	  float_val[0] = GetP(31 - setup->left.line_mix_gain, 5);
	  float_val[1] = GetP(31 - setup->right.line_mix_gain, 5);
	  *ioSize = 8;
	}
	break;
  case BLC_LINE_IN_MUTE:
	*int_val = !!setup->left.line_mix_mute;
	break;
  case BLC_DAC_LEVEL:
	if (size < 2 * sizeof(float))
	  err = B_BAD_VALUE;
	else {
	  float_val[0] = GetP(63 - setup->left.dac_attn, 6);
	  float_val[1] = GetP(63 - setup->right.dac_attn, 6);
	  *ioSize = 8;
	}
	break;
  case BLC_DAC_MUTE:
	*int_val = !!setup->left.dac_mute;
	break;
  case BLC_LOOP_LEVEL:
	float_val[0] = GetP(63 - setup->loop_attn, 6);
	break;
  case BLC_LOOP_MUTE:
	*int_val = !setup->loop_enable;
	break;
  case BLC_SPEAKER_LEVEL:
	float_val[0] = GetP(setup->mono_gain, 6);
	break;
  case BLC_SPEAKER_MUTE:
	*int_val = !!setup->mono_mute;
	break;
  case BLC_SAMPLE_RATE:
	*int_val = setup->sample_rate;
	break;
  default:
	err = B_BAD_VALUE;
  }

  release_sem(lock);
  return err;
}

void 
BLegoConsumer::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
  sem_id lock = fDevSpec->lock;
  sound_setup* setup = &fDevSpec->setup;
  const float* float_val = (float*) value;
  const int32* int_val = (int32*) value;

  if (fDevSpec->is_null_device) return;


  if (size < 4)
	return;
  if (acquire_sem(lock) < 0)
	return;
  if (fDataChangeTime < when)
	fDataChangeTime = when;
	
  switch(id) {
  case BLC_CD_LEVEL:
	if (size >= 2 * sizeof(float)) {
	  setup->left.aux1_mix_gain = 31 - SetP(float_val[0], 5);
	  setup->right.aux1_mix_gain = 31 - SetP(float_val[1], 5);
	}
	break;
  case BLC_CD_MUTE:
	setup->left.aux1_mix_mute = setup->right.aux1_mix_mute = !!*int_val;
	break;
  case BLC_LINE_IN_LEVEL:
	if (size >= 2 * sizeof(float)) {
	  setup->left.line_mix_gain = 31 - SetP(float_val[0], 5);
	  setup->right.line_mix_gain = 31 - SetP(float_val[1], 5);
	}
	break;
  case BLC_LINE_IN_MUTE:
	setup->left.line_mix_mute = setup->right.line_mix_mute = !!*int_val;
	break;
  case BLC_DAC_LEVEL:
	if (size >= 2 * sizeof(float)) {
	  setup->left.dac_attn = 63 - SetP(float_val[0], 6);
	  setup->right.dac_attn = 63 - SetP(float_val[1], 6);
	  //printf("BLegoConsumer: DAC_LEVEL = %d\n", setup->left.dac_attn);
	}
	break;
  case BLC_DAC_MUTE:
	setup->left.dac_mute = setup->right.dac_mute = !!*int_val;
	break;
  case BLC_LOOP_LEVEL:
	setup->loop_attn = 63 - SetP(float_val[0], 6);
	break;
  case BLC_LOOP_MUTE:
	setup->loop_enable = !*int_val;
	break;
  case BLC_SPEAKER_LEVEL:
	setup->mono_gain = SetP(float_val[0], 6);
	break;
  case BLC_SPEAKER_MUTE:
	setup->mono_mute = !!*int_val;
	break;
  case BLC_SAMPLE_RATE:
	setup->sample_rate = (sample_rate) *int_val;
	fPlayFormat.u.raw_audio.frame_rate = sr_enum_to_float(setup->sample_rate);
	fSampleRate = (int32) fPlayFormat.u.raw_audio.frame_rate;
	break;
  default:
	goto exuent;
  }

  IOCTL(fd, SOUND_SET_PARAMS, &fDevSpec->setup, sizeof(sound_setup));

exuent:
  release_sem(lock);
}

status_t 
BLegoConsumer::StartControlPanel(BMessenger * /*out_messenger*/)
{
	return B_ERROR;
}

void
BLegoConsumer::MakeName()
{
	const char * str = fDevSpec->path;
	const char * strip = "/dev/audio/old/";
	if (!strncmp(str, strip, strlen(strip))) str += strlen(strip);
	if (strlen(str) > 50) str += strlen(str)-50;
	sprintf(fName, "Audio Out %s", str);
}
