#include <Bitmap.h>
#include <BufferConsumer.h>
#include <Debug.h>
#include <MediaKit.h>
#include "VideoConsumerNode.h"
#include "TimedEventQueue.h"
#include "VideoView.h"

const bigtime_t kMaxJitter = 1000;


VideoConsumerNode::VideoConsumerNode(const char *name, VideoView *outputView,
	 media_node *timeSource, bool debugOutput)
	:	BMediaNode(name),
		BBufferConsumer(B_MEDIA_RAW_VIDEO),
		fOutputView(outputView),
		fUsingBitmapBufferGroup(true),
		fConnected(false),
		fStopping(false),
		fBufferGroup(0),
		fKeepGoing(true),
		fDebugOutput(debugOutput)
{
	fControlPort = create_port(10, "Video View Control Port");
	// Check for error...

	Debug("Start control thread\n");
	fControlLoopThread = spawn_thread(StartControlLoop, "Control Thread",
		B_REAL_TIME_PRIORITY, this);
	resume_thread(fControlLoopThread);

	Debug("Register node with media server\n");
	BMediaRoster::Roster()->RegisterNode(this);	

	Debug("Set up my time source\n");
	BMediaRoster *roster = BMediaRoster::Roster();
	status_t err = roster->SetTimeSourceFor(Node().node, timeSource->node);
	if (err != B_OK) {
		Debug("Couldn't set time source for window\n");
		return;
	}
	
	fEventQueue = new BTimedEventQueue();
	fTimeSource = roster->MakeTimeSourceFor(Node());

	for (int buf = 0; buf < kNumBuffers; buf++)
		fOutputBitmap[buf] = NULL;
}

VideoConsumerNode::~VideoConsumerNode()
{
	// Stop thread
	close_port(fControlPort);
	fKeepGoing = false;
	int32 retval;
	wait_for_thread(fControlLoopThread, &retval);

	// Flush all buffers
	fEventQueue->FlushEvents(0, BTimedEventQueue::B_ALWAYS);
	delete fEventQueue;

	// Clean up bitmaps
	if (fUsingBitmapBufferGroup) {
		fBufferGroup->ReclaimAllBuffers();
		delete fBufferGroup;
		for (int index = 0; index < kNumBuffers; index++)
			delete fOutputBitmap[index];
	} else {
		delete fOutputBitmap[0];
	}
}

void 
VideoConsumerNode::ControlLoop()
{
	Debug("Control loop started\n");
	while (fKeepGoing) {
		bigtime_t nextEventTime = B_INFINITE_TIMEOUT;

		if (fConnected) {
			bigtime_t now = fTimeSource->Now();
	
			// if we received a Stop, deal with it
			if (fStopping) {
				if (now >= fStopTime) {
					fStopping = false;
					fStopped = true;
					fEventQueue->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
						BTimedEventQueue::B_HANDLE_BUFFER);
				} else 
					nextEventTime = fStopTime - now;
			}	

			while (fEventQueue->HasEvents()) {
				bigtime_t bufferDisplayTime = fEventQueue->FirstEventTime();

				// Queue up early buffer
				if (bufferDisplayTime > now + kMaxJitter) {
					nextEventTime = bufferDisplayTime - now;
					break;
				}

				// This buffer is right on time.  Display it.	
				media_timed_event event;
				fEventQueue->RemoveFirstEvent(&event);
				bufferDisplayTime = event.event_time;
				BBuffer *buffer = (BBuffer *) event.pointer;

				BBitmap *bitmapToDraw = 0;
				if (fUsingBitmapBufferGroup) {
					for (int i = 0; i < kNumBuffers; i++) {
						if (fBitmapID[i] == buffer->ID()) {
							bitmapToDraw = fOutputBitmap[i];
							break;
						}
					}
					if (bitmapToDraw == 0)
						debugger("Codec/reader fed me a bum buffer.");
				} else {
					memcpy(fOutputBitmap[0]->Bits(), buffer->Data(),
						fOutputBitmap[0]->BitsLength());
					bitmapToDraw = fOutputBitmap[0];
				}

				fOutputView->SetBitmap(bitmapToDraw);
				buffer->Recycle();
			}
		}

		char message[B_MEDIA_MESSAGE_SIZE];
		int32 code;
		
		ssize_t sizeRead = read_port_etc(fControlPort, &code, (void*) message, 
			B_MEDIA_MESSAGE_SIZE, B_TIMEOUT, nextEventTime);
	
		if (sizeRead == B_TIMED_OUT)
			continue; // Display buffer that was early.

		if (sizeRead == B_BAD_PORT_ID)
			break;
	
		if (sizeRead < 0) {
			Debug("Error reading port: %s\n", strerror(sizeRead));
			break;
		}

		if (BMediaNode::HandleMessage(code, (void*) message, sizeRead) &&
			BBufferConsumer::HandleMessage(code, (void*) message, sizeRead)) {
				BMediaNode::HandleBadMessage(code, (void*) message, sizeRead);
		}
	}

	Debug("Control Loop Exiting\n");
}


