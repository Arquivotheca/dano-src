#include "GameConsumer.h"
#include "GameAddOn.h"
#include "GameParameterMap.h"
#include "game_tools.h"
#if __RTLOG
	#include "rtlog.h"
#endif

#include <Autolock.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <Debug.h>
#include <MediaRoster.h>
#include <RealtimeAlloc.h>

#include <ParameterWeb.h>

#include <scheduler.h>
#include <errno.h>
#include <cstring>
#include <cmath>

#define D_TROUBLE(x)	PRINT(x)
#define D_EVENT(x)		PRINT(x)
#define D_MESSAGE(x)	//PRINT(x)
#define D_METHOD(x)		//PRINT(x)
#define D_WARNING(x)	PRINT(x)
#define D_FORMAT(x)		PRINT(x)
#define D_TIMING(x)		//PRINT(x)
#define D_SETTINGS(x)	PRINT(x)

using namespace BPrivate;

// #pragma mark --- constants  ---
// ------------------------------------------------------------------------ //
	
const uint32 STREAM_PAGE_COUNT					= 2;
const uint32 MAX_ZEROBUFFER_FRAME_SIZE			= 32;

const uint32 STREAM_QUEUE_ENTRY_ALLOC_CHUNK		= 16;

const bigtime_t TIME_SOURCE_FREEWHEEL_PERIOD	= 50000LL;

enum _event_t
{
	E_TIME_SOURCE_FREEWHEEL		= BTimedEventQueue::B_USER_EVENT,
	E_START_STREAM
};

enum _message_t
{
	M_SERVICE_TIME_SOURCE		= M_GAMENODE_FIRST_PRIVATE_MESSAGE
};

// #pragma mark --- GameConsumer::DacInfo ---
// ------------------------------------------------------------------------ //

class GameConsumer::DacInfo
{
public:
	DacInfo();
	void* operator new(size_t s);
	void operator delete(void* p, size_t s);

	status_t Refresh(int fd, int32 id =GAME_NO_ID);
	
public:
	const game_codec_info			info;
	const media_multi_audio_format	currentFormat;
	const media_multi_audio_format	requiredFormat;
	const media_multi_audio_format	preferredFormat;
	bigtime_t						hardwareLatency;

private:
	void GetRequiredFormat(
		media_multi_audio_format* oRequired);
	void GetPreferredFormat(
		const media_multi_audio_format& required,
		media_multi_audio_format* oPreferred);
};

// ------------------------------------------------------------------------ //

GameConsumer::DacInfo::DacInfo() :
	currentFormat(media_multi_audio_format::wildcard),
	requiredFormat(media_multi_audio_format::wildcard),
	preferredFormat(media_multi_audio_format::wildcard),
	hardwareLatency(0LL)
{
	game_codec_info* i = const_cast<game_codec_info*>(&info);
	memset(i, 0, sizeof(info));
	i->codec_id = -1;
}

void *
GameConsumer::DacInfo::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameConsumer::DacInfo::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

status_t 
GameConsumer::DacInfo::Refresh(int fd, int32 id)
{
	D_METHOD(("GameConsumer::DacInfo::Refresh()\n"));
	status_t err;
	
	if (info.codec_id == GAME_NO_ID)
	{
		if (id == GAME_NO_ID)
		{
			D_TROUBLE((
				"GameConsumer::DacInfo::Refresh():\n\t"
				"no ID!\n"));
			return B_BAD_VALUE;
		}
		const_cast<game_codec_info*>(&info)->codec_id = id;
	}	
	
	// get DAC status
	G<game_get_codec_infos> ggdi;
	ggdi.info = const_cast<game_codec_info*>(&info);
	ggdi.in_request_count = 1;
	if (ioctl(fd, GAME_GET_CODEC_INFOS, &ggdi) < 0)
	{
		D_TROUBLE((
			"GameConsumer::DacInfo::Refresh():\n\t"
			"GAME_GET_CODEC_INFOS(%d): %s\n",
			info.codec_id, strerror(errno)));
		return errno;
	}

	D_FORMAT((
		"*** DAC info for 0x%x '%s':\n\t"
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
		info.codec_id, info.name,
		info.max_stream_count,
		info.cur_stream_count,
		info.min_chunk_frame_count,
		info.chunk_frame_count_increment,
		info.max_chunk_frame_count,
		info.cur_chunk_frame_count,
		info.frame_rates,
		info.cvsr_min, info.cvsr_max,
		info.formats,
		info.designations,
		info.channel_counts,
		info.cur_frame_rate,
		info.cur_cvsr,
		info.cur_format,
		info.cur_channel_count));

	// build format description
	media_multi_audio_format& f = const_cast<media_multi_audio_format&>(currentFormat);
	f = media_multi_audio_format::wildcard; // references are weird.
	f.byte_order = B_MEDIA_HOST_ENDIAN;
	f.channel_count = info.cur_channel_count;
	f.frame_rate = (info.cur_frame_rate & B_SR_CVSR) ?
		f.frame_rate = info.cur_cvsr :
		frame_rate_for(info.cur_frame_rate);
	for (int32 n = 0; n < sizeof(format_map)/sizeof(*format_map); n++)
	{
		if (info.cur_format == format_map[n].driver)
		{
			f.format = format_map[n].media;
			break;
		}
	}
	ASSERT(f.format != media_multi_audio_format::wildcard.format);
	if (f.format == media_multi_audio_format::B_AUDIO_INT)
	{
		f.valid_bits = int_format_valid_bits(info.cur_format);
	}
	f.buffer_size = (f.format & 0x0f) * f.channel_count * info.cur_chunk_frame_count;

	// extrapolate required & preferred formats
	GetRequiredFormat(const_cast<media_multi_audio_format*>(&requiredFormat));
	GetPreferredFormat(requiredFormat, const_cast<media_multi_audio_format*>(&preferredFormat));

	// calculate hardware latency
	hardwareLatency = bigtime_t(double(info.cur_latency) * 1e6 / f.frame_rate);

	return B_OK;
}

void 
GameConsumer::DacInfo::GetRequiredFormat(
	media_multi_audio_format *oRequired)
{
	D_METHOD(("GameConsumer::DacInfo::GetRequiredFormat()\n"));
	media_multi_audio_format& w = media_multi_audio_format::wildcard;
	
	// base on current configuration
	*oRequired = currentFormat;
	
	if (!info.cur_stream_count)
	{
		// DAC is configurable
		if ((info.frame_rates & B_SR_CVSR) ||
			count_1_bits(info.frame_rates) > 1)
		{
			oRequired->frame_rate = w.frame_rate;
		}
		if (count_1_bits(info.channel_counts) > 1)
		{
			oRequired->channel_count = w.channel_count;
		}
		if (count_1_bits(info.formats) > 1)
		{
			oRequired->format = w.format;
		}
	}
	
	if (info.stream_capabilities.capabilities & GAME_STREAMS_HAVE_FRAME_RATE)
		oRequired->frame_rate = w.frame_rate;
	if (info.stream_capabilities.capabilities & GAME_STREAMS_HAVE_CHANNEL_COUNT)
		oRequired->channel_count = w.channel_count;
	if (info.stream_capabilities.capabilities & GAME_STREAMS_HAVE_FORMAT)
		oRequired->format = w.format;
}

void 
GameConsumer::DacInfo::GetPreferredFormat(
	const media_multi_audio_format& required,
	media_multi_audio_format *oPreferred)
{
	D_METHOD(("GameConsumer::DacInfo::GetPreferredFormat()\n"));
	status_t err;
	media_multi_audio_format& w = media_multi_audio_format::wildcard;
	
	*oPreferred = required;
	if (oPreferred->frame_rate == w.frame_rate)
	{
		oPreferred->frame_rate = PREFERRED_FRAME_RATE;
		err = constrain_frame_rate(info, &oPreferred->frame_rate);
	}
	if (oPreferred->format == w.format)
	{
		// current ADC format
		oPreferred->format = currentFormat.format;
	}
	if (oPreferred->channel_count == w.channel_count)
	{
		// current ADC channel_count
		oPreferred->channel_count = currentFormat.channel_count;
	}
}

// #pragma mark --- GameConsumer::Stream ---
// ------------------------------------------------------------------------ //

class GameConsumer::Stream
{
public:
	enum queue_entry_state
	{
		// entry available
		NOT_QUEUED,
		// user buffer received but no device buffer available
		PENDING,
		// queued on device
		PLAYING
	};

	Stream(GameConsumer* node);
	~Stream();
	
	void* operator new(size_t s);
	void operator delete(void* p, size_t s);

	int32 ID() const;
	const media_multi_audio_format& Format() const;
	const DacInfo* DAC() const;
	BBufferGroup* BufferGroup();

	uint32 BaseFrameOffset() const { BAutolock _l(_node->QueueLock()); return _baseFrameOffset; }
	bigtime_t BasePerfTime() const { BAutolock _l(_node->QueueLock()); return _basePerfTime; }
	bigtime_t BaseRealTime() const { BAutolock _l(_node->QueueLock()); return _baseRealTime; }
	uint64 FramesPlayed() const { BAutolock _l(_node->QueueLock()); return _framesPlayed; }
	uint64 FramesAcquired() const { BAutolock _l(_node->QueueLock()); return _framesAcquired; }
	game_stream_timing LastTiming() const { BAutolock _l(_node->QueueLock()); return _lastTiming; }
	
	status_t Open(
		const DacInfo* dac,
		const media_multi_audio_format& format,
		const game_format_spec& spec,
		int32 bufferCount);
	status_t Close();
	
	status_t Start();
	bool IsStarted() const;
	status_t Stop();
	
	void SetEnabled(
		bool enabled);
	bool IsEnabled() const;

	void SetTimeSource(
		bool isTimeSource);
	bool IsTimeSource() const;

	// the node's _queueLock must be held before calling any of the following:

	bool IsQueueChunkAvailable() const;
	// if buffer is 0, fill the next chunk with zero-amplitude samples
	status_t QueueBuffer(
		BBuffer* buffer);

	status_t AddQueueEntry(
		queue_entry_state state,
		BBuffer* userBuffer);
	status_t PeekQueueEntry(
		queue_entry_state state,
		BBuffer** outUserBuffer =0,
		bigtime_t* outQueuedRealTime =0);
	status_t PopQueueEntry(
		queue_entry_state state,
		BBuffer** outUserBuffer =0,
		bigtime_t* outQueuedRealTime =0);
	bool IsQueueEmpty(queue_entry_state state) const;
	void ClearQueue(queue_entry_state state);

public:
	Stream*							next;

private:
	struct queue_entry
	{
		int32						state;
		BBuffer*					user; // 0 if no user buffer needs recycling
		bigtime_t					queuedRealTime;
		struct queue_entry*			next;
	};

	GameConsumer* const				_node;
	int32							_id;
	sem_id							_sem;
	const DacInfo*					_dac;
	media_multi_audio_format		_format;
	uint32							_bufferFrames;

	// pingpong buffer allocated for this stream
	int32							_bufferID;
	area_id							_area;
	area_id							_localArea;
	size_t							_offset;
	void*							_data;
	size_t							_pageCount;
	// the Media Kit aliases for each buffer page, in order
	BBufferGroup*					_bufferGroup;
	// ordered set of pointers to the above BBuffer aliases
	BBuffer**						_internalBuffers;
	// queued buffers
	queue_entry*					_pendingEntries;
	queue_entry*					_queuedEntries;
	queue_entry*					_entryHeap;
	uint32							_entryHeapSize;
	uint32							_entryHeapNext;

	// * playback state
	bool							_enabled;			// expect buffers on this stream?
	bool							_run;				// has the stream been started?
	mutable bool					_isTimeSource;

