/* ++++++++++

   FILE:  EmuProducer.cpp
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

+++++ */

#include "EmuProducer.h"

#include <byteorder.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include <ParameterWeb.h>
#include <TimeSource.h>
#include <tr_debug.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <MediaAddOn.h>

#include "sainput.h"
#include "sout8210.h"

//#define TRACE printf
//#define DEBUG_DO_MESSAGE	1
//#define DEBUG_SERVICE_RUN	1
//#define DEBUG_ITERATE_FORMATS	1

#if !DEBUG
#define printf
#endif

#if 0
#define PUBLISH_TIME(PT, RT, D) { \
	fprintf(stderr, "PublishTime(%Ld, %Ld, %f)\n", PT, RT, (float)(D)); \
	PublishTime(PT, RT, D); \
	fprintf(stderr, "RealTimeFor(%Ld, 0) = %Ld\n", PT, RealTimeFor(PT, 0)); }
#else
#define PUBLISH_TIME(PT, RT, D) { \
	PublishTime(PT, RT, D); }
#endif

//#define X(N) {printf("<%d> ",(N));fflush(stdout);}
#define PERR(EXP) { \
	status_t err = (EXP); \
	if (err < 0) \
	fprintf (stderr, "BEmuProducer: " #EXP " == 0x%x (%s)\n", err, strerror(err)); \
}

#define IOCTL(FD,OP,AD,SZ) (ioctl(FD,OP,AD,SZ) < 0 ? errno : B_OK)

#define DEFAULT_TIMEOUT 6000000L
#define DEFAULT_N_BUFFERS 6
#define PRE_LATENCY 3000
#define JITTER_LATENCY 2000
#define MIN_BUF_SIZE 512
#define STARTING_DRIFT 1.0

static void
saCallback(stSAInputBuffer* buf)
{
  for (; buf; buf = buf->pNextBuffer)
	reinterpret_cast<BEmuProducer*>(buf->dwUser1)->DoBuffer(buf);
}

#define FX_WRITEINSTRUCTION(ID,usP,ulUCL,ulUCH) {\
	LSEPtrWrite(ID,0x04000000+((ULONG)(usP)<<17),ulUCL); \
	LSEPtrWrite(ID,0x04010000+((ULONG)(usP)<<17),ulUCH); \
}

static void
set_input_channel(dev_spec* dev)
{
  /* Map input channel to Multirate Src (0x2a, 0x2b) */
  int32 src = dev->settings.input_channel & 0x1f;
  FX_WRITEINSTRUCTION(dev->fHALID, 0, (0x40 << 10) | 0x40,
					  (0x2a << 10) | src);
  FX_WRITEINSTRUCTION(dev->fHALID, 1, (0x40 << 10) | 0x40,
					  (0x2b << 10) | (src + 1));
}  


BEmuProducer::BEmuProducer(BEmuAddOn *addon, dev_spec* spec, char* name,
						   int32 id, status_t* status)
  : BMediaNode(name),
	BMediaEventLooper(),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	mLock("emu producer lock", true),
	fAddOn(addon),
	fBufferGroup(NULL),
	fChangeTag(0),
	fInternalID(id),
	fPlaybackSample(0),
	fPlaybackSync(0),
	fCapturing(false),
	fStopping(false),
	fDevSpec(spec)
{
	SetPriority(B_REAL_TIME_PRIORITY);
	memset(fInputBuffers, 0, sizeof(fInputBuffers));
	memset(fBufSpec, 0, sizeof(fBufSpec));
	status_t err = BEmuProducer_Initialize();
	if (status)
	  *status = err;
}

BEmuProducer::~BEmuProducer()
{
  BMediaEventLooper::Quit();
  fClient->Reset(NULL);
  fDevSpec->close();
  delete fBufferGroup;
}

status_t
BEmuProducer::DeleteHook(BMediaNode*)
{
  delete this;
  return B_OK;
}

void
BEmuProducer::SetTimeSource(BTimeSource * time_source)
{
	BMediaNode::SetTimeSource(time_source);
}

status_t
BEmuProducer::SetupEmu()
{
  stSAInputConfig config;
  memset(&config, 0, sizeof(config));
  config.callback = saCallback;
  config.sampleFormat = saFormatSigned16PCM;
  config.sampleRate = saRate44_1K;
  config.byNumChans = 2;
  config.dwChanSelect = 3;

  fInputMgr = new SAInputMgr();
  fInputDevice = new SA8210WaveInputDevice(fDevSpec->fHRMID);
  fInputMgr->AddDevice(fInputDevice);
  if (fInputMgr->IsBad()) {
	printf("AddDevice() failed.\n");
	return B_ERROR;
  }

  fClient = fInputMgr->CreateDevClient(0, &config);
  if (fClient == NULL) {
	printf("CreateDevClient() failed.\n");
	return B_ERROR;
  }

  set_input_channel(fDevSpec);
  return B_NO_ERROR;
}

