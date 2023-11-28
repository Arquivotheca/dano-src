// multibuffer implementation
// +++++ to do
//
// - adjust default chunk size
//
// - sample format conversion
//
// - loosen restriction on buffer_size: the capture thread can deliver data via
//   BBuffers that differ in size from the internal driver buffers, but there's
//   no such flexibility in the format negotiation algorithm.
//
// - use media_realtime_init_* !!!

#include "GameADC.h"
#include "GameProducer.h"

#include <Buffer.h>
#include <BufferGroup.h>
#include <Locker.h>
#include <RealtimeAlloc.h>
#include <TimeSource.h>

#include <scheduler.h>

#include <Debug.h>
#include <errno.h>
#include <cstring>
#include <cmath>

#if DEBUG
#define ASSERT_LOCKED {ASSERT(_lock); ASSERT(_lock->IsLocked());}
#else
#define ASSERT_LOCKED
#endif

#define D_TROUBLE(x)	PRINT(x)
#define D_METHOD(x)		PRINT(x)
#define D_WARNING(x)	PRINT(x)
#define D_FORMAT(x)		PRINT(x)
#define D_REALTIME(x)	PRINT(x)

using namespace BPrivate;

// #pragma mark --- constants  ---
// ------------------------------------------------------------------------ //

const int32 DRIVER_BUFFER_COUNT      	= 4;

// #pragma mark --- internal classes ---
// ------------------------------------------------------------------------ //

class GameADC::Stream
{
public:
	Stream(GameADC* _adc);
	~Stream();

	void* operator new(size_t s);
	void operator delete(void* p, size_t s);

	int32 ID() const { return _id; }
	const media_multi_audio_format& Format() const { return _format; }
	bigtime_t SchedulingLatency() const { return _schedulingLatency; }

	status_t Open(
		const media_multi_audio_format& format,
		const game_format_spec& codecSpec,
		const game_format_spec& streamSpec,
		int32 bufferCount);
	bool IsOpened() const;
	status_t Close();

	status_t MarkForRemoval();
	bool IsRemovalPending() const;

	status_t Start();
	bool IsRunning() const;
	status_t Stop(bool wait =false);

	// set/create buffer group for given endpoint
	status_t SetBufferGroup(GameADC::Endpoint* endpoint, BBufferGroup* group);

public: // exposed members

	GameADC* const				adc;
	Endpoint*					endpoints;
	Stream*						next;

private: // guts

	// GAME_NO_ID if not open
	int32						_id;
	media_multi_audio_format	_format;
	int32						_bufferID;
	area_id						_area;
	area_id						_localArea;
	size_t						_offset;
	void*						_data;
	size_t						_pageCount;
	sem_id						_sem;

	double						_usecsPerFrame;

	thread_id					_thread;
	volatile bool				_run;
	bigtime_t					_schedulingLatency;
	volatile bool				_streamRunning;

	volatile bool				_removed;

	status_t AllocateBuffers(int32 count);
	status_t FreeBuffers();
	void ClearEndpoints();
		
	// guts
	static status_t CaptureThreadEntry(void* cookie);
	void CaptureThread();

	void WaitForThread();
};

// ------------------------------------------------------------------------ //

class GameADC::Endpoint
{
public:
	Endpoint(
		const media_multi_audio_format& f,
		const media_source& s,
		const media_destination& d);
	~Endpoint();

	void* operator new(size_t s);
	void operator delete(void* p, size_t s);
	
	void ClearBufferGroup();

	bool						enabled;
	media_multi_audio_format	format;
	media_source				source;
	media_destination			destination;

	BBufferGroup*				group;
	BBuffer**					internalBuffers;
	
	uint64						frameCount;
	BBuffer*					deferredBuffer;
	
	bool						removed;

	Endpoint*					next;
};

// #pragma mark --- GameADC::Stream ---
// ------------------------------------------------------------------------ //


GameADC::Stream::Stream(GameADC* _adc) :
	adc(_adc),
	endpoints(0),
	next(0),
	_id(GAME_NO_ID),
	_bufferID(GAME_NO_ID),
	_area(-1),
	_localArea(-1),
	_offset(0),
	_data(0),
	_pageCount(0),
	_sem(-1),
	_thread(-1),
	_run(false),
	_schedulingLatency(500LL),
	_streamRunning(false),
	_removed(false)
{
	ASSERT(adc);
	D_METHOD(("GameADC::Stream::Stream()\n"));
}

GameADC::Stream::~Stream()
{
	D_METHOD(("GameADC::Stream::~Stream()\n"));
	if (IsRunning()) Stop();
	if (IsOpened()) Close();
	ClearEndpoints();
}

void *
GameADC::Stream::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameADC::Stream::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

status_t 
GameADC::Stream::Open(
	const media_multi_audio_format& format,
	const game_format_spec& codecSpec,
	const game_format_spec& streamSpec,
	int32 bufferCount)
{
	D_METHOD(("GameADC::Stream::Open()\n"));
	status_t err;
	// consistency check
	if (_sem >= B_OK)
	{
		PRINT((
			"GameADC::Stream::Open():\n\t"
			"already open (sem)\n"));
		return B_NOT_ALLOWED;
	}
	if (_id != GAME_NO_ID)
	{
		PRINT((
			"GameADC::Stream::Open():\n\t"
			"already open (id)\n"));
		return B_NOT_ALLOWED;
	}
	if (_removed)
	{
		PRINT((
			"GameADC::Stream::Open():\n\t"
			"stream removal pending\n"));
		return B_NOT_ALLOWED;
	}

	// open the stream
	char sem_name[64];
	sprintf(sem_name, "%s.buffers.%x",
		adc->Name(),
		_id);
	_sem = create_sem(bufferCount-1, sem_name);
	if (_sem < B_OK)
	{
		PRINT((
			"GameADC::Stream::Open():\n\t"
			"create_sem(): %s\n",
			strerror(_sem)));
		return _sem;
	}
	G<game_open_stream> gos;
	gos.codec_id = adc->ID();
	gos.request = 0; // +++++ volume/pan?
	gos.stream_sem = _sem;
	
	// configure stream
	if ((streamSpec.frame_rate & B_SR_CVSR) &&
		streamSpec.cvsr != codecSpec.cvsr)
	{
		gos.request |= GAME_STREAM_FR;
		gos.cvsr_rate = streamSpec.cvsr;
	}
	else
	if (streamSpec.frame_rate != codecSpec.frame_rate)
	{
		gos.request |= GAME_STREAM_FR;
		gos.frame_rate = streamSpec.frame_rate;
	}
	if (streamSpec.channel_count != codecSpec.channel_count)
	{
		gos.request |= GAME_STREAM_CHANNEL_COUNT;
		gos.channel_count = streamSpec.channel_count;
	}
	if (streamSpec.format != codecSpec.format)
	{
		gos.request |= GAME_STREAM_FORMAT;
		gos.format = streamSpec.format;
	}
	
	if (ioctl(adc->DeviceFD(), GAME_OPEN_STREAM, &gos) < 0)
	{
		D_TROUBLE((
			"GameADC::Stream::Open():\n\t"
			"GAME_OPEN_STREAM: %s\n",
			strerror(errno)));
		delete_sem(_sem);
		_sem = -1;
	}
	
	// update state
	_id = gos.out_stream_id;
	_format = format;
	_usecsPerFrame = 1e6 / double(_format.frame_rate);

	// allocate buffers
	err = AllocateBuffers(bufferCount);
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameADC::Stream::Open():\n\t"
			"AllocateBuffers(%ld): %s\n",
			bufferCount, strerror(err)));

		// clean up & bail out
		Close();
		return err;
	}
	
	return B_OK;
}

