/* ++++++++++

   FILE:  LegoProducer.cpp
   REVS:  $Revision: 1.1 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

+++++ */

#include "LegoProducer.h"

#include <Autolock.h>
#include <byteorder.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <R3MediaDefs.h>
#include <scheduler.h>

#include "ParameterWeb.h"
#include "TimeSource.h"
#include "tr_debug.h"
#include "sound.h"
#include "Buffer.h"
#include "BufferGroup.h"
#include "MediaAddOn.h"

//#define TRACE printf
//#define DEBUG_DO_MESSAGE	1
//#define DEBUG_SERVICE_RUN	1
//#define DEBUG_ITERATE_FORMATS	1

#if !DEBUG
#define printf(x...)
#endif

#define X(N) {printf("<%d> ",(N));fflush(stdout);}
#define PERR(EXP) { \
	status_t err = (EXP); \
	if (err < 0) \
	fprintf (stderr, "BLegoProducer: " #EXP " == 0x%lx (%s)\n", err, strerror(err)); \
}

#define IOCTL(FD,OP,AD,SZ) (ioctl(FD,OP,AD,SZ) < 0 ? errno : B_OK)

#define DEFAULT_TIMEOUT 6000000L
#define DEFAULT_N_BUFFERS 16
#define DEFAULT_SCHED_LATENCY 500

enum {
  BLP_SOURCE,
  BLP_ADC_LEVEL,
  BLP_NULL_PARAMS
};


BLegoProducer::BLegoProducer(BLegoAddOn *addon, dev_spec* spec,
								char* name, int32 id, status_t* status)
	: BMediaNode(name),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	fRunThreadID(B_BAD_THREAD_ID),
	fServiceThreadID(B_BAD_THREAD_ID),
	fControlPort(-1),
	fAddOn(addon),
	fInputSize(spec->preferred_buffer_size),
	fCaptureTime(0),
	fChangeTag(0),
	fInternalID(id)
{
	fDevSpec = spec;
	fSchedLatency = DEFAULT_SCHED_LATENCY;
	fDefaultBufferGroup = new BBufferGroup(fInputSize, DEFAULT_N_BUFFERS, B_ANY_ADDRESS);
	fBufferGroup = fDefaultBufferGroup;
	status_t err = fBufferGroup->InitCheck();
	if (err < 0)
	  PERR(fBufferGroup->InitCheck())
	else
	  err = BLegoProducer_Initialize();
	if (status)
	  *status = err;
}


BLegoProducer::~BLegoProducer()
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
	delete fBufferGroup;
}

void
BLegoProducer::SetTimeSource(BTimeSource * time_source)
{
	BMediaNode::SetTimeSource(time_source);
}

