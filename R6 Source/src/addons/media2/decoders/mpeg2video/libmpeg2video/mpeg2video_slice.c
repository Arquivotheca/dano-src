#include "mpeg2video_slice.h"
#include "mpeg2video_motion_compensation.h"
#include "mpeg2video_motion_vectors.h"
#include "mpeg2video_block.h"

#include <mpeg2lib.h>

#include <support/Debug.h>

void 
mpeg2video_parse_slice(mpeg2video_decoder_t *decoder, uint8 slice_vertical_position)
{
	int32 i;
	uint32 macroblock_address;
	uint8 quantiser_scale_code;
	mpeg2_macroblock_t last_mb_direction;
	
	mpeg2video_reset_dct_predictors(decoder);
	mpeg2video_reset_motion_predictors(decoder);

	decoder->slice_info.mb_row=slice_vertical_position-1;
	
	if (decoder->sequence_info.vertical_size>2800)
	{
		uint8 slice_vertical_position_extension=mpeg2bits_get(decoder->bitstream,3);

		decoder->slice_info.mb_row+=(slice_vertical_position_extension<<7);
	}

	macroblock_address=decoder->slice_info.mb_row*decoder->sequence_info.mb_width;

	quantiser_scale_code=mpeg2bits_get(decoder->bitstream,5);

	if (decoder->sequence_info.is_mpeg2)
		decoder->slice_info.quantiser_scale=mpeg2video_get_quantiser_scale(quantiser_scale_code,decoder->picture_info.q_scale_type);
	else
		decoder->slice_info.quantiser_scale=quantiser_scale_code;

	if (mpeg2bits_get(decoder->bitstream,1))
	{
		mpeg2bits_skip(decoder->bitstream,8);
		while (mpeg2bits_get(decoder->bitstream,1))
			mpeg2bits_skip(decoder->bitstream,8);
	}

	last_mb_direction=(mpeg2_macroblock_t)0;
	
	do
	{
		mpeg2_macroblock_t mb_type;
		uint16 pattern_code;
		bool have_prediction;
		
		uint32 mb_addr_increment=get_mb_addr_increment(decoder);

		uint32 next_coded_macroblock_address=macroblock_address+mb_addr_increment-1;
		
		while (macroblock_address<next_coded_macroblock_address)
		{
			mpeg2_macroblock_t mb_type;
			
			decoder->slice_info.mb_row=macroblock_address/decoder->sequence_info.mb_width;
			decoder->slice_info.mb_column=macroblock_address % decoder->sequence_info.mb_width;

			mb_type=(mpeg2_macroblock_t)0;
			
			if (decoder->picture_info.picture_coding_type==P_PICTURE)
			{
				mb_type=MACROBLOCK_MOTION_FORWARD;
				
				if (decoder->picture_info.picture_structure==FRAME_PICTURE)
				{
					decoder->slice_info.fieldframe_motion_type=FRAME_BASED;
					decoder->slice_info.motion_vector_count=1;
					decoder->slice_info.motion_vector_format=FRAME_MOTION_VECTOR;
				}
				else
				{
					decoder->slice_info.fieldframe_motion_type=FIELD_BASED;
					decoder->slice_info.motion_vector_count=1;
					decoder->slice_info.motion_vector_format=FIELD_MOTION_VECTOR;
					// TODO predict from the same field as the current
				}
								
				mpeg2video_reset_motion_predictors(decoder);
			}
			else if (decoder->picture_info.picture_coding_type==B_PICTURE)
			{
				mb_type=last_mb_direction;

				if (decoder->picture_info.picture_structure==FRAME_PICTURE)
				{				
					decoder->slice_info.fieldframe_motion_type=FRAME_BASED;
					decoder->slice_info.motion_vector_count=1;
					decoder->slice_info.motion_vector_format=FRAME_MOTION_VECTOR;
				}
				else
				{
					decoder->slice_info.fieldframe_motion_type=FIELD_BASED;
					decoder->slice_info.motion_vector_count=1;
					decoder->slice_info.motion_vector_format=FIELD_MOTION_VECTOR;
					// TODO predict from the same field as the current
				}				
			}
			else
			{
//				PRINT(("ASSERTION WOULD FAIL\n"));
//				TRESPASS();
			}
			
			if (!(mb_type & MACROBLOCK_INTRA))
			{
				DEBUG_ONLY(bool have_prediction;)
					
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_save_state(decoder->bitstream);
#endif

				DEBUG_ONLY(have_prediction=)motion_compensation(decoder,mb_type);

#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_restore_state(decoder->bitstream);
#endif

				ASSERT(have_prediction);
			}
			else
				TRESPASS();
							
			++macroblock_address;
		}
		
		decoder->slice_info.mb_row=macroblock_address/decoder->sequence_info.mb_width;
		decoder->slice_info.mb_column=macroblock_address % decoder->sequence_info.mb_width;

		mb_type=(mpeg2_macroblock_t)get_macro_block_type(decoder);

		last_mb_direction=(mpeg2_macroblock_t)(mb_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD));
		
		if (mb_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD))
		{
			if (decoder->picture_info.picture_structure==FRAME_PICTURE)
			{
				if (decoder->picture_info.frame_pred_frame_dct)
					decoder->slice_info.fieldframe_motion_type=FRAME_BASED;
				else
					decoder->slice_info.fieldframe_motion_type=(mpeg2_fieldframe_motion_t)mpeg2bits_get(decoder->bitstream,2);
				
				decoder->slice_info.motion_vector_count=(decoder->slice_info.fieldframe_motion_type==FIELD_BASED) ? 2 : 1;
				decoder->slice_info.motion_vector_format=(decoder->slice_info.fieldframe_motion_type==FRAME_BASED) ? FRAME_MOTION_VECTOR : FIELD_MOTION_VECTOR;
			}
			else
			{
				decoder->slice_info.fieldframe_motion_type=(mpeg2_fieldframe_motion_t)mpeg2bits_get(decoder->bitstream,2);
				decoder->slice_info.motion_vector_count=(decoder->slice_info.fieldframe_motion_type==MC16x8) ? 2 : 1;
				decoder->slice_info.motion_vector_format=FIELD_MOTION_VECTOR;
			}						
		}
		else if ((mb_type & MACROBLOCK_INTRA) && decoder->picture_info.concealment_motion_vectors)
		{
			if (decoder->picture_info.picture_structure==FRAME_PICTURE)
			{			
				decoder->slice_info.fieldframe_motion_type=FRAME_BASED;
				decoder->slice_info.motion_vector_count=1;
				decoder->slice_info.motion_vector_format=FRAME_MOTION_VECTOR;
			}
			else
			{
				decoder->slice_info.fieldframe_motion_type=FIELD_BASED;
				decoder->slice_info.motion_vector_count=1;
				decoder->slice_info.motion_vector_format=FIELD_MOTION_VECTOR;
			}
		}
		
		if (decoder->picture_info.picture_structure==FRAME_PICTURE
			&& !decoder->picture_info.frame_pred_frame_dct
			&& (mb_type & (MACROBLOCK_INTRA|MACROBLOCK_PATTERN)))
		{
			decoder->slice_info.dct_type=mpeg2bits_get(decoder->bitstream,1);
		}
		else
		{
			if (decoder->picture_info.frame_pred_frame_dct)
				decoder->slice_info.dct_type=0;
			else
			{
				// dct_type is unused in any other case
			}
		}
		
		if (mb_type & MACROBLOCK_QUANT)
		{
			uint8 quantiser_scale_code=mpeg2bits_get(decoder->bitstream,5);
			
			if (decoder->sequence_info.is_mpeg2)
				decoder->slice_info.quantiser_scale=mpeg2video_get_quantiser_scale(quantiser_scale_code,decoder->picture_info.q_scale_type);
			else
				decoder->slice_info.quantiser_scale=quantiser_scale_code;
		}

		if ((mb_type & MACROBLOCK_MOTION_FORWARD)
			|| ((mb_type & MACROBLOCK_INTRA) && decoder->picture_info.concealment_motion_vectors))
		{
			parse_motion_vectors(decoder,0);
		}
		
		if (mb_type & MACROBLOCK_MOTION_BACKWARD)
		{
			parse_motion_vectors(decoder,1);		
		}
		
		if ((mb_type & MACROBLOCK_INTRA) && decoder->picture_info.concealment_motion_vectors)
		{
			DEBUG_ONLY(uint8 marker=)mpeg2bits_get(decoder->bitstream,1);
			ASSERT(marker);
		}
			
		pattern_code=get_pattern_code(decoder,mb_type);

		if (!(mb_type & MACROBLOCK_INTRA) || mb_addr_increment>1)
			mpeg2video_reset_dct_predictors(decoder);

		for (i=0;i<decoder->sequence_info.block_count;++i)
		{
			if (pattern_code & (1<<i))
				parse_block(decoder,i,mb_type);
		}

		if ((mb_type & MACROBLOCK_INTRA) && !decoder->picture_info.concealment_motion_vectors)
			mpeg2video_reset_motion_predictors(decoder);
			
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_save_state(decoder->bitstream);
#endif

		have_prediction=false;
		if (!(mb_type & MACROBLOCK_INTRA))
			have_prediction=motion_compensation(decoder,mb_type);

		for (i=0;i<decoder->sequence_info.block_count;++i)
		{
			if (pattern_code & (1<<i))
			{
				apply_block_idct(decoder,i);
				add_or_copy_block(decoder,i,have_prediction);
			}
		}

