#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "dvEncoder.h"

media_encoded_video_format::video_encoding dv_encoding;

void register_encoder(void)
{
	//printf("DVEncoder loaded\n");
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[5];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));

	formatDescription[0].family = B_AVI_FORMAT_FAMILY;
	formatDescription[0].u.avi.codec = 'CDVC';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'cdvc';
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'dvsd';
	formatDescription[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription[3].u.avi.codec = 'pcdv';
	formatDescription[4].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[4].u.quicktime.vendor = 0;
	formatDescription[4].u.quicktime.codec  = 'dvc ';

	err = formatObject.MakeFormatFor(formatDescription, 5, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		//printf("DVEncoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	dv_encoding = mediaFormat.u.encoded_video.encoding;
}

Encoder *instantiate_encoder(void)
{
	if(dv_encoding == 0)
		return NULL;
	return new DVEncoder();
}

DVEncoder::DVEncoder()
{
	numframes=0;
	tt=0;
	fOutputBuffer = NULL;
}

DVEncoder::~DVEncoder()
{
	if (fOutputBuffer)
		free(fOutputBuffer);
	fOutputBuffer = NULL;
}

status_t 
DVEncoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "DV Compression by Canopus Inc.");
	strcpy(mci->short_name, "dv");
	return B_OK;
}

status_t
DVEncoder::SetFormat(media_file_format *mfi, media_format *in_fmt,
                     media_format *out_format)
{
	in_fmt->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	in_fmt->require_flags = 0;
	out_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	out_format->require_flags = 0;

	if (in_fmt->type != B_MEDIA_RAW_VIDEO) {
		in_fmt->type = B_MEDIA_RAW_VIDEO;
		in_fmt->u.raw_video = media_raw_video_format::wildcard;
	}
	media_raw_video_format *rvf = &in_fmt->u.raw_video;

	rvf->interlace = 1;
	rvf->display.line_width = 720;
	if(rvf->display.line_count >= 512) {
		rvf->display.line_count = 576;
	}
	else {
		rvf->display.line_count = 480;
	}

	switch(rvf->display.format) {
		case B_YCbCr422:
		case B_RGB32:
		case B_RGB16:
		case B_RGB15:
			break;
		default:
			rvf->display.format = B_YCbCr422;
	}

	out_format->type = B_MEDIA_ENCODED_VIDEO;
	out_format->u.encoded_video = media_encoded_video_format::wildcard;
	out_format->u.encoded_video.output = *rvf;
	out_format->u.encoded_video.encoding = dv_encoding;
	
	fWidth  = in_fmt->u.raw_video.display.line_width;
	fHeight = in_fmt->u.raw_video.display.line_count;
	fSpace  = in_fmt->u.raw_video.display.format;

	return B_NO_ERROR;
}

status_t 
DVEncoder::StartEncoder()
{
	if (fDVCodec.InitCheck() < B_OK)
		return fDVCodec.InitCheck();
	
	if (fHeight==480)
		fOutputSize=120000;
	else
		fOutputSize=144000;

	if(fOutputBuffer)
		free(fOutputBuffer);

	fOutputBuffer = (char *)malloc(fOutputSize);

	if (fOutputBuffer == NULL)
		return B_NO_MEMORY;

	return B_NO_ERROR;
}


status_t
DVEncoder::Encode(const void *in_buffer, int64 num_frames,
                  media_encode_info *info)
{
	size_t  output_size = fOutputSize;

	if (fOutputSize==120000)
		fDVCodec.EncodeDVNTSC((void*)in_buffer, fOutputBuffer, fSpace);
	else
		fDVCodec.EncodeDVPAL((void*)in_buffer, fOutputBuffer, fSpace);
	
	info->flags |= B_MEDIA_KEY_FRAME;
	return WriteChunk(fOutputBuffer, output_size, info);
}
