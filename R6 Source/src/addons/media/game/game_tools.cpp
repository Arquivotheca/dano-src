
#include "game_tools.h"

#include <cstdlib>
#include <cstring>
#include <Debug.h>

#define D_TROUBLE(x)	PRINT(x)
#define D_METHOD(x)		PRINT(x)
#define D_FORMAT(x)		PRINT(x)

using namespace BPrivate;

// #pragma mark --- format conversion helpers ---
// ------------------------------------------------------------------------ //

float BPrivate::frame_rate_for(uint32 flag)
{
	for (int n = 0; n < sizeof(fixed_rates)/sizeof(*fixed_rates); n++)
	{
		if (fixed_rates[n].flag == flag) return fixed_rates[n].rate;
	}
	return 0.0f;
}


uint32 BPrivate::int_format_valid_bits(uint32 driver_format)
{
	switch (driver_format)
	{
		case B_FMT_24BIT:
			return 24;
		case B_FMT_20BIT:
			return 20;
		case B_FMT_18BIT:
			return 18;
		default:
			ASSERT(!"int_format_valid_bits(): not an int format");
			return 0;
	}
}

// return the number of bits set in the given value
uint32 BPrivate::count_1_bits(uint32 value)
{
	uint32 acc = 0;
	for (int n = 0; n < 32; n++, value >>= 1)
	{
		acc += (value & 1);
	}
	return acc;
}

// Validate all non-zero fields in the given format spec against the
// provided constraints.
status_t BPrivate::validate_game_format_spec(
	const game_format_spec& spec,
	uint32 allowed_frame_rates,
	float min_cvsr,
	float max_cvsr,
	uint32 allowed_channel_counts,
	uint32 allowed_designations,
	uint32 allowed_formats)
{
	// frame rate
	if (spec.frame_rate)
	{
		if (spec.frame_rate & B_SR_CVSR)
		{
			if (spec.cvsr < min_cvsr || spec.cvsr > max_cvsr)
			{
				D_FORMAT((
					"validate_game_format_spec():\n\t"
					"bad cvsr: %.2f [ %.2f, %.2f ]\n",
					spec.cvsr, min_cvsr, max_cvsr));
				return B_MEDIA_BAD_FORMAT;
			}
		}
		else
		if (!(allowed_frame_rates & spec.frame_rate))
		{
			D_FORMAT((
				"validate_game_format_spec():\n\t"
				"bad frame_rate: 0x%x [ 0x%x ]\n",
				spec.frame_rate, allowed_frame_rates));
			return B_MEDIA_BAD_FORMAT;
		}
	}
	// channel count
	if (spec.channel_count)
	{
		if (!(allowed_channel_counts & (1 << spec.channel_count-1)))
		{
			D_FORMAT((
				"validate_game_format_spec():\n\t"
				"bad channel_count: 0x%x [ 0x%x ]\n",
				spec.channel_count, allowed_channel_counts));
			return B_MEDIA_BAD_FORMAT;
		}
	}
	// designations
	// +++++
	// format
	if (spec.format)
	{
		if (!(allowed_formats & spec.format))
		{
			D_FORMAT((
				"validate_game_format_spec():\n\t"
				"bad format: 0x%x [ 0x%x ]\n",
				spec.format, allowed_formats));
			return B_MEDIA_BAD_FORMAT;
		}
	}
	return B_OK;
}

// Make and validate valid driver-style format settings; return
// error without modifying ioSpec if any relevant part of mediaFormat is
// unspecified or not allowed, or if any modified fields in ioSpec would
// not have satisfied the given constraints.
// Note that it doesn't care if unchanged fields in ioSpec are invalid -- only
// the fields which need to be modified to match mediaFormat are checked against
// the given constraints.

