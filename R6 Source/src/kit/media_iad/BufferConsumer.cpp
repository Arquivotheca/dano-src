/*	BufferConsumer.cpp	*/

#include <trinity_p.h>
#include <BufferConsumer.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <BufferProducer.h>
#include <tr_debug.h>
#include <buffer_id_cache.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "port_sync.h"

//	don't delete this, it's meant for developers to see!
#define DIAGNOSTIC fprintf

BBufferConsumer::BBufferConsumer(
	media_type consumer_type) :
	BMediaNode("%ERROR%")
{
	_m_consumer_type = consumer_type;
	AddNodeKind(B_BUFFER_CONSUMER);
	_mBufferCache = new _buffer_id_cache(static_cast<_BMediaRosterP*>(BMediaRoster::Roster()));
}


BBufferConsumer::~BBufferConsumer()
{
	/*** we should break all connections here ***/
	delete _mBufferCache;
}


status_t
BBufferConsumer::AcceptFormat(
	const media_destination & input,
	media_format * format)
{
	return B_ERROR;
}


status_t
BBufferConsumer::GetNextInput(
	int32 * cookie,
	media_input * out_input)
{
	return B_ERROR;
}


void
BBufferConsumer::DisposeInputCookie(
	int32 cookie)
{
}


status_t
BBufferConsumer::HandleMessage(
	int32 message,
	const void * data,
	size_t size)
{
	status_t shouldReturn = B_OK;
	switch (message) {
	case BC_ACCEPT_FORMAT: {
		accept_format_a reply;
		reply.format = ((accept_format_q *)data)->format;
		reply.error = AcceptFormat(((accept_format_q *)data)->input, &reply.format);
		dlog("BC_ACCEPT_FORMAT reply port %d", ((accept_format_q *)data)->reply);
		_write_port_sync(((accept_format_q *)data)->reply, BC_ACCEPT_FORMAT_REPLY, &reply, sizeof(reply));
		} break;
	case BC_ITERATE_INPUTS: {
		iterate_inputs_a reply;
		reply.cookie = ((iterate_inputs_q *)data)->cookie;
		reply.error = GetNextInput(&reply.cookie, &reply.input);
		dlog("BC_ITERATE_INPUTS returns cookie %d error %x", reply.cookie, reply.error);
		if ((reply.error == B_OK) && ((reply.input.node.node < 0) || (reply.input.node.port < 0))) {
			fprintf(stderr, "%s warning: returning bad node from GetNextInput()\n", Name());
			reply.input.node = Node();
		}
		_write_port_sync(((iterate_inputs_q *)data)->reply, BC_ITERATE_INPUTS_REPLY, &reply, sizeof(reply));
		} break;
	case BC_DISPOSE_COOKIE: {
		DisposeInputCookie(((dispose_cookie_q *)data)->cookie);
		} break;
	case BC_HOOKED_UP: {
		hooked_up_a reply;
		reply.input.node = Node();
		reply.input.source = ((hooked_up_q *)data)->input.source;
		reply.input.destination = ((hooked_up_q *)data)->input.destination;
		reply.input.format = ((hooked_up_q *)data)->input.format;
//		reply.input.source_node = ((hooked_up_q *)data)->input.source_node;
		strncpy(reply.input.name, ((hooked_up_q *)data)->input.name, B_MEDIA_NAME_LENGTH);
		reply.error = Connected(((hooked_up_q *)data)->input.source, ((hooked_up_q *)data)->input.destination,
			((hooked_up_q *)data)->input.format, &reply.input);
		if (!dcheck(reply.input.source == ((hooked_up_q *)data)->input.source)) {
			fprintf(stderr, "ERROR: consumer cannot change source field in Connected()\n");
			reply.input.source = ((hooked_up_q *)data)->input.source;
		}
		reply.cookie = ((hooked_up_q *)data)->cookie;
		_write_port_sync(((hooked_up_q *)data)->reply, BC_HOOKED_UP_REPLY, &reply, sizeof(reply));
		} break;
	case M_NODE_DIED: {
		int32 i,cookie = 0;
		media_input mi;
		/*	Dammit!  Well, it turns out nodes don't know the node ids of
			the nodes they are connected to.  So, quick hack: when we get
			this message, check all our input ports. */
		while (GetNextInput(&cookie,&mi) == B_OK) {
			if (mi.source != media_source::null) {
				if (port_count(mi.source.port) < 0) {
					DisposeInputCookie(cookie);
					Disconnected(mi.source, mi.destination);
					cookie = 0;
				};
			};
		};
/*
		node_died_q *q = (node_died_q*)data;
		while (GetNextInput(&cookie,&mi) == B_OK) {
			for (i=0;i<q->nodeCount;i++) {
				if (mi.source.id == q->nodes[i]) {
					Disconnected(mi.source, mi.destination);
					break;
				};
			};
		};
*/
		shouldReturn = B_ERROR;
		} break;
	case BC_BROKEN: {
		Disconnected(((broken_q *)data)->producer, ((broken_q *)data)->where);
		} break;
	case M_BUFFER: {
		if (((media_header *)data)->buffer == -1)	//	small buffer?
		{
			BufferReceived(new(const_cast<void *>(data)) BBuffer((media_header *)data));
		}
		else
		{
			BBuffer * buf = _mBufferCache->FindBuffer(((media_header *)data)->buffer);
	//		BBuffer * buf = ((_BMediaRosterP *)BMediaRoster::Roster())->
	//			FindBuffer(((media_header *)data)->buffer);
#if DEBUG
			if (buf->ID() != ((media_header *)data)->buffer) {
				dlog("ERROR: FindBuffer(%d) returns %d", 
					((media_header *)data)->buffer, buf->ID());
			}
#endif
			if (buf) {
				buf->SetHeader((media_header *)data);
				debug_got_buf(((media_header *)data)->buffer);
				BufferReceived(buf);
			}
			else {
				dlog("Unknown buffer %d in BBufferConsumer::M_BUFFER\n", 
					((media_header *)data)->buffer);
			}
		}
		} break;
	case BC_DATA_STATUS: {
		ProducerDataStatus(
			((data_status_q *)data)->whom, 
			((data_status_q *)data)->status,
			((data_status_q *)data)->at_time);
		} break;
	case BC_GET_LATENCY: {
		get_latency_a ans;
		ans.error = GetLatencyFor(
			((get_latency_q *)data)->destination,
			&ans.latency,
			&ans.timesource);
		write_port(((get_latency_q *)data)->reply, BC_GET_LATENCY_REPLY, &ans, sizeof(ans));
		} break;
	case BC_CHANGE_FORMAT: {
		change_format_a ans;
		change_format_q & q(*(change_format_q *)data);
		int32 change_tag = NewChangeTag();
		ans.error = FormatChanged(q.output, q.input, change_tag, q.format);
		ans.change_tag = change_tag;
		write_port(((change_format_q *)data)->reply, BC_CHANGE_FORMAT_REPLY, &ans, sizeof(ans));
		} break;
	case BC_FIND_SEEK_TAG: {
		find_seek_tag_a ans;
		find_seek_tag_q & q(*(find_seek_tag_q *)data);
		ans.cookie = q.cookie;
		ans.error = SeekTagRequested(q.destination, q.in_time, q.in_flags,
			&ans.tag, &ans.out_time, &ans.out_flags);
		write_port(q.reply, BC_FIND_SEEK_TAG_REPLY, &ans, sizeof(ans));
		} break;
	default:
#if DEBUG
		if ((message & 0xffffff00) == 0x40000200L) {
			fprintf(stderr, "BBufferConsumer: unknown message code %d\n", message);
			abort();
		}
#endif
		shouldReturn = B_ERROR;
	}
	return shouldReturn;
}


