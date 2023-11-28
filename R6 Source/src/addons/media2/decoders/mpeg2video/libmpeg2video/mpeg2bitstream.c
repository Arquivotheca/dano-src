#include "mpeg2bitstream.h"
#include "mpeg2buffer.h"
#include "mpeg2video_defs.h"

#include <support/ByteOrder.h>
#include <malloc.h>

mpeg2video_bitstream_t *
mpeg2bits_create()
{
	mpeg2video_bitstream_t *bitstream
		=(mpeg2video_bitstream_t *)malloc(sizeof(mpeg2video_bitstream_t));
		
	bitstream->first_input_buffer=NULL;
	bitstream->last_input_buffer=NULL;
		
	bitstream->input_buffer_guard_sem=create_sem(1,"input_buffer_guard");
	bitstream->input_buffer_read_sem=create_sem(0,"input_buffer_read");
	
	bitstream->next_data=NULL;
	bitstream->data_end=NULL;
	bitstream->current_word=0;
	bitstream->bits_left=0;
	bitstream->temporary_storage_length=0;
	bitstream->current_buffer=NULL;
	
	return bitstream;
}

void 
mpeg2bits_destroy(mpeg2video_bitstream_t *bitstream)
{
	buffer_t *current;
	
	while (acquire_sem(bitstream->input_buffer_guard_sem)==B_INTERRUPTED)
		;

	if (bitstream->current_buffer)
		bitstream->current_buffer->release_ref(bitstream->current_buffer);
	
	current=bitstream->first_input_buffer;
	while (current)
	{
		buffer_t *next_current=current->next;
		
		current->release_ref(current);
		current=next_current;
	}
	
	delete_sem(bitstream->input_buffer_read_sem);
	delete_sem(bitstream->input_buffer_guard_sem);
	
	free(bitstream);
}

void
mpeg2bits_enqueue_code (mpeg2video_bitstream_t *bitstream, int32 code)
{
	buffer_t *buffer;
	
	buffer=(buffer_t *)malloc(sizeof(buffer_t));
	buffer->data=NULL;
	buffer->size=0;
	buffer->cookie=NULL;
	buffer->acquire_ref=NULL;
	buffer->release_ref=NULL;

	buffer->code=code;
	
	mpeg2bits_enqueue_buffer (bitstream,buffer);
}

void
mpeg2bits_enqueue_buffer (mpeg2video_bitstream_t *bitstream, buffer_t *buffer)
{
	buffer->next=NULL;

	buffer->tagged=false;
	
	while (acquire_sem(bitstream->input_buffer_guard_sem)==B_INTERRUPTED)
		;
		
	if (bitstream->last_input_buffer)
		bitstream->last_input_buffer->next=buffer;
	else
		bitstream->first_input_buffer=buffer;
		
	bitstream->last_input_buffer=buffer;
	
	release_sem(bitstream->input_buffer_guard_sem);
	
	release_sem(bitstream->input_buffer_read_sem);
}
							
buffer_t *
mpeg2bits_next_input_buffer (mpeg2video_bitstream_t *bitstream, int32 *code)
{
	buffer_t *next_input_buffer;
	
	while (acquire_sem(bitstream->input_buffer_read_sem)==B_INTERRUPTED)
		;

	while (acquire_sem(bitstream->input_buffer_guard_sem)==B_INTERRUPTED)
		;

	next_input_buffer=bitstream->first_input_buffer;
	bitstream->first_input_buffer=bitstream->first_input_buffer->next;
	
	if (bitstream->first_input_buffer==NULL)
		bitstream->last_input_buffer=NULL;
		
	release_sem(bitstream->input_buffer_guard_sem);

	if (next_input_buffer->data==NULL)
	{
		// this is a special "code"-buffer
		
		*code=next_input_buffer->code;
		free(next_input_buffer);			// we created this ourselve so we
											// use the regular free call here
											
		next_input_buffer=NULL;
	}
	
	return next_input_buffer;
}

#if !defined(USE_MMX_BITSTREAM)

uint32 
mpeg2bits_get(mpeg2video_bitstream_t *bitstream, uint32 count)
{
	uint32 result;
	
	if (count>bitstream->bits_left)
	{
		result=bitstream->current_word>>(64-bitstream->bits_left);
		count-=bitstream->bits_left;

		if (bitstream->next_data>=bitstream->data_end)
			mpeg2bits_refill_from_buffer(bitstream);
		
		bitstream->current_word=B_BENDIAN_TO_HOST_INT64(*bitstream->next_data++);
		bitstream->bits_left=64-count;
		result=(result<<count) | (bitstream->current_word>>bitstream->bits_left);
		bitstream->current_word<<=count;
	}
	else
	{
		result=bitstream->current_word>>(64-count);
		bitstream->current_word<<=count;
		bitstream->bits_left-=count;
	}
	
	return result;	
}

