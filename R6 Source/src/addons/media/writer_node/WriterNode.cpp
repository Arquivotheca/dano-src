
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include <Debug.h>
#include <Entry.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Buffer.h>
#include <TimedEventQueue.h>
#include <TimeSource.h>

#include "WriterNode.h"
#include "WriterAddOn.h"


//fixme	hplus:	if input audio format has different buffer size
//	than encoder, we should provide buffering.

#if NDEBUG
	#define FUNC(x)
#else
	#define FUNC(x) printf x
#endif

//hplus	fixme:
//	it's un-clear what to put here right now, so we put on some baggy raver pants
#define MY_LATENCY 30000LL



	//	calculate duration of a buffer of data
	//
	static bigtime_t
	format_latency(const media_format & format)
	{
		switch (format.type) {
		case B_MEDIA_RAW_AUDIO:
			if (!(format.u.raw_audio.format & 0xf) || !format.u.raw_audio.channel_count ||
					(format.u.raw_audio.frame_rate < 0.00001)) {
				return 0LL;	//	invalid format, no latency
			}
			return bigtime_t((format.u.raw_audio.buffer_size*1000000L)/double(format.u.raw_audio.frame_rate*
					format.u.raw_audio.channel_count*(format.u.raw_audio.format & 0xf)));
		case B_MEDIA_RAW_VIDEO:
			if (format.u.raw_video.field_rate < 0.00001) {
				return 0LL;	//	invalid format, no latency
			}
			return bigtime_t(1000000LL/double(format.u.raw_video.field_rate));
			break;
		case B_MEDIA_ENCODED_AUDIO:
			if (!(format.u.encoded_audio.output.format & 0xf) || !format.u.encoded_audio.output.channel_count ||
					(format.u.encoded_audio.output.frame_rate < 0.00001)) {
				return 0LL;	//	invalid format, no latency
			}
			return bigtime_t((format.u.encoded_audio.output.buffer_size*1000000L)/double(format.u.encoded_audio.output.frame_rate*
					format.u.encoded_audio.output.channel_count*(format.u.encoded_audio.output.format & 0xf)));
			break;
		case B_MEDIA_ENCODED_VIDEO:
			if (format.u.encoded_video.output.field_rate < 0.00001) {
				return 0LL;	//	invalid format, no latency
			}
			return bigtime_t(1000000LL/double(format.u.encoded_video.output.field_rate));
			break;
		}
		return 0LL;		//	unknown format, no latency
	}


WriterNode::WriterNode(WriterAddOn *addon, const flavor_info * flavor, const char *name, status_t * out_error) :
	BMediaNode(name), BMediaEventLooper(), BBufferConsumer(B_MEDIA_UNKNOWN_TYPE), BFileInterface()
{
	FUNC(("WriterNode::WriterNode(0x%lx, %p:0x%lx, '%s')\n", addon, flavor, flavor->internal_id, name));
	m_outputFile = NULL;
	memset(m_inputs, 0, sizeof(m_inputs));
	for (int ix=0; ix<MAX_TRACKS; ix++) {
		printf("WriterNode() a %d (this %p)\n", ix, this);
		m_tracks[ix] = 0;
		m_producerStatus[ix] = B_DATA_AVAILABLE;
		m_inputs[ix].destination.port = ControlPort();
		m_inputs[ix].destination.id = ix+1;
		m_inputs[ix].source = media_source::null;
		const media_file_format * mff = reinterpret_cast<const media_file_format *>(flavor->internal_id);
		printf("WriterNode() b %p (this %p) (mff %p)\n", m_inputs[ix].name, this, mff);
#if DEBUG
		int l;
		for (l=0; l<sizeof(mff->short_name); l++) {
			if (!mff->short_name[l]) break;
		}
		printf("l = %d, size %d\n", l, sizeof(mff->short_name));
#endif
		sprintf(m_inputs[ix].name, "%s input %d", mff ? mff->short_name : "writer", ix+1);
	}
	printf("WriterNode() c\n");
	memset(m_encoderChosen, 0, sizeof(m_encoderChosen));
	memset(m_encoder, 0, sizeof(m_encoder));
	m_addon = addon;
	m_flavor = flavor;
	m_fileFormat = reinterpret_cast<const media_file_format *>(m_flavor->internal_id);
	if (out_error) *out_error = B_OK;
	m_needHeaderCommit = false;
	m_headerCommitted = false;
	m_latency = MY_LATENCY;
	printf("WriterNode() d\n");
}


