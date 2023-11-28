#include <stdio.h>
#include "MSADPCM.h"

//#define DEBUG printf
#define DEBUG if (0) printf

#define WAVE_MS_ADPCM 	0x0002

media_encoded_audio_format::audio_encoding wawencoding;
media_encoded_audio_format::audio_encoding aviencoding;

void register_encoder(void)
{
	DEBUG("MSADPCM encoder loaded\n");
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[1];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
	mediaFormat.u.encoded_audio = media_encoded_audio_format::wildcard;
	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_AVI_FORMAT_FAMILY;
	formatDescription[0].u.avi.codec = 0x65610000 + WAVE_MS_ADPCM;
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		DEBUG("msadpcm encoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	aviencoding = mediaFormat.u.encoded_audio.encoding;

	mediaFormat.type = B_MEDIA_ENCODED_AUDIO;
	mediaFormat.u.encoded_audio = media_encoded_audio_format::wildcard;
	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_WAV_FORMAT_FAMILY;
	formatDescription[0].u.wav.codec = WAVE_MS_ADPCM;
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		DEBUG("msadpcm encoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	wawencoding = mediaFormat.u.encoded_audio.encoding;
}

BPrivate::Encoder *instantiate_encoder(void) {
	return new MSADPCMEncoder();
}

MSADPCMEncoder::MSADPCMEncoder() {
	fHeader.in_buffer = NULL;
	fHeader.out_buffer = NULL;
	
	fLastEncodeInfo = NULL;
	fHeader.in_length = 0;
}

MSADPCMEncoder::~MSADPCMEncoder() {
	if (fHeader.in_buffer)
		free(fHeader.in_buffer);
	if (fHeader.out_buffer)
		free(fHeader.out_buffer);
}

status_t 
MSADPCMEncoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Microsoft Adaptive PCM");
	strcpy(mci->short_name, "ms-adpcm");
	return B_OK;
}

static int16 standard_coeff[COEFF_COUNT][2] =
{
	{256, 0},
	{512, -256},
	{0, 0},
	{192, 64},
	{240, 0},
	{460, -208},
	{392, -232}
};

status_t
MSADPCMEncoder::InitFormats(media_file_format *mfi, media_format *in_fmt,
                            media_format *out_fmt)
{
	int32						i, frame_count;

	/* Are we compatible with the required family ? */
	if (mfi->family == B_AVI_FORMAT_FAMILY) {
		fHeader.family = B_AVI_FORMAT_FAMILY;
	}
	else if (mfi->family == B_WAV_FORMAT_FAMILY) {
		fHeader.family = B_WAV_FORMAT_FAMILY;
	}
	else {
		mfi->family == B_WAV_FORMAT_FAMILY;
		fHeader.family = B_WAV_FORMAT_FAMILY;
DEBUG("We can't handle this family (%d)\n", mfi->family);
	}

	if (in_fmt->type != B_MEDIA_RAW_AUDIO) {
		in_fmt->type = B_MEDIA_RAW_AUDIO;
		in_fmt->u.raw_audio = media_raw_audio_format::wildcard;
	}

	in_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_fmt->require_flags = 0;
	out_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_fmt->require_flags = 0;
		
	/* Fill in the internal format descriptor for input */
	if(in_fmt->u.raw_audio.channel_count <= 0)
		in_fmt->u.raw_audio.channel_count = 1;
	else if(in_fmt->u.raw_audio.channel_count > 4)
		in_fmt->u.raw_audio.channel_count = 4;
	fHeader.in_fmt.wf.nChannels = in_fmt->u.raw_audio.channel_count;
	fHeader.in_fmt.wf.nSamplesPerSec = (int32)in_fmt->u.raw_audio.frame_rate;

	switch (in_fmt->u.raw_audio.format) {
	case media_raw_audio_format::B_AUDIO_UCHAR :
		fHeader.in_fmt.wBitsPerSample = 8;
//		frame_count = in_fmt->u.raw_audio.buffer_size/fHeader.in_fmt.wf.nChannels;
		break;
	case media_raw_audio_format::B_AUDIO_SHORT :
		fHeader.in_fmt.wBitsPerSample = 16;
//		frame_count = in_fmt->u.raw_audio.buffer_size/(2*fHeader.in_fmt.wf.nChannels);
		break;
	default:
		in_fmt->u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
		/* fall through */
	case media_raw_audio_format::B_AUDIO_FLOAT :
		fHeader.in_fmt.wBitsPerSample = 32;
//		frame_count = in_fmt->u.raw_audio.buffer_size/(4*fHeader.in_fmt.wf.nChannels);
		break;
	}
	frame_count = 0x3f4;
	
	/* Fill in the internal format descriptor for output */
	fHeader.out_fmt.wfx.nChannels = in_fmt->u.raw_audio.channel_count;
	fHeader.out_fmt.wfx.nSamplesPerSec = (int32)in_fmt->u.raw_audio.frame_rate;
	fHeader.out_fmt.wfx.wBitsPerSample = 4;
	fHeader.out_fmt.wNumCoef = COEFF_COUNT;
	fHeader.out_fmt.wSamplesPerBlock = frame_count;
	for (i=0; i<COEFF_COUNT; i++) {
		fHeader.out_fmt.aCoef[i].iCoef1 = standard_coeff[i][0];
		fHeader.out_fmt.aCoef[i].iCoef2 = standard_coeff[i][1];
	}

	/* Fill in the encoded output format */
	out_fmt->type = B_MEDIA_ENCODED_AUDIO;
	out_fmt->u.encoded_audio = media_encoded_audio_format::wildcard;
	if (fHeader.family == B_AVI_FORMAT_FAMILY) {
		out_fmt->u.encoded_audio.encoding = aviencoding;
	}
	else {
		out_fmt->u.encoded_audio.encoding = wawencoding;
	}
	if(out_fmt->u.encoded_audio.encoding == 0)
		return B_ERROR;

	out_fmt->type = B_MEDIA_ENCODED_AUDIO;
	out_fmt->u.encoded_audio.output.frame_rate = (int32)in_fmt->u.raw_audio.frame_rate;
	out_fmt->u.encoded_audio.output.channel_count = in_fmt->u.raw_audio.channel_count;
	out_fmt->u.encoded_audio.output.format = media_raw_audio_format::B_AUDIO_SHORT;
	out_fmt->u.encoded_audio.output.byte_order =
		B_HOST_IS_LENDIAN ? B_MEDIA_LITTLE_ENDIAN : B_MEDIA_BIG_ENDIAN;
	out_fmt->u.encoded_audio.output.buffer_size =
		(frame_count*in_fmt->u.raw_audio.channel_count*fHeader.in_fmt.wBitsPerSample)/8;
	out_fmt->u.encoded_audio.frame_size =
		((14+frame_count-2)*in_fmt->u.raw_audio.channel_count+1)/2;
	out_fmt->u.encoded_audio.bit_rate = (out_fmt->u.encoded_audio.frame_size*8*
										 out_fmt->u.encoded_audio.output.frame_rate)/frame_count;
DEBUG("packet %d -> %d\n",
	  out_fmt->u.encoded_audio.output.buffer_size,
	  out_fmt->u.encoded_audio.frame_size);
	
	return B_OK;
}

status_t MSADPCMEncoder::SetFormat(media_file_format *mfi,
								   media_format *in_fmt, media_format *out_fmt) {
	status_t		err;
	
	/* Check the formats and reset the internal formats */
	err = InitFormats(mfi, in_fmt, out_fmt);
	if (err != B_OK)
		return err;
	
	fHeader.in_length = 0;
	fHeader.in_length_max = out_fmt->u.encoded_audio.output.buffer_size;
	fHeader.out_buffer_size = out_fmt->u.encoded_audio.frame_size;
	return B_OK;
}

status_t 
MSADPCMEncoder::StartEncoder()
{
	/* Initialise other internal states */
	free(fHeader.in_buffer);
	fHeader.in_buffer = (char*)malloc(fHeader.in_length_max);
	if (fHeader.in_buffer == NULL)
		return B_NO_MEMORY;
	free(fHeader.out_buffer);
	fHeader.out_buffer = (char*)malloc(fHeader.out_buffer_size);
	if (fHeader.out_buffer == NULL)
		return B_NO_MEMORY;
	
	fHeader.frame_size = (fHeader.in_fmt.wBitsPerSample*fHeader.in_fmt.wf.nChannels)/8;
	return B_OK;
}

void MSADPCMEncoder::AttachedToTrack() 
{
	int				i, key;

	fMetaData.Format = B_HOST_TO_LENDIAN_INT16(WAVE_FORMAT_ADPCM);
	fMetaData.Channels = B_HOST_TO_LENDIAN_INT16(fHeader.in_fmt.wf.nChannels);
	fMetaData.SamplesPerSec = B_HOST_TO_LENDIAN_INT32(fHeader.in_fmt.wf.nSamplesPerSec);
	fMetaData.AvgBytesPerSec =
		B_HOST_TO_LENDIAN_INT32((fHeader.in_fmt.wf.nSamplesPerSec*fHeader.in_fmt.wf.nChannels*
								 (14+fHeader.out_fmt.wSamplesPerBlock-2))/
								(fHeader.out_fmt.wSamplesPerBlock*2));
	key = fMetaData.SamplesPerSec*fMetaData.Channels;
	if (key <= 11100)
		fMetaData.BlockAlign = B_HOST_TO_LENDIAN_INT16(256);
	else if (key <= 22200)
		fMetaData.BlockAlign = B_HOST_TO_LENDIAN_INT16(512);
	else
		fMetaData.BlockAlign = B_HOST_TO_LENDIAN_INT16(1024);
	fMetaData.BitsPerSample = B_HOST_TO_LENDIAN_INT16(4);
	fMetaData.ExtensionSize = B_HOST_TO_LENDIAN_INT16((2+COEFF_COUNT*2)*sizeof(int16));
	fMetaData.SamplesPerBlock = B_HOST_TO_LENDIAN_INT16(fHeader.out_fmt.wSamplesPerBlock);
	fMetaData.NumCoefficients = B_HOST_TO_LENDIAN_INT16(COEFF_COUNT);

	for (i=0; i<COEFF_COUNT; i++)
	{
		fMetaData.Coefficients[i].Coef1 = B_HOST_TO_LENDIAN_INT16(standard_coeff[i][0]);
		fMetaData.Coefficients[i].Coef2 = B_HOST_TO_LENDIAN_INT16(standard_coeff[i][1]);
	}

	fMetaData.Style = 0;
	fMetaData.ByteCount = 0;

	if (fHeader.family == B_AVI_FORMAT_FAMILY)
		AddTrackInfo('strf', (const char*)&fMetaData, sizeof(fMetaData));
	else if (fHeader.family == B_WAV_FORMAT_FAMILY)
		AddTrackInfo('strf', (const char*)&fMetaData, sizeof(fMetaData)-sizeof(fMetaData.Style)-sizeof(fMetaData.ByteCount));
}

status_t MSADPCMEncoder::EncodeBuffer(const char *src, int32 src_length) {
	uint32		length;

	length = adpcmEncode4Bit(&fHeader.in_fmt, (char*)src,
							 &fHeader.out_fmt, fHeader.out_buffer, src_length);
DEBUG("Encoding %d bytes -> %d bytes\n", src_length, length);
	fLastEncodeInfo->flags |= B_MEDIA_KEY_FRAME;

	return WriteChunk(fHeader.out_buffer, length, fLastEncodeInfo);
}

status_t
MSADPCMEncoder::Encode(const void *in_buffer, int64 num_frames,
                       media_encode_info *info)
{
	int32		total_length, copy_length;
	status_t	err;
	const char  *in_ptr = (const char *)in_buffer;

	fLastEncodeInfo = info;
	
	total_length = num_frames*fHeader.frame_size;
	while (total_length > 0) {
		// Can we encode full output buffers directly from the input_stream ?
		while ((fHeader.in_length == 0) && (total_length >= fHeader.in_length_max)) {
			err = EncodeBuffer(in_ptr, fHeader.in_length_max);
			if (err != B_OK)
				return err;
			in_ptr += fHeader.in_length_max;
			total_length -= fHeader.in_length_max;
		}
		// We need to use the encoder input buffer.
		copy_length = fHeader.in_length_max - fHeader.in_length;
		if (total_length < copy_length)
			copy_length = total_length;
		memcpy(fHeader.in_buffer+fHeader.in_length, in_ptr, copy_length);
		in_ptr += copy_length;
		fHeader.in_length += copy_length;
		total_length -= copy_length;
		// Is the encoder input buffer full ?
		if (fHeader.in_length == fHeader.in_length_max) {
			err = EncodeBuffer(fHeader.in_buffer, fHeader.in_length);
			fHeader.in_length = 0;
			if (err != B_OK)
				return err;
		}		
	}
	return B_OK;
}

status_t MSADPCMEncoder::Flush() {
	int32		err;

	if (fLastEncodeInfo && fHeader.in_length > 0) {
		err = EncodeBuffer(fHeader.in_buffer, fHeader.in_length);
		fHeader.in_length = 0;
		return err;
	}	
	return B_OK;
}



