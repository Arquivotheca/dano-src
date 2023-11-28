//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS
//	VideoConsumer.cpp

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <Buffer.h>
#include <string.h>
#include <scheduler.h>
#include <TimeSource.h>
#include <MediaRoster.h>
#include <Application.h>
#include <BufferGroup.h>
#include <MediaNode.h>

#include "VideoConsumer.h"

#define M1 ((double)1000000.0)
#define JITTER		20000

#if DEBUG
#define PRINTF		printf
#else
#define PRINTF		(void)
#endif

#define	FUNCTION	//PRINTF
#define ERROR		//PRINTF
#define PROGRESS	//PRINTF
#define PULSE		//PRINTF
#define LOOP		//PRINTF

// Remove for R4.1!
//#include "rt_allocator.h"
//extern rtm_pool * _rtm_pool;
// Remove for R4.1!

media_raw_video_format vid_format = { 29.97,1,0,239,B_VIDEO_TOP_LEFT_RIGHT,1,1,{B_RGB32,320,240,320*4,0,0}};

//---------------------------------------------------------------

VideoConsumer::VideoConsumer(const uint32 internal_id, const char * name, BMediaAddOn *addon) :
	BMediaNode(name),
	BBufferConsumer(B_MEDIA_RAW_VIDEO),
	mInternalID(internal_id),
	mAddOn(addon),
	mDisplayThread(0),
	mDisplayQuit(false),		
	mRunning(false),	
	mConnectionCount(0),
	mMyLatency(20000)
{
	FUNCTION("VideoConsumer::VideoConsumer\n");

	// Remove for R4.1
//	rtm_create_pool(&_rtm_pool, (B_PAGE_SIZE * 2));
	// Remove for R4.1

	AddNodeKind(B_PHYSICAL_OUTPUT);

	mPort = create_port(3, "VideoWindow Port");
	
	for (int connection = 0; connection < MAX_CONNECTIONS; connection++)
	{
		mConnectionActive[connection] = false;
		mFormat[connection] = vid_format;
		mWindow[connection] = NULL;
		mView[connection] = NULL;
		mOurBuffers[connection] = false;
		mBuffers[connection] = NULL;
		mSource[connection] = media_source::null;
		mDestination[connection].port = mPort;
		mDestination[connection].id = connection;
		sprintf(mName[connection], "Video Window %d", connection+1);
		for (uint32 j = 0; j < 3; j++)
		{
			mBitmap[connection][j] = NULL;
			mBufferMap[connection][j] = 0;
		}
	}
	
	// create a buffer queue and buffer available semaphore
	mEventQueue = new BTimedEventQueue;
	
	// start the drawing thread
	int drawPrio = suggest_thread_priority(B_VIDEO_PLAYBACK, 30, JITTER, 5000);
	PROGRESS("Suggested draw thread priority is %d\n", drawPrio);
	mDisplayThread = spawn_thread(vRun, "Video Window Draw", drawPrio, this);
	resume_thread(mDisplayThread);

}

//---------------------------------------------------------------

VideoConsumer::~VideoConsumer()
{
	FUNCTION("VideoConsumer::~VideoConsumer\n");
	status_t status;
	
	// signal the helper thread to quit
	mDisplayQuit = true;
	close_port(mPort);
	
	// wait for it to die
	if (mDisplayThread != 0)
		wait_for_thread(mDisplayThread, &status);

	mEventQueue->FlushEvents(B_INFINITE_TIMEOUT, BTimedEventQueue::B_BEFORE_TIME);	
	for (int connection = 0; connection < MAX_CONNECTIONS; connection++)
	{
		DeleteWindow(connection);
		DeleteBuffers(connection);
	}

	delete_port(mPort);
	delete mEventQueue;
	
	// remove for R4.1
//	rtm_delete_pool(_rtm_pool);
	// remove for R4.1
}

/********************************
	From BMediaNode
********************************/

