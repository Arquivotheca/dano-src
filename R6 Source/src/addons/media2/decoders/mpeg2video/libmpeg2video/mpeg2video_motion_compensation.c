#include "mpeg2video_motion_compensation.h"
#include "mpeg2video_slice.h"

#include <mpeg2lib.h>

#include <support/Debug.h>

bool
motion_compensation (mpeg2video_decoder_t *decoder, mpeg2_macroblock_t mb_type)
{
	bool dirty;
	
	if (decoder->picture_info.picture_coding_type==P_PICTURE
			&& !(mb_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_INTRA)))
	{
		mpeg2video_reset_motion_predictors(decoder);

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
	}

	dirty=false;
	
	if (mb_type & MACROBLOCK_MOTION_FORWARD)
	{
		ASSERT (decoder->picture_info.picture_structure==FRAME_PICTURE);

		if (decoder->slice_info.fieldframe_motion_type==FRAME_BASED)
			forward_backward_motion_compensation(decoder,0,dirty,false,false);
		else if (decoder->slice_info.fieldframe_motion_type==FIELD_BASED)
		{
			forward_backward_motion_compensation(decoder,0,dirty,true,false);
			forward_backward_motion_compensation(decoder,0,dirty,true,true);
		}
//		else
//			TRESPASS();
		
		dirty=true;
	}

	if (mb_type & MACROBLOCK_MOTION_BACKWARD)
	{
		ASSERT (decoder->picture_info.picture_structure==FRAME_PICTURE);
				
		if (decoder->slice_info.fieldframe_motion_type==FRAME_BASED)
			forward_backward_motion_compensation(decoder,1,dirty,false,false);
		else if (decoder->slice_info.fieldframe_motion_type==FIELD_BASED)
		{
			forward_backward_motion_compensation(decoder,1,dirty,true,false);
			forward_backward_motion_compensation(decoder,1,dirty,true,true);
		}
		else
			TRESPASS();
		
		dirty=true;
	}

	ASSERT (decoder->picture_info.picture_structure==FRAME_PICTURE);

	if (decoder->slice_info.fieldframe_motion_type==FRAME_BASED)
	{
		if (mb_type & MACROBLOCK_MOTION_FORWARD)
		{
			decoder->slice_info.PMV[1][0][0]=decoder->slice_info.PMV[0][0][0];
			decoder->slice_info.PMV[1][0][1]=decoder->slice_info.PMV[0][0][1];
		}

		if (mb_type & MACROBLOCK_MOTION_BACKWARD)
		{
			decoder->slice_info.PMV[1][1][0]=decoder->slice_info.PMV[0][1][0];
			decoder->slice_info.PMV[1][1][1]=decoder->slice_info.PMV[0][1][1];
		}
	}

	return dirty;
}

void
forward_backward_motion_compensation (mpeg2video_decoder_t *decoder,
										uint8 backward, bool average,
										bool field, bool bottom)
{
	buffer_t *src;
	uint8 height;
	size_t skip;
	size_t dst_offset;
	size_t src_offset;
	int16 dx,dy;
	
	if (decoder->ref_picture[backward]==NULL)
		return;		

	src=decoder->ref_picture[backward];				

	height=field ? 8 : 16;
	skip=field ? decoder->width*2 : decoder->width;
	dst_offset=field ? decoder->width*bottom : 0;
	src_offset=field ? decoder->width*decoder->slice_info.motion_vertical_field_select[bottom][backward] : 0;
	
	dx=decoder->slice_info.PMV[bottom][backward][0];
	dy=decoder->slice_info.PMV[bottom][backward][1];

	if (field)
		dy/=2;
	
	MotionCompensationDispatch((uint8 *)decoder->current_picture->data+dst_offset,
								(const uint8 *)src->data+src_offset,skip,1,height,
				16*decoder->slice_info.mb_column,(field ? 8 : 16)*decoder->slice_info.mb_row,
				dx,dy,
				average);					
	
	ASSERT(decoder->sequence_info.chroma_format==YUV420);

	height=field ? 4 : 8;
	skip=field ? decoder->uv_width*2 : decoder->uv_width;
	dst_offset=field ? decoder->uv_width*bottom : 0;
	src_offset=field ? decoder->uv_width*decoder->slice_info.motion_vertical_field_select[bottom][backward] : 0;
						
	MotionCompensationDispatch((uint8 *)decoder->current_picture->data+decoder->u_offset+dst_offset,
								(const uint8 *)src->data+decoder->u_offset+src_offset,skip,0,height,
				8*decoder->slice_info.mb_column,(field ? 4 : 8)*decoder->slice_info.mb_row,
				dx/2,dy/2,
				average);					

	MotionCompensationDispatch((uint8 *)decoder->current_picture->data+decoder->v_offset+dst_offset,
								(const uint8 *)src->data+decoder->v_offset+src_offset,skip,0,height,
				8*decoder->slice_info.mb_column,(field ? 4 : 8)*decoder->slice_info.mb_row,
				dx/2,dy/2,
				average);
				
	asm ("emms");
}
 
void 
add_or_copy_block (mpeg2video_decoder_t *decoder, uint8 block_num, bool add)
{
	buffer_t *pic=decoder->current_picture;

	uint8 *ptr;
	size_t stride;
	const int16 *block_ptr;
			
	if (block_num<4)
	{
		uint16 x=decoder->slice_info.mb_column*16;
		uint16 y;
		
		if (block_num & 1)
			x+=8;
			
		y=decoder->slice_info.mb_row*16;

		if (!decoder->slice_info.dct_type) // frame DCT
		{
			if (block_num & 2)
				y+=8;
				
			stride=decoder->width;
			ptr=(uint8 *)pic->data+y*stride;
		}
		else
		{
			if (block_num & 2)
				++y;

			stride=decoder->width*2;
			ptr=(uint8 *)pic->data+y*decoder->width;
		}
		
		ptr+=x;
	}
	else
	{
		uint16 x;
		uint16 y;
		
		bool is_U=!(block_num&1);
		ptr=is_U ? (uint8 *)pic->data+decoder->u_offset : (uint8 *)pic->data+decoder->v_offset;

		if (decoder->sequence_info.chroma_format!=YUV444)
			x=decoder->slice_info.mb_column*8;
		else
			x=decoder->slice_info.mb_column*16+(block_num&8 ? 8 : 0);	

		if (decoder->sequence_info.chroma_format==YUV420)
			y=decoder->slice_info.mb_row*8;
		else
			y=decoder->slice_info.mb_row*16;							

		if (!decoder->slice_info.dct_type) // frame DCT
		{
			if (block_num & 2)
				y+=8;
				
			stride=decoder->uv_width;
			ptr+=y*stride;
		}		
		else
		{
			if (decoder->sequence_info.chroma_format==YUV420)
			{
				stride=decoder->uv_width;
				ptr+=y*stride;
			}
			else
			{
				if (block_num & 2)
					++y;
	
				stride=2*decoder->uv_width;
	
				ptr+=y*decoder->uv_width;
			}
		}
		
		ptr+=x;
	}
 
	block_ptr=decoder->block[block_num];

	if (add)
		add_block(ptr,block_ptr,stride);
	else
		copy_block(ptr,block_ptr,stride);
}
