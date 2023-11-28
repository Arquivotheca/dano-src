/* ++++++++++

   FILE:  EmuConsumer.cpp
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

+++++ */

#include "EmuConsumer.h"

#include <byteorder.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <scheduler.h>
#include <assert.h>

#include "ParameterWeb.h"
#include "tr_debug.h"
#include "Buffer.h"
#include "MediaAddOn.h"
#include "mixer_i586.h"

//#define TRACE printf
//#define DEBUG_DO_MESSAGE	1
//#define DEBUG_SERVICE_RUN	1
//#define DEBUG_ITERATE_FORMATS	1
#define DUMP_FILE 0

#if !DEBUG
#define fprintf
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

//#define X(N) {fprintf(stderr, "<%d>\n",(N));fflush(stdout);}
#define PERR(EXP) { \
	status_t err = (EXP); \
	if (err < 0) \
	fprintf (stderr, "BEmuConsumer: " #EXP " == 0x%x (%s)\n", \
			 err, strerror(err)); \
}

#define MIN_BUF_SIZE 16
#define OUTPUT_BUF_SIZE fPlayFormat.u.raw_audio.buffer_size
#define OUTPUT_BUF_TIME (OUTPUT_BUF_SIZE * 2500LL / 441)
#define NON_SCHED_LATENCY ((N_OUTPUT_BUFFERS - 1) * OUTPUT_BUF_TIME)
#define STARTING_DRIFT 1.0

#if DUMP_FILE
int dump_file = -1;
int dump_cnt = 0;
#endif

static void
saCallback(stSAOutputBuffer* buf)
{
  for (; buf; buf = buf->pNextBuffer)
	release_sem(buf->dwUser1);
}

static void
set_send_channels(SAOutputClient* client, dev_spec* dev)
{
  client->SetSendRouting(0, dev->settings.send_channel_1);
  client->SetSendRouting(1, dev->settings.send_channel_1 + 1);
  client->SetSendRouting(2, dev->settings.send_channel_2);
  client->SetSendRouting(3, dev->settings.send_channel_2 + 1);
  for (int i = 0; i < 4; i++)
	client->SetSendAmount(i, 255);
}


BEmuConsumer::BEmuConsumer(BEmuAddOn *addon, dev_spec* spec,
						   char* name, int32 id, status_t* status)
  : BMediaNode(name),
	BMediaEventLooper(),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	BTimeSource(),
	fAddOn(addon),
	fClient(NULL),
	fOutputMgr(NULL),
	fPlaying(false),
	fInternalID(id),
	mLock("emu consumer lock", true)
{
  //fprintf(stderr, "BEmuConsumer::BEmuConsumer()\n");
  fDevSpec = spec;
  SetPriority(B_REAL_TIME_PRIORITY);
  status_t err = BEmuConsumer_Initialize();
  if (*status)
	*status = err;
}


BEmuConsumer::~BEmuConsumer()
{
  BMediaEventLooper::Quit();
  fDevSpec->close();
}

status_t
BEmuConsumer::DeleteHook(BMediaNode*)
{
  delete this;
  return B_OK;
}

void
BEmuConsumer::SetTimeSource(BTimeSource * time_source)
{
  BMediaNode::SetTimeSource(time_source);
}

status_t
BEmuConsumer::SetupEmu()
{
  stSAOutputConfig config;
  memset(&config, 0, sizeof(config));
  config.callback = saCallback;
  config.sampleFormat = saFormatSigned16PCM;
  config.dwSampleRate = 44100;
  config.byNumChans = 2;
  config.playbackMode = sapmControlFrequency;

  fOutputMgr = new SAOutputMgr();
  fOutputDevice = new SASE8210OutputDevice(fDevSpec->fHRMID,  0);
  if (!fOutputMgr || !fOutputDevice)
	return B_NO_MEMORY;

  fOutputMgr->AddDevice(fOutputDevice);
  if (fOutputMgr->IsBad()) {
	fprintf(stderr, "AddDevice() failed.\n");
	return B_ERROR;
  }

  fClient = fOutputMgr->CreateDevClient(0, &config);
  if (fClient == NULL) {
	fprintf(stderr, "CreateDevClient() failed.\n");
	return B_ERROR;
  }
  fDevSpec->fOutputClient = fClient;

  set_send_channels(fClient, fDevSpec);

  return B_NO_ERROR;
}