//---------------------------------------------------------------

BMediaAddOn *
VideoConsumer::AddOn(long *cookie) const
{
	FUNCTION("VideoConsumer::AddOn\n");
	*cookie = mInternalID;
	return mAddOn;
}

//---------------------------------------------------------------

port_id
VideoConsumer::ControlPort() const
{
	FUNCTION("VideoConsumer::ControlPort\n");
	return mPort;
}


//---------------------------------------------------------------

void
VideoConsumer::Start(
	bigtime_t performance_time)
{
	FUNCTION("VideoConsumer::Start @ %.4f, now: %.4f\n",
		(double)performance_time/M1, (double)TimeSource()->Now()/M1);
	
	mEventQueue->PushEvent(performance_time, BTimedEventQueue::B_START, NULL, BTimedEventQueue::B_NO_CLEANUP, 0);
}


//---------------------------------------------------------------

void
VideoConsumer::Stop(
	bigtime_t performance_time,
	bool immediate)
{
	FUNCTION("VideoConsumer::Stop @ %.4f, now: %.4f\n",
		(double)performance_time/M1, (double)TimeSource()->Now()/M1);
	
	bigtime_t timeToStop = performance_time;
	if (immediate)
		timeToStop = TimeSource()->PerformanceTimeFor(BTimeSource::RealTime());
	mEventQueue->PushEvent(timeToStop, BTimedEventQueue::B_STOP, NULL, BTimedEventQueue::B_NO_CLEANUP, 0);
}


//---------------------------------------------------------------

void
VideoConsumer::Seek(
	bigtime_t media_time,
	bigtime_t performance_time)
{
	FUNCTION("VideoConsumer::Seek\n");
	
	/* we only play in performance_time -- we don't seek */
}


//---------------------------------------------------------------

void
VideoConsumer::TimeWarp(bigtime_t at_real_time,
	bigtime_t performance_time)
{
	FUNCTION("VideoConsumer::TimeWarp perf time %.4f @ %.4f\n",
		(double)performance_time/M1, (double)at_real_time/M1);
	
}

//---------------------------------------------------------------

#ifndef R4_MK_API
status_t
VideoConsumer::RequestCompleted(const media_request_info & info)
{
	FUNCTION("VideoConsumer::RequestCompleted\n");
	switch( info.what)
	{
		case media_request_info::B_SET_VIDEO_CLIPPING_FOR:
			PROGRESS("   B_SET_VIDEO_CLIPPING_FOR\n");
			return B_OK;
		case media_request_info::B_REQUEST_FORMAT_CHANGE:
			PROGRESS("   B_REQUEST_FORMAT_CHANGE\n");
			return B_OK;
		case media_request_info::B_SET_OUTPUT_ENABLED:
			PROGRESS("   B_SET_OUTPUT_ENABLED\n");
			return B_OK;
		case media_request_info::B_SET_OUTPUT_BUFFERS_FOR:
			PROGRESS("   B_SET_OUTPUT_BUFFERS\n");
			if (info.status == B_OK)
				mOurBuffers[(uint32)(info.user_data)] = true;
			else
				mOurBuffers[(uint32)(info.user_data)] = false;
			return B_OK;
		case media_request_info::B_FORMAT_CHANGED:
			PROGRESS("   B_FORMAT_CHANGE_REQUESTED\n");
			return B_OK;
		default:
			ERROR("   UNIMPLEMENTED 'WHAT'\n");
			return B_ERROR;
	}
}
#endif

//---------------------------------------------------------------

status_t
VideoConsumer::HandleMessage(int32 message, const void * data, size_t size)
{
	status_t status = B_OK;
	
//fprintf(stderr, "VideoConsumer::HandleMessage(%x)\n", message);
//
	if ((status = BMediaNode::HandleMessage(message, data, size)) != B_OK)
		if ((status = BBufferConsumer::HandleMessage(message, data, size)) != B_OK)
			{
				BMediaNode::HandleBadMessage(message, data, size);
				status = B_ERROR;
			}
			
	return status;
}

