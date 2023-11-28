#include <OS.h>
#include <string.h>
#include <List.h>
#include <File.h>
#include <scheduler.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <MediaFormats.h>
#include <TimeSource.h>
#include <stdlib.h>
#include <stdio.h>
#include <Debug.h>
#include <Autolock.h>
#include <errno.h>

#include "Extractor.h"
#include "ExtractorNode.h"
#include "ExtractorAddon.h"
#include "addons.h"

using namespace BPrivate;


#define CALL(x)		//printf x
#define ERROR(x)	//printf x

#define MAX_BUFFERS		256
#define CHUNK_SIZE		65536
#define VERY_EARLY		-0x7FFFFFFFFFFFFFFLL

ExtractorNode::ExtractorNode(BMediaAddOn *myAddOn, char *name) :
BBufferProducer(B_MEDIA_UNKNOWN_TYPE /* ?? */),
BMediaNode(name),
BFileInterface()
{
	CALL(("Created extractor for '%s'\n",name));
	fMediaAddOn = myAddOn;
	fConnectionCount = 0;
	fTracks = NULL;
	fTrackCount = 0;
	fRunning = false;
	fChunkSize = CHUNK_SIZE;
	fMediaTime = 0;
	fMediaDuration = 0;
	fPerformanceToMediaTime = -1;
	fExtractor = NULL;
	fFile = NULL;

	fMyLatency = 20000;
	fReadAhead = 200000;

	fMaxDownstreamLatency = 0;

	fControlPort = create_port(3, "ExtractorNode port");
	fServiceThread = spawn_thread(init_reader_service_thread, "ExtractorNode", B_NORMAL_PRIORITY, this);
	resume_thread(fServiceThread);
}

ExtractorNode::~ExtractorNode()
{
	CALL(("ExtractorNode::~ExtractorNode()\n"));

	status_t myErr;

	close_port(fControlPort);
	wait_for_thread(fServiceThread, &myErr);

	fQ.FlushEvents(B_INFINITE_TIMEOUT,BTimedEventQueue::B_BEFORE_TIME,true,BTimedEventQueue::B_HANDLE_BUFFER);
	RecyclePending();

	if (fTracks) {
		for (int32 i=0;i<fTrackCount;i++) {
			fExtractor->FreeCookie(i, fTracks[i].extractor_cookie);
		}
		delete[] fTracks;
		fTracks = NULL;
	}

	if (fExtractor) {
		delete fExtractor;
		fExtractor = NULL;
	}

	for (int32 i=0;i<fChunkCount;i++) 
		delete fChunks[i].bufferGroup;

	delete_area(fBufferArea);
	fBufferArea = 0;
	
	if (fChunks) free(fChunks);
	fChunks = NULL;

	if (fFile) {
		delete fFile;
		fFile = NULL;
	}

	CALL(("ExtractorNode::~ExtractorNode() done\n"));
}

status_t ExtractorNode::init_reader_service_thread(void *data)
{
	return ((ExtractorNode *)data)->ServiceLoop();
}

enum {
	FIRST_AFTER = 1,
	LAST_BEFORE,
	LAST_KEYFRAME_BEFORE
};

status_t ExtractorNode::SeekTrack(int32 in_stream, bigtime_t *inout_time)
{
	ExtractorTrack		*stream;
	chunk				*first,*second,*whatIWant;
	status_t			err;
	uint8				*packetPointer;
	int32				packetLength;
	int32				oldPacketLength;
	int32				partialOffset;
	off_t				filePos;
	off_t				oldFilePos;
	bool 				done = false;

	CALL(("ExtractorNode::SeekTrack()\n"));

	if (in_stream >= fTrackCount) return B_BAD_VALUE;

	stream = &fTracks[in_stream];
	filePos = stream->pos;
	packetLength = 0;

	Extractor *extractor = fExtractor->fExtractors[in_stream];
	int32	local_stream = fExtractor->fStreamNums[in_stream];

	// check whether we've reached the end of the file yet

	do {
		if (fChunkSize - stream->pos_offs < packetLength) {

			first = PullChunk(filePos);
			whatIWant = GrabChunk();

			// save the partial packet
			
			partialOffset = fChunkSize - stream->pos_offs;
			if (whatIWant == first)	memmove(whatIWant->ptr, first->ptr + (fChunkSize - partialOffset), partialOffset);
			else					memcpy (whatIWant->ptr, first->ptr + (fChunkSize - partialOffset), partialOffset);

			whatIWant->size = packetLength;
			whatIWant->filePos = -1;
			whatIWant->latestBuffer = VERY_EARLY;
			whatIWant->wasOverlap = true;
			whatIWant->refCount++;

			// seek to the beginning of the next chunk
			stream->pos += partialOffset;
			stream->pos_offs = 0;
			stream->pos_chunk += fChunkSize;

			// determine of much of the new chunk has to be copied to complete
			// the partial packet.

			second = PullChunk(stream->pos);
			memcpy(whatIWant->ptr + partialOffset, second->ptr, packetLength - partialOffset);
			partialOffset = packetLength - partialOffset;

			ASSERT(whatIWant != second);

			whatIWant->refCount--;
			packetPointer = whatIWant->ptr;

		} else {
			// complete buffer

			whatIWant = PullChunk(filePos);
			packetLength = fChunkSize - stream->pos_offs;
			if (fExtractor->fFileSize - filePos < packetLength)
				packetLength = fExtractor->fFileSize - filePos;
			packetPointer = whatIWant->ptr + stream->pos_offs;
		}

		// call SplitNext

		oldPacketLength = packetLength;
		oldFilePos = filePos;

		err = extractor->Seek(local_stream, stream->extractor_cookie,
		                      B_SEEK_BY_TIME, 0, inout_time,
							  &stream->frame, &filePos, (char*)packetPointer,
							  &packetLength, &done);
		if (err) {
			ERROR(("Extractor::SeekTo(): got error from Split (%s)\n", strerror(err)));
			return err;
		}

		// a couple of sanity checks

		if (filePos > fExtractor->fFileSize) {
			ERROR(("Extractor::SeekTo(): Split returns filePos beyond EOF! (%Lx > %Lx)\n", filePos, fExtractor->fFileSize));
			return B_ERROR;
		}
		if ((filePos + packetLength) > fExtractor->fFileSize) {
			ERROR(("Extractor::SeekTo(): Split returns packetLength beyond EOF! (%Lx > %Lx)\n", filePos, fExtractor->fFileSize));
			return B_ERROR;
		}

		// update the file position
		// optimization: don't do very slow 64 bit division when avoidable

		if (!((filePos >= stream->pos_chunk) && (filePos - stream->pos_chunk < fChunkSize)))
			if ((filePos >= stream->pos_chunk + fChunkSize) && (filePos - stream->pos_chunk < 2*fChunkSize))
				stream->pos_chunk += fChunkSize;
			else 
				if ((filePos >= stream->pos_chunk - fChunkSize) && (filePos < stream->pos_chunk))
					stream->pos_chunk -= fChunkSize;
				else
					stream->pos_chunk = (filePos / fChunkSize) * fChunkSize;
		stream->pos_offs = filePos - stream->pos_chunk;
		stream->pos = filePos;

	} while (!done);

	return B_OK;
}

