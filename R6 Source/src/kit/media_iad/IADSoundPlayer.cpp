
#include <ByteOrder.h>
#include <SoundPlayer.h>
#include <Autolock.h>
#include <Application.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#include "miniplay.h"



#if !NDEBUG
#define FPRINTF fprintf
#else
#define FPRINTF
#endif

#define RESAMPLE //FPRINTF
#define LOCATION //FPRINTF
#define NODE //FPRINTF
#define DATA //FPRINTF
#define CONNECT FPRINTF
#define LATENCY //FPRINTF

//	don't take this out; developers need to see this info!
#define DIAGNOSTIC fprintf


BSoundPlayer::BSoundPlayer(
	const media_raw_audio_format * format,
	const char * name,
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void * cookie, sound_player_notification what, ...),
	void * cookie) :
	_m_lock(name ? name : "BSoundPlayer")
{
fprintf(stderr, "Frame rate is %f, sample format 0x%lx, channel count %ld\n", format->frame_rate, format->format, format->channel_count);
	Init(format, name, PlayBuffer, Notifier, cookie);
}

void BSoundPlayer::Init(
	const media_raw_audio_format * format,
	const char * name,
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void * cookie, sound_player_notification what, ...),
	void * cookie)
{
	NODE(stderr, "BSoundPlayer::Init()\n");
	_PlayBuffer = PlayBuffer;
	_m_cookie = cookie;
	_Notifier = Notifier;
	_m_buf = 0; // deprecated
	_m_bufsize = 0;
	_m_has_data = 0;
	_m_thread = -1;
	_m_sem = -1;
	_m_playID = -1;
	_m_stop_go = create_sem(0, "stop_go");
	_m_init_err = B_OK;
	_m_format = *format;
	_m_volume = 1.0;
	if (_m_format.frame_rate < 4000.0) {
		_m_format.frame_rate = 44100.0;
	}
	else if (_m_format.frame_rate > 65535.0) {
		_m_format.frame_rate = 44100.0;
	}
	if ((_m_format.format != 0x11) && (_m_format.format != 0x2)) {
		_m_format.format = 0x2;
	}
	if (_m_format.channel_count == media_multi_audio_format::wildcard.channel_count) {
		_m_format.channel_count = 2;
	}
	_m_format.byte_order = B_MEDIA_HOST_ENDIAN;
	_m_perfTime = 0;
}

BSoundPlayer::~BSoundPlayer()
{
	fprintf(stderr, "~BSoundPlayer()\n");
	ASSERT(!_m_lock.IsLocked());
	Stop();
}

status_t
BSoundPlayer::Start()
{
	BAutolock lock(_m_lock);
	if (_m_thread > 0) return EALREADY;
	_m_bufsize = 0;
	media_multi_audio_format streamFormat = media_multi_audio_format::wildcard;
	memcpy(&streamFormat, &_m_format, sizeof(_m_format));
	_m_playID = mini_new_output_stream(&streamFormat, 2);
	if (_m_playID < 0) return (_m_init_err = _m_playID);
	if (_m_format.buffer_size > 0 && _m_format.buffer_size != streamFormat.buffer_size) {
		fprintf(stderr, "BSoundPlayer::Start(): abort: requested buffer_size %ld, got %ld\n",
			_m_format.buffer_size, streamFormat.buffer_size);
		mini_close_output_stream(_m_playID);
		return B_MEDIA_BAD_FORMAT;
	}
	memcpy(&_m_format, &streamFormat, sizeof(_m_format));
	const size_t frameSize = (_m_format.format & 0xf)*_m_format.channel_count;
	_m_bufsize = _m_format.buffer_size / frameSize;
fprintf(stderr, "_m_format.buffer_size = %ld (fmt 0x%lx, chan %ld, frames %ld)\n",
		_m_format.buffer_size, _m_format.format, _m_format.channel_count, _m_bufsize);

	mini_set_volume(_m_playID, _m_volume, _m_volume);

	//	spawn thread
	_m_stop_go = create_sem(0, "sound stop go");
	if (_m_stop_go < 0) {
		mini_close_output_stream(_m_playID);
		_m_playID = -1;
		return (_m_init_err = _m_stop_go);
	}
	_m_thread = spawn_thread(player_thread, "player_thread", 115, this);
	if (_m_thread < 0) {
		delete_sem(_m_stop_go);
		_m_stop_go = -1;
		mini_close_output_stream(_m_playID);
		_m_playID = -1;
		return (_m_init_err = _m_thread);
	}
	SetHasData(true);
	resume_thread(_m_thread);
	return B_OK;
}

void BSoundPlayer::Stop(
	bool block,
	bool flush)
{
	//	signal termination
	delete_sem(_m_stop_go);
	_m_stop_go = -1;
	_m_has_data = 0;
	status_t err;
	//	wait for thread
	if (find_thread(0) != _m_thread) wait_for_thread(_m_thread, &err);
}

BSoundPlayer::BufferPlayerFunc BSoundPlayer::BufferPlayer() const
{
	return _PlayBuffer;
}

void BSoundPlayer::SetBufferPlayer(
	void (*PlayBuffer)(void * cookie, void * buffer, size_t size, const media_raw_audio_format & format))
{
	BAutolock lock(_m_lock);
	_PlayBuffer = PlayBuffer;
}

BSoundPlayer::EventNotifierFunc BSoundPlayer::EventNotifier() const
{
	return _Notifier;
}

void BSoundPlayer::SetNotifier(
	void (*Notifier)(void * cookie, sound_player_notification what, ...))
{
	BAutolock lock(_m_lock);
	_Notifier = Notifier;
}

