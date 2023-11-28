#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "MJPEGcodec.h"


media_encoded_video_format::video_encoding mjpeg_encoding;

void register_encoder(void)
{
	//printf("MJPEGEncoder loaded\n");
	status_t 					err;
	media_format				mediaFormat;
	media_format_description	formatDescription[2];
	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	memset(&formatDescription, 0, sizeof(media_format_description));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'MJPG';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'MJPG';
	err = formatObject.MakeFormatFor(formatDescription, 2, &mediaFormat,
	                                 BMediaFormats::B_SET_DEFAULT);
	if(err != B_NO_ERROR) {
		printf("MJPEGEncoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	mjpeg_encoding = mediaFormat.u.encoded_video.encoding;
}

Encoder *instantiate_encoder(void)
{
	if(mjpeg_encoding == 0)
		return NULL;
	return new MJPEGEncoder();
}


MJPEGEncoder::MJPEGEncoder()
{
	fQuality         = 75;
	fOutputBuffer    = NULL;
	fBuffer1 		 = NULL;
	fBuffer2 		 = NULL;
	fStream.linePtrs = NULL;
}

MJPEGEncoder::~MJPEGEncoder()
{
	if (fOutputBuffer)
		free(fOutputBuffer);
	if (fBuffer1)
		free(fBuffer1);
	if (fBuffer2)
		free(fBuffer2);
	fOutputBuffer = NULL;
	fBuffer1 = NULL;
	fBuffer2 = NULL;
	
	if (fStream.linePtrs)
		free(fStream.linePtrs);
	fStream.linePtrs = NULL;
}


status_t 
MJPEGEncoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Motion-JPEG Compression");
	strcpy(mci->short_name, "MJPG");
	return B_OK;
}

status_t
MJPEGEncoder::SetFormat(media_file_format *mfi,
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

	if (mfi->family != B_AVI_FORMAT_FAMILY) {
		mfi->family = B_AVI_FORMAT_FAMILY;
	}

	out_format->type = B_MEDIA_ENCODED_VIDEO;
	out_format->u.encoded_video.output = in_fmt->u.raw_video;
	out_format->u.encoded_video.encoding = mjpeg_encoding;

	fWidth  = in_fmt->u.raw_video.display.line_width;
	fHeight = in_fmt->u.raw_video.display.line_count;

	return B_OK;
}

status_t 
MJPEGEncoder::StartEncoder()
{
	InitJPEG(fWidth, fHeight / 2, &fStream);

	fOutputSize   = fWidth * fHeight * 4;
	fOutputBuffer = (char *)malloc(fOutputSize);
	if (fOutputBuffer == NULL)
		return B_NO_MEMORY;
		
	fBuffer1 = (char *)malloc(fOutputSize / 2);
	if (fBuffer1 == NULL)
		return B_NO_MEMORY;
	fBuffer2 = (char *)malloc(fOutputSize / 2);
	if (fBuffer2 == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


status_t
MJPEGEncoder::Encode(const void *in_buffer, int64 num_frames,
                     media_encode_info *info)
{
	size_t  output_size = fOutputSize;
	size_t  output_size1,output_size2;
	unsigned char * fInBuffer,*fBuf1,*fBuf2;
	char * fOut;
	uint32 * preg;
	int32 i;
	
	fInBuffer = (unsigned char *)(in_buffer);
	fBuf1 = (unsigned char *) (fBuffer1);
	for(i=0;i<fHeight / 2;i++)
	{
		memcpy(fBuf1,fInBuffer,4*fWidth);
		fBuf1 += 4 * fWidth;
		fInBuffer += 2 * 4 * fWidth;
	}
	fInBuffer = (unsigned char *)(in_buffer);
	fInBuffer += 4 * fWidth;
	fBuf2 = (unsigned char *) (fBuffer2);
	for(i=0;i<fHeight / 2;i++)
	{
		memcpy(fBuf2,fInBuffer,4*fWidth);
		fBuf2 += 4 * fWidth;
		fInBuffer += 2 * 4 * fWidth;
	}
	
	fOut = fOutputBuffer;
	
	fOut[0] = 'B';
	fOut[1] = 'M';
	
	fOut += 0x52;
	
	EncodeJPEG((char *)fBuffer2,
			   fOut,
			   &output_size2,
			   fWidth,
			   fHeight/2,
			   fQuality,
			   &fStream);
			   
	fOut += output_size2;
	
	preg = (uint32 *) fOutputBuffer;
	preg[3] = B_HOST_TO_BENDIAN_INT32(0x52 + output_size2);
	
	EncodeJPEG((char *)fBuffer1,
			   fOut,
			   &output_size1,
			   fWidth,
			   fHeight/2,
			   fQuality,
			   &fStream);						


	output_size = 0x52 + output_size1 + output_size2;
	
	info->flags |= B_MEDIA_KEY_FRAME;
	return WriteChunk(fOutputBuffer, output_size, info);
}
	
status_t 
MJPEGEncoder::GetEncodeParameters(encode_parameters *parameters) const
{
	parameters->quality = fQuality / 100.0;
	return B_NO_ERROR;
}

status_t 
MJPEGEncoder::SetEncodeParameters(encode_parameters *parameters)
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