bool 
GameADC::Stream::IsOpened() const
{
	return _id != GAME_NO_ID;
}


status_t 
GameADC::Stream::Close()
{
	D_METHOD(("GameADC::Stream::Close()\n"));
	status_t err;

	// stop
	if (IsRunning())
	{
		err = Stop();
		if (err < B_OK)
		{
			PRINT((
				"GameADC::Stream::Close():\n\t"
				"Stop(): %s\n",
				strerror(err)));
		}
	}
	// clean up
	FreeBuffers();
	// close
	G<game_close_stream> gcs;
	gcs.stream = _id;
	gcs.flags =
		GAME_CLOSE_DELETE_SEM_WHEN_DONE;
	if (ioctl(adc->DeviceFD(), GAME_CLOSE_STREAM, &gcs) < 0)
	{
		PRINT((
			"GameADC::Stream::Close():\n\t"
			"GAME_CLOSE_STREAM: %s\n",
			strerror(errno)));
	}
	_sem = -1;
	_id = GAME_NO_ID;
	return err;
}

status_t 
GameADC::Stream::MarkForRemoval()
{
	D_METHOD(("GameADC::Stream::MarkForRemoval()\n"));
	if (_removed) return B_NOT_ALLOWED;
	_removed = true;
	return B_OK;
}

bool 
GameADC::Stream::IsRemovalPending() const
{
	return _removed;
}

status_t 
GameADC::Stream::Start()
{
	D_METHOD(("GameADC::Stream::Start()\n"));
	ASSERT(adc->Locker()->IsLocked());
	status_t err;
	// consistency check
	if (!IsOpened())
	{
		D_WARNING((
			"GameADC::Stream::Start():\n\t"
			"not open\n"));
		return B_NOT_ALLOWED;
	}
	if (IsRunning())
	{
		D_WARNING((
			"GameADC::Stream::Start():\n\t"
			"already running\n"));
		return B_NOT_ALLOWED;
	}
	if (_removed)
	{
		D_WARNING((
			"GameADC::Stream::Start():\n\t"
			"stream removal pending\n"));
		return B_NOT_ALLOWED;
	}
	// wait for existing thread to stop, if any
	WaitForThread();	
	// spawn thread
	char thread_name[64];
	sprintf(thread_name, "%s.capture.%x",
		adc->Name(),
		_id);
	thread_id tid = spawn_thread(
		CaptureThreadEntry,
		thread_name,
		B_REAL_TIME_PRIORITY,
		this);
	if (tid < B_OK)
	{
		D_TROUBLE((
			"GameADC::Stream::Start():\n\t"
			"spawn_thread(): %s\n",
			strerror(tid)));
		return tid;
	}
	// go
	_schedulingLatency = estimate_max_scheduling_latency(tid);
	_run = true;
	resume_thread(tid);
	_thread = tid;
	return B_OK;
}

bool 
GameADC::Stream::IsRunning() const
{
	return _run;
}

status_t 
GameADC::Stream::Stop(bool wait)
{
	D_METHOD(("GameADC::Stream::Stop()\n"));
	ASSERT(adc->Locker()->IsLocked());
	status_t err;
	// consistency check
	if (!IsOpened())
	{
		D_WARNING((
			"GameADC::Stream::Stop():\n\t"
			"not open\n"));
		return B_NOT_ALLOWED;
	}
	if (!_run)
	{
		PRINT((
			"GameADC::Stream::Stop():\n\t"
			"not running\n"));
		return B_NOT_ALLOWED;
	}
	thread_id tid = _thread;
	if (tid < B_OK)
	{
		// this shouldn't happen
		PRINT((
			"GameADC::Stream::Stop():\n\t"
			"warning: no thread!\n"));
	}
	// tell the thread to die
	_run = false;
	WaitForThread();

	// stop the stream
	if (_streamRunning)
	{
		H<game_run_stream> grs;
		grs.stream = _id;
		grs.state = GAME_STREAM_STOPPED;
		G<game_run_streams> grss;
		grss.streams = &grs;
		grss.in_stream_count = 1;
		if (ioctl(adc->DeviceFD(), GAME_RUN_STREAMS, &grss) < 0)
		{
			D_WARNING((
				"GameADC::Stream::Stop():\n\t"
				"GAME_RUN_STREAMS[ STOP ]: %s\n",
				strerror(errno)));
		}
		_streamRunning = false;
	}

	D_METHOD(("GameADC::Stream::Stop() done.\n"));
	return B_OK;
}


void 
GameADC::Stream::ClearEndpoints()
{
	BLocker* const lock = adc->Locker();
	lock->Lock();
	D_METHOD(("GameADC::Stream::ClearEndpoints()\n"));
	for (Endpoint* e = endpoints; e;)
	{
		Endpoint* prev = e;
		e = e->next;
		delete prev;
	}
	endpoints = 0;
	lock->Unlock();
}

status_t 
GameADC::Stream::SetBufferGroup(GameADC::Endpoint* e, BBufferGroup* newGroup)
{
	D_METHOD(("GameADC::Stream::SetBufferGroup()\n"));
	ASSERT(adc->Locker()->IsLocked());
	ASSERT(e);
	status_t err;

	e->ClearBufferGroup();
	if (newGroup)
	{
		e->group = newGroup;
	}
	else
	{
		// chop up a single ping-pong buffer into chunks
		ASSERT(_pageCount > 1);
		ASSERT(_format.buffer_size > 0);
		ASSERT(_data);

		// generate cloned buffers
		e->group = new BBufferGroup();
		e->internalBuffers = (BBuffer**)rtm_alloc(0, sizeof(BBuffer*) * _pageCount);
		
		size_t offset = 0;
		for (int n = 0; n < _pageCount; n++, offset += _format.buffer_size)
		{
			buffer_clone_info clone;
			clone.area = _area;
			clone.offset = _offset + offset;
			clone.size = _format.buffer_size;
			clone.flags = 0;	
			err = e->group->AddBuffer(clone, e->internalBuffers + n);
			if (err < B_OK)
			{
				PRINT((
					"GameADC::Stream::SetBufferGroup():\n\t"
					"BBufferGroup::AddBuffer(): %s\n",
					strerror(err)));

				// clean up & bail
				e->ClearBufferGroup();
				return err;
			}
		}
	}
	return B_OK;
}


status_t 
GameADC::Stream::AllocateBuffers(int32 count)
{
	D_METHOD(("GameADC::Stream::AllocateBuffers()\n"));
	status_t err;
	// sanity check
	if (IsRunning())
	{
		PRINT((
			"GameADC::Stream::AllocateBuffers():\n\t"
			"stream currently running\n"));
		return B_NOT_ALLOWED;
	}
	if (_id == GAME_NO_ID)
	{
		PRINT((
			"GameADC::Stream::AllocateBuffers():\n\t"
			"no stream\n"));
		return B_NOT_ALLOWED;
	}
	if (_format.buffer_size <= 0)
	{
		PRINT((
			"GameADC::Stream::AllocateBuffers():\n\t"
			"invalid buffer size\n"));
		return B_BAD_VALUE;
	}
	// clean up
	if (_bufferID != GAME_NO_ID)
	{
		FreeBuffers();
	}
	
	H<game_open_stream_buffer> gosb;
	gosb.stream = _id;
	gosb.byte_size = _format.buffer_size * count;
	G<game_open_stream_buffers> gosbs;
	gosbs.in_request_count = 1;
	gosbs.buffers = &gosb;
	if (ioctl(adc->DeviceFD(), GAME_OPEN_STREAM_BUFFERS, &gosbs) < 0)
	{
		D_TROUBLE((
			"GameADC::Stream::AllocateBuffers():\n\t"
			"GAME_OPEN_STREAM_BUFFERS: %s\n", strerror(errno)));
		return errno;
	}
	
	_bufferID = gosb.buffer;
	_area = gosb.area;
	_offset = gosb.offset;
	_pageCount = count;

	_localArea = clone_area("adc.local", &_data, B_ANY_ADDRESS, B_READ_AREA|B_WRITE_AREA, _area);
	if (_localArea < 0)
	{
		err = _localArea;
		D_TROUBLE((
			"GameADC::Stream::AllocateBuffers():\n\t"
			"clone_area(): %s\n", strerror(err)));
		_localArea = 0;
		FreeBuffers();
		return err;
	}
	_data = (int8*)_data + _offset;

	return B_OK;	
}