status_t
ExtractorNode::GetNextChunk(int32 in_stream, BBuffer **out_buffer)
{
	ExtractorTrack		*stream;
	chunk				*first,*second,*whatIWant;
	status_t			err;
	uint8				*packetPointer;
	uint8				*bufferStart;
	int32				packetLength;
	int32				oldPacketLength;
	int32				bufferLength;
	int32				partialOffset;
	off_t				filePos;
	off_t				oldFilePos;
	media_header		mh;

	CALL(("ExtractorNode::GetNextChunk()\n"));

	if (in_stream >= fTrackCount) return B_BAD_VALUE;

	Extractor *extractor = fExtractor->fExtractors[in_stream];
	int32	local_stream = fExtractor->fStreamNums[in_stream];

	stream = &fTracks[in_stream];
	filePos = stream->pos;
	packetLength = 0;

	// check whether we've reached the end of the file yet

	if (stream->pos == fExtractor->fFileSize)
		return B_LAST_BUFFER_ERROR;

	do {
		if (fChunkSize - stream->pos_offs < packetLength) {

			first = PullChunk(filePos);
			whatIWant = GrabChunk();

			// save the partial packet
			
			partialOffset = fChunkSize - stream->pos_offs;
			if (whatIWant == first)
				memmove(whatIWant->ptr, first->ptr + (fChunkSize - partialOffset), partialOffset);
			else
				memcpy(whatIWant->ptr, first->ptr + (fChunkSize - partialOffset), partialOffset);

			whatIWant->size = packetLength;
			whatIWant->filePos = -1;
			whatIWant->latestBuffer = VERY_EARLY;
			whatIWant->wasOverlap = true;
			whatIWant->refCount++;

			// seek to the beginning of the next chunk
			stream->pos += partialOffset;
			stream->pos_offs = 0;
			stream->pos_chunk += fChunkSize;

			// determine of much of the new chunk has to be copied to complete
			// the partial packet.

			second = PullChunk(stream->pos);
			memcpy(whatIWant->ptr + partialOffset, second->ptr, packetLength - partialOffset);
			partialOffset = packetLength - partialOffset;

			ASSERT(whatIWant != second);

			whatIWant->refCount--;
			packetPointer = whatIWant->ptr;

		} else {
			// complete buffer

			whatIWant = PullChunk(filePos);
			
			packetLength = fChunkSize - stream->pos_offs;
			if (fExtractor->fFileSize - filePos < packetLength)
				packetLength = fExtractor->fFileSize - filePos;
			packetPointer = whatIWant->ptr + stream->pos_offs;
		}

		// call SplitNext

		oldPacketLength = packetLength;
		oldFilePos = filePos;
		bufferStart = NULL;

		err = extractor->SplitNext(local_stream, stream->extractor_cookie,
			&filePos, (char *)packetPointer,
			&packetLength, (char **)&bufferStart, &bufferLength, &mh);
		if (err) {
			printf("Extractor::GetNextChunk(): got error from Split (%s)\n", strerror(err));
			return err;
		}

		// a couple of sanity checks

		if (bufferStart) {
			if (bufferStart - packetPointer >= oldPacketLength) {
				printf("Extractor::GetNextChunk(): Split returns bufferStart out of range (%p not in %p-%p)\n", bufferStart, packetPointer, packetPointer+packetLength);
				return B_ERROR;
			}
			if ((bufferLength < 0) || (bufferStart + bufferLength - packetPointer > oldPacketLength)) {
				printf("Extractor::GetNextChunk(): Split returns bufferLength out of range (%.8lx not in %.8x-%.8lx)\n", bufferLength, 1, packetLength - (bufferStart - packetPointer));
				return B_ERROR;
			}
		}
		if (filePos > fExtractor->fFileSize) {
			printf("Extractor::GetNextChunk(): Split returns filePos beyond EOF! (%Lx > %Lx)\n", filePos, fExtractor->fFileSize);
			return B_ERROR;
		}
		if (!bufferStart) {
			if (filePos + packetLength > fExtractor->fFileSize) {
				printf("Extractor::GetNextChunk(): Split returns packetLength beyond EOF! (%Lx + %lx > %Lx)\n", filePos, packetLength, fExtractor->fFileSize);
				return B_ERROR;
			}
		}

		// update the file position
		// optimization: don't do very slow 64 bit division when avoidable

		if (!((filePos >= stream->pos_chunk) && (filePos - stream->pos_chunk < fChunkSize)))
			if ((filePos >= stream->pos_chunk + fChunkSize) && (filePos - stream->pos_chunk < 2*fChunkSize))
				stream->pos_chunk += fChunkSize;
			else  if ((filePos >= stream->pos_chunk - fChunkSize) && (filePos < stream->pos_chunk))
				stream->pos_chunk -= fChunkSize;
			else
				stream->pos_chunk = (filePos / fChunkSize) * fChunkSize;
		stream->pos_offs = filePos - stream->pos_chunk;
		stream->pos = filePos;

	} while (!bufferStart);


//	printf("sending track %d data in chunk %d (media time = %Ld)\n",in_stream,whatIWant-fChunks,mediaTime);

	BBuffer *buf = whatIWant->GrabBuffer();

	buf->Header()->data_offset = bufferStart - whatIWant->ptr;
	buf->Header()->size_used = bufferLength;
	buf->Header()->start_time = mh.start_time - fPerformanceToMediaTime;
	buf->Header()->file_pos = mh.file_pos;
	buf->Header()->orig_size = mh.orig_size;
	
	if (mh.type == B_MEDIA_ENCODED_VIDEO)
		buf->Header()->u.encoded_video.field_flags = mh.u.encoded_video.field_flags;
	*((bigtime_t*)buf->Header()->user_data) = mh.start_time;
	*((chunk**)(buf->Header()->user_data+8)) = whatIWant;

	whatIWant->refCount++;
	*out_buffer = buf;
	
	stream->frame++;

	return B_OK;
}

