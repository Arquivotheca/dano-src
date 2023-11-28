#include "trinity_p.h"

#include <Autolock.h>
#include <Debug.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <parsedate.h>
#include <Application.h>
#include <Alert.h>
#include <Mime.h>
#include <TypeConstants.h>
#include <MediaAddOn.h>
#include <alloca.h>
#include <assert.h>
#include <stdlib.h>
#include <String.h>

#include <algorithm>
#include <map>

#include "smart_ptr.h"
#include "tr_debug.h"
#include "ParameterWeb.h"
#include "BufferProducer.h"
#include "BufferConsumer.h"
#include "Buffer.h"
#include "BufferGroup.h"
#include "MediaAddOn.h"
#include "timesource_p.h"
#include "port_sync.h"

#include <rt_alloc.h>

#if !NDEBUG
#define FPRINTF fprintf
#define FPRINT(x) fprintf x
#else
#define FPRINTF
#define FPRINT(x) (void)0
#endif

#define REFCNT FPRINTF
#define DEFAULT FPRINTF
#define FORMATS //FPRINTF
#define ERRORS FPRINTF

#define SYS_TIME_SOURCE_NODE	1


//	don't turn this off, developers need to see it!
#define DIAGNOSTIC fprintf

bool BPrivate::media_debug = false;

port_id BMediaRoster::_mReplyPort;
int32 BMediaRoster::_mReplyPortRes;
int32 BMediaRoster::_mReplyPortUnavailCount;

namespace BPrivate {
	class AsyncNodeReleaser {
	public:
	
		AsyncNodeReleaser(BMediaNode *node, port_id notifyPort);
	
	private:
	
		static int32 StartReleaseThread(void *castToRelease);
		void ReleaseNode();
	
		BMediaNode *fNode;
		port_id fNotifyPort;
	};
}

using namespace BPrivate;

status_t _write_port_etc_sync(port_id port, int32 msg_code, const void *msg_buffer,
			size_t buffer_size, uint32 flags, bigtime_t timeout)
{
	if (buffer_size > B_MEDIA_MESSAGE_SIZE-sizeof(sem_id)) return B_BAD_VALUE;
	uint8 * buffer = (uint8 *)alloca(buffer_size+sizeof(sem_id));
	memcpy(buffer,msg_buffer,buffer_size);
	sem_id sem = create_sem(0,"write_port_etc_sync");
	*((int32*)(buffer+buffer_size)) = sem;
	status_t err = write_port_etc(port,msg_code,buffer,buffer_size+4,flags,timeout);
	if (err >= 0) acquire_sem(sem);		// sync iff the send succeeded
	delete_sem(sem);
	return err;
};

status_t _write_port_sync(port_id port, int32 msg_code, const void *msg_buffer, size_t buffer_size)
{
	if (buffer_size > B_MEDIA_MESSAGE_SIZE-sizeof(sem_id)) return B_BAD_VALUE;
	uint8 * buffer = (uint8 *)alloca(buffer_size+sizeof(sem_id));
	memcpy(buffer,msg_buffer,buffer_size);
	sem_id sem = create_sem(0,"write_port_sync");
	*((int32*)(buffer+buffer_size)) = sem;
	status_t err = write_port(port,msg_code,buffer,buffer_size+4);
	if (err >= 0) err = acquire_sem(sem);		// sync iff the send succeeded
	delete_sem(sem);
	return err;
};

ssize_t _read_port_etc_sync(port_id port, int32 *msg_code, void *msg_buffer,
			size_t buffer_size, uint32 flags, bigtime_t timeout, sem_id *out_to_release)
{
	uint8 * buffer = (uint8 *)alloca(buffer_size+4);
	ssize_t size = read_port_etc(port, msg_code, buffer, buffer_size+4, flags, timeout);
	if (size < 4) return (size < 0) ? size : B_IO_ERROR;
	sem_id sem = *((sem_id*)(buffer+size-4));
	memcpy(msg_buffer,buffer,size-4);
	if (out_to_release) {
		*out_to_release = sem;
	}
	else {
		release_sem(sem);
	}
	return size-4;
};

const char * B_MEDIA_SERVER_SIGNATURE = "application/x-vnd.Be.media-server";

BMediaRoster * BMediaRoster::_sDefault = NULL;
static BLocker sInitLock("MediaRoster Init Lock");
bool BMediaRoster::_isMediaServer = false;
static bool g_atExitCalled;

namespace BPrivate {
	class AtExitCaller {
			int i;	//	unused, mostly
	public:
			AtExitCaller();
	};
	AtExitCaller atExitCaller;
}

#if 0
class _DefaultDeleter {
public:
	_DefaultDeleter()
	{
#if 0   /* don't do this unless it's really necessary */
		const char * ptr = getenv("MEDIA_LOCKED_MEMORY_SIZE");
		size_t size = ptr ? atoi(ptr) : 400000;
		if (size < 128000) size = 128000;
		//	Using more than 3 MB of locked heap? Hardly likely.
		if (size > 3*1024*1024) size = 3*1024*1024;
		thread_info ti;
		get_thread_info(find_thread(NULL), &ti);
		if (strcmp(ti.name, "media_server")) {
			// the media_server doesn't use locked allocators because
			// it's so special
			rtm_create_pool(&_rtm_pool, size);
		}
#endif
		debug_attach();
	}

	~_DefaultDeleter()
	{
		debug_dump_bufs();
		if (BMediaRoster::_sDefault != NULL) {
			((_BMediaRosterP *)BMediaRoster::_sDefault)->
					BadMediaAddonsMayCrashHere(kNoIAmNotTheAddonServer);
			#if MEDIA_ROSTER_IS_LOOPER
				BMediaRoster::_sDefault->Lock();
				BMediaRoster::_sDefault->Quit();
			#else
				delete BMediaRoster::_sDefault;
			#endif
		}
		if (_rtm_pool != 0)
			rtm_delete_pool(_rtm_pool);

	}
};
#endif	//	 if 0

void
_BMediaRosterP::roster_cleanup()
{
	//	call user-registered cleanup functions
	_m_cleanupLock.Lock();
	for (cleanup_func_list::iterator ptr(_m_cleanupFuncs.begin()); ptr != _m_cleanupFuncs.end();) {
		void (*func)(void *) = (*ptr).first;
		void * pp = (*ptr).second;
		++ptr;
		(*func)(pp);
	}
	_m_cleanupFuncs.clear();
	_m_cleanupLock.Unlock();
	//	call our clean-up, if not already done
	if (BMediaRoster::_sDefault != NULL) {
		((_BMediaRosterP *)BMediaRoster::_sDefault)->BadMediaAddonsMayCrashHere(kNoIAmNotTheAddonServer);
		#if MEDIA_ROSTER_IS_LOOPER
			if (BMediaRoster::_sDefault->Lock())
				BMediaRoster::_sDefault->Quit();
		#else
			delete BMediaRoster::_sDefault;
		#endif
	}
//	DON'T DELETE THIS CODE
//	It forces a reference to the global instance
	if (&BPrivate::atExitCaller == 0) {
		puts("atExitCaller lives at NULL?");
	}
}

//	Do this in a global constructor because CL-Amp
//	uses atexit() to clean up his use of audio. The library
//	will get loaded before his app, so this will work (even
//	though the standard doesn't guarantee it).
BPrivate::AtExitCaller::AtExitCaller()
{
	atexit(_BMediaRosterP::roster_cleanup);
	i = 1;
}


//static _DefaultDeleter sTheInstance;

static const char * prog_name()
{
	static char pname[256];
	if (pname[0])
		return pname;

	app_info ai;
	be_app->GetAppInfo(&ai);
	strcpy(pname, ai.ref.name);
	return pname;
}

ssize_t
BMediaRoster::MediaFlags(
	media_flags cap,
	void * buf,
	size_t maxSize)
{
	sInitLock.Lock();
	ssize_t rsize = B_ERROR;
	void * rdata = NULL;
	switch (cap)
	{
	case B_MEDIA_FLAGS_VERSION:
		{
static uint32 media_version;
			rsize = 4;
			rdata = &media_version;
			if (media_version == 0)
			{
				media_version = parsedate(__DATE__ " " __TIME__, -1);
				if (media_version == (uint32)-1)
				{	/* parsedate() error? Then there's no version. */
					rsize = B_ERROR;
				}
			}
		}
		break;
	case B_MEDIA_CAP_IS_SERVER:
		{
			rdata = &_isMediaServer;
			rsize = 1;
		}
		break;
	case B_MEDIA_CAP_SET_SERVER:
		{
			_isMediaServer = *(bool *)buf;
			dlog("B_MEDIA_CAP_SET_SERVER %s", _isMediaServer ? "true" : "false");
		}
		break;
	case B_MEDIA_CAP_SET_DEBUG:
		{
			BPrivate::media_debug = *(bool *)buf;
		}
		break;
	default:
		/* do nothing */
		break;
	}
	if ((rdata != NULL) && (buf != NULL) && (rsize > 0))
	{
		if (maxSize > rsize)
			maxSize = rsize;
		memcpy(buf, rdata, maxSize);
	}
	sInitLock.Unlock();
	return rsize;
}


BMediaRoster::BMediaRoster() :
#if MEDIA_ROSTER_IS_LOOPER
	BLooper("_Media_Roster_", 20)
#else
	BHandler("_Media_Roster_")
#endif
{
	thread_info thinfo;
	get_thread_info(find_thread(NULL), &thinfo);
	team_info tminfo;
	get_team_info(thinfo.team, &tminfo);
	((_BMediaRosterP *)this)->_mInMediaAddonServer =
		(strstr(tminfo.args, "media_addon_server") != 0);
//	FPRINTF(stderr, "MediaRoster is %s media_addon_server\n",
//		((_BMediaRosterP *)this)->_mInMediaAddonServer ?
//		"in" : "not in");

//	(void) &sTheInstance;
	_mReplyPort = create_port(1, "MediaRoster reply_port");
	_mReplyPortRes = 0;
	_mReplyPortUnavailCount = 0;	/* debugging aid */
}


BMediaRoster::~BMediaRoster()
{
	PRINT(("BMediaRoster::~BMediaRoster()\n"));
	if (_sDefault == this) {
		_sDefault = NULL;
	}
	delete_port(_mReplyPort);
	_mReplyPort = 0;
}



status_t
BMediaRoster::GetVideoInput(
	media_node * out_node)
{
	return ((_BMediaRosterP *)_sDefault)->GetNodeFor(DEFAULT_VIDEO_INPUT, out_node);
}


status_t
BMediaRoster::GetAudioInput(
	media_node * out_node)
{
	return ((_BMediaRosterP *)_sDefault)->GetNodeFor(DEFAULT_AUDIO_INPUT, out_node);
}


status_t
BMediaRoster::GetVideoOutput(
	media_node * out_node)
{
	return ((_BMediaRosterP *)_sDefault)->GetNodeFor(DEFAULT_VIDEO_OUTPUT, out_node);
}


status_t
BMediaRoster::GetAudioOutput(
	media_node * out_node)
{
	return ((_BMediaRosterP *)_sDefault)->GetNodeFor(DEFAULT_AUDIO_OUTPUT, out_node);
}


status_t
BMediaRoster::GetAudioOutput(
	media_node * out_node,
	int32 * out_input_id,
	BString * out_input_name)
{
	if (!out_node || !out_input_id || !out_input_name) return B_BAD_VALUE;
	status_t err = ((_BMediaRosterP *)_sDefault)->GetNodeFor(DEFAULT_AUDIO_OUTPUT, out_node);
	if (err < B_OK)
		return err;
	BMessage msg;
	err = GetDefaultInfo(DEFAULT_AUDIO_OUTPUT, msg);
	if (err < B_OK) {
//		ReleaseNode(*out_node);
		/* it's OK to not have any saved default info! */
		*out_input_id = -1;
		out_input_name->SetTo("");
		return B_OK;
	}
	if (msg.FindInt32("be:_input_id", out_input_id) < B_OK)
		*out_input_id = -1;
	const char * nm = 0;
	if (msg.FindString("be:_input_name", &nm) < B_OK)
		out_input_name->SetTo("");
	else
		out_input_name->SetTo(nm);
	return B_OK;
}


status_t
BMediaRoster::GetAudioMixer(
	media_node * out_node)
{
	return ((_BMediaRosterP *)_sDefault)->GetNodeFor(DEFAULT_AUDIO_MIXER, out_node);
}


status_t
BMediaRoster::GetTimeSource(
	media_node * out_node)
{
	media_node clone;
	_StReleaseNode release_clone(clone);
	if (GetAudioOutput(&clone) || !(clone.kind & B_TIME_SOURCE))
	{
		*out_node = ((_BMediaRosterP *)_sDefault)->ReturnNULLTimeSource()->Node();
		return B_OK;
	}
	
	*out_node = clone;
	return B_OK;
}


BTimeSource *
BMediaRoster::MakeTimeSourceFor(
	const media_node & node)
{
	dlog("MakeTimeSourceFor(%d)", node.node);
	BTimeSource * source = NULL;
	get_timesource_q cmd;
	cmd.cookie = node.node;

	if (node.port == B_BAD_VALUE) {
		if (node.kind & B_TIME_SOURCE) {
			assert(node.node > 0);
			return new _BTimeSourceP(node.node);
		}
	}
	if (node.node <= 0) {
		return NULL;
	}
	cmd.reply = checkout_reply_port("MakeTimeSourceFor");
	if (cmd.reply < 0) {
		return NULL;
	}
	status_t err = write_port_etc(node.port, M_GET_TIMESOURCE, &cmd, sizeof(cmd),
		B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		get_timesource_a reply;
		int32 code = 0;
		while ((err = read_port_etc(cmd.reply, &code, &reply, sizeof(reply),
			B_TIMEOUT, DEFAULT_TIMEOUT)) >= 0) {
			if (code == M_GET_TIMESOURCE_REPLY) {
				break;
			}
			dlog("stale reply: %x (expected %x); retrying", code, M_GET_TIMESOURCE_REPLY);
			code = 0;
		}
		if (err >= B_OK) {
			err = reply.time_source;
		}
		if (err >= B_OK) {
			source = new _BTimeSourceP(reply.time_source);
		}
		else {
			dlog("weird reply in MakeTimeSourceFor(): %x", err);
		}
	}
	else {
		dlog("MakeTimeSourceFor() failed: %x", err);
	}
	checkin_reply_port(cmd.reply);
	return source;
}

	static bool req_k(
		uint32 flags,
		uint32 match)
	{
		return (flags & match) == match;
	}


status_t
BMediaRoster::SetAudioInput(
	const media_node & node)
{
	if (!req_k(node.kind, B_PHYSICAL_INPUT | B_BUFFER_PRODUCER)) {
		return B_MEDIA_BAD_NODE;
	}
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultNode(DEFAULT_AUDIO_INPUT, node);
}


status_t
BMediaRoster::SetAudioInput(
	const dormant_node_info & node)
{
	//todo:	get flavor info and check kinds
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultDormant(DEFAULT_AUDIO_INPUT, node);
}


status_t
BMediaRoster::SetAudioOutput(
	const media_node & node)
{
	if (!req_k(node.kind, B_PHYSICAL_OUTPUT | B_BUFFER_CONSUMER | B_TIME_SOURCE)) {
		return B_MEDIA_BAD_NODE;
	}
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultNode(DEFAULT_AUDIO_OUTPUT, node);
}


status_t
BMediaRoster::SetAudioOutput(
	const media_input & output)
{
	if (!req_k(output.node.kind, B_PHYSICAL_OUTPUT | B_BUFFER_CONSUMER | B_TIME_SOURCE)) {
		return B_MEDIA_BAD_NODE;
	}
	return ((_BMediaRosterP *)_sDefault)->SaveDefaultNode(DEFAULT_AUDIO_OUTPUT, output.node, &output.destination, output.name);
}


status_t
BMediaRoster::SetAudioOutput(
	const dormant_node_info & node)
{
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultDormant(DEFAULT_AUDIO_OUTPUT, node);
}


status_t
BMediaRoster::SetVideoInput(
	const media_node & node)
{
	if (!req_k(node.kind, B_PHYSICAL_INPUT | B_BUFFER_PRODUCER)) {
		return B_MEDIA_BAD_NODE;
	}
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultNode(DEFAULT_VIDEO_INPUT, node);
}


status_t
BMediaRoster::SetVideoInput(
	const dormant_node_info & node)
{
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultDormant(DEFAULT_VIDEO_INPUT, node);
}


status_t
BMediaRoster::SetVideoOutput(
	const media_node & node)
{
	if (!req_k(node.kind, B_PHYSICAL_OUTPUT | B_BUFFER_CONSUMER)) {
		return B_MEDIA_BAD_NODE;
	}
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultNode(DEFAULT_VIDEO_OUTPUT, node);
}


status_t
BMediaRoster::SetVideoOutput(
	const dormant_node_info & node)
{
	return ((_BMediaRosterP *)_sDefault)->
		SaveDefaultDormant(DEFAULT_VIDEO_OUTPUT, node);
}


status_t
BMediaRoster::GetNodeFor(
	media_node_id node,
	media_node * clone)
{
	if (!clone) return B_BAD_VALUE;
	BAutolock _lck(((_BMediaRosterP *)_sDefault)->_mNodeMapLock);
	return ((_BMediaRosterP *)_sDefault)->AcquireNodeReference(node, clone);
}

status_t
BMediaRoster::GetSystemTimeSource(
	media_node * clone)
{
	if (!clone) return B_BAD_VALUE;
	BTimeSource *source = ((_BMediaRosterP *)_sDefault)->ReturnNULLTimeSource();
	if (!source) 
		return B_MEDIA_BAD_NODE;

	*clone = source->Node();
	return B_OK;
}

status_t
BMediaRoster::ReleaseNode(
	const media_node & node)
{
	return ((_BMediaRosterP *)_sDefault)->ReleaseNodeP(node);
}



	#define get_message_raw(m,n,p) _get_message_raw(m,n,B_RAW_TYPE,p,sizeof(*p))
	static status_t
	_get_message_raw(
		BMessage & m,
		const char * name,
		type_code type,
		void * ptr,
		size_t size)
	{
		void * out;
		ssize_t os = 0;
		status_t err = m.FindData(name, type, (const void **)&out, &os);
		if (!err)
		{
			if (os > size)
				os = size;
			memcpy(ptr, out, os);
		}
		return err;
	}

	static bool
	format_has_wildcards(
		media_format & fmt)
	{
		bool ret = false;
		switch (fmt.type)
		{
		case B_MEDIA_RAW_AUDIO:
			if (fmt.u.raw_audio.frame_rate <= media_raw_audio_format::wildcard.frame_rate)
				ret = true;
			else if (fmt.u.raw_audio.channel_count <= media_raw_audio_format::wildcard.channel_count)
				ret = true;
			else if (fmt.u.raw_audio.format <= media_raw_audio_format::wildcard.format)
				ret = true;
			else if (fmt.u.raw_audio.byte_order <= media_raw_audio_format::wildcard.byte_order)
				ret = true;
			break;
		case B_MEDIA_RAW_VIDEO:
			if (fmt.u.raw_video.field_rate <= media_raw_video_format::wildcard.field_rate)
				ret = true;
			else if (fmt.u.raw_video.display.format <= media_raw_video_format::wildcard.display.format)
				ret = true;
			else if (fmt.u.raw_video.interlace <= media_raw_video_format::wildcard.interlace)
				ret = true;
			else if (fmt.u.raw_video.display.line_width <= media_raw_video_format::wildcard.display.line_width)
				ret = true;
			else if (fmt.u.raw_video.display.line_count <= media_raw_video_format::wildcard.display.line_count)
				ret = true;
			else if (fmt.u.raw_video.first_active <= media_raw_video_format::wildcard.first_active)
				ret = true;
			else if (fmt.u.raw_video.orientation <= media_raw_video_format::wildcard.orientation)
				ret = true;
			else if (fmt.u.raw_video.display.bytes_per_row <= media_raw_video_format::wildcard.display.bytes_per_row)
				ret = true;
			else if (fmt.u.raw_video.pixel_width_aspect <= media_raw_video_format::wildcard.pixel_width_aspect)
				ret = true;
			else if (fmt.u.raw_video.pixel_height_aspect <= media_raw_video_format::wildcard.pixel_height_aspect)
				ret = true;
			break;
		default:
			ret = true;
			break;
		}
		return ret;
	}

	static status_t
	format_negotiation(
		media_source src,
		media_destination dst,
		media_format & fmt,
		port_id use_port)
	{
		sem_id toRelease;
		status_t error = B_OK;
		dlog("entering format_negotiation()");
		propose_format_q cmd;
		cmd.reply = use_port;
		cmd.cookie = find_thread(NULL);
		cmd.output = src;
		cmd.format = fmt;
		dlog("producer");
		error = write_port(src.port, BP_PROPOSE_FORMAT, &cmd, sizeof(cmd));
		if (error < B_OK) {
			dlog("write_port(%d) failed in format_negotiation %x %s", src.port, error, strerror(error));
		}
		if (error >= B_OK) {
			int32 code;
			propose_format_a reply;
			error = _read_port_etc_sync(use_port, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease);
			reply.format.MetaData();
			release_sem(toRelease);
			if (error < B_OK) {
				dlog("read_port(%d) failed in format_negotiation %x %s", use_port, error, strerror(error));
			}
			if (error >= B_OK) {
				error = reply.error;
				if (error < B_OK) {
					fmt = reply.format;
					dlog("producer returns error in format_negotiation %x %s", error, strerror(error));
				}
			}
			if (error >= B_OK) {
				accept_format_q acc;
				acc.reply = use_port;
				acc.input = dst;
				acc.format = reply.format;
				dlog("consumer");
				error = write_port(dst.port, BC_ACCEPT_FORMAT, &acc, sizeof(acc));
				if (error < B_OK) {
					dlog("write_port(%d) error in format_negotiation %x %s", dst.port, error, strerror(error));
				}
				if (error >= B_OK) {
					accept_format_a rep2;
					error = _read_port_etc_sync(use_port, &code, &rep2, sizeof(rep2), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease);
					rep2.format.MetaData();
					release_sem(toRelease);
					if (error < B_OK) {
						dlog("read_port(%d) error in format_negotiation %x %s", use_port, error, strerror(error));
					}
					if (error >= B_OK) {
						error = rep2.error;
						if (error < B_OK) {
							dlog("client error in format_negotiation %x %s", error, strerror(error));
						}
					}
					fmt = rep2.format;
					if (error >= B_OK) {
						error = B_OK;
					}
				}
			}
		}
		if (error >= B_OK) {
			dlog("format_negotiation() returns successfully: %x", error);
		}
		return error;
	}