#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_restore_state(decoder->bitstream);
#endif

		++macroblock_address;
	}
	while (mpeg2bits_peek(decoder->bitstream,23)!=0);

	if (macroblock_address==decoder->sequence_info.num_macroblocks)
		output_current_picture(decoder);
}

void
mpeg2video_reset_dct_predictors (mpeg2video_decoder_t *decoder)
{
	static const int16 kDCTResetValue[4] = { 128,256,512,1024 };

	int32 i;
	
	for (i=0;i<3;++i)
		decoder->slice_info.dc_dct_pred[i]=kDCTResetValue[decoder->picture_info.intra_dc_precision];
}

void
mpeg2video_reset_motion_predictors (mpeg2video_decoder_t *decoder)
{
	int32 r,s,t;
	
	for (r=0;r<2;++r)
		for (s=0;s<2;++s)
			for (t=0;t<2;++t)
				decoder->slice_info.PMV[r][s][t]=0;				
}

uint8
mpeg2video_get_quantiser_scale (uint8 quantiser_scale_code, uint8 q_scale_type)
{
	static const uint8 kQuantiserScaleOdd[31] = { 1,2,3,4,5,6,7,8,10,12,14,16,18,20,
											22,24,28,32,36,40,44,48,52,56,64,72,
											80,88,96,104,112 };

	if (q_scale_type)
		return kQuantiserScaleOdd[quantiser_scale_code-1];
	
	return quantiser_scale_code<<1;
}