status_t BPrivate::make_game_format_spec(
	const media_multi_audio_format& mediaFormat,
	uint32 allowed_frame_rates,
	float min_cvsr,
	float max_cvsr,
	uint32 allowed_channel_counts,
	uint32 allowed_designations,
	uint32 allowed_formats,
	game_format_spec* ioSpec)
{
#if DEBUG
	char fmtbuf[256];
	media_format f;
	f.type = B_MEDIA_RAW_AUDIO;
	f.u.raw_audio = mediaFormat;
	string_for_format(f, fmtbuf, 255);
	D_FORMAT(("make_game_format_spec((%s), 0x%x, [%.2f, %.2f], 0x%x, 0x%x, 0x%x)\n",
		fmtbuf,
		allowed_frame_rates, min_cvsr, max_cvsr,
		allowed_channel_counts,
		allowed_designations,
		allowed_formats));
#endif
	status_t err;
	
	media_multi_audio_format& w = media_multi_audio_format::wildcard;

	if (mediaFormat.format == w.format ||
		mediaFormat.frame_rate == w.frame_rate ||
		mediaFormat.channel_count == w.channel_count)
	{
		D_TROUBLE((
			"make_game_format_spec():\n\t"
			"wildcard in format\n"));
		return B_MEDIA_BAD_FORMAT;
	}
	
	game_format_spec newSpec;
	memset(&newSpec, 0, sizeof(game_format_spec));

	// * build new format spec
	
	if (allowed_frame_rates & B_SR_CVSR ||
		ioSpec->frame_rate & B_SR_CVSR)
	{
		if (!(ioSpec->frame_rate & B_SR_CVSR) ||
			ioSpec->cvsr != mediaFormat.frame_rate)
		{
			newSpec.frame_rate = B_SR_CVSR;
			newSpec.cvsr = mediaFormat.frame_rate;
		}
	}
	else
	{
		uint16 frame_rate = 0;
		for (int n = 0; n < sizeof(fixed_rates)/sizeof(*fixed_rates); n++)
		{
			float distance = fabs(mediaFormat.frame_rate - fixed_rates[n].rate);
			if (distance < 2.0)
			{
				frame_rate = fixed_rates[n].flag;
				break;
			}
		}
		if (!frame_rate)
		{
			D_TROUBLE((
				"make_game_format_spec():\n\t"
				"no match for Media Kit frame rate %.2f\n",
				mediaFormat.frame_rate));
			return B_MEDIA_BAD_FORMAT;
		}
		if (ioSpec->frame_rate != frame_rate)
		{
			newSpec.frame_rate = frame_rate;
		}
	}

	if (ioSpec->channel_count != mediaFormat.channel_count)
	{
		newSpec.channel_count = mediaFormat.channel_count;
	}

	// +++++ designations
	
	uint16 format = 0;
	for (int n = 0; n < sizeof(format_map)/sizeof(*format_map); n++)
	{
		if (mediaFormat.format == format_map[n].media &&
			(mediaFormat.format != media_multi_audio_format::B_AUDIO_INT ||
			 mediaFormat.valid_bits == int_format_valid_bits(format_map[n].driver)))
		{
			format = format_map[n].driver;
			break;
		}
	}
	if (!format)
	{
		D_FORMAT((
			"make_game_format_spec():\n\t"
			"no match for Media Kit format type %ld\n",
			mediaFormat.format));
		return B_MEDIA_BAD_FORMAT;
	}
	if (ioSpec->format != format)
	{
		newSpec.format = format;
	}
	
	// validate
	err = validate_game_format_spec(
		newSpec,
		allowed_frame_rates, min_cvsr, max_cvsr,
		allowed_channel_counts,
		allowed_designations,
		allowed_formats);
	if (err < B_OK)
	{
		return err;
	}
	
	// merge changes
	if (newSpec.frame_rate || newSpec.cvsr)
	{
		ioSpec->frame_rate = newSpec.frame_rate;
		ioSpec->cvsr = newSpec.cvsr;
	}
	if (newSpec.channel_count)
	{
		ioSpec->channel_count = newSpec.channel_count;
	}
	if (newSpec.format)
	{
		ioSpec->format = newSpec.format;
	}

	D_FORMAT(("make_game_format_spec(): (0x%x / %.2f), 0x%x, 0x%x)\n",
		ioSpec->frame_rate,
		ioSpec->cvsr,
		ioSpec->channel_count,
		ioSpec->format));

	return B_OK;
}

