/* multi.cpp */
/* the multinode is the ultimate in modern technology */
/* it slices, it dices */
/* it has all the ins and outs */
/* for multi_audio drivers that is */
/* also know as drivers that publish in */
/* dev/audio/multi/... */

#include "multi.h"

#include "convert.h"

#include <scheduler.h>
#include <Debug.h>
#include <File.h>
#include <MediaAddOn.h>
#include <MediaDefs.h>
#include <ParameterWeb.h>
#include <stdio.h>
#include <TimedEventQueue.h>
#include <unistd.h>

/* these spit out various debuggin info */
/* they are listed in ascending order of annoyingness */
/* change (void)0 to PRINT(x) to enable your favorites */ 
#define TERROR(x) void(0)//PRINT(x)
#define LATE(x) void(0)//PRINT(x)
#define FUNCTION(x) void(0)//PRINT(x)
#define TEMP(x) void(0)//PRINT(x)
#define FORMATS(x) PRINT(x)
#define CONNECTION(x) void(0)//PRINT(x)
#define DESCRIPTION(x) void(0)//PRINT(x)
#define BUFFERINFO(x) (void)0//PRINT(x)
#define OPTIONS(x) (void)0//PRINT(x)
#define SETUP(x) (void)0//PRINT(x)
#define MIX(x) void(0)//PRINT(x)
#define WEB(x) (void)0//PRINT(x)
#define PARAMETER(x) PRINT(x)
#define ENABLE(x) (void)0//PRINT(x)
#define DATASTATUS(x) (void)0//PRINT(x)
#define GETNEXT(x) (void)0//PRINT(x)
#define HANDLE(x) (void)0//PRINT(x)
#define BUFFERREC(x) (void)0//PRINT(x)
#define SENDINFO(x) void(0)//PRINT(x)
#define MORESENDINFO(x) (void)0//PRINT(x)
#define TIMING(x) (void)0//PRINT(x)
#define PUBLISHSOMETIME(x) (void)0//PRINT(x)
#define PUBLISHTIME(x) (void)0//PRINT(x)
#define EXCHANGEINFO(x) (void)0//PRINT(x)

/* set this to one to create multi audio mix controls */
#if !defined(CREATE_MIXER_CONTROLS)
#define CREATE_MIXER_CONTROLS 0
#endif

/* set this to one to turn use timestamps in BufferReceived */
#if !defined(USE_PLAYBACK_TIMING)
#define USE_PLAYBACK_TIMING 1
#endif

/* set these to one to enable all channels all the time*/
#if !defined(ENABLE_ALL_CHANNELS)
#define ENABLE_ALL_CHANNELS 0
#endif

/* set this to 1 to normal inputs to outputs inside the node */
#define DEBUG_LOOPBACK 0

/* 10 little endians */
#if B_HOST_IS_BENDIAN
#define ENDIAN B_MEDIA_BIG_ENDIAN
#else
#define ENDIAN B_MEDIA_LITTLE_ENDIAN
#endif

/* parameters */
const int32 M_NULL = 0;
const int32 M_RATES = 10;
const int32 M_CVSR = 11;
const int32 M_FORMATS = 20;
const int32 M_BUFFER_SIZE = 25;
const int32 M_LOCK_SOURCES = 30;
const int32 M_TIMECODE_SOURCES = 40;
const int32 M_ENABLE_ALL_CHANNELS = 50;

/* default values */
const int32 OUTPUT_BUFFER_MULTIPLIER = 3;
const int32 DEFAULT_BUFFER_COUNT = 2;
const int32 MAX_BUFFER_SIZE = 8192;
const float DRIFT_STABILITY = 0.95;			/* averaging factor 0 to 1.0 */
const bigtime_t PUT_EM_IN_YOUR_MOUTH = 500LL;	/* microsecs */

/* BMultiNode */
BMultiNode::BMultiNode(const char *name,
						BMultiAudioAddon *addon,
						int32 flavor,
						int fd,
						multi_setup setup) :
	BMediaNode(name),
	BMediaEventLooper(),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BTimeSource(),
	BControllable(),
	mAddon(addon),
	mFlavorID(flavor),
	mDriver(fd),
	mSetup(setup),
	mTotalChannels(0),
	mIOChannels(0),
	mFirstOutputChannelID(0),
	mFirstInputChannelID(0),
	mBufferSize(setup.enable_all_channels),
	mConnectedOutputs(0),
	mConnectedInputs(0),
	mBufferCount(DEFAULT_BUFFER_COUNT),
	mRecordIndex(0),	
	mPlaybackIndex(0),
	mPlayedCount(0),
	mBufferInfoPlayedFramesCount(0),
	mDrift(1.0),
	mEventQ(EventQueue()),
	mRTQ(RealTimeQueue()),
	mOutputs(EndPoint::B_OUTPUT),
	mInputs(EndPoint::B_INPUT),
	mRecordFormat(),
	mPlaybackFormat(),
	mDescription(),
	mChannelInfo(NULL),
	mFormatInfo(),
	mBufferList(),
	mRecordBufferDescriptions(NULL),
	mPlaybackBufferDescriptions(NULL),
	mBufferInfo(),
	mChannelEnable(),
	mMixChannelInfo(),
	mMixControlInfo(),
	mMixValueInfo(),
	mMixConnectionInfo(),
	mNextExchangeTime(0),
	mRealStartTime(0),
	mPerfStartTime(0),
	mBufferDuration(0),
	mChocolateSaltyBallsFactor(PUT_EM_IN_YOUR_MOUTH)
{
	memset(&mBufferInfo, 0, sizeof(mBufferInfo));
	memset(&mChannelEnable, 0, sizeof(mChannelEnable));
	FUNCTION(("BMultiNode::BMultiNode\n"));
	AddNodeKind(B_PHYSICAL_INPUT);
	mRecordFormat.type = B_MEDIA_RAW_AUDIO;
	mRecordFormat.u.raw_audio.frame_rate = mSetup.frame_rate;
	mRecordFormat.u.raw_audio.channel_count = 1;
	mRecordFormat.u.raw_audio.format = mSetup.sample_format;
	mRecordFormat.u.raw_audio.byte_order = ENDIAN;
	mRecordFormat.u.raw_audio.buffer_size = mSetup.buffer_size;
	AddNodeKind(B_PHYSICAL_OUTPUT);
	mPlaybackFormat.type = B_MEDIA_RAW_AUDIO;
	mPlaybackFormat.u.raw_audio.frame_rate = mSetup.frame_rate;
	mPlaybackFormat.u.raw_audio.channel_count = 1;
	mPlaybackFormat.u.raw_audio.format = mSetup.sample_format;
	mPlaybackFormat.u.raw_audio.byte_order = ENDIAN;
	mPlaybackFormat.u.raw_audio.buffer_size = mSetup.buffer_size;

#if DEBUG
	if (USE_PLAYBACK_TIMING)
		OPTIONS(("BMultiNode::BMultiNode Using Playback Timing\n"));
#endif
}


BMultiNode::~BMultiNode()
{
	FUNCTION(("BMultiNode::~BMultiNode\n"));
	/* quit the MEL */
	Quit();
	/* in case we didn't already stop */
	StopDMA();
	/* tell addon it can close the driver */
	if (mAddon)
		mAddon->CloseDevice(mDriver);
	/* clean up */
	rtm_free(mChannelEnable.enable_bits);
	rtm_free(mChannelEnable.connectors);
	for (int i=0; i<mBufferCount; i++)
		rtm_free(mPlaybackBufferDescriptions[i]);
	rtm_free(mPlaybackBufferDescriptions);
	for (int i=0; i<mBufferCount; i++)
		rtm_free(mRecordBufferDescriptions[i]);
	rtm_free(mRecordBufferDescriptions);
	/* dug - delete buffer groups for each channel */
	/* dug - delete the channels */
}

/* from MediaNode */
/* from MediaNode */
/* from MediaNode */
//#pragma mark ---MediaNode---

BMediaAddOn *
BMultiNode::AddOn(int32 *internal_id) const
{
	FUNCTION(("BMultiNode::AddOn\n"));
	*internal_id = mFlavorID;
	return mAddon;
}

void 
BMultiNode::SetRunMode(run_mode mode)
{
	FUNCTION(("BMultiNode::SetRunMode mode:%x\n", mode));
	/* dug - we only run in recording mode */
}

void 
BMultiNode::Preroll()
{
	FUNCTION(("BMultiNode::Preroll\n"));
	/* dug - nothing yet */
}

void 
BMultiNode::SetTimeSource(BTimeSource *time_source)
{
	FUNCTION(("BMultiNode::SetTimeSource time_source:%x\n", time_source));
	/* dug - do I need to do anything? */
}

status_t 
BMultiNode::HandleMessage(int32 message, const void *data, size_t size)
{
	FUNCTION(("BMultiNode::HandleMessage 0x%x\n", message));
	return B_ERROR;
}

status_t 
BMultiNode::RequestCompleted(const media_request_info &info)
{
	FUNCTION(("BMultiNode::RequestCompleted\n"));
	return B_OK;
}

static int32
get_priority()
{
	return suggest_thread_priority(B_AUDIO_PLAYBACK | B_AUDIO_RECORDING, 300, 500, 200);
}

void 
BMultiNode::NodeRegistered()
{
	FUNCTION(("BMultiNode::NodeRegistered node %d port %d\n", Node().node, Node().port));
	GetDescription();
	SetupEnable();
	SetupFormats();
	SetupBuffers();
	SetupOutputs();
	SetupInputs();
	SetupMix();
	MakeParameterWeb();
	SetEventLatency(MAX(mFormatInfo.output_latency, mFormatInfo.input_latency));
	SetPriority(get_priority());
	PublishTime(0, RealTime(), 0);
	Run();
	FUNCTION(("BMultiNode::NodeRegistered scheduling latency %Ld\n", SchedulingLatency()));
}

/* from MediaEventLooper */
/* from MediaEventLooper */
/* from MediaEventLooper */
//#pragma mark ---MediaEventLooper---

void 
BMultiNode::HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent)
{
#if DEBUG
	if (lateness < 0)
		TERROR(("BMultiNode::HandleEvent - we're late by %Ld\n", lateness));
#endif
	switch(event->type) {
		case BTimedEventQueue::B_START:
		{
			HANDLE(("BMultiNode::HandleEvent - B_START at %Ld\n", event->event_time));
			if (RunState() != BMediaEventLooper::B_STOPPED) {
				TERROR(("BMultiNode::HandleEvent - B_START already started!\n"));
				break;
			}
			/* set the times */
			if (!realTimeEvent) {
				mPerfStartTime = event->event_time;
				mRealStartTime = TimeSource()->RealTimeFor(mPerfStartTime, 0);
			}
			else { 
				mRealStartTime = event->event_time;
			}
			StartDMA();
			break;
		}	
		case BTimedEventQueue::B_STOP:
			HANDLE(("BMultiNode::HandleEvent - B_STOP at %Ld\n", event->event_time));
			if (RunState() != BMediaEventLooper::B_STARTED) {
				TERROR(("BMultiNode::HandleEvent - B_STOP already stopped!\n"));
				break;
			}
			/* we finish stopping in MultiBufferExchange when RunState == B_STOPPED */
			break;
		case BTimedEventQueue::B_SEEK:
			HANDLE(("BMultiNode::HandleEvent - B_SEEK\n"));
			if (!realTimeEvent) {
				HANDLE(("BMultiNode::HandleEvent - B_SEEK ignoring performance time seek\n"));
				break;		
			}
			if (RunState() != BMediaEventLooper::B_STARTED) {
				HANDLE(("BMultiNode::HandleEvent - seeking time to %Ld\n", event->bigdata));
				/* this should get rounded to the nearest multiple of the buffer duration */
				mPerfStartTime = event->bigdata;
				break;
			}
			else {
				/* dug - do the right thing */
			}
			break;

		case BTimedEventQueue::B_WARP:
			HANDLE(("BMultiNode::HandleEvent - B_WARP\n"));
			break;

		case BTimedEventQueue::B_DATA_STATUS:
		{
			HANDLE(("BMultiNode::B_DATA_STATUS %Ld\n", event->event_time));
			/* dug - maybe flush buffers? */
			int32 channel = (int32)event->pointer;
			MultiChannel *ch = NULL;
			if (mInputs.EndPointAt(channel, (EndPoint **)&ch) < B_OK)
				return;
			ch->SetDataStatus(event->data);
			if (event->data == B_DATA_NOT_AVAILABLE) {
				/* dug - only write zeros beyond the event time */
				WriteZeros(ch);
			}
			break;
		}
		case BTimedEventQueue::B_HANDLE_BUFFER:
			BUFFERREC(("BMultiNode::B_HANDLE_BUFFER\n"));
			UseBuffer((BBuffer *)event->pointer);
			break;
			
		case BTimedEventQueue::B_HARDWARE:
			EXCHANGEINFO(("BMultiNode::HandleEvent - B_HARDWARE\n"));
		 	if (MultiBufferExchange() < B_OK) {
				/* dug - send node in distress */
				StopDMA();
				/* drift is zero when stopped */
				PublishTime(mPerfStartTime, mBufferInfo.played_real_time, 0.0);
				PUBLISHTIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
				PUBLISHSOMETIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
			}
			break;
		case BTimedEventQueue::B_PARAMETER:
			EXCHANGEINFO(("BMultiNode::HandleEvent - B_PARAMETER\n"));
			ApplySetParameter(event->data, event->user_data);
			break;
		
		default:
			HANDLE(("BMultiNode::HandleEvent - default\n"));
			break;
	}
}

/* from BufferConsumer */
/* from BufferConsumer */
/* from BufferConsumer */
//#pragma mark ---BufferConsumer---
status_t 
BMultiNode::AcceptFormat(const media_destination &dest, media_format *format)
{
	FORMATS(("BMultiNode::AcceptFormat channel %d\n", dest.id));
#if DEBUG
	char str[100];
	string_for_format(*format, str, 100);
	FORMATS(("BMultiNode::AcceptFormat with format %s\n", str));
#endif
	status_t err = B_OK;
	media_format fmt(mPlaybackFormat);

	/* handle multichannel connections */
	if (format->u.raw_audio.channel_count > 0) {
		fmt.u.raw_audio.channel_count = format->u.raw_audio.channel_count;
		fmt.u.raw_audio.buffer_size *= format->u.raw_audio.channel_count;
	}
	/* compensate for differing sample formats */
	if (format->u.raw_audio.format > 0 && format->u.raw_audio.format != fmt.u.raw_audio.format) {
		float ratio = (float)(format->u.raw_audio.format & 0xf)/(fmt.u.raw_audio.format & 0xf);
		fmt.u.raw_audio.buffer_size = (size_t)(fmt.u.raw_audio.buffer_size * ratio);
		fmt.u.raw_audio.format = format->u.raw_audio.format;
	}
	if (!format_is_compatible(*format, fmt)){
		err = B_MEDIA_BAD_FORMAT;
	}

	if(B_OK==CanDoFormatChange(&dest,&fmt)) // check if none of our downstream neighbours dislike the proposed change
		*format = fmt;
	else
		err=B_MEDIA_BAD_FORMAT;
	
#if DEBUG
	string_for_format(*format, str, 100);
	FORMATS(("BMultiNode::AcceptFormat returning format %s\n", str));
#endif
	return err;
}