	uint32							_nextPageIndex;		// next page to queue in pingpong buffer
	uint32							_queuedPageCount;	// outstanding queued pages

	uint32							_baseFrameOffset;	// frame count corresponding to stream 'start'
	bigtime_t						_basePerfTime;
	bigtime_t						_baseRealTime;
	game_stream_timing				_lastTiming;
	uint64							_framesPlayed;		// frames played according to driver
	uint64							_framesAcquired;	// known number of frames played

	// +++++ late-buffer state
		
	mutable thread_id				_recycleThread;
	
	static status_t RecycleThreadEntry(void* user)
	{
		((GameConsumer::Stream*)user)->RecycleThread();
		return B_OK;
	}
	void RecycleThread();
	void StartRecycleThread();
	void StopRecycleThread();

	status_t AllocateBuffers(int32 count);
	status_t FreeBuffers();
};

// ------------------------------------------------------------------------ //

GameConsumer::Stream::Stream(GameConsumer* node) :
	next(0),
	_node(node),
	_id(GAME_NO_ID),
	_dac(0),
	_bufferID(GAME_NO_ID),
	_area(-1),
	_localArea(-1),
	_offset(0),
	_data(0),
	_pageCount(0),
	_bufferGroup(0),
	_internalBuffers(0),
	_pendingEntries(0),
	_queuedEntries(0),
	_entryHeapNext(0),
	_enabled(true),
	_run(false),
	_isTimeSource(false),
	_nextPageIndex(0),
	_queuedPageCount(0),
	_baseFrameOffset(0),
	_basePerfTime(0LL),
	_baseRealTime(0LL),
	_framesPlayed(0LL),
	_framesAcquired(0LL),
	_recycleThread(-1)
{
	D_METHOD(("GameConsumer::Stream::Stream()\n"));
	ASSERT(_node);
	memset(&_lastTiming, 0, sizeof(_lastTiming));
	
	_entryHeapSize = STREAM_QUEUE_ENTRY_ALLOC_CHUNK;
	_entryHeap = (queue_entry*)rtm_alloc(0, _entryHeapSize * sizeof(queue_entry));
}

GameConsumer::Stream::~Stream()
{
	D_METHOD(("GameConsumer::Stream::~Stream()\n"));
	if (_id != GAME_NO_ID)
	{
		D_TROUBLE((
			"!!! WARNING: GameConsumer::Stream being deleted but stream is still open.\n"));
		FreeBuffers();
	}
	if (_bufferGroup) delete _bufferGroup;
	if (_internalBuffers) rtm_free(_internalBuffers);
	ClearQueue(PENDING);
	ClearQueue(PLAYING);
	if (_entryHeap) rtm_free(_entryHeap);
}

void *
GameConsumer::Stream::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameConsumer::Stream::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

int32 
GameConsumer::Stream::ID() const
{
	return _id;
}

const media_multi_audio_format &
GameConsumer::Stream::Format() const
{
	return _format;
}

const GameConsumer::DacInfo*
GameConsumer::Stream::DAC() const
{
	return _dac;
}

BBufferGroup *
GameConsumer::Stream::BufferGroup()
{
	return _bufferGroup;
}

status_t 
GameConsumer::Stream::Open(
	const DacInfo* in_dac,
	const media_multi_audio_format &in_format,
	const game_format_spec &spec,
	int32 bufferCount)
{
	D_METHOD(("GameConsumer::Stream::Open()\n"));
#if DEBUG
	char fmtbuf[256];
	media_format f;
	f.type = B_MEDIA_RAW_AUDIO;
	f.u.raw_audio = in_format;
	string_for_format(f, fmtbuf, 255);
#endif
	D_FORMAT(("  target format: %s\n", fmtbuf));
	D_FORMAT(("  spec: (%x / %.2f), %x, %x\n",
		spec.frame_rate,
		spec.cvsr,
		spec.channel_count,
		spec.format));
	status_t err;
	
	// create sem
	char semName[64];
	sprintf(semName, "%s.pingpong", in_dac->info.name);
	_sem = create_sem(0, semName);
	if (_sem < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::Stream::Open():\n\t"
			"create_sem(): %s\n",
			strerror(_sem)));
		return _sem;
	}
	
	// open stream
	_dac = in_dac;
	G<game_open_stream> gos;
	gos.codec_id = _dac->info.codec_id;
	gos.request = 0;
	gos.stream_sem = _sem;
	if (spec.frame_rate != _dac->info.cur_frame_rate)
	{
		gos.request |= GAME_STREAM_FR;
		gos.frame_rate = spec.frame_rate;
		D_FORMAT(("> set frame_rate: %x\n", gos.frame_rate));
	}
	if ((spec.frame_rate & B_SR_CVSR) &&
		spec.cvsr != _dac->info.cur_cvsr)
	{
		gos.request |= GAME_STREAM_FR;
		gos.cvsr_rate = spec.cvsr;
		D_FORMAT(("> set CVSR: %.2f\n", gos.cvsr_rate));
	}
	else
	if (spec.channel_count != _dac->info.cur_channel_count)
	{
		gos.request |= GAME_STREAM_CHANNEL_COUNT;
		gos.channel_count = spec.channel_count;
		D_FORMAT(("> set channel_count: %x\n", gos.channel_count));
	}
	if (spec.format != _dac->info.cur_format)
	{
		gos.request |= GAME_STREAM_FORMAT;
		gos.format = spec.format;
		D_FORMAT(("> set format: %x\n", gos.format));
	}
	if (ioctl(_node->DeviceFD(), GAME_OPEN_STREAM, &gos) < 0)
	{
		D_TROUBLE((
			"GameConsumer::Stream::Open():\n\t"
			"GAME_OPEN_STREAM: %s\n",
			strerror(errno)));
		return errno;
	}

	// fill in stream description
	_id = gos.out_stream_id;
	_format = in_format;
	_bufferFrames = _format.buffer_size / ((_format.format & 0x0f) * _format.channel_count);

	// allocate buffer set
	err = AllocateBuffers(bufferCount);
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::Stream::Open():\n\t"
			"AllocateBuffers(%ld): %s\n",
			bufferCount, strerror(err)));

		// clean up & bail out
		Close();
		return err;
	}
#if 0
	// BBufferGroup currently doesn't play well with the pingpong scheme, in which the
	// pages must be queued in order, always starting with the 0'th.  if we can finagle
	// it into handling this kind of streaming, we can avoid an extra copy at queue time.
		
	// create buffer group aliases
	_bufferGroup = new BBufferGroup();
	_internalBuffers = (BBuffer**)rtm_alloc(0, sizeof(BBuffer*) * bufferCount);
	for (uint32 n = 0; n < bufferCount; n++)
	{
		buffer_clone_info clone;
		const game_buffer* b = _buffers->BufferAt(n);
		ASSERT(b);
		clone.area = b->area;
		clone.offset = b->offset;
		clone.size = b->size;
		clone.flags = 0;	
		err = _bufferGroup->AddBuffer(clone, _internalBuffers + n);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::Stream::Open():\n\t"
				"BBufferGroup::AddBuffer(): %s\n",
				strerror(err)));
			return err;
		}
	}
#endif

	// init acquire/recycle loop
	StartRecycleThread();

	return B_OK;
}

status_t 
GameConsumer::Stream::Close()
{
	D_METHOD(("GameConsumer::Stream::Close()\n"));
	status_t err;

	if (_id != GAME_NO_ID)
	{
		FreeBuffers();
		if (_bufferGroup)
		{
			delete _bufferGroup;
			_bufferGroup = 0;
		}
		if (_internalBuffers)
		{
			rtm_free(_internalBuffers);
			_internalBuffers = 0;
		}

		G<game_close_stream> gcs;
		gcs.stream = _id;
		gcs.flags = GAME_CLOSE_DELETE_SEM_WHEN_DONE;
		if (ioctl(_node->DeviceFD(), GAME_CLOSE_STREAM, &gcs) < 0)
		{
			D_TROUBLE((
				"GameConsumer::Stream::Close():\n\t"
				"GAME_CLOSE_STREAM: %s\n",
				strerror(errno)));
		}
		
		_id = GAME_NO_ID;
	}

	StopRecycleThread();

	BAutolock _l(_node->QueueLock());
	ClearQueue(PENDING);
	ClearQueue(PLAYING);

	return B_OK;
}

status_t 
GameConsumer::Stream::Start()
{
	D_METHOD(("GameConsumer::Stream::Start(): %x\n", _id));
	status_t err;

	// queue looping/pingpong buffer
	G<game_set_stream_buffer> gssb;
	gssb.stream = _id;
	gssb.flags = GAME_BUFFER_PING_PONG;
	gssb.buffer = _bufferID;
	gssb.frame_count = _bufferFrames * _pageCount;
	gssb.page_frame_count = _bufferFrames;
	if (ioctl(_node->DeviceFD(), GAME_SET_STREAM_BUFFER, &gssb) < 0)
	{
		D_TROUBLE((
			"!!! GameConsumer::Stream::Start():\n\t"
			"GAME_SET_STREAM_BUFFER(0x%x): %s\n",
			_id, strerror(errno)));
		return errno;
	}
	// start the stream
	H<game_run_stream> grs;
	grs.stream = _id;
	grs.state = GAME_STREAM_RUNNING;
	G<game_run_streams> grss;
	grss.in_stream_count = 1;
	grss.streams = &grs;
	if (ioctl(_node->DeviceFD(), GAME_RUN_STREAMS, &grss) < 0)
	{
		D_TROUBLE((
			"!!! GameConsumer::Stream::Start():\n\t"
			"GAME_RUN_STREAM(0x%x): %s\n",
			_id, strerror(errno)));
		return errno;
	}
	
	_run = true;

	// init timesource state
	_framesAcquired = 0LL;
	_framesPlayed = 0LL;
	_lastTiming = grs.out_timing;
	_baseFrameOffset = 0;
	_baseRealTime = grs.out_timing.at_time;
	_basePerfTime = _node->TimeSource()->PerformanceTimeFor(_baseRealTime);
	D_TIMING((
		">>> start %x at real(%Ld), perf(%Ld), now(%Ld)\n",
		_id, _baseRealTime, _basePerfTime, system_time()));
#if __RTLOG
	_node->log->Log("stream start", 0, 0, _baseRealTime, _basePerfTime);
#endif
	return B_OK;
}

bool 
GameConsumer::Stream::IsStarted() const
{
	return _run;
}

status_t 
GameConsumer::Stream::Stop()
{
	D_METHOD(("GameConsumer::Stream::Stop(): %x\n", _id));
	status_t err;

	H<game_run_stream> grs;
	grs.stream = _id;
	grs.state = GAME_STREAM_STOPPED;
	G<game_run_streams> grss;
	grss.in_stream_count = 1;
	grss.streams = &grs;
	if (ioctl(_node->DeviceFD(), GAME_RUN_STREAMS, &grss) < 0)
	{
		D_TROUBLE((
			"!!! GameConsumer::Stream::Stop():\n\t"
			"GAME_RUN_STREAMS(stop %d): %s\n",
			_id, strerror(errno)));
	}

	// reset stream state even if the stop request failed for some weird-ass reason

	BAutolock _l(_node->QueueLock());
	_run = false;
	_nextPageIndex = 0;
	_queuedPageCount = 0;

	ClearQueue(PENDING);
	ClearQueue(PLAYING);

	if (_isTimeSource)
	{
		// asynchronously notify control thread that it needs a new time source
		_isTimeSource = false;
		err = write_port(_node->ControlPort(), M_SERVICE_TIME_SOURCE, 0, 0);
		if (err < B_OK)
		{
			D_TROUBLE((
				"!!! GameConsumer::Stream::Stop():\n\t"
				"write_port(node): %s\n", strerror(err)));
		}
	}	

#if __RTLOG
	_node->log->Flush();
#endif
	return err;
}

