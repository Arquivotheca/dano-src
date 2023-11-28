#if !defined(__GAME_TOOLS_H__)
#define __GAME_TOOLS_H__

#include <Debug.h>
#include <MediaDefs.h>
#include "game_audio.h"

namespace BPrivate {

// ------------------------------------------------------------------------ //
// game_audio helpers

template<class C> class G : public C {
public:
	G() { memset(this, 0, sizeof(*this)); info_size = sizeof(*this); }
};

template<class C> class H : public C {
public:
	H() { memset(this, 0, sizeof(*this)); }
};

// ------------------------------------------------------------------------ //
// game-audio format conversion & negotiation helpers

// the frame rate to suggest by default
const float PREFERRED_FRAME_RATE = 44100.0;

struct game_format_spec
{
	uint32	frame_rate;
	float	cvsr;
	int16	channel_count;
	uint32	designations;
	uint32	format;
};

static const struct
{
	uint32		flag;
	float		rate;
}
fixed_rates[] =
{
	{ B_SR_8000, 8000. },
	{ B_SR_11025, 11025. },
	{ B_SR_12000, 12000. },
	{ B_SR_16000, 16000. },
	{ B_SR_22050, 22050. },
	{ B_SR_24000, 24000. },
	{ B_SR_32000, 32000. },
	{ B_SR_44100, 44100. },
	{ B_SR_48000, 48000. },
	{ B_SR_64000, 64000. },
	{ B_SR_88200, 88200. },
	{ B_SR_96000, 96000. },
	{ B_SR_176400, 176400. },
	{ B_SR_192000, 192000. },
	{ B_SR_384000, 384000. },
	{ B_SR_1536000, 1536000. },
};

static const struct
{
	uint32	media;
	uint32	driver;
}
format_map[] =
{
	{ media_multi_audio_format::B_AUDIO_CHAR,	B_FMT_8BIT_S },
	{ media_multi_audio_format::B_AUDIO_UCHAR,	B_FMT_8BIT_U },
	{ media_multi_audio_format::B_AUDIO_SHORT,	B_FMT_16BIT },
	{ media_multi_audio_format::B_AUDIO_INT,	B_FMT_32BIT },
	{ media_multi_audio_format::B_AUDIO_INT,	B_FMT_24BIT },
	{ media_multi_audio_format::B_AUDIO_INT,	B_FMT_20BIT },
	{ media_multi_audio_format::B_AUDIO_INT,	B_FMT_18BIT },
	{ media_multi_audio_format::B_AUDIO_FLOAT,	B_FMT_FLOAT }
};

// ------------------------------------------------------------------------ //

// convert driver framerate spec into a floating point framerate; return
// 0.0 if frame_rate_flag is invalid.
float frame_rate_for(
	uint32 frame_rate_flag);

// given a driver format constant describing 32-bit sample format
// (ie. 18+ bits) return the number of valid bits. 
uint32 int_format_valid_bits(
	uint32 driver_format);

// count the number of bits set in the given value
uint32 count_1_bits(
	uint32 value);

// Validate all non-zero fields in the given format spec against the
// provided constraints.
status_t validate_game_format_spec(
	const game_format_spec& spec,
	uint32 allowed_frame_rates,
	float min_cvsr,
	float max_cvsr,
	uint32 allowed_channel_counts,
	uint32 allowed_designations,
	uint32 allowed_formats);

// Build and validate valid driver-style format settings; return
// error without modifying ioSpec if any relevant part of mediaFormat is
// unspecified or not allowed, or if any modified fields in ioSpec would
// not have satisfied the given constraints.
// Note that it doesn't matter if unchanged fields in ioSpec are invalid -- only
// fields that need to be modified in order to be equivalent to mediaFormat are
// checked against the given constraints.
status_t make_game_format_spec(
	const media_multi_audio_format& mediaFormat,
	uint32 allowed_frame_rates,
	float min_cvsr,
	float max_cvsr,
	uint32 allowed_channel_counts,
	uint32 allowed_designations,
	uint32 allowed_formats,
	game_format_spec* ioSpec);
	
inline bigtime_t buffer_duration(const media_multi_audio_format &format)
{
	return bigtime_t(
		(1e6 * double(format.buffer_size)) /
		(double((format.format & 0x0f) * format.channel_count) * format.frame_rate));
}

// general format constraint tools

status_t constrain_frame_rate(const game_codec_info& info, float* ioFrameRate);
status_t constrain_format(const game_codec_info& info, uint32 *ioFormat, int16* ioValidBits);
status_t constrain_channel_count(const game_codec_info& info, uint32 *ioChannelCount);
status_t constrain_buffer_size(uint32 format, uint32 channel_count, uint32 frame_count, size_t *ioBufferSize);

}; // BPrivate
#endif //__GAME_TOOLS_H__
