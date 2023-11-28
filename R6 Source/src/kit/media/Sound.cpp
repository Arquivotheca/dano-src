#include <Entry.h>
#include <Sound.h>
#include <MediaDefs.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Debug.h>
#include <OS.h>
#include <scheduler.h>
#include <MediaRoster.h>
#include <RealtimeAlloc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "TrackReader.h"


//	we spool from disk using triple-buffering of 128k buffers
#define PRELOAD_PAGE (128*1024L)
#define PRELOAD_BUFS 3
//	warn when it takes this long to read a buffer (because glitches are near)
//	#define WARNING_TIME 550000LL
//	we never load sounds bigger than 3 MB into memory
#define MEM_PLAY_SIZE_MAX 3072L*1024L
#define FADE_MUL 0.99995

#define DO_NOTHING(x...)

#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF DO_NOTHING
#endif

#define GET_DATA DO_NOTHING //FPRINTF
#define ENDIAN DO_NOTHING //FPRINTF
#define THREADS DO_NOTHING //FPRINTF
#define FORMAT DO_NOTHING //FPRINTF



static size_t format_frame_size(const media_raw_audio_format & fmt)
{
	return (fmt.format&0xf)*fmt.channel_count;
}

BSound::BSound(
	void * data,
	size_t size,
	const media_raw_audio_format & format,
	bool free_when_done)
{
	_m_checkStopped = false;
	_m_free_when_done = free_when_done;
	_m_data = data;
	_m_size = size;
	_m_format = format;
	_m_bound_player = NULL;
	_m_bind_flags = 0;
	float targets[] = {
		11025.0, 12000.0, 22050.0, 24000.0, 32000.0, 44100.0, 48000.0
	};
	for (uint ix=0; ix<sizeof(targets)/sizeof(float); ix++) {
		//	if within one percent of the target, make it the target
		if (fabs(1.0-_m_format.frame_rate/targets[ix])<=0.01) {
			_m_format.frame_rate = targets[ix];
		}
	}
	_m_file = NULL;
	_m_trackReader = NULL;
	_m_ref_count = 1;
	_m_area = -1;
	_m_avail_sem = -1;
	_m_free_sem = -1;
	_m_loader_thread = -1;
	_m_read_pos = 0;
	_m_check_token = -1;
	_m_prev_sem_count = 0;
	_m_error = 0;
}