void 
GameConsumer::Stream::SetEnabled(bool enabled)
{
	if (!enabled && _enabled)
	{
		BAutolock _l(_node->QueueLock());
		// shutting down? queue one zero-filled page
		if (IsQueueChunkAvailable())
		{
			PRINT(("queue zero\n"));
			QueueBuffer(0);
		}
		else
		{
			AddQueueEntry(PENDING, 0);
		}
	}
	_enabled = enabled;
}

bool 
GameConsumer::Stream::IsEnabled() const
{
	return _enabled;
}

void 
GameConsumer::Stream::SetTimeSource(bool isTimeSource)
{
	_isTimeSource = isTimeSource;
}

bool 
GameConsumer::Stream::IsTimeSource() const
{
	return _isTimeSource;
}


bool 
GameConsumer::Stream::IsQueueChunkAvailable() const
{
	ASSERT(_node->QueueLock().IsLocked());
	return _queuedPageCount < _pageCount;
}

status_t 
GameConsumer::Stream::QueueBuffer(BBuffer *buffer)
{
	D_METHOD(("GameConsumer::Stream::QueueBuffer(0x%x)\n", buffer ? buffer->ID() : 0));
	ASSERT(_node->QueueLock().IsLocked());
	ASSERT(!buffer || _queuedPageCount < _pageCount);
	ASSERT(!buffer || buffer->SizeUsed() <= _format.buffer_size);
	ASSERT(_bufferID != GAME_NO_ID);
	ASSERT(_data);
	
	// fill
	void* page = (int8*)_data + (_nextPageIndex * _format.buffer_size);
	if (buffer)
	{
		memcpy(page, buffer->Data(), buffer->SizeUsed());
		if (buffer->SizeUsed() < _format.buffer_size)
		{
			// 0-pad incomplete buffers
			memset((int8*)page + buffer->SizeUsed(), 0, _format.buffer_size - buffer->SizeUsed());
		}
		// mark queued
		AddQueueEntry(PLAYING, buffer);
//		AddQueueEntry(PLAYING, 0);

#if __RTLOG
		_node->log->Log("queued: now/start", buffer->ID(), _nextPageIndex, _node->TimeSource()->Now(), buffer->Header()->start_time);
#endif

		// advance page index
		_queuedPageCount++;
		_nextPageIndex = ++_nextPageIndex % _pageCount;

//		buffer->Recycle();
	}
	else
	{
		// fill the buffer page with zero-amplitude samples but don't mark it
		// as queued.  this allows us to ensure that the producer's buffers are
		// completely played before we stop the stream.
		const uint8 zero = (_format.format == media_raw_audio_format::B_AUDIO_UCHAR) ? 128 : 0;
		memset(page, zero, _format.buffer_size);

#if __RTLOG
		_node->log->Log("0 queued");
#endif
	}

	return B_OK;
}

status_t 
GameConsumer::Stream::AddQueueEntry(queue_entry_state state, BBuffer *userBuffer)
{
	ASSERT(_node->QueueLock().IsLocked());
	ASSERT(state != NOT_QUEUED);
	
	// linear search for unused entry, starting with slot following last alloc'd entry
	status_t err;
	queue_entry* nq = 0;
	for (uint32 n = _entryHeapNext; !nq && n < _entryHeapSize; n++)
	{
		if (_entryHeap[n].state == NOT_QUEUED)
		{
			nq = &_entryHeap[n];
			_entryHeapNext = n+1 % _entryHeapSize;
			break;
		}
	}
	for (uint32 n = 0; !nq && n < _entryHeapNext; n++)
	{
		if (_entryHeap[n].state == NOT_QUEUED)
		{
			nq = &_entryHeap[n];
			_entryHeapNext = n+1 % _entryHeapSize;
			break;
		}
	}
	if (!nq)
	{
		// none found; grow the heap
		uint32 newHeapSize = _entryHeapSize + STREAM_QUEUE_ENTRY_ALLOC_CHUNK;
		err = rtm_realloc((void**)&_entryHeap, newHeapSize * sizeof(queue_entry));
		if (err < B_OK) return err;
		memset(
			_entryHeap + _entryHeapSize,
			0,
			STREAM_QUEUE_ENTRY_ALLOC_CHUNK * sizeof(queue_entry));
		nq = _entryHeap + _entryHeapSize;
		_entryHeapNext = _entryHeapSize+1;
		_entryHeapSize = newHeapSize;
	}
	ASSERT(nq);

	nq->state = state;
	nq->user = userBuffer;
	nq->queuedRealTime = system_time();
	nq->next = 0;
	queue_entry** top = (state == PENDING) ? &_pendingEntries : &_queuedEntries;
	for (queue_entry* q = *top; q; q = q->next)
	{
		if (!q->next)
		{
			q->next = nq;
			return B_OK;
		}
	}
	*top = nq;
	return B_OK;
}

status_t 
GameConsumer::Stream::PeekQueueEntry(queue_entry_state state, BBuffer **outUserBuffer, bigtime_t *outQueuedRealTime)
{
	ASSERT(_node->QueueLock().IsLocked());

	queue_entry* q = (state == PENDING) ? _pendingEntries : _queuedEntries;
	if (!q) return B_ERROR;

	if (outUserBuffer) *outUserBuffer = q->user;
	if (outQueuedRealTime) *outQueuedRealTime = q->queuedRealTime;

	return B_OK;
}


status_t 
GameConsumer::Stream::PopQueueEntry(queue_entry_state state, BBuffer **outUserBuffer, bigtime_t *outQueuedRealTime)
{
	ASSERT(_node->QueueLock().IsLocked());

	queue_entry** q = (state == PENDING) ? &_pendingEntries : &_queuedEntries;
	if (!*q) return B_ERROR;
	
	if (outUserBuffer) *outUserBuffer = (*q)->user;
	if (outQueuedRealTime) *outQueuedRealTime = (*q)->queuedRealTime;
	(*q)->state = NOT_QUEUED;
	*q = (*q)->next;

	return B_OK;
}

bool 
GameConsumer::Stream::IsQueueEmpty(queue_entry_state state) const
{
	ASSERT(_node->QueueLock().IsLocked());

	return (state == PENDING) ? !_pendingEntries : !_queuedEntries;
}

void 
GameConsumer::Stream::ClearQueue(queue_entry_state state)
{
	ASSERT(_node->QueueLock().IsLocked());

	queue_entry** q = (state == PENDING) ? &_pendingEntries : &_queuedEntries;
	for (; *q;)
	{
		queue_entry* dq = *q;
		*q = (*q)->next;
		if (dq->user) dq->user->Recycle();
		dq->state = NOT_QUEUED;
	}
}

void 
GameConsumer::Stream::RecycleThread()
{
	D_METHOD(("GameConsumer::Stream::RecycleThread(): start\n"));
	status_t err;
	sem_id sem = _sem;
	
	while (true)
	{
		G<game_get_timing> ggt;
		ggt.source = _id;
		ggt.flags = GAME_ACQUIRE_STREAM_SEM;
		ggt.timeout_at = B_INFINITE_TIMEOUT;
//		PRINT(("ack...\n"));
		if (ioctl(_node->DeviceFD(), GAME_GET_TIMING, &ggt) < 0)
		{
			D_TROUBLE((
				"!!! RecycleThread():\n\t"
				"ABORT: GAME_GET_TIMING: %s\n", strerror(errno)));
			break;
		}
//		PRINT(("...ack OK\n"));
		if (_recycleThread < 0) break;
		
		BAutolock _l(_node->QueueLock());
#if __RTLOG
		_node->log->Log("acquired", 0, 0, system_time(), _node->TimeSource()->Now());
#endif

		_queuedPageCount--;
		_framesAcquired += _bufferFrames;
		
		// recycle buffer if necessary
		BBuffer* userBuffer;
		err = PopQueueEntry(PLAYING, &userBuffer);
		if (err < B_OK)
		{
			D_WARNING(("*** RecycleThread(): stream starved; stopping.\n"));
#if __RTLOG
			_node->log->Log("starved; STOP");
#endif
			Stop();
			continue;
		}
		else if (userBuffer) userBuffer->Recycle();

		if (IsQueueEmpty(PLAYING))
		{
			if (IsEnabled())
			{
				// +++++
				// no buffers are queued and the stream is enabled.
				// prepare to send a late notice (which will actually be sent when the
				// producer gets around to queueing another buffer.)
				D_TIMING(("!!! LATE\n"));
			}
			D_TIMING((">>> RecycleThread(): last queued page ack'd; stopping.\n"));
			if (!IsQueueEmpty(PENDING))
			{
				// this really shouldn't happen!
				D_WARNING(("!!! pending buffers dumped!\n"));
			}
#if __RTLOG
			_node->log->Log("done; STOP");
#endif
			Stop();
			continue;
		}
		
		// queue deferred buffers
		while (_queuedPageCount < _pageCount)
		{
			err = PopQueueEntry(PENDING, &userBuffer);
			if (err < B_OK) break;
//			PRINT(("queue deferred: 0x%x\n", userBuffer ? userBuffer->ID() : 0));
			QueueBuffer(userBuffer);
		}
		
		// update timing state
		if (ggt.timing.frames_played < _lastTiming.frames_played)
		{
			// handle frame count overflow case
			_framesPlayed += (ggt.timing.frames_played +
				(ULONG_MAX - _lastTiming.frames_played));
		}
		else
		{
			_framesPlayed +=
				(ggt.timing.frames_played - _lastTiming.frames_played);
		}
		_lastTiming = ggt.timing;

		if (!_baseFrameOffset)
		{
			// starting may have incurred unknown latency; now we are definitely
			// running and can act as a solid timing reference
			_baseFrameOffset = _framesAcquired;
			_baseRealTime = _lastTiming.at_time;
			// +++++ is this threadsafe, or should the node control thread do this bit?
			_basePerfTime = _node->TimeSource()->PerformanceTimeFor(_baseRealTime);
		}

		if (_isTimeSource)
		{
			// signal control thread to publish time
			err = write_port(_node->ControlPort(), M_SERVICE_TIME_SOURCE, 0, 0);
			if (err < B_OK)
			{
				D_TROUBLE((
					"!!! RecycleThread():\n\t"
					"write_port(node): %s\n", strerror(err)));
			}
		}
	}

	D_METHOD(("GameConsumer::Stream::RecycleThread(): finished\n"));
}

void 
GameConsumer::Stream::StartRecycleThread()
{
	D_METHOD(("GameConsumer::Stream::StartRecycleThread()\n"));
	ASSERT(_recycleThread < 0);
	ASSERT(_dac);
	char threadName[64];
	sprintf(threadName, "%s.recycle", _dac->info.name);
	_recycleThread = spawn_thread(&RecycleThreadEntry, threadName, B_REAL_TIME_PRIORITY, this);
	if (_recycleThread < 0)
	{
		D_TROUBLE((
			"GameConsumer::Stream::StartRecycleThread():\n\t"
			"spawn_thread(): %s\n", _recycleThread));
		return;
	}
	resume_thread(_recycleThread);
}

void 
GameConsumer::Stream::StopRecycleThread()
{
	D_METHOD(("GameConsumer::Stream::StopRecycleThread()\n"));
	thread_id tid = _recycleThread;
	if (tid < 0)
	{
		D_WARNING((
			"GameConsumer::Stream::StopRecycleThread():\n\t"
			"not started.\n"));
		return;
	}
	_recycleThread = -1;
	status_t err;
	kill_thread(tid);
	while (wait_for_thread(tid, &err) == B_INTERRUPTED) {}
	D_METHOD(("DONE: GameConsumer::Stream::StopRecycleThread()\n"));
}

