#ifndef _LOGO_ANIM_H
#define _LOGO_ANIM_H

#include <SupportDefs.h>

typedef struct anim_buffer {
	uint16		h, v;
	uint16		color[15];
	uint16		length;
	char		*data;
} anim_buffer;

typedef struct anim_rect {
	uint8		v;
	uint8		dv;
	uint16		h;
	uint16		dh;
} anim_rect;

typedef struct anim_frame {
	int16		offset;
	uint8		blur;
	uint8		zoom;
	uint8		noise;
	uint8		plans;
	int8		delay;
	uint8		_rsvd_;
	anim_rect	erase;
	anim_rect	draw;
} anim_frame;

typedef struct anim_header {
	uint8		frame_count;
	uint8		buffer_v;
	uint8		off1_v;
	uint8		off2_v;
	uint16		off1_h;
	uint16		off2_h;
	uint16		buffer_h;
	uint16		front_margin;
	uint16		total_margin;
	uint16		bottom_offset;
	uint16		_rsvd_;
	anim_rect	rect2;
	anim_frame	*frames;
	anim_buffer buf[3];
} anim_header;

anim_header	*init_animation(char *path);
void		free_animation(anim_header *header);
int32		get_next_frame(char *data, int32 rowbyte, anim_frame *frame, anim_header *header);

#endif