status_t
BLegoProducer::BLegoProducer_Initialize()
{
	// Now setup our service port
	fControlPort = create_port(10, "BLegoProducer service port");

	// Let the system know we do physical input
	AddNodeKind(B_PHYSICAL_INPUT);

	fd = fDevSpec->fd;

	// Create a completion semaphore
	fReadCompletion = create_sem(0, "input completion");
	fReadsPending = 0;
	ioctl(fd, SOUND_SET_CAPTURE_COMPLETION_SEM,
		  &fReadCompletion, sizeof(fReadCompletion));

	sound_setup p;
	status_t err = IOCTL(fd, SOUND_GET_PARAMS, &p, sizeof(p)); 
	if (err < 0) {
	  fprintf(stderr, "BLegoProducer::BLegoProducer_Initialize"
			  " - SOUND_GET_PARAMS returned: %lx\n", err);
	  p.sample_rate = kHz_44_1;
	}
	else {
	  // print out sound params
	  printf("  Sample Rate: %d\n", p.sample_rate);
	  printf("  Capt Format: %d\n", p.capture_format);
	  // unmute DAC
	  p.left.dac_mute = 0;
	  p.right.dac_mute = 0;
	  ioctl(fd, SOUND_SET_PARAMS, &p, sizeof(p));
	}

	fCaptureFormat.type = B_MEDIA_RAW_AUDIO;
	fCaptureFormat.u.raw_audio.frame_rate = sr_enum_to_float(p.sample_rate);
	if (fCaptureFormat.u.raw_audio.frame_rate == 0)
	  fCaptureFormat.u.raw_audio.frame_rate = 44100;
	fCaptureFormat.u.raw_audio.channel_count = 2;
	fCaptureFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	fCaptureFormat.u.raw_audio.byte_order
	  = (B_HOST_IS_BENDIAN ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN);
	fCaptureFormat.u.raw_audio.buffer_size = fInputSize;

	fUsecPerByte = 1e6
	  / (double) fCaptureFormat.u.raw_audio.frame_rate
	  / ((fCaptureFormat.u.raw_audio.format & 0xf)
		 * fCaptureFormat.u.raw_audio.channel_count);

	mRunning = false;
	mStartTime = 0;		/* when to start in performance time */
	mStopTime = 0;		/* when to stop in performance time */

	fCaptureDestination = media_destination::null;
	fCaptureSource.port = fControlPort;
	fCaptureSource.id = 0;
	fDataChangeTime = 0;
	
	fNextBuffer = 0;

	MakeName();

	return B_OK;
}

void
BLegoProducer::NodeRegistered()
{
	ConstructControlWeb();

	// Start the control port servicing thread
	// In R4 you'll do it like this
	//int servicePrio = suggest_thread_priority(B_AUDIO_PLAYBACK, 0, 0, 0);

	fServiceThreadKeepGoing = true;
	fServiceThreadID = spawn_thread(ServiceThread, "Lego Service Thread",
		B_REAL_TIME_PRIORITY, this);
	resume_thread(fServiceThreadID);
}

/******************
	BMediaNode
*******************/

BMediaAddOn*
BLegoProducer::AddOn(int32* flavor_id) const
{	
	// Who instantiated you -- or NULL for app class
	*flavor_id = fInternalID;
	return fAddOn;
}

port_id
BLegoProducer::ControlPort() const
{
	//TRACE("BLegoProducer::ControlPort: %d\n", fControlPort);
	return fControlPort;
}

void
BLegoProducer::HandleBadMessage(int32 /*code*/, const void * /*data*/, size_t /*size*/)
{
	// do nothing for now
	printf("BLegoProducer::HandleBadMessage node %d port %d code 0x%x\n", ID(), ControlPort(), code);
}

status_t 
BLegoProducer::HandleMessage(int32 code, const void *data, size_t size)
{
#ifdef DEBUG_DO_MESSAGE
	printf("BLegoProducer::HandleMessage - BEGIN (0x%x)\n", code);
#endif

	status_t status = BMediaNode::HandleMessage(code, data, size);
	if (status == B_OK)
		goto handled;
//	status = BTimeSource::HandleMessage(code, data, size);
#ifdef DEBUG_DO_MESSAGE
		printf("Status from BTimeSource::HandleMessage: 0x%x\n", status);
#endif
	if (status == B_OK)
		goto handled;
	status = BBufferProducer::HandleMessage(code, data, size);
	if (status == B_OK)
		goto handled;
	status = BControllable::HandleMessage(code, data, size);
	if (status == B_OK)
		goto handled;

	HandleBadMessage(code, data, size);

handled:
#ifdef DEBUG_DO_MESSAGE
	printf("BLegoProducer::HandleMessage - END\n");
#endif

	return status;
}


/*********************
	BMediaNode
**********************/