status_t
BMediaRoster::Connect(
	const media_source & from,
	const media_destination & to,
	media_format * with_format,
	media_output * out_output,
	media_input * out_input)
{
	return Connect(from, to, with_format, out_output, out_input, 0, 0);
}

#define kConnectTested 1
#define kConnectPersonalStudio 2

static int32
test_flag_function()
{
	app_info info;
	int32 ret = 0;
	if (be_app->GetAppInfo(&info)) return ret;
static const char * sPersonalStudio = "application/x-vnd.adamation-ps";
	if (!strcasecmp(info.signature, sPersonalStudio)) ret |= kConnectPersonalStudio;
	return ret;
}

status_t
BMediaRoster::Connect(
	const media_source & from,
	const media_destination & to,
	media_format * with_format,
	media_output * out_output,
	media_input * out_input,
	uint32 in_flags,
	void * /* _reserved */)
{
#if DEBUG
	if (with_format->type == B_MEDIA_RAW_AUDIO) {
		media_raw_audio_format ff(with_format->u.raw_audio);
		FPRINTF(stderr, "format: %g;%x;%d;%d;%d\n", ff.frame_rate, ff.format, 
				ff.channel_count, ff.byte_order, ff.buffer_size);
	}
#endif
	if (from.port <= 0) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (to.port <= 0) {
		return B_MEDIA_BAD_SOURCE;
	}
	if (!with_format) {
		return B_BAD_VALUE;
	}
	if (!out_output) {
		return B_BAD_VALUE;
	}
	if (!out_input) {
		return B_BAD_VALUE;
	}
static int32 test_flags = 0;
	if (!test_flags) {
		test_flags = test_flag_function() | kConnectTested;
	}
	if (test_flags & kConnectPersonalStudio) {
		if ((with_format->type == B_MEDIA_RAW_VIDEO) &&
				(with_format->u.raw_video.display.format == B_RGBA32)) {
			with_format->u.raw_video.display.format = B_RGB32;
		}
	}
	status_t error = B_OK;
	port_id port = checkout_reply_port("Connect");
	if (port < B_OK)
	{
		dlog("checkout_reply_port() failed in Connect(): %x %s", port, strerror(port));
		return port;
	}
	/* figure out what the format's supposed to be */
//	if (format_has_wildcards(*with_format))
//	{
		error = format_negotiation(from, to, *with_format, port);
		if (error != B_OK) {
			dlog("format_negotiation(%d, %d, %d) failed %x %s", 
				from.port, to.port, port, error, strerror(error));
			FORMATS(stderr, "format_negotiation(%d, %d, %d) failed %x %s", 
				from.port, to.port, port, error, strerror(error));
		}
//	}
	sem_id toRelease;
	prepare_connection_q pcq;
	prepare_connection_a pca;
	hooked_up_q huq;
	hooked_up_a hua;
	hookup_q hq;
	hookup_a ha;
	int32 code;
	bool tell_producer = false;

	pcq.reply = port;
	pcq.cookie = find_thread(NULL);
	pcq.from = from;
	pcq.where = to;
	pcq.format = *with_format;
//	pcq.destination_node = BMediaNode::RecoverNodeID(to.port);
	if (error >= B_OK) {
		dlog("HOOKUP: calling ProposeConnection()");
		error = write_port(from.port, BP_PREPARE_CONNECTION, &pcq, sizeof(pcq));
		if (error < B_OK) {
			dlog("write_port(%d) producer failed %x %s", from.port, error, strerror(error));
		}
		else {
			tell_producer = true;
		}
	}
	if (error >= B_OK) {
read_prepare_connection_again:
		error = _read_port_etc_sync(pcq.reply, &code, &pca, sizeof(pca), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease);
		pca.output.format.MetaData();
		release_sem(toRelease);
		if (error < B_OK) {
			dlog("read_port(%d) from producer failed %x %s", pcq.reply, error, strerror(error));
		}
		else if (code != BP_PREPARE_CONNECTION_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BP_PREPARE_CONNECTION_REPLY);
			goto read_prepare_connection_again;
		}
		else {
			error = pca.error;
			if (error < B_OK) {
				*with_format = pca.output.format;
			}
		}
	}
	if (error >= B_OK) {
		dlog("HOOKUP: calling Connected() (producer %d)", pca.output.source.port);
		huq.reply = port;
		huq.cookie = find_thread(NULL);
		huq.input.source = pca.output.source;
		huq.input.destination = to;
		huq.input.format = pca.output.format;
		strncpy(huq.input.name, pca.output.name, B_MEDIA_NAME_LENGTH);
//		huq.input.source_node = pca.output.node.node;
		error = write_port(to.port, BC_HOOKED_UP, &huq, sizeof(huq));
	}
	if (error >= B_OK) {
read_hooked_up_again:
		error = _read_port_etc_sync(huq.reply, &code, &hua, sizeof(hua), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease);
		hua.input.format.MetaData();
		release_sem(toRelease);
		if (error < B_OK) {
			dlog("read_port(%d) from consumer failed %x %s", huq.reply, error, strerror(error));
		}
		else if (code != BC_HOOKED_UP_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BC_HOOKED_UP_REPLY);
			goto read_hooked_up_again;
		}
		else {
			error = hua.error;
			if (error < B_OK) {
				*with_format = hua.input.format;
			}
		}
	}
	status_t save_error = error;
	if (tell_producer) {
		dlog("HOOKUP: calling Connect() on producer (%x, %d)", error, hua.input.destination.port);
		if (error > B_OK) {
			error = B_OK;
		}
		hq.reply = port;
		hq.cookie = find_thread(NULL);
		hq.status = error;
		hq.output.source = hua.input.source;
		hq.output.destination = hua.input.destination;
		hq.output.format = hua.input.format;
		strncpy(hq.output.name, hua.input.name, B_MEDIA_NAME_LENGTH);
//		hq.output.destination_node = hua.input.node.node;
		hq.flags = in_flags;
		error = write_port(from.port, BP_HOOKUP, &hq, sizeof(hq));
	}
	if (error >= B_OK) {
read_hookup_again:
		error = _read_port_etc_sync(hq.reply, &code, &ha, sizeof(ha), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease);
		ha.output.format.MetaData();
		release_sem(toRelease);
		if (error < B_OK) {
			dlog("read_port(%d) from consumer failed %x %s", hq.reply, error, strerror(error));
		}
		else if (code != BP_HOOKUP_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BP_HOOKUP_REPLY);
			goto read_hookup_again;
		}
	}
	if (error >= 0 && save_error < 0) {
		error = save_error;
	}
	if (error >= B_OK) {
		*out_output = ha.output;
		*out_input = hua.input;
		*with_format = hua.input.format;
		assert(format_is_compatible(ha.output.format, hua.input.format));
		error = B_OK;
	}
	checkin_reply_port(port);
#if 0
	if (error >= 0)
	{
		error = B_OK;
		/* broadcast the newly made connection */
		dlog("proceeding with hookup -- informing server");
		BMessage req(MEDIA_MAKE_CONNECTION);
		BMessage reply;
		req.AddData("input", B_RAW_TYPE, out_input, sizeof(*out_input));
		req.AddData("output", B_RAW_TYPE, out_output, sizeof(*out_output));
		req.AddData("format", B_RAW_TYPE, with_format, sizeof(*with_format));
		error = ((_BMediaRosterP *)_sDefault)->Turnaround(&req, &reply);
		if (error != B_OK)
			dlog("Error returned from Connect is %x %s", error, strerror(error));
	}
#endif
	if (error == B_OK) {
		BMessage notification;
		notification.AddData("output", B_RAW_TYPE, out_output, sizeof(*out_output));
		notification.AddData("input", B_RAW_TYPE, out_input, sizeof(*out_input));
		notification.AddData("format", B_RAW_TYPE, with_format, sizeof(*with_format));
		(void)((_BMediaRosterP *)_sDefault)->Broadcast(notification, B_MEDIA_CONNECTION_MADE);
	}

	if (error != B_OK) {
		FORMATS(stderr, "Connect() failed error %x (%s)\n", error, strerror(error));
	}
	return error;
}


status_t
BMediaRoster::Disconnect(
	media_node_id source_node,
	const media_source & source,
	media_node_id destination_node,
	const media_destination & destination)
{
	/* should we somehow validate the node IDs? */
	status_t error = B_OK;
	/* here, we try to break the connection */
	if (error == B_OK)
	{
		break_q cmd;
		break_a ans;
		cmd.reply = checkout_reply_port("Disconnect");
		cmd.cookie = 0;
		cmd.from = source;
		cmd.where = destination;
		error = write_port(source.port, BP_BREAK, &cmd, sizeof(cmd));
		/*** status? change count, maybe ***/
		int32 code = 0;
		if (error >= B_OK) while ((error = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT)) > 0) {
			if (code != BP_BREAK_REPLY) {
				dlog("stale reply: %x (expected %x); retrying", code, BP_BREAK_REPLY);
				continue;
			}
			break;
		}
		if (error == 0) {
			error = B_ERROR;
		}
		else if (error > 0) {
			error = ans.error;
		}
		checkin_reply_port(cmd.reply);
	}
//	if (error >= 0)
	{
		broken_q cmd2;
		cmd2.producer = source;
		cmd2.where = destination;
		status_t err = write_port(destination.port, BC_BROKEN, &cmd2, sizeof(cmd2));
		if (err < 0) {
			error = err;
		}
	}
#if 0
	if (error >= 0)
	{
		error = B_OK;
		/* broadcast the newly broken connection */
		BMessage req(MEDIA_BREAK_CONNECTION);
		BMessage reply;
		req.AddData("destination", B_RAW_TYPE, &destination, sizeof(destination));
		req.AddInt32("destination_node", destination_node);
		req.AddData("source", B_RAW_TYPE, &source, sizeof(source));
		req.AddInt32("source_node", source_node);
/*** Candidate for async ***/
		error = ((_BMediaRosterP *)_sDefault)->Turnaround(&req, &reply);
	}
#endif
	if (error == B_OK) {
		BMessage notification;
		notification.AddData("source", B_RAW_TYPE, &source, sizeof(source));
		notification.AddData("destination", B_RAW_TYPE, &destination, sizeof(destination));
		(void)((_BMediaRosterP *)_sDefault)->Broadcast(notification, B_MEDIA_CONNECTION_BROKEN);
	}
	return error;
}


BMediaRoster *
BMediaRoster::Roster(
	status_t * error)
{
	/* Media Server nodes can't access the media roster */
	if (!dcheck(_BMediaRosterP::_isMediaServer == false)) return NULL;
	BAutolock lock(sInitLock);
	if ((_sDefault == NULL) && (be_app == NULL))
	{
		DIAGNOSTIC(stderr, "It is an error to call BMediaRoster::Roster() before there is a valid be_app.\n");
		*error = B_BAD_VALUE;
		return NULL;
	}
	if (_sDefault == NULL) {
		(void)new _BMediaRosterP(error);
#if MEDIA_ROSTER_IS_LOOPER
		if (_sDefault) _sDefault->Run();
#endif
	}
	return _sDefault;
}


BMediaRoster *
BMediaRoster::CurrentRoster()
{
	return _sDefault;
}


status_t
BMediaRoster::StartWatching(
	const BMessenger & where,
	int32 notificationType)
{
//	FPRINTF(stderr, "StartWatching() (no node)\n");
	if (!where.IsValid()) return B_BAD_VALUE;
	BMessage notr(MEDIA_REQUEST_NOTIFICATIONS);
	notr.AddMessenger("messenger", where);
	notr.AddInt32("which", notificationType);
	BMessage reply;
/*** Candidate for async ***/
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&notr, &reply);
	return error;
}


status_t
BMediaRoster::StartWatching(
	const BMessenger & where,
	const media_node & node,
	int32 notificationType)
{
//	FPRINTF(stderr, "StartWatching('%c%c%c%c') (node %ld)\n", notificationType>>24, notificationType>>16, notificationType>>8, notificationType, node.node);
	if (!where.IsValid()) return B_BAD_VALUE;
	if (node.node <= 0) return B_MEDIA_BAD_NODE;
	BMessage notr(MEDIA_REQUEST_NOTIFICATIONS);
	notr.AddMessenger("messenger", where);
	notr.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	notr.AddInt32("which", notificationType);
	BMessage reply;
/*** Candidate for async ***/
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&notr, &reply);
	return error;
}

status_t
BMediaRoster::StartWatching(
	const BMessenger & where)
{
	return StartWatching(where,B_MEDIA_WILDCARD);
}


status_t
BMediaRoster::StopWatching(
	const BMessenger & where,
	int32 notificationType)
{
	if (!where.IsValid()) return B_BAD_VALUE;
	BMessage notr(MEDIA_CANCEL_NOTIFICATIONS);
	notr.AddMessenger("messenger", where);
	notr.AddInt32("which", notificationType);
	BMessage reply;
/*** Candidate for async ***/
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&notr, &reply);
	return error;
}

status_t
BMediaRoster::StopWatching(
	const BMessenger & where,
	const media_node & node,
	int32 notificationType)
{
	if (!where.IsValid()) return B_BAD_VALUE;
	if (node.node <= 0) return B_MEDIA_BAD_NODE;
	BMessage notr(MEDIA_CANCEL_NOTIFICATIONS);
	notr.AddMessenger("messenger", where);
	notr.AddData("node", B_RAW_TYPE, &node, sizeof(node));
	notr.AddInt32("which", notificationType);
	BMessage reply;
/*** Candidate for async ***/
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&notr, &reply);
	return error;
}

status_t
BMediaRoster::StopWatching(
	const BMessenger & where)
{
	if (!where.IsValid()) return B_BAD_VALUE;
	return StopWatching(where,B_MEDIA_WILDCARD);
}

status_t
BMediaRoster::RegisterNode(
	BMediaNode * node)
{
	if (!node) return B_BAD_VALUE;
	return ((_BMediaRosterP*)_sDefault)->RegisterNodeP(node, true);
}

status_t
BMediaRoster::UnregisterNode(
	BMediaNode * node)
{
	if (!node) return B_BAD_VALUE;
	BMessage unreg(MEDIA_UNREGISTER_NODE);
	BMessage reply;
	unreg.AddInt32("media_node_id", node->ID());
	{
		//	However a Node is deleted, it has to be removed from the local node table.
		BAutolock _lck(((_BMediaRosterP*)_sDefault)->_mNodeMapLock);
		map<media_node_id, BMediaNode*>::iterator that(((_BMediaRosterP*)_sDefault)->_mLocalInstances.find(node->ID()));
		if (that != ((_BMediaRosterP*)_sDefault)->_mLocalInstances.end()) {
			((_BMediaRosterP*)_sDefault)->_mLocalInstances.erase(that);
		}
	}
/*** Candidate for async ***/
	status_t error = ((_BMediaRosterP*)_sDefault)->Turnaround(&unreg, &reply);
	return error;
}


status_t
BMediaRoster::SetOutputBuffersFor(
//	media_node_id node,
	const media_source & output,
	BBufferGroup * buffers,
	bool will_reclaim)
{
	return ((_BMediaRosterP *)_sDefault)->_SetOutputBuffersImp_(output, NULL, NULL, -1, buffers, will_reclaim);
#if 0
	bool will_reclaim_set = false;
	dassert(!(will_reclaim && !buffers));
	BMessage msg(MEDIA_SET_OUTPUT_BUFFERS);
	status_t error = msg.AddData("output", B_RAW_TYPE, &output, sizeof(output));
//	if (error == B_OK) {
//		error = msg.AddInt32("media_node_id", node);
//	}
	if (error == B_OK) {
		error = msg.AddBool("will_reclaim", will_reclaim);
	}
	if (error == B_OK) {
		if (buffers) {
			buffers->WillReclaim();
			will_reclaim_set = true;
			error = buffers->AddBuffersTo(&msg, "media_buffer_id");
		}
		else {
			error = msg.AddInt32("media_buffer_id", -1);
		}
	}
	BMessage reply;
	if (error == B_OK) {
		error = ((_BMediaRosterP *)_sDefault)->Turnaround(&msg, &reply);
	}
	if (error == B_OK) {
		error = ParseCommand(reply);
	}
	if (buffers) {
		if (!will_reclaim) {
			delete buffers; /* We take ownership, and then the other guy does. */
		}
		else if ((error != B_OK) && will_reclaim_set)
		{
			buffers->ReclaimAllBuffers();
		}
	}
	return error;
#endif
}


status_t
BMediaRoster::SetTimeSourceFor(
	media_node_id node,
	media_node_id time_source)
{
	if ((node <= 0) || (time_source <= 0))
		return B_MEDIA_BAD_NODE;
	dlog("SetTimeSourceFor(%d, %d)", node, time_source);
	BMessage query(MEDIA_SET_TIMESOURCE);
	query.AddInt32("media_node_id", node);
	query.AddInt32("time_source", time_source);
	BMessage reply;
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &reply);
	if (error == B_OK)
	{
		error = ParseCommand(reply);
	}
	return error;
}