status_t 
GameADC::Stream::FreeBuffers()
{
	D_METHOD(("GameADC::Stream::FreeBuffers()\n"));
	if (IsRunning())
	{
		PRINT((
			"GameADC::Stream::FreeBuffers():\n\t"
			"stream currently running\n"));
		return B_NOT_ALLOWED;
	}
	if (_localArea >= 0)
	{
		delete_area(_localArea);
		_localArea = -1;
	}
	if (_bufferID != GAME_NO_ID)
	{
		H<game_close_stream_buffer> gcsb;
		gcsb.buffer = _bufferID;
		G<game_close_stream_buffers> gcsbs;
		gcsbs.in_request_count = 1;
		gcsbs.buffers = &gcsb;
		if (ioctl(adc->DeviceFD(), GAME_CLOSE_STREAM_BUFFERS, &gcsbs) < 0)
		{
			D_WARNING((
				"GameADC::Stream::FreeBuffers():\n\t"
				"GAME_CLOSE_STREAM_BUFFERS: %s\n", strerror(errno)));
		}
		_bufferID = -1;
		_area = -1;
	}
	_data = 0;
	_pageCount = 0;
	return B_OK;
}

status_t 
GameADC::Stream::CaptureThreadEntry(void *cookie)
{
	((GameADC::Stream*)cookie)->CaptureThread();
	return B_OK;
}

void 
GameADC::Stream::CaptureThread()
{
	D_METHOD(("GameADC::Stream::CaptureThread()\n"));
	status_t err;

	BLocker* const lock = adc->Locker();
	
	// *** begin critical section ***
	lock->Lock();

	// init constants
	const int fd = adc->DeviceFD();
	if (fd < B_OK)
	{
		D_TROUBLE((
			"GameADC::Stream::CaptureThread():\n\t"
			"FD error: %s\n",
			strerror(fd)));
		lock->Unlock();
		return;
	}
	const media_multi_audio_format format = _format;
	if (!format.format ||
		!format.channel_count)
	{
#if DEBUG
		char fmt_buf[256];
		media_format f;
		f.type = B_MEDIA_RAW_AUDIO;
		f.u.raw_audio = format;
		string_for_format(f, fmt_buf, 255);
		D_TROUBLE((
			"GameADC::Stream::CaptureThread():\n\t"
			"invalid format: %s\n",
			fmt_buf));
#endif
		lock->Unlock();
		return;
	}
	const int32 bufferFrames = format.buffer_size /
		((format.format & 0x0f) * format.channel_count);
	if (_pageCount < 2)
	{
		D_TROUBLE((
			"GameADC::Stream::CaptureThread():\n\t"
			"capture buffer too small!\n"));
		lock->Unlock();
		return;
	}
	const size_t bufferCount = _pageCount;

	const int streamID = _id;
	const sem_id sem = _sem;
	
	// init endpoint state
	for (Endpoint* e = endpoints; e; e = e->next)
	{
		e->frameCount = 0;
		e->deferredBuffer = 0;
	}

	// get the device going
	G<game_set_stream_buffer> gssb;
	gssb.stream = streamID;
	gssb.flags = GAME_BUFFER_PING_PONG;
	gssb.frame_count = bufferFrames * bufferCount;
	gssb.page_frame_count = bufferFrames;
	gssb.buffer = _bufferID;
	if (ioctl(fd, GAME_SET_STREAM_BUFFER, &gssb) < 0)
	{
		D_TROUBLE((
			"GameADC::Stream::CaptureThread():\n\t"
			"GAME_SET_STREAM_BUFFER: %s\n",
			strerror(errno)));
		lock->Unlock();
		return;
	}
	H<game_run_stream> grs;
	grs.stream = streamID;
	grs.state = GAME_STREAM_RUNNING;
	G<game_run_streams> grss;
	grss.in_stream_count = 1;
	grss.streams = &grs;
	if (ioctl(fd, GAME_RUN_STREAMS, &grss) < 0)
	{
		D_TROUBLE((
			"GameADC::Stream::CaptureThread():\n\t"
			"GAME_RUN_STREAMS: %s\n",
			strerror(errno)));
		lock->Unlock();
		return;
	}
	_streamRunning = true;
	bigtime_t startTime = grs.out_timing.at_time;
	uint32 lastFramesPlayed = grs.out_timing.frames_played;

	// *** end critical section ***
	lock->Unlock();

	int32 deliverIndex = 0;
		
	while (_run)
	{
		// wait for buffer to be filled
		G<game_get_timing> ggt;
		ggt.source = streamID;
		ggt.flags = GAME_ACQUIRE_STREAM_SEM;
		ggt.timeout_at = B_INFINITE_TIMEOUT;
		if (ioctl(fd, GAME_GET_TIMING, &ggt) < 0)
		{
			D_TROUBLE((
				"GameADC::Stream::CaptureThread():\n\t"
				"GAME_GET_TIMING: %s\n",
				strerror(errno)));
			break;
		}
		
		// time to die?
		do {
			if (!_run) return;
		} while(lock->LockWithTimeout(100000)<B_OK);

		// handle frames_played wraparound
		uint32 adjustedFramesPlayed = ggt.timing.frames_played;
		bool framesPlayedWrapped = (adjustedFramesPlayed < lastFramesPlayed);
		if (framesPlayedWrapped) adjustedFramesPlayed += (ULONG_MAX - lastFramesPlayed);
		lastFramesPlayed = ggt.timing.frames_played;

		// service endpoints
		BTimeSource* timeSource = adc->Node()->TimeSource();

		for (Endpoint* e = endpoints; e; e = e->next)
		{
			if (e->removed)
			{
				// endpoint slated for deletion
				continue;
			}
			if (!e->enabled)
			{
				// endpoint muted: account for frames missed and move on.
				e->frameCount += bufferFrames;
				continue;
			}
			if (e->destination == media_destination::null)
			{
				D_WARNING((
					"GameADC::Stream::CaptureThread():\n\t"
					"no destination for endpoint %ld\n",
					e->source.id));
				continue;
			}

			if (framesPlayedWrapped) e->frameCount -= ULONG_MAX;
			int64 frameCountDelta = e->frameCount - adjustedFramesPlayed;

			// deliver
			if (e->internalBuffers)
			{
				// internal buffer: find corresponding user buffer

				BBuffer* buf = e->internalBuffers[deliverIndex];
				err = e->group->RequestBuffer(buf);
				if (err < B_OK)
				{
					D_WARNING((
						"GameADC::Stream::CaptureThread():\n\t"
						"RequestBuffer(index %ld): %s\n",
						deliverIndex, strerror(err)));
					continue;
				}
				
				// +++++ take device framerate into account
				bigtime_t bufferTime =
					ggt.timing.at_time +
					bigtime_t(
						_usecsPerFrame * double(frameCountDelta));

				buf->Header()->start_time = timeSource->PerformanceTimeFor(bufferTime);
				buf->Header()->size_used = format.buffer_size;
				buf->Header()->time_source = timeSource->ID();
				buf->Header()->type = B_MEDIA_RAW_AUDIO;
				err = adc->Node()->SendBuffer(buf, e->destination);
				if (err < B_OK)
				{
					D_WARNING((
						"GameADC::Stream::CaptureThread():\n\t"
						"SendBuffer(clone %ld -> %ld): %s\n",
						buf->ID(), e->destination.id, strerror(err)));
				}
				
				e->frameCount += bufferFrames;
			}
			else
			{
				// external buffer: copy data into one or more user-provided
				// buffers

				ASSERT(deliverIndex < bufferCount);
				size_t srcRemaining = format.buffer_size;
				size_t srcOffset = 0;
				
				const size_t destBufSize = e->format.buffer_size;
				const uint32 destBufFrames = destBufSize /
					((e->format.format & 0x0f) * e->format.channel_count);

				while (srcRemaining > 0)
				{
					// get next outbound buffer
					BBuffer* buf = 0;
					if (e->deferredBuffer)
					{
						buf = e->deferredBuffer;
						e->deferredBuffer = 0;
					}
					else
					{
						buf = e->group->RequestBuffer(destBufSize);
						if (!buf)
						{
							D_WARNING((
								"GameADC::Stream::CaptureThread():\n\t"
								"RequestBuffer(): %s\n",
								strerror(err)));
							break;
						}
						buf->Header()->size_used = 0;
					}
					
					// fill buffer
					size_t chunk = destBufSize - buf->Header()->size_used;
					if (chunk > srcRemaining) chunk = srcRemaining;
					memcpy(
						(int8*)buf->Data() + buf->Header()->size_used,
						(int8*)_data + srcOffset,
						chunk);
					srcOffset += chunk;
					srcRemaining -= chunk;
					buf->Header()->size_used += chunk;
					
					if (buf->Header()->size_used < destBufSize)
					{
						// incomplete [final] buffer to be sent when more
						// data is available
						ASSERT(!srcRemaining);
						e->deferredBuffer = buf;
					}
					else
					{
						// ready to go
						// +++++ take device framerate into account
						bigtime_t bufferTime =
							ggt.timing.at_time +
							bigtime_t(
								_usecsPerFrame * double(frameCountDelta));
		
						buf->Header()->start_time = timeSource->PerformanceTimeFor(bufferTime);
						buf->Header()->size_used = destBufSize;
						buf->Header()->time_source = timeSource->ID();
						buf->Header()->type = B_MEDIA_RAW_AUDIO;
						err = adc->Node()->SendBuffer(buf, e->destination);
						if (err < B_OK)
						{
							D_WARNING((
								"GameADC::Stream::CaptureThread():\n\t"
								"SendBuffer(%ld -> %ld): %s\n",
								buf->ID(), e->destination.id, strerror(err)));
						}
						
						e->frameCount += destBufFrames;
					}
				} // while (srcRemaining > 0)
			}
		} // for (Endpoint* e ...

		// *** end critical section ***
		lock->Unlock();

		deliverIndex = (deliverIndex+1) % bufferCount;		
	}

	D_REALTIME(("GameADC::Stream::CaptureThread() done.\n"));
}

