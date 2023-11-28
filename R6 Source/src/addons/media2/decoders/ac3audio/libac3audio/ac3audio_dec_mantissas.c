#include "ac3audio_dec_mantissas.h"

#include <stdlib.h>
#include <string.h>

void
ac3audio_init_mantissa_decoding (ac3audio_decoder_t *decoder)
{
	int32 group_code;
	
	for (group_code=0;group_code<32;++group_code)
	{
		static const float mantissa_value[3] = { (-2<<15)/3,0,(2<<15)/3 };

		decoder->fast_table_1[group_code][2]=mantissa_value[group_code/9];
		decoder->fast_table_1[group_code][1]=mantissa_value[(group_code/3)%3];
		decoder->fast_table_1[group_code][0]=mantissa_value[group_code%3];		
	}

	for (group_code=0;group_code<128;++group_code)
	{
		static const float mantissa_value[5] = { (-4<<15)/5,(-2<<15)/5,0,(2<<15)/5,(4<<15)/5 };

		decoder->fast_table_2[group_code][2]=mantissa_value[group_code/25];
		decoder->fast_table_2[group_code][1]=mantissa_value[(group_code/5)%5];
		decoder->fast_table_2[group_code][0]=mantissa_value[group_code%5];		
	}

	for (group_code=0;group_code<128;++group_code)
	{
		static const float mantissa_value[11] = { (-10<<15)/11,(-8<<15)/11,(-6<<15)/11,
												(-4<<15)/11,(-2<<15)/11,0,
												(2<<15)/11,(4<<15)/11,(6<<15)/11,
												(8<<15)/11,(10<<15)/11 };

		decoder->fast_table_4[group_code][1]=mantissa_value[group_code/11];
		decoder->fast_table_4[group_code][0]=mantissa_value[group_code%11];
	}
}

float
ac3audio_get_float (ac3audio_decoder_t *decoder, bool dither,
						uint8 bap, ac3audio_get_float_state_t *state,
						uint8 exp)
{
	static const float exp_lut[ 25 ] =
	{
	    6.10351562500000000000000000e-05,
	    3.05175781250000000000000000e-05,
	    1.52587890625000000000000000e-05,
	    7.62939453125000000000000000e-06,
	    3.81469726562500000000000000e-06,
	    1.90734863281250000000000000e-06,
	    9.53674316406250000000000000e-07,
	    4.76837158203125000000000000e-07,
	    2.38418579101562500000000000e-07,
	    1.19209289550781250000000000e-07,
	    5.96046447753906250000000000e-08,
	    2.98023223876953125000000000e-08,
	    1.49011611938476562500000000e-08,
	    7.45058059692382812500000000e-09,
	    3.72529029846191406250000000e-09,
	    1.86264514923095703125000000e-09,
	    9.31322574615478515625000000e-10,
	    4.65661287307739257812500000e-10,
	    2.32830643653869628906250000e-10,
	    1.16415321826934814453125000e-10,
	    5.82076609134674072265625000e-11,
	    2.91038304567337036132812500e-11,
	    1.45519152283668518066406250e-11,
	    7.27595761418342590332031250e-12,
	    3.63797880709171295166015625e-12,
	};

	uint16 group_code;
				
	switch (bap)
	{
		case 0:
			return !dither ? 0.0f : (-1.0f+2.0f*((float)rand())/RAND_MAX)*0.707f*exp_lut[exp];
			
		case 1:
		{
			if (!state[0].queue_count)
			{
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_restore_state(decoder->bitstream);
#endif

				group_code=mpeg2bits_get(decoder->bitstream,5);

#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_save_state(decoder->bitstream);
#endif
				
				state[0].queue_count=3;
				state[0].queue=decoder->fast_table_1[group_code];
			}
				
			return state[0].queue[--state[0].queue_count]*exp_lut[exp];
		}
		
		case 2:
		{
			if (!state[1].queue_count)
			{	
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_restore_state(decoder->bitstream);
#endif

				group_code=mpeg2bits_get(decoder->bitstream,7);

#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_save_state(decoder->bitstream);
#endif
	
				state[1].queue_count=3;
				state[1].queue=decoder->fast_table_2[group_code];
			}
				
			return state[1].queue[--state[1].queue_count]*exp_lut[exp];
		}
		
		case 3:
		{
			static const float mantissa_value[7] = { (-6<<15)/7,(-4<<15)/7,(-2<<15)/7,0,
												(2<<15)/7,(4<<15)/7,(6<<15)/7 };
			
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_restore_state(decoder->bitstream);
#endif

			group_code=mpeg2bits_get(decoder->bitstream,3);
			
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_save_state(decoder->bitstream);
#endif

			return mantissa_value[group_code]*exp_lut[exp];
		}

		case 4:
		{
			if (!state[2].queue_count)
			{	
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_restore_state(decoder->bitstream);
#endif

				group_code=mpeg2bits_get(decoder->bitstream,7);

#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_save_state(decoder->bitstream);
#endif
	
				state[2].queue_count=2;
				state[2].queue=decoder->fast_table_4[group_code];
			}
				
			return state[2].queue[--state[2].queue_count]*exp_lut[exp];
		}
		
		case 5:
		{
			static const float mantissa_value[15] = { (-14<<15)/15,(-12<<15)/15,(-10<<15)/15,
												(-8<<15)/15,(-6<<15)/15,(-4<<15)/15,
												(-2<<15)/15,0,(2<<15)/15,(4<<15)/15,
												(6<<15)/15,(8<<15)/15,(10<<15)/15,
												(12<<15)/15,(14<<15)/15 };
			
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_restore_state(decoder->bitstream);
#endif

			group_code=mpeg2bits_get(decoder->bitstream,4);

#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_save_state(decoder->bitstream);
#endif
			
			return mantissa_value[group_code]*exp_lut[exp];
		}
		
		default:
		{
			// 6<=bap<=15
			
			static const uint8 qntztab[10] = { 5,6,7,8,9,10,11,12,14,16 };
				
			const uint8 k_bitcount=qntztab[bap-6];
			
#if defined(USE_MMX_BITSTREAM)
			mpeg2bits_restore_state(decoder->bitstream);
#endif

			group_code=mpeg2bits_get(decoder->bitstream,k_bitcount);

#if defined(USE_MMX_BITSTREAM)
			mpeg2bits_save_state(decoder->bitstream);
#endif
			
			return ((int16)(group_code<<(16-k_bitcount)))*exp_lut[exp];
		}
	}
}

