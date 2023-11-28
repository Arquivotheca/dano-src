#include <stdio.h>
#include "raw_encoder.h"

namespace B {
namespace Media2 {


extern "C" B::Private::Encoder *
instantiate_nth_encoder(int32 index)
{
	//printf("RawEncoder: instantiate_nth_encoder %d\n", index);
	if(index == 0)
		return new RawEncoder(false);
	else if(index == 1)
		return new RawEncoder(true);
	else
		return NULL;
}


RawEncoder::RawEncoder(bool audio)
	: audio(audio)
{
}

RawEncoder::~RawEncoder()
{
}

status_t 
RawEncoder::GetCodecInfo(media_codec_info *mci) const
{
	if(audio) {
		strcpy(mci->pretty_name, "Raw Audio");
		strcpy(mci->short_name, "raw-audio");
	}
	else {
		strcpy(mci->pretty_name, "Raw Video");
		strcpy(mci->short_name, "raw-video");
	}
	return B_NO_ERROR;
}

status_t
RawEncoder::SetFormat(media_file_format *,
					  media_format *in_fmt, media_format *out_fmt)
{
	in_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_fmt->require_flags = 0;
	out_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_fmt->require_flags = 0;

	if(audio) {
		if(in_fmt->type != B_MEDIA_RAW_AUDIO) {
			in_fmt->type = B_MEDIA_RAW_AUDIO;
			in_fmt->u.raw_audio = media_raw_audio_format::wildcard;
		}
		switch (in_fmt->u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_UCHAR :
				frame_size = 1;
				break;
			case media_raw_audio_format::B_AUDIO_SHORT :
				frame_size = 2;
				break;

			default:
				in_fmt->u.raw_audio.format = media_raw_audio_format::B_AUDIO_INT;
				/* fall through */
			case media_raw_audio_format::B_AUDIO_INT :
			case media_raw_audio_format::B_AUDIO_FLOAT :
				frame_size = 4;
				break;
		}
		frame_size *= in_fmt->u.raw_audio.channel_count;
	}
	else {
		if (in_fmt->type != B_MEDIA_RAW_VIDEO) {
			in_fmt->type = B_MEDIA_RAW_VIDEO;
			in_fmt->u.raw_video = media_raw_video_format::wildcard;
		}
		frame_size = in_fmt->u.raw_video.display.bytes_per_row *
			         in_fmt->u.raw_video.display.line_count;
	}
	*out_fmt = *in_fmt;

	return B_OK;
}

status_t
RawEncoder::Encode(const void *in_buffer, int64 num_frames,
                   media_encode_info *info)
{
	info->flags = B_MEDIA_KEY_FRAME;
	return WriteChunk(in_buffer, num_frames * frame_size, info);
}

} } // B::Media2
