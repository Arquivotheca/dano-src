#include "mpeg2video_extension.h"
#include "mpeg2video_seq_header.h"
#include "mpeg2video_picture.h"

#include <support/Debug.h>
#include <string.h>

void 
mpeg2video_parse_extension(mpeg2video_decoder_t *decoder)
{
	uint8 extension_code=mpeg2bits_get(decoder->bitstream,4);
	
	switch (extension_code)
	{
		case C_EXTENSION_STARTCODE_SEQUENCE:
		{
			parse_sequence_extension(decoder);
			break;
		}
			
		case C_EXTENSION_STARTCODE_QUANT_MATRIX:
		{
			if (mpeg2bits_get(decoder->bitstream,1))	// load_intra_quantizer_matrix
				mpeg2video_get_quant_matrix(decoder,decoder->sequence_info.quant_matrix[0]);
							
			memcpy(&decoder->sequence_info.quant_matrix[2],decoder->sequence_info.quant_matrix[0],64);
		
			if (mpeg2bits_get(decoder->bitstream,1))	// load_non_intra_quantizer_matrix
				mpeg2video_get_quant_matrix(decoder,decoder->sequence_info.quant_matrix[1]);
			
			memcpy(&decoder->sequence_info.quant_matrix[3],decoder->sequence_info.quant_matrix[1],64);

			if (mpeg2bits_get(decoder->bitstream,1))	// load_chroma_intra_quantizer_matrix
				mpeg2video_get_quant_matrix(decoder,decoder->sequence_info.quant_matrix[2]);
			
			if (mpeg2bits_get(decoder->bitstream,1))	// load_chroma_non_intra_quantizer_matrix
				mpeg2video_get_quant_matrix(decoder,decoder->sequence_info.quant_matrix[3]);
			
			break;
		}
		
		case C_EXTENSION_STARTCODE_PICTURE_CODING:
		{
			if (decoder->current_picture!=NULL)
				parse_picture_coding_extension(decoder);

			break;
		}

		case C_EXTENSION_STARTCODE_SEQUENCE_DISPLAY:
			break;
			
		case C_EXTENSION_STARTCODE_PICTURE_DISPLAY:
			break;
		
		default:
			TRESPASS();
			break;													
	}
}

void 
parse_sequence_extension (mpeg2video_decoder_t *decoder)
{
	uint8 hor_size_extension;
	uint8 ver_size_extension;
	uint16 bitrate_extension;
	DEBUG_ONLY(uint8 marker;)
	uint8 frame_rate_extension_n;
	uint8 frame_rate_extension_d;
	
	decoder->sequence_info.is_mpeg2=true;
	
	mpeg2bits_skip(decoder->bitstream,8);

	decoder->sequence_info.progressive=mpeg2bits_get(decoder->bitstream,1);

	decoder->sequence_info.chroma_format=(mpeg2_chroma_format_t)mpeg2bits_get(decoder->bitstream,2);	
	
	hor_size_extension=mpeg2bits_get(decoder->bitstream,2);
	
	decoder->sequence_info.horizontal_size|=((uint32)hor_size_extension)<<14;

	ver_size_extension=mpeg2bits_get(decoder->bitstream,2);

	decoder->sequence_info.vertical_size|=((uint32)ver_size_extension)<<14;

	bitrate_extension=mpeg2bits_get(decoder->bitstream,12);
	
	decoder->sequence_info.bit_rate|=((uint32)bitrate_extension)<<18;
	
	DEBUG_ONLY(marker=)mpeg2bits_get(decoder->bitstream,1);
	ASSERT(marker);
	
	mpeg2bits_skip(decoder->bitstream,9);
	
	frame_rate_extension_n=mpeg2bits_get(decoder->bitstream,2);
	frame_rate_extension_d=mpeg2bits_get(decoder->bitstream,5);
	
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_save_state(decoder->bitstream);
#endif

	decoder->sequence_info.frame_rate*=((float)frame_rate_extension_n+1)/((float)frame_rate_extension_d+1);

#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_restore_state(decoder->bitstream);
#endif
	
	update_derived_sequence_info(decoder);
}

void 
parse_picture_coding_extension (mpeg2video_decoder_t *decoder)
{
	int32 i,j;
	
	for (i=0;i<2;++i)
		for (j=0;j<2;++j)
			decoder->picture_info.f_code[i][j]=mpeg2bits_get(decoder->bitstream,4);
			
	decoder->picture_info.intra_dc_precision=mpeg2bits_get(decoder->bitstream,2);

	decoder->picture_info.picture_structure=(mpeg2_picture_structure_t)mpeg2bits_get(decoder->bitstream,2);

	decoder->picture_info.top_field_first=mpeg2bits_get(decoder->bitstream,1);

//	printf ("top_field_first      = %d\n",decoder->picture_info.top_field_first);	
	
	decoder->picture_info.frame_pred_frame_dct=mpeg2bits_get(decoder->bitstream,1);
	decoder->picture_info.concealment_motion_vectors=mpeg2bits_get(decoder->bitstream,1);
	decoder->picture_info.q_scale_type=mpeg2bits_get(decoder->bitstream,1);	
	
	decoder->picture_info.intra_vlc_format=mpeg2bits_get(decoder->bitstream,1);
	decoder->picture_info.alternate_scan=mpeg2bits_get(decoder->bitstream,1);
	decoder->picture_info.repeat_first_field=mpeg2bits_get(decoder->bitstream,1);

	decoder->current_picture->repeat_first_field=decoder->picture_info.repeat_first_field;

//	printf ("picture_structure    = %d\n",decoder->picture_info.picture_structure);	
//	printf ("repeat_first_field   = %d\n",decoder->picture_info.repeat_first_field);
//	printf ("progressive_sequence = %d\n",decoder->sequence_info.progressive);
	
	decoder->picture_info.chroma_420_type=mpeg2bits_get(decoder->bitstream,1);
	decoder->picture_info.progressive_frame=mpeg2bits_get(decoder->bitstream,1);
		
//	printf ("progressive_frame    = %d\n",decoder->picture_info.progressive_frame);

	if (mpeg2bits_get(decoder->bitstream,1))
		mpeg2bits_skip(decoder->bitstream,20);

	update_derived_picture_info(decoder);
}

