#include "ac3audio.h"
#include "ac3audio_dec_mantissas.h"
#include "ac3audio_imdct.h"
#include "ac3audio_bsi.h"
#include "ac3audio_dec_exponents.h"
#include "ac3audio_alloc_bits.h"
#include "ac3audio_audblk.h"
#include "ac3audio_uncouple.h"
#include "ac3audio_rematrix.h"

#include <support/Debug.h>

#include <malloc.h>
#include <string.h>

ac3audio_decoder_t *
ac3audio_create (output_audio_coding_mode_t output_coding_mode,
					acquire_buffer_func acquire_output_buffer,
					send_buffer_func send_output_buffer, void *cookie)
{
	static const float kWindowCoeffsConstants[256] =
	{
		0.00014, 0.00024, 0.00037, 0.00051, 0.00067, 0.00086, 0.00107, 0.00130,
		0.00157, 0.00187, 0.00220, 0.00256, 0.00297, 0.00341, 0.00390, 0.00443,
		0.00501, 0.00564, 0.00632, 0.00706, 0.00785, 0.00871, 0.00962, 0.01061,
		0.01166, 0.01279, 0.01399, 0.01526, 0.01662, 0.01806, 0.01959, 0.02121,
		0.02292, 0.02472, 0.02662, 0.02863, 0.03073, 0.03294, 0.03527, 0.03770,
		0.04025, 0.04292, 0.04571, 0.04862, 0.05165, 0.05481, 0.05810, 0.06153,
		0.06508, 0.06878, 0.07261, 0.07658, 0.08069, 0.08495, 0.08935, 0.09389,
		0.09859, 0.10343, 0.10842, 0.11356, 0.11885, 0.12429, 0.12988, 0.13563,
		0.14152, 0.14757, 0.15376, 0.16011, 0.16661, 0.17325, 0.18005, 0.18699,
		0.19407, 0.20130, 0.20867, 0.21618, 0.22382, 0.23161, 0.23952, 0.24757,
		0.25574, 0.26404, 0.27246, 0.28100, 0.28965, 0.29841, 0.30729, 0.31626,
		0.32533, 0.33450, 0.34376, 0.35311, 0.36253, 0.37204, 0.38161, 0.39126,
		0.40096, 0.41072, 0.42054, 0.43040, 0.44030, 0.45023, 0.46020, 0.47019,
		0.48020, 0.49022, 0.50025, 0.51028, 0.52031, 0.53033, 0.54033, 0.55031,
		0.56026, 0.57019, 0.58007, 0.58991, 0.59970, 0.60944, 0.61912, 0.62873,
		0.63827, 0.64774, 0.65713, 0.66643, 0.67564, 0.68476, 0.69377, 0.70269,
		0.71150, 0.72019, 0.72877, 0.73723, 0.74557, 0.75378, 0.76186, 0.76981,
		0.77762, 0.78530, 0.79283, 0.80022, 0.80747, 0.81457, 0.82151, 0.82831,
		0.83496, 0.84145, 0.84779, 0.85398, 0.86001, 0.86588, 0.87160, 0.87716,
		0.88257, 0.88782, 0.89291, 0.89785, 0.90264, 0.90728, 0.91176, 0.91610,
		0.92028, 0.92432, 0.92822, 0.93197, 0.93558, 0.93906, 0.94240, 0.94560,
		0.94867, 0.95162, 0.95444, 0.95713, 0.95971, 0.96217, 0.96451, 0.96674,
		0.96887, 0.97089, 0.97281, 0.97463, 0.97635, 0.97799, 0.97953, 0.98099,
		0.98236, 0.98366, 0.98488, 0.98602, 0.98710, 0.98811, 0.98905, 0.98994,
		0.99076, 0.99153, 0.99225, 0.99291, 0.99353, 0.99411, 0.99464, 0.99513,
		0.99558, 0.99600, 0.99639, 0.99674, 0.99706, 0.99736, 0.99763, 0.99788,
		0.99811, 0.99831, 0.99850, 0.99867, 0.99882, 0.99895, 0.99908, 0.99919,
		0.99929, 0.99938, 0.99946, 0.99953, 0.99959, 0.99965, 0.99969, 0.99974,
		0.99978, 0.99981, 0.99984, 0.99986, 0.99988, 0.99990, 0.99992, 0.99993,
		0.99994, 0.99995, 0.99996, 0.99997, 0.99998, 0.99998, 0.99998, 0.99999,
		0.99999, 0.99999, 0.99999, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000,
		1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000
	};

	ac3audio_decoder_t *decoder;
	
	decoder=(ac3audio_decoder_t *)malloc(sizeof(ac3audio_decoder_t));

	decoder->output_coding_mode=output_coding_mode;
	decoder->first_mono_channel_selected=true;
	
	decoder->acquire_output_buffer=acquire_output_buffer;
	decoder->send_output_buffer=send_output_buffer;
	decoder->cookie=cookie;
	
	decoder->window_coeffs=(float *)memalign(16,sizeof(kWindowCoeffsConstants));
	memcpy(decoder->window_coeffs,kWindowCoeffsConstants,sizeof(kWindowCoeffsConstants));

	decoder->samples=(ac3audio_sample_t *)memalign(16,sizeof(ac3audio_sample_t));
		
	ac3audio_init_mantissa_decoding(decoder);
	ac3audio_imdct_init(decoder);
	
	decoder->bitstream=mpeg2bits_create();
	
	decoder->tid=spawn_thread(ac3audio_threadfunc,"ac3audio_thread",
								B_NORMAL_PRIORITY,decoder);

	resume_thread(decoder->tid);
									
	return decoder;
}