status_t
BMediaRoster::GetParameterWebFor(
	const media_node & node,
	BParameterWeb ** out_web)
{
	if (!out_web) return B_BAD_VALUE;
	*out_web = NULL;
	if ((node.port <= 0) || !(node.kind & B_CONTROLLABLE)) {
		return B_MEDIA_BAD_NODE;
	}
	smart_ptr<get_web_q> cmd;
	smart_ptr<get_web_a> ans;
	cmd->reply = checkout_reply_port("GetParameterWebFor");
	status_t err = write_port(node.port, CT_GET_WEB, (void*)cmd, sizeof(*cmd));
	if (err < B_OK) {
		dlog("GetParameterWebFor write_port returns %x", err);
		checkin_reply_port(cmd->reply);
		return err;
	}
	int32 code;
read_get_web_again:
	err = read_port_etc(cmd->reply, &code, (void*)ans, sizeof(*ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if ((err >= B_OK) && (code != CT_GET_WEB_REPLY)) {
		dlog("stale reply: %x (expected %x); retrying", code, CT_GET_WEB_REPLY);
		goto read_get_web_again;
	}
	checkin_reply_port(cmd->reply);
	if (err < B_OK) {
		dlog("GetParameterWebFor read_port returns %x", err);
		return err;
	}
	err = ans->error;
	if (err >= B_OK) {
		*out_web = new BParameterWeb;
#if DEBUG
		free(malloc(1));
#endif
		if (!(ans->flags & _BIG_FLAT_WEB)) {
			err = (*out_web)->Unflatten(B_MEDIA_PARAMETER_WEB_TYPE, ans->raw_data, ans->size);
		}
		else {
			err = ((_BMediaRosterP *)_sDefault)->UnflattenHugeWeb(*out_web, ans->big.big_size,
					ans->big.area, ans->big.owner);
		}
		if (err < B_OK) {
			dlog("GetParameterWebFor Unflatten returns %x for %x bytes", err, ans->size);
			delete *out_web;
			*out_web = NULL;
		}
	}
	return err;
}


status_t
BMediaRoster::StartControlPanel(
	const media_node & node,
	BMessenger * out_messenger)
{
	if (node.port <= 0) return B_MEDIA_BAD_NODE;
	if (!(node.kind & B_CONTROLLABLE)) return B_MEDIA_BAD_NODE;
	start_control_panel_q cmd;
	cmd.reply = checkout_reply_port("StartControlPanel");
	cmd.cookie = find_thread(NULL);
	status_t err = write_port(node.port, CT_START_CONTROL_PANEL, &cmd, sizeof(cmd));
	if (err < 0) {
		checkin_reply_port(cmd.reply);
		return err;
	}
	start_control_panel_a ans;
	int32 code;
read_start_control_panel_again:
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if ((err >= B_OK) && (code != CT_START_CONTROL_PANEL_REPLY)) {
		dlog("stale reply: %x (expected %x); retrying", code, CT_START_CONTROL_PANEL_REPLY);
		goto read_start_control_panel_again;
	}
	if (err >= B_OK) err = ans.error;
	checkin_reply_port(cmd.reply);
	if ((err >= B_OK) && out_messenger) {
		*out_messenger = ans.messenger;
	}
	return err;
}


status_t
BMediaRoster::GetDormantNodes(
	dormant_node_info * out_info,
	int32 * io_count,
	const media_format * has_input,
	const media_format * has_output,
	const char * name,
	uint64 require_kinds,
	uint64 deny_kinds)
{
	dlog("GetDormantNodes(%x,%x,%x,%x,%x)", out_info, io_count, has_input, has_output, name);
	if (!io_count || !out_info) return B_BAD_VALUE;
	if (*io_count < 1) return B_BAD_VALUE;
	int32 max_hits = *io_count;
	BMessage query(MEDIA_QUERY_LATENTS);
	query.AddInt32("be:max_hits", *io_count);
	if (has_input != NULL) query.AddData("be:input_format", B_RAW_TYPE, 
		has_input, sizeof(*has_input));
	if (has_output != NULL) query.AddData("be:output_format", B_RAW_TYPE, 
		has_output, sizeof(*has_output));
	if (name != NULL) query.AddString("be:name", name);
	if (require_kinds != 0) query.AddInt64("be:require_kinds", require_kinds);
	if (deny_kinds != 0) query.AddInt64("be:deny_kinds", deny_kinds);
	BMessage reply;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &reply);
	if (err >= B_OK) {
		err = reply.FindInt32("be:out_hits", io_count);
		dlog("be:out_hits returns %x (%d)", err, *io_count);
	}
	if (err >= B_OK) {
		if (*io_count < max_hits) max_hits = *io_count;
		for (int ix=0; ix<max_hits; ix++) {
			dormant_node_info ** ptr = NULL;
			int32 size = 0;
			err = reply.FindData("be:dormant_node_info", B_RAW_TYPE, ix,
				(const void **)&ptr, &size);
			dlog("be:dormant_node_info returns %x (%x)", err, ptr);
			if (err < B_OK) { *io_count = ix; err = 0; break; }
			if (size > sizeof(**ptr)) size = sizeof(**ptr);
			memcpy(&out_info[ix], ptr, size);
		}
	}
	if (err < B_OK) *io_count = 0;
	return err < B_OK ? err : B_OK;
}

// Old version.
status_t
BMediaRoster::InstantiateDormantNode(
	const dormant_node_info & info,
	media_node * node)
{
	if (node == NULL) return B_BAD_VALUE;
	return InstantiateDormantNode(info, node, B_FLAVOR_IS_LOCAL);
}

status_t
BMediaRoster::InstantiateDormantNode(
	const dormant_node_info & info,
	media_node * out_node,
	uint32 flags)
{
	if (out_node == NULL) return B_BAD_VALUE;
	return ((_BMediaRosterP *)_sDefault)->InstantiateDormantNode(info, out_node, flags);
}



status_t
BMediaRoster::GetDormantNodeFor(
	const media_node & node,
	dormant_node_info * info)
{
	if (info == NULL) return B_BAD_VALUE;
	if (node.node <= 0) return B_MEDIA_BAD_NODE;
	BMessage query(MEDIA_GET_DORMANT_NODE);
	query.AddInt32("be:_node", node.node);
	BMessage reply;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &reply);
	if (err >= B_OK) {
		dormant_node_info * ptr = NULL;
		ssize_t size;
		if (!reply.FindData("be:_dormant_node_info", B_RAW_TYPE, (const void**)&ptr, &size) &&
			(ptr != NULL)) {
			*info = *ptr;
		}
		else {
			err = B_MEDIA_ADDON_FAILED;
		}
	}
	return err;
}


status_t
BMediaRoster::GetDormantFlavorInfoFor(
	const dormant_node_info & dormant,
	dormant_flavor_info * out_flavor)
{
	if (out_flavor == NULL) return B_BAD_VALUE;
	return ((_BMediaRosterP *)_sDefault)->GetDormantFlavorInfoForP(dormant, out_flavor, NULL, 0);
}


status_t
_BMediaRosterP::GetDormantFlavorInfoForP(
	const dormant_node_info & dormant,
	dormant_flavor_info * out_flavor,
	char * outPath,
	ssize_t inBufSize)
{
	if (out_flavor == NULL) return B_BAD_VALUE;
	BMessage query(MEDIA_GET_DORMANT_FLAVOR);
	query.AddData("be:_dormant", B_RAW_TYPE, &dormant, sizeof(dormant));
	BMessage reply;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &reply);
	if (err >= B_OK) {
		err = reply.FindFlat("be:_flavor", out_flavor);
	}
	if ((err >= B_OK) && (outPath != NULL) && (inBufSize > 0)) {
		const char * str = NULL;
		err = reply.FindString("be:_path", &str);
		if (str != NULL) {
			strncpy(outPath, str, inBufSize);
			outPath[inBufSize-1] = 0;
		}
	}
	return err;
}


status_t
BMediaRoster::GetLatencyFor(
	const media_node & producer,
	bigtime_t * latency)
{
	dlog("producer.port = %d, producer.kind = %x\n", producer.port, producer.kind);
	if (!dcheck(producer.port > 0)) return B_MEDIA_BAD_NODE;
	if (!dcheck(producer.kind & B_BUFFER_PRODUCER)) return B_MEDIA_BAD_NODE;
	calc_total_latency_q cmd;
	cmd.reply = checkout_reply_port("GetLatencyFor");
	cmd.cookie = find_thread(NULL);
	status_t err = write_port(producer.port, BP_CALC_TOTAL_LATENCY, 
		&cmd, sizeof(cmd));
	if (err < B_OK) {
		checkin_reply_port(cmd.reply);
		return err;
	}
	int32 code;
	calc_total_latency_a ans;
read_calc_total_latency_again:
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		if (code != BP_CALC_TOTAL_LATENCY_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BP_CALC_TOTAL_LATENCY_REPLY);
			goto read_calc_total_latency_again;
		}
		err = ans.error;
		*latency = ans.latency;
	}
	checkin_reply_port(cmd.reply);
	return err;
}


status_t
BMediaRoster::GetInitialLatencyFor(
	const media_node & node,
	bigtime_t * out_latency,
	uint32 * out_flags)
{
	if (!out_latency) return B_BAD_VALUE;
	if (!(node.kind & B_BUFFER_PRODUCER)) return B_MEDIA_BAD_NODE;
	get_initial_latency_q q;
	get_initial_latency_a ans;
	q.cookie = find_thread(NULL);
	q.reply = checkout_reply_port("GetInitialLatencyFor");
	status_t err = write_port_etc(node.port, BP_GET_INITIAL_LATENCY, &q, sizeof(q),
			B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK) return err;
	int32 code;
read_get_initial_latency_again:
	while ((err = read_port_etc(q.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT)) == B_INTERRUPTED)
		/* do it again */ ;
	if (err < B_OK) {
		checkin_reply_port(q.reply);
		return err;
	}
	if (code != BP_GET_INITIAL_LATENCY_REPLY) {
		goto read_get_initial_latency_again;
	}
	checkin_reply_port(q.reply);
	if (ans.error < B_OK) return ans.error;
	*out_latency = ans.latency;
	if (out_flags) *out_flags = ans.flags;
	return B_OK;
}


status_t
BMediaRoster::GetStartLatencyFor(
	const media_node & time_source,
	bigtime_t * latency)
{
	if (latency == NULL) return B_BAD_VALUE;
	if (!dcheck(time_source.port > 0)) return B_MEDIA_BAD_NODE;
	if (!dcheck(time_source.kind & B_TIME_SOURCE)) return B_MEDIA_BAD_NODE;
	get_start_latency_q cmd;
	cmd.reply = checkout_reply_port("GetStartLatencyFor");
	cmd.cookie = find_thread(NULL);
	status_t err = write_port(time_source.port, TS_GET_START_LATENCY, 
		&cmd, sizeof(cmd));
	if (err < B_OK) {
		checkin_reply_port(cmd.reply);
		return err;
	}
	int32 code;
	get_start_latency_a ans;
read_get_start_latency_again:
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		if (code != TS_GET_START_LATENCY_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, TS_GET_START_LATENCY_REPLY);
			goto read_get_start_latency_again;
		}
		err = ans.error;
		*latency = ans.latency;
	}
	checkin_reply_port(cmd.reply);
	return err;
}


status_t
BMediaRoster::GetFileFormatsFor(
	const media_node & file_interface, 
	media_file_format * out_formats,
	int32 * io_num_infos)
{
	iterate_file_formats_q cmd;
	iterate_file_formats_a ans;
	if ((file_interface.node <= 0) || (file_interface.port <= 0)) {
		return B_MEDIA_BAD_NODE;
	}
	if (out_formats == 0) return B_BAD_VALUE;
	if (io_num_infos == 0) return B_BAD_VALUE;
	cmd.reply = checkout_reply_port("GetFileFormatsFor");
	if (cmd.reply < 0) return cmd.reply;
	cmd.cookie = 0;
	status_t err = B_OK;
	int32 cnt = 0;
	while (true) {
		err = write_port_etc(file_interface.port, FI_ITERATE_FILE_FORMATS, 
			&cmd, sizeof(cmd), B_TIMEOUT, DEFAULT_TIMEOUT);
		if (err >= B_OK) {
			int32 code;
read_iterate_file_formats_again:
			err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), 
				B_TIMEOUT, DEFAULT_TIMEOUT);
			if ((err >= B_OK) && (code != FI_ITERATE_FILE_FORMATS_REPLY)) {
				dlog("stale reply: %x (expected %x); retrying", code, FI_ITERATE_FILE_FORMATS_REPLY);
				goto read_iterate_file_formats_again;	/* old reply from previous use? */
			}
			if (err >= B_OK) {
				cmd.cookie = ans.cookie;
			}
		}
		if ((err >= B_OK) && (cnt < *io_num_infos)) {
			if (ans.error < B_OK) {
				err = B_OK;
				break;
			}
			out_formats[cnt++] = ans.format;
		}
		else {
			break;
		}
	}
	checkin_reply_port(cmd.reply);
	if (err != B_TIMEOUT) {
		dispose_file_format_cookie_q cmd2;
		cmd2.cookie = cmd.cookie;
		(void)write_port_etc(file_interface.port, FI_DISPOSE_FILE_FORMAT_COOKIE, 
			&cmd2, sizeof(cmd2), B_TIMEOUT, DEFAULT_TIMEOUT);
	}
	*io_num_infos = cnt;
	return err > 0 ? 0 : err;
}


status_t
BMediaRoster::SetRefFor(
	const media_node & file_interface,
	const entry_ref & file,
	bool create_and_truncate,
	bigtime_t * out_length)		/* if create is false */
{
	specify_file_q cmd;
	specify_file_a ans;
	if (file_interface.node <= 0 || file_interface.port <= 0) {
		return B_MEDIA_BAD_NODE;
	}
	if (!(file_interface.kind & B_FILE_INTERFACE)) {
		return B_MEDIA_BAD_NODE;
	}
	cmd.reply = checkout_reply_port("SetRefFor");
	if (cmd.reply < 0) return cmd.reply;
	cmd.cookie = 0;
	status_t err = B_OK;
	cmd.create = create_and_truncate;
	cmd.device = file.device;
	cmd.directory = file.directory;
	strncpy(cmd.name, file.name, 256);
	cmd.name[255] = 0;
	err = write_port_etc(file_interface.port, FI_SPECIFY_FILE, &cmd, sizeof(cmd),
		B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK) {
		checkin_reply_port(cmd.reply);
		return err;
	}
	int32 code;
read_specify_file_again:
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if ((err >= B_OK) && (code != FI_SPECIFY_FILE_REPLY)) {
		dlog("stale reply: %x (expected %x); retrying", code, FI_SPECIFY_FILE_REPLY);
		goto read_specify_file_again;	/* old reply from previous use? */
	}
	checkin_reply_port(cmd.reply);
	if (err < B_OK) {
		return err;
	}
	if (out_length != 0) *out_length = ans.length;
	return B_OK;
}


status_t
BMediaRoster::GetRefFor(
	const media_node & node,
	entry_ref * out_file,
	BMimeType * out_mime_type)
{
	get_cur_file_q cmd;
	get_cur_file_a ans;
	status_t err = B_OK;
	if ((node.node <= 0) || (node.port <= 0)) return B_MEDIA_BAD_NODE;
	if (!(node.kind & B_FILE_INTERFACE)) return B_MEDIA_BAD_NODE;
	cmd.reply = checkout_reply_port("GetRefFor");
	if (cmd.reply < 0) return cmd.reply;
	cmd.cookie = 0;
	err = write_port_etc(node.port, FI_GET_CUR_FILE, &cmd, sizeof(cmd),
		B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK) {
		checkin_reply_port(cmd.reply);
		return err;
	}
	int32 code;
read_get_cur_file_again:
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), 
		B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		if (code != FI_GET_CUR_FILE_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, FI_GET_CUR_FILE_REPLY);
			goto read_get_cur_file_again;	/* old reply from previous use? */
		}
		err = ans.error;
	}
	if (err < B_OK) {
		checkin_reply_port(cmd.reply);
		return err;
	}
	if (out_mime_type) {
		ans.mime_type[255] = 0;
		out_mime_type->SetTo(ans.mime_type);
	}
	if (out_file) {
		out_file->device = ans.device;
		out_file->directory = ans.directory;
		out_file->set_name(ans.name);
	}
	return B_OK;
}


status_t
BMediaRoster::SniffRefFor(
	const media_node & file_interface,
	const entry_ref & file,
	BMimeType * mime_type_returned,
	float * out_capability)
{
	sniff_file_q cmd;
	sniff_file_a ans;
	if ((file_interface.node < 0) || (file_interface.port < 0)) {
		return B_MEDIA_BAD_NODE;
	}
	if (!(file_interface.kind & B_FILE_INTERFACE)) {
		return B_MEDIA_BAD_NODE;
	}
	cmd.reply = checkout_reply_port("SniffRefFor");
	if (cmd.reply < 0) return cmd.reply;
	cmd.cookie = 0;
	status_t err = B_OK;
	cmd.device = file.device;
	cmd.directory = file.directory;
	strncpy(cmd.name, file.name, 256);
	cmd.name[255] = 0;
	err = write_port_etc(file_interface.port, FI_SNIFF_FILE, &cmd, sizeof(cmd),
		B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK) {
		checkin_reply_port(cmd.reply);
		return err;
	}
	int32 code;
read_sniff_file_again:
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if ((err >= B_OK) && (code != FI_SNIFF_FILE_REPLY)) {
		dlog("stale reply: %x (expected %x); retrying", code, FI_SNIFF_FILE_REPLY);
		goto read_sniff_file_again;
	}
	checkin_reply_port(cmd.reply);
	if (err < B_OK) {
		return err;
	}
	if (mime_type_returned) {
		ans.mime_type[255] = 0;
		mime_type_returned->SetTo(ans.mime_type);
	}
	if (out_capability) {
		*out_capability = ans.quality;
	}
	return ans.error;
}


/* This is the generic "here's a file, now can someone please play it" interface */
status_t
BMediaRoster::SniffRef(
	const entry_ref & file,
	uint64 require_node_kinds,		/* if you need an EntityInterface or BufferConsumer or something */
	dormant_node_info * out_node,
	BMimeType * out_mime_type)
{
	if (!out_node) return B_BAD_VALUE;
	BMessage query(MEDIA_SNIFF_FILE);
	query.AddRef("refs", &file);
	require_node_kinds |= B_FILE_INTERFACE;
	query.AddInt64("be:kinds", require_node_kinds);
	BMessage answer;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &answer);
	const char * type;
	const dormant_node_info * node;
	int32 size;
	if (err >= B_OK) {
		err = answer.FindData("be:latent_sniffs", B_RAW_TYPE, (const void **)&node, &size);
	}
	if (err >= B_OK) {
		err = answer.FindString("be:sniff_mime", &type);
	}
	if (err >= B_OK) {
		memcpy(out_node, node, sizeof(*node));
		if (out_mime_type) {
			out_mime_type->SetTo(type);
		}
	}
	return err >= B_OK ? B_OK : err;
}


status_t
BMediaRoster::GetDormantNodeForType(
	const BMimeType & type,
	uint64 require_node_kinds,
	dormant_node_info * out_node)
{
	if (!out_node) return B_BAD_VALUE;
	BMessage query(MEDIA_SNIFF_FILE);
	query.AddString("be:mime_type", type.Type());
	query.AddInt64("be:kinds", require_node_kinds);
	BMessage answer;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &answer);
	const dormant_node_info * node;
	int32 size;
	if (err >= B_OK) {
		err = answer.FindData("be:type_sniffs", B_RAW_TYPE, (const void **)&node, &size);
	}
	if (err >= B_OK) {
		memcpy(out_node, node, sizeof(*node));
	}
	return err >= B_OK ? B_OK : err;
}


status_t
BMediaRoster::GetReadFileFormatsFor(
	const dormant_node_info & in_node,
	media_file_format * out_read_formats,
	int32 in_read_count,
	int32 * out_read_count)
{
	assert(out_read_formats != NULL);
	if (!out_read_formats) {
		return B_BAD_VALUE;
	}
	BMessage query(MEDIA_GET_DORMANT_FILE_FORMATS);
	query.AddData("be:dormant_node", B_RAW_TYPE, &in_node, sizeof(in_node));
	BMessage answer;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &answer);
	const media_file_format * out_formats;
	int32 count;
	type_code type;
	if (err >= B_OK) {
		err = answer.GetInfo("be:read_formats", &type, &count);
	}
	if (err >= B_OK) {
		if (out_read_count) {
			*out_read_count = count;
		}
		if (count > in_read_count) {
			count = in_read_count;
		}
		for (int ix=0; ix<count && err>=0; ix++) {
			ssize_t size;
			err = answer.FindData("be:read_formats", type, ix, (const void**)&out_formats, &size);
			if (err >= B_OK) {
				out_read_formats[ix] = *out_formats;
			}
			else if (out_read_count) {
				*out_read_count = ix;
			}
		}
	}
	return err > B_OK ? B_OK : err;
}


status_t
BMediaRoster::GetWriteFileFormatsFor(
	const dormant_node_info & in_node,
	media_file_format * out_write_formats,
	int32 in_write_count,
	int32 * out_write_count)
{
	assert(out_write_formats != NULL);
	if (!out_write_formats) {
		return B_BAD_VALUE;
	}
	BMessage query(MEDIA_GET_DORMANT_FILE_FORMATS);
	query.AddData("be:dormant_node", B_RAW_TYPE, &in_node, sizeof(in_node));
	BMessage answer;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &answer);
	const media_file_format * out_formats;
	int32 count;
	type_code type;
	if (err >= B_OK) {
		err = answer.GetInfo("be:write_formats", &type, &count);
	}
	if (err >= B_OK) {
		if (out_write_count) {
			*out_write_count = count;
		}
		if (count > in_write_count) {
			count = in_write_count;
		}
		for (int ix=0; ix<count && err>=0; ix++) {
			ssize_t size;
			err = answer.FindData("be:write_formats", type, ix, (const void**)&out_formats, &size);
			if (err >= B_OK) {
				out_write_formats[ix] = *out_formats;
			}
			else if (out_write_count) {
				*out_write_count = ix;
			}
		}
	}
	return err > B_OK ? B_OK : err;
}


ssize_t
BMediaRoster::GetNodeAttributesFor(
	const media_node & node,
	media_node_attribute * outArray,
	size_t inMaxCount)
{
	if ((inMaxCount != 0) && !outArray) return B_BAD_VALUE;
	if ((node.node <= 0) || (node.port <= 0)) return B_MEDIA_BAD_NODE;
	char buf[B_MEDIA_MESSAGE_SIZE];
	get_attributes_q cmd;
	cmd.reply = checkout_reply_port("GetNodeAttributesFor");
	cmd.cookie = find_thread(NULL);
	status_t err = write_port_etc(node.port, M_GET_ATTRIBUTES, &cmd, sizeof(cmd), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK)
	{
badluck:
		checkin_reply_port(cmd.reply);
		return err;
	}
	int32 code;
read_attributes_again:
	code = 0;
	err = read_port_etc(cmd.reply, &code, buf, B_MEDIA_MESSAGE_SIZE, B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK)
		goto badluck;
	if (code != M_GET_ATTRIBUTES_REPLY)
		goto read_attributes_again;
	checkin_reply_port(cmd.reply);
	get_attributes_a & ans(*(get_attributes_a *)buf);
	if (ans.error < 0) return ans.error;
	if (inMaxCount > ans.error) inMaxCount = ans.error;
	memcpy(outArray, &(&ans)[1], sizeof(media_node_attribute)*inMaxCount);
	return ans.error; 
}

status_t
BMediaRoster::GetFormatFor(
	const media_output & output,
	media_format * io_format,
	uint32)
{
	if (io_format == 0) return B_BAD_VALUE;
	if (!(output.node.kind & B_BUFFER_PRODUCER)) return B_MEDIA_BAD_NODE;
	if (output.source == media_source::null) return B_MEDIA_BAD_SOURCE;
	port_id use_port = checkout_reply_port("GetFormatFor(output)");
	if (use_port < B_OK) return use_port;
	sem_id toRelease = -1;
	status_t error = B_OK;
	propose_format_q cmd;
	cmd.reply = use_port;
	cmd.cookie = find_thread(NULL);
	cmd.output = output.source;
	cmd.format = *io_format;
	error = write_port(output.source.port, BP_PROPOSE_FORMAT, &cmd, sizeof(cmd));
	if (error < B_OK) {
		checkin_reply_port(use_port);
		return error;
	}
	int32 code = 0;
	propose_format_a reply;
format_again:
	error = _read_port_etc_sync(use_port, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT*5, &toRelease);
	if ((error > 0) && (code != BP_PROPOSE_FORMAT_REPLY)) goto format_again;
	if (error >= B_OK) {
		(void)reply.format.MetaData();
		release_sem(toRelease);
		error = reply.error;
	}
	*io_format = reply.format;
	if (error >= B_OK) {
		error = B_OK;
	}
	checkin_reply_port(use_port);
	return error;
}

