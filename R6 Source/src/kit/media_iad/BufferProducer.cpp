/*	BufferProducer.cpp	*/

#include "trinity_p.h"
#include "BufferProducer.h"
#include "BufferGroup.h"
#include "Buffer.h"
#include "tr_debug.h"
#include "BufferConsumer.h"	/* for producer status */

#include <Region.h>
#include <stdlib.h>
#include <set>
#include <string.h>
#include <stdio.h>
#include "port_sync.h"

#define FPrintf fprintf


/* There are two strategies for buffer producers. */
/* The first is a filter. It is entirely driven from BufferReceived(). */
/* If it gets a SetBufferGroup() call, it will just pass that on to */
/* whomever is before it in the chain. */
/* The second is something that actually produces its own buffers. It */
/* will need a buffer group to get buffers from. When started, it should */
/* create a buffer group for its own use if it hasn't already received a */
/* SetBufferGroup() call. */


BBufferProducer::BBufferProducer(
	media_type producer_type) :
	BMediaNode("%ERROR%")
{
	_m_producer_type = producer_type;
	_m_change = 0;
	_m_recordDelay = 0;
	_m_initialLatency = 0;
	_m_initialFlags = 0;
	AddNodeKind(B_BUFFER_PRODUCER);
}


status_t
BBufferProducer::FormatSuggestionRequested(
	media_type type,
	int32 quality,
	media_format * format)
{
	return B_ERROR;
}


status_t
BBufferProducer::GetNextOutput(	/* cookie starts as 0 */
	int32 * cookie,
	media_output * out_output)
{
	return B_ERROR;
}


status_t
BBufferProducer::DisposeOutputCookie(
	int32 cookie)
{
	return B_ERROR;
}


BBufferProducer::~BBufferProducer()
{
	/*** we should break all connections here ***/
}


