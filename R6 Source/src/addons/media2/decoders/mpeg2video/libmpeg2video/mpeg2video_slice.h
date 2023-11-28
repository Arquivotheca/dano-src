#ifndef C_MPEG2_VIDEO_SLICE_H

#define C_MPEG2_VIDEO_SLICE_H

#include "mpeg2video.h"

void mpeg2video_parse_slice (mpeg2video_decoder_t *decoder, uint8 slice_vertical_position);
void mpeg2video_reset_dct_predictors(mpeg2video_decoder_t *decoder);
void mpeg2video_reset_motion_predictors(mpeg2video_decoder_t *decoder);
uint8 mpeg2video_get_quantiser_scale(uint8 quantiser_scale_code, uint8 q_scale_type);
uint32 get_mb_addr_increment(mpeg2video_decoder_t *decoder);
uint16 get_pattern_code(mpeg2video_decoder_t *decoder, mpeg2_macroblock_t mb_type);
uint8 get_coded_block_pattern(mpeg2video_decoder_t *decoder);
uint8 get_macro_block_type(mpeg2video_decoder_t *decoder);
uint8 get_i_picture_macro_block_type(mpeg2video_decoder_t *decoder);
uint8 get_p_picture_macro_block_type(mpeg2video_decoder_t *decoder);
uint8 get_b_picture_macro_block_type(mpeg2video_decoder_t *decoder);
void apply_block_idct(mpeg2video_decoder_t *decoder, uint8 block_num);
void output_current_picture(mpeg2video_decoder_t *decoder);

#endif
