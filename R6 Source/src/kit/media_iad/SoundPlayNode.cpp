#include <TimeSource.h>
#include <BufferGroup.h>
#include <SoundPlayer.h>
#include <Buffer.h>
#include <Autolock.h>

#include <scheduler.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SoundPlayNode.h"
#include "tr_debug.h"


namespace BPrivate {
	extern bool media_debug;
}


//	for debugging
#define USE_SOUND 1


#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF
#endif

#define WARNING //FPRINTF
#define LATE //FPRINTF
#define TRANSPORT //FPRINTF
#define NODE //FPRINTF
#define TIMING //FPRINTF
#define FORMAT FPRINTF
#define EVENT //FPRINTF
#define CALL	//FPRINTF

//	don't remove this!
#define DIAGNOSTIC fprintf

#define PRETEND_OK_COUNT 10

#define PREFERRED_SAMPLE_FORMAT media_raw_audio_format::B_AUDIO_FLOAT
#define PREFERRED_FRAME_RATE 44100
#if B_HOST_IS_BENDIAN
 #define PREFERRED_BYTE_ORDER B_MEDIA_BIG_ENDIAN
#else
 #define PREFERRED_BYTE_ORDER B_MEDIA_LITTLE_ENDIAN
#endif
#define PREFERRED_CHANNEL_COUNT 2
#define PREFERRED_BUFFER_SIZE 4096

#define M1 ((double)1000000.0)

#define READ_AHEAD 12000

_SoundPlayNode::_SoundPlayNode(
	const char * name,
	BSoundPlayer * player,
	const media_raw_audio_format * format) :
	BMediaNode(name), 
	BBufferProducer(B_MEDIA_RAW_AUDIO),
	BMediaEventLooper()
{
	m_frames_played = 0;
	m_player = player;
	m_downstream_latency = 15000;	// some guess
	m_buffers = NULL;
	m_media_time = 0;	//	target time for seeks
	m_delta = -1;		//	added to running frame count to generate buffer time
	m_muted = false;
	m_haveData = false;
	m_bufferReady = false;
	m_connected = false;
	m_prerolled = NULL;
	m_output.source.port = ControlPort();
	m_output.source.id = 1;
	m_raw_format = media_raw_audio_format::wildcard;
	if (format != NULL) {
		m_raw_format = *format;
	}
	if (m_raw_format.frame_rate <= media_raw_audio_format::wildcard.frame_rate) {
		m_raw_format.frame_rate = PREFERRED_FRAME_RATE;
	}
	if (m_raw_format.format <= media_raw_audio_format::wildcard.format) {
		m_raw_format.format = PREFERRED_SAMPLE_FORMAT;
	}
	if (m_raw_format.byte_order <= media_raw_audio_format::wildcard.byte_order) {
		m_raw_format.byte_order = PREFERRED_BYTE_ORDER;
	}
	if (m_raw_format.channel_count <= media_raw_audio_format::wildcard.channel_count) {
		m_raw_format.channel_count = PREFERRED_CHANNEL_COUNT;
	}
	if (m_raw_format.buffer_size <= media_raw_audio_format::wildcard.buffer_size) {
		m_raw_format.buffer_size = PREFERRED_BUFFER_SIZE;
	}
	m_output.format.type = B_MEDIA_RAW_AUDIO;
	m_output.format.u.raw_audio = media_raw_audio_format::wildcard;
	FormatSuggestionRequested(B_MEDIA_RAW_AUDIO, 0, &m_output.format);
	SetPriority(suggest_thread_priority(B_LIVE_AUDIO_MANIPULATION, 100, 3000, 100));

#if !NDEBUG
	char fm2[100];
	string_for_format(m_output.format, fm2, 100);
	fprintf(stderr, "SoundPlayNode format: %s\n", fm2);
#endif
}

_SoundPlayNode::~_SoundPlayNode()
{
	NODE(stderr, "_SoundPlayNode::~_SoundPlayNode()\n");

	EventQueue()->FlushEvents(B_INFINITE_TIMEOUT,BTimedEventQueue::B_BEFORE_TIME);
	if (m_prerolled) m_prerolled->Recycle();
	if (m_buffers) delete m_buffers;
}