status_t
BEmuProducer::BEmuProducer_Initialize()
{
	// Let the system know we do physical input
	AddNodeKind(B_PHYSICAL_INPUT);

	status_t err = SetupEmu();
	if (err < 0)
	  return err;

	fCaptureFormat.type = B_MEDIA_RAW_AUDIO;
	fCaptureFormat.u.raw_audio.frame_rate = 44100.0;
	fCaptureFormat.u.raw_audio.channel_count = 2;
	fCaptureFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	fCaptureFormat.u.raw_audio.byte_order
	  = (B_HOST_IS_BENDIAN ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN);
	fCaptureFormat.u.raw_audio.buffer_size = fDevSpec->preferred_buffer_size;

	fUsecPerFrame = 1E6 / fCaptureFormat.u.raw_audio.frame_rate;
	fLastPublishedReal = 0;
	fDrift = STARTING_DRIFT;
	fNextSample = 0;

	fCaptureDestination = media_destination::null;
	fCaptureSource.port = ControlPort();
	fCaptureSource.id = 0;
	fDataChangeTime = 0;

	MakeName();

	return B_OK;
}

void
BEmuProducer::NodeRegistered() {
	ConstructControlWeb();
	BMediaEventLooper::NodeRegistered();
}

/******************
	BMediaNode
*******************/

BMediaAddOn*
BEmuProducer::AddOn(int32* internal_id) const
{	
	// Who instantiated you -- or NULL for app class
	*internal_id = fInternalID;
	return fAddOn;
}

status_t 
BEmuProducer::HandleMessage(int32 code, const void *data, size_t size)
{
  return B_ERROR;
}

/*********************
	BBufferProducer
**********************/

status_t
BEmuProducer::FormatSuggestionRequested(media_type type, int32 quality, media_format * format)
{
	if (type == B_MEDIA_NO_TYPE)
		type = B_MEDIA_RAW_AUDIO;
	if (type != B_MEDIA_RAW_AUDIO)
		return B_MEDIA_BAD_FORMAT;

	quality = quality;
	*format = fCaptureFormat;
	format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;

	return B_OK;
}

status_t
BEmuProducer::FormatChangeRequested(const media_source & source,
				const media_destination & destination,
				media_format * io_format, int32 * out_change_count)
{
	return B_ERROR;
}

status_t
BEmuProducer::FormatProposal(const media_source & output, media_format * format)
{
	media_format suggestion = fCaptureFormat;
	bool bad = false;

	if (format->type != B_MEDIA_NO_TYPE) {
		if (format->type != B_MEDIA_RAW_AUDIO) {
			*format = suggestion;
			return B_MEDIA_BAD_FORMAT;
		}
	
		if (format->u.raw_audio.frame_rate > media_raw_audio_format::wildcard.frame_rate && 
			format->u.raw_audio.frame_rate != fCaptureFormat.u.raw_audio.frame_rate)
			bad = true;
	
		if (format->u.raw_audio.channel_count > media_raw_audio_format::wildcard.channel_count && 
			format->u.raw_audio.channel_count != fCaptureFormat.u.raw_audio.channel_count)
			bad = true;
	
		if (format->u.raw_audio.format > media_raw_audio_format::wildcard.format &&
			format->u.raw_audio.format != fCaptureFormat.u.raw_audio.format)
			bad = true;
	
		if (format->u.raw_audio.byte_order > media_raw_audio_format::wildcard.byte_order && 
			format->u.raw_audio.byte_order != fCaptureFormat.u.raw_audio.byte_order)
			bad = true;
	
		if (format->u.raw_audio.buffer_size > media_raw_audio_format::wildcard.buffer_size)
		  if (format->u.raw_audio.buffer_size < MIN_BUF_SIZE) {
			suggestion.u.raw_audio.buffer_size = MIN_BUF_SIZE;
			bad = true;
		  }
		  else
			suggestion.u.raw_audio.buffer_size = format->u.raw_audio.buffer_size;
	}

	*format = suggestion;
	return bad ? B_MEDIA_BAD_FORMAT : B_OK;
}