bool BMultiNode::IsUpstreamNode(const media_destination *dest)
{
	// check if the provided destination is also a source for
	// the multinode, by comparing the port-id to the port-ids
	// of all the inputs (the port is supposedly unique across
	// nodes

	int32 cookie=0;
	media_input mediainput;
	while(GetNextInput(&cookie, &mediainput)==B_OK)
	{
		if(mediainput.source != media_source::null)
		{
			if(mediainput.source.port == dest->port)
				return true;
		}
	}
	return false;
}

status_t BMultiNode::CanDoFormatChange(const media_destination *dest, media_format *format)
{

	/* ask all downstream nodes if the format is OK for them */
	/* if a destination is provided, skip that node */
	status_t returnvalue=B_OK;
	
	int32 cookie=0;
	media_output mediaoutput;
	int32 previous_cookie=cookie;
	while(GetNextOutput(&cookie, &mediaoutput)==B_OK)
	{
		if(mediaoutput.destination == media_destination::null)
		{
//			printf("output %d not connected, skipping\n",previous_cookie);
		}
		else
		{
			if(dest && *dest == mediaoutput.destination)
			{
//				printf("skipping node %d, which is the source of the change-request\n",previous_cookie);
			}
			else if(IsUpstreamNode(&mediaoutput.destination))
			{
				// downstream node also happens to be the upstream node
//				printf("skipping node %d, which is also an upstream node\n",previous_cookie);
			}
			else
			{
				status_t status=ProposeFormatChange(format,mediaoutput.destination);
				if(B_OK==status)
				{
//					printf("destination %d agrees to format-proposal\n",previous_cookie);
				}
				else
				{
//					printf("destination %d disagrees with format-proposal: %08x (%s) port: %d, ID: %ld\n",previous_cookie,status,strerror(status), mediaoutput.destination.port, mediaoutput.destination.id);
					returnvalue=status; // return the reason for failing (most likely B_MEDIA_BAD_FORMAT)
					//break;  // fail: we have a node that doesn't want to play with us anymore if we change the format
				}
			}
		}
		previous_cookie=cookie;
	}
	return returnvalue;
}

status_t 
BMultiNode::GetNextInput(int32 *cookie, media_input *out_input)
{
	GETNEXT(("BMultiNode::GetNextInput %d\n", *cookie));
	if (*cookie == 0) {
		/* get ready */
	}
	else if (*cookie == -1) {
		return B_ERROR;
	}
	/* try to find a channel that matches cookie */
	MultiChannel *ch = NULL;
	if (mInputs.EndPointAt(*cookie, (EndPoint **)&ch) < B_OK) {
		TERROR(("Can't find input at %d\n", *cookie));
		return B_ERROR;
	}
	/* get the input */
	*out_input = ch->Input();
	
	/* set cookie to the next map entry */
	if (mInputs.NextEndPoint(*cookie, (EndPoint **)&ch) < B_OK)
		*cookie = -1;
	else
		*cookie = ch->ID();
	
#if DEBUG
	char str[100];
	string_for_format(out_input->format, str, 100);
	GETNEXT(("input %d format %s\n", *cookie, str));
#endif

	return B_OK;
}

void 
BMultiNode::DisposeInputCookie(int32 cookie)
{
	FUNCTION(("BMultiNode::DisposeInputCookie\n"));
	cookie = 0;
}

void 
BMultiNode::BufferReceived(BBuffer *buffer)
{
	ASSERT(buffer);
	BUFFERREC(("BMultiNode::BufferReceived for channel %d\n", buffer->Header()->destination));
	if (RunState() != BMediaEventLooper::B_STARTED)
		buffer->Recycle();
	else if (buffer->Header()->start_time > TimeSource()->PerformanceTimeFor(mNextExchangeTime)) {
		/* push buffer onto queue if it's early */
		media_timed_event event(buffer->Header()->start_time,
								BTimedEventQueue::B_HANDLE_BUFFER,
								(void *)buffer,
								BTimedEventQueue::B_RECYCLE_BUFFER);
		mEventQ->AddEvent(event);
	}
	else if (buffer->Header()->start_time < TimeSource()->PerformanceTimeFor(mNextExchangeTime-mBufferDuration)) {
		/* recycle it if it's late */
		LATE(("BMultiNode::BufferReceived late\n"));
		buffer->Recycle();
	}
	else
		UseBuffer(buffer);
}

void 
BMultiNode::ProducerDataStatus(const media_destination &for_whom, int32 status, bigtime_t at_performance_time)
{
	DATASTATUS(("BMultiNode::ProducerDataStatus channel %d\n", for_whom.id));
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS);
	event.pointer = (void *)for_whom.id;
	event.data = status;
	mEventQ->AddEvent(event);
}

status_t 
BMultiNode::GetLatencyFor(const media_destination &for_whom, bigtime_t *out_latency, media_node_id *out_timesource)
{
	/* dug - should I include SchedulingLatency? */
	/* the driver model induces one buffer of latency on the output */
	*out_latency = mFormatInfo.output_latency + mBufferDuration;
	*out_timesource = TimeSource()->ID();
	FUNCTION(("BMultiNode::GetLatencyFor channel %d returning %Ld\n", for_whom.id, *out_latency));
	return B_OK;
}

status_t 
BMultiNode::Connected(const media_source &producer, const media_destination &where, const media_format &with_format, media_input *out_input)
{
	FUNCTION(("BMultiNode::Connected trying %d\n", where.id));
#if DEBUG
	char str[100];
	string_for_format(with_format, str, 100);
	FORMATS(("BMultiNode::Connected with format %s\n", str));
#endif
	/* treat channel zero as a wildcard */
	MultiChannel *ch = NULL;
	if (where.id == 0)
	{
		/* find first available slot and connect there */
		if (mInputs.EndPointAt(1, (EndPoint **)&ch) < B_OK)
			return B_MEDIA_BAD_DESTINATION;
		while (ch){
			if ((ch->Source() == media_source::null) && !ch->Obscured())
				break;
middle_of_loop:
			if (mInputs.NextEndPoint(ch->ID(), (EndPoint **)&ch) < B_OK)
				return B_MEDIA_BAD_DESTINATION;
		}
		//	validate that there are N free channels starting from this channel
		int togo = with_format.u.raw_audio.channel_count-1;
		MultiChannel * nx = ch;
		status_t endErr = B_OK;
		while ((togo > 0) && ((endErr = mInputs.NextEndPoint(nx->ID(), (EndPoint **)&nx)) >= B_OK)) {
			if (nx->Obscured() || (nx->Source() != media_source::null)) {
				break;
			}
			togo--;
		}
		if (togo != 0) {
			if (endErr < B_OK) {
				return B_MEDIA_BAD_DESTINATION;
			}
			ch = nx;	//	else try something further down the list
			goto middle_of_loop;
		}
		CONNECTION(("BMultiNode::Connected Found input channel %ld\n", ch->ID()));
	}
	else
	{
		/* find the channel */
		if (mInputs.EndPointAt(where.id, (EndPoint **)&ch) < B_OK)
			return B_MEDIA_BAD_DESTINATION;
	}
	/* check if it's already taken */
	if ((ch->Source() != media_source::null) || ch->Obscured()) {
		CONNECTION(("BMultiNode::Connected channel %ld already taken\n", ch->ID()));
		return B_MEDIA_BAD_DESTINATION;
	}

	/* handle multichannel formats */
	media_format fmt(ch->Format());
	if (with_format.u.raw_audio.channel_count > 1) {
		fmt.u.raw_audio.channel_count = with_format.u.raw_audio.channel_count;
		fmt.u.raw_audio.buffer_size *= with_format.u.raw_audio.channel_count;
	}
	/* compensate for differing sample formats */
	if (with_format.u.raw_audio.format > 0 && with_format.u.raw_audio.format != fmt.u.raw_audio.format) {
		float ratio = (float)(with_format.u.raw_audio.format & 0xf)/(fmt.u.raw_audio.format & 0xf);
		fmt.u.raw_audio.buffer_size = (size_t)(fmt.u.raw_audio.buffer_size * ratio);
		fmt.u.raw_audio.format = with_format.u.raw_audio.format;
	}		
	/* confirm that the formats are compatible */
	if (!format_is_compatible(fmt, with_format)) {
		char str1[200], str2[200];
		string_for_format(fmt, str1, 200);
		string_for_format(with_format, str2, 200);
		CONNECTION(("BMultiNode::Connected bad formats: %s / %s\n", str1, str2));
		return B_MEDIA_BAD_FORMAT;
	}
	
	/* make sure enough channels are available */
	if (with_format.u.raw_audio.channel_count > 1)
	{
		MultiChannel *next = ch;
		for (uint32 i=1; i < with_format.u.raw_audio.channel_count; i++)
		{
			if (mInputs.NextEndPoint(next->ID(), (EndPoint **)&next) < B_OK
				|| next->Source() != media_source::null)
				return B_MEDIA_BAD_DESTINATION;
		}
		/* they are available, so obscure them now */
		next = ch;
		for (uint32 i=1; i < with_format.u.raw_audio.channel_count; i++){
			if (mInputs.NextEndPoint(next->ID(), (EndPoint **)&next) < B_OK)
				return B_MEDIA_BAD_DESTINATION;
			next->SetFormat(&with_format);
			next->SetSource(producer);
			next->SetObscured(ch->ID());
		}
	}
	/* enable the channels */
	WriteZeros(ch);
	EnableChannels(ch->ChannelID(), with_format.u.raw_audio.channel_count);
	
	/* fill in our channel */
	ch->SetFormat(&with_format);
	ch->SetSource(producer);
	ch->SetConnected(true);
	mConnectedInputs++;
	CONNECTION(("BMultiNode::Connected %d ConnectedInputs\n", mConnectedInputs));
	
	/* set output values */
	out_input->node = Node();
	out_input->format = ch->Format();
	out_input->source = ch->Source();
	out_input->destination = ch->Destination();
	strcpy(out_input->name, ch->Name());
	CONNECTION(("BMultiNode::Connected returning %d\n", ch->ID()));
	return B_OK;
}

void 
BMultiNode::Disconnected(const media_source &producer, const media_destination &consumer)
{
	FUNCTION(("BMultiNode::Disconnected\n"));
	/* find the channel */
	MultiChannel *ch = NULL;
	if (mInputs.EndPointAt(consumer.id, (EndPoint **)&ch) < B_OK)
		return;
	
	/* if there were channels obscured, unobscure them */
	if (ch->Format().u.raw_audio.channel_count > 1) {
		MultiChannel *next = ch;
		for (uint32 i=1; i < ch->Format().u.raw_audio.channel_count; i++) {
			if (mInputs.NextEndPoint(next->ID(), (EndPoint **)&next) < B_OK) {
				TERROR(("BMultiNode::Disconnected  Couldn't find all obscured channels\n"));
				break;
			}
			next->SetObscured(0);
			next->SetSource(media_source::null);
			next->SetFormat(&mPlaybackFormat);
		}
	}
	/* disable the channel */
	WriteZeros(ch);
	DisableChannels(ch->ChannelID(), ch->Format().u.raw_audio.channel_count);
	
	/* reset it's destination */
	ch->SetSource(media_source::null);
	ch->SetFormat(&mPlaybackFormat);
	ch->SetConnected(false);
	mConnectedInputs--;
	
	CONNECTION(("BMultiNode::Disconnected %d ConnectedInputs\n", mConnectedInputs));
	return;
}

status_t 
BMultiNode::FormatChanged(const media_source &producer, const media_destination &consumer, int32 change_tag, const media_format &format)
{
	FUNCTION(("BMultiNode::FormatChanged\n"));
	/* dug - support this */
	return B_ERROR;
}

/* from BufferProducer */
/* from BufferProducer */
/* from BufferProducer */
//#pragma mark ---BufferProducer---

status_t 
BMultiNode::FormatSuggestionRequested(media_type type, int32 quality, media_format *format)
{
	FUNCTION(("BMultiNode::FormatSuggestionRequested\n"));
	*format = mRecordFormat;
	if (type != B_MEDIA_RAW_AUDIO)
		return B_ERROR;
	return B_OK;
}

status_t 
BMultiNode::FormatProposal(const media_source &output, media_format *format)
{
	FUNCTION(("BMultiNode::FormatProposal\n"));
	status_t err = B_OK;

#if DEBUG
	char str[100];
	string_for_format(*format, str, 100);
	FORMATS(("BMultiNode::FormatProposal proposed: %s\n", str));
#endif
	
	media_format fmt(mRecordFormat);
	
#if DEBUG
	string_for_format(fmt, str, 100);
	FORMATS(("BMultiNode::FormatProposal channel: %s\n", str));
#endif
	/* handle multichannel connections */
	if (format->u.raw_audio.channel_count > 0) {
		fmt.u.raw_audio.channel_count = format->u.raw_audio.channel_count;
		fmt.u.raw_audio.buffer_size *= format->u.raw_audio.channel_count;
	}
	/* compensate for differing sample formats */
	if (format->u.raw_audio.format > 0 && format->u.raw_audio.format != fmt.u.raw_audio.format) {
		float ratio = (float)(format->u.raw_audio.format & 0xf)/(fmt.u.raw_audio.format & 0xf);
		fmt.u.raw_audio.buffer_size = (size_t)(fmt.u.raw_audio.buffer_size * ratio);
		fmt.u.raw_audio.format = format->u.raw_audio.format;
	}
	
	if (!format_is_compatible(*format, fmt)){
#if DEBUG
		char str1[100];
		char str2[100];
		string_for_format(fmt, str1, 100);
		string_for_format(*format, str2, 100);
		FORMATS(("FormatProposal failure; channel: %s; proposed: %s\n", str1, str2));
#endif
		err = B_MEDIA_BAD_FORMAT;
	}
	*format = fmt;
	
#if DEBUG
	string_for_format(*format, str, 100);
	FORMATS(("BMultiNode::FormatProposal returned:%s (err %s)\n", str, strerror(err)));
#endif

	return err;
}