void 
GameADC::Stream::WaitForThread()
{
	D_METHOD(("GameADC::Stream::WaitForThread()\n"));
	status_t err;
	thread_id tid = _thread;
	if (tid < B_OK) return;
	while (wait_for_thread(tid, &err) == B_INTERRUPTED) {}
	_thread = -1;
	D_REALTIME(("\tWaitForThread() complete.\n"));
}

// #pragma mark --- GameADC::Endpoint ---
// ------------------------------------------------------------------------ //


GameADC::Endpoint::Endpoint(
	const media_multi_audio_format &f, const media_source &s, const media_destination &d) :
	enabled(true),
	format(f),
	source(s),
	destination(d),
	group(0),
	internalBuffers(0),
	deferredBuffer(0),
	removed(false),
	next(0)
{
	D_METHOD(("GameADC::Endpoint::Endpoint(0x%x)\n", source.id));
}

GameADC::Endpoint::~Endpoint()
{
	D_METHOD(("GameADC::Endpoint::~Endpoint()\n"));
	ClearBufferGroup();
}

void *
GameADC::Endpoint::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameADC::Endpoint::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

void 
GameADC::Endpoint::ClearBufferGroup()
{
	D_METHOD(("GameADC::Endpoint::ClearBufferGroup()\n"));
	if (group)
	{
		delete group;
		group = 0;
	}
	if (internalBuffers)
	{
		rtm_free(internalBuffers);
		internalBuffers = 0;
	}
}



// #pragma mark --- GameADC::EndpointIterator ---
// ------------------------------------------------------------------------ //


GameADC::EndpointIterator::EndpointIterator() :
	stream(0), endpoint(0) {}

GameADC::EndpointIterator::EndpointIterator(const EndpointIterator &clone)
{
	operator=(clone);
}

GameADC::EndpointIterator &
GameADC::EndpointIterator::operator=(const EndpointIterator &clone)
{
	stream = clone.stream;
	endpoint = clone.endpoint;
	return *this;
}

void *
GameADC::EndpointIterator::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameADC::EndpointIterator::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

bool 
GameADC::EndpointIterator::IsValid() const
{
	return stream;
}

bool 
GameADC::EndpointIterator::IsValidEndpoint() const
{
	return stream && endpoint;
}


GameADC::EndpointIterator::EndpointIterator(GameADC::Stream *s) :
	stream(s), endpoint(s ? s->endpoints : 0) {}

GameADC::EndpointIterator::EndpointIterator(GameADC::Stream *s, GameADC::Endpoint *e) :
	stream(s), endpoint(e) {}

void 
GameADC::EndpointIterator::Advance()
{
	ASSERT(stream);
	if (endpoint)
	{
		endpoint = endpoint->next;
	}
	while (!endpoint && stream)
	{
		stream = stream->next;
		endpoint = (stream ? stream->endpoints : 0);
	}
}



// #pragma mark --- GameADC ---
// ------------------------------------------------------------------------ //

GameADC::GameADC(GameProducer* node, int32 adc_id) :
	_fd(node->DeviceFD()),
	_node(node),
	_lock(node->Locker()),
	_freeEndpoint(0),
	_streams(0),
	_nextEndpointOrdinal(0),
	_running(false),
	_iteratorCount(0)
{
	D_METHOD(("GameADC::GameADC()\n"));
	ASSERT(node);
	ASSERT_LOCKED;
	status_t err;

	// get ADC info
	memset(&_info, 0, sizeof(_info));
	_info.codec_id = adc_id;
	RefreshCodecInfo();	
}


GameADC::~GameADC()
{
	D_METHOD(("GameADC::~GameADC()\n"));
	ASSERT_LOCKED;
	if (_freeEndpoint)
		delete _freeEndpoint;
	ClearStreams();
}

void *
GameADC::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameADC::operator delete(void *p, size_t s)
{
	rtm_free(p);
}


int32 
GameADC::ID() const
{
	return _info.codec_id;
}

int 
GameADC::DeviceFD() const
{
	return _fd;
}

const char *
GameADC::Name() const
{
	return _info.name;
}

BLocker *
GameADC::Locker() const
{
	return _lock;
}

GameProducer *
GameADC::Node() const
{
	return _node;
}


// ------------------------------------------------------------------------ //

status_t 
GameADC::Start()
{
	D_METHOD(("GameADC::Start()\n"));
	ASSERT_LOCKED;
	status_t err;
	if (_running) return B_NOT_ALLOWED;
	for (Stream* s = _streams; s; s = s->next)
	{
		if (s->ID() == GAME_NO_ID)
			continue;
		err = s->Start();
		if (err < B_OK)
		{
			PRINT(("!!! GameADC::Stream[%d]::Start(): %s\n",
				s->ID(),
				strerror(err)));
		}
	}
	_running = true;
	return B_OK;
}

