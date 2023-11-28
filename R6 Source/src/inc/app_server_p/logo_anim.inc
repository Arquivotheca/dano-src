#include "logo_anim.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void draw_blur(char *out_data, int32 out_row, anim_buffer *buf, int32 blur, int32 zoom, int32 noise) {
	int32	i, v, count, index, index2, max, mask;
	int32	val, val2, crc_alea;
	uint8	*in;
	uint16	*color;
	uint16	*out;
	
	v = buf->v;
	in = (uint8*)buf->data;
	color = buf->color;	 
	count = (2<<blur)-1;
	max = ((buf->h<<7)/zoom)<<1;
	crc_alea = 0x2ef719e5;
	noise |= noise<<16;
	do {
		val = 0;
		index = 0;
		index2 = 0;
		out = (uint16*)out_data;
		i = max+count-1;
		do {
			mask = (count-i)>>31;
			val2 = in[index>>7];
			val += ((val2<<12)|val2)&(0xf000f&mask);
			index += zoom & mask;
			mask = (i-max)>>31;
			val2 = in[index2>>7];
			val -= ((val2<<12)|val2)&(0xf000f&mask);
			index2 += zoom & mask;
			val2 = (val>>blur)&((crc_alea>>7)|crc_alea|noise);
			*(uint16*)(((char*)out)+out_row) = color[val2>>16];
			*out++ = color[val2&0xf];
			crc_alea <<= 1;
			crc_alea ^= (crc_alea>>31)&0x1fe36e1b;
			i-=2;
		} while (i>0);
		in += buf->h;
		out_data += 2*out_row;
		v--;
	} while (v > 0);
}

static void erase_rect(char *data, int32 rowbyte, int32 dh, int32 dv) {
	for (;dv>0; dv--) {
		memset(data, 0, dh*2);
		data += rowbyte;
	}
}

anim_header *init_animation(char *path) {
	int32		i;
	FILE		*fp;
	anim_header	*header;
	
	header = NULL;
	fp = fopen(path, "rb");
	if ((header = (anim_header*)malloc(sizeof(anim_header))) == NULL)
		goto error_exit_quick;
	if (fread((char*)header, sizeof(anim_header), 1, fp) != 1)
		goto error_exit_quick;
	header->frames = NULL;
	header->buf[0].data = NULL;
	header->buf[1].data = NULL;
	header->buf[2].data = NULL;
	if ((header->frames = (anim_frame*)malloc(sizeof(anim_frame)*header->frame_count)) == NULL)
		goto error_exit;
	if (fread((char*)header->frames, sizeof(anim_frame)*header->frame_count, 1, fp) != 1)
		goto error_exit;
	for (i=0; i<3; i++) {
		if ((header->buf[i].data = (char*)malloc(header->buf[i].length)) == NULL)
			goto error_exit;
		if (fread(header->buf[i].data, header->buf[i].length, 1, fp) != 1)
			goto error_exit;
	}
	fclose(fp);
	return header;
	
error_exit:
	if (header) {
		if (header->frames)
			free(header->frames);
		for (i=0; i<3; i++)
			if (header->buf[i].data)
				free(header->buf[i].data);
error_exit_quick:
		free(header);
	}
	return NULL;
}

void free_animation(anim_header *header) {
	int32		i;

	if (header) {
		for (i=0; i<3; i++)
			free(header->buf[i].data);
		free(header->frames);
		free(header);
	}
}

int32 get_next_frame(char *data, int32 rowbyte, anim_frame *frame, anim_header *header) {
	if (frame->plans&8)
		erase_rect(data+header->rect2.h*2+header->rect2.v*rowbyte, rowbyte,
				   header->rect2.dh, header->rect2.dv);
	if (frame->plans&16)
		erase_rect(data+frame->erase.h*2+frame->erase.v*rowbyte, rowbyte,
				   frame->erase.dh, frame->erase.dv);
	if (frame->plans&2)
		draw_blur(data+frame->offset*2 + header->off1_v*rowbyte+header->off1_h*2, rowbyte, header->buf+1,
				  frame->blur, frame->zoom*4, frame->noise);
	if (frame->plans&1)
		draw_blur(data+frame->offset*2, rowbyte, header->buf+0,
				  frame->blur, frame->zoom*4, frame->noise);
	if (frame->plans&4)
		draw_blur(data + header->off2_v*rowbyte+header->off2_h*2, rowbyte, header->buf+2,
				  frame->blur, 128, 15);
	return 50000*(int32)frame->delay;
}