status_t
BEmuProducer::GetNextOutput(int32 * cookie, media_output * out_destination)
{
	if (*cookie)
		return B_ERROR;
	
	BAutolock lock(mLock);
	out_destination->node = Node();
	out_destination->source = fCaptureSource;
	out_destination->destination = fCaptureDestination;
	out_destination->format = fCaptureFormat;
//	sprintf(out_destination->name, "BEmuProducer %d", *cookie);
	strcpy(out_destination->name, fName);
	*cookie = *cookie+1;
	
	return B_OK;
}


status_t
BEmuProducer::DisposeOutputCookie(int32 cookie)
{
	cookie = cookie; /* ignore cookie */
	
	return B_OK;
}


status_t
BEmuProducer::SetBufGroup(BBufferGroup* group)
{
  if (fBufferGroup == group)
	return B_OK;

  if (fBufferGroup) {
	stSAInputBuffer* buf = NULL;
	fClient->Reset(&buf);
	for (; buf; buf = buf->pNextBuffer) {
	  reinterpret_cast<BBuffer*>(buf->dwUser2)->Recycle();
	  fBufSpec[buf->dwUser3].pending = false;
	}
	fBufferGroup->ReclaimAllBuffers();
	delete fBufferGroup;
  }

  if (group)
	fBufferGroup = group;
  else {
	fBufferGroup = new BBufferGroup(fDevSpec->preferred_buffer_size,
									DEFAULT_N_BUFFERS, B_ANY_ADDRESS);
	status_t err = fBufferGroup->InitCheck();
	if (err != B_OK) {
	  PERR(err);
	  return err;
	}
  }
  return B_OK;
}

status_t
BEmuProducer::SetBufferGroup(const media_source& for_source,
							 BBufferGroup* group)
{
	BAutolock lock(mLock);

	if (for_source.port != ControlPort())
	  return B_MEDIA_BAD_SOURCE;
	if (for_source.id != 0)
	  return B_MEDIA_BAD_SOURCE;
	if (fCaptureDestination == media_destination::null)
	  return B_MEDIA_BAD_SOURCE;
	if (RunState() != B_STOPPED)
	  return B_ERROR;

	return SetBufGroup(group);
}

/*
	Method: SetVideoClipping
	
	We aren't a video node, so there isn't anything
	for us to do for this particular method.
*/

status_t
BEmuProducer::VideoClippingChanged(
	const media_source &for_source,
	int16 num_shorts,
	int16 * clip_data,
	const media_video_display_info & display,
	int32 * out_from_change_count)
{
	num_shorts = num_shorts; /* ignore num_shorts */
	clip_data = clip_data; /* ignore clip_data */
	out_from_change_count = out_from_change_count; /* ignore out_from_change_count */

	return B_ERROR;		/* this is not a video node... */
}


status_t
BEmuProducer::GetLatency(bigtime_t* out_latency)
{
  bigtime_t buf_latency = (bigtime_t) (fInputBufSize / 4 * fUsecPerFrame);
  bigtime_t downstream_latency = 0;
  status_t err = BBufferProducer::GetLatency(&downstream_latency);
  if (err != B_OK)
	return err;
  *out_latency = buf_latency + downstream_latency + SchedulingLatency();
  return B_OK;
}


