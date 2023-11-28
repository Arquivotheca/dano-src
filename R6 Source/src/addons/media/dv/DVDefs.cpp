#include <media/MediaDefs.h>
#include <media/MediaFormats.h>

#include <string.h>

#include "DVDefs.h"

const media_video_display_info dv_ntsc_video_info = {
		B_NO_COLOR_SPACE, DV_NTSC_WIDTH, DV_NTSC_HEIGHT, 0, 0, 0, 0,
		{ 0, 0, 0 }
};

const media_raw_video_format dv_ntsc_raw_format = {
		29.97, 1, 0, DV_NTSC_HEIGHT - 1, B_VIDEO_TOP_LEFT_RIGHT, 0, 0,
		dv_ntsc_video_info
};

media_encoded_video_format dv_ntsc_format = {
		dv_ntsc_raw_format, 0, 0,
		media_encoded_video_format::B_ANY, DV_NTSC_ENCODED_FRAME_SIZE, 0, 0,
		{ 0, 0, 0 }
};

const media_video_display_info dv_pal_video_info = {
		B_NO_COLOR_SPACE, DV_PAL_WIDTH, DV_PAL_HEIGHT, 0, 0, 0, 0,
		{ 0, 0, 0 }
};

const media_raw_video_format dv_pal_raw_format = {
		25.0, 1, 0, DV_PAL_HEIGHT - 1, B_VIDEO_TOP_LEFT_RIGHT, 0, 0,
		dv_pal_video_info
};

media_encoded_video_format dv_pal_format = {
		dv_pal_raw_format, 0, 0,
		media_encoded_video_format::B_ANY, DV_PAL_ENCODED_FRAME_SIZE, 0, 0,
		{ 0, 0, 0 }
};

class DVDefsAutorun {
public:
	DVDefsAutorun();
};

DVDefsAutorun::DVDefsAutorun()
{
	media_format				mediaFormat;
	media_format_description	formatDescription[5];
	BMediaFormats				formatObject;

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

	if (formatObject.MakeFormatFor(formatDescription, 5, &mediaFormat) == B_OK) {
		dv_ntsc_format.encoding = mediaFormat.u.encoded_video.encoding;
		dv_pal_format.encoding = mediaFormat.u.encoded_video.encoding;
	}
}

DVDefsAutorun _;
