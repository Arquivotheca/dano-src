#include "mpeg2video_picture.h"

#include <support/Debug.h>

void 
mpeg2video_parse_picture(mpeg2video_decoder_t *decoder)
{
	bool has_start_time;
	bigtime_t start_time=0;
	status_t result;
	
	ASSERT(decoder->current_picture==NULL);
	
	has_start_time=decoder->bitstream->current_buffer->has_start_time;

	if (has_start_time)
		start_time=decoder->bitstream->current_buffer->start_time;
	
	mpeg2bits_skip(decoder->bitstream,10);
	
	decoder->picture_info.picture_coding_type=(mpeg2_picture_coding_t)mpeg2bits_get(decoder->bitstream,3);

	mpeg2bits_skip(decoder->bitstream,16);
	
	if (decoder->picture_info.picture_coding_type==P_PICTURE
		|| decoder->picture_info.picture_coding_type==B_PICTURE)
	{
		if (decoder->sequence_info.is_mpeg2)
			mpeg2bits_skip(decoder->bitstream,4);
		else
		{
			mpeg2bits_skip(decoder->bitstream,1);
			decoder->picture_info.f_code[0][0]=decoder->picture_info.f_code[0][1]=mpeg2bits_get(decoder->bitstream,3);
		}
	}
	
	if (decoder->picture_info.picture_coding_type==B_PICTURE)
	{
		if (decoder->sequence_info.is_mpeg2)
			mpeg2bits_skip(decoder->bitstream,4);
		else
		{
			mpeg2bits_skip(decoder->bitstream,1);
			decoder->picture_info.f_code[1][0]=decoder->picture_info.f_code[1][1]=mpeg2bits_get(decoder->bitstream,3);
		}
	}
	
	while (mpeg2bits_get(decoder->bitstream,1))
		mpeg2bits_skip(decoder->bitstream,8);

	result=decoder->acquire_output_buffer(decoder->cookie,&decoder->current_picture);

	if (result<B_OK)
	{
		decoder->current_picture=NULL;
		return;
	}
	
	decoder->current_picture->repeat_first_field=false;

	if (has_start_time)
	{
		decoder->current_picture->has_start_time=true;
		decoder->current_picture->start_time=start_time;
	}
		
	if (decoder->picture_info.picture_coding_type!=B_PICTURE)
	{
		if (decoder->ref_picture[0]!=NULL)
		{
			decoder->ref_picture[0]->release_ref(decoder->ref_picture[0]);
			decoder->ref_picture[0]=NULL;
		}
		
		decoder->ref_picture[0]=decoder->ref_picture[1];
		decoder->ref_picture[1]=NULL;
	}

	if (!decoder->sequence_info.is_mpeg2)
	{
		decoder->picture_info.intra_dc_precision=0;
		decoder->picture_info.picture_structure=FRAME_PICTURE;
		decoder->picture_info.frame_pred_frame_dct=true;
		decoder->picture_info.concealment_motion_vectors=false;
		decoder->picture_info.q_scale_type=0;
		decoder->picture_info.intra_vlc_format=0;
		decoder->picture_info.alternate_scan=false;
	}
	
	update_derived_picture_info(decoder);
}

void
update_derived_picture_info (mpeg2video_decoder_t *decoder)
{
	decoder->picture_info.intra_dc_mult=1<<(3-decoder->picture_info.intra_dc_precision);	
}
