#ifndef C_AC3AUDIO_BSI_H

#define C_AC3AUDIO_BSI_H

#include "ac3audio.h"

void ac3audio_parse_bsi (ac3audio_decoder_t *decoder);

void ac3audio_parse_bsi_params (ac3audio_decoder_t *decoder,
								ac3audio_bsi_param_t *bsi_param);

#endif
