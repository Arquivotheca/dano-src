#include <MediaTrack.h>
#include <Extractor.h>
#include <MediaFormats.h>
#include <ByteOrder.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <OS.h>
#include <Debug.h>

#include "MSADPCMDecoder.h"
#include "msadpcm_decode.h"
#include <RIFFTypes.h>

//#define DEBUG	printf
#define DEBUG	if (0) printf

#define WAVE_MS_ADPCM 	0x0002

media_encoded_audio_format::audio_encoding wavencoding;
media_encoded_audio_format::audio_encoding aviencoding;
static media_format mediaFormat[2];
static media_format_description	formatDescription[2];

status_t
get_next_description(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount)
{
	if(cookie == NULL || otype == NULL || odesc == NULL || ocount == NULL)
		return B_BAD_VALUE;

	switch(*cookie) {
	case 0:
		*otype = B_MEDIA_ENCODED_AUDIO;

		formatDescription[0].family = B_AVI_FORMAT_FAMILY;
		formatDescription[0].u.avi.codec = 0x65610000 + WAVE_MS_ADPCM;

		*odesc = &formatDescription[0];
		*ocount = 1;
		break;
	case 1:
		*otype = B_MEDIA_ENCODED_AUDIO;

		formatDescription[1].family = B_WAV_FORMAT_FAMILY;
		formatDescription[1].u.wav.codec = WAVE_MS_ADPCM;

		*odesc = &formatDescription[1];
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
			//printf("MSADPCMDecoder: MakeFormatFor failed, %s\n", strerror(err));
		}
	}
	aviencoding = mediaFormat[0].u.encoded_audio.encoding;
	wavencoding = mediaFormat[1].u.encoded_audio.encoding;

	*out_format = mediaFormat;
	*out_count = 2;
}

Decoder *instantiate_decoder() {
	if ((aviencoding == 0) && (wavencoding == 0)) return 0;
	return new MSADPCMDecoder();
}

MSADPCMDecoder::MSADPCMDecoder() {
	fBlocSize = 0;
	fOutputCacheSize = 0;
	fHeader = NULL;
	fOutputCache = NULL;
	count = 0;
	offset = 0;
	in_buffer = NULL;
}

MSADPCMDecoder::~MSADPCMDecoder() {
	if (fHeader != NULL)
		free(fHeader);
	if (fOutputCache != NULL)
		free(fOutputCache);
}

status_t
MSADPCMDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "MS-ADPCM Audio Compression");
	strcpy(mci->short_name, "msadpcm");
	return B_OK;
}


struct meta_data {
	uint32				fNumCoef;
	int16				fListCoef[2*MSADPCM_MAX_COEF];
};

status_t MSADPCMDecoder::Sniff(const media_format *in_format,
                               const void *in_info, size_t in_size) {
	int32		i, numCoef;
	int16		*CoefList = NULL;
	status_t	err;
	int32		SamplesPerBlock;

	if (in_format->type != B_MEDIA_ENCODED_AUDIO)
		return B_BAD_TYPE;
		
	if(in_info == NULL)
		return B_BAD_TYPE;
	if(in_format->u.encoded_audio.encoding == 0) {
		return B_BAD_TYPE;
	}
	else if (in_format->u.encoded_audio.encoding == wavencoding) {
		// Set input format.
		if(in_size < sizeof(meta_data)) {
			DEBUG("Bad waw info\n");
			return B_BAD_VALUE;
		}
		numCoef = ((meta_data*)in_info)->fNumCoef;
		CoefList = ((meta_data*)in_info)->fListCoef;
		SamplesPerBlock = in_format->u.encoded_audio.output.buffer_size/
			(2*in_format->u.encoded_audio.output.channel_count);
	}
	else if(in_format->u.encoded_audio.encoding == aviencoding) {
		AVIAUDSHeader *aviaudheader = (AVIAUDSHeader *)in_info;
		
		if(in_size != sizeof(AVIAUDSHeader)) {
			DEBUG("Bad avi info\n");
			return B_BAD_VALUE;
		}
		if(aviaudheader->Format != 2) {
			DEBUG("AVI header format does not match\n");
			return B_BAD_TYPE;
		}

		numCoef = aviaudheader->NumCoefficients;
		CoefList = (int16*)aviaudheader->Coefficients;
		SamplesPerBlock = aviaudheader->SamplesPerBlock;
	}
	else {
		return B_BAD_TYPE;
	}
	if(fHeader)
		free(fHeader);
	fHeader = (ADPCMWaveFormat*)malloc(sizeof(ADPCMWaveFormat)+sizeof(ADPCMCoefSet)*(numCoef-1));
	if (fHeader == NULL)
		return B_NO_MEMORY;
	fHeader->wfx.nSamplesPerSec = (uint32)in_format->u.encoded_audio.output.frame_rate;
	fHeader->wfx.nChannels = in_format->u.encoded_audio.output.channel_count;
	
	if(fHeader->wfx.nChannels <= 0) {
		return B_BAD_VALUE;
	}
	
	fHeader->wSamplesPerBlock = SamplesPerBlock;
	DEBUG("wSamplesPerBlock: %d\n", fHeader->wSamplesPerBlock);
	fHeader->wNumCoef = numCoef;
	DEBUG("NumCoeff: %d\n", numCoef);
	for (i=0; i<numCoef; i++) {
		fHeader->aCoef[i].iCoef1 = CoefList[2*i];
		fHeader->aCoef[i].iCoef2 = CoefList[2*i+1];
	DEBUG("Coeff[%d]: %d, %d\n", i, fHeader->aCoef[i].iCoef1, fHeader->aCoef[i].iCoef2);
	}

	fBlocSize = in_format->u.encoded_audio.output.buffer_size;
	fOutputCacheSize = fBlocSize;
	DEBUG("fBlocSize: %d\n", fBlocSize);
	fOutputCache = (char*)malloc(fBlocSize);
	if (fOutputCache == NULL)
		return B_NO_MEMORY;
	// Set output format
	fHeaderOut.wf.nChannels = fHeader->wfx.nChannels;
	fHeaderOut.wf.nSamplesPerSec = fHeader->wfx.nSamplesPerSec;
	fHeaderOut.wBitsPerSample = 16;
	return B_OK;
}

