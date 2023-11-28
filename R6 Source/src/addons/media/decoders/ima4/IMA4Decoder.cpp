#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <OS.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <MediaTrack.h>
#include <Extractor.h>
#include <MediaFormats.h>
#include <Decoder.h>

#include "IMA4Decoder.h"
#include "ima4.h"

#if DEBUG
#define FPRINTF(x)	fprintf x
#else
#define FPRINTF(x)	if (0) fprintf x
#endif

#define IS_SWAPPED 		true

#define QT_PSize2SCount(x)	(((x)-2)<<1)
#define QT_SCount2PSize(x)	(((x)>>1)+2)

#define WAVE_IMA_ADPCM	0x0011

media_encoded_audio_format::audio_encoding ima4_encoding;
media_encoded_audio_format::audio_encoding wav_encoding;
media_format mediaFormats[2];
static media_format_description	formatDescription[3];

status_t
get_next_description(int32 *cookie, media_type *otype, const media_format_description **odesc, int32 *ocount)
{
	if(cookie == NULL || otype == NULL || odesc == NULL || ocount == NULL)
		return B_BAD_VALUE;

	switch(*cookie) {
	case 0:
		*otype = B_MEDIA_ENCODED_AUDIO;

		formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
		formatDescription[0].u.beos.format = 'ima4';
		formatDescription[1].family = B_QUICKTIME_FORMAT_FAMILY;
		formatDescription[1].u.quicktime.codec = 'ima4';
		formatDescription[1].u.quicktime.vendor = 0;

		*odesc = formatDescription;
		*ocount = 2;
		break;

	case 1:
		*otype = B_MEDIA_ENCODED_AUDIO;

		formatDescription[2].family = B_WAV_FORMAT_FAMILY;
		formatDescription[2].u.wav.codec = WAVE_IMA_ADPCM;
		
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
	status_t err;
	int32 count;
	int32 cookie;

	if(out_format == NULL || out_count == NULL)
		return;

	*out_format = NULL;
	*out_count = 0;

	cookie = 0;
	for(int n = 0; n < 2; n++)
	{
		err = get_next_description(&cookie, &type, &desc, &count);
		if(err != B_OK)
			break;

		mediaFormats[n].type = type;
		memset(&mediaFormats[n].u, 0, sizeof(mediaFormats[n].u));
		
		err = formatObject.MakeFormatFor(desc, count, &mediaFormats[n]);
		if(err != B_NO_ERROR) {
			FPRINTF((stderr, "IMA4Decoder: MakeFormatFor failed, %s\n", strerror(err)));
		}
	}
	
	ima4_encoding = mediaFormats[0].u.encoded_audio.encoding;
	wav_encoding = mediaFormats[1].u.encoded_audio.encoding;
	*out_format = mediaFormats;
	*out_count = 2;
}

Decoder *instantiate_decoder() {
	FPRINTF((stderr, "IMA4: instantiated\n"));
	if(ima4_encoding == 0 || wav_encoding == 0)
		return NULL;
	return new IMA4Decoder();
}

IMA4Decoder::IMA4Decoder() {
	fBlocSize = 0;
	fChannelCount = 0;
	fRate = 0.0;
	fOutputCache = NULL;
	fPacketSize = 0;
	fDecodedPacketCount = 0;
	fPacketHeaderSize = 0;
	fChannelsPerPacket = 0;
	fPacketChunkSamples = 0;
	count = 0;
	cached_count = 0;
	offset = 0;
	cached_buffer = NULL;
	fOutputCache = NULL;
	in_buffer = NULL;
}

IMA4Decoder::~IMA4Decoder() {
	if (fOutputCache != NULL)
		free(fOutputCache);
}

status_t
IMA4Decoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "IMA-4 Audio Compression");
	strcpy(mci->short_name, "ima4");
	return B_OK;
}

status_t
IMA4Decoder::Sniff(const media_format *in_format,
                   const void *in_info, size_t in_size) {
	FPRINTF((stderr,"IMA4Decoder::Sniff()\n"));
	status_t					err;
	media_format				mediaFormat;
	media_format_description	formatDescription;
	BMediaFormats				formatObject;

	if (in_format->type != B_MEDIA_ENCODED_AUDIO)
		return B_BAD_TYPE;
	if (in_format->u.encoded_audio.encoding != ima4_encoding &&
		in_format->u.encoded_audio.encoding != wav_encoding)
		return B_BAD_TYPE;

	fEncoding = in_format->u.encoded_audio.encoding;
	fRate = in_format->u.encoded_audio.output.frame_rate;
	fChannelCount = in_format->u.encoded_audio.output.channel_count;
	if (sizeof(out_type) == 2)
		fBlocSize = in_format->u.encoded_audio.output.buffer_size;
	else
		fBlocSize = in_format->u.encoded_audio.output.buffer_size*2;
	
	if(fEncoding == ima4_encoding)
	{
		// quicktime encoding has fixed packet size
		fPacketSize = 34;
		fChannelsPerPacket = 1;
		fPacketHeaderSize = 2;
		fDecodedPacketCount = QT_PSize2SCount(fPacketSize);
		fOutputCache = (out_type*)malloc(fDecodedPacketCount*sizeof(out_type)*fChannelCount);
	}
	else
	{
		// wav encoding
		fPacketSize = in_format->u.encoded_audio.frame_size;
		fChannelsPerPacket = fChannelCount;
		fPacketChunkSamples = 8;
		fPacketHeaderSize = 4 * fChannelCount;
		fDecodedPacketCount = in_format->u.encoded_audio.output.buffer_size / sizeof(out_type);
		fOutputCache = (out_type*)malloc(in_format->u.encoded_audio.output.buffer_size);
	}
	return B_OK;
}

