
#ifndef SHARED_DEFS
#define SHARED_DEFS

#ifdef __cplusplus
extern "C" {
#endif

#define AS_OVERLAY_BITMAP 0x00000004

enum {
	CURSOR_SYSTEM_DEFAULT = 1,
	CURSOR_I_BEAM = 2
};

/*----------------------------------------------------------------*/

typedef struct {
	long		video_mode;
	long		bit_per_pixel;
	long		h_size;
	long		v_size;
	char		*base;
	long		rowbyte;
	long		byte_jump;
	ulong		space;
	float		min_rate;
	float		max_rate;
	float		cur_rate;
	uchar		h_pos;
	uchar		v_pos;
	uchar		width;
	uchar		height;
} screen_desc;

#ifdef __cplusplus
}
#endif

#endif
