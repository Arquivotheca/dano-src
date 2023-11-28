#ifndef C_AC3AUDIO_UNCOUPLE_H

#define C_AC3AUDIO_UNCOUPLE_H

#include "ac3audio.h"

void ac3audio_uncouple(ac3audio_decoder_t *decoder);

void ac3audio_uncouple_channel(ac3audio_decoder_t *decoder,
								const float *cpl_tmp, uint8 ch);

float ac3audio_convert_to_float(uint16 mantissa, uint8 exponent);

#endif