uint32 
mpeg2bits_peek(mpeg2video_bitstream_t *bitstream, uint32 count)
{
	uint32 result;
	
	if (count>bitstream->bits_left)
	{
		uint32 temp;
		
		result=bitstream->current_word>>(64-bitstream->bits_left);
		count-=bitstream->bits_left;

		if (bitstream->next_data>=bitstream->data_end)
			mpeg2bits_refill_from_buffer(bitstream);
		
		temp=B_BENDIAN_TO_HOST_INT32(*(const uint32 *)bitstream->next_data);
		
		result=(result<<count) | (temp>>(32-count));
	}
	else
		result=bitstream->current_word>>(64-count);
	
	return result;
}

void 
mpeg2bits_skip(mpeg2video_bitstream_t *bitstream, uint32 count)
{
	if (count>bitstream->bits_left)
	{
		count-=bitstream->bits_left;

		if (bitstream->next_data>=bitstream->data_end)
			mpeg2bits_refill_from_buffer(bitstream);
		
		bitstream->current_word=B_BENDIAN_TO_HOST_INT64(*bitstream->next_data++)<<count;
		bitstream->bits_left=64-count;
	}
	else
	{
		bitstream->current_word<<=count;
		bitstream->bits_left-=count;
	}
}

#endif

void 
mpeg2bits_skip_to_byte_boundary (mpeg2video_bitstream_t *bitstream)
{
	uint32 n=bitstream->bits_left%8;
	
	if (n>0)
		mpeg2bits_skip(bitstream,n);
}

void
mpeg2bits_skip_to_startcode (mpeg2video_bitstream_t *bitstream)
{
	mpeg2bits_skip_to_byte_boundary(bitstream);
	
	while (mpeg2bits_peek(bitstream,24)!=0x000001)
		mpeg2bits_skip(bitstream,8);
}

void 
mpeg2bits_refill_from_buffer(mpeg2video_bitstream_t *bitstream)
{
	// make sure there are at least 64 bits of valid data
	// pointed to by mNextData
	// (char *)mDataEnd-(char *)mNextData MUST be a multiple of 8 bytes

	bitstream->temporary_storage_length=0;
	
	while (bitstream->temporary_storage_length<8)
	{
		int32 code;

		if (bitstream->current_buffer!=NULL)
		{
			while (bitstream->temporary_storage_length<8
					&& bitstream->current_buffer_offset<bitstream->current_buffer->size)
			{
				bitstream->temporary_storage[bitstream->temporary_storage_length++]=((const uint8 *)bitstream->current_buffer->data)[bitstream->current_buffer_offset];
				++bitstream->current_buffer_offset;
			}

			if (bitstream->temporary_storage_length==8)
				continue;
		}
	
		if (bitstream->current_buffer!=NULL)
		{
			bitstream->current_buffer->release_ref(bitstream->current_buffer);			
			bitstream->current_buffer=NULL;
		}

		bitstream->current_buffer=mpeg2bits_next_input_buffer(bitstream,&code);

		bitstream->current_buffer_offset=0;
		
		if (bitstream->current_buffer==NULL)
		{
			if (code!=MPEG2VIDEO_ZERO_FILLER)
			{
				longjmp(bitstream->setjmp_cookie,code);
				// this line should never be reached
			}
			
			while (bitstream->temporary_storage_length<8)
				bitstream->temporary_storage[bitstream->temporary_storage_length++]=0;

			break;				
		}
		
		if (!bitstream->temporary_storage_length && bitstream->current_buffer->size>=8)
		{
			bitstream->next_data=(const uint64 *)bitstream->current_buffer->data;
			bitstream->data_end=bitstream->next_data+(bitstream->current_buffer->size/8);
			
			bitstream->current_buffer_offset=bitstream->current_buffer->size & ~7;
			
			return;
		}
	}
	
	bitstream->next_data=(const uint64 *)bitstream->temporary_storage;
	bitstream->data_end=bitstream->next_data+1;
}

void 
mpeg2bits_clear(mpeg2video_bitstream_t *bitstream)
{
	buffer_t *current;
	int32 count;
	
	while (acquire_sem(bitstream->input_buffer_guard_sem)==B_INTERRUPTED)
		;

	if (bitstream->current_buffer)
	{
		bitstream->current_buffer->release_ref(bitstream->current_buffer);
		bitstream->current_buffer=NULL;
	}
	
	count=0;
	
	current=bitstream->first_input_buffer;
	while (current)
	{
		buffer_t *next_current=current->next;
		
		++count;
		
		current->release_ref(current);
		current=next_current;
	}

	while (acquire_sem_etc(bitstream->input_buffer_read_sem,count,0,0)==B_INTERRUPTED)
		;

	bitstream->first_input_buffer=bitstream->last_input_buffer=NULL;
	
	bitstream->next_data=NULL;
	bitstream->data_end=NULL;
	bitstream->current_word=0;
	bitstream->bits_left=0;
	bitstream->temporary_storage_length=0;

	release_sem(bitstream->input_buffer_guard_sem);
}