status_t MSADPCMDecoder::Format(media_format *inout_format) {
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	inout_format->type = B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = (float)fHeader->wfx.nSamplesPerSec;
	inout_format->u.raw_audio.channel_count = fHeader->wfx.nChannels;
	inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	inout_format->u.raw_audio.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	inout_format->u.raw_audio.buffer_size = fBlocSize;
	return B_OK;
}

status_t MSADPCMDecoder::Decode(void *_out_buffer, int64 *out_frameCount,
								media_header *mh, media_decode_info *info)
{
	int32			i, copy_count, needed_count, decoded_count;
	status_t		err;
	char			*out_buffer = (char*)_out_buffer;

	*out_frameCount = 0;
	needed_count = fBlocSize/2;
	
	while (needed_count > 0) {
		if (count > 0) {
			if (count > needed_count)
				copy_count = needed_count;
			else
				copy_count = count;
			memcpy(out_buffer, in_buffer, copy_count*2);
			*out_frameCount += copy_count/fHeader->wfx.nChannels;
			count -= copy_count;
			needed_count -= copy_count;
			in_buffer += copy_count*2;
			out_buffer += copy_count*2;
		}		
		if (needed_count > 0) {
			err = GetNextChunk((const void**)&in_buffer, &count, &fMh, info);
			if (err != B_OK) {
				if ((err == B_LAST_BUFFER_ERROR) && (*out_frameCount != 0))
					break;
				return err;
			}
DEBUG("GetNextChunk: returned %d\n", count);
			decoded_count = ((count*2)/fHeader->wfx.nChannels)-14+2;
DEBUG("Need: %d (/%d)\n", needed_count, decoded_count);
			// The decoder works in blocks.  Even though we may be actually
			// requesting less than a full block, it will fill in the rest.
			// round up the decoded size for our check so we don't overrun the
			// buffer
			int32 decodeMultiple = fHeader->wfx.nChannels * fHeader->wSamplesPerBlock;
			int32 outputBufferNeeded = ((decoded_count + decodeMultiple - 1) / decodeMultiple)
				* decodeMultiple;

			if ((needed_count >= outputBufferNeeded) &&
				(offset == 0)) {
DEBUG("Direct copy: %d -> ", count);
				count = adpcmDecode4Bit(fHeader, in_buffer, &fHeaderOut, out_buffer, count)/2;
DEBUG("%d\n", count);
				out_buffer += count*2;
				*out_frameCount += count/fHeader->wfx.nChannels;
				needed_count -= count;
				count = 0;
			}
			else {
DEBUG("Indirect copy: %d -> ", count);
				if(count*4 > fOutputCacheSize) {
					char *newOutputCache = (char*)realloc(fOutputCache, count*4);
					if(newOutputCache == NULL)
						return B_NO_MEMORY;
					fOutputCacheSize = count*4;
					fOutputCache = newOutputCache;
				}
				count = adpcmDecode4Bit(fHeader, in_buffer, &fHeaderOut, fOutputCache, count)/2;
DEBUG("%d\n", count);
				fMh.start_time += (int64)((1000000.0*(float)offset)/
					(float)(fHeader->wfx.nChannels*fHeader->wfx.nSamplesPerSec));
				in_buffer = fOutputCache + offset*2;
				if (count < offset) {
DEBUG("MSADPCMDecoder::Decode: offset (%d) > Chunk\n", offset);
					offset -= count;
					count = 0;
				}
				else {
					count -= offset;
					offset = 0;
				}
			}
		}
	}
DEBUG("return %d frames\n", *out_frameCount);
	*mh = fMh;
	mh->start_time -= (int64)((1000000.0*(float)(*out_frameCount))/
							  (float)fHeader->wfx.nSamplesPerSec);
	return B_OK;
}

status_t MSADPCMDecoder::Reset(int32 in_towhat,
		  				   int64 in_requiredFrame, int64 *inout_frame,
						   bigtime_t in_requiredTime, bigtime_t *inout_time) {
	int64		delta_frame;
	
DEBUG("MSADPCMDecoder::Reset %d %Ld %Ld\n", in_towhat, in_requiredFrame, in_requiredTime);

	if (in_towhat == B_SEEK_BY_TIME)
		delta_frame = ((in_requiredTime - *inout_time)*(int64)fHeader->wfx.nSamplesPerSec)/1000000;
	else if (in_towhat == B_SEEK_BY_FRAME)
		delta_frame = in_requiredFrame - *inout_frame;
	else
		return B_BAD_VALUE;
	if(delta_frame < 0)
		delta_frame = 0;
	*inout_frame += delta_frame; 
	*inout_time += (delta_frame*1000000LL)/fHeader->wfx.nSamplesPerSec; 
	offset = delta_frame*fHeader->wfx.nChannels;
	in_buffer = NULL;
	count = 0;
	return B_OK;
}