status_t
BEmuConsumer::BEmuConsumer_Initialize()
{
	// Let the system know we do physical output
	AddNodeKind(B_PHYSICAL_OUTPUT);
	
	status_t err = SetupEmu();
	if (err < 0)
	  return err;

	// Create a completion semaphore
	fWriteCompletion = create_sem(N_OUTPUT_BUFFERS, "output completion");

	fPlayFormat.type = B_MEDIA_RAW_AUDIO;
	fPlayFormat.u.raw_audio.frame_rate = 44100.0;
	fPlayFormat.u.raw_audio.channel_count = 2;
	fPlayFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	fPlayFormat.u.raw_audio.byte_order
	  = (B_HOST_IS_BENDIAN ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN);
	fPlayFormat.u.raw_audio.buffer_size = fDevSpec->preferred_buffer_size;

	fLastPublishedReal = 0;
	fDrift = STARTING_DRIFT;
	fPerfSyncSample = 0;
	fPerfSyncStart = 0;
	fDataChangeTime = 0;

	fPlaySource = media_source::null;
	fPlayDestination.port = ControlPort();
	fPlayDestination.id = 0;
	
	fNextBuffer = 0;

	MakeName();

	return B_OK;
}

void
BEmuConsumer::NodeRegistered()
{
	ConstructControlWeb();
	BMediaEventLooper::NodeRegistered();
}

/******************
	BMediaNode
*******************/

BMediaAddOn*
BEmuConsumer::AddOn(int32* internal_id) const
{	
	// Who instantiated you -- or NULL for app class
	*internal_id = fInternalID;
	return fAddOn;
}

status_t 
BEmuConsumer::HandleMessage(int32 code, const void *data, size_t size)
{
  return B_ERROR;
}


/*********************
	BBufferConsumer
**********************/

status_t
BEmuConsumer::FormatChanged(const media_source & producer,const media_destination & consumer, 
	int32 from_change_count, const media_format & format)
{
	return B_ERROR;
}

status_t
BEmuConsumer::AcceptFormat(const media_destination &dest, media_format * format)
{
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

		if (format->u.raw_audio.format != fPlayFormat.u.raw_audio.format &&
				format->u.raw_audio.format != media_raw_audio_format::wildcard.format)
			bad = true;

		if (format->u.raw_audio.byte_order != fPlayFormat.u.raw_audio.byte_order &&
				format->u.raw_audio.byte_order != media_raw_audio_format::wildcard.byte_order)
			bad = true;

		if (format->u.raw_audio.buffer_size > media_raw_audio_format::wildcard.buffer_size) {
		  if (format->u.raw_audio.buffer_size > suggestion.u.raw_audio.buffer_size)
			bad = true;
		  else if (format->u.raw_audio.buffer_size < MIN_BUF_SIZE) {
			suggestion.u.raw_audio.buffer_size = MIN_BUF_SIZE;
			bad = true;
		  }
		  else
			suggestion.u.raw_audio.buffer_size = format->u.raw_audio.buffer_size;
		}
	}
	
	*format = suggestion;
	return bad ? B_MEDIA_BAD_FORMAT : B_OK;
}

status_t
BEmuConsumer::GetNextInput(int32* cookie, media_input* out_input)
{
  //fprintf(stderr, "BEmuConsumer::GetNextInput(%d) - BEGIN: fd = %d\n", *cookie, fDevSpec->fd);
	if (fDevSpec->fd < 0 || *cookie)
		return B_MEDIA_BAD_DESTINATION;

	BAutolock lock(mLock);
	out_input->node = Node();
	out_input->destination = fPlayDestination;
	out_input->source = fPlaySource;	
	out_input->format = fPlayFormat;
//	sprintf(out_input->name, "BEmuConsumer Input %d", *cookie);
	strcpy(out_input->name, fName);
	*cookie = 1;

	return B_OK;
}

void
BEmuConsumer::DisposeInputCookie(int32 cookie)
{
	// Don't do anything special since we didn't allocate
	// anything on the cookie's behalf.
}

