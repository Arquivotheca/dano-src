#ifndef C_MPEG2_VIDEO_MOTION_VECTORS_H

#define C_MPEG2_VIDEO_MOTION_VECTORS_H

#include "mpeg2video.h"

void parse_motion_vectors(mpeg2video_decoder_t *decoder, uint8 s);
void parse_motion_vector(mpeg2video_decoder_t *decoder, uint8 r, uint8 s);
int8 get_motion_code(mpeg2video_decoder_t *decoder);
int8 get_dm_vector(mpeg2video_decoder_t *decoder);
void decode_mv(int16 *pmv, uint8 r_size, int8 motion_code, uint8 motion_residual);

#endif
