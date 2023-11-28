#ifndef C_AC3AUDIO_DOWNMIX_H

#define C_AC3AUDIO_DOWNMIX_H

#include "ac3audio.h"

void ac3audio_passthru(ac3audio_decoder_t *decoder, int16 *output);
void ac3audio_conventional_stereo(ac3audio_decoder_t *decoder, int16 *output);

#endif