bigtime_t _SoundPlayNode::Now()
{
	return frames_duration(m_frames_played);
}

struct _auto_free {
	void * p;
public:
	_auto_free(void * pp)
		{
			p = pp;
		}
	~_auto_free()
		{
			free(p);
		}
};

void 
_SoundPlayNode::HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent)
{
#if !NDEBUG
	bigtime_t realTime = BTimeSource::RealTime();
	bigtime_t eventTime = TimeSource()->RealTimeFor(event->event_time, EventLatency() + SchedulingLatency());
	EVENT(stderr, "_SoundPlayNode::HandleEvent(0x%lx, %Ld @ %Ld (%Ld), %Ld, %d)\n", event->type, eventTime, realTime, realTime - eventTime ,lateness, realTimeEvent);
#endif

	switch(event->type)
	{
		case BTimedEventQueue::B_START:
			{
				if (RunState() == B_STOPPED)
				{
					m_frames_played = 0;
					m_delta = event->event_time;
					if (m_connected && !m_muted && m_haveData) SendDataStatus(B_DATA_AVAILABLE, m_output.destination, event->event_time);
					bigtime_t at = frames_duration(m_frames_played)+m_delta;
					if (m_haveData)
					{
						media_timed_event newEvent(at, BTimedEventQueue::B_HANDLE_BUFFER);
						EventQueue()->AddEvent(newEvent);
					}
				}
			}
			break;
		
		case BTimedEventQueue::B_STOP:
			{
				if (RunState() == B_STARTED)
				{
					if (m_connected && !m_muted && m_haveData) SendDataStatus(B_DATA_NOT_AVAILABLE, m_output.destination, event->event_time);
					EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_AFTER_TIME, true, BTimedEventQueue::B_HANDLE_BUFFER);
				}
			}
			break;
			
		case BTimedEventQueue::B_HANDLE_BUFFER:
			{
				bigtime_t at = frames_duration(m_frames_played)+m_delta;
				BBuffer *buf = m_prerolled;
				m_prerolled = NULL;
				if (!buf) buf = ReadyBuffer(at);
				buf->Header()->start_time = at;
				m_frames_played += buf->Header()->size_used/frame_size(m_output.format.u.raw_audio);
				if (buf && (SendBuffer(buf, m_output.destination) < B_OK)) buf->Recycle();
				
				bigtime_t tsNow = TimeSource()->Now();
				bigtime_t neededToSendBy = at - m_downstream_latency;
				if (tsNow > neededToSendBy) m_delta += tsNow - neededToSendBy;
				at = frames_duration(m_frames_played)+m_delta;

				if (RunState() == B_STARTED && m_haveData)
				{
					media_timed_event newEvent(at, BTimedEventQueue::B_HANDLE_BUFFER);
					EventQueue()->AddEvent(newEvent);
				}
			}
			break;
		default:
			break;
	}
}

void _SoundPlayNode::Preroll()
{
	CALL(stderr, "_SoundPlayNode::Preroll\n");
	if (!m_bufferReady && !m_prerolled) m_prerolled = ReadyBuffer(-1);
}

void 
_SoundPlayNode::NodeRegistered()
{
	m_output.node = Node();
	Run();
	m_private_latency = SchedulingLatency() *2 + READ_AHEAD;
	SetEventLatency(m_private_latency);
}

status_t _SoundPlayNode::HandleMessage(
	int32 message,
	const void * data,
	size_t size)
{
	if (message == 0x60000001L) {
		if (!m_haveData) {
			m_haveData = true;
			bigtime_t eventTime = TimeSource()->Now();
			if (m_connected && !m_muted && RunState() == B_STARTED) SendDataStatus(B_DATA_AVAILABLE, m_output.destination, eventTime);
			m_frames_played = 0;
			m_delta = eventTime;
			if (RunState() == B_STARTED)
			{
				media_timed_event event(m_delta,BTimedEventQueue::B_HANDLE_BUFFER);
				EventQueue()->AddEvent(event);
			}
		};
	} else if (message == 0x60000002L) {
		if (m_haveData) {
			m_haveData = false;
			if (m_connected && !m_muted && RunState() == B_STARTED) SendDataStatus(B_DATA_NOT_AVAILABLE, m_output.destination,
				frames_duration(m_frames_played)+m_delta);
		};
	} else {
		return B_ERROR;
	}
	return B_OK;
}

