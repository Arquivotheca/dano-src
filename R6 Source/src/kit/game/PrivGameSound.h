#if !defined(PrivGameSound_h)
#define PrivGameSound_h 1

#include <SoundPlayer.h>
#include <rt_alloc.h>
#include "GameSoundDefs.h"

class BGameSound;
class BSoundFile;

namespace BPrivate {

extern rtm_pool * get_game_pool();

class PrivGameSound : public BSoundPlayer
{
public:

static	PrivGameSound * MakePlayer(
							const gs_audio_format & format);
static	PrivGameSound * CurPlayer();
static	void Shutdown();

		status_t MakeSound(
							const gs_audio_format & format,
							const void * data,
							size_t size,
							bool looping,
							gs_id * out_handle,
							int prev_ix = -1);
		status_t OptimizeSound(
							gs_id in_handle);
		status_t SetCallback(	//	the callback may be called with wrap-around inducing parameters so you should be prepared to handle that
							gs_id in_handle,
							void (*callback)(void * cookie, gs_id handle, void * buffer, int32 offset, int32 size, size_t buf_size),
							void * cookie);
		status_t CloneSound(
							gs_id in_handle,
							gs_id * out_handle);
		status_t ReleaseSound(
							gs_id in_handle);
		status_t GetSoundInfo(
							gs_id in_handle,
							gs_audio_format * out_format,
							void ** out_buffer,
							size_t * out_size);

		status_t StartSound(
							gs_id sound,
							bool ignoreRestart = false);
		bool IsPlaying(
							gs_id sound);
		status_t StopSound(
							gs_id sound);
		status_t ResumeSound(
							gs_id sound);
		status_t SetSoundGain(
							gs_id sound,
							float gain,
							float ramp = 0.0);
		status_t SetSoundPan(
							gs_id sound,
							float pan,
							float ramp = 0.0);
		status_t SetSoundSamplingRate(
							gs_id sound,
							float rate,
							float ramp = 0.0);
		status_t SetSoundLooping(
							gs_id sound,
							bool looping);
		status_t GetSoundGain(
							gs_id sound,
							float * gain,
							float * ramp = NULL);
		status_t GetSoundPan(
							gs_id sound,
							float * pan,
							float * ramp = NULL);
		status_t GetSoundSamplingRate(
							gs_id sound,
							float * rate,
							float * ramp = NULL);
		status_t GetSoundLooping(
							gs_id sound,
							bool * looping);
private:

friend class PrivDeleter;

		struct game_sound {
			char * data;
			int32 * ref_count;
			size_t size;
			size_t offset;
			bool optimized;
			bool stereo;
			bool playing;
			bool looping;
			gs_audio_format format;
			void (*callback)(void * cookie, gs_id handle, void * buffer, int32 offset, int32 size, size_t buf_size);
			void * cookie;

			int gain;		//	target, 0 to UNITY_GAIN*MAX_GAIN
			int pan;		//	target, -SPLIT_GAIN to SPLIT_GAIN
			int gain_ramp;
			int gain_l;		//	current value, 0 to UNITY_GAIN*SPLIT_GAIN
			int gain_r;		//	current value, 0 to UNITY_GAIN*SPLIT_GAIN
			int gain_l_delta;
			int gain_r_delta;
			int phase;		//	for resampling
			int last_left;
			int last_right;
			float sr_target;
			int sr_ramp;
			float sr_delta;
			int32 off_delta;
		};

static	int32 _m_didInit;
static	PrivGameSound * _m_player;
		bool _m_playingAny;
		bool _m_reserved_b[3];
		uint32 _m_reserved_u[1];

		struct game_sound * _m_sounds;
		BLocker _m_lock;
		gs_audio_format _m_format;

		PrivGameSound(
							const gs_audio_format & format);
virtual	~PrivGameSound();

virtual	void PlayBuffer(
							void * data,
							size_t size,
							const media_raw_audio_format & format);

static	void convert_sound(	//	used for optimize
							game_sound & sound,
							void * dest,
							size_t size,
							const gs_audio_format & outfmt);
static	void copy_sound(	//	used while playing
							game_sound & sound,
							void * dest,
							size_t size,
							const gs_audio_format & outfmt);
static	void mix_sound(		//	used while playing
							game_sound & sound,
							void * dest,
							size_t size,
							const gs_audio_format & outfmt);

};

void format_from_file(BSoundFile & inFile, gs_audio_format * out_format);
size_t frame_size_for(const gs_audio_format & format);

}
using namespace BPrivate;


#endif	//	PrivGameSound_h
