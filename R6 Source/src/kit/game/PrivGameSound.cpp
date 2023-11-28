#include <string.h>
#include <stdio.h>
#include <rt_alloc.h>
#include <assert.h>
#include <stdlib.h>

#include <Errors.h>
#include <Autolock.h>
#include <Application.h>
#include <Roster.h>
#if _SUPPORTS_MEDIA_NODES
 #include <SoundFile.h>
#endif
#include <math.h>
#include <GameSound.h>

#include "PrivGameSound.h"
#if _SUPPORTS_MEDIA_NODES
#include <trinity_p.h>
#else
#include <Locker.h>
#endif

#if DEBUG
#define FPRINTF fprintf
#else
#define FPRINTF (void)
#endif


#define SPLIT_GAIN 16384L
#define UNITY_GAIN 32768L
#define MAX_GAIN 32767L
#define SAMP_RANGE (SPLIT_GAIN*UNITY_GAIN-1)

#define DIVIDE_UNITY_GAIN(x) (((int32)(x))>>15)


static int32 MAX_SOUNDS = 256;


#define HANDLE_TO_SLOT(h) ((h)-1)
#define SLOT_TO_HANDLE(s) ((s)+1)
#define VALID_HANDLE(h) (((h)>=1)&&((h)<=MAX_SOUNDS)&&(_m_sounds[HANDLE_TO_SLOT(h)].data!=NULL))

static bool fr_eq(float fr, float eq)
{
	return fabs(1.0-fr/eq) < 0.005;
}

static void snap_fr(float & fr)
{
	static float snaplist[] = { 44100.0, 22050.0, 11025.0 };
	for (int ix=0; ix<sizeof(snaplist)/sizeof(float); ix++) {
		if (fabs(1.0-fr/snaplist[ix]) < 0.005) {
			fr = snaplist[ix];
			break;
		}
	}
}

namespace BPrivate {
#if 0
class PrivDeleter
{
public:
	 PrivDeleter() { }
	~PrivDeleter() { PrivGameSound::Shutdown(); }
};
PrivDeleter _pd;
#endif

static void gamesound_cleanup(void *)
{
	PrivGameSound::Shutdown();
}

#if !_SUPPORTS_MEDIA_NODES
static BLocker cLocker;
struct cleanup {
	struct cleanup * next;
	void (*func)(void *);
	void * cookie;
};
cleanup * sCleanup;
static void add_cleanup(void(*func)(void *), void * cookie=0)
{
	BAutolock lock(cLocker);
	cleanup * cu = new cleanup;
	cu->next = sCleanup;
	cu->func = func;
	cu->cookie = cookie;
	sCleanup = cu;
}
static void remove_cleanup(void(*func)(void *), void * cookie=0)
{
	BAutolock lock(cLocker);
	for (cleanup ** cup = &sCleanup; *cup != 0;) {
		if (((*cup)->func == func) && ((*cup)->cookie == cookie)) {
			cleanup * d = *cup;
			*cup = d->next;
			delete d;
		}
		else {
			cup = &(*cup)->next;
		}
	}
}
#endif

static rtm_pool * _the_game_pool = NULL;
static int32 did_it = 0;
static bool lockSet = false;
static bool isLocked = false;

#define DEFAULT_POOL_SIZE (1024*1024L)
#define MIN_POOL_SIZE (128*1024L)
#define MAX_POOL_SIZE (4*1024*1024L)

static size_t
get_default_game_pool_size()
{
	const char * sizeenv = getenv("GAME_SOUND_SIZE");
	if (!sizeenv) {
		return DEFAULT_POOL_SIZE;
	}
	size_t ret = atoi(sizeenv);
	if (ret < MIN_POOL_SIZE || ret > MAX_POOL_SIZE) {
		return DEFAULT_POOL_SIZE;
	}
	return ret;
}

size_t game_pool_size = get_default_game_pool_size();

rtm_pool *
get_game_pool()
{
	if (atomic_add(&did_it, 1) == 0) {
		rtm_pool *temp;
		uint32 flags = 0;
		if (lockSet) {
			flags = isLocked ? B_RTM_FORCE_LOCKED : B_RTM_FORCE_UNLOCKED;
		}
		(void)rtm_create_pool_etc(&temp, game_pool_size, "_BGameSound_", flags);
		_the_game_pool = temp;
	} else {
		while (_the_game_pool == NULL) {
			snooze(3000);
		}
	}
	
	return _the_game_pool;
}

}   // end of namespace 


status_t
BGameSound::SetMemoryPoolSize(
	size_t size)
{
	size_t realMax = MAX_POOL_SIZE;
	if (lockSet && !isLocked) {
		realMax *= 4;
	}
	if ((size < MIN_POOL_SIZE) || (size > realMax)) {
		fprintf(stderr, "BGameSound::SetMemoryPoolSize(%ld): pool size must be %ld to %ld bytes.\n",
				size, (long)MIN_POOL_SIZE, (long)realMax);
		return B_BAD_VALUE;
	}
	if (_the_game_pool != 0) {
		fprintf(stderr, "BGameSound::SetMemoryPoolSize(): Memory pool is already allocated.\n");
		return EALREADY;
	}
	game_pool_size = size;
	return B_OK;
}

status_t
BGameSound::LockMemoryPool(
	bool lockInCore)
{
	if (lockSet) return EALREADY;
	isLocked = lockInCore;
	lockSet = true;
	return B_OK;
}


int32 PrivGameSound::_m_didInit;
PrivGameSound * PrivGameSound::_m_player;

int32
BGameSound::SetMaxSoundCount(
	int32 in_maxCount)
{
	if (PrivGameSound::CurPlayer() != 0) return MAX_SOUNDS;
	if (in_maxCount < 32) in_maxCount = 32;
	if (in_maxCount > 1024) in_maxCount = 1024;
	MAX_SOUNDS = in_maxCount;
	return in_maxCount;
}



size_t
BPrivate::frame_size_for(const gs_audio_format & format)
{
	return format.channel_count * (format.format & 0xf);
}

