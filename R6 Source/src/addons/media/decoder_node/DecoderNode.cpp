/* DecoderNode.cpp by Simon Clarke */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <OS.h>
#include <TimeSource.h>
#include <Locker.h>
#include <Autolock.h>
#include <scheduler.h>
#include <MediaFormats.h>
#include <TimedEventQueue.h>
#include "Decoder.h"
#include "DecoderNode.h"
#include <Extractor.h>

extern const int32 gCodecID;
extern const int32 gCodecVendor;

extern const float gCacheFactor;
extern const float gScaleFactor;

extern const int32 gReferenceDataRate;
extern const bigtime_t gReferenceTime320240;

extern const float gCacheFactor = 1.0;
extern const float gScaleFactor = 0.8;

extern const int32 gReferenceDataRate = 200;
extern const bigtime_t gReferenceTime320240 = 30000;

#define CALL(x...) //printf

#define VERY_EARLY -0x7FFFFFFFFFFFFFFLL

enum {
	B_INPUT_BUFFER_AVAILABLE = B_ERRORS_END+1,
	B_OUTPUT_BUFFER_AVAILABLE,
	B_CONNECTION_ESTABLISHED
};

DecoderNode::DecoderNode(BMediaAddOn *myAddOn, int32 flags) :
BBufferProducer(B_MEDIA_UNKNOWN_TYPE /* ?? */),
BBufferConsumer(B_MEDIA_UNKNOWN_TYPE /* ?? */),
BMediaNode("BCodec Node"),
fBufferQ()
{
	CALL("DecoderNode::DecoderNode()\n");

	fMediaAddOn = myAddOn;
	fFlags = flags;

	fBufferCount = 0;
	fBuffersQueued = 0;
	fBufferGroup = NULL;
	fBufferSize = 0;
	fImageData = NULL;
	fRunning = false;
	fClearNextBuffer = false;
	fOutputConnected = false;
	fInputConnected = false;
	fOutputEnabled = true;
	fAlienBufferGroup = false;
	fShouldBeSending = true;
	fNeedSyncBuffer = false;
	fSyncBuffer = NULL;
	fDecoder = NULL;
	fDecoderID = 0;
	fCurOutBuffer = NULL;
	fCurInBuffer = NULL;

	fMyLatency = 25000;
	fDownstreamLatency = 0;
	fProcessThread = -1;

	fControlPort = create_port(15, "DecoderNode port");

	fOutput.destination = media_destination::null;
	fOutput.source.port = fControlPort;
	fOutput.source.id = 0;
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;	

	fInput.source = media_source::null;
	fInput.destination.port = fControlPort;
	fInput.destination.id = 0;
	fInput.format.type = B_MEDIA_ENCODED_VIDEO;
	fInput.format.u.encoded_video = media_encoded_video_format::wildcard;	

	fProcessThread = LaunchProcessThread();
}

DecoderNode::~DecoderNode()
{
	CALL("DecoderNode::~DecoderNode()\n");
	FreeResources();
}

void DecoderNode::FreeResources()
{
	CALL("DecoderNode::FreeResources()\n");
	int32 myErr;
	close_port(fControlPort);
	wait_for_thread(fProcessThread, &myErr);
	if (fImageData) {
		free(fImageData);
		fImageData = NULL;
	};
	
	if (fCurOutBuffer) {
		fCurOutBuffer->Recycle();
		fCurOutBuffer = NULL;
	};

	if (fCurInBuffer) {
		fCurInBuffer->Recycle();
		fCurInBuffer = NULL;
	};

	fQ.FlushEvents(B_INFINITE_TIMEOUT,BTimedEventQueue::B_BEFORE_TIME,true,BTimedEventQueue::B_HANDLE_BUFFER);
	fBufferQ.FlushEvents(0, BTimedEventQueue::B_ALWAYS);
	fAlienBufferGroup = false;
	DestroyBuffers();

	if (fDecoder) {
		delete fDecoder;
		fDecoder = NULL;
		
		_AddonManager *mgr = __get_decoder_manager();
		mgr->ReleaseAddon(fDecoderID);
		fDecoderID = 0;
	}
}

status_t init_node_process_thread(void *data)
{
	((DecoderNode *)data)->MainLoop();
	return B_OK;
}

