#ifndef C_MPEG2_VIDEO_MOTION_COMPENSATION_H

#define C_MPEG2_VIDEO_MOTION_COMPENSATION_H

#include "mpeg2video.h"

bool motion_compensation(mpeg2video_decoder_t *decoder, mpeg2_macroblock_t mb_type);
void forward_backward_motion_compensation(mpeg2video_decoder_t *decoder, uint8 backward, bool average, bool field, bool bottom);
void add_or_copy_block(mpeg2video_decoder_t *decoder, uint8 block_num, bool add);

#endif