BSound::BSound(
	const entry_ref * sound_file,
	bool load_into_memory)
{
	_m_checkStopped = false;
	BMediaFile * file = new BMediaFile(sound_file);
	_m_trackReader = NULL;
	media_format fmt;
	BMediaTrack * track = NULL;
	for (int ix=0; ix<file->CountTracks(); ix++) {
		BMediaTrack * trk = file->TrackAt(ix);
		fmt.type = B_MEDIA_RAW_AUDIO;
		fmt.u.raw_audio = media_raw_audio_format::wildcard;
		status_t err = trk->DecodedFormat(&fmt);
		if ((err == B_OK) && (fmt.type == B_MEDIA_RAW_AUDIO)) {
			track = trk;
			break;
		}
		file->ReleaseTrack(trk);
	}
	_m_read_pos = 0;
	_m_error = file->InitCheck();
	if (!_m_error && !track) {
		_m_error = B_MEDIA_BAD_FORMAT;
	}
	_m_area = -1;
	_m_avail_sem = -1;
	_m_free_sem = -1;
	_m_loader_thread = -1;
	_m_read_pos = 0;
	_m_check_token = -1;
	_m_prev_sem_count = 0;
	_m_bound_player = NULL;
	_m_bind_flags = 0;
	if (_m_error > 0) {
		_m_error = 0;
	}
	_m_ref_count = 1;
	sprintf(m_tname, "_loader:%.23s", sound_file->name);
	if (_m_error < B_OK) {
		_m_data = NULL;
		_m_size = 0;
		_m_file = NULL;
		_m_trackReader = NULL;
		_m_free_when_done = false;
	}
	else {
		_m_size = format_frame_size(fmt.u.raw_audio)*track->CountFrames();
		_m_format = fmt.u.raw_audio;

#if DEBUG
{
char fmt[100];
media_format f2;
f2.type = B_MEDIA_RAW_AUDIO;
(media_raw_audio_format &)f2.u.raw_audio = _m_format;
string_for_format(f2, fmt, 100);
fprintf(stderr, "file format: %s\n", fmt);
}
#endif

		if (_m_error >= B_OK) {
			uint32 lock_flag = 0;
			{	//	check whether to lock buffers
				uint32 rtf = 0;
				BMediaRoster::CurrentRoster()->GetRealtimeFlags(&rtf);
				if (rtf & B_MEDIA_REALTIME_AUDIO)
					lock_flag = B_FULL_LOCK;
			}
			//	refuse to load files > 3 MB into memory
			//	but always load if it's smaller than the double-buffer size
			if ((_m_size <= PRELOAD_PAGE*PRELOAD_BUFS) || (load_into_memory && (_m_size <= MEM_PLAY_SIZE_MAX))) {
				_m_data = NULL;
				_m_area = create_area(sound_file->name, &_m_data, B_ANY_ADDRESS,
					(_m_size+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1), lock_flag, B_READ_AREA|B_WRITE_AREA);
				_m_file = NULL;	//	play out of memory
				_m_trackReader = NULL;
				_m_free_when_done = true;
				if (!_m_data) {
					if (_m_area >= 0) {
						delete_area(_m_area);
						_m_area = B_NO_MEMORY;
					}
					_m_error = _m_area;
					if (_m_size > PRELOAD_PAGE*PRELOAD_BUFS) {
						//	maybe it works with a smaller buffer
						goto read_from_disk;
					}
				}
				else {
					BTrackReader rdtrk(track, fmt.u.raw_audio);
					_m_error = rdtrk.ReadFrames(_m_data, rdtrk.CountFrames());
					if (_m_error <= 0) {
						free_data();
						_m_error = (_m_error) ? _m_error : B_ERROR;
					}
#if 0
					else if (file->FileFormat() == B_AIFF_FILE && file->SampleSize() == 1) {
						int cnt = _m_size/4;
						uint32 x = 0x80808080;	//	signed bytes...
						for (int ix=0; ix<cnt; ix++) {
							((uint32 *)_m_data)[ix] = ((uint32 *)_m_data)[ix]^x;
						}
					}
#endif
				}
			}
			else {	//	file too big (or not loaded)
read_from_disk:
				_m_file = file;
				_m_trackReader = new BTrackReader(track, fmt.u.raw_audio);
				_m_data = NULL;
				file = NULL;	//	so it's not deleted
				track = NULL;
				_m_area = create_area(sound_file->name, &_m_data, B_ANY_ADDRESS,
					PRELOAD_PAGE*PRELOAD_BUFS, lock_flag, B_READ_AREA|B_WRITE_AREA);
				_m_free_when_done = true;
				if (!_m_data) {
					if (_m_area >= 0) {
						delete_area(_m_area);
						_m_area = B_NO_MEMORY;
					}
					_m_error = _m_area;
				}
				else {
#if DEBUG
					free(malloc(14));
#endif
					_m_loader_thread = 0;
					Reset();
				}
			}
		}
		else {	//	no file there
			_m_file = NULL;
			_m_trackReader = NULL;
			_m_data = NULL;
			_m_free_when_done = false;
		}
	}
	if ((track != 0) && (file != 0)) file->ReleaseTrack(track);
	if (file != 0) delete file;
	float targets[] = {
		11025.0, 12000.0, 22050.0, 24000.0, 32000.0, 44100.0, 48000.0
	};
	for (uint ix=0; ix<sizeof(targets)/sizeof(float); ix++) {
		//	if within one percent of the target, make it the target
		if (fabs(1.0-_m_format.frame_rate/targets[ix])<=0.01) {
			_m_format.frame_rate = targets[ix];
		}
	}
#if !NDEBUG
	char msg[200];
	media_format tmpf;
	tmpf.type = B_MEDIA_RAW_AUDIO;
	(media_raw_audio_format &)tmpf.u.raw_audio = _m_format;
	string_for_format(tmpf, msg, 200);
	FORMAT(stderr, "BSound file format %s\n", msg);
#endif
}


void
BSound::Reset()
{
	if (_m_loader_thread < 0) return;	//	nothing to re-set
	delete_sem(_m_avail_sem);
	delete_sem(_m_free_sem);
	status_t s;
	wait_for_thread(_m_loader_thread, &s);
	int32 prio = 50; // suggest_thread_priority(B_AUDIO_PLAYBACK, 100, 3000, 100);
	_m_loader_thread = spawn_thread(load_entry, m_tname, prio, this);
	if (_m_loader_thread < B_OK) {
		_m_error = _m_loader_thread;
		_m_avail_sem = -1;
		_m_free_sem = -1;
	}
	else {
		_m_avail_sem = create_sem(0, "BSound available");
		_m_free_sem = create_sem(PRELOAD_PAGE*PRELOAD_BUFS, "BSound free");
		_m_read_pos = 0;
		off_t zero = 0;
		_m_trackReader->SeekToFrame(&zero);
		_m_error = resume_thread(_m_loader_thread);
	}
}