thread_id DecoderNode::LaunchProcessThread()
{
	thread_id		thread;
	int32			threadPriority;

	CALL("DecoderNode::LaunchProcessThread()\n");

	threadPriority = suggest_thread_priority(B_VIDEO_PLAYBACK);
	
	resume_thread(
		thread = spawn_thread(
			init_node_process_thread,
			"DecoderNode Process Thread",
			threadPriority,
			this));
	
	return thread;
}

bigtime_t DecoderNode::NextProcess()
{
	CALL("DecoderNode::NextProcess()\n");
	const media_timed_event *event = fQ.FindFirstMatch(VERY_EARLY,BTimedEventQueue::B_AFTER_TIME,true,BTimedEventQueue::B_HANDLE_BUFFER);
	if (event) return event->event_time;
	else return B_INFINITE_TIMEOUT;
};

#define RETURN_IF_CONNECTED		0x00000001
#define RETURN_IF_INPUT			0x00000002
#define RETURN_IF_OUTPUT		0x00000004
#define RETURN_IF_DISCONNECTED	0x00000008

status_t DecoderNode::ProcessLoop(uint32 flags)
{
	bigtime_t	eventTime,waitUntil;
	int32		eventWhat;
	void *		eventPointer;
	uint32		eventCleanup;
	int64		eventData;
	int32 		msgCode = 0;
	uint8  		mediaMsg[B_MEDIA_MESSAGE_SIZE];
	status_t	err;

	bool returnIfConnected = flags & RETURN_IF_CONNECTED;
	bool returnIfInputReady = flags & RETURN_IF_INPUT;
	bool returnIfOutputReady = flags & RETURN_IF_OUTPUT;
	bool returnIfDisconnected = flags & RETURN_IF_DISCONNECTED;

	CALL("DecoderNode::ProcessLoop()\n");
	fLoopState = 1;

	while (	(fLoopState > 0) &&
			!(fOutputConnected && returnIfConnected) &&
			!(fBufferQ.HasEvents() && returnIfInputReady) &&
			!((fBuffersQueued < fBufferCount) && returnIfOutputReady) &&
			!(!fOutputConnected && returnIfDisconnected)) {
		const media_timed_event *event = fQ.FirstEvent();
		if (event)
		{
			eventTime = event->event_time;
			eventWhat = event->type;
			eventPointer = event->pointer;
			eventCleanup = event->cleanup;
			eventData = event->bigdata;
		} else {
			eventTime = B_INFINITE_TIMEOUT;
			eventWhat = BTimedEventQueue::B_NO_EVENT;
			eventPointer = NULL;
			eventCleanup = 0;
			eventData = 0;
		}

		waitUntil = TimeSource()->RealTimeFor(eventTime,fMyLatency+fDownstreamLatency);

		err = read_port_etc(fControlPort, &msgCode, mediaMsg, B_MEDIA_MESSAGE_SIZE, B_ABSOLUTE_TIMEOUT, waitUntil);

		if (err >= 0) 
			err = HandleMessage(msgCode, mediaMsg, B_MEDIA_MESSAGE_SIZE);
		else if (err != B_TIMED_OUT) {
			printf("Error from port read %08lx:'%s'\n",err,strerror(err));
			fLoopState = err;
		} else {
			media_timed_event poppedEvent;
			fQ.RemoveFirstEvent(&poppedEvent);
			eventTime = poppedEvent.event_time;
			eventWhat = poppedEvent.type;
			eventPointer = poppedEvent.pointer;
			eventCleanup = poppedEvent.cleanup;
			eventData = poppedEvent.bigdata;
			
			switch (eventWhat) {
				case BTimedEventQueue::B_START: 
					if (!fRunning) {
						fRunning = true;
						if (fOutputConnected && fOutputEnabled)
							SendDataStatus(B_DATA_AVAILABLE, fOutput.destination, eventTime);
					}
					break;
				case BTimedEventQueue::B_STOP:
					if (fRunning) {
						fRunning = false;
						if (fOutputConnected && fOutputEnabled)
							SendDataStatus(B_DATA_NOT_AVAILABLE, fOutput.destination, eventTime);
						fQ.FlushEvents(B_INFINITE_TIMEOUT, BTimedEventQueue::B_ALWAYS, true,
							BTimedEventQueue::B_HANDLE_BUFFER);
					}
					break;
					
				case BTimedEventQueue::B_SEEK:
					// We don't have a notion of seeks
					printf("CINEPAK GOT SEEKED!!!!\n");
					break;
					
				case BTimedEventQueue::B_DATA_STATUS:
					fShouldBeSending = (eventData == B_DATA_AVAILABLE);
					break;
					
				case BTimedEventQueue::B_HANDLE_BUFFER: {
					BBuffer *buffer = (BBuffer *)eventPointer;
					if (fRunning && fOutputConnected && fOutputEnabled) {
						if ((err=SendBuffer(buffer, fOutput.destination)) != B_OK)
							buffer->Recycle();
					} else
						buffer->Recycle();
					fBuffersQueued--;
				} break;
			};
		};
	};
	
	if (fLoopState > 0) {
		if (fDecoder && returnIfConnected) {
			fLoopState = B_CONNECTION_ESTABLISHED;
		} else if ((fBuffersQueued < fBufferCount) && returnIfOutputReady) {
			fLoopState = B_OUTPUT_BUFFER_AVAILABLE;
			if (fNeedSyncBuffer && fSyncBuffer) {
				fCurOutBuffer = fSyncBuffer;
				fBufferGroup->RequestBuffer(fCurOutBuffer);
			} else {
				fCurOutBuffer = fSyncBuffer = fBufferGroup->RequestBuffer((size_t)0);
			};
		} else if (fBufferQ.HasEvents() && returnIfInputReady) {
			media_timed_event poppedBuffer;
			status_t err;
			fLoopState = B_INPUT_BUFFER_AVAILABLE;
			if (((err = fBufferQ.RemoveFirstEvent(&poppedBuffer)) == B_OK) &&
				(poppedBuffer.type == BTimedEventQueue::B_HANDLE_BUFFER)) {
				fCurInBuffer = (BBuffer*)poppedBuffer.pointer;
			} else {
				fLoopState = err;
				fCurInBuffer = NULL;
			}
		};
	};

	return fLoopState;
};