#if _SUPPORTS_MEDIA_NODES
void
BPrivate::format_from_file(BSoundFile & file, gs_audio_format * format)
{
	format->frame_rate = file.SamplingRate();
	format->channel_count = file.CountChannels();
	switch (file.SampleSize()) {
	default:
	case 1:
		format->format = 0x11;
		break;
	case 2:
		format->format = 0x2;
		break;
	}
	switch (file.ByteOrder()) {
	case B_LITTLE_ENDIAN:
		format->byte_order = B_MEDIA_LITTLE_ENDIAN;
		break;
	case B_BIG_ENDIAN:
		format->byte_order = B_MEDIA_BIG_ENDIAN;
		break;
	default:
#if B_HOST_IS_BENDIAN
		format->byte_order = B_MEDIA_BIG_ENDIAN;
#else
		format->byte_order = B_MEDIA_LITTLE_ENDIAN;
#endif
		break;
	}
	format->buffer_size = 0;
}
#endif

#define FIXED_FREQS 0

PrivGameSound *
PrivGameSound::MakePlayer(const gs_audio_format &format)
{
	if (!(atomic_or(&_m_didInit, 1) & 1)) {	//	test if we're first
		gs_audio_format fmt(format);
#if FIXED_FREQS
		if (!fr_eq(fmt.frame_rate, 44100.0) && !fr_eq(fmt.frame_rate, 22050.0) && 
				!fr_eq(fmt.frame_rate, 11025.0)) {
			float new_rate = 22050.0;
			if (new_rate < fmt.frame_rate) new_rate = 44100.0;
			fprintf(stderr, "PrivGameSound::MakePlayer(): only 44.1, 22.05 and 11.025 kHz frame rates supported for now\n");
			fprintf(stderr, "framerate was %g, is %g\n", fmt.frame_rate, new_rate);
			fmt.frame_rate = new_rate;
		}
		else {
#endif
			snap_fr(fmt.frame_rate);
#if FIXED_FREQS
		}
#endif
		fmt.channel_count = 2;
		if (fmt.format != 0x2) {
			//	print warning to developer?
			//	fprintf(stderr, "PrivGameSound::MakePlayer(): only int16 output format supported\n");
			fmt.format = 0x2;
		}
#if B_HOST_IS_BENDIAN
		fmt.byte_order = B_MEDIA_BIG_ENDIAN;
#else
		fmt.byte_order = B_MEDIA_LITTLE_ENDIAN;
#endif
		fmt.buffer_size = 0;
		_m_player = new PrivGameSound(fmt);
		status_t err = _m_player->InitCheck();
		if (err == B_OK) {
			_m_player->Start();
		}
		else {
			fprintf(stderr, "PrivGameSound: cannot create sound player (%s)\n",
					strerror(err));
			delete _m_player;
			_m_player = NULL;
		}
		atomic_or(&_m_didInit, 2);	//	signal completion
	}
	else while (!(_m_didInit & 2)) {
		snooze(50000);
	}
	return _m_player;
}

PrivGameSound *
PrivGameSound::CurPlayer()
{
	return _m_player;
}

void
PrivGameSound::Shutdown()
{
	if (_m_player != NULL) {
		_m_player->Stop();
		delete _m_player;
		_m_player = NULL;
		_m_didInit = 0;
	}
}

	static bool
	format_eq(
		const gs_audio_format & fmt1,
		const gs_audio_format & fmt2)
	{
		if ((fmt1.frame_rate > 0) && (fmt2.frame_rate > 0) &&
				(fabs(fmt1.frame_rate/fmt2.frame_rate-1.0) >= 0.005))
			return false;
		if ((fmt1.channel_count > 0) && (fmt2.channel_count > 0) &&
				(fmt1.channel_count != fmt2.channel_count))
			return false;
		if ((fmt1.format > 0) && (fmt2.format > 0) &&
				(fmt1.format != fmt2.format))
			return false;
		if ((fmt1.byte_order > 0) && (fmt2.byte_order > 0) &&
				(fmt1.byte_order != fmt2.byte_order))
			return false;
		//	ignore buffer_size
		return true;
	}