status_t
BSound::InitCheck()
{
	return _m_error > 0 ? 0 : _m_error;
}


BSound * 
BSound::AcquireRef()
{
	atomic_add(&_m_ref_count, 1);
	return this;
}

bool 
BSound::ReleaseRef()
{
	if (atomic_add(&_m_ref_count, -1) == 1) {
		delete this;
		return false;
	}
	return true;
}

int32 
BSound::RefCount() const
{
	return _m_ref_count;
}

bigtime_t 
BSound::Duration() const
{
	return (bigtime_t)(_m_size*1000000LL/((_m_format.format&0xf)*(_m_format.channel_count)*_m_format.frame_rate));
}

const media_raw_audio_format &
BSound::Format() const
{
	return _m_format;
}

const void * 
BSound::Data() const	/* returns NULL for files */
{
	return _m_data;
}

off_t 
BSound::Size() const
{
	return _m_size;
}


bool 
BSound::GetDataAt(
	off_t offset,
	void * into_buffer,
	size_t buffer_size,
	size_t * out_used)
{
	ssize_t to_copy = _m_size - offset;
	int framesize = _m_format.channel_count * (_m_format.format&0xf);
	if (to_copy > (ssize_t)buffer_size) {
		to_copy = buffer_size;
	}
	if (to_copy > 0) {
		to_copy -= to_copy % framesize;
		if (buffer_size > (size_t)to_copy) {
			uchar zero = 0;
			if (_m_format.format == 0x11) {
				zero = 0x80;
			}
			memset((char *)into_buffer + to_copy, zero, buffer_size - to_copy);
		}
	}
	if (to_copy <= 0) {
		GET_DATA(stderr, "GetDataAt() sees that we exhausted our data\n");
		*out_used = 0;
		return false;
	}
	if (_m_checkStopped = check_stop()) {
		*out_used = 0;
		return false;
	}
	if (_m_data && !_m_trackReader) {
		GET_DATA(stderr, "BSound: Copying data %d @ %Ld @ %Ld\n", to_copy/framesize, offset, system_time());
		memcpy(into_buffer, ((char *)_m_data)+offset, to_copy);
	}
	else if (_m_data && _m_trackReader) {
		ASSERT(framesize == _m_trackReader->FrameSize());
		_m_error = acquire_sem_etc(_m_avail_sem, to_copy, B_TIMEOUT, 50000LL);
		if (_m_error < B_OK) {
			//	play silence
			GET_DATA(stderr, "BSound playing silence: %x\n", _m_error);
			uchar value = (_m_format.format == media_raw_audio_format::B_AUDIO_UCHAR) ? 0x80 : 0;
			memset(into_buffer, value, buffer_size);
			*out_used = to_copy;
			return true;
		}
		//	get data
		//	should copy as part of swapping, really...
//	remove another unnecessary copy step (when swapping)
//		memcpy(into_buffer, ((char *)_m_data)+_m_read_pos, to_copy);
		void * src = ((char *)_m_data)+_m_read_pos;
#if 0
		if ((_m_format.format == media_raw_audio_format::B_AUDIO_UCHAR) &&
			(_m_file->FileFormat() == B_AIFF_FILE)) {
			uint32 x = 0x80808080;
			int cnt = to_copy/4;
			for (int ix=0; ix<cnt; ix++) {
				((uint32*)into_buffer)[ix] = ((uint32*)src)[ix]^x;
			}
		} else {
#endif
			memcpy(into_buffer, src, to_copy);
#if 0
		}
#endif

		assert(_m_read_pos + to_copy <= PRELOAD_PAGE*PRELOAD_BUFS);
		_m_read_pos = (_m_read_pos + to_copy) % (PRELOAD_PAGE*PRELOAD_BUFS);
		release_sem_etc(_m_free_sem, to_copy, 0);
		GET_DATA(stderr, "_m_error: %d (to_copy %d, _m_size %d)\n", _m_error, to_copy, _m_size);
	}
	else {
		*out_used = 0;
		return false;
	}
	*out_used = (_m_error >= 0) ? to_copy : 0;
	return *out_used > 0;
}


