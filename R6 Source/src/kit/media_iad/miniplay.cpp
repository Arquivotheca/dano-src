
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include "miniplay.h"
#include "miniproto.h"
#include "minilocker.h"
#include "miniautolock.h"


#define MAX_STREAMS 32

struct cloned_buffer : public minisnd_buffer {
		area_id			local_area;
		int8*			data;
};

enum stream_type_t {
		NO_STREAM		=0,
		MIX_STREAM,
		CAPTURE_STREAM
};

struct stream {
		stream_type_t	type;
		int32			id;
		int32			srv_id;
		int				debugfd;
		sem_id			sem;
		size_t			frame_size;
		int32			buffer_frames;
		int32			buffer_count;
		int32			next_buffer_index;
		cloned_buffer	buffers[MAX_MIXCHANNEL_BUFFERS];
};

static char sDeviceName[64];

static stream sStreams[MAX_STREAMS];
static int32 sID = 1;
static MiniLocker sLock("miniplay/streams");
static port_id sPort = -1;
static port_id sReplyPort = -1;

static status_t
alloc_stream(
	stream_type_t type,
	stream** outStream)
{
	MiniAutolock _l(sLock);
	if (sPort < 0) {
		if (!sDeviceName[0]) {
			const char * dev = getenv("AUDIO_DEVICE");
			if (!dev) {
				strcpy(sDeviceName, "/dev/audio/old/sonic_vibes/1");
			}
		}
		sPort = find_port(PORT_NAME);
		if (sPort < 0) {
			fprintf(stderr, "%s: no such port\n", PORT_NAME);
			return sPort;
		}
	}
	stream * s = 0;
	int ix = 0;
	while (ix < MAX_STREAMS) {
		if (sStreams[(ix+sID)%MAX_STREAMS].type == NO_STREAM) {
			s = &sStreams[(ix+sID)%MAX_STREAMS];
			s->type = type;
			s->id = sID+ix;
			sID += (ix+1);
			break;
		}
		ix++;
	}
	if (s == 0) return EBUSY;
	*outStream = s;
	return B_OK;
}

static status_t
free_stream_buffers(
	stream * s)
{
	for (int32 n = 0; n < s->buffer_count; n++) {
		area_id clone = s->buffers[n].local_area;
		if (clone)
		{
			for (int32 forward = n; forward < s->buffer_count; forward++)
			{
				if (s->buffers[forward].local_area == clone)
					s->buffers[forward].local_area = 0;
			}
			delete_area(clone);
		}
	}
	s->buffer_count = 0;
	return B_OK;
}

static status_t
clone_stream_buffers(
	stream * s,
	const minisnd_buffer * buffers,
	int32 bufferCount)
{
	memset(s->buffers, 0, sizeof(s->buffers));
	for (int32 n = 0; n < s->buffer_count; n++) {
		memcpy(&s->buffers[n], &buffers[n], sizeof(minisnd_buffer));

		// clone or reuse a local area;
		s->buffers[n].local_area = 0;
		for(int32 prev = 0; prev < n; prev++) {
			if(s->buffers[prev].area == s->buffers[n].area) {
				// reuse
				s->buffers[n].local_area = s->buffers[prev].local_area;
				s->buffers[n].data = (int8*)s->buffers[prev].data +
					s->buffers[n].offset - s->buffers[prev].offset;
				break;
			}
		}
		if(!s->buffers[n].local_area) {
			// clone
			void* base;
			area_id cloned = clone_area(
				"local mixbuffer",
				&base,
				B_ANY_ADDRESS,
				B_READ_AREA | B_WRITE_AREA,
				s->buffers[n].area);
			if(cloned < 0) {
				free_stream_buffers(s);
				return cloned;
			}
			s->buffers[n].local_area = cloned;
			s->buffers[n].data = (int8*)base + s->buffers[n].offset;
		}
	}
	return B_OK;
}