status_t 
PrivGameSound::MakeSound(const gs_audio_format &format, const void *data, size_t size, bool looping, gs_id *out_handle, int prev_ix)
{
	BAutolock lock(_m_lock);
	for (int ix=0; ix<MAX_SOUNDS; ix++) {
		if (_m_sounds[ix].data == NULL) {
			*out_handle = SLOT_TO_HANDLE(ix);
			if (prev_ix >= 0) {
				int32 nc = atomic_add(_m_sounds[prev_ix].ref_count, 1);
				assert(nc >= 1);
FPRINTF(stderr, "Setting ref count for %ld (%p) to %ld\n", ix, _m_sounds[prev_ix].data, _m_sounds[prev_ix].ref_count[0]);
				_m_sounds[ix].data = _m_sounds[prev_ix].data;
				_m_sounds[ix].ref_count = _m_sounds[prev_ix].ref_count;
			}
			else {
				_m_sounds[ix].data = (char *)rtm_alloc(get_game_pool(), size);
				if (_m_sounds[ix].data == NULL) {
					fprintf(stderr, "PrivGameSound: rtm_alloc(%ld) failed\n", size);
					return B_NO_MEMORY;
				}
				_m_sounds[ix].ref_count = (int32 *)rtm_alloc(get_game_pool(), sizeof(int32));
				if (_m_sounds[ix].ref_count == NULL) {
					rtm_free(_m_sounds[ix].data);
					_m_sounds[ix].data = NULL;
					fprintf(stderr, "PrivGameSound: rtm_alloc(%d) failed\n", sizeof(int32));
					return B_NO_MEMORY;
				}
				*_m_sounds[ix].ref_count = 1;
FPRINTF(stderr, "Setting ref count for %ld (%p) to %ld\n", ix, _m_sounds[ix].data, 1);
			}
			_m_sounds[ix].size = size;
			_m_sounds[ix].playing = false;
			_m_sounds[ix].offset = 0;
			_m_sounds[ix].format = format;
			if (_m_sounds[ix].format.frame_rate < 1) _m_sounds[ix].format.frame_rate = _m_format.frame_rate;
			if (!_m_sounds[ix].format.channel_count) _m_sounds[ix].format.channel_count = 1;
			if (!_m_sounds[ix].format.format) _m_sounds[ix].format.format = _m_format.format;
			if (!_m_sounds[ix].format.byte_order) _m_sounds[ix].format.format = _m_format.byte_order;
			if (!_m_sounds[ix].format.buffer_size) _m_sounds[ix].format.buffer_size = _m_format.buffer_size;
			_m_sounds[ix].looping = looping;
			if (prev_ix < 0) {
				_m_sounds[ix].optimized = _m_sounds[prev_ix].optimized;
			}
			else {
				_m_sounds[ix].optimized = format_eq(format, _m_format);
			}
			_m_sounds[ix].stereo = (format.channel_count > 1);
			_m_sounds[ix].callback = 0;
			_m_sounds[ix].cookie = 0;
			_m_sounds[ix].gain = UNITY_GAIN*SPLIT_GAIN;
			_m_sounds[ix].pan = 0;
			_m_sounds[ix].gain_ramp = 0;
			_m_sounds[ix].gain_l = MAX_GAIN*SPLIT_GAIN;
			_m_sounds[ix].gain_r = MAX_GAIN*SPLIT_GAIN;
			_m_sounds[ix].gain_l_delta = 0;
			_m_sounds[ix].gain_r_delta = 0;
			_m_sounds[ix].phase = int(_m_format.frame_rate);
			_m_sounds[ix].last_left = 0;
			_m_sounds[ix].last_right = 0;
			_m_sounds[ix].sr_ramp = 0;
			_m_sounds[ix].sr_target = format.frame_rate;
			_m_sounds[ix].sr_delta = 0.0;
			_m_sounds[ix].off_delta = 0;
			if (prev_ix < 0) {
				if (data != NULL) {
					memcpy(_m_sounds[ix].data, data, size);
				}
				else {
					if (_m_sounds[ix].format.format == 0x11) {
						memset(_m_sounds[ix].data, 0x80, size);
					}
					else {
						memset(_m_sounds[ix].data, 0x00, size);
					}
				}
			}
			return B_OK;
		}
	}
	return B_ERROR;
}

/*
status_t 
PrivGameSound::OptimizeSound(gs_id in_handle)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
//	fprintf(stderr, "optimizing handle %ld\n", in_handle);
	if (_m_sounds[ix].optimized) {
//		fprintf(stderr, "sound already optimized\n");
		return B_OK;
	}
	if (_m_sounds[ix].looping) return B_MISMATCHED_VALUES;

	return B_OK;	//	we no longer optimize sounds, because we dynamically re-sample

	//	some slop for imprecision in resampling
	size_t new_size = (size_t)(4+1.005*_m_format.frame_rate/_m_sounds[ix].format.frame_rate*_m_sounds[ix].size);
	new_size = new_size * (_m_format.format & 0xf) / (_m_sounds[ix].format.format & 0xf);
	gs_audio_format fmt(_m_format);
	snap_fr(fmt.frame_rate);
	fmt.channel_count = _m_sounds[ix].format.channel_count;
	char * data = (char *)rtm_alloc(get_game_pool(), new_size);
	if (data == NULL) {
//		fprintf(stderr, "OptimizeSound(%d): no memory (%d bytes)\n", in_handle, new_size);
		return B_NO_MEMORY;
	}
	_m_sounds[ix].offset = 0;
	//	do the actual format conversion
	convert_sound(_m_sounds[ix], data, new_size, fmt);
	if (atomic_add(_m_sounds[ix].ref_count, -1) == 1) {
		rtm_free(_m_sounds[ix].data);
		rtm_free(_m_sounds[ix].ref_count);
	}
	_m_sounds[ix].data = data;
	_m_sounds[ix].ref_count = (int32 *)(rtm_alloc(get_game_pool(), sizeof(int32)));
	*_m_sounds[ix].ref_count = 1;
	_m_sounds[ix].size = new_size;
	_m_sounds[ix].format = fmt;
	_m_sounds[ix].sr_target = fmt.frame_rate;
	_m_sounds[ix].optimized = true;
	return B_OK;
}
*/

status_t
PrivGameSound::SetCallback(
	gs_id in_handle,
	void (*callback)(void * cookie, gs_id handle, void * data, int32 offset, int32 size, size_t buf_size),
	void * cookie)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	const gs_audio_format &fmt(_m_sounds[ix].format);
//	if (!fr_eq(fmt.frame_rate, 44100.0) && !fr_eq(fmt.frame_rate, 22050.0) && 
//			!fr_eq(fmt.frame_rate, 11025.0)) {
//		fprintf(stderr, "PrivGameSound::SetCallback(): only 44.1, 22.05 and 11.025 kHz frame rates supported in this version\n");
//		fprintf(stderr, "framerate was %g\n", _m_sounds[ix].format.frame_rate);
//		return B_UNSUPPORTED;
//	}
	_m_sounds[ix].callback = callback;
	_m_sounds[ix].cookie = cookie;
	return B_OK;
}

status_t 
PrivGameSound::CloneSound(gs_id in_handle, gs_id *out_handle)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	return MakeSound(_m_sounds[ix].format, _m_sounds[ix].data, _m_sounds[ix].size,
			_m_sounds[ix].looping, out_handle, ix);
}

status_t 
PrivGameSound::ReleaseSound(gs_id in_handle)
{
	BAutolock lock(_m_lock);
//	fprintf(stderr, "releasing handle %d\n", in_handle);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	_m_sounds[ix].playing = false;
FPRINTF(stderr, "Setting ref count for %ld (%p) to %ld\n", ix, _m_sounds[ix].data, _m_sounds[ix].ref_count[0]-1);
	if (atomic_add(_m_sounds[ix].ref_count, -1) == 1) {
FPRINTF(stderr, "......... bye bye! (Release)\n");
		rtm_free(_m_sounds[ix].data);
		rtm_free(_m_sounds[ix].ref_count);
	}
	_m_sounds[ix].data = 0;
	_m_sounds[ix].ref_count = 0;
	return B_OK;
}