void
BEmuConsumer::ProducerDataStatus(const media_destination &for_whom, int32 status, bigtime_t at_performance_time)
{
  media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS);
  event.pointer = (void *)for_whom.id;
  event.bigdata = status;
  if (EventQueue()->AddEvent(event) != B_OK)
	fProducerStatus = status;
}

status_t
BEmuConsumer::GetLatencyFor(const media_destination &,
	bigtime_t * out_latency, media_node_id * out_timesource)
{
	*out_timesource = TimeSource()->ID();
	*out_latency = EventLatency();
	return B_OK;
}

void
BEmuConsumer::BufferReceived(BBuffer* buffer)
{
  //fprintf(stderr, "BEmuConsumer::BufferReceived(%x) - BEGIN\n", buffer);

#if DUMP_FILE
	if (dump_file > 0 && dump_cnt < 1000) {
		fprintf(stderr, "%x:%x\n", buffer->Data(), buffer->Header()->size_used);
		write(dump_file, buffer->Data(), buffer->Header()->size_used);
		dump_cnt++;
	}
	else if (dump_cnt > 999) {
		close(dump_file);
		dump_file = -1;
		dump_cnt = 0;
	}
#endif

	if (RunState() == BMediaEventLooper::B_STARTED) {
	  bigtime_t when = buffer->Header()->start_time;
	  media_timed_event event(when, BTimedEventQueue::B_HANDLE_BUFFER,
							  buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
	  event.bigdata = when;
	  if (EventQueue()->AddEvent(event) != B_OK)
		buffer->Recycle();
	}
	else
	  buffer->Recycle();
}

status_t
BEmuConsumer::Connected(		/* be sure to call BBufferConsumer::Connected()! */
	const media_source &producer,	/* here's a good place to request buffer group usage */
	const media_destination &where,
	const media_format & with_format,
	media_input * out_input)
{
  //fprintf(stderr, "BEmuConsumer::Connected()\n");
	BBufferConsumer::Connected(producer, where, with_format, out_input);

	BAutolock lock(mLock);
	fPlaySource = producer;
	out_input->node = Node();
	out_input->source = fPlaySource;
	out_input->destination = fPlayDestination;
	out_input->format = with_format;
//	sprintf(out_input->name, "BEmuConsumer hookedup");
	strcpy(out_input->name, fName);
#if DUMP_FILE
	if (dump_file < 0) {
		dump_file = open("/tmp/dumped_data.raw", O_RDWR | O_CREAT | O_TRUNC, 0666);
	}
#endif

	OUTPUT_BUF_SIZE = with_format.u.raw_audio.buffer_size;
	int32 obuf_pages = (N_OUTPUT_BUFFERS * OUTPUT_BUF_SIZE + 4095) / 4096;
	if (obuf_pages > fDevSpec->fOutputAreaPages) {
	  if (fDevSpec->fOutputBufferBase)
		osFreePages(fDevSpec->fOutputArea);
	  fDevSpec->fOutputBufferBase = (char*)
		osAllocContiguousPages(obuf_pages, (OSPHYSADDR *) &fDevSpec->fOutputBufferPhysical,
							   (uint32*) &fDevSpec->fOutputArea);
	  if (!fDevSpec->fOutputBufferBase) {
		fDevSpec->fOutputAreaPages = 0;
		fprintf(stderr, "BEmuConsumer::Start - osAllocContiguousPages(%d) failed\n", obuf_pages);
	  }
	  else
		fDevSpec->fOutputAreaPages = obuf_pages;
	}

	SetEventLatency(NON_SCHED_LATENCY);
	SetBufferDuration(OUTPUT_BUF_TIME);

	return B_OK;
}


void
BEmuConsumer::Disconnected(const media_source &producer, const media_destination &where)
{
  //fprintf(stderr, "BEmuConsumer::Disconnected()\n");
  fPlaySource = media_source::null;
}


/*********************
	BTimeSource
**********************/

status_t 
BEmuConsumer::TimeSourceOp(const time_source_op_info &op, void *_reserved)
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
BEmuConsumer::ControlLoop()
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
	  fprintf(stderr, "WaitForMessage error: %s (%d) ONLINE\n",
			  strerror(err), err);
	  SetRunState(B_IN_DISTRESS);
	  ReportError(B_NODE_IN_DISTRESS);
	  break;
	}
  }
}