uint32
get_mb_addr_increment (mpeg2video_decoder_t *decoder)
{
	typedef struct vlc_
	{
	  uint8 val;
	  uint8 len;
	} vlc_t;

	static const vlc_t kMBAtab1[16] =
		{ {0xff,0}, {0xff,0}, {7,5}, {6,5}, {5,4}, {5,4}, {4,4}, {4,4},
		  {3,3}, {3,3}, {3,3}, {3,3}, {2,3}, {2,3}, {2,3}, {2,3}
		};
	
	static const vlc_t kMBAtab2[104] =
	{
	  {33,11}, {32,11}, {31,11}, {30,11}, {29,11}, {28,11}, {27,11}, {26,11},
	  {25,11}, {24,11}, {23,11}, {22,11}, {21,10}, {21,10}, {20,10}, {20,10},
	  {19,10}, {19,10}, {18,10}, {18,10}, {17,10}, {17,10}, {16,10}, {16,10},
	  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},  {15,8},
	  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},  {14,8},
	  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},  {13,8},
	  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},  {12,8},
	  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},  {11,8},
	  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},  {10,8},
	  {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
	  {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},   {9,7},
	  {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},
	  {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7},   {8,7}
	};

	uint32 mb_addr_increment=0;

	uint16 code;
	while ((code=mpeg2bits_peek(decoder->bitstream,11))==8 || (!decoder->sequence_info.is_mpeg2 && code==15))
	{
		mpeg2bits_skip(decoder->bitstream,11);
		
		if (code==8)
			mb_addr_increment+=33;
	}

  	if(code>=1024)
	{
		mpeg2bits_skip(decoder->bitstream,1);
    	return mb_addr_increment + 1;
  	}

  	if(code >= 128)
	{
    	code >>= 6;
    	mpeg2bits_skip(decoder->bitstream,kMBAtab1[code].len);
    	return mb_addr_increment + kMBAtab1[code].val;
  	}

  	code -= 24;
  	mpeg2bits_skip(decoder->bitstream,kMBAtab2[code].len);

  	return mb_addr_increment + kMBAtab2[code].val;
}

