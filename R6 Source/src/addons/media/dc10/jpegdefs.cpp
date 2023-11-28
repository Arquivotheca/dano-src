#include <media/MediaDefs.h>
#include <media/MediaFormats.h>

#include <string.h>

#include "jpegdefs.h"

//NTSC
const media_video_display_info pjpeg_ntsc_video_info = {
		B_NO_COLOR_SPACE, JPEG_NTSC_WIDTH , JPEG_NTSC_HEIGHT , 0, 0, 0, 0,
		{ 0, 0, 0 }
};

const media_raw_video_format pjpeg_ntsc_raw_format = {
		29.97, 1, 0, (JPEG_NTSC_HEIGHT  ) - 1, B_VIDEO_TOP_LEFT_RIGHT, 0, 0,
		pjpeg_ntsc_video_info
};

media_encoded_video_format pjpeg_ntsc_format = {
		pjpeg_ntsc_raw_format, 0, 0,
		media_encoded_video_format::B_ANY, 0, 0, 0,
		{ 0, 0, 0 }
};



const media_video_display_info pjpeg_half_ntsc_video_info = {
		B_NO_COLOR_SPACE, JPEG_NTSC_WIDTH / 2 , JPEG_NTSC_HEIGHT / 2, 0, 0, 0, 0,
		{ 0, 0, 0 }
};

const media_raw_video_format pjpeg_half_ntsc_raw_format = {
		29.97, 1, 0, (JPEG_NTSC_HEIGHT / 2) - 1, B_VIDEO_TOP_LEFT_RIGHT, 0, 0,
		pjpeg_half_ntsc_video_info
};

media_encoded_video_format pjpeg_ntsc_half_format = {
		pjpeg_half_ntsc_raw_format, 0, 0,
		media_encoded_video_format::B_ANY, 0, 0, 0,
		{ 0, 0, 0 }
};


//PAL SECAM
const media_video_display_info pjpeg_pal_video_info = {
		B_NO_COLOR_SPACE, JPEG_PAL_WIDTH , JPEG_PAL_HEIGHT , 0, 0, 0, 0,
		{ 0, 0, 0 }
};

const media_raw_video_format pjpeg_pal_raw_format = {
		25.0, 1, 0, ( JPEG_PAL_HEIGHT ) - 1, B_VIDEO_TOP_LEFT_RIGHT, 0, 0,
		pjpeg_pal_video_info
};

media_encoded_video_format pjpeg_pal_format = {
		pjpeg_pal_raw_format, 0, 0,
		media_encoded_video_format::B_ANY, 0, 0, 0,
		{ 0, 0, 0 }
};

const media_video_display_info pjpeg_half_pal_video_info = {
		B_NO_COLOR_SPACE, JPEG_PAL_WIDTH / 2, JPEG_PAL_HEIGHT / 2, 0, 0, 0, 0,
		{ 0, 0, 0 }
};

const media_raw_video_format pjpeg_half_pal_raw_format = {
		25.0, 1, 0, ( JPEG_PAL_HEIGHT / 2 ) - 1, B_VIDEO_TOP_LEFT_RIGHT, 0, 0,
		pjpeg_half_pal_video_info
};

media_encoded_video_format pjpeg_pal_half_format = {
		pjpeg_half_pal_raw_format, 0, 0,
		media_encoded_video_format::B_ANY, 0, 0, 0,
		{ 0, 0, 0 }
};

class JPEGDefsAutorun {
public:
	JPEGDefsAutorun();
};

JPEGDefsAutorun::JPEGDefsAutorun()
{
	media_format				mediaFormat1;
	media_format				mediaFormat2;
	media_format_description	formatDescription1[4];
	media_format_description	formatDescription2[2];
	BMediaFormats				formatObject;

	mediaFormat1.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat1.u.encoded_video = media_encoded_video_format::wildcard;

	memset(formatDescription1, 0, sizeof(formatDescription1));

	formatDescription1[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription1[0].u.beos.format = 'jpeg';
	formatDescription1[1].family = B_QUICKTIME_FORMAT_FAMILY;
	formatDescription1[1].u.quicktime.codec = 'jpeg';
	formatDescription1[1].u.quicktime.vendor = 0;
	formatDescription1[2].family = B_AVI_FORMAT_FAMILY;
	formatDescription1[2].u.avi.codec = 'jpeg';
	formatDescription1[3].family = B_AVI_FORMAT_FAMILY;
	formatDescription1[3].u.avi.codec = 'JPEG';

	if (formatObject.MakeFormatFor(formatDescription1, 4, &mediaFormat1) == B_OK) {
		pjpeg_ntsc_half_format.encoding = mediaFormat1.u.encoded_video.encoding;
		pjpeg_pal_half_format.encoding = mediaFormat1.u.encoded_video.encoding;
	}
	
	
	mediaFormat2.type = B_MEDIA_ENCODED_VIDEO;
	mediaFormat2.u.encoded_video = media_encoded_video_format::wildcard;
	memset(formatDescription2, 0, sizeof(formatDescription2));
	formatDescription2[0].family = B_BEOS_FORMAT_FAMILY;
	formatDescription2[0].u.beos.format = 'MJPG';
	formatDescription2[1].family = B_AVI_FORMAT_FAMILY;
	formatDescription2[1].u.avi.codec = 'MJPG';
	
	if (formatObject.MakeFormatFor(formatDescription2, 2, &mediaFormat2) == B_OK) {
		pjpeg_ntsc_format.encoding = mediaFormat2.u.encoded_video.encoding;
		pjpeg_pal_format.encoding = mediaFormat2.u.encoded_video.encoding;
	}

}

JPEGDefsAutorun _;