void
ac3audio_clear_coeffs (ac3audio_decoder_t *decoder)
{
	memset(&decoder->coeffs,0,sizeof(ac3audio_coeff_t));
}

void
ac3audio_mantissa_unpack (ac3audio_decoder_t *decoder)
{
	ac3audio_get_float_state_t state[3];
	int32 bin,ch;
	
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_save_state(decoder->bitstream);
#endif

	ac3audio_clear_coeffs(decoder);
	
	memset(state,0,3*sizeof(ac3audio_get_float_state_t));
	
	ch=0;
	do
	{
		for (bin=0;bin<decoder->audblk.endmant[ch];++bin)
		{
			decoder->coeffs.fbw[ch][bin]=ac3audio_get_float (decoder,
												decoder->audblk.dithflag[ch],
												decoder->bap.fbw[ch][bin],
												state,
												decoder->exponents.fbw[ch][bin]);
		}
		++ch;
	}
	while (!decoder->audblk.chincpl[ch] && ch<decoder->bsi.nfchans);

	if (decoder->audblk.cplinu)
	{
		for (bin=decoder->audblk.cplstrtmant;bin<decoder->audblk.cplendmant;++bin)
		{
			decoder->coeffs.cpl[bin]=ac3audio_get_float(decoder,
											false,decoder->bap.cpl[bin],
											state,
											decoder->exponents.cpl[bin]);
		}
	}
	
	while (ch<decoder->bsi.nfchans)
	{
		for (bin=0;bin<decoder->audblk.endmant[ch];++bin)
		{
			decoder->coeffs.fbw[ch][bin]=ac3audio_get_float(decoder,
											decoder->audblk.dithflag[ch],
											decoder->bap.fbw[ch][bin],
											state,
											decoder->exponents.fbw[ch][bin]);
		}

		++ch;		
	}
	
	if (decoder->bsi.lfeon)
	{
		for (bin=0;bin<7;++bin)
		{
			decoder->coeffs.lfe[bin]=ac3audio_get_float(decoder,
											false,decoder->bap.lfe[bin],
											state,
											decoder->exponents.lfe[bin]);
		}		
	}

#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_restore_state(decoder->bitstream);
#endif
}