uint16
get_pattern_code (mpeg2video_decoder_t *decoder, mpeg2_macroblock_t mb_type)
{
	uint16 pc=0;
	
	if (mb_type & MACROBLOCK_INTRA)
		pc=(1<<12)-1;
		
	if (mb_type & MACROBLOCK_PATTERN)
	{
		int32 i;
		uint8 codec_block_pattern_420=get_coded_block_pattern(decoder);
	
		for (i=0;i<6;++i)
		{
			if (codec_block_pattern_420 & (1<<(5-i)))
				pc|=1<<i;
		}
		
		if (decoder->sequence_info.chroma_format==YUV422)
		{
			uint8 coded_block_pattern_1=mpeg2bits_get(decoder->bitstream,2);
			
			for (i=6;i<8;++i)
			{
				if (coded_block_pattern_1 & (1<<(7-i)))
					pc|=1<<i;
			}
		}
		else if (decoder->sequence_info.chroma_format==YUV444)
		{
			uint8 coded_block_pattern_2=mpeg2bits_get(decoder->bitstream,6);

			for (i=8;i<12;++i)
			{
				if (coded_block_pattern_2 & (1<<(11-i)))
					pc|=1<<i;
			}
		}	
	}
	
	return pc;
}

uint8
get_coded_block_pattern (mpeg2video_decoder_t *decoder)
{
	typedef struct vlc_
	{
	  uint8 val;
	  uint8 len;
	} vlc_t;

	static const vlc_t kCBPtab0[32] =
	{ {0xff,0}, {0xff,0}, {0xff,0}, {0xff,0},
	  {0xff,0}, {0xff,0}, {0xff,0}, {0xff,0},
	  {62,5}, {2,5},  {61,5}, {1,5},  {56,5}, {52,5}, {44,5}, {28,5},
	  {40,5}, {20,5}, {48,5}, {12,5}, {32,4}, {32,4}, {16,4}, {16,4},
	  {8,4},  {8,4},  {4,4},  {4,4},  {60,3}, {60,3}, {60,3}, {60,3}
	};
	
	static const vlc_t kCBPtab1[64] =
	{ {0xff,0}, {0xff,0}, {0xff,0}, {0xff,0},
	  {58,8}, {54,8}, {46,8}, {30,8},
	  {57,8}, {53,8}, {45,8}, {29,8}, {38,8}, {26,8}, {37,8}, {25,8},
	  {43,8}, {23,8}, {51,8}, {15,8}, {42,8}, {22,8}, {50,8}, {14,8},
	  {41,8}, {21,8}, {49,8}, {13,8}, {35,8}, {19,8}, {11,8}, {7,8},
	  {34,7}, {34,7}, {18,7}, {18,7}, {10,7}, {10,7}, {6,7},  {6,7},
	  {33,7}, {33,7}, {17,7}, {17,7}, {9,7},  {9,7},  {5,7},  {5,7},
	  {63,6}, {63,6}, {63,6}, {63,6}, {3,6},  {3,6},  {3,6},  {3,6},
	  {36,6}, {36,6}, {36,6}, {36,6}, {24,6}, {24,6}, {24,6}, {24,6}
	};
	
	static const vlc_t kCBPtab2[8] =
	{ {0xff,0}, {0,9}, {39,9}, {27,9}, {59,9}, {55,9}, {47,9}, {31,9}
	};	

  	uint16 code;

  	if ((code=mpeg2bits_peek(decoder->bitstream,9)) >= 128)
	{
    	code >>= 4;
    	mpeg2bits_skip(decoder->bitstream,kCBPtab0[code].len);
    	return kCBPtab0[code].val;
  	}

  	if(code >= 8)
	{
    	code >>= 1;
    	mpeg2bits_skip(decoder->bitstream,kCBPtab1[code].len);
    	return kCBPtab1[code].val;
  	}

  	if(code < 1)
  	{
  		TRESPASS();
  		return 0;
  	}	

  	mpeg2bits_skip(decoder->bitstream,kCBPtab2[code].len);
  	return kCBPtab2[code].val;
}