status_t 
GameConsumer::Stream::AllocateBuffers(int32 count)
{
	D_METHOD(("GameConsumer::Stream::AllocateBuffers()\n"));
	status_t err;
	// sanity check
	if (IsStarted())
	{
		PRINT((
			"GameConsumer::Stream::AllocateBuffers():\n\t"
			"stream currently running\n"));
		return B_NOT_ALLOWED;
	}
	if (_id == GAME_NO_ID)
	{
		PRINT((
			"GameConsumer::Stream::AllocateBuffers():\n\t"
			"no stream\n"));
		return B_NOT_ALLOWED;
	}
	if (_format.buffer_size <= 0)
	{
		PRINT((
			"GameConsumer::Stream::AllocateBuffers():\n\t"
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
	if (ioctl(_node->DeviceFD(), GAME_OPEN_STREAM_BUFFERS, &gosbs) < 0)
	{
		D_TROUBLE((
			"GameConsumer::Stream::AllocateBuffers():\n\t"
			"GAME_OPEN_STREAM_BUFFERS: %s\n", strerror(errno)));
		return errno;
	}
	
	_bufferID = gosb.buffer;
	_area = gosb.area;
	_offset = gosb.offset;
	_pageCount = count;
	_localArea = clone_area("dac.local", &_data, B_ANY_ADDRESS, B_READ_AREA|B_WRITE_AREA, _area);
	if (_localArea < 0)
	{
		err = _localArea;
		D_TROUBLE((
			"GameConsumer::Stream::AllocateBuffers():\n\t"
			"clone_area(): %s\n", strerror(err)));
		_localArea = 0;
		FreeBuffers();
		return err;
	}
	_data = (int8*)_data + _offset;
	PRINT(("alloc'd buffer 0x%x: offset %ld\n", _bufferID, _offset));
	return B_OK;
}

status_t 
GameConsumer::Stream::FreeBuffers()
{
	D_METHOD(("GameConsumer::Stream::FreeBuffers()\n"));
	if (IsStarted())
	{
		PRINT((
			"GameConsumer::Stream::FreeBuffers():\n\t"
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
		if (ioctl(_node->DeviceFD(), GAME_CLOSE_STREAM_BUFFERS, &gcsbs) < 0)
		{
			D_WARNING((
				"GameConsumer::Stream::FreeBuffers():\n\t"
				"GAME_CLOSE_STREAM_BUFFERS: %s\n", strerror(errno)));
		}
		_bufferID = -1;
		_area = -1;
	}
	_data = 0;
	_pageCount = 0;
	return B_OK;
}


// #pragma mark --- Endpoint ---
// ------------------------------------------------------------------------ //

class GameConsumer::Endpoint
{
public:
	Endpoint(
		Stream* in_stream,
		const media_source& in_source,
		const media_destination& in_destination);
	~Endpoint();

	void* operator new(size_t s);
	void operator delete(void* p, size_t s);

	media_multi_audio_format		format;
	media_source					source;
	media_destination				destination;

	Stream*							stream;	
	bigtime_t						hardwareLatency;
	bool							removed;
	
	Endpoint*						next;
};

// ------------------------------------------------------------------------ //

GameConsumer::Endpoint::Endpoint(
	Stream *in_stream,
	const media_source &in_source,
	const media_destination& in_destination) :
	
	source(in_source),
	destination(in_destination),
	stream(in_stream),
	removed(false),
	next(0)
{
	ASSERT(in_stream);
	format = in_stream->Format();
	ASSERT(in_stream->DAC());
	hardwareLatency = in_stream->DAC()->hardwareLatency;
}

GameConsumer::Endpoint::~Endpoint()
{
}

void *
GameConsumer::Endpoint::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameConsumer::Endpoint::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

// #pragma mark --- EndpointIterator ---
// ------------------------------------------------------------------------ //

class GameConsumer::EndpointIterator
{
public:
	EndpointIterator();
	EndpointIterator(const EndpointIterator& clone);
	EndpointIterator& operator=(const EndpointIterator& clone);

	void* operator new(size_t s);
	void operator delete(void* p, size_t s);
	
	bool IsValid() const;

private:
	friend class GameConsumer;

	EndpointIterator(GameConsumer::Endpoint* e);
	void Advance();
	Endpoint*						endpoint;
};

// ------------------------------------------------------------------------ //

GameConsumer::EndpointIterator::EndpointIterator() :
	endpoint(0) {}

GameConsumer::EndpointIterator::EndpointIterator(const EndpointIterator &clone)
{
	operator=(clone);
}

GameConsumer::EndpointIterator &
GameConsumer::EndpointIterator::operator=(const EndpointIterator &clone)
{
	 endpoint = clone.endpoint;
	 return *this;
}

void *
GameConsumer::EndpointIterator::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameConsumer::EndpointIterator::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

bool 
GameConsumer::EndpointIterator::IsValid() const
{
	 return endpoint != 0;
}

GameConsumer::EndpointIterator::EndpointIterator(GameConsumer::Endpoint *e) :
	endpoint(e) {}

void 
GameConsumer::EndpointIterator::Advance()
{
	 if (endpoint) endpoint = endpoint->next;
}


// #pragma mark --- GameConsumer  ---
// ------------------------------------------------------------------------ //

GameConsumer::~GameConsumer()
{
	D_METHOD(("GameConsumer::~GameConsumer()\n"));

	_addon->StopWatchingSettings(Node(), _path.String());
	
	// halt service thread
	Quit();	

	// clean up
	ClearEndpointsAndStreams();
	
	delete [] _dac;

	if (_fd >= 0) close(_fd);
	delete _parameterMap;

#if __RTLOG
	if (log) delete log;
#endif
}

GameConsumer::GameConsumer(
	const char *name, GameAddOn *addon, const char *devicePath, int32 dacID, int32 addonID) :
	
	// base classes
	BMediaNode(name),
	BBufferConsumer(B_MEDIA_RAW_AUDIO),
	BControllable(),
	BMediaEventLooper(),
	
	// state
	_initStatus(B_ERROR),
	_addon(addon),
	_path(devicePath),
	_fd(-1),
	_dacID(dacID),
	_addonID(addonID),
	_dacCount(0),
	_dac(0),
	_preferredFrameRate(0.0f),
	_preferredSampleFormat(0),
	_preferredSampleBits(0),
	_dacFrameCount(0),
	_endpoints(0),
	_iteratorCount(0),
	_nextEndpointID(1),
	_queueLock("BGameConsumer queueLock"),
	_streams(0),
	_timeSourceStream(0),
	_lastPerfPublished(0LL),
	_lastRealPublished(system_time()),
	_drift(1.0f),
	_perfQueue(EventQueue()),
	_realQueue(RealTimeQueue()),
	_parameterMap(0)
#if __RTLOG
	,
	log(new RTLogContext("GameConsumer", "/tmp/game_consumer.log", 1024))
#endif
{
	D_METHOD(("GameConsumer::GameConsumer()\n"));
	AddNodeKind(B_PHYSICAL_OUTPUT);

	// +++++ limit stack appropriately
	status_t err = media_realtime_init_thread(find_thread(0), 0, B_MEDIA_REALTIME_AUDIO);
	if (err < B_OK)
	{
		D_WARNING((
			"GameConsumer::GameConsumer():\n\t"
			"media_realtime_init_thread(): %s\n", strerror(err)));
	}

	_initStatus = B_OK;
}

void *
GameConsumer::operator new(size_t s)
{
	return rtm_alloc(0, s);
}

void 
GameConsumer::operator delete(void *p, size_t s)
{
	rtm_free(p);
}

status_t 
GameConsumer::InitCheck() const
{
	return _initStatus;
}

int 
GameConsumer::DeviceFD() const
{
	return _fd;
}

BLocker &
GameConsumer::QueueLock() const
{
	return const_cast<BLocker&>(_queueLock);
}

// #pragma mark --- [GameConsumer] BMediaNode  ---
// ------------------------------------------------------------------------ //

status_t 
GameConsumer::HandleMessage(int32 code, const void *data, size_t size)
{
	D_METHOD(("GameConsumer::HandleMessage()\n"));
	
	switch (code)
	{
		case M_GAMENODE_PARAM_CHANGED:
		{
			gamenode_param_changed_msg& m = *(gamenode_param_changed_msg*)data;
			D_SETTINGS(("GameConsumer: M_GAMENODE_PARAM_CHANGED: %x, %x\n",
				m.mixer_id, m.control_id));
			// +++
			// implement to support unbound mixers (for which a single parameter
			// may apply to several codecs.)
			return B_OK;
		}

		case M_SERVICE_TIME_SOURCE:
			D_MESSAGE(("M_SERVICE_TIME_SOURCE\n"));
			ServiceTimeSource();
			return B_OK;
	}
	return B_ERROR;
}

BMediaAddOn *
GameConsumer::AddOn(int32 *outID) const
{
	*outID = _addonID;
	return _addon;
}

void 
GameConsumer::SetRunMode(run_mode mode)
{
	D_METHOD(("GameConsumer::SetRunMode(%ld)\n", mode));
	BMediaEventLooper::SetRunMode(mode);
}

void 
GameConsumer::NodeRegistered()
{
	D_METHOD(("GameConsumer::NodeRegistered()\n"));
	status_t err;
	// open device
	_fd = open(_path.String(), O_RDWR);
	if (_fd < 0)
	{
		D_TROUBLE((
			"GameConsumer::NodeRegistered():\n\t"
			"open('%s'): %s\n",
			_path.String(),
			strerror(_fd)));
		return;
	}

	// build single codec description
	_dacCount = 1;
	_dac = new DacInfo[1];
	_dac[0].Refresh(_fd, _dacID);
	
	// query each DAC for its status and build preferred format
	RefreshDacInfo();
	
	// figure an appropriate buffer size
	err = InitFrameCount();
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::NodeRegistered():\n\t"
			"InitFrameCount(): %s\n",
			strerror(err)));
		return;
	}
	
	// set up parameter web; this refreshes saved hardware-mixer settings.
	err = BuildParameterWeb();
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::NodeRegistered():\n\t"
			"BuildParameterWeb(): %s\n",
			strerror(err)));
		return;
	}

	SetPriority(B_REAL_TIME_PRIORITY);

	// publish initial timestamp
	PublishTime(0LL, 0LL, 0.0);

	Run();
	D_TIMING((">>> internal latency is %Ld\n", EventLatency() + SchedulingLatency()));
}

// #pragma mark --- [GameConsumer] BMediaEventLooper  ---
// ------------------------------------------------------------------------ //

