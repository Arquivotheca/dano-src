#include "ac3audio_dec_exponents.h"

void
ac3audio_decode_exponent_set (uint8 strategy, uint8 group_count,
									uint8 initial_exponent,
									const uint8 *encoded_exponents,
									uint8 *out_exps)
{
	const uint8 group_size=1<<(strategy-1);
	uint16 i,j;
	
	uint8 gexp=initial_exponent;
	
	for (i=0;i<group_count;++i)
	{
		gexp += encoded_exponents[i]/25;
		gexp -= 2;
		
		for (j=0;j<group_size;++j)
			*out_exps++ = gexp;

		gexp += (encoded_exponents[i]%25)/5;
		gexp -= 2;

		for (j=0;j<group_size;++j)
			*out_exps++ = gexp;

		gexp += (encoded_exponents[i]%25)%5;
		gexp -= 2;

		for (j=0;j<group_size;++j)
			*out_exps++ = gexp;				
	}
}
									
void 
ac3audio_decode_exponents (ac3audio_decoder_t *decoder)
{
	int32 ch;
	
	for (ch=0;ch<decoder->bsi.nfchans;++ch)
	{
		if (decoder->audblk.chexpstr[ch])
		{
			decoder->exponents.fbw[ch][0]=decoder->audblk.exps[ch][0];
			
			ac3audio_decode_exponent_set(decoder->audblk.chexpstr[ch],
												decoder->audblk.nchgrps[ch],
												decoder->audblk.exps[ch][0],
												&decoder->audblk.exps[ch][1],
												&decoder->exponents.fbw[ch][1]);
		}
	}

	if (decoder->audblk.cplinu)
	{
		if (decoder->audblk.cplexpstr)
		{
			ac3audio_decode_exponent_set(decoder->audblk.cplexpstr,
												decoder->audblk.ncplgrps,
												decoder->audblk.cplabsexp*2,
												decoder->audblk.cplexps,
												decoder->exponents.cpl+decoder->audblk.cplstrtmant);
		}
	}

	if (decoder->bsi.lfeon)
	{
		if (decoder->audblk.lfeexpstr)
		{
			decoder->exponents.lfe[0]=decoder->audblk.lfeexps[0];
			
			ac3audio_decode_exponent_set(decoder->audblk.lfeexpstr,
												2,
												decoder->audblk.lfeexps[0],
												&decoder->audblk.lfeexps[1],
												&decoder->exponents.lfe[1]);			
		}
	}
}

