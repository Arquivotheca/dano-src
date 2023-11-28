#ifndef C_AC3AUDIO_DECODE_EXPONENTS_H

#define C_AC3AUDIO_DECODE_EXPONENTS_H

#include "ac3audio.h"

void ac3audio_decode_exponent_set (uint8 strategy, uint8 group_count,
									uint8 initial_exponent,
									const uint8 *encoded_exponents,
									uint8 *out_exps);
									
void ac3audio_decode_exponents(ac3audio_decoder_t *decoder);

#endif
