#ifndef C_AC3AUDIO_IMDCT_H

#define C_AC3AUDIO_IMDCT_H

#include "ac3audio.h"

void ac3audio_imdct(ac3audio_decoder_t *decoder, int16 *output);
void ac3audio_imdct_destroy(ac3audio_decoder_t *decoder);
void ac3audio_imdct_init(ac3audio_decoder_t *decoder);

void ac3audio_imdct_512(ac3audio_decoder_t *decoder, const float *X,
						float *x, float *delay);

void ac3audio_imdct_256(ac3audio_decoder_t *decoder, const float *X,
						float *x, float *delay);

void ac3audio_ifft64 (ac3audio_decoder_t *decoder, complex_t *Z);
void ac3audio_ifft128(ac3audio_decoder_t *decoder, complex_t *Z);

void complex_multiply(complex_t *a, const complex_t *b);

#endif
