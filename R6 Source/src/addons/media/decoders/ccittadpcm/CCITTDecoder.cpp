#include <MediaTrack.h>
#include <Extractor.h>
#include <MediaFormats.h>
#include <ByteOrder.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <OS.h>
#include <Debug.h>
#include "g72x.h"
#include "CCITTDecoder.h"

//#define DEBUG	printf
#define DEBUG	if (0) printf


media_encoded_audio_format::audio_encoding ccitt_encodings[6];
static media_format g_mediaFormat[6];
static media_format_description	formatDescription[6];
//------------------------------------------------------------------------------
#define WAVE_FORMAT_G721_4 0x7214
#define WAVE_FORMAT_G723_3 0x7233
#define WAVE_FORMAT_G723_5 0x7235
//------------------------------------------------------------------------------

status_t
get_next_description(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount)
{
	if(cookie == NULL || otype == NULL || odesc == NULL || ocount == NULL)
		return B_BAD_VALUE;

	switch(*cookie) {
	case 0:
		formatDescription[*cookie].family = B_MISC_FORMAT_FAMILY;
		formatDescription[*cookie].u.misc.file_format = '.snd';
		formatDescription[*cookie].u.misc.codec = '7214';
		break;
	case 1:
		formatDescription[*cookie].family = B_MISC_FORMAT_FAMILY;
		formatDescription[*cookie].u.misc.file_format = '.snd';
		formatDescription[*cookie].u.misc.codec = '7233';
		break;
	case 2:
		formatDescription[*cookie].family = B_MISC_FORMAT_FAMILY;
		formatDescription[*cookie].u.misc.file_format = '.snd';
		formatDescription[*cookie].u.misc.codec = '7235';
		break;
	case 3: 	// wav/721
		formatDescription[*cookie].family      = B_WAV_FORMAT_FAMILY;
		formatDescription[*cookie].u.wav.codec = WAVE_FORMAT_G721_4;
		break;
	case 4: 	// wav/723_3
		formatDescription[*cookie].family      = B_WAV_FORMAT_FAMILY;
		formatDescription[*cookie].u.wav.codec = WAVE_FORMAT_G723_3;
		break;
	case 5: 	// wav/723_5
		formatDescription[*cookie].family      = B_WAV_FORMAT_FAMILY;
		formatDescription[*cookie].u.wav.codec = WAVE_FORMAT_G723_5;
		break;
	default:
		return B_BAD_INDEX;
	}


	*otype = B_MEDIA_ENCODED_AUDIO;
	*odesc = &formatDescription[*cookie];
	*ocount = 1;

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
	for(i = 0; i < 6; i++) {
		err = get_next_description(&cookie, &type, &desc, &count);
		if(err != B_OK)
			break;
		
		g_mediaFormat[i].type = type;
		memset(&g_mediaFormat[i].u, 0, sizeof(g_mediaFormat[i].u));

		err = formatObject.MakeFormatFor(desc, count, &g_mediaFormat[i]);
		if(err != B_OK) {
			//printf("CCITTADPCMDecoder: MakeFormatFor failed, %s\n", strerror(err));
		}
		else {
			ccitt_encodings[i] = g_mediaFormat[i].u.encoded_audio.encoding;
		}
	}

	*out_format = g_mediaFormat;
	*out_count = 6;
}