status_t
BEmuProducer::PrepareToConnect(const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name)
{
	status_t err = FormatProposal(what, format);
	if (err != B_OK)
		return err;

	BAutolock lock(mLock);
	if (what.port != ControlPort() || what.id != 0)
		return B_MEDIA_BAD_SOURCE;

	ASSERT(fCaptureDestination == media_destination::null);
	if (where == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;

	fCaptureDestination = where;

	media_source source(ControlPort(), what.id);
	*out_source = source;
	strcpy(out_name, fName);

	return B_OK;
}


void
BEmuProducer::Connect(
		status_t error, 
		const media_source & source,
		const media_destination & destination,
		const media_format & format,
		char * out_name)
{
	printf("BEmuProducer::Connect()\n");
	BBufferProducer::Connect(error, source, destination, format, out_name);

	BAutolock lock(mLock);
	if (error != B_OK) {
		fCaptureDestination = media_destination::null;
		return;
	}

	fInputBufSize = format.u.raw_audio.buffer_size;
	fSampleRate = (int32) (format.u.raw_audio.frame_rate + 0.5);
	fUsecPerFrame = 1E6 / format.u.raw_audio.frame_rate;
	fCaptureDestination = destination;
	strcpy(out_name, fName);
	fDisableOutput = false;
	fCapturing = false;

	if (fBufferGroup == NULL) {
	  fBufferGroup = new BBufferGroup(format.u.raw_audio.buffer_size,
									  DEFAULT_N_BUFFERS, B_ANY_ADDRESS);
	  if (fBufferGroup->InitCheck() != B_OK)
		PERR(fBufferGroup->InitCheck());
	}
}


void
BEmuProducer::Disconnect(const media_source &what, const media_destination &where)
{
	//fprintf(stderr, "BEmuProducer::Disconnect()\n");
	ASSERT(!(what.port != ControlPort() || what.id != 0));
	if (what.port != ControlPort() || what.id != 0)
		return;

	ASSERT(fCaptureDestination == where);
	if (where == media_destination::null)
		return;

	BAutolock lock(mLock);
	
	fStopTime = 0;
	if (RunState() != BMediaEventLooper::B_STOPPED) {
	  fStopping = true;
	  SetRunState(BMediaEventLooper::B_STOPPED);
	}

	/* stop using the buffer group */
	SetBufGroup(NULL);

	/* break the connection */
	fCaptureDestination = media_destination::null;
}

void 
BEmuProducer::LateNoticeReceived(const media_source & what,
				bigtime_t how_much, bigtime_t performance_time)
{
}

void
BEmuProducer::EnableOutput(const media_source & what,
			bool enabled,
			int32 * change_tag)
{
	if (what.port != ControlPort() || what.id)
		return;
	BAutolock lock(mLock);
	fDisableOutput = !enabled;
	*change_tag = ++fChangeTag;
}

/*********************
	BTimeSource
**********************/

void
BEmuProducer::SetRunMode(run_mode mode)
{
  BMediaEventLooper::SetRunMode(mode);
  BTimeSource::SetRunMode(mode);
}

status_t 
BEmuProducer::TimeSourceOp(const time_source_op_info &op, void *_reserved)
{
	media_timed_event event;
	switch(op.op)
	{
		case B_TIMESOURCE_START:
			event.event_time = op.real_time;
			event.type = BTimedEventQueue::B_START;
			break;
		case B_TIMESOURCE_STOP:
			event.event_time = op.real_time;
			event.type = BTimedEventQueue::B_STOP;
			break;
		case B_TIMESOURCE_SEEK:
			event.event_time = op.real_time;
			event.bigdata = op.performance_time;
			event.type = BTimedEventQueue::B_SEEK;
			break;
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			event.event_time = system_time();
			event.type = BTimedEventQueue::B_STOP;
			DispatchEvent(&event, 0, true);
			return B_OK;
		default:
			return B_ERROR;
	}
	RealTimeQueue()->AddEvent(event);
	return B_OK;
}

/*********************
	BMediaEventLooper
**********************/

void
BEmuProducer::ControlLoop()
{
  while(true) {
	BTimedEventQueue* eq = EventQueue();
	BTimedEventQueue* rtq = RealTimeQueue();
	bigtime_t e_latency = EventLatency();
	bigtime_t s_latency = SchedulingLatency();
	bigtime_t duration = BufferDuration();

	bigtime_t waitUntil = B_INFINITE_TIMEOUT;
	bool useRealTimeEvent = true;
	bigtime_t rEventTime = B_INFINITE_TIMEOUT;
	const media_timed_event *rEvent = NULL;
	bigtime_t pEventTime = B_INFINITE_TIMEOUT;
	const media_timed_event *pEvent = NULL;
	bool bufferEvent = false;

	if (eq->HasEvents()) {
	  pEvent = eq->FirstEvent();

	  if (pEvent) {
		pEventTime = TimeSource()->RealTimeFor(pEvent->event_time,
											   e_latency + s_latency);
		if (pEvent->type == BTimedEventQueue::B_HANDLE_BUFFER)
		  bufferEvent = true;
	  }
	}	

	if (rtq->HasEvents()) {
	  rEvent = rtq->FirstEvent();
	  if (rEvent)
		rEventTime = rEvent->event_time - s_latency;
	}

	if (pEventTime < rEventTime) {
	  waitUntil = pEventTime;
	  if (bufferEvent && (duration > e_latency))
		waitUntil -= (duration - e_latency);
	  useRealTimeEvent = false;
	}
	else {
	  waitUntil = rEventTime;
	  useRealTimeEvent = true;
	}

	status_t err = WaitForMessage(waitUntil);

	if (err == B_TIMED_OUT || err == B_WOULD_BLOCK) {
	  const media_timed_event *nextEvent = NULL;
	  bigtime_t lateness = BTimeSource::RealTime();

	  if (useRealTimeEvent) {
		nextEvent = rEvent;
		if (nextEvent)
		  lateness -= rEventTime;
	  }
	  else {
		nextEvent = pEvent;
		if (nextEvent)
		  lateness -= pEventTime;
	  }

	  if (nextEvent) {
		DispatchEvent(nextEvent, lateness, useRealTimeEvent);
		if (useRealTimeEvent)
		  rtq->RemoveEvent(nextEvent);
		else
		  eq->RemoveEvent(nextEvent);
	  }
	}
	else if (err < B_OK) {
	  fprintf(stderr, "BEmuProducer: WaitForMessage error: %s (%d) ONLINE\n",
			  strerror(err), err);
	  SetRunState(B_IN_DISTRESS);
	  ReportError(B_NODE_IN_DISTRESS);
	  break;
	}
  }
}

void
BEmuProducer::HandleEvent(const media_timed_event* event,
						  bigtime_t lateness,
						  bool realTimeEvent)
{
  bigtime_t when = event->event_time;

  switch(event->type) {
  case BTimedEventQueue::B_START:
	if (RunState() == BMediaEventLooper::B_STOPPED) {
		printf("BEmuProducer::HandlEvent(): B_START\n");
		BAutolock lock(mLock);
		fStopping = false;
		fPlaybackSync = 0;
		fPlaybackSample = 0;
		for (int32 i = 0; i < N_INPUT_BUFFERS; i++) {
		  stSAInputBuffer* sabuf = &fInputBuffers[i];
		  buf_spec* spec = &fBufSpec[i];
		  if (!spec->pending) {
			BBuffer *ebuf = fBufferGroup->RequestBuffer(MIN_BUF_SIZE);
			if (ebuf == NULL)
			  break;

			sabuf->virtAddr = (void*) ebuf->Data();
			sabuf->dwSize = MIN(fInputBufSize, ebuf->Size());
			sabuf->dwUser1 = (DWORD) this;	// Not 64-bit safe
			sabuf->dwUser2 = (DWORD) ebuf;	// Not 64-bit safe
			sabuf->dwUser3 = i;
			if (!fCapturing)
			  fNextSample = 0;
			spec->sample = fNextSample;
			fNextSample += sabuf->dwSize / 4;
			spec->pending = true;
			//printf("calling AddBuffer()\n");
			fClient->AddBuffer(sabuf);
			if (!fCapturing) {
			  fCapturing = true;
			  printf("Calling Record()...");
			  fClient->Record();
			  printf("done\n");
			}
		  }
		}

		bigtime_t rt = realTimeEvent ? when : TimeSource()->RealTimeFor(when, 0);
		bigtime_t pt = PerformanceTimeFor(rt);
		PUBLISH_TIME(pt, rt, STARTING_DRIFT);
		BroadcastTimeWarp(rt, pt);
		fLastPublishedReal = 0;
	}
	break;
  case BTimedEventQueue::B_STOP:
	if (RunState() == BMediaEventLooper::B_STARTED) {
		printf("BEmuProducer::HandlEvent(): B_STOP\n");
		BAutolock lock(mLock);
		fStopTime = realTimeEvent ? TimeSource()->PerformanceTimeFor(when) : when;
		fStopping = true;
		stSAInputBuffer* blist = NULL;
		fClient->Stop(&blist);
		saCallback(blist);
		fPlaybackSync = 0;
		fPlaybackSample = 0;
		if (fDevSpec->fOutputClient)
		  if (fDevSpec->fOutputClient->GetSampleRate() == 44096)
			fDevSpec->fOutputClient->SetSampleRate(44100);

		bigtime_t rt = realTimeEvent ? when : TimeSource()->RealTimeFor(when, 0);
		PUBLISH_TIME(PerformanceTimeFor(rt), rt, 0)
	}
	break;
  }
}

status_t
BEmuProducer::DoBuffer(stSAInputBuffer* sabuf)
{
	SAOutputClient* out_client;
	int32 playback_sample;
	int32 playback_rate;
	bool playback_sync = false;

	BAutolock lock(mLock);

	// Get next full buffer
	BBuffer* fbuf = reinterpret_cast<BBuffer*> (sabuf->dwUser2);
	buf_spec* spec = &fBufSpec[sabuf->dwUser3];
	int32 fbuf_sample = spec->sample;
	int32 fbuf_size = MIN(fInputBufSize, fbuf->Size());

	// Need to sync playback?
	if (fSampleRate == 44100) {
	  out_client = fDevSpec->fOutputClient;
	  if (out_client) {
		playback_rate = out_client->GetSampleRate();
		if (playback_rate == 44100 || playback_rate == 44096) {
		  playback_sync = true;
		  playback_sample = out_client->GetCurrentSampleFrame();
		}
	  }
	}

	// Compute performance time for buffer
	int32 current_sample = fClient->GetCurrentSampleFrame();
	int32 offset = fbuf_size;
	if (current_sample != -1) {
	  offset = current_sample - fbuf_sample;
	  offset <<= 3;
	  offset >>= 3;
	}
	bigtime_t rtime = system_time() - (bigtime_t) (fUsecPerFrame * offset);
	bigtime_t ptime = TimeSource()->PerformanceTimeFor(rtime);
	if (fStopping && ptime >= fStopTime) {
	  fbuf->Recycle();
	  spec->pending = false;
	  return B_OK;
	}

	// Queue next empty buffer
	BBuffer *ebuf = fBufferGroup->RequestBuffer(MIN_BUF_SIZE);
	if (ebuf == NULL)
	  spec->pending = false;
	else {
	  sabuf->virtAddr = (void*) ebuf->Data();
	  sabuf->dwSize = MIN(fInputBufSize, ebuf->Size());
	  sabuf->dwUser2 = (DWORD) ebuf;		// Not 64-bit safe
	  spec->sample = fNextSample;
	  fNextSample += sabuf->dwSize / 4;
	  //fprintf(stderr,"calling AddBuffer(%d)\n",sabuf->dwSize);
	  fClient->AddBuffer(sabuf);
	}

	// Send full buffer
	if (fDisableOutput || fCaptureDestination == media_destination::null)
	  fbuf->Recycle();
	else {
	  fbuf->Header()->time_source = TimeSource()->ID();
	  fbuf->Header()->start_time = ptime;
	  fbuf->Header()->size_used = fbuf_size;
	  fbuf->Header()->type = B_MEDIA_RAW_AUDIO;
	  //printf("BEmuProducer::FLUSH() sending 0x%x\n", fbuf);
	  status_t err = SendBuffer(fbuf, fCaptureDestination);
	  if (err) {
		printf("BEmuProducer::FLUSH(): SendBuffer() returned 0x%x\n", err);
		fbuf->Recycle();
	  }
	}

//fprintf(stderr, "PT %d CS %d\n", (int)(ptime/1000), current_sample);
//fprintf(stderr,"0x%08x\n", current_sample);

	if (rtime && fLastPublishedReal)
	  if (rtime != fLastPublishedReal) {
		float drift = ((float) (ptime - fLastPublishedPerf)
					   / (rtime - fLastPublishedReal));
		if (drift > 0.5 && drift < 2.0)
		  fDrift = fDrift * 0.98 + drift * 0.02;
		fDrift = 1.0;
	  }
	fLastPublishedPerf = ptime;
	fLastPublishedReal = rtime;
//bigtime_t t1 = system_time();
//fprintf(stderr, "< %f  %f  %Ld >\n", t1 / 1000000.0, rtime / 1000000.0, rtime - t1);
//fprintf(stderr, "RT %d NOW %d\n", (int)(rtime/1000), (int)(system_time()/1000));
	if (system_time() - rtime < 1000000)
	  PUBLISH_TIME(ptime, rtime, fDrift);

	if (playback_sync) {
	  // Synchronize playback rate
	  if (fPlaybackSample == 0)
		fPlaybackSample = playback_sample;

	  if (fPlaybackSample == playback_sample) {
		// If playback not running, reset to 44.1k
		fPlaybackSync = 0;
		if (playback_rate == 44096)
		  out_client->SetSampleRate(44100);
	  }
	  else {
		fPlaybackSample = playback_sample;
		int32 sync = playback_sample - current_sample;
		if (fPlaybackSync == 0)
		  fPlaybackSync = sync;
		int32 delta = sync - fPlaybackSync;
		delta <<= 3;
		delta >>= 3;
		if (delta > 10)
		  if (delta > 100)
			fPlaybackSample = 0;
		  else if (playback_rate == 44100)
			out_client->SetSampleRate(44096);
		if (delta < -10)
		  if (delta < -100)
			fPlaybackSample = 0;
		  else if (playback_rate == 44096)
			out_client->SetSampleRate(44100);
	  }
	}

	return B_OK;
}


/*********************
	BControllable
**********************/

#define MCP(ID, NAME, TYPE, UNIT, BOT, TOP, STEP) \
	MakeContinuousParameter(2*(ID), B_MEDIA_RAW_AUDIO, NAME, TYPE, UNIT, BOT, TOP, STEP);
#define MDP(ID, NAME, TYPE) MakeDiscreteParameter(2*(ID), B_MEDIA_RAW_AUDIO, NAME, TYPE);
#define MNP(NAME, TYPE) MakeNullParameter(null_id++, B_MEDIA_RAW_AUDIO, NAME, TYPE);
#define MM(ID) MakeDiscreteParameter(2*(ID)+1, B_MEDIA_RAW_AUDIO, "Mute", B_MUTE);

void
BEmuProducer::ConstructControlWeb()
{
  int32 null_id = 'EMU~';
  BParameterWeb* web = new BParameterWeb;
  BParameterGroup* main = web->MakeGroup("Audio Input");

  if (fDevSpec->fMXRTYPE == MXRTYPE_AC97) {
	BParameterGroup* mux = main->MakeGroup("Source");
	mux->MNP("Source", B_GENERIC);
	BDiscreteParameter* source = mux->MDP(mxrRecordSelect, "", B_INPUT_MUX);
	if (fDevSpec->fHC & 0x4000)
	  source->AddItem(0x16, "SPDIF In");
	source->AddItem(4, "Line In");
	source->AddItem(0, "Mic");
	source->AddItem(1, "CD");
	source->AddItem(0x12, "CD SPDIF");
	//	source->AddItem(2, "Video");
	source->AddItem(3, "Aux");
	source->AddItem(5, "Line Out");
	source->AddItem(6, "Speaker Out");
	source->AddItem(7, "TAD");
	/*
	  for (int i = 16; i < 32; i += 2) {
	  char b[16];
	  sprintf(b, "%d", i);
	  source->AddItem(i, b);
	  }
	  */

	BParameterGroup* record = main->MakeGroup("Analog Record Level");
	record->MNP("Analog Record Level", B_GENERIC);
	BDiscreteParameter* mute = record->MM(mxrRecordGain);
	BContinuousParameter* level = record->MCP(mxrRecordGain, "Gain",
											  B_GAIN, "dB", 0.0, 22.5, 1.5);
	level->SetChannelCount(2);
	level->AddInput(mute);

	BParameterGroup* boost = main->MakeGroup("Mic Boost");
	boost->MNP("Mic Boost", B_GENERIC);
	boost->MakeDiscreteParameter(2 * mxrMICVolume, B_MEDIA_RAW_AUDIO,
								 "+20dB", B_ENABLE);
  }
  else if (fDevSpec->fMXRTYPE == MXRTYPE_ECARD) {
	bool has_bay = mxrGetControlValue(fDevSpec->fMXRID, mxrHasAudioBay);
	BParameterGroup* mux = main->MakeGroup("Source");
	mux->MNP("Source", B_GENERIC);
	BDiscreteParameter* source = mux->MDP(mxrMuxSelect1, "", B_INPUT_MUX);
	source->AddItem(0x100, "E-Card Analog In");
	source->AddItem(0x160, "E-Card SPDIF In");
	if (has_bay) {
	  source->AddItem(0x180, "E-Drive Analog In");
	  source->AddItem(0x161, "E-Drive SPDIF In");
	}
	source->AddItem(0x164, "CD SPDIF");

	BParameterGroup* record = main->MakeGroup("E-Card Analog Input Level");
	record->MNP("E-Card Analog Input Level", B_GENERIC);
	BContinuousParameter* level = record->MCP(mxrADCGain, "Gain",
											  B_GAIN, "dB", 0, 31.0, 0.5);
	level->SetChannelCount(2);
  }
  else
	main->MNP("Urecognized Mixer Type", B_GENERIC);

  SetParameterWeb(web);
}

status_t
BEmuProducer::GetParameterValue(int32 id, bigtime_t* last_change,
								 void* value, size_t* ioSize)
{
  card_settings* settings = &fDevSpec->settings;
  float* float_val = (float*) value;
  int32* int_val = (int32*) value;
  size_t size = *ioSize;
  if (size < 4)
	return B_BAD_VALUE;
  *last_change = fDataChangeTime;
  *ioSize = 4;

  bool mute = id & 1;
  id = id / 2;
  if (id < 0 || id > 16)
	return B_BAD_VALUE;

  int32 cv = mxrGetControlValue(fDevSpec->fMXRID, (enum MXRCONTROLID) id);
  if (cv != settings->mxr_regs[id])
	if (acquire_sem(fDevSpec->lock) == B_OK) {
	  settings->mxr_regs[id] = cv;
	  release_sem(fDevSpec->lock);
	}

  if (mute)
	*int_val = !!(cv & 0x8000);
  else switch (id) {
  case mxrMICVolume:
	*int_val = !!(cv & 0x40);
	break;
  case mxrRecordSelect:
	if (settings->input_channel == 0x10)
	  *int_val = cv & 7;
	else
	  *int_val = settings->input_channel;
	break;
  case mxrRecordGain:
	if (size < 8)
	  return B_BAD_VALUE;
	*ioSize = 8;
	float_val[0] = 1.5 * ((cv >> 8) & 0x0f);
	float_val[1] = 1.5 * (cv & 0x0f);
	break;
  case mxrRecordGainMic:
	float_val[0] = 1.5 * (cv & 0x0f);
	break;
  case mxrMuxSelect1:
	*int_val = (settings->input_channel << 4) + (cv & 7);
	break;
  case mxrADCGain:
	if (size < 8)
	  return B_BAD_VALUE;
	*ioSize = 8;
	float_val[0] = 0.5 * ((cv & 0xff) - 192);
	float_val[1] = 0.5 * ((cv >> 8) - 192);
	break;
  default:
	return B_BAD_VALUE;
  }

  return B_OK;
}

void 
BEmuProducer::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
  card_settings* settings = &fDevSpec->settings;
  const float* float_val = (float*) value;
  const int32* int_val = (int32*) value;
  if (size < 4)
	return;
  if (fDataChangeTime < when)
	fDataChangeTime = when;

  bool mute = id & 1;
  id = id / 2;
  if (id < 0 || id > 16)
	return;
  int32 cv = mxrGetControlValue(fDevSpec->fMXRID, (enum MXRCONTROLID) id);
  int32 new_cv = cv;
  if (acquire_sem(fDevSpec->lock) < 0)
	return;

  if (mute)
	new_cv = *int_val ? (cv | 0x8000) : (cv & ~0x8000);
  else {
	switch (id) {
	case mxrMICVolume:
	  new_cv = *int_val ? (cv | 0x40) : (cv & ~0x40);
	  break;
	case mxrRecordSelect:
	  if (*int_val > 7) {
		settings->input_channel = *int_val;
		set_input_channel(fDevSpec);
		break;
	  }
	  settings->input_channel = 0x10;
	  set_input_channel(fDevSpec);
	  new_cv = *int_val & 7;
	  new_cv |= new_cv << 8;
	  {
//		int32 new_gain = settings->ac97_record_gain[*int_val & 7];
//		mxrSetControlValue(fDevSpec->fMXRID, mxrRecordGain, new_gain);
//		fDevSpec->settings.mxr_regs[mxrRecordGain] = new_gain;
	  }
	  break;
	case mxrRecordGain:
	  if (size >= 8) {
		new_cv = cv & 0x8000;
		new_cv |= (int32) ((256 / 1.5) * float_val[0] + 0.5);
		new_cv |= (int32) (float_val[1] / 1.5 + 0.5);
		new_cv &= 0x8f0f;
//		if (settings->input_channel == 0x10)
//		  settings->ac97_record_gain[settings->mxr_regs[mxrRecordSelect] & 7] = cv;
	  }
	  break;
	case mxrRecordGainMic:
	  new_cv = cv & 0x8000;
	  new_cv |= (int32) (float_val[0] / 1.5 + 0.5);
	  new_cv &= 0x800f;
	  break;
	case mxrMuxSelect1:
	  settings->input_channel = *int_val >> 4;
	  set_input_channel(fDevSpec);
	  new_cv = *int_val & 7;
	  break;
	case mxrADCGain:
	  if (size >= 8) {
	  new_cv = 0xff & (int32) (float_val[0] * 2 + 192.5);
	  new_cv += ((int32) (float_val[1] * 2 + 192.5)) << 8;
	  }
	  break;
	}
  }
  if (new_cv != cv) {
	mxrSetControlValue(fDevSpec->fMXRID, (enum MXRCONTROLID) id, new_cv);
	fDevSpec->settings.mxr_regs[id] = new_cv;
  }
  release_sem(fDevSpec->lock);
}

status_t 
BEmuProducer::StartControlPanel(BMessenger * out_messenger)
{
	return B_ERROR;
}

void
BEmuProducer::MakeName()
{
	const char * str = fDevSpec->path;
	const char * strip = "/dev/audio/emu10k/";
	if (!strncmp(str, strip, strlen(strip))) str += strlen(strip);
	if (strlen(str) > 50) str += strlen(str)-50;
	sprintf(fName, "E-mu 10k1 In %s", str);
}
