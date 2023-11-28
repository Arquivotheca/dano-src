#ifndef C_MPEG2_VIDEO_EXTENSION_H

#define C_MPEG2_VIDEO_EXTENSION_H

#include "mpeg2video.h"

void mpeg2video_parse_extension (mpeg2video_decoder_t *decoder);
void parse_sequence_extension(mpeg2video_decoder_t *decoder);
void parse_picture_coding_extension(mpeg2video_decoder_t *decoder);

#endif