status_t 
PrivGameSound::GetSoundInfo(gs_id in_handle, gs_audio_format *out_format, void **out_buffer, size_t *out_size)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	*out_format = _m_sounds[ix].format;
	if (out_buffer) *out_buffer = _m_sounds[ix].data;
	if (out_size) *out_size = _m_sounds[ix].size;
	return B_OK;
}

status_t 
PrivGameSound::StartSound(gs_id in_handle, bool ignoreRestart)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if ((_m_sounds[ix].playing) && (ignoreRestart || _m_sounds[ix].looping)) return B_OK;
	_m_sounds[ix].offset = 0;
	bool set = false;
	if (!_m_playingAny) set = true;
	_m_sounds[ix].playing = true;
//	_m_playingMask |= (1LL << ix);
	_m_playingAny = true;
	if (set) SetHasData(true);
	return B_OK;
}

bool 
PrivGameSound::IsPlaying(gs_id in_handle)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	return _m_sounds[ix].playing;
}

status_t 
PrivGameSound::StopSound(gs_id in_handle)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if (!_m_sounds[ix].playing) return EALREADY;
	_m_sounds[ix].playing = false;
//	_m_playingMask &= ~(1LL << ix);
	_m_playingAny = false;
	for (int ix=0; ix<MAX_SOUNDS; ix++) {
		if (_m_sounds[ix].playing) {
			_m_playingAny = true;
			break;
		}
	}
//	if (!_m_playingMask) SetHasData(false);	//	done on demand in PlayBuffer instead
	return B_OK;
}

//	almost like StartSound(), except we don't re-set to 0 offset
status_t
PrivGameSound::ResumeSound(gs_id in_handle)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if (_m_sounds[ix].playing) return EALREADY;
	bool set = false;
	if (!_m_playingAny) set = true;
	_m_sounds[ix].playing = true;
	_m_playingAny = true;
	if (set) SetHasData(true);
	return B_OK;
}

status_t
PrivGameSound::SetSoundGain(
	gs_id in_handle,
	float gain,
	float ramp)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	_m_sounds[ix].gain = (int)(gain*UNITY_GAIN*SPLIT_GAIN);
	if (ramp <= 0.0) {
		_m_sounds[ix].gain_l = (int)(gain*UNITY_GAIN*SPLIT_GAIN - gain*_m_sounds[ix].pan*UNITY_GAIN);
		_m_sounds[ix].gain_r = (int)(gain*UNITY_GAIN*SPLIT_GAIN + gain*_m_sounds[ix].pan*UNITY_GAIN);
		_m_sounds[ix].gain_ramp = 0;
	}
	else {
		int count = (int)(_m_format.frame_rate*ramp);
		_m_sounds[ix].gain_l_delta = (int)(((gain*SPLIT_GAIN*UNITY_GAIN - gain*_m_sounds[ix].pan*UNITY_GAIN) - _m_sounds[ix].gain_l)/count);
		_m_sounds[ix].gain_r_delta = (int)(((gain*SPLIT_GAIN*UNITY_GAIN + gain*_m_sounds[ix].pan*UNITY_GAIN) - _m_sounds[ix].gain_r)/count);
		_m_sounds[ix].gain_ramp = count;
//		fprintf(stderr, "gain_ramp = %d; %d - %d\n", _m_sounds[ix].gain_ramp,
//				_m_sounds[ix].gain_l_delta, _m_sounds[ix].gain_r_delta);
	}
	return B_OK;
}

status_t
PrivGameSound::SetSoundPan(
	gs_id in_handle,
	float pan,
	float ramp)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	_m_sounds[ix].pan = (int)(pan*SPLIT_GAIN);
	if (ramp <= 0.0) {
		_m_sounds[ix].gain_l = (int)(_m_sounds[ix].gain - _m_sounds[ix].gain*pan);
		_m_sounds[ix].gain_r = (int)(_m_sounds[ix].gain + _m_sounds[ix].gain*pan);
		_m_sounds[ix].gain_ramp = 0;
	}
	else {
		int count = (int)(_m_format.frame_rate*ramp);
								//	pan is floating point -1.0 to 1.0 here!
		_m_sounds[ix].gain_l_delta = (int)(((_m_sounds[ix].gain - _m_sounds[ix].gain*pan) -
				_m_sounds[ix].gain_l)/count);
		_m_sounds[ix].gain_r_delta = (int)(((_m_sounds[ix].gain + _m_sounds[ix].gain*pan) -
				_m_sounds[ix].gain_r)/count);
		_m_sounds[ix].gain_ramp = count;
//		fprintf(stderr, "gain_ramp = %d; %d - %d\n", _m_sounds[ix].gain_ramp,
//				_m_sounds[ix].gain_l_delta, _m_sounds[ix].gain_r_delta);
	}
	return B_OK;
}

status_t
PrivGameSound::SetSoundSamplingRate(
	gs_id in_handle,
	float rate,
	float ramp)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if (rate < 10.0) return B_BAD_VALUE;
	if (rate > 200000.0) return B_BAD_VALUE;