void
BBufferConsumer::BufferReceived(
	BBuffer * buffer)
{
}


status_t
BBufferConsumer::Connected(
	const media_source & producer,
	const media_destination & where,
	const media_format & with_format,
	media_input * out_input)
{
	return B_ERROR;
}


void
BBufferConsumer::Disconnected(
	const media_source & producer,
	const media_destination & where)
{
	/* shyeah, right! */
}


status_t
BBufferConsumer::RegionToClipData(
	const BRegion * region,
	int32 * format,
	int32 * ioSize,
	void * data)
{
	int cnt;
	status_t err = BBufferProducer::clip_region_to_shorts(region, (int16 *)data, *ioSize/2, &cnt);
	if (err == B_OK)
	{
		*ioSize = 2*cnt;
		*format = BBufferProducer::B_CLIP_SHORT_RUNS;
	}
	return err;
}


void
BBufferConsumer::NotifyLateProducer(
	const media_source & for_output,
	bigtime_t how_much,
	bigtime_t performance_time)
{
	youre_late_q cmd;
	cmd.how_much = how_much;
	cmd.source = for_output;
	cmd.performance_time = performance_time;
	write_port(for_output.port, BP_YOURE_LATE, &cmd, sizeof(cmd));
}



status_t
BBufferConsumer::SetVideoClippingFor(		//	new in 4.1
	const media_source & output,
	const media_destination & destination, 
	const int16 * shorts,
	int32 short_count,
	const media_video_display_info & display,
	void * user_data,
	int32 * change_tag,
	void * _reserved_)
{
	video_clip_q cmd;
	status_t err = B_OK;

	if (short_count*sizeof(int16) > sizeof(cmd.data)) {
		return B_MEDIA_BAD_CLIP_FORMAT;
	}
	memcpy(cmd.data, shorts, short_count*sizeof(int16));
	port_id port = -1;

	cmd.user_data = user_data;
	*change_tag = NewChangeTag();
	cmd.cookie = *change_tag;
	cmd.source = output;
	cmd.destination = destination;
	cmd.display = display;
	cmd.format = BBufferProducer::B_CLIP_SHORT_RUNS;
	cmd.size = short_count*sizeof(int16);
	err = write_port(output.port, BP_VIDEO_CLIP, &cmd, sizeof(cmd)-sizeof(cmd.data)+cmd.size);
	if (err > 0) {
		err = 0;
	}
	return err;
}