static status_t
create_stream(
	stream_type_t type,
	stream ** outStream,
	media_multi_audio_format * ioFormat,
	uint16 * ioBufferCount)
{
	if (type != MIX_STREAM && type != CAPTURE_STREAM) return B_BAD_TYPE;
	if (!ioFormat) return B_BAD_VALUE;
	if (!ioBufferCount) return B_BAD_VALUE;
retry:
	stream * s = 0;
	status_t err = alloc_stream(type, &s);
	if (err < B_OK)
	{
		fprintf(stderr, "alloc_stream(): %s\n", strerror(err));
		return err;
	}
	s->sem = create_sem((type == MIX_STREAM) ? *ioBufferCount : 0, "stream_sem");
	if (s->sem < 0) {
		fprintf(stderr, "create_sem(): %s\n", strerror(err));
		s->type = NO_STREAM;
		return s->sem;
	}
	// send stream creation request
	port_id reply = create_port(1, "reply_port");
	if (reply < 0) {
		delete_sem(s->sem);
		s->sem = -1;
		s->type = NO_STREAM;
		return reply;
	}
	if (type == MIX_STREAM)
	{
		new_mixchannel_info nci;
		nci.i_sem = s->sem;
		nci.io_format = *ioFormat;
		strcpy(nci.i_name, sDeviceName);
		nci.io_buffer_count = *ioBufferCount;
		nci.o_error = reply;
		err = write_port_etc(sPort, MAKE_MIX_CHANNEL, &nci, sizeof(nci), B_TIMEOUT, SND_TIMEOUT);
		if (err >= 0)
		{
			int32 code;
			err = read_port_etc(reply, &code, &nci, sizeof(nci), B_TIMEOUT, SND_TIMEOUT);
			if ((err >= 0) && ((err = nci.o_error) >= 0))
			{
				*ioFormat = nci.io_format;
				s->srv_id = nci.o_id;
				s->frame_size = (nci.io_format.format & 0x0f) * nci.io_format.channel_count;
				s->buffer_frames = nci.io_format.buffer_size / s->frame_size;
				s->buffer_count = nci.io_buffer_count;
				s->next_buffer_index = 0;
				err = clone_stream_buffers(s, nci.o_buffers, nci.io_buffer_count);
			}
		}
	}
	else
	{
		new_capturechannel_info nci;
		nci.i_sem = s->sem;
		strcpy(nci.i_name, sDeviceName);
		nci.io_buffer_count = *ioBufferCount;
		nci.o_error = reply;
		err = write_port_etc(sPort, MAKE_CAPTURE_CHANNEL, &nci, sizeof(nci), B_TIMEOUT, SND_TIMEOUT);
		if (err >= 0)
		{
			int32 code;
			err = read_port_etc(reply, &code, &nci, sizeof(nci), B_TIMEOUT, SND_TIMEOUT);
			if ((err >= 0) && ((err = nci.o_error) >= 0))
			{
				*ioFormat = nci.o_format;
				s->srv_id = nci.o_id;
				s->frame_size = (nci.o_format.format & 0x0f) * nci.o_format.channel_count;
				s->buffer_frames = nci.o_format.buffer_size / s->frame_size;
				s->buffer_count = nci.io_buffer_count;
				s->next_buffer_index = nci.o_first_buffer_index;
				err = clone_stream_buffers(s, nci.o_buffers, nci.io_buffer_count);
			}
		}
	}

	delete_port(reply);

	if (err < 0) {
		delete_sem(s->sem);
		s->sem = -1;
		s->type = NO_STREAM;
		if (err == B_BAD_PORT_ID) {
			sPort = -1;
			goto retry;
		}
		return err;
	}
	
	s->debugfd = -1;

	// adjust sem to final buffer count	(for mix streams)
	if ((type == MIX_STREAM) && (s->buffer_count > *ioBufferCount)) {
		release_sem_etc(s->sem, s->buffer_count - *ioBufferCount, B_DO_NOT_RESCHEDULE);
	}
	*ioBufferCount = s->buffer_count;
	if (s->id > B_OK) *outStream = s;
	return s->id;
}	