//	_m_sounds[ix].pan = (int)(pan*SPLIT_GAIN);
	_m_sounds[ix].sr_target = rate;
	if (ramp <= 0.0) {
//		_m_sounds[ix].gain_l = (int)(_m_sounds[ix].gain - _m_sounds[ix].gain*pan);
//		_m_sounds[ix].gain_r = (int)(_m_sounds[ix].gain + _m_sounds[ix].gain*pan);
		_m_sounds[ix].format.frame_rate = rate;
		_m_sounds[ix].sr_ramp = 0;
	}
	else {
		int count = (int)(_m_format.frame_rate*ramp);
								//	pan is floating point -1.0 to 1.0 here!
//		_m_sounds[ix].gain_l_delta = (int)(((_m_sounds[ix].gain - _m_sounds[ix].gain*pan) -
//				_m_sounds[ix].gain_l)/count);
//		_m_sounds[ix].gain_r_delta = (int)(((_m_sounds[ix].gain + _m_sounds[ix].gain*pan) -
//				_m_sounds[ix].gain_r)/count);
		_m_sounds[ix].sr_delta = (rate-_m_sounds[ix].format.frame_rate)/count;
		_m_sounds[ix].sr_ramp = count;
//		fprintf(stderr, "gain_ramp = %d; %d - %d\n", _m_sounds[ix].gain_ramp,
//				_m_sounds[ix].gain_l_delta, _m_sounds[ix].gain_r_delta);
	}
	return B_OK;
}

status_t
PrivGameSound::SetSoundLooping(
	gs_id in_handle,
	bool looping)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	_m_sounds[ix].looping = looping;
	return B_OK;
}

status_t
PrivGameSound::GetSoundGain(
	gs_id in_handle,
	float * gain,
	float * ramp)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if (gain) *gain = _m_sounds[ix].gain/((float)UNITY_GAIN*SPLIT_GAIN);
	if (ramp) *ramp = _m_sounds[ix].gain_ramp/_m_format.frame_rate;
	return B_OK;
}

status_t
PrivGameSound::GetSoundPan(
	gs_id in_handle,
	float * pan,
	float * ramp)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if (pan) *pan = _m_sounds[ix].pan/(float)SPLIT_GAIN;
	if (ramp) *ramp = _m_sounds[ix].gain_ramp/_m_format.frame_rate;
	return B_OK;
}

status_t
PrivGameSound::GetSoundSamplingRate(
	gs_id in_handle,
	float * rate,
	float * ramp)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if (rate) *rate = _m_sounds[ix].sr_target;
	if (ramp) *ramp = _m_sounds[ix].sr_ramp/_m_format.frame_rate;
	return B_OK;
}

status_t
PrivGameSound::GetSoundLooping(
	gs_id in_handle,
	bool * looping)
{
	BAutolock lock(_m_lock);
	if (!VALID_HANDLE(in_handle)) return B_BAD_VALUE;
	int ix = HANDLE_TO_SLOT(in_handle);
	if (!looping) return B_BAD_VALUE;
	*looping = _m_sounds[ix].looping;
	return B_OK;
}


	static const char * get_name()
	{
		static char name[256];
		if (name[0]) return name;
		if (be_app != NULL) {
			app_info info;
			be_app->GetAppInfo(&info);
			strcpy(name, info.ref.name);
		}
		else {
			strcpy(name, "Some GameSound");
		}
		return name;
	}

PrivGameSound::PrivGameSound(const gs_audio_format & format) :
	BSoundPlayer(reinterpret_cast<const media_raw_audio_format *>(&format), get_name()),
	_m_lock(get_name())
{
	media_raw_audio_format f = Format();
	_m_format = *reinterpret_cast<gs_audio_format *>(&f);
	_m_playingAny = false;
	_m_sounds = reinterpret_cast<game_sound *>(rtm_alloc(get_game_pool(), sizeof(game_sound)*MAX_SOUNDS));
	if (_m_sounds == NULL) {
		SetInitError(B_NO_MEMORY);
		return;
	}
	memset(_m_sounds, 0, sizeof(game_sound)*MAX_SOUNDS);
	SetHasData(true);	//	will go back to 0 after playing one buffer
#if _SUPPORTS_MEDIA_NODES
	_BMediaRosterP::AddCleanupFunction(gamesound_cleanup, NULL);
#else
	add_cleanup(gamesound_cleanup);
#endif
}


PrivGameSound::~PrivGameSound()
{
	//fixme	we should have a debug-only check that everything
	//	gets released, but nothing gets released twice.
	if (_m_sounds != NULL) {
		for (int ix=0; ix<MAX_SOUNDS; ix++) {
			if (_m_sounds[ix].ref_count != NULL) {
FPRINTF(stderr, "Setting ref count for %ld (%p) to %ld\n", ix, _m_sounds[ix].data, _m_sounds[ix].ref_count[0]-1);
				if (atomic_add(_m_sounds[ix].ref_count, -1) == 1) {
FPRINTF(stderr, ".............bye, bye! (destructor)\n");
					rtm_free(_m_sounds[ix].data);
					rtm_free(_m_sounds[ix].ref_count);
					_m_sounds[ix].data = NULL;
					_m_sounds[ix].ref_count = NULL;
				}
			}
		}
	}
	rtm_free(_m_sounds);
#if _SUPPORTS_MEDIA_NODES
	_BMediaRosterP::RemoveCleanupFunction(gamesound_cleanup, NULL);
#else
	remove_cleanup(gamesound_cleanup);
#endif
}

