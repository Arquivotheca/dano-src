#include <MediaTrack.h>
#include <Extractor.h>
#include <MediaFormats.h>
#include <ByteOrder.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <OS.h>
#include <Debug.h>

#include "ULAW.h"


enum {
	ULAW_ENCODING
};

media_encoded_audio_format::audio_encoding ulaw_encoding;
media_format mediaFormat;
static media_format_description	formatDescription;

status_t
get_next_description(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount)
{
	if(cookie == NULL || otype == NULL || odesc == NULL || ocount == NULL)
		return B_BAD_VALUE;

	switch(*cookie) {
	case 0:
		*otype = B_MEDIA_ENCODED_AUDIO;

		formatDescription.family = B_MISC_FORMAT_FAMILY;
		formatDescription.u.misc.file_format = '.snd';
		formatDescription.u.misc.codec = 'ulaw';

		*odesc = &formatDescription;
		*ocount = 1;
		break;
	default:
		return B_BAD_INDEX;
	}

	(*cookie)++;

	return B_OK;
}

void
register_decoder(const media_format ** out_format, int32 * out_count)
{
	const media_format_description *desc;
	BMediaFormats formatObject;
	media_type type;
	int32 count;
	int32 cookie;
	status_t err;
	int i;

	if(out_format == NULL || out_count == NULL)
		return;

	cookie = 0;
	err = get_next_description(&cookie, &type, &desc, &count);
	if(err != B_OK) {
		*out_format = NULL;
		*out_count = 0;
		return;
	}
	
	mediaFormat.type = type;
	memset(&mediaFormat.u, 0, sizeof(mediaFormat.u));

	err = formatObject.MakeFormatFor(desc, count, &mediaFormat);
	if(err != B_OK) {
		//printf("ULAWDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	else {
		ulaw_encoding = mediaFormat.u.encoded_audio.encoding;
	}

	*out_format = &mediaFormat;
	*out_count = 1;
}

Decoder *instantiate_decoder() {
	if(ulaw_encoding == 0)
		return NULL;
	return new ULAWDecoder();
}

ULAWDecoder::ULAWDecoder() {
	fChannelCount = 0;
	fBlocSize = 0;
	fRate = 0;
	in_buffer = NULL;
	count = 0;
	offset = 0;
}

ULAWDecoder::~ULAWDecoder() {
}

static int32 base_values[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };

status_t ULAWDecoder::InitTable(int32 method) {
    int32		i, byte, exponent, mantisse, value;

	if (method == ULAW_ENCODING) {
		for (i=0; i<256; i++) {
		    byte = ~i;
		    exponent = (byte >> 4) & 7;
		    mantisse = byte & 15;
		    value = base_values[exponent] + (mantisse << (exponent + 3));
		    if (byte & 0x80)
		    	fTable[i] = -value;
		    else
		    	fTable[i] = value;
		}
		return B_OK;
	}
	return B_BAD_TYPE;
}


status_t
ULAWDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "uLaw Audio Compression");
	strcpy(mci->short_name, "ulaw");
	return B_OK;
}



status_t
ULAWDecoder::Sniff(const media_format *in_format,
                   const void *in_info, size_t in_size)
{
	status_t	err;

	if (in_format->type != B_MEDIA_ENCODED_AUDIO)
		return B_BAD_TYPE;
	if (in_format->u.encoded_audio.encoding != ulaw_encoding)
		return B_BAD_TYPE;

	fChannelCount = in_format->u.encoded_audio.output.channel_count;
	fRate = in_format->u.encoded_audio.output.frame_rate;
	fBlocSize = in_format->u.encoded_audio.output.buffer_size;
	return InitTable(ULAW_ENCODING);
}

status_t ULAWDecoder::Format(media_format *inout_format) {
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	inout_format->type = B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = fRate;
	inout_format->u.raw_audio.channel_count = fChannelCount;
	inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	inout_format->u.raw_audio.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	inout_format->u.raw_audio.buffer_size = fBlocSize;
	return B_OK;
}

status_t ULAWDecoder::Decode(void *out_buffer, int64 *out_frameCount,
							 media_header *mh, media_decode_info *info)
{
	int32		i, copy_count, needed_count;
	status_t	err;
	int16		*out_ptr = (int16*)out_buffer;

	*out_frameCount = 0;
	needed_count = fBlocSize/2;
	
	while (needed_count > 0) {
		if (count > 0) {
			if (count > needed_count)
				copy_count = needed_count;
			else
				copy_count = count;
			for (i=0; i<copy_count; i++)
				out_ptr[i] = fTable[((uint8*)in_buffer)[i]];
			*out_frameCount += copy_count/fChannelCount;
			count -= copy_count;
			needed_count -= copy_count;
			fMh.start_time += (int64)((1000000.0*(float)copy_count)/((float)fChannelCount*fRate));
			in_buffer += copy_count;
			out_ptr += copy_count;
		}		
		if (needed_count > 0) {
			err = GetNextChunk((const void**)&in_buffer, &count, &fMh, info);
			if (err != B_OK) {
				if ((err == B_LAST_BUFFER_ERROR) && (*out_frameCount != 0))
					break;
				return err;
			}
			in_buffer += offset;
			fMh.start_time += (int64)((1000000.0*(float)offset)/((float)fChannelCount*fRate));
			if(count < offset) {
//				printf("ULAWDecoder::Decode: offset > Chunk\n");
				count = 0;
				offset -= count;
			}
			else {
				count -= offset;
				offset = 0;
			}
		}
	}
	*mh = fMh;
	mh->start_time -= (int64)((1000000.0*(float)(*out_frameCount))/fRate);
	return B_OK;
}

status_t ULAWDecoder::Reset(int32 in_towhat,
		  				   int64 in_requiredFrame, int64 *inout_frame,
						   bigtime_t in_requiredTime, bigtime_t *inout_time) {
	int64		delta_frame;
	
	if (in_towhat == B_SEEK_BY_TIME)
		delta_frame = ((in_requiredTime - *inout_time)*(int64)fRate)/1000000;
	else if (in_towhat == B_SEEK_BY_FRAME)
		delta_frame = in_requiredFrame - *inout_frame;
	else
		return B_BAD_VALUE;
	if(delta_frame < 0)
		delta_frame = 0;
	*inout_frame += delta_frame; 
	*inout_time += (delta_frame*1000000LL)/(int32)fRate; 
	offset = delta_frame*fChannelCount;
	in_buffer = NULL;
	count = 0;
	return B_OK;
}