void
BEmuConsumer::SetRunMode(run_mode mode)
{
  BMediaEventLooper::SetRunMode(mode);
  BTimeSource::SetRunMode(mode);
}

void
BEmuConsumer::HandleEvent(const media_timed_event* event,
						  bigtime_t lateness,
						  bool realTimeEvent)
{
  bigtime_t when = event->event_time;

  if(0)fprintf(stderr, "BEmuConsumer::HandleEvent(%Ld, %ld, 0x%lx, %ld, %ld, %Ld)\n",
		  event->event_time, event->type, (uint32)event->pointer, event->cleanup,
		  event->data, event->bigdata);

  switch(event->type) {
  case BTimedEventQueue::B_START:
	if (RunState() == BMediaEventLooper::B_STOPPED) {
	  bigtime_t rt = realTimeEvent ? when : TimeSource()->RealTimeFor(when, 0);
	  bigtime_t pt = PerformanceTimeFor(rt);
	  PUBLISH_TIME(pt, rt, STARTING_DRIFT);
	  BroadcastTimeWarp(rt, pt);
	  fLastPublishedReal = 0;
	}
	break;

  case BTimedEventQueue::B_STOP:
	if (RunState() == BMediaEventLooper::B_STARTED) {
	  bigtime_t rt = realTimeEvent ? when : TimeSource()->RealTimeFor(when, 0);
	  PUBLISH_TIME(PerformanceTimeFor(rt), rt, 0);
	}
	break;

  case BTimedEventQueue::B_SEEK:
	{
	  bigtime_t rt = realTimeEvent ? when : TimeSource()->RealTimeFor(when, 0);
	  PUBLISH_TIME(event->bigdata, rt,
				   RunState() == BMediaEventLooper::B_STARTED ? fDrift : 0);
	  BroadcastTimeWarp(rt, event->bigdata);
	  fLastPublishedReal = 0;
	}
	break;

  case BTimedEventQueue::B_WARP:
	EventQueue()->FlushEvents(event->bigdata, BTimedEventQueue::B_BEFORE_TIME,
							  false, BTimedEventQueue::B_HANDLE_BUFFER);
	break;

  case BTimedEventQueue::B_DATA_STATUS:
	fProducerStatus = event->bigdata;
	break;

  case BTimedEventQueue::B_HANDLE_BUFFER:
	BBuffer* ibuf = reinterpret_cast<BBuffer*>(event->pointer);
	DoBuffer(event, ibuf, lateness);
	ibuf->Recycle();
	break;
  }
}

