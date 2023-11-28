#ifndef C_MPEG2_VIDEO_PICTURE_H

#define C_MPEG2_VIDEO_PICTURE_H

#include "mpeg2video.h"

void mpeg2video_parse_picture (mpeg2video_decoder_t *decoder);
void update_derived_picture_info (mpeg2video_decoder_t *decoder);

#endif
