/* DVDefs.h */

#ifndef DV_DEFS_H
#define DV_DEFS_H

#define	DV_NTSC_WIDTH				720
#define DV_NTSC_HEIGHT				480
#define DV_NTSC_BPR					(DV_NTSC_WIDTH * 4)
#define DV_NTSC_VIDEO_SIZE			(DV_NTSC_BPR * DV_NTSC_HEIGHT)
#define DV_NTSC_ENCODED_FRAME_SIZE	120000

extern media_encoded_video_format dv_ntsc_format;

#define	DV_PAL_WIDTH				720
#define DV_PAL_HEIGHT				576
#define DV_PAL_BPR					(DV_PAL_WIDTH * 4)
#define DV_PAL_VIDEO_SIZE			(DV_PAL_BPR * DV_PAL_HEIGHT)
#define DV_PAL_ENCODED_FRAME_SIZE	144000

extern media_encoded_video_format dv_pal_format;

#endif