status_t IMA4Decoder::Format(media_format *inout_format) {
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	inout_format->type = B_MEDIA_RAW_AUDIO;
	inout_format->u.raw_audio.frame_rate = fRate;
	inout_format->u.raw_audio.channel_count = fChannelCount;
	if (sizeof(out_type) == 2)
		inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;
	else
		inout_format->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	inout_format->u.raw_audio.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	inout_format->u.raw_audio.buffer_size = fBlocSize;
	return B_OK;
}

status_t IMA4Decoder::Decode(void *out_buffer, int64 *out_frameCount,
							 media_header *mh, media_decode_info *info)
{
	int32				needed_count = fBlocSize/sizeof(out_type);
	status_t			err;
	uint8				*out_ptr = (uint8*)out_buffer;

	const uint32 encBlockSize = (fChannelsPerPacket > 1) ? fPacketSize : fPacketSize*fChannelCount;
	const uint32 decBlockSize = (fChannelsPerPacket > 1) ? fDecodedPacketCount : fDecodedPacketCount*fChannelCount;
	const uint32 decBlockFrames = (fChannelsPerPacket > 1) ? fDecodedPacketCount/fChannelCount : fDecodedPacketCount;

	*out_frameCount = 0;

	while (needed_count > 0) {
		// use cached samples if any
		if (cached_count > 0) {
			// Part of the cached buffer is really useful
			if (cached_count > offset) {
				cached_buffer += offset;
				cached_count -= offset;
				offset = 0;
			
				uint32 copy_count;
				if (cached_count > needed_count)
					copy_count = needed_count;
				else
					copy_count = cached_count;

				memcpy(out_ptr, cached_buffer, copy_count*sizeof(out_type));
					   
				cached_count -= copy_count;
				cached_buffer += copy_count;
				needed_count -= copy_count;
				*out_frameCount += copy_count/fChannelCount;
				out_ptr += sizeof(out_type)*copy_count;
			}
			// The whole cached buffer is inside the offset
			else {
				offset -= cached_count;
				cached_count = 0;
			}
		}
		

		// Process the offset (for full packets on all channels)
		while ((count >= encBlockSize) &&
			   (offset >= decBlockSize))
		{
			offset -= decBlockSize;
			count -= encBlockSize;
			fMh.start_time += (int64)((1000000.0*(float)decBlockFrames)/fRate);
			in_buffer += encBlockSize;
		}

		// Directly decode full packets into the output buffer (for all channels)
		if (offset == 0)
		{
			while ((count >= encBlockSize) &&
				   (needed_count >= decBlockSize))
			{
				uint32 samplesWritten;
				err = DecodePacketBlock(out_ptr, &samplesWritten);
				if(err < B_OK)
				{
					FPRINTF((stderr, "IMA4Decoder::Decode(): invalid data; aborting.\n"));
					*mh = fMh;
					mh->start_time -= (int64)((1000000.0*(float)(*out_frameCount))/fRate);
					return err;
				} 
				out_ptr += (samplesWritten*sizeof(out_type));
								
				uint32 framesWritten = samplesWritten / fChannelCount;
				fMh.start_time += (int64)((1000000.0*(float)framesWritten)/fRate);
				needed_count -= samplesWritten;
				*out_frameCount += framesWritten;
			}
		}

		// If there is encoded data left, and we still need decoded data, but
		// we were not able to process a full packet, then it's time to generate
		// a cached buffer
		if ((count > 0) && (needed_count > 0))
		{
			uint32 samplesWritten;
			err = DecodePacketBlock((uint8*)fOutputCache, &samplesWritten);
			if(err < B_OK)
			{
				FPRINTF((stderr, "IMA4Decoder::Decode(): invalid data; aborting.\n"));
				*mh = fMh;
				mh->start_time -= (int64)((1000000.0*(float)(*out_frameCount))/fRate);
				return err;
			} 
			
			uint32 framesWritten = samplesWritten / fChannelCount;
			cached_count += samplesWritten;
			fMh.start_time += (int64)((1000000.0*(float)framesWritten)/fRate);
			cached_buffer = fOutputCache;
		}

		// if there is no encode datas left, but we still need more decoded
		// data, it's time to get another chunk.
		if ((count == 0) && (needed_count > 0) &&
			(cached_count == 0))
		{
			err = GetNextChunk((const void**)&in_buffer, &count, &fMh, info);
			if (err != B_OK) {
				if ((err == B_LAST_BUFFER_ERROR) && (*out_frameCount != 0))
					break;
				*mh = fMh;
				return err;
			}
		}
	}

	*mh = fMh;
	mh->start_time -= (int64)((1000000.0*(float)(*out_frameCount))/fRate);

	return B_OK;
}