#if 0
int32
mini_new_output_stream(
	size_t * out_size,
	uint16 backlog,
	uint16 sr,
	bool stereo,
	bool narrow,
	size_t buf_size)
{
	fprintf(stderr, "mini_new_output_stream(%f, %s, %s, %ld, %d)\n", (float)sr, stereo ? "stereo" : "mono", narrow ? "narrow" : "wide", buf_size, backlog);
	stream * s = 0;
	size_t bufferFrames = buf_size;
	status_t err = create_stream(MIX_STREAM, &s, &bufferFrames, &backlog, &sr, &stereo, &narrow);
	if (err < B_OK)
	{
		fprintf(stderr, "!!! create_stream(): %s\n", strerror(err));
		return err;
	}
	*out_size = bufferFrames;
	const char * dloc = getenv("MINI_DEBUG_STREAM_LOC");
	if (dloc != 0) {
		char path[1024];
		sprintf(path, "%s/%d.raw", dloc, s->srv_id);
		s->debugfd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0666);
	}
	return s->id;
}
#endif

int32 
mini_new_output_stream(media_multi_audio_format *io_format, uint16 buffer_count)
{
	if (!io_format) return B_ERROR;
#if DEBUG
	char fmt_buf[256];
	media_format f;
	f.type = B_MEDIA_RAW_AUDIO;
	f.u.raw_audio = *io_format;
	string_for_format(f, fmt_buf, 255);
	fprintf(stderr, "mini_new_output_stream(%s, %d bufs)\n", fmt_buf, buffer_count);
#endif
	stream * s = 0;
	status_t err = create_stream(MIX_STREAM, &s, io_format, &buffer_count);
	if (err < B_OK)
	{
		fprintf(stderr, "!!! create_stream(): %s\n", strerror(err));
		return err;
	}
	const char * dloc = getenv("MINI_DEBUG_STREAM_LOC");
	if (dloc) {
		char path[1024];
		sprintf(path, "%s/%d.raw", dloc, s->srv_id);
		s->debugfd = open(path, O_RDWR|O_CREAT|O_TRUNC, 0666);
	}
	return s->id;
}


