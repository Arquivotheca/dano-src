#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "PJPEGcodec.h"


media_encoded_video_format::video_encoding pjpeg_encoding;
media_encoded_video_format::video_encoding mjpeg_encoding;
media_encoded_video_format::video_encoding dmb1_encoding;
static media_format mediaFormat[3];

void register_decoder(const media_format ** out_formats, int32 * out_count)
{
	//printf("PJPEGDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[4];
	BMediaFormats				formatObject;

	mediaFormat[0].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[0].u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'jpeg';
	formatDescription[1].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[1].u.quicktime.codec = 'jpeg';
	formatDescription[1].u.quicktime.vendor = 0;
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'jpeg';
	formatDescription[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription[3].u.avi.codec = 'JPEG';
	err = formatObject.MakeFormatFor(formatDescription, 4, &mediaFormat[0]);
	if(err != B_NO_ERROR) {
		//printf("PJPEGDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	pjpeg_encoding = mediaFormat[0].u.encoded_video.encoding;

	mediaFormat[1].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[1].u.encoded_video = media_encoded_video_format::wildcard;
	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'MJPG';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'MJPG';
//	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
//	formatDescription[2].u.avi.codec = 'dmb1';
//	formatDescription[2].family = B_QUICKTIME_FORMAT_FAMILY;
//	formatDescription[2].u.quicktime.codec = 'smc ';
//	formatDescription[2].u.quicktime.vendor = 0;
	err = formatObject.MakeFormatFor(formatDescription, 2, &mediaFormat[1]);
	if(err != B_NO_ERROR) {
		//printf("PJPEGDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	mjpeg_encoding = mediaFormat[1].u.encoded_video.encoding;

	mediaFormat[2].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[2].u.encoded_video = media_encoded_video_format::wildcard;
	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'dmb1';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'dmb1';
	err = formatObject.MakeFormatFor(formatDescription, 2, &mediaFormat[2]);
	if(err != B_NO_ERROR) {
		//printf("PJPEGDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	dmb1_encoding = mediaFormat[2].u.encoded_video.encoding;

	*out_formats = mediaFormat;
	*out_count = 3;
};

Decoder *instantiate_decoder(void)
{
	if ((pjpeg_encoding == 0) && (mjpeg_encoding == 0) && (dmb1_encoding == 0)) return 0;
	return new PJPEGDecoder();
}

PJPEGDecoder::PJPEGDecoder()
{
}

PJPEGDecoder::~PJPEGDecoder()
{
	/* XXXdbg -- need to free resources used by the jpeg code? */
}


status_t
PJPEGDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Photo-JPEG Compression");
	strcpy(mci->short_name, "pjpeg");
	return B_OK;
}


//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
PJPEGDecoder::Sniff(const media_format *in_format,
                    const void *in_info, size_t in_size)
{
	status_t					err;
	const media_raw_video_format *rvf = &in_format->u.raw_video;

//printf("PJPEGDecoder::Sniff %d my %d %d\n",
//in_format->u.encoded_video.encoding, pjpeg_encoding, mjpeg_encoding);
	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;

	if (in_format->u.encoded_video.encoding == pjpeg_encoding) {
		//printf("PJPEG\n");
		fMJPEG = false;
	}
	else if (in_format->u.encoded_video.encoding == mjpeg_encoding) {
		//printf("MJPEG 1\n");
		fMJPEG = true;
		fTopFieldF1 = false;
	}
	else if (in_format->u.encoded_video.encoding == dmb1_encoding) {
		//printf("MJPEG 2\n");
		fMJPEG = true;
		fTopFieldF1 = true;
	}
	else {
		return B_MEDIA_BAD_FORMAT;
	}

	fOutputFormat = in_format->u.encoded_video.output;
	fOutputFormat.display.format = B_RGB32;
	fOutputFormat.display.bytes_per_row = 4*fOutputFormat.display.line_width;
	
	if (fOutputFormat.interlace == 2)
		return B_MEDIA_BAD_FORMAT;

	
	fInputBuffer = NULL;
	//fField = 0;

	/*
	if(fMJPEG == true)
	{
		//printf("MJPEG  \n");
	}
	else
	{
		//printf("PJPEG  \n");
	}
	*/
	

	return B_NO_ERROR;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
PJPEGDecoder::Format(media_format *inout_format)
{
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;
	
	if(inout_format->type != B_MEDIA_RAW_VIDEO) {
		inout_format->type = B_MEDIA_RAW_VIDEO;
		inout_format->u.raw_video = media_raw_video_format::wildcard;
	}

	media_raw_video_format *rvf = &inout_format->u.raw_video;
	
	
	if(rvf->display.bytes_per_row !=
	   media_video_display_info::wildcard.bytes_per_row &&
	   rvf->display.format == B_RGB32) {
		fOutputFormat.display.bytes_per_row = rvf->display.bytes_per_row;
	}
	inout_format->u.raw_video = fOutputFormat;


	
	InitJPEG(fOutputFormat.display.line_width, fOutputFormat.display.line_count,
	         &fStream);

	return B_OK;
}



//	Decode() outputs a frame. It gets the chunks from the file
//				by invoking GetNextChunk(). If Decode() is invoked
//				on a key frame, then it should reset its state before it
//				decodes the frame. it is guaranteed that the output buffer will
//				not be touched by the application, which implies this buffer
//				may be used to cache its state from frame to frame. 
status_t
PJPEGDecoder::Decode(void *out_buffer, int64 *inout_framecount,
					 media_header *mh, media_decode_info *info)
{
	uint8 *compressed_data;
	int32 size;
	uint32 offset = 0;
	uint32 out_offset_f1 = 0;
	uint32 out_offset_f2 = 0;
	status_t		err;
	int field = fField;
	int32 interlaced;
	
	if(fMJPEG == true) {
		if(fTopFieldF1) {
			out_offset_f1 = 0;
			out_offset_f2 = fOutputFormat.display.bytes_per_row;
		}
		else {
			out_offset_f1 = fOutputFormat.display.bytes_per_row;
			out_offset_f2 = 0;
		}
	}

	
	
	err = GetNextChunk(&fInputBuffer, &fInputBufferSize, &fMh, info);
	if (err != B_OK)
		return err;
	fInputBufferTime = fMh.start_time;
	
	
	
	compressed_data = (uint8*)fInputBuffer;
	size = fInputBufferSize;
	*mh = fMh;
/*
printf("data:");
for(int i=0; i<16; i++) {
	printf(" %02x", compressed_data[i]);
}

printf("\n");
printf("data:");
for(int i=82; i<98; i++) {
	printf(" %02x", compressed_data[i]);
}
printf("\n");
*/

	
	
		if (compressed_data[0] == 'B' && compressed_data[1] == 'M')
		{
			offset = 0x52;
		}
		else
		{
			offset = 0;
		}
		//printf("decode 1 at offset %lx size %d\n", offset, size);
		DecodeJPEG(compressed_data + offset,
				   (uint8*)out_buffer+out_offset_f1,
				   fOutputFormat.display.bytes_per_row/4*(fMJPEG?2:1),
				   fOutputFormat.display.line_count/(fMJPEG?2:1),
				   size - offset,
				   &fStream);
	//printf("decode 1 fin\n");
		if(fMJPEG) {
			mh->type = B_MEDIA_RAW_VIDEO;
			mh->u.raw_video.field_number = 0;
		}
	
	if(fMJPEG) {
		
		offset = B_BENDIAN_TO_HOST_INT32(((uint32*)compressed_data)[3]);
		//printf("decode 2 at offset %lx size %d\n", offset, size);
		if(offset < size) {
			DecodeJPEG(compressed_data + offset,
					   (uint8*)out_buffer+out_offset_f2,
					//+fOutputFormat.display.line_count/2*fOutputFormat.display.bytes_per_row,
					   fOutputFormat.display.bytes_per_row/4*2,
					   fOutputFormat.display.line_count/2,
					   size - offset,
					   &fStream);
		}
		//printf("decode 2 fin\n");
	}
	

	*inout_framecount = 1;

	return B_OK;
}

status_t 
PJPEGDecoder::Reset(int32 in_towhat, int64 in_requiredFrame, int64 *inout_frame, bigtime_t in_requiredTime, bigtime_t *inout_time)
{
	fField = 0;
	fInputBuffer = NULL;
	if(fMJPEG && !fCombineFields) {
		*inout_frame *= 2;
	}
	return B_NO_ERROR;
}