/* if we had RPC or RMI, this would be a snap! */
status_t
BBufferProducer::HandleMessage(	/* call this from the thread that listens to the port */
	int32 message,
	const void * data,
	size_t size)
{
	status_t shouldReturn = B_OK;
	switch (message) {
	case BP_SUGGEST_FORMAT: {
		suggest_format_a reply;
		reply.format = ((suggest_format_q *)data)->format;
		reply.error = FormatSuggestionRequested(
			((suggest_format_q *)data)->type, 
			((suggest_format_q *)data)->quality, 
			&reply.format);
		_write_port_sync(((suggest_format_q *)data)->reply, 
			BP_SUGGEST_FORMAT_REPLY, &reply, sizeof(reply));
		} break;
	case BP_PROPOSE_FORMAT: {
		propose_format_a reply;
		reply.format = ((propose_format_q *)data)->format;
		reply.error = FormatProposal(
			((propose_format_q *)data)->output, 
			&reply.format);
		reply.cookie = ((propose_format_q *)data)->cookie;
		_write_port_sync(((propose_format_q *)data)->reply,
			BP_PROPOSE_FORMAT_REPLY, &reply, sizeof(reply));
		} break;
	case BP_ITERATE_OUTPUTS: {
		iterate_outputs_a reply;
		reply.cookie = ((iterate_outputs_q *)data)->cookie;
		reply.error = GetNextOutput(&reply.cookie, &reply.output);
		dlog("BP_ITERATE_OUTPUTS returns cookie %d error %x", reply.cookie, reply.error);
		if ((reply.error == B_OK) && ((reply.output.node.node < 0) || (reply.output.node.port < 0))) {
			fprintf(stderr, "%s warning: returning bad node from GetNextOutput()\n", Name());
			reply.output.node = Node();
		}
		_write_port_sync(((iterate_outputs_q *)data)->reply, 
			BP_ITERATE_OUTPUTS_REPLY, &reply, sizeof(reply));
		} break;
	case BP_DISPOSE_COOKIE: {
		DisposeOutputCookie(((dispose_cookie_q *)data)->cookie);
		} break;
	case BP_PREPARE_CONNECTION: {
		prepare_connection_a reply;
		reply.cookie = ((prepare_connection_q *)data)->cookie;
		reply.output.format = ((prepare_connection_q *)data)->format;
		reply.output.source = ((prepare_connection_q *)data)->from;
		reply.output.destination = ((prepare_connection_q *)data)->where;
		reply.output.name[0] = 0;
		reply.output.node = Node();
//		reply.output.destination_node = ((prepare_connection_q *)data)->destination_node;
		reply.error = PrepareToConnect(((prepare_connection_q *)data)->from,
			((prepare_connection_q *)data)->where, 
			&reply.output.format, &reply.output.source, reply.output.name);
		_write_port_sync(((prepare_connection_q *)data)->reply, BP_PREPARE_CONNECTION_REPLY,
			&reply, sizeof(reply));
		} break;
	case BP_HOOKUP: {
		hookup_a reply;
		reply.cookie = ((hookup_q *)data)->cookie;
		reply.output.node = Node();
		reply.output.source = ((hookup_q *)data)->output.source;
		reply.output.destination = ((hookup_q *)data)->output.destination;
		reply.output.format = ((hookup_q *)data)->output.format;
//		reply.output.destination_node = ((hookup_q *)data)->output.destination_node;
		strncpy(reply.output.name, ((hookup_q *)data)->output.name, B_MEDIA_NAME_LENGTH);
		Connect(((hookup_q *)data)->status, reply.output.source, reply.output.destination, 
			reply.output.format, reply.output.name);
		if (((hookup_q *)data)->flags & BMediaRoster::B_CONNECT_MUTED) {
			int32 dummy = 0;
			EnableOutput(reply.output.source, false, &dummy);
		}
		_write_port_sync(((hookup_q *)data)->reply, BP_HOOKUP_REPLY,
			&reply, sizeof(reply));
		} break;
	case M_NODE_DIED: {
		int32 i,cookie = 0;
		media_output mo;
		/*	Dammit!  Well, it turns out nodes don't know the node ids of
			the nodes they are connected to.  So, quick hack: when we get
			this message, check all our output ports. */
		while (GetNextOutput(&cookie,&mo) == B_OK) {
			if (mo.destination != media_destination::null) {
				if (port_count(mo.destination.port) < 0) {
					DisposeOutputCookie(cookie);
					Disconnect(mo.source, mo.destination);
					cookie = 0;
				};
			};
		};
/*
		node_died_q *q = (node_died_q*)data;
		while (GetNextOutput(&cookie,&mo) == B_OK) {
			for (i=0;i<q->nodeCount;i++) {
				if (mo.destination.id == q->nodes[i]) {
					Disconnect(mo.source, mo.destination);
					break;
				};
			};
		};
*/
		shouldReturn = B_ERROR;
		} break;
	case BP_BREAK: {
		Disconnect(((break_q *)data)->from, ((break_q *)data)->where);
		if (((break_q *)data)->reply >= 0) {
			break_a ans;
			ans.error = B_OK;
			ans.cookie = ((break_q *)data)->cookie;
			write_port(((break_q *)data)->reply, BP_BREAK_REPLY, &ans, sizeof(ans));
		}
		} break;
	case BP_SET_BUFFERS: {
		status_t error = B_OK;
		BBufferGroup * grp = NULL;
		set_buffers_q & req(*(set_buffers_q *)data);
		if (req.buffers[0] != -1)
		{
			grp = new BBufferGroup(req.count, req.buffers);
			error = grp->InitCheck();
			if (error == B_OK)
				grp->SetOwnerPort(ControlPort());
		}
		if (error == B_OK)
			error = SetBufferGroup(req.where, grp);
		if (error != B_OK)
			delete grp;
		if (req.destination.port >= 0)
		{
			media_request_info ans;
			ans.what = media_request_info::B_SET_OUTPUT_BUFFERS_FOR;
			ans.status = error;
			ans.cookie = req.cookie;
			ans.change_tag = req.change_tag;
			ans.user_data = req.user_data;
			ans.source = req.where;
			ans.destination = req.destination;
			(void)SendRequestResult(ans, ans.destination.port);
		}
		//	else it was a call through the 4.0 API, so don't worry about it
		} break;
	case BP_YOURE_LATE: {
		LateNoticeReceived(((youre_late_q *)data)->source, ((youre_late_q *)data)->how_much, 
			((youre_late_q *)data)->performance_time);
		} break;
	case BP_CALC_TOTAL_LATENCY: {
		calc_total_latency_a ans;
		ans.latency = 0;
		ans.error = GetLatency(&ans.latency);
		ans.cookie = ((calc_total_latency_q *)data)->cookie;
		write_port(((calc_total_latency_q *)data)->reply, BP_CALC_TOTAL_LATENCY_REPLY, 
			&ans, sizeof(ans));
		} break;
	case BP_REQUEST_FORMAT_CHANGE: {
		media_request_info ans;
		ans.what = media_request_info::B_REQUEST_FORMAT_CHANGE;
		request_format_change_q & req(*(request_format_change_q *)data);
		ans.format = req.format;
		ans.cookie = 0;
		ans.status = FormatChangeRequested(
				req.source,
				req.destination,
				&ans.format,
				&ans.cookie);
		ans.source = req.source;
		ans.destination = req.destination;
		ans.user_data = req.user_data;
		ans.change_tag = req.cookie;
		(void)SendRequestResult(ans, ans.destination.port, true);
		} break;
	case BP_MUTE_OUTPUT: {
		int32 dummy = 0;
		mute_output_q & req(*(mute_output_q *)data);
		EnableOutput(((mute_output_q *)data)->source, !req.muted, &dummy);
		//	iterate through outputs to find the one to send OK to
		int32 cookie = 0;
		media_output output;
		bool found = false;
		if (req.destination.port >= 0)
		{
			output.source = req.source;
			output.destination = req.destination;
			found = true;
		}
		if (!found)
		{
			while (!GetNextOutput(&cookie, &output))
				if ((found = (output.source == ((mute_output_q *)data)->source)) != false)
					break;
			DisposeOutputCookie(cookie);
		}
		if (found)
		{
			media_request_info ans;
			ans.what = media_request_info::B_SET_OUTPUT_ENABLED;
			ans.user_data = req.user_data;
			ans.change_tag = req.cookie;
			ans.status = B_OK;
			ans.cookie = dummy;
			ans.source = output.source;
			ans.destination = output.destination;
			SendRequestResult(ans, ans.destination.port);
		}
		else
		{
			fprintf(stderr, "%s: EnableOutput() called for nonexistent output!\n", Name());
		}
		} break;
	case BP_SET_RATE: {
		set_rate_a ans;
		ans.cookie = ((set_rate_q *)data)->cookie;
		ans.error = SetPlayRate(((set_rate_q *)data)->numer, ((set_rate_q *)data)->denom);
		write_port(((set_rate_q *)data)->reply, BP_SET_RATE_REPLY, &ans, sizeof(ans));
		} break;
	case BP_REQUEST_BUFFER: {
		request_buffer_q & req(*(request_buffer_q *)data);
		AdditionalBufferRequested(req.source, req.prev_buffer, req.prev_time,
			(req.flags & request_buffer_q::B_TAG_VALID) ? &req.prev_tag : NULL);
		} break;
	case BP_LATENCY_CHANGED: {
		latency_changed_q & req(*(latency_changed_q *)data);
		LatencyChanged(req.source, req.destination, req.new_latency, req.flags);
		} break;
	case BP_VIDEO_CLIP: {
		media_request_info reply;
		reply.what = media_request_info::B_SET_VIDEO_CLIPPING_FOR;
		video_clip_q & req(*(video_clip_q *)data);
		reply.cookie = 0;
		reply.status = VideoClippingChanged(
				req.source, 
				req.size/sizeof(int16), 
				(int16 *)req.data,
				req.display,
				&reply.cookie);
		reply.user_data = req.user_data;
		reply.source = req.source;
		reply.change_tag = req.cookie;
		//	find the destination to send to...
		int32 cookie = 0;
		bool found = false;
		media_output output;
		if (req.destination.port >= 0)
		{
			found = true;
			output.destination = req.destination;
			output.format.u.raw_video.display = req.display;
		}
		if (!found)
		{
			while (!GetNextOutput(&cookie, &output))
				if ((found = (output.source == reply.source)) != false)
					break;
			DisposeOutputCookie(cookie);
		}
		if (found)
		{
			reply.destination = output.destination;
			reply.format = output.format;
			(void)SendRequestResult(reply, output.destination.port);
		}
		else
		{
			fprintf(stderr, "%s: VideoClippingChanged() called with an invalid output (%d:%d)\n",
				Name(), reply.source.port, reply.source.id);
		}
		} break;
	case BP_GET_INITIAL_LATENCY: {
			get_initial_latency_q & q(*(get_initial_latency_q *)data);
			get_initial_latency_a ans;
			ans.error = B_OK;
			ans.cookie = q.cookie;
			ans.latency = _m_initialLatency;
			ans.flags = _m_initialFlags;
			(void)write_port_etc(q.reply, BP_GET_INITIAL_LATENCY_REPLY, &ans, sizeof(ans),
					B_TIMEOUT, DEFAULT_TIMEOUT);
		}
		break;
	default:
#if DEBUG
		if ((message & 0xffffff00) == 0x40000100L) {
			fprintf(stderr, "BBufferProducer: unknown message code %d\n", message);
			abort();
		}
#endif
		shouldReturn = B_ERROR;
		break;
	}
	return shouldReturn;
}