status_t 
BMultiNode::FormatChangeRequested(const media_source &source, const media_destination &destination, media_format *io_format, int32 *_deprecated_)
{
	FUNCTION(("BMultiNode::FormatChangeRequested\n"));
	/* dug - support this */
	return B_ERROR;
}

status_t 
BMultiNode::GetNextOutput(int32 *cookie, media_output *out_output)
{
	GETNEXT(("BMultiNode::GetNextOutput %d\n", *cookie));
	if (*cookie == 0)
	{
		/* get ready */
	}
	else if (*cookie == -1)
	{
		return B_ERROR;
	}
	/* try to find a channel that matches cookie */
	MultiChannel *ch = NULL;
	if (mOutputs.EndPointAt(*cookie, (EndPoint **)&ch) < B_OK)
	{
		TERROR(("BMultiNode::GetNextOutput Can't find output at %d\n", *cookie));
		return B_ERROR;
	}
	/* get the output */
	*out_output = ch->Output();
	
#if DEBUG
	char str[100];
	string_for_format(out_output->format, str, 100);
	GETNEXT(("output %d format %s\n", *cookie, str));
#endif

	/* set cookie to the next map entry */
	if (mOutputs.NextEndPoint(*cookie, (EndPoint **)&ch) < B_OK)
		*cookie = -1;
	else
		*cookie = ch->ID();
	
	return B_OK;
}

status_t 
BMultiNode::DisposeOutputCookie(int32 cookie)
{
	FUNCTION(("BMultiNode::DisposeOutputCookie\n"));
	cookie = 0;
	return B_OK;
}

status_t 
BMultiNode::SetBufferGroup(const media_source &for_source, BBufferGroup *group)
{
	FUNCTION(("BMultiNode::SetBufferGroup\n"));

	MultiChannel *ch = NULL;
	if (mOutputs.EndPointAt(for_source.id, (EndPoint **)&ch) < B_OK)
		return B_ERROR;

	if (group == NULL){
		group = new BBufferGroup(ch->Format().u.raw_audio.buffer_size,
								OUTPUT_BUFFER_MULTIPLIER * mBufferCount /* * ch->Format().u.raw_audio.channel_count*/);
		status_t err = group->InitCheck();
		if (err < B_OK)
			TERROR(("BMultiNode::SetBufferGroup buffer group failed on InitCheck:\n %s\n", strerror(err)));
	}

	ch->SetBufferGroup(group, false);
	
	return B_OK;
}

status_t
BMultiNode::GetLatency(bigtime_t* latency)
{	
	// ODS 15-Dec-1999: Modified to include buffer & converter latencies.
	status_t err = BBufferProducer::GetLatency(latency);
	if (err == B_OK) {
		*latency += mFormatInfo.input_latency + mBufferDuration;
		FUNCTION(("BMultiNode::GetLatency returns %Ld\n", *latency));
	} else {
		FUNCTION(("BMultiNode::GetLatency failed: %s\n", strerror(err)));
	}
	return err;
}