status_t
BBufferConsumer::SetOutputEnabled(				//	new in 4.1
	const media_source & source,
	const media_destination & destination,
	bool enabled,
	void * user_data,
	int32 * change_tag,
	void * _reserved_)
{
	if (source.port < 0) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (change_tag == 0) {
		return B_BAD_VALUE;
	}
	mute_output_q cmd;
	cmd.user_data = user_data;
	*change_tag = NewChangeTag();
	cmd.cookie = *change_tag;
	cmd.source = source;
	cmd.destination = destination;
	cmd.muted = !enabled;
	status_t err = write_port(source.port, BP_MUTE_OUTPUT, &cmd, sizeof(cmd));
	return err >= B_OK ? B_OK : err;
}



status_t
BBufferConsumer::RequestFormatChange(			//	new in 4.1
	const media_source & source,
	const media_destination & destination,
	const media_format & to_format,
	void * user_data, 
	int32 * change_tag,
	void * _reserved_)
{
	if (to_format.MetaData() != 0) {
		DIAGNOSTIC(stderr, "RequestFormatChange() cannot take formats with meta data.\n");
		return B_MEDIA_BAD_FORMAT;
	}
	if (source.port < 0) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (destination.port < 0) {
		return B_MEDIA_BAD_DESTINATION;
	}
	request_format_change_q cmd;
	cmd.user_data = user_data;
	*change_tag = NewChangeTag();
	cmd.cookie = *change_tag;
	cmd.source = source;
	cmd.destination = destination;
	cmd.format = to_format;
	status_t err = write_port(source.port, BP_REQUEST_FORMAT_CHANGE, &cmd, sizeof(cmd));
	return err >= B_OK ? B_OK : err;
}


status_t
BBufferConsumer::RequestAdditionalBuffer(
	const media_source & source,
	BBuffer * prev_buffer,
	void *)
{
	if (source.port < 0)
		return B_MEDIA_BAD_SOURCE;
	if (prev_buffer == NULL)
		return B_BAD_VALUE;

	request_buffer_q cmd;
	cmd.source = source;
	media_header & hdr = *prev_buffer->Header();
	cmd.prev_buffer = hdr.buffer;
	cmd.prev_time = hdr.start_time;
	if (hdr.user_data_type == media_header::B_SEEK_TAG)
	{
		cmd.prev_tag = *((media_seek_tag *)hdr.user_data);
		cmd.flags = request_buffer_q::B_TAG_VALID;
	}
	else
		cmd.flags = 0;

	status_t err = write_port_etc(source.port, BP_REQUEST_BUFFER, &cmd, sizeof(cmd),
			B_TIMEOUT, DEFAULT_TIMEOUT);
	return (err > B_OK) ? B_OK : err;
}


status_t
BBufferConsumer::RequestAdditionalBuffer(
	const media_source & source,
	bigtime_t start_time,
	void *)
{
	if (source.port < 0)
		return B_MEDIA_BAD_SOURCE;

	request_buffer_q cmd;
	cmd.source = source;
	cmd.prev_buffer = -1;
	cmd.prev_time = start_time;
	cmd.flags = 0;

	status_t err = write_port_etc(source.port, BP_REQUEST_BUFFER, &cmd, sizeof(cmd),
			B_TIMEOUT, DEFAULT_TIMEOUT);
	return (err > B_OK) ? B_OK : err;
}


status_t
BBufferConsumer::FormatChanged(
	const media_source &,
	const media_destination &,
	int32 from_change_count,
	const media_format &)
{
	return B_ERROR;
}


media_type
BBufferConsumer::ConsumerType()
{
	return _m_consumer_type;
}


status_t
BBufferConsumer::SeekTagRequested(
	const media_destination & destination,
	bigtime_t in_previous_time,
	uint32 in_flags, 
	media_seek_tag * out_seek_tag,
	bigtime_t * out_tagged_time,
	uint32 * out_flags)
{
	if (out_tagged_time) *out_tagged_time = 0;
	if (out_flags) *out_flags = 0;
	return B_ERROR;
}