WriterNode::~WriterNode()
{
	FUNC(("WriterNode::~WriterNode()\n"));
	Quit();
	if (m_outputFile) {
		m_outputFile->CloseFile();
		delete m_outputFile;
	}
}

BMediaAddOn *
WriterNode::AddOn(int32 *internal_id) const
{
	FUNC(("WriterNode::AddOn(0x%lx)\n", internal_id));
	if (internal_id) *internal_id = m_flavor->internal_id;
	return m_addon;
}

void 
WriterNode::NodeRegistered()
{
	FUNC(("WriterNode::NodeRegistered()\n"));
	BMediaEventLooper::Run();
}

status_t 
WriterNode::HandleMessage(int32 message, const void *data, size_t size)
{
	FUNC(("WriterNode::HandleMessage(%ld, 0x%lx, %ld)\n", message, data, size));
	//	there are no custom messages -- right now, at least
	return B_ERROR;
}

void 
WriterNode::Preroll()
{
	if (m_needHeaderCommit && !m_headerCommitted) {
		status_t err = m_outputFile->CommitHeader();
		m_headerCommitted = true;
		m_needHeaderCommit = false;
	}
}

	static size_t
	count_frames(BBuffer * buffer, const media_format & format)
	{
		switch (format.type) {
		case B_MEDIA_RAW_AUDIO:
			return buffer->SizeUsed()/((format.u.raw_audio.format & 0xf) *
					(format.u.raw_audio.channel_count));
			break;
		case B_MEDIA_RAW_VIDEO:
			return 1;
			break;
		default:
			break;
		}
		TRESPASS();
		return buffer->SizeUsed();
	}