void 
PrivGameSound::PlayBuffer(void *data, size_t size, const media_raw_audio_format &format)
{
	BAutolock lock(_m_lock);
	if (!_m_playingAny) {
		if (format.format == 0x11) {
			memset(data, 0x80, size);
		}
		else {
			memset(data, 0, size);
		}
		SetHasData(false);
		return;
	}
	const gs_audio_format & fmt(*reinterpret_cast<const gs_audio_format *>(&format));
	bool first = true;
	for (int ix=0; ix<MAX_SOUNDS; ix++) {
		if (_m_sounds[ix].playing) {
			//	do callbacks
			size_t cvt_size;
//			fprintf(stderr, "format.frame_rate: %g,  fmt.frame_rate: %g,  size: %d\n",
//					_m_sounds[ix].format.frame_rate, fmt.frame_rate, size);
			if (_m_sounds[ix].sr_target > _m_sounds[ix].format.frame_rate) {
				cvt_size = (size_t)ceil(size*(_m_sounds[ix].format.format&0xf)*_m_sounds[ix].sr_target*_m_sounds[ix].format.channel_count/
						(format.frame_rate*(format.format&0xf)*format.channel_count));
			}
			else {
				cvt_size = (size_t)ceil(size*(_m_sounds[ix].format.format&0xf)*_m_sounds[ix].format.frame_rate*_m_sounds[ix].format.channel_count/
						(format.frame_rate*(format.format&0xf)*format.channel_count));
			}
			cvt_size = (cvt_size+3)&~3;
			if (_m_sounds[ix].callback) {
//				fprintf(stderr, "callback is non-0\n");
				assert(_m_sounds[ix].off_delta >= 0);
				if (_m_sounds[ix].off_delta < cvt_size) {
					(*_m_sounds[ix].callback)(_m_sounds[ix].cookie, SLOT_TO_HANDLE(ix), _m_sounds[ix].data,
							_m_sounds[ix].offset+_m_sounds[ix].off_delta, cvt_size-_m_sounds[ix].off_delta,
							_m_sounds[ix].size);
				}
			}
			//	playing may change inside callback
			bool wasPlaying =  _m_sounds[ix].playing;
			int32 old_offset = _m_sounds[ix].offset;
			if (first) {
				copy_sound(_m_sounds[ix], data, size, fmt);
				first = false;
			}
			else {
				mix_sound(_m_sounds[ix], data, size, fmt);
			}
			if (wasPlaying && !_m_sounds[ix].playing) {
				_m_sounds[ix].off_delta = 0;
			}
			else {
				if (old_offset > _m_sounds[ix].offset) {	//	wrap?
					_m_sounds[ix].off_delta = old_offset+cvt_size-(_m_sounds[ix].offset+_m_sounds[ix].size);
				}
				else {
					_m_sounds[ix].off_delta = old_offset+cvt_size-_m_sounds[ix].offset;
				}
			}
#if 0
			fprintf(stderr, "offset %ld, old %ld, cvt_size %ld, delta %ld\n",
					_m_sounds[ix].offset, old_offset, cvt_size, _m_sounds[ix].off_delta);
#endif
		}
	}
	_m_playingAny = !first;
}


namespace BPrivate {
inline int16 clamp_16(float f)
{
	if (f >= 1.0) return 32767;
	if (f <= -1.0) return -32767;
	return (int16)(f*32767);
}
inline uint8 clamp_8(float f)
{
	if (f >= 1.0) return 255;
	if (f <= -1.0) return 1;
	return (uint8)(f*127+128);
}
}
using namespace BPrivate;

void
PrivGameSound::convert_sound(
	game_sound & sound,
	void * buf,
	size_t size,
	const gs_audio_format & outfmt)
{
//	fprintf(stderr, "PrivGameSound::convert_sound()\n");
//	fprintf(stderr, "input: %lx %s @ %.3f kHz\n", sound.format.format,
//			(sound.format.channel_count == 2) ? "stereo" : "mono",
//			sound.format.frame_rate);
//	fprintf(stderr, "output: %lx %s @ %.3f kHz\n", outfmt.format,
//			(outfmt.channel_count == 2) ? "stereo" : "mono",
//			outfmt.frame_rate);
//	assert(_m_lock.IsLocked());
	//	going from stereo to mono is a bad idea, and vice versa is not good
	assert(outfmt.channel_count == sound.format.channel_count);
	assert((outfmt.format == 0x2) || (outfmt.format == 0x11));
	assert((sound.format.format == 0x2) || (sound.format.format == 0x11));
	//	we only support downsampling in this version
	assert(sound.format.frame_rate <= outfmt.frame_rate);

	bool stereo = (outfmt.channel_count == 2);
	float samp1 = 0;
	float samp2 = 0;
	float mem1 = 0;
	float mem2 = 0;
	float inPhase = 1.0;
	const float inDelta = sound.format.frame_rate/outfmt.frame_rate;
	const float filter_n = inDelta;
	const float filter_o = 1.0-inDelta;

	void * dest = buf;
	const void * src = sound.data;
	int32 srcSize = sound.size;

	//	if speed was necessary, we could factor this loop 8 ways...
	while (true) {
		if (inPhase >= 1.0) {	//	get new sample
			if (srcSize <= 0) {
				break;	//	done
			}
			inPhase -= 1.0;
			switch (sound.format.format) {
			case 0x2:
				samp1 = *(int16*)src/32767.0;
				src = &((int16 *)src)[1];
				if (stereo) {
					samp2 = *(int16*)src/32767.0;
					src = &((int16 *)src)[1];
				}
				break;
			case 0x11:
				samp1 = (*(uint8*)src-128)/127.0;
				src = &((uint8 *)src)[1];
				if (stereo) {
					samp2 = (*(uint8*)src-128)/127.0;
					src = &((uint8 *)src)[1];
				}
				break;
			}
			srcSize -= (sound.format.format & 0xf) * (stereo ? 2 : 1);
		}
		//	run our integrating el-cheapo filter
		switch (outfmt.format) {
		case 0x2:
			mem1 = mem1*filter_o+samp1*filter_n;
			*(int16 *)dest = clamp_16(mem1);
			dest = &((int16 *)dest)[1];
			if (stereo) {
				mem2 = mem2*filter_o+samp2*filter_n;
				*(int16 *)dest = clamp_16(mem2);
				dest = &((int16 *)dest)[1];
			}
			break;
		case 0x11:
			mem1 = mem1*filter_o+samp1*filter_n;
			*(uint8 *)dest = clamp_8(mem1);
			dest = &((uint8 *)dest)[1];
			if (stereo) {
				mem2 = mem2*filter_o+samp2*filter_n;
				*(uint8 *)dest = clamp_8(mem2);
				dest = &((uint8 *)dest)[1];
			}
			break;
		}
		inPhase += inDelta;
	}
	int32 toClear = (((char *)buf)+size-(char*)dest);
//	fprintf(stderr, "convert_sound(): %ld bytes to clear\n", toClear);
	if (toClear > 0) {
		if (outfmt.format == 0x11) {
			memset(dest, 0x80, toClear);
		}
		else {
			memset(dest, 0x00, toClear);
		}
	}
}