int32
mini_close_output_stream(
	int32 strid)
{
	if (sPort < 0) return sPort;
	stream * s = &sStreams[strid%MAX_STREAMS];
	if (s->id != strid) {
//		fprintf(stderr, "mini_close_output_stream(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	if (s->type != MIX_STREAM) {
		return B_BAD_TYPE;
	}
	status_t err = write_port_etc(sPort, REMOVE_MIX_CHANNEL, &s->srv_id, sizeof(s->id),
			B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	s->id = -1;
	delete_sem(s->sem);
	s->sem = -1;
	if (s->debugfd > 0) {
		close(s->debugfd);
		s->debugfd = -1;
	}
	free_stream_buffers(s);
	s->type = NO_STREAM;
	return err < 0 ? err : 0;
}

int32
mini_write_output_stream(
	int32 strid,
	const void * data,
	int16 frame_count)
{
	if ((frame_count < 0) || (data == 0)) return B_BAD_VALUE;
	stream * s = &sStreams[strid%MAX_STREAMS];
	if (s->id != strid) {
//		fprintf(stderr, "mini_write_output_stream(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	if (s->type != MIX_STREAM) {
		return B_BAD_TYPE;
	}
	const char * ptr = (const char *)data;
	size_t buffer_size = s->buffer_frames * s->frame_size;

	size_t togo = frame_count*s->frame_size;
	status_t err = 0;
	while (togo > 0) {
		size_t towrite = togo;
		if (towrite > buffer_size) towrite = buffer_size;
		if (s->id != strid) return B_BAD_VALUE;	//	for closing stream while writing
if (s->debugfd > 0) {
	write(s->debugfd, data, towrite);
}
		err = acquire_sem_etc(s->sem, 1, B_TIMEOUT, SND_TIMEOUT);
		if (err < 0) goto errs;
		cloned_buffer& buf = s->buffers[s->next_buffer_index];
		memcpy(buf.data, ptr, towrite);
		if(towrite < buffer_size) {
			// zerofill
			memset(buf.data+towrite, 0, buffer_size - towrite);
		}
		err = write_port_etc(sPort, buf.id, 0, 0, B_TIMEOUT, SND_TIMEOUT);
		if (err == B_BAD_PORT_ID) sPort = -1;
		if (err < 0) goto errs;
		togo -= towrite;
		ptr += towrite;
		s->next_buffer_index = (s->next_buffer_index+1) % s->buffer_count;
	}
	return frame_count;
errs:
//fprintf(stderr, "mini_write_output_stream(): error %s\n", strerror(err));
	if (togo == frame_count*s->frame_size) return err;
	return frame_count-(togo/s->frame_size);
}

int32 
mini_acquire_output_buffer(int32 stream_id,
	int32 *o_buffer_id, size_t *o_buffer_size, void **o_buffer_data, bigtime_t timeout)
{
	stream * s = &sStreams[stream_id%MAX_STREAMS];
	if (s->id != stream_id) {
		return B_BAD_VALUE;
	}
	if (s->type != MIX_STREAM) {
		return B_BAD_TYPE;
	}

	status_t err = acquire_sem_etc(s->sem, 1, B_TIMEOUT, timeout);
	if(err < 0) {
		return err;
	}
	
	cloned_buffer& buf = s->buffers[s->next_buffer_index];
	s->next_buffer_index = (s->next_buffer_index+1) % s->buffer_count;
	*o_buffer_id = buf.id;
	*o_buffer_size = buf.size;
	*o_buffer_data = buf.data;
	
	return B_OK;
}

int32 
mini_queue_output_buffer(int32 buffer_id)
{
	status_t err = write_port_etc(sPort, buffer_id, 0, 0, B_TIMEOUT, SND_TIMEOUT);
	if(err == B_BAD_PORT_ID) {
		sPort = -1;
	}
	return err;
}

#if 0
int32 
mini_new_input_stream(size_t *out_size, uint16 *out_sr, bool *out_stereo, bool *out_narrow)
{
	stream * s = 0;
	size_t bufferFrames = 0;
	uint16 bufferCount = 0;
	status_t err = create_stream(CAPTURE_STREAM, &s, &bufferFrames, &bufferCount, out_sr, out_stereo, out_narrow);
	if (err < B_OK)
	{
		fprintf(stderr, "!!! create_stream(): %s\n", strerror(err));
		return err;
	}
	*out_size = bufferFrames;
	return s->id;
}
#endif

int32 
mini_new_input_stream(media_multi_audio_format *out_format)
{
	if (!out_format) return B_BAD_VALUE;
	stream * s = 0;
	uint16 bufferCount = 0;
	status_t err = create_stream(CAPTURE_STREAM, &s, out_format, &bufferCount);
	if (err < B_OK)
	{
		fprintf(stderr, "!!! create_stream(): %s\n", strerror(err));
		return err;
	}
	return s->id;
}

int32 
mini_close_input_stream(int32 strid)
{
	if (sPort < 0) return sPort;
	stream * s = &sStreams[strid%MAX_STREAMS];
	if (s->id != strid) {
//		fprintf(stderr, "mini_close_output_stream(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	if (s->type != CAPTURE_STREAM) {
		return B_BAD_TYPE;
	}
	status_t err = write_port_etc(sPort, REMOVE_CAPTURE_CHANNEL, &s->srv_id, sizeof(s->id),
			B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	s->id = -1;
	delete_sem(s->sem);
	s->sem = -1;
	free_stream_buffers(s);
	s->type = NO_STREAM;
	return err < 0 ? err : 0;
}

int32 
mini_read_input_stream(int32 strid, const void * data, int16 bufferCount)
{
	//fprintf(stderr, "mini_read_input_stream(0x%x, data, %d)\n", strid, bufferCount);
	if (bufferCount < 0 || !data) return B_BAD_VALUE;
	stream * s = &sStreams[strid%MAX_STREAMS];
	if (s->id != strid) {
		fprintf(stderr, "mini_read_input_stream(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	if (s->type != CAPTURE_STREAM) {
		return B_BAD_TYPE;
	}
	const size_t bufferSize = s->buffer_frames * s->frame_size;
	int8* ptr = (int8*)data;
	status_t err = 0;
	int destBufferCount = 0;
	for(; destBufferCount < bufferCount; destBufferCount++) {
		// seek to next source buffer
		cloned_buffer& buf = s->buffers[s->next_buffer_index];
		s->next_buffer_index = (s->next_buffer_index+1) % s->buffer_count;
		// wait for buffer to fill
		err = acquire_sem_etc(s->sem, 1, B_TIMEOUT, SND_TIMEOUT);
		if (err < B_OK) goto errs;
		// copy contents
		memcpy(ptr, buf.data, bufferSize);
		ptr += bufferSize;
	}

	return destBufferCount;	

errs:
	fprintf(stderr, "mini_read_input_stream(): error %s\n", strerror(err));
	return (!destBufferCount) ? err : destBufferCount;
}

int32 
mini_acquire_input_buffer(int32 strid, size_t *o_size, void **o_data)
{
	if (!o_size || !o_data) return B_BAD_VALUE;
	stream * s = &sStreams[strid%MAX_STREAMS];
	if (s->id != strid) {
		fprintf(stderr, "mini_read_input_stream(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	if (s->type != CAPTURE_STREAM) {
		return B_BAD_TYPE;
	}
	cloned_buffer& buf = s->buffers[s->next_buffer_index];
	s->next_buffer_index = (s->next_buffer_index+1) % s->buffer_count;
	*o_size = s->buffer_frames * s->frame_size;
	*o_data = buf.data;
	// wait for buffer to fill
	return acquire_sem_etc(s->sem, 1, B_TIMEOUT, SND_TIMEOUT);
}

int32
mini_set_volume(
	int32 strid,
	float volL,
	float volR)
{
	return mini_adjust_volume(strid, volL, volR, VOL_SET_VOL);
}

int32
mini_adjust_volume(
	int32 strid, 
	float volL, 
	float volR,
	uint32 flags)
{
	MiniAutolock lock(sLock);
	if (sPort < 0) {
		sPort = find_port(PORT_NAME);
		if (sPort < 0) {
			fprintf(stderr, "%s: no such port\n", PORT_NAME);
			return sPort;
		}
	}
	stream * s = &sStreams[strid%MAX_STREAMS];
	if ((strid > 0) && (s->id != strid)) {
//		fprintf(stderr, "mini_adjust_volume(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	set_volume_info svi;
	svi.i_id = (strid > 0) ? s->srv_id : strid;
	memset(svi.i_vols, 0, sizeof(svi.i_vols));
	svi.i_vols[0] = volL;
	svi.i_vols[1] = volR;
	svi.i_flags = flags;
	status_t err = write_port_etc(sPort, SET_VOLUME, &svi, sizeof(svi), B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	return err > 0 ? B_OK : err;
}

int32 
mini_adjust_multi_volume(int32 strid, float *volumeN, uint32 channelCount, uint32 flags)
{
	if (!volumeN || channelCount > 6 || channelCount < 2) return B_BAD_VALUE;
	MiniAutolock lock(sLock);
	if (sPort < 0) {
		sPort = find_port(PORT_NAME);
		if (sPort < 0) {
			fprintf(stderr, "%s: no such port\n", PORT_NAME);
			return sPort;
		}
	}
	stream * s = &sStreams[strid%MAX_STREAMS];
	if ((strid > 0) && (s->id != strid)) {
//		fprintf(stderr, "mini_adjust_volume(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	set_volume_info svi;
	svi.i_id = (strid > 0) ? s->srv_id : strid;
	memset(svi.i_vols, 0, sizeof(svi.i_vols));
	memcpy(svi.i_vols, volumeN, sizeof(float) * channelCount);
	svi.i_flags = flags;
	status_t err = write_port_etc(sPort, SET_VOLUME, &svi, sizeof(svi), B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	return err > 0 ? B_OK : err;
}


int32
mini_get_volume(
	int32 strid,
	float * outL,
	float * outR,
	bool * outMute)
{
	MiniAutolock lock(sLock);
	if (sPort < 0) {
		sPort = find_port(PORT_NAME);
		if (sPort < 0) {
			fprintf(stderr, "%s: no such port\n", PORT_NAME);
			return sPort;
		}
	}
	stream * s = &sStreams[strid%MAX_STREAMS];
	if ((strid > 0) && (s->id != strid)) {
//		fprintf(stderr, "mini_get_volume(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	get_volume_info gvi;
	gvi.i_id = (strid > 0) ? s->srv_id : strid;
	if (sReplyPort < 0) sReplyPort = create_port(1, "snd_server reply port");
	if (sReplyPort < 0) return sReplyPort;
	gvi.i_port = sReplyPort;
	status_t err = write_port_etc(sPort, GET_VOLUME, &gvi, sizeof(gvi), B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	int32 code;
	struct get_vols_reply gvr;
	if (err >= 0) {
		err = read_port_etc(gvi.i_port, &code, &gvr, sizeof(gvr), B_TIMEOUT, SND_TIMEOUT);
		if (err >= 0) {
			err = code;
		}
	}
	if (err >= 0) {
		if (outL) *outL = gvr.vols[0];
		if (outR) *outR = gvr.vols[1];
		if (outMute) *outMute = gvr.mute;
	}
	return err > 0 ? B_OK : err;
}

int32 
mini_get_multi_volume(int32 strid, float * outVolume, uint32 channelCount, bool * outMute)
{
	if (outVolume && (channelCount > 6 || channelCount < 2)) return B_BAD_VALUE;
	MiniAutolock lock(sLock);
	if (sPort < 0) {
		sPort = find_port(PORT_NAME);
		if (sPort < 0) {
			fprintf(stderr, "%s: no such port\n", PORT_NAME);
			return sPort;
		}
	}
	stream * s = &sStreams[strid%MAX_STREAMS];
	if ((strid > 0) && (s->id != strid)) {
//		fprintf(stderr, "mini_get_volume(): bad stream id %ld\n", strid);
		return B_BAD_VALUE;
	}
	get_volume_info gvi;
	gvi.i_id = (strid > 0) ? s->srv_id : strid;
	if (sReplyPort < 0) sReplyPort = create_port(1, "snd_server reply port");
	if (sReplyPort < 0) return sReplyPort;
	gvi.i_port = sReplyPort;
	status_t err = write_port_etc(sPort, GET_VOLUME, &gvi, sizeof(gvi), B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	int32 code;
	struct get_vols_reply gvr;
	if (err >= 0) {
		err = read_port_etc(gvi.i_port, &code, &gvr, sizeof(gvr), B_TIMEOUT, SND_TIMEOUT);
		if (err >= 0) {
			err = code;
		}
	}
	if (err >= 0) {
		if (outVolume) memcpy(outVolume, gvr.vols, sizeof(float) * channelCount);
		if (outMute) *outMute = gvr.mute;
	}
	return err > 0 ? B_OK : err;
}


int32 
mini_set_adc_source(int32 stream)
{
	if (stream != miniMainOut &&
		stream != miniCDIn &&
		stream != miniPhoneIn &&
		stream != miniLineIn &&
		stream != miniMicIn) return B_BAD_VALUE;
	MiniAutolock lock(sLock);
	if (sPort < 0)
	{
		sPort = find_port(PORT_NAME);
		if (sPort < 0)
		{
			fprintf(stderr, "%s: no such port\n", PORT_NAME);
			return sPort;
		}
	}
	status_t err = write_port_etc(sPort, SET_ADC_SOURCE, &stream, sizeof(stream), B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	return err > 0 ? B_OK : err;	
}

int32 
mini_get_adc_source(int32 *outStream)
{
	MiniAutolock lock(sLock);
	if (sPort < 0)
	{
		sPort = find_port(PORT_NAME);
		if (sPort < 0)
		{
			fprintf(stderr, "%s: no such port\n", PORT_NAME);
			return sPort;
		}
	}
	if (sReplyPort < 0) sReplyPort = create_port(1, "snd_server reply port");
	if (sReplyPort < 0) return sReplyPort;
	status_t err = write_port_etc(sPort, GET_ADC_SOURCE, &sReplyPort, sizeof(sReplyPort), B_TIMEOUT, SND_TIMEOUT);
	if (err == B_BAD_PORT_ID) sPort = -1;
	int32 code;
	if (err >= 0) {
		err = read_port_etc(sReplyPort, &code, 0, 0, B_TIMEOUT, SND_TIMEOUT);
		if (err >= 0) *outStream = err;
	}
	return err > 0 ? B_OK : err;
}