bool 
GameADC::IsRunning()
{
	ASSERT_LOCKED;
	return _running;
}

status_t 
GameADC::Stop(bool wait)
{
	D_METHOD(("GameADC::Stop()\n"));
	ASSERT_LOCKED;
	status_t err;
	if (!_running) return B_NOT_ALLOWED;
	for (Stream* s = _streams; s; s = s->next)
	{
		if (s->ID() == GAME_NO_ID)
			continue;
		err = s->Stop(wait);
		if (err < B_OK)
		{
			PRINT(("!!! GameAdc::Stream[%d]::Stop(): %s\n",
				s->ID(),
				strerror(err)));
		}
	}
	_running = false;
	return B_OK;
}

// ------------------------------------------------------------------------ //

status_t 
GameADC::GetNextOutput(
	int32 *ioCookie,
	media_multi_audio_format *outFormat,
	media_source *outSource,
	media_destination *outDestination)
{
	D_METHOD(("GameADC::GetNextOutput()\n"));
	ASSERT_LOCKED;
	EndpointIterator* it = (EndpointIterator*)*ioCookie;
	if (!it)
	{
		++_iteratorCount;
		it = new EndpointIterator(_streams);
		*ioCookie = int32(it);
		
		if (!_freeEndpoint)
		{
			// no streams available, so this ADC is pretty much useless.
			return B_ERROR;
		}

		// return free-connection
		*outFormat = _freeEndpoint->format;
		*outSource = _freeEndpoint->source;
		*outDestination = _freeEndpoint->destination;
		return B_OK;
	}

	while (it->stream)
	{
		ASSERT(it->endpoint);
		it->Advance();
		if (!it->stream) break;

		if (it->stream->IsRemovalPending() || it->endpoint->removed)
		{
			// don't report deleted endpoints
			continue;
		}

		*outFormat = it->endpoint->format;
		*outSource = it->endpoint->source;
		*outDestination = it->endpoint->destination;
		return B_OK;
	}

	// out of endpoints to report	
	return B_ERROR;
}

status_t 
GameADC::DisposeOutputCookie(int32 cookie)
{
	D_METHOD(("GameADC::DisposeOutputCookie()\n"));
	ASSERT_LOCKED;
	if (cookie)
	{
		ASSERT(_iteratorCount > 0);
		if (!--_iteratorCount)
			Reap();
		delete (EndpointIterator*)cookie;
	}
	return B_OK;
}

// ------------------------------------------------------------------------ //

status_t 
GameADC::ValidateFormat(media_multi_audio_format *ioFormat, int32* outStreamID) const
{
	D_METHOD(("GameADC::ValidateFormat()\n"));
	ASSERT_LOCKED;
	status_t ret = B_OK;
	status_t err;
	media_multi_audio_format& w = media_multi_audio_format::wildcard;

	media_multi_audio_format required;
	media_multi_audio_format preferred;
	
	// take a fresh snapshot of the codec state.
	const_cast<GameADC*>(this)->RefreshCodecInfo();
	
	if (_info.cur_stream_count < _info.max_stream_count)
	{
		GetRequiredFormat(&required);
		GetPreferredFormat(required, &preferred);
		*outStreamID = GAME_NO_ID;
	}
	else
	{
		// constrain to a currently existing stream's format
		required = *ioFormat;
		err = FindNearestStreamFormat(&required, outStreamID);
		if (err < B_OK)
		{
			D_WARNING((
				"GameADC::ValidateFormat():\n\t"
				"no streams available.\n"));
			return B_ERROR;
		}

		// no wildcards should exist at this point
		ASSERT(required.frame_rate != w.frame_rate);
		ASSERT(required.channel_count != w.channel_count);
		ASSERT(required.format != w.format);
		ASSERT(required.byte_order != w.byte_order);
		ASSERT(required.buffer_size != w.buffer_size);
		preferred = required;
	}
	
	// specialize/correct
	if (ioFormat->frame_rate == w.frame_rate)
		ioFormat->frame_rate = preferred.frame_rate;
	else
	if (required.frame_rate == w.frame_rate)
	{
		// apply frame-rate constraints
		err = constrain_frame_rate(_info, &ioFormat->frame_rate);
		if (err < B_OK)
			ret = err;
	}
	else
	if (ioFormat->frame_rate != required.frame_rate)
	{
		ret = B_MEDIA_BAD_FORMAT;
		ioFormat->frame_rate = required.frame_rate;
	}

	if (ioFormat->format == w.format)
		ioFormat->format = preferred.format;
	else
	if (required.format == w.format)
	{
		// apply format constraints
		err = constrain_format(_info, &ioFormat->format, &ioFormat->valid_bits);
		if (err < B_OK)
			ret = err;
	}
	else
	if (ioFormat->format != required.format)
	{
		ret = B_MEDIA_BAD_FORMAT;
		ioFormat->format = required.format;
	}
	
	if (ioFormat->channel_count == w.channel_count)
		ioFormat->channel_count = preferred.channel_count;
	else
	if (required.channel_count == w.channel_count)
	{
		// apply channel-count constraints
		err = constrain_channel_count(_info, &ioFormat->channel_count);
		if (err < B_OK)
			ret = err;
	}
	else
	if (ioFormat->channel_count != required.channel_count)
	{
		ret = B_MEDIA_BAD_FORMAT;
		ioFormat->channel_count = required.channel_count;
	}

	if (ioFormat->byte_order == w.byte_order)
		ioFormat->byte_order = required.channel_count;
	else
	if (ioFormat->byte_order != required.byte_order)
	{
		ASSERT(required.byte_order != w.byte_order);
		ret = B_MEDIA_BAD_FORMAT;
		ioFormat->byte_order = required.byte_order;
	}

	// buffer-size constraint comes last, since it depends on format & channel-count
	err = constrain_buffer_size(
		ioFormat->format,
		ioFormat->channel_count,
		_info.cur_chunk_frame_count,
		&ioFormat->buffer_size);
	if (err < B_OK)
		ret = err;

	return ret;
}

void 
GameADC::GetRequiredFormat(media_multi_audio_format *oFormat) const
{
	D_METHOD(("GameADC::GetRequiredFormat()\n"));
	ASSERT_LOCKED;
	media_multi_audio_format& w = media_multi_audio_format::wildcard;

	// base format on current ADC configuration
	*oFormat = _format;

	// non- or partially-committed device: individual streams may
	// be somewhat configurable; if no streams exist, the ADC may be
	// configurable as well.
	if (_info.cur_stream_count < _info.max_stream_count)
	{
		if (!_info.cur_stream_count)
		{
			// look for some amount of configurability at the ADC level.
			if ((_info.frame_rates & B_SR_CVSR) ||
				count_1_bits(_info.frame_rates) > 1)
			{
				oFormat->frame_rate = w.frame_rate;
			}
			if (count_1_bits(_info.channel_counts) > 1)
			{
				oFormat->channel_count = w.channel_count;
			}
			if (count_1_bits(_info.formats) > 1)
			{
				oFormat->format = w.format;
			}
		}
		
		if (_info.stream_capabilities.capabilities & GAME_STREAMS_HAVE_FRAME_RATE)
			oFormat->frame_rate = w.frame_rate;
		if (_info.stream_capabilities.capabilities & GAME_STREAMS_HAVE_CHANNEL_COUNT)
			oFormat->channel_count = w.channel_count;
		if (_info.stream_capabilities.capabilities & GAME_STREAMS_HAVE_FORMAT)
			oFormat->format = w.format;
	}
	// fully committed device: mark fields that differ between
	// current streams' formats and the ADC format as wildcards (formats will
	// be constrained to that of a currently matching stream in ValidateFormat().)
	else
	{
		for (Stream* s = _streams; s; s = s->next)
		{
			if (oFormat->frame_rate != w.frame_rate &&
				oFormat->frame_rate != s->Format().frame_rate)
				oFormat->frame_rate = w.frame_rate;
			if (oFormat->channel_count != w.channel_count &&
				oFormat->channel_count != s->Format().channel_count)
				oFormat->channel_count = w.channel_count;
			if (oFormat->format != w.format &&
				oFormat->format != s->Format().format)
				oFormat->format = w.format;
		}
	}
}