status_t
BMediaRoster::GetFormatFor(
	const media_input & input,
	media_format * io_format,
	uint32)
{
	if (io_format == 0) return B_BAD_VALUE;
	if (!(input.node.kind & B_BUFFER_CONSUMER)) return B_MEDIA_BAD_NODE;
	if (input.destination == media_destination::null) return B_MEDIA_BAD_DESTINATION;
	port_id use_port = checkout_reply_port("GetFormatFor(input)");
	if (use_port < B_OK) return use_port;
	sem_id toRelease = -1;
	accept_format_q acc;
	acc.reply = use_port;
	acc.input = input.destination;
	acc.format = *io_format;
	status_t error = write_port_etc(input.destination.port, BC_ACCEPT_FORMAT, &acc, sizeof(acc), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (error < B_OK) {
		checkin_reply_port(use_port);
		return error;
	}
	accept_format_a rep2;
	int32 code = 0;
accept_again:
	error = _read_port_etc_sync(use_port, &code, &rep2, sizeof(rep2), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease);
	if ((error > 0) && (code != BC_ACCEPT_FORMAT_REPLY)) goto accept_again;
	if (error >= B_OK) {
		(void)rep2.format.MetaData();
		release_sem(toRelease);
		error = rep2.error;
	}
	*io_format = rep2.format;
	if (error >= B_OK) {
		error = B_OK;
	}
	checkin_reply_port(use_port);
	return error;
}

status_t
BMediaRoster::GetFormatFor(
	const media_node & node,
	media_format * io_format,
	float quality)
{
	if (!io_format) return B_BAD_VALUE;
	if (!(node.kind & B_BUFFER_PRODUCER)) return B_MEDIA_BAD_NODE;
	suggest_format_q q;
	suggest_format_a a;
	int32 code;
	port_id port = checkout_reply_port("GetFormatFor(node)");
	if (port < B_OK) return port;
	q.reply = port;
	q.cookie = find_thread(NULL);
	q.format = *io_format;
	q.type = io_format->type;
	q.quality = (int32)(quality*100);
	status_t err = write_port_etc(node.port, BP_SUGGEST_FORMAT,
			&q, sizeof(q), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err < B_OK) {
		checkin_reply_port(port);
		return err;
	}
	sem_id to_release;
	while ((err = _read_port_etc_sync(q.reply, &code, &a, sizeof(a), B_TIMEOUT, DEFAULT_TIMEOUT*5, &to_release)) > 0) {
		if (code != BP_SUGGEST_FORMAT_REPLY) {
DIAGNOSTIC(stderr, "Stale reply code 0x%x received in GetFormatFor()\n", code);
			continue;
		}
		memcpy(io_format, &a.format, sizeof(media_format));
		(void)(io_format->MetaData());
		status_t r2 = release_sem(to_release);
		break;
	}
	if (!err) err = B_ERROR;
	if (err > 0) err = B_OK;
	checkin_reply_port(port);
	return err;
}


media_node_id
BMediaRoster::NodeIDFor(
	port_id port)
{
	if (port < 1) return B_BAD_PORT_ID;
	BMessage msg(MEDIA_GET_NODE_ID);
	status_t err = msg.AddInt32("be:port", port);
	if (err < B_OK) return err;
	BMessage reply;
	err = ((_BMediaRosterP *)_sDefault)->Turnaround(&msg, &reply);
	if (err < B_OK) return err;
	int32 node = 0;
	err = reply.FindInt32("be:node_id", &node);
	if (err < B_OK) return err;
	return (node > 0) ? node : B_ERROR;
}


status_t 
BMediaRoster::GetInstancesFor(media_addon_id addon, int32 flavor, media_node_id *out_id, int32 * io_size)
{
	BMessage msg(MEDIA_FIND_RUNNING_INSTANCES);
	int32 cnt = 1;
	if (io_size == 0) io_size = &cnt;
	msg.AddInt32("be:_addon_id", addon);
	msg.AddInt32("be:_flavor_id", flavor);
	BMessage reply;
	status_t err = ((_BMediaRosterP *)BMediaRoster::_sDefault)->Turnaround(&msg, &reply);
	if (err == B_OK) {
		int32 cnt = 0;
		type_code type;
		reply.GetInfo("be:_node_id", &type, &cnt);
		if (*io_size > cnt) *io_size = cnt;
		for (int ix=0; ix<*io_size; ix++) {
			err = reply.FindInt32("be:_node_id", ix, &out_id[ix]);
			if (err < B_OK) {
				break;
			}
		}
		*io_size = cnt;
	}
	return err;
}

status_t
BMediaRoster::StartNode(
	const media_node & node,
	bigtime_t at_time)
{

#if 0
	// this is no longer needed as it is handled properly inside of StartTimeSource
	if ((node.port == B_BAD_VALUE) && (node.kind & B_TIME_SOURCE)) {
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectStart(at_time);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;
	} else if (node.port < 0) {
		return B_MEDIA_BAD_NODE;
	}
#endif

	if ((node.port <= 0) || (node.node <= 0)) return B_MEDIA_BAD_NODE;
		
	start_q cmd;
	cmd.performance_time = at_time;
	status_t error = write_port(node.port, M_START, &cmd, sizeof(cmd));
	if (error > 0) {
		error = B_OK;
	}
	return error;
}


status_t
BMediaRoster::StopNode(
	const media_node & node,
	bigtime_t at_time,
	bool immediate)
{
	dlog("StopNode(%d)", node.node);
#if 0
	// this is no longer needed as it is handled inside of StopTimeSource
	if ((node.port == B_BAD_VALUE) && (node.kind & B_TIME_SOURCE)) {
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectStop(at_time,immediate);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;
	} else if (node.port < 0) {
		return B_MEDIA_BAD_NODE;
	}
#endif
	if ((node.port <= 0) || (node.node <= 0)) return B_MEDIA_BAD_NODE;

	stop_q cmd;
	cmd.performance_time = at_time;
	cmd.flags = (immediate ? stop_q::STOP_SYNC : 0);
	cmd.reply = -1;
	if (immediate) {
		cmd.reply = checkout_reply_port("StopNode");
		cmd.cookie = find_thread(NULL);
	}
	status_t error = write_port(node.port, M_STOP, &cmd, sizeof(cmd));
	if (error >= 0) {
		if (immediate) {
			int32 code;
			stop_a ans;
			while ((error = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), 
				B_TIMEOUT, DEFAULT_TIMEOUT)) > 0) {
				if (code == M_STOP_REPLY) {
					error = ans.error;
					break;
				}
				dlog("Stale reply %x in StopNode(): retrying");
			}
		}
		if (error > 0) {
			error = 0;
		}
	}
	else {
		dlog("write_port() in StopNode() failed: %x", error);
	}
	if (cmd.reply > -1) {
		checkin_reply_port(cmd.reply);
	}
	return error;
}

status_t 
BMediaRoster::StartTimeSource(
	const media_node &node,
	bigtime_t at_real_time)
{
	/* this only works on time source nodes! */
	if (!(node.kind & B_TIME_SOURCE))
		return B_MEDIA_BAD_NODE;

	if (node.port == B_BAD_VALUE)
	{
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectStart(at_real_time);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;		
	}
	else if (node.port <= 0)
		return B_MEDIA_BAD_NODE;
		
	ts_op_q cmd;
	cmd.op = BTimeSource::B_TIMESOURCE_START;
	cmd.real_time = at_real_time;
	cmd.performance_time = 0;
	
	status_t error = write_port(node.port, TS_OP, &cmd, sizeof(cmd));
	if (error > 0)
		error = B_OK;
	return error;

}

status_t 
BMediaRoster::StopTimeSource(
	const media_node &node,
	bigtime_t at_real_time,
	bool immediate)
{
	/* this only works on time source nodes! */
	if (!(node.kind & B_TIME_SOURCE))
		return B_MEDIA_BAD_NODE;

	if (node.port == B_BAD_VALUE)
	{
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectStop(at_real_time,immediate);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;
	}
	else if (node.port < 0)
		return B_MEDIA_BAD_NODE;
		
	ts_op_q cmd;
	cmd.op = (immediate ? BTimeSource::B_TIMESOURCE_STOP_IMMEDIATELY : BTimeSource::B_TIMESOURCE_STOP);
	cmd.real_time = at_real_time;
	cmd.performance_time = 0;
	cmd.reply = (immediate ? checkout_reply_port("StopTimeSource") : -1);
	
	status_t error = write_port(node.port, TS_OP, &cmd, sizeof(cmd));

	if (error >= 0) {
		if (immediate) {
			int32 code;
			ts_op_a ans;
			while ((error = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), 
				B_TIMEOUT, DEFAULT_TIMEOUT)) > 0) {
				if (code == TS_OP_REPLY) {
					error = ans.error;
					break;
				}
			}
		}
		if (error > 0) {
			error = 0;
		}
	}
	if (cmd.reply > -1) {
		checkin_reply_port(cmd.reply);
	}
	return error;
}

status_t 
BMediaRoster::SeekTimeSource(
	const media_node &node,
	bigtime_t to_performance_time,
	bigtime_t at_real_time)
{
	/* this only works on time source nodes! */
	if (!(node.kind & B_TIME_SOURCE))
		return B_MEDIA_BAD_NODE;

	if (node.port == B_BAD_VALUE) {
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectSeek(at_real_time,to_performance_time);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;
	}
	else if (node.port <= 0)
		return B_MEDIA_BAD_NODE;
		
	ts_op_q cmd;
	cmd.op = BTimeSource::B_TIMESOURCE_SEEK;
	cmd.real_time = at_real_time;
	cmd.performance_time = to_performance_time;
	
	status_t error = write_port(node.port, TS_OP, &cmd, sizeof(cmd));
	if (error > 0)
		error = B_OK;
	return error;
}

status_t
BMediaRoster::SyncToNode(
	const media_node & node,
	bigtime_t at_time,
	bigtime_t timeout)
{
	dlog("SyncToNode(%d)", node.node);
	if (node.port <= 0) return B_MEDIA_BAD_NODE;

	sync_q cmd;
	cmd.performance_time = at_time;
	//	we depend on this being a unique port in NODE_DELETED
	//	cleaning up and releasing waiting syncs
	cmd.reply = create_port(1, "SyncToNode Reply");
	cmd.cookie = find_thread(NULL);
	((_BMediaRosterP *)_sDefault)->_RegisterSync(node, cmd);
	status_t error = write_port(node.port, M_SYNC, &cmd, sizeof(cmd));
	if (error >= 0) {
		int32 code;
		sync_a ans;
		while ((error = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, timeout)) > 0) {
			if (code == M_SYNC_REPLY) {
				error = ans.error;
				break;
			}
			dlog("Stale reply %x in SyncToNode(): retrying");
		}
		if (error > 0) error = 0;
	}
	else {
		dlog("write_port() in SyncToNode() failed: %x", error);
	}
	((_BMediaRosterP *)_sDefault)->_CancelSync(node, cmd);
	if (cmd.reply > -1) {
		delete_port(cmd.reply);
	}
	return error;
}


status_t
BMediaRoster::SeekNode(
	const media_node & node,
	bigtime_t to_time,
	bigtime_t at_time)	/* if already running */
{
#if 0
	// this is no longer needed as it is handled in SeekTimeSource
	if ((node.port == B_BAD_VALUE) && (node.kind & B_TIME_SOURCE)) {
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectSeek(at_time,to_time);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;
	} else if (node.port < 0) {
		return B_MEDIA_BAD_NODE;
	}
#endif
	if (node.port <= 0)
		return B_MEDIA_BAD_NODE;

	seek_q cmd;
	cmd.media_time = to_time;
	cmd.performance_time = at_time;
	status_t error = write_port(node.port, M_SEEK, &cmd, sizeof(cmd));
	if (error > 0) {
		error = B_OK;
	}
	return error;
}


status_t
BMediaRoster::SetRunModeNode(
	const media_node & node,
	BMediaNode::run_mode mode)
{
	if ((node.port == B_BAD_VALUE) && (node.kind & B_TIME_SOURCE)) {
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectSetRunMode(mode);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;
	}
	else if (node.port <= 0) {
		return B_MEDIA_BAD_NODE;
	}
	set_run_mode_q cmd;
	cmd.mode = mode;
	cmd.delay = 0;
	status_t error = write_port(node.port, M_SET_RUN_MODE, &cmd, sizeof(cmd));
	if (error > 0) {
		error = B_OK;
	}
	return error;
}


status_t
BMediaRoster::RollNode(
	const media_node & node,
	bigtime_t startPerformance,
	bigtime_t stopPerformance,
	bigtime_t mediaTime)
{
	if ((node.port == B_BAD_VALUE) && (node.kind & B_TIME_SOURCE)) {
		DIAGNOSTIC(stderr, "media kit warning: RollNode() called on system_time() time source\n");
		return B_MEDIA_BAD_NODE;
	}
	else if (node.port <= 0) {
		return B_MEDIA_BAD_NODE;
	}
	roll_q cmd;
	cmd.start = startPerformance;
	cmd.stop = stopPerformance;
	cmd.media = mediaTime;
	status_t error = write_port(node.port, M_ROLL, &cmd, sizeof(cmd));
	if (error > 0) {
		error = B_OK;
	}
	return error;
}


status_t
BMediaRoster::SetProducerRunModeDelay(
	const media_node & node,
	bigtime_t delay, 
	BMediaNode::run_mode mode)
{
	if (!(node.kind & B_BUFFER_PRODUCER))
		return B_MEDIA_BAD_NODE;
	if ((node.port == B_BAD_VALUE) && (node.kind & B_TIME_SOURCE)) {
		DIAGNOSTIC(stderr, "media kit warning: system_time() time source is a producer?\n");
		if (BTimeSource *ts=((_BMediaRosterP *)_sDefault)->GetSysTimeSrcClone(node.node)) {
			ts->DirectSetRunMode(mode);
			ts->Release();
			return B_OK;
		};
		return B_MEDIA_BAD_NODE;
	}
	else if (node.port <= 0) {
		return B_MEDIA_BAD_NODE;
	}
	set_run_mode_q cmd;
	cmd.mode = mode;
	cmd.delay = delay;
	status_t error = write_port(node.port, M_SET_RUN_MODE, &cmd, sizeof(cmd));
	if (error > 0) {
		error = B_OK;
	}
	return error;
}


status_t
BMediaRoster::PrerollNode(
	const media_node & node)
{
	if (node.port <= 0) {
		return B_MEDIA_BAD_NODE;
	}
	preroll_q cmd;
	/* Why not checkout_reply_port? Well, we're using the linear port_id as a token */
	cmd.reply = create_port(1, "Preroll reply");
	cmd.cookie = cmd.reply;
	status_t err = write_port(node.port, M_PREROLL, &cmd, sizeof(cmd));
	if (err < 0) return err;
	int32 code;
	preroll_a ans;
read_preroll_again:
	err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err > 0) err = B_OK;
	if ((err >= B_OK) && (code != M_PREROLL_REPLY)) {
		dlog("stale reply: %x (expected %x); retrying", code, M_PREROLL_REPLY);
		goto read_preroll_again;
	}
	delete_port(cmd.reply);
	return err;
}


status_t
BMediaRoster::SetProducerRate(
	const media_node & producer,
	int32 numer,
	int32 denom)
{
	if (producer.port <= 0) {
		return B_MEDIA_BAD_NODE;
	}
	if (!(producer.kind & B_BUFFER_PRODUCER)) {
		return B_MEDIA_BAD_NODE;
	}
	set_rate_q cmd;
	cmd.reply = checkout_reply_port("SetProducerRate");
	cmd.cookie = find_thread(NULL);
	cmd.numer = numer;
	cmd.denom = denom;
	status_t err = write_port_etc(producer.port, BP_SET_RATE, &cmd, sizeof(cmd), B_TIMEOUT, DEFAULT_TIMEOUT);
	if (err >= B_OK) {
		set_rate_a ans;
		int32 code = 0;
read_set_rate_again:
		err = read_port_etc(cmd.reply, &code, &ans, sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
		if (err >= B_OK) {
			if (code != BP_SET_RATE_REPLY) {
				dlog("stale reply: %x (expected %x); retrying", code, BP_SET_RATE_REPLY);
				goto read_set_rate_again;
			}
			err = ans.error;
		}
	}
	checkin_reply_port(cmd.reply);
	return err;
}




#if 0
	static bool is_sync_cmd(int32 command, int32 * reply_cmd)
	{
		//	At one point we tried to make set_buffers sync
		//	it fails because there's a race between sending
		//	buffers in Bt848producer and handling the message
		//	when establishing a second connection to the
		//	videowindow, which shows that the attempt was
		//	mis-guided.
		if (command == BP_SET_BUFFERS)
		{
			*reply_cmd = BP_SET_BUFFERS_REPLY;
			return true;
		}
		return false;
	}
#endif

status_t
BMediaRoster::ParseCommand(
	BMessage & reply)
{
	status_t error = B_ERROR;
	const void * data = NULL;
	ssize_t size = 0;
	int32 command = 0;
	int32 port = -1;
	if (!reply.FindInt32("command", &command) && 
		!reply.FindData("data", B_RAW_TYPE, (const void **)&data, &size)) {
		status_t ret = B_OK;
		int ix = 0;
		error = B_OK;
#if 0
		int32 sync_cmd = 0;
		bool is_sync = is_sync_cmd(command, &sync_cmd);
		if (is_sync)
		{
			//	the offset of these is always the same for sync commands
			((sync_cmd_q *)data)->reply = ((_BMediaRosterP *)BMediaRoster::_sDefault)->checkout_reply_port("ParseCommand");
			((sync_cmd_q *)data)->cookie = find_thread(NULL);
		}
#endif
		while (!reply.FindInt32("port", ix, &port))
		{
			dlog("ParseCommand write_port(%d, %x)", port, command);
			ret = write_port(port, command, data, size);
			if (ret < 0)
			{
				error = ret;
			}
#if 0
			if (is_sync)
			{
				int32 out_cmd;
				//	the layout of the beginning of all sync answers is the same
				sync_cmd_a ans;
	read_sync_again:
				ret = read_port_etc(((sync_cmd_q *)data)->reply, &out_cmd, &ans,
					sizeof(ans), B_TIMEOUT, DEFAULT_TIMEOUT);
				if (ret >= 0)
				{
					if (out_cmd != sync_cmd)
					{
						DIAGNOSTIC(stderr, "ParseCommand read_port() receives %x, expected %x\n",
							out_cmd, sync_cmd);
						goto read_sync_again;
					}
					error = ((sync_cmd_a *)data)->error;
				}
				else
				{
					error = ret;
				}
			}
#endif
			ix++;
		}
#if 0
		if (is_sync)
		{
			((_BMediaRosterP *)BMediaRoster::_sDefault)->checkin_reply_port(((set_buffers_q *)data)->reply);
		}
#endif
	}
	return error;
}


port_id
BMediaRoster::checkout_reply_port(
	const char * name)
{
#if !UNIQUE_REPLY_PORTS
	if (atomic_or(&_mReplyPortRes, 1) == 0)
		return _mReplyPort;
#if DEBUG
	if (atomic_add(&_mReplyPortUnavailCount, 1) == 8) {
		DIAGNOSTIC(stderr, "It seems likely that there is an unbalanced call to checkout_reply_port()\n");
	}
#endif
#endif
	return create_port(1, name ? name : "Contention for reply_port");
}


void
BMediaRoster::checkin_reply_port(
	port_id port)
{
	if (_mReplyPort == port) {
		atomic_and(&_mReplyPortUnavailCount, 0);
		while (!dcheck(port_count(port) <= 0)) {
			int32 code = 0;
			read_port_etc(port, &code, NULL, 0, B_TIMEOUT, 1);
			dlog("extra reply code: %x", code);
		}
		atomic_and(&_mReplyPortRes, ~1);
	}
	else {
		delete_port(port);	/* temporary contention port */
	}
}

status_t
BMediaRoster::GetLiveNodeInfo(
	const media_node & node,
	live_node_info * info)
{
	if (!info) return B_BAD_VALUE;
	if ((node.node <= 0) || (node.port <= 0)) return B_MEDIA_BAD_NODE;
	BMessage query(MEDIA_QUERY_NODES);
	BMessage reply;
	query.AddInt32("be:node", node.node);
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &reply);
	const live_node_info * out = NULL;
	if (error >= B_OK) {
		int32 size;
		error = reply.FindData("live_nodes", B_RAW_TYPE, (const void **)&out, &size);
	}
	if (error >= B_OK) {
		*info = *out;
		error = B_OK;
	}
	return error;
}