int32 
VideoConsumerNode::StartControlLoop(void *castToVideoConsumerNode)
{
	((VideoConsumerNode*)castToVideoConsumerNode)->ControlLoop();
	return 0;
}

port_id 
VideoConsumerNode::ControlPort() const
{
	return fControlPort;
}

BMediaAddOn *
VideoConsumerNode::AddOn(int32 *internal_id) const
{
	*internal_id = 0;
	return 0;
}

status_t 
VideoConsumerNode::AcceptFormat(const media_destination&,
	media_format * format)
{
	Debug("Accept Format Called for video output node\n");

	PRINT(("VideoConsumerNode::AcceptFormat %08x\n",format->u.raw_video.display.format));

	format->type = B_MEDIA_RAW_VIDEO;
	if (format->u.raw_video.display.format ==
		media_raw_video_format::wildcard.display.format) {
		format->u.raw_video.display.format = B_RGB32;
	}

	status_t err = B_OK;
	uint32 imageWidth = format->u.raw_video.display.line_width;
	uint32 imageHeight = format->u.raw_video.display.line_count;
	color_space imageColorSpace = format->u.raw_video.display.format;
	
	if (fUsingBitmapBufferGroup) {
		for (int buf = 0; buf < kNumBuffers; buf++) {
			PRINT(("Creating bitmaps %dx%d\n", (int)imageWidth, (int)imageHeight));
			if (fOutputBitmap[buf])
				delete fOutputBitmap[buf];
			fOutputBitmap[buf] = new BBitmap(BRect(0, 0, imageWidth - 1, imageHeight - 1),
				B_BITMAP_IS_AREA, imageColorSpace);
			if (!fOutputBitmap[buf] || !fOutputBitmap[buf]->IsValid()) {
				err = B_ERROR;
				break;
			}
		}
	}

	if (err) {
		fEventQueue->FlushEvents(0, BTimedEventQueue::B_ALWAYS);
		delete fBufferGroup;
		fBufferGroup = 0;
	} else 
		format->u.raw_video.display.bytes_per_row = fOutputBitmap[0]->BytesPerRow();

	Debug("Accepting format\n");
	return err;
}



status_t 
VideoConsumerNode::GetNextInput(int32 *cookie, media_input *out_input)
{
	if (*cookie != 0) 
		return B_MEDIA_BAD_DESTINATION;

	out_input->destination.port = fControlPort;
	out_input->destination.id = *cookie;
	out_input->source = media_source::null;
	out_input->node = Node();
	out_input->format.type = B_MEDIA_RAW_VIDEO;		
	out_input->format.u.raw_video = media_raw_video_format::wildcard;
	(*cookie)++;

	return B_OK;
}

void 
VideoConsumerNode::DisposeInputCookie(int32)
{
}


