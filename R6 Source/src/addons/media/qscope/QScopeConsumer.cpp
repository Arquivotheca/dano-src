/* ++++++++++

   FILE:  LegoConsumer.h
   REVS:  $Revision: 1.13 $
   NAME:  William Adams
   DATE:  Mon Jun 29 18:55:07 PDT 1998

   Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.

+++++ */

#include "QScopeConsumer.h"
#include "QScope_Parameters.h"

#include <byteorder.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <MediaDefs.h>

#include "ParameterWeb.h"
#include "TimeSource.h"
#include "tr_debug.h"
#include "Buffer.h"
#include "MediaAddOn.h"
#include "QScopeWindow.h"
#include "QScopeSubscriber.h"

//#define TRACE printf
//#define DEBUG_DO_MESSAGE	1
//#define DEBUG_SERVICE_RUN	1
//#define DEBUG_ITERATE_FORMATS	1
//#define DEBUG_BUFFER_RECEIVED

#define DEFAULT_TIMEOUT 6000000L
#define CHUNK_SIZE 2048
#define PRE_LATENCY 3000
#define JITTER_LATENCY 2000


QScopeConsumer::QScopeConsumer(BMediaAddOn *addon)
	: BMediaNode("QScope Consumer"),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	fControlPort(-1),
	fControlPortThread(-1),
	fAddOn(addon),
	fBufferHandler(0),
	fScopeWindow(0)
{
	QScopeConsumer_Initialize();
}


QScopeConsumer::~QScopeConsumer()
{
	// Quit the service thread
}

void
QScopeConsumer::ConstructControlWeb()
{
	BParameterWeb *newWeb = new BParameterWeb();
	
	BParameterGroup *mainGroup = newWeb->MakeGroup("Main");

	BParameterGroup *inputGroup = mainGroup->MakeGroup("Input");
	BDiscreteParameter * theStream = mainGroup->MakeDiscreteParameter(CONTROL_STREAM, B_MEDIA_NO_TYPE, "Stream", B_GENERIC);
		theStream->AddItem(MSG_DAC_STREAM,"DAC");
		theStream->AddItem(MSG_ADC_STREAM,"ADC");
	BDiscreteParameter * theChannel = mainGroup->MakeDiscreteParameter(CONTROL_CHANNEL, B_MEDIA_NO_TYPE, "Channel", B_GENERIC);
		theChannel->AddItem(MSG_LEFT_CHANNEL,"Left");
		theChannel->AddItem(MSG_RIGHT_CHANNEL,"Right");
		theChannel->AddItem(MSG_STEREO_CHANNELS,"Stereo");

	BParameterGroup *timeGroup = mainGroup->MakeGroup("Time");
	BDiscreteParameter * theTimeDiv = mainGroup->MakeDiscreteParameter(CONTROL_TIMEDIV, B_MEDIA_NO_TYPE, "Time", B_GENERIC);
		theTimeDiv->AddItem(MSG_TIME_DIV_100us,"0.1ms");
		theTimeDiv->AddItem(MSG_TIME_DIV_200us,"0.2ms");
		theTimeDiv->AddItem(MSG_TIME_DIV_500us,"0.5ms");
		theTimeDiv->AddItem(MSG_TIME_DIV_1ms,"1ms");
		theTimeDiv->AddItem(MSG_TIME_DIV_2ms,"2ms");
		theTimeDiv->AddItem(MSG_TIME_DIV_5ms,"5ms");
		theTimeDiv->AddItem(MSG_TIME_DIV_10ms,"10ms");
	
	BParameterGroup *triggerGroup = mainGroup->MakeGroup("Trigger");
	BDiscreteParameter * theTriggerMode = mainGroup->MakeDiscreteParameter(CONTROL_TRIGGER, B_MEDIA_NO_TYPE, "Trigger Mode", B_GENERIC);
		theTriggerMode->AddItem(MSG_TRIGGER_OFF,"Off");
		theTriggerMode->AddItem(MSG_TRIGGER_LEVEL,"Level");
		theTriggerMode->AddItem(MSG_TRIGGER_PEAK,"Peak");
		theTriggerMode->AddItem(MSG_TRIGGER_LEFT,"Left");
		theTriggerMode->AddItem(MSG_TRIGGER_RIGHT,"Right");

	// Put a level slider here
	
	BDiscreteParameter * theSlope = mainGroup->MakeDiscreteParameter(CONTROL_SLOPE, B_MEDIA_NO_TYPE, "Slope", B_GENERIC);
		theChannel->AddItem(MSG_SLOPE_POS,"POS");
		theChannel->AddItem(MSG_SLOPE_NEG,"NEG");
	// Put a holdoff slider here

	// And lastly, the illumination control
	BDiscreteParameter * theIlluminator = mainGroup->MakeDiscreteParameter(CONTROL_ILLUMINATE, B_MEDIA_NO_TYPE, "Illumination", B_ENABLE);
		theIlluminator->AddItem(MSG_ILLUMINATION,"None");
		theIlluminator->AddItem(MSG_ILLUMINATION,"Illuminate");


	SetParameterWeb(newWeb);
}

