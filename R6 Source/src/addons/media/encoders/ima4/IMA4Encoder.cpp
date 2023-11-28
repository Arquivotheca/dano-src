#include <stdio.h>
#include <ByteOrder.h>
#include <Debug.h>
#include <MediaFormats.h>
#include <Encoder.h>
#include "IMA4Encoder.h"

#if DEBUG
#define PRINTF printf
#else
#define PRINTF if (0) printf
#endif

#define WAVE_IMA_ADPCM	0x0011

media_encoded_audio_format::audio_encoding ima4_encoding;
media_encoded_audio_format::audio_encoding wav_encoding;

void
register_encoder(void)
{
	PRINTF("IMA4Encoder loaded\n");
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[2];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
	mediaFormat.u.encoded_audio = media_encoded_audio_format::wildcard;
	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'ima4';
	formatDescription[1].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[1].u.quicktime.codec = 'ima4';
	formatDescription[1].u.quicktime.vendor = 0;
	err = formatObject.MakeFormatFor(formatDescription, 2, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		PRINTF("IMA4Encoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	ima4_encoding = mediaFormat.u.encoded_audio.encoding;

	mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
	mediaFormat.u.encoded_audio = media_encoded_audio_format::wildcard;
	memset(formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family = B_WAV_FORMAT_FAMILY;
	formatDescription[0].u.wav.codec = WAVE_IMA_ADPCM;
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat);
	if(err != B_NO_ERROR) {
		PRINTF("IMA4Encoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	wav_encoding = mediaFormat.u.encoded_audio.encoding;
}

Encoder *
instantiate_encoder(void)
{
	if(ima4_encoding == 0 || wav_encoding == 0)
		return NULL;
	return new IMA4Encoder();
}

IMA4Encoder::IMA4Encoder() :
	fInBuffer(0),
	fOutBuffer(0),
	fInLength(0),
	fConvertBuffer(0)
{
	PRINTF("IMA4Encoder::IMA4Encoder()\n");
}

IMA4Encoder::~IMA4Encoder() {
	if (fInBuffer)
		free(fInBuffer);
	if (fOutBuffer)
		free(fOutBuffer);
	if (fConvertBuffer)
		free(fConvertBuffer);
}

status_t 
IMA4Encoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "IMA Adaptive PCM");
	strcpy(mci->short_name, "ima4");
	return B_OK;
}

status_t
IMA4Encoder::InitFormats(media_file_format *mfi,
                         media_format *in_fmt, media_format *out_fmt) {

	PRINTF("IMA4Encoder::InitFormats()\n");
	status_t	err = B_NO_ERROR;

	/* Are we compatible with the required family ? */
	if(mfi) {
		if(mfi->family == B_QUICKTIME_FORMAT_FAMILY ||
			mfi->family == B_ANY_FORMAT_FAMILY) {
			fFamily = B_QUICKTIME_FORMAT_FAMILY;
		}
		else if(mfi->family == B_WAV_FORMAT_FAMILY) {
			fFamily = B_WAV_FORMAT_FAMILY;
		}
		else {
			PRINTF("We can't handle this family (%d)\n", mfi->family);
			return B_MEDIA_BAD_FORMAT;
		}
	}
	else {
		PRINTF("We can't handle no family\n");
		return B_BAD_TYPE;
	}

	/* Look at the input format, and check if we know how to handle it */
	if (in_fmt->type != B_MEDIA_RAW_AUDIO) {
		in_fmt->type = B_MEDIA_RAW_AUDIO;
		in_fmt->u.raw_audio = media_raw_audio_format::wildcard;
	}

	in_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_fmt->require_flags = 0;
	out_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_fmt->require_flags = 0;

	/* Fill in the internal format descriptor */
	if(in_fmt->u.raw_audio.channel_count <= 0) {
		in_fmt->u.raw_audio.channel_count = 1;
	}
	if(in_fmt->u.raw_audio.channel_count > MAX_CHANNEL) {
		in_fmt->u.raw_audio.channel_count = MAX_CHANNEL;
	}
	fChannelCount = in_fmt->u.raw_audio.channel_count;
	fFrameRate = in_fmt->u.raw_audio.frame_rate;
	switch (in_fmt->u.raw_audio.format) {
		default:
			in_fmt->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
			/* fall through */
		case media_raw_audio_format::B_AUDIO_FLOAT :
			fFormat = IMA4_LFLOAT;
			fBytesPerSample = 4;
			break;
	
		case media_raw_audio_format::B_AUDIO_UCHAR :
			fFormat = IMA4_LUINT8;
			fBytesPerSample = 1;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT :
			fFormat = IMA4_LINT16;
			fBytesPerSample = 2;
			break;
		case media_raw_audio_format::B_AUDIO_INT :
			fFormat = IMA4_LINT32;
			fBytesPerSample = 4;
			break;
	}
	PRINTF("fBytesPerSample: %d\n", fBytesPerSample);
	if (in_fmt->u.raw_audio.byte_order == B_MEDIA_BIG_ENDIAN)
		fFormat += IMA4_BUINT8-IMA4_LUINT8;
	
	out_fmt->type = B_MEDIA_ENCODED_AUDIO;
	out_fmt->u.encoded_audio = media_encoded_audio_format::wildcard;
	out_fmt->u.encoded_audio.output.frame_rate = (int32)fFrameRate;
	out_fmt->u.encoded_audio.output.channel_count = fChannelCount;
	out_fmt->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
	switch(fFamily)
	{
	case B_QUICKTIME_FORMAT_FAMILY:
		out_fmt->u.encoded_audio.encoding = ima4_encoding;
		out_fmt->u.encoded_audio.frame_size = IMA4_PACKET_SIZE * fChannelCount;
		fConvertBufferSize = IMA4_SAMPLE_COUNT;
		fFrameSamples = IMA4_SAMPLE_COUNT * fChannelCount;
		out_fmt->u.encoded_audio.output.buffer_size =
			fFrameSamples *
			(out_fmt->u.encoded_audio.output.format & 0x0f);
		break;

	case B_WAV_FORMAT_FAMILY:
		out_fmt->u.encoded_audio.encoding = wav_encoding;
		out_fmt->u.encoded_audio.frame_size = 512 * fChannelCount;
		fConvertBufferSize = 8;
		fFrameSamples =
			(out_fmt->u.encoded_audio.frame_size - (4*fChannelCount)) * 2 +	fChannelCount;
		out_fmt->u.encoded_audio.output.buffer_size =
			fFrameSamples *
			(out_fmt->u.encoded_audio.output.format & 0x0f);
		break;
	
	default:
		PRINTF("wound up with invalid family (%d)\n", fFamily);
	}
	out_fmt->u.encoded_audio.output.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;

	out_fmt->u.encoded_audio.bit_rate = out_fmt->u.encoded_audio.output.frame_rate * 4;
	return err;
}

status_t
IMA4Encoder::SetFormat(media_file_format *mfi, media_format *in_fmt,
                       media_format *out_fmt)
{
	status_t		err;
	
	// Check the formats and reset the internal formats
	err = InitFormats(mfi, in_fmt, out_fmt);
	if (err != B_OK)
		return err;
	
	// Initialise other internal states
	fInLengthMax = fFrameSamples * fBytesPerSample;
	fOutBufferSize = out_fmt->u.encoded_audio.frame_size;
	fInputFrameSize = fBytesPerSample*fChannelCount;

	return B_OK;
}

status_t 
IMA4Encoder::StartEncoder()
{
	int i;
	for (i=0; i<fChannelCount; i++) {
		fPrevValue[i] = 0;
		fPrevIndex[i] = 0;
	}

	if (fInBuffer)
		free(fInBuffer);
	fInBuffer = (char*)malloc(fInLengthMax);
	if (fInBuffer == NULL)
		return B_NO_MEMORY;
	fInLength = 0;

	if (fOutBuffer)
		free(fOutBuffer);
	fOutBuffer = (char*)malloc(fOutBufferSize);
	if (fOutBuffer == NULL)
		return B_NO_MEMORY;

	if (fConvertBuffer)
		free(fConvertBuffer);
	fConvertBuffer = (int16*)malloc(fConvertBufferSize * sizeof(int16));
	if (fConvertBuffer == NULL)
		return B_NO_MEMORY;

	return B_NO_ERROR;
}

void 
IMA4Encoder::AttachedToTrack()
{
	if(fFamily == B_WAV_FORMAT_FAMILY)
	{
		meta_data m;
		m.format_tag = B_HOST_TO_LENDIAN_INT16(WAVE_IMA_ADPCM);
		m.channel_count = B_HOST_TO_LENDIAN_INT16(fChannelCount);
		m.samples_per_sec = B_HOST_TO_LENDIAN_INT32(fFrameRate);
		m.avg_bytes_per_sec = B_HOST_TO_LENDIAN_INT32(
			(fFrameRate / 4) * fChannelCount);
		m.block_align = B_HOST_TO_LENDIAN_INT16(fOutBufferSize);
		m.bits_per_sample = B_HOST_TO_LENDIAN_INT16(4);
		m.sample_size = B_HOST_TO_LENDIAN_INT16(4);
		m.cb_size = B_HOST_TO_LENDIAN_INT16(fOutBufferSize);
		m.samples_per_block = B_HOST_TO_LENDIAN_INT16(
			fInLengthMax / fBytesPerSample);
		
		AddTrackInfo(0, (const char*)&m, sizeof(m));
	}
}


status_t 
IMA4Encoder::EncodeBuffer(const char *src, media_encode_info *info)
{
	int32 sampleCount = fFrameSamples;
	
	uint32		length = 0;
	memset(fOutBuffer, 0, fOutBufferSize);
	char* out_ptr = fOutBuffer;
	
	if(fFamily == B_QUICKTIME_FORMAT_FAMILY)
	{
		// bytes of data per channel
		int32 byteCount = sampleCount / (2 * fChannelCount);
		for (int32 i=0; i<fChannelCount; i++)
		{
			// write header
			int32 written = IMAQTHeader(
				out_ptr,
				&fPrevValue[i],
				&fPrevIndex[i]);
			out_ptr += written;
			length += written;

			// write frame content
			written = IMAEncode4(
				(char*)src+fBytesPerSample*i,
				out_ptr,
				fChannelCount,
				sampleCount / fChannelCount,
				fFormat,
				&fPrevValue[i],
				&fPrevIndex[i],
				fConvertBuffer);
			ASSERT(written == byteCount);
			out_ptr += written;
			length += written;
		}
	}
	else
	{
		ASSERT(fFamily == B_WAV_FORMAT_FAMILY);
		
		char* src_ptr = (char*)src;

		// write header
		int32 written = IMAWavHeader(
			src_ptr, out_ptr, fChannelCount, fFormat,
			fPrevValue, fPrevIndex);
		out_ptr += written;
		length += written;
		src_ptr += (fChannelCount * fBytesPerSample);
		sampleCount -= fChannelCount;		
		
		int32 byteCount = sampleCount / 2;

 		// write frame content interleaved in chunks of 8 samples
 		const int32 chunkSamples = 8;
		ASSERT(!((sampleCount/fChannelCount) % chunkSamples));
		
		while(sampleCount)
		{
			ASSERT(sampleCount > 0);
			for (int32 i=0; i<fChannelCount; i++)
			{
				written = IMAEncode4(
					src_ptr + (i * fBytesPerSample),
					out_ptr,
					fChannelCount,
					chunkSamples,
					fFormat,
					&fPrevValue[i],
					&fPrevIndex[i],
					fConvertBuffer);
				ASSERT(written == chunkSamples/2);
				length += written;
				out_ptr += written;
			}
			src_ptr += (fBytesPerSample * fChannelCount * 8);
			sampleCount -= (chunkSamples * fChannelCount);
		}
	}

	info->flags |= B_MEDIA_KEY_FRAME;
	return WriteChunk(fOutBuffer, length, info);
}

status_t
IMA4Encoder::Encode(const void *in_buffer, int64 num_frames,
                    media_encode_info *info)
{
	int32		total_length, copy_length;
	status_t	err;
	const char	*in_ptr = (const char *)in_buffer;

	fLastEncodeInfo = info;
	total_length = num_frames*fInputFrameSize;
	while (total_length > 0) {
		// Can we encode full output buffers directly from the input_stream ?
		while ((fInLength == 0) && (total_length >= fInLengthMax)) {
			err = EncodeBuffer(in_ptr, info);
			if (err != B_OK)
				return err;
			in_ptr += fInLengthMax;
			total_length -= fInLengthMax;
		}
		// We need to use the encoder input buffer.
		copy_length = fInLengthMax - fInLength;
		if (total_length < copy_length)
			copy_length = total_length;
		memcpy(fInBuffer+fInLength, in_ptr, copy_length);
		in_ptr += copy_length;
		fInLength += copy_length;
		total_length -= copy_length;
		// Is the encoder input buffer full ?
		if (fInLength == fInLengthMax) {
			err = EncodeBuffer(fInBuffer, info);
			fInLength = 0;
			if (err != B_OK)
				return err;
		}
	}
	return B_OK;
}

status_t IMA4Encoder::Flush() {
	int32		err;

	if (fInLength > 0) {
		if(fInLength < fInLengthMax)
		{
			// zero-pad the input buffer (partial ima4 packets aren't allowed)
			memset(fInBuffer + fInLength, 0, fInLengthMax - fInLength);
			fInLength = fInLengthMax;
		}
		err = EncodeBuffer(fInBuffer, fLastEncodeInfo);
		fInLength = 0;
		return err;
	}	
	return B_OK;
}