void 
GameConsumer::HandleEvent(const media_timed_event *event, bigtime_t howLate, bool realTimeEvent)
{
	status_t err;
	switch (event->type)
	{
		case E_TIME_SOURCE_FREEWHEEL:
		{
			ServiceTimeSource();
			break;
		}
		case E_START_STREAM:
		{
			Stream* s = (Stream*)event->pointer;
			if (!s)
			{
				D_WARNING(("GameConsumer: E_START_STREAM: no stream specified.\n"));
				break;
			}
			err = s->Start();
			
			if (err < B_OK)
			{
				D_TROUBLE((
					"GameConsumer: E_START_STREAM:\n\t"
					"Stream::Start(): %s\n", strerror(err)));
			}
			BAutolock _l(_queueLock);
			if (!_timeSourceStream)
			{
				s->SetTimeSource(true);
				_timeSourceStream = s;
			}
			break;	
		}
		case BTimedEventQueue::B_START:
		{
			D_EVENT(("GameConsumer: B_START\n"));
			if (RunState() != BMediaEventLooper::B_STOPPED)
			{
				D_WARNING(("GameConsumer: B_START: bad state %ld\n", RunState()));
				break;
			}
			// start publishing time
			ServiceTimeSource();
			// since existing streams are stopped/flushed when the node is stopped,
			// there's no extra work to do when [re]starting (streams are started on
			// demand as buffers are queued.)
			break;
		}
		case BTimedEventQueue::B_STOP:
		{
			D_EVENT(("GameConsumer: B_STOP\n"));
			if (RunState() != BMediaEventLooper::B_STARTED)
			{
				D_WARNING(("GameConsumer: B_STOP: bad state %ld\n", RunState()));
				break;
			}
			// stop publishing time
			FlushTimeSourceEvents();
			// stop streams
			StopAllStreams();
#if __RTLOG
			log->Flush();
#endif
			break;
		}
		case BTimedEventQueue::B_SEEK:
		{
			D_EVENT(("GameConsumer: B_SEEK\n"));
			if (!realTimeEvent) break;
			if (RunState() == BMediaEventLooper::B_STARTED)
			{
				// adjust current performance time
				// +++++
			}
			else
			{
				// set initial performance time
				// +++++
			}
			// +++++ broadcast time warp
			break;
		}
		case BTimedEventQueue::B_WARP:
		{
			D_EVENT(("GameConsumer: B_WARP\n"));
			break;
		}
		case BTimedEventQueue::B_DATA_STATUS:
		{
			D_EVENT(("GameConsumer: B_DATA_STATUS (%s)\n",
				(event->data == B_DATA_AVAILABLE) ? "ON" : "OFF"));
#if __RTLOG
			log->Log("data.status: when/late", event->data, 0,
				event->event_time,
				howLate);
#endif
			Stream* s = (Stream*)event->pointer;
			s->SetEnabled(event->data == B_DATA_AVAILABLE);
			break;
		}
		default:
			D_EVENT(("GameConsumer: event 0x%x\n", event->type));
			break;
	}
}

// #pragma mark --- [GameConsumer] BBufferConsumer  ---
// ------------------------------------------------------------------------ //

status_t 
GameConsumer::AcceptFormat(const media_destination &destination, media_format *ioFormat)
{
	D_METHOD(("GameConsumer::AcceptFormat()\n"));
#if DEBUG
	char fmt_str[256];
	string_for_format(*ioFormat, fmt_str, 255);
	D_FORMAT(("\tin:  %s\n", fmt_str));
#endif
	status_t err = B_OK;

	if (destination.id != 0) return B_MEDIA_BAD_DESTINATION;	
	if (ioFormat->type != B_MEDIA_RAW_AUDIO) return B_MEDIA_BAD_FORMAT;
	
	RefreshDacInfo();
	
	if (ioFormat->u.raw_audio.channel_count == media_multi_audio_format::wildcard.channel_count)
	{
		// +++++ need to centralize preferred channel count
		ioFormat->u.raw_audio.channel_count = 2;
	}

	DacInfo** dacs = new DacInfo*[_dacCount];
	uint32 dacsUsed;

	err = BuildSplitChannelMap(ioFormat->u.raw_audio, dacs, &dacsUsed);
	if (err == B_OK)
	{
		err = ValidateFormat(
			&ioFormat->u.raw_audio,
			dacs,
			dacsUsed);
		if (err < B_OK)
		{
			D_FORMAT((
				"GameConsumer::AcceptFormat():\n\t"
				"ValidateFormat(): %s\n",
				strerror(err)));
		}
	}
	else
	{
		// unhandled channel configuration
		D_FORMAT((
			"GameConsumer::AcceptFormat():\n\t"
			"BuildSplitChannelMap(): %s\n",
			strerror(err)));
	}
	
#if DEBUG
	string_for_format(*ioFormat, fmt_str, 255);
	D_FORMAT(("\tout: %s\n", fmt_str));
#endif
	
	delete [] dacs;
	return err;
}

void 
GameConsumer::BufferReceived(BBuffer *buffer)
{
	D_METHOD(("\nGameConsumer::BufferReceived(0x%x)\n",
		buffer->ID()));
	ASSERT(buffer);
	
	// +em: we don't check to see if the node's running.
	// does it matter?
	
#if __RTLOG
	log->Log("buf.rcvd: now/start", buffer->ID(), 0,
		TimeSource()->PerformanceTimeFor(system_time()),
		buffer->Header()->start_time);
#endif

	status_t err;
	// find endpoint/stream
	Endpoint* e;
	const media_header& header = *buffer->Header();
	err = FindEndpoint(header.destination, &e);
	if (err < B_OK)
	{
		D_WARNING((
			"GameConsumer::BufferReceived():\n\t"
			"no endpoint for destination %ld: %s\n",
			header.destination, strerror(err)));
		buffer->Recycle();
		return;
	}
	// sanity check on buffer size
	if (buffer->SizeUsed() != e->format.buffer_size)
	{
		D_WARNING((
			"GameConsumer::BufferReceived():\n\t"
			"rejecting wrong-size buffer (%ld > %ld).\n",
			buffer->SizeUsed(), e->format.buffer_size));
		buffer->Recycle();
		return;
	}
	
	BAutolock _l(_queueLock);
	
	// fetch stream
	Stream* s = e->stream;
	if (!s)
	{
		D_TROUBLE((
			"!!! GameConsumer::BufferReceived():\n\t"
			"no stream for destination %ld!\n",
			header.destination));
		buffer->Recycle();
		return;
	}

	bool needStart = !s->IsStarted() && s->IsQueueEmpty(Stream::PLAYING);
	
	// +++++ send notice if late
	
	if (s->IsQueueChunkAvailable())
	{
		// queue direct to device	
//		PRINT(("queue direct: 0x%x\n", buffer->ID()));
		err = s->QueueBuffer(buffer);
		if (err < B_OK)
		{
			D_TROUBLE((
				"!!! GameConsumer::BufferReceived():\n\t"
				"Stream::QueueBuffer(): %s\n",
				strerror(err)));
			return;
		}
	}
	else
	{
		// defer until the next page is done playing
#if __RTLOG
		log->Log("deferred", buffer->ID(), 0, 0, buffer->Header()->start_time);
#endif
//		PRINT(("defer: 0x%x\n", buffer->ID()));
		s->AddQueueEntry(Stream::PENDING, buffer);
	}
	
	// if the stream is not yet started, queue a service event at the requested
	// time.  note that this is the only case in which the start_time is honored.
	if (needStart)
	{
		ASSERT(s->IsQueueEmpty(Stream::PENDING)); // weird case
		_perfQueue->AddEvent(media_timed_event(
			header.start_time,
			E_START_STREAM,
			(void*)s,
			0));
#if __RTLOG
		log->Log("q.start stream: real/perf", 0, 0,
			TimeSource()->RealTimeFor(header.start_time, 0),
			header.start_time);
#endif
	}
}

status_t 
GameConsumer::Connected(
	const media_source &source,
	const media_destination &destination,
	const media_format &format,
	media_input *outInput)
{
	D_METHOD(("GameConsumer::Connected()\n"));
	status_t err;
	if (destination.id != 0) return B_MEDIA_BAD_DESTINATION;
	
	// validate the format
	RefreshDacInfo();
	DacInfo** dacs = (DacInfo**)alloca(sizeof(DacInfo*) * _dacCount);
	uint32 dacsUsed;
	err = BuildSplitChannelMap(format.u.raw_audio, dacs, &dacsUsed);
	if (err == B_OK)
	{
		media_multi_audio_format af = format.u.raw_audio;
		err = ValidateFormat(
			&af,
			dacs,
			dacsUsed);
		if (err < B_OK)
		{
			D_FORMAT((
				"GameConsumer::Connected():\n\t"
				"ValidateFormat(): %s\n",
				strerror(err)));
			return err;
		}
	}
	else
	{
		// unhandled channel configuration
		D_FORMAT((
			"GameConsumer::Connected():\n\t"
			"BuildSplitChannelMap(): %s\n",
			strerror(err)));
		return err;
	}
	ASSERT(dacsUsed > 0);

	// create & configure stream
	// +++++ create one stream per DAC once/if multistream support lands

	if (!dacs[0]->info.cur_stream_count)
	{
		// update DAC-level format settings only as necessary
		err = SetDacFormat(dacs[0], format.u.raw_audio);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::Connected():\n\t"
				"SetDacFormat(): %s\n",
				strerror(err)));
			return err;
		}
		
		err = dacs[0]->Refresh(_fd);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::Connected():\n\t"
				"DacInfo::Refresh(): %s\n",
				strerror(err)));
			return err;
		}		
	}
	
	// make/validate stream-level format
	game_format_spec streamSpec;
	streamSpec.frame_rate = dacs[0]->info.cur_frame_rate;
	streamSpec.cvsr = dacs[0]->info.cur_cvsr;
	streamSpec.channel_count = dacs[0]->info.cur_channel_count;
	streamSpec.designations = dacs[0]->info.designations;
	streamSpec.format = dacs[0]->info.cur_format;

	err = make_game_format_spec(
		format.u.raw_audio,
		dacs[0]->info.stream_capabilities.frame_rates,
		dacs[0]->info.stream_capabilities.cvsr_min,
		dacs[0]->info.stream_capabilities.cvsr_max,
		dacs[0]->info.stream_capabilities.channel_counts,
		dacs[0]->info.stream_capabilities.designations,
		dacs[0]->info.stream_capabilities.formats,
		&streamSpec);
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::Connected():\n\t"
			"make_game_format_spec(): %s\n",
			strerror(err)));
		return err;
	}

	Stream* s;
	err = CreateStream(
		dacs[0],
		format.u.raw_audio,
		streamSpec,
		STREAM_PAGE_COUNT,
		&s);
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::Connected():\n\t"
			"CreateStream(): %s\n",
			strerror(err)));
		return err;
	}
	
	// create endpoint
	Endpoint* e;
	CreateEndpoint(s, source, &e);
	ASSERT(e);
	
	// pass back buffer group
	if (s->BufferGroup())
	{
		int32 tag;
		err = SetOutputBuffersFor(source, destination, s->BufferGroup(), 0, &tag, true);
		if (err < B_OK)
		{
			D_WARNING((
				"GameConsumer::Connected():\n\t"
				"SetOutputBuffersFor(): %s\n",
				strerror(err)));
		}
	}

	DescribeInput(e, outInput);
	
	// refresh latency for all endpoints, in case creating the stream had
	// side effects
	RefreshLatency();

	return B_OK;
}

void 
GameConsumer::Disconnected(const media_source &source, const media_destination &destination)
{
	D_METHOD(("GameConsumer::Disconnected(%ld)\n", destination.id));
	status_t err;

	// validate destination
	Endpoint* e;
	err = FindEndpoint(destination.id, &e);
	if (err < B_OK)
	{
		D_WARNING((
			"GameConsumer::Disconnected():\n\t"
			"FindEndpoint(%ld): %s\n",
			destination.id, strerror(err)));
		return;
	}

	// remove endpoint & associated stream(s)
	DeleteEndpoint(e);

	// refresh latency for all endpoints, in case deleting the stream had
	// side effects
	RefreshLatency();	
}

void 
GameConsumer::DisposeInputCookie(int32 cookie)
{
	D_METHOD(("GameConsumer::DisposeInputCookie()\n"));
	EndpointIterator* it = (EndpointIterator*)cookie;
	if (it) delete it;
	if (!--_iteratorCount) ReapEndpoints();
}

status_t 
GameConsumer::FormatChanged(const media_source &source, const media_destination &destination, int32 changeTag, const media_format &newFormat)
{
	D_METHOD(("GameConsumer::FormatChanged()\n"));
	// no.
	return B_NOT_ALLOWED;
}