void 
VideoConsumerNode::BufferReceived(BBuffer *buffer)
{
	if (fStopped) {
		buffer->Recycle();
		return ;
	}

	media_timed_event event(buffer->Header()->start_time,
			BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
			
	if (fEventQueue->AddEvent(event) != B_OK) {
		printf("could not add buffer event to fEventQueue!\n");
		buffer->Recycle();
	}
}


void 
VideoConsumerNode::ProducerDataStatus(const media_destination&, int32,
	bigtime_t)
{
}


status_t 
VideoConsumerNode::GetLatencyFor(const media_destination&, bigtime_t *latency,
	media_node_id *timesource)
{
	*latency = 1000;
	*timesource = TimeSource()->ID();
	return B_OK;
}


status_t 
VideoConsumerNode::Connected(const media_source &producer, const media_destination 
	&where, const media_format &with_format, media_input *input)
{
	fStopped = false;

	// ToDo:
	// need to delete buffers properly when we get an error
	
	Debug("VideoView output node connected\n");

	input->node = Node();
	input->source = producer;
	input->format = with_format;
	input->destination.port = fControlPort;
	input->destination.id = 0;
	
	uint32 imageWidth = with_format.u.raw_video.display.line_width;
	uint32 imageHeight = with_format.u.raw_video.display.line_count;	
	color_space imageColorSpace = with_format.u.raw_video.display.format;

	if (fUsingBitmapBufferGroup) {
		Debug("Using bitmap buffer group\n");
		fEventQueue->FlushEvents(0, BTimedEventQueue::B_ALWAYS);
		delete fBufferGroup;
		fBufferGroup = new BBufferGroup;
		if (fBufferGroup->InitCheck() != B_OK) {
			Debug("Error %s occured creating buffer group\n", strerror(
				fBufferGroup->InitCheck()));
			return fBufferGroup->InitCheck();
		}
	
		for (int buf = 0; buf < kNumBuffers; buf++) {
			PRINT(("Creating bitmaps %dx%d\n", (int)imageWidth, (int)imageHeight));
			if (fOutputBitmap[buf] && fOutputBitmap[buf]->IsValid()) 
				Debug("Created bitmap is valid\n");
			else {
				Debug("Create bitmap is *not* valid, connect failed %08x\n",imageColorSpace);
				return B_ERROR;
			}
	
			void *bitmapBits = fOutputBitmap[buf]->Bits();
			buffer_clone_info bufferInfo;
			area_id area = area_for(bitmapBits);
			if (area < 0) {
				Debug("Couldn't find area for bitmap: %s.  data starts at %p\n",
					strerror(area), bitmapBits);
				return area;
			}
	
			bufferInfo.area = area;
	
			area_info info;
			if (get_area_info(area, &info) != B_OK) {
				Debug("Couldn't get info for area\n");
				return B_ERROR;
			}
	
			bufferInfo.offset = 0;//(uint32) fOutputBitmap[buf]->Bits() -(uint32) info.address;
			bufferInfo.size = fOutputBitmap[buf]->BitsLength();
			bufferInfo.flags = buf;
			
			Debug("Added bitmap/buffer.  Area = %li.  Start = %ld\n",
				bufferInfo.area, bufferInfo.offset);
			status_t error = fBufferGroup->AddBuffer(bufferInfo);
			if (error != B_OK) {
				Debug("Couldn't add buffer to group, %s\n", strerror(error));
				return error;
			}
		}
	
		Debug("Connected.  Image (%li, %li) colors %i\n", imageWidth, imageHeight,
			imageColorSpace);
		int32 dummy;
		status_t err = SetOutputBuffersFor(producer, where, fBufferGroup, 0, &dummy,
			true);
		if (err != B_OK) {
			Debug("Couldn't set buffer group for codec: %s\n", strerror(err));
			return err;
		}
		BBuffer *buffers[kNumBuffers];
		fBufferGroup->GetBufferList(kNumBuffers,(BBuffer **)&buffers);
		for (int32 z=0;z<kNumBuffers;z++)
			fBitmapID[z]=buffers[z]->ID();
	
	} else {
		fOutputBitmap[0] = new BBitmap(BRect(0, 0, imageWidth - 1, imageHeight - 1), 
			B_BITMAP_IS_AREA,	imageColorSpace);
	}

	fStopping = false;
	fConnected = true;
	Debug("connected OK\n");
	return B_OK;
}

void 
VideoConsumerNode::Disconnected(const media_source &producer, const media_destination &
	where)
{
	Debug("Video consumer node disconnected\n");
	fEventQueue->FlushEvents(0, BTimedEventQueue::B_ALWAYS);	
	Debug("Resetting output buffers for producer node\n");
	int32 dummy;
	SetOutputBuffersFor(producer, where, NULL, 0, &dummy, false);
	fConnected = false;
	Debug("Video consumer node disconnected ok\n");
}

status_t 
VideoConsumerNode::FormatChanged(const media_source &, const media_destination&,
	int32, const media_format&)
{
	Debug("Format changed\n");
	return B_ERROR;
}

void 
VideoConsumerNode::Stop(bigtime_t performance_time, bool immediate)
{
	printf("Video consumer node received stop\n");
	if (immediate) {
		fStopping = false;
		fStopped = true;
		fEventQueue->FlushEvents(0, BTimedEventQueue::B_ALWAYS, true,
			BTimedEventQueue::B_HANDLE_BUFFER);
	} else {
		fStopTime = performance_time;
		fStopping = true;
	}
}

void
VideoConsumerNode::Debug(const char *fmt, ...)
{
	if (!fDebugOutput)
		return ;
		
	va_list ap; 
	va_start(ap, fmt); 
	vfprintf(stderr, fmt, ap);
	va_end(ap); 		
}