//---------------------------------------------------------------

void
VideoConsumer::BufferReceived(BBuffer * buffer)
{
	uint32 connection = buffer->Header()->destination;
	
	LOOP("VideoConsumer::Buffer #%d received for connection %d\n", buffer->ID(), connection);

	if (!mRunning)
	{
		buffer->Recycle();
		return;
	}

	mEventQueue->PushEvent(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
						buffer, BTimedEventQueue::B_NO_CLEANUP, 0);

}


//---------------------------------------------------------------

void
VideoConsumer::ProducerDataStatus(
	const media_destination &,
	int32 status,
	bigtime_t)
{
	FUNCTION("VideoConsumer::ProducerDataStatus()\n");
	
}

//---------------------------------------------------------------

void
VideoConsumer::CreateWindow(
	const char *name,
	const uint32 connection,
	const media_format & with_format)
{
	// create a new window for this connection
	uint32 mXSize = with_format.u.raw_video.display.line_width;
	uint32 mYSize = with_format.u.raw_video.display.line_count;	

	mWindow[connection] = new VideoWindow(BRect(40 + connection*10, 40 + connection*10, 40 + connection*10 + (mXSize-1), 40 + connection*10 + (mYSize - 1)),
								 name, B_TITLED_WINDOW, B_NOT_CLOSABLE | B_NOT_MINIMIZABLE);
	mView[connection]	= new BView(BRect(0, 0, (mXSize-1),(mYSize - 1)), "Play View", B_FOLLOW_ALL, B_WILL_DRAW);
	mWindow[connection]->AddChild(mView[connection]);
	mWindow[connection]->Show();
}

//---------------------------------------------------------------

void
VideoConsumer::DeleteWindow(
	const uint32 connection)	
{
	if (mWindow[connection] != 0)
		if (mWindow[connection]->Lock())
		{
			mWindow[connection]->Close();
			mWindow[connection] = 0;
		}
}


//---------------------------------------------------------------

status_t
VideoConsumer::CreateBuffers(
	const uint32 connection,
	const media_format & with_format)
{
	FUNCTION("VideoConsumer::CreateBuffers\n");
	status_t status = B_OK;

	// create a buffer group
	uint32 mXSize = with_format.u.raw_video.display.line_width;
	uint32 mYSize = with_format.u.raw_video.display.line_count;	
	uint32 mRowBytes = with_format.u.raw_video.display.bytes_per_row;
	color_space mColorspace = with_format.u.raw_video.display.format;
	PROGRESS("VideoConsumer::CreateBuffers - Colorspace = %d\n", mColorspace);

	mBuffers[connection] = new BBufferGroup();
	status = mBuffers[connection]->InitCheck();
	if (B_OK != status)
	{
		ERROR("VideoConsumer::CreateBuffers - ERROR CREATING BUFFER GROUP\n");
		return status;
	}
	// and attach the  bitmaps to the buffer group
	for (uint32 j=0; j < 3; j++)
	{
		mBitmap[connection][j] = new BBitmap(BRect(0, 0, (mXSize-1), (mYSize - 1)), mColorspace, false, true);
		if (mBitmap[connection][j]->IsValid())
		{						
			buffer_clone_info info;
			if ((info.area = area_for(mBitmap[connection][j]->Bits())) == B_ERROR)
				ERROR("VideoConsumer::CreateBuffers - ERROR IN AREA_FOR\n");;
			info.offset = 0;
			info.size = (size_t)mBitmap[connection][j]->BitsLength();
			info.flags = j;
			info.buffer = 0;

			if ((status = mBuffers[connection]->AddBuffer(info)) != B_OK)
			{
				ERROR("VideoConsumer::CreateBuffers - ERROR ADDING BUFFER TO GROUP\n");
				return status;
			} else PROGRESS("VideoConsumer::CreateBuffers - SUCCESSFUL ADD BUFFER TO GROUP\n");
		}
		else 
		{
			ERROR("VideoConsumer::CreateBuffers - ERROR CREATING VIDEO RING BUFFER: %08x\n", status);
			return B_ERROR;
		}	
	}
	
	BBuffer ** buffList = new BBuffer * [3];
	for (int j = 0; j < 3; j++) buffList[j] = 0;
	
	if ((status = mBuffers[connection]->GetBufferList(3, buffList)) == B_OK)					
		for (int j = 0; j < 3; j++)
			if (buffList[j] != NULL)
			{
				mBufferMap[connection][j] = (uint32) buffList[j];
				PROGRESS(" j = %d buffer = %08x\n", j, mBufferMap[connection][j]);
			}
			else
			{
				ERROR("VideoConsumer::CreateBuffers ERROR MAPPING RING BUFFER\n");
				return B_ERROR;
			}
	else
		ERROR("VideoConsumer::CreateBuffers ERROR IN GET BUFFER LIST\n");
		
	FUNCTION("VideoConsumer::CreateBuffers - EXIT\n");
	return status;
}