void * BSoundPlayer::Cookie() const
{
	return _m_cookie;
}

void BSoundPlayer::SetCookie(void * cookie)
{
	BAutolock lock(_m_lock);
	_m_cookie = cookie;
}

void BSoundPlayer::SetCallbacks(
	void (*PlayBuffer)(void *, void * buffer, size_t size, const media_raw_audio_format & format),
	void (*Notifier)(void *, sound_player_notification what, ...),
	void * cookie)
{
	BAutolock lock(_m_lock);
	_PlayBuffer = PlayBuffer;
	_Notifier = Notifier;
	_m_cookie = cookie;
}

bigtime_t BSoundPlayer::Latency()
{
	return 20000LL;
}

bigtime_t BSoundPlayer::CurrentTime()
{
	return _m_perfTime;
}

bigtime_t BSoundPlayer::PerformanceTime()
{
	return _m_perfTime;
}

void
BSoundPlayer::SetHasData(
	bool has_data)
{
	BAutolock lock(_m_lock);
	if (has_data) {
		if (!atomic_or(&_m_has_data, 1))
			release_sem_etc(_m_stop_go, 1, B_DO_NOT_RESCHEDULE);
	}
	else {
		atomic_and(&_m_has_data, ~1);
	}
}

bool
BSoundPlayer::HasData()
{
	return _m_has_data;
}

static float map_from_db(float db)
{
	float ret = exp(db/6*log(2));
	return ret;
}

static float map_to_db(float straight)
{
	if (straight < 0.000001) straight = 0.000001;
	float ret = 6*log(straight)/log(2);
	return ret;
}

float BSoundPlayer::Volume()
{
	return _m_volume;
}

void BSoundPlayer::SetVolume(
	float new_volume)
{
	_m_volume = new_volume;
	mini_set_volume(_m_playID, new_volume, new_volume);
}

void BSoundPlayer::PlayBuffer(
	void * buffer,
	size_t size,
	const media_raw_audio_format & format)
{
	puts("PlayBuffer() called???");
	memset(buffer, 0, size);
}


status_t
BSoundPlayer::InitCheck()
{
	return _m_init_err;
}

void
BSoundPlayer::SetInitError(
	status_t in_error)
{
	_m_init_err = in_error;
}


media_raw_audio_format
BSoundPlayer::Format() const
{
	return reinterpret_cast<const media_raw_audio_format&>(_m_format);
}


status_t 
BSoundPlayer::player_thread(void * arg)
{
	BSoundPlayer * sp = (BSoundPlayer *)arg;
	int64 fcnt = 0;
	int frameSize = (sp->_m_format.format & 0xf)*sp->_m_format.channel_count;
	int32 fpb = sp->_m_format.buffer_size/frameSize;
	bigtime_t a = system_time(), b;
	off_t duration = (bigtime_t)(fpb*1000000LL/sp->_m_format.frame_rate);
	while (1) {
		if (!sp->_m_has_data) {
fprintf(stderr, "SoundPlayer: no data, waiting\n");
			status_t err;
			if ((err = acquire_sem(sp->_m_stop_go)) == B_BAD_SEM_ID)
				break;
			if (err < 0) {
fprintf(stderr, "player_thread acquire_sem() error: %s\n", strerror(err));
				snooze(1000);	//	avert potential disaster
				continue;
			}
			b = system_time();
			int64 f = int64((b-a)*(double)sp->_m_format.frame_rate/1000000LL);
			f -= (f % fpb);	//	round down to nearest whole buffer
			fcnt += f;
			sp->_m_perfTime = int64(fcnt*1000000LL/(double)sp->_m_format.frame_rate);
			a += (bigtime_t)(1000000LL*f/sp->_m_format.frame_rate);
			continue;
		}
		int32 buf_id;
		size_t buf_size;
		void* buf;
		status_t err = mini_acquire_output_buffer(sp->_m_playID,
			&buf_id, &buf_size, &buf, 2000000LL);
		if(err < B_OK) {
			fprintf(stderr,
				"SoundPlayer: buffer acquire failed: %s\nABORTING\n",
				strerror(err));
			break;
		}
		sp->_m_lock.Lock();
		if (sp->_PlayBuffer) {
			(*sp->_PlayBuffer)(sp->_m_cookie, buf, buf_size, sp->_m_format);
		}
		else {
			sp->PlayBuffer(buf, buf_size, sp->_m_format);
		}
		fcnt += fpb;
		sp->_m_perfTime = int64(fcnt*1000000LL/(double)sp->_m_format.frame_rate);
		sp->_m_lock.Unlock();
		mini_queue_output_buffer(buf_id);
//		b = system_time();
//		if (b-a > fpb*1000000L/sp->_m_format.frame_rate+1000LL) {
//			DIAGNOSTIC(stderr, "one buffer took %f ms\n", (float)(b-a)/1000.0);
//		}
//		snooze_until(a+duration-2000LL, 0);
		a = system_time();
	}
	sp->_m_lock.Lock();
	sp->_m_thread = -1;
	mini_close_output_stream(sp->_m_playID);
	sp->_m_playID = -1;
	sp->_m_lock.Unlock();
	return B_OK;
}

void 
BSoundPlayer::Notify(BSoundPlayer::sound_player_notification what, ...)
{
	//	placeholder
}




status_t BSoundPlayer::_Reserved_SoundPlayer_0(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_1(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_2(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_3(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_4(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_5(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_6(void *, ...) { return B_ERROR; }
status_t BSoundPlayer::_Reserved_SoundPlayer_7(void *, ...) { return B_ERROR; }