void
BLegoProducer::Start(bigtime_t performance_time)		/* be sure to call BMediaNode::Start()! */
{
	//printf("BLegoProducer::Start()\n");
	BMediaNode::Start(performance_time);
	BAutolock lock(&mLock);

	if (!mRunning)
	{
		int32 total_size = fInputSize + sizeof(audio_buffer_header);
		for (int i = 0; i < N_INPUT_BUFFERS; i++) {
		  if (!fDevSpec->fInputHeaders[i]) {
			int32 size = (total_size + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
			/* workaround for keeping area hanging around post-quit */
			/* The name has to be per-device, else this will clobber other cards. */
			char name[32];
			sprintf(name, "_LIB %.20s %d", fDevSpec->path + 15, i);
			bool create = true;
			fDevSpec->fInputAreas[i] = find_area(name);
			if (fDevSpec->fInputAreas[i] > 0) {
				area_info info;
				if (get_area_info(fDevSpec->fInputAreas[i], &info) >= B_OK) {
					if ((info.size >= (size_t)size) && !(((long)info.address)&0x80000000)) {
						create = false;
						fDevSpec->fInputHeaders[i] = (audio_buffer_header *)info.address;
					}
				}
			}
			if (create)
			  fDevSpec->fInputAreas[i] = create_area (name,
										  (void**) &fDevSpec->fInputHeaders[i],
										  B_ANY_KERNEL_ADDRESS, size, B_CONTIGUOUS,
										  B_READ_AREA | B_WRITE_AREA);
			if (fDevSpec->fInputAreas[i] < 0)
			  fDevSpec->fInputHeaders[i] = NULL;
			fDevSpec->fInputHeaders[i]->reserved_1 = total_size;
		  }
		  if (!fDevSpec->fInputHeaders[i])
			printf("BLegoConsumer::Start - create_area(%d) failed\n", total_size);
		}
		fInputPointer = 0;
		fDisableOutput = false;
	
		mStartTime = performance_time;

		// Start run thread
		int servicePrio = B_REAL_TIME_PRIORITY;
		fRunThreadKeepGoing = true;
		fRunThreadID = spawn_thread(RunThread, "Lego Producer Run Thread", servicePrio, this);
		fSchedLatency = estimate_max_scheduling_latency(fRunThreadID);
		resume_thread(fRunThreadID);
	
		mRunning = true;
	}
}


void
BLegoProducer::Stop(bigtime_t performance_time, bool immediate)
{
	//printf("BLegoProducer::Stop()\n");
	BMediaNode::Stop(performance_time, immediate);

	mLock.Lock();

	if (mRunning)
	{
		mStopTime = performance_time;
		if (fRunThreadID != B_BAD_THREAD_ID) {
			fRunThreadKeepGoing = false;
			mLock.Unlock();

			int32 st;
			wait_for_thread(fRunThreadID, &st);

			mLock.Lock();
			fRunThreadID = B_BAD_THREAD_ID;
		}

		fInputHeader = NULL;
		mRunning = false;
	}

	mLock.Unlock();
}

void
BLegoProducer::TimeWarp(bigtime_t real_time, bigtime_t performance_time)
{
	BMediaNode::TimeWarp(real_time, performance_time);
}


/*********************
	BBufferProducer
**********************/

status_t
BLegoProducer::FormatSuggestionRequested(media_type type, int32 quality, media_format * format)
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
BLegoProducer::FormatChangeRequested(const media_source & /*source*/,
				const media_destination & /*destination*/,
				media_format * /*io_format*/, int32 * /*out_change_count*/)
{
	return B_ERROR;
}

status_t
BLegoProducer::FormatProposal(const media_source & /*output*/, media_format * format)
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
	
		if (format->u.raw_audio.format == media_raw_audio_format::wildcard.format)
			suggestion.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
		else if (format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_SHORT
					&& format->u.raw_audio.format != media_raw_audio_format::B_AUDIO_FLOAT)	
			bad = true;
		else
		  suggestion.u.raw_audio.format = format->u.raw_audio.format;
	
		if (format->u.raw_audio.byte_order > media_raw_audio_format::wildcard.byte_order && 
			format->u.raw_audio.byte_order != fCaptureFormat.u.raw_audio.byte_order)
			bad = true;
	
		if (format->u.raw_audio.buffer_size > media_raw_audio_format::wildcard.buffer_size && 
			format->u.raw_audio.buffer_size != fCaptureFormat.u.raw_audio.buffer_size)
			bad = true;
	}

	*format = suggestion;
	return bad ? B_MEDIA_BAD_FORMAT : B_OK;
}