//---------------------------------------------------------------

void
VideoConsumer::DeleteBuffers(
	const uint32 connection)
{
	FUNCTION("VideoConsumer::DeleteBuffers\n");
	status_t status;
	
	if (mBuffers[connection] != NULL)
	{
		mBuffers[connection]->ReclaimAllBuffers();
		delete mBuffers[connection];
		mBuffers[connection] = NULL;
		
		for (uint32 j = 0; j < 3; j++) {
			PROGRESS("connection: %ld j: %ld mBitmap[%ld][%ld]: 0x%x\n",
				connection, j, connection, j, mBitmap[connection][j]);

			
			if (mBitmap[connection][j] &&  mBitmap[connection][j]->IsValid())
			{
				delete mBitmap[connection][j];
				mBitmap[connection][j] = NULL;
			}
		}
	}
}

//---------------------------------------------------------------

status_t
VideoConsumer::Connected(
	const media_source & producer,
	const media_destination & where,
	const media_format & with_format,
	media_input * out_input)
{
	FUNCTION("VideoConsumer::Connected\n");

	uint32 connection;
	
	// find an unused connection
	for (connection = 0; connection < MAX_CONNECTIONS; connection++)
		if (!mConnectionActive[connection])
			break;
			
	if (connection == MAX_CONNECTIONS) {
//fprintf(stderr, "VideoConsumer::Connected() too many connections\n");
		return B_MEDIA_NOT_CONNECTED;
	}
		
//fprintf(stderr, "VideoConsumer::Connected()\n");
	
	// fill out the media input struct
	out_input->node = Node();
	out_input->source = producer;
	out_input->destination = mDestination[connection];
	out_input->format = with_format;
	if (!out_input->name[0]) {
		sprintf(out_input->name, "Video Window %d", connection + 1);
	}
	strncpy(mName[connection], out_input->name, NAME_SIZE);
	mName[connection][NAME_SIZE-1] = 0;

	mSource[connection] = producer;
	mFormat[connection] = with_format.u.raw_video;
		
	CreateWindow((const char *)out_input->name, connection, with_format);
	
#ifdef R4_MK_API
	if (CreateBuffers(connection, with_format) == B_OK)
		BMediaRoster::Roster()->SetOutputBuffersFor(producer, mBuffers[connection], true);
#else
	int32 change_tag = 1;	
	if (CreateBuffers(connection, with_format) == B_OK)
		BBufferConsumer::SetOutputBuffersFor(producer, mDestination[connection], 
											mBuffers[connection], (void *)connection, &change_tag, true);
#endif
	else
	{
		ERROR("VideoConsumer::Connected - COULDN'T CREATE BUFFERS\n");
//fprintf(stderr, "VideoConsumer::Connected() couldn't create buffers\n");
		return B_ERROR;
	}

	mConnectionActive[connection] = true;
	mConnectionCount++;	
		
	FUNCTION("VideoConsumer::Connected - EXIT\n");
	return B_OK;
}