void 
GameADC::GetPreferredFormat(const media_multi_audio_format &required, media_multi_audio_format *oPreferred) const
{
	D_METHOD(("GameADC::GetPreferredFormat()\n"));
	ASSERT_LOCKED;
	media_multi_audio_format& w = media_multi_audio_format::wildcard;

	*oPreferred = required;
	if (oPreferred->frame_rate == w.frame_rate)
	{
		// current ADC frame rate
		if (_info.cur_frame_rate & B_SR_CVSR)
			oPreferred->frame_rate = _info.cur_cvsr;
		else
		{
			for (int n = 0; n < sizeof(fixed_rates)/sizeof(*fixed_rates); n++)
			{
				if (fixed_rates[n].flag == _info.cur_frame_rate)
				{
					oPreferred->frame_rate = fixed_rates[n].rate;
					break;
				}
			}
			if (oPreferred->frame_rate == w.frame_rate)
			{
				PRINT((
					"GameADC::GetPreferredFormat\n\t"
					"invalid current fixed rate from driver: 0x%x; defaulting to 44100\n",
					_info.cur_frame_rate));
				oPreferred->frame_rate = 44100.0f;
			}
		}
	}
	if (oPreferred->format == w.format)
	{
		// current ADC format
		for (int n = 0; n < sizeof(format_map)/sizeof(*format_map); n++)
		{
			if (format_map[n].driver == _info.cur_format)
			{
				oPreferred->format = format_map[n].media;
				if (oPreferred->format == media_multi_audio_format::B_AUDIO_INT)
				{
					oPreferred->valid_bits = int_format_valid_bits(_info.cur_format);
				}
				break;
			}
			if (oPreferred->format == w.format)
			{
				PRINT((
					"GameADC::GetPreferredFormat\n\t"
					"invalid current format from driver: 0x%x; defaulting to short\n",
					_info.cur_format));
				oPreferred->format = media_multi_audio_format::B_AUDIO_SHORT;
			}
		}
	}
	if (oPreferred->channel_count == w.channel_count)
	{
		// current ADC channel_count
		oPreferred->channel_count = uint32(_info.cur_channel_count);
	}
}


status_t 
GameADC::FindNearestStreamFormat(media_multi_audio_format *ioFormat, int32 *outStreamID) const
{
	D_METHOD(("GameADC::FindNearestStreamFormat()\n"));
	ASSERT_LOCKED;

	const int16 frameRateScore 		= 4;
	const int16 channelCountScore	= 2;
	const int16 formatScore			= 1;
	
	media_multi_audio_format& w = media_multi_audio_format::wildcard;
	media_multi_audio_format bestFormat = media_multi_audio_format::wildcard;
	int32 bestStreamID = GAME_NO_ID;
	int16 bestScore = 0;
	
	for (Stream* s = _streams; s; s = s->next)
	{
		int16 score = 0;
		if (ioFormat->frame_rate != w.frame_rate &&
			ioFormat->frame_rate == s->Format().frame_rate)
			score += frameRateScore;

		if (ioFormat->channel_count != w.channel_count &&
			ioFormat->channel_count == s->Format().channel_count)
			score += channelCountScore;

		if (ioFormat->format != w.format &&
			ioFormat->format == s->Format().format)
			score += formatScore;
		
		if (score > bestScore)
		{
			bestFormat = s->Format();
			bestScore = score;
			bestStreamID = s->ID();
		}
	}
	
	if (bestStreamID != GAME_NO_ID)
	{
		*outStreamID = bestStreamID;
		*ioFormat = bestFormat;
		return B_OK;
	}
	else
	{
		// no streams
		return B_ERROR;
	}
}

// ------------------------------------------------------------------------ //

// linear search for matching endpoint/stream
status_t 
GameADC::FindEndpoint(
	int32 sourceID, EndpointIterator *outEndpoint) const
{
	D_METHOD(("GameADC::FindEndpoint()\n"));
	ASSERT_LOCKED;
	status_t err;
	Stream* s;
	Endpoint* e;
	for (s = _streams; s; s = s->next)
	{
		for (e = s->endpoints; e && e->source.id != sourceID; e = e->next) {}
		if (e) break;
	}
	if (!s)
	{
		PRINT((
			"GameADC::FindEndpoint():\n\t"
			"stream for source %ld not found\n",
			sourceID));
		return B_BAD_VALUE;
	}
	if (!e)
	{
		PRINT((
			"GameADC::FindEndpoint():\n\t"
			"endpoint for source %ld not found\n",
			sourceID));
		return B_BAD_VALUE;
	}
	if (outEndpoint)
	{
		outEndpoint->stream = s;
		outEndpoint->endpoint = e;
	}
	return B_OK;
}


status_t 
GameADC::ReserveEndpoint(media_multi_audio_format* ioFormat, int32 *outSourceID)
{
	D_METHOD(("GameADC::ReserveEndpoint()\n"));
	ASSERT_LOCKED;
	status_t err;
	
	// * locate/create stream

	Stream* s;

	// validate format
	int32 streamID = GAME_NO_ID;
	err = ValidateFormat(ioFormat, &streamID);
	if (err < B_OK)
	{
		char fmt_buf[256];
		media_format f;
		f.type = B_MEDIA_RAW_AUDIO;
		f.u.raw_audio = *ioFormat;
		string_for_format(f, fmt_buf, 255);
		D_FORMAT((
			"GameADC::ReserveEndpoint():\n\t"
			"ValidateFormat() failed: %s\n",
			fmt_buf));
		return err;
	}

	// if the format is only available for an existing stream, find it
	if (streamID != GAME_NO_ID)
	{
		for (s = _streams; s && s->ID() != streamID; s = s->next) {}
		if (!s)
		{
			D_TROUBLE((
				"GameADC::ReserveEndpoint():\n\t"
				"stream %d not found!\n",
				streamID));
			return B_BAD_VALUE;
		}				
	}
	else
	{
		// append new stream object to end of stream set
		Stream** sp;
		for (sp = &_streams; *sp; sp = &(*sp)->next) {}
		s = new Stream(this);
		*sp = s;
	}
	
	if (!_info.cur_stream_count)
	{
		// update ADC settings
		err = SetCodecFormat(*ioFormat, true);
		if (err < B_OK)
		{
			PRINT((
				"GameADC::ReserveEndpoint():\n\t"
				"SetCodecFormat(): %s\n",
				strerror(err)));
			return err;
		}
		err = RefreshCodecInfo();
		if (err < B_OK)
		{
			PRINT((
				"GameADC::ReserveEndpoint():\n\t"
				"RefreshCodecInfo(): %s\n",
				strerror(err)));
			return err;
		}
	}

	// open stream if necessary
	if (!s->IsOpened())
	{
		// current codec spec
		game_format_spec codecSpec;
		codecSpec.frame_rate = _info.cur_frame_rate;
		codecSpec.cvsr = _info.cur_cvsr;
		codecSpec.channel_count = _info.cur_channel_count;
		codecSpec.designations = 0; // +++++ ?
		codecSpec.format = _info.cur_format;		

		game_format_spec streamSpec = codecSpec;
		err = make_game_format_spec(
			*ioFormat,
			_info.stream_capabilities.frame_rates,
			_info.stream_capabilities.cvsr_min, _info.stream_capabilities.cvsr_max,
			_info.stream_capabilities.channel_counts,
			_info.stream_capabilities.designations,
			_info.stream_capabilities.formats,
			&streamSpec);
		if (err < B_OK)
		{
			PRINT((
				"GameADC::Stream::ReserveEndpoint():\n\t"
				"make_game_format_spec(): %s\n",
				strerror(err)));
			return err;
		}

		err = s->Open(*ioFormat, codecSpec, streamSpec, DRIVER_BUFFER_COUNT);
		if (err < B_OK)
		{
			PRINT((
				"GameADC::ReserveEndpoint():\n\t"
				"GameADC::Stream::Open(): %s\n",
				strerror(err)));
			return err;
		}
	}
	
	// create & populate endpoint
	media_source source(_node->Node().port, MakeEndpointID());
	if (source.id < 0) return B_ERROR;
	
	Endpoint* ep;
	for (ep = s->endpoints; ep && ep->next; ep = ep->next) {}
	Endpoint* e = new Endpoint(
		*ioFormat,
		source,
		media_destination::null);
	if (ep)
		ep->next = e;
	else
		s->endpoints = e;

	// set up a default buffer group
	err = s->SetBufferGroup(e, 0);
	if (err < B_OK)
	{
		PRINT((
			"GameADC::ReserveEndpoint():\n\t"
			"GameADC::Stream::SetBufferGroup(): %s\n",
			strerror(err)));
		return err;
	}

	// return source ID
	*outSourceID = source.id;
	return B_OK;
}