status_t
BLegoProducer::GetNextOutput(int32 * cookie, media_output * out_destination)
{
	if (*cookie)
		return B_ERROR;
	
	BAutolock lock(&mLock);
	out_destination->node = Node();
	out_destination->source = fCaptureSource;
	out_destination->destination = fCaptureDestination;
	out_destination->format = fCaptureFormat;
//	sprintf(out_destination->name, "BLegoProducer %d", *cookie);
	strcpy(out_destination->name, fName);
	*cookie = *cookie+1;
	
	return B_OK;
}


status_t
BLegoProducer::DisposeOutputCookie(int32 cookie)
{
	cookie = cookie; /* ignore cookie */
	
	return B_OK;
}


status_t
BLegoProducer::SetBufferGroup(const media_source &for_source, BBufferGroup * group)
{
	BAutolock lock(&mLock);
	if (for_source.port != fControlPort ||
		for_source.id != 0 || 
		fCaptureDestination == media_destination::null)
	{
		return B_MEDIA_BAD_SOURCE;
	}
	if (fBufferGroup != fDefaultBufferGroup)
	{
		delete fBufferGroup;
	}
	if (group == NULL)
	{
		fBufferGroup = fDefaultBufferGroup;
	}
	else
	{
		fBufferGroup = group;
	}
	
	return B_OK;
}

/*
	Method: SetVideoClipping
	
	We aren't a video node, so there isn't anything
	for us to do for this particular method.
*/

status_t
BLegoProducer::VideoClippingChanged(
	const media_source &/*for_source*/,
	int16 /*num_shorts*/,
	int16 * /*clip_data*/,
	const media_video_display_info & /*display*/,
	int32 * /*out_from_change_count*/)
{
	return B_ERROR;		/* this is not a video node... */
}


status_t
BLegoProducer::GetLatency(bigtime_t* out_latency)
{
  bigtime_t buf_latency = (bigtime_t) (fInputSize * fUsecPerByte);
  bigtime_t downstream_latency = 0;
  status_t err = BBufferProducer::GetLatency(&downstream_latency);
  if (err != B_OK)
	return err;
  *out_latency = buf_latency + downstream_latency + fSchedLatency;
  return B_OK;
}


status_t
BLegoProducer::PrepareToConnect(const media_source & what,
				const media_destination & where,
				media_format * format,
				media_source * out_source,
				char * out_name)
{
//	(void)BBufferProducer::PrepareToConnect(what, where, format, out_source, out_name);
	status_t err = FormatProposal(what, format);
	if (err != B_OK)
		return err;

	BAutolock lock(&mLock);
	if (what.port != fControlPort || what.id != 0)
		return B_MEDIA_BAD_SOURCE;
	ASSERT(fCaptureDestination == media_destination::null);
	if (where == media_destination::null)
		return B_MEDIA_BAD_DESTINATION;
	fCaptureDestination = where;

	media_source source(fControlPort, what.id);
	*out_source = source;
	strcpy(out_name, fName);

	return B_OK;
}


void
BLegoProducer::Connect(
		status_t error, 
		const media_source & /*source*/,
		const media_destination & destination,
		const media_format & /*format*/,
		char * out_name)
{
//	BBufferProducer::Connect(error, source, destination, format, out_name);

	BAutolock lock(&mLock);
	if (error != B_OK) {
		fCaptureDestination = media_destination::null;
		return;
	}

	fCaptureDestination = destination;
	strcpy(out_name, fName);
}