void
QScopeConsumer::QScopeConsumer_Initialize()
{
	// Now setup our service port
	fControlPort = create_port(3, "QScopeConsumer service port");

	fAudioFormat.type = B_MEDIA_RAW_AUDIO;
	fAudioFormat.u.raw_audio.frame_rate = 44100.0;
	fAudioFormat.u.raw_audio.channel_count = 2;
	fAudioFormat.u.raw_audio.format = 0x2; // media_raw_audio_format::B_AUDIO_SHORT;
	fAudioFormat.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	fAudioFormat.u.raw_audio.buffer_size = CHUNK_SIZE;

	// Set the play and record semaphores
	fBufferPlayedSem = create_sem(0,"buffer_played");
	fBufferRecordedSem = create_sem(0,"buffer_recorded");
	fBufferReceivedSem = create_sem(0,"buffer_received");

	mLast = 0;
	mRunning = false;
	mStarting = false;
	mStopping = false;
	mStartTime = 0;		/* when to start in performance time */
	mStopTime = 0;		/* when to stop in performance time */

	
	fProducerConnections[0] = media_source::null;

	fInputConnection = media_source::null;

	fDestination.port = fControlPort;
	fDestination.id = 0;
	
	ConstructControlWeb();


	// Construct a window and get it up on the screen
	fScopeWindow = new QScopeWindow();
	fBufferHandler = fScopeWindow->Subscriber();
	
	// Create the thread that will service commands
	// coming off the control port.
	//int servicePrio = suggest_thread_priority(B_DEFAULT_MEDIA_PRIORITY, 0, 0, 0);
	int servicePrio = B_DISPLAY_PRIORITY;
	fControlPortThread = spawn_thread(QScopeConsumer::ServiceThreadEntry, "QScopeConsumer::ServiceThread", servicePrio, this);
	resume_thread(fControlPortThread);
}

BMediaAddOn*
QScopeConsumer::AddOn(long *) const
{	
	// Who instantiated you -- or NULL for app class
	return fAddOn;
}

port_id
QScopeConsumer::ControlPort() const
{
	//TRACE("QScopeConsumer::ControlPort: %d\n", fControlPort);
	return fControlPort;
}

status_t 
QScopeConsumer::HandleMessage(int32 code, const void *data, size_t size)
{
#ifdef DEBUG_DO_MESSAGE
	printf("QScopeConsumer::HandleMessage - BEGIN (0x%x)\n", code);
#endif

	status_t status = BMediaNode::HandleMessage(code, data, size);
	if (status != B_OK)
	{
#ifdef DEBUG_DO_MESSAGE
		printf("Status from BMediaNode::HandleMessage: 0x%x\n", status);
#endif
		status = BBufferConsumer::HandleMessage(code, data, size);
		
		if (status!=B_OK)
			status = BControllable::HandleMessage(code, data, size);
	}
	
	if (status != B_OK)
	{
		HandleBadMessage(code, data, size);
	}
	
#ifdef DEBUG_DO_MESSAGE
	printf("QScopeConsumer::HandleMessage - END\n");
#endif

	return status;
}

status_t
QScopeConsumer::FormatChanged(const media_source & producer,const media_destination & consumer, 
	int32 from_change_count, const media_format & format)
{
	return B_ERROR;
}


/*
	BBufferConsumer specific stuff
*/