void 
WriterNode::HandleEvent(const media_timed_event *event, bigtime_t lateness, bool realTimeEvent)
{
	FUNC(("WriterNode::HandleEvent(%Ld, %ld, 0x%lx, %ld, %ld, %Ld)\n", event->event_time,
			event->type, (uint32)event->pointer, event->cleanup, event->data, event->bigdata));

	switch (event->type) {
	case BTimedEventQueue::B_START:
		if (m_needHeaderCommit) {
			Preroll();
		}
#if 0
		//	if in offline mode
		... this should only happen once buffers are reveived ...
		... else there is a race when this Node starts before upstreams ...
		if (RunMode() == B_OFFLINE) {
			for (int ix=0; ix<MAX_TRACKS; ix++) {
				//	send a requestadditionalbuffer message to each connected upstream person
				if ((m_inputs[ix].source != media_source::null) && (m_producerStatus[ix] == B_DATA_AVAILABLE)) {
					RequestAdditionalBuffer(m_inputs[ix].source, event->event_time);
				}
			}
		}
#endif
		break;
	case BTimedEventQueue::B_STOP:
		for (int ix=0; ix<MAX_TRACKS; ix++) {
			//	make sure we un-mark connected people from their "stopped" position
			if (m_producerStatus[ix] == B_PRODUCER_STOPPED) {
				m_producerStatus[ix] == B_DATA_AVAILABLE;
			}
		}
		break;
	case BTimedEventQueue::B_WARP:
	case BTimedEventQueue::B_SEEK:
		//fixme	hplus: when we support buffer time stamping,
		//	we should do seeking, too.
		//	Could also happen when we're in replace mode
		break;
	case BTimedEventQueue::B_DATA_STATUS:
		m_producerStatus[event->data] = event->bigdata;
		recalc_time();
		break;
	case BTimedEventQueue::B_HANDLE_BUFFER: {
			BBuffer * b = reinterpret_cast<BBuffer *>(event->pointer);
			//	write the buffer out - reordering is done in the buffer queue, so that will already have happened
			int ix = b->Header()->destination-1;
			if (m_tracks[ix] != 0) {
				//fixme	hplus:	we should only do key frames every so often
				uint32 flags = B_MEDIA_KEY_FRAME;
				if ((m_inputs[ix].format.type == B_MEDIA_RAW_AUDIO) || (m_inputs[ix].format.type == B_MEDIA_RAW_VIDEO)) {
					size_t frame_count = count_frames(b, m_inputs[ix].format);
					status_t err = m_tracks[ix]->WriteFrames((const char *)b->Data(), frame_count, flags);
					if (err < B_OK) {
						fprintf(stderr, "WriterNode::WriteFrames(): %s\n", strerror(err));
					}
				}
				else {
					switch (m_inputs[ix].format.type) {
					case B_MEDIA_ENCODED_AUDIO:
						flags = b->Header()->u.encoded_audio.buffer_flags & B_MEDIA_KEY_FRAME;
						break;
					case B_MEDIA_ENCODED_VIDEO:
						flags = b->Header()->u.encoded_audio.buffer_flags & B_MEDIA_KEY_FRAME;
						break;
					default:
						//	nothing
						;
					}
					status_t err = m_tracks[ix]->WriteChunk((const char *)b->Data(), b->SizeUsed(), flags);
					if (err < B_OK) {
						fprintf(stderr, "WriterNode::WriteChunk(): %s\n", strerror(err));
					}
				}
			}
			if (RunMode() == B_OFFLINE) {
				for (int ix=0; ix<MAX_TRACKS; ix++) {
					//	send a requestadditionalbuffer message to each connected upstream person
					if ((m_inputs[ix].source != media_source::null) && (m_producerStatus[ix] == B_DATA_AVAILABLE)) {
						RequestAdditionalBuffer(m_inputs[ix].source, b);
					}
				}
			}
			//	recycle the buffer
			b->Recycle();
		}
		break;
	}
}


	static BTimedEventQueue::queue_action
	update_times(media_timed_event * event, void * context)
	{
		assert(event->type == BTimedEventQueue::B_HANDLE_BUFFER);
		BBuffer * b = reinterpret_cast<BBuffer *>(event->pointer);
		bigtime_t * times = reinterpret_cast<bigtime_t *>(context);
		times[b->Header()->destination-1] = b->Header()->start_time;
		return BTimedEventQueue::B_NO_ACTION;
	}

void
WriterNode::recalc_time()
{
	bool active[MAX_TRACKS];
	bool any_active = false;

	//	figure out whether any channels are active
	for (int ix=0; ix<MAX_TRACKS; ix++) {
		if (m_inputs[ix].source == media_source::null) {
			active[ix] = false;
			continue;
		}
		if (m_producerStatus[ix] != B_DATA_AVAILABLE) {
			active[ix] = false;
			continue;
		}
		any_active = true;
		active[ix] = true;
	}
	//	if so, find the latest common time
	if (any_active) {
		//	find the latest time for each channel
		bigtime_t last_seen[MAX_TRACKS];
		for (int ix=0; ix<MAX_TRACKS; ix++) last_seen[ix] = LONGLONG_MIN;
		EventQueue()->DoForEach(update_times, last_seen, 0, BTimedEventQueue::B_ALWAYS,
				true, BTimedEventQueue::B_HANDLE_BUFFER);
		//	figure out the minimum of those among active channels
		bigtime_t now = LONGLONG_MAX;
		for (int ix=0; ix<MAX_TRACKS; ix++) {
			if (active[ix]) {
				if (last_seen[ix] < now) {
					now = last_seen[ix];
				}
			}
		}
		//	update offline time
		if (now == LONGLONG_MAX) {
			//	no data, no action
		}
		else if (now > OfflineTime()) {
			SetOfflineTime(now);
		}
	}
	//	else find a stop event, if any
	else {
		const media_timed_event * e = EventQueue()->FindFirstMatch(0, BTimedEventQueue::B_ALWAYS,
				true, BTimedEventQueue::B_STOP);
		//	and set the time to that time, so it'll get handled
		if (e != 0) {
			SetOfflineTime(e->event_time);
		}
	}
}