uint8
get_macro_block_type (mpeg2video_decoder_t *decoder)
{
	switch (decoder->picture_info.picture_coding_type)
	{
		case I_PICTURE:
			return get_i_picture_macro_block_type(decoder);
			
		case P_PICTURE:
			return get_p_picture_macro_block_type(decoder);

		case B_PICTURE:
			return get_b_picture_macro_block_type(decoder);

		default:
			TRESPASS();
			return 0;
	}
	
  	return 0;
}

uint8
get_i_picture_macro_block_type (mpeg2video_decoder_t *decoder)
{
	DEBUG_ONLY(uint8 one;)

	if (mpeg2bits_get(decoder->bitstream,1))
		return 2;
	
	DEBUG_ONLY(one=)mpeg2bits_get(decoder->bitstream,1);
	ASSERT(one);
	
	return 0x22;
}

uint8
get_p_picture_macro_block_type (mpeg2video_decoder_t *decoder)
{
	uint8 mb_type=0;
	
	if (mpeg2bits_get(decoder->bitstream,1))
		mb_type=20;
	else
	{
		if (mpeg2bits_get(decoder->bitstream,1))
			mb_type=4;
		else
		{
			if (mpeg2bits_get(decoder->bitstream,1))
				mb_type=16;
			else
			{
				switch (mpeg2bits_get(decoder->bitstream,2))
				{
					case 3:
						mb_type=2;
						break;
					
					case 2:
						mb_type=52;
						break;
					
					case 1:
						mb_type=36;
						break;
					
					case 0:
					{
						DEBUG_ONLY(uint8 one=)mpeg2bits_get(decoder->bitstream,1);
						ASSERT(one);
						
						mb_type=34;
						break;
					}
				}
			}
		}
	}
	
	return mb_type;
}

uint8
get_b_picture_macro_block_type (mpeg2video_decoder_t *decoder)
{
	uint8 mb_type=0;
	
	switch (mpeg2bits_get(decoder->bitstream,2))
	{
		case 2:
			mb_type=24;
			break;

		case 3:				
			mb_type=28;
			break;
		
		case 1:
		{
			if (!mpeg2bits_get(decoder->bitstream,1))
				mb_type=8;
			else
				mb_type=12;
				
			break;
		}
		
		case 0:
		{
			switch (mpeg2bits_get(decoder->bitstream,2))
			{
				case 2:
					mb_type=16;
					break;
				
				case 3:
					mb_type=20;
					break;
				
				case 1:
				{
					if (mpeg2bits_get(decoder->bitstream,1))
						mb_type=2;
					else
						mb_type=60;
						
					break;
				}
				
				case 0:
				{
					switch (mpeg2bits_get(decoder->bitstream,2))
					{
						case 3:
							mb_type=52;
							break;
						
						case 2:
							mb_type=44;
							break;
						
						case 1:
							mb_type=34;
							break;
						
						case 0:
							TRESPASS();
							break;
					}
					
					break;
				}
			}
			
			break;
		}
	}
	
	return mb_type;
}

void
apply_block_idct (mpeg2video_decoder_t *decoder, uint8 block_num)
{
	int16 *block=decoder->block[block_num];

	if (decoder->block_sparse_index[block_num]!=0)
		idct(block);
	else // compute sparse ifft
	{
		int32 i;
		
		const int16 c=block[0]/8;
		for (i=0;i<64;++i)
			block[i]=c;
	}
}

void
output_current_picture (mpeg2video_decoder_t *decoder)
{
	if (decoder->picture_info.picture_coding_type==B_PICTURE)
		output_picture(decoder,decoder->current_picture,false);
	else
	{
		if (decoder->ref_picture[0]!=NULL)
		{
			decoder->ref_picture[0]->acquire_ref(decoder->ref_picture[0]);
			output_picture(decoder,decoder->ref_picture[0],false);
		}
		
		decoder->ref_picture[1]=decoder->current_picture;
	}

	decoder->current_picture=NULL;
}
