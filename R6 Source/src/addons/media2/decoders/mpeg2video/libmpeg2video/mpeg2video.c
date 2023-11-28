#include "mpeg2video.h"
#include "mpeg2video_seq_header.h"
#include "mpeg2video_picture.h"
#include "mpeg2video_slice.h"
#include "mpeg2video_extension.h"

#include <support/Debug.h>

#include <malloc.h>

mpeg2video_decoder_t *
mpeg2video_create (int32 width, int32 height,
					mpeg2_chroma_format_t chroma_format,
					acquire_buffer_func acquire_output_buffer,
					send_buffer_func send_output_buffer,
					void *cookie)
{
	mpeg2video_decoder_t *decoder;
	int32 i;
		
	decoder=(mpeg2video_decoder_t *)malloc(sizeof(mpeg2video_decoder_t));
	
	decoder->width=width;
	decoder->height=height;
	
	if (chroma_format!=YUV444)
		decoder->uv_width=width/2;
	else
		decoder->uv_width=width;
	
	if (chroma_format==YUV420)
		decoder->uv_height=height/2;
	else
		decoder->uv_height=height;

	decoder->u_offset=width*height;
	decoder->v_offset=decoder->u_offset+decoder->uv_width*decoder->uv_height;				
	
	decoder->block[0]=(int16 *)memalign(16,sizeof(int16)*64*8);
	// since block[0] is aligned all the other blocks are too
	
	for (i=1;i<8;++i)
		decoder->block[i]=decoder->block[i-1]+64;

	decoder->acquire_output_buffer=acquire_output_buffer;
	decoder->send_output_buffer=send_output_buffer;
	decoder->cookie=cookie;

	decoder->bitstream=mpeg2bits_create();

	decoder->current_picture=NULL;
	decoder->ref_picture[0]=decoder->ref_picture[1]=NULL;
	decoder->last_picture_output=NULL;
	
	decoder->next_presentation_time=0;

	decoder->no_of_b_frames_to_skip=0;
		
	decoder->tid=spawn_thread(mpeg2video_threadfunc,"mpeg2video_thread",
								B_NORMAL_PRIORITY,decoder);

	resume_thread(decoder->tid);
									
	return decoder;
}

void 
mpeg2video_destroy (mpeg2video_decoder_t *decoder)
{
	status_t dummy;
	
	mpeg2bits_enqueue_code(decoder->bitstream,MPEG2VIDEO_SHUTDOWN);
	
	while (wait_for_thread(decoder->tid,&dummy)==B_INTERRUPTED)
		;
		
	mpeg2bits_destroy(decoder->bitstream);
		
	free(decoder);
}

void 
mpeg2video_decode (mpeg2video_decoder_t *decoder, buffer_t *buffer)
{
	mpeg2bits_enqueue_buffer(decoder->bitstream,buffer);
}

void
mpeg2video_flush (mpeg2video_decoder_t *decoder)
{
	mpeg2bits_enqueue_code(decoder->bitstream,MPEG2VIDEO_ZERO_FILLER);
	mpeg2bits_enqueue_code(decoder->bitstream,MPEG2VIDEO_FLUSH);
}

void 
mpeg2video_repeat_last_picture(mpeg2video_decoder_t *decoder)
{
	mpeg2bits_enqueue_code(decoder->bitstream,MPEG2VIDEO_REPEAT_LAST);
}

void
mpeg2video_late(mpeg2video_decoder_t *decoder, bigtime_t how_late)
{
	how_late=how_late;
	atomic_add(&decoder->no_of_b_frames_to_skip,1);
}