void
BLegoProducer::Disconnect(const media_source &what, const media_destination &where)
{
	ASSERT(!(what.port != fControlPort || what.id != 0));
	if (what.port != fControlPort || what.id != 0)
	{
		return;
	}
	ASSERT(fCaptureDestination == where);
	if (where == media_destination::null)
	{
		return;
	}
	BAutolock lock(&mLock);
	
	/* stop using the buffer group */
	if (fBufferGroup != fDefaultBufferGroup)
	{
		delete fBufferGroup;
		fBufferGroup = fDefaultBufferGroup;
	}
	
	/* break the connection */
	fCaptureDestination = media_destination::null;
}

void 
BLegoProducer::LateNoticeReceived(const media_source & /*what*/,
				bigtime_t /*how_much*/, bigtime_t /*performance_time*/)
{
}

void
BLegoProducer::EnableOutput(const media_source & what,
			bool enabled,
			int32 * change_tag)
{
	if (what.port != fControlPort || what.id)
		return;
	BAutolock lock(&mLock);
	fDisableOutput = !enabled;
	*change_tag = ++fChangeTag;
}


int32 BLegoProducer::RunThread(void *_castToProducer)
{
	BLegoProducer *producer = (BLegoProducer*) _castToProducer;
	while (producer->fRunThreadKeepGoing && producer->DoRun() == B_OK)
		;

	return 0;
}

int32 BLegoProducer::ServiceThread(void *_castToProducer)
{
	BLegoProducer *producer = (BLegoProducer*) _castToProducer;

	while (producer->fServiceThreadKeepGoing) {
		int32 code = 0;
		char message_buffer[B_MEDIA_MESSAGE_SIZE];
		status_t err = read_port(producer->ControlPort(), &code, message_buffer,
			sizeof(message_buffer));
		if (err == B_INTERRUPTED)
			continue;
	
		if (err < B_OK)
			break;
		
		producer->HandleMessage(code, message_buffer, err);
	}

	return 0;
}


status_t
BLegoProducer::DoRun()
{
	BBuffer *obuf = fBufferGroup->RequestBuffer(16);
	char* dst = (char*) obuf->Data();
	int32 osize = obuf->Size();
	int32 optr = 0;
	status_t err = B_OK;

	if (osize > fInputSize)
	  osize = fInputSize;

	while (fReadsPending < (fInputPointer ? N_INPUT_BUFFERS - 1 : N_INPUT_BUFFERS)) {
	  fInputHeader = fDevSpec->fInputHeaders[(fNextBuffer + fReadsPending) % N_INPUT_BUFFERS];
	  err = IOCTL(fd, SOUND_UNSAFE_READ, fInputHeader, fInputHeader->reserved_1);
	  if (err == B_OK)
		++fReadsPending;
	  else if (err != B_INTERRUPTED) {
		printf("BLegoProducer::DoRun - Result of UNSAFE READ %x\n", err);
		return err;
	  }
	}

	while (optr < osize) {
	  if (fInputPointer >= fInputSize) {
		if (fReadsPending < N_INPUT_BUFFERS) {
		  fInputHeader = fDevSpec->fInputHeaders[fNextBuffer];
		  do err = IOCTL(fd, SOUND_UNSAFE_READ, fInputHeader, fInputHeader->reserved_1);
		  while	(err == B_INTERRUPTED);
		  if (err == B_OK)
			++fReadsPending;
		  else if (err == B_RESOURCE_UNAVAILABLE)
			err = B_OK;
		  else
			printf("BLegoProducer::DoRun - Result of UNSAFE READ %x\n", err);
		  fNextBuffer = (fNextBuffer + 1) % N_INPUT_BUFFERS;
		}
		fInputPointer = 0;
	  }

	  if (fReadsPending >= N_INPUT_BUFFERS) {
		// wait for completion
		err = acquire_sem_etc(fReadCompletion, 1, B_TIMEOUT, DEFAULT_TIMEOUT);
		if (err == B_OK)
		  --fReadsPending;
		else {
		  printf("BLegoProducer::DoRun - Result of WAITING (0x%x)\n", err);
		  return B_OK;
		}
	  }

	  fInputHeader = fDevSpec->fInputHeaders[fNextBuffer];
	  fInputBuffer = (char*)(fInputHeader + 1);
	  fCaptureTime = fInputHeader->time + (bigtime_t) (fUsecPerByte * fInputPointer);
	  //fprintf(stderr,"<%Ld>\n",system_time()-fCaptureTime);
	  switch (fCaptureFormat.u.raw_audio.format) {
	    case media_raw_audio_format::B_AUDIO_FLOAT: {
		  size_t n = (size_t)(osize - optr) / sizeof(float);
		  if (n > (size_t)(fInputSize - fInputPointer) / sizeof(short))
			n = (size_t)(fInputSize - fInputPointer) / sizeof(short);
		  for (size_t i = 0; i < n; i++) {
			*(float*)(dst + optr) = (1.0 / 32768) * *(short*)(fInputBuffer + fInputPointer);
			fInputPointer += sizeof(short);
			optr += sizeof(float);
		  }
		  break;
		}
	    default: {
		  size_t n = osize - optr;
		  if (n > (size_t)(fInputSize - fInputPointer))
			n = (size_t)(fInputSize - fInputPointer);
		  memcpy(dst + optr, fInputBuffer + fInputPointer, n);
		  fInputPointer += n;
		  optr += n;
		  break;
		}
	  }
	}

	Flush(obuf, osize, fCaptureTime);
	return err;		// on error, bail out of thread
}