void
PrivGameSound::copy_sound(
	game_sound & sound,
	void * dest,
	size_t size,
	const gs_audio_format & outfmt)
{
	assert(outfmt.channel_count == 2);
	assert(outfmt.format == 0x2);

//	if (!sound.optimized && !sound.callback) {
//		fprintf(stderr, "WARNING: ignoring non-optimized sound with no callback\n");
//		return;
//	}
	//	we only do integral-factor sample rate conversion for now
//	int copies = (int)(outfmt.frame_rate/sound.format.frame_rate);
	bool expand = (sound.format.channel_count == 1);
	int frames = size/(2*(outfmt.format & 0xf));	//	samples in output buffer
//	int inframes = (int(frames*sound.format.frame_rate/outfmt.format.frame_rate);
	const char * end = ((char *)dest)+size;
	int phase = sound.phase;
	int target = int(outfmt.frame_rate);
	int delta = int(sound.format.frame_rate);

#if 0
	fprintf(stderr, "copy_sound( 0%x , %d , %d , %d )\n", sound.data+sound.offset, phase, target, delta);
#endif

	//	0-th order sample rate conversion (nearest-sample)
	int samp1 = sound.last_left;
	int samp2 = sound.last_right;
	if (sound.format.format == 0x11) {
		//	format expansion
		int conv_l = 258 * DIVIDE_UNITY_GAIN(sound.gain_l);
		int conv_r = 258 * DIVIDE_UNITY_GAIN(sound.gain_r);
		while (frames > 0) {
			//	this conversion is as close as we can get without fractional bit arithmetics
			while (phase >= target) {
				samp1 = (int)(((int)*((uchar *)sound.data+sound.offset)-128)*conv_l);
				if (samp1 < -SAMP_RANGE) samp1 = -SAMP_RANGE;
				if (samp1 > SAMP_RANGE) samp1 = SAMP_RANGE;
				if (expand) {
					samp2 = (int)(((int)*((uchar *)sound.data+sound.offset)-128)*conv_r);
					sound.offset += 1;
				}
				else {
					samp2 = (int)(((int)*((uchar *)sound.data+sound.offset+1)-128)*conv_r);
					sound.offset += 2;
				}
				if (samp2 < -SAMP_RANGE) samp2 = -SAMP_RANGE;
				if (samp2 > SAMP_RANGE) samp2 = SAMP_RANGE;
				phase -= target;
				if (sound.offset >= sound.size) {
					if (sound.looping) {
//						fprintf(stderr, "sound.looping is true\n");
						sound.offset = 0;
					}
					else {
						sound.playing = false;
						samp1 = samp2 = 0;
						frames = 1;		//	force a break
					}
				}
			}
//			for (int ix=0; ix<copies; ix++) {
			*(int16*)dest = samp1>>14;
			((int16*)dest)[1] = samp2>>14;
			dest = &((int16 *)dest)[2];
			phase += delta;
//			}
			if (sound.gain_ramp) {
				sound.gain_l += sound.gain_l_delta;
				sound.gain_r += sound.gain_r_delta;
				sound.gain_ramp--;
				conv_l = 258 * DIVIDE_UNITY_GAIN(sound.gain_l);
				conv_r = 258 * DIVIDE_UNITY_GAIN(sound.gain_r);
			}
			if (sound.sr_ramp) {
				sound.format.frame_rate += sound.sr_delta;
				delta = int(sound.format.frame_rate);
				if (!--sound.sr_ramp) {
					sound.format.frame_rate = sound.sr_target;
				}
			}
			frames--;
		}
	}
	else {
		int conv_l = DIVIDE_UNITY_GAIN(sound.gain_l);
		int conv_r = DIVIDE_UNITY_GAIN(sound.gain_r);
		while (frames > 0) {
			//	this conversion is as close as we can get without fractional bit arithmetics
			while (phase >= target) {
				samp1 = (int)(*(int16 *)(sound.data+sound.offset) * conv_l);
				if (samp1 < -SAMP_RANGE) samp1 = -SAMP_RANGE;
				if (samp1 > SAMP_RANGE) samp1 = SAMP_RANGE;
				if (expand) {
					samp2 = (int)(*(int16 *)(sound.data+sound.offset) * conv_r);
					sound.offset += 2;
				}
				else {
					samp2 = (int)(*(int16 *)(sound.data+sound.offset+2) * conv_r);
					sound.offset += 4;
				}
				if (samp2 < -SAMP_RANGE) samp2 = -SAMP_RANGE;
				if (samp2 > SAMP_RANGE) samp2 = SAMP_RANGE;
				phase -= target;
				if (sound.offset >= sound.size) {
					if (sound.looping) {
						sound.offset = 0;
					}
					else {
						sound.playing = false;
						samp1 = 0;
						samp2 = 0;
						break;
					}
				}
			}
//			for (int ix=0; ix<copies; ix++) {
			*(int16*)dest = samp1>>14;
			((int16*)dest)[1] = samp2>>14;
			dest = &((int16 *)dest)[2];
			phase += delta;
//			}
			if (sound.gain_ramp) {
				sound.gain_l += sound.gain_l_delta;
				sound.gain_r += sound.gain_r_delta;
				sound.gain_ramp--;
				conv_l = DIVIDE_UNITY_GAIN(sound.gain_l);
				conv_r = DIVIDE_UNITY_GAIN(sound.gain_r);
			}
			if (sound.sr_ramp) {
				sound.format.frame_rate += sound.sr_delta;
				delta = int(sound.format.frame_rate);
				if (!--sound.sr_ramp) {
					sound.format.frame_rate = sound.sr_target;
				}
			}
			frames--;
		}
	}
	sound.last_left = samp1;
	sound.last_right = samp2;
	sound.phase = phase;
	if ((char *)dest < end) {
		if (outfmt.format == 0x11) {
			memset(dest, 0x80, end-(char *)dest);
		}
		else {
			memset(dest, 0, end-(char *)dest);
		}
	}
}