#define LOOP(a) //printf a
status_t ExtractorNode::ServiceLoop()
{
	bigtime_t	eventTime,waitUntil,occurance;
	int32		eventWhat;
	void *		eventPointer;
	uint32		eventCleanup;
	int64		eventData;
	int32 		msgCode = 0;
	uint8  		mediaMsg[B_MEDIA_MESSAGE_SIZE];
	status_t	err;

	CALL(("ExtractorNode::ServiceLoop() entered\n"));

	while (true) {
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
		
//		eventTime = fQ.NextEventTime(&eventWhat,&eventPointer,&eventCleanup,&eventData);
		occurance = TimeSource()->RealTimeFor(eventTime,fMaxDownstreamLatency);
		waitUntil = occurance-(fMyLatency+fReadAhead);

		LOOP(("Extractor: Reading from port (timeout = %Ld)\n",waitUntil-system_time()));
		err = read_port_etc(fControlPort, &msgCode, mediaMsg, B_MEDIA_MESSAGE_SIZE, B_ABSOLUTE_TIMEOUT, waitUntil);

		if (err >= 0) {
			LOOP(("Calling HandleMessage\n"));
			err = HandleMessage(msgCode, mediaMsg, B_MEDIA_MESSAGE_SIZE);
		} else if (err != B_TIMED_OUT) {
			LOOP(("Port is gone!\n"));
			return err;
		} else {
			LOOP(("Popping event\n"));
			media_timed_event poppedEvent;
			fQ.RemoveFirstEvent(&poppedEvent);
			eventTime = poppedEvent.event_time;
			eventWhat = poppedEvent.type;
			eventPointer = poppedEvent.pointer;
			eventCleanup = poppedEvent.cleanup;
			eventData = poppedEvent.bigdata;

//			fQ.PopEvent(&eventTime,&eventWhat,&eventPointer,&eventCleanup,&eventData);
			if (fExtractor) {
				switch (eventWhat) {
					case BTimedEventQueue::B_START:
						if (!fRunning) {
							LOOP(("Start! %Ld\n",eventTime));
							fPerformanceToMediaTime = fMediaTime - eventTime;
							RestampPending();
							fRunning = true;
							ExtractorTrack *t = fTracks;
							for (int32 track=0;track<fTrackCount;track++,t++) {
								if (t->pos < fExtractor->fFileSize) {
									SendDataStatus(B_DATA_AVAILABLE, t->destination, eventTime);
									PushNextBuffer(track);
								}
							}
						}
						break;
					case BTimedEventQueue::B_STOP: 
						if (fRunning) {
							LOOP(("Stop! %Ld\n",eventTime));
							fRunning = false;
							fMediaTime = eventData + fPerformanceToMediaTime;
							fQ.FlushEvents(B_INFINITE_TIMEOUT, BTimedEventQueue::B_ALWAYS, true,
								BTimedEventQueue::B_HANDLE_BUFFER);
						}
						break;
					case BTimedEventQueue::B_TIMER:
						LOOP(("Notify!\n"));
						TimerExpired(eventTime, eventData);
						break;
					case BTimedEventQueue::B_SEEK: {
							LOOP(("Seek!\n"));
							int32 alreadySeeked = -1;
							ExtractorTrack *t = fTracks;
							fMediaTime = eventData;
		
							fQ.FlushEvents(B_INFINITE_TIMEOUT, BTimedEventQueue::B_BEFORE_TIME,
								true, BTimedEventQueue::B_HANDLE_BUFFER);
							RecyclePending();
		
							for (int32 track=0;track<fTrackCount;track++,t++) {
								if ((t->format.type == B_MEDIA_RAW_VIDEO)
									|| (t->format.type == B_MEDIA_ENCODED_VIDEO)) {
									SeekTrack(track,&fMediaTime);
									alreadySeeked = track;
									break;
								}
							}
		
							fPerformanceToMediaTime = fMediaTime - eventTime;
		
							t = fTracks;
							for (int32 chunk=0;chunk<fChunkCount;chunk++) fChunks[chunk].seekedSinceSent = true;
							for (int32 track=0;track<fTrackCount;track++,t++) {
								bigtime_t theTime = fMediaTime;
								SendDataStatus(B_DATA_NOT_AVAILABLE, t->destination, eventTime);
								SendDataStatus(B_DATA_AVAILABLE, t->destination, eventTime);
								if (track != alreadySeeked)
									SeekTrack(track,&theTime);
								if (fRunning)
									PushNextBuffer(track);
							}
						}
						break;
					case BTimedEventQueue::B_HANDLE_BUFFER: {
						LOOP(("Handle buffer!\n"));
						BBuffer *buffer = (BBuffer*)eventPointer;
						ExtractorTrack *t = &fTracks[eventData];
						t->pendingBuffer = NULL;
						
						if (buffer) {
							chunk *theChunk = *((chunk**)(buffer->Header()->user_data+8));
							bigtime_t perfTime = buffer->Header()->start_time;
							bigtime_t mediaTime = *((bigtime_t*)buffer->Header()->user_data);
							buffer->Header()->time_source = TimeSource()->ID();
							switch (buffer->Header()->type = t->format.type) {
								case B_MEDIA_RAW_VIDEO: {
									/* fill in header for raw */
									buffer->Header()->u.raw_video.field_gamma = 1.0f;
									buffer->Header()->u.raw_video.field_sequence = t->frame;
									buffer->Header()->u.raw_video.first_active_line = 0;
									buffer->Header()->u.raw_video.line_count = t->format.Height();
								} break;
								case B_MEDIA_ENCODED_VIDEO: {
									/* fill in header for encoded */
									buffer->Header()->u.encoded_video.field_gamma = 1.0f;
									buffer->Header()->u.encoded_video.field_sequence = t->frame;
									buffer->Header()->u.encoded_video.first_active_line = 0;
									buffer->Header()->u.encoded_video.line_count = t->format.Height();
								} break;
								default:
									// do nothing for non-video buffers
									break;
							}
							
							if (SendBuffer(buffer,t->destination) != B_OK) 
								buffer->Recycle();
	
							bigtime_t now = TimeSource()->Now();
							bigtime_t slippage = (now - fMaxDownstreamLatency) - (mediaTime - fPerformanceToMediaTime);
							if (slippage > 0)
								fPerformanceToMediaTime -= slippage;
	
							if ((t->pos < fExtractor->fFileSize) && fRunning)
								PushNextBuffer(eventData);
							
							theChunk->refCount--;
							
							bigtime_t chunkStamp = perfTime;
							if (t->pendingBuffer)
								chunkStamp = t->pendingBuffer->Header()->start_time;
							if (theChunk->latestBuffer < chunkStamp)
								theChunk->latestBuffer = chunkStamp;
						} else 
							ERROR(("Failed to grab buffer!\n"));
					}
					break;
				}
			}
		}
	}

	return B_OK;
}