status_t
BBufferProducer::PrepareToConnect(
	const media_source & from, 
	const media_destination & to,
	media_format * format,
	media_source * out_source,
	char * out_name)
{
	return B_ERROR;
}

void
BBufferProducer::Connect(
	status_t status,
	const media_source & from,
	const media_destination & where,
	const media_format & format,
	char * io_name)
{
}


void
BBufferProducer::Disconnect(
	const media_source & from,
	const media_destination & where)
{
	/* whatever */
}


void
BBufferProducer::LateNoticeReceived(
	const media_source & what,
	bigtime_t how_much,
	bigtime_t performance_time)
{
	/* whatever */
}


void
BBufferProducer::AdditionalBufferRequested(			//	used to be Reserved 0
	const media_source & source,
	media_buffer_id prev_buffer,
	bigtime_t prev_time,
	const media_seek_tag * prev_tag)	//	may be NULL
{
	//	so what?
}


status_t
BBufferProducer::SendBuffer(
	BBuffer * buffer,
	const media_destination & destination)
{
	media_header * hdr = buffer->Header();
	hdr->buffer = buffer->ID();
	/* Test that the size/offset values are consistent. */
	if (hdr->size_used > buffer->Size())
		return B_MEDIA_BAD_BUFFER;
	/* SendBuffer() is OK to enter multiple times -- here, you'll get a bogus error if */
	/* entering SendBuffer() 1000 times at the same time, so don't :-) */
	if (atomic_add((int32 *)&_m_change, 1) > 999) {	/* mutual exclusion with ChangeFormat() */
		atomic_add((int32 *)&_m_change, -1);
		FPrintf(stderr, "SendBuffer() CHANGE_IN_PROGRESS");
		return B_MEDIA_CHANGE_IN_PROGRESS;
	}
	hdr->destination = destination.id;
	hdr->start_time += _m_recordDelay;
	if (hdr->time_source <= 0) {	//	if this is a pass-through buffers, don't smash it
		hdr->time_source = _mTimeSourceID;
	}
//	hdr->change_tag = ChangeTag();
	debug_sent_buf(hdr->buffer);
	buffer->SetGroupOwnerPort(ControlPort());
//	mark the buffer as currently "owned" (used by) the destination
	buffer->SetCurrentOwner(destination.port);
	status_t err = write_port(destination.port, M_BUFFER, hdr, sizeof(*hdr));
//	hdr->time_source = 0;
//	dlog("SendBuffer(%d, %d) returns %x", hdr->buffer, destination.port, err);
	atomic_add((int32 *)&_m_change, -1);
	return err > B_OK ? B_OK : err;
}


