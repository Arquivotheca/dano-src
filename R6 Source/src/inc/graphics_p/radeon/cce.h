/******************************************************************************
 * defines file for Radeon  CCE subsystem                                     *
 *                                                                            *
 * Copyright (c) 2000 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/

#ifndef _CCE_H
#define _CCE_H

// CCE operating mode constants for Radeon_CCEInit ()

#define CSQ_MODE_PRIDIS_INDDIS                     0
#define CSQ_MODE_PRIPIO_INDDIS                     1
#define CSQ_MODE_PRIBM__INDDIS                     2
#define CSQ_MODE_PRIPIO_INDBM                      3
#define CSQ_MODE_PRIBM__INDBM                      4
#define CSQ_MODE_PRIPIO_INDPIO                     5

// CCE shutdown constants for Radeon_CCEEnd ()

#define CCE_END_WAIT                        0
#define CCE_END_NOWAIT                      1

// Type-0, -1, and -2 packet headers

#define CCE_PACKET0                         0x00000000
#define CCE_PACKET1                         0x40000000
#define CCE_PACKET2                         0x80000000
#define CCE_PACKET_0_ONE_REG_WR	           (0x00000001 << 15)

// IT_OPCODEs for type-3 packets without the GUI_CNTL block.

#define CCE_PACKET3_NOP                     0xC0001000
#define CCE_PACKET3_PAINT                   0xC0001100
#define CCE_PACKET3_BITBLT                  0xC0001200
#define CCE_PACKET3_SMALLTEXT               0xC0001300
#define CCE_PACKET3_HOSTDATA_BLT            0xC0001400
#define CCE_PACKET3_POLYLINE                0xC0001500
#define CCE_PACKET3_POLYSCANLINES           0xC0001800
#define CCE_PACKET3_NEXT_CHAR               0xC0001900
#define CCE_PACKET3_PAINT_MULTI             0xC0001A00
#define CCE_PACKET3_BITBLT_MULTI            0xC0001B00
#define CCE_PACKET3_PLY_NEXTSCAN            0xC0001D00
#define CCE_PACKET3_SET_SCISSORS            0xC0001E00

// IT_OPCODEs for type-3 packets with the GUI_CNTL block.

#define CCE_PACKET3_CNTL_PAINT              0xC0009100
#define CCE_PACKET3_CNTL_BITBLT             0xC0009200
#define CCE_PACKET3_CNTL_SMALLTEXT          0xC0009300
#define CCE_PACKET3_CNTL_HOSTDATA_BLT       0xC0009400
#define CCE_PACKET3_CNTL_POLYLINE           0xC0009500
#define CCE_PACKET3_CNTL_POLYSCANLINES      0xC0009800
#define CCE_PACKET3_CNTL_NEXT_CHAR          0xC0009900
#define CCE_PACKET3_CNTL_PAINT_MULTI        0xC0009A00
#define CCE_PACKET3_CNTL_BITBLT_MULTI       0xC0009B00
#define CCE_PACKET3_CNTL_TRANS_BITBLT       0xC0009C00

// IT_OPCODEs for 3D packets

#define CCE_PACKET3_3D_RNDR_GEN_INDX_PRIM   0xC0002300
#define CCE_PACKET3_LOAD_MICROCODE          0xC0002400
#define CCE_PACKET3_3D_RNDR_GEN_PRIM        0xC0002500
#define CCE_PACKET3_WAIT_FOR_IDLE           0xC0002600
#define CCE_PACKET3_3D_DRAW_VBUF            0xC0002800
#define CCE_PACKET3_3D_DRAW_IMMD            0xC0002900
#define CCE_PACKET3_3D_DRAW_INDX            0xC0002A00
#define CCE_PACKET3_LOAD_PALETTE            0xC0002C00
#define CCE_PACKET3_PURGE                   0xC0002D00
#define CCE_PACKET3_NEXT_VERTEX_BUNDLE      0xC0002E00
#define CCE_PACKET3_3D_LOAD_VBPNTR          0xC0002F00
#define CCE_PACKET3_3D_CLEAR_ZMASK          0xC0003200

// Status codes for CCE library functions

#define CCE_SUCCESS                 0       // Success
#define CCE_FAIL_BUS_MASTER_INIT    1       // Unable to set up Bus Mastering
#define CCE_FAIL_INVALID_CCE_MODE   2       // Invalid PM4_BUFFER_CNTL mode
#define CCE_FAIL_BAD_PACKET         3       // Bad packet was submitted
#define CCE_FAIL_TIMEOUT            4       // CCE FIFO/Idle timeout

// Settings GUI_CNTL bit constants for CCE_PACKET3_CNTL_* packets.
// Similar to DP_GUI_MASTER_CNTL constants.

#define CCE_GC_SRC_PITCH_OFFSET_DEFAULT     0x00000000
#define CCE_GC_SRC_PITCH_OFFSET_SUPPLIED    0x00000001
#define CCE_GC_DST_PITCH_OFFSET_DEFAULT     0x00000000
#define CCE_GC_DST_PITCH_OFFSET_SUPPLIED    0x00000002
#define CCE_GC_SRC_CLIP_DEFAULT             0x00000000
#define CCE_GC_SRC_CLIP_SUPPLIED            0x00000004
#define CCE_GC_DST_CLIP_DEFAULT             0x00000000
#define CCE_GC_DST_CLIP_SUPPLIED            0x00000008
#define CCE_GC_BRUSH_8x8MONO                0x00000000
#define CCE_GC_BRUSH_8x8MONO_LBKGD          0x00000010
#define CCE_GC_BRUSH_32x1MONO               0x00000060
#define CCE_GC_BRUSH_32x1MONO_LBKGD         0x00000070
#define CCE_GC_BRUSH_8x8COLOR               0x000000A0
#define CCE_GC_BRUSH_SOLIDCOLOR             0x000000D0
#define CCE_GC_BRUSH_SOLIDCOLOR_LINES       0x000000E0
#define CCE_GC_BRUSH_NONE                   0x000000F0
#define CCE_GC_DST_8BPP                     0x00000200
#define CCE_GC_DST_15BPP                    0x00000300
#define CCE_GC_DST_16BPP                    0x00000400
#define CCE_GC_DST_32BPP                    0x00000600
#define CCE_GC_DST_8BPP_RGB332              0x00000700
#define CCE_GC_DST_8BPP_Y8                  0x00000800
#define CCE_GC_DST_8BPP_RGB8                0x00000900
#define CCE_GC_DST_16BPP_VYUY422            0x00000B00
#define CCE_GC_DST_16BPP_YVYU422            0x00000C00
#define CCE_GC_DST_32BPP_AYUV444            0x00000E00
#define CCE_GC_DST_16BPP_ARGB4444           0x00000F00
#define CCE_GC_SRC_MONO                     0x00000000
#define CCE_GC_SRC_MONO_LBKGD               0x00001000
#define CCE_GC_SRC_DSTCOLOR                 0x00003000
#define CCE_GC_BYTE_ORDER_MSB_TO_LSB        0x00000000
#define CCE_GC_BYTE_ORDER_LSB_TO_MSB        0x00004000
#define CCE_GC_DP_CONVERSION_TEMP_9300      0x00008000
#define CCE_GC_DP_CONVERSION_TEMP_6500      0x00000000
#define CCE_GC_DP_SRC_RECT                  0x02000000
#define CCE_GC_DP_SRC_HOST                  0x03000000
#define CCE_GC_DP_SRC_HOST_BYTEALIGN        0x04000000
#define CCE_GC_SRC_4BPP_CLUT_DEFAULT        0x08000000
#define CCE_GC_SRC_4BPP_CLUT_APPLIED        0x08000000
#define CCE_GC_DST_CLR_CMP_FCN_LEAVE        0x00000000
#define CCE_GC_DST_CLR_CMP_FCN_CLEAR        0x10000000
#define CCE_GC_WRITE_MASK_LEAVE             0x00000000
#define CCE_GC_WRITE_MASK_SET               0x40000000
#define CCE_GC_BRUSH_Y_X_DEFAULT            0x00000000
#define CCE_GC_BRUSH_Y_X_SUPPLIED           0x80000000

// VC_FORMAT constants.

#define CCE_VC_FRMT_RHW			0x00000001
#define CCE_VC_FRMT_DIFFUSE_BGR		0x00000002
#define CCE_VC_FRMT_DIFFUSE_A		0x00000004
#define CCE_VC_FRMT_DIFFUSE_ARGB	0x00000008
#define CCE_VC_FRMT_SPEC_BGR		0x00000010
#define CCE_VC_FRMT_SPEC_F		0x00000020
#define CCE_VC_FRMT_SPEC_FRGB		0x00000040
#define CCE_VC_FRMT_S_T			0x00000080
#define CCE_VC_FRMT_S2_T2		0x00000100
#define CCE_VC_FRMT_RHW2		0x00000200

#define CCE_VC_CNTL_PRIM_TYPE_NONE	0x00000000
#define CCE_VC_CNTL_PRIM_TYPE_POINT	0x00000001
#define CCE_VC_CNTL_PRIM_TYPE_LINE	0x00000002
#define CCE_VC_CNTL_PRIM_TYPE_POLY_LINE	0x00000003
#define CCE_VC_CNTL_PRIM_TYPE_TRI_LIST	0x00000004
#define CCE_VC_CNTL_PRIM_TYPE_TRI_FAN	0x00000005
#define CCE_VC_CNTL_PRIM_TYPE_TRI_STRIP	0x00000006
#define CCE_VC_CNTL_PRIM_TYPE_TRI_TYPE2	0x00000007

#define CCE_VC_CNTL_PRIM_WALK_IND	0x00000010
#define CCE_VC_CNTL_PRIM_WALK_LIST	0x00000020
#define CCE_VC_CNTL_PRIM_WALK_RING	0x00000030

#endif // _CCE_H
