#ifndef __BANSHEE_REGS_H__
#define __BANSHEE_REGS_H__

// 2D
#define V5_2D_STATUS							0x100000
#define V5_2D_INTR_CTRL							0x100004
#define V5_2D_CLIP_0_MIN						0x100008
#define V5_2D_CLIP_0_MAX						0x10000C
#define V5_2D_DST_BASE_ADDR						0x100010
#define V5_2D_DST_FORMAT						0x100014
#define V5_2D_SRC_COLOR_KEY_MIN					0x100018
#define V5_2D_SRC_COLOR_KEY_MAX					0x10001C
#define V5_2D_DST_COLOR_KEY_MIN					0x100020
#define V5_2D_DST_COLOR_KEY_MAX					0x100024
#define V5_2D_BRES_ERROR_0						0x100028
#define V5_2D_BRES_ERROR_1						0x10002C
#define V5_2D_ROP								0x100030
#define V5_2D_SRC_BASE_ADDR						0x100034
#define V5_2D_COMMAND_EXTRA						0x100038
#define V5_2D_LINE_STIPPLE						0x10003C
#define V5_2D_LINE_STYLE						0x100040
#define V5_2D_PATTERN_0_ALIAS					0x100044
#define V5_2D_PATTERN_1_ALIAS					0x100048
#define V5_2D_CLIP_1_MIN						0x10004C
#define V5_2D_CLIP_1_MAX						0x100050
#define V5_2D_SRC_FORMAT						0x100054
#define V5_2D_SRC_SIZE							0x100058
#define V5_2D_SRC_X_Y							0x10005C
#define V5_2D_COLOR_BACK						0x100060
#define V5_2D_COLOR_FORE						0x100064
#define V5_2D_DST_SIZE							0x100068
#define V5_2D_DST_X_Y							0x10006C
#define V5_2D_COMMAND							0x100070

// 3D
#define V5_3D_STATUS							0x200000
//#define V5_3D_SWAP_BUFFER_CMD					0x200128
//#define V5_3D_CLIP_LEFT_RIGHT					0x200118

#define V5_3D_FBZ_COLOR_PATH					0x200104
#define V5_3D_FOG_MODE							0x200108
#define V5_3D_ALPHA_MODE						0x20010c
#define V5_3D_FBZ_MODE							0x200110
#define V5_3D_LFB_MODE							0x200114
#define V5_3D_CLIP_LEFT_RIGHT					0x200118
#define V5_3D_CLIP_TOP_BOTTOM					0x20011c

#define V5_3D_NOP_CMD							0x200120
#define V5_3D_FASTFILL_CMD						0x200124
#define V5_3D_SWAPBUFFER_CMD					0x200128
#define V5_3D_FOG_COLOR							0x20012c
#define V5_3D_ZA_COLOR							0x200130
#define V5_3D_COLOR_0							0x200144
#define V5_3D_COLOR_1							0x200148

#define V5_3D_RENDER_MODE						0x2001e0
#define V5_3D_STENCIL_MODE						0x2001e4
#define V5_3D_STENCIL_OP						0x2001e8
#define V5_3D_COL_BUFFER_ADDR					0x2001ec
#define V5_3D_COL_BUFFER_STRIDE					0x2001f0
#define V5_3D_AUX_BUFFER_ADDR					0x2001f4
#define V5_3D_AUX_BUFFER_STRIDE					0x2001f8

#define V5_3D_CLIP_LEFT_RIGHT1					0x200200
#define V5_3D_CLIP_TOP_BOTTOM1					0x200204
#define V5_3D_COMBINE_MODE						0x200208
#define V5_3D_SLI_CTRL							0x20020c
#define V5_3D_AA_CTRL							0x200210
#define V5_3D_CHIP_MASK							0x200214
#define V5_3D_LEFT_DESKTOP_BUF					0x200218

#define V5_3D_LEFT_OVERLAY_BUF					0x200250
#define V5_3D_SWAP_BUFFER_PEND					0x20024c

#define V5_3D_TEX_MODE							0x200300
#define V5_3D_TEX_LOD							0x200304
#define V5_3D_TEX_DETAIL						0x200308
#define V5_3D_TEX_BASE_ADDR						0x20030c
#define V5_3D_TEX_BASE_ADDR_1					0x200310
#define V5_3D_TEX_BASE_ADDR_2					0x200314
#define V5_3D_TEX_BASE_ADDR_3_8					0x200318
#define V5_3D_TEX_STRIDE						0x20031c
#define V5_3D_TEX_INIT1							0x200320