status_t
BEmuConsumer::DoBuffer(const media_timed_event* event,
					   BBuffer* ibuf,
					   bigtime_t lateness)
{
	BTimeSource* ts = TimeSource();
	status_t err;

	if (fDevSpec->fd < 0)
	  return B_OK;

	bigtime_t now = ts->RealTime();
	bigtime_t then = ts->RealTimeFor(event->event_time, 0);

	char* src = (char*) ibuf->Data();
	int32 isize = ibuf->Header()->size_used;

	//fprintf(stderr, "BEmuConsumer::PlayRun - buffer size = %d\n", isize);

	if (isize > OUTPUT_BUF_SIZE) {
	  fprintf(stderr, "BEmuConsumer::PlayRun - input buffer too large %d\n", isize);
	  return B_OK;
	}
	if (then < now) {
	  fprintf(stderr, "<%dms late>\n", (int)((now - then)/1000));
	  return B_OK;
	}
	
	// wait for completion
	while (1) {
	  err = acquire_sem_etc(fWriteCompletion, 1, B_TIMEOUT, EventLatency());
	  if (err == B_OK)
		break;
	  if (err != B_INTERRUPTED)
		return err;
	}

	//fprintf(stderr, "BEmuConsumer::PlayRun - write and recycle (%Ld)\n", now);

	fOutputBuffer = fDevSpec->fOutputBufferBase + fNextBuffer * OUTPUT_BUF_SIZE;
	stSAOutputBuffer* sabuf = &fOutputBuffers[fNextBuffer];
	memset(sabuf, 0, sizeof(*sabuf));
	sabuf->virtAddr = (void*) fOutputBuffer;
	sabuf->dwSize = isize;
	sabuf->dwUser1 = fWriteCompletion;

	// hack around slow physical address calculation in _AddBuffer()
	int buf_pages = (OUTPUT_BUF_SIZE + 4095) / 4096;
	sabuf->dwReserved = (DWORD) NewAlloc(4 * buf_pages);
	if (sabuf->dwReserved)
	  for (int i = 0; i < buf_pages; i++)
		((int32*) sabuf->dwReserved)[i] = (fDevSpec->fOutputBufferPhysical
										   + i * 4096
										   + fNextBuffer * OUTPUT_BUF_SIZE);

	memcpy(fOutputBuffer, src, isize);

	bigtime_t ptime = ibuf->Header()->start_time;
	int32 time_skipped = ptime - (fPerfSyncStart + isize * 2500LL/441);
	if (time_skipped > 1000) {
	  bigtime_t wait = then - ts->RealTime();
	  if (wait > 20 && wait <= EventLatency())
		snooze(wait);
	}

	fNextBuffer = (fNextBuffer + 1) % N_OUTPUT_BUFFERS;

	//static bigtime_t t1, t2;
	//t2 = system_time();
	//fprintf(stderr, ">> %d\n", (uint32) ((t2 - t1) / 1000));
	//t1 = t2;
	//fprintf(stderr, "calling AddBuffer(%d)\n", i);
	fClient->AddBuffer(sabuf);
	//fprintf(stderr, "returned from AddBuffer(%d)\n", i);
	if (!fPlaying) {
	  fPlaying = true;
	  //fprintf(stderr, "calling Play()\n");
	  fClient->Play();
	  //fprintf(stderr, "returned from Play()\n");
	  fPerfSyncSample = 0;
	}

	bigtime_t current_time = system_time();
	int32 current_sample = fClient->GetCurrentSampleFrame();
//fprintf(stderr, "PT %d CS %d\n", (int)(ptime/1000), current_sample);
//fprintf(stderr,"0x%08x\n", current_sample);

	//fprintf(stderr, "< %Ld >\n", then - now);

	if (current_sample != -1) {
	  int32 offset = fPerfSyncSample - current_sample;
	  offset <<= 3;
	  offset >>= 3;
	  bigtime_t rtime = current_time + (offset * 10000LL / 441);
	  if (rtime && fLastPublishedReal)
		if (rtime != fLastPublishedReal) {
		  float drift = ((float) (ptime - fLastPublishedPerf)
						 / (rtime - fLastPublishedReal));
		  if (drift > 0.5 && drift < 2.0)
			fDrift = fDrift * 0.98 + drift * 0.02;
		  fDrift = 1.0;
		}
//fprintf(stderr, "< %f  %f >\n", fPerfSyncDelta / 1000000.0, clock/1000000.0);
	  fLastPublishedPerf = ptime;
	  fLastPublishedReal = rtime;
//bigtime_t t1 = system_time();
//fprintf(stderr, "< %f  %f  %Ld >\n", t1 / 1000000.0, rtime / 1000000.0, rtime - t1);
//		BAutolock lock(mLock);
//fprintf(stderr, "RT %d NOW %d\n", (int)(rtime/1000), (int)(system_time()/1000));
	  if (system_time() - rtime < 1000000)
		PUBLISH_TIME(ptime, rtime, fDrift);
	}

	fPerfSyncStart = ptime;
	fPerfSyncSample += isize / 4;

	return err;
}


/*********************
	BControllable
**********************/

#define MCP(ID, NAME, TYPE, UNIT, BOT, TOP, STEP) \
	MakeContinuousParameter(2*(ID), B_MEDIA_RAW_AUDIO, NAME, TYPE, UNIT, BOT, TOP, STEP);
#define MDP(ID, NAME, TYPE) MakeDiscreteParameter(2*(ID), B_MEDIA_RAW_AUDIO, NAME, TYPE);
#define MNP(NAME, TYPE) MakeNullParameter(null_id++, B_MEDIA_RAW_AUDIO, NAME, TYPE);
#define MM(ID) MakeDiscreteParameter(2*(ID)+1, B_MEDIA_RAW_AUDIO, "Mute", B_MUTE);