int32 
mpeg2video_threadfunc (void *param)
{
	bool done;
	
	mpeg2video_decoder_t *decoder=(mpeg2video_decoder_t *)param;
	
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_restore_state(decoder->bitstream);
#endif

	do
	{
		int val;
		uint32 code;
		
		val=setjmp(decoder->bitstream->setjmp_cookie);
		
		done=false;
		
		switch (val)
		{
			case MPEG2VIDEO_SHUTDOWN:
			{
				done=true;
				break;
			}

			case MPEG2VIDEO_FLUSH:
			{
				ASSERT(decoder->current_picture==NULL);
				
				if (decoder->current_picture!=NULL)
				{
					decoder->current_picture->release_ref(decoder->current_picture);				
					decoder->current_picture=NULL;
				}
				
				if (decoder->ref_picture[0]!=NULL)
				{
					decoder->ref_picture[0]->release_ref(decoder->ref_picture[0]);
					decoder->ref_picture[0]=NULL;
				}

				if (decoder->ref_picture[1]!=NULL)
				{
					output_picture(decoder,decoder->ref_picture[1],false);					
					decoder->ref_picture[1]=NULL;
				}
					
				break;
			}
			
			case MPEG2VIDEO_REPEAT_LAST:
			{
				if (decoder->last_picture_output!=NULL)
				{
					decoder->last_picture_output->acquire_ref(decoder->last_picture_output);

					output_picture(decoder,decoder->last_picture_output,true);
				}
				
				break;
			}

			case MPEG2VIDEO_LATE:
			{
				++decoder->no_of_b_frames_to_skip;
				
				break;
			}
						
			default:
			{
				mpeg2bits_skip_to_startcode(decoder->bitstream);
				
				code=mpeg2bits_get(decoder->bitstream,32);

				if (code>=0x101 && code<=0x1af)
				{
					if (code==0x101
						&& decoder->picture_info.picture_coding_type==B_PICTURE
						&& decoder->no_of_b_frames_to_skip>0)
					{
						// skip this picture
						
						bigtime_t T,duration;
						
						atomic_add(&decoder->no_of_b_frames_to_skip,-1);
						
						if (decoder->current_picture->has_start_time)
							decoder->next_presentation_time=decoder->current_picture->start_time;
							
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_save_state(decoder->bitstream);
#endif

						T=0.5+1000000LL/(2*decoder->sequence_info.frame_rate);
				
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_restore_state(decoder->bitstream);
#endif

						duration=decoder->current_picture->repeat_first_field ? 3*T : 2*T;				
				
						decoder->next_presentation_time+=duration;
						
						decoder->current_picture->release_ref(decoder->current_picture);
						decoder->current_picture=NULL;
					}
					else if (decoder->current_picture!=NULL)
						mpeg2video_parse_slice(decoder,code & 0xff);
				}
				else switch (code)
				{
					case C_SEQUENCE_HEADER_CODE:
						mpeg2video_parse_sequence_header(decoder);
						break;
						
					case C_PICTURE_START_CODE:
						mpeg2video_parse_picture(decoder);
						break;
					
					case C_EXTENSION_START_CODE:
						mpeg2video_parse_extension(decoder);
						break;

					case C_SEQUENCE_END_CODE:
					{
						ASSERT(decoder->current_picture==NULL);
						
						if (decoder->current_picture!=NULL)
						{
							decoder->current_picture->release_ref(decoder->current_picture);				
							decoder->current_picture=NULL;
						}
						
						if (decoder->ref_picture[0]!=NULL)
						{
							decoder->ref_picture[0]->release_ref(decoder->ref_picture[0]);
							decoder->ref_picture[0]=NULL;
						}
		
						if (decoder->ref_picture[1]!=NULL)
						{
							output_picture(decoder,decoder->ref_picture[1],false);					
							decoder->ref_picture[1]=NULL;
						}
						
						break;
					}
														
					default:
						break;
				}
				break;
			}
		}		
	}
	while (!done);
	
	if (decoder->last_picture_output)
	{
		decoder->last_picture_output->release_ref(decoder->last_picture_output);
		decoder->last_picture_output=NULL;
	}
	
	if (decoder->current_picture!=NULL)
	{
		decoder->current_picture->release_ref(decoder->current_picture);
		decoder->current_picture=NULL;
	}
	
	if (decoder->ref_picture[0]!=NULL)
	{
		decoder->ref_picture[0]->release_ref(decoder->ref_picture[0]);
		decoder->ref_picture[0]=NULL;
	}

	if (decoder->ref_picture[1]!=NULL)
	{
		decoder->ref_picture[1]->release_ref(decoder->ref_picture[1]);
		decoder->ref_picture[1]=NULL;
	}

	return 0;
}

void 
output_picture (mpeg2video_decoder_t *decoder, buffer_t *picture, bool is_repeat)
{
	bigtime_t T;
	bigtime_t duration;

#if defined(USE_MMX_BITSTREAM)
		mpeg2bits_save_state(decoder->bitstream);
#endif

	if (!is_repeat)
	{	
		if (decoder->last_picture_output)
		{
			decoder->last_picture_output->release_ref(decoder->last_picture_output);
			decoder->last_picture_output=NULL;
		}
	
		decoder->last_picture_output=picture;
		decoder->last_picture_output->acquire_ref(decoder->last_picture_output);
		
		if (picture->has_start_time)
			decoder->next_presentation_time=picture->start_time;
		else
		{
			picture->has_start_time=true;
			picture->start_time=decoder->next_presentation_time;
		}
			
		T=0.5+1000000LL/(2*decoder->sequence_info.frame_rate);

		duration=picture->repeat_first_field ? 3*T : 2*T;				

		decoder->next_presentation_time+=duration;
	}
	
	if (decoder->send_output_buffer(decoder->cookie,picture)<B_OK)
		picture->release_ref(picture);
	
	picture=NULL;
		
#if defined(USE_MMX_BITSTREAM)
	mpeg2bits_restore_state(decoder->bitstream);
#endif

	if (!is_repeat)
		decoder->last_picture_output->has_start_time=false;
}