status_t 
BMultiNode::PrepareToConnect(const media_source &what, const media_destination &where, media_format *format, media_source *out_source, char *out_name)
{
	FUNCTION(("BMultiNode::PrepareToConnect trying channel %d\n", what.id));
#if DEBUG
	char str[100];
	string_for_format(*format, str, 100);
	FORMATS(("BMultiNode::PrepareToConnect with format %s\n", str));
#endif
	/* treat channel zero as a wildcard */
	MultiChannel *ch = NULL;
	if (what.id == 0)
	{
		/* find first available slot and connect there */
		if (mOutputs.EndPointAt(1, (EndPoint **)&ch) < B_OK) {
			CONNECTION(("BMultiNode::PrepareToConnect EndPointAt(1) fails\n"));
			return B_MEDIA_BAD_SOURCE;
		}
		while (ch){
			if ((ch->ID() != 0) && (ch->Destination() == media_destination::null))
				break;
middle_of_loop:
			if (mOutputs.NextEndPoint(ch->ID(), (EndPoint **)&ch) < B_OK) {
				CONNECTION(("BMultiNode::PrepareToConnect: No more endpoints\n"));
				return B_MEDIA_BAD_SOURCE;
			}
		}
		//	Validate that there are N free channels starting from this channel
		//	we need to do this here as well as later, because "0" means "find a
		//	match anywhere". Suppose two channels were unused, then two connected,
		//	then four unused, and we were asked for a 4-channel connection. We
		//	should return the block of channels 5-8.
		int togo = format->u.raw_audio.channel_count-1;
		MultiChannel * nx = ch;
		status_t endErr = B_OK;
		while ((togo > 0) && ((endErr = mOutputs.NextEndPoint(nx->ID(), (EndPoint **)&nx)) >= B_OK)) {
			if (nx->Obscured() || (nx->Destination() != media_destination::null)) {
				break;
			}
			togo--;
		}
		if (togo != 0) {
			if (endErr < B_OK) {
				return B_MEDIA_BAD_SOURCE;
			}
			ch = nx;	//	else try something further down the list
			goto middle_of_loop;
		}
	}
	else
	{
		/* find the channel */
		if (mOutputs.EndPointAt(what.id, (EndPoint **)&ch) < B_OK) {
			CONNECTION(("BMultiNode::PrepareToConnect endpoint %ld is bad\n", what.id));
			return B_MEDIA_BAD_SOURCE;
		}
	}
	CONNECTION(("BMultiNode::PrepareToConnect ch->Source().id is %ld\n", ch->Source().id));
	/* check if it's already taken */
	if (ch->Destination() != media_destination::null) {
		CONNECTION(("BMultiNode::PrepareToConnect endpoint %ld is already connected\n", what.id));
		return B_MEDIA_BAD_SOURCE;
	}

	media_format fmt = ch->Format();

	/* handle multichannel connections */
	if (format->u.raw_audio.channel_count > 0) {
		fmt.u.raw_audio.channel_count = format->u.raw_audio.channel_count;
		fmt.u.raw_audio.buffer_size *= format->u.raw_audio.channel_count;
	}
	/* compensate for differing sample formats */
	if (format->u.raw_audio.format > 0 && format->u.raw_audio.format != fmt.u.raw_audio.format) {
		float ratio = (float)(format->u.raw_audio.format & 0xf)/(fmt.u.raw_audio.format & 0xf);
		fmt.u.raw_audio.buffer_size = (size_t)(fmt.u.raw_audio.buffer_size * ratio);
		fmt.u.raw_audio.format = format->u.raw_audio.format;
	}
	/* confirm that the formats are compatible */
	if (!format_is_compatible(fmt, *format)) {
		char str1[100];
		char str2[100];
		string_for_format(fmt, str1, 100);
		string_for_format(*format, str2, 100);
		CONNECTION(("BMultiNode::PrepareToConnect formats are incompatible: %s / %s\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	*format = fmt;
		
	/* make sure enough channels are available */
	if (format->u.raw_audio.channel_count > 1)
	{
		MultiChannel *next = ch;
		for (uint32 i=1; i < format->u.raw_audio.channel_count; i++) {
			if (mOutputs.NextEndPoint(next->ID(), (EndPoint **)&next) < B_OK
				|| next->Destination() != media_destination::null) {
				CONNECTION(("BMultiNode::PrepareToConnect can't allocate %ld channels\n", format->u.raw_audio.channel_count));
				return B_MEDIA_BAD_SOURCE;
			}
		}
		/* they are available, so obscure them now */
		next = ch;
		for (uint32 i=1; i < format->u.raw_audio.channel_count; i++){
			if (mOutputs.NextEndPoint(next->ID(), (EndPoint **)&next) < B_OK) {
				CONNECTION(("BMultiNode::PrepareToConnect iteration error when obscuring channels.\n"));
				return B_MEDIA_BAD_SOURCE;
			}
			next->SetFormat(format);
			next->SetDestination(where);
			next->SetObscured(ch->ID());
		}
	}
	
	/* fill in our channel */
	ch->SetFormat(format);
	ch->SetDestination(where);
	
	/* set output values */
	*out_source = ch->Source();
	strcpy(out_name, ch->Name());
	CONNECTION(("BMultiNode::PrepareToConnect returning connection source %ld:%ld name '%s'\n", out_source->id, out_source->port, out_name));
	FUNCTION(("BMultiNode::PrepareToConnect returning channel %d\n", ch->ID()));
	CONNECTION(("BMultiNode::PrepareToConnect %d ConnectedOutputs\n", mConnectedOutputs));
	return B_OK;
}

void 
BMultiNode::Connect(status_t error, const media_source &source, const media_destination &destination, const media_format &format, char *io_name)
{
	FUNCTION(("BMultiNode::Connect on channel %d\n", source.id));
#if DEBUG
	char str[100];
	string_for_format(format, str, 100);
	FORMATS(("BMultiNode::Connect with format %s\n", str));
#endif
	/* find the channel */
	MultiChannel *ch = NULL;
	if (mOutputs.EndPointAt(source.id, (EndPoint **)&ch) < B_OK)
		return;

	if (error < B_OK)
	{
		CONNECTION(("BMultiNode::Connect connection failed\n"));
		/* if the connection failed, reset it's destination */
		ch->SetDestination(media_destination::null);
		ch->SetFormat(&format);
		/* if there were channels obscured, unobscure them */
		if (format.u.raw_audio.channel_count > 1)
		{
			MultiChannel *next = ch;
			for (uint32 i=1; i < format.u.raw_audio.channel_count; i++){
				if (mOutputs.NextEndPoint(next->ID(), (EndPoint **)&next) < B_OK) {
					TERROR(("BMultiNode::Connect  Couldn't find all obscured channels\n"));
					return;
				}
				next->SetObscured(0);
				next->SetDestination(media_destination::null);
				next->SetFormat(&format);
			}
		}
		return;
	}
	
	/* create the buffers for output */
	BBufferGroup *group = new BBufferGroup(format.u.raw_audio.buffer_size,
										OUTPUT_BUFFER_MULTIPLIER * mBufferCount /* * format.u.raw_audio.channel_count */);
	status_t err = group->InitCheck();
	if (err < B_OK)
		TERROR(("BMultiNode::Connect buffer group failed on InitCheck:\n %s\n", strerror(err)));
	
	/* enable the channel */
	EnableChannels(ch->ChannelID(), format.u.raw_audio.channel_count);
	
	/* fill in our channel with the finalized values */
	ch->SetBufferGroup(group, false);
	ch->SetFormat(&format);
	ch->SetDestination(destination);
	ch->SetOutputEnabled(true);
	ch->SetConnected(true);
	
	mConnectedOutputs++;
	CONNECTION(("BMultiNode::Connect %d ConnectedOutputs\n", mConnectedOutputs));
	
	/* set output values */
	strcpy(io_name, ch->Name());
}

void 
BMultiNode::Disconnect(const media_source &source, const media_destination &dest)
{
	FUNCTION(("BMultiNode::Disconnect\n"));
	/* find the channel */
	MultiChannel *ch = NULL;
	if (mOutputs.EndPointAt(source.id, (EndPoint **)&ch) < B_OK)
		return;

	/* if there were channels obscured, unobscure them */
	if (ch->Format().u.raw_audio.channel_count > 1)
	{
		MultiChannel *next = ch;
		for (uint32 i=1; i < ch->Format().u.raw_audio.channel_count; i++){
			if (mOutputs.NextEndPoint(next->ID(), (EndPoint **)&next) < B_OK){
				TERROR(("BMultiNode::Disconnect  Couldn't find all obscured channels\n"));
				break;
			}
			/* reset it's destination */
			next->SetDestination(media_destination::null);
			next->SetObscured(0);
			next->SetFormat(&mRecordFormat);
		}
	}
	/* disable the channel */
	DisableChannels(ch->ChannelID(), ch->Format().u.raw_audio.channel_count);
	
	/* reset it's destination */
	ch->SetDestination(media_destination::null);
	ch->SetFormat(&mRecordFormat);
	ch->SetBufferGroup(NULL, false);
	ch->SetOutputEnabled(false);
	ch->SetConnected(false);
	mConnectedOutputs--;
	CONNECTION(("BMultiNode::Disconnect done %d ConnectedOutputs\n", mConnectedOutputs));
}

void 
BMultiNode::LateNoticeReceived(const media_source &what, bigtime_t how_much, bigtime_t performance_time)
{
	FUNCTION(("BMultiNode::LateNoticeReceived\n"));
	/* dug - Yer not the boss of me! */
}

void 
BMultiNode::EnableOutput(const media_source &what, bool enabled, int32 *_deprecated_)
{
	FUNCTION(("BMultiNode::EnableOutput(%d, %s)\n", what.id, enabled ? "true" : "false"));
	MultiChannel *ch = NULL;
	if (mOutputs.EndPointAt(what.id, (EndPoint **)&ch) < B_OK)
		return;
	else
		ch->SetOutputEnabled(enabled);

}

/* from TimeSource */
/* from TimeSource */
/* from TimeSource */
//#pragma mark ---TimeSource---

status_t 
BMultiNode::TimeSourceOp(const time_source_op_info &op, void *_reserved)
{
	FUNCTION(("BMultiNode::TimeSourceOp\n"));
	media_timed_event event;
	switch (op.op)
	{
		case B_TIMESOURCE_START:
			FUNCTION(("BMultiNode::TimeSourceOp B_TIMESOURCE_START: %Ld (%Ld)\n", op.real_time, system_time()));
			event.event_time = op.real_time;
			event.type = BTimedEventQueue::B_START;
			break;
		case B_TIMESOURCE_STOP:
			FUNCTION(("BMultiNode::TimeSourceOp B_TIMESOURCE_STOP: %Ld\n", op.real_time));
			event.event_time = op.real_time;
			event.type = BTimedEventQueue::B_STOP;
			break;
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			FUNCTION(("BMultiNode::TimeSourceOp B_TIMESOURCE_STOP_IMMEDIATELY\n"));
			/* add an event so that MediaEventLooper is aware of the state */
			event.event_time = system_time();
			event.type = BTimedEventQueue::B_STOP;
			StopDMA();
			/* drift is zero when stopped */
			PublishTime(mPerfStartTime, mBufferInfo.played_real_time, 0.0);
			PUBLISHTIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
			PUBLISHSOMETIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
			break;
		case B_TIMESOURCE_SEEK:
			FUNCTION(("BMultiNode::TimeSourceOp B_TIMESOURCE_SEEK: perf %Ld at real %Ld\n",
						op.performance_time, op.real_time));
			event.event_time = op.real_time;
			event.bigdata = op.performance_time;
			event.type = BTimedEventQueue::B_SEEK;
			break;
		default:
			TERROR(("BMultiNode::TimeSourceOp I dunno this one: %d\n", op.op));
			return B_ERROR;
	}
	mRTQ->AddEvent(event);
	return B_OK;
}

/* from Controllable */
//#pragma mark ---Controllable---

status_t 
BMultiNode::GetParameterValue(int32 id, bigtime_t *last_change, void *value, size_t *ioSize)
{
	FUNCTION(("BMultiNode::GetParameterValue\n"));
	status_t err = B_OK;
	if (*ioSize < sizeof(int32))
		err = B_ERROR;
	else
		switch (id) {
			case M_NULL:
				err = B_ERROR;
			break;
			case M_RATES:
				*(uint32 *)value = mFormatInfo.input.rate;
			break;
			case M_CVSR:
				*(float *)value = mFormatInfo.input.cvsr / 1000.0;
			break;
			case M_FORMATS:
				*(int32 *)value = mFormatInfo.input.format;
			break;
			case M_BUFFER_SIZE:
				*(int32 *)value = mBufferSize;
			break;
			case M_LOCK_SOURCES:
				*(int32 *)value = mChannelEnable.lock_source;
			break;
			case M_TIMECODE_SOURCES:
				*(int32 *)value = mChannelEnable.timecode_source;
			break;
			case M_ENABLE_ALL_CHANNELS:
				*(bool *)value = mSetup.enable_all_channels;
			break;
			default:
				err = B_ERROR;
			break;
		}
	*ioSize = sizeof(int32);
	return err;
}

void 
BMultiNode::SetParameterValue(int32 id, bigtime_t when, const void *value, size_t size)
{
	//FUNCTION(("BMultiNode::SetParameterValue\n"));
//	printf("BMultiNode::SetParameterValue\n");
	ASSERT(size <= sizeof(int32));	
	if (when <= TimeSource()->Now())
	{
//		printf("applying now\n");
		ApplySetParameter(id, value);
	}
	else {
//		printf("applying at %Ld (+%Ld)\n",when,when-TimeSource()->Now());
		/* dug - if it's in the future, push it on the Q */
		media_timed_event event;
		event.pointer = (void *)event.user_data;
		event.event_time = when;
		event.data = id;
		event.type=BTimedEventQueue::B_PARAMETER;
		if (M_CVSR == id) {
			float *rate = (float *)(event.user_data);
			*rate = *(float *)value;
			PARAMETER(("BMultiNode::SetParameterValue - rate %f\n", *rate));
		} else {
			int32 *info = (int32 *)(event.user_data);
			*info = *(int32 *)value;
			PARAMETER(("BMultiNode::SetParameterValue - value %ld\n", *info));
		}
		mEventQ->AddEvent(event);
	}
}

/* from MultiNode */
/* from MultiNode */
/* from MultiNode */
//#pragma mark ---MultiNode---

void 
BMultiNode::ApplySetParameter(int32 parameter, const void *value)
{
	FUNCTION(("BMultiNode::ApplySetParameter\n"));
//	printf("BMultiNode::ApplySetParameter\n");
	switch (parameter) {
		case M_NULL:
			break;
		case M_RATES:
			PARAMETER(("BMultiNode::ApplySetParameter - M_RATES 0x%x\n", *(uint32 *)value));
			mFormatInfo.output.rate = *(uint32 *)value;
			mFormatInfo.input.rate = mFormatInfo.output.rate;
			mFormatInfo.output.cvsr = convert_multi_rate_to_media_rate(mFormatInfo.output.rate, mFormatInfo.output.cvsr);
			mFormatInfo.input.cvsr = mFormatInfo.output.cvsr;
			SetGlobalFormat();
			BroadcastNewParameterValue(TimeSource()->Now(), parameter, (void*)value, sizeof(int32));
			break;
		case M_CVSR:
			PARAMETER(("BMultiNode::ApplySetParameter - M_CVSR %f\n", *(float *)value));
			mFormatInfo.output.cvsr = *(float *)value * 1000.0;
			mFormatInfo.input.cvsr = mFormatInfo.output.cvsr;
			mFormatInfo.output.rate = convert_media_rate_to_multi_rate(mFormatInfo.output.cvsr);
			mFormatInfo.input.rate = mFormatInfo.output.rate;
			SetGlobalFormat();
			break;
		case M_FORMATS:
			PARAMETER(("BMultiNode::ApplySetParameter - M_FORMATS 0x%x\n", *(uint32 *)value));
			mFormatInfo.output.format = *(uint32 *)value;
			mFormatInfo.input.format = *(uint32 *)value;
			SetGlobalFormat();
			break;
		case M_BUFFER_SIZE:
			PARAMETER(("BMultiNode::ApplySetParameter - M_BUFFER_SIZE %ld\n", *(int32 *)value));
			mSetup.buffer_size = *(int32 *)value;
			/* dug - need to stop whole caboose here */
			//SetupBuffers();
			/* dug - start up again if previousely running */
			mAddon->SetupChanged(mDriver, mSetup);
			break;
		case M_LOCK_SOURCES:
			if (ioctl(mDriver, B_MULTI_GET_ENABLED_CHANNELS, &mChannelEnable, 0) < 0)
				TERROR(("BMultiNode::ApplySetParameter: B_MULTI_GET_ENABLED_CHANNELS failed\n"));
			if (mChannelEnable.lock_source != *(uint32 *)value) {
				mChannelEnable.lock_source = *(uint32 *)value;
				/* dug - there should be a parameter to choose lock_data */
				mChannelEnable.lock_data = 0;
				mSetup.lock_source = mChannelEnable.lock_source;
				mSetup.lock_data = mChannelEnable.lock_data;
				mAddon->SetupChanged(mDriver, mSetup);
				SetEnabledChannels();
			}
			break;
		case M_TIMECODE_SOURCES:
			if (ioctl(mDriver, B_MULTI_GET_ENABLED_CHANNELS, &mChannelEnable, 0) < 0)
				TERROR(("BMultiNode::ApplySetParameter: B_MULTI_GET_ENABLED_CHANNELS failed\n"));
			if (mChannelEnable.timecode_source != *(uint32 *)value) {
				mChannelEnable.timecode_source = *(uint32 *)value;
				mSetup.timecode_source = mChannelEnable.timecode_source;
				mAddon->SetupChanged(mDriver, mSetup);
				SetEnabledChannels();
			}
			break;
		case M_ENABLE_ALL_CHANNELS:
			if (mSetup.enable_all_channels != *(bool *)value) {
				mSetup.enable_all_channels = *(bool *)value;
				mAddon->SetupChanged(mDriver, mSetup);
				if (mSetup.enable_all_channels)
					EnableAllChannels();
				else
					DisableAllChannels();
			}
			break;
		default:
			break;		
	}
}

void 
BMultiNode::MakeParameterWeb()
{
	FUNCTION(("BMultiNode::MakeParameterWeb\n"));
	uint32 mask = 0x1;
	char name[64];
	BParameterWeb *web = new BParameterWeb();
	
	/* format group */
	BParameterGroup *setup = web->MakeGroup("Setup");
	BParameterGroup *format = setup->MakeGroup("Format");
	format->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "Format", B_GENERIC);
	/* format selector */
	WEB(("BMultiNode::MakeParameterWeb creating format selector\n"));
	BDiscreteParameter *formats = format->MakeDiscreteParameter(M_FORMATS, B_MEDIA_RAW_AUDIO, "Sample Format", B_GENERIC); //dug - B_AUDIO_FORMAT);
	while (mask < B_FMT_IS_GLOBAL) {
		if (mDescription.input_formats & mask) {
			string_for_multi_format(mask, name);
			WEB(("BMultiNode::MakeParameterWeb adding item -%s- to format rate selector\n", name));
			formats->AddItem(mask, name);
		}
		mask <<= 1;
	}
	mask = 0x1;
	/* frame rate selector */
	WEB(("BMultiNode::MakeParameterWeb creating frame rate selector\n"));	
	BDiscreteParameter *rates = format->MakeDiscreteParameter(M_RATES, B_MEDIA_RAW_AUDIO, "Frame Rate", B_FRAME_RATE);
	while (mask < B_SR_CVSR) {
		if (mDescription.input_rates & mask) {
			sprintf(name, "%.3g kHz", convert_multi_rate_to_media_rate(mask, 0)/1000.0);
			WEB(("BMultiNode::MakeParameterWeb adding item -%s- to frame rate selector\n", name));
			rates->AddItem(mask, name);
		}
		mask <<= 1;
	}
	if (mDescription.input_rates & B_SR_CVSR) {
		sprintf(name, "CVSR");
		WEB(("BMultiNode::MakeParameterWeb adding item -%s- to frame rate selector\n", name));
		rates->AddItem(B_SR_CVSR, name);
		/* cvsr slider */
		format->MakeContinuousParameter(M_CVSR, B_MEDIA_RAW_AUDIO, "CVSR", B_FRAME_RATE, "kHz", mDescription.min_cvsr_rate/1000.0, mDescription.max_cvsr_rate/1000.0, 0.1);
	}
	mask = 0x1;
	/* buffer size selector */
	WEB(("BMultiNode::MakeParameterWeb creating frame rate selector\n"));	
	BDiscreteParameter *sizes = setup->MakeDiscreteParameter(M_BUFFER_SIZE, B_MEDIA_RAW_AUDIO, "Default Buffer Size", B_GENERIC);
	for (int32 size = 64; size <= MAX_BUFFER_SIZE;) {
		sprintf(name, "%ld frames : %.3g ms at %.3g kHz", size, (float)size*1000.0/mSetup.frame_rate, mSetup.frame_rate/1000.0);
		WEB(("BMultiNode::MakeParameterWeb adding item -%s- to buffer size selector\n", name));
		sizes->AddItem(size, name);
		size *= 2;
	}

	/* sync group */
	if (mDescription.lock_sources > 0 || mDescription.timecode_sources > 0) {
		BParameterGroup *sync = setup->MakeGroup("Synchronization");
		sync->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "Synchronization", B_GENERIC);
		/* lock sources */
		if (mDescription.lock_sources > 0) {
			WEB(("BMultiNode::MakeParameterWeb creating lock source selector\n"));
			BDiscreteParameter *lock_sources = sync->MakeDiscreteParameter(M_LOCK_SOURCES, B_MEDIA_RAW_AUDIO, "Lock Source", B_GENERIC);
			while (mask <= B_MULTI_LOCK_SPDIF) {
				if (mDescription.lock_sources & mask) {
					string_for_multi_lock_sources(mask, name, mChannelEnable.lock_data);
					WEB(("BMultiNode::MakeParameterWeb adding item -%s- to lock source selector\n", name));
					lock_sources->AddItem(mask, name);
				}
				mask <<= 1;
			}
			mask = 0x1;
		}
		/* timecode sources */
		if (mDescription.timecode_sources > 0) {
			WEB(("BMultiNode::MakeParameterWeb creating timecode source selector\n"));
			BDiscreteParameter *timecode_sources = sync->MakeDiscreteParameter(M_TIMECODE_SOURCES, B_MEDIA_RAW_AUDIO, "Timecode Source", B_GENERIC);
			while (mask <= B_MULTI_TIMECODE_FIREWIRE) {
				if (mDescription.timecode_sources & mask) {
					string_for_multi_timecode_sources(mask, name);
					WEB(("BMultiNode::MakeParameterWeb adding item -%s- to timecode source selector\n", name));
					timecode_sources->AddItem(mask, name);
				}
				mask <<= 1;
			}
		}
	}

#if CREATE_MIXER_CONTROLS
	/* mixer */
	BParameterGroup *mixer = web->MakeGroup("Mixer");
	for (int32 i=0; i<mTotalChannels; i++) {
		mMixChannelInfo.info_size = sizeof(multi_mix_channel_info);
		mMixChannelInfo.channel_count = 1;
		int32 channel = i; 
		mMixChannelInfo.channels = &channel;
		mMixChannelInfo.max_count = 0;
		mMixChannelInfo.controls = NULL;					
		/* list mix channels */
		status_t err = ioctl(mDriver, B_MULTI_LIST_MIX_CHANNELS, &mMixChannelInfo, 0);
		if (B_OK != err) {
			MIX(("Failed on B_MULTI_LIST_MIX_CHANNELS \n"));
			continue;
		}
		/* find out how many there are and make room */
		if (mMixChannelInfo.actual_count == 0)
			continue;
		/* create a group for this channel */
		BParameterGroup *ch = mixer->MakeGroup("Channel");	
		char label[256];
		sprintf(label, "Channel %ld", i);
		ch->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);
		mMixChannelInfo.max_count = mMixChannelInfo.actual_count;
		int32 *controls = (int32 *)malloc(sizeof(int32) * mMixChannelInfo.actual_count);
		void * to_free = controls;
		if (controls == NULL) {
			break;
		}
		mMixChannelInfo.controls = &controls;
		/* get them all */
		err = ioctl(mDriver, B_MULTI_LIST_MIX_CHANNELS, &mMixChannelInfo ,0);
		if (B_OK != err) {
			MIX(("Failed on B_MULTI_LIST_MIX_CHANNELS \n"));
			free(controls);
			break; /* for loop */
		}
		for (int32 tmp = 0; tmp < mMixChannelInfo.actual_count; tmp++) {
			/* list mix controls */
			multi_mix_control mmc;
			mMixControlInfo.info_size = sizeof(multi_mix_control_info);
			mMixControlInfo.control_count = 1;
			mMixControlInfo.controls = &mmc;
			mmc.id = *controls;
			err = ioctl(mDriver, B_MULTI_LIST_MIX_CONTROLS, &mMixControlInfo ,0);
			MIX(("ctl:%d flags:%x master:%d name:%s \n",mmc.id, mmc.flags, mmc.master, mmc.name));
			if (B_OK != err) {
				MIX(("Failed on B_MULTI_LIST_MIX_CONTROLS \n"));
				continue;
			}
			if (mmc.flags & B_MULTI_MIX_GAIN) {
				ch->MakeContinuousParameter(mmc.id, B_MEDIA_RAW_AUDIO, mmc.name, B_GAIN,
													"dB", mmc.gain.min_gain, mmc.gain.max_gain,
													mmc.gain.granularity);
				MIX(("\tMin:%f  Max:%f  Granularity:%f\n",mmc.gain.min_gain,
															mmc.gain.max_gain,
															mmc.gain.granularity));
			}
			if (mmc.flags & B_MULTI_MIX_MUX) {
				/* dug - how do i make a selection list here? */
			}
			if (mmc.flags & B_MULTI_MIX_ENABLE) {
				ch->MakeDiscreteParameter(mmc.id, B_MEDIA_RAW_AUDIO, mmc.name, B_ENABLE);
			}
			controls++;
		}
		free(to_free);
		controls = NULL;
	}
#endif //CREATE_MIXER_CONTROLS
		
	/* channel enabler */
	WEB(("BMultiNode::MakeParameterWeb creating channel enabler\n"));
	if ((B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_OUTPUTS & mDescription.interface_flags) ||
		(B_MULTI_INTERFACE_CLICKS_WHEN_ENABLING_INPUTS & mDescription.interface_flags)) {
		BParameterGroup *options = web->MakeGroup("Options");
		BParameterGroup *enable = options->MakeGroup("Description");
		char description0[] = "Channels are normally enabled and disabled when";
		char description1[] = "a connection is made by an application.";
		char description2[] = "This device may glitch when enabling channels.";
		char description3[] = "To avoid this, enable all channels all the time.";
		char description4[] = "Be aware, this may cause more load on the cpu.";
		enable->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "Enabling Channels", B_GENERIC);	
		enable->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, description0, B_GENERIC);	
		enable->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, description1, B_GENERIC);	
		enable->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, description2, B_GENERIC);	
		enable->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, description3, B_GENERIC);	
		enable->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, description4, B_GENERIC);	
		enable->MakeDiscreteParameter(M_ENABLE_ALL_CHANNELS, B_MEDIA_RAW_AUDIO, "Enable All Channels", B_ENABLE);
	}

	/* about */
	WEB(("BMultiNode::MakeParameterWeb creating about\n"));
	BParameterGroup *about = web->MakeGroup("About");
	BParameterGroup *description = about->MakeGroup("About");
	char label[256];
	/* name and vendor */
	sprintf(label, "The %s by %s", mDescription.friendly_name, mDescription.vendor_info);
	description->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);
	/* input and output count */
	sprintf(label, "%ld outputs  %ld inputs", mDescription.output_channel_count, mDescription.input_channel_count);
	description->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);

	/* latency */
	BParameterGroup *latency = about->MakeGroup("Latency");
	latency->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "Latency", B_GENERIC);		
	sprintf(label, "Total Input to Output: %.4g ms",
			((2*mBufferDuration)+mFormatInfo.input_latency+mFormatInfo.output_latency)/1000.0);
	latency->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);
	/* inputs */
	BParameterGroup *input = latency->MakeGroup("Input");
	input->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "Input", B_GENERIC);		
	sprintf(label, "   BeOS: %.3g ms", mBufferDuration/1000.0);
	input->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);	
	sprintf(label, "   %s: %.3g ms", mDescription.friendly_name, mFormatInfo.input_latency/1000.0);
	input->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);	
	sprintf(label, "   Total: %.3g ms", (mBufferDuration + mFormatInfo.input_latency)/1000.0);
	input->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);	
	/* outputs */
	BParameterGroup *output = latency->MakeGroup("Output");
	output->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, "Output", B_GENERIC);		
	sprintf(label, "   BeOS: %.3g ms", mBufferDuration/1000.0);
	output->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);	
	sprintf(label, "   %s: %.3g ms", mDescription.friendly_name, mFormatInfo.output_latency/1000.0);
	output->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);	
	sprintf(label, "   Total: %.3g ms", (mBufferDuration + mFormatInfo.output_latency)/1000.0);
	output->MakeNullParameter(M_NULL, B_MEDIA_RAW_AUDIO, label, B_GENERIC);	

	SetParameterWeb(web);
}