status_t ExtractorNode::PushNextBuffer(int32 track)
{
	status_t err;
	ExtractorTrack *t = fTracks+track;

	CALL(("ExtractorNode::PushNextBuffer()\n"));

	if ((!t->pendingBuffer) && (err=GetNextChunk(track,&t->pendingBuffer))) return err;

	media_timed_event event(t->pendingBuffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
		t->pendingBuffer,  BTimedEventQueue::B_NO_CLEANUP);
	event.bigdata = track;
	fQ.AddEvent(event);
	
//	fQ.PushEvent(t->pendingBuffer->Header()->start_time,BTimedEventQueue::B_HANDLE_BUFFER,t->pendingBuffer,BTimedEventQueue::B_NO_CLEANUP,track);

	return B_OK;
};

void ExtractorNode::RecyclePending()
{
	CALL(("ExtractorNode::RecyclePending()\n"));

	for (int32 i=0;i<fTrackCount;i++) {
		if (fTracks[i].pendingBuffer) {
			(*((chunk**)(fTracks[i].pendingBuffer->Header()->user_data+8)))->refCount--;
			fTracks[i].pendingBuffer->Recycle();
			fTracks[i].pendingBuffer = NULL;
		};
	};

	for (int32 i=0;i<fChunkCount;i++) {
		ASSERT(fChunks[i].refCount == 0);
	};
};

void ExtractorNode::RestampPending()
{
	CALL(("ExtractorNode::RestampPending()\n"));

	for (int32 i=0;i<fTrackCount;i++) {
		if (fTracks[i].pendingBuffer) {
			fTracks[i].pendingBuffer->Header()->start_time =
				*((bigtime_t*)fTracks[i].pendingBuffer->Header()->user_data) - fPerformanceToMediaTime;
		};
	};
};