status_t
BBufferProducer::SendDataStatus(
	int32 status,
	const media_destination & destination,
	bigtime_t at_time)
{
	data_status_q cmd;
	cmd.whom = destination;
	cmd.status = status;
	cmd.at_time = at_time;
	status_t err = write_port(destination.port, BC_DATA_STATUS, &cmd, sizeof(cmd));
	return (err > B_OK) ? B_OK : err;
}


status_t
BBufferProducer::ProposeFormatChange(
	media_format * format,
	const media_destination & destination)
{
	accept_format_q cmd;
	port_id port = create_port(1, "ProposeFormatChange reply");
	if (port < B_OK) {
		return port;
	}
	cmd.reply = port;
	cmd.input = destination;
	cmd.format = *format;
	status_t error = write_port(destination.port, BC_ACCEPT_FORMAT, &cmd, sizeof(cmd));
	if (error >= B_OK) {
		int32 code;
		accept_format_a reply;
		error = read_port_etc(port, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT);
		if (error >= B_OK) {
			error = reply.error;
			*format = reply.format;
		}
	}
	delete_port(port);
	return error;
}


status_t
BBufferProducer::ChangeFormat(
	const media_source & source,
	const media_destination & destination,
	media_format * format)
{
	change_format_q cmd;
	int tries = 0;
	if (atomic_add((int32 *)&_m_change, 1000) > 0) {	/*	Mutual exclusion with SendBuffer()	*/
		atomic_add((int32 *)&_m_change, -1000);
		fprintf(stderr, "WARNING: ChangeFormat() and SendBuffer() clash detected\n");
		return B_MEDIA_CHANGE_IN_PROGRESS;
	}
	port_id port = create_port(1, "ChangeFormat reply");
	if (port < B_OK) {
		atomic_add((int32 *)&_m_change, -1000);
		return port;
	}
	cmd.reply = port;
	cmd.output = source;
	cmd.input = destination;
	cmd.format = *format;
//	cmd.change_tag = IncrementChangeTag();
	status_t error = write_port_etc(destination.port, BC_CHANGE_FORMAT, &cmd, sizeof(cmd), 
		B_TIMEOUT, DEFAULT_TIMEOUT);
	change_format_a reply = { -1, -1 };
	if (error >= B_OK) {
		int32 code;
again:
		error = read_port_etc(port, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT);
		if (error >= B_OK) {
			if (code != BC_CHANGE_FORMAT_REPLY) {
				dlog("Stale reply to BC_CHANGE_FORMAT; re-trying\n");
				goto again;
			}
//			dlog("ChangeFormat read_port() %x error %x\n", error, reply.error);
			error = reply.error;
		}
	}
	//	send information about this exchange to destination
	{
		media_request_info comp;
		comp.what = media_request_info::B_FORMAT_CHANGED;
		comp.change_tag = reply.change_tag;
		comp.status = error;
		comp.user_data = NULL;
		comp.cookie = 0;
		comp.source = source;
		comp.destination = destination;
		comp.format = *format;
		(void)SendRequestResult(comp, destination.port);
	}
	if (error >= B_OK) {
		error = B_OK;
		(void)((_BMediaRosterP *)BMediaRoster::_sDefault)->MediaFormatChanged(source, destination, *format);
	}
	delete_port(port);
	atomic_add((int32 *)&_m_change, -1000);
	return error;
}