void 
BMultiNode::GetSettings()
{
}

void 
BMultiNode::SaveSettings()
{
}

void 
BMultiNode::GetDescription()
{
	FUNCTION(("BMultiNode::GetDescription\n"));
	/* get rid of old channels if there are any */
	if (mChannelInfo != NULL) {
		rtm_free(mChannelInfo);
		mChannelInfo = NULL;
		mDescription.request_channel_count = 0;
	}
	/* find out how many channels there are to get */
	mDescription.info_size = sizeof(mDescription);
	mDescription.channels = mChannelInfo;
	if (ioctl(mDriver, B_MULTI_GET_DESCRIPTION, &mDescription, 0) < 0) {
		TERROR(("BMultiNode::GetDescription: cannot get description\n"));
		TERROR(("BMultiNode::GetDescription: &mDescription = %x\n", &mDescription));
		return;
	}
	
	mTotalChannels = mDescription.output_channel_count + mDescription.input_channel_count + mDescription.output_bus_channel_count
							+ mDescription.input_bus_channel_count + mDescription.aux_bus_channel_count;
	mIOChannels = mDescription.output_channel_count + mDescription.input_channel_count;
	mFirstOutputChannelID = 0;
	mFirstInputChannelID = mDescription.output_channel_count;
	
	DESCRIPTION(("Friendly name:\t%s\nVendor:\t\t%s\n",
				mDescription.friendly_name,mDescription.vendor_info));
	DESCRIPTION(("%ld outputs\t%ld inputs\n%ld out busses\t%ld in busses\n",
				mDescription.output_channel_count,mDescription.input_channel_count,
				mDescription.output_bus_channel_count,mDescription.input_bus_channel_count));
	DESCRIPTION(("\nChannels\n"
			 "ID\tKind\tDesig\tConnectors\n"));

	mChannelInfo = (multi_channel_info *) rtm_alloc(NULL, sizeof(multi_channel_info) * mTotalChannels);
	if (!mChannelInfo)
	{
		TERROR(("BMultiNode::GetDescription - mChannelInfo allocation failed\n"));
	}
	mDescription.channels = mChannelInfo;
	mDescription.request_channel_count = mTotalChannels;
	if (ioctl(mDriver, B_MULTI_GET_DESCRIPTION, &mDescription, 0) < 0)
	{
		TERROR(("BMultiNode::GetDescription: B_MULTI_GET_DESCRIPTION failed\n"));
		return;
	}
#if DEBUG

	for (int i = 0 ; i < (mDescription.output_channel_count + mDescription.input_channel_count); i++)
	{
		DESCRIPTION(("%ld\t%d\t%lu\t0x%lx\n",mDescription.channels[i].channel_id,
											mDescription.channels[i].kind,
											mDescription.channels[i].designations,
											mDescription.channels[i].connectors));
	}			 
	DESCRIPTION(("\n"));	
	DESCRIPTION(("Output rates\t\t0x%lx\n",mDescription.output_rates));
	DESCRIPTION(("Input rates\t\t0x%lx\n",mDescription.input_rates));
	DESCRIPTION(("Max CVSR\t\t%.0f\n",mDescription.max_cvsr_rate));
	DESCRIPTION(("Min CVSR\t\t%.0f\n",mDescription.min_cvsr_rate));
	DESCRIPTION(("Output formats\t\t0x%lx\n",mDescription.output_formats));
	DESCRIPTION(("Input formats\t\t0x%lx\n",mDescription.input_formats));
	DESCRIPTION(("Lock sources\t\t0x%lx\n",mDescription.lock_sources));
	DESCRIPTION(("Timecode sources\t0x%lx\n",mDescription.timecode_sources));
	DESCRIPTION(("Interface flags\t\t0x%lx\n",mDescription.interface_flags));
	DESCRIPTION(("Start Latency:\t\t%Ld\n",mDescription.start_latency));
	DESCRIPTION(("Control panel string:\t\t%s\n",mDescription.control_panel));
	DESCRIPTION(("\n"));
#endif
}

void 
BMultiNode::SetupEnable()
{
	FUNCTION(("BMultiNode::SetupEnable\n"));
	/* round UP to nearest whole byte, minimum 32bits */
	int32 byteCount = MIN(4, (mIOChannels+7)/8);
	uchar *bits = (uchar *)rtm_alloc(NULL, byteCount * sizeof(uchar));
	uint32 *cons = (uint32 *)rtm_alloc(NULL, mTotalChannels * sizeof(uint32));
	mChannelEnable.info_size = sizeof(mChannelEnable);
	mChannelEnable.enable_bits = bits;
	mChannelEnable.connectors = cons;
	
	if (ioctl(mDriver, B_MULTI_GET_ENABLED_CHANNELS, &mChannelEnable, 0) < 0)
	{
		TERROR(("BMultiNode::SetupEnable: B_MULTI_GET_ENABLED_CHANNELS failed\n"));
	}	
	/* only enable connected channels */
	/* enable channel 0 - first record channel - to keep timing going */
	B_SET_CHANNEL(bits, 0, 1);
	
	/* enable them all */
	if (mSetup.enable_all_channels) {
		for (int i=0; i<mIOChannels; i++){
			B_SET_CHANNEL(bits, i, 1);
		}
	}
	/* allow all connectors */
	for (int i=0; i<mTotalChannels; i++) {
		cons[i] = 0xffffffff;
	}
	mChannelEnable.lock_source = B_MULTI_LOCK_INTERNAL;
	mChannelEnable.lock_data = 0;
	mChannelEnable.timecode_source = 0;
	if (ioctl(mDriver, B_MULTI_SET_ENABLED_CHANNELS, &mChannelEnable, 0) < 0) {
		TERROR(("BMultiNode::SetupEnable: B_MULTI_SET_ENABLED_CHANNELS failed\n"));
	}
}

void 
BMultiNode::SetupFormats()
{
	/* the media format is still incomplete after using this function */
	/* you must call get buffers in order to have all the info needed */
	/* don't forget that inputs in the driver are outputs in the node */ 
	FUNCTION(("BMultiNode::SetupFormats\n"));
	status_t err = B_OK;
	/* get the current format */
	mFormatInfo.info_size = sizeof(mFormatInfo);
	err = ioctl(mDriver, B_MULTI_GET_GLOBAL_FORMAT, &mFormatInfo, 0);
	if (err < B_OK) {
		TERROR(("BMultiNode::SetupFormats B_MULTI_GET_GLOBAL_FORMAT failed\n"));
		return;
	}
	
	/* make sure the format we want is possible */
	uint32 rate = convert_media_rate_to_multi_rate(mPlaybackFormat.u.raw_audio.frame_rate);
	float frame_rate =  mPlaybackFormat.u.raw_audio.frame_rate;
	uint32 format = convert_media_format_to_multi_format(mPlaybackFormat.u.raw_audio.format);
	
	if (mFormatInfo.output.rate != rate && (rate & mDescription.output_rates))
		mFormatInfo.output.rate = rate;
	if (mFormatInfo.output.cvsr != frame_rate && frame_rate >= mDescription.min_cvsr_rate
		&& frame_rate <= mDescription.max_cvsr_rate)
		mFormatInfo.output.cvsr = frame_rate;
	if (mFormatInfo.output.format != format && (format & mDescription.output_formats))
		mFormatInfo.output.format = format;
	mFormatInfo.input.rate = mFormatInfo.output.rate;
	mFormatInfo.input.cvsr = mFormatInfo.output.cvsr;
	mFormatInfo.input.format = mFormatInfo.output.format;
	
	/* set the format */
	err = ioctl(mDriver, B_MULTI_SET_GLOBAL_FORMAT, &mFormatInfo, 0);
	if (err < B_OK) {
		TERROR(("BMultiNode::SetupFormats B_MULTI_SET_GLOBAL_FORMAT failed\n"));
		return;
	}
	
	/* get the format again to find out latencies */
	err = ioctl(mDriver, B_MULTI_GET_GLOBAL_FORMAT, &mFormatInfo, 0);
	if (err < B_OK) {
		TERROR(("BMultiNode::SetupFormats B_MULTI_GET_GLOBAL_FORMAT failed\n"));
		return;
	}
	/* output format */
	mPlaybackFormat.u.raw_audio.frame_rate = convert_multi_rate_to_media_rate(mFormatInfo.output.rate,
																			mFormatInfo.output.cvsr);
	mPlaybackFormat.u.raw_audio.format = convert_multi_format_to_media_format(mFormatInfo.output.format);
	/* input format */	
	mRecordFormat.u.raw_audio.frame_rate = convert_multi_rate_to_media_rate(mFormatInfo.input.rate,
																			mFormatInfo.input.cvsr);
	mRecordFormat.u.raw_audio.format = convert_multi_format_to_media_format(mFormatInfo.input.format);
	
	FUNCTION(("BMultiNode::SetupFormats latency: input %Ld output %Ld\n",
				mFormatInfo.input_latency, mFormatInfo.output_latency));
}