status_t
BBufferConsumer::SetOutputBuffersFor(
	const media_source & source,
	const media_destination & destination,
	BBufferGroup * group,
	void * user_data,
	int32 * change_tag, 			//	passed to RequestCompleted()
	bool will_reclaim,
	void * _reserved_)
{
	*change_tag = NewChangeTag();
	_BMediaRosterP * r = (_BMediaRosterP *)BMediaRoster::CurrentRoster();
	if (r == NULL) return B_NO_INIT;
	if (group != 0) group->SetOwnerPort(source.port);
	status_t err = r->_SetOutputBuffersImp_(source, &destination, user_data, *change_tag, group, will_reclaim);
	if (err < B_OK) {
		if (group != 0) group->SetOwnerPort(destination.port);
	}
	return err;
}


status_t
BBufferConsumer::SendLatencyChange(
	const media_source & source,
	const media_destination & destination,
	bigtime_t my_new_latency,
	uint32 flags)
{
	if (source.port < 0)
		return B_MEDIA_BAD_SOURCE;
	if (destination.port < 0)
		return B_MEDIA_BAD_DESTINATION;

	latency_changed_q cmd;
	cmd.source = source;
	cmd.destination = destination;
	cmd.new_latency = my_new_latency;

	status_t err = write_port_etc(source.port, BP_LATENCY_CHANGED, &cmd, sizeof(cmd),
			B_TIMEOUT, DEFAULT_TIMEOUT);
	return (err > B_OK) ? B_OK : err;
}


		/* Mmmh, stuffing! */
status_t
BBufferConsumer::_Reserved_BufferConsumer_0(void *)
{
	/*fixme	we should re-route this to call SeekTagRequested */
	return B_ERROR;				/* no longer virtual */
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_1(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_2(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_3(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_4(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_5(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_6(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_7(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_8(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_9(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_10(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_11(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_12(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_13(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_14(void *)
{
	return B_ERROR;
}

status_t
BBufferConsumer::_Reserved_BufferConsumer_15(void *)
{
	return B_ERROR;
}




status_t
BBufferConsumer::SetVideoClippingFor(		//	deprecated static version
	const media_source & output,
	const int16 * shorts,
	int32 short_count,
	const media_video_display_info & display,
	int32 * change_tag)
{
	video_clip_q cmd;
	status_t err = B_OK;

	if (short_count*sizeof(int16) > sizeof(cmd.data)) {
		return B_MEDIA_BAD_CLIP_FORMAT;
	}
	memcpy(cmd.data, shorts, short_count*sizeof(int16));
	cmd.user_data = NULL;
	*change_tag = NewChangeTag();
	cmd.cookie = *change_tag;
	cmd.user_data = NULL;
	cmd.source = output;
	cmd.destination.port = -1;
	cmd.display = display;
	cmd.format = BBufferProducer::B_CLIP_SHORT_RUNS;
	cmd.size = short_count*sizeof(int16);
	err = write_port(output.port, BP_VIDEO_CLIP, &cmd, sizeof(cmd)-sizeof(cmd.data)+cmd.size);
	if (err > 0) {
		err = 0;
	}
	return err;
}



status_t
BBufferConsumer::SetOutputEnabled(			//	deprecated static version
	const media_source & source,
	bool enabled,
	int32 * change_tag)
{
	if (source.port < 0) {
		return B_MEDIA_BAD_SOURCE;
	}
	mute_output_q cmd;
	*change_tag = NewChangeTag();
	cmd.cookie = *change_tag;
	cmd.user_data = NULL;
	cmd.source = source;
	cmd.destination.port = -1;
	cmd.muted = !enabled;
	status_t err = write_port(source.port, BP_MUTE_OUTPUT, &cmd, sizeof(cmd));
	return err >= B_OK ? B_OK : err;
}



status_t
BBufferConsumer::RequestFormatChange(		//	deprecated static version
	const media_source & source,
	const media_destination & destination,
	media_format * io_format,
	int32 * change_tag)
{
	if (io_format->MetaData() != 0) {
		//	no meta-data can travel backwards
		DIAGNOSTIC(stderr, "RequestFormatChange() cannot take formats with meta data.\n");
		return B_MEDIA_BAD_FORMAT;
	}
	if (source.port < 0) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (destination.port < 0) {
		return B_MEDIA_BAD_DESTINATION;
	}
	request_format_change_q cmd;
	*change_tag = NewChangeTag();
	cmd.cookie = *change_tag;
	cmd.user_data = NULL;
	cmd.source = source;
	cmd.destination = destination;
	cmd.format = *io_format;
	status_t err = write_port(source.port, BP_REQUEST_FORMAT_CHANGE, &cmd, sizeof(cmd));
	return err >= B_OK ? B_OK : err;
}