status_t ExtractorNode::BuildDescriptions()
{
	CALL(("ExtractorNode::BuildDescriptions()\n"));

	fMediaDuration = 0;
	if (fTracks) {
		for (int32 i=0;i<fTrackCount;i++) {
			fExtractor->FreeCookie(i, fTracks[i].extractor_cookie);
		}
		delete[] fTracks;
	}
	fTracks = new ExtractorTrack[fTrackCount];

	for (int32 i=0;i<fTrackCount;i++) {
		fTracks[i].bufferGroup = NULL;
		fTracks[i].pendingBuffer = NULL;
		fTracks[i].bufferSize = 0;
		fTracks[i].destination = media_destination::null;
		fTracks[i].source = media_source::null;
		fTracks[i].enabled = true;
		fTracks[i].connected = false;
		fTracks[i].downstreamLatency = 0;
		fTracks[i].pos = 0;
		fTracks[i].pos_chunk = 0;
		fTracks[i].pos_offs = 0;
		fTracks[i].frame = 0;
		fTracks[i].frameCount = 0;
		fTracks[i].duration = 0;

		status_t myerr;
		myerr = fExtractor->AllocateCookie(i, &fTracks[i].extractor_cookie);
		if (myerr) {
			ERROR(("Extractor: error getting cookie for track %ld (%s)\n", i, strerror(myerr)));
			i--;
			fTrackCount--;
			continue;
		}

		int32 metaDataSize=0;
		void *metaData=NULL;
		myerr = fExtractor->TrackInfo(i,&fTracks[i].format,&metaData,&metaDataSize);
		if (myerr) {
			ERROR(("Extractor: error getting track info for track %ld (%s)\n", i, strerror(myerr)));
			i--;
			fTrackCount--;
			continue;
		}

		fTracks[i].format.SetMetaData(metaData,metaDataSize);
		fExtractor->CountFrames(i,&fTracks[i].frameCount);
		fExtractor->GetDuration(i,&fTracks[i].duration);
		if (fMediaDuration < fTracks[i].duration)
			fMediaDuration = fTracks[i].duration;
	}

	fChunkCount = fTrackCount * 4;
	fBufferSize = fChunkSize * fChunkCount;
	fBufferArea = create_area("Extractor buffers",(void**)&fBufferPtr,
		B_ANY_ADDRESS,fBufferSize,B_NO_LOCK,B_READ_AREA|B_WRITE_AREA);
	fChunks = (chunk*)malloc(fChunkCount * sizeof(chunk));

	for (int32 i=0,offs=0;i<fChunkCount;i++,offs+=fChunkSize) {
		fChunks[i].filePos = -1;
		fChunks[i].offset = offs;
		fChunks[i].ptr = ((uint8*)fBufferPtr)+offs;
		fChunks[i].size = 0;
		fChunks[i].refCount = 0;
		fChunks[i].seekedSinceSent = false;
		fChunks[i].totalSize = fChunkSize;
		fChunks[i].area = fBufferArea;
		fChunks[i].latestBuffer = VERY_EARLY;
		fChunks[i].bufferGroup = new BBufferGroup();
	}
	
	return B_OK;
}

/* MediaNode calls */

port_id ExtractorNode::ControlPort() const
{
	return fControlPort;
}

BMediaAddOn *ExtractorNode::AddOn(int32 *internal_id) const
{
	return fMediaAddOn;
}

void ExtractorNode::Start(bigtime_t performance_time)
{
	CALL(("ExtractorNode::Start()\n"));

	media_timed_event event(performance_time, BTimedEventQueue::B_START);
	fQ.AddEvent(event);
//	fQ.PushEvent(performance_time, BTimedEventQueue::B_START, NULL, 0, 0);
}

void ExtractorNode::Stop(bigtime_t performance_time, bool immediate)
{
	CALL(("ExtractorNode::Stop()\n"));

	if (immediate) {
		// for a synchronous stop, go ahead and flush the event queue
		// and set ourselves to stopped immediately
		fRunning = false;
		fQ.FlushEvents(B_INFINITE_TIMEOUT, BTimedEventQueue::B_ALWAYS, true,
			BTimedEventQueue::B_HANDLE_BUFFER);
		// make sure we update the media/performance time mapping here as well
		// as in the usual BTimedEventQueue::B_STOP event handler
		fMediaTime = TimeSource()->Now() + fPerformanceToMediaTime;
	} else {
		bigtime_t qTime = performance_time;
		media_timed_event event(qTime, BTimedEventQueue::B_STOP);
		event.bigdata = performance_time;
		fQ.AddEvent(event);
	}
}

void ExtractorNode::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	CALL(("ExtractorNode::Seek()\n"));

	media_timed_event event(performance_time, BTimedEventQueue::B_SEEK);
	event.bigdata = media_time;
	fQ.AddEvent(event);
//	fQ.PushEvent(performance_time, BTimedEventQueue::B_SEEK, NULL, 0, media_time);
}

void ExtractorNode::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	CALL(("ExtractorNode::TimeWarp()\n"));

	media_timed_event event(TimeSource()->PerformanceTimeFor(at_real_time),
		BTimedEventQueue::B_WARP);
	event.bigdata = to_performance_time;
	fQ.AddEvent(event);
	
//	fQ.PushEvent(TimeSource()->PerformanceTimeFor(at_real_time),
//				BTimedEventQueue::B_WARP, NULL, 0, to_performance_time);
}

status_t ExtractorNode::AddTimer(bigtime_t at_time, int32 cookie)
{
	CALL(("ExtractorNode::AddTimer()\n"));

	media_timed_event event(at_time, BTimedEventQueue::B_TIMER);
	event.bigdata = cookie;
	fQ.AddEvent(event);
//	fQ.PushEvent(at_time, BTimedEventQueue::B_TIMER, NULL, 0, cookie);
	return B_OK;
}

void ExtractorNode::SetRunMode(run_mode mode)
{
	int32 threadPriority;
	int32 threadType;

	CALL(("ExtractorNode::SetRunMode()\n"));

	if ((RunMode() == B_OFFLINE) && (mode != B_OFFLINE)) {
		/* set threads back to their higher priority */
		threadType = B_DEFAULT_MEDIA_PRIORITY; 
		for (int32 i = 0; i < fTrackCount; i++) {
			switch (fTracks[i].format.type) {
				case B_MEDIA_RAW_VIDEO: 
				case B_MEDIA_ENCODED_VIDEO:
					if (threadType != B_LIVE_AUDIO_MANIPULATION)
						threadType = B_LIVE_VIDEO_MANIPULATION; 
					break;
				case B_MEDIA_RAW_AUDIO: 
				case B_MEDIA_ENCODED_AUDIO:
					threadType = B_LIVE_AUDIO_MANIPULATION; 
					break;
				default:
					// no other media types need special thread priorities
					break;
			}
		}

		threadPriority = suggest_thread_priority(threadType);
		set_thread_priority(fServiceThread, threadPriority);
	} else if (mode == B_OFFLINE) {
		/* set threads to a lower priority */
		threadPriority = suggest_thread_priority(B_OFFLINE_PROCESSING);
		set_thread_priority(fServiceThread, threadPriority);
	}

	CALL(("ExtractorNode::SetRunMode() exiting\n"));
}

