#ifndef C_MPEG2_VIDEO_SEQ_HEADER_H

#define C_MPEG2_VIDEO_SEQ_HEADER_H

#include "mpeg2video.h"

void mpeg2video_parse_sequence_header (mpeg2video_decoder_t *decoder);

float mpeg2video_get_frame_rate (uint8 code);
void mpeg2video_get_quant_matrix(mpeg2video_decoder_t *decoder, uint8 *qmatrix);
void mpeg2video_load_default_quant_matrix(uint8 *qmatrix, const uint8 *default_matrix);
void update_derived_sequence_info(mpeg2video_decoder_t *decoder);

extern const uint8 kZigZagOffsetMMX[2][64];
extern const uint8 kZigZagOffset[2][64];

#endif