/* If there are no nodes (unlikely in real life) this will return B_NAME_NOT_FOUND */
status_t
BMediaRoster::GetLiveNodes(
	live_node_info * out_live_nodes,
	int32 * io_count,
	const media_format * has_input,
	const media_format * has_output,
	const char * name,
	uint64 node_kinds)
{
	if (!io_count) return B_BAD_VALUE;
	if (*io_count && !out_live_nodes) return B_BAD_VALUE;
	int32 buf_num_nodes = *io_count;
	*io_count = 0;
	BMessage query(MEDIA_QUERY_NODES);
	BMessage replyNodes;
	query.AddInt32("be:max_hits", *io_count);
	if (has_input != NULL) query.AddData("be:input_format", B_RAW_TYPE, 
		has_input, sizeof(*has_input));
	if (has_output != NULL) query.AddData("be:output_format", B_RAW_TYPE, 
		has_output, sizeof(*has_output));
	if (name != NULL) query.AddString("be:name", name);
	if (node_kinds != 0) query.AddInt32("be:node_kinds", node_kinds);
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &replyNodes);
	int32 count = -1;
	type_code type = (type_code)-1;
	if (error == B_OK)
	{
		error = replyNodes.GetInfo("live_nodes", &type, &count);
	}
	if (error == B_OK)
	{
		*io_count = count;
		if (count > buf_num_nodes)
		{
			count = buf_num_nodes;
		}
		for (int ix=0; (ix<count) && (error==B_OK); ix++)
		{
			live_node_info * out_ptr;
			ssize_t size;
			error = replyNodes.FindData("live_nodes", type, ix, (const void **)&out_ptr, &size);
			if (error == B_OK)
			{
				out_live_nodes[ix] = *out_ptr;
			}
		}
	}
	return error;
}

status_t
BMediaRoster::GetFreeInputsFor(
	const media_node & node,
	media_input * out_free_inputs,
	int32 buf_num_inputs,
	int32 * out_total_count,
	media_type filter_type)
{
	if (!out_total_count) return B_BAD_VALUE;
	if (buf_num_inputs && !out_free_inputs) return B_BAD_VALUE;
	sem_id toRelease;
	*out_total_count = 0;
	if ((node.node <= 0) || (node.port <= 0) || !(node.kind & B_BUFFER_CONSUMER)) {
		dlog("GetFreeInputsFor(%d) returns B_MEDIA_BAD_NODE", node.node);
		return B_MEDIA_BAD_NODE;
	}
	/* free inputs are inputs that return null for source */
	iterate_inputs_q cmd;
	iterate_inputs_a reply;
	port_id p = checkout_reply_port("GetFreeInputsFor");
	if (p < 0)
	{
		dlog("checkout_reply_port() failed %x %s", p, strerror(p));
		return p;
	}
	status_t err = B_OK;
	cmd.reply = p;
	cmd.cookie = 0;
	int32 oc = 0;
	while ((err = write_port_etc(node.port, BC_ITERATE_INPUTS, &cmd, sizeof(cmd), 
		B_TIMEOUT, DEFAULT_TIMEOUT)) >= 0) {
		int32 code;
read_iterate_inputs_again:
		if ((err = _read_port_etc_sync(p, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease)) < 0) break;
		reply.input.format.MetaData();
		release_sem(toRelease);
		if (code != BC_ITERATE_INPUTS_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BC_ITERATE_INPUTS_REPLY);
			goto read_iterate_inputs_again;
		}
		cmd.cookie = reply.cookie;
		if (reply.error != B_OK) {
			break;
		}
		/* only provide input if there's space for it */
		if ((oc < buf_num_inputs) && (reply.input.source == media_source::null) &&
			(!filter_type || !reply.input.format.type || (filter_type == reply.input.format.type))) {
			dlog("returning %d: %d,%d", oc, reply.input.destination.port, reply.input.destination.id);
			out_free_inputs[oc] = reply.input;
			oc++;
		}
#if DEBUG
		else {
			if (oc >= buf_num_inputs) {
				dlog("ran out of available inputs");
			}
			else {
				dlog("mismatch; filter_type %x; input type %x; %s", 
					filter_type, reply.input.format.type, 
					(reply.input.source == media_source::null) ? 
					"not connected" : "already connected");
			}
		}
#endif
	}
	dispose_cookie_q cmd2;
	cmd2.cookie = cmd.cookie;
	/* If we time out with a valid cookie, there might be a leak in the target node. Oh, well. */
	if (err < B_OK) {
		dlog("write_port_etc(%d) read_port_etc(%d) failed in GetFreeInputsFor(): %x", 
			node.port, p, err);
	}
	else {
		(void)write_port_etc(node.port, BC_DISPOSE_COOKIE, &cmd2, sizeof(cmd2), 
			B_TIMEOUT, DEFAULT_TIMEOUT);
		dlog("GetFreeInputsFor returns %d items", oc);
	}
	checkin_reply_port(p);
	*out_total_count = oc;
	if (err > 0) {
		err = 0;
	}
	return err;
}


status_t
BMediaRoster::GetConnectedInputsFor(
	const media_node & node,
	media_input * out_active_inputs,
	int32 buf_num_inputs,
	int32 * out_total_count)
{
	if (!out_total_count) return B_BAD_VALUE;
	if (buf_num_inputs && !out_active_inputs) return B_BAD_VALUE;
	*out_total_count = 0;
	if ((node.node <= 0) || (node.port <= 0) || !(node.kind & B_BUFFER_CONSUMER)) {
		dlog("GetConnectedInputsFor(%d) returns B_MEDIA_BAD_NODE", node.node);
		return B_MEDIA_BAD_NODE;
	}
#if 0
	BMessage queryInputs(MEDIA_QUERY_INPUTS);
	queryInputs.AddInt32("media_node_id", node.node);
	BMessage replyInputs;
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&queryInputs, &replyInputs);
	int32 count = -1;
	type_code type = (type_code)-1;
	if (error == B_OK)
	{
		error = replyInputs.GetInfo("node_inputs", &type, &count);
	}
	if (error == B_OK)
	{
		*out_total_count = count;
		if (count > buf_num_inputs)
		{
			count = buf_num_inputs;
		}
		for (int ix=0; (ix<count) && (error==B_OK); ix++)
		{
			media_input * out_ptr;
			ssize_t size;
			error = replyInputs.FindData("node_inputs", type, ix, (const void **)&out_ptr, &size);
			if (error == B_OK)
			{
				out_active_inputs[ix] = *out_ptr;
			}
		}
	}
#endif
	/* connected inputs are inputs that don't return null for source */
	iterate_inputs_q cmd;
	iterate_inputs_a reply;
	port_id p = checkout_reply_port("GetConnectedInputsFor");
	if (p < 0)
	{
		dlog("checkout_reply_port() failed %x %s", p, strerror(p));
		return p;
	}
	status_t err = B_OK;
	cmd.reply = p;
	cmd.cookie = 0;
	int32 oc = 0;
	while ((err = write_port_etc(node.port, BC_ITERATE_INPUTS, &cmd, sizeof(cmd), 
		B_TIMEOUT, DEFAULT_TIMEOUT)) >= 0) {
		int32 code;
read_iterate_inputs_again:
		if ((err = _read_port_etc_sync(p, &code, &reply, sizeof(reply), 
			B_TIMEOUT, DEFAULT_TIMEOUT, NULL)) < 0) {
			break;
		}
		if (code != BC_ITERATE_INPUTS_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BC_ITERATE_INPUTS_REPLY);
			goto read_iterate_inputs_again;
		}
		cmd.cookie = reply.cookie;
		if (reply.error != B_OK) {
			break;
		}
		/* only provide input if there's space for it */
		if ((oc < buf_num_inputs) && !(reply.input.source == media_source::null))
		{
			dlog("returning %d: %d,%d", oc, reply.input.destination.port, reply.input.destination.id);
			out_active_inputs[oc] = reply.input;
			oc++;
		}
#if DEBUG
		else if (oc >= buf_num_inputs)
		{
			dlog("ran out of available inputs");
			break;
		}
#endif
	}
	dispose_cookie_q cmd2;
	cmd2.cookie = cmd.cookie;
	/* If we time out with a valid cookie, there might be a leak in the target node. Oh, well. */
	if (err < B_OK) {
		dlog("write_port_etc(%d) read_port_etc(%d) failed in GetConnectedInputsFor(): %x", 
			node.port, p, err);
	}
	else {
		(void)write_port_etc(node.port, BC_DISPOSE_COOKIE, &cmd2, sizeof(cmd2), 
			B_TIMEOUT, DEFAULT_TIMEOUT);
		dlog("GetConnectedInputsFor returns %d items", oc);
	}
	checkin_reply_port(p);
	*out_total_count = oc;
	if (err > 0) {
		err = 0;
	}
	return err;
}


status_t 
BMediaRoster::GetAllInputsFor(const media_node &node, media_input *out_inputs, int32 buf_num_inputs, int32 *out_total_count)
{
	if (!out_total_count) return B_BAD_VALUE;
	if (buf_num_inputs && !out_inputs) return B_BAD_VALUE;
	*out_total_count = 0;
	if ((node.node <= 0) || (node.port <= 0) || !(node.kind & B_BUFFER_CONSUMER)) {
		dlog("GetConnectedInputsFor(%d) returns B_MEDIA_BAD_NODE", node.node);
		return B_MEDIA_BAD_NODE;
	}
	/* connected inputs are inputs that don't return null for source */
	iterate_inputs_q cmd;
	iterate_inputs_a reply;
	port_id p = checkout_reply_port("GetAllInputsFor");
	if (p < 0)
	{
		dlog("checkout_reply_port() failed %x %s", p, strerror(p));
		return p;
	}
	status_t err = B_OK;
	cmd.reply = p;
	cmd.cookie = 0;
	int32 oc = 0;
	while ((err = write_port_etc(node.port, BC_ITERATE_INPUTS, &cmd, sizeof(cmd), 
		B_TIMEOUT, DEFAULT_TIMEOUT)) >= 0) {
		int32 code;
read_iterate_inputs_again:
		if ((err = _read_port_etc_sync(p, &code, &reply, sizeof(reply), 
			B_TIMEOUT, DEFAULT_TIMEOUT, NULL)) < 0) {
			break;
		}
		if (code != BC_ITERATE_INPUTS_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BC_ITERATE_INPUTS_REPLY);
			goto read_iterate_inputs_again;
		}
		cmd.cookie = reply.cookie;
		if (reply.error != B_OK) {
			break;
		}
		/* only provide input if there's space for it */
		if (oc < buf_num_inputs)
		{
			dlog("returning %d: %d,%d", oc, reply.input.destination.port, reply.input.destination.id);
			out_inputs[oc] = reply.input;
			oc++;
		}
#if DEBUG
		else if (oc >= buf_num_inputs)
		{
			dlog("ran out of available inputs");
			break;
		}
#endif
	}
	dispose_cookie_q cmd2;
	cmd2.cookie = cmd.cookie;
	/* If we time out with a valid cookie, there might be a leak in the target node. Oh, well. */
	if (err < B_OK) {
		dlog("write_port_etc(%d) read_port_etc(%d) failed in GetAllInputsFor(): %x", 
			node.port, p, err);
	}
	else {
		(void)write_port_etc(node.port, BC_DISPOSE_COOKIE, &cmd2, sizeof(cmd2), 
			B_TIMEOUT, DEFAULT_TIMEOUT);
		dlog("GetAllInputsFor returns %d items", oc);
	}
	checkin_reply_port(p);
	*out_total_count = oc;
	if (err > 0) {
		err = 0;
	}
	return err;
}


status_t
BMediaRoster::GetFreeOutputsFor(
	const media_node & node,
	media_output * out_free_outputs,
	int32 buf_num_outputs,
	int32 * out_total_count,
	media_type filter_type)
{
	if (!out_total_count) return B_BAD_VALUE;
	if (buf_num_outputs && !out_free_outputs) return B_BAD_VALUE;
	sem_id toRelease;
	*out_total_count = 0;
	if ((node.node <= 0) || (node.port <= 0) || !(node.kind & B_BUFFER_PRODUCER)) {
		dlog("GetFreeOutputsFor(%d) returns B_MEDIA_BAD_NODE", node.node);
		return B_MEDIA_BAD_NODE;
	}
	/* free outputs are outputs that return null for source */
	iterate_outputs_q cmd;
	iterate_outputs_a reply;
	port_id p = checkout_reply_port("GetFreeOutputsFor");
	if (p < 0)
	{
		dlog("checkout_reply_port() failed in GetFreeOutputsFor: %x %s", p, strerror(p));
		return p;
	}
	status_t err = B_OK;
	cmd.reply = p;
	cmd.cookie = 0;
	int32 oc = 0;
	while ((err = write_port_etc(node.port, BP_ITERATE_OUTPUTS, &cmd, sizeof(cmd), 
		B_TIMEOUT, DEFAULT_TIMEOUT)) >= 0) {
		int32 code;
read_iterate_outputs_again:
		if ((err = _read_port_etc_sync(p, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease)) < 0) break;
		reply.output.format.MetaData();
		release_sem(toRelease);
		if (code != BP_ITERATE_OUTPUTS_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BP_ITERATE_OUTPUTS_REPLY);
			goto read_iterate_outputs_again;
		}
		cmd.cookie = reply.cookie;
		if (reply.error != B_OK) {
//			dlog("GetFreeOutputsFor: error %x %s", reply.error, strerror(reply.error));
			break;
		}
		/* only provide output if there's space for it */
		if ((oc < buf_num_outputs) && (reply.output.destination == media_destination::null) &&
			(!filter_type || !reply.output.format.type || (filter_type == reply.output.format.type))) {
			dlog("returning %d: %d,%d", oc, reply.output.source.port, reply.output.source.id);
			out_free_outputs[oc] = reply.output;
			oc++;
		}
#if DEBUG
		else {
			if (oc >= buf_num_outputs) {
				dlog("ran out of available outputs");
				break;
			}
			else {
				dlog("mismatch; filter_type %x; output type %x; %s", 
					filter_type, reply.output.format.type, 
					(reply.output.destination == media_destination::null) ? 
					"not connected" : "already connected");
			}
		}
#endif
	}
	if (err < B_OK) {
		dlog("write_port_etc(%d) read_port_etc(%d) in GetFreeOutputsFor() failed: %x %s", 
			node.port, p, err, strerror(err));
	}
	else {
		dispose_cookie_q cmd2;
		cmd2.cookie = cmd.cookie;
		(void)write_port_etc(node.port, BP_DISPOSE_COOKIE, &cmd2, sizeof(cmd2), 
			B_TIMEOUT, DEFAULT_TIMEOUT);
		dlog("GetFreeOutputsFor returns %d items", oc);
	}
	/* If we time out with a valid cookie, there might be a leak in the target node. Oh, well. */
	*out_total_count = oc;
	if (err > 0) {
		err = 0;
	}
	checkin_reply_port(p);
	return err;
}


status_t
BMediaRoster::GetConnectedOutputsFor(
	const media_node & node,
	media_output * out_active_outputs,
	int32 buf_num_outputs,
	int32 * out_total_count)
{
	if (!out_total_count) return B_BAD_VALUE;
	if (buf_num_outputs && !out_active_outputs) return B_BAD_VALUE;
	*out_total_count = 0;
	if ((node.node <= 0) || (node.port <= 0) || !(node.kind & B_BUFFER_PRODUCER)) {
		dlog("GetConnectedOutputsFor(%d) returns B_MEDIA_BAD_NODE", node.node);
		return B_MEDIA_BAD_NODE;
	}
#if 0
	BMessage queryOutputs(MEDIA_QUERY_OUTPUTS);
	queryOutputs.AddInt32("media_node_id", node.node);
	BMessage replyOutputs;
	status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&queryOutputs, &replyOutputs);
	int32 count = -1;
	type_code type = (type_code)-1;
	if (error == B_OK)
	{
		error = replyOutputs.GetInfo("node_outputs", &type, &count);
	}
	if (error == B_OK)
	{
		*out_total_count = count;
		if (count > buf_num_outputs)
		{
			count = buf_num_outputs;
		}
		for (int ix=0; (ix<count) && (error==B_OK); ix++)
		{
			media_output * out_ptr;
			ssize_t size;
			error = replyOutputs.FindData("node_outputs", type, ix, (const void **)&out_ptr, &size);
			if (error == B_OK)
			{
				out_active_outputs[ix] = *out_ptr;
			}
		}
	}
#endif
	/* connected outputs are outputs that don't return null for source */
	sem_id toRelease;
	iterate_outputs_q cmd;
	iterate_outputs_a reply;
	port_id p = checkout_reply_port("GetConnectedOutputsFor");
	if (p < 0)
	{
		dlog("checkout_reply_port() failed in GetConnectedOutputsFor: %x %s", p, strerror(p));
		return p;
	}
	status_t err = B_OK;
	cmd.reply = p;
	cmd.cookie = 0;
	int32 oc = 0;
	while ((err = write_port_etc(node.port, BP_ITERATE_OUTPUTS, &cmd, sizeof(cmd), 
		B_TIMEOUT, DEFAULT_TIMEOUT)) >= 0) {
		int32 code;
read_iterate_outputs_again:
		if ((err = _read_port_etc_sync(p, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease)) < 0) break;
		reply.output.format.MetaData();
		release_sem(toRelease);
		if (code != BP_ITERATE_OUTPUTS_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BP_ITERATE_OUTPUTS_REPLY);
			goto read_iterate_outputs_again;
		}
		cmd.cookie = reply.cookie;
		if (reply.error != B_OK) {
//			dlog("GetConnectedOutputsFor: error %x %s", reply.error, strerror(reply.error));
			break;
		}
		/* only provide output if there's space for it */
		if ((oc < buf_num_outputs) && !(reply.output.destination == media_destination::null))
		{
			dlog("returning %d: %d,%d", oc, reply.output.source.port, reply.output.source.id);
			out_active_outputs[oc] = reply.output;
			oc++;
		}
#if DEBUG
		else if (oc >= buf_num_outputs)
		{
			dlog("ran out of available outputs");
			break;
		}
#endif
	}
	if (err < B_OK) {
		dlog("write_port_etc(%d) read_port_etc(%d) in GetConnectedOutputsFor() failed: %x %s", 
			node.port, p, err, strerror(err));
	}
	else {
		dispose_cookie_q cmd2;
		cmd2.cookie = cmd.cookie;
		(void)write_port_etc(node.port, BP_DISPOSE_COOKIE, &cmd2, sizeof(cmd2), 
			B_TIMEOUT, DEFAULT_TIMEOUT);
		dlog("GetConnectedOutputsFor returns %d items", oc);
	}
	/* If we time out with a valid cookie, there might be a leak in the target node. Oh, well. */
	*out_total_count = oc;
	if (err > 0) {
		err = 0;
	}
	checkin_reply_port(p);
	return err;
}


status_t 
BMediaRoster::GetAllOutputsFor(const media_node &node, media_output *out_outputs, int32 buf_num_outputs, int32 *out_total_count)
{
	if (!out_total_count) return B_BAD_VALUE;
	if (buf_num_outputs && !out_outputs) return B_BAD_VALUE;
	*out_total_count = 0;
	if ((node.node <= 0) || (node.port <= 0) || !(node.kind & B_BUFFER_PRODUCER)) {
		dlog("GetConnectedOutputsFor(%d) returns B_MEDIA_BAD_NODE", node.node);
		return B_MEDIA_BAD_NODE;
	}
	/* connected outputs are outputs that don't return null for source */
	sem_id toRelease;
	iterate_outputs_q cmd;
	iterate_outputs_a reply;
	port_id p = checkout_reply_port("GetAllOutputsFor");
	if (p < 0)
	{
		dlog("checkout_reply_port() failed in GetAllOutputsFor: %x %s", p, strerror(p));
		return p;
	}
	status_t err = B_OK;
	cmd.reply = p;
	cmd.cookie = 0;
	int32 oc = 0;
	while ((err = write_port_etc(node.port, BP_ITERATE_OUTPUTS, &cmd, sizeof(cmd), 
		B_TIMEOUT, DEFAULT_TIMEOUT)) >= 0) {
		int32 code;
read_iterate_outputs_again:
		if ((err = _read_port_etc_sync(p, &code, &reply, sizeof(reply), B_TIMEOUT, DEFAULT_TIMEOUT, &toRelease)) < 0) break;
		reply.output.format.MetaData();
		release_sem(toRelease);
		if (code != BP_ITERATE_OUTPUTS_REPLY) {
			dlog("stale reply: %x (expected %x); retrying", code, BP_ITERATE_OUTPUTS_REPLY);
			goto read_iterate_outputs_again;
		}
		cmd.cookie = reply.cookie;
		if (reply.error != B_OK) {
//			dlog("GetAllOutputsFor: error %x %s", reply.error, strerror(reply.error));
			break;
		}
		/* only provide output if there's space for it */
		if (oc < buf_num_outputs)
		{
			dlog("returning %d: %d,%d", oc, reply.output.source.port, reply.output.source.id);
			out_outputs[oc] = reply.output;
			oc++;
		}
#if DEBUG
		else if (oc >= buf_num_outputs)
		{
			dlog("ran out of available outputs");
			break;
		}
#endif
	}
	if (err < B_OK) {
		dlog("write_port_etc(%d) read_port_etc(%d) in GetAllOutputsFor() failed: %x %s", 
			node.port, p, err, strerror(err));
	}
	else {
		dispose_cookie_q cmd2;
		cmd2.cookie = cmd.cookie;
		(void)write_port_etc(node.port, BP_DISPOSE_COOKIE, &cmd2, sizeof(cmd2), 
			B_TIMEOUT, DEFAULT_TIMEOUT);
		dlog("GetAllOutputsFor returns %d items", oc);
	}
	/* If we time out with a valid cookie, there might be a leak in the target node. Oh, well. */
	*out_total_count = oc;
	if (err > 0) {
		err = 0;
	}
	checkin_reply_port(p);
	return err;
}