void ExtractorNode::Preroll()
{
	CALL(("ExtractorNode::Preroll()\n"));
	ExtractorTrack *t = fTracks;
	for (int32 track=0;track<fTrackCount;track++,t++) 
		if (!t->pendingBuffer)
			GetNextChunk(track,&t->pendingBuffer);
}

status_t ExtractorNode::HandleMessage(
							int32 					message,
							const void 				*data,
							size_t 					size)
{
	CALL(("ExtractorNode::HandleMessage()\n"));

	status_t err;
	if ((err = BMediaNode::HandleMessage(message, data, size)) &&
		(err = BBufferProducer::HandleMessage(message, data, size)) &&
		(err = BFileInterface::HandleMessage(message, data, size)))
		HandleBadMessage(message, data, size);

	return B_OK;
}

//#pragma mark -

/* FileInterface calls */

status_t ExtractorNode::GetNextFileFormat(
								int32 					*cookie,
								media_file_format 		*out_format)
{
	CALL(("ExtractorNode::GetNextFileFormat()\n"));

	if ((*cookie >= 0) && (*cookie < 1)) {
		*cookie += 1;

		strcpy(out_format->mime_type, "video/quicktime");

		out_format->capabilities = media_file_format::B_READABLE |
							media_file_format::B_KNOWS_RAW_VIDEO |
							media_file_format::B_KNOWS_RAW_AUDIO |
							media_file_format::B_KNOWS_ENCODED_VIDEO |
							media_file_format::B_KNOWS_ENCODED_AUDIO |
							media_file_format::B_KNOWS_OTHER;
							
	} else {
		return B_ERROR;
	} 

	return B_OK;
}

void ExtractorNode::DisposeFileFormatCookie(
								int32 					cookie)
{
	CALL(("ExtractorNode::DisposeFileFormatCookie()\n"));
}

status_t ExtractorNode::GetDuration(
								bigtime_t 				*out_time)
{
	CALL(("ExtractorNode::GetDuration()\n"));

	*out_time = fMediaDuration;

	return B_OK;
}

status_t ExtractorNode::SniffRef(const entry_ref 		&ref,
								char 					*out_mime_type,
								float 					*out_quality)
{
	CALL(("ExtractorNode::SniffRef()\n"));
	status_t err = B_OK;

	if (fExtractor) {
		delete fExtractor;
		fExtractor = NULL;
	}

	if (fFile) {
		delete fFile;
		fFile = NULL;
	}

	fFile = new BFile(&ref, B_READ_ONLY);
	if ((err = fFile->InitCheck()) != B_OK) {
		delete fFile;
		fFile = NULL;
		return err;
	}

	MediaExtractor	*extractor = new MediaExtractor(0);
	err = extractor->SetSource(fFile, &fTrackCount);
	if(err != B_NO_ERROR) {
		delete extractor;
		delete fFile;
		fFile = NULL;
		return err;
	}
	fExtractor = extractor;
	*out_quality = 1.0;
	return B_OK;
}

status_t ExtractorNode::SetRef(	const entry_ref 		&ref,
								bool 					create,
								bigtime_t 				*out_time)
{
	status_t err = B_OK;
	CALL(("ExtractorNode::SetRef()\n"));

	if (create) return B_ERROR;

	BFile *file = new BFile(&ref, B_READ_ONLY);
	if ((err = file->InitCheck()) != B_OK) {
		delete file;
		return err;
	}

	if (fFile == NULL || *file != *fFile) {
		if (fExtractor) {
			delete fExtractor;
			fExtractor = NULL;
		}

		if (fFile) {
			delete fFile;
			fFile = NULL;
		}

		fFile = file;

		MediaExtractor	*extractor = new MediaExtractor(0);
		err = extractor->SetSource(fFile, &fTrackCount);
		if(err != B_NO_ERROR) {
			delete extractor;
			delete fFile;
			fFile = NULL;
			return err;
		}
		fExtractor = extractor;
	} else {
		delete file;
		file = NULL;
	}


	fRef = ref;
	fChunkSize = fExtractor->GetChunkSize();

	if ((err = BuildDescriptions()) != B_OK)
		goto error1;

	fExtractor->fFileSize = fFile->Seek(0,SEEK_END);
	fFile->Seek(0,SEEK_SET);

	*out_time = fMediaDuration;

	CALL(("ExtractorNode::SetRef() - returning error '%s'\n",strerror(err)));
	
	return B_OK;

error1:
	CALL(("ExtractorNode::SetRef() - error %s\n",strerror(err)));
	if (fExtractor) {
		delete fExtractor;
		fExtractor = NULL;
	}
	
	delete fFile;
	fFile = NULL;
	return err;
}

status_t ExtractorNode::GetRef(	entry_ref 				*out_ref,
								char 					*out_mime_type)
{
	CALL(("ExtractorNode::GetRef()\n"));

	if (fFile == NULL)
		return B_ERROR;

	*out_ref = fRef;
	strcpy(out_mime_type, "video/quicktime");

	return B_OK;
}

//#pragma mark -

