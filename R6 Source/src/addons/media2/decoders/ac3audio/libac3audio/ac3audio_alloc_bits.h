#ifndef C_AC3AUDIO_ALLOCATE_BITS_H

#define C_AC3AUDIO_ALLOCATE_BITS_H

#include "ac3audio.h"

void ac3audio_alloc_bits (ac3audio_decoder_t *decoder);

void ac3audio_alloc_bits_channel (ac3audio_decoder_t *decoder,
									uint8 *bap_coeff,
									const uint8 *exp,
									uint8 start, uint8 end,
									uint16 lowcomp, uint16 fastgain,
									int16 snroffset,
									uint8 deltbae, uint8 deltnseg,
									const uint8 *deltoffst,
									const uint8 *deltba,
									const uint8 *deltlen,
									bool is_lfe);

int16 ac3audio_calc_low_comp (uint16 a, uint16 b0, uint16 b1, uint8 bin);									
int16 ac3audio_log_add (uint16 a, uint16 b);
void ac3audio_clear_bap(ac3audio_decoder_t *decoder);
								
#endif