status_t BPrivate::constrain_frame_rate(const game_codec_info& info, float* ioFrameRate)
{
	status_t err = B_OK;
	if (info.frame_rates & B_SR_CVSR)
	{
		// validate continuously variable frame rate
		if (*ioFrameRate < info.cvsr_min)
		{
			err = B_MEDIA_BAD_FORMAT;
			*ioFrameRate = info.cvsr_min;
		}
		else
		if (*ioFrameRate > info.cvsr_max)
		{
			err = B_MEDIA_BAD_FORMAT;
			*ioFrameRate = info.cvsr_max;
		}
		return err;
	}
	float nearest = 1e20;
	float nearestRate = 0.0;
	uint32 nearestFlag;
	for (int n = 0; n < sizeof(fixed_rates)/sizeof(*fixed_rates); n++)
	{
		if (info.frame_rates & fixed_rates[n].flag)
		{
			float distance = fabs(fixed_rates[n].rate - *ioFrameRate);
			if (distance < nearest)
			{
				nearest = distance;
				nearestRate = fixed_rates[n].rate;
				nearestFlag = fixed_rates[n].flag;
			}
		}
	}
	ASSERT(nearestRate != 0.0);
	err = (*ioFrameRate != nearestRate) ? B_MEDIA_BAD_FORMAT : B_OK;
	*ioFrameRate = nearestRate;
	return err;
}

status_t BPrivate::constrain_format(const game_codec_info& info, uint32 *ioFormat, int16* ioValidBits)
{
	status_t err = B_MEDIA_BAD_FORMAT;
	uint16 valid = 0;
	for (int n = 0; n < sizeof(format_map)/sizeof(*format_map); n++)
	{
		if (*ioFormat == format_map[n].media &&
			(info.formats & format_map[n].driver))
		{
			// scan all int formats for best possible match
			if (*ioFormat == media_multi_audio_format::B_AUDIO_INT)
			{
				uint32 cur_valid = int_format_valid_bits(format_map[n].driver);
				if (cur_valid == *ioValidBits)
				{
					// perfect match
					err = B_OK;
					valid = cur_valid;
					break;
				}
				else
				if (!valid)
				{
					// hang onto first imperfect match (this selects the
					// highest-quality format.)
					valid = cur_valid;
				}
			}
			// simple match
			else
			{
				err = B_OK;
				break;
			}
		}
	}
	if (*ioFormat == media_multi_audio_format::B_AUDIO_INT)
	{
		*ioValidBits = valid;
	}
	if (err == B_MEDIA_BAD_FORMAT)
	{
		// suggest current codec format
		*ioFormat = info.cur_format;
	}
	return err;
}

status_t BPrivate::constrain_channel_count(const game_codec_info& info, uint32 *ioChannelCount)
{
	if (info.channel_counts & (1 << *ioChannelCount-1))
		return B_OK;
	// find nearest allowable channel count:
	// work downward
	for (uint32 cc = *ioChannelCount - 1; cc >= 1; cc--)
	{
		if (info.channel_counts & (1 << cc-1))
		{
			*ioChannelCount = cc;
			return B_MEDIA_BAD_FORMAT;
		}
	}
	// work upward
	for (uint32 cc = *ioChannelCount + 1; cc <= 32; cc++)
	{
		if (info.channel_counts & (1 << cc-1))
		{
			*ioChannelCount = cc;
			return B_MEDIA_BAD_FORMAT;
		}
	}
	ASSERT(!"shouldn't get here");
	return B_ERROR;	
}

status_t BPrivate::constrain_buffer_size(uint32 format, uint32 channel_count, uint32 frame_count, size_t *ioBufferSize)
{
	ASSERT(channel_count > 0);
	status_t err = B_OK;
	const uint32 frame_size = (format & 0x0f) * channel_count;
	size_t req_size = frame_size * frame_count;
	if (*ioBufferSize != req_size)
	{
		if (*ioBufferSize != media_multi_audio_format::wildcard.buffer_size)
		{
			err = B_MEDIA_BAD_FORMAT;
		}
		*ioBufferSize = req_size;
	}
	return err;
}