status_t 
GameConsumer::GetLatencyFor(
	const media_destination &destination,
	bigtime_t *outLatency,
	media_node_id *outTimeSource)
{
	D_METHOD(("GameConsumer::GetLatencyFor()\n"));
	status_t err;

	*outTimeSource = TimeSource()->ID();
	bigtime_t latency = SchedulingLatency() + EventLatency();
	if (!destination.id)
	{
		// estimated (preferred-format) buffer latency for free-connection dest.
		media_input in;
		err = DescribeFreeInput(&in);
		if (err < B_OK) return err;
		latency += buffer_duration(in.format.u.raw_audio);
	}
	else
	{
		// buffer latency for connected destination
		Endpoint* e;
		err = FindEndpoint(destination.id, &e);
		if (err < B_OK) return err;
		latency += buffer_duration(e->format);
		// hardware latency for connected destination
		latency += e->hardwareLatency;
	}
	D_TIMING(("*** latency for %ld: %Ld\n",
		destination.id, latency));
	*outLatency = latency;
	return B_OK;
}

status_t 
GameConsumer::GetNextInput(int32 *ioCookie, media_input *outInput)
{
	D_METHOD(("GameConsumer::GetNextInput()\n"));
	status_t err;
	if (!*ioCookie)
	{
		++_iteratorCount;
		*ioCookie = (int32)new EndpointIterator(_endpoints);
		err = DescribeFreeInput(outInput);
		if (err == B_OK) return err;
	}
	EndpointIterator* it = (EndpointIterator*)*ioCookie;
	if (!it->IsValid()) return B_ERROR;
	DescribeInput(it->endpoint, outInput);
	it->Advance();
	return B_OK;
}

void 
GameConsumer::ProducerDataStatus(const media_destination &destination, int32 status, bigtime_t tpWhen)
{
	D_METHOD(("GameConsumer::ProducerDataStatus(): %ld @ perf(%Ld)\n",
		status, tpWhen));
#if __RTLOG
	log->Log("data status", status, 0, 0, tpWhen);
#endif
	Endpoint* e;
	status_t err = FindEndpoint(destination.id, &e);
	if (err < B_OK)
	{
		D_WARNING((
			"GameConsumer::ProducerDataStatus():\n\t"
			"bad destination %ld: %s\n",
			destination.id, strerror(err)));
		return;
	}
	ASSERT(e->stream);
	// the data status takes effect one buffer-duration early, to give us a chance
	// to queue some silence when the stream stops.
	media_timed_event ev(
		tpWhen - buffer_duration(e->stream->Format()),
		BTimedEventQueue::B_DATA_STATUS);
	ev.pointer = e->stream;
	ev.data = status;
	err = _perfQueue->AddEvent(ev);
	if (err < B_OK)
	{
		D_WARNING((
			"GameConsumer::ProducerDataStatus():\n\t"
			"_perfQueue->AddEvent(): %s\n",
			strerror(err)));
	}
}

// #pragma mark --- [GameConsumer] BTimeSource  ---
// ------------------------------------------------------------------------ //

status_t 
GameConsumer::TimeSourceOp(const time_source_op_info &op, void *_reserved)
{
	D_METHOD(("GameConsumer::TimeSourceOp()\n"));
	media_timed_event ev;
	switch (op.op)
	{
		case B_TIMESOURCE_START:
			D_EVENT(("GameConsumer: B_TIMESOURCE_START\n"));
			ev.event_time = op.real_time;
			ev.type = BTimedEventQueue::B_START;
			break;
		case B_TIMESOURCE_STOP:
			D_EVENT(("GameConsumer: B_TIMESOURCE_STOP\n"));
			ev.event_time = op.real_time;
			ev.type = BTimedEventQueue::B_STOP;
			break;
		case B_TIMESOURCE_STOP_IMMEDIATELY:
			D_EVENT(("GameConsumer: B_TIMESOURCE_STOP_IMMEDIATELY\n"));
			// queue an event to update RunState()
			ev.event_time = system_time();
			ev.type = BTimedEventQueue::B_STOP;
			// do it now
			FlushTimeSourceEvents();
			StopAllStreams();
			break;
		case B_TIMESOURCE_SEEK:
			D_EVENT(("GameConsumer: B_TIMESOURCE_SEEK\n"));
			ev.event_time = op.real_time;
			ev.bigdata = op.performance_time;
			ev.type = BTimedEventQueue::B_SEEK;
			break;
		default:
			D_TROUBLE((
				"GameConsumer::TimeSourceOp():\n\t"
				"unknown op %ld\n", op.op));
			return B_ERROR;
	}
	_realQueue->AddEvent(ev);
	return B_OK;
}


// #pragma mark --- [GameConsumer] BControllable  ---
// ------------------------------------------------------------------------ //

status_t 
GameConsumer::GetParameterValue(int32 id, bigtime_t *outLastChangeTime, void *outValue, size_t *ioSize)
{
	status_t err = _parameterMap ?
		_parameterMap->GetParameterValue(_fd, id, outValue, ioSize) :
		EPERM;
	if (err == B_OK) *outLastChangeTime = 0LL;
	else
	{
		D_TROUBLE((
			"GameConsumer::GetParameterValue():\n\t"
			"GetParameterValue(%x): %s\n",
			id, strerror(err)));
	}
	return err;
}

void 
GameConsumer::SetParameterValue(int32 id, bigtime_t changeTime, const void *value, size_t size)
{
	D_METHOD(("GameConsumer::SetParameterValue() %ld\n", id));
	status_t err = _parameterMap ?
		_parameterMap->SetParameterValue(_fd, id, value, size) :
		EPERM;
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::SetParameterValue():\n\t"
			"SetParameterValue(%x): %s\n",
			id, strerror(err)));
	}
	else
	{
		// notify the addon of the change
		const game_mixer_control * ci = _parameterMap->ControlAt(id & ~GameParameterMap::PARAM_ID_MASK);
		if (!ci)
		{
			D_TROUBLE((
				"GameConsumer::SetParameterValue():\n\t"
				"_parameterMap->ControlAt() couldn't find %ld\n", id));
			return;
		}
		err = _addon->CodecParameterChanged(Node(), _path.String(), ci->mixer_id, ci->control_id);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::SetParameterValue():\n\t"
				"_addon->CodecParameterChanged(): %s\n", strerror(err)));
		}
	}
}

// #pragma mark --- [GameConsumer] event handlers  ---
// ------------------------------------------------------------------------ //

void 
GameConsumer::ServiceTimeSource()
{
	bigtime_t now = system_time();
	if (!_timeSourceStream ||
		!_timeSourceStream->IsTimeSource())
	{
		// need a new clock: search for a currently running stream
		_timeSourceStream = 0;
		for (Stream* s = _streams; s && !_timeSourceStream; s = s->next)
		{
			if (s->IsStarted()) _timeSourceStream = s;
		}
	}
	if (_timeSourceStream &&
		_timeSourceStream->BaseFrameOffset())
	{
		// +++++
		// currently, we only use a stream to act as a time source once frames have
		// actually been acknowledged as played, since starting a stream takes an unknow[able,n]
		// amount of time.
		game_stream_timing t = _timeSourceStream->LastTiming();
		if (t.at_time == _lastRealPublished)
		{
			// timing info not updated since last service
			return;
		}
		bigtime_t perfElapsed = bigtime_t(
			(double(_timeSourceStream->FramesPlayed() - _timeSourceStream->BaseFrameOffset()) * 1e6) /
			_timeSourceStream->Format().frame_rate);
		_lastPerfPublished = _timeSourceStream->BasePerfTime() + perfElapsed;
		_lastRealPublished = t.at_time;
		bigtime_t realElapsed = _lastRealPublished - _timeSourceStream->BaseRealTime();
		if (perfElapsed > 0 && realElapsed > 0)
		{
			// fetch current drift
			_drift = t.cur_frame_rate / _timeSourceStream->Format().frame_rate;
		}
		PublishTime(_lastPerfPublished, _lastRealPublished, _drift);
#if __RTLOG
		log->Log("TIME/active", _drift, _timeSourceStream->FramesPlayed(), _lastPerfPublished, _lastRealPublished);
#endif
//		D_TIMING(("^^^ stream: perf(%Ld) @ real(%Ld), now %Ld, drift %.4f: %Ld fr.played\n",
//			_lastPerfPublished, _lastRealPublished, now, _drift, _timeSourceStream->FramesPlayed()));
	}
	else
	{
		// freewheel
		bigtime_t perfElapsed = bigtime_t(double(now - _lastRealPublished) * _drift);
		PublishTime(_lastPerfPublished + perfElapsed, now, _drift);
#if __RTLOG
//		log->Log("TIME/passiv", _drift, 0, _lastPerfPublished + perfElapsed, now);
#endif
//		D_TIMING(("^^^ freewheel: perf(%Ld) @ real(%Ld), drift %.4f\n",
//			_lastPerfPublished + perfElapsed, now, _drift));

		media_timed_event ev(now + TIME_SOURCE_FREEWHEEL_PERIOD, E_TIME_SOURCE_FREEWHEEL);
		_realQueue->AddEvent(ev);
	}
}

void 
GameConsumer::FlushTimeSourceEvents()
{
	_realQueue->FlushEvents(0LL, BTimedEventQueue::B_ALWAYS, true, E_TIME_SOURCE_FREEWHEEL);
}


// #pragma mark --- [GameConsumer] format negotiation  ---
// ------------------------------------------------------------------------ //


status_t 
GameConsumer::ValidateFormat(media_multi_audio_format *ioFormat,
	DacInfo** dacs, uint32 dacCount)
{
	D_METHOD(("GameConsumer::ValidateFormat()\n"));
	if (!_dacCount) return B_ERROR;
	status_t ret = B_OK;
	status_t err;
	
	media_multi_audio_format& w = media_multi_audio_format::wildcard;

	// specialize/correct
	if (ioFormat->frame_rate == w.frame_rate)
		ioFormat->frame_rate = _preferredFrameRate;
	else
	{
		// apply frame-rate constraints
		err = ConstrainFrameRate(&ioFormat->frame_rate, dacs, dacCount);
		if (err < B_OK)
			ret = err;
	}

	if (ioFormat->format == w.format)
	{
		ioFormat->format = _preferredSampleFormat;
		ioFormat->valid_bits = _preferredSampleBits;
	}
	else
	{
		// apply format constraints
		err = ConstrainFormat(&ioFormat->format, &ioFormat->valid_bits, dacs, dacCount);
		if (err < B_OK) ret = err;
	}
	
	// temporary channel-count constraints
	// +++++
	if (ioFormat->channel_count == w.channel_count)
		ioFormat->channel_count = 2;
	err = constrain_channel_count(_dac[0].info, &ioFormat->channel_count);
	if (err < B_OK) ret = err;

	// force host endianness
	if (ioFormat->byte_order != B_MEDIA_HOST_ENDIAN)
	{
		if (ioFormat->byte_order != w.byte_order)
			ret = B_MEDIA_BAD_FORMAT;
		ioFormat->byte_order = B_MEDIA_HOST_ENDIAN;
	}

	err = constrain_buffer_size(
		ioFormat->format,
		ioFormat->channel_count,
		_dacFrameCount,
		&ioFormat->buffer_size);
	if (err < B_OK)	ret = err;

	return ret;	
}