status_t DecoderNode::NextChunk(void **chunkData, size_t *chunkLen, media_header *mh)
{
	if (fCurInBuffer) {
		fCurInBuffer->Recycle();
		fCurInBuffer = NULL;
	};

	status_t err = B_INPUT_BUFFER_AVAILABLE;
	do {
		if (fCurInBuffer && (err == B_INPUT_BUFFER_AVAILABLE)) {
			*mh = *fCurInBuffer->Header();
			*chunkData = fCurInBuffer->Data();
			*chunkLen = fCurInBuffer->Header()->size_used;
			return B_OK;
		};
	} while ((err = ProcessLoop(RETURN_IF_INPUT|RETURN_IF_DISCONNECTED)) == B_INPUT_BUFFER_AVAILABLE);
	
	return err;
};

status_t DecoderNode::_NextChunk(void *userData, void **chunkData, size_t *chunkLen, media_header *mh)
{
	return ((DecoderNode*)userData)->NextChunk(chunkData,chunkLen,mh);
};

void DecoderNode::MainLoop()
{
	status_t err = B_OK;
	int64 frameCount;
	media_header mh;
	BBuffer *inBuffer,*outBuffer;

	while (err != B_BAD_PORT_ID) {
		if (!fOutputConnected) {
			err = ProcessLoop(RETURN_IF_CONNECTED);
		} else if (!fCurOutBuffer) {
			err = ProcessLoop(RETURN_IF_OUTPUT|RETURN_IF_DISCONNECTED);
		} else {
			fLoopState = 1;
			err = fDecoder->Decode(fCurOutBuffer->Data(),&frameCount, &mh, NULL);
			inBuffer = fCurInBuffer;
			outBuffer = fCurOutBuffer;

			if (inBuffer && (err == B_OK)) {
				if (inBuffer->Header()->type == B_MEDIA_ENCODED_VIDEO) {
					outBuffer->Header()->type = B_MEDIA_RAW_VIDEO;
					outBuffer->Header()->start_time = mh.start_time;
					outBuffer->Header()->size_used = fOutput.format.Height() * fOutput.format.u.raw_video.display.bytes_per_row;
					outBuffer->Header()->u.raw_video.field_gamma = mh.u.encoded_video.field_gamma;
					outBuffer->Header()->u.raw_video.field_sequence = mh.u.encoded_video.field_sequence;
					outBuffer->Header()->u.raw_video.field_number = mh.u.encoded_video.field_number;
					outBuffer->Header()->u.raw_video.line_count = mh.u.encoded_video.line_count;
					outBuffer->Header()->u.raw_video.first_active_line = mh.u.encoded_video.first_active_line;
				} else {
					outBuffer->Header()->type = B_MEDIA_RAW_AUDIO;
					outBuffer->Header()->start_time = mh.start_time;
					outBuffer->Header()->size_used =
						fOutput.format.u.raw_audio.channel_count *
						(fOutput.format.u.raw_audio.format & 0xF) *
						frameCount;
				};
				
				media_timed_event newEvent(outBuffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
						outBuffer, BTimedEventQueue::B_RECYCLE_BUFFER);
				newEvent.bigdata = outBuffer->Header()->start_time;
				fQ.AddEvent(newEvent);
				fBuffersQueued++;
			} else if (fCurOutBuffer)
				fCurOutBuffer->Recycle();
			fCurOutBuffer = NULL;
		};
	};
}