/* BufferProducer calls */

status_t ExtractorNode::FormatSuggestionRequested(
								media_type				type,
								int32 					quality,
								media_format 			*format)
{
	CALL(("ExtractorNode::FormatSuggestionRequested()\n"));

	/* First let's look and see how many outputs we've got of the specified type. */
	int32 it=-1, count=0;
	for (int32 i=0;i<fTrackCount;i++) 
		if (type == fTracks[i].format.type) {
			it = i;
			count++;
		}

	if (count == 1)
		*format = fTracks[it].format;
	else if (count > 1) {
		switch (type) {
			case B_MEDIA_RAW_VIDEO:
				format->type = B_MEDIA_RAW_VIDEO;
				format->u.raw_video = media_raw_video_format::wildcard;
				break;
			case B_MEDIA_ENCODED_VIDEO:
				format->type = B_MEDIA_ENCODED_VIDEO;
				format->u.encoded_video = media_encoded_video_format::wildcard;
				break;
			case B_MEDIA_RAW_AUDIO:
				format->type = B_MEDIA_RAW_AUDIO;
				format->u.raw_audio = media_raw_audio_format::wildcard;
				break;
			case B_MEDIA_ENCODED_AUDIO:
				format->type = B_MEDIA_ENCODED_AUDIO;
				format->u.encoded_audio = media_encoded_audio_format::wildcard;
				break;
			default:
				return B_MEDIA_BAD_FORMAT;
		}
	} else
		return B_MEDIA_BAD_FORMAT;

	return B_OK;
}

status_t ExtractorNode::FormatProposal(
								const media_source 		&output,
								media_format 			*format)
{
	CALL(("ExtractorNode::FormatProposal()id:%ld\n", output.id));

	bool match = format_is_compatible(*format, fTracks[output.id].format);
	format->SpecializeTo(&fTracks[output.id].format);
	format->MetaData();
	
	return match?B_OK:B_MEDIA_BAD_FORMAT;
}

status_t ExtractorNode::PrepareToConnect(
								const media_source 		&what,
								const media_destination &where,
								media_format 			*format,
								media_source 			*out_source,
								char 					*io_name)
{
	CALL(("ExtractorNode::PrepareToConnect()\n"));

	bool match = format_is_compatible(*format, fTracks[what.id].format);
	format->SpecializeTo(&fTracks[what.id].format);
	if (match) 
		strncpy(io_name, fRef.name, B_MEDIA_NAME_LENGTH);
	return match?B_OK:B_MEDIA_BAD_FORMAT;
}

void ExtractorNode::Connect(	status_t 				error, 
								const media_source 		&source,
								const media_destination &destination,
								const media_format 		&format,
								char 					*out_name)
{
	CALL(("ExtractorNode::Connect()\n"));

	/* tell the about to be streamed buffer where to go */	
	fTracks[source.id].destination = destination;
	fTracks[source.id].source = source;
	fTracks[source.id].bufferGroup = NULL;
	fTracks[source.id].connected = true;
	fConnectionCount++;

	BBufferProducer::GetLatency(&fTracks[source.id].downstreamLatency);
//	GetLatency does not work here - call FindLatencyFor
//	media_node_id tsId;
//	FindLatencyFor(destination, &fTracks[source.id].downstreamLatency, &tsId);
	if (fMaxDownstreamLatency < fTracks[source.id].downstreamLatency)
		fMaxDownstreamLatency = fTracks[source.id].downstreamLatency;

	CALL(("ExtractorNode::Connect() - %ld : latency = %Ld\n", source.id, fTracks[source.id].downstreamLatency));
}

void ExtractorNode::Disconnect(	const media_source 		&what,
								const media_destination &where)
{
	CALL(("ExtractorNode::Disconnect()\n"));

	fConnectionCount--;
	CALL(("ExtractorNode::Disconnect() - half done\n"));

	/* free buffer group */
	if (fTracks[what.id].bufferGroup) {
		delete fTracks[what.id].bufferGroup;
		fTracks[what.id].bufferGroup = NULL;
	}

	fMaxDownstreamLatency = 0;
	for (int32 i=0;i<fTrackCount;i++) 
		if (fTracks[i].connected && (fMaxDownstreamLatency < fTracks[i].downstreamLatency))
			fMaxDownstreamLatency = fTracks[i].downstreamLatency;

	CALL(("ExtractorNode::Disconnect() - done\n"));
}

status_t ExtractorNode::FormatChangeRequested(
								const media_source 		&source,
								const media_destination &destination,
								media_format 			*io_format,
								int32 					*out_change_count)
{
	CALL(("ExtractorNode::FormatChangeRequested()\n"));

	return B_ERROR;
}

status_t ExtractorNode::GetNextOutput(
								int32 					*cookie,
								media_output 			*out_output)
{
	CALL(("ExtractorNode::GetNextOutput()\n"));

	if (*cookie == fTrackCount) {
		CALL(("ExtractorNode::GetNextOutput() - Described All\n"));
		return B_BAD_INDEX;
	}

	out_output->node = Node();
	out_output->source.port = fControlPort;
	out_output->source.id = *cookie;
	out_output->destination = fTracks[*cookie].destination;
	out_output->format = fTracks[*cookie].format;

//	PrintFormat(out_output->format);

/*
	char fmtstr[1024];
	string_for_format(fTracks[*cookie].format,fmtstr,1024);
	
	switch (fTracks[*cookie].format.type) {
		case B_MEDIA_RAW_VIDEO:
		case B_MEDIA_ENCODED_VIDEO:		
			sprintf(out_output->name, "Extractor Video");
			CALL(("Extractor Video %s\n",fmtstr));
			break;

		case B_MEDIA_RAW_AUDIO:
		case B_MEDIA_ENCODED_AUDIO:
			sprintf(out_output->name,  "Extractor Audio");
			CALL(("Extractor Audio %s\n",fmtstr));
			break;
	
		default:
			sprintf(out_output->name, "Extractor (unknown)");
			break;
		}
*/

	(*cookie)++;

	return B_OK;
}

