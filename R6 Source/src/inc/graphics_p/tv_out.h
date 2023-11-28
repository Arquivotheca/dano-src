#if !defined(_TV_OUT_H_)
#define _TV_OUT_H_


#include <Accelerant.h>

/* the TV_OUT field of display_mode.flags is 4 bits wide */
enum {
	B_TV_OUT_NONE	= 0x0 << 27,
	B_TV_OUT_NTSC	= 0x1 << 27,
	B_TV_OUT_NTSC_J	= 0x2 << 27,
	B_TV_OUT_PAL	= 0x3 << 27,
	B_TV_OUT_PAL_M	= 0x4 << 27,
	/* HDTV modes go here.  Do we need more bits? */
	B_TV_OUT_MASK	= 0xf << 27
};

/* features */
enum {
	B_GET_TV_OUT_ADJUSTMENTS_FOR_MODE = 0x0810000,
	B_GET_TV_OUT_ADJUSTMENTS,
	B_SET_TV_OUT_ADJUSTMENTS
};

enum {
	B_TV_OUT_H_POS			= 1 << 0,
	B_TV_OUT_V_POS			= 1 << 1,
	B_TV_OUT_START_ACTIVE	= 1 << 2,
	B_TV_OUT_BLACK_LEVEL	= 1 << 3,
	B_TV_OUT_CONTRAST		= 1 << 4
	/* other parms to follow */
};

typedef struct {
	uint16	max_value;
	uint16	default_value;
} tv_out_limits;

/* the following two structures must be kept in sync */
typedef struct {
	tv_out_limits	h_pos;
	tv_out_limits	v_pos;
	tv_out_limits	start_active_video;
	tv_out_limits	black_level;
	tv_out_limits	contrast;
	tv_out_limits	_UNUSED[27];
} tv_out_adjustment_limits;

typedef struct {
	uint16	h_pos;
	uint16	v_pos;
	uint16	start_active_video;
	uint16	black_level;
	uint16	contrast;
	uint16	_UNUSED[27];
} tv_out_adjustments;

/* For a given display mode, return the adjustments and ranges available.  Returns which adjustments
in uint32 as a bitmask.  Returns ranges in *adj */
typedef status_t (*get_tv_out_adjustments_for_mode)(const display_mode *dm, uint32 *which, tv_out_adjustment_limits *adj);
/* retrieve the tv out parameters for the currently active display mode */
typedef status_t (*get_tv_out_adjustments)(uint32 *which, tv_out_adjustments *adj);
/* adjust the tv out parameters for the currently active display mode */
typedef status_t (*set_tv_out_adjustments)(uint32  which, tv_out_adjustments *adj);

#endif