void DecoderNode::ProducerDataStatus(const media_destination & /*for_whom*/, int32 status, bigtime_t at_performance_time)
{
	media_timed_event event(at_performance_time, BTimedEventQueue::B_DATA_STATUS);
	event.bigdata = status;
	fQ.AddEvent(event);
}

bigtime_t DecoderNode::GuessOwnLatency(int32 imageWidth, int32 imageHeight)
{
	CALL("DecoderNode::GuessOwnLatency()\n");

	system_info		info;
	const float		clockSpeed = 233.0f;
	const float		refWidth = 320.0f, refHeight = 240.0f;
	float			factor, widthFactor, heightFactor;
	float			outTime;

	get_system_info(&info);

	factor = (float)((info.cpu_clock_speed / 1000000) / clockSpeed);
	if (factor != 1.0)
		factor = factor / 2;	

	if (imageWidth == 320 || imageHeight == 240) {
		widthFactor = 1.0f;
		heightFactor = 1.0f;
	} else { 
		widthFactor = (float)(imageWidth / refWidth) * gScaleFactor;
		heightFactor = (float)(imageHeight / refHeight) * gScaleFactor;
	}

	outTime = (float)gReferenceTime320240;
	outTime *= factor;
	outTime *= widthFactor;
	outTime *= heightFactor;

	return (bigtime_t)outTime;
}

//#pragma mark -

/* MediaNode calls */

port_id DecoderNode::ControlPort() const
{
	return fControlPort;
}

BMediaAddOn *DecoderNode::AddOn(int32 */*internal_id*/) const
{
	return fMediaAddOn;
}

void DecoderNode::SetRunMode(	run_mode 			mode)
{
	CALL("DecoderNode::SetRunMode() 0x%x\n", mode);

	int32		threadPriority;

	if (mode != B_OFFLINE) {
		/* set threads back to their higher priority */
		threadPriority = suggest_thread_priority(B_VIDEO_PLAYBACK);
		set_thread_priority(fProcessThread, threadPriority);
	} else {
		/* set threads to a lower priority */
		threadPriority = suggest_thread_priority(B_OFFLINE_PROCESSING);		
		set_thread_priority(fProcessThread, threadPriority);
	}
}

void DecoderNode::Start(bigtime_t performance_time)
{
	CALL("DecoderNode::Start()\n");
	media_timed_event event(performance_time, BTimedEventQueue::B_START);
	fQ.AddEvent(event);
//	fQ.PushEvent(performance_time, BTimedEventQueue::B_START, NULL, 0, 0);
}

void DecoderNode::Stop(bigtime_t performance_time, bool immediate)
{
	if (immediate) {
		fRunning = false;
		fQ.FlushEvents(B_INFINITE_TIMEOUT, BTimedEventQueue::B_ALWAYS, true,
			BTimedEventQueue::B_HANDLE_BUFFER);
	} else {
		bigtime_t qTime = performance_time;
		media_timed_event event(qTime, BTimedEventQueue::B_STOP);
		event.bigdata = performance_time;
		fQ.AddEvent(event);
	}

//	fQ.PushEvent(qTime, BTimedEventQueue::B_STOP, NULL, 0, performance_time);
}