void 
ac3audio_destroy (ac3audio_decoder_t *decoder)
{
	status_t dummy;
	
	mpeg2bits_enqueue_code(decoder->bitstream,AC3AUDIO_SHUTDOWN);
	
	while (wait_for_thread(decoder->tid,&dummy)==B_INTERRUPTED)
		;
	
	mpeg2bits_destroy(decoder->bitstream);
	
	ac3audio_imdct_destroy(decoder);

	free(decoder->samples);
	
	free(decoder->window_coeffs);
	free(decoder);
}

void 
ac3audio_decode (ac3audio_decoder_t *decoder, buffer_t *buffer)
{
	mpeg2bits_enqueue_buffer(decoder->bitstream,buffer);
}

void
ac3audio_flush (ac3audio_decoder_t *decoder)
{
	mpeg2bits_enqueue_code(decoder->bitstream,AC3AUDIO_FLUSH);
}

int32 
ac3audio_threadfunc(void *param)
{
	ac3audio_decoder_t *decoder=(ac3audio_decoder_t *)param;
	status_t result;
	buffer_t *input_buffer=NULL;
	buffer_t *output_buffer=NULL;
	bool abort;
	int32 i;
	int16 *output_ptr;
	int32 total_channel_count;
	
	while (true)
	{
		int val;
		
		val=setjmp(decoder->bitstream->setjmp_cookie);
		
		if (val==AC3AUDIO_SHUTDOWN)
			break;
		else if (val==AC3AUDIO_FLUSH)
		{
			if (output_buffer)
			{
				output_buffer->release_ref(output_buffer);
				output_buffer=NULL;
			}
			
			mpeg2bits_clear(decoder->bitstream);
			
			continue;
		}
		
		mpeg2bits_skip_to_byte_boundary(decoder->bitstream);
		
		while (mpeg2bits_peek(decoder->bitstream,16)!=0x0b77)
			mpeg2bits_skip(decoder->bitstream,8);

		result=decoder->acquire_output_buffer(decoder->cookie,&output_buffer);
	
		if (result<B_OK)
			continue;

		input_buffer=decoder->bitstream->current_buffer;
		
		if (!input_buffer->tagged)
		{
			input_buffer->tagged=true;
			
			if (input_buffer->has_start_time)
			{
				output_buffer->has_start_time=true;
				output_buffer->start_time=input_buffer->start_time;
			}
		}
		
		mpeg2bits_skip(decoder->bitstream,16); // skip 0x0b77
		
		decoder->syncinfo.crc1=mpeg2bits_get(decoder->bitstream,16);
		decoder->syncinfo.fscod=mpeg2bits_get(decoder->bitstream,2);
		decoder->syncinfo.frmsizecod=mpeg2bits_get(decoder->bitstream,6);

		ac3audio_parse_bsi(decoder);
		
		switch (decoder->output_coding_mode)
		{
			case C_PASS_THROUGH:
				total_channel_count=decoder->bsi.nfchans+decoder->bsi.lfeon ? 1 : 0;
				break;
			
			case C_CONVENTIONAL_STEREO:
			case C_MATRIX_SURROUND_STEREO:
				total_channel_count=2;
				break;
			
			case C_MONO:
				total_channel_count=1;
				break;
	
			default:
				TRESPASS();
				break;
		}
			
		memset(&decoder->audblk,0,sizeof(ac3audio_audblk_t));
		memset(&decoder->exponents,0,sizeof(ac3audio_exponent_t));		

		ac3audio_clear_bap(decoder);

		output_ptr=(int16 *)output_buffer->data;
		
		abort=false;
		for (i=0;!abort && i<6;++i)
		{
			status_t result=ac3audio_parse_audblk(decoder,i==0);
			
			if (result<B_OK)
				abort=true;
			else
			{
				ac3audio_decode_exponents(decoder);
				ac3audio_alloc_bits(decoder);
				ac3audio_mantissa_unpack(decoder);
	
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_save_state(decoder->bitstream);
#endif
	
				ac3audio_uncouple(decoder);
				
				if (decoder->bsi.acmod==2)
					ac3audio_rematrix(decoder);
	
				ac3audio_imdct(decoder,output_ptr);
	
#if defined(USE_MMX_BITSTREAM)
				mpeg2bits_restore_state(decoder->bitstream);
#endif
				
				output_ptr+=256*total_channel_count;
			}
		}
		
		if (!abort)
		{
#if defined(USE_MMX_BITSTREAM)
			mpeg2bits_save_state(decoder->bitstream);
#endif
	
			if (decoder->send_output_buffer(decoder->cookie,output_buffer)<B_OK)
				output_buffer->release_ref(output_buffer);
	
#if defined(USE_MMX_BITSTREAM)
			mpeg2bits_restore_state(decoder->bitstream);
#endif		
		}
		else
			output_buffer->release_ref(output_buffer);
		
		output_buffer=NULL;
	}
	
	if (output_buffer)
	{
		output_buffer->release_ref(output_buffer);
		output_buffer->release_ref(output_buffer);
		output_buffer=NULL;
	}
			
	return 0;
}

