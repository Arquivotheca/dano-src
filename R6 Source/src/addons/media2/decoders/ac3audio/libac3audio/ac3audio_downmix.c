#include "ac3audio_downmix.h"
#include <mpeg2lib.h>

#include <support/Debug.h>

#define GAIN 32000.0f

void
ac3audio_passthru (ac3audio_decoder_t *decoder, int16 *output)
{
	int32 i,ch;
	
	for (i=0;i<256;++i)
	{
		for (ch=0;ch<decoder->bsi.nfchans;++ch)
			*output++=((int16)decoder->samples->channel[ch][i]*GAIN);
			
		if (decoder->bsi.lfeon)
			*output++=((int16)decoder->samples->channel[5][i]*GAIN);		
	}
}

void
ac3audio_conventional_stereo (ac3audio_decoder_t *decoder, int16 *output)
{
	int32 i;
	
	switch (decoder->bsi.acmod)
	{
		case C_INDEPENDENT_MONO:
		{
			// dual-mono
			
			const uint8 channel=decoder->first_mono_channel_selected ? 0 : 1;
			
			for (i=0;i<256;++i,output+=2)
				output[0]=output[1]=((int16)GAIN*decoder->samples->channel[channel][i]);
				
			break;
		}
		
		case C_CENTER:
		{
			for (i=0;i<256;++i,output+=2)
				output[0]=output[1]=((int16)GAIN*decoder->samples->channel[0][i]);

			break;
		}
		
		case C_LEFT_RIGHT:
		{
			// stereo

			for (i=0;i<256;++i)
			{
				*output++=((int16)GAIN*decoder->samples->channel[0][i]);
				*output++=((int16)GAIN*decoder->samples->channel[1][i]);
			}
								
			break;
		}
		
		case C_LEFT_CENTER_RIGHT:
		{
			const float kCenterMixLevel[3] = { 0.707f,0.596f,0.5f };
			
			float center_mix_level=(decoder->bsi.cmixlev<3) ? kCenterMixLevel[decoder->bsi.cmixlev]
														  : kCenterMixLevel[1];

			const float c=GAIN/(1.0f+center_mix_level);
			
			for (i=0;i<256;++i)
			{
				*output++=((int16)c*((decoder->samples->channel[0][i]
							+decoder->samples->channel[1][i]*center_mix_level)));
	
				*output++=((int16)c*((decoder->samples->channel[2][i]
							+decoder->samples->channel[1][i]*center_mix_level)));
			}

			break;
		}
		
		case C_LEFT_CENTER_RIGHT_LSURROUND_RSURROUND:
		{
			const float kCenterMixLevel[3] = { 0.707f,0.596f,0.5f };
			const float kSurroundMixLevel[3] = { 0.707f,0.5f,0.0f };
			
			float center_mix_level=(decoder->bsi.cmixlev<3) ? kCenterMixLevel[decoder->bsi.cmixlev]
														  : kCenterMixLevel[1];

			float surround_mix_level=(decoder->bsi.surmixlev<3) ? kSurroundMixLevel[decoder->bsi.surmixlev]
														  : kSurroundMixLevel[1];
			
			if (!decoder->bsi.lfeon)
			{
				const float c=GAIN/(1.0f+center_mix_level+surround_mix_level);
				
				for (i=0;i<256;++i)
				{
					*output++=((int16)c*((decoder->samples->channel[0][i]
								+decoder->samples->channel[1][i]*center_mix_level
								+decoder->samples->channel[3][i]*surround_mix_level)));
	
					*output++=((int16)c*((decoder->samples->channel[2][i]
								+decoder->samples->channel[1][i]*center_mix_level
								+decoder->samples->channel[4][i]*surround_mix_level)));
				}
			}
			else
			{
				const float lfe_gain=0.707;
				const float c=GAIN/(1.0f+center_mix_level+surround_mix_level+lfe_gain);

				downmix_5_plus_1 (output,decoder->samples->channel[0],
										&center_mix_level,
										&surround_mix_level,
										&c);
			}
						
			break;
		}
		
		default:
		{
			TRESPASS();
			break;
		}
	}
}