void 
BMultiNode::SetupBuffers()
{
	FUNCTION(("BMultiNode::SetupBuffers\n"));
	memset(&mBufferList, 0, sizeof(mBufferList));
	mBufferList.info_size = sizeof(mBufferList);
	memset(&mBufferInfo, 0, sizeof(mBufferInfo));
	mBufferInfo.info_size = sizeof(mBufferInfo);
	
	/* playback buffers */
	mBufferList.request_playback_buffers = mBufferCount;
	mBufferList.request_playback_channels = mDescription.output_channel_count;
	/* allocate buffer_descriptions */
	mPlaybackBufferDescriptions = (buffer_desc **)rtm_alloc(NULL, mBufferCount * sizeof(buffer_desc *));
	for (int i=0; i<mBufferCount; i++)
		mPlaybackBufferDescriptions[i] = (buffer_desc *)rtm_alloc(NULL, mDescription.output_channel_count * sizeof(buffer_desc));
	mBufferList.playback_buffers = mPlaybackBufferDescriptions;
	
	/* record buffers */
	mBufferList.request_record_buffers = mBufferCount;
	mBufferList.request_record_channels = mDescription.input_channel_count;
	/* allocate buffer_descriptions */
	mRecordBufferDescriptions = (buffer_desc **)rtm_alloc(NULL, mBufferCount * sizeof(buffer_desc *));
	for (int i=0; i<mBufferCount; i++)
		mRecordBufferDescriptions[i] = (buffer_desc *)rtm_alloc(NULL, mDescription.input_channel_count * sizeof(buffer_desc));
	mBufferList.record_buffers = mRecordBufferDescriptions;
	/* ask for our preferred buffer size */
	mBufferList.request_playback_buffer_size = mSetup.buffer_size;
	mBufferList.request_record_buffer_size = mSetup.buffer_size;
	
	/* do the cocktail */
	status_t err = ioctl(mDriver, B_MULTI_GET_BUFFERS, &mBufferList, 0);
	if (err < B_OK) {
		TERROR(("BMultiNode::SetupBuffers Failed on B_MULTI_GET_BUFFERS\n"));
		return;
	}
	/* set our values */
	mBufferSize = mBufferList.return_playback_buffer_size;
	mBufferCount = mBufferList.return_playback_buffers;
	/* set the buffer_size based on what was returned */
	mRecordFormat.u.raw_audio.buffer_size = mBufferSize * (mRecordFormat.u.raw_audio.format & 0xf);
	mPlaybackFormat.u.raw_audio.buffer_size = mBufferSize * (mPlaybackFormat.u.raw_audio.format & 0xf);
	/* calculate and set the buffer duration for correct MEL operation */
	mBufferDuration = (bigtime_t)(mBufferSize * 1000000LL / mPlaybackFormat.u.raw_audio.frame_rate);
	SetBufferDuration(mBufferDuration);
#if DEBUG
	/* sanity check playback == record */
	if (mBufferSize != mBufferList.return_record_buffer_size)
		TERROR(("BMultiNode::SetupBuffers playback_buffer_size %d != %d record_buffer_size\n",
				mBufferList.return_playback_buffer_size, mBufferList.return_record_buffer_size));
	if (mBufferCount != mBufferList.return_record_buffers)
		TERROR(("BMultiNode::SetupBuffers return_playback_buffers %d != %d return_record_buffers\n",
				mBufferList.return_playback_buffers, mBufferList.return_record_buffers));
	/* print out a list of the buffers returned */
	BUFFERINFO(("\n"));
	for (int i = 0; i<2; i++)
		for (int j=0; j < mDescription.output_channel_count; j++)
			BUFFERINFO(("Play buffers[%d][%d]: 0x%p\n", i, j, mPlaybackBufferDescriptions[i][j]));
	for (int i = 0; i<2; i++)
		for (int j=0; j < mDescription.input_channel_count; j++)
			BUFFERINFO(("Record buffers[%d][%d]: 0x%p\n", i, j, mRecordBufferDescriptions[i][j]));
	BUFFERINFO(("\n"));
	BUFFERINFO(("buffer size is %d frames %Ld microsec\n", mBufferList.return_playback_buffer_size, mBufferDuration));
#endif
}

void 
BMultiNode::SetupOutputs()
{
	FUNCTION(("BMultiNode::SetupOutputs\n"));
	MultiChannel *ch = NULL;
	media_output out;
	out.node = Node();
	out.destination = media_destination::null;
	out.format.type = B_MEDIA_RAW_AUDIO;	/* just audio, that's what we do */

	/* ok, here it goes, outputs from the node are inputs in the driver, blech! */
	for (int32 i=0; i <= mDescription.input_channel_count; i++)
	{
		/* source id is created here */
		out.source.port = ControlPort();
		out.source.id = i;
		
		/* use global format except for channel 0 */
		out.format = mRecordFormat;
		if (i == 0)
			out.format.u.raw_audio.channel_count = 0;
		
		/* create channel */
		if (i == 0) {
			sprintf(out.name, "Any Input");
			ch = new MultiChannel(i, EndPoint::B_OUTPUT, mChannelInfo[mFirstInputChannelID + i], out.name);
		}
		else {	
			sprintf(out.name, "Input %ld", i);
			ch = new MultiChannel(i, EndPoint::B_OUTPUT, mChannelInfo[mFirstInputChannelID + i - 1], out.name);
		}
		ch->SetOutput(&out);
		mOutputs.AddEndPoint(ch);
		SETUP(("input %d : %s\n", ch->ID(), out.name));
	}
}

void 
BMultiNode::SetupInputs()
{
	FUNCTION(("BMultiNode::SetupInputs\n"));
	MultiChannel *ch = NULL;
	media_input in;
	in.node = Node();
	in.source = media_source::null;
	in.format.type = B_MEDIA_RAW_AUDIO;	/* just audio, that's what we do */

	/* ok, here it goes, inputs from the node are outputs in the driver, blech! */
	for (int32 i=0; i <= mDescription.output_channel_count; i++)
	{
		/* destination id is created here */
		in.destination.port = ControlPort();
		in.destination.id = i;
		
		/* use global format except for channel 0 */
		in.format = mPlaybackFormat;
		/* format is free */
		in.format.u.raw_audio.format = media_raw_audio_format::wildcard.format;
		if (i == 0)
			in.format.u.raw_audio.channel_count = 0;
		
		/* create channel */
		if (i == 0) {
			sprintf(in.name, "Any Output");
			ch = new MultiChannel(i, EndPoint::B_INPUT, mChannelInfo[i], in.name);
		}
		else {
			sprintf(in.name, "Output %ld", i);
			ch = new MultiChannel(i, EndPoint::B_INPUT, mChannelInfo[i-1], in.name);
		}
		ch->SetInput(&in);
		mInputs.AddEndPoint(ch);
		SETUP(("output %d : %s\n", ch->ID(), in.name));
	}
}

void 
BMultiNode::SetupMix()
{
#if CREATE_MIXER_CONTROLS
	FUNCTION(("BMultiNode::SetupMix\n"));
	status_t err = B_OK;
	
	for (int32 i=0; i<mTotalChannels; i++) {
		mMixChannelInfo.info_size = sizeof(multi_mix_channel_info);
		mMixChannelInfo.channel_count = 1;
		int32 channel = i; 
		mMixChannelInfo.channels = &channel;
		mMixChannelInfo.max_count = 0;
		mMixChannelInfo.controls = NULL;					
		/* list mix channels */
		err = ioctl(mDriver, B_MULTI_LIST_MIX_CHANNELS, &mMixChannelInfo, 0);
		if (B_OK != err) {
			MIX(("Failed on B_MULTI_LIST_MIX_CHANNELS \n"));
			break;
		}
		/* find out how many there are and make room */
		mMixChannelInfo.max_count = mMixChannelInfo.actual_count;
		int32 *controls = (int32 *)malloc(sizeof(int32) * mMixChannelInfo.actual_count);
		void *to_free = controls;
		if (controls == NULL) {
			break;
		}
		mMixChannelInfo.controls = &controls;
		/* get them all */
		err = ioctl(mDriver, B_MULTI_LIST_MIX_CHANNELS, &mMixChannelInfo ,0);
		if (B_OK != err) {
			MIX(("Failed on B_MULTI_LIST_MIX_CHANNELS \n"));
			free(controls);
			break; /* for loop */
		}
		for (int32 tmp = 0; tmp < mMixChannelInfo.actual_count; tmp++) {
			/* list mix controls */
			multi_mix_control mmc;
			mMixControlInfo.info_size = sizeof(multi_mix_control_info);
			mMixControlInfo.control_count = 1;
			mMixControlInfo.controls = &mmc;
			mmc.id = *controls;
			err = ioctl(mDriver, B_MULTI_LIST_MIX_CONTROLS, &mMixControlInfo ,0);
			MIX(("ctl:%d flags:%x master:%d name:%s \n",mmc.id, mmc.flags, mmc.master, mmc.name));
			if (mmc.flags & B_MULTI_MIX_GAIN) {
				MIX(("\tMin:%f  Max:%f  Granularity:%f\n",mmc.gain.min_gain,
															mmc.gain.max_gain,
															mmc.gain.granularity));
			}
			if (B_OK != err) {
				MIX(("Failed on B_MULTI_LIST_MIX_CONTROLS \n"));
			}
			/* get mix values */
			multi_mix_value  mmv;
			mMixValueInfo.info_size = sizeof(multi_mix_value_info);
			mMixValueInfo.item_count= 1;
			mMixValueInfo.values = &mmv;
			mmv.id = *controls;
			err = ioctl(mDriver, B_MULTI_GET_MIX, &mMixValueInfo ,0);
			if (B_OK != err) {
				MIX(("Failed on B_MULTI_GET_MIX 2nd time\n"));
			}
			else {
				MIX(("%ld",mmv.id));
				MIX(("\t%f",mmv.gain));
				MIX(("\t\t%d",mmv.enable));
				MIX(("\t\t\t%x",mmv.mux));
				MIX(("\t\t\t\t%ld",mmv.ramp));
				MIX(("\n"));
			}
			controls++;
		}
		free(to_free);
		controls = NULL;
	}
	/* list mix connections */
	mMixConnectionInfo.info_size = sizeof(multi_mix_connection_info);
	mMixConnectionInfo.max_count = 0;
	err = ioctl(mDriver, B_MULTI_LIST_MIX_CONNECTIONS, &mMixConnectionInfo, 0);
	if (B_OK != err) {
		MIX(("Failed on B_MULTI_LIST_MIX_CONNECTIONS (1st time)\n"));
	}
	mMixConnectionInfo.max_count = mMixConnectionInfo.actual_count;
	multi_mix_connection *mmc = (multi_mix_connection *)rtm_alloc(NULL, sizeof(multi_mix_connection) * mMixConnectionInfo.max_count);
	if (mmc == NULL) {
		return;
	}	
	mMixConnectionInfo.connections = mmc;
	err = ioctl(mDriver, B_MULTI_LIST_MIX_CONNECTIONS, &mMixConnectionInfo, 0);
	if (B_OK != err) {
		MIX(("Failed on B_MULTI_LIST_MIX_CONNECTIONS (2nd time)\n"));
	}
	else {
		for (int32 j=0; j < mMixConnectionInfo.max_count; j++)
			MIX(("from:%d to:%d \n",mmc[j].from, mmc[j].to));
	}
#endif
}

void 
BMultiNode::EnableAllChannels()
{
	FUNCTION(("BMultiNode::EnableAllChannels\n"));
	EnableChannels(0, mIOChannels);
}

void 
BMultiNode::DisableAllChannels()
{
	FUNCTION(("BMultiNode::DisableAllChannels\n"));
	bool needed = false;
	MultiChannel *ch = NULL;
	/* inputs */
	if (mInputs.EndPointAt(1, (EndPoint **)&ch) < B_OK)
		ch = NULL;
	while (ch) {
		/* only disable unconnected channels */
		if (!ch->Connected() && !ch->Obscured()) {
			if (B_TEST_CHANNEL(mChannelEnable.enable_bits, ch->ChannelID())) {
				B_SET_CHANNEL(mChannelEnable.enable_bits, ch->ChannelID(), 0);
				needed = true;
			}
		}
		if (mInputs.NextEndPoint(ch->ID(), (EndPoint **)&ch) < B_OK)
			break;
	}
	/* outputs */
	if (mOutputs.EndPointAt(1, (EndPoint **)&ch) < B_OK)
		ch = NULL;
	while (ch) {
		/* only disable unconnected channels */
		if (!ch->Connected() && !ch->Obscured()) {
			if (B_TEST_CHANNEL(mChannelEnable.enable_bits, ch->ChannelID())) {
				B_SET_CHANNEL(mChannelEnable.enable_bits, ch->ChannelID(), 0);
				needed = true;
			}
		}
		if (mOutputs.NextEndPoint(ch->ID(), (EndPoint **)&ch) < B_OK)
			break;			
	}
	/* only set if something changes */
	if (needed)
		SetEnabledChannels();		
}

void 
BMultiNode::EnableChannels(int32 id, int32 count)
{
	FUNCTION(("BMultiNode::EnableChannels %ld %ld\n", id, count));
	if (ioctl(mDriver, B_MULTI_GET_ENABLED_CHANNELS, &mChannelEnable, 0) < 0)
		TERROR(("BMultiNode::EnableChannels: B_MULTI_GET_ENABLED_CHANNELS failed\n"));
	bool needed = false;
	for (int i=id; i < id+count; i++) {
		if (!B_TEST_CHANNEL(mChannelEnable.enable_bits, i)) {
			B_SET_CHANNEL(mChannelEnable.enable_bits, i, 1);
			needed = true;
		}
	}
	/* only set if something changes */
	if (needed)			
		SetEnabledChannels();
}

void 
BMultiNode::DisableChannels(int32 id, int32 count)
{
	FUNCTION(("BMultiNode::DisableChannels %ld %ld\n", id, count));
	/* keep channel 0 enabled so dma continues */
	if (id == 0 || mSetup.enable_all_channels)
		return;
	if (ioctl(mDriver, B_MULTI_GET_ENABLED_CHANNELS, &mChannelEnable, 0) < 0)
		TERROR(("BMultiNode::DisableChannels: B_MULTI_GET_ENABLED_CHANNELS failed\n"));
	
	bool needed = false;
	for (int i=id; i < id+count; i++) {
		if (B_TEST_CHANNEL(mChannelEnable.enable_bits, i)) {
			B_SET_CHANNEL(mChannelEnable.enable_bits, i, 0);
			needed = true;
		}
	}
	/* only set if something changes */
	if (needed)
		SetEnabledChannels();
}

void 
BMultiNode::SetEnabledChannels()
{
	FUNCTION(("BMultiNode::SetEnabledChannels\n"));
	StopDMA();
	if (ioctl(mDriver, B_MULTI_SET_ENABLED_CHANNELS, &mChannelEnable, 0) < B_OK)
		TERROR(("BMultiNode::SetEnabledChannels: B_MULTI_SET_ENABLED_CHANNELS failed\n"));
	if (RunState() == BMediaEventLooper::B_STARTED) {
		mRealStartTime = TimeSource()->RealTime();
		StartDMA();
	}
}