status_t
BBufferProducer::FindLatencyFor(
	const media_destination & destination,
	bigtime_t * out_latency,
	media_node_id * out_node)
{
	if (!dcheck(destination.port > 0)) return B_MEDIA_BAD_DESTINATION;
	get_latency_q cmd;
	cmd.reply = create_port(1, "GetLatencyFor reply");
	cmd.cookie = find_thread(NULL);
	cmd.destination = destination;
	status_t err = write_port(destination.port, BC_GET_LATENCY, &cmd, sizeof(cmd));
	if (err < B_OK) {
		delete_port(cmd.reply);
		return err;
	}
	int32 code;
	get_latency_a ans;
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		*out_latency = ans.latency;
		*out_node = ans.timesource;
		err = ans.error;
	}
	delete_port(cmd.reply);
	return err;
}


status_t
BBufferProducer::FindSeekTag(
	const media_destination & for_destination,
	bigtime_t in_previous_time,
	media_seek_tag * out_tag,
	bigtime_t * out_tagged_time,
	uint32 * out_flags,
	uint32 in_flags)
{
	find_seek_tag_q cmd;
	find_seek_tag_a ans;
	cmd.reply = create_port(1, "FindSeekTag reply");
	cmd.cookie = find_thread(NULL);
	cmd.destination = for_destination;
	cmd.in_time = in_previous_time;
	cmd.in_flags = in_flags;
	status_t err = write_port_etc(for_destination.port, BC_FIND_SEEK_TAG, &cmd, sizeof(cmd),
		B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK)
	{
		delete_port(cmd.reply);
		return err;
	}
	int32 code = 0;
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	delete_port(cmd.reply);
	if (err < B_OK)
		return err;
	if (out_tag) *out_tag = ans.tag;
	if (out_tagged_time) *out_tagged_time = ans.out_time;
	if (out_flags) *out_flags = ans.out_flags;
	return B_OK;
}