status_t IMA4Decoder::Reset(int32 in_towhat,
		  				   int64 in_requiredFrame, int64 *inout_frame,
						   bigtime_t in_requiredTime, bigtime_t *inout_time) {
	int64		delta_frame;
	if (in_towhat == B_SEEK_BY_TIME)
		delta_frame = ((in_requiredTime - *inout_time)*(int64)fRate)/1000000LL;
	else if (in_towhat == B_SEEK_BY_FRAME)
		delta_frame = in_requiredFrame - *inout_frame;
	else
		return B_BAD_VALUE;
	if(delta_frame < 0)
		delta_frame = 0;
	*inout_frame += delta_frame; 
	*inout_time += (delta_frame*1000000LL)/(int32)fRate; 
	offset = delta_frame*fChannelCount;
	count = 0;
	cached_count = 0;
	in_buffer = NULL;
	return B_OK;
}

// decode packet(s) (one for each channel if QT, one for WAV)
// (will decode a partial block if not enough data is available)
status_t 
IMA4Decoder::DecodePacketBlock(uint8 *decoded, uint32 *outWritten)
{
	status_t err;
	if(fEncoding == ima4_encoding)
	{
		uint32 decodeBytes = (count < fPacketSize) ? count : fPacketSize;
		uint32 decodeSamples = QT_PSize2SCount(decodeBytes);

		for (uint32 j=0; j<fChannelCount; j++)
		{
			ima4_adpcm_state state;
			err = ReadPacketHeader(&state);
			if(err < B_OK)
				return err;
		
			IMADecode4(
				(const int8*)in_buffer,
				(char*)decoded+sizeof(out_type)*j,
				fChannelCount,
				decodeSamples,
				&state,
				IS_SWAPPED);
			
			in_buffer += (decodeSamples / 2);
			count -= decodeBytes;
		}
		*outWritten = decodeSamples * fChannelCount;
	}
	else
	{
		ASSERT(fEncoding == wav_encoding);
		ASSERT(fChannelCount <= 2);
		ima4_adpcm_state state[2];
		err = ReadPacketHeader(state);
		if(err < B_OK)
			return err;
		count -= fPacketHeaderSize;

		*outWritten = 0;
		
		// previous values as encoded in header are the first samples
		for (uint32 j=0; j<fChannelCount; j++)
			((out_type*)decoded)[j] = state[j].valprev;
		*outWritten += fChannelCount;
		decoded += (fChannelCount * sizeof(out_type));

		const uint32 packetChunkBytes = fPacketChunkSamples / 2;
		const uint32 contentSize = fPacketSize - fPacketHeaderSize;
		uint32 decodeBytes = (count < contentSize) ? count : contentSize;
		uint32 remaining;
		if(count < contentSize)
		{
			uint32 decodeChunks = count / packetChunkBytes;
			remaining = decodeChunks * packetChunkBytes;
		}
		else
		{
			remaining = contentSize;
		}

		while(remaining)
		{
			for (uint32 j=0; j<fChannelCount; j++)
			{
				IMADecode4(
					(const int8*)in_buffer,
					(char*)decoded+sizeof(out_type)*j,
					fChannelCount,
					fPacketChunkSamples,
					&state[j],
					IS_SWAPPED);
				in_buffer += packetChunkBytes;
				count -= packetChunkBytes;
			}
			*outWritten += (fChannelCount*fPacketChunkSamples);
			decoded += (fChannelCount*fPacketChunkSamples*sizeof(out_type));
			remaining -= (fChannelCount*packetChunkBytes);
		}
	}
	return B_OK;
}

// read a QT or WAV header; for WAV, expects state to be an array of
// size fChannelCount
status_t 
IMA4Decoder::ReadPacketHeader(ima4_adpcm_state *state)
{
	if(!state)
		return B_BAD_VALUE;
	if(fEncoding == ima4_encoding)
	{
		int32 value = B_BENDIAN_TO_HOST_INT16(((int16*)in_buffer)[0]);
		state->index = value & 0x7f;
		state->valprev = value & 0xFF80;
		in_buffer += 2;

		if(state->index > 88)
		{
			FPRINTF((stderr,"!!! IMA4Decoder::ReadPacketHeader() : bad ima4 index %d\n", state->index));
			return B_BAD_VALUE;
		}
		return B_OK;
	}
	else if(fEncoding == wav_encoding)
	{
		for(uint32 n = 0; n < fChannelCount; n++, state++, in_buffer += 4)
		{
			state->valprev = B_LENDIAN_TO_HOST_INT16(((int16*)in_buffer)[0]);
			state->index = in_buffer[2];
			if(state->index > 88)
			{
				FPRINTF((stderr,"!!! IMA4Decoder::ReadPacketHeader() : bad ima4 index %d\n", state->index));
				return B_BAD_VALUE;
			}

			if(in_buffer[3] != 0)
				return B_BAD_VALUE;
		}
		return B_OK;
	}
	else
		return B_BAD_VALUE;
}