void 
BMultiNode::SetGlobalFormat()
{
//	thread_info ti;
//	get_thread_info(find_thread(NULL),&ti);
//	printf("SetGlobalFormat in thread %s\n",ti.name);

#if 1
	mPerfStartTime=mPerfStartTime+(bigtime_t)((mBufferInfo.played_frames_count-mBufferInfoPlayedFramesCount)
								* 1000000LL / mPlaybackFormat.u.raw_audio.frame_rate);; // set new time base
	mBufferInfoPlayedFramesCount=mBufferInfo.played_frames_count;
	mRealStartTime = TimeSource()->RealTimeFor(mPerfStartTime, 0);
	mPlayedCount = 0;
#endif

	FUNCTION(("BMultiNode::SetGlobalFormat\n"));
	FORMATS(("BMultiNode::SetGlobalFormat output.rate 0x%x\n", mFormatInfo.output.rate));
	FORMATS(("BMultiNode::SetGlobalFormat output.cvsr %f\n", mFormatInfo.output.cvsr));
	FORMATS(("BMultiNode::SetGlobalFormat output.format 0x%x\n", mFormatInfo.output.format));
	FORMATS(("BMultiNode::SetGlobalFormat input.rate 0x%x\n", mFormatInfo.input.rate));
	FORMATS(("BMultiNode::SetGlobalFormat input.cvsr %f\n", mFormatInfo.input.cvsr));
	FORMATS(("BMultiNode::SetGlobalFormat input.format 0x%x\n", mFormatInfo.input.format));
	
	if (ioctl(mDriver, B_MULTI_SET_GLOBAL_FORMAT, &mFormatInfo, 0) < B_OK) {
		TERROR(("BMultiNode::SetGlobalFormat B_MULTI_SET_GLOBAL_FORMAT failed\n"));
		return;
	}
	/* get the format to find out latencies */
	if (ioctl(mDriver, B_MULTI_GET_GLOBAL_FORMAT, &mFormatInfo, 0) < B_OK) {
		TERROR(("BMultiNode::SetGlobalFormat B_MULTI_GET_GLOBAL_FORMAT failed\n"));
		return;
	}
	/* output format */
	mPlaybackFormat.u.raw_audio.frame_rate = convert_multi_rate_to_media_rate(mFormatInfo.output.rate,
																			mFormatInfo.output.cvsr);
	mPlaybackFormat.u.raw_audio.format = convert_multi_format_to_media_format(mFormatInfo.output.format);
	/* input format */	
	mRecordFormat.u.raw_audio.frame_rate = convert_multi_rate_to_media_rate(mFormatInfo.input.rate,
																			mFormatInfo.input.cvsr);
	mRecordFormat.u.raw_audio.format = convert_multi_format_to_media_format(mFormatInfo.input.format);
	
	mSetup.frame_rate = mPlaybackFormat.u.raw_audio.frame_rate;
	mSetup.sample_format = mPlaybackFormat.u.raw_audio.format;
	mAddon->SetupChanged(mDriver, mSetup);
	FUNCTION(("BMultiNode::SetGlobalFormat latency: input %Ld output %Ld\n",
				mFormatInfo.input_latency, mFormatInfo.output_latency));


	CanDoFormatChange(NULL,&mPlaybackFormat);
#if 1
	// tell all the upstream nodes we're changing
	int32 cookie=0;
	media_input mediainput;
	media_format f;
	f.type=B_MEDIA_RAW_AUDIO;
	f.u.raw_audio=media_multi_audio_format::wildcard;
	f.u.raw_audio.frame_rate=mPlaybackFormat.u.raw_audio.frame_rate;
	while(GetNextInput(&cookie, &mediainput)==B_OK)
	{
		if(mediainput.source != media_source::null)
		{
			int32 tag;
		//	printf("Calling RequestFormatChange...\n");
			status_t status;
			status=RequestFormatChange(mediainput.source,  mediainput.destination, f, NULL, &tag);
		//	printf("RequestFormatChange returned %08x (%s)\n",status,strerror(status));
		}
	}
//	printf("done changing\n");
#endif
}

void 
BMultiNode::WriteZeros(MultiChannel *ch)
{
	DATASTATUS(("BMultiNode::WriteZeros for channel %d\n", ch->ID()));
	/* figure out the strides based on the format for this channel */
	const media_raw_audio_format &channel = ch->Format().u.raw_audio;
	size_t samplesize = mPlaybackFormat.u.raw_audio.format & 0xf;
	int32 framestocopy = mBufferSize;			/* in frames! */
	/* loop for buffer count */
	for (int k=0; k < mBufferCount; k++) {
		size_t playstride = mPlaybackBufferDescriptions[k][ch->ChannelID()].stride;
		/* loop for interleaved samples */
		for (uint32 j=0; j < channel.channel_count; j++){
			char *play = (char *)mPlaybackBufferDescriptions[k][ch->ChannelID()+j].base;
			if (samplesize == 4){
				int32 data = 0;
				for (int i=0; i<framestocopy; i++)
				{
					*(int32 *)play = data;
					play += playstride;
				}
			}else if (samplesize == 2){
				int16 data = 0;
				for (int i=0; i<framestocopy; i++)
				{
					*(int16 *)play = data;
					play += playstride;
				}
			}else{
				uchar data = 128;
				for (int i=0; i<framestocopy; i++)
				{
					*play = data;
					play += playstride;
				}
			}
		}
	}
}