static status_t
emit_rects(
	BRect * rects,
	int count,
	int bottom,
	BRegion * target)
{
	for (int ix=0; ix<count; ix++)
	{
		rects[ix].bottom = bottom;
		target->Include(rects[ix]);
	}
	return B_OK;
}


/*** possible optimization: change format to mean negative == reproduce previous line X times ***/
/*** re-code this to use STL to avoid specific limitation ***/
#define MAX_RECTS_PER_LINE 32

status_t
BBufferProducer::clip_shorts_to_region(
	int16 * data,
	int count,
	BRegion * output)
{
	BRect active_rects[MAX_RECTS_PER_LINE];
	int active_count = 0;
	int line = 0;
	status_t err = B_OK;
	int c_cnt;

	/* support offset */
	if (count < 2) return B_BAD_VALUE;
	int16 off_x = *(data++);
	int16 off_y = *(data++);
	count -= 2;

	while ((count > 0) && !err) {
		/* -1 means "extend previous strips for this line" */
		if (*data < 0) {
			/* handle negative count -- repeat what's already there */
			line -= *data;
			count--;
			data++;
			continue;
		}
		if ((active_count == 0) && (*data == 0)) {
			/* another blank line? */
			line++;
			count--;
			data++;
			continue;
		}
		/* Since we're going to be using some other set of horizontal strips, emit the current set */
		err = emit_rects(active_rects, active_count, line-1, output);
		active_count = *(data++)/2; count--;
		if (count < active_count*2) {
			err = B_BAD_VALUE;
			break;
		}
		if (active_count > MAX_RECTS_PER_LINE) {
			err = B_NO_MEMORY;	/* B_REGION_TOO_COMPLEX */
			break;
		}
		/* gather new set of horizontal stripes */
		for (c_cnt = 0; c_cnt < active_count; c_cnt++) {
			active_rects[c_cnt].top = line;
			active_rects[c_cnt].left = *(data++); count--;
			active_rects[c_cnt].right = *(data++); count--;
		}
		line++;
	}
	if (active_count && !err) {
		err = emit_rects(active_rects, active_count, line-1, output);
	}
	output->OffsetBy(off_x, off_y);
	return err;
}


class left_sort_rect
{
public:
	inline bool operator()(const BRect *a, const BRect *b) const
		{
			return a->left < b->left;
		}
};

