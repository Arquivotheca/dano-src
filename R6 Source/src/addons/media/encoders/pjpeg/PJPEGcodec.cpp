#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "PJPEGcodec.h"


media_encoded_video_format::video_encoding pjpeg_encoding;

void register_encoder(void)
{
	//printf("PJPEGEncoder loaded\n");
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[4];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'jpeg';
	formatDescription[1].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[1].u.quicktime.codec = 'jpeg';
	formatDescription[1].u.quicktime.vendor = 0;
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'jpeg';
	formatDescription[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription[3].u.avi.codec = 'JPEG';
	err = formatObject.MakeFormatFor(formatDescription, 4, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		//printf("PJPEGEncoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	pjpeg_encoding = mediaFormat.u.encoded_video.encoding;
}

Encoder *instantiate_encoder(void)
{
	if(pjpeg_encoding == 0)
		return NULL;
	return new PJPEGEncoder();
}


PJPEGEncoder::PJPEGEncoder()
{
	fQuality         = 75;
	fOutputBuffer    = NULL;
	fStream.linePtrs = NULL;
}

PJPEGEncoder::~PJPEGEncoder()
{
	if (fOutputBuffer)
		free(fOutputBuffer);
	fOutputBuffer = NULL;

	if (fStream.linePtrs)
		free(fStream.linePtrs);
	fStream.linePtrs = NULL;
}


status_t 
PJPEGEncoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Photo-JPEG Compression");
	strcpy(mci->short_name, "pjpeg");
	return B_OK;
}

status_t
PJPEGEncoder::SetFormat(media_file_format *mfi,
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

	in_fmt->u.raw_video.display.format = B_RGB32;

	if (mfi->family != B_QUICKTIME_FORMAT_FAMILY &&
	    mfi->family != B_AVI_FORMAT_FAMILY) {
		mfi->family = B_AVI_FORMAT_FAMILY;
	}

	out_format->type = B_MEDIA_ENCODED_VIDEO;
	out_format->u.encoded_video.output = in_fmt->u.raw_video;
	out_format->u.encoded_video.encoding = pjpeg_encoding;

	fWidth  = in_fmt->u.raw_video.display.line_width;
	fHeight = in_fmt->u.raw_video.display.line_count;

	return B_OK;
}

status_t 
PJPEGEncoder::StartEncoder()
{
	InitJPEG(fWidth, fHeight, &fStream);

	fOutputSize   = fWidth * fHeight * 4;
	fOutputBuffer = (char *)malloc(fOutputSize);

	if (fOutputBuffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
PJPEGEncoder::Encode(const void *in_buffer, int64 num_frames,
                     media_encode_info *info)
{
	size_t  output_size = fOutputSize;
	
	EncodeJPEG((char *)in_buffer,
			   fOutputBuffer,
			   &output_size,
			   fWidth,
			   fHeight,
			   fQuality,
			   &fStream);						

	info->flags |= B_MEDIA_KEY_FRAME;
	return WriteChunk(fOutputBuffer, output_size, info);
}
	
status_t 
PJPEGEncoder::GetEncodeParameters(encode_parameters *parameters) const
{
	parameters->quality = fQuality / 100.0;
	return B_NO_ERROR;
}

status_t 
PJPEGEncoder::SetEncodeParameters(encode_parameters *parameters)
{
	fQuality = (int)(parameters->quality * 100);
	if(fQuality < 0) {
		fQuality = 0;
		parameters->quality = 0.0;
	}
	if(fQuality > 100) {
		fQuality = 100;
		parameters->quality = 1.0;
	}
	return B_NO_ERROR;
}
