#ifndef C_MPEG2_BITSTREAM_H

#define C_MPEG2_BITSTREAM_H

#include "mpeg2buffer.h"

#include <OS.h>
#include <setjmp.h>

typedef struct mpeg2video_bitstream_
{
	const uint64 *next_data;	// +0
	const uint64 *data_end;		// +4

	uint64 current_word;		// +8
	uint32 bits_left;			// +16
	
	buffer_t *first_input_buffer;
	buffer_t *last_input_buffer;
	
	sem_id input_buffer_guard_sem;
	sem_id input_buffer_read_sem;
	
	uint8 temporary_storage[8];
	size_t temporary_storage_length;
	
	buffer_t *current_buffer;
	size_t current_buffer_offset;
	
	jmp_buf setjmp_cookie;	

} mpeg2video_bitstream_t;

mpeg2video_bitstream_t *mpeg2bits_create();
void mpeg2bits_destroy (mpeg2video_bitstream_t *bitstream);

void mpeg2bits_enqueue_code(mpeg2video_bitstream_t *bitstream, int32 code);
void mpeg2bits_enqueue_buffer(mpeg2video_bitstream_t *bitstream, buffer_t *buffer);
buffer_t *mpeg2bits_next_input_buffer(mpeg2video_bitstream_t *bitstream, int32 *code);
void mpeg2bits_refill_from_buffer(mpeg2video_bitstream_t *bitstream);

uint32 mpeg2bits_get (mpeg2video_bitstream_t *bitstream, uint32 count);
uint32 mpeg2bits_peek (mpeg2video_bitstream_t *bitstream, uint32 count);
void mpeg2bits_skip (mpeg2video_bitstream_t *bitstream, uint32 count);
void mpeg2bits_skip_to_byte_boundary (mpeg2video_bitstream_t *bitstream);
void mpeg2bits_skip_to_startcode(mpeg2video_bitstream_t *bitstream);
void mpeg2bits_clear(mpeg2video_bitstream_t *bitstream);

#if defined(USE_MMX_BITSTREAM)

void mpeg2bits_save_state (mpeg2video_bitstream_t *bitstream);
void mpeg2bits_restore_state (mpeg2video_bitstream_t *bitstream);

#endif

#endif