struct ac97_channel_spec {
  char* name;
  MXRCONTROLID id;
  int32 n_channels;
};

static ac97_channel_spec channel_list[] = {
  {"Mixer", mxrPCMOutVolume, 2},
  {"Line In", mxrLineInVolume, 2},
  {"Mic", mxrMICVolume, 1},
  {"CD", mxrCDVolume, 2},
  //  {"Video", mxrVideoVolume, 2},
  {"Aux", mxrAuxVolume, 2},
  {"TAD", mxrPhoneVolume, 1},
  {0, (MXRCONTROLID) 0, 0}};

void
BEmuConsumer::ConstructControlWeb()
{
  int32 null_id = 'EMU~';
  BParameterWeb* web = new BParameterWeb;
  BParameterGroup* main = web->MakeGroup("Output");

  if (fDevSpec->fMXRTYPE == MXRTYPE_AC97) {
	BParameterGroup* analog = web->MakeGroup("Analog Mix");
	analog->MNP("Analog Mix", B_GENERIC);

	BParameterGroup* out = analog->MakeGroup("Output Levels");
	BParameterGroup* lineout = out->MakeGroup("Line Out");
	lineout->MNP("Line Out", B_GENERIC);
	BDiscreteParameter* mute = lineout->MM(mxrMasterVolume);
	BContinuousParameter* level = lineout->MCP(mxrMasterVolume, "Level",
											   B_MASTER_GAIN, "dB", -46.5, 0.0, 1.5);
	level->SetChannelCount(2);
	level->AddInput(mute);

	BParameterGroup* speaker = out->MakeGroup("Speaker");
	speaker->MNP("Speaker", B_GENERIC);
	mute = speaker->MM(mxrMonoVolume);
	level = speaker->MCP(mxrMonoVolume, "Level", B_MASTER_GAIN, "dB", -46.5, 0.0, 1.5);
	level->SetChannelCount(1);
	level->AddInput(mute);

	BParameterGroup* channels = analog->MakeGroup("Channels");
	for (ac97_channel_spec* chan = channel_list; chan->name; chan++) {
	  BParameterGroup* grp = channels->MakeGroup(chan->name);
	  grp->MNP(chan->name, B_GENERIC);
	  mute = grp->MM(chan->id);
	  level = grp->MCP(chan->id, "Gain", B_GAIN, "dB", -34.5, 12.0, 1.5);
	  level->SetChannelCount(chan->n_channels);
	  level->AddInput(mute);
	}
  }
  else if (fDevSpec->fMXRTYPE == MXRTYPE_ECARD) {
	main->MNP("Use Audio Mixer to set output level." , B_GENERIC);
	if (mxrGetControlValue(fDevSpec->fMXRID, mxrHasAudioBay)) {
	  BParameterGroup* digital = main->MakeGroup("Digital Send");
	  BDiscreteParameter* send =
		digital->MakeDiscreteParameter('SEND', B_MEDIA_RAW_AUDIO,
									   "Digital Send", B_OUTPUT_MUX);
	  send->AddItem(2, "E-Card SPDIF Out");
	  send->AddItem(4, "E-Drive SPDIF Out");
	}
  }
  else
	main->MNP("Urecognized Mixer Type" , B_GENERIC);

  SetParameterWeb(web);
}