void DecoderNode::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	CALL("DecoderNode::Seek()\n");
	media_timed_event event(performance_time, BTimedEventQueue::B_SEEK);
	event.bigdata = media_time;
	fQ.AddEvent(event);
//	fQ.PushEvent(performance_time, BTimedEventQueue::B_SEEK, NULL, 0, media_time);
}

void DecoderNode::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	CALL("DecoderNode::TimeWarp()\n");
	media_timed_event event(TimeSource()->PerformanceTimeFor(at_real_time),
		BTimedEventQueue::B_WARP);
	event.bigdata = to_performance_time;
	fQ.AddEvent(event);
//	fQ.PushEvent(TimeSource()->PerformanceTimeFor(at_real_time),
//				BTimedEventQueue::B_WARP, NULL, 0, to_performance_time);
}
void DecoderNode::Preroll()
{
	CALL("DecoderNode::Preroll()\n");
}

status_t DecoderNode::HandleMessage(
							int32 					message,
							const void 				*data,
							size_t 					size)
{
	if (BBufferConsumer::HandleMessage(message, data, size) == B_OK)
		return B_OK;

	if (BBufferProducer::HandleMessage(message, data, size) == B_OK)
		return B_OK;

	if (BMediaNode::HandleMessage(message, data, size) == B_OK)
		return B_OK;

	BMediaNode::HandleBadMessage(message, data, size);

	return B_OK;
}

//#pragma mark -

/* BufferProducer calls */

status_t DecoderNode::FormatSuggestionRequested(
								media_type				type,
								int32 					/*quality*/,
								media_format 			*format)
{
	CALL("DecoderNode::FormatSuggestionRequested()\n");
	if (type == B_MEDIA_RAW_VIDEO)
		format->u.raw_video = media_raw_video_format::wildcard;
	else if (type == B_MEDIA_RAW_AUDIO)
		format->u.raw_audio = media_raw_audio_format::wildcard;

	return B_OK;
}

status_t DecoderNode::FormatProposal(
								const media_source 		&/*output*/,
								media_format 			*format)
{
	CALL("DecoderNode::FormatProposal() ************************************* %d\n",format->u.raw_video.display.bytes_per_row);
	media_format f = *format;
	if (fDecoder) fDecoder->Format(format);
	if (format->Matches(&f)) return B_OK;
	return B_MEDIA_BAD_FORMAT;
}

status_t DecoderNode::FormatChangeRequested(
								const media_source 		&/*source*/,
								const media_destination &/*destination*/,
								media_format 			*/*io_format*/,
								int32 					*/*out_change_count*/)
{
	CALL("DecoderNode::FormatChangeRequested()\n");

	return B_OK;
}

status_t DecoderNode::GetNextOutput(
								int32 					*cookie,
								media_output 			*out_output)
{
	CALL("DecoderNode::GetNextOutput()\n");

	if (*cookie != 0) return B_ERROR;
	fOutput.node = Node();
	*out_output = fOutput;
	if ((fOutput.destination == media_destination::null) && fInputConnected) {
		if (fInput.format.type == B_MEDIA_ENCODED_VIDEO) {
			out_output->format.type = B_MEDIA_RAW_VIDEO;
			out_output->format.ColorSpace() = (color_space)0;
			out_output->format.Height() = fInput.format.Height();
			out_output->format.Width() = fInput.format.Width();
		} else if (fInput.format.type == B_MEDIA_ENCODED_AUDIO) {
			out_output->format.type = B_MEDIA_RAW_AUDIO;
			out_output->format.u.raw_audio = media_raw_audio_format::wildcard;
		}
	}
	strcpy(out_output->name,"codec output");
	*cookie = 1;
	return B_OK;
}

status_t DecoderNode::DisposeOutputCookie(
								int32 					/*cookie*/)
{
	return B_OK;
}