// find best frame-rate match across all DACs
status_t 
GameConsumer::ConstrainFrameRate(float *ioFrameRate,
	DacInfo** dacs, uint32 dacCount) const
{
	if (!_dacCount) return B_ERROR;
	status_t err;
	float nearest = 0.;
	for (int n = 0; n < dacCount; n++)
	{
		float cur = *ioFrameRate;
		err = constrain_frame_rate(dacs[n]->info, &cur);
		if (err == B_OK && *ioFrameRate-cur > nearest-cur) nearest = cur;
	}
	if (nearest == 0.)
	{
		D_FORMAT((
			"GameConsumer::ConstrainFrameRate(%ld, %d):\n\t"
			"No matches.\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	*ioFrameRate = nearest;
	return B_OK;
}

// find best format match across all DACs, not very thoroughly
status_t 
GameConsumer::ConstrainFormat(uint32 *ioFormat, int16 *ioValidBits,
	DacInfo** dacs, uint32 dacCount) const
{
	if (!_dacCount) return B_ERROR;
	status_t err;
	uint32 format = 0;
	int16 valid = 32;
	for (int n = 0; n < dacCount; n++)
	{
		uint32 curFormat = *ioFormat;
		int16 curValid = *ioValidBits;
		
		err = constrain_format(dacs[n]->info, &curFormat, &curValid);
		if (err < B_OK)
		{
			D_FORMAT((
				"GameConsumer::ConstrainFormat(%ld, %d):\n\t"
				"DAC %d match failed: %s\n",
				*ioFormat, *ioValidBits, dacs[n]->info.codec_id, strerror(err)));
			return err;
		}
		
		if (!format)
		{
			format = curFormat;
			valid = curValid;
		}
		else
		{
			if (curFormat != format)
			{
				D_FORMAT((
					"GameConsumer::ConstrainFormat(%ld, %d):\n\t"
					"unmutual DAC at %ld\n",
					*ioFormat, *ioValidBits, n));
				return B_MEDIA_BAD_FORMAT;
			}
			if (curFormat == media_multi_audio_format::B_AUDIO_INT &&
				curValid < valid)
			{
				valid = curValid;
			}
		}
	}
	
	*ioFormat = format;
	*ioValidBits = valid;
	return B_OK;
}

status_t 
GameConsumer::BuildSplitChannelMap(
	const media_multi_audio_format &format,
	DacInfo **dacs, uint32 *outDacCount,
	split_channel_entry *splits, uint32 *outSplitCount)
{
	// +++++ splits/>2 channels not implemented.

	D_METHOD(("GameConsumer::BuildSplitChannelMap()\n"));
	if (!_dacCount) return B_ERROR;
	if (format.channel_count < 1 || format.channel_count > 2) return B_MEDIA_BAD_FORMAT;
	
	dacs[0] = &_dac[0];
	*outDacCount = 1;
	
	if (splits)
	{
		splits[0].source_channel_count = format.channel_count;
		splits[0].dac = dacs[0];
		splits[0].dac_channel_offset = 0;
		*outSplitCount = 1;
	}

	return B_OK;
}

status_t 
GameConsumer::SetDacFormat(DacInfo *dac, const media_multi_audio_format &format)
{
	D_METHOD(("GameConsumer::SetDacFormat()\n"));
	H<game_codec_format> gcf;
	G<game_set_codec_formats> gscf;
	gscf.formats = &gcf;
	gscf.in_request_count = 1;

	game_format_spec spec;
	spec.frame_rate = dac->info.cur_frame_rate;
	spec.cvsr = dac->info.cur_cvsr;
	spec.channel_count = dac->info.cur_channel_count;
	spec.designations = 0;
	spec.format = dac->info.cur_format;		
	
	D_FORMAT(("SetDacFormat() restrictions:\n 0x%x 0x%x 0x%x 0x%x\n",
		dac->info.frame_rates, dac->info.channel_counts, dac->info.designations, dac->info.formats));
	status_t err = make_game_format_spec(
		format,
		dac->info.frame_rates, dac->info.cvsr_min, dac->info.cvsr_max,
		dac->info.channel_counts, dac->info.designations, dac->info.formats,
		&spec);
	if (err < B_OK)
		return err;

	gcf.codec = dac->info.codec_id;
	gcf.flags = GAME_CODEC_FAIL_IF_DESTRUCTIVE;

	if (spec.frame_rate != dac->info.cur_frame_rate &&
		!(dac->info.stream_capabilities.frame_rates & spec.frame_rate))
	{
		gcf.flags |= GAME_CODEC_SET_FRAME_RATE;
		gcf.frame_rate = spec.frame_rate;
		gcf.cvsr = spec.cvsr;
	}
	if (spec.channel_count != dac->info.cur_channel_count &&
		!(dac->info.stream_capabilities.channel_counts & (1 << spec.channel_count-1)))
	{
		gcf.flags |= GAME_CODEC_SET_CHANNELS;
		gcf.channels = spec.channel_count;
	}
	if (spec.format != dac->info.cur_format &&
		!(dac->info.stream_capabilities.formats & spec.format))
	{
		gcf.flags |= GAME_CODEC_SET_FORMAT;
		gcf.format = spec.format;
	}
	
	return (ioctl(_fd, GAME_SET_CODEC_FORMATS, &gscf) < 0) ? errno : B_OK;
}


void 
GameConsumer::RefreshDacInfo()
{
	D_METHOD(("GameConsumer::RefreshDacInfo()\n"));
	status_t err;
	for (int n = 0; n < _dacCount; n++)
	{
		_dac[n].Refresh(_fd);
	}

	// figure preferred frame rate
	_preferredFrameRate = PREFERRED_FRAME_RATE;
	bool matched;
	for (int pass = 0; pass < 2; pass++)
	{
		matched = true;
		for (int n = 0; n < _dacCount; n++)
		{
			err = constrain_frame_rate(_dac[n].info, &_preferredFrameRate);
			if (err < B_OK)
			{
				matched = false;
				break;
			}
		}
		if (matched) break;
	}
	if (!matched)
	{
		D_TROUBLE((
			"GameConsumer::RefreshDacInfo():\n\t"
			"failed to find a consistent frame rate for all DACs.\n"));
		// punt
		_preferredFrameRate = PREFERRED_FRAME_RATE;
	}
	
	// figure preferred sample format for the first DAC:
	// - try int32/24/18
	// - try int16
	_preferredSampleFormat = media_multi_audio_format::B_AUDIO_INT;
	_preferredSampleBits = 32;
	err = constrain_format(_dac[0].info, &_preferredSampleFormat, &_preferredSampleBits);
	if (err < B_OK)
	{
		_preferredSampleFormat = media_multi_audio_format::B_AUDIO_SHORT;
		err = constrain_format(_dac[0].info, &_preferredSampleFormat, &_preferredSampleBits);
		if (err < B_OK)
		{
			D_WARNING((
				"GameConsumer::RefreshDacInfo():\n\t"
				"gah, the first DAC doesn't like B_AUDIO_SHORT. using %ld\n",
				_preferredSampleFormat));
		}
	}

	// test the format on remaining DACs
	for (int32 n = 1; n < _dacCount; n++)
	{
		uint32 format = _preferredSampleFormat;
		int16 bits = _preferredSampleBits;
		err = constrain_format(_dac[n].info, &format, &bits);
		if (err < B_OK)
		{
			D_TROUBLE((
				"*** GameConsumer::RefreshDacInfo():\n\t"
				"DAC %ld doesn't like format %ld\n",
				n, _preferredSampleFormat));
			// +++++ this is bad.
		}
	}
	
	// fetch the buffer size from each DAC
	_dacFrameCount = 0;
	for (int32 n = 0; n < _dacCount; n++)
	{
		if (!n) _dacFrameCount = _dac[n].info.cur_chunk_frame_count;
		else if(_dac[n].info.cur_chunk_frame_count != _dacFrameCount)
		{
			D_TROUBLE((
				"*** GameConsumer::RefreshDacInfo():\n\t"
				"chunk frame count mismatch: %ld, %ld on DAC %d\n",
				_dacFrameCount, _dac[n].info.cur_chunk_frame_count, _dac[n].info.codec_id));
			// +++++ this is also bad.
		}
	}
}

void 
GameConsumer::RefreshLatency()
{
	D_METHOD(("GameConsumer::RefreshLatency()\n"));
	// scan endpoints, notifying if their hardware latency differs from
	// the associated DAC's.
	for (Endpoint* e = _endpoints; e; e = e->next)
	{
		if (!e->stream) continue;
		ASSERT(e->stream->DAC());
		if (e->hardwareLatency == e->stream->DAC()->hardwareLatency) continue;
		// changed!
		e->hardwareLatency = e->stream->DAC()->hardwareLatency;
		bigtime_t total;
		media_node_id timesource;
		status_t err = GetLatencyFor(e->destination, &total, &timesource);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::RefreshLatency():\n\t"
				"GetLatencyFor(): %s\n"));
			continue;
		}
		err = SendLatencyChange(e->source, e->destination, total, 0);
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::RefreshLatency():\n\t"
				"SendLatencyChange(): %s\n"));
		}
	}
}

// figure the best buffer size for all DACs on this CPU
// +++++ I'm not bothering with a good multi-DAC algorithm until we need it.
status_t 
GameConsumer::InitFrameCount()
{
	D_METHOD(("GameConsumer::InitFrameCount()\n"));
	ASSERT(_dacCount > 0);
	status_t err;
	// get the recommended size
	BMediaRoster* roster = BMediaRoster::Roster();
	if (!roster)
	{
		D_TROUBLE((
			"GameConsumer::InitFrameCount():\n\t"
			"couldn't get BMediaRoster instance.\n"));
		return B_MEDIA_SYSTEM_FAILURE;
	}
	ssize_t sysSize = roster->AudioBufferSizeFor(
		2, // +++++ need a centrally-defined preferred channel count
		_preferredSampleFormat,
		_preferredFrameRate,
		B_PCI_BUS);
	if (sysSize <= 0)
	{
		D_TROUBLE((
			"GameConsumer::InitFrameCount():\n\t"
			"AudioBufferSizeFor() incoherent.\n"));
		return B_MEDIA_SYSTEM_FAILURE;
	}
	uint32 frameCount = _dac[0].info.min_chunk_frame_count;
	while (frameCount < sysSize)
	{
		if (_dac[0].info.chunk_frame_count_increment > 0)
		{
			frameCount += _dac[0].info.chunk_frame_count_increment;
		}
		else
		{
			frameCount <<= 1;
		}
		if (frameCount >= _dac[0].info.max_chunk_frame_count)
		{
			frameCount = _dac[0].info.max_chunk_frame_count;
			D_WARNING((
				"GameConsumer::InitFrameCount():\n\t"
				"hit max_chunk_frame_count of %ld for DAC 0.\n", frameCount));
			break;
		}
	}
	// set it up
	int setDac = 0;
	for (; setDac < _dacCount; setDac++)
	{
		H<game_codec_format> gcf;
		gcf.codec = _dac[setDac].info.codec_id;
		gcf.flags = GAME_CODEC_FAIL_IF_DESTRUCTIVE | GAME_CODEC_SET_CHUNK_FRAME_COUNT;
		gcf.chunk_frame_count = frameCount;
		G<game_set_codec_formats> gscf;
		gscf.formats = &gcf;
		gscf.in_request_count = 1;
		err = (ioctl(_fd, GAME_SET_CODEC_FORMATS, &gscf) < 0) ? errno : B_OK;
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::InitFrameCount():\n\t"
				"failed to set frame count %ld for DAC %d: %s\n\t"
				"reverting to %ld\n",
				frameCount, gcf.codec, strerror(err), _dacFrameCount));
			goto revert;
		}
	}
	_dacFrameCount = frameCount;
	return B_OK;
	