BSound::BSound(
	const media_raw_audio_format & format)
{
	_m_checkStopped = false;
	_m_data = NULL;
	_m_file = NULL;
	_m_trackReader = NULL;
	_m_size = 0;
	_m_ref_count = 1;
	_m_format = format;
	_m_area = -1;
	_m_avail_sem = -1;
	_m_free_sem = -1;
	_m_loader_thread = -1;
	_m_free_when_done = false;
	_m_read_pos = 0;
	_m_bound_player = NULL;
	_m_bind_flags = 0;
}


BSound::~BSound()
{
	if (_m_avail_sem >= 0) {
		delete_sem(_m_avail_sem);
	}
	if (_m_free_sem >= 0) {
		delete_sem(_m_free_sem);
	}
	if (_m_loader_thread >= 0) {
//		kill_thread(_m_loader_thread);
		status_t err;
		wait_for_thread(_m_loader_thread, &err);
	}
	THREADS(stderr, "loader_thread %d is no more\n", tr);
	if (_m_free_when_done) {
		free_data();
	}
	if ((_m_trackReader != 0) && (_m_file != 0)) _m_file->ReleaseTrack(_m_trackReader->Track());
	delete _m_trackReader;
	delete _m_file;
}


status_t
BSound::BindTo(
	BSoundPlayer * player,
	const media_raw_audio_format & /*format*/)
{
	if (atomic_or(&_m_bind_flags, 1) & 1)	//	if already bound
	{										//	this is NOT a race!
		if (_m_bound_player != player)		//	fail if it's not us
			return EAGAIN;
	}
	_m_bound_player = player;
	//	format conversion if we're staticly loaded
	return B_OK;
}


status_t
BSound::UnbindFrom(
	BSoundPlayer * player)
{
	if (_m_bound_player != player)
		return B_BAD_VALUE;
	atomic_and(&_m_bind_flags, ~1);
	return B_OK;
}