//------------------------------------------------------------------------------
Decoder *instantiate_decoder()
{
	DEBUG("CCITT: instantiate_decoder\n");
	
	if (
		(ccitt_encodings[0] == 0) &&
		(ccitt_encodings[1] == 0) &&
		(ccitt_encodings[2] == 0) &&
		(ccitt_encodings[3] == 0) &&
		(ccitt_encodings[4] == 0) &&
		(ccitt_encodings[5] == 0)
	   )
	{
		return NULL;
	}
	else
	{
		return new CCITTDecoder();
	}
}
//------------------------------------------------------------------------------
CCITTDecoder::CCITTDecoder()
{
	DEBUG("CCITTDecoder::CCITTDecoder (%p)\n", this);  

	fBlocSize = 0;
	fSampleFormat = 0;
	fChannelCount = 0;
	fRate = 0.0;
	int64 dummy = 0;
	Reset(B_SEEK_BY_FRAME, 0, &dummy, 0, &dummy);
}
//------------------------------------------------------------------------------
CCITTDecoder::~CCITTDecoder()
{
	DEBUG("CCITTDecoder::~CCITTDecoder (%p)\n", this);  
}
//------------------------------------------------------------------------------
status_t
CCITTDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "CCITT Audio Compression");
	strcpy(mci->short_name, "CCITT-Audio");
	return B_OK;
}
//------------------------------------------------------------------------------
status_t
CCITTDecoder::Sniff(const media_format *in_format,
                    const void *in_info, size_t in_size) {
	int32						i, numCoef;
	status_t					err;
	media_format				mediaFormat[3];
	media_format_description	formatDescription;
	BMediaFormats				formatObject;

	if (in_format->type != B_MEDIA_ENCODED_AUDIO)
		return B_BAD_TYPE;
	if(in_format->u.encoded_audio.encoding == 0)
		return B_BAD_TYPE;

	for (i=0; i<6; i++)
		if (in_format->u.encoded_audio.encoding == ccitt_encodings[i]) {
			fRate = in_format->u.encoded_audio.output.frame_rate;
			fChannelCount = in_format->u.encoded_audio.output.channel_count;
			fBlocSize = in_format->u.encoded_audio.output.buffer_size;
			fSampleFormat = i;
			return B_OK;
		}

	return B_BAD_TYPE;
}
//------------------------------------------------------------------------------
status_t
CCITTDecoder::Format(media_format *inout_format)
{
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
//------------------------------------------------------------------------------
status_t CCITTDecoder::Decode(void *out_buffer, int64 *out_frameCount,
							  media_header *mh, media_decode_info *info)
{
	int32		needed_count;	// samples needed to fill the output buffer
	int16		*out_ptr = (int16*)out_buffer; 

	*out_frameCount = 0;
	needed_count = fBlocSize/2;   	// divided by 2: because we output shorts

	while (needed_count > 0) {
		if (count > 0) {
			int32		copy_count;		
			int32		i;
			int32 		code;			// 3, 4 or 5 bits code
			if (count > needed_count)
				copy_count = needed_count;
			else
				copy_count = count;
			switch (fSampleFormat%3)
			{
			case 0:		// 721_4 (0:misc; 3:wav)				
				for (i=0; i<copy_count ;i++) {
					if (in_bits < 4) {
						value |= ((uint8)(*in_buffer++))<<in_bits;
						in_bits += 8;
					}
					code = value & 0x0f;
					value >>= 4;
					in_bits -= 4;
					out_ptr[i] = g721_decoder(code, AUDIO_ENCODING_LINEAR, &fState);
				}
				break;
			case 1:		// 723_3 (1:misc; 4:wav)				
				for (i=0; i<copy_count ;i++) {
					if (in_bits < 3) {
						value |= ((uint8)(*in_buffer++))<<in_bits;
						in_bits += 8;
					}
					code = value & 0x07;
					value >>= 3;
					in_bits -= 3;
					out_ptr[i] = g723_24_decoder(code, AUDIO_ENCODING_LINEAR, &fState);
				}
				break;
			case 2:		// 723_5 (2:misc; 5:wav)				
				for (i=0; i<copy_count ;i++) {
					if (in_bits < 5) {
						value |= ((uint8)(*in_buffer++))<<in_bits;
						in_bits += 8;
					}
					code = value & 0x1f;
					value >>= 5;
					in_bits -= 5;
					out_ptr[i] = g723_40_decoder(code, AUDIO_ENCODING_LINEAR, &fState);
				}
				break;
			}
			fMh.start_time += (int64)((1000000.0*(float)copy_count)/((float)fChannelCount*fRate));
			*out_frameCount += copy_count/fChannelCount;
				//NOTE: it is assumed that copy_count % fChannelCount == 0
			count -= copy_count;
			needed_count -= copy_count;
			out_ptr += copy_count;
		}		
		if (needed_count > 0) {
			status_t	err;
			err = GetNextChunk((const void**)&in_buffer, &count, &fMh, info);
			if (err != B_OK) {
				if ((err == B_LAST_BUFFER_ERROR) && (*out_frameCount != 0))
					break;
				return err;
			}
			switch (fSampleFormat%3)
			{
			case 0:		// 721_4 (0:misc; 3:wav)				
				count = (count*8)/4;
				break;
			case 1:		// 723_3 (1:misc; 4:wav)				
				count = (count*8)/3;
				//NOTE: it is assumed that (count*8)%3 == 0
				break;
			case 2:		// 723_5 (2:misc; 5:wav)				
				count = (count*8)/5;
				//NOTE: it is assumed that (count*8)%5 == 0
				break;
			}
		}
	}
	*mh = fMh;
	mh->start_time -= (int64)((1000000.0*(float)(*out_frameCount))/fRate);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t CCITTDecoder::Reset(int32 in_towhat,
		  				   int64 in_requiredFrame, int64 *inout_frame,
						   bigtime_t in_requiredTime, bigtime_t *inout_time) {
	g72x_init_state(&fState);
	value = 0;
	in_bits = 0;
	count = 0;
	return B_OK;
}
//------------------------------------------------------------------------------