/*
	Method: Flush
	
	This method is responsible for sending the specified
	buffer out to all those who are attached.
*/

void 
BLegoProducer::Flush(BBuffer *aBuffer, int32 used, bigtime_t capture_time)
{
//printf("BLegoProducer::FLUSH() 0x%x\n", aBuffer);
	if (aBuffer == NULL)
		return;

	BAutolock lock(&mLock);

	if (fCaptureDestination == media_destination::null || fDisableOutput)
		aBuffer->Recycle();
	else {
		aBuffer->Header()->time_source = TimeSource()->ID();
		aBuffer->Header()->start_time = TimeSource()->PerformanceTimeFor(capture_time /* + downstream latency */);
		aBuffer->Header()->size_used = used;
		aBuffer->Header()->type = B_MEDIA_RAW_AUDIO;
		//printf("BLegoProducer::FLUSH() sending 0x%x\n", aBuffer);
		status_t err = SendBuffer(aBuffer, fCaptureDestination);
		if (err) {
			printf("BLegoProducer::FLUSH(): SendBuffer() returned 0x%x\n", err);
			aBuffer->Recycle();
		}
	}
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
BLegoProducer::ConstructControlWeb()
{
  platform_type platform = B_AT_CLONE_PLATFORM;
  system_info si;
  if (get_system_info(&si) == B_NO_ERROR)
	platform = si.platform_type;

  BParameterWeb* web = new BParameterWeb;
  BParameterGroup* grp = web->MakeGroup("Input");
  int null_id = BLP_NULL_PARAMS;

  if (fDevSpec->is_null_device) {
	grp->MakeNullParameter(1, B_MEDIA_RAW_AUDIO, "This device produces silence.", B_GENERIC);
	SetParameterWeb(web);
	return;
  }
  BNullParameter* cd_in = grp->MNP("CD", B_WEB_PHYSICAL_INPUT);
  BNullParameter* line_in = grp->MNP("Line In", B_WEB_PHYSICAL_INPUT);
  BNullParameter* mic_in = grp->MNP("Mic", B_WEB_PHYSICAL_INPUT);
  BNullParameter* boost_in = grp->MNP("Mic +20dB", B_WEB_PHYSICAL_INPUT);
  BNullParameter* adc = grp->MNP("ADC", B_WEB_ADC_CONVERTER);

  BDiscreteParameter* mux = grp->MDP(BLP_SOURCE, "Source", B_INPUT_MUX);
  mux->AddInput(cd_in);
  mux->AddInput(line_in);
  mux->AddInput(mic_in);
  mux->AddInput(boost_in);
  mux->MakeItemsFromInputs();

  BContinuousParameter* in_level = grp->MCP(BLP_ADC_LEVEL, B_MEDIA_RAW_AUDIO, "Level",
											B_GAIN, "", QUIET, LOUD, STEP(16));
  in_level->SetChannelCount(2);
  in_level->AddInput(mux);

  adc->AddInput(in_level);
  SetParameterWeb(web);
}

status_t
BLegoProducer::GetParameterValue(int32 id, bigtime_t* last_change,
								 void* value, size_t* ioSize)
{
  sem_id lock = fDevSpec->lock;
  sound_setup* setup = &fDevSpec->setup;
  float* float_val = (float*) value;
  int32* int_val = (int32*) value;
  size_t size = *ioSize;
  //printf("BLegoProducer::GetParameterValue - %d  Size: %d\n", id, *size);

  if (fDevSpec->is_null_device) return B_BAD_INDEX;

	LockParameterWeb();
	int32 paramCount = Web() ? Web()->CountParameters() : 0;
	UnlockParameterWeb();
  if (id < 0 || id >= paramCount)
	return B_BAD_VALUE;
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
  case BLP_SOURCE:
	if (setup->left.adc_source == aux1)
	  *int_val = 0;		// CD
	else if (setup->left.adc_source == mic)
	  *int_val = (setup->left.mic_gain_enable ? 3 : 2);
	else
	  *int_val = 1;		// Line in
	break;
  case BLP_ADC_LEVEL:
	if (size < 2 * sizeof(float))
	  err = B_BAD_VALUE;
	else {
	  float_val[0] = GetP(setup->left.adc_gain, 4);
	  float_val[1] = GetP(setup->right.adc_gain, 4);
	  *ioSize = 8;
	}
	break;
  }

  release_sem(lock);
  return err;
}

