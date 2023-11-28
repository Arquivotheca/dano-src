#include "mpeg2video_seq_header.h"

#include <support/Debug.h>
#include <string.h>

static const uint8
kDefaultQuantMatrixIntra[64] = { 8,16,19,22,26,27,29,34,
								16,16,22,24,27,29,34,37,
								19,22,26,27,29,34,34,38,
								22,22,26,27,29,34,37,40,
								22,26,27,29,32,35,40,48,
								26,27,29,32,35,40,48,58,
								26,27,29,34,38,46,56,69,
								27,29,35,38,46,56,69,83 };

static const uint8
kDefaultQuantMatrixNonIntra[64] = { 16,16,16,16,16,16,16,16,
									16,16,16,16,16,16,16,16,
									16,16,16,16,16,16,16,16,
									16,16,16,16,16,16,16,16,
									16,16,16,16,16,16,16,16,
									16,16,16,16,16,16,16,16,
									16,16,16,16,16,16,16,16,
									16,16,16,16,16,16,16,16 };														

const uint8
kZigZagOffsetMMX[2][64] = {
{
	0, 4, 8, 16, 12, 1, 5, 9, 
	20, 24, 32, 28, 17, 13, 2, 6, 
	10, 21, 25, 36, 40, 48, 44, 33, 
	29, 18, 14, 3, 7, 11, 22, 26, 
	37, 41, 52, 56, 60, 49, 45, 34, 
	30, 19, 15, 23, 27, 38, 42, 53, 
	57, 61, 50, 46, 35, 31, 39, 43, 
	54, 58, 62, 51, 47, 55, 59, 63
},
{
	0, 8, 16, 24, 4, 12, 1, 9, 
	20, 28, 32, 40, 48, 56, 60, 52, 
	44, 36, 25, 17, 5, 13, 2, 10, 
	21, 29, 33, 41, 49, 57, 37, 45, 
	53, 61, 18, 26, 6, 14, 3, 11, 
	22, 30, 34, 42, 50, 58, 38, 46, 
	54, 62, 19, 27, 7, 15, 23, 31, 
	35, 43, 51, 59, 39, 47, 55, 63
}

};

const uint8
kZigZagOffset[2][64] = { { 0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
							12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
							35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
							58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63 },
						 { 0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
							41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
							51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
							53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63 } };

void 
mpeg2video_parse_sequence_header (mpeg2video_decoder_t *decoder)
{
	uint8 code;
	DEBUG_ONLY(uint8 marker;)
	
	decoder->sequence_info.is_mpeg2=false;

	decoder->sequence_info.horizontal_size=mpeg2bits_get(decoder->bitstream,12);
	
	decoder->sequence_info.vertical_size=mpeg2bits_get(decoder->bitstream,12);

	mpeg2bits_skip(decoder->bitstream,4);

	code=mpeg2bits_get(decoder->bitstream,4);

#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_save_state(decoder->bitstream);
#endif

	decoder->sequence_info.frame_rate=mpeg2video_get_frame_rate(code);
		
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_restore_state(decoder->bitstream);
#endif

	decoder->sequence_info.bit_rate=mpeg2bits_get(decoder->bitstream,18);

	DEBUG_ONLY(marker=)mpeg2bits_get(decoder->bitstream,1);
	ASSERT(marker);
	
	mpeg2bits_skip(decoder->bitstream,11);

	if (mpeg2bits_get(decoder->bitstream,1))	// load_intra_quantizer_matrix
		mpeg2video_get_quant_matrix(decoder,decoder->sequence_info.quant_matrix[0]);
	else
		mpeg2video_load_default_quant_matrix(decoder->sequence_info.quant_matrix[0],kDefaultQuantMatrixIntra);
		
	memcpy(&decoder->sequence_info.quant_matrix[2],decoder->sequence_info.quant_matrix[0],64);

	if (mpeg2bits_get(decoder->bitstream,1))	// load_non_intra_quantizer_matrix
		mpeg2video_get_quant_matrix(decoder,decoder->sequence_info.quant_matrix[1]);
	else
		mpeg2video_load_default_quant_matrix(decoder->sequence_info.quant_matrix[1],kDefaultQuantMatrixNonIntra);
		
	memcpy(&decoder->sequence_info.quant_matrix[3],decoder->sequence_info.quant_matrix[1],64);

	decoder->sequence_info.progressive=true;
	decoder->sequence_info.chroma_format=YUV420;	

	update_derived_sequence_info(decoder);
}

float 
mpeg2video_get_frame_rate(uint8 code)
{
	static const float kFrameRateTable[] = { 24000.0f/1001.0f,24.0f,25.0f,30000.0f/1001.0f,
												30.0f,50.0f,60000.0f/1001.0f,60.0f };

	ASSERT(code>=1 && code<=8);
	
	return kFrameRateTable[code-1];
}

void
mpeg2video_get_quant_matrix (mpeg2video_decoder_t *decoder, uint8 *qmatrix)
{
	int32 i;
	
	for (i=0;i<64;++i)
		qmatrix[kZigZagOffsetMMX[0][i]]=mpeg2bits_get(decoder->bitstream,8);
}

void
mpeg2video_load_default_quant_matrix (uint8 *qmatrix, const uint8 *default_matrix)
{
	int32 i;
	
	for (i=0;i<64;++i)
	{
		qmatrix[kZigZagOffsetMMX[0][i]]=default_matrix[kZigZagOffset[0][i]];
	}
}

void
update_derived_sequence_info (mpeg2video_decoder_t *decoder)
{
	decoder->sequence_info.mb_width=(decoder->sequence_info.horizontal_size+15)/16;
	
	if (decoder->sequence_info.progressive)
		decoder->sequence_info.mb_height=(decoder->sequence_info.vertical_size+15)/16;
	else
		decoder->sequence_info.mb_height=(decoder->sequence_info.vertical_size+31)/32;	// for fields

	switch (decoder->sequence_info.chroma_format)
	{
		case YUV420:
			decoder->sequence_info.block_count=6;
			break;
		
		case YUV422:
			decoder->sequence_info.block_count=8;
			break;
		
		case YUV444:
			decoder->sequence_info.block_count=12;
			break;
	}

	decoder->sequence_info.num_macroblocks=decoder->sequence_info.mb_width*decoder->sequence_info.mb_height;
	
	if (!decoder->sequence_info.progressive)
		decoder->sequence_info.num_macroblocks*=2;
}
