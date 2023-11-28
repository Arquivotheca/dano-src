#ifndef C_AC3AUDIO_DECODE_MANTISSAS_H

#define C_AC3AUDIO_DECODE_MANTISSAS_H

#include "ac3audio.h"

void ac3audio_init_mantissa_decoding(ac3audio_decoder_t *decoder);

float ac3audio_get_float (ac3audio_decoder_t *decoder, bool dither,
							uint8 bap, ac3audio_get_float_state_t *state,
							uint8 exp);
							
void ac3audio_clear_coeffs(ac3audio_decoder_t *decoder);
void ac3audio_mantissa_unpack(ac3audio_decoder_t *decoder);

#endif