status_t 
GameADC::ConnectEndpoint(
	const EndpointIterator& it,
	const media_destination &destination,
	const media_multi_audio_format &format)
{
	D_METHOD(("GameADC::ConnectEndpoint()\n"));
	ASSERT_LOCKED;
	status_t err;
	if (!it.IsValidEndpoint())
	{
		PRINT((
			"GameADC::ConnectEndpoint():\n\t"
			"invalid endpoint\n"));
		return B_BAD_VALUE;
	}
	
	// connect
	it.endpoint->format = format;
	it.endpoint->destination = destination;
	
	// start
	if (_running && !it.stream->IsRunning())
	{
		err = it.stream->Start();
		if (err < B_OK)
		{
			PRINT((
				"GameADC::ConnectEndpoint():\n\t"
				"GameADC::Stream::Start(): %s\n",
				strerror(err)));
			return err;
		}
	}

	// +++++ what else needs to happen here?
	
	return B_OK;
}

// removing an endpoint will (eventually) result in the stream being closed
// if no other endpoints depend on it.

status_t 
GameADC::CancelEndpoint(const EndpointIterator& it)
{
	D_METHOD(("GameADC::CancelEndpoint()\n"));
	ASSERT_LOCKED;
	if (!it.IsValidEndpoint())
	{
		PRINT((
			"GameADC::CancelEndpoint():\n\t"
			"invalid endpoint\n"));
		return B_BAD_VALUE;
	}
	return RemoveEndpoint(it);
}

status_t 
GameADC::Disconnect(const EndpointIterator& it)
{
	D_METHOD(("GameADC::Disconnect()\n"));
	ASSERT_LOCKED;
	if (!it.IsValidEndpoint())
	{
		PRINT((
			"GameADC::Disconnect():\n\t"
			"invalid endpoint\n"));
		return B_BAD_VALUE;
	}
	return RemoveEndpoint(it);
}

status_t 
GameADC::Enable(const EndpointIterator& it, bool enabled)
{
	D_METHOD(("GameADC::Enable()\n"));
	ASSERT_LOCKED;
	if (!it.IsValidEndpoint())
	{
		PRINT((
			"GameADC::Enable():\n\t"
			"invalid endpoint\n"));
		return B_BAD_VALUE;
	}
	it.endpoint->enabled = enabled;
	return B_OK;
}

status_t 
GameADC::SetBufferGroup(const EndpointIterator& it, BBufferGroup *group)
{
	D_METHOD(("GameADC::SetBufferGroup()\n"));
	ASSERT_LOCKED;
	if (!it.IsValidEndpoint())
	{
		PRINT((
			"GameADC::SetBufferGroup():\n\t"
			"invalid endpoint\n"));
		return B_BAD_VALUE;
	}
	return it.stream->SetBufferGroup(it.endpoint, group);
}

status_t 
GameADC::GetInternalLatency(bigtime_t *outLatency)
{
	D_METHOD(("GameADC::GetInternalLatency()\n"));
	ASSERT_LOCKED;
	status_t err;
	ASSERT(_freeEndpoint);
	*outLatency = buffer_duration(_freeEndpoint->format);
	for (Stream* s = _streams; s; s = s->next)
	{
		bigtime_t base = s->SchedulingLatency();
		for (Endpoint* e = s->endpoints; e; e = e->next)
		{
			bigtime_t latency = buffer_duration(e->format);
			if (base + latency > *outLatency) *outLatency = base + latency;
		}
	}
	return B_OK;
}

// ------------------------------------------------------------------------ //

void 
GameADC::InitFreeEndpoint(int32 adcID)
{
	D_METHOD(("GameADC::InitFreeEndpoint()\n"));
	ASSERT(!_freeEndpoint);

	if (_info.cur_stream_count == _info.max_stream_count)
	{
		D_WARNING((
			"GameADC::InitFreeEndpoint():\n\t"
			"no free streams for this codec.\n"));
		return;
	}

	media_source source(_node->Node().port, adcID);

	// format description must be filled in
	_freeEndpoint = new Endpoint(
		media_multi_audio_format::wildcard,
		source,
		media_destination::null);
}

status_t 
GameADC::SetCodecFormat(const media_multi_audio_format &format, bool streamPrecedence)
{
	D_METHOD(("GameADC::SetCodecFormat()\n"));
	H<game_codec_format> gcf;
	G<game_set_codec_formats> gscf;
	gscf.formats = &gcf;
	gscf.in_request_count = 1;

	game_format_spec spec;
	spec.frame_rate = _info.cur_frame_rate;
	spec.cvsr = _info.cur_cvsr;
	spec.channel_count = _info.cur_channel_count;
	spec.designations = 0; // +++++ ?
	spec.format = _info.cur_format;		
	
	D_FORMAT(("SetCodecFormat() restrictions:\n 0x%x 0x%x 0x%x 0x%x\n",
		_info.frame_rates, _info.channel_counts, _info.designations, _info.formats));
	status_t err = make_game_format_spec(
		format,
		_info.frame_rates, _info.cvsr_min, _info.cvsr_max,
		_info.channel_counts, _info.designations, _info.formats,
		&spec);
	if (err < B_OK)
		return err;

	gcf.codec = _info.codec_id;
	gcf.flags = GAME_CODEC_FAIL_IF_DESTRUCTIVE;

	if (spec.frame_rate != _info.cur_frame_rate &&
		!(streamPrecedence &&
			(_info.stream_capabilities.frame_rates & spec.frame_rate)))
	{
		gcf.flags |= GAME_CODEC_SET_FRAME_RATE;
		gcf.frame_rate = spec.frame_rate;
		gcf.cvsr = spec.cvsr;
	}
	if (spec.channel_count != _info.cur_channel_count &&
		!(streamPrecedence &&
			(_info.stream_capabilities.channel_counts & (1 << spec.channel_count-1))))
	{
		gcf.flags |= GAME_CODEC_SET_CHANNELS;
		gcf.channels = spec.channel_count;
	}
	if (spec.format != _info.cur_format &&
		!(streamPrecedence &&
			(_info.stream_capabilities.formats & spec.format)))
	{
		gcf.flags |= GAME_CODEC_SET_FORMAT;
		gcf.format = spec.format;
	}
	
	return (ioctl(_fd, GAME_SET_CODEC_FORMATS, &gscf) < 0) ? errno : B_OK;
}

