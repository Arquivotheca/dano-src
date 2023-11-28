#include <ByteOrder.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <OS.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <Message.h>
#include "Extractor.h"

#include "RAWDecoder.h"

media_encoded_audio_format::audio_encoding sgn8_encoding;
media_format mediaFormat[2];
static media_format_description	formatDescription[3];

status_t
get_next_description(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount)
{
	if(cookie == NULL || otype == NULL || odesc == NULL || ocount == NULL)
		return B_BAD_VALUE;

	switch(*cookie) {
	case 0:
		*otype = B_MEDIA_ENCODED_AUDIO;

		formatDescription[0].family = B_AIFF_FORMAT_FAMILY;
		formatDescription[0].u.aiff.codec = 'sgn8';
		formatDescription[1].family = B_AVR_FORMAT_FAMILY;
		formatDescription[1].u.avr.id = 'sgn8';

		*odesc = &formatDescription[0];
		*ocount = 2;
		break;
	case 1:
		*otype = B_MEDIA_RAW_AUDIO;

		formatDescription[2].family = B_BEOS_FORMAT_FAMILY;
		formatDescription[2].u.beos.format = 0x1;

		*odesc = &formatDescription[2];
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
	for(i = 0; i < 2; i++) {
		err = get_next_description(&cookie, &type, &desc, &count);
		if(err != B_OK)
			break;
		
		mediaFormat[i].type = type;
		memset(&mediaFormat[i].u, 0, sizeof(mediaFormat[i].u));

		err = formatObject.MakeFormatFor(desc, count, &mediaFormat[i]);
		if(err != B_OK) {
			//printf("RAWDecoder: MakeFormatFor failed, %s\n", strerror(err));
		}
	}
	sgn8_encoding = mediaFormat[0].u.encoded_audio.encoding;

	*out_format = mediaFormat;
	*out_count = 2;
}

Decoder *instantiate_decoder() {
//	if (sgn8_encoding == 0) return 0;
	return new RAWDecoder();
}

RAWDecoder::RAWDecoder() {
	fSampleFormat = 0;
	fChannelCount = 0;
	fBlocSize = 0;
	fSamplePerBloc = 0;
	fSampleSize = 0;
	fSwapByte = false;
	fRate = 0;
	in_buffer = NULL;
	offset = 0;
	size = 0;
}

RAWDecoder::~RAWDecoder() {
}


status_t
RAWDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Raw Audio");
	strcpy(mci->short_name, "raw-audio");
	return B_OK;
}



status_t
RAWDecoder::Sniff(const media_format *in_format,
                  const void *in_info, size_t in_size)
{
	status_t	err;

	if (in_format->type == B_MEDIA_RAW_AUDIO) {
		fSampleFormat = in_format->u.raw_audio.format;
		fChannelCount = in_format->u.raw_audio.channel_count;
		fBlocSize = in_format->u.raw_audio.buffer_size;
		fSwapByte = (in_format->u.raw_audio.byte_order !=
					 (B_HOST_IS_LENDIAN?B_MEDIA_LITTLE_ENDIAN:B_MEDIA_BIG_ENDIAN));
		fRate = in_format->u.raw_audio.frame_rate;
		switch (fSampleFormat) {
		case media_raw_audio_format::B_AUDIO_CHAR :
		case media_raw_audio_format::B_AUDIO_UCHAR :
			fSampleSize = 1;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT :
			fSampleSize = 2;
			break;
		case media_raw_audio_format::B_AUDIO_INT :
		case media_raw_audio_format::B_AUDIO_FLOAT :
			fSampleSize = 4;
			break;
		default:
			return B_BAD_TYPE;
		}
		fSampleSize *= fChannelCount;
		fSamplePerBloc = fBlocSize/fSampleSize;
		fChannelMask = in_format->u.raw_audio.channel_mask;
		fValidBits = in_format->u.raw_audio.valid_bits;
		fMatrixMask = in_format->u.raw_audio.matrix_mask;
		return B_OK;
	}
	else if (in_format->type == B_MEDIA_ENCODED_AUDIO) {
		if (sgn8_encoding != 0 && in_format->u.encoded_audio.encoding == sgn8_encoding) {
			fSampleFormat = 0x1;
			fChannelCount = in_format->u.encoded_audio.output.channel_count;
			fBlocSize = in_format->u.encoded_audio.output.buffer_size;
			fSwapByte = false;
			fRate = in_format->u.encoded_audio.output.frame_rate;
			fSampleSize = fChannelCount;
			fSamplePerBloc = fBlocSize/fSampleSize;
			return B_OK;
		}
	}
	return B_BAD_TYPE;
}

status_t RAWDecoder::Format(media_format *inout_format) {
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	inout_format->type = B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = fRate;
	inout_format->u.raw_audio.channel_count = fChannelCount;
	if (fSampleFormat == 0x1)
		inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_UCHAR;
	else
		inout_format->u.raw_audio.format = fSampleFormat;
	inout_format->u.raw_audio.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	inout_format->u.raw_audio.buffer_size = fBlocSize;
	inout_format->u.raw_audio.channel_mask = fChannelMask;
	inout_format->u.raw_audio.valid_bits = fValidBits;
	inout_format->u.raw_audio.matrix_mask = fMatrixMask;
	return B_OK;
}

