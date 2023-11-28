#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <MediaFormats.h>

#include "Decoder.h"
#include "MediaTrack.h"
//#include "Extractor.h"

#include "Cinepak.h"
//#include "cine_functions.h"

int   cpDecompress(void *ptr,unsigned char *data,unsigned char *baseAddr);
void *cpDecompressInit(long real_width,long real_height,color_space cs);
void  cpDecompressCleanup(void *ptr);


#include <MediaDefs.h>

media_encoded_video_format::video_encoding my_encoding;
static media_format mediaFormat;

void register_decoder(const media_format ** out_format, int32 * out_count)
{
	// printf("CinepakDecoder loaded\n");
	status_t 					err;
	media_format_description	formatDescription[3];
	const int					formatDescription_count =
		sizeof(formatDescription) / sizeof(media_format_description);

	BMediaFormats				formatObject;

	mediaFormat.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription, 0, sizeof(formatDescription));
	formatDescription[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription[0].u.beos.format = 'cvid';
	formatDescription[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription[1].u.avi.codec = 'cvid';
	formatDescription[2].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription[2].u.quicktime.codec = 'cvid';
	formatDescription[2].u.quicktime.vendor = 0;

	err = formatObject.MakeFormatFor(formatDescription, formatDescription_count, &mediaFormat);
	if(err != B_NO_ERROR) {
		//printf("CinepakDecoder: MakeFormatFor failed, %s\n", strerror(err));
	}
	my_encoding = mediaFormat.u.encoded_video.encoding;
	*out_format = &mediaFormat;
	*out_count = 1;
}


Decoder *instantiate_decoder(void)
{
	if(my_encoding == 0) {
		return NULL;
	}
	return new CinepakDecoder();
}

CinepakDecoder::CinepakDecoder()
{
	dptr = NULL;
}

CinepakDecoder::~CinepakDecoder()
{
	cpDecompressCleanup(dptr);
}


status_t
CinepakDecoder::GetCodecInfo(media_codec_info *mci) const
{
	strcpy(mci->pretty_name, "Cinepak Compression");
	strcpy(mci->short_name, "cinepak");
	return B_OK;
}


//	Sniff() is called when looking for a Decoder for a BMediaTrack.
//				it should return an error if the Decoder cannot handle
//				this BMediaTrack. Otherwise, it should initialize the object.
status_t
CinepakDecoder::Sniff(const media_format *in_format,
                      const void *in_info, size_t in_size)
{
	status_t 					err;
	media_format				tempFormat;

	const media_raw_video_format *rvf = &in_format->u.raw_video;

	if (in_format->type != B_MEDIA_ENCODED_VIDEO)
		return B_BAD_TYPE;
		
	if(in_format->u.encoded_video.encoding != my_encoding) {
		//printf("CinepakDecoder::Sniff encoding %d does not match cinepack (%d)\n",
		//       in_format->u.encoded_video.encoding, my_encoding);
		return B_BAD_TYPE;
	}
	
	output_format = in_format->u.encoded_video.output;

	//width  = in_format->u.encoded_video.output.display.line_width;
	//height = in_format->u.encoded_video.output.display.line_count;

	return B_OK;
}


//	Format() gets upon entry the format that the application wants and
//				returns that format untouched if it can output it, or the
//				closest match if it cannot output that format.
status_t
CinepakDecoder::Format(media_format *inout_format)
{
	if(inout_format->type != B_MEDIA_RAW_VIDEO) {
		inout_format->type = B_MEDIA_RAW_VIDEO;
		inout_format->u.raw_video = media_raw_video_format::wildcard;
	}
	media_raw_video_format *rvf = &inout_format->u.raw_video;

	/* Unupported buffer flags */
	inout_format->deny_flags = B_MEDIA_LINEAR_UPDATES
	                         | B_MEDIA_MAUI_UNDEFINED_FLAGS;
	/*  Required buffer flags */
	inout_format->require_flags = B_MEDIA_RETAINED_DATA;

	if(rvf->field_rate == media_raw_video_format::wildcard.field_rate) {
		rvf->field_rate = output_format.field_rate;
	}
	rvf->interlace = output_format.interlace;
	rvf->first_active = output_format.first_active;
	rvf->last_active = output_format.last_active;
	rvf->orientation = B_VIDEO_TOP_LEFT_RIGHT;
	rvf->pixel_width_aspect = output_format.pixel_width_aspect;
	rvf->pixel_height_aspect = output_format.pixel_height_aspect;

	switch(rvf->display.format) {
#ifdef __INTEL__	/* FIXME: This should probably use <endian.h> for the right define */
	case B_RGB32:
#else
	case B_RGB32_BIG:
#endif
		bitmap_depth = 32;
		break;

#ifdef __INTEL__	/* FIXME: This should probably use <endian.h> for the right define */
	case B_RGB16:
#else
	case B_RGB16_BIG:
#endif
		bitmap_depth = 16;
		break;

#ifdef __INTEL__	/* FIXME: This should probably use <endian.h> for the right define */
	case B_RGB15:
#else
	case B_RGB15_BIG:
#endif
		bitmap_depth = 16;
		break;

	default:     /* nothing else is supported; use RGB32 */
#ifdef __INTEL__	/* FIXME: This should probably use <endian.h> for the right define */
		rvf->display.format = B_RGB32;
#else
		rvf->display.format = B_RGB32_BIG;
#endif
		
		bitmap_depth = 32;
		break;
	}
	output_format.display.format = rvf->display.format;
	rvf->display.line_width = output_format.display.line_width;
	rvf->display.line_count = output_format.display.line_count;
	rvf->display.bytes_per_row =
			bitmap_depth / 8 * rvf->display.line_width;

	rvf->display.bytes_per_row = (rvf->display.bytes_per_row + 31) & ~31;

	output_format.display.bytes_per_row = rvf->display.bytes_per_row;
	rvf->display.pixel_offset = 0;
	rvf->display.line_offset = 0;

	dptr=cpDecompressInit(output_format.display.line_width,
	                      (output_format.display.line_count+2)&~2,
	                      output_format.display.format);
	return B_OK;
}


//	Decode() outputs a frame. It gets the chunks from the file
//				by invoking GetTrack()->GetNextChunk(). If Decode() is invoked
//				on a key frame, then it should reset its state before it
//				decodes the frame. it is guaranteed that the output buffer will
//				not be touched by the application, which implies this buffer
//				may be used to cache its state from frame to frame. 
status_t
CinepakDecoder::Decode(void *output, int64 *inout_frameCount,
					   media_header *mh, media_decode_info *info)
{
	status_t		err;
	const void		*compressed_data;
	size_t 			size;
	bigtime_t		time_to_decode = B_INFINITE_TIMEOUT;

	err = GetNextChunk(&compressed_data, &size, mh, info);
	if (err != B_OK)
		return err;

	cpDecompress(dptr,(unsigned char *)compressed_data,
				 (unsigned char *)output);
	*inout_frameCount = 1;

	return err;
}
