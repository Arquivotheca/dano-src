#ifndef C_MPEG2_VIDEO_DECODER_H

#define C_MPEG2_VIDEO_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mpeg2buffer.h"
#include "mpeg2bitstream.h"
#include "mpeg2video_defs.h"

typedef struct mpeg2video_sequence_info_
{
	bool is_mpeg2;
			
	uint16 horizontal_size;
	uint16 vertical_size;

	float frame_rate;
	uint32 bit_rate;

	uint8 quant_matrix[4][64];
	
	bool progressive;		
	mpeg2_chroma_format_t chroma_format;

	// derived from coded data:
	
	uint16 mb_width;
	uint16 mb_height;

	uint8 block_count;

	uint32 num_macroblocks;
	
} mpeg2video_sequence_info_t;

typedef struct picture_info_
{
	mpeg2_picture_coding_t picture_coding_type;

	uint8 f_code[2][2];
	
	uint8 intra_dc_precision;
	mpeg2_picture_structure_t picture_structure;
	bool top_field_first;
	bool frame_pred_frame_dct;
	bool concealment_motion_vectors;
	uint8 q_scale_type;		
	uint8 intra_vlc_format;
	bool alternate_scan;
	bool repeat_first_field;
	uint8 chroma_420_type;
	bool progressive_frame;
	
	// derived from coded data:
	
	uint8 intra_dc_mult;
	
} picture_info_t;

typedef struct slice_info_
{
	int16 dc_dct_pred[3];
	int16 PMV[2][2][2];

	uint16 mb_row;
	uint16 mb_column;
	
	uint8 quantiser_scale;

	mpeg2_fieldframe_motion_t fieldframe_motion_type;
	uint8 motion_vector_count;
	mpeg2_motion_vector_format_t motion_vector_format;
	uint8 motion_vertical_field_select[2][2];
	int8 dmvector[2];
			
	uint8 dct_type;
									
} slice_info_t;

typedef status_t (*acquire_buffer_func) (void *cookie, buffer_t **buffer);
typedef status_t (*send_buffer_func) (void *cookie, buffer_t *buffer);

typedef struct mpeg2video_decoder_
{
	int32 width,height;
	int32 uv_width,uv_height;
	size_t u_offset,v_offset;
	
	int16 *block[8];
	int8 block_sparse_index[8];
	
	acquire_buffer_func acquire_output_buffer;
	send_buffer_func send_output_buffer;
	
	void *cookie;
	
	mpeg2video_bitstream_t *bitstream;
	
	buffer_t *current_picture;
	buffer_t *ref_picture[2];
	buffer_t *last_picture_output;
	
	bigtime_t next_presentation_time;
	
	thread_id tid;
	
	mpeg2video_sequence_info_t sequence_info;
	picture_info_t picture_info;
	slice_info_t slice_info;

	vint32 no_of_b_frames_to_skip;
		
} mpeg2video_decoder_t;

mpeg2video_decoder_t *mpeg2video_create (int32 width, int32 height,
											mpeg2_chroma_format_t chroma_format,
											acquire_buffer_func acquire_output_buffer,
											send_buffer_func send_output_buffer,
											void *cookie);
											
void mpeg2video_destroy (mpeg2video_decoder_t *decoder);
void mpeg2video_decode (mpeg2video_decoder_t *decoder, buffer_t *buffer);
void mpeg2video_flush(mpeg2video_decoder_t *decoder);
void mpeg2video_repeat_last_picture (mpeg2video_decoder_t *decoder);
void mpeg2video_late(mpeg2video_decoder_t *decoder, bigtime_t how_late);

int32 mpeg2video_threadfunc (void *param);

void output_picture (mpeg2video_decoder_t *decoder, buffer_t *picture, bool is_repeat);

#ifdef __cplusplus
}
#endif

#endif