status_t
QScopeConsumer::AcceptFormat(const media_destination &dest, media_format * format)
{
	printf("QScopeConsumer::FormatSuggestionRequested\n");

	if (format->type != B_MEDIA_NO_TYPE)
	{
		if (format->type != B_MEDIA_RAW_AUDIO)
			return B_MEDIA_BAD_FORMAT;
			
		if (format->u.raw_audio.frame_rate != fAudioFormat.u.raw_audio.frame_rate &&
				format->u.raw_audio.frame_rate != media_raw_audio_format::wildcard.frame_rate)
			return B_MEDIA_BAD_FORMAT;

		if (format->u.raw_audio.channel_count != fAudioFormat.u.raw_audio.channel_count &&
				format->u.raw_audio.channel_count != media_raw_audio_format::wildcard.channel_count)
			return B_MEDIA_BAD_FORMAT;

		if (format->u.raw_audio.format != fAudioFormat.u.raw_audio.format &&
				format->u.raw_audio.format != media_raw_audio_format::wildcard.format)
			return B_MEDIA_BAD_FORMAT;

		if (format->u.raw_audio.byte_order != fAudioFormat.u.raw_audio.byte_order &&
				format->u.raw_audio.byte_order != media_raw_audio_format::wildcard.byte_order)
			return B_MEDIA_BAD_FORMAT;

		if (format->u.raw_audio.buffer_size != fAudioFormat.u.raw_audio.buffer_size &&
				format->u.raw_audio.buffer_size != media_raw_audio_format::wildcard.buffer_size)
			return B_MEDIA_BAD_FORMAT;
	}
	
	*format = fAudioFormat;

	return B_OK;
}

status_t
QScopeConsumer::GetNextInput(int32 * cookie, media_input * out_consumer)
{
printf("QScopeConsumer::GetNextInput - BEGIN\n");
	if (*cookie == 0)
	{
		out_consumer->node = Node();

		out_consumer->destination = fDestination;

		out_consumer->source = fInputConnection;
		
		out_consumer->format = fAudioFormat;

		sprintf(out_consumer->name, "QScope Consumer %d", *cookie);
		
		*cookie = 1;

		return B_OK;
	}

	return B_MEDIA_BAD_DESTINATION;
}

void
QScopeConsumer::DisposeInputCookie(int32 cookie)
{
	// Don't do anything special since we didn't allocate
	// anything on the cookie's behalf.
}

void
QScopeConsumer::ProducerDataStatus(const media_destination &for_whom, int32 status, bigtime_t at_media_time)
{
	status = status;
	at_media_time = at_media_time;
}

status_t
QScopeConsumer::GetLatencyFor(const media_destination &,
	bigtime_t * out_latency, media_node_id * out_timesource)
{
	*out_timesource = TimeSource()->ID();
	*out_latency = 1000000LL * (CHUNK_SIZE / 4) / 44100;
}

status_t
QScopeConsumer::DirectPlayBuffer(BBuffer *buf)
{
	status_t result = B_OK;

	audio_buffer_header *header = (audio_buffer_header *)malloc(CHUNK_SIZE+sizeof(audio_buffer_header));
	header->buffer_number = 0;
	header->time = system_time();
	header->reserved_1 = CHUNK_SIZE+sizeof(audio_buffer_header);
	audio_buffer_header *ptr = header + 1;
	
	if (buf)
	{
/*
		// WAA 
		// This is where we need to send the buffer to the scope
		// so that it can display the data.
		if (fd >= 0)
		{
			media_header *buffHeader = buf->Header();
			mLast = buffHeader->start_time;
			
			//printf("QScopeConsumer::PlayRun - write and recycle (%Lx)\n", mLast);
			
			// Copy the buffer locally and recycle as
			// soon as possible
			memcpy(ptr, buf->Data(), buffHeader->size_used);
			buf->Recycle();

			//printf("QScopeConsumer::PlayRun - BUFFER: %d SIZE: 0%d\n", buf->ID(), buffHeader->size_used);
			
			// wait for completion
			result = acquire_sem(fWriteCompletion);
			//result = acquire_sem_etc(fWriteCompletion, 1, B_TIMEOUT, 500000);
			//printf("QScopeConsumer::PlayRun - Result of WAITING (0x%x)\n", result);

			 result = ioctl(fd, SOUND_UNSAFE_WRITE, header);
			//printf("QScopeConsumer::PlayRun - Result of UNSAFE WRITE (0x%x)\n", result);
		} else
*/
			buf->Recycle();
	} else
	{
		//printf("QScopeConsumer::PlayRun - Timed out, but no buffer to play\n");
	}

	return result;
}

