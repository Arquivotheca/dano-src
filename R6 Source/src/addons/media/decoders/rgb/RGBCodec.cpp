#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
#include "Extractor.h"

#include "RIFFTypes.h"
#include "RGBCodec.h"

#include "convert.h"

media_encoded_video_format::video_encoding avi_rgb_encoding;
media_encoded_video_format::video_encoding quicktime_rgb_encoding;
media_format mediaFormat[2];

void register_decoder(const media_format ** out_format, int32 * out_count)
{
	//printf("RGBDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[2];
	BMediaFormats				formatObject;

	mediaFormat[0].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[0].u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_AVI_FORMAT_FAMILY;
	formatDescription[0].u.avi.codec = 0;
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'RGB2';

	err = formatObject.MakeFormatFor(formatDescription, 2, &mediaFormat[0]);
	if(err != B_NO_ERROR) {
		//printf("RGBDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	avi_rgb_encoding = mediaFormat[0].u.encoded_video.encoding;

	mediaFormat[1].type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat[1].u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[0].u.quicktime.vendor = 0;
	formatDescription[0].u.quicktime.codec = 'raw ';

	err = formatObject.MakeFormatFor(formatDescription, 1, &mediaFormat[1]);
	if(err != B_NO_ERROR) {
		//printf("RGBDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	quicktime_rgb_encoding = mediaFormat[1].u.encoded_video.encoding;

	*out_format = mediaFormat;
	*out_count = 2;
};

Decoder *instantiate_decoder(void)
{
	if ((avi_rgb_encoding == 0) && (quicktime_rgb_encoding == 0)) return 0;
	return new RGBCodecDecoder();
}

RGBCodecDecoder::RGBCodecDecoder()
{
	convert = NULL;
}

RGBCodecDecoder::~RGBCodecDecoder()
{
}


status_t
RGBCodecDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Raw RGB Codec");
	strcpy(mci->short_name, "raw-video");
	return B_OK;
}


//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
RGBCodecDecoder::Sniff(const media_format *in_format,
					   const void *in_info, size_t in_size)
{
	//const media_raw_video_format *rvf = &in_format->u.raw_video;
	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;

	if(in_format->u.encoded_video.encoding == avi_rgb_encoding) {
		AVIVIDSHeader *vids = (AVIVIDSHeader *)in_info;
		
		if(vids == NULL)
			return B_BAD_TYPE;

		if(vids->BitCount != 16 &&
	       vids->BitCount != 24 &&
	       vids->BitCount != 32)
			return B_BAD_TYPE;
		upsidedown = vids->Height >= 0;
		bitmap_depth = vids->BitCount;
		bigendian = false;
	}
	else if(in_format->u.encoded_video.encoding == quicktime_rgb_encoding) {
		upsidedown = false;
		bigendian = true;
		switch(in_format->u.encoded_video.output.display.format) {
			case B_RGB15_BIG:
				bitmap_depth = 16;
				break;
			case B_RGB24_BIG:
				bitmap_depth = 24;
				break;
			case B_RGB32_BIG:
				bitmap_depth = 32;
				break;
			default:
				return B_BAD_TYPE;
		}
	}
	else {
		return B_BAD_TYPE;
	}
	output_format = in_format->u.encoded_video.output;
	return B_NO_ERROR;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
RGBCodecDecoder::Format(media_format *inout_format)
{
	if(inout_format->type != B_MEDIA_RAW_VIDEO) {
		inout_format->type = B_MEDIA_RAW_VIDEO;
		inout_format->u.raw_video = media_raw_video_format::wildcard;
	}
	media_raw_video_format *rvf = &inout_format->u.raw_video;
	
	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = 0;

	if(rvf->field_rate == media_raw_video_format::wildcard.field_rate) {
		rvf->field_rate = output_format.field_rate;
	}
	rvf->interlace = output_format.interlace;
	rvf->first_active = output_format.first_active;
	rvf->last_active = output_format.last_active;
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = output_format.pixel_width_aspect;
	rvf->pixel_height_aspect = output_format.pixel_height_aspect;
	rvf->display.line_width = output_format.display.line_width;
	rvf->display.line_count = output_format.display.line_count;
	rvf->display.pixel_offset = 0;
	rvf->display.line_offset = 0;

	convert = get_converter(bitmap_depth, bigendian, upsidedown, &rvf->display);

	if(convert == NULL)
		return B_BAD_TYPE;

	output_format.display.bytes_per_row = rvf->display.bytes_per_row;

	// this may be wrong
	src_bytes_per_row = (output_format.display.line_width +
		(output_format.display.line_width&1)) * bitmap_depth/8;

	//printf("src_bytes_per_row: %d\n", src_bytes_per_row);
	//printf("dest_bytes_per_row: %d\n", dest_bytes_per_row);

	return B_OK;
}

//	Decode() outputs a frame. It gets the chunks from the file
//				by invoking GetNextChunk(). If Decode() is invoked
//				on a key frame, then it should reset its state before it
//				decodes the frame. it is guaranteed that the output buffer will
//				not be touched by the application, which implies this buffer
//				may be used to cache its state from frame to frame. 

status_t
RGBCodecDecoder::Decode(void *out_buffer, int64 *frame_count,
						media_header *mh, media_decode_info *info)
{
	const void  *compressed_data;
	size_t		size;
	status_t	err;

	err = GetNextChunk(&compressed_data, &size, mh, info);
	if (err != B_OK)
		return err;
		
	*frame_count = 1;

	if(convert == NULL)
		return B_ERROR;
		
	if(size < src_bytes_per_row * output_format.display.line_count) {
		printf("chunk size (%ld) < image size (%ld) (%ld*%ld*%d)\n",
		       size, output_format.display.line_count * src_bytes_per_row,
		       output_format.display.line_width,
		       output_format.display.line_count, bitmap_depth);
		convert(output_format.display.line_width, size / src_bytes_per_row,
		        compressed_data, src_bytes_per_row,
		        out_buffer, output_format.display.bytes_per_row);
		return B_NO_ERROR;
	}
	if(size > src_bytes_per_row * output_format.display.line_count) {
		printf("chunk size (%ld) > image size (%ld) (%ld*%ld*%d)\n",
		       size, output_format.display.line_count * src_bytes_per_row,
		       output_format.display.line_width,
		       output_format.display.line_count, bitmap_depth);
	}

	//bigtime_t t1 = system_time();
	convert(output_format.display.line_width, output_format.display.line_count,
	        compressed_data, src_bytes_per_row,
	        out_buffer, output_format.display.bytes_per_row);
	//bigtime_t t2 = system_time();
	//printf("time to copy frame: %Ld\n", t2-t1);

	return B_OK;
}
	