status_t ExtractorNode::DisposeOutputCookie(
								int32 					cookie)
{
	CALL(("ExtractorNode::DisposeOutputCookie()\n"));

	return B_OK;
}

status_t ExtractorNode::SetBufferGroup(
								const media_source 		&for_source,
								BBufferGroup 			*group)
{
	CALL(("ExtractorNode::SetBufferGroup()\n"));

	if (fTracks[for_source.id].bufferGroup) {
		delete fTracks[for_source.id].bufferGroup;
		fTracks[for_source.id].bufferGroup = group;
	}

	return B_OK;
}

status_t ExtractorNode::VideoClippingChanged(
								const media_source 		&for_source,
								int16 					num_shorts,
								int16 					*clip_data,
								const media_video_display_info &display,
								int32 					*out_from_change_count)
{
	CALL(("ExtractorNode::VideoClippingChanged()\n"));
	return B_OK;
}

void ExtractorNode::LateNoticeReceived(
								const media_source 		&what,
								bigtime_t 				how_much,
								bigtime_t 				performance_time)
{
	CALL(("ExtractorNode::LateNoticeReceived()\n"));
	fMyLatency += 5000;
}

void ExtractorNode::EnableOutput(const media_source 		&what,
								bool 					enabled,
								int32 					*change_tag)
{
	CALL(("ExtractorNode::EnableOutput()\n"));

	fTracks[what.id].enabled = enabled;
}

/* Chunk manipulation. */

chunk * ExtractorNode::GrabChunk()
{
	CALL(("ExtractorNode::GrabChunk()\n"));

	chunk *		best = NULL;
	bigtime_t	bestTime = B_INFINITE_TIMEOUT;
	
	for (int32 i=0;i<fChunkCount;i++) 
		if ((fChunks[i].latestBuffer < bestTime)
			&& fChunks[i].seekedSinceSent
			&& !fChunks[i].refCount) {
			best = &fChunks[i];
			bestTime = fChunks[i].latestBuffer;
		}

	if (best == NULL) 
		for (int32 i=0;i<fChunkCount;i++) 
			if ((fChunks[i].latestBuffer < bestTime) &&
				!fChunks[i].refCount) {
				best = &fChunks[i];
				bestTime = fChunks[i].latestBuffer;
			}
	
	best->WaitForChunk();
	best->seekedSinceSent = false;
	best->latestBuffer = VERY_EARLY;
	return best;
}

chunk * ExtractorNode::PullChunk(int64 filePos)
{
	CALL(("ExtractorNode::PullChunk()\n"));

	int64 chunkPos = filePos - (filePos % fChunkSize);

	chunk *c = fChunks;
	for (int32 i=0;i<fChunkCount;i++) {
		if ((c->filePos == chunkPos) && !c->wasOverlap)
			return c;
		c++;
	}
	
	chunk * it = GrabChunk();
	it->filePos = chunkPos;
	it->size = fChunkSize;
	it->wasOverlap = false;
	fFile->ReadAt(chunkPos,it->ptr,fChunkSize);
	return it;
}

#if 0
BBuffer * ExtractorNode::GrabData(int64 filePos, int32 size, bigtime_t toBePlayedAt)
{
	int32 i,j,overlap;
	BBuffer *buf = NULL;
	/* First, we'll look for a chunk that has this data already */

	int32 first = PullChunk(filePos);
	if (fChunks[first].latestBuffer < toBePlayedAt) fChunks[first].latestBuffer = toBePlayedAt;
	if ((filePos+size) < (fChunks[first].filePos + fChunks[first].size)) {
		/* It ends in the same chunk, which means no copy is neccessary */
		buf = fChunks[first].GrabBuffer();
		buf->Header()->data_offset = filePos - fChunks[first].filePos;
		buf->Header()->size_used = size;
		buf->Header()->start_time = toBePlayedAt;
//		printf("BUFFER: chunk=%d, buffer=%d (no overlap)\n",first,buf->ID());
		return buf;
	};

	/*	The data overlaps into another chunk.  First reserve a chunk for use in the overlap... */
	overlap = GrabChunk();

	/*	Now we do the mem copy.  The 'overlap' chunk could very well be 'first'!  If it is,
		we do a memmove, but otherwise don't have to take any special action. */
	int32 copyFrom = filePos - fChunks[first].filePos;
	int32 sizeToCopy = fChunks[first].size - copyFrom;
	if (overlap == first)	memmove(fChunks[overlap].ptr,fChunks[first].ptr+copyFrom,sizeToCopy);
	else					memcpy (fChunks[overlap].ptr,fChunks[first].ptr+copyFrom,sizeToCopy);

	fChunks[overlap].size = size;
	fChunks[overlap].filePos = -1;
	fChunks[overlap].latestBuffer = toBePlayedAt;
	fChunks[overlap].wasOverlap = true;

	int32 second = PullChunk(filePos+size-1);
	memcpy(fChunks[overlap].ptr+sizeToCopy,fChunks[second].ptr,size-sizeToCopy);

	buf = fChunks[overlap].GrabBuffer();
//	printf("BUFFER: chunk=%d, buffer=%d (overlap)\n",overlap,buf->ID());
	buf->Header()->data_offset = 0;
	buf->Header()->size_used = size;
	buf->Header()->start_time = toBePlayedAt;
			
	ASSERT(overlap != second);
	return buf;
};

#endif