//---------------------------------------------------------------

void
VideoConsumer::Disconnected(
	const media_source & producer,
	const media_destination & where)
{
	FUNCTION("VideoConsumer::Disconnect\n");

	uint32 connection = where.id;
	
	PROGRESS("VideoConsumer::Disconnect Connection #%d\n", connection);
	
	DeleteWindow(connection);
	DeleteBuffers(connection);	

	mConnectionActive[connection] = false;
	mSource[connection] = media_source::null;
	sprintf(mName[connection], "Video Window %d", connection+1);
	mConnectionCount--;
}

//---------------------------------------------------------------

status_t 
VideoConsumer::AcceptFormat(const media_destination  &dest, media_format *format)
{
	char format_string[256];		
	string_for_format(*format, format_string, 256);

	FUNCTION("VideoConsumer::AcceptFormat: %s\n", format_string);

	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;
	
	if (format->type != B_MEDIA_RAW_VIDEO)
	{	
		format->u.raw_video = vid_format;
//fprintf(stderr, "VideoConsumer::AcceptFormat() not RAW_VIDEO\n");
		return B_MEDIA_BAD_FORMAT;
	}

	bool goodFormat = true;

	//field_rate
	if (format->u.raw_video.field_rate == media_raw_video_format::wildcard.field_rate)
		format->u.raw_video.field_rate = 29.97;
	if (format->u.raw_video.field_rate < 1 || format->u.raw_video.field_rate > 60)
	{
		format->u.raw_video.field_rate = 29.97;	
		goodFormat = false;
	}
	
	if (format->u.raw_video.interlace == media_raw_video_format::wildcard.interlace)
		format->u.raw_video.interlace = 1;
		
	if (format->u.raw_video.interlace > 2)
	{
		format->u.raw_video.interlace = 1;
		goodFormat = false;
	}

	if (format->u.raw_video.first_active == media_raw_video_format::wildcard.first_active)
		format->u.raw_video.first_active = 0;
	if (format->u.raw_video.first_active != 0)
	{
		format->u.raw_video.first_active = 0;
		goodFormat = false;
	}

	if (format->u.raw_video.orientation == media_raw_video_format::wildcard.orientation)
		format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	if (format->u.raw_video.orientation != B_VIDEO_TOP_LEFT_RIGHT)
	{
		format->u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
		goodFormat = false;
	}

	if (format->u.raw_video.pixel_width_aspect == media_raw_video_format::wildcard.pixel_width_aspect)
		format->u.raw_video.pixel_width_aspect = 1;
#if 0
	if (format->u.raw_video.pixel_width_aspect != 1)
	{
		format->u.raw_video.pixel_width_aspect = 1;
		goodFormat = false;
	}
#endif
	
	if (format->u.raw_video.pixel_height_aspect == media_raw_video_format::wildcard.pixel_height_aspect)
		format->u.raw_video.pixel_height_aspect = 1;
#if 0
	if (format->u.raw_video.pixel_height_aspect != 1)
	{
		format->u.raw_video.pixel_height_aspect = 1;
		goodFormat = false;
	}
#endif
	
	if (format->u.raw_video.display.format == media_video_display_info::wildcard.format)
		format->u.raw_video.display.format = B_RGB32;
	
	// if we can't draw bitmaps in this color space don't accept it
	uint32 supportFlags = 0;
	bool colorSpaceSupported = bitmaps_support_space(format->u.raw_video.display.format, &supportFlags);
	if (!colorSpaceSupported || !(supportFlags & B_VIEWS_SUPPORT_DRAW_BITMAP) )
	{
		format->u.raw_video.display.format = B_RGB32;
		goodFormat = false;
	}

	if (format->u.raw_video.display.pixel_offset == media_video_display_info::wildcard.pixel_offset)
		format->u.raw_video.display.pixel_offset = 0;
	if (format->u.raw_video.display.pixel_offset != 0)
	{
		format->u.raw_video.display.pixel_offset = 0;
		goodFormat = false;
	}

	if (format->u.raw_video.display.line_offset == media_video_display_info::wildcard.line_offset)
		format->u.raw_video.display.line_offset = 0;
		
#if 0
	if (format->u.raw_video.display.line_offset != 0)
	{
		format->u.raw_video.display.line_offset = 0;
		goodFormat = false;
	}
#endif
	
	// last_active <= line_count -1
	if (format->u.raw_video.last_active == media_raw_video_format::wildcard.last_active)
	{
		if (format->u.raw_video.display.line_count == media_video_display_info::wildcard.line_count)
		{
			// both are wildcards so lets pick a line count
			format->u.raw_video.display.line_count = 240;
		}
		// set the last_active based on the line count
		format->u.raw_video.last_active = format->u.raw_video.display.line_count -1;
	
	}
	else {
		if (format->u.raw_video.display.line_count == media_video_display_info::wildcard.line_count)
			format->u.raw_video.display.line_count = format->u.raw_video.last_active + 1;
		
		if (format->u.raw_video.last_active > format->u.raw_video.display.line_count -1)
		{
			ERROR("raw_video.last_active beyond the size of the display!\n");
			format->u.raw_video.last_active = format->u.raw_video.display.line_count -1;
			goodFormat = false;
		}
		else {
//			fClipBitmap = true;
//			INFO("we need to clip the bitmap\n");
		}
	}
	
	// bytes_per_row = line_width * bytes_per_colorspace
	
	//determine size of a pixel in bytes
	size_t pixelChunk = 0;
	size_t rowAlignment = 0;
	size_t pixelsPerChunk = 0;
	status_t err = get_pixel_size_for(format->u.raw_video.display.format, &pixelChunk,
		&rowAlignment, &pixelsPerChunk);
		
	int pixelSize = (int)(pixelChunk/pixelsPerChunk);
	
	if (format->u.raw_video.display.line_width == media_video_display_info::wildcard.line_width)
	{
		if (format->u.raw_video.display.bytes_per_row == media_video_display_info::wildcard.bytes_per_row)
		{
			format->u.raw_video.display.line_width = 320;
			format->u.raw_video.display.bytes_per_row = 320 * pixelSize;
		}
		
		else {
			// bytes per row defined - use it to determine line width
			format->u.raw_video.display.line_width = (uint32)(format->u.raw_video.display.bytes_per_row / pixelSize);
		}
	}
	else {
		if (format->u.raw_video.display.bytes_per_row == media_video_display_info::wildcard.bytes_per_row)
		{
			format->u.raw_video.display.bytes_per_row = format->u.raw_video.display.line_width * pixelSize;
		}
		else {
			if (format->u.raw_video.display.line_width !=
				(uint32)(format->u.raw_video.display.bytes_per_row/pixelSize))
			{
				// set bytes per row based on line_width
				format->u.raw_video.display.bytes_per_row = format->u.raw_video.display.line_width * pixelSize;
				goodFormat = false;
			}
		}
	}
		
	string_for_format(*format, format_string, 256);

	FUNCTION("AcceptFormat %s: %s\n", (goodFormat)?"OK":"BadFormat", format_string);
	
//fprintf(stderr, "VideoConsumer::AcceptFormat() %s %s\n", format_string, goodFormat ? "is good" : "is bad");

	if (goodFormat)
		return B_OK;
	else 
		return B_MEDIA_BAD_FORMAT;
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetNextInput(
	int32 * cookie,
	media_input * out_input)
{
	FUNCTION("VideoConsumer::GetNextInput\n");
	if (*cookie < 0) return B_BAD_INDEX;

	bool seenFree = false;
	for (int ix=0; ix<*cookie; ix++) {
		if (mSource[ix] == media_source::null) {
			seenFree = true;
			break;
		}
	}
	while (seenFree && (*cookie < MAX_CONNECTIONS) && !mConnectionActive[*cookie]) {
		(*cookie)++;
	}

	// custom build a destination for this connection
	// put connection number in id
	if (*cookie < MAX_CONNECTIONS)
	{
		out_input->destination = mDestination[*cookie];
		out_input->source = mSource[*cookie];
		out_input->node = Node();
		if (!mConnectionActive[*cookie]) {
			out_input->format.u.raw_video = media_raw_video_format::wildcard;
		}
		else {
			out_input->format.u.raw_video = mFormat[*cookie];
		}
		out_input->format.type = B_MEDIA_RAW_VIDEO;
		strncpy(out_input->name, mName[*cookie], sizeof(out_input->name));
		(*cookie)++;
		return B_OK;
	}
	else
	{
		FUNCTION("VideoConsumer::GetNextInput - - BAD INDEX\n");
		return B_MEDIA_BAD_DESTINATION;
	}
}

//---------------------------------------------------------------

void
VideoConsumer::DisposeInputCookie(int32 /*cookie*/)
{
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetLatencyFor(
	const media_destination & /* input */,
	bigtime_t * out_latency,
	media_node_id * out_timesource)
{
	FUNCTION("VideoConsumer::GetLatencyFor\n");
	*out_latency = mMyLatency;
	*out_timesource = TimeSource()->ID();
	return B_OK;
}


//---------------------------------------------------------------

status_t
VideoConsumer::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format &format)
{
	FUNCTION("VideoConsumer::FormatChanged\n");
	
	uint32 connection = consumer.id;
	mFormat[connection] = format.u.raw_video;
	
	DeleteWindow(connection);
	DeleteBuffers(connection);
	
	CreateWindow(mName[connection], connection, format);
	return CreateBuffers(connection, format);;
}

//---------------------------------------------------------------

status_t
VideoConsumer::vRun(void * data)
{
	FUNCTION("VideoConsumer::vRun\n");
	((VideoConsumer *)data)->DisplayThread();
	return 0;
}

//---------------------------------------------------------------
struct
media_message 
{
	char whatever[B_MEDIA_MESSAGE_SIZE];
};

void
VideoConsumer::DisplayThread()
{
	FUNCTION("VideoConsumer::DisplayThread\n");
	
	bigtime_t	timeout;
	media_message msg;

	while (!mDisplayQuit)
	{

		status_t err = B_OK;
		int32 code=0;
		uint32 index;

		bigtime_t perfTime = 0;
		int32 what = BTimedEventQueue::B_NO_EVENT;
		void *pointer = NULL;
		uint32 cleanup = BTimedEventQueue::B_NO_CLEANUP;
		bigtime_t data = 0;
		
		while ( !mEventQueue->HasEvents() || (mEventQueue->NextEvent(NULL) > TimeSource()->Now()) )
		{
			timeout = mEventQueue->NextEvent(NULL) - TimeSource()->Now();
			if ((err = read_port_etc(mPort, &code, &msg, sizeof(msg), B_TIMEOUT, timeout)) > 0)
				HandleMessage(code, &msg, err );
			if (err == B_TIMED_OUT)
				break;
			if (err == B_BAD_PORT_ID) {
//fprintf(stderr, "VideoWindow quit? %s\n", mDisplayQuit ? "true" : "false");
				break;
			}
		}
		if (mDisplayQuit) {
			break;
		}

		/* we have timed out - so handle the next event */
		if (mEventQueue->PopEvent(&perfTime, &what, &pointer, &cleanup, &data) == B_OK)
		{
			switch (what)
			{
				case BTimedEventQueue::B_START:
					if (!mRunning)
						mRunning = true;
					break;
				case BTimedEventQueue::B_STOP:
					if (mRunning)
						mRunning = false;
					break;
				case BTimedEventQueue::B_HANDLE_BUFFER:
					{
					BBuffer *buffer = (BBuffer *) pointer;
					uint32 connection = buffer->Header()->destination;
					if (mRunning && mConnectionActive[connection])
					{
						// see if this is one of our buffers
						uint32 index = 0;
						mOurBuffers[connection] = true;
						while(index < 3)
							if ((uint32)buffer == mBufferMap[connection][index])
								break;
							else
								index++;
								
						if (index == 3)
						{
							// no, buffers belong to consumer
							mOurBuffers[connection] = false;
							index = 0;
						}
													
						if ( 1 || (RunMode() == B_OFFLINE) ||
							 ((TimeSource()->Now() > (buffer->Header()->start_time - JITTER)) &&
							  (TimeSource()->Now() < (buffer->Header()->start_time + JITTER))) )
						{
							if (!mOurBuffers[connection])
								memcpy(mBitmap[connection][index]->Bits(), buffer->Data(),mBitmap[connection][index]->BitsLength());
								
							if (mWindow[connection]->Lock())
							{
								uint32 flags;
								if ((mBitmap[connection][index]->ColorSpace() == B_GRAY8) &&
									!bitmaps_support_space(mBitmap[connection][index]->ColorSpace(), &flags))
								{
									// handle mapping of GRAY8 until app server knows how
									uint32 *start = (uint32 *)mBitmap[connection][index]->Bits();
									int32 size = mBitmap[connection][index]->BitsLength();
									uint32 *end = start + size/4;
									for (uint32 *p = start; p < end; p++)
										*p = (*p >> 3) & 0x1f1f1f1f;									
								}
								
								mView[connection]->DrawBitmap(mBitmap[connection][index], mView[connection]->Frame());
								mWindow[connection]->Unlock();
							}
						}
						else
							PROGRESS("VidConsumer::DisplayThread - DROPPED FRAME\n");
						buffer->Recycle();
					}
					else
						buffer->Recycle();
					}
					break;
				default:
					break;
			}			
		}
	}
}

//---------------------------------------------------------------

VideoWindow::VideoWindow (BRect frame, const char *title, window_type type, uint32 flags) :
	BWindow(frame,title,type,flags)
{
	FUNCTION("VideoWindow::VideoWindow\n");
}

//---------------------------------------------------------------

VideoWindow::~VideoWindow()
{
	FUNCTION("VideoWindow::~VideoWindow\n");
}

//---------------------------------------------------------------

bool
VideoWindow::QuitRequested()
{
	FUNCTION("VideoWindow::QuitRequested\n");
	return false;
}

//---------------------------------------------------------------

void
VideoWindow::MessageReceived(BMessage *message)
{
	FUNCTION("VideoWindow::Message Received\n");
	BControl *p;
	uint32 i;

	p = NULL;
	message->FindPointer((const char *)"source", (void **)&p);
		
	switch (message->what)
	{
		case B_KEY_DOWN:
			PRINTF("VideoWindow::Message Received - KEY DOWN\n");
			const char *key; 
			ssize_t numBytes;
            message->FindData((const char *)"bytes", (type_code)B_ANY_TYPE, (const void **)&key, &numBytes);
            
            switch (*key)
            {
				default:
					BWindow::MessageReceived(message);
					break;
			}
			break;
		case B_QUIT_REQUESTED:
			PRINTF("QUIT REQUESTED!!!\n");
			break;
		default:
#if DEBUG
			message->PrintToStream();
#endif
			BWindow::MessageReceived(message);
			break;
	}

}

//---------------------------------------------------------------