status_t 
GameADC::RefreshCodecInfo()
{
	D_METHOD(("GameADC::RefreshCodecInfo()\n"));
	status_t err;
	
	// get general ADC status
	G<game_get_codec_infos> ggai;
	ggai.info = &_info;
	ggai.in_request_count = 1;
	if (ioctl(_fd, GAME_GET_CODEC_INFOS, &ggai) < 0)
	{
		PRINT((
			"GameADC::UpdateADCInfo()\n\t"
			"GAME_GET_CODEC_INFOS(%d): %s\n",
			_info.codec_id, strerror(errno)));
		return errno;
	}
	
	D_FORMAT((
		"*** ADC info for 0x%x '%s':\n\t"
		"max_stream_count:              %d\n\t"
		"cur_stream_count:              %d\n\t"
		"min_chunk_frame_count:         %ld\n\t"
		"chunk_frame_count_increment:   %ld\n\t"
		"max_chunk_frame_count:         %ld\n\t"
		"cur_chunk_frame_count:         %ld\n\t"
		"frame_rates:                   0x%x\n\t"
		"cvsr_min, cvsr_max:            %.2f, %.2f\n\t"
		"formats:                       0x%x\n\t"
		"designations:                  0x%x\n\t"
		"channel_counts:                0x%x\n\t"
		"cur_frame_rate:                0x%x\n\t"
		"cur_cvsr:                      %.2f\n\t"
		"cur_format:                    0x%x\n\t"
		"cur_channel_count:             %ld\n",
		_info.codec_id, _info.name,
		_info.max_stream_count,
		_info.cur_stream_count,
		_info.min_chunk_frame_count,
		_info.chunk_frame_count_increment,
		_info.max_chunk_frame_count,
		_info.cur_chunk_frame_count,
		_info.frame_rates,
		_info.cvsr_min, _info.cvsr_max,
		_info.formats,
		_info.designations,
		_info.channel_counts,
		_info.cur_frame_rate,
		_info.cur_cvsr,
		_info.cur_format,
		_info.cur_channel_count));

	// describe ADC format
	_format = media_multi_audio_format::wildcard;

	_format.byte_order = B_MEDIA_HOST_ENDIAN;
	_format.channel_count = _info.cur_channel_count;
	if (_info.cur_frame_rate & B_SR_CVSR)
	{
		_format.frame_rate = _info.cur_cvsr;
	}
	else
	{
		for (int32 n = 0; n < sizeof(fixed_rates)/sizeof(*fixed_rates); n++)
		{
			if (_info.cur_frame_rate == fixed_rates[n].flag)
			{
				_format.frame_rate = fixed_rates[n].rate;
				break;
			}
		}
		ASSERT(_format.frame_rate != media_multi_audio_format::wildcard.frame_rate);
	}
	for (int32 n = 0; n < sizeof(format_map)/sizeof(*format_map); n++)
	{
		if (_info.cur_format == format_map[n].driver)
		{
			_format.format = format_map[n].media;
			break;
		}
	}
	ASSERT(_format.format != media_multi_audio_format::wildcard.format);
	if (_format.format == media_multi_audio_format::B_AUDIO_INT)
	{
		_format.valid_bits = int_format_valid_bits(_info.cur_format);
	}
	constrain_buffer_size(
		_format.format,
		_format.channel_count,
		_info.cur_chunk_frame_count,
		&_format.buffer_size);

	if (!_freeEndpoint)
	{
		InitFreeEndpoint(_info.codec_id);
	}

	if (_freeEndpoint)
	{
		// update endpoint format
		GetRequiredFormat(&_freeEndpoint->format);
	}	
	return B_OK;
}

void 
GameADC::ClearStreams()
{
	D_METHOD(("GameADC::ClearStreams()\n"));
	ASSERT_LOCKED;
	for (Stream* s = _streams; s;)
	{
		Stream* ds = s;
		s = s->next;
		delete ds;
	}
	_streams = 0;
}

status_t 
GameADC::RemoveEndpoint(const EndpointIterator& it)
{
	D_METHOD(("GameADC::RemoveEndpoint()\n"));
	ASSERT_LOCKED;
	if (!it.IsValidEndpoint())
	{
		PRINT((
			"GameADC::RemoveEndpoint():\n\t"
			"invalid endpoint\n"));
		return B_BAD_VALUE;
	}
	// find link to stream node
	Stream** s = &_streams;
	for (; *s != it.stream; s = &(*s)->next) {}
	if (!*s)
	{
		PRINT((
			"GameADC::RemoveEndpoint():\n\t"
			"stream %d not in set!\n",
			it.stream->ID()));
		return B_BAD_VALUE;
	}
	// find link to endpoint node
	
	for (Endpoint** e = &(*s)->endpoints; (*e);)
	{
		if (*e == it.endpoint)
		{
			// remove stream if removing the last endpoint
			bool remove_stream = (*e == (*s)->endpoints && !(*e)->next);

			// whether or not the endpoint is to be deleted now, the
			// buffer group needs to be cleaned up.
			(*e)->ClearBufferGroup();

			if (_iteratorCount)
			{
				// defer deletion
				(*e)->removed = true;
				if (remove_stream)
				{
					(*s)->MarkForRemoval();;
					// +++++ any other removal tasks to be done immediately?
					if ((*s)->IsRunning())
					{
						(*s)->Stop();
					}
				}
			}
			else
			{
				// delete now
				if (remove_stream)
				{
					Stream* ds = *s;
					*s = ds->next;
					delete ds;
				}
				else
				{
					Endpoint* de = *e;
					*e = de->next;
					delete de;
				}
			}
			
			// stay up to date
			if (remove_stream)
			{
				RefreshCodecInfo();
			}
			return B_OK;
		}
		else
		{
			e = &(*e)->next;
		}
	}
	PRINT((
		"GameADC::RemoveEndpoint():\n\t"
			"endpoint %d not in set!\n",
			it.endpoint->source.id));
	return B_BAD_VALUE;
}

void 
GameADC::Reap()
{
	D_METHOD(("GameADC::Reap()\n"));
	ASSERT_LOCKED;
	ASSERT(!_iteratorCount);
	Stream** s = &_streams;
	bool streamsRemoved = false;
	while (*s)
	{
		Endpoint** e = &(*s)->endpoints;
		while (*e)
		{
			if ((*e)->removed)
			{
				Endpoint* de = *e;
				*e = de->next;
				delete de;
			}
			else
			{
				e = &(*e)->next;
			}
		}
		if ((*s)->IsRemovalPending())
		{
			streamsRemoved = true;
			Stream* ds = *s;
			*s = ds->next;
			delete ds;
		}
		else
			s = &(*s)->next;
		
	}
	if (streamsRemoved)
	{
		RefreshCodecInfo();
	}
}

int32 
GameADC::MakeEndpointID()
{
	uint32 sanity = 65536;
	while (sanity--)
	{
		int32 id = _nextEndpointOrdinal++;
		if (FindEndpoint(id) < B_OK) return id;
	}
	D_TROUBLE((
		"GameADC::MakeEndpointID():\n\t"
		"endpoint ID space full.\n"));
	return -1;
}