status_t
BMediaRoster::GetDefaultInfo(
	media_node_id node, 
	BMessage & reply)
{
	BMessage query(MEDIA_GET_DEFAULT_INFO);
	query.AddInt32("be:node", node);
	status_t err = ((_BMediaRosterP *)BMediaRoster::Roster())->Turnaround(&query, &reply);
	return err;
}


status_t
BMediaRoster::SetRunningDefault(
	media_node_id for_node,
	const media_node & node)
{
	if ((node.node <= 0) || (node.port <= 0)) return B_MEDIA_BAD_NODE;
	BMessage query(MEDIA_SET_RUNNING_DEFAULT);
	BMessage reply;
	query.AddInt32("be:node", for_node);
	query.AddData("be:info", B_RAW_TYPE, &node, sizeof(node));
	status_t err = ((_BMediaRosterP *)BMediaRoster::Roster())->Turnaround(&query, &reply);
	return err;
}

status_t 
BMediaRoster::SetRealtimeFlags(uint32 in_enabled)
{
	BMessage msg(MEDIA_SET_REALTIME_FLAGS);
	status_t err = msg.AddInt32("be:flags", in_enabled);
	if (err != B_OK)
		return err;

	BMessage reply;
	err = ((_BMediaRosterP *)BMediaRoster::Roster())->Turnaround(&msg, &reply);
	return err;
}


status_t 
BMediaRoster::GetRealtimeFlags(uint32 *out_enabled)
{
	if (!out_enabled) return B_BAD_VALUE;
	BMessage msg(MEDIA_GET_REALTIME_FLAGS);
	BMessage reply;
	status_t err = ((_BMediaRosterP *)BMediaRoster::Roster())->Turnaround(&msg, &reply);
	if (err != B_OK)
		return err;
		
	return reply.FindInt32("be:flags", (int32*) out_enabled);
}


ssize_t 
BMediaRoster::AudioBufferSizeFor(
	int32 channel_count,
	uint32 sample_format,
	float frame_rate,
	bus_type bus_kind)
{
	if (frame_rate < 8192) {
		frame_rate = 8192;
	}
	//	start with a conservative formula for ISA stuff in 66 MHz BeBoxes
	size_t r = 2048 * channel_count * (sample_format & 0xf);	//	8k buffers
	//	check for user overrides
	uint32 flags = 0;
	const char * x = getenv("OVERRIDE_AUDIO_BUFFER_FRAME_COUNT");
	if (x != NULL) {
		int rr = atoi(x);
		if ((rr & (rr-1)) || (rr < 16) || (rr > 8192)) {
			DIAGNOSTIC(stderr, "BMediaRoster: OVERRIDE_AUDIO_BUFFER_FRAME_COUNT must be power of 2 >= 16 <= 8192\n");
		}
		else {
			r = rr * channel_count * (sample_format & 0xf);
		}
	}
	//	check for real-time enabled
	else if (!GetRealtimeFlags(&flags) && (flags & B_MEDIA_REALTIME_AUDIO)) {
		system_info info;
		get_system_info(&info);
		switch (bus_kind) {
		//	PCI sound cards get an extra boost just by being PCI
		case B_PCI_BUS:
			r /= 2;				//	4k buffers
			if (info.platform_type == B_MAC_PLATFORM) {
				//	fast Macs get more
				if (info.cpu_clock_speed > 175000000) {
					r /= 2;		//	2k buffers
				}
				//	really fast Macs get even more
				if ((info.cpu_clock_speed > 220000000) && (info.cpu_type >= B_CPU_PPC_604e)) {
					r /= 2;		//	1k buffers
				}
			}
			else if (info.platform_type == B_BEBOX_PLATFORM) {
				//	fast beboxes with PCI cards get more
				if ((info.cpu_clock_speed > 100000000) && (info.cpu_type >= B_CPU_PPC_603e)) {
					r /= 2;		//	2k buffers
				}
			}
			else if (info.platform_type == B_AT_CLONE_PLATFORM) {
				if (info.cpu_clock_speed >= 250000000) {
					r /= 2;		//	2k buffers
				}
				if (info.bus_clock_speed >= 95000000) {
					r /= 2;		//	1k buffers
				}
				if (info.cpu_clock_speed >= 425000000) {
					r /= 2;		//	512b buffers
				}
			}
			break;
		default:
			if (info.cpu_clock_speed > 120000000) {
				//	faster CPUs get more (this is the BeBox 133 case)
				r /= 2;		//	4k buffers
			}
			if (info.bus_clock_speed > 95000000) {
				//	fast busses get more (this is the "tricked-out PII or better with ISA sound" case)
				r /= 2;		//	2k buffers
			}
			break;
		}
	}
	//	adjust for actual sampling rate
	int rate = 32000;
	while (frame_rate <= rate) {
		rate >>= 1;
		r >>= 1;
	}
	return r;
}

void
BMediaRoster::MessageReceived(
	BMessage * message)
{
#if MEDIA_ROSTER_IS_LOOPER
	BLooper::MessageReceived(message);
#else
	BHandler::MessageReceived(message);
#endif
}

#if MEDIA_ROSTER_IS_LOOPER
bool
BMediaRoster::QuitRequested()
{
	return BLooper::QuitRequested();
}
#endif

BHandler *
BMediaRoster::ResolveSpecifier(
	BMessage *msg,
	int32 index,
	BMessage *specifier,
	int32 form,
	const char *property)
{
#if MEDIA_ROSTER_IS_LOOPER
	return BLooper::ResolveSpecifier(msg, index, specifier, form, property);
#else
	return BHandler::ResolveSpecifier(msg, index, specifier, form, property);
#endif
}

status_t
BMediaRoster::GetSupportedSuites(
	BMessage *data)
{
#if MEDIA_ROSTER_IS_LOOPER
	return BLooper::GetSupportedSuites(data);
#else
	return BHandler::GetSupportedSuites(data);
#endif
}


status_t
BMediaRoster::_Reserved_MediaRoster_0(void *)
{
	return B_ERROR;
}

status_t
BMediaRoster::_Reserved_MediaRoster_1(void *)
{
	return B_ERROR;
}

status_t
BMediaRoster::_Reserved_MediaRoster_2(void *)
{
	return B_ERROR;
}

status_t
BMediaRoster::_Reserved_MediaRoster_3(void *)
{
	return B_ERROR;
}

status_t
BMediaRoster::_Reserved_MediaRoster_4(void *)
{
	return B_ERROR;
}

status_t
BMediaRoster::_Reserved_MediaRoster_5(void *)
{
	return B_ERROR;
}

status_t
BMediaRoster::_Reserved_MediaRoster_6(void *)
{
	return B_ERROR;
}

status_t
BMediaRoster::_Reserved_MediaRoster_7(void *)
{
	return B_ERROR;
}










/***************************** PRIVATE *****************************/
//#pragma mark --- Private ---







/* and here comes the "meaty" private part */


BLocker _BMediaRosterP::_m_cleanupLock("CleanupLock");
_BMediaRosterP::cleanup_func_list _BMediaRosterP::_m_cleanupFuncs;

_BMediaRosterP::_BMediaRosterP(
	status_t * out_error) :
	_mNodeMapLock("NodeMapLock"), _m_nodeWatchLock("NodeWatchLock"), m_syncNodeListLock("SyncToNode() list")
{
	if (out_error) *out_error = 0;
	dlog("Creating _BMediaRosterP()");

	_mBufBenaphore = 1;
	_mBufSemaphore = create_sem(0, "_BMediaRosterP Buffers");
	_m_buf_wait_count = 0;
	_m_buf_wait = create_sem(0, "Buffer Arrival CondVar");
	_mAreaBenaphore = 1;
	_mAreaSemaphore = create_sem(0, "_BMediaRosterP Areas");
	_mSystemTimeSource = NULL;
//	_mNodeMapBenaphore = 1;
//	_mNodeMapSemaphore = create_sem(0, "_BMediaRosterP Referenced Node Map");
	_mDefaultHook = NULL;
	_mDefaultCookie = NULL;

#if !MEDIA_ROSTER_IS_LOOPER
	be_app->Lock();
	be_app->AddHandler(this);
	be_app->Unlock();
#endif

	status_t error = B_OK;
	_mServer = BMessenger(B_MEDIA_SERVER_SIGNATURE, -1, &error);
	if ((error < B_OK) || !_mServer.IsValid()) {
		DIAGNOSTIC(stderr,  "cannot connect to media_server: is it running? [%lx]\n", error);
		error = B_MEDIA_SYSTEM_FAILURE;
		if (out_error) *out_error = error;
		return;
	}
	BMessage reg(MEDIA_REGISTER_APP);
	BMessage reply;
	reg.AddMessenger("roster", BMessenger(this));
	_sDefault = this;
	error = Turnaround(&reg, &reply);
	if (error < B_OK) {
		DIAGNOSTIC(stderr, "cannot register with media_server! [%lx]\n", error);
	}
	
	//	Force a reference to the system_time() time source. Since we do
	//	this first in the addon_server, it will get the magic ID of 1 !!!
	//	Doing this in other teams just clones Node 1.
	(void) ReturnNULLTimeSource();
}


_BMediaRosterP::~_BMediaRosterP()
{
#if DEBUG
	FPRINTF(stderr, "_BMediaRosterP::~_BMediaRosterP()\n");
#endif
	BMessage reg(MEDIA_UNREGISTER_APP);
	BMessage reply;
	
#if DEBUG && 0
	{
		BAutolock _lck(_mNodeMapLock);

		for (map<media_node_id, pair<media_node, int32> >::iterator ptr(_mReferencedNodes.begin());
				ptr != _mReferencedNodes.end(); ptr++)
		{
			FPRINTF(stderr, "referenced node map slot %d: node %d count %d\n",
					(*ptr).first, (*ptr).second.first.node, (*ptr).second.second);
		}
	}
#endif
	status_t error = Turnaround(&reg, &reply);
	if (error < B_OK) {
		DIAGNOSTIC(stderr, "cannot unregister from media_server! [%lx]\n", error);
	}

	//	we can use LockWithTimeout because we're going away
	if (m_syncNodeListLock.LockWithTimeout(500000)) {
		for (sync_node_list::iterator ptr(m_syncNodeList.begin()); ptr != m_syncNodeList.end(); ptr++) {
			delete_port((*ptr).second.reply);
		}
		m_syncNodeList.clear();
		m_syncNodeListLock.Unlock();
	}
	//	in case someone Quit()s us before the atexit() gets called, clean up here, too.
	if (_m_cleanupLock.LockWithTimeout(500000) == B_OK) {
		for (cleanup_func_list::iterator ptr(_m_cleanupFuncs.begin()); ptr != _m_cleanupFuncs.end(); ptr++) {
			(*ptr).first((*ptr).second);
		}
		_m_cleanupFuncs.clear();
		_m_cleanupLock.Unlock();
	}

	//	Clean up cloned buffers, now that everything else is gone. This is useful
	//	for the Media prefs panel, or anyone else hanging around post-restart-media-services.
	(void)atomic_add(&_mBufBenaphore, -1);
	acquire_sem_etc(_mBufSemaphore, 1, B_TIMEOUT, 100000);
	for (buffers_map::iterator bp(_mBuffers.begin()); bp != _mBuffers.end();) {
		BBuffer * buf = (*bp).second;
		_mBuffers.erase(bp++);
		delete buf;
	}

//	if (_mSystemTimeSource) _mSystemTimeSource->Release();	/* hope it goes away! */
	delete_sem(_mBufSemaphore);
	delete_sem(_m_buf_wait);
	dlog("~_BMediaRosterP done");
}



status_t
_BMediaRosterP::SaveDefaultNode(
	media_node_id id,
	const media_node & node,
	const media_destination * destination,
	const char * name)
{
	BMessage save(MEDIA_SET_DEFAULT);
	BMessage reply;
	save.AddInt32("default", id);
	save.AddInt32("media_node_id", node.node);
	if (destination) {
		save.AddInt32("destination", destination->id);
	}
	if (name) {
		save.AddString("name", name);
	}
/*** Candidate for async ***/

	DEFAULT(stderr, "SaveDefaultNode(%d, %d)\n", id, node);
	live_node_info lni;
	if (!GetLiveNodeInfo(node, &lni)) {
		DEFAULT(stderr, "name: %x\n", lni.name);
	}
	else {
		DEFAULT(stderr, "!!!Node info error!!!\n");
	}

	status_t error = Turnaround(&save, &reply);
	return error;
}


status_t
_BMediaRosterP::SaveDefaultDormant(
	media_node_id id,
	const dormant_node_info & dormant)
{
	BMessage save(MEDIA_SET_DEFAULT);
	save.AddInt32("default", id);

	DEFAULT(stderr, "SaveDefaultDormant(%d, %d, %d, %s)\n", id, dormant.addon, dormant.flavor_id, dormant.name);
	dormant_flavor_info info;
	char path[PATH_MAX];
	status_t error = GetDormantFlavorInfoForP(dormant, &info, path, sizeof(path));
	if (error < B_OK) {
		DEFAULT(stderr, "GetDormantFlavorInfoFor() failed: %s\n", strerror(error));
		return error;
	}
	save.AddString("be:_path", path);
	save.AddInt32("be:_internal_id", dormant.flavor_id);
	save.AddString("be:_flavor_name", info.name);

/*** Candidate for async ***/
	BMessage reply;
	error = Turnaround(&save, &reply);
	return error;
}


status_t
_BMediaRosterP::Turnaround(
	BMessage * message,
	BMessage * reply,
	bigtime_t send_timeout,
	bigtime_t receive_timeout)
{
	status_t err;
	if (_isMediaServer) {
#if DEBUG
		debugger("MEDIA_SERVER: Turnaround() called in media_server team\n");
#else
		DIAGNOSTIC(stderr, "MEDIA_SERVER: Turnaround called within media_server\n");
#endif
//		dlog("Sending message to be_app");
		if (reply == NULL)
			err = BMessenger(be_app).SendMessage(message, (BHandler *)NULL, send_timeout);
		else
			err = BMessenger(be_app).SendMessage(message, reply, send_timeout, receive_timeout);
	}
	else {
//		dlog("Sending message to media_server");
//		dlog("_sDefault: %08x", _sDefault);
//		long * ptr = (long *)&((_BMediaRosterP *)_sDefault)->_mServer;
//		dlog("&_mServer %08x : %08x %08x %08x\n",  ptr, ptr[0], ptr[1], ptr[2]);
		if (!_sDefault) {
			status_t err = B_OK;
			if (!BMediaRoster::Roster(&err)) {
				dlog("Cannot create media roster: %x", err);
				return err;
			}
		}
		FPRINTF(stderr, "Turnaround('%c%c%c%c')\n", message->what>>24, message->what>>16, message->what>>8, message->what);
		if (reply == NULL)
			err = ((_BMediaRosterP *)_sDefault)->_mServer.SendMessage(message, (BHandler *)NULL, send_timeout);
		else
			err = ((_BMediaRosterP *)_sDefault)->_mServer.SendMessage(message, reply, send_timeout, receive_timeout);
	}
	if (err == B_OK && (reply != NULL)) {
		if (!reply->FindInt32("error", &err) && (err != B_OK)) {
			dlog("Turnaround error %x", err);
		}
	}
	if (BPrivate::media_debug && (err < B_OK)) {
		DIAGNOSTIC(stderr, "Thr %ld: BMediaRoster::Turnaround(0x%lx) returns 0x%lx\n",
				find_thread(NULL), message->what, err);
	}
	return err;
}


status_t
_BMediaRosterP::RegisterBuffer(
	BBuffer * buffer)
{
	BMessage query(MEDIA_REGISTER_BUFFER);
	buffer_clone_info info = buffer->CloneInfo();
#if DEBUG
	area_info ainfo;
	get_area_info(info.area, &ainfo);
	if (info.size > 2000000) {
		DIAGNOSTIC(stderr, "(thread %d) WARNING: buffer in area %ld, area is big (%ld)\n", 
			find_thread(NULL), ainfo.area, ainfo.size);
	}
#endif
	query.AddData("clone_info", B_RAW_TYPE, &info, sizeof(info));
	BMessage reply;	
	status_t err;
	//	We can't take the chance that the buffer notification gets here
	//	before we register the buffer, so keep this lock locked for a
	//	long time :-( ) It's not so bad because of the ID/buffer lookup
	//	cache in BBufferGroup and BBufferConsumer, though.
	auto_lock_bufs _alb(this);
//	lock_bufs();
	err = ((_BMediaRosterP *)_sDefault)->Turnaround(&query, &reply);
	if (err == B_OK) {
		err = reply.FindInt32("media_buffer_id", &buffer->_mBufferID);
	}
	if (err < B_OK) {
		dlog("RegisterBuffer returns %x (%s)", err, strerror(err));
	}
	else {
		//	remember the buffer we just got so we don't create another area for it
		//	as we get the registration notification
		_mBuffers[buffer->ID()] = buffer;
	}
//	unlock_bufs();
	return err;
}


BBuffer *
_BMediaRosterP::FindBuffer(
	media_buffer_id id)
{
	auto_lock_bufs alb(NULL);
try_again:
//	lock_bufs();
	alb.lock(this);
	buffers_map::iterator ptr(_mBuffers.find(id));
	BBuffer * ret = NULL;
	if (ptr != _mBuffers.end()) {
		ret = (*ptr).second;
//		unlock_bufs();
		alb.unlock();
	}
	else {
		atomic_add(&_m_buf_wait_count, 1);
//		unlock_bufs();
		alb.unlock();
		status_t err = acquire_sem_etc(_m_buf_wait, 1, B_TIMEOUT, DEFAULT_TIMEOUT);
		atomic_add(&_m_buf_wait_count, -1);
		if (err < B_OK) {
			dlog("FindBuffer() wait for buffer registration timed out --- should manually clone here!");
			ERRORS(stderr, "FindBuffer() wait for buffer registration timed out!!!\n");
			return NULL;
		}
		goto try_again;
	}
	return ret;
}

status_t
_BMediaRosterP::ReclaimOutputBuffers(
	BBufferGroup * into)
{
	if (!into) return B_BAD_VALUE;
	status_t error;
	dlog("_BMediaRosterP::ReclaimOutputBuffers()");
	BMessage request(MEDIA_RECLAIM_OUTPUT_BUFFERS);
	into->AddBuffersTo(&request, "media_buffer_id",false);
	request.AddInt32("recycle", into->_mBufferListArea);	/* area to recycle to */
	dlog("ReclaimOutputBuffers for area %d", into->_mBufferListArea);
	BMessage reply;
	error = Turnaround(&request, &reply);
	dlog("_BMediaRosterP::ReclaimOutputBuffers() returns %x", error);
	return error;
}


status_t 
_BMediaRosterP::RegisterUnownedNode(BMediaNode *node)
{
	return ((_BMediaRosterP*)_sDefault)->RegisterNodeP(node, false);
}

status_t 
_BMediaRosterP::RegisterNodeP(BMediaNode *node, bool acquireReference)
{
	if (!node)
		return B_BAD_VALUE;

	node->_inspect_classes();

	BMessage reg(MEDIA_REGISTER_NODE);
	BMessage reply;
	reg.AddMessenger("media_roster", BMessenger(this));
	/* add actual live_node_info from the node here, too! */
	media_node info;
	info.node = -1;
	info.port = node->ControlPort();
	info.kind = node->Kinds();
	reg.AddData("media_node", B_RAW_TYPE, &info, sizeof(info));
	int32 internal_id = 0;
	BMediaAddOn * addon = node->AddOn(&internal_id);
	status_t error = B_OK;
	if (addon != NULL) {
		image_id id = addon->ImageID();
		media_addon_id addon_id = addon->AddonID();
		image_info info;
		error = get_image_info(id, &info);
		if (error == B_OK) {
			reg.AddString("be:_path", info.name);
			reg.AddInt32("be:_internal_id", internal_id);
			reg.AddInt32("be:_addon_id", addon_id);
		}
	}
	reg.AddString("be:node_name", node->Name());
	if (node->Kinds() & B_BUFFER_PRODUCER) {
		reg.AddInt32("be:producer_type", dynamic_cast<BBufferProducer *>(node)->ProducerType());
	}
	if (node->Kinds() & B_BUFFER_CONSUMER) {
		reg.AddInt32("be:consumer_type", dynamic_cast<BBufferConsumer *>(node)->ConsumerType());
	}
	if (error == B_OK) {
		error = ((_BMediaRosterP*)_sDefault)->Turnaround(&reg, &reply);
	}
	if (error == B_OK) {
		error = reply.FindInt32("media_node_id", &node->_mNodeID);
		dlog("Registered %s as %d", node->_mName, node->_mNodeID);
	}
	if (error == B_OK) {
		BAutolock _lck(((_BMediaRosterP*)_sDefault)->_mNodeMapLock);
		if (((_BMediaRosterP *)_sDefault)->_mLocalInstances.find(node->Node().node) !=
				((_BMediaRosterP *)_sDefault)->_mLocalInstances.end()) {
DIAGNOSTIC(stderr, "Wooah! Node %d already exists as %s/%p; new node %p\n", node->Node().node, node->Name(),
		((_BMediaRosterP *)_sDefault)->_mLocalInstances[node->Node().node], node);
		}
		((_BMediaRosterP*)_sDefault)->_mLocalInstances[node->Node().node] = node;
		//	do this while we hold the lock
		if (acquireReference) {
			media_node dummy;
			error = ((_BMediaRosterP*)_sDefault)->AcquireNodeReference(node->ID(), &dummy);
		}
	}

	if ((error == B_OK) && (node->Kinds() & B_TIME_SOURCE))
		dynamic_cast<BTimeSource *>(node)->FinishCreate();

	if (error == B_OK)
		node->NodeRegistered();

	return error;
}