// Video
#define V5_VID_STATUS							0x0000
#define V5_VID_MISC_INIT_0						0x0010
#define V5_VID_MISC_INIT_1						0x0014
#define V5_VID_DRAM_INIT_0						0x0018
#define V5_VID_DRAM_INIT_1						0x001C
#define V5_VID_VGA_INIT_0						0x0028
#define V5_VID_VGA_INIT_1						0x002C
#define V5_VID_TV_OUT_BLANK_V_COUNT				0x003C
#define V5_VID_PLL_CTRL_0						0x0040
#define V5_VID_PLL_CTRL_1						0x0044
#define V5_VID_PLL_CTRL_2						0x0048
#define V5_VID_DAC_MODE							0x004C
#define V5_VID_DAC_ADDR							0x0050
#define V5_VID_DAC_DATA							0x0054
#define V5_VID_MAX_RGB_DELTA					0x0058
#define V5_VID_PROC_CFG							0x005C
#define V5_VID_CUR_PAT_ADDR						0x0060
#define V5_VID_CUR_LOC							0x0064
#define V5_VID_CUR_C_0							0x0068
#define V5_VID_CUR_C_1							0x006C
#define V5_VID_IN_FORMAT						0x0070
#define V5_VID_TV_OUT_BLANK_H_COUNT				0x0074
#define V5_VID_SERIAL_PARALLEL_PORT				0x0078
#define V5_VID_IN_X_DECIM_DELTAS				0x007C
#define V5_VID_IN_DECIM_INIT_ERRS				0x0080
#define V5_VID_IN_Y_DECIM_DELTAS				0x0084
#define V5_VID_PIXEL_BUF_T_HOLD					0x0088
#define V5_VID_CHROMA_MIN						0x008C
#define V5_VID_CHROMA_MAX						0x0090
#define V5_VID_IN_STATUS_CURRENT_LINE			0x0094
#define V5_VID_SCREEN_SIZE						0x0098
#define V5_VID_OVERLAY_START_COORDS				0x009C
#define V5_VID_OVERLAY_END_SCREEN_COORD			0x00A0
#define V5_VID_OVERLAY_DUDX						0x00A4
#define V5_VID_OVERLAY_DUDX_OFFSET_SRC_WIDTH	0x00A8
#define V5_VID_OVERLAY_DVDY						0x00AC
#define V5_VID_OVERLAY_DVDY_OFFSET				0x00E0
#define V5_VID_DESKTOP_START_ADDR				0x00E4
#define V5_VID_DESKTOP_OVERLAY_STRIDE			0x00E8
#define V5_VID_IN_ADDR_0						0x00EC
#define V5_VID_IN_ADDR_1						0x00F0
#define V5_VID_IN_ADDR_2						0x00F4
#define V5_VID_IN_STRIDE						0x00F8
#define V5_VID_CUR_OVERLAY_START_ADDR			0x00FC

// CMD & AGP
#define V5_AGP_REQ_SIZE							0x80000
#define V5_AGP_HOST_ADDRESS_LOW					0x80004
#define V5_AGP_HOST_ADDRESS_HIGH				0x80008
#define V5_AGP_GRAPHICS_ADDRESS					0x8000C
#define V5_AGP_GRAPHICS_STRIDE					0x80010
#define V5_AGP_MOVE_CMD							0x80014
#define V5_CMD_BASE_ADDR_0						0x80020
#define V5_CMD_BASE_SIZE_0						0x80024
#define V5_CMD_BUMP_0							0x80028
#define V5_CMD_RD_PTR_L_0						0x8002C
#define V5_CMD_RD_PTR_H_0						0x80030
#define V5_CMD_A_MIN_0							0x80034
#define V5_CMD_A_MAX_0							0x8003C
#define V5_CMD_STATUS_0							0x80040
#define V5_CMD_FIFO_DEPTH_0						0x80044
#define V5_CMD_FIFO_HOLE_CNT_0					0x80048
#define V5_CMD_BASE_ADDR_1						0x80050
#define V5_CMD_BASE_SIZE_1						0x80054
#define V5_CMD_BUMP_1							0x80058
#define V5_CMD_RD_PTR_L_1						0x8005C
#define V5_CMD_RD_PTR_H_1						0x80060
#define V5_CMD_A_MIN_1							0x80064
#define V5_CMD_A_MAX_1							0x8006C
#define V5_CMD_STATUS_1							0x80070
#define V5_CMD_FIFO_DEPTH_1						0x80074
#define V5_CMD_FIFO_HOLE_CNT_1					0x80078
#define V5_CMD_FIFO_THRESH						0x80080
#define V5_CMD_HOLE_INT							0x80084



#endif
