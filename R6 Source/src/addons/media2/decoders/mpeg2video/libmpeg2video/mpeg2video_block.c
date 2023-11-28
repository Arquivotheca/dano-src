#include "mpeg2video_block.h"
#include "mpeg2video_seq_header.h"

#include <support/Debug.h>
#include <string.h>

typedef struct dct_vlc_
{
  uint8 run;
  int16 level;
  uint8 len;
} dct_vlc_t;

static const dct_vlc_t
kDCTtabfirst[12] =
{
  {0,2,4}, {2,1,4}, {1,1,3}, {1,1,3},
  {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1},
  {0,1,1}, {0,1,1}, {0,1,1}, {0,1,1}
};

static const dct_vlc_t
kDCTtabnext[12] =
{
  {0,2,4},  {2,1,4},  {1,1,3},  {1,1,3},
  {64,0,2}, {64,0,2}, {64,0,2}, {64,0,2}, /* EOB */
  {0,1,2},  {0,1,2},  {0,1,2},  {0,1,2}
};

static const dct_vlc_t
kDCTtab0[60] =
{
  {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
  {2,2,7}, {2,2,7}, {9,1,7}, {9,1,7},
  {0,4,7}, {0,4,7}, {8,1,7}, {8,1,7},
  {7,1,6}, {7,1,6}, {7,1,6}, {7,1,6},
  {6,1,6}, {6,1,6}, {6,1,6}, {6,1,6},
  {1,2,6}, {1,2,6}, {1,2,6}, {1,2,6},
  {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
  {13,1,8}, {0,6,8}, {12,1,8}, {11,1,8},
  {3,2,8}, {1,3,8}, {0,5,8}, {10,1,8},
  {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
  {0,3,5}, {0,3,5}, {0,3,5}, {0,3,5},
  {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
  {4,1,5}, {4,1,5}, {4,1,5}, {4,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5}
};

static const dct_vlc_t
kDCTtab0a[252] =
{
  {65,0,6}, {65,0,6}, {65,0,6}, {65,0,6}, /* Escape */
  {7,1,7}, {7,1,7}, {8,1,7}, {8,1,7},
  {6,1,7}, {6,1,7}, {2,2,7}, {2,2,7},
  {0,7,6}, {0,7,6}, {0,7,6}, {0,7,6},
  {0,6,6}, {0,6,6}, {0,6,6}, {0,6,6},
  {4,1,6}, {4,1,6}, {4,1,6}, {4,1,6},
  {5,1,6}, {5,1,6}, {5,1,6}, {5,1,6},
  {1,5,8}, {11,1,8}, {0,11,8}, {0,10,8},
  {13,1,8}, {12,1,8}, {3,2,8}, {1,4,8},
  {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
  {2,1,5}, {2,1,5}, {2,1,5}, {2,1,5},
  {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
  {1,2,5}, {1,2,5}, {1,2,5}, {1,2,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {3,1,5}, {3,1,5}, {3,1,5}, {3,1,5},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {1,1,3}, {1,1,3}, {1,1,3}, {1,1,3},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4}, /* EOB */
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {64,0,4}, {64,0,4}, {64,0,4}, {64,0,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,3,4}, {0,3,4}, {0,3,4}, {0,3,4},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,1,2}, {0,1,2}, {0,1,2}, {0,1,2},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,2,3}, {0,2,3}, {0,2,3}, {0,2,3},
  {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
  {0,4,5}, {0,4,5}, {0,4,5}, {0,4,5},
  {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
  {0,5,5}, {0,5,5}, {0,5,5}, {0,5,5},
  {9,1,7}, {9,1,7}, {1,3,7}, {1,3,7},
  {10,1,7}, {10,1,7}, {0,8,7}, {0,8,7},
  {0,9,7}, {0,9,7}, {0,12,8}, {0,13,8},
  {2,3,8}, {4,2,8}, {0,14,8}, {0,15,8}
};

static const dct_vlc_t
kDCTtab1[8] =
{
  {16,1,10}, {5,2,10}, {0,7,10}, {2,3,10},
  {1,4,10}, {15,1,10}, {14,1,10}, {4,2,10}
};

static const dct_vlc_t
kDCTtab1a[8] =
{
  {5,2,9}, {5,2,9}, {14,1,9}, {14,1,9},
  {2,4,10}, {16,1,10}, {15,1,9}, {15,1,9}
};

static const dct_vlc_t
kDCTtab2[16] =
{
  {0,11,12}, {8,2,12}, {4,3,12}, {0,10,12},
  {2,4,12}, {7,2,12}, {21,1,12}, {20,1,12},
  {0,9,12}, {19,1,12}, {18,1,12}, {1,5,12},
  {3,3,12}, {0,8,12}, {6,2,12}, {17,1,12}
};

static const dct_vlc_t
kDCTtab3[16] =
{
  {10,2,13}, {9,2,13}, {5,3,13}, {3,4,13},
  {2,5,13}, {1,7,13}, {1,6,13}, {0,15,13},
  {0,14,13}, {0,13,13}, {0,12,13}, {26,1,13},
  {25,1,13}, {24,1,13}, {23,1,13}, {22,1,13}
};

static const dct_vlc_t
kDCTtab4[16] =
{
  {0,31,14}, {0,30,14}, {0,29,14}, {0,28,14},
  {0,27,14}, {0,26,14}, {0,25,14}, {0,24,14},
  {0,23,14}, {0,22,14}, {0,21,14}, {0,20,14},
  {0,19,14}, {0,18,14}, {0,17,14}, {0,16,14}
};

static const dct_vlc_t
kDCTtab5[16] =
{
  {0,40,15}, {0,39,15}, {0,38,15}, {0,37,15},
  {0,36,15}, {0,35,15}, {0,34,15}, {0,33,15},
  {0,32,15}, {1,14,15}, {1,13,15}, {1,12,15},
  {1,11,15}, {1,10,15}, {1,9,15}, {1,8,15}
};

static const dct_vlc_t
kDCTtab6[16] =
{
  {1,18,16}, {1,17,16}, {1,16,16}, {1,15,16},
  {6,3,16}, {16,2,16}, {15,2,16}, {14,2,16},
  {13,2,16}, {12,2,16}, {11,2,16}, {31,1,16},
  {30,1,16}, {29,1,16}, {28,1,16}, {27,1,16}
};

inline int32 
get_mpeg2_dct_coeff_intra (mpeg2video_decoder_t *decoder)
{
	const dct_vlc_t *tab; 

   	uint16 code = mpeg2bits_peek(decoder->bitstream,16);

DEBUG_ONLY(
	if (!(code & ~15))
	{
		TRESPASS();
		return 64;
	}
	else )if (!(code & ~31))
   		tab = &kDCTtab6[code - 16];
	else if (!(code & ~63))
   		tab = &kDCTtab5[(code >> 1) - 16];
	else if (!(code & ~127))
   		tab = &kDCTtab4[(code >> 2) - 16];
	else if (!(code & ~255))
   		tab = &kDCTtab3[(code >> 3) - 16];
	else if (!(code & ~511))
   		tab = &kDCTtab2[(code >> 4) - 16];
	else if (!(code & ~1023))
   	{
   		if(decoder->picture_info.intra_vlc_format)     
	  	  	tab = &kDCTtab1a[(code >> 6) - 8];
   		else              
			tab = &kDCTtab1[(code >> 6) - 8];
	}
	else if (!(code & ~16383) || decoder->picture_info.intra_vlc_format)
	{
   		if(decoder->picture_info.intra_vlc_format) 
			tab = &kDCTtab0a[(code >> 8) - 4];
   		else 
			tab = &kDCTtab0[(code >> 8) - 4];
   	}
	else
		tab = &kDCTtabnext[(code >> 12) - 4];
	
	mpeg2bits_skip(decoder->bitstream,tab->len);
	
   	if(tab->run == 65)
	{
/* escape */
		const uint8 run=mpeg2bits_get(decoder->bitstream,6);
		
		int16 level=mpeg2bits_get(decoder->bitstream,12);
		
		ASSERT(level & 2047);
					
		if (level>=2048)
			level-=4096;

		return level<<16 | run;
   	}
   	else if (tab->run==64)
   		return 0;
  	else 
	{
		const int16 level=mpeg2bits_get(decoder->bitstream,1) ? -tab->level : tab->level;		
		return level<<16 | tab->run;
   	}
}

inline int32 
get_mpeg2_dct_coeff_non_intra (mpeg2video_decoder_t *decoder, bool first)
{
	const dct_vlc_t *tab; 

   	uint16 code = mpeg2bits_peek(decoder->bitstream,16);

	if (!(code & ~15))
	{
		TRESPASS();
		return 0;
	}
	else if (!(code & ~31))
   		tab = &kDCTtab6[code - 16];
	else if (!(code & ~63))
   		tab = &kDCTtab5[(code >> 1) - 16];
	else if (!(code & ~127))
   		tab = &kDCTtab4[(code >> 2) - 16];
	else if (!(code & ~255))
   		tab = &kDCTtab3[(code >> 3) - 16];
	else if (!(code & ~511))
   		tab = &kDCTtab2[(code >> 4) - 16];
	else if (!(code & ~1023))
   		tab = &kDCTtab1[(code >> 6) - 8];
	else if (!(code & ~16383))
   		tab = &kDCTtab0[(code >> 8) - 4];
	else
	{
		code=(code>>12)-4;
		
   	  	tab = first ? &kDCTtabfirst[code] : &kDCTtabnext[code];
   	}		
	
	mpeg2bits_skip(decoder->bitstream,tab->len);
	
   	if(tab->run == 65)
	{
/* escape */
		const uint8 run=mpeg2bits_get(decoder->bitstream,6);
		
		int16 level=mpeg2bits_get(decoder->bitstream,12);
		
		ASSERT(level & 2047);
					
		if (level>=2048)
			level-=4096;

		return level<<16 | run;
   	}
   	else if (tab->run==64)
   		return 0;
   	else 
	{
		const int16 level=mpeg2bits_get(decoder->bitstream,1) ? -tab->level : tab->level;		
		return level<<16 | tab->run;
   	}
}

uint8
get_dct_dc_size_chrominance (mpeg2video_decoder_t *decoder)
{
	switch (mpeg2bits_get(decoder->bitstream,2))
	{
		case 0:
			return 0;
		
		case 1:
			return 1;
		
		case 2:
			return 2;
		
		case 3:
		{
			if (!mpeg2bits_get(decoder->bitstream,1))
				return 3;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 4;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 5;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 6;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 7;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 8;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 9;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 10;
				
			return 11;
		}
	}
	
	TRESPASS();
	return 0;
}

uint8
get_dct_dc_size_luminance (mpeg2video_decoder_t *decoder)
{
	switch (mpeg2bits_get(decoder->bitstream,2))
	{
		case 0:
			return 1;
		
		case 1:
			return 2;
		
		case 2:
			return mpeg2bits_get(decoder->bitstream,1) ? 3 : 0;
		
		case 3:
		{
			if (!mpeg2bits_get(decoder->bitstream,1))
				return 4;
			
			if (!mpeg2bits_get(decoder->bitstream,1))
				return 5;
			
			if (!mpeg2bits_get(decoder->bitstream,1))
				return 6;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 7;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 8;
			
			if (!mpeg2bits_get(decoder->bitstream,1))
				return 9;

			if (!mpeg2bits_get(decoder->bitstream,1))
				return 10;
				
			return 11;
		}
	}

	TRESPASS();
	return 0;
}

void 
parse_block (mpeg2video_decoder_t *decoder, uint8 block_num, mpeg2_macroblock_t mb_type)
{
	uint8 cc=(block_num<4) ? 0 : (block_num&1)+1;

	int16 *block=decoder->block[block_num];
	memset(block,0,64*sizeof(int16));
	
	if (!decoder->sequence_info.is_mpeg2)
	{
		if (mb_type & MACROBLOCK_INTRA)
			decoder->block_sparse_index[block_num]=parse_mpeg1_intra_block(decoder,block,cc);
		else
			decoder->block_sparse_index[block_num]=parse_mpeg1_non_intra_block(decoder,block,cc);
	}
	else
	{
		if (mb_type & MACROBLOCK_INTRA)
			decoder->block_sparse_index[block_num]=parse_mpeg2_intra_block(decoder,block,cc);
		else
			decoder->block_sparse_index[block_num]=parse_mpeg2_non_intra_block(decoder,block,cc);
	}
}

int8 
parse_mpeg2_intra_block (mpeg2video_decoder_t *decoder, int16 *block, uint8 cc)
{
	uint8 dct_dc_size=(cc==0) ? get_dct_dc_size_luminance(decoder)
							  : get_dct_dc_size_chrominance(decoder);
	
	uint16 dct_diff;
	int16 val;
	uint8 parity;
	int32 code;
	
	uint8 n=1;
	
	const uint8 *qmatrix=(decoder->sequence_info.chroma_format==YUV420 || cc==0)
							? decoder->sequence_info.quant_matrix[0]
							: decoder->sequence_info.quant_matrix[2];

	const uint8 *zigzag=kZigZagOffsetMMX[decoder->picture_info.alternate_scan];
	
	if (dct_dc_size==0)
		dct_diff=0;
	else
	{
		uint16 dct_dc_differential=mpeg2bits_get(decoder->bitstream,dct_dc_size);
	
		uint16 half_range=1<<(dct_dc_size-1);
		
		if (dct_dc_differential>=half_range)
			dct_diff=dct_dc_differential;
		else
			dct_diff=(dct_dc_differential+1)-(half_range*2);
	}
	
	decoder->slice_info.dc_dct_pred[cc]+=dct_diff;
	
	val=decoder->slice_info.dc_dct_pred[cc]<<(3-decoder->picture_info.intra_dc_precision);

	block[0]=val;
	
	parity=val;

	if ((code=get_mpeg2_dct_coeff_intra(decoder))==0)
	{
		if (parity & 1)
		{
			block[63]^=1;
			return -1;
		}

		return 0;	
	}
	
	do
	{
		int16 level;
		uint8 m;
		
		n+=(code & 0xffff);
		level=code>>16;
		
		m=zigzag[n];
		
		block[m]=(level*qmatrix[m]*decoder->slice_info.quantiser_scale)/16;		
		parity^=block[m];

//		if (block[m]<-2048)
//			block[m]=-2048;
//		else if (block[m]>2047)
//			block[m]=2047;				

		++n;
	}	
	while ((code=get_mpeg2_dct_coeff_intra(decoder))!=0);
	
	if (parity & 1)
		block[63]^=1;

	return -1;
}

int8 
parse_mpeg2_non_intra_block (mpeg2video_decoder_t *decoder, int16 *block, uint8 cc)
{
	uint8 n=0;
	
	const uint8 *qmatrix=(decoder->sequence_info.chroma_format==YUV420 || cc==0)
							? decoder->sequence_info.quant_matrix[1]
							: decoder->sequence_info.quant_matrix[3];

	const uint8 *zigzag=kZigZagOffsetMMX[decoder->picture_info.alternate_scan];

	uint8 parity=1;
	int16 level;
	uint8 m;
		
	int32 code=get_mpeg2_dct_coeff_non_intra(decoder,true);
	ASSERT(code!=0);
	
	n+=(code & 0xffff);

	level=code>>16;
	
	m=zigzag[n];

	block[m]=((2*level+(level<0 ? -1 : 1))*qmatrix[m]*decoder->slice_info.quantiser_scale)/32;				
	parity^=block[m];

	++n;
	
	code=get_mpeg2_dct_coeff_non_intra(decoder,false);
	
	if (code==0)
	{
		if (parity & 1)
		{
			block[63]^=1;
			return -1;
		}
		
		return m;
	}
	
	do
	{
		int16 level;
		uint8 m;
		
		n+=(code & 0xffff);

		level=code>>16;
		
		m=zigzag[n];

		block[m]=((2*level+(level<0 ? -1 : 1))*qmatrix[m]*decoder->slice_info.quantiser_scale)/32;				
		parity^=block[m];
		
//		if (block[m]<-2048)
//			block[m]=-2048;
//		else if (block[m]>2047)
//			block[m]=2047;				

		++n;
	}	
	while ((code=get_mpeg2_dct_coeff_non_intra(decoder,false))!=0);

	if (parity & 1)
		block[63]^=1;

	return -1;
}

int8 
parse_mpeg1_intra_block (mpeg2video_decoder_t *decoder, int16 *block, uint8 cc)
{
	uint8 dct_dc_size=(cc==0) ? get_dct_dc_size_luminance(decoder)
							  : get_dct_dc_size_chrominance(decoder);
	
	uint16 dct_diff;
	int16 val;
	uint8 run;
	int16 level;
	bool sign;
	uint8 n=1;
	
	const uint8 *qmatrix=(decoder->sequence_info.chroma_format==YUV420 || cc==0)
							? decoder->sequence_info.quant_matrix[0]
							: decoder->sequence_info.quant_matrix[2];

	
	if (dct_dc_size==0)
		dct_diff=0;
	else
	{
		uint16 dct_dc_differential=mpeg2bits_get(decoder->bitstream,dct_dc_size);
	
		uint16 half_range=1<<(dct_dc_size-1);
		
		if (dct_dc_differential>=half_range)
			dct_diff=dct_dc_differential;
		else
			dct_diff=(dct_dc_differential+1)-(half_range*2);
	}
	
	decoder->slice_info.dc_dct_pred[cc]+=dct_diff;
	
	val=decoder->slice_info.dc_dct_pred[cc]<<3;

	block[0]=val;
	

	if (get_mpeg1_dct_coeff_intra(decoder,&run,&level,&sign))
		return 0;
		
	do
	{
		uint8 m;
		
		n+=run;

		m=kZigZagOffsetMMX[0][n];

		level=(level*qmatrix[m]*decoder->slice_info.quantiser_scale)>>3;
		level=(level-1)|1;
		
		block[m]=sign ? -level : level;
		
		if (block[m]<-2048)
			block[m]=-2048;
		else if (block[m]>2047)
			block[m]=2047;				

		++n;
	}	
	while (!get_mpeg1_dct_coeff_intra(decoder,&run,&level,&sign));
	
	return -1;
}

int8 
parse_mpeg1_non_intra_block (mpeg2video_decoder_t *decoder, int16 *block, uint8 cc)
{
	uint8 n=0;
	
	const uint8 *qmatrix=(decoder->sequence_info.chroma_format==YUV420 || cc==0)
							? decoder->sequence_info.quant_matrix[1]
							: decoder->sequence_info.quant_matrix[3];

	uint8 run;
	int16 level;
	bool sign;
	while (!get_mpeg1_dct_coeff_non_intra(decoder,n==0,&run,&level,&sign))
	{
		uint8 m;
		
		n+=run;

		m=kZigZagOffsetMMX[0][n];

		level=(((level<<1)+1)*qmatrix[m]*decoder->slice_info.quantiser_scale)>>4;
		level=(level-1)|1;
		
		block[m]=sign ? -level : level;
		
		if (block[m]<-2048)
			block[m]=-2048;
		else if (block[m]>2047)
			block[m]=2047;				

		++n;
	}	

	return -1;
}

bool
get_mpeg1_dct_coeff_intra (mpeg2video_decoder_t *decoder,
							uint8 *run, int16 *level, bool *sign)
{
	const dct_vlc_t *tab; 

   	uint16 code = mpeg2bits_peek(decoder->bitstream,16);

   	if(code >= 16384)
		tab = &kDCTtabnext[(code >> 12) - 4];
   	else 
	if(code >= 1024) tab = &kDCTtab0[(code >> 8) - 4];
   	else 
	if(code >= 512) tab = &kDCTtab1[(code >> 6) - 8];
   	else 
	if(code >= 256) tab = &kDCTtab2[(code >> 4) - 16];
   	else 
	if(code >= 128) tab = &kDCTtab3[(code >> 3) - 16];
   	else 
	if(code >= 64) tab = &kDCTtab4[(code >> 2) - 16];
   	else 
	if(code >= 32) tab = &kDCTtab5[(code >> 1) - 16];
   	else 
	if(code >= 16) tab = &kDCTtab6[code - 16];
   	else 
	{
		TRESPASS();
   	  	return true;
   	}
	
	mpeg2bits_skip(decoder->bitstream,tab->len);
	
   	if(tab->run == 64)
   		return true;

   	if(tab->run == 65)
	{
/* escape */
		*run=mpeg2bits_get(decoder->bitstream,6);
		
   		if((*level = mpeg2bits_get(decoder->bitstream,8)) == 0) 
			*level = mpeg2bits_get(decoder->bitstream,8);
   		else 
		if((*level) == 128)         
			*level = ((int16)mpeg2bits_get(decoder->bitstream,8)) - 256;
   		else 
		if((*level) > 128)          
			(*level) -= 256;
			
		if ((*level)<0)
		{
			*sign=true;
			*level=-*level;
		}
		else
			*sign=false;
   	}
   	else 
	{
   		*run=tab->run;
   		*level=tab->level;

   		*sign=mpeg2bits_get(decoder->bitstream,1);
   	}
   	
   	return false;
}


bool
get_mpeg1_dct_coeff_non_intra (mpeg2video_decoder_t *decoder,
									bool first_coeff, uint8 *run,
									int16 *level, bool *sign)
{
	const dct_vlc_t *tab; 

   	uint16 code = mpeg2bits_peek(decoder->bitstream,16);

   	if(code >= 16384)
	{
   	    if(first_coeff) 
			tab = &kDCTtabfirst[(code >> 12) - 4];
   	    else      
			tab = &kDCTtabnext[(code >> 12) - 4];
   	}
   	else 
	if(code >= 1024) tab = &kDCTtab0[(code >> 8) - 4];
   	else 
	if(code >= 512)  tab = &kDCTtab1[(code >> 6) - 8];
   	else 
	if(code >= 256)  tab = &kDCTtab2[(code >> 4) - 16];
   	else 
	if(code >= 128)  tab = &kDCTtab3[(code >> 3) - 16];
   	else 
	if(code >= 64)   tab = &kDCTtab4[(code >> 2) - 16];
   	else 
	if(code >= 32)   tab = &kDCTtab5[(code >> 1) - 16];
   	else 
	if(code >= 16)   tab = &kDCTtab6[code - 16];
   	else 
	{
		TRESPASS();
		return true;
   	}

   	mpeg2bits_skip(decoder->bitstream,tab->len);

/* end of block */
   	if(tab->run == 64)
		return true;
		
   	if(tab->run == 65)
	{
/* escape */
		*run=mpeg2bits_get(decoder->bitstream,6);
		
   		if((*level = mpeg2bits_get(decoder->bitstream,8)) == 0) 
			*level = mpeg2bits_get(decoder->bitstream,8);
   		else 
		if((*level) == 128)         
			*level = ((int16)mpeg2bits_get(decoder->bitstream,8)) - 256;
   		else 
		if((*level) > 128)          
			(*level) -= 256;

		if ((*level)<0)
		{
			*sign=true;
			*level=-*level;
		}
		else
			*sign=false;
   	}
   	else 
	{
   		*run=tab->run;
   		*level=tab->level;

   		*sign=mpeg2bits_get(decoder->bitstream,1);
   	}
   	
   	return false;
}