void
_BMediaRosterP::OrphanReclaimableBuffers(
	BBufferGroup * eloper)
{
	if (!eloper) {
		return;
	}
	BMessage msg(MEDIA_ORPHAN_RECLAIMABLE_BUFFERS);
	status_t error = eloper->AddBuffersTo(&msg, "buffers");
	if (error == B_OK)
	{
		msg.AddBool("was_owner", !eloper->CanReclaim());
	}
	BMessage reply;
	if (error == B_OK)
	{
/*** Candidate for async ***/
		error = Turnaround(&msg, &reply);
	}
	dassert(error == B_OK);
}


BLocker _BMediaRosterP::s_bufferGroupLock("s_bufferGroupLock");
list<area_id> _BMediaRosterP::s_bufferGroupsToRegister;
list<area_id> _BMediaRosterP::s_bufferGroupsToUnregister;

void
_BMediaRosterP::AddBufferGroupToBeRegistered(
	area_id groupArea)
{
	if (groupArea < B_OK) return;
	{
		BAutolock lock(s_bufferGroupLock);
		s_bufferGroupsToRegister.push_back(groupArea);
	}
	BMessage msg(ROSTER_RESCAN_BUFFER_GROUPS);
	//	if we don't pick it up this time, we'll pick it up the next
	BMessenger(this).SendMessage(&msg, (BHandler *)NULL, 3000LL);
}

void
_BMediaRosterP::AddBufferGroupToBeUnregistered(
	area_id groupArea)
{
	if (groupArea < B_OK) return;
	{
		BAutolock lock(s_bufferGroupLock);
		s_bufferGroupsToUnregister.push_back(groupArea);
	}
	BMessage msg(ROSTER_RESCAN_BUFFER_GROUPS);
	//	if we don't pick it up this time, we'll pick it up the next
	BMessenger(this).SendMessage(&msg, (BHandler *)NULL, 3000LL);
}



area_id
_BMediaRosterP::NewAreaUser(
	area_id to_clone)
{
	area_id ret = -1;
//	lock_areas();
	auto_lock_areas ala(this);
	areas_map::iterator iter = _mAreas.find(to_clone);
	if (iter == _mAreas.end())
	{
		void * addr = NULL;
		/* try to replicate buffer at exact position in memory */
		ret = clone_area("Cloned Buffers", &addr, B_ANY_ADDRESS, 
			B_READ_AREA | B_WRITE_AREA, to_clone);
		if (ret >= 0)
		{
			_mAreas[to_clone] = pair<area_id, int32>(ret, 1);
		}
		else
		{
			FPRINTF(stderr, "clone_area(%ld) fails with %lx (%s)\n", to_clone, ret, strerror(ret));
			dlog("clone_area(%d) fails with %x (%s)", to_clone, ret, strerror(ret));
			area_info a_info = { 0, "", 0 };
			status_t err = get_area_info(to_clone, &a_info);
			FPRINTF(stderr, "area_info: %lx   %ld %s 0x%lx\n", err, a_info.area, a_info.name, a_info.size);
//			debugger("ick!");
			dlog("area_info: %x   %d %s 0x%x", err, a_info.area, a_info.name, a_info.size);
		}
	}
	else
	{
		(*iter).second.second += 1;
		ret = (*iter).second.first;
	}
//	unlock_areas();
	return ret;
}


status_t
_BMediaRosterP::RemoveAreaUser(
	area_id orig_area)
{
	status_t ret = B_BAD_VALUE;
//	lock_areas();
	auto_lock_areas ala(this);
	areas_map::iterator ptr(_mAreas.find(orig_area));
	if (ptr != _mAreas.end())
	{
		(*ptr).second.second--;
		if ((*ptr).second.second == 0)
		{	/* last user of this area */
			delete_area((*ptr).second.first);
			_mAreas.erase(ptr);
		}
		ret = B_OK;
	}
//	unlock_areas();	
	return ret;
}


status_t
_BMediaRosterP::RegisterDedicatedArea(
	area_id no_clone)
{
//	lock_areas();
	auto_lock_areas ala(this);
	_mAreas[no_clone] = pair<area_id, int32>(no_clone, 0);
//	unlock_areas();
	return B_OK;
}


status_t
_BMediaRosterP::MediaFormatChanged(
	const media_source & source, 
	const media_destination & destination, 
	const media_format & format)
{
	BMessage msg(MEDIA_FORMAT_CHANGED);
	msg.AddData("be:_source", B_RAW_TYPE, &source, sizeof(source));
	msg.AddData("be:_destination", B_RAW_TYPE, &destination, sizeof(destination));
	msg.AddData("be:_format", B_RAW_TYPE, &format, sizeof(format));
	//	Asynch message
	return ((_BMediaRosterP *)_sDefault)->Turnaround(&msg, NULL);
}


void
_BMediaRosterP::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
	case B_MEDIA_NODE_DELETED: {
//		FPRINTF(stderr, "Roster got B_MEDIA_NODE_DELETED\n");
		bool cancelNotify = false;
		{
			int32 id,i=0;
			auto_lock_areas ala(this);
			while (message->FindInt32("media_node_id",i,&id) == B_OK) {
				systimesrc_map::iterator ptr(_mSysTimeSrcMap.find(id));
				if (ptr != _mSysTimeSrcMap.end()) {
					(*ptr).second->Release();
					_mSysTimeSrcMap.erase(ptr);
					if (_mSysTimeSrcMap.size() == 0) cancelNotify = true;
				};
				if (m_syncNodeListLock.Lock()) {
					for (sync_node_list::iterator ptr2(m_syncNodeList.begin());
							ptr2 != m_syncNodeList.end();) {
						if ((*ptr2).first.node == id) {
							delete_port((*ptr2).second.reply);
							m_syncNodeList.erase(ptr2++);
						}
						else {
							++ptr2;
						}
					}
					m_syncNodeListLock.Unlock();
				}
				i++;
			};
		}
		
		if (cancelNotify) StopWatching(BMessenger(this),B_MEDIA_NODE_DELETED);
		} break;
	case B_MEDIA_BUFFER_CREATED: {
		auto_lock_bufs alb(this);
		buffer_clone_info * info = NULL;
		ssize_t size = 0;
		for (int ix=0; !message->FindData("clone_info", B_RAW_TYPE, ix, (const void **)&info, &size); ix++) {
			media_buffer_id id = info->buffer;
			if (_mBuffers.find(id) == _mBuffers.end()) {
				BBuffer * buf = new BBuffer(*info);
				if (buf->Data() != NULL) {
					_mBuffers[id] = buf;
					int32 wc = _m_buf_wait_count;
					if (wc > 0) {
						release_sem_etc(_m_buf_wait, wc, 0);
					}
				}
				else {
					dlog("Can't find area %d for buffer %d ?", info->area, id);
					delete buf;
				}
			}
			else {
				//	buffer already registered -- probably because we created it
			}
		}
		} break;
	case B_MEDIA_BUFFER_DELETED: {
		auto_lock_bufs alb(this);
		int32 id;
		for (int ix=0; !message->FindInt32("media_buffer_id", ix, &id); ix++) {
			buffers_map::iterator ptr = _mBuffers.find(id);
			if (ptr != _mBuffers.end()) {
				BBuffer * buf = (*ptr).second;
				_mBuffers.erase(ptr);
				delete buf;
			}
		}
		} break;
	case B_MEDIA_DEFAULT_CHANGED: {
		// Need to change mapping in _mDefaultIDMap
		} break;

	case MEDIA_RELEASE_NODE_REFERENCE: {
		media_node_id nodeID;
		if (message->FindInt32("be:node_id", &nodeID) != B_OK) {
			DIAGNOSTIC(stderr, "MEDIA KIT WARNING: MediaRoster received bogus release node message\n");
			break;
		}
		
		BAutolock _lck(_mNodeMapLock);
		if (_lck.IsLocked()) {
			map<media_node_id, BMediaNode*>::iterator i = _mLocalInstances.find(nodeID);
			if (i != _mLocalInstances.end()) {
				{
					// Remove from local node map.
					BAutolock lock(_m_nodeWatchLock);
					watch_map::iterator ptr(_m_nodeWatch.find((*i).second->Node()));
					if (ptr != _m_nodeWatch.end())
						_m_nodeWatch.erase(ptr);
				}
				
				port_id p;
				if (message->FindInt32("be:reply_port", &p) != B_OK) {
					DIAGNOSTIC(stderr, "MEDIA KIT WARNING: MediaRoster received bogus release node message\n");
					break;
				}
				
				// The node releaser will actually release the node and
				// notify the requestor (in a separate thread).  This frees
				// the looper thread to continue servicing requests (which
				// it needs to do in the process of deleting), and prevents
				// a deadlock.
				BMediaNode * node = (*i).second;
				_mLocalInstances.erase(i);
				new AsyncNodeReleaser(node, p);
	
			} else {
				DIAGNOSTIC(stderr, "MEDIA KIT WARNING: Received request to delete non-existant node\n");
				port_id p;
				status_t error = B_MEDIA_BAD_NODE;
				if (message->FindInt32("be:reply_port", &p) == B_OK)
					write_port_etc(p, MEDIA_REFERENCE_RELEASED, &error, sizeof(error),
						B_TIMEOUT, DEFAULT_TIMEOUT);
			}
		}
		else {
			FPRINTF(stderr, "MEDIA_RELEASE_NODE_REFERENCE race fixed up\n");
		}
		} break;

	case MEDIA_SET_NODE_WATCH_LIST: {
//			puts("SET_NODE_WATCH_LIST");
			BAutolock lock(_m_nodeWatchLock);
			const media_node * node;
			ssize_t size;
			if (!message->FindData("be:node", B_RAW_TYPE, (const void **)&node, &size)) {
				watch_map::iterator beg(_m_nodeWatch.find(*node));
				if (beg != _m_nodeWatch.end()) {
//					puts("already there");
					(*beg).second.clear();
				}
				else {
					_m_nodeWatch.insert(watch_map::value_type(*node, watch_map::mapped_type()));
					beg = _m_nodeWatch.find(*node);
//					puts("insert new slot");
				}
				ASSERT(beg != _m_nodeWatch.end());
				BMessenger msgr;
				int32 i;
				int cnt;
				for (cnt=0; !message->FindMessenger("be:messenger", cnt, &msgr); cnt++) {
					char name[20];
					sprintf(name, "be:what:%d", cnt);
					int ix;
					for (ix=0; !message->FindInt32(name, ix, &i); ix++) {
//						FPRINTF(stderr, "inserting '%c%c%c%c' for node %d\n", i>>24, i>>16, i>>8, i, node->node);
						(*beg).second.insert(watch_map::mapped_type::value_type(i, msgr));
					}
//					FPRINTF(stderr, "There is no what code at count %d for name %s\n", ix, name);
				}
//				FPRINTF(stderr, "There is no messenger at count %d\n", cnt);
			}
//			else {
//				FPRINTF(stderr, "There is no node data\n");
//			}
		}
		break;

	case MEDIA_BROADCAST_MESSAGE: {
//			puts("BROADCAST_MESSAGE");
			const media_node * node;
			ssize_t size;
			if (!message->FindData("be:node", B_RAW_TYPE, (const void **)&node, &size)) {
				int32 what;
				if (!message->FindInt32("be:old_what", &what)) {
					message->what = what;
//					FPRINTF(stderr, "what code is '%c%c%c%c'\n", what>>24, what>>16, what>>8, what);
					BAutolock lock(_m_nodeWatchLock);
					watch_map::iterator f(_m_nodeWatch.find(*node));
					if (f != _m_nodeWatch.end()) {
						watch_map::mapped_type::iterator g((*f).second.lower_bound(what));
						watch_map::mapped_type::iterator h((*f).second.upper_bound(what));
						while (g != h) {
//							FPRINTF(stderr, "Sending message to %d\n", (*g).second.Team());
							(*g).second.SendMessage(message, (BHandler *)NULL, DEFAULT_TIMEOUT);
							g++;
						}
						if (what != B_MEDIA_WILDCARD) {
							watch_map::mapped_type::iterator i((*f).second.lower_bound(B_MEDIA_WILDCARD));
							watch_map::mapped_type::iterator j((*f).second.upper_bound(B_MEDIA_WILDCARD));
							while (i != j) {
//								FPRINTF(stderr, "Sending message to %d (wildcard)\n", (*g).second.Team());
								(*i).second.SendMessage(message, (BHandler *)NULL, DEFAULT_TIMEOUT);
								i++;
							}
						}
					}
//					else {
//						FPRINTF(stderr, "nobody is watching node %d\n", node->node);
//					}
				}
//				else {
//					FPRINTF(stderr, "no what code\n");
//				}
			}
//			else {
//				FPRINTF(stderr, "no node info\n");
//			}
		}
		break;

	case 'DUMP': {
		char urg[1024];
		BMessage reply('DUMR');
		sprintf(urg, "\n--- Buffer Dump: --------------------------------------\n");
		reply.AddString("dump", urg);
//		lock_bufs();
		{
		auto_lock_bufs alb(this);
		for (buffers_map::iterator ptr(_mBuffers.begin()); ptr != _mBuffers.end(); ptr++) {
			BBuffer * buf = (*ptr).second;
			buffer_clone_info bci;
			if (buf != NULL) {
				bci = buf->CloneInfo();
			}
			sprintf(urg, "buffer %ld: %p   (%ld %ld %ld %ld %ld)\n", (*ptr).first, buf,
				bci.buffer, bci.area, bci.size, bci.offset, bci.flags);
			reply.AddString("dump", urg);
		}
		}
//		unlock_bufs();
		sprintf(urg, "\n--- Areas Dump: ---------------------------------------\n");
		reply.AddString("dump", urg);
//		lock_areas();
		{
			auto_lock_areas ala(this);
		for (areas_map::iterator ptr(_mAreas.begin()); ptr != _mAreas.end(); ptr++) {
			area_info ainfo;
			ainfo.name[0] = 0;
			get_area_info((*ptr).first, &ainfo);
			sprintf(urg, "area %ld:  clone %ld refcount %ld   %s\n",
				(*ptr).first, (*ptr).second.first, (*ptr).second.second, ainfo.name);
			reply.AddString("dump", urg);
		}
		}
//		unlock_areas();
		thread_info tinfo;
		get_thread_info(find_thread(NULL), &tinfo);
		reply.AddInt32("team", tinfo.team);
		message->SendReply(&reply);
		} break;
	case ROSTER_RESCAN_BUFFER_GROUPS: {
			if (s_bufferGroupLock.Lock()) {
				if ((s_bufferGroupsToRegister.size() > 0) || (s_bufferGroupsToUnregister.size() > 0)) {
					BMessage msg(MEDIA_BUFFER_GROUP_REG);
					thread_info info;
					get_thread_info(find_thread(NULL), &info);
					msg.AddInt32("be:_team", info.team);
					for (list<area_id>::iterator reg(s_bufferGroupsToRegister.begin()); reg != s_bufferGroupsToRegister.end(); reg++) {
						msg.AddInt32("be:_register_buf", (*reg));
					}
					for (list<area_id>::iterator reg(s_bufferGroupsToUnregister.begin()); reg != s_bufferGroupsToUnregister.end(); reg++) {
						msg.AddInt32("be:_unregister_buf", (*reg));
					}
					s_bufferGroupsToRegister.clear();
					s_bufferGroupsToUnregister.clear();
					s_bufferGroupLock.Unlock();	//	unlock before turnaround
					Turnaround(&msg, NULL, DEFAULT_TIMEOUT, 0);
				}
				else {
					s_bufferGroupLock.Unlock();	//	unlock if there isn't anything
				}
			}
		}
		break;
	case MEDIA_ACK_FLAT_WEB: {
			int32 area;
			if (!message->FindInt32("be:_area", &area)) {
				FPRINT(("ACK_FLAT_WEB area = %ld\n", area));
				RemoveAreaUser(area);
			}
			else {
				FPRINT(("ACK_FLAT_WEB did not have be:_area item\n"));
			}
		}
		break;
	default: {
		dlog("Unknown MessageReceived('%.4s')\n", &message->what);
		BMediaRoster::MessageReceived(message);
		} break;
	}
}


BTimeSource *
_BMediaRosterP::ReturnNULLTimeSource()
{
	sInitLock.Lock();
	if (!_mSystemTimeSource) {
		if (_mInMediaAddonServer) {
			_mSystemTimeSource = new _SysTimeSource("system_time() time source");
			if (_mSystemTimeSource->ID() != SYS_TIME_SOURCE_NODE) {
				DIAGNOSTIC(stderr, "_mSystemTimeSource->ID() != SYS_TIME_SOURCE_NODE");
			}
		}
		else {
			_mSystemTimeSource = GetSysTimeSrcClone(SYS_TIME_SOURCE_NODE);
		}
	}
	sInitLock.Unlock();
	return _mSystemTimeSource;
}


status_t
_BMediaRosterP::Broadcast(
	BMessage & msg,
	uint32 what)
{
//	FPRINTF(stderr, "MediaRoster::Broadcast('%c%c%c%c')\n", what>>24, what>>16, what>>8, what);
	msg.AddInt32("be:old_what", what);
	msg.what = MEDIA_BROADCAST_MESSAGE;
	status_t err = ((_BMediaRosterP *)_sDefault)->_mServer.SendMessage(&msg);
	return err;
}

status_t
_BMediaRosterP::Broadcast(
	BMediaNode * node,
	BMessage & msg,
	uint32 what)
{
	if (!node)
		return B_BAD_VALUE;
//	FPRINTF(stderr, "MediaRoster::Broadcast(node %d, '%c%c%c%c')\n", node->ID(), what>>24, what>>16, what>>8, what);
	msg.AddInt32("be:old_what", what);
	media_node nod = node->Node();
	msg.AddData("be:node", B_RAW_TYPE, &nod, sizeof(nod));
	msg.what = MEDIA_BROADCAST_MESSAGE;
	return PostMessage(&msg);
}

//	MUST be called with _mNodeMapLock held!
status_t 
_BMediaRosterP::AcquireNodeReference(media_node_id id, media_node *out_node)
{
	if (id < 0) {
		int sava = id;
		id = FindRealIDForDefault(id);

		if (id < 0) {
			if (BPrivate::media_debug) DIAGNOSTIC(stderr, "Thr %d: Can't find node for alias %ld\n", find_thread(0), sava);
			return B_MEDIA_BAD_NODE;
		}
	}

	map<media_node_id, pair<media_node, int32> >::iterator ptr(_mReferencedNodes.find(id));
	if (ptr == _mReferencedNodes.end()) {
		// First reference from this app.  Add to referenced nodes map
		// and increment the global reference count
//		FPRINTF(stderr, "Incrementing global ref count for node %d\n", id);
		BMessage request(MEDIA_ACQUIRE_NODE_REFERENCE);
		BMessage reply;
		request.AddInt32("media_node_id", id);
	
		status_t error = ((_BMediaRosterP *)_sDefault)->Turnaround(&request, &reply);
		if (error == B_OK) {
			media_node * ret;
			ssize_t size;
			error = reply.FindData("media_node", B_RAW_TYPE, (const void **)&ret, &size);
			if (error == B_OK) {
				*out_node = *ret;
				_mReferencedNodes[id] = pair<media_node, int32>(*out_node, 1);
			} else {
				DIAGNOSTIC(stderr, "ACQUIRE_NODE_REFERENCE request didn't seem to have the fields "
					"I wanted\n");
			}
		} else {
			DIAGNOSTIC(stderr, "Couldn't acquire global reference!\n");
		}

		return error;
	} else {
		// Already referenced from this app, increment local reference count.
		atomic_add(&(*ptr).second.second, 1);
		*out_node = (*ptr).second.first;
			// CAR and CDR lives!
			//
			//	This is very sick Jon. --jeff

//		FPRINTF(stderr, "%s: Setting local ref count for %d to %d\n",
//				prog_name(), (*ptr).second.first.node, (*ptr).second.second);
	}

	return B_OK;
}



status_t
_BMediaRosterP::GetMediaTypes(
	BList * out_types)
{
	if (!out_types) return B_BAD_VALUE;
	BMessage msg(MEDIA_TYPE_ITEM_OP);
	msg.AddInt32("be:operation", B_OP_GET_TYPES);
	BMessage rep;
	status_t err = Turnaround(&msg, &rep);
	if (err == B_OK) {
		const char * type = NULL;
		for (int ix=0; !rep.FindString("be:type", ix, &type); ix++) {
			char * dup = type ? strdup(type) : NULL;
			if (!dup) {
				err = B_NO_MEMORY;
				break;
			}
			out_types->AddItem(dup);
		}
	}
	return err;
}

status_t
_BMediaRosterP::AddMediaType(
	const char * type)
{
	if (!type) return B_BAD_VALUE;
	BMessage msg(MEDIA_TYPE_ITEM_OP);
	msg.AddInt32("be:operation", B_OP_ADD_TYPE);
	msg.AddString("be:type", type);
	BMessage rep;
	status_t err = Turnaround(&msg, &rep);
	return err;
}