void 
BMultiNode::UseBuffer(BBuffer *buffer)
{
	BUFFERREC(("BMultiNode::UseBuffer for channel %d\n", buffer->Header()->destination));
	/* find the channel */
	int32 dest = buffer->Header()->destination;
	MultiChannel *ch = NULL;
	if (mInputs.EndPointAt(dest, (EndPoint **)&ch) < B_OK) {
		TERROR(("BMultiNode::BufferReceived ERROR - Can't find input at %d\n", dest));
		buffer->Recycle();
		return;
	}
	/* figure out the strides based on the format for this channel */
	const media_raw_audio_format &channel = ch->Format().u.raw_audio;
	size_t samplesize = channel.format & 0xf;
	size_t datastride = samplesize * channel.channel_count;
	size_t playstride = mPlaybackBufferDescriptions[mPlaybackIndex][ch->ChannelID()].stride;
	int32 framestocopy = mBufferSize;				/* in frames! */
	/* copy only used portion of incoming buffers */
	if (mBufferSize * datastride > buffer->SizeUsed())
		framestocopy = buffer->SizeUsed() / datastride;
	/* loop for interleaved samples */
	for (uint32 j=0; j < channel.channel_count; j++){
		char *play = (char *)mPlaybackBufferDescriptions[mPlaybackIndex][ch->ChannelID()+j].base;
		char *data = (char *)buffer->Data()+(j*samplesize);
		switch (mPlaybackFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_UCHAR:{
				switch (channel.format) {
					case media_raw_audio_format::B_AUDIO_UCHAR:
						copy_uchar_into_uchar(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_SHORT:
						copy_short_into_uchar(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_INT:
						copy_int_into_uchar(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_FLOAT:
						copy_float_into_uchar(data, play, datastride, playstride, framestocopy);
						break;
				}
				break;
			}
			case media_raw_audio_format::B_AUDIO_SHORT:{
				switch (channel.format) {
					case media_raw_audio_format::B_AUDIO_UCHAR:
						copy_uchar_into_short(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_SHORT:
						copy_short_into_short(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_INT:
						copy_int_into_short(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_FLOAT:
						copy_float_into_short(data, play, datastride, playstride, framestocopy);
						break;
				}
				break;
			}
			case media_raw_audio_format::B_AUDIO_INT:{
				switch (channel.format) {
					case media_raw_audio_format::B_AUDIO_UCHAR:
						copy_uchar_into_int(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_SHORT:
						copy_short_into_int(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_INT:
						copy_int_into_int(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_FLOAT:
						copy_float_into_int(data, play, datastride, playstride, framestocopy);
						break;
				}
				break;
			}
			case media_raw_audio_format::B_AUDIO_FLOAT:{
				switch (channel.format) {
					case media_raw_audio_format::B_AUDIO_UCHAR:
						copy_uchar_into_float(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_SHORT:
						copy_short_into_float(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_INT:
						copy_int_into_float(data, play, datastride, playstride, framestocopy);
						break;
					case media_raw_audio_format::B_AUDIO_FLOAT:
						copy_float_into_float(data, play, datastride, playstride, framestocopy);
						break;
				}
				break;
			}
		}
	}
	BUFFERREC(("*********blocked on Recycle()\n"));
	buffer->Recycle();	
	BUFFERREC(("Recycle() unblocked *********\n"));
}

void 
BMultiNode::SendBuffers()
{
//	thread_info ti;
//	get_thread_info(find_thread(NULL),&ti);
//	printf("SendBuffers in thread %s\n",ti.name);

	SENDINFO(("BMultiNode::SendBuffers Connected Outputs %d\n", mConnectedOutputs));
	/* find the connected outputs */
	MultiChannel *ch = NULL;
	BBuffer *buffer = NULL;
	/* get the first channel */
	if (mOutputs.EndPointAt(1, (EndPoint **)&ch) < B_OK) {
		TERROR(("BMultiNode::SendBuffers - Where did all my outputs go, looong tiiime ru-unning?\n"));
		return;
	}
	while (ch)
	{
		/* look for connected outputs */
		if (ch->Connected() && !ch->Obscured() && ch->OutputEnabled())
		{
			/* figure out the strides based on the format for this channel */
			media_raw_audio_format channel = ch->Format().u.raw_audio;
			size_t samplesize = channel.format & 0xf;
			size_t datastride = samplesize * channel.channel_count;
			size_t recstride = mRecordBufferDescriptions[mRecordIndex][ch->ID()-1].stride;
			int32 framestocopy = mBufferSize;	/* in frames! */
			/* sanity check */
			ASSERT (framestocopy * datastride == channel.buffer_size);
			/* get a buffer */
			SENDINFO(("********channel %ld blocked on request buffer\n", ch->ID()));
			buffer = ch->BufferGroup()->RequestBuffer(channel.buffer_size);
			SENDINFO(("request buffer unblocked **********\n"));
			/* loop for interleaved data */
			for (uint32 j=0; j < channel.channel_count; j++){
				char *rec = (char *)mRecordBufferDescriptions[mRecordIndex][ch->ID()-1+j].base;
				char *data = (char *)buffer->Data() + (j*samplesize);
				switch (channel.format) {
					case media_raw_audio_format::B_AUDIO_UCHAR:{
						switch (mRecordFormat.u.raw_audio.format) {
							case media_raw_audio_format::B_AUDIO_UCHAR:
								copy_uchar_into_uchar(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_SHORT:
								copy_short_into_uchar(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_INT:
								copy_int_into_uchar(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_FLOAT:
								copy_float_into_uchar(rec, data, recstride, datastride, framestocopy);
								break;
						}
						break;
					}
					case media_raw_audio_format::B_AUDIO_SHORT:{
						switch (mRecordFormat.u.raw_audio.format) {
							case media_raw_audio_format::B_AUDIO_UCHAR:
								copy_uchar_into_short(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_SHORT:
								copy_short_into_short(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_INT:
								copy_int_into_short(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_FLOAT:
								copy_float_into_short(rec, data, recstride, datastride, framestocopy);
								break;
						}
						break;
					}
					case media_raw_audio_format::B_AUDIO_INT:{
						switch (mRecordFormat.u.raw_audio.format) {
							case media_raw_audio_format::B_AUDIO_UCHAR:
								copy_uchar_into_int(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_SHORT:
								copy_short_into_int(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_INT:
								copy_int_into_int(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_FLOAT:
								copy_float_into_int(rec, data, recstride, datastride, framestocopy);
								break;
						}
						break;
					}
					case media_raw_audio_format::B_AUDIO_FLOAT:{
						switch (mRecordFormat.u.raw_audio.format) {
							case media_raw_audio_format::B_AUDIO_UCHAR:
								copy_uchar_into_float(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_SHORT:
								copy_short_into_float(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_INT:
								copy_int_into_float(rec, data, recstride, datastride, framestocopy);
								break;
							case media_raw_audio_format::B_AUDIO_FLOAT:
								copy_float_into_float(rec, data, recstride, datastride, framestocopy);
								break;
						}
						break;
					}
				}
			}
			/* set the header info */
			buffer->Header()->size_used = channel.buffer_size;
			buffer->Header()->start_time = TimeSource()->PerformanceTimeFor(mBufferInfo.recorded_real_time)
											- mBufferDuration;	
			SENDINFO(("BMultiNode::SendBuffers sending out channel %d at %Ld\n", ch->ID(), buffer->Header()->start_time));
			if (SendBuffer(buffer, ch->Destination()) < B_OK) {
				SENDINFO(("BMultiNode::SendBuffers send failed, recycling\n"));
				buffer->Recycle();
			}
		}
#if DEBUG
		else {
			MORESENDINFO(("ID %ld; %s; %s; %s\n", ch->ID(),
				ch->Connected() ? "connected" : "not connected",
				ch->Obscured() ? "obscured" : "not obscured",
				ch->OutputEnabled() ? "enabled" : "not enabled"));
		}
#endif		
		if (mOutputs.NextEndPoint(ch->ID(), (EndPoint **)&ch) < B_OK)
			break;
	}
	SENDINFO(("BMultiNode::SendBuffers done\n"));
}

void 
BMultiNode::StartDMA()
{
	FUNCTION(("BMultiNode::StartDMA\n"));
	/* set up */
	mNextExchangeTime = mRealStartTime;
	mPlayedCount = 0;
	mPlaybackIndex = 0;
	/* publish start time with last drift */
	PublishTime(mPerfStartTime, mRealStartTime, mDrift);
	PUBLISHTIME(("MNPUB: STARTING perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mRealStartTime, mDrift));
	PUBLISHSOMETIME(("MNPUB: STARTING perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mRealStartTime, mDrift));
	if (MultiBufferExchange(true) < B_OK){
		/* dug - send node in distress */
		TERROR(("BMultiNode::HandleEvent - MultiBufferExchange failed - stopping\n"));
		StopDMA();
		/* drift is zero when stopped */
		PublishTime(mPerfStartTime, mBufferInfo.played_real_time, 0.0);
		PUBLISHTIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
		PUBLISHSOMETIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
		return;
	}
}

void 
BMultiNode::StopDMA()
{
	FUNCTION(("BMultiNode::StopDMA\n"));
	if (TimeSource()->IsRunning()) {
		/* store the current perftime */
		mPerfStartTime = PerformanceTimeFor(mNextExchangeTime);
	}
	/* stop dma */
	if (ioctl(mDriver, B_MULTI_BUFFER_FORCE_STOP, 0, 0) < B_OK)
		TERROR(("BMultiNode::StopDMA B_MULTI_FORCE_STOP ioctl failed\n"));
	/* flush any HARDWARE or BUFFER events */
	mRTQ->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HARDWARE);
	mEventQ->FlushEvents(mPerfStartTime, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
}

status_t
BMultiNode::MultiBufferExchange(bool starting)
{
#if DEBUG
	static int64 count = 0;
	count++;
	static bigtime_t last = 0LL;
	bigtime_t before = system_time();
	EXCHANGEINFO(("MNEX: target %Ld, realtime %Ld\n", mNextExchangeTime, before));
#endif
	if (ioctl(mDriver, B_MULTI_BUFFER_EXCHANGE, &mBufferInfo, 0) < 0) {
		TERROR(("BMultiNode::MultiBufferExchange: B_MULTI_BUFFER_EXCHANGE ioctl failed\n"));
		return B_ERROR;
	}
#if DEBUG
	bigtime_t now = system_time();
	if (before-last > mBufferDuration || before-last < SchedulingLatency() || now-before > SchedulingLatency() +300LL){
		TIMING(("time between exchanges is %Ld\n", before-last));
		TIMING(("time blocked on exchange is %Ld\n", now-before));
	}
	last = now;
	EXCHANGEINFO(("MNEX: flags 0x%x\n", mBufferInfo.flags));
#endif
	
	if (starting){
		/* if we just started then push an event and return */
		EXCHANGEINFO(("MNEX: ---START---\n"));
		/* update playback index */
		mPlaybackIndex = ++mPlaybackIndex % mBufferCount;
		mBufferInfo.playback_buffer_cycle = mPlaybackIndex;
		EXCHANGEINFO(("MNEX: PlaybackIndex %d\n", mPlaybackIndex));
		/* update next time */
		mNextExchangeTime = RealTimeFor(mPerfStartTime + (bigtime_t)(mBufferSize * 1000000LL / mPlaybackFormat.u.raw_audio.frame_rate), 0);
		EXCHANGEINFO(("MNEX: NextExchangeTime %Ld\n", mNextExchangeTime));
		media_timed_event event(mNextExchangeTime, BTimedEventQueue::B_HARDWARE);
		mRTQ->AddEvent(event);
		mLastTime = mRealStartTime;
		return B_OK;
	}
	else if (RunState() == BMediaEventLooper::B_STARTED){
		/* publish time before updating*/
		EXCHANGEINFO(("MNEX: NextExchangeTime %Ld\n", mNextExchangeTime));
		EXCHANGEINFO(("MNEX: played_frames_count %Ld\n", mBufferInfo.played_frames_count));
		bigtime_t perftime = (bigtime_t)((mBufferInfo.played_frames_count-mBufferInfoPlayedFramesCount)
								* 1000000LL / mPlaybackFormat.u.raw_audio.frame_rate);
		bigtime_t realtime = mBufferInfo.played_real_time - mRealStartTime;
		mDrift = (DRIFT_STABILITY * mDrift)+((1.0-DRIFT_STABILITY)*(double)perftime/(double)realtime);
		PublishTime(mPerfStartTime + perftime, mBufferInfo.played_real_time, mDrift);
//		printf("perftime: %Ld (%Ld+((%Ld-%Ld)*1000000LL/%f)\n",mPerfStartTime+perftime,mPerfStartTime,mBufferInfo.played_frames_count,mBufferInfoPlayedFramesCount,mPlaybackFormat.u.raw_audio.frame_rate);
#if DEBUG
		PUBLISHTIME(("MNPUB: perf %Ld  real %Ld  drift %f\n", mPerfStartTime + perftime, mBufferInfo.played_real_time, mDrift));
		if (!(count%1000)) {
			PUBLISHSOMETIME(("MNPUB: perf %Ld  real %Ld  drift %f\n", mPerfStartTime + perftime, mBufferInfo.played_real_time, mDrift));
		}
		/* watch for missed interrupts */
		if (mBufferInfo.played_frames_count - mPlayedCount != mBufferSize){
			TIMING(("MN: *******MISSED %Ld SAMPLES!!!!! %Ld, %Ld, %d\n", mBufferInfo.played_frames_count - mPlayedCount - mBufferSize,
															mBufferInfo.played_frames_count, mPlayedCount, mBufferSize));
		}
		if (((mBufferInfo.played_real_time - mLastTime) < (mBufferDuration - 110LL)
			|| (mBufferInfo.played_real_time - mLastTime) > (mBufferDuration + 110LL))) {
			TIMING(("driver said exchange was %Ld apart\n", mBufferInfo.played_real_time - mLastTime));
		}
#endif
		/* update playback index */
		ASSERT(mBufferInfo.played_frames_count >= mPlayedCount);
		int32 increment = (mBufferInfo.played_frames_count - mPlayedCount) / mBufferSize;
		mPlaybackIndex = (mPlaybackIndex + increment) % mBufferCount;
		ASSERT(mPlaybackIndex < mBufferCount && mPlaybackIndex > -1);
		mBufferInfo.playback_buffer_cycle = mPlaybackIndex;
		EXCHANGEINFO(("MNEX: PlaybackIndex %d\n", mPlaybackIndex));
		mPlayedCount += (increment * mBufferSize);
		/* update elapsed time */
		mNextExchangeTime = RealTimeFor(mPerfStartTime + (bigtime_t)((mBufferSize + mPlayedCount - mBufferInfoPlayedFramesCount)
									* 1000000LL / mPlaybackFormat.u.raw_audio.frame_rate), 0);
		/* add next time to do exchange */
		EXCHANGEINFO(("MNEX: NextExchangeTime %Ld\n", mNextExchangeTime));
		media_timed_event event(mNextExchangeTime, BTimedEventQueue::B_HARDWARE);
		mRTQ->AddEvent(event);
		mLastTime = mBufferInfo.played_real_time;
	}
	else if (RunState() == BMediaEventLooper::B_STOPPED){
		EXCHANGEINFO(("MNEX: ---STOP---\n"));
		StopDMA();
		/* drift is zero when stopped */
		PublishTime(mPerfStartTime, mBufferInfo.played_real_time, 0.0);
		PUBLISHTIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
		PUBLISHSOMETIME(("MNPUB: STOPPED perf %Ld  real %Ld  drift %f\n", mPerfStartTime, mBufferInfo.played_real_time, 0.0));
	}
	else {
		TERROR(("BMultiNode: We must be in a bad state: 0x%x\n", RunState()));
		return B_ERROR;
	}
	/* put anything that must happen AFTER the first call past this line */
	/* ================================================================= */
	
	/* update record index */
	mRecordIndex = mBufferInfo.record_buffer_cycle;
	ASSERT(mRecordIndex < mBufferCount && mRecordIndex > -1);
	EXCHANGEINFO(("MNEX: RecordIndex %d\n", mRecordIndex));
	
	/* dug - send recorded buffers here if we have any connections*/
	if (mConnectedOutputs > 0)
		SendBuffers();
	
#if DEBUG_LOOPBACK
	/* this copies the just recorded buffers to the about to play buffers */
	/* dug - this only works for 4 byte frames */
	int32 copy = (mDescription.output_channel_count < mDescription.input_channel_count) ? mDescription.output_channel_count : mDescription.input_channel_count;
	EXCHANGEINFO(("MNEX: DEBUG_LOOPBACK %d channels, %d samples\n", copy, mBufferSize));		
	for (int i=0; i < copy; i++){
		int32 *play = (int32 *)mPlaybackBufferDescriptions[mPlaybackIndex][i].base;
		int32 *rec = (int32 *)mRecordBufferDescriptions[mRecordIndex][i].base;
		for (int j=0; j < mBufferSize; j++)
			*play++ = *rec++;
	}
#endif

	return B_OK;
}

/* utilities */
//#pragma mark ---utilities---

uint32
BMultiNode::convert_multi_format_to_media_format(uint32 format)
{
	int32 out = 0;
	switch (format)
	{
		case B_FMT_8BIT_S:
			out = media_raw_audio_format::B_AUDIO_CHAR;
		break;
		case B_FMT_8BIT_U:
			out = media_raw_audio_format::B_AUDIO_UCHAR;
		break;
		case B_FMT_16BIT:
			out = media_raw_audio_format::B_AUDIO_SHORT;
		break;
		case B_FMT_18BIT:
		case B_FMT_20BIT:
		case B_FMT_24BIT:
		case B_FMT_32BIT:
			out = media_raw_audio_format::B_AUDIO_INT;
		break;
		case B_FMT_FLOAT:
			out = media_raw_audio_format::B_AUDIO_FLOAT;
		break;
		case B_FMT_DOUBLE:
		case B_FMT_EXTENDED:
		case B_FMT_BITSTREAM:
		case B_FMT_IS_GLOBAL:
		case B_FMT_SAME_AS_INPUT:
		default:
			out = media_raw_audio_format::B_AUDIO_FLOAT;
		break;
	}
	FUNCTION(("convert_multi_format_to_media_format multi 0x%x to media 0x%x\n", format, out));
	return out;
}

uint32
BMultiNode::convert_media_format_to_multi_format(uint32 format)
{
	uint32 out = 0;
	switch (format)
	{
		case media_raw_audio_format::B_AUDIO_UCHAR:
			out = B_FMT_8BIT_U;
		break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			out = B_FMT_16BIT;
		break;
		case media_raw_audio_format::B_AUDIO_INT:
			/* dug - how to determine correct one? */
			out = B_FMT_24BIT;
		break;
		case media_raw_audio_format::B_AUDIO_FLOAT:
			out = B_FMT_FLOAT;
		break;
		default:
			out = B_FMT_16BIT;
		break;
	}
	FUNCTION(("convert_media_format_to_multi_format media 0x%x to multi 0x%x\n", format, out));
	return out;
}

float
BMultiNode::convert_multi_rate_to_media_rate(uint32 rate, float cvsr)
{
	float out = 0.0;
	switch (rate)
	{
		case B_SR_8000:
			out = 8000;
		break;
		case B_SR_11025:
			out = 11025;
		break;
		case B_SR_12000:
			out = 12000;
		break;
		case B_SR_16000:
			out = 16000;
		break;
		case B_SR_22050:
			out = 22050;
		break;
		case B_SR_24000:
		 	out = 24000;
		break;
		case B_SR_32000:
			out = 32000;
		break;
		case B_SR_44100:
			out = 44100;
		break;
		case B_SR_48000:
			out = 48000;
		break;
		case B_SR_88200:
			out = 88200;
		break;
		case B_SR_96000:
			out = 96000;
		break;
		case B_SR_176400:
			out = 176400;
		break;
		case B_SR_192000:
			out = 192000;
		break;
		case B_SR_384000:
			out = 384000;
		break;
		case B_SR_1536000:
			out = 1536000;
		break;
		case B_SR_IS_GLOBAL:
		case B_SR_SAME_AS_INPUT:
		case B_SR_CVSR:
		default:
			out = cvsr;
		break;
	}
	FUNCTION(("convert_multi_rate_to_media_rate multi 0x%x, %f to media %f\n", rate, cvsr, out));
	return out;
}

uint32
BMultiNode::convert_media_rate_to_multi_rate(float rate)
{
	uint32 out;
	
	if (rate == 8000.0)
		out = B_SR_8000;
	else if (rate == 11025.0)
		out = B_SR_11025;
	else if (rate == 12000.0)
		out = B_SR_12000;
	else if (rate == 16000.0)
		out = B_SR_16000;
	else if (rate == 22050.0)
		out = B_SR_22050;
	else if (rate == 24000.0)
		out = B_SR_24000;
	else if (rate == 32000.0)
		out = B_SR_32000;
	else if (rate == 44100.0)
		out = B_SR_44100;
	else if (rate == 48000.0)
		out = B_SR_48000;
	else if (rate == 88200.0)
		out = B_SR_88200;
	else if (rate == 96000.0)
		out = B_SR_96000;
	else if (rate == 176400)
		out = B_SR_176400;
	else if (rate == 192000.0)
		out = B_SR_192000;
	else if (rate == 384000.0)
		out = B_SR_384000;
	else if (rate == 1536000.0)
		out = B_SR_1536000;
	else
		out = B_SR_CVSR;

	FUNCTION(("convert_media_rate_to_multi_rate media %f to multi 0x%x\n", rate, out));
	return out;
}

void 
BMultiNode::string_for_multi_format(int32 format, char *str)
{
	/* str must be 32 bytes */
	switch (format)
	{
		case B_FMT_8BIT_S:
			sprintf(str, "8 bit");
		break;
		case B_FMT_8BIT_U:
			sprintf(str, "8 bit");
		break;
		case B_FMT_16BIT:
			sprintf(str, "16 bit");
		break;
		case B_FMT_18BIT:
			sprintf(str, "18 bit");
		break;
		case B_FMT_20BIT:
			sprintf(str, "20 bit");
		break;
		case B_FMT_24BIT:
			sprintf(str, "24 bit");
		break;
		case B_FMT_32BIT:
			sprintf(str, "32 bit");
		break;
		case B_FMT_FLOAT:
			sprintf(str, "float");
		break;
		case B_FMT_DOUBLE:
			sprintf(str, "double");
		break;
		case B_FMT_EXTENDED:
			sprintf(str, "extended");
		break;
		case B_FMT_BITSTREAM:
			sprintf(str, "bit stream");
		break;
		case B_FMT_IS_GLOBAL:
			sprintf(str, "format is global");
		break;
		case B_FMT_SAME_AS_INPUT:
			sprintf(str, "output same as input");
		break;
		default:
			sprintf(str, "bad format");
		break;
	}
	FUNCTION(("string_for_multi_format 0x%x is %s\n", format, str));
}

void 
BMultiNode::string_for_multi_lock_sources(int32 lock_source, char *str, int32 lock_data)
{
	/* str must be 32 bytes */
	switch (lock_source)
	{
		case B_MULTI_LOCK_INPUT_CHANNEL:
			sprintf(str, "input %ld", lock_data);
		break;
		case B_MULTI_LOCK_INTERNAL:
			sprintf(str, "internal");
		break;
		case B_MULTI_LOCK_WORDCLOCK:
			sprintf(str, "wordclock");
		break;
		case B_MULTI_LOCK_SUPERCLOCK:
			sprintf(str, "superclock");
		break;
		case B_MULTI_LOCK_LIGHTPIPE:
			sprintf(str, "lightpipe");
		break;
		case B_MULTI_LOCK_VIDEO:
			sprintf(str, "video");
		break;
		case B_MULTI_LOCK_FIRST_CARD:
			sprintf(str, "card 1");
		break;
		case B_MULTI_LOCK_MTC:
			sprintf(str, "MTC");
		break;
		case B_MULTI_LOCK_SPDIF:
			sprintf(str, "S/PDIF");
		break;
		default:
			sprintf(str, "bad lock_source");
		break;
	}
	FUNCTION(("string_for_multi_lock_sources 0x%x is %s\n", lock_source, str));
}

void 
BMultiNode::string_for_multi_timecode_sources(int32 timecode_source, char *str)
{
	/* str must be 32 bytes */
	switch (timecode_source)
	{
		case B_MULTI_TIMECODE_MTC:
			sprintf(str, "MTC");
		break;
		case B_MULTI_TIMECODE_VTC:
			sprintf(str, "VTC");
		break;
		case B_MULTI_TIMECODE_SMPTE:
			sprintf(str, "SMPTE");
		break;
		case B_MULTI_TIMECODE_SUPERCLOCK:
			sprintf(str, "superclock");
		break;
		case B_MULTI_TIMECODE_FIREWIRE:
			sprintf(str, "firewire");
		break;
		default:
			sprintf(str, "bad timecode_source");
		break;
	}
	FUNCTION(("string_for_multi_timecode_sources 0x%x is %s\n", timecode_source, str));
}