BMediaAddOn* _SoundPlayNode::AddOn(
	int32 * internal_id) const
{
	return NULL;
}

BBuffer * _SoundPlayNode::ReadyBuffer(bigtime_t buffer_perf)
{
	if (!m_buffers) return NULL;
	if (m_delta == -1) m_delta = TimeSource()->Now() + 12000;
//	printf("creating new buffer for %Ld (delta=%Ld)\n",buffer_perf,m_delta);
	BBuffer *buffer = m_buffers->RequestBuffer(m_output.format.u.raw_audio.buffer_size);
	if (buffer) {
		if (m_player->_PlayBuffer) {
			BAutolock lock(m_player->_m_lock);
			m_player->_m_perfTime = (buffer_perf)==-1?-1:TimeSource()->RealTimeFor(buffer_perf,0);
			(*m_player->_PlayBuffer)(
				m_player->_m_cookie, buffer->Data(), m_output.format.u.raw_audio.buffer_size, 
				m_output.format.u.raw_audio);
		} else {
			m_player->PlayBuffer(
				buffer->Data(), m_output.format.u.raw_audio.buffer_size, 
				m_output.format.u.raw_audio);
		}
		buffer->Header()->size_used = m_output.format.u.raw_audio.buffer_size;
	};
	return buffer;
	
};

status_t _SoundPlayNode::FormatSuggestionRequested(
	media_type type,
	int32 /*quality*/,
	media_format * format)
{
	NODE(stderr, "_SoundPlayNode::FormatSuggestionRequested()\n");
	if (type <= 0) type = B_MEDIA_RAW_AUDIO;
	if (type != B_MEDIA_RAW_AUDIO) return B_MEDIA_BAD_FORMAT;
	format->type = type;
	format->u.raw_audio = media_raw_audio_format::wildcard;
	format->u.raw_audio.frame_rate = m_raw_format.frame_rate;
	format->u.raw_audio.channel_count = m_raw_format.channel_count;
	format->u.raw_audio.format = m_raw_format.format;
	format->u.raw_audio.byte_order = m_raw_format.byte_order;
	format->u.raw_audio.buffer_size = 0; //m_raw_format.buffer_size;
#if !NDEBUG
	char fmt[100];
	string_for_format(*format, fmt, 100);
	FORMAT(stderr, "return format %s\n", fmt);
#endif
	return B_OK;
}