status_t DecoderNode::SetBufferGroup(
								const media_source 		&/*for_source*/,
								BBufferGroup 			*group)
{	
	CALL("DecoderNode::SetBufferGroup()\n");

	if (fBufferGroup) {
		fQ.FlushEvents(B_INFINITE_TIMEOUT,BTimedEventQueue::B_BEFORE_TIME,true,BTimedEventQueue::B_HANDLE_BUFFER);
		if (fCurOutBuffer) {
			fCurOutBuffer->Recycle();
			fCurOutBuffer = NULL;
		};
		delete fBufferGroup;
	};
	fBufferGroup = group;
	if (group) {
		fAlienBufferGroup = true;
		fBufferCount = 1; // pretend there is only one
	} else {
		fAlienBufferGroup = false;
		fBufferCount = 0;
	};
	return B_OK;
}

status_t DecoderNode::VideoClippingChanged(
								const media_source 		&/*for_source*/,
								int16 					/*num_shorts*/,
								int16 					*/*clip_data*/,
								const media_video_display_info &/*display*/,
								int32 					*/*out_from_change_count*/)
{
	return B_ERROR;
}

status_t DecoderNode::PrepareToConnect(
								const media_source 		&/*what*/,
								const media_destination &/*where*/,
								media_format 			*format,
								media_source 			*out_source,
								char 					*/*io_name*/)

{
	CALL("DecoderNode::PrepareToConnect()************************************* %d\n",format->u.raw_video.display.bytes_per_row);

	media_format f = *format;
	if (fDecoder) fDecoder->Format(format);
	else {
		format->type = B_MEDIA_RAW_VIDEO;
		format->Height() = fInput.format.Height();
		format->Width() = fInput.format.Width();
	};
	
	if (!format->Matches(&f)) return B_MEDIA_BAD_FORMAT;

	fOutput.format = *format;
	*out_source = fOutput.source;
	return B_OK;
}

void DecoderNode::Connect(		status_t 				/*error*/, // Hmmmmm really should check this...
								const media_source 		&source,
								const media_destination &destination,
								const media_format 		&format,
								char 					*/*out_name*/)
{
	media_node_id		timeSource;

	fOutput.node = Node();
	fOutput.source = source;
	fOutput.destination = destination;
	fOutput.format = format;

	/* GetLatency() does not work? - do it manually */
	FindLatencyFor(destination, &fDownstreamLatency, &timeSource);

	if (fDecoder)
		fDecoder->Format(&fOutput.format);
	fNeedSyncBuffer = (fOutput.format.type == B_MEDIA_RAW_VIDEO);
	if (fInputConnected)
		 CreateBuffers();
	fOutputConnected = true;
}

void DecoderNode::Disconnect(	const media_source 		&/*what*/,
								const media_destination &/*where*/)
{
	CALL("DecoderNode::Disconnect() - id:%ld\n", what.id);
	
	fQ.FlushEvents(B_INFINITE_TIMEOUT,BTimedEventQueue::B_BEFORE_TIME, true,
		BTimedEventQueue::B_HANDLE_BUFFER);
	fBufferQ.FlushEvents(0, BTimedEventQueue::B_ALWAYS);
	if (fCurOutBuffer) {
		fCurOutBuffer->Recycle();
		fCurOutBuffer = NULL;
	};
	fBuffersQueued = 0;
	
	if (fAlienBufferGroup) {
		fAlienBufferGroup = false;
		DestroyBuffers();
	} else if (!fInputConnected) 
		DestroyBuffers();

	fOutputConnected = false;
	fOutput.node = Node();
	fOutput.destination = media_destination::null;
	fOutput.source.port = fControlPort;
	fOutput.source.id = 0;
	fOutput.format.type = B_MEDIA_ENCODED_VIDEO;
	fOutput.format.u.encoded_video = media_encoded_video_format::wildcard;	
}

void DecoderNode::LateNoticeReceived(
								const media_source 		&/*what*/,
								bigtime_t 				/*how_much*/,
								bigtime_t 				/*performance_time*/)
{
	fMyLatency += 5000;
}

void DecoderNode::EnableOutput(const media_source 		&/*what*/,
								bool 					enabled,
								int32 					*/*change_tag*/)
{
	CALL("DecoderNode::EnableOutput()\n");
	fOutputEnabled = enabled;
}

//#pragma mark -

