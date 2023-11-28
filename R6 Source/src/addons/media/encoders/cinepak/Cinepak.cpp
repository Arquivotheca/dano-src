#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "Cinepak.h"

int   cpCompress(void *ptr,unsigned char *data,unsigned char *baseAddr,int32 *keyframe);
void *cpCompressInit(long real_width,long real_height,color_space cs);
void  cpSetQuality(float quality);
void  cpCompressCleanup(void *ptr);

media_encoded_video_format::video_encoding cinepak_encoding;

void register_encoder(void)
{
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[3];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'cvid';
	formatDescription[1].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[1].u.quicktime.codec = 'cvid';
	formatDescription[1].u.quicktime.vendor = 0;
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'cvid';
	err = formatObject.MakeFormatFor(formatDescription, 3, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		//printf("CinepakEncoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	cinepak_encoding = mediaFormat.u.encoded_video.encoding;
}

Encoder *instantiate_encoder(void)
{
	if(cinepak_encoding == 0)
		return NULL;
	return new CinepakEncoder();
}


CinepakEncoder::CinepakEncoder()
{
	fQuality         = .75;
	fOutputBuffer    = NULL;
	cinepakPtr		= NULL;
}

CinepakEncoder::~CinepakEncoder()
{
	if (fOutputBuffer)
		free(fOutputBuffer);
	fOutputBuffer = NULL;
	if (cinepakPtr)
		cpCompressCleanup(cinepakPtr);
	cinepakPtr=NULL;
}


status_t 
CinepakEncoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Cinepak Compression");
	strcpy(mci->short_name, "cinepak");
	return B_OK;
}

status_t
CinepakEncoder::GetEncodeParameters(encode_parameters *parameters) const
{
    parameters->quality = fQuality;
    return B_NO_ERROR;
}

status_t 
CinepakEncoder::SetEncodeParameters(encode_parameters *parameters)
{
	fQuality = parameters->quality;
	if(fQuality < 0) {
		fQuality = 0;
		parameters->quality = 0.0;
	}
	if(fQuality > 1.0) {
		fQuality = 1.0;
		parameters->quality = 1.0;
	}
	cpSetQuality(fQuality);
	return B_NO_ERROR;
}

status_t
CinepakEncoder::SetFormat(media_file_format *mfi,
						media_format *in_fmt, media_format *out_format)

{
	in_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_fmt->require_flags = 0;
	out_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_format->require_flags = 0;

	if(in_fmt->type != B_MEDIA_RAW_VIDEO) {
		in_fmt->type = B_MEDIA_RAW_VIDEO;
		in_fmt->u.raw_video = media_raw_video_format::wildcard;
	}
//printf("Set format, format=%d\n",in_fmt->u.raw_video.display.format);
	
	// default to RGB32...
	switch (in_fmt->u.raw_video.display.format)
	{
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
		case B_RGB32:
		case B_RGB16:
		case B_RGB15:
			break;
#else
		case B_RGB32_BIG:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
			break;
#endif
		default:
#if defined(__INTEL__) || defined(__ARMEL__)	/* FIXME: This should probably use <endian.h> for the right define */
			in_fmt->u.raw_video.display.format = B_RGB32;
#else
			in_fmt->u.raw_video.display.format = B_RGB32_BIG;
#endif
			break;
	}
//	printf("Set formatto %d\n",in_fmt->u.raw_video.display.format);

	if (mfi->family != B_QUICKTIME_FORMAT_FAMILY &&
	    mfi->family != B_AVI_FORMAT_FAMILY) {
		mfi->family = B_AVI_FORMAT_FAMILY;
	}

	out_format->type = B_MEDIA_ENCODED_VIDEO;
	out_format->u.encoded_video.output = in_fmt->u.raw_video;
	out_format->u.encoded_video.encoding = cinepak_encoding;

	fWidth  = in_fmt->u.raw_video.display.line_width;
	fHeight = in_fmt->u.raw_video.display.line_count;
	fColorspace = in_fmt->u.raw_video.display.format;
	
	return B_OK;
}

status_t 
CinepakEncoder::StartEncoder()
{
	cinepakPtr=cpCompressInit(fWidth,fHeight,fColorspace);
	
	fOutputSize   = fWidth * fHeight * 4;
	fOutputBuffer = (char *)malloc(fOutputSize);

	cpSetQuality(fQuality);

	if (fOutputBuffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
CinepakEncoder::Encode(const void *in_buffer, int64 num_frames,
                     media_encode_info *info)
{
	size_t  output_size = fOutputSize;
	int32	keyframe = 0;

	if (info->flags & B_MEDIA_KEY_FRAME)
		keyframe = 1;

	output_size=cpCompress(cinepakPtr,(unsigned char *)in_buffer,
				 (unsigned char *)fOutputBuffer,&keyframe);

	if (keyframe) info->flags |= B_MEDIA_KEY_FRAME;
	
	return WriteChunk(fOutputBuffer, output_size, info);
}
	