class top_sort_rect
{
public:
	inline bool operator()(const BRect *a, const BRect *b)
		{
			return a->top < b->top;
		}
};

class bottom_sort_rect
{
public:
	inline bool operator()(const BRect *a, const BRect *b)
		{
			return a->bottom < b->bottom;
		}
};

/* error checking is done before this function is entered */
static void
emit_shorts(
	multiset<BRect*,left_sort_rect> & active,
	int16 *& data,
	int & used)
{
	if (!active.size())	/* deal with special case */
	{
		*data = 0;
		data++;
		used += 1;
		return;
	}
	multiset<BRect*,left_sort_rect>::iterator ptr(active.begin());
	int where = 1;
	while (ptr != active.end())
	{
		float left = (*ptr)->left;
		float right = (*ptr)->right;
		ptr++;
		/* merge overlapping rects -- abutting rects overlap, too (and coordinates are inclusive) */
		while (ptr != active.end() && (*ptr)->left <= right+1)
		{
			if ((*ptr)->right > right)
				right = (*ptr)->right;
			ptr++;
		}
		data[where++] = (int16)left;
		data[where++] = (int16)right;
	}
	data[0] = where-1;
	used += where;
	data += where;
}


status_t
BBufferProducer::clip_region_to_shorts(
	const BRegion * input,
	int16 * data,
	int max_count,
	int * out_count)
{
	if (max_count < 3) return B_NO_MEMORY;
	BRegion region(*input);	/* because input is "const" and CountRects() isn't */
	status_t err = B_OK;
	BRect * rects = new BRect[region.CountRects()];
	{
		int used = 0;
		multiset<BRect*,top_sort_rect> tops;
		multiset<BRect*,bottom_sort_rect> bottoms;
		multiset<BRect*,left_sort_rect> active; /* sorted on left */
		int line = 0;
		int16 * olddata = NULL;
	
		int off_y = (int)input->Frame().top;
		int off_x = (int)input->Frame().left;
		/* offset */
		data[0] = off_x;
		data[1] = off_y;
		data += 2;
		used += 2;

		/* rip through the region, inserting rects sorted on top and bottom */
		for (int ix=0; ix<region.CountRects(); ix++) {
			rects[ix] = region.RectAt(ix);
			rects[ix].OffsetBy(-off_x, -off_y);
			tops.insert(&rects[ix]);
			bottoms.insert(&rects[ix]);
		}
		/* iterate until all Rects are accounted for */
		multiset<BRect*,top_sort_rect>::iterator starts(tops.begin());
		multiset<BRect*,bottom_sort_rect>::iterator stops(bottoms.begin());
		bool negative = false;
		while (stops != bottoms.end()) {
			bool change = false;
			/* de-activate what needs de-activating */
			while (stops != bottoms.end() && (*stops)->bottom < line) {
				change = true;
				active.erase(*stops);
				stops++;
			}
			/* activate what needs activating */
			while (starts != tops.end() && (*starts)->top <= line) {
				change = true;
				active.insert(*starts);
				starts++;
			}
			/* emit a line */
			if (!change) {
			repeat_line:
				if (!negative) {
					if (used >= max_count) {
						err = B_NO_MEMORY;
						break;
					}
					negative = true;
					*(data++) = -1;	/* no change */
					used++;
				}
				else {
					data[-1] --;	/* collate successive negatives */
				}
			}
			else {
			/*** Should really check for overflow of output buffer here... ***/
				if (used >= max_count-2*active.size()) {
					err = B_NO_MEMORY;
					break;
				}
				int16 * newdata = data;
				int newused = used;
				emit_shorts(active, data, used);
				/* merge identical ranges (can happen) */
				if (olddata && (olddata[0] == newdata[0])) {
					for (int ix=1; ix<=olddata[0]; ix++) {
						if (olddata[ix] != newdata[ix]) {
							goto naah;	/* different */
						}
					}
					data = newdata;
					used = newused;
					goto repeat_line;
				}
				else {
				naah:	/* keep the change; it is now our reference */
					negative = false;
					olddata = newdata;
				}
			}
			line++;
		}
		/* detect trailing 0s */
		int zz = -1;
		while (used+zz >= 0) {
			if (data[zz] != 0)
				break;
			zz--;
		}
		used += (zz + 1);
	
		if (err == B_OK)
			*out_count = used;
	}
	delete [] rects;	/* don't leak memory */

	return err;
}