status_t BSound::_Reserved_Sound_0(void *) { return B_ERROR; }	//	BindTo
status_t BSound::_Reserved_Sound_1(void *) { return B_ERROR; }	//	UnbindFrom
status_t BSound::_Reserved_Sound_2(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_3(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_4(void *) { return B_ERROR; }
status_t BSound::_Reserved_Sound_5(void *) { return B_ERROR; }

void BSound::free_data()
{
	if (_m_area >= 0) {
		delete_area(_m_area);
		_m_area = -1;
		_m_data = NULL;
	}
	else {
		free(_m_data);
		_m_data = NULL;
	}
}

status_t BSound::load_entry(void * arg)
{
	THREADS(stderr, "boo!\n");
#if DEBUG
	free(malloc(14));
#endif
	((BSound *)arg)->loader_thread();
	return 0;
}

void BSound::loader_thread()
{
	status_t err = B_OK;
	int page = 0;
	int frame_size = _m_trackReader->FrameSize();
	int frame_chunk = PRELOAD_PAGE/frame_size;
	bool running = true;
	assert(_m_loader_thread == find_thread(NULL));
	size_t total_loaded = 0;
	bigtime_t thirst = (bigtime_t)(_m_format.frame_rate*_m_format.channel_count*(_m_format.format&0xf));
	bigtime_t warning_time = 1000000LL*PRELOAD_PAGE/thirst;
	while (running) {
		if ((err = acquire_sem_etc(_m_free_sem, PRELOAD_PAGE, 0, 0)) < B_OK) {
			_m_size = total_loaded;
			break;
		}
		size_t used;
		bigtime_t load_start = system_time();
		used = _m_trackReader->ReadFrames(((char *)_m_data)+page, frame_chunk);
		if ((load_start = (system_time() - load_start)) > warning_time) {
			fprintf(stderr, "BSound: it took %g ms to load another chunk! (disk is busy?)\n", (double)load_start/1000.0);
		}
		if (used <= 0) {
			_m_size = total_loaded;
			break;
		}
		if (check_stop()) {
			_m_size = total_loaded;	//	signal stop in GetData()
			break;
		}
#if 0
		if (check_stop()) {
			THREADS(stderr, "loader_thread %d stop detected\n", find_thread(NULL));
			running = false;
			//	ramp data for soft fade out here
			int fc = used*_m_file->CountChannels();
			float mult = 1.0;
			int fcs = fc;
			const float multmult = pow(FADE_MUL, 44100.0/_m_file->SamplingRate());
			page = (page+PRELOAD_PAGE)%(PRELOAD_BUFS*PRELOAD_PAGE);
		for (int chnk = 0; chnk < PRELOAD_BUFS-1; chnk++) {
			fc = (chnk < PRELOAD_BUFS-2) ? PRELOAD_PAGE/_m_file->SampleSize() : fcs;
			page = (page+PRELOAD_PAGE)%(PRELOAD_BUFS*PRELOAD_PAGE);
			switch (_m_file->SampleFormat()) {
			case B_FLOAT_SAMPLES:
#if B_HOST_IS_LENDIAN
				if (_m_file->ByteOrder() == B_BIG_ENDIAN) {
#else
				if (_m_file->ByteOrder() == B_LITTLE_ENDIAN) {	// }
#endif
					float * p = (float *)(((char *)_m_data)+page);
					while (fc-- > 0) {
						*p = B_SWAP_FLOAT(B_SWAP_FLOAT(*p)*mult);
//						*p = B_SWAP_FLOAT((fc & 64) ? -0.5 : 0.5);
						p++;
						mult = mult*multmult;
					}
				}
				else {
					float * p = (float *)(((char *)_m_data)+page);
					while (fc-- > 0) {
						*p = *p * mult;
//						*p = (fc & 64) ? -0.5 : 0.5;
						p++;
						mult = mult*multmult;
					}
				}
				break;
			case B_LINEAR_SAMPLES:
				//	assuming 0x80 is mid-point here
				if (_m_file->SampleSize() == 1) {
					unsigned char * p = ((unsigned char *)_m_data)+page;
					while (fc-- > 0) {
						*p = ((unsigned char)((signed char)(*p^0x80)*mult))^0x80;
//						*p = (fc & 64) ? 0x40 : 0xc0;
						p++;
						mult = mult*multmult;
					}
				}
				else
#if B_HOST_IS_LENDIAN
				if (_m_file->ByteOrder() == B_BIG_ENDIAN) {
#else
				if (_m_file->ByteOrder() == B_LITTLE_ENDIAN) {	//	}
#endif
					int16 * p = (int16 *)(((char *)_m_data)+page);
					while (fc-- > 0) {
//						The in-line version curiously produces bad data:
						int16 x = B_SWAP_INT16(*p);
						x = (int16)(x * mult);
						*p = B_SWAP_INT16(x);
//						*p = B_SWAP_INT16((fc & 64) ? -15000 : 15000);
						p++;
						mult = mult*multmult;
					}
				}
				else {
					int16 * p = (int16 *)(((char *)_m_data)+page);
					while (fc-- > 0) {
						*p = (int16)(*p * mult);
//						*p = (fc & 64) ? -15000 : 15000;
						p++;
						mult = mult*multmult;
					}
				}
				break;
			default:
				//	do nothing
				break;
			}
		}	//	extra for loop
		}
#endif
		size_t loaded = used*frame_size;
		total_loaded += loaded;
		if (loaded < PRELOAD_PAGE) {
			//	assuming 0x80 is mid-point here
			if (_m_trackReader->SampleSize() == 1) {
				memset(((char *)_m_data)+page+loaded, 0x80, PRELOAD_PAGE-loaded);
			}
			else {
				memset(((char *)_m_data)+page+loaded, 0, PRELOAD_PAGE-loaded);
			}
		}
		if (!running) {
			_m_size = total_loaded;
		}
		release_sem_etc(_m_avail_sem, PRELOAD_PAGE, 0);
		page = (page+PRELOAD_PAGE)%(PRELOAD_PAGE*PRELOAD_BUFS);
	}
	if (err < B_OK) {
		THREADS(stderr, "BSound loader: acquire_sem error %x (%s)\n", err, strerror(err));
	}
	THREADS(stderr, "loader_thread %d is done\n", _m_loader_thread);
//	_m_loader_thread = -1;	//	allow for re-start
	THREADS(stderr, "BSound loader: done\n");
}

bool BSound::check_stop()
{
	if (_m_checkStopped) return true;
	if (_m_check_token <= 0) return false;
	int32 sem_count = 0;
	status_t err = get_sem_count(_m_check_token, &sem_count);
	bool ret = err || (sem_count > _m_prev_sem_count);
	if (!ret) {
		_m_prev_sem_count = sem_count;
	}
	THREADS(stderr, "check_stop(%d): %x %d\n", _m_check_token, err, sem_count);
	_m_checkStopped = ret;
	return ret;
}

status_t BSound::Perform(int32 /*code*/, ...)
{
	return B_ERROR;
}