status_t
BEmuConsumer::GetParameterValue(int32 id, bigtime_t* last_change,
								void* value, size_t* ioSize)
{
  float* float_val = (float*) value;
  int32* int_val = (int32*) value;
  size_t size = *ioSize;
  if (size < 4)
	return B_BAD_VALUE;
  *last_change = fDataChangeTime;
  *ioSize = 4;

  if (id == 'SEND') {
	*int_val = fDevSpec->settings.send_channel_2;
	return B_OK;
  }

  bool mute = id & 1;
  id = id / 2;
  if (id < 0 || id > 16)
	return B_BAD_VALUE;

  int32 cv = mxrGetControlValue(fDevSpec->fMXRID, (enum MXRCONTROLID) id);
  if (cv != fDevSpec->settings.mxr_regs[id])
	if (acquire_sem(fDevSpec->lock) == B_OK) {
	  fDevSpec->settings.mxr_regs[id] = cv;
	  release_sem(fDevSpec->lock);
	}

  if (mute)
	*int_val = !!(cv & 0x8000);
  else switch (id) {
  case mxrMasterVolume:
	if (size < 8)
	  return B_BAD_VALUE;
	*ioSize = 8;
	float_val[0] = -1.5 * ((cv >> 8) & 0x3f);
	float_val[1] = -1.5 * (cv & 0x3f);
	break;
  case mxrMonoVolume:
	float_val[0] = -1.5 * (cv & 0x3f);
	break;
  case mxrPCMOutVolume:
  case mxrLineInVolume:
  case mxrCDVolume:
  case mxrVideoVolume:
  case mxrAuxVolume:
	if (size < 8)
	  return B_BAD_VALUE;
	*ioSize = 8;
	float_val[0] = 12 - 1.5 * ((cv >> 8) & 0x1f);
	float_val[1] = 12 - 1.5 * (cv & 0x1f);
	break;
  case mxrMICVolume:
  case mxrPhoneVolume:
	float_val[0] = 12 - 1.5 * (cv & 0x3f);
	break;
  default:
	return B_BAD_VALUE;
  }

  return B_OK;
}

void 
BEmuConsumer::SetParameterValue(int32 id, bigtime_t when,
								const void *value, size_t size)
{
  const float* float_val = (float*) value;
  const int32* int_val = (int32*) value;
  if (size < 4)
	return;

  if (fDataChangeTime < when)
	fDataChangeTime = when;

  if (id == 'SEND') {
	if (acquire_sem(fDevSpec->lock) < 0)
	  return;
	fDevSpec->settings.send_channel_2 = *int_val;
	set_send_channels(fClient, fDevSpec);
	release_sem(fDevSpec->lock);
	return;
  }

  bool mute = id & 1;
  id /= 2;
  if (id < 0 || id > 32)
	return;

  int32 cv = mxrGetControlValue(fDevSpec->fMXRID, (enum MXRCONTROLID) id);
  int32 new_cv = cv;
  if (acquire_sem(fDevSpec->lock) < 0)
	return;

  if (mute)
	new_cv = *int_val ? (cv | 0x8000) : (cv & ~0x8000);
  else {
	switch (id) {
	case mxrMasterVolume:
	  if (size >= 8) {
		new_cv = cv & 0x8000;
		new_cv |= (int32) ((256 / -1.5) * float_val[0] + 0.5);
		new_cv |= (int32) (float_val[1] / -1.5 + 0.5);
		new_cv &= 0x9f1f;
	  }
	  break;
	case mxrMonoVolume:
	  new_cv = cv & 0x8000;
	  new_cv |= (int32) (float_val[0] / -1.5 + 0.5);
	  new_cv &= 0x801f;
	  break;
	case mxrPCMOutVolume:
	case mxrLineInVolume:
	case mxrCDVolume:
	case mxrVideoVolume:
	case mxrAuxVolume:
	  if (size >= 8) {
		new_cv = cv & 0x8000;
		new_cv |= (int32) ((256 / -1.5) * (float_val[0] - 12) + 0.5);
		new_cv |= (int32) ((float_val[1] - 12) / -1.5 + 0.5);
		new_cv &= 0x9f1f;
	  }
	  break;
	case mxrMICVolume:
	case mxrPhoneVolume:
	  new_cv = cv & 0x8000;
	  new_cv |= (int32) ((float_val[0] - 12) / -1.5 + 0.5);
	  new_cv &= 0x801f;
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
BEmuConsumer::StartControlPanel(BMessenger * out_messenger)
{
	return B_ERROR;
}

void
BEmuConsumer::MakeName()
{
	const char * str = fDevSpec->path;
	const char * strip = "/dev/audio/emu10k/";
	if (!strncmp(str, strip, strlen(strip))) str += strlen(strip);
	if (strlen(str) > 50) str += strlen(str)-50;
	sprintf(fName, "E-mu 10k1 Out %s", str);
}