status_t
BBufferProducer::ClipDataToRegion(
	int32 format,
	int32 size,
	const void * data, 
	BRegion * region)
{
	region->MakeEmpty();

	status_t err = B_OK;
	switch (format) {
	case BBufferProducer::B_CLIP_SHORT_RUNS:
		err = clip_shorts_to_region((int16 *)data, size/2, region);
		break;
	default:
		err = B_MEDIA_BAD_CLIP_FORMAT;
	}
	return err;
}


status_t
BBufferProducer::GetLatency(
	bigtime_t * out_latency)
{
//	The latency is always the same; the recordDelay does not affect it.
//	
//	if ((RunMode() == BMediaNode::B_RECORDING) && (_m_recordDelay != 0))
//	{
//		*out_latency = _m_recordDelay;
//		return B_OK;
//	}
	int32 cookie = 0;
	media_output output;
	bigtime_t total = 0;
	while (GetNextOutput(&cookie, &output) == B_OK) {
		bigtime_t now = 0;
		media_node_id time;
		if (output.destination != media_destination::null) {
			status_t err = FindLatencyFor(output.destination, &now, &time);
			if (err == B_OK && now > total) total = now;
		}
	}
	DisposeOutputCookie(cookie);
	*out_latency = total;
	return B_OK;
}


status_t
BBufferProducer::SetPlayRate(
	int32 numer,
	int32 denom)
{
	&numer;
	&denom;
	return B_ERROR;
}


media_type
BBufferProducer::ProducerType()
{
	return _m_producer_type;
}


		/* Mmmh, stuffing! */
status_t
BBufferProducer::_Reserved_BufferProducer_0(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_1(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_2(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_3(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_4(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_5(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_6(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_7(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_8(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_9(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_10(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_11(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_12(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_13(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_14(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::_Reserved_BufferProducer_15(void *)
{
	return B_ERROR;
}

status_t
BBufferProducer::SendRequestResult(
	const media_request_info & result,
	port_id port, bool sync)
{
	status_t err;

	if (sync)
		err = _write_port_etc_sync(port, M_REQUEST_COMPLETED, &result, sizeof(result), B_TIMEOUT, DEFAULT_TIMEOUT);
	else
		err = write_port_etc(port, M_REQUEST_COMPLETED, &result, sizeof(result), B_TIMEOUT, DEFAULT_TIMEOUT);

	if (err < 0)
	{
		fprintf(stderr, "Uh-oh: SendRequestResult() failed (0x%x, %s)\n(port %ld) will not be happy.\n",
				err, strerror(err), port);
	}
	return err;
}

void
BBufferProducer::PSetRecordDelay(
	bigtime_t inDelay)
{
	_m_recordDelay = inDelay;
}


status_t
BBufferProducer::VideoClippingChanged(
	const media_source & for_source,
	int16 num_shorts,
	int16 * clip_data,
	const media_video_display_info & display,
	int32 * _deprecated_)
{
	&for_source;
	&num_shorts;
	&clip_data;
	&display;
	&_deprecated_;

	return B_ERROR;
}


void
BBufferProducer::LatencyChanged(
	const media_source & source,
	const media_destination & destination,
	bigtime_t new_latency,
	uint32 flags)
{
	&source;
	&destination;
	&new_latency;
	&flags;
	/* do nothing */
}


void
BBufferProducer::SetInitialLatency(
	bigtime_t inLatency,
	uint32 flags)
{
	&flags;
	_m_initialLatency = inLatency;
	_m_initialFlags = flags;
}