status_t
_BMediaRosterP::GetTypeItems(
	const char * type,
	BList * out_items)
{
	if (!type) return B_BAD_VALUE;
	if (!out_items) return B_BAD_VALUE;
	BMessage msg(MEDIA_TYPE_ITEM_OP);
	msg.AddInt32("be:operation", B_OP_GET_ITEMS);
	msg.AddString("be:type", type);
	BMessage rep;
	status_t err = Turnaround(&msg, &rep);
	if (err == B_OK) {
		const char * item = NULL;
		for (int ix=0; !rep.FindString("be:item", ix, &item); ix++) {
			media_item * it = new media_item;
			strcpy(it->name, item);
			if (rep.FindRef("be:ref", ix, &it->ref)) {
				delete it;
				err = B_ENTRY_NOT_FOUND;
				break;
			}
			out_items->AddItem(it);
		}
	}
	return err;
}

status_t
_BMediaRosterP::SetTypeItem(
	const char * type,
	const char * item,
	const entry_ref & ref)
{
	if (!type) return B_BAD_VALUE;
	if (!item) return B_BAD_VALUE;
	BMessage msg(MEDIA_TYPE_ITEM_OP);
	msg.AddInt32("be:operation", B_OP_SET_ITEM);
	msg.AddString("be:type", type);
	msg.AddString("be:item", item);
	msg.AddRef("be:ref", &ref);
	BMessage rep;
	status_t err = Turnaround(&msg, &rep);
	return err;
}

status_t
_BMediaRosterP::RemoveTypeItemRef(
	const char * type,
	const char * item,
	const entry_ref & ref)
{
	if (!type) return B_BAD_VALUE;
	if (!item) return B_BAD_VALUE;
	BMessage msg(MEDIA_TYPE_ITEM_OP);
	msg.AddInt32("be:operation", B_OP_CLEAR_ITEM);
	msg.AddString("be:type", type);
	msg.AddString("be:item", item);
	msg.AddRef("be:ref", &ref);
	BMessage rep;
	status_t err = Turnaround(&msg, &rep);
	return err;
}


status_t
_BMediaRosterP::RemoveTypeItem(
	const char * type,
	const char * item)
{
	if (!type) return B_BAD_VALUE;
	if (!item) return B_BAD_VALUE;
	BMessage msg(MEDIA_TYPE_ITEM_OP);
	msg.AddInt32("be:operation", B_OP_REMOVE_ITEM);
	msg.AddString("be:type", type);
	msg.AddString("be:item", item);
	BMessage rep;
	status_t err = Turnaround(&msg, &rep);
	return err;
}

status_t
_BMediaRosterP::MediaErrorMessage(
	const char * message)
{
	if (!_sDefault) {
		return B_MEDIA_SYSTEM_FAILURE;
	}
	BMessage msg(MEDIA_ERROR_MESSAGE);
	msg.AddString("be:_message", message);
	return ((_BMediaRosterP *)_sDefault)->Turnaround(&msg, NULL);
}

status_t
_BMediaRosterP::RegisterGUID(
	area_id * guid_area)
{
	BMessage msg(MEDIA_GUID_OP);
	msg.AddInt32("be:_op", GUID_INIT);
	BMessage reply;
	status_t err = ((_BMediaRosterP *)_sDefault)->Turnaround(&msg, &reply);
	if (err >= B_OK) {
		err = reply.FindInt32("be:_guid_area", guid_area);
	}
	return err;
}

status_t
_BMediaRosterP::UpdateGUID()
{
	BMessage msg(MEDIA_GUID_OP);
	msg.AddInt32("be:_op", GUID_UPDATE);
	return ((_BMediaRosterP *)_sDefault)->Turnaround(&msg, NULL);
}


status_t
_BMediaRosterP::ReleaseNodeP(
	const media_node & node)
{
	status_t err = B_OK;
	_mNodeMapLock.Lock();

	map<media_node_id, pair<media_node, int32> >::iterator ptr(
		_mReferencedNodes.find(node.node));
	if (ptr == _mReferencedNodes.end()) {
		DIAGNOSTIC(stderr, "media kit warning: ReleaseNode(%ld) called too many times\n",
			node.node);
#if DEBUG
		debugger("ReleaseNode called too many times");
#endif
		_mNodeMapLock.Unlock();
		return B_MEDIA_BAD_NODE;
	}

	FPRINTF(stderr, "ReleaseNodeP(%ld) count %ld\n", node.node, (*ptr).second.second);
	if (atomic_add(&(*ptr).second.second, -1) == 1) {
		PRINT(("Local ref count for %d hit zero, decrement global reference count\n",
			node.node));
		_mReferencedNodes.erase(ptr);
		_mNodeMapLock.Unlock();

		port_id replyPort = checkout_reply_port("ReleaseNodeP");

		BMessage query(MEDIA_RELEASE_NODE_REFERENCE);
		query.AddInt32("be:node_id", node.node);
		query.AddInt32("be:reply_port", replyPort);		
		status_t error = Turnaround(&query, 0, DEFAULT_TIMEOUT, DEFAULT_TIMEOUT);

		// Wait for the destructor to complete before continuing.
		while (true) {
			int32 what;
			if (read_port_etc(replyPort, &what, &error, sizeof(status_t), B_TIMEOUT,
				DEFAULT_TIMEOUT) == B_TIMED_OUT)
				break;
				
		 	if (what == MEDIA_REFERENCE_RELEASED);
				break;
		}
		
		checkin_reply_port(replyPort);
		return error;
	}

	_mNodeMapLock.Unlock();
	return B_OK;
}

media_node_id
_BMediaRosterP::FindRealIDForDefault(media_node_id id)
{
	ASSERT (id < 0); 
	
	BAutolock _lck(_mNodeMapLock);
	map<media_node_id, media_node_id>::iterator iter =
		_mDefaultIDMap.find(id);
	if (iter == _mDefaultIDMap.end()) {
		FPRINT((stderr, "FindRealIDForDefault(%d): not in _mDefaultIDMap\n", id));
		media_node_id realID;
		BMessage msg(MEDIA_GET_RUNNING_DEFAULT);
		BMessage reply;
		msg.AddInt32("be:node", id);
		if (((_BMediaRosterP *)_sDefault)->Turnaround(&msg, &reply) != B_OK) {
			FPRINT((stderr, "FindRealIDForDefault(%d): Server didn't know, either\n", id));
			return -1;
		}

		if (reply.FindInt32("be:node", &realID) == B_OK) {
			_mDefaultIDMap[id] = realID;
			FPRINT((stderr, "FindRealIDForDefault(%d): result is %d\n", id, realID));
			return realID;
		} else {
			FPRINT((stderr, "FindRealIDForDefault(%d): message is malformed\n", id));
			return -1;
		}
	} else {
		FPRINT((stderr, "FindRealIDForDefault(%d): cached response %d\n", id, (*iter).second));
		return (*iter).second;
	}
}

status_t
_BMediaRosterP::InstantiateDormantNode(
	const dormant_node_info & info,
	media_node * out_node,
	uint32 flags)
{
	BAutolock _lck(_mNodeMapLock);

	// Get flavor information about this node.
	BMessage query(MEDIA_GET_LATENT_INFO);
	query.AddInt32("be:_addon_id", info.addon);
	query.AddInt32("be:_flavor_id", info.flavor_id);
	BMessage reply;
	status_t err = Turnaround(&query, &reply);
	if (err < B_OK) 
		return err;
	
	const char *addonPath;
	dormant_flavor_info flavorInfo;
	if ((reply.FindString("be:_addon_path", &addonPath) != B_OK) ||
		(reply.FindFlat("be:_flavor_info", &flavorInfo) != B_OK)) {
		return B_NAME_NOT_FOUND;
	}		

	if ((flavorInfo.flavor_flags & B_FLAVOR_IS_GLOBAL) ||
		((flags & B_FLAVOR_IS_GLOBAL) &&
		!(flavorInfo.flavor_flags & B_FLAVOR_IS_LOCAL))) {
		// Load into media addon server
		BMessage query(MEDIA_INSTANTIATE_PERSISTENT_NODE);
		query.AddInt32("be:_addon", info.addon);
		query.AddInt32("be:_flavor_id", info.flavor_id);
		BMessage reply;
		status_t err = Turnaround(&query, &reply);
		if (err < B_OK) {
			if (BPrivate::media_debug) {
				DIAGNOSTIC(stderr, "MEDIA_INSTANTIATE_PERSISTENT_NODE request returned error\n");
			}
			return err;
		}
			
		media_node * ptr = NULL;
		ssize_t size;
		if (reply.FindData("be:_media_node", B_RAW_TYPE, (const void**)&ptr, &size)
			!= B_OK || ptr == NULL) {
			FPRINTF(stderr, "Instantiation of %s failed\n", info.name);
			return B_NAME_NOT_FOUND;
		}
//		*out_node = *ptr;
//fixme:	this does not acquire a global reference
//		((_BMediaRosterP *)_sDefault)->_mReferencedNodes[ptr->node] = pair<media_node, int32>(*out_node, 1);
		return ((_BMediaRosterP *)_sDefault)->AcquireNodeReference(ptr->node, out_node);
	} else {
//		FPRINTF(stderr, "Instantiate node locally\n");
		// Load into this team
		BMediaAddOn *addon;
		map<media_addon_id, BMediaAddOn*>::iterator entry = _mLoadedAddons.find(info.addon);
		if (entry != _mLoadedAddons.end()) {
			addon = (*entry).second;
		} else {
			// Load addon
			image_id addOnImage = load_add_on(addonPath);
			if (addOnImage < 0)
				return addOnImage;
				
			BMediaAddOn* (*make_media_addon)(int32);
	
			err = get_image_symbol(addOnImage, "make_media_addon", B_SYMBOL_TYPE_ANY,
				(void **)&make_media_addon);
			if (err < 0)
				return err;
				
			addon = (*make_media_addon)(addOnImage);
			if (addon == 0) {
				unload_add_on(addOnImage);
				return B_MEDIA_ADDON_FAILED;
			}
		
			_mLoadedAddons[info.addon] = addon;
		}

		BMediaNode *node = addon->InstantiateNodeFor(&flavorInfo, 0, &err);
		if (err != B_OK) 
			return err;
	
		RegisterNode(node);
		*out_node = node->Node();
	}

	return B_OK;
}

void
_BMediaRosterP::SetDefaultHook(
	void (*func)(int32, void *),
	void * cookie)
{
	_mDefaultHook = func;
	_mDefaultCookie = cookie;
}

BTimeSource *
_BMediaRosterP::GetSysTimeSrcClone(media_node_id nodeID)
{
	bool requestNotify = false;
	_BTimeSourceP *ts=NULL;

	if (_mSystemTimeSource && (_mSystemTimeSource->ID() == nodeID)) {
		_mSystemTimeSource->Acquire();
		return _mSystemTimeSource;
	};

	{
		auto_lock_areas ala(this);
		systimesrc_map::iterator ptr(_mSysTimeSrcMap.find(nodeID));
		if (ptr != _mSysTimeSrcMap.end()) {
			ts = (*ptr).second;
			ts->Acquire();
		};
	}

	if (!ts) {
		ts = new _BTimeSourceP(nodeID);
		if (ts->_mArea < 0) {
			ts->Release();
			return NULL;
		};
		/*	The ReleaseMaster call lets the master SysTimeSource of this
			clone get deleted before the clone does, by unreferencing here.
			This should be fine, because 1) the clone works just as well in the
			absence of the master, and 2) we get a B_MEDIA_NODE_DELETED message
			which tells us to get rid of this clone. */
		ts->ReleaseMaster();
		{
			auto_lock_areas ala(this);
			systimesrc_map::iterator ptr(_mSysTimeSrcMap.find(nodeID));
			if (ptr == _mSysTimeSrcMap.end()) {
				ts->Acquire();	//	user will release this object as well as NODE_DELETED
				_mSysTimeSrcMap[nodeID] = ts;
				if (_mSysTimeSrcMap.size() == 1) requestNotify = true;
			} else {
				ts->Release();
				ts = (*ptr).second;
				ts->Acquire();
			};
		}
	};
	
	if (requestNotify) StartWatching(BMessenger(this),B_MEDIA_NODE_DELETED);
	return ts;
};

status_t
_BMediaRosterP::_SetOutputBuffersImp_(
	const media_source & output,
	const media_destination * destination,
	void * user_data,
	int32 change_tag, 
	BBufferGroup * buffers,
	bool will_reclaim)
{
	bool will_reclaim_set = false;
	dassert(!(will_reclaim && !buffers));
	BMessage msg(MEDIA_SET_OUTPUT_BUFFERS);
	status_t error = msg.AddData("output", B_RAW_TYPE, &output, sizeof(output));
	if ((error == B_OK) && (destination != NULL)) error = msg.AddData("be:destination",
			B_RAW_TYPE, destination, sizeof(media_destination));
	if (error == B_OK) error = msg.AddPointer("be:user_data", user_data);
	assert(sizeof(BBufferGroup *) == sizeof(int32));
	if (error == B_OK) error = msg.AddInt32("be:cookie", (int32)buffers);
	if (error == B_OK) error = msg.AddInt32("be:change_tag", (int32)change_tag);
//	if (error == B_OK) {
//		error = msg.AddInt32("media_node_id", node);
//	}
	if (error == B_OK) {
		error = msg.AddBool("will_reclaim", will_reclaim);
	}
	if (error == B_OK) {
		if (buffers) {
			buffers->WillReclaim();
			will_reclaim_set = true;
			error = buffers->AddBuffersTo(&msg, "media_buffer_id");
		}
		else {
			error = msg.AddInt32("media_buffer_id", -1);
		}
	}
	BMessage reply;
	if (error == B_OK) {
		error = ((_BMediaRosterP *)_sDefault)->Turnaround(&msg, &reply);
	}
	if (error == B_OK) {
		error = ParseCommand(reply);
	}
	if (buffers) {
		if (!will_reclaim) {
			delete buffers; /* We take ownership, and then the other guy does. */
		}
		else if ((error != B_OK) && will_reclaim_set)
		{
			buffers->ReclaimAllBuffers();
		}
	}
	return error;
}


status_t 
_BMediaRosterP::FlattenHugeWeb(BParameterWeb *web, size_t size, area_id *out_area, BMessenger *out_owner)
{
	size_t rounded = (size+B_PAGE_SIZE-1)&-B_PAGE_SIZE;
	void * base = 0;
	char name[32];
	sprintf(name, "_flat_web_%d", web->Node().node);
	status_t err = create_area(name, &base, B_ANY_ADDRESS, rounded, B_NO_LOCK,
			B_READ_AREA | B_WRITE_AREA);
	*out_area = err;
	if (err < B_OK) return err;
	err = web->Flatten(base, rounded);
	if (err < B_OK) {
		delete_area(*out_area);
		*out_area = -1;
	}
	else {
		(void)RegisterDedicatedArea(*out_area);	//	refcount is at 0
		area_id use = NewAreaUser(*out_area);	//	refcount goes to 1
		ASSERT(use == *out_area);
		BMessenger msgr(this);
		*out_owner = msgr;
	}
	return err;
}

status_t 
_BMediaRosterP::UnflattenHugeWeb(BParameterWeb *target, size_t size, area_id in_area, BMessenger &owner)
{
	area_id clone = NewAreaUser(in_area);
	if (clone < B_OK) return clone;
	area_info info;
	status_t err = get_area_info(clone, &info);
	if (err < B_OK) {
		RemoveAreaUser(in_area);
		return err;
	}
	err = target->Unflatten(B_MEDIA_PARAMETER_WEB_TYPE, info.address, size);
	RemoveAreaUser(in_area);
	BMessage msg(MEDIA_ACK_FLAT_WEB);		//	tell whomever holds on to this area that he can go home now
	msg.AddInt32("be:_area", in_area);
	owner.SendMessage(&msg);
	return err;
}

status_t
_BMediaRosterP::BadMediaAddonsMayCrashHere(
	bool ImTheAddonServer)
{
	if ((!ImTheAddonServer) != (!_mInMediaAddonServer)) {
//FPRINTF(stderr, "BadMediaAddonsMayCrashHere(%s): error\n", ImTheAddonServer ? "true" : "false");
		return B_MISMATCHED_VALUES;
	}
//FPRINTF(stderr, "BadMediaAddonsMayCrashHere(%s): proceeding\n", ImTheAddonServer ? "true" : "false");
	BAutolock lock(_mNodeMapLock);
	for (int ix=0; ix<100; ix++) {
		int ts_count = 0;
		for (map<media_node_id, BMediaNode*>::iterator ptr(_mLocalInstances.begin());
				ptr != _mLocalInstances.end();) {
			if ((dynamic_cast<_BTimeSourceP *>((*ptr).second) != 0) ||
					(dynamic_cast<_SysTimeSource *>((*ptr).second) != 0)) {
				ptr++;
				ts_count++;
				continue;	//	don't do time sources first time through.
			}
			map<media_node_id, BMediaNode *>::iterator del(ptr);
			media_node_id id = (*ptr).first;
			ptr++;
			if ((*del).second->Release() == NULL) {	//	Release() may delete this item from the map
				del = _mLocalInstances.find(id);
				if (del != _mLocalInstances.end()) {
					_mLocalInstances.erase(del);
				}
			}
		}
		if (_mLocalInstances.size() <= ts_count) {
			break;
		}
	}
	for (map<media_node_id, BMediaNode*>::iterator ptr(_mLocalInstances.begin());
			ptr != _mLocalInstances.end();) {
		int32 refs = 100;
		media_node_id id = (*ptr).second->ID();
		BMediaNode * a_node = (*ptr).second;
		ptr++;
		while (a_node->Release() != NULL) {
			refs--;
			if (refs == 0) {
				break;
			}
		}
		if (refs > 0) {
			map<media_node_id, BMediaNode *>::iterator del(_mLocalInstances.find(id));
			if (del != _mLocalInstances.end()) {
				_mLocalInstances.erase(del);
			}
		}
	}
	if (_mLocalInstances.size() > 0) {
		DIAGNOSTIC(stderr, "media kit warning: %d nodes not released correctly (>100 ref counts)\n", _mLocalInstances.size());
		for (map<media_node_id, BMediaNode*>::iterator ptr(_mLocalInstances.begin());
				ptr != _mLocalInstances.end(); ptr++) {
			DIAGNOSTIC(stderr, "node %d: %s (%d)\n", (*ptr).second->ID(), (*ptr).second->Name(),
					(*ptr).second->_mRefCount);
		}
	}
	return B_OK;
}



status_t
_BMediaRosterP::AddCleanupFunction(
	void (*function)(void * cookie),
	void * cookie)
{
	if (!_m_cleanupLock.Lock()) {
		return B_ERROR;
	}
	_m_cleanupFuncs.push_back(cleanup_func_list::value_type(function, cookie));
	_m_cleanupLock.Unlock();
	return B_OK;
}


status_t
_BMediaRosterP::RemoveCleanupFunction(
	void (*function)(void * cookie),
	void * cookie)
{
	if (!_m_cleanupLock.Lock()) {
		return B_ERROR;
	}
	status_t err = B_OK;
	cleanup_func_list::value_type item(function, cookie);
	cleanup_func_list::iterator found(find(_m_cleanupFuncs.begin(), _m_cleanupFuncs.end(), item));
	if (found != _m_cleanupFuncs.end()) {
		_m_cleanupFuncs.erase(found);
	}
	else {
		err = B_BAD_INDEX;
	}
	_m_cleanupLock.Unlock();
	return err;
}


void
_BMediaRosterP::_RegisterSync(
	const media_node & node,
	const sync_q & cmd)
{
	BAutolock lock(m_syncNodeListLock);
	if (!lock.IsLocked()) return;
	m_syncNodeList.push_back(sync_node_list::value_type(node, cmd));
}


static bool operator==(const sync_q & a, const sync_q & b)
{
	return (a.cookie == b.cookie) && (a.reply == b.reply);
}

void
_BMediaRosterP::_CancelSync(
	const media_node & node,
	const sync_q & cmd)
{
	BAutolock lock(m_syncNodeListLock);
	if (!lock.IsLocked()) return;
	for (sync_node_list::iterator ptr(m_syncNodeList.begin());
			ptr != m_syncNodeList.end(); ptr++) {
		if (((*ptr).first == node) && ((*ptr).second == cmd)) {
			m_syncNodeList.erase(ptr);
			return;
		}
	}
}




AsyncNodeReleaser::AsyncNodeReleaser(BMediaNode *node, port_id notifyPort)
	:	fNode(node),
		fNotifyPort(notifyPort)
{
	resume_thread(spawn_thread(StartReleaseThread, "Rheinmachefrau", B_NORMAL_PRIORITY,
		this));
}

int32 AsyncNodeReleaser::StartReleaseThread(void *castToRelease)
{
	AsyncNodeReleaser *releaser = (AsyncNodeReleaser*) castToRelease;
	releaser->ReleaseNode();
	delete releaser;
	return 0;
}

void AsyncNodeReleaser::ReleaseNode()
{
	fNode->Release();
	status_t error = B_OK;
	write_port_etc(fNotifyPort, MEDIA_REFERENCE_RELEASED, &error, sizeof(error), B_TIMEOUT,
		DEFAULT_TIMEOUT);
}