Decoder *
find_decoder(const media_format *fmt, int32 *id, const void *metaData, int32 metaDataSize)
{
	_AddonManager *mgr = __get_decoder_manager();
	image_id       imgid;
	Decoder     *(*make_decoder)(void);
	Decoder		  *decoder = NULL;
	int32          cookie=0;


	while ((imgid = mgr->GetNextAddon(&cookie, id)) > 0) {

		if (get_image_symbol(imgid, "instantiate_decoder",
							 B_SYMBOL_TYPE_TEXT,
							 (void **)&make_decoder) != B_OK) {
			mgr->ReleaseAddon(*id);
			continue;
		}
		
		decoder = make_decoder();
		if (decoder == NULL) {
			mgr->ReleaseAddon(*id);
			continue;
		}

		//
		// XXXdbg -- need a way to get the extra track info to
		// pass to the decoder.
		//
		if (decoder->Sniff(fmt, metaData, metaDataSize) == B_OK) {
			bigtime_t time = 0;
			int64	frame;
			if(decoder->Reset(B_SEEK_BY_TIME, frame, &frame, time, &time) == B_OK)
				break; // got it!
		}


		delete decoder;
		mgr->ReleaseAddon(*id);
		decoder = NULL;
	}
	
	return decoder;
}


/* BufferConsumer calls */

status_t DecoderNode::AcceptFormat(
								const media_destination &/*dest*/,
								media_format 			*format)
{
	if (fDecoder) {
		delete fDecoder;
		fDecoder = NULL;
		
		_AddonManager *mgr = __get_decoder_manager();
		mgr->ReleaseAddon(fDecoderID);
		fDecoderID = 0;
	}

	const void *meta = format->MetaData();
	int32 metaSize = format->MetaDataSize();

	fDecoder = find_decoder(format, &fDecoderID, meta, metaSize);
	if (fDecoder == NULL)
		return B_MEDIA_BAD_FORMAT;
	
	fCurFmt = *format;
	return B_OK;
}

status_t DecoderNode::GetNextInput(
								int32 					*cookie,
								media_input 			*out_input)
{
	CALL("DecoderNode::GetNextInput()\n");

	if (fInput.source != media_source::null) {
		if (*cookie != 0) return B_ERROR;
		fInput.node = Node();
		*out_input = fInput;
		strcpy(out_input->name,"codec input");
		*cookie = 1;
	} else {
		if (*cookie > 1) return B_ERROR;
		fInput.node = Node();
		*out_input = fInput;
		if (*cookie == 0) {
			out_input->format.type = B_MEDIA_ENCODED_VIDEO;
			out_input->format.u.encoded_video = media_encoded_video_format::wildcard;
		} else {
			out_input->format.type = B_MEDIA_ENCODED_AUDIO;
			out_input->format.u.encoded_audio = media_encoded_audio_format::wildcard;
		};
		strcpy(out_input->name,"codec input");
		(*cookie)++;
	};

	return B_OK;
}

void DecoderNode::DisposeInputCookie(int32 /*cookie*/)
{
}

void DecoderNode::BufferReceived(BBuffer *buffer)
{
	if (!fRunning || !fOutputConnected) {
		buffer->Recycle();
		return;
	};

	/*	Now, if the codec node seeked, the performance time may have gone
		_backwards_.  This would cause us to decode them in the wrong order,
		and that would look like crap.  To prevent that, remove any buffers
		that are "newer" than this buffer.  Because the mentioned case will
		only happen during a seek, this should be okay. */
	
	fBufferQ.FlushEvents(buffer->Header()->start_time, BTimedEventQueue::B_AFTER_TIME, true, BTimedEventQueue::B_HANDLE_BUFFER);

	media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
	fBufferQ.AddEvent(event);
}
							
status_t DecoderNode::GetLatencyFor(
								const media_destination &/*for_whom*/,
								bigtime_t 				*out_latency,
								media_node_id 			*out_timesource)
{
	*out_timesource = TimeSource()->ID();
	*out_latency = fMyLatency + fDownstreamLatency;
	return B_OK;
}