int32 RAWDecoder::CopyBuffer(const char *from, char *to, int32 size) {
	int32		i, out_frameCount;
	uint8		*from8, *to8;
	uint16		*from16, *to16;
	uint32		*from32, *to32;
	float		*fromf, *tof;

	if (from == NULL || to == NULL || size == 0) {
		return 0;
	}

	/* do the copy and potential endianess conversion */
	if (fSwapByte)
		switch (fSampleFormat) {
		case media_raw_audio_format::B_AUDIO_UCHAR :
			out_frameCount = size/fChannelCount;
			goto just_memcpy;
		case media_raw_audio_format::B_AUDIO_CHAR :
			goto make_unsigned;
		case media_raw_audio_format::B_AUDIO_SHORT :
			from16 = (uint16*)from;
			to16 = (uint16*)to;
			for (i=(size>>1)-1; i>=0; i--)
				to16[i] = B_SWAP_INT16(from16[i]);
			out_frameCount = (size>>1)/fChannelCount;
			break;
		case media_raw_audio_format::B_AUDIO_INT :
			from32 = (uint32*)from;
			to32 = (uint32*)to;
			for (i=(size>>2)-1; i>=0; i--)
				to32[i] = B_SWAP_INT32(from32[i]);
			out_frameCount = (size>>2)/fChannelCount;
			break;
		case media_raw_audio_format::B_AUDIO_FLOAT :
			fromf = (float*)from;
			tof = (float*)to;
			for (i=(size>>2)-1; i>=0; i--)
				tof[i] = B_SWAP_FLOAT(fromf[i]);
			out_frameCount = (size>>2)/fChannelCount;
			break;
		}
	else
		switch (fSampleFormat) {
		case 0x1 :
make_unsigned:
			from32 = (uint32*)from;
			to32 = (uint32*)to;
			for (i=(size>>2)-1; i>=0; i--)
				to32[i] = from32[i]^0x80808080;
			if (size & 2)
				*(uint16*)(to+(size&0xfffffffc)) =
					*(uint16*)(from+(size&0xfffffffc)) ^ 0x8080;
			if (size & 1)
				to[size-1] = from[size-1] ^ 0x80;
			out_frameCount = size/fChannelCount;
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR :
			out_frameCount = size/fChannelCount;
			goto just_memcpy;
		case media_raw_audio_format::B_AUDIO_SHORT :
			out_frameCount = (size>>1)/fChannelCount;
			goto just_memcpy;
		case media_raw_audio_format::B_AUDIO_INT :
		case media_raw_audio_format::B_AUDIO_FLOAT :
			out_frameCount = (size>>2)/fChannelCount;
just_memcpy:
			memcpy(to, from, size);
			break;
		}
	return out_frameCount;
}

status_t RAWDecoder::Decode(void *out_buffer, int64 *out_frameCount,
							media_header *mh, media_decode_info *info)
{
	int32		copy_size, needed_size;
	status_t	err;
	char		*outptr = (char*)out_buffer;

	*out_frameCount = 0;
	needed_size = fBlocSize;
	
	while (needed_size > 0) {
		if (size > 0) {
			if (size > needed_size)
				copy_size = needed_size;
			else
				copy_size = size;
if ((uint32)in_buffer < 0x4000 || (uint32)in_buffer > (uint32)0xfffff000) {
	char buff[256];
	sprintf(buff, "in_buffer 0x%x copy_size %d, needed_size %d size %d offset %Ld\n",
			in_buffer, copy_size, needed_size, size, offset);
	debugger(buff);
}
			*out_frameCount += CopyBuffer(in_buffer, outptr, copy_size);
			size -= copy_size;
			needed_size -= copy_size;
			fMh.start_time += (int64)((1000000.0*(float)copy_size)/((float)fSampleSize*fRate));
			in_buffer += copy_size;
			outptr += copy_size;
		}		
		if (needed_size > 0) {
			err = GetNextChunk((const void**)&in_buffer, &size, &fMh, info);
			if (err != B_OK) {
				if ((err == B_LAST_BUFFER_ERROR) && (*out_frameCount != 0))
					break;
				return err;
			}
			in_buffer += offset;
if ((uint32)in_buffer < 0x4000 || (uint32)in_buffer > (uint32)0xfffff000) {
	char buff[256];
	sprintf(buff, "in_buffer 0x%x offset %Ld\n", in_buffer, offset);
	debugger(buff);
}
			fMh.start_time += (int64)((1000000.0*(float)offset)/((float)fSampleSize*fRate));
			if(size < offset) {
				printf("RAWDecoder::Decode: offset > Chunk\n");
				size = 0;
				offset -= size;
			}
			else {
				size -= offset;
				offset = 0;
			}
		}
	}
	
	*mh = fMh;
	mh->start_time -= (int64)((1000000.0*(float)(*out_frameCount))/fRate);
	return B_OK;
}

status_t RAWDecoder::Reset(int32 in_towhat,
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
	offset = delta_frame*(off_t)fSampleSize;

	in_buffer = NULL;
	size = 0;
	return B_OK;
}
