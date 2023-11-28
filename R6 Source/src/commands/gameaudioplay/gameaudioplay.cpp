
//	Copyright 2000 Be, Incorporated. All rights reserved.

//	To do:
//	playing more than one stream (for devices that support it)
//	determining buffer size from command line (re-buffering between read/play)
//	implement randbuf for recording


#include <MediaDefs.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <game_audio2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <Application.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <File.h>
#include <signal.h>
#include <vector>


static int BSIZE = 0;
static int NBUFS = 4;
static int FREQ = 0;
static int NCHAN = 0;
static int FMT = 0;
static int LIMIT = 0;
static bool f_record = false;
static bool f_block = false;
static bool f_flush = false;
static bool f_runfirst = false;
static bool f_randbuf = false;
static bool f_pingpong = false;
static bool f_pause = false;
static bool f_rand_queue = false;

template<class T> class G : public T {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class T> class H : public T {
public:
	H() { memset(this, 0, sizeof(*this)); }
};

#define C(x) do { status_t s = (x); if (s < 0) { perror(#x); exit(1); } } while(0)

static void print_game_codec_info(game_codec_info * X) {
	printf("codec_id=%d\n", X->codec_id); // 	int16	codec_id;
	printf("linked_codec_id=%d\n", X->linked_codec_id); // 	int16	linked_codec_id;
	printf("linked_mixer_id=%d\n", X->linked_mixer_id); // 	int16	linked_mixer_id;
// 
	printf("max_stream_count=%d\n", X->max_stream_count); // 	int16	max_stream_count;
	printf("cur_stream_count=%d\n", X->cur_stream_count); // 	int16	cur_stream_count;
// 
	printf("min_chunk_frame_count=%ld\n", X->min_chunk_frame_count); // 	size_t	min_chunk_frame_count;		/*	device is not more precise */
	printf("chunk_frame_count_increment=%ld\n", X->chunk_frame_count_increment); // 	int32	chunk_frame_count_increment;	/*	-1 for power-of-two,
	printf("max_chunk_frame_count=%ld\n", X->max_chunk_frame_count); // 	size_t	max_chunk_frame_count;
	printf("cur_chunk_frame_count=%ld\n", X->cur_chunk_frame_count); // 	size_t	cur_chunk_frame_count;
// 
	printf("name=%s\n", X->name); // 	char	name[32];
	printf("frame_rates=%ld\n", X->frame_rates); // 	uint32	frame_rates;
	printf("formats=%ld\n", X->formats); // 	uint32	formats;
	printf("designations=%ld\n", X->designations); // 	uint32	designations;
	printf("channel_counts=%ld\n", X->channel_counts); // 	uint32	channel_counts;		/*	0x1 for mono, 0x2 for stereo,
// 
	printf("cur_frame_rate=%lx\n", X->cur_frame_rate); // 	uint32	cur_frame_rate;
	printf("cur_cvsr=%f\n", X->cur_cvsr); // 	float	cur_cvsr;
	printf("cur_format=%ld\n", X->cur_format); // 	uint32	cur_format;
	printf("cur_channel_count=%d\n", X->cur_channel_count); // 	int16	cur_channel_count;
	printf("\n");
}

#if 0
static void print_game_codec_info(game_codec_info * X) {
	printf("codec_id=%d\n", X->codec_id); // 	int16	codec_id;
	printf("linked_codec_id=%d\n", X->linked_codec_id); // 	int16	linked_codec_id;
	printf("linked_mixer_id=%d\n", X->linked_mixer_id); // 	int16	linked_mixer_id;
// 
	printf("max_stream_count=%d\n", X->max_stream_count); // 	int16	max_stream_count;
	printf("cur_stream_count=%d\n", X->cur_stream_count); // 	int16	cur_stream_count;
// 
	printf("min_chunk_frame_count=%ld\n", X->min_chunk_frame_count); // 	size_t	min_chunk_frame_count;		/*	device is not more precise,
	printf("chunk_frame_count_increment=%ld\n", X->chunk_frame_count_increment); // 	int32	chunk_frame_count_increment;	/*	-1 for power-of-two,
	printf("max_chunk_frame_count=%ld\n", X->max_chunk_frame_count); // 	size_t	max_chunk_frame_count;
	printf("cur_chunk_frame_count=%ld\n", X->cur_chunk_frame_count); // 	size_t	cur_chunk_frame_count;
// 
	printf("name=%s\n", X->name); // 	char	name[32];
	printf("frame_rates=%ld\n", X->frame_rates); // 	uint32	frame_rates;
	printf("formats=%ld\n", X->formats); // 	uint32	formats;
	printf("designations=%ld\n", X->designations); // 	uint32	designations;
	printf("channel_counts=%ld\n", X->channel_counts); // 	uint32	channel_counts;		/*	0x1 for mono, 0x2 for stereo,
// 
	printf("cur_frame_rate=%ld\n", X->cur_frame_rate); // 	uint32	cur_frame_rate;
	printf("cur_cvsr=%f\n", X->cur_cvsr); // 	float	cur_cvsr;
	printf("cur_format=%ld\n", X->cur_format); // 	uint32	cur_format;
	printf("cur_channel_count=%d\n", X->cur_channel_count); // 	int16	cur_channel_count;
	printf("\n");
}
#endif

static void print_game_mixer_info(game_mixer_info * X) {
	printf("mixer_id=%d\n", X->mixer_id); // 	int16	mixer_id;
	printf("linked_codec_id=%d\n", X->linked_codec_id); // 	int16	linked_codec_id;	/*	-1 for none or for multiple (in which
// 
	printf("name=%s\n", X->name); // 	char	name[32];
	printf("control_count=%d\n", X->control_count); // 	int16	control_count;
	printf("\n");
}


static void
print_data(
	int fd)
{
	G<game_get_info> ggi;
	C(ioctl(fd, GAME_GET_INFO, &ggi));
	printf("name=%s\nvendor=%s\nversion=%d.%d\n%d DACs\n%d ADCs\n%d mixers\n%d buffers\n\n",
		ggi.name, ggi.vendor, ggi.ordinal, ggi.version, ggi.dac_count, ggi.adc_count,
		ggi.mixer_count, ggi.buffer_count);
	if (ggi.dac_count > 0) {
		G<game_get_codec_infos> ggdis;
		ggdis.info = new game_codec_info[ggi.dac_count];
		ggdis.in_request_count = ggi.dac_count;
		memset(ggdis.info, 0, sizeof(game_codec_info)*ggi.dac_count);
		for (int ix=0; ix<ggi.dac_count; ix++) {
			ggdis.info[ix].codec_id = GAME_MAKE_DAC_ID(ix);
		}
		C(ioctl(fd, GAME_GET_CODEC_INFOS, &ggdis));
		for (int ix=0; ix<ggi.dac_count; ix++) {
			print_game_codec_info(ggdis.info+ix);
		}
		delete[]ggdis.info;
	}
	if (ggi.adc_count > 0) {
		G<game_get_codec_infos> ggais;
		ggais.info = new game_codec_info[ggi.adc_count];
		ggais.in_request_count = ggi.adc_count;
		memset(ggais.info, 0, sizeof(game_codec_info)*ggi.adc_count);
		for (int ix=0; ix<ggi.adc_count; ix++) {
			ggais.info[ix].codec_id = GAME_MAKE_ADC_ID(ix);
		}
		C(ioctl(fd, GAME_GET_CODEC_INFOS, &ggais));
		for (int ix=0; ix<ggi.adc_count; ix++) {
			print_game_codec_info(ggais.info+ix);
		}
		delete[]ggais.info;
	}
	if (ggi.mixer_count > 0) {
		G<game_get_mixer_infos> ggmis;
		ggmis.info = new game_mixer_info[ggi.mixer_count];
		memset(ggmis.info, 0, sizeof(game_mixer_info)*ggi.mixer_count);
		for (int ix=0; ix<ggi.mixer_count; ix++) {
			ggmis.info[ix].mixer_id = GAME_MAKE_MIXER_ID(ix);
		}
		C(ioctl(fd, GAME_GET_MIXER_INFOS, &ggmis));
		for (int ix=0; ix<ggi.mixer_count; ix++) {
			print_game_mixer_info(ggmis.info+ix);
		}
		delete[] ggmis.info;
	}
}

static void
configure_codec(
	int fd,
	int codec,
	const media_raw_audio_format & fmt)	//	should really be multi_audio_format
{
	game_codec_format gcf;
	memset(&gcf, 0, sizeof(gcf));
	G<game_set_codec_formats> gscfs;
	gscfs.formats = &gcf;
	gscfs.in_request_count = 1;
	gcf.codec = codec;
	gcf.flags = GAME_CODEC_SET_ALL;
	gcf.channels = NCHAN ? NCHAN : fmt.channel_count;
	gcf.chunk_frame_count = fmt.buffer_size/((fmt.format & 0xf)*fmt.channel_count);
	//	with multi_audio_format, we could do better (specific bit depths)
	switch (FMT ? FMT : fmt.format) {
	case 0x1:
		gcf.format = B_FMT_8BIT_S;
		break;
	case 0x2:
		gcf.format = B_FMT_16BIT;
		break;
	case 0x4:
		gcf.format = B_FMT_32BIT;
		break;
	case 0x11:
		gcf.format = B_FMT_8BIT_U;
		break;
	case 0x24:
		gcf.format = B_FMT_FLOAT;
		break;
	default:
		gcf.flags &= ~GAME_CODEC_SET_FORMAT;
		fprintf(stderr, "unknown format: 0x%x\n", fmt.format);
		break;
	}
	float frame_rate = fmt.frame_rate;
	if (FREQ > 0) {
		frame_rate = FREQ;
	}
	if (fabs(frame_rate-48000.0) < 2.0) {
		gcf.frame_rate = B_SR_48000;
	}
	else if (fabs(frame_rate-44100.0) < 2.0) {
		gcf.frame_rate = B_SR_44100;
	}
	else if (fabs(frame_rate-32000.0) < 2.0) {
		gcf.frame_rate = B_SR_32000;
	}
	else if (fabs(frame_rate-24000.0) < 2.0) {
		gcf.frame_rate = B_SR_24000;
	}
	else if (fabs(frame_rate-22050.0) < 2.0) {
		gcf.frame_rate = B_SR_22050;
	}
	else if (fabs(frame_rate-16000.0) < 2.0) {
		gcf.frame_rate = B_SR_16000;
	}
	else if (fabs(frame_rate-12000.0) < 2.0) {
		gcf.frame_rate = B_SR_12000;
	}
	else if (fabs(frame_rate-11025.0) < 2.0) {
		gcf.frame_rate = B_SR_11025;
	}
	else if (fabs(frame_rate-8000.0) < 2.0) {
		gcf.frame_rate = B_SR_8000;
	}
	else {
		gcf.frame_rate = B_SR_CVSR;
		gcf.cvsr = frame_rate;
	}
	C(ioctl(fd, GAME_SET_CODEC_FORMATS, &gscfs));
}

static int
open_stream(
	int fd,
	sem_id sem,
	int id)
{
	G<game_open_stream> gos;
	gos.codec_id = id;
	gos.request = 0;	//GAME_STREAM_VOLUME;
	gos.stream_sem = sem;
	C(ioctl(fd, GAME_OPEN_STREAM, &gos));
	return gos.out_stream_id;
}

typedef struct buf_info {
	area_id orig;
	area_id clone;
	void * ptr;
	size_t size;
	int16 buf_id;
} buf_info;

static void
alloc_buffers(
	int fd,
	int stream,
	size_t size,
	buf_info * buffers,
	int n)
{
	memset(buffers, 0, sizeof(*buffers)*n);
	if (f_pingpong) {
		H<game_open_stream_buffer> gosb;
		G<game_open_stream_buffers> gosbs;
		gosb.stream = stream;
		gosb.byte_size = size * n;
		gosbs.in_request_count = 1;
		gosbs.buffers = &gosb;
		C(ioctl(fd, GAME_OPEN_STREAM_BUFFERS, &gosbs));
		buffers[0].orig = gosb.area;
		void * d;
		buffers[0].clone = clone_area("buffers (pingpong)", &d,
				B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, buffers[0].orig);
		C(buffers[0].clone);
		for (int ix=0; ix<n; ix++) {
			buffers[ix].orig = buffers[0].orig;
			buffers[ix].clone = buffers[0].clone;
			buffers[ix].ptr = ((char *)d)+gosb.offset+ix*size;
			buffers[ix].size = size;
			buffers[ix].buf_id = gosb.buffer;
		}
printf("allocated size %d num %d byte_size %d \n",size,n,size*n);		
	}
	else
	{
		G<game_open_stream_buffers> gosbs;
		vector<game_open_stream_buffer> gosb(n);
		gosbs.buffers = &gosb[0];
		gosbs.in_request_count = n;
		memset(gosbs.buffers, 0, sizeof(game_open_stream_buffer)*n);
		for (int ix=0; ix<n; ix++)
		{
			gosb[ix].stream = stream;
			gosb[ix].byte_size = size;
		}
		C(ioctl(fd, GAME_OPEN_STREAM_BUFFERS, &gosbs));
		for (int ix=0; ix<n; ix++)
		{
printf("index %d of %d, out_status %s, area %d\n", ix, n, strerror(gosb[ix].out_status), gosb[ix].area);
			C(gosb[ix].out_status);
			buffers[ix].orig = gosb[ix].area;
			for (int iy=0; iy<ix; iy++)
			{
				if (buffers[iy].orig == gosb[ix].area)
				{
					buffers[ix].clone = buffers[iy].clone;
					break;
				}
			}
			if (!buffers[ix].clone)
			{
				void * d;
				buffers[ix].clone = clone_area("buffers", &d, B_ANY_ADDRESS,
					B_READ_AREA | B_WRITE_AREA, buffers[ix].orig);
				C(buffers[ix].clone);
			}
			area_info info;
			C(get_area_info(buffers[ix].clone, &info));
			buffers[ix].ptr = ((char *)info.address)+gosb[ix].offset;
			buffers[ix].size = gosb[ix].byte_size;
			buffers[ix].buf_id = gosb[ix].buffer;
		}
	}
}


BLocker timing_lock("timing_lock");
bigtime_t last_ioctl_time;
uint32 last_ioctl_frames;
float actual_frame_rate;
size_t actual_block_size;


static void
update_timing(
	const game_stream_timing & timing)
{
	timing_lock.Lock();
	last_ioctl_time = timing.at_time;
	last_ioctl_frames = timing.frames_played;
	timing_lock.Unlock();
}

static void
get_stream_timing(
	int fd,
	int stream)
{
	G<game_get_timing> ggt;
	ggt.source = stream;
	C(ioctl(fd, GAME_GET_TIMING, &ggt));
	update_timing(ggt.timing);
}

static void
queue_buffer(
	int fd,
	int buffer,
	size_t frame_count,
	int n,
	int stream,
	int flags)
{
	G<game_queue_stream_buffer> gqsb;
	gqsb.stream = stream;
	gqsb.flags = flags;
	gqsb.buffer = buffer;
	gqsb.frame_count = frame_count*n;
	gqsb.page_frame_count = frame_count;
	C(ioctl(fd, GAME_QUEUE_STREAM_BUFFER, &gqsb));
	update_timing(gqsb.out_timing);
}

static void
close_stream(
	int fd,
	int stream,
	sem_id sem)
{
	G<game_close_stream> gcs;
	gcs.stream = stream;
	gcs.flags = GAME_CLOSE_DELETE_SEM_WHEN_DONE;
	if (f_block) {
		gcs.flags |= GAME_CLOSE_BLOCK_UNTIL_COMPLETE;
	}
	if (f_flush) {
		gcs.flags |= GAME_CLOSE_FLUSH_DATA;
	}
	fprintf(stderr, "issuing close%s%s\n", f_block ? " blocking" : "",
		f_flush ? " flushing" : "");
	C(ioctl(fd, GAME_CLOSE_STREAM, &gcs));
	fprintf(stderr, "waiting for sem\n");
	while (acquire_sem(sem) != B_BAD_SEM_ID)
		;
	fprintf(stderr, "close done\n");
}

static void
run_stream(
	int fd,
	int strm,
	size_t size,
	int n,
	const buf_info & pongbuf)
{
	game_run_stream grs1;
	memset(&grs1, 0, sizeof(grs1));
	grs1.stream = strm;
	grs1.state = GAME_STREAM_RUNNING;
	G<game_run_streams> grs;
	grs.in_stream_count = 1;
	grs.streams = &grs1;
	C(ioctl(fd, GAME_RUN_STREAMS, &grs));
	if (f_pingpong) {
		printf("queue buffer with size %d num %d\n",size,n);
		queue_buffer(fd, pongbuf.buf_id, size>>2, n, strm, GAME_BUFFER_LOOPING |
			GAME_BUFFER_PING_PONG);
	}
	else {
		update_timing(grs1.out_timing);
	}
}

static status_t
pause_thread( void * cookie)
{
	int fd   = ((int *)(cookie))[0];
	int strm = ((int *)(cookie))[1];

	srand(system_time());
	snooze(3000000LL);

	while (1) {
		fprintf(stderr, "pause\n");
		
		game_run_stream grs1;
		memset(&grs1, 0, sizeof(grs1));
		grs1.stream = strm;
		grs1.state = GAME_STREAM_PAUSED;
		G<game_run_streams> grs;
		grs.in_stream_count = 1;
		grs.streams = &grs1;

		timing_lock.Lock();
		bigtime_t time_a = system_time();
		C(ioctl(fd, GAME_RUN_STREAMS, &grs));
		bigtime_t stopTimeEst = (bigtime_t)(last_ioctl_time+(grs1.out_timing.frames_played-last_ioctl_frames)*1000000LL/grs1.out_timing.cur_frame_rate);
		if ((grs1.out_timing.at_time < time_a) || (fabs(1.0-grs1.out_timing.cur_frame_rate/actual_frame_rate) > 0.03) ||
				(grs1.out_timing.at_time > stopTimeEst+500LL)) {
			fprintf(stderr, "ERROR: pausing stream returns inconsistent timing\n");
			fprintf(stderr, "time_a %Ld \n", time_a);
			fprintf(stderr, "last_ioctl_time %Ld  last_ioctl_frames %ld\n", last_ioctl_time, last_ioctl_frames);
			fprintf(stderr, "out.at_time %Ld  out.at_frame %ld\n", grs1.out_timing.at_time, grs1.out_timing.frames_played);
			fprintf(stderr, "out.cur_frame_rate %g  actual_frame_rate %g\n", grs1.out_timing.cur_frame_rate, actual_frame_rate);
			fprintf(stderr, "stop time estimate: %Ld\n", stopTimeEst);
		}
		timing_lock.Unlock();

		snooze((((rand()>>5) & 0x3)+4) * 250000LL);

		fprintf(stderr, "resume\n");
		grs1.state = GAME_STREAM_RUNNING;
		grs.streams = &grs1;
		timing_lock.Lock();
		time_a = system_time();
		last_ioctl_frames = grs1.out_timing.frames_played;
		last_ioctl_time = time_a;
		C(ioctl(fd, GAME_RUN_STREAMS, &grs));
		if ((grs1.out_timing.at_time < time_a) || (fabs(1.0-grs1.out_timing.cur_frame_rate/actual_frame_rate) > 0.03) ||
				(grs1.out_timing.at_time > time_a + actual_block_size*1000000LL/actual_frame_rate+1000LL) ||
				(grs1.out_timing.frames_played != last_ioctl_frames)) {
			fprintf(stderr, "ERROR: pausing stream returns inconsistent timing\n");
			fprintf(stderr, "time_a %Ld \n", time_a);
			fprintf(stderr, "last_ioctl_time %Ld  last_ioctl_frames %ld\n", last_ioctl_time, last_ioctl_frames);
			fprintf(stderr, "out.at_time %Ld  out.at_frame %ld\n", grs1.out_timing.at_time, grs1.out_timing.frames_played);
			fprintf(stderr, "out.cur_frame_rate %g  actual_frame_rate %g\n", grs1.out_timing.cur_frame_rate, actual_frame_rate);
		}
		timing_lock.Unlock();
		snooze( (((rand()>>5) & 0x7)+4) * 250000LL);
	}
	return B_OK;
}


static void
play_track(
	BMediaTrack * mt,
	const media_raw_audio_format & fmt,
	int fd)
{
	print_data(fd);
	sem_id sem = create_sem(NBUFS-1, "buffer_sem");
	int strm;
	int64 frames;
	int cnt = 0;
	int run_cnt = NBUFS-1;

	configure_codec(fd, GAME_MAKE_DAC_ID(0), fmt);
	actual_frame_rate = fmt.frame_rate;
	actual_block_size = fmt.buffer_size/(fmt.channel_count*(fmt.format & 0xf));
	strm = open_stream(fd, sem, GAME_MAKE_DAC_ID(0));

	if (f_randbuf) {
		//	Because this happens AFTER we create the semaphore, we
		//	know there are TWO free buffers when acquire_sem() returns.
		//	Thus we know we can, randomly, shuffle those two to permute
		//	the buffer ordering into something random, ever-changing.
		NBUFS += 1;
	}
	buf_info buffers[NBUFS];	//fixme: gcc-ism
	alloc_buffers(fd, strm, fmt.buffer_size, buffers, NBUFS);

	if (f_runfirst) {
		run_cnt = 0;
		run_stream(fd, strm, fmt.buffer_size, NBUFS, buffers[0]);
	}

	if (f_pause) {
		int ax[2] = { fd, strm };
		resume_thread(spawn_thread(pause_thread, "pause_thread", 30, (void *)ax));
	}

	while (mt->ReadFrames(buffers[cnt].ptr, &frames) == B_OK) {
		if (!f_pingpong) {
			if (f_rand_queue) {
				snooze(((rand()>>5) & 0x3) * 15000LL);
			}
			queue_buffer(fd, buffers[cnt].buf_id, frames, 1, strm,
				GAME_RELEASE_SEM_WHEN_DONE | GAME_GET_LAST_TIME);
		}
		else {
			get_stream_timing(fd, strm);
		}
		cnt++;
		cnt %= NBUFS;
		if (run_cnt > 0) {
			run_cnt--;
			if (run_cnt == 0) {
				run_stream(fd, strm, fmt.buffer_size, NBUFS, buffers[0]);
			}
		}
		acquire_sem(sem);
		if (f_randbuf && (rand() & 0x40)) {
			buf_info t = buffers[cnt];
			buffers[cnt] = buffers[(cnt+1)%NBUFS];
			buffers[(cnt+1)%NBUFS] = t;
		}
	}
	close_stream(fd, strm, sem);	//	will wait, deletes sem
}

static int
play_args(
	int argc,
	char * argv[],
	int fd)
{
	BFile f(argv[2], O_RDONLY);
	BMediaFile mf(&f);
	if (mf.InitCheck() < 0) {
		fprintf(stderr, "%s: cannot open (%s)\n", argv[2], strerror(mf.InitCheck()));
		return 1;
	}
	BMediaTrack * mt = mf.TrackAt(0);
	media_format fmt;
	fmt.type = B_MEDIA_RAW_AUDIO;
	if (mt->DecodedFormat(&fmt) || (fmt.type != B_MEDIA_RAW_AUDIO)) {
		fprintf(stderr, "%s: first track is not audio\n", argv[2]);
		return 1;
	}

	fprintf(stderr, "playing...\n");
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	set_thread_priority(find_thread(NULL), 100);
	play_track(mt, fmt.u.raw_audio, fd);
	set_thread_priority(find_thread(NULL), info.priority);

	fprintf(stderr, "done\n");
	mf.ReleaseTrack(mt);

	return 0;
}

static void
find_wav(
	media_file_format * ff)
{
	int32 cookie = 0;
	while (B_OK == get_next_file_format(&cookie, ff)) {
		if (!strcmp(ff->file_extension, "wav")) {
			fprintf(stderr, "Found format '%s'\n", ff->pretty_name);
			return;
		}
	}
	fprintf(stderr, "Could not find the WAV format writer.\n");
	exit(1);
}

static volatile bool record_running = true;

static void
inth(int)
{
	record_running = false;
	write(2, "interrupt\n", 11);
}

static status_t
record_write(
	void * port_v)
{
	int buf_siz = 256*1024;
	static char buffa[256*1024];
	int offs = 0;
	int r;
	int32 code = 0;
	int mx = 0;

	while ((r = read_port((port_id)port_v, &code, &buffa[offs], buf_siz-offs)) > 0) {
		if (mx < r) mx = r;
		offs += r;
		if (buf_siz-offs < mx) {
			status_t err = ((BMediaTrack *)code)->WriteChunk(buffa, offs);
			if (err < 0) {
				fprintf(stderr, "error writing track: %s\n", strerror(err));
			}
			offs = 0;
		}
	}
	if (offs > 0) {
		((BMediaTrack *)code)->WriteChunk(buffa, offs);
	}
	delete_port((port_id)port_v);
	return 0;
}

static void
record_track(
	BMediaTrack * track,
	const media_raw_audio_format & fmt,
	int fd)
{
	signal(SIGINT, inth);
	signal(SIGHUP, inth);
	print_data(fd);
	/* record using method 1: chunk-sized buffers that are queued */
	sem_id sem = create_sem(NBUFS-1, "buffer_sem");
	int strm;
	record_running = true;

	port_id record_port = create_port(100, "record_port");
	thread_id record_thread = spawn_thread(record_write, "record_write", 20, (void *)record_port);
	resume_thread(record_thread);

	configure_codec(fd, GAME_MAKE_ADC_ID(0), fmt);
	strm = open_stream(fd, sem, GAME_MAKE_ADC_ID(0));

	buf_info buffers[NBUFS];
	alloc_buffers(fd, strm, fmt.buffer_size, buffers, NBUFS);

if (f_randbuf) {
	fprintf(stderr, "warning: --randbuf is not implemented for --record\n");
}
	int64 frames = fmt.buffer_size>>2;
	int cnt = 0;
	int run_cnt = NBUFS;
	int back = 0;
	if (f_runfirst) {
		run_stream(fd, strm, frames, NBUFS, buffers[0]);
	}
	while (record_running) {
		if (!f_pingpong) {
			queue_buffer(fd, buffers[cnt].buf_id, frames, 1, strm,
				GAME_RELEASE_SEM_WHEN_DONE | GAME_GET_LAST_TIME);
		}
		cnt++;
		cnt %= NBUFS;
		if (run_cnt > 0) {
			run_cnt--;
			if ((run_cnt == 0) && !f_runfirst) {
				run_stream(fd, strm, frames, NBUFS, buffers[0]);
			}
		}
		acquire_sem(sem);
		if (run_cnt == 0) {
//			status_t err = track->WriteChunk(buffers[back].ptr, fmt.buffer_size);
			write_port(record_port, (uint32)track, buffers[back].ptr, fmt.buffer_size);
			if (LIMIT > 0) {
				if ((LIMIT -= fmt.buffer_size) <= 0) {
					break;
				}
			}
//			if (err < B_OK) {
//				fprintf(stderr, "Error calling WriteTrack(): %s\n", err);
//				break;
//			}
			back++;
			back %= NBUFS;
		}
	}
	close_port(record_port);
	close_stream(fd, strm, sem);	//	will wait, deletes sem
	status_t s;
	wait_for_thread(record_thread, &s);
}

static int
record_args(
	int argc,
	char * argv[],
	int fd)
{
	BFile f(argv[2], O_RDWR | O_CREAT | O_TRUNC);
	media_file_format wav;
	find_wav(&wav);
	BMediaFile mf(&f, &wav);
	if (mf.InitCheck() < 0) {
		fprintf(stderr, "%s: cannot create (%s)\n", argv[2], strerror(mf.InitCheck()));
		return 1;
	}
	media_format fmt;
	fmt.type = B_MEDIA_RAW_AUDIO;
	fmt.u.raw_audio.frame_rate = FREQ ? FREQ : 44100;
	fmt.u.raw_audio.channel_count = NCHAN ? NCHAN : 2;
	fmt.u.raw_audio.format = FMT ? FMT : 2;
	fmt.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
	BMediaTrack * mt = mf.CreateTrack(&fmt);
printf("fmt.byte_order = %ld\n", fmt.u.raw_audio.byte_order);
	if (!mt) {
		fprintf(stderr, "%s: could not add audio track\n", argv[2]);
		return 1;
	}
 	mf.CommitHeader();
 	if ((fmt.u.raw_audio.buffer_size == 0) || (BSIZE > 32)) {
 		fmt.u.raw_audio.buffer_size = BSIZE;
	}

	fprintf(stderr, "recording...\n");
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	set_thread_priority(find_thread(NULL), 100);
	record_track(mt, fmt.u.raw_audio, fd);
	set_thread_priority(find_thread(NULL), info.priority);

	mf.CloseFile();
	fprintf(stderr, "done\n");

	return 0;
}

static struct {
	const char * arg;
	int * ptr;
	int min;
	int max;
	const char * help;
}
opts[] = {
	{ "NBUFS", &NBUFS, 2, 16, "number of buffers or pingpong pages" },
	{ "FREQ", &FREQ, 4000, 100000, "override sampling frequency" },
	{ "NCHAN", &NCHAN, 1, 12, "override number of channels" },
	{ "FMT", &FMT, 1, 36, "override format (2 == short, 17 == uchar, 36 == float)" },
	{ "BSIZE", &BSIZE, 1024, 65536, "recording buffer size or pingpong page size (in bytes)" },
	{ "LIMIT", &LIMIT, 0, 10000000, "recording file size limit (in bytes)" },
};
static struct {
	const char * arg;
	bool * ptr;
	const char * help;
}
swits[] = {
	{ "-record", &f_record, "record instead of playback" },
	{ "-block", &f_block, "wait for close_stream" },
	{ "-flush", &f_flush, "flush on close_stream" },
	{ "-runfirst", &f_runfirst, "run_stream before queue_buffer" },
	{ "-randbuf", &f_randbuf, "choose random buffer order when queuing" },
	{ "-pingpong", &f_pingpong, "use one large buffer in pingpong mode" },
	{ "-pause", &f_pause, "random pauses and continues for your listening pleasure" },
	{ "-rand_queue", &f_rand_queue, "random queue times" },
};

static struct {
	bool * leaveset;
	bool * clear;
	const char * msg;
}
interlock[] = {
	{ &f_randbuf, &f_pingpong, "--randbuf clears --pingpong" },
};

int
main(
	int argc,
	char * argv[])
{
	BApplication app("application/x-vnd.hplus.gameaudioplay");
	struct stat st;
	const char * arg;
	bool found;

	//	check parameters
	while ((argv[1] != 0) && ((arg = strchr(argv[1], '=')) != 0)) {
		argv++;
		argc--;
		found = false;
		for (int ix=0; ix<sizeof(opts)/sizeof(opts[0]); ix++) {
			if (!strncmp(*argv, opts[ix].arg, arg-*argv)) {
				int i = atoi(arg+1);
				if (i < opts[ix].min || i > opts[ix].max) {
					fprintf(stderr, "usage: %d <= %s <= %d\n",
						opts[ix].min, opts[ix].arg, opts[ix].max);
				}
				else {
					*opts[ix].ptr = i;
				}
				found = true;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "%s: unknown argument\n", *argv);
		}
	}

	//	check options
	while ((argv[1] != 0) && (argv[1][0] == '-')) {
		argv++;
		argc--;
		bool sense = true;
		found = false;
		const char * arg = *argv+1;
		if (!strncmp(arg, "-no-", 4)) {
			sense = false;
			arg += 3;
		}
		for (int ix=0; ix<sizeof(swits)/sizeof(swits[0]); ix++) {
			if (!strcmp(arg, swits[ix].arg)) {
				*swits[ix].ptr = sense;
				found = true;
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "-%s: unknown argument\n", arg);
		}
	}

	//	check option interlock
	for (int ix=0; ix<sizeof(interlock)/sizeof(interlock[0]); ix++) {
		if (*interlock[ix].leaveset && *interlock[ix].clear) {
			*interlock[ix].clear = false;
			fprintf(stderr, "warning: %s\n", interlock[ix].msg);
		}
	}

	//	check real arguments
	if ((argc != 3) || (stat(argv[1], &st) < 0) || (!f_record && (stat(argv[2], &st) < 0))) {
		fprintf(stderr, "usage: gameaudioplay [values] [options] device filename\n");
		fprintf(stderr, "\nvalues:\n");
		for (int ix=0; ix<sizeof(opts)/sizeof(opts[0]); ix++) {
			fprintf(stderr, "%8s=n   %s (current %d, min %d, max %d)\n",
				opts[ix].arg, opts[ix].help, *opts[ix].ptr, opts[ix].min, opts[ix].max);
		}
		fprintf(stderr, "\noptions:\n");
		for (int ix=0; ix<sizeof(swits)/sizeof(swits[0]); ix++) {
			fprintf(stderr, "-%-10s  %s\n", swits[ix].arg, swits[ix].help);
		}
		return 1;
	}
	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: cannot open (%s)\n", argv[1], strerror(errno));
		return 1;
	}
	int ret = 0;
	if (f_record) {
		ret = record_args(argc, argv, fd);
	}
	else {
		ret = play_args(argc, argv, fd);
	}
	close(fd);
	return ret;
}