void 
BLegoProducer::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
  sem_id lock = fDevSpec->lock;
  sound_setup* setup = &fDevSpec->setup;
  const float* float_val = (float*) value;
  const int32* int_val = (int32*) value;

  //printf("BLegoProducer::GetParameterValue - %d  Size: %d\n", id, *ioSize);

  if (fDevSpec->is_null_device) return;

	LockParameterWeb();
	int32 paramCount = Web() ? Web()->CountParameters() : 0;
	UnlockParameterWeb();
  if (id < 0 || id >= paramCount)
	return;
  if (size < 4)
	return;
  if (acquire_sem(lock) < 0)
	return;

  if (fDataChangeTime < when)
	fDataChangeTime = when;
	
  switch(id) {
  case BLP_SOURCE: {
	enum adc_source source = line;
	if (*int_val == 0)
	  source = aux1;
	else if (*int_val == 2 || *int_val == 3)
	  source = mic;
	setup->left.adc_source = setup->right.adc_source = source;
	setup->left.mic_gain_enable = setup->right.mic_gain_enable = (*int_val == 3);
	break;
  }
  case BLP_ADC_LEVEL:
	if (size >= 2 * sizeof(float)) {
	  setup->left.adc_gain = SetP(float_val[0], 4);
	  setup->right.adc_gain = SetP(float_val[1], 4);
	}
	break;
  }

  IOCTL(fd, SOUND_SET_PARAMS, &fDevSpec->setup, sizeof(sound_setup));
  release_sem(lock);
}

status_t 
BLegoProducer::StartControlPanel(BMessenger * /*out_messenger*/)
{
	return B_ERROR;
}

void
BLegoProducer::MakeName()
{
	const char * str = fDevSpec->path;
	const char * strip = "/dev/audio/old/";
	if (!strncmp(str, strip, strlen(strip))) str += strlen(strip);
	if (strlen(str) > 50) str += strlen(str)-50;
	sprintf(fName, "Audio In %s", str);
}