status_t 
WriterNode::AcceptFormat(const media_destination &dest, media_format *format)
{
	FUNC(("WriterNode::AcceptFormat(%ld:%ld, %ld)\n", dest.port, dest.id, format->type));
	//fixme	hplus:	should check format
	//	if encoded but no encoder specified, decline
	//	if we have output file
		//	check output format for that file
	//	else
		//	check against flavor formats
	return B_OK;
}

status_t 
WriterNode::GetNextInput(int32 *cookie, media_input *out_input)
{
	FUNC(("WriterNode::GetNextInput(%ld, 0x%lx)\n", *cookie, out_input));
	media_input * input = reinterpret_cast<media_input *>(*cookie);
	if (input == 0) {
		input = m_inputs;
	}
	else {
		input++;
	}
	if (input >= &m_inputs[MAX_TRACKS]) return B_BAD_INDEX;
	*out_input = *input;
	*cookie = reinterpret_cast<int32>(input);
	return B_OK;
}

void 
WriterNode::DisposeInputCookie(int32 cookie)
{
	FUNC(("WriterNode::DisposeInputCookie(%ld)\n", cookie));
	&cookie;
}

void 
WriterNode::BufferReceived(BBuffer *buffer)
{
	FUNC(("WriterNode::BufferReceived(%ld)\n", buffer->ID()));
	//fixme	hplus:	B_OFFLINE mode (also needs MediaEventLooper fixes)
	int ix = buffer->Header()->destination-1;
	if ((ix < 0) || (ix >= MAX_TRACKS) || (m_tracks[ix] == 0)) {
		PRINT(("Buffer received for bad destination %d\n", ix+1));
		buffer->Recycle();
		return;
	}
	bigtime_t now, then;
	then = buffer->Header()->start_time;
	if (RunMode() != B_OFFLINE) {
		now = TimeSource()->Now();
	}
	else {
		now = then;
	}
	//	if buffer is late and run mode is not accepting of that
	if (then < now) {
		//	if previous status was data-not-available, ignore that it's late
		if (m_producerStatus[ix] == B_DATA_NOT_AVAILABLE) {
			goto put_in_queue;
		}
		//	else send late notice and drop buffer
		switch (RunMode()) {
		case B_DROP_DATA:
			buffer->Recycle();
			PRINT(("dropping late buffer (%Ld < %Ld)\n", then, now));
			break;
		case B_INCREASE_LATENCY: {
				m_latency += now-then;
				SendLatencyChange(m_inputs[ix].source, m_inputs[ix].destination, m_latency);
			}
		case B_DECREASE_PRECISION:
		default:
			NotifyLateProducer(m_inputs[ix].source, now-then, then);
		case B_RECORDING:
		case B_OFFLINE:
			goto put_in_queue;
		}
	}
	else {
put_in_queue:
		//	mark producer as producing
		m_producerStatus[ix] = B_DATA_AVAILABLE;
		//	push buffer on event queue
		media_timed_event e(then, BTimedEventQueue::B_HANDLE_BUFFER, buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
		if (EventQueue()->AddEvent(e) != B_OK) {
			buffer->Recycle();
		}
	}
}

void 
WriterNode::ProducerDataStatus(const media_destination &for_whom, int32 status, bigtime_t at_performance_time)
{
	FUNC(("WriterNode::ProducerDataStatus(%ld, %ld, %Ld)\n", for_whom.id, status, at_performance_time));
	int ix = for_whom.id-1;
	if ((ix < 0) || (ix >= MAX_TRACKS) || (m_inputs[ix].source == media_source::null)) {
		fprintf(stderr, "WriterNode(%d)::ProducerDataStatus() for bad destination %d\n",
				ID(), for_whom.id);
		return;
	}
	//	push event on queue
	media_timed_event e(at_performance_time, BTimedEventQueue::B_DATA_STATUS, NULL, 0, ix, status, NULL);
	if (EventQueue()->AddEvent(e) != B_OK) {
		m_producerStatus[ix] = status;	//	do it now if we can't queue it
	}
}

status_t 
WriterNode::GetLatencyFor(const media_destination &for_whom, bigtime_t *out_latency, media_node_id *out_timesource)
{
	FUNC(("WriterNode::GetLatencyFor(%ld, %Ld, 0x%lx)\n", for_whom.id, *out_latency, out_timesource));
	if (out_timesource) *out_timesource = TimeSource()->ID();
	*out_latency = MY_LATENCY;
	bigtime_t max_latency = 0;
	for (int ix=0; ix<MAX_TRACKS; ix++) {
		if (m_inputs[ix].source != media_source::null) {
			bigtime_t cur_latency = format_latency(m_inputs[ix].format);
			if (cur_latency > max_latency) max_latency = cur_latency;
		}
	}
	*out_latency += max_latency;
	return B_OK;
}

status_t 
WriterNode::Connected(const media_source &producer, const media_destination &where,
	const media_format &with_format, media_input *out_input)
{
	FUNC(("WriterNode::Connected(%ld:%ld, %ld, %ld, '%s')\n", producer.port, producer.id, where.id, with_format.type, out_input->name));
	int ix = where.id-1;
	if ((ix < 0) || (ix >= MAX_TRACKS)) {
		return B_MEDIA_BAD_DESTINATION;
	}
	assert(m_inputs[ix].destination == where);
	if ((m_outputFile != 0) && (m_tracks[ix] != 0)) {
		PRINT(("Re-connecting to track not yet supported\n"));
		return B_UNSUPPORTED;
		//fixme	hplus:
		//	reconnect to an existing track
		//	check that the format is OK
		//	write empty data buffers if there's a gap
	}
	m_inputs[ix].format = with_format;
	if (m_outputFile) {
		if (((with_format.type == B_MEDIA_RAW_VIDEO) || (with_format.type == B_MEDIA_RAW_AUDIO))
				&& m_encoderChosen[ix]) {
			m_tracks[ix] = m_outputFile->CreateTrack(&m_inputs[ix].format, &m_encoder[ix]);
		}
		else {
			m_tracks[ix] = m_outputFile->CreateTrack(&m_inputs[ix].format);
		}
	}
	m_inputs[ix].source = producer;
	*out_input = m_inputs[ix];
	//	update any other guys about possible new latency
	bigtime_t old_latency = m_latency;
	m_latency = MY_LATENCY;
	GetLatencyFor(media_destination::null, &m_latency, NULL);
	if (m_latency != old_latency) for (int ix=0; ix<MAX_TRACKS; ix++) {
		if ((m_inputs[ix].source != media_source::null) && (m_inputs[ix].source != producer)) {
			SendLatencyChange(m_inputs[ix].source, m_inputs[ix].destination, m_latency);
		}
	}
	return B_OK;
}

void 
WriterNode::Disconnected(const media_source &producer, const media_destination &where)
{
	FUNC(("WriterNode::Disconnected(%ld:%ld, %ld)\n", producer.port, producer.id, where.id));
	int32 ix = where.id-1;
	if ((ix < 0) || (ix >= MAX_TRACKS)) {
		return;		//	bad connection
	}
	assert(m_inputs[ix].source == producer);
	m_inputs[ix].source = media_source::null;
	sprintf(m_inputs[ix].name, "%s input %d", m_fileFormat ? m_fileFormat->short_name : "writer", ix+1);
	//	we're going to hang on to the track to allow re-connections
}

status_t 
WriterNode::FormatChanged(const media_source &producer, const media_destination &consumer, int32 change_tag, const media_format &format)
{
	FUNC(("WriterNode::FormatChanged(%ld:%ld, %ld, %ld, %ld)\n", producer.port, producer.id, consumer.id, change_tag, format.type));
	//fixme	hplus:
	//	if we do more than one track, it would be nice to allow format changes by just
	//	creating another track. Also, if there has been no data written yet, deleting
	//	the track and re-creating it would be spiffy, too.
	return B_MEDIA_BAD_FORMAT;
}

status_t 
WriterNode::SeekTagRequested(const media_destination &destination, bigtime_t in_target_time, uint32 in_flags, media_seek_tag *out_seek_tag, bigtime_t *out_tagged_time, uint32 *out_flags)
{
	FUNC(("WriterNode::SeekTagRequested(%ld, %Ld, 0x%lx, 0x%lx, 0x%lx)\n", destination.id, in_target_time, in_flags, out_seek_tag, out_flags));
	//	getting seek tags from the writer node sounds kind-of silly
	return B_BAD_INDEX;
}

status_t 
WriterNode::GetNextFileFormat(int32 *cookie, media_file_format *out_format)
{
	FUNC(("WriterNode::GetNextFileFormat(%ld, 0x%lx)\n", *cookie, out_format));
	if (*cookie == 0) {
		*out_format = *m_fileFormat;
		*cookie = 1;
		return B_OK;
	}
	return B_BAD_INDEX;		//	no more
}

void 
WriterNode::DisposeFileFormatCookie(int32 cookie)
{
	FUNC(("WriterNode::DisposeFileFormatCookie(%ld)\n", cookie));
	//	do nothing
}

status_t 
WriterNode::GetDuration(bigtime_t *out_time)
{
	FUNC(("WriterNode::GetDuration(0x%lx)\n", out_time));
	//	we're a writer node
	return B_ERROR;
}

status_t 
WriterNode::SniffRef(const entry_ref &file, char *out_mime_type, float *out_quality)
{
	FUNC(("WriterNode::SniffRef('%s', 0x%lx, 0x%lx)\n", file.name, out_mime_type, out_quality));
	//	we're a writer node
	return B_ERROR;
}

status_t 
WriterNode::SetRef(const entry_ref &file, bool create, bigtime_t *out_time)
{
	FUNC(("WriterNode::SetRef('%s', %s, 0x%lx)\n", file.name, create ? "true" : "false", out_time));
	//	we could support replacing by being a little smarter here...
	if (!create) return B_BAD_VALUE;
	if (m_outputFile) {
		//	flush file if this is a re-target
		m_outputFile->CloseFile();
		delete m_outputFile;
		for (int ix=0; ix<MAX_TRACKS; ix++) {
			m_tracks[ix] = NULL;
		}
	}
	m_entry.SetTo(&file);
	m_outputFile = new BMediaFile(&file, m_fileFormat);	//fixme	hplus: should use real ID
	m_headerCommitted = false;
	m_needHeaderCommit = false;
	status_t err = m_outputFile->InitCheck();
	if (err == B_OK) {
		for (int ix=0; ix<MAX_TRACKS; ix++) {
			if (m_inputs[ix].source != media_source::null) {
				m_tracks[ix] = m_outputFile->CreateTrack(&m_inputs[ix].format);
				if (!m_tracks[ix]) {
					err = B_MEDIA_BAD_FORMAT;
				}
				else {
					m_needHeaderCommit = true;
				}
			}
			else {
				memset(&m_inputs[ix].format, 0, sizeof(media_format));
			}
		}
	}
	//fixme	hplus:	reset media time to 0 and stuff
	return err;
}

status_t 
WriterNode::GetRef(entry_ref *out_ref, char *out_mime_type)
{
	FUNC(("WriterNode::GetRef(0x%lx, 0x%lx)\n", out_ref, out_mime_type));
	status_t err = m_entry.GetRef(out_ref);
	if (err == B_OK && out_mime_type) {
		if (!m_fileFormat || !m_fileFormat->mime_type[0]) {
			strcpy(out_mime_type, "application/octet-stream");
		}
		else {
			strcpy(out_mime_type, m_fileFormat->mime_type);
		}
	}
	return err;
}