revert:
	// something went awry; revert touched DACs to the original frame count
	for (int n = 0; n < setDac; n++)
	{
		H<game_codec_format> gcf;
		gcf.codec = _dac[n].info.codec_id;
		gcf.flags = GAME_CODEC_FAIL_IF_DESTRUCTIVE | GAME_CODEC_SET_CHUNK_FRAME_COUNT;
		gcf.chunk_frame_count = _dacFrameCount;
		G<game_set_codec_formats> gscf;
		gscf.formats = &gcf;
		gscf.in_request_count = 1;
		err = (ioctl(_fd, GAME_SET_CODEC_FORMATS, &gscf) < 0) ? errno : B_OK;
		if (err < B_OK)
		{
			// bad bad bad
			D_TROUBLE((
				"GameConsumer::InitFrameCount():\n\t"
				"failed to revert frame count for DAC %d: %s\n",
				gcf.codec, strerror(err)));
			return err;
		}
	}
	return B_OK;
}

status_t 
GameConsumer::DescribeFreeInput(media_input *outInput)
{
	D_METHOD(("GameConsumer::DescribeFreeInput()\n"));
	ASSERT(_dacCount);

	outInput->destination.id = 0;
	outInput->destination.port = Node().port;
	outInput->source = media_source::null;
	outInput->format.type = B_MEDIA_RAW_AUDIO;
	
	media_multi_audio_format& f = outInput->format.u.raw_audio;
	RefreshDacInfo();
	
	if (_dac[0].info.cur_stream_count == _dac[0].info.max_stream_count)
	{
		// no streams available
		return B_ERROR;
	}

	f.frame_rate = _preferredFrameRate;
	f.format = _preferredSampleFormat;
	f.valid_bits = _preferredSampleBits;
	f.channel_count = 2; // +++++ define somewhere more visible
	f.byte_order = B_MEDIA_HOST_ENDIAN;
	constrain_buffer_size(
		f.format,
		f.channel_count,
		_dacFrameCount,
		&f.buffer_size);

	return B_OK;
}

void 
GameConsumer::DescribeInput(Endpoint *endpoint, media_input *outInput)
{
	D_METHOD(("GameConsumer::DescribeInput()\n"));
	ASSERT(endpoint);

	outInput->format.type = B_MEDIA_RAW_AUDIO;
	outInput->format.u.raw_audio = endpoint->format;
	outInput->source = endpoint->source;
	outInput->destination = endpoint->destination;
	outInput->node = Node();
}

// #pragma mark --- [GameConsumer] stream management ---
// ------------------------------------------------------------------------ //

status_t 
GameConsumer::CreateStream(
	DacInfo* dac,
	const media_multi_audio_format &format,
	const game_format_spec &streamSpec,
	int32 bufferCount,
	Stream **outStream)
{
	D_METHOD(("GameConsumer::CreateStream()\n"));
	status_t err;
	if (format.buffer_size <= 0)
	{
		D_TROUBLE((
			"GameConsumer::CreateStream():\n\t"
			"invalid buffer size\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	// create stream
	Stream* s = new Stream(this);
	err = s->Open(dac, format, streamSpec, bufferCount);
	if (err < B_OK)
	{
		D_TROUBLE((
			"GameConsumer::CreateStream():\n\t"
			"GameBufferSet::Stream::Open(): %s\n",
			strerror(err)));
		goto bail;
	}
#if 0
	// +++ emu10k.test
	_addon->RefreshSettings(_path.String());
#endif

	// insert in set
	Stream* cs;
	for (cs = _streams; cs; s = cs->next)
	{
		if (!cs->next)
		{
			cs->next = s;
			return B_OK;
		}
	}
	_streams = s;
	*outStream = s;
	return B_OK;
	
bail:
	if (s)
	{
		BAutolock _l(QueueLock());
		s->Close();
		delete s;
	}
	*outStream = 0;
	return err;
}

void 
GameConsumer::DeleteStream(Stream *ds)
{
	D_METHOD(("GameConsumer::DeleteStream()\n"));
	status_t err;

	BAutolock _l(_queueLock);

	// remove all queued events for the stream
	FlushStreamEvents(ds);
	
	// shut down the stream
	ds->Close();
	
	// remove it from the set
	Stream** s = &_streams;
	while (*s)
	{
		if (*s == ds)
		{
			*s = ds->next;
			delete ds;
			return;
		}
		s = &(*s)->next;
	}
	D_TROUBLE((
		"!!! GameConsumer::DeleteStream():\n\t"
		"stream %p not in set!\n",
		ds));
	delete ds;
}


void 
GameConsumer::StopAllStreams()
{
	D_METHOD(("GameConsumer::StopAllStreams()\n"));
	status_t err;

	BAutolock _l(_queueLock);

	for (Stream* s = _streams; s; s = s->next)
	{
		if (!s->IsStarted()) continue;
		err = s->Stop();
		if (err < B_OK)
		{
			D_TROUBLE((
				"GameConsumer::StopAllStreams():\n\t"
				"StopStream(%d): %s\n",
				s->ID(), strerror(err)));
			continue;
		}
	}
}

static BTimedEventQueue::queue_action
flush_stream_event(media_timed_event* event, void* context)
{
	switch (event->type)
	{
		case BTimedEventQueue::B_DATA_STATUS:
		case E_START_STREAM:
			if (!context) return BTimedEventQueue::B_REMOVE_EVENT;
			if (event->pointer == context) return BTimedEventQueue::B_REMOVE_EVENT;
			break;
	}
	return BTimedEventQueue::B_NO_ACTION;
}

void 
GameConsumer::FlushStreamEvents(Stream *s)
{
	D_METHOD(("GameConsumer::FlushStreamEvents()\n"));
	ASSERT(s);
	BAutolock _l(QueueLock());
	// flush any events related to the stream
	_realQueue->DoForEach(&flush_stream_event, s);
	_perfQueue->DoForEach(&flush_stream_event, s);
	// clear queue slots
	s->ClearQueue(Stream::PENDING);
	s->ClearQueue(Stream::PLAYING);
}

void 
GameConsumer::FlushAllStreamEvents()
{
	D_METHOD(("GameConsumer::FlushAllStreamEvents()\n"));
	BAutolock _l(QueueLock());
	// flush all stream-related events
	_realQueue->DoForEach(&flush_stream_event, 0);
	_perfQueue->DoForEach(&flush_stream_event, 0);
	// clear queue slots for each stream
	for (Stream* s = _streams; s; s = s->next)
	{
		s->ClearQueue(Stream::PENDING);
		s->ClearQueue(Stream::PLAYING);
	}
}

// #pragma mark --- [GameConsumer] endpoint management ---
// ------------------------------------------------------------------------ //

status_t 
GameConsumer::FindEndpoint(int32 destinationID, Endpoint **outEndpoint) const
{
	Endpoint* e;
	for (e = _endpoints; e && e->destination.id != destinationID; e = e->next) {}
	if (e)
	{
		*outEndpoint = e;
		return B_OK;
	}
	return B_MEDIA_BAD_DESTINATION;
}

void 
GameConsumer::CreateEndpoint(Stream *forStream, const media_source &source, Endpoint **outEndpoint)
{
	D_METHOD(("GameConsumer::CreateEndpoint()\n"));
	ASSERT(forStream);
	Endpoint* ne = new Endpoint(
		forStream,
		source,
		media_destination(Node().port, _nextEndpointID++));
	*outEndpoint = ne;
	for (Endpoint* e = _endpoints; e; e = e->next)
	{
		if (!e->next)
		{
			e->next = ne;
			return;
		}
	}
	_endpoints = ne;
	return;
}

status_t 
GameConsumer::DeleteEndpoint(Endpoint* de)
{
	D_METHOD(("GameConsumer::DeleteEndpoint()\n"));
	Endpoint** e;
	for (e = &_endpoints; *e;)
	{
		if ((*e) == de)
		{
			// shut down & remove the stream
			Stream*& s = (*e)->stream;
			ASSERT(s);
			DeleteStream(s);
			s = 0;

			RefreshDacInfo();

			if (_iteratorCount)
			{
				// mark endpoint for later removal
				(*e)->removed = true;
			}
			else
			{
				// remove endpoint
				*e = de->next;
				delete de;
			}
			return B_OK;
		}
		else
		{
			e = &(*e)->next;
		}
	}
	D_WARNING((
		"GameConsumer::DeleteEndpoint():\n\t"
		"endpoint %p not in set.\n",
		de));
	delete de;
	return B_OK;
}

void 
GameConsumer::ReapEndpoints()
{
	D_METHOD(("GameConsumer::ReapEndpoints()\n"));
	ASSERT(!_iteratorCount);
	Endpoint** e = &_endpoints;
	while (*e)
	{
		if ((*e)->removed)
		{
			ASSERT((*e)->stream == 0);
			Endpoint* de = *e;
			*e = de->next;
			delete de;
		}
		else
		{
			e = &(*e)->next;
		}
	}
}

void 
GameConsumer::ClearEndpointsAndStreams()
{
	D_METHOD(("GameConsumer::ClearEndpointsAndStreams()\n"));

	BAutolock _l(_queueLock);

	Endpoint* e = _endpoints;
	while (e)
	{
		if (e->stream)
		{
			DeleteStream(e->stream);
		}
		Endpoint* de = e;
		e = e->next;
		delete de;
	}
	_endpoints = 0;
}

// #pragma mark --- [GameConsumer] parameter web initialization ---
// ------------------------------------------------------------------------ //

status_t 
GameConsumer::BuildParameterWeb()
{
	D_METHOD(("GameConsumer::BuildParameterWeb()\n"));
	if (_parameterMap) return EPERM;
	status_t err;
	
	G<game_get_info> ggi;
	if (ioctl(_fd, GAME_GET_INFO, &ggi) < 0) return errno;
	
	game_mixer_info* mixerInfo = (game_mixer_info*)alloca(sizeof(game_mixer_info) * ggi.mixer_count);
	if (!mixerInfo) return B_NO_MEMORY;
	memset(mixerInfo, 0, sizeof(game_mixer_info) * ggi.mixer_count);
	for (int n = 0; n < ggi.mixer_count; n++) mixerInfo[n].mixer_id = GAME_MAKE_MIXER_ID(n);

	G<game_get_mixer_infos> ggmi;
	ggmi.info = mixerInfo;
	ggmi.in_request_count = ggi.mixer_count;
	if (ioctl(_fd, GAME_GET_MIXER_INFOS, &ggmi) < 0) return errno;
	
	// build parameter mapping for mixers we care about
	_parameterMap = new GameParameterMap;

	for (int n = 0; n < ggmi.out_actual_count; n++)
	{
		int32 codec_id = mixerInfo[n].linked_codec_id;
		if (codec_id != GAME_NO_ID && !GAME_IS_DAC(codec_id)) continue;
		err = _parameterMap->AppendMixerControls(_fd, mixerInfo[n]);
		if (err < B_OK)
		{
			D_TROUBLE(("GameConsumer::BuildParameterWeb():\n\t"
				"AppendMixerControls('%s'): %s\n", mixerInfo[n].name, strerror(err)));
		}
		err = _addon->StartWatchingSettings(Node(), _path.String(), mixerInfo[n].mixer_id);
		if (err < B_OK)
		{
			D_TROUBLE(("GameConsumer::BuildParameterWeb():\n\t"
				"_addon->StartWatchingSettings('%s'): %s\n", mixerInfo[n].name, strerror(err)));
		}
	}

	// generate the web
	BParameterWeb* web = 0;
	err = _parameterMap->MakeParameterWeb(_fd, &web,
		GameParameterMap::INCLUDE_DAC_MIXERS |
		GameParameterMap::INCLUDE_UNBOUND_MIXERS |
		GameParameterMap::ENABLE_ADVANCED_CONTROLS);
	if (err < B_OK)
	{
		D_TROUBLE(("GameConsumer::BuildParameterWeb():\n\t"
			"MakeParameterWeb(): %s\n", strerror(err)));
		delete _parameterMap; _parameterMap = 0;
		return err;
	}

	SetParameterWeb(web);
	return B_OK;
}

