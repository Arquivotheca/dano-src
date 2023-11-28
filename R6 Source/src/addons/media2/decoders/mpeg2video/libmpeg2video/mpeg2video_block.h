#ifndef C_MPEG2_VIDEO_BLOCK_H

#define C_MPEG2_VIDEO_BLOCK_H

#include "mpeg2video.h"

uint8 get_dct_dc_size_chrominance (mpeg2video_decoder_t *decoder);
uint8 get_dct_dc_size_luminance(mpeg2video_decoder_t *decoder);

void parse_block (mpeg2video_decoder_t *decoder, uint8 block_num,
					mpeg2_macroblock_t mb_type);

int8 parse_mpeg2_intra_block (mpeg2video_decoder_t *decoder,
								int16 *block, uint8 cc);

int8 parse_mpeg2_non_intra_block (mpeg2video_decoder_t *decoder,
									int16 *block, uint8 cc);

int8 parse_mpeg1_intra_block (mpeg2video_decoder_t *decoder,
								int16 *block, uint8 cc);

int8 parse_mpeg1_non_intra_block (mpeg2video_decoder_t *decoder,
									int16 *block, uint8 cc);

bool get_mpeg1_dct_coeff_intra(mpeg2video_decoder_t *decoder,
								uint8 *run, int16 *level, bool *sign);


bool get_mpeg1_dct_coeff_non_intra(mpeg2video_decoder_t *decoder,
									bool first_coeff, uint8 *run,
									int16 *level, bool *sign);
									
int32 get_mpeg2_dct_coeff_intra(mpeg2video_decoder_t *decoder);
int32 get_mpeg2_dct_coeff_non_intra(mpeg2video_decoder_t *decoder, bool first);

#endif
