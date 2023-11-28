#include "mpeg2video_motion_vectors.h"

#include <support/Debug.h>

void 
parse_motion_vectors (mpeg2video_decoder_t *decoder, uint8 s)
{
	if (decoder->slice_info.motion_vector_count==1)
	{
		if (decoder->slice_info.motion_vector_format==FIELD_MOTION_VECTOR && decoder->slice_info.fieldframe_motion_type!=DUAL_PRIME)
			decoder->slice_info.motion_vertical_field_select[0][s]=mpeg2bits_get(decoder->bitstream,1);
		
		parse_motion_vector(decoder,0,s);

		// update non-coded motion vectors
		
		decoder->slice_info.PMV[1][s][0] = decoder->slice_info.PMV[0][s][0];
		decoder->slice_info.PMV[1][s][1] = decoder->slice_info.PMV[0][s][1];
	}
	else
	{
		decoder->slice_info.motion_vertical_field_select[0][s]=mpeg2bits_get(decoder->bitstream,1);

		parse_motion_vector(decoder,0,s);		

		decoder->slice_info.motion_vertical_field_select[1][s]=mpeg2bits_get(decoder->bitstream,1);

		parse_motion_vector(decoder,1,s);		
	}
}

int8
get_motion_code (mpeg2video_decoder_t *decoder)
{
	typedef struct vlc_
	{
	  uint8 val;
	  uint8 len;
	} vlc_t;

	static const vlc_t kMVtab0[8] =
	{ {0xff,0}, {3,3}, {2,2}, {2,2}, {1,1}, {1,1}, {1,1}, {1,1}
	};
	
	static const vlc_t	kMVtab1[8] =
	{ {0xff,0}, {0xff,0}, {0xff,0}, {7,6}, {6,6}, {5,6}, {4,5}, {4,5}
	};
	
	static const vlc_t	kMVtab2[12] =
	{ {16,9}, {15,9}, {14,9}, {13,9},
	  {12,9}, {11,9}, {10,8}, {10,8},
	  {9,8},  {9,8},  {8,8},  {8,8}
	};

 	uint16 code;
	int8 val;

  	if(mpeg2bits_get(decoder->bitstream,1))
    	return 0;

	code=mpeg2bits_peek(decoder->bitstream,9);

	val=0;
		
  	if (code>=64)
	{
    	code>>=6;
    	mpeg2bits_skip(decoder->bitstream,kMVtab0[code].len);
    	val=kMVtab0[code].val;
  	}
	else if (code>=24)
	{
    	code>>=3;
    	mpeg2bits_skip(decoder->bitstream,kMVtab1[code].len);
    	val=kMVtab1[code].val;
  	}
	else if (code>=12)
	{
		code-=12;
    	mpeg2bits_skip(decoder->bitstream,kMVtab2[code].len);
    	val=kMVtab2[code].val;
	}
	else
		TRESPASS();

	return mpeg2bits_get(decoder->bitstream,1) ? -val : val;
}

int8
get_dm_vector (mpeg2video_decoder_t *decoder)
{
	if (!mpeg2bits_get(decoder->bitstream,1))
		return 0;
	
	if (mpeg2bits_get(decoder->bitstream,1))
		return -1;
	else
		return 1;
}

void
parse_motion_vector (mpeg2video_decoder_t *decoder, uint8 r, uint8 s)
{
	int32 t;
	
	for (t=0;t<2;++t)
	{
		int8 motion_code=get_motion_code(decoder);

		uint8 motion_residual=0;
		if (decoder->picture_info.f_code[s][t]!=1 && motion_code!=0)
			motion_residual=mpeg2bits_get(decoder->bitstream,decoder->picture_info.f_code[s][t]-1);
		else
			motion_residual=0;

		if (decoder->slice_info.fieldframe_motion_type==DUAL_PRIME)
			decoder->slice_info.dmvector[t]=get_dm_vector(decoder);

		if (decoder->slice_info.motion_vector_format==FIELD_MOTION_VECTOR
				&& t==1
				&& decoder->picture_info.picture_structure==FRAME_PICTURE)
		{
			decoder->slice_info.PMV[r][s][t]>>=1;		// _not_ /=2
		}

		decode_mv(&decoder->slice_info.PMV[r][s][t],decoder->picture_info.f_code[s][t]-1,
					motion_code,motion_residual);

		if (decoder->slice_info.motion_vector_format==FIELD_MOTION_VECTOR
				&& t==1
				&& decoder->picture_info.picture_structure==FRAME_PICTURE)
		{
			decoder->slice_info.PMV[r][s][t]<<=1;
		}
	}
}

void
decode_mv (int16 *pmv, uint8 r_size,
			int8 motion_code, uint8 motion_residual)
{
	int lim = 16 << r_size;
	int vec = 0 ? (*pmv >> 1) : (*pmv);

	if(motion_code > 0)
	{
    	vec += ((motion_code - 1) << r_size) + motion_residual + 1;
    	if(vec >= lim) vec -= lim + lim;
	}
	else 
	if(motion_code < 0)
	{
    	vec -= ((-motion_code - 1) << r_size) + motion_residual + 1;
    	if(vec < -lim) vec += lim + lim;
	}
	*pmv = 0 ? (vec << 1) : vec;
}