void
QScopeConsumer::BufferReceived(BBuffer * buffer)
{
#ifdef DEBUG_BUFFER_RECEIVED
	printf("QScopeConsumer::BufferReceived() - BEGIN\n");
#endif

	fBufferHandler->ProcessBuffer((int16 *)buffer->Data(), buffer->Header()->size_used);
	buffer->Recycle();
}

status_t
QScopeConsumer::Connected(		/* be sure to call BBufferConsumer::Connected()! */
	const media_source &producer,	/* here's a good place to request buffer group usage */
	const media_destination &where,
	const media_format & with_format,
	media_input * out_input)
{
printf("QScopeConsumer::Connected\n");
	BBufferConsumer::Connected(producer, where, with_format, out_input);
	
	out_input->node = Node();
	out_input->source = producer;
	out_input->destination = fDestination;
	out_input->format = with_format;
	sprintf(out_input->name, "QScopeConsumer hookedup");
	return B_OK;
}


/*
	From BBufferConsumer
	
	Disconnected is called for a consumer when a connection
	is broken.
*/

void
QScopeConsumer::Disconnected(const media_source &producer, const media_destination &where)
{
	fProducerConnections[where.id] = media_source::null;
}


void
QScopeConsumer::ServiceRun()
{
	while (true)
	{
		// read message
		int32 code = 0;
		status_t err = read_port(fControlPort, &code, msg, sizeof(msg));
#ifdef DEBUG_SERVICE_RUN
		printf("QScopeConsumer::ServiceRun read_port returns %x\n", err);
#endif
		if (err < 0)
		{
			printf("QScopeConsumer::ServiceRun() - Unexpected error %x\n", err);
			if (err == B_INTERRUPTED)
				continue;
			break;
		}
		
		// dispatch message
		if (code == 0x60000000)
			break;
		
		status_t status;
#ifdef DEBUG_SERVICE_RUN
		printf("QScopeConsumer::ServiceRun - About to HandleMessage(0x%x)\n", code);
#endif
		status = HandleMessage(code, msg, sizeof(msg));
#ifdef DEBUG_SERVICE_RUN
		printf("QScopeConsumer::ServiceRun - DID HandleMessage(0x%x)\n", status);
#endif
	}
}

status_t
QScopeConsumer::ServiceThreadEntry(void * data)
{
	((QScopeConsumer *)data)->ServiceRun();
	
	return B_NO_ERROR;
}



void
QScopeConsumer::Start(bigtime_t performance_time)		/* be sure to call BMediaNode::Start()! */
{
	BMediaNode::Start(performance_time);

	if (!mRunning || mStopping)
	{
		mStarting = true;
		mStartTime = performance_time;
	}
}


void
QScopeConsumer::Stop(bigtime_t performance_time, bool immediate)
{
	BMediaNode::Stop(performance_time, immediate);

	if (mRunning || mStarting)
	{
		mStopping = true;
		mStopTime = performance_time;
	}
	
	// called from within loop
}

void
QScopeConsumer::TimeWarp(bigtime_t real_time, bigtime_t performance_time)
{
	BMediaNode::TimeWarp(real_time, performance_time);
	
}

status_t 
QScopeConsumer::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *ioSize)
{
	printf("QScopeConsumer::GetParameterValue - %d  Size: %d\n", id, *ioSize);
	
	*last_change = bigtime_t(0);
	
	switch(id)
	{
		case 1:
			*((int32 *)value) = int32(100);
			*ioSize = sizeof(int32);
		break;

		case 2:
			*((int32 *)value) = int32(200);
			*ioSize = sizeof(int32);
		break;

		case 3:
		default:
			*((int32 *)value) = int32(300);
			*ioSize = sizeof(int32);
		break;
		
	}
	
	return B_OK;
}

void 
QScopeConsumer::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
}

status_t 
QScopeConsumer::StartControlPanel(BMessenger * out_messenger)
{
	return B_ERROR;
}