void
PrivGameSound::mix_sound(
	game_sound & sound,
	void * dest,
	size_t size,
	const gs_audio_format & outfmt)
{
	assert(outfmt.channel_count == 2);
	assert(outfmt.format == 0x2);

//	if (!sound.optimized && !sound.callback) {
//		fprintf(stderr, "WARNING: ignoring non-optimized sound with no callback\n");
//		return;
//	}
	//	we only do integral-factor sample rate conversion for now
//	int copies = (int)(outfmt.frame_rate/sound.format.frame_rate);
	bool expand = (sound.format.channel_count == 1);
	int frames = size/(2*(outfmt.format & 0xf));	//	samples in output buffer
//	int inframes = (int(frames*sound.format.frame_rate/outfmt.format.frame_rate);
	const char * end = ((char *)dest)+size;
	int phase = sound.phase;
	int target = int(outfmt.frame_rate);
	int delta = int(sound.format.frame_rate);

	//	0-th order sample rate conversion (nearest-sample)
	int samp1 = sound.last_left;
	int samp2 = sound.last_right;
	if (sound.format.format == 0x11) {
		//	format expansion
		int conv_l = 258 * DIVIDE_UNITY_GAIN(sound.gain_l);
		int conv_r = 258 * DIVIDE_UNITY_GAIN(sound.gain_r);
		while (frames > 0) {
			//	this conversion is as close as we can get without fractional bit arithmetics
			while (phase >= target) {
				samp1 = (int)(((int)*((uchar *)sound.data+sound.offset)-128)*conv_l);
				if (samp1 < -SAMP_RANGE) samp1 = -SAMP_RANGE;
				if (samp1 > SAMP_RANGE) samp1 = SAMP_RANGE;
				if (expand) {
					samp2 = (int)(((int)*((uchar *)sound.data+sound.offset)-128)*conv_r);
					sound.offset += 1;
				}
				else {
					samp2 = (int)(((int)*((uchar *)sound.data+sound.offset+1)-128)*conv_r);
					sound.offset += 2;
				}
				if (samp2 < -SAMP_RANGE) samp2 = -SAMP_RANGE;
				if (samp2 > SAMP_RANGE) samp2 = SAMP_RANGE;
				phase -= target;
				if (sound.offset >= sound.size) {
					if (sound.looping) {
						sound.offset = 0;
					}
					else {
						sound.playing = false;
						samp1 = samp2 = 0;
						break;
					}
				}
			}
//			for (int ix=0; ix<copies; ix++) {
			int d = ((int16 *)dest)[0]+(samp1>>14);
			if (d < -32767) d = -32767;
			if (d > 32767) d = 32767;
			((int16 *)dest)[0] = d;
			d = ((int16 *)dest)[1]+(samp2>>14);
			if (d < -32767) d = -32767;
			if (d > 32767) d = 32767;
			((int16 *)dest)[1] = d;
			dest = &((int16 *)dest)[2];
			phase += delta;
//			}
			if (sound.gain_ramp) {
				sound.gain_l += sound.gain_l_delta;
				sound.gain_r += sound.gain_r_delta;
				sound.gain_ramp--;
				conv_l = 258 * DIVIDE_UNITY_GAIN(sound.gain_l);
				conv_r = 258 * DIVIDE_UNITY_GAIN(sound.gain_r);
			}
			if (sound.sr_ramp) {
				sound.format.frame_rate += sound.sr_delta;
				delta = int(sound.format.frame_rate);
				if (!--sound.sr_ramp) {
					sound.format.frame_rate = sound.sr_target;
				}
			}
			frames--;
		}
	}
	else {
		int conv_l = DIVIDE_UNITY_GAIN(sound.gain_l);
		int conv_r = DIVIDE_UNITY_GAIN(sound.gain_r);
		while (frames > 0) {
			//	this conversion is as close as we can get without fractional bit arithmetics
			while (phase >= target) {
				samp1 = (int)(*(int16 *)(sound.data+sound.offset) * conv_l);
				if (samp1 < -SAMP_RANGE) samp1 = -SAMP_RANGE;
				if (samp1 > SAMP_RANGE) samp1 = SAMP_RANGE;
				if (expand) {
					samp2 = (int)(*(int16 *)(sound.data+sound.offset) * conv_r);
					sound.offset += 2;
				}
				else {
					samp2 = (int)(*(int16 *)(sound.data+sound.offset+2) * conv_r);
					sound.offset += 4;
				}
				if (samp2 < -SAMP_RANGE) samp2 = -SAMP_RANGE;
				if (samp2 > SAMP_RANGE) samp2 = SAMP_RANGE;
				phase -= target;
				if (sound.offset >= sound.size) {
					if (sound.looping) {
						sound.offset = 0;
					}
					else {
						sound.playing = false;
						samp1 = 0;
						samp2 = 0;
						break;
					}
				}
			}
//			for (int ix=0; ix<copies; ix++) {
			int d = ((int16 *)dest)[0]+(samp1>>14);
			if (d < -32767) d = -32767;
			if (d > 32767) d = 32767;
			((int16 *)dest)[0] = d;
			d = ((int16 *)dest)[1]+(samp2>>14);
			if (d < -32767) d = -32767;
			if (d > 32767) d = 32767;
			((int16 *)dest)[1] = d;
			dest = &((int16 *)dest)[2];
			phase += delta;
//			}
			if (sound.gain_ramp) {
				sound.gain_l += sound.gain_l_delta;
				sound.gain_r += sound.gain_r_delta;
				sound.gain_ramp--;
				conv_l = DIVIDE_UNITY_GAIN(sound.gain_l);
				conv_r = DIVIDE_UNITY_GAIN(sound.gain_r);
			}
			if (sound.sr_ramp) {
				sound.format.frame_rate += sound.sr_delta;
				delta = int(sound.format.frame_rate);
				if (!--sound.sr_ramp) {
					sound.format.frame_rate = sound.sr_target;
				}
			}
			frames--;
		}
	}
	sound.last_left = samp1;
	sound.last_right = samp2;
	sound.phase = phase;
}