void DecoderNode::CreateBuffers()
{
	if (fOutput.format.type == B_MEDIA_RAW_AUDIO)
		fBufferSize = fOutput.format.u.raw_audio.buffer_size;
	else if (fOutput.format.type == B_MEDIA_RAW_VIDEO)
		fBufferSize = fOutput.format.Height() * fOutput.format.u.raw_video.display.bytes_per_row;
//	if (!fImageData) fImageData = malloc(fBufferSize);
	if (fAlienBufferGroup) return;
	fBufferCount = 3;
	fBufferGroup = new BBufferGroup(fBufferSize,fBufferCount);
};

void DecoderNode::DestroyBuffers()
{
	if (fAlienBufferGroup) return;
	if (fBufferGroup) delete fBufferGroup;
	fBufferGroup = NULL;
	fBufferSize = 0;
	fBufferCount = 0;
};

status_t DecoderNode::Connected(
							const media_source 		&producer,
							const media_destination &where,
							const media_format 		&with_format,
							media_input 			*out_input)
{
	CALL("DecoderNode::Connected()\n");

//	media_node_id timeSource;

	fInput.node = Node();
	fInput.source = producer;
	fInput.destination = where;
	fInput.destination.id = 0;
	fInput.format = with_format;
	
	if (memcmp(&fCurFmt, &with_format, sizeof(media_format)) != 0) {
		if (fDecoder) {
			delete fDecoder;
			fDecoder = NULL;
		
			_AddonManager *mgr = __get_decoder_manager();
			mgr->ReleaseAddon(fDecoderID);
			fDecoderID = 0;
		}

		fDecoder = find_decoder(&with_format, &fDecoderID, with_format.MetaData(), with_format.MetaDataSize());
		if (fDecoder == NULL)
			return B_ERROR;
	}
	fCurFmt = with_format;

	if (fDecoder == NULL)
		return B_ERROR;

	fDecoder->fNextChunk = DecoderNode::_NextChunk;
	fDecoder->fUserData = this;

	if (fOutputConnected)
		CreateBuffers();
	fInputConnected = true;
	*out_input = fInput;
	
	return B_OK;
}

void DecoderNode::Disconnected(	const media_source 		&/*producer*/,
								const media_destination &/*where*/)
{
	CALL("DecoderNode::Disconnected()\n");
	
	fBufferQ.FlushEvents(0, BTimedEventQueue::B_ALWAYS);
	if (fCurInBuffer) {
		fCurInBuffer->Recycle();
		fCurInBuffer = NULL;
	};

	if (!fOutputConnected) {
		fQ.FlushEvents(B_INFINITE_TIMEOUT,BTimedEventQueue::B_BEFORE_TIME,true,BTimedEventQueue::B_HANDLE_BUFFER);
		if (fCurOutBuffer) {
			fCurOutBuffer->Recycle();
			fCurOutBuffer = NULL;
		};
		fBuffersQueued = 0;
		DestroyBuffers();
		delete fDecoder;
		fDecoder = NULL;
	};
	fInputConnected = false;

	fInput.node = Node();
	fInput.source = media_source::null;
	fInput.destination.port = fControlPort;
	fInput.destination.id = 0;
	fInput.format.type = B_MEDIA_RAW_VIDEO;
	fInput.format.u.raw_video = media_raw_video_format::wildcard;	
}

status_t DecoderNode::FormatChanged(
							const media_source 		&/*producer*/,
							const media_destination &/*consumer*/, 
							int32 					/*from_change_count*/,
							const media_format 		&/*format*/)
{
	CALL("DecoderNode::FormatChanged()\n");
	return B_OK;
}

/***************************************************************************/

/* Useful(?) debugging stuff */

void DecoderNode::PrintBufferGroup(BBufferGroup *group)
{
	int32			count, i;
	BBuffer			**buffers, *buffer;
	
	CALL("DecoderNode::PrintBufferGroup()\n");

	if (group->CountBuffers(&count) == B_OK) {
		buffers = (BBuffer **)malloc(sizeof(void *) * count);
		if (group->GetBufferList(count, buffers) == B_OK) {
			for (i = 0; i < count; i++) {
				buffer = buffers[i];
				if (buffer)
					printf("%ld: id:%ld  d:%p s:%ld f:%4lx\n", i, buffer->ID(), buffer->Data(), buffer->Size(), buffer->Flags());
			}
		}
		free(buffers);
	}
}
