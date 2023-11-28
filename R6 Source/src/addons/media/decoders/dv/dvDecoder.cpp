#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>
#include <ByteOrder.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"

#include "dvDecoder.h"

media_encoded_video_format::video_encoding dv_video_encoding;
media_encoded_video_format::video_encoding dv_stream_encoding;

#if 0
void register_decoder(void)
{
        //printf("DVDecoder loaded\n");
        status_t                                        err;
        media_format                            mediaFormat;
        media_format_description        formatDescription[5];
        const int                                       formatDescription_count =
                sizeof(formatDescription) / sizeof(media_format_description);

        BMediaFormats                           formatObject;

        mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
        mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

        memset(formatDescription, 0, sizeof(formatDescription));

        formatDescription[0].family = B_AVI_FORMAT_FAMILY;
        formatDescription[0].u.avi.codec = 'cdvc';
        formatDescription[1].family = B_AVI_FORMAT_FAMILY;
        formatDescription[1].u.avi.codec = 'CDVC';
        formatDescription[2].family = B_AVI_FORMAT_FAMILY;
        formatDescription[2].u.avi.codec = 'dvsd';
        formatDescription[3].family = B_AVI_FORMAT_FAMILY;
        formatDescription[3].u.avi.codec = 'pcdv';
        formatDescription[4].family = B_QUICKTIME_FORMAT_FAMILY;
        formatDescription[4].u.quicktime.vendor = 0;
        formatDescription[4].u.quicktime.codec  = 'dvc ';

        err = formatObject.MakeFormatFor(formatDescription, formatDescription_count, &mediaFormat);
        if(err != B_NO_ERROR) {
                //printf("DVDecoder: MakeFormatFor failed, %s\n", strerror(err));
        }
        my_encoding = mediaFormat.u.encoded_video.encoding;
}

#else
static media_format mediaFormat[2];
void register_decoder(const media_format ** out_format, int32 * out_count)
{
	//printf("DVDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[5];
	BMediaFormats				formatObject;

	mediaFormat[0].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[0].u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));

	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'dv_v';
	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat[0]);
	if(err != B_NO_ERROR) {
		//printf("DVDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	dv_video_encoding = mediaFormat[0].u.encoded_video.encoding;

	mediaFormat[1].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[1].u.encoded_video = media_encoded_video_format::wildcard;
	
	memset(formatDescription, 0, sizeof(formatDescription));
	
	formatDescription[0].family = B_AVI_FORMAT_FAMILY;
	formatDescription[0].u.avi.codec = 'cdvc';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'CDVC';
	formatDescription[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription[2].u.avi.codec = 'dvsd';
	formatDescription[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription[3].u.avi.codec = 'pcdv';
	formatDescription[4].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[4].u.quicktime.vendor = 0;
	formatDescription[4].u.quicktime.codec  = 'dvc ';
	
	err = formatObject.MakeFormatFor(formatDescription, 5, &mediaFormat[1]);
	if(err != B_NO_ERROR) {
	        //printf("DVDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	dv_stream_encoding = mediaFormat[1].u.encoded_video.encoding;
	*out_format = mediaFormat;
	*out_count = 2;
}
#endif

Decoder *instantiate_decoder(void)
{
	if ((dv_stream_encoding == 0) && (dv_video_encoding == 0)) {
		return 0;
	}

	return new DVDecoder();
}

DVDecoder::DVDecoder()
{
	fDVCodec = NULL;
}

DVDecoder::~DVDecoder()
{
	if (fDVCodec)
		delete fDVCodec;
}

status_t
DVDecoder::GetCodecInfo(media_codec_info *mci) const
{
    strcpy(mci->pretty_name, "DV Compression by Canopus Inc.");
    strcpy(mci->short_name, "dv");
    return B_OK;
}

//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
DVDecoder::Sniff(const media_format *in_format,
                 const void *in_info, size_t in_size)
{
	status_t err = B_OK;

	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;

	if (in_format->u.encoded_video.encoding == 0)
		return B_BAD_TYPE;
		
	if (in_format->u.encoded_video.encoding != dv_video_encoding &&
	    in_format->u.encoded_video.encoding != dv_stream_encoding)
		return B_BAD_TYPE;

	if (!fDVCodec) {
		fDVCodec = new DVCodec();
		if (!fDVCodec) {
			err = B_NO_MEMORY;
		} else
			err = fDVCodec->InitCheck();
	}
	if (err < B_OK)
		return err;

	output_format = in_format->u.encoded_video.output;

	return B_OK;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
DVDecoder::Format(media_format *inout_format)
{
	int bytes_per_pixel;
	media_raw_video_format *rvf = &inout_format->u.raw_video;

	inout_format->type = B_MEDIA_RAW_VIDEO;
	
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_LINEAR_UPDATES
	                         | B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	if(rvf->field_rate == media_raw_video_format::wildcard.field_rate) {
		rvf->field_rate = output_format.field_rate;
	}
	rvf->interlace = 1;
	rvf->first_active = output_format.first_active;
	rvf->last_active = output_format.last_active;
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = output_format.pixel_width_aspect;
	rvf->pixel_height_aspect = output_format.pixel_height_aspect;

	switch(rvf->display.format) {
		default:
			rvf->display.format = B_RGB32;
		case B_RGB32:
			bytes_per_pixel = 4;
			break;

		case B_RGB16:
		case B_RGB15:
		case B_YCbCr422:
			bytes_per_pixel = 2;
			break;
	}

	output_format.display.format = rvf->display.format;
	rvf->display.line_width = output_format.display.line_width;
	rvf->display.line_count = output_format.display.line_count;
#if 0
	if(rvf->display.bytes_per_row ==
	   media_raw_video_format::wildcard.display.bytes_per_row) {
		rvf->display.bytes_per_row =
			bytes_per_pixel * output_format.display.line_width;
	}
	output_format.display.bytes_per_row = rvf->display.bytes_per_row;
#endif
	rvf->display.bytes_per_row =
			bytes_per_pixel * output_format.display.line_width;
	rvf->display.pixel_offset = 0;
	rvf->display.line_offset = 0;

	return B_OK;
}


//	Decode() outputs a frame. It gets the chunks from the file
//				by invoking GetNextChunk(). If Decode() is invoked
//				on a key frame, then it should reset its state before it
//				decodes the frame. it is guaranteed that the output buffer will
//				not be touched by the application, which implies this buffer
//				may be used to cache its state from frame to frame. 
status_t
DVDecoder::Decode(void *output, int64 *frame_count, media_header *mh,
                  media_decode_info *info)
{
	const void *compressed_data;
	status_t err;
	size_t size;

	if (!fDVCodec) {
		printf("Codec not yet initialized; call Sniff first!\n");
		/* XXX: do it here then */
		return B_NO_INIT;
	}

	err = GetNextChunk(&compressed_data, &size, mh, info);
	if (err < B_OK)
		return err;

	if (!compressed_data)
		return B_ERROR;

	/* at this point the encoded DV data is pointed to by compressed_data
	   and is size bytes long.  we need to decode it into the buffer
	   pointed to by output.  the format we decode to was given to us in
	   our Format() hook.
    */
	if (size == 120000)
		fDVCodec->DecodeDVNTSC((void*)compressed_data, output,
				output_format.display.format);
	else if (size == 144000)
		fDVCodec->DecodeDVPAL((void*)compressed_data, output,
				output_format.display.format);

	*frame_count = 1;

	return B_OK;
}
	