status_t _SoundPlayNode::FormatProposal(
	const media_source & output,
	media_format * format)
{
	if (output != m_output.source) {
		NODE(stderr, "_SoundPlayNode::FormatProposal(): bad source\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (format->type <= 0) {
		FormatSuggestionRequested(B_MEDIA_RAW_AUDIO, 0, format);
	}
	else {
		if (format->type != B_MEDIA_RAW_AUDIO) {
			goto err;
		}
		if (format->u.raw_audio.format <= media_raw_audio_format::wildcard.format) {
			format->u.raw_audio.format = m_raw_format.format;
		}
		if (format->u.raw_audio.channel_count <= media_raw_audio_format::wildcard.channel_count) {
			format->u.raw_audio.channel_count = m_raw_format.channel_count;
		}
		else if (format->u.raw_audio.channel_count != m_raw_format.channel_count) {
			goto err;
		}
		if (format->u.raw_audio.format <= media_raw_audio_format::wildcard.format) {
			format->u.raw_audio.format = m_raw_format.format;
		}
		//	we don't care about byte order because we can handle both
		//	we also don't set the buffer size here
//		if (format->u.raw_audio.buffer_size < 1) {
//			format->u.raw_audio.buffer_size = m_raw_format.buffer_size;
//		}
		if ((format->u.raw_audio.buffer_size < 0) || (format->u.raw_audio.buffer_size > 65536)) {
			goto err;
		}
		if (format->u.raw_audio.byte_order <= media_raw_audio_format::wildcard.byte_order) {
			format->u.raw_audio.byte_order = m_raw_format.byte_order;
		}
	}
	m_output.format = *format;
//	format->u.raw_audio.byte_order = media_raw_audio_format::wildcard.byte_order;
#if !NDEBUG
	char fmt[100];
	string_for_format(*format, fmt, 100);
	FORMAT(stderr, "FormatProposal: %s\n", fmt);
#endif
	return B_OK;
err:
	FormatSuggestionRequested(B_MEDIA_RAW_AUDIO, 0, format);
	return B_MEDIA_BAD_FORMAT;
}

status_t _SoundPlayNode::FormatChangeRequested(
	const media_source & source,
	const media_destination & destination,
	media_format * io_format,
	int32 * out_change_count)
{
	status_t err = FormatProposal(source, io_format);
	if (err < B_OK) return err;
//	*out_change_count = IncrementChangeTag();
	m_output.format = *io_format;
	alloc_buffers();
	return B_OK;
}

status_t _SoundPlayNode::GetNextOutput(
	int32 * cookie,
	media_output * out_output)
{
	if (*cookie == 0) {
		*out_output = m_output;
		*cookie = 1;
	}
	else {
		return B_BAD_INDEX;
	}
	return B_OK;
}

status_t _SoundPlayNode::DisposeOutputCookie(
	int32 /* cookie */)
{
	return B_OK;
}

status_t _SoundPlayNode::SetBufferGroup(
	const media_source & for_source,
	BBufferGroup * group)
{
	if (for_source != m_output.source) {
		NODE(stderr, "_SoundPlayNode::SetBufferGroup(): bad source\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (group != m_buffers) {
		delete m_buffers;
		m_buffers = group;
	}
	return B_OK;
}

status_t _SoundPlayNode::VideoClippingChanged(
	const media_source& /*for_source*/,
	int16 /*num_shorts*/,
	int16* /*clip_data*/,
	const media_video_display_info& /*display*/,
	int32* /*out_from_change_count*/)
{
	return B_ERROR;
}

status_t _SoundPlayNode::GetLatency(
	bigtime_t * out_latency)
{
	CALL(stderr, "_SoundPlayNode::GetLatency\n");

	status_t err = BBufferProducer::GetLatency(out_latency);
	if (err >= B_OK) {
		*out_latency += m_private_latency;
	}

	CALL(stderr, "_SoundPlayNode::GetLatency out_latency: %Ld\n", *out_latency);

	return err;
}

status_t _SoundPlayNode::PrepareToConnect(
	const media_source & what,
	const media_destination & where,
	media_format * format,
	media_source * out_source,
	char * out_name)
{
	NODE(stderr, "_SoundPlayNode::PrepareToConnect()\n");
	if (what != m_output.source) {
		NODE(stderr, "_SoundPlayNode::PrepareToConnect(): bad source\n");
		return B_MEDIA_BAD_SOURCE;
	}
	if (m_output.destination != media_destination::null) {
		NODE(stderr, "_SoundPlayNode::PrepareToConnect(): already connected\n");
		return B_MEDIA_BAD_DESTINATION;
	}
	if (!m_output.format.u.raw_audio.buffer_size) {
		if (format->u.raw_audio.buffer_size > 0) {
			m_output.format.u.raw_audio.buffer_size = format->u.raw_audio.buffer_size;
		}
		else {
			format->u.raw_audio.buffer_size = m_raw_format.buffer_size;
		}
	}
#if DEBUG
		char fmt[100];
		string_for_format(m_output.format, fmt, 100);
		FORMAT(stderr, "we're suggesting %s\n", fmt);
		string_for_format(*format, fmt, 100);
		FORMAT(stderr, "he's expecting %s\n", fmt);
#endif
	if (!format_is_compatible(*format, m_output.format)) {
		NODE(stderr, "_SoundPlayNode::PrepareToConnect(): bad format\n");
		return B_MEDIA_BAD_FORMAT;
	}
	m_output.format = *format;
	*out_source = m_output.source;
	strncpy(out_name, Name(), B_MEDIA_NAME_LENGTH);
	return B_OK;
}

void _SoundPlayNode::Connect(
	status_t error, 
	const media_source & source,
	const media_destination & destination,
	const media_format & format,
	char * io_name)
{
	if (error < B_OK) {
		NODE(stderr, "_SoundPlayNode::Connect(): we were told about an error\n");
		m_output.destination = media_destination::null;
		return;
	}
	m_output.destination = destination;
	m_output.format = format;
	m_connected = true;

	media_node_id tsId;
	FindLatencyFor(destination, &m_downstream_latency, &tsId);
	SetEventLatency(m_downstream_latency + m_private_latency);
	strncpy(io_name, Name(), B_MEDIA_NAME_LENGTH);
	alloc_buffers();
}

void _SoundPlayNode::alloc_buffers()
{
	delete m_buffers;

	bigtime_t latency = m_private_latency + m_downstream_latency;
	int count = (int)(latency/(1000000.0*m_output.format.u.raw_audio.buffer_size/(frame_size(
		m_output.format.u.raw_audio)*m_output.format.u.raw_audio.frame_rate)))+2;
	// three buffers allow triple-buffering
	if (count < 3) count = 3;
	// don't use more than 128 kB for future buffers
	if (count*m_output.format.u.raw_audio.buffer_size > 128000) {
		count = 128000/m_output.format.u.raw_audio.buffer_size;
		// make sure there's at least one buffer, though
		if (count < 1) count = 1;
	}
FPRINTF(stderr, "SoundPlayNode::Connect() format %g;%x;%d;%d;%d\n", m_output.format.u.raw_audio.frame_rate, 
		m_output.format.u.raw_audio.format, m_output.format.u.raw_audio.channel_count, 
		m_output.format.u.raw_audio.byte_order, m_output.format.u.raw_audio.buffer_size);
#if NDEBUG
	if (BPrivate::media_debug) 
#endif
		DIAGNOSTIC(stderr, "SoundPlayNode creating %d buffers of size %ld for latency %Ld\n",
			count,m_output.format.u.raw_audio.buffer_size, latency);
	m_buffers = new BBufferGroup(m_output.format.u.raw_audio.buffer_size, count);
}

void _SoundPlayNode::Disconnect(
	const media_source & what,
	const media_destination & where)
{
	if ((what != m_output.source) || (where != m_output.destination)) {
		NODE(stderr, "_SoundPlayNode::Disconnect(): bad connection\n");
		return;
	}

	if (m_prerolled) m_prerolled->Recycle();
	EventQueue()->FlushEvents(B_INFINITE_TIMEOUT,BTimedEventQueue::B_BEFORE_TIME);

	m_output.destination = media_destination::null;
	m_output.format.u.raw_audio.buffer_size = 0;
	m_connected = false;
	m_prerolled = NULL;
	delete m_buffers;
	m_buffers = NULL;
}

void _SoundPlayNode::LateNoticeReceived(
	const media_source & /*what*/,
	bigtime_t how_much,
	bigtime_t performance_time)
{
	LATE(stderr, "_SoundPlayNode::LateNoticeReceived(%.4f @ %.4f)\n",
		(double)how_much/M1, (double)performance_time/M1);
	switch (RunMode()) {
	case B_OFFLINE:
		/* shouldn't be here at all */
		break;
	case B_DECREASE_PRECISION:
		/* hope to catch up */
		break;
	case B_INCREASE_LATENCY:
		if (how_much > 3000) {
			how_much = 3000;
		}
		m_downstream_latency += how_much;
		m_private_latency += how_much/2;
		break;
	default:	/* drop data */
		m_frames_played += m_output.format.u.raw_audio.buffer_size/frame_size(m_output.format.u.raw_audio);
		break;
	}
}

void _SoundPlayNode::EnableOutput(
	const media_source & what,
	bool enabled,
	int32 * change_tag)
{
	if (what != m_output.source) {
		NODE(stderr, "_SoundPlayNode::EnableOutput(): bad source\n");
		return;
	}
	m_muted = !enabled;
//	*change_tag = IncrementChangeTag();
	if (m_output.destination != media_destination::null) {
		SendDataStatus(m_muted ? B_DATA_NOT_AVAILABLE : B_DATA_AVAILABLE, 
			m_output.destination, TimeSource()->Now());
	}
}

bigtime_t _SoundPlayNode::frames_duration(
	int64 num_frames)
{
	return (bigtime_t)(num_frames*1000000LL/(double)m_output.format.u.raw_audio.frame_rate);
}

size_t _SoundPlayNode::frame_size(
	const media_raw_audio_format & format)
{
	return format.channel_count * (format.format & 0xf);
}
