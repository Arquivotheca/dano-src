/******************************************************************************
 * REGDEF.H                                                                   *
 *     Header file for Rage 128 Chapter 3 sample code                         *
 *     -all register offsets are expressed in bytes-                          *
 *                                                                            *
 * Copyright (c) 1999 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/

///////////////////////////////////////////////////////////
//
// FILE: defineR128.h
// DESC: defines for the Rage 128 card, direct from ATI
// 
/////////////////////////////////////////////////////////////


 
 
// define REGDEF_H for other files that include this file
#ifndef REGDEF_H
#define REGDEF_H

/******************************************************************************
 *                                                                            *
 *  RAGE128 REGISTER OFFSET CONSTANTS SECTION                                 *
 *                                                                            *
 ******************************************************************************/

/******************************************************************************
 *   VGA REGISTER CONSTANTS                                                   *
 ******************************************************************************/
#define ioGENMO_WT                                 0x03c2
#define ioGENMO_RD                                 0x03cc
#define ioGENFC_RD                                 0x03ca
#define ioGENFC_WT                                 0x03ba
#define ioGENFC_WT_ALT1                            0x03da
#define ioGENS0                                    0x03c2
#define ioGENS1                                    0x03ba
#define ioGENS1_ALT1                               0x03da
#define ioGENENB                                   0x03c3
#define ioDAC_DATA                                 0x03c9
#define ioDAC_MASK                                 0x03c6
#define ioDAC_R_INDEX                              0x03c7
#define ioDAC_W_INDEX                              0x03c8
#define ioSEQ8_IDX                                 0x03c4
#define ioSEQ8_DATA                                0x03c5
#define ioCRTC8_IDX                                0x03b4
#define ioCRTC8_IDX_ALT1                           0x03d4
#define ioCRTC8_DATA                               0x03b5
#define ioCRTC8_DATA_ALT1                          0x03d5
#define ioGRPH8_IDX                                0x03ce
#define ioGRPH8_DATA                               0x03cf
#define ioATTRX                                    0x03c0
#define ioATTRDR                                   0x03c1
#define ioATTRDW                                   0x03c0

/******************************************************************************
 *               NON-GUI Block I/O Mapped Registers                           *
 ******************************************************************************/
#define ioMM_INDEX                                 0x0000
#define ioMM_DATA                                  0x0004
#define ioCLOCK_CNTL_INDEX                         0x0008
#define ioCLOCK_CNTL_DATA                          0x000c
#define ioBIOS_0_SCRATCH                           0x0010
#define ioBIOS_1_SCRATCH                           0x0014
#define ioBIOS_2_SCRATCH                           0x0018
#define ioBIOS_3_SCRATCH                           0x001c
#define ioBUS_CNTL                                 0x0030
#define ioMEM_VGA_WP_SEL                           0x0038
#define ioMEM_VGA_RP_SEL                           0x003c
#define ioGEN_INT_CNTL                             0x0040
#define ioGEN_INT_STATUS                           0x0044
#define ioCRTC_GEN_CNTL                            0x0050
#define ioCRTC_EXT_CNTL                            0x0054
#define ioDAC_CNTL                                 0x0058
#define ioCRTC_STATUS                              0x005c
#define ioPALETTE_INDEX                            0x00b0
#define ioPALETTE_DATA                             0x00b4
#define ioCONFIG_CNTL                              0x00e0
#define ioCONFIG_XSTRAP                            0x00e4
#define ioCONFIG_BONDS                             0x00e8
#define ioGEN_RESET_CNTL                           0x00f0
#define ioGEN_STATUS                               0x00f4
#define ioCONFIG_MEMSIZE                           0x00f8


/******************************************************************************
 *               NON-GUI Block Memory Mapped Registers                        *
 ******************************************************************************/
#define MM_INDEX                                   0x0000
#define MM_DATA                                    0x0004
#define CLOCK_CNTL_INDEX                           0x0008
#define CLOCK_CNTL_DATA                            0x000c
#define BIOS_0_SCRATCH                             0x0010
#define BIOS_1_SCRATCH                             0x0014
#define BIOS_2_SCRATCH                             0x0018
#define BIOS_3_SCRATCH                             0x001c
#define BUS_CNTL                                   0x0030
#define BUS_CNTL1                                  0x0034
#define MEM_VGA_WP_SEL                             0x0038
#define MEM_VGA_RP_SEL                             0x003c
#define GEN_INT_CNTL                               0x0040
#define GEN_INT_STATUS                             0x0044
#define CRTC_GEN_CNTL                              0x0050
#define CRTC_EXT_CNTL                              0x0054
#define DAC_CNTL                                   0x0058
#define CRTC_STATUS                                0x005c
#define GPIO_MONID                                 0x0068
#define I2C_CNTL_1                                 0x0094
#define PALETTE_INDEX                              0x00b0
#define PALETTE_DATA                               0x00b4
#define CONFIG_CNTL                                0x00e0
#define CONFIG_XSTRAP                              0x00e4
#define CONFIG_BONDS                               0x00e8
#define GEN_RESET_CNTL                             0x00f0
#define GEN_STATUS                                 0x00f4
#define CONFIG_MEMSIZE                             0x00f8
#define CONFIG_APER_0_BASE                         0x0100
#define CONFIG_APER_1_BASE                         0x0104
#define CONFIG_APER_SIZE                           0x0108
#define CONFIG_REG_1_BASE                          0x010c
#define CONFIG_REG_APER_SIZE                       0x0110
#define CONFIG_MEMSIZE_EMBEDDED                    0x0114
#define TEST_DEBUG_CNTL                            0x0120
#define TEST_DEBUG_MUX                             0x0124
#define HW_DEBUG                                   0x0128
#define TEST_DEBUG_OUT                             0x012c
#define HOST_PATH_CNTL                             0x0130
#define SW_SEMAPHORE                               0x013c
#define MEM_CNTL                                   0x0140
#define EXT_MEM_CNTL                               0x0144
#define MEM_ADDR_CONFIG                            0x0148
#define MEM_INTF_CNTL                              0x014c
#define MEM_STR_CNTL                               0x0150
#define MEM_INIT_LAT_TIMER                         0x0154
#define MEM_SDRAM_MODE_REG                         0x0158
#define AGP_BASE                                   0x0170
#define AGP_CNTL                                   0x0174
#define AGP_APER_OFFSET                            0x0178
#define PCI_GART_PAGE                              0x017c
#define PC_NGUI_MODE                               0x0180
#define PC_NGUI_CTLSTAT                            0x0184
#define VIDEOMUX_CNTL                              0x0190
#define MPP_TB_CONFIG                              0x01C0
#define MPP_GP_CONFIG                              0x01C8
#define VIPH_CONTROL                               0x01D0
#define CRTC_H_TOTAL_DISP                          0x0200
#define CRTC_H_SYNC_STRT_WID                       0x0204
#define CRTC_V_TOTAL_DISP                          0x0208
#define CRTC_V_SYNC_STRT_WID                       0x020c
#define CRTC_VLINE_CRNT_VLINE                      0x0210
#define CRTC_CRNT_FRAME                            0x0214
#define CRTC_GUI_TRIG_VLINE                        0x0218
#define CRTC_DEBUG                                 0x021c
#define CRTC_OFFSET                                0x0224
#define CRTC_OFFSET_CNTL                           0x0228
#define CRTC_PITCH                                 0x022c
#define OVR_CLR                                    0x0230
#define OVR_WID_LEFT_RIGHT                         0x0234
#define OVR_WID_TOP_BOTTOM                         0x0238
#define SNAPSHOT_VH_COUNTS                         0x0240
#define SNAPSHOT_F_COUNT                           0x0244
#define N_VIF_COUNT                                0x0248
#define SNAPSHOT_VIF_COUNT                         0x024c
#define CUR_OFFSET                                 0x0260
#define CUR_HORZ_VERT_POSN                         0x0264
#define CUR_HORZ_VERT_OFF                          0x0268
#define CUR_CLR0                                   0x026c
#define CUR_CLR1                                   0x0270
#define DAC_CRC_SIG                                0x02cc
#define DDA_CONFIG                                 0x02e0
#define DDA_ON_OFF                                 0x02e4
#define VGA_DDA_CONFIG                             0x02e8
#define VGA_DDA_ON_OFF                             0x02ec
#define OV0_Y_X_START                              0x0400
#define OV0_Y_X_END                                0x0404
#define OV0_EXCLUSIVE_HORZ                         0x0408
#define OV0_EXCLUSIVE_VERT                         0x040c
#define OV0_REG_LOAD_CNTL                          0x0410
#define OV0_SCALE_CNTL                             0x0420
#define OV0_V_INC                                  0x0424
#define OV0_P1_V_ACCUM_INIT                        0x0428
#define OV0_P23_V_ACCUM_INIT                       0x042c
#define OV0_P1_BLANK_LINES_AT_TOP                  0x0430
#define OV0_P23_BLANK_LINES_AT_TOP                 0x0434
#define OV0_VID_BUF0_BASE_ADRS                     0x0440
#define OV0_VID_BUF1_BASE_ADRS                     0x0444
#define OV0_VID_BUF2_BASE_ADRS                     0x0448
#define OV0_VID_BUF3_BASE_ADRS                     0x044c
#define OV0_VID_BUF4_BASE_ADRS                     0x0450
#define OV0_VID_BUF5_BASE_ADRS                     0x0454
#define OV0_VID_BUF_PITCH0_VALUE                   0x0460
#define OV0_VID_BUF_PITCH1_VALUE                   0x0464
#define OV0_OCTWORDS_PER_LINE_M1                   0x046c
#define OV0_AUTO_FLIP_CNTRL                        0x0470
#define OV0_DEINTERLACE_PATTERN                    0x0474
#define OV0_H_INC                                  0x0480
#define OV0_STEP_BY                                0x0484
#define OV0_P1_H_ACCUM_INIT                        0x0488
#define OV0_P23_H_ACCUM_INIT                       0x048c
#define OV0_P1_X_START_END                         0x0494
#define OV0_P2_X_START_END                         0x0498
#define OV0_P3_X_START_END                         0x049c
#define OV0_FILTER_CNTL                            0x04a0
#define OV0_FOUR_TAP_COEF_0                        0x04b0
#define OV0_FOUR_TAP_COEF_1                        0x04b4
#define OV0_FOUR_TAP_COEF_2                        0x04b8
#define OV0_FOUR_TAP_COEF_3                        0x04bc
#define OV0_FOUR_TAP_COEF_4                        0x04c0
#define OV0_COLOR_CNTL                             0x04e0
#define OV0_VIDEO_KEY_CLR                          0x04e4
#define OV0_VIDEO_KEY_MASK                         0x04e8
#define OV0_GRAPHICS_KEY_CLR                       0x04ec
#define OV0_GRAPHICS_KEY_MASK                      0x04f0
#define OV0_KEY_CNTL                               0x04f4
#define OV0_TEST                                   0x04f8
#define SUBPIC_CNTL                                0x0540
#define PM4_BUFFER_OFFSET                          0x0700
#define PM4_BUFFER_CNTL                            0x0704
#define PM4_BUFFER_WM_CNTL                         0x0708
#define PM4_BUFFER_DL_RPTR_ADDR                    0x070c
#define PM4_BUFFER_DL_RPTR                         0x0710
#define PM4_BUFFER_DL_WPTR                         0x0714
#define PM4_VC_FPU_SETUP                           0x071c
#define PM4_FPU_CNTL                               0x0720
#define PM4_VC_FORMAT                              0x0724
#define PM4_VC_CNTL                                0x0728
#define PM4_VC_I01                                 0x072c
#define PM4_VC_VLOFF                               0x0730
#define PM4_VC_VLSIZE                              0x0734
#define PM4_IW_INDOFF                              0x0738
#define PM4_IW_INDSIZE                             0x073c
#define PM4_FPU_FPX0                               0x0740
#define CRC_CMDFIFO_ADDR                           0x0740
#define PM4_FPU_FPY0                               0x0744
#define CRC_CMDFIFO_DOUT                           0x0744
#define PM4_FPU_FPX1                               0x0748
#define PM4_FPU_FPY1                               0x074c
#define PM4_FPU_FPX2                               0x0750
#define PM4_FPU_FPY2                               0x0754
#define PM4_FPU_FPY3                               0x0758
#define PM4_FPU_FPY4                               0x075c
#define PM4_FPU_FPY5                               0x0760
#define PM4_FPU_FPY6                               0x0764
#define PM4_FPU_FPR                                0x0768
#define PM4_FPU_FPG                                0x076c
#define PM4_FPU_FPB                                0x0770
#define PM4_FPU_FPA                                0x0774
#define PM4_FPU_INTXY0                             0x0780
#define PM4_FPU_INTXY1                             0x0784
#define PM4_FPU_INTXY2                             0x0788
#define PM4_FPU_INTARGB                            0x078c
#define PM4_FPU_FPTWICEAREA                        0x0790
#define PM4_FPU_DMAJOR01                           0x0794
#define PM4_FPU_DMAJOR12                           0x0798
#define PM4_FPU_DMAJOR02                           0x079c
#define PM4_FPU_STAT                               0x07a0
#define PM4_STAT                                   0x07b8
#define PM4_TEST_CNTL                              0x07d0
#define PM4_MICROCODE_ADDR                         0x07d4
#define PM4_MICROCODE_RADDR                        0x07d8
#define PM4_MICROCODE_DATAH                        0x07dc
#define PM4_MICROCODE_DATAL                        0x07e0
#define PM4_CMDFIFO_ADDR                           0x07e4
#define PM4_CMDFIFO_DATAH                          0x07e8
#define PM4_CMDFIFO_DATAL                          0x07ec
#define PM4_BUFFER_ADDR                            0x07f0
#define PM4_BUFFER_DATAH                           0x07f4
#define PM4_BUFFER_DATAL                           0x07f8
#define PM4_MICRO_CNTL                             0x07fc
#define VID_BUFFER_CONTROL                         0x0900
#define CAP_INT_CNTL                               0x0908
#define CAP_INT_STATUS                             0x090c
#define CAP0_BUF0_OFFSET                           0x0920
#define CAP0_BUF1_OFFSET                           0x0924
#define CAP0_BUF0_EVEN_OFFSET                      0x0928
#define CAP0_BUF1_EVEN_OFFSET                      0x092c
#define CAP0_BUF_PITCH                             0x0930
#define CAP0_V_WINDOW                              0x0934
#define CAP0_H_WINDOW                              0x0938
#define CAP0_VBI_ODD_OFFSET                        0x093c
#define CAP0_VBI_EVEN_OFFSET                       0x0940
#define CAP0_VBI_V_WINDOW                          0x0944
#define CAP0_VBI_H_WINDOW                          0x0948
#define CAP0_PORT_MODE_CNTL                        0x094c
#define CAP0_TRIG_CNTL                             0x0950
#define CAP0_DEBUG                                 0x0954
#define CAP0_CONFIG                                0x0958
#define CAP0_ANC_ODD_OFFSET                        0x095c
#define CAP0_ANC_EVEN_OFFSET                       0x0960
#define CAP0_ANC_H_WINDOW                          0x0964
#define CAP0_VIDEO_SYNC_TEST                       0x0968
#define CAP0_ONESHOT_BUF_OFFSET                    0x096c
#define CAP0_BUF_STATUS                            0x0970
#define CAP0_DWNSC_XRATIO                          0x0978
#define CAP0_XSHARPNESS                            0x097c
#define CAP1_BUF0_OFFSET                           0x0990
#define CAP1_BUF1_OFFSET                           0x0994
#define CAP1_BUF0_EVEN_OFFSET                      0x0998
#define CAP1_BUF1_EVEN_OFFSET                      0x099c
#define CAP1_BUF_PITCH                             0x09a0
#define CAP1_V_WINDOW                              0x09a4
#define CAP1_H_WINDOW                              0x09a8
#define CAP1_VBI_ODD_OFFSET                        0x09ac
#define CAP1_VBI_EVEN_OFFSET                       0x09b0
#define CAP1_VBI_V_WINDOW                          0x09b4
#define CAP1_VBI_H_WINDOW                          0x09b8
#define CAP1_PORT_MODE_CNTL                        0x09bc
#define CAP1_TRIG_CNTL                             0x09c0
#define CAP1_DEBUG                                 0x09c4
#define CAP1_CONFIG                                0x09c8
#define CAP1_ANC_ODD_OFFSET                        0x09cc
#define CAP1_ANC_EVEN_OFFSET                       0x09d0
#define CAP1_ANC_H_WINDOW                          0x09d4
#define CAP1_VIDEO_SYNC_TEST                       0x09d8
#define CAP1_ONESHOT_BUF_OFFSET                    0x09dc
#define CAP1_BUF_STATUS                            0x09e0
#define CAP1_DWNSC_XRATIO                          0x09e8
#define CAP1_XSHARPNESS                            0x09ec
#define BM_FRAME_BUF_OFFSET                        0x0a00
#define BM_SYSTEM_MEM_ADDR                         0x0a04
#define BM_COMMAND                                 0x0a08
#define BM_STATUS                                  0x0a0c
#define BM_QUEUE_STATUS                            0x0a10
#define BM_QUEUE_FREE_STATUS                       0x0A14
#define BM_CHUNK_0_VAL                             0x0a18
#define BM_CHUNK_1_VAL                             0x0a1C
#define BM_VIP0_BUF                                0x0A20
#define BM_VIP0_ACTIVE                             0x0A24
#define BM_VIP1_BUF                                0x0A30
#define BM_VIP1_ACTIVE                             0x0A34
#define BM_VIP2_BUF                                0x0A40
#define BM_VIP2_ACTIVE                             0x0A44
#define BM_VIP3_BUF                                0x0A50
#define BM_VIP3_ACTIVE                             0x0A54
#define BM_VIDCAP_BUF0                             0x0a60
#define BM_VIDCAP_BUF1                             0x0a64
#define BM_VIDCAP_BUF2                             0x0a68
#define BM_VIDCAP_ACTIVE                           0x0a6c
#define BM_GUI                                     0x0a80
#define SURFACE_DELAY                              0x0b00

/******************************************************************************
 * PCI Config Space Alias Block Memory Mapped Registers (header only)         *
 ******************************************************************************/
#define PCIHDR_VENDOR_ID                           0x0f00
#define PCIHDR_DEVICE_ID                           0x0f02
#define PCIHDR_COMMAND                             0x0f04
#define PCIHDR_STATUS                              0x0f06
#define PCIHDR_REVISION_ID                         0x0f08
#define PCIHDR_REGPROG_INF                         0x0f09
#define PCIHDR_SUB_CLASS                           0x0f0a
#define PCIHDR_BASE_CODE                           0x0f0b
#define PCIHDR_CACHE_LINE                          0x0f0c
#define PCIHDR_LATENCY                             0x0f0d
#define PCIHDR_HEADER                              0x0f0e
#define PCIHDR_BIST                                0x0f0f
#define PCIHDR_MEM_BASE                            0x0f10
#define PCIHDR_IO_BASE                             0x0f14
#define PCIHDR_REG_BASE                            0x0f18
#define PCIHDR_ADAPTER_ID                          0x0f2c
#define PCIHDR_BIOS_ROM                            0x0f30
#define PCIHDR_CAPABILITIES_PTR                    0x0f34
#define PCIHDR_INTERRUPT_LINE                      0x0f3c
#define PCIHDR_INTERRUPT_PIN                       0x0f3d
#define PCIHDR_MIN_GRANT                           0x0f3e
#define PCIHDR_MAX_LATENCY                         0x0f3f
#define PCIHDR_CAPABILITIES_ID                     0x0f50
#define PCIHDR_AGP_STATUS                          0x0f54
#define PCIHDR_AGP_COMMAND                         0x0f58
#define PCIHDR_PMI_REGISTER                        0x0f5c
#define PCIHDR_PWR_MNGMT_CNTL_STATUS               0x0f60

/******************************************************************************
 *               NON-GUI PM4 FIFO DATA Registers                              *
 ******************************************************************************/
#define PM4_FIFO_DATA_EVEN                         0x1000
#define PM4_FIFO_DATA_ODD                          0x1004

/******************************************************************************
 *                  GUI Block Memory Mapped Registers                         *
 *                     These registers are FIFOed.                            *
 ******************************************************************************/
#define DST_OFFSET                                 0x1404
#define DST_PITCH                                  0x1408
#define DST_WIDTH                                  0x140c
#define DST_HEIGHT                                 0x1410
#define SRC_X                                      0x1414
#define SRC_Y                                      0x1418
#define DST_X                                      0x141c
#define DST_Y                                      0x1420
#define SRC_PITCH_OFFSET                           0x1428
#define DST_PITCH_OFFSET                           0x142c
#define SRC_Y_X                                    0x1434
#define DST_Y_X                                    0x1438
#define DST_HEIGHT_WIDTH                           0x143c
#define DP_GUI_MASTER_CNTL                         0x146c
#define BRUSH_SCALE                                0x1470
#define BRUSH_Y_X                                  0x1474
#define DP_BRUSH_BKGD_CLR                          0x1478
#define DP_BRUSH_FRGD_CLR                          0x147c
#define BRUSH_DATA0                                0x1480
#define BRUSH_DATA1                                0x1484
#define BRUSH_DATA2                                0x1488
#define BRUSH_DATA3                                0x148c
#define BRUSH_DATA4                                0x1490
#define BRUSH_DATA5                                0x1494
#define BRUSH_DATA6                                0x1498
#define BRUSH_DATA7                                0x149c
#define BRUSH_DATA8                                0x14a0
#define BRUSH_DATA9                                0x14a4
#define BRUSH_DATA10                               0x14a8
#define BRUSH_DATA11                               0x14ac
#define BRUSH_DATA12                               0x14b0
#define BRUSH_DATA13                               0x14b4
#define BRUSH_DATA14                               0x14b8
#define BRUSH_DATA15                               0x14bc
#define BRUSH_DATA16                               0x14c0
#define BRUSH_DATA17                               0x14c4
#define BRUSH_DATA18                               0x14c8
#define BRUSH_DATA19                               0x14cc
#define BRUSH_DATA20                               0x14d0
#define BRUSH_DATA21                               0x14d4
#define BRUSH_DATA22                               0x14d8
#define BRUSH_DATA23                               0x14dc
#define BRUSH_DATA24                               0x14e0
#define BRUSH_DATA25                               0x14e4
#define BRUSH_DATA26                               0x14e8
#define BRUSH_DATA27                               0x14ec
#define BRUSH_DATA28                               0x14f0
#define BRUSH_DATA29                               0x14f4
#define BRUSH_DATA30                               0x14f8
#define BRUSH_DATA31                               0x14fc
#define BRUSH_DATA32                               0x1500
#define BRUSH_DATA33                               0x1504
#define BRUSH_DATA34                               0x1508
#define BRUSH_DATA35                               0x150c
#define BRUSH_DATA36                               0x1510
#define BRUSH_DATA37                               0x1514
#define BRUSH_DATA38                               0x1518
#define BRUSH_DATA39                               0x151c
#define BRUSH_DATA40                               0x1520
#define BRUSH_DATA41                               0x1524
#define BRUSH_DATA42                               0x1528
#define BRUSH_DATA43                               0x152c
#define BRUSH_DATA44                               0x1530
#define BRUSH_DATA45                               0x1534
#define BRUSH_DATA46                               0x1538
#define BRUSH_DATA47                               0x153c
#define BRUSH_DATA48                               0x1540
#define BRUSH_DATA49                               0x1544
#define BRUSH_DATA50                               0x1548
#define BRUSH_DATA51                               0x154c
#define BRUSH_DATA52                               0x1550
#define BRUSH_DATA53                               0x1554
#define BRUSH_DATA54                               0x1558
#define BRUSH_DATA55                               0x155c
#define BRUSH_DATA56                               0x1560
#define BRUSH_DATA57                               0x1564
#define BRUSH_DATA58                               0x1568
#define BRUSH_DATA59                               0x156c
#define BRUSH_DATA60                               0x1570
#define BRUSH_DATA61                               0x1574
#define BRUSH_DATA62                               0x1578
#define BRUSH_DATA63                               0x157c
#define DST_WIDTH_X                                0x1588
#define DST_HEIGHT_WIDTH_8                         0x158c
#define SRC_X_Y                                    0x1590
#define DST_X_Y                                    0x1594
#define DST_WIDTH_HEIGHT                           0x1598
#define DST_WIDTH_X_INCY                           0x159c
#define DST_HEIGHT_Y                               0x15a0
#define DST_X_SUB                                  0x15a4
#define DST_Y_SUB                                  0x15a8
#define SRC_OFFSET                                 0x15ac
#define SRC_PITCH                                  0x15b0
#define DST_HEIGHT_WIDTH_BW                        0x15b4
#define CLR_CMP_CNTL                               0x15c0
#define CLR_CMP_CLR_SRC                            0x15c4
#define CLR_CMP_CLR_DST                            0x15c8
#define CLR_CMP_MASK                               0x15cc
#define DP_SRC_FRGD_CLR                            0x15d8
#define DP_SRC_BKGD_CLR                            0x15dc
#define GUI_SCRATCH_REG0                           0x15e0
#define GUI_SCRATCH_REG1                           0x15e4
#define GUI_SCRATCH_REG2                           0x15e8
#define GUI_SCRATCH_REG3                           0x15ec
#define GUI_SCRATCH_REG4                           0x15f0
#define GUI_SCRATCH_REG5                           0x15f4
#define LEAD_BRES_ERR                              0x1600
#define LEAD_BRES_INC                              0x1604
#define LEAD_BRES_DEC                              0x1608
#define TRAIL_BRES_ERR                             0x160c
#define TRAIL_BRES_INC                             0x1610
#define TRAIL_BRES_DEC                             0x1614
#define TRAIL_X                                    0x1618
#define LEAD_BRES_LNTH                             0x161c
#define TRAIL_X_SUB                                0x1620
#define LEAD_BRES_LNTH_SUB                         0x1624
#define DST_BRES_ERR                               0x1628
#define DST_BRES_INC                               0x162c
#define DST_BRES_DEC                               0x1630
#define DST_BRES_LNTH                              0x1634
#define DST_BRES_LNTH_SUB                          0x1638
#define SC_LEFT                                    0x1640
#define SC_RIGHT                                   0x1644
#define SC_TOP                                     0x1648
#define SC_BOTTOM                                  0x164c
#define SRC_SC_RIGHT                               0x1654
#define SRC_SC_BOTTOM                              0x165c
#define AUX_SC_CNTL                                0x1660
#define AUX1_SC_LEFT                               0x1664
#define AUX1_SC_RIGHT                              0x1668
#define AUX1_SC_TOP                                0x166c
#define AUX1_SC_BOTTOM                             0x1670
#define AUX2_SC_LEFT                               0x1674
#define AUX2_SC_RIGHT                              0x1678
#define AUX2_SC_TOP                                0x167c
#define AUX2_SC_BOTTOM                             0x1680
#define AUX3_SC_LEFT                               0x1684
#define AUX3_SC_RIGHT                              0x1688
#define AUX3_SC_TOP                                0x168c
#define AUX3_SC_BOTTOM                             0x1690
#define GUI_DEBUG0                                 0x16a0
#define GUI_DEBUG1                                 0x16a4
#define GUI_TIMEOUT                                0x16b0
#define GUI_TIMEOUT0                               0x16b4
#define GUI_TIMEOUT1                               0x16b8
#define GUI_PROBE                                  0x16bc
#define DP_CNTL                                    0x16c0
#define DP_DATATYPE                                0x16c4
#define DP_MIX                                     0x16c8
#define DP_WRITE_MASK                              0x16cc
#define DP_CNTL_XDIR_YDIR_YMAJOR                   0x16d0
#define DEFAULT_OFFSET                             0x16e0
#define DEFAULT_PITCH                              0x16e4
#define DEFAULT_SC_BOTTOM_RIGHT                    0x16e8
#define SC_TOP_LEFT                                0x16ec
#define SC_BOTTOM_RIGHT                            0x16f0
#define SRC_SC_BOTTOM_RIGHT                        0x16f4
#define WAIT_UNTIL                                 0x1720
#define CACHE_CNTL                                 0x1724
#define GUI_STAT                                   0x1740
#define PC_GUI_MODE                                0x1744
#define PC_GUI_CTLSTAT                             0x1748
#define PC_DEBUG_MODE                              0x1760
#define BRES_DST_ERR_DEC                           0x1780
#define TRAIL_BRES_T12_ERR_DEC                     0x1784
#define TRAIL_BRES_T12_INC                         0x1788
#define DP_T12_CNTL                                0x178c
#define DST_BRES_T1_LNTH                           0x1790
#define DST_BRES_T2_LNTH                           0x1794
#define HOST_DATA0                                 0x17c0
#define HOST_DATA1                                 0x17c4
#define HOST_DATA2                                 0x17c8
#define HOST_DATA3                                 0x17cc
#define HOST_DATA4                                 0x17d0
#define HOST_DATA5                                 0x17d4
#define HOST_DATA6                                 0x17d8
#define HOST_DATA7                                 0x17dc
#define HOST_DATA_LAST                             0x17e0
#define SECONDARY_SCALE_PITCH                      0x1980
#define SECONDARY_SCALE_X_INC                      0x1984
#define SECONDARY_SCALE_Y_INC                      0x1988
#define SECONDARY_SCALE_HACC                       0x198c
#define SECONDARY_SCALE_VACC                       0x1990
#define SCALE_SRC_HEIGHT_WIDTH                     0x1994
#define SCALE_OFFSET_0                             0x1998
#define SCALE_PITCH                                0x199c
#define SCALE_X_INC                                0x19a0
#define SCALE_Y_INC                                0x19a4
#define SCALE_HACC                                 0x19a8
#define SCALE_VACC                                 0x19ac
#define SCALE_DST_X_Y                              0x19b0
#define SCALE_DST_HEIGHT_WIDTH                     0x19b4
#define SCALE_3D_CNTL                              0x1a00
#define SCALE_3D_DATATYPE                          0x1a20
#define SETUP_CNTL                                 0x1bc4
#define SOLID_COLOR                                0x1bc8
#define WINDOW_XY_OFFSET                           0x1bcc
#define DRAW_LINE_POINT                            0x1bd0
#define SETUP_CNTL_PM4                             0x1bd4
#define DST_PITCH_OFFSET_C                         0x1c80
#define DP_GUI_MASTER_CNTL_C                       0x1c84
#define SC_TOP_LEFT_C                              0x1c88
#define SC_BOTTOM_RIGHT_C                          0x1c8c

#define CLR_CMP_MASK_3D                            0x1A28
#define MISC_3D_STATE_CNTL_REG                     0x1CA0
#define MC_SRC1_CNTL                               0x19D8
#define TEX_CNTL                                   0x1800

/******************************************************************************
 *                                                                            *
 *            RAGE128 INDIRECT REGISTER INDICES CONSTANTS SECTION             *
 *                                                                            *
 ******************************************************************************/

/******************************************************************************
 *                CLOCK_CNTL_INDEX Indexed Registers                          *
 ******************************************************************************/
#define CLK_PIN_CNTL                               0x0001
#define PPLL_CNTL                                  0x0002
#define PPLL_REF_DIV                               0x0003
#define PPLL_DIV_0                                 0x0004
#define PPLL_DIV_1                                 0x0005
#define PPLL_DIV_2                                 0x0006
#define PPLL_DIV_3                                 0x0007
#define VCLK_ECP_CNTL                              0x0008
#define HTOTAL_CNTL                                0x0009
#define X_MPLL_REF_FB_DIV                          0x000a
#define XPLL_CNTL                                  0x000b
#define XDLL_CNTL                                  0x000c
#define XCLK_CNTL                                  0x000d
#define MPLL_CNTL                                  0x000e
#define MCLK_CNTL                                  0x000f
#define AGP_PLL_CNTL                               0x0010
#define FCP_CNTL                                   0x0012
#define PLL_TEST_CNTL                              0x0013


/******************************************************************************
 *                                                                            *
 *               RAGE128 REGISTER MASK CONSTANTS SECTION                      *
 *                                                                            *
 ******************************************************************************/

/******************************************************************************
 *               RAGE128 Non-GUI Register Mask Constants                      *
 *  These are typically ORed together and are used with the register access   *
 *  functions.                                                                *
 ******************************************************************************/

/* AGP_CNTL bit constants */
#define AGP_APER_SIZE_4M                           0x0000003f
#define AGP_APER_SIZE_8M                           0x0000003e
#define AGP_APER_SIZE_16M                          0x0000003c
#define AGP_APER_SIZE_32M                          0x00000038
#define AGP_APER_SIZE_64M                          0x00000030
#define AGP_APER_SIZE_128M                         0x00000020
#define AGP_APER_SIZE_256M                         0x00000000
#define AGP_HIGH_PRIORITY_READ                     0x00010000
#define AGP_TRDY_SET                               0x00020000

/* BM_CHUNK_0_VAL bit constants */
#define BM_PTR_FORCE_TO_PCI                        0x00200000
#define BM_PM4_RD_FORCE_TO_PCI                     0x00400000
#define BM_GLOBAL_FORCE_TO_PCI                     0x00800000
#define BM_VIP3_NOCHUNK                            0x10000000
#define BM_VIP2_NOCHUNK                            0x20000000
#define BM_VIP1_NOCHUNK                            0x40000000
#define BM_VIP0_NOCHUNK                            0x80000000

/* BM_COMMAND bit constants */
#define BM_INTERRUPT_DIS                           0x08000000
#define BM_TRANSFER_DEST_REG                       0x10000000
#define BM_FORCE_TO_PCI                            0x20000000
#define BM_FRAME_OFFSET_HOLD                       0x40000000
#define BM_END_OF_LIST                             0x80000000

/* BM_QUEUE_STATUS bit constants */
#define BM_STATUS_VIP0_ACTIVE                      0x01000000
#define BM_STATUS_VIP1_ACTIVE                      0x02000000
#define BM_STATUS_VIP2_ACTIVE                      0x03000000
#define BM_STATUS_VIP3_ACTIVE                      0x04000000
#define BM_STATUS_GUI_ACTIVE                       0x40000000
#define BM_STATUS_VIDCAP_ACTIVE                    0x80000000

/* BM_VIP?_BUF/VAPCAP/GUI bit constants */
#define BM_TRIG_SYS_TO_FRAME_IMM                   0x00000000
#define BM_TRIG_FRAME_TO_SYS_IMM                   0x00000001

/* BUS_CNTL bit constants */
#define BUS_DBL_RESYNC                             0x00000001
#define BUS_MSTR_RESET                             0x00000002
#define BUS_FLUSH_BUF                              0x00000004
#define BUS_STOP_REQ_DIS                           0x00000008
#define BUS_ROTATION_DIS                           0x00000010
#define BUS_MASTER_DIS                             0x00000040
#define BUS_ROM_WRT_EN                             0x00000080
#define BUS_DIS_ROM                                0x00001000
#define BUS_PCI_READ_RETRY_EN                      0x00002000
#define BUS_AGP_AD_STEPPING_EN                     0x00004000
#define BUS_PCI_WRT_RETRY_EN                       0x00008000
#define BUS_MSTR_RD_MULT                           0x00100000
#define BUS_MSTR_RD_LINE                           0x00200000
#define BUS_SUSPEND                                0x00400000
#define LAT_16X                                    0x00800000
#define BUS_RD_DISCARD_EN                          0x01000000
#define BUS_RD_ABORT_EN                            0x02000000
#define BUS_MSTR_WS                                0x04000000
#define BUS_PARKING_DIS                            0x08000000
#define BUS_MSTR_DISCONNECT_EN                     0x10000000
#define BUS_WRT_BURST                              0x20000000
#define BUS_READ_BURST                             0x40000000
#define BUS_RDY_READ_DLY                           0x80000000

/* CLOCK_CNTL_INDEX bit constants */
#define PLL_WR_EN                                  0x00000080

/* CONFIG_CNTL bit constants */
#define CFG_VGA_RAM_EN                             0x00000100

/* CRTC_EXT_CNTL bit constants */
#define VGA_ATI_LINEAR                             0x00000008
#define VGA_128KAP_PAGING                          0x00000010

/* CRTC_GEN_CNTL bit constants */
#define CRTC_DBL_SCAN_EN                           0x00000001
#define CRTC_CUR_EN                                0x00010000

/* CRTC_STATUS bit constants */
#define CRTC_VBLANK                                0x00000001

/* CUR_OFFSET, CUR_HORZ_VERT_POSN, CUR_HORZ_VERT_OFF bit constants */
#define CUR_LOCK                                   0x80000000

/* DAC_CNTL bit constants */
#define DAC_8BIT_EN                                0x00000100
#define DAC_4BPP_PIX_ORDER                         0x00000200
#define DAC_CRC_EN                                 0x00080000

/* GEN_RESET_CNTL bit constants */
#define SOFT_RESET_GUI                             0x00000001
#define SOFT_RESET_VCLK                            0x00000100
#define SOFT_RESET_PCLK                            0x00000200
#define SOFT_RESET_ECP                             0x00000400
#define SOFT_RESET_DISPENG_XCLK                    0x00000800

/* MEM_CNTL bit constants */
#define MEM_CTLR_STATUS_IDLE                       0x00000000
#define MEM_CTLR_STATUS_BUSY                       0x00100000
#define MEM_SEQNCR_STATUS_IDLE                     0x00000000
#define MEM_SEQNCR_STATUS_BUSY                     0x00200000
#define MEM_ARBITER_STATUS_IDLE                    0x00000000
#define MEM_ARBITER_STATUS_BUSY                    0x00400000
#define MEM_REQ_UNLOCK                             0x00000000
#define MEM_REQ_LOCK                               0x00800000

/* MM_INDEX bit constants */
#define MM_APER                                    0x80000000

/* PC_GUI_CTLSTAT bit constants */
#define PC_BUSY_INIT                               0x10000000
#define PC_BUSY_GUI                                0x20000000
#define PC_BUSY_NGUI                               0x40000000
#define PC_BUSY                                    0x80000000

/* PCI_GART_BASE bit constants */
#define PCI_GART_DIS                               0x00000001

/* PM4_BUFFER_CNTL bit constants */
#define PM4_BUFFER_CNTL_NOUPDATE                   0x08000000
#define PM4_BUFFER_CNTL_NONPM4                     0x00000000
#define PM4_BUFFER_CNTL_192PM4PIO                  0x10000000
#define PM4_BUFFER_CNTL_192PM4BM                   0x20000000
#define PM4_BUFFER_CNTL_128PM4PIO_64INDBM          0x30000000
#define PM4_BUFFER_CNTL_128PM4BM_64INDBM           0x40000000
#define PM4_BUFFER_CNTL_64PM4PIO_128INDBM          0x50000000
#define PM4_BUFFER_CNTL_64PM4BM_128INDBM           0x60000000
#define PM4_BUFFER_CNTL_64PM4PIO_64VERBM_64INDBM   0x70000000
#define PM4_BUFFER_CNTL_64PM4BM_64VERBM_64INDBM    0x80000000
#define PM4_BUFFER_CNTL_64PM4PIO_64VERPIO_64INDPIO 0xf0000000

/* PM4_BUFFER_DL_WPTR bit constants */
#define PM4_BUFFER_DL_DONE                         0x80000000

/* PM4_MICRO_CNTL bit constants */
#define PM4_MICRO_FREERUN                          0x40000000
#define PM4_MICRO_STEP                             0x80000000

/******************************************************************************
 *                                                                            *
 *                   RAGE128 GUI Register Mask Constants                      *
 *                                                                            *
 *      These are typically ORed together and are used with the register      *
 *      access functions.                                                     *
 *                                                                            *
 ******************************************************************************/

/* AUX_SC_CNTL bit constants */
#define AUX1_SC_EN                                 0x00000001
#define AUX1_SC_ADD                                0x00000000
#define AUX1_SC_SUBTRACT                           0x00000002
#define AUX2_SC_EN                                 0x00000004
#define AUX2_SC_ADD                                0x00000000
#define AUX2_SC_SUBTRACT                           0x00000008
#define AUX3_SC_EN                                 0x00000010
#define AUX3_SC_ADD                                0x00000000
#define AUX3_SC_SUBTRACT                           0x00000020

/* CLR_CMP_CNTL bit constants */
#define COMPARE_SRC_FALSE                          0x00000000
#define COMPARE_SRC_TRUE                           0x00000001
#define COMPARE_SRC_NOT_EQUAL                      0x00000004
#define COMPARE_SRC_EQUAL                          0x00000005
#define COMPARE_SRC_EQUAL_FLIP                     0x00000007
#define COMPARE_DST_FALSE                          0x00000000
#define COMPARE_DST_TRUE                           0x00000100
#define COMPARE_DST_NOT_EQUAL                      0x00000400
#define COMPARE_DST_EQUAL                          0x00000500
#define COMPARE_DESTINATION                        0x00000000
#define COMPARE_SOURCE                             0x01000000
#define COMPARE_SRC_AND_DST                        0x02000000

/* DP_CNTL bit constants */
#define DST_X_RIGHT_TO_LEFT                        0x00000000
#define DST_X_LEFT_TO_RIGHT                        0x00000001
#define DST_Y_BOTTOM_TO_TOP                        0x00000000
#define DST_Y_TOP_TO_BOTTOM                        0x00000002
#define DST_X_MAJOR                                0x00000000
#define DST_Y_MAJOR                                0x00000004
#define DST_X_TILE                                 0x00000008
#define DST_Y_TILE                                 0x00000010
#define DST_LAST_PEL                               0x00000020
#define DST_TRAIL_X_RIGHT_TO_LEFT                  0x00000000
#define DST_TRAIL_X_LEFT_TO_RIGHT                  0x00000040
#define DST_TRAP_FILL_RIGHT_TO_LEFT                0x00000000
#define DST_TRAP_FILL_LEFT_TO_RIGHT                0x00000080
#define DST_BRES_SIGN                              0x00000100
#define DST_HOST_BIG_ENDIAN_EN                     0x00000200
#define DST_POLYLINE_NONLAST                       0x00008000
#define DST_RASTER_STALL                           0x00010000
#define DST_POLY_EDGE                              0x00040000

/* DP_CNTL_YDIR_XDIR_YMAJOR bit constants (short version of DP_CNTL) */
#define DST_X_MAJOR_S                              0x00000000
#define DST_Y_MAJOR_S                              0x00000001
#define DST_Y_BOTTOM_TO_TOP_S                      0x00000000
#define DST_Y_TOP_TO_BOTTOM_S                      0x00008000
#define DST_X_RIGHT_TO_LEFT_S                      0x00000000
#define DST_X_LEFT_TO_RIGHT_S                      0x80000000

/* DP_DATATYPE bit constants */
#define DST_8BPP                                   0x00000002
#define DST_15BPP                                  0x00000003
#define DST_16BPP                                  0x00000004
#define DST_24BPP                                  0x00000005
#define DST_32BPP                                  0x00000006
#define DST_8BPP_RGB332                            0x00000007
#define DST_8BPP_Y8                                0x00000008
#define DST_8BPP_RGB8                              0x00000009
#define DST_16BPP_VYUY422                          0x0000000b
#define DST_16BPP_YVYU422                          0x0000000c
#define DST_32BPP_AYUV444                          0x0000000e
#define DST_16BPP_ARGB4444                         0x0000000f
#define BRUSH_8x8MONO                              0x00000000
#define BRUSH_8x8MONO_LBKGD                        0x00000100
#define BRUSH_8x1MONO                              0x00000200
#define BRUSH_8x1MONO_LBKGD                        0x00000300
#define BRUSH_1x8MONO                              0x00000400
#define BRUSH_1x8MONO_LBKGD                        0x00000500
#define BRUSH_32x1MONO                             0x00000600
#define BRUSH_32x1MONO_LBKGD                       0x00000700
#define BRUSH_32x32MONO                            0x00000800
#define BRUSH_32x32MONO_LBKGD                      0x00000900
#define BRUSH_8x8COLOR                             0x00000a00
#define BRUSH_8x1COLOR                             0x00000b00
#define BRUSH_1x8COLOR                             0x00000c00
#define BRUSH_SOLIDCOLOR                           0x00000d00
#define SRC_MONO                                   0x00000000
#define SRC_MONO_LBKGD                             0x00010000
#define SRC_DSTCOLOR                               0x00030000
#define BYTE_ORDER_MSB_TO_LSB                      0x00000000
#define BYTE_ORDER_LSB_TO_MSB                      0x40000000
#define DP_CONVERSION_TEMP                         0x80000000

/* DP_GUI_MASTER_CNTL bit constants */
#define GMC_SRC_PITCH_OFFSET_DEFAULT               0x00000000
#define GMC_SRC_PITCH_OFFSET_LEAVE                 0x00000001
#define GMC_DST_PITCH_OFFSET_DEFAULT               0x00000000
#define GMC_DST_PITCH_OFFSET_LEAVE                 0x00000002
#define GMC_SRC_CLIP_DEFAULT                       0x00000000
#define GMC_SRC_CLIP_LEAVE                         0x00000004
#define GMC_DST_CLIP_DEFAULT                       0x00000000
#define GMC_DST_CLIP_LEAVE                         0x00000008
#define GMC_BRUSH_8x8MONO                          0x00000000
#define GMC_BRUSH_8x8MONO_LBKGD                    0x00000010
#define GMC_BRUSH_8x1MONO                          0x00000020
#define GMC_BRUSH_8x1MONO_LBKGD                    0x00000030
#define GMC_BRUSH_1x8MONO                          0x00000040
#define GMC_BRUSH_1x8MONO_LBKGD                    0x00000050
#define GMC_BRUSH_32x1MONO                         0x00000060
#define GMC_BRUSH_32x1MONO_LBKGD                   0x00000070
#define GMC_BRUSH_32x32MONO                        0x00000080
#define GMC_BRUSH_32x32MONO_LBKGD                  0x00000090
#define GMC_BRUSH_8x8COLOR                         0x000000a0
#define GMC_BRUSH_8x1COLOR                         0x000000b0
#define GMC_BRUSH_1x8COLOR                         0x000000c0
#define GMC_BRUSH_SOLIDCOLOR                       0x000000d0
#define GMC_DST_8BPP                               0x00000200
#define GMC_DST_15BPP                              0x00000300
#define GMC_DST_16BPP                              0x00000400
#define GMC_DST_24BPP                              0x00000500
#define GMC_DST_32BPP                              0x00000600
#define GMC_DST_8BPP_RGB332                        0x00000700
#define GMC_DST_8BPP_Y8                            0x00000800
#define GMC_DST_8BPP_RGB8                          0x00000900
#define GMC_DST_16BPP_VYUY422                      0x00000b00
#define GMC_DST_16BPP_YVYU422                      0x00000c00
#define GMC_DST_32BPP_AYUV444                      0x00000e00
#define GMC_DST_16BPP_ARGB4444                     0x00000f00
#define GMC_SRC_MONO                               0x00000000
#define GMC_SRC_MONO_LBKGD                         0x00001000
#define GMC_SRC_DSTCOLOR                           0x00003000
#define GMC_BYTE_ORDER_MSB_TO_LSB                  0x00000000
#define GMC_BYTE_ORDER_LSB_TO_MSB                  0x00004000
#define GMC_DP_CONVERSION_TEMP_9300                0x00008000
#define GMC_DP_CONVERSION_TEMP_6500                0x00000000
#define GMC_DP_SRC_RECT                            0x02000000
#define GMC_DP_SRC_HOST                            0x03000000
#define GMC_DP_SRC_HOST_BYTEALIGN                  0x04000000
#define GMC_3D_FCN_EN_CLR                          0x00000000
#define GMC_3D_FCN_EN_SET                          0x08000000
#define GMC_DST_CLR_CMP_FCN_LEAVE                  0x00000000
#define GMC_DST_CLR_CMP_FCN_CLEAR                  0x10000000
#define GMC_AUX_CLIP_LEAVE                         0x00000000
#define GMC_AUX_CLIP_CLEAR                         0x20000000
#define GMC_WRITE_MASK_LEAVE                       0x00000000
#define GMC_WRITE_MASK_SET                         0x40000000

/* DP_MIX bit constants */
#define DP_SRC_RECT                                0x00000200
#define DP_SRC_HOST                                0x00000300
#define DP_SRC_HOST_BYTEALIGN                      0x00000400

/******************************************************************************
 *                                                                            *
 *              DP_MIX and DP_GUI_MASTER_CNTL ROP3 RPN constants              *
 *                                                                            *
 *      P = pattern                                                           *
 *      S = source                                                            *
 *      D = destination                                                       *
 *      o = OR                                                                *
 *      a = AND                                                               *
 *      x = XOR                                                               *
 *      n = NOT                                                               *
 *                                                                            *
 ******************************************************************************/
#define ROP3_ZERO                                  0x00000000
#define ROP3_DPSoon                                0x00010000
#define ROP3_DPSona                                0x00020000
#define ROP3_PSon                                  0x00030000
#define ROP3_SDPona                                0x00040000
#define ROP3_DPon                                  0x00050000
#define ROP3_PDSxnon                               0x00060000
#define ROP3_PDSaon                                0x00070000
#define ROP3_SDPnaa                                0x00080000
#define ROP3_PDSxon                                0x00090000
#define ROP3_DPna                                  0x000a0000
#define ROP3_PSDnaon                               0x000b0000
#define ROP3_SPna                                  0x000c0000
#define ROP3_PDSnaon                               0x000d0000
#define ROP3_PDSonon                               0x000e0000
#define ROP3_Pn                                    0x000f0000
#define ROP3_PDSona                                0x00100000
#define ROP3_DSon                                  0x00110000
#define ROP3_SDPxnon                               0x00120000
#define ROP3_SDPaon                                0x00130000
#define ROP3_DPSxnon                               0x00140000
#define ROP3_DPSaon                                0x00150000
#define ROP3_PSDPSanaxx                            0x00160000
#define ROP3_SSPxDSxaxn                            0x00170000
#define ROP3_SPxPDxa                               0x00180000
#define ROP3_SDPSanaxn                             0x00190000
#define ROP3_PDSPaox                               0x001a0000
#define ROP3_SDPSxaxn                              0x001b0000
#define ROP3_PSDPaox                               0x001c0000
#define ROP3_DSPDxaxn                              0x001d0000
#define ROP3_PDSox                                 0x001e0000
#define ROP3_PDSoan                                0x001f0000
#define ROP3_DPSnaa                                0x00200000
#define ROP3_SDPxon                                0x00210000
#define ROP3_DSna                                  0x00220000
#define ROP3_SPDnaon                               0x00230000
#define ROP3_SPxDSxa                               0x00240000
#define ROP3_PDSPanaxn                             0x00250000
#define ROP3_SDPSaox                               0x00260000
#define ROP3_SDPSxnox                              0x00270000
#define ROP3_DPSxa                                 0x00280000
#define ROP3_PSDPSaoxxn                            0x00290000
#define ROP3_DPSana                                0x002a0000
#define ROP3_SSPxPDxaxn                            0x002b0000
#define ROP3_SPDSoax                               0x002c0000
#define ROP3_PSDnox                                0x002d0000
#define ROP3_PSDPxox                               0x002e0000
#define ROP3_PSDnoan                               0x002f0000
#define ROP3_PSna                                  0x00300000
#define ROP3_SDPnaon                               0x00310000
#define ROP3_SDPSoox                               0x00320000
#define ROP3_Sn                                    0x00330000
#define ROP3_SPDSaox                               0x00340000
#define ROP3_SPDSxnox                              0x00350000
#define ROP3_SDPox                                 0x00360000
#define ROP3_SDPoan                                0x00370000
#define ROP3_PSDPoax                               0x00380000
#define ROP3_SPDnox                                0x00390000
#define ROP3_SPDSxox                               0x003a0000
#define ROP3_SPDnoan                               0x003b0000
#define ROP3_PSx                                   0x003c0000
#define ROP3_SPDSonox                              0x003d0000
#define ROP3_SPDSnaox                              0x003e0000
#define ROP3_PSan                                  0x003f0000
#define ROP3_PSDnaa                                0x00400000
#define ROP3_DPSxon                                0x00410000
#define ROP3_SDxPDxa                               0x00420000
#define ROP3_SPDSanaxn                             0x00430000
#define ROP3_SDna                                  0x00440000
#define ROP3_DPSnaon                               0x00450000
#define ROP3_DSPDaox                               0x00460000
#define ROP3_PSDPxaxn                              0x00470000
#define ROP3_SDPxa                                 0x00480000
#define ROP3_PDSPDaoxxn                            0x00490000
#define ROP3_DPSDoax                               0x004a0000
#define ROP3_PDSnox                                0x004b0000
#define ROP3_SDPana                                0x004c0000
#define ROP3_SSPxDSxoxn                            0x004d0000
#define ROP3_PDSPxox                               0x004e0000
#define ROP3_PDSnoan                               0x004f0000
#define ROP3_PDna                                  0x00500000
#define ROP3_DSPnaon                               0x00510000
#define ROP3_DPSDaox                               0x00520000
#define ROP3_SPDSxaxn                              0x00530000
#define ROP3_DPSonon                               0x00540000
#define ROP3_Dn                                    0x00550000
#define ROP3_DPSox                                 0x00560000
#define ROP3_DPSoan                                0x00570000
#define ROP3_PDSPoax                               0x00580000
#define ROP3_DPSnox                                0x00590000
#define ROP3_DPx                                   0x005a0000
#define ROP3_DPSDonox                              0x005b0000
#define ROP3_DPSDxox                               0x005c0000
#define ROP3_DPSnoan                               0x005d0000
#define ROP3_DPSDnaox                              0x005e0000
#define ROP3_DPan                                  0x005f0000
#define ROP3_PDSxa                                 0x00600000
#define ROP3_DSPDSaoxxn                            0x00610000
#define ROP3_DSPDoax                               0x00620000
#define ROP3_SDPnox                                0x00630000
#define ROP3_SDPSoax                               0x00640000
#define ROP3_DSPnox                                0x00650000
#define ROP3_DSx                                   0x00660000
#define ROP3_SDPSonox                              0x00670000
#define ROP3_DSPDSonoxxn                           0x00680000
#define ROP3_PDSxxn                                0x00690000
#define ROP3_DPSax                                 0x006a0000
#define ROP3_PSDPSoaxxn                            0x006b0000
#define ROP3_SDPax                                 0x006c0000
#define ROP3_PDSPDoaxxn                            0x006d0000
#define ROP3_SDPSnoax                              0x006e0000
#define ROP3_PDSxnan                               0x006f0000
#define ROP3_PDSana                                0x00700000
#define ROP3_SSDxPDxaxn                            0x00710000
#define ROP3_SDPSxox                               0x00720000
#define ROP3_SDPnoan                               0x00730000
#define ROP3_DSPDxox                               0x00740000
#define ROP3_DSPnoan                               0x00750000
#define ROP3_SDPSnaox                              0x00760000
#define ROP3_DSan                                  0x00770000
#define ROP3_PDSax                                 0x00780000
#define ROP3_DSPDSoaxxn                            0x00790000
#define ROP3_DPSDnoax                              0x007a0000
#define ROP3_SDPxnan                               0x007b0000
#define ROP3_SPDSnoax                              0x007c0000
#define ROP3_DPSxnan                               0x007d0000
#define ROP3_SPxDSxo                               0x007e0000
#define ROP3_DPSaan                                0x007f0000
#define ROP3_DPSaa                                 0x00800000
#define ROP3_SPxDSxon                              0x00810000
#define ROP3_DPSxna                                0x00820000
#define ROP3_SPDSnoaxn                             0x00830000
#define ROP3_SDPxna                                0x00840000
#define ROP3_PDSPnoaxn                             0x00850000
#define ROP3_DSPDSoaxx                             0x00860000
#define ROP3_PDSaxn                                0x00870000
#define ROP3_DSa                                   0x00880000
#define ROP3_SDPSnaoxn                             0x00890000
#define ROP3_DSPnoa                                0x008a0000
#define ROP3_DSPDxoxn                              0x008b0000
#define ROP3_SDPnoa                                0x008c0000
#define ROP3_SDPSxoxn                              0x008d0000
#define ROP3_SSDxPDxax                             0x008e0000
#define ROP3_PDSanan                               0x008f0000
#define ROP3_PDSxna                                0x00900000
#define ROP3_SDPSnoaxn                             0x00910000
#define ROP3_DPSDPoaxx                             0x00920000
#define ROP3_SPDaxn                                0x00930000
#define ROP3_PSDPSoaxx                             0x00940000
#define ROP3_DPSaxn                                0x00950000
#define ROP3_DPSxx                                 0x00960000
#define ROP3_PSDPSonoxx                            0x00970000
#define ROP3_SDPSonoxn                             0x00980000
#define ROP3_DSxn                                  0x00990000
#define ROP3_DPSnax                                0x009a0000
#define ROP3_SDPSoaxn                              0x009b0000
#define ROP3_SPDnax                                0x009c0000
#define ROP3_DSPDoaxn                              0x009d0000
#define ROP3_DSPDSaoxx                             0x009e0000
#define ROP3_PDSxan                                0x009f0000
#define ROP3_DPa                                   0x00a00000
#define ROP3_PDSPnaoxn                             0x00a10000
#define ROP3_DPSnoa                                0x00a20000
#define ROP3_DPSDxoxn                              0x00a30000
#define ROP3_PDSPonoxn                             0x00a40000
#define ROP3_PDxn                                  0x00a50000
#define ROP3_DSPnax                                0x00a60000
#define ROP3_PDSPoaxn                              0x00a70000
#define ROP3_DPSoa                                 0x00a80000
#define ROP3_DPSoxn                                0x00a90000
#define ROP3_D                                     0x00aa0000
#define ROP3_DPSono                                0x00ab0000
#define ROP3_SPDSxax                               0x00ac0000
#define ROP3_DPSDaoxn                              0x00ad0000
#define ROP3_DSPnao                                0x00ae0000
#define ROP3_DPno                                  0x00af0000
#define ROP3_PDSnoa                                0x00b00000
#define ROP3_PDSPxoxn                              0x00b10000
#define ROP3_SSPxDSxox                             0x00b20000
#define ROP3_SDPanan                               0x00b30000
#define ROP3_PSDnax                                0x00b40000
#define ROP3_DPSDoaxn                              0x00b50000
#define ROP3_DPSDPaoxx                             0x00b60000
#define ROP3_SDPxan                                0x00b70000
#define ROP3_PSDPxax                               0x00b80000
#define ROP3_DSPDaoxn                              0x00b90000
#define ROP3_DPSnao                                0x00ba0000
#define ROP3_DSno                                  0x00bb0000
#define ROP3_SPDSanax                              0x00bc0000
#define ROP3_SDxPDxan                              0x00bd0000
#define ROP3_DPSxo                                 0x00be0000
#define ROP3_DPSano                                0x00bf0000
#define ROP3_PSa                                   0x00c00000
#define ROP3_SPDSnaoxn                             0x00c10000
#define ROP3_SPDSonoxn                             0x00c20000
#define ROP3_PSxn                                  0x00c30000
#define ROP3_SPDnoa                                0x00c40000
#define ROP3_SPDSxoxn                              0x00c50000
#define ROP3_SDPnax                                0x00c60000
#define ROP3_PSDPoaxn                              0x00c70000
#define ROP3_SDPoa                                 0x00c80000
#define ROP3_SPDoxn                                0x00c90000
#define ROP3_DPSDxax                               0x00ca0000
#define ROP3_SPDSaoxn                              0x00cb0000
#define ROP3_S                                     0x00cc0000
#define ROP3_SDPono                                0x00cd0000
#define ROP3_SDPnao                                0x00ce0000
#define ROP3_SPno                                  0x00cf0000
#define ROP3_PSDnoa                                0x00d00000
#define ROP3_PSDPxoxn                              0x00d10000
#define ROP3_PDSnax                                0x00d20000
#define ROP3_SPDSoaxn                              0x00d30000
#define ROP3_SSPxPDxax                             0x00d40000
#define ROP3_DPSanan                               0x00d50000
#define ROP3_PSDPSaoxx                             0x00d60000
#define ROP3_DPSxan                                0x00d70000
#define ROP3_PDSPxax                               0x00d80000
#define ROP3_SDPSaoxn                              0x00d90000
#define ROP3_DPSDanax                              0x00da0000
#define ROP3_SPxDSxan                              0x00db0000
#define ROP3_SPDnao                                0x00dc0000
#define ROP3_SDno                                  0x00dd0000
#define ROP3_SDPxo                                 0x00de0000
#define ROP3_SDPano                                0x00df0000
#define ROP3_PDSoa                                 0x00e00000
#define ROP3_PDSoxn                                0x00e10000
#define ROP3_DSPDxax                               0x00e20000
#define ROP3_PSDPaoxn                              0x00e30000
#define ROP3_SDPSxax                               0x00e40000
#define ROP3_PDSPaoxn                              0x00e50000
#define ROP3_SDPSanax                              0x00e60000
#define ROP3_SPxPDxan                              0x00e70000
#define ROP3_SSPxDSxax                             0x00e80000
#define ROP3_DSPDSanaxxn                           0x00e90000
#define ROP3_DPSao                                 0x00ea0000
#define ROP3_DPSxno                                0x00eb0000
#define ROP3_SDPao                                 0x00ec0000
#define ROP3_SDPxno                                0x00ed0000
#define ROP3_DSo                                   0x00ee0000
#define ROP3_SDPnoo                                0x00ef0000
#define ROP3_P                                     0x00f00000
#define ROP3_PDSono                                0x00f10000
#define ROP3_PDSnao                                0x00f20000
#define ROP3_PSno                                  0x00f30000
#define ROP3_PSDnao                                0x00f40000
#define ROP3_PDno                                  0x00f50000
#define ROP3_PDSxo                                 0x00f60000
#define ROP3_PDSano                                0x00f70000
#define ROP3_PDSao                                 0x00f80000
#define ROP3_PDSxno                                0x00f90000
#define ROP3_DPo                                   0x00fa0000
#define ROP3_DPSnoo                                0x00fb0000
#define ROP3_PSo                                   0x00fc0000
#define ROP3_PSDnoo                                0x00fd0000
#define ROP3_DPSoo                                 0x00fe0000
#define ROP3_ONE                                   0x00ff0000

/******************************************************************************
 *              DP_MIX, DP_GUI_MASTER_CNTL ROP3 named constants               *
 *                                                                            *
 *                  Pattern (P):     1 1 1 1 0 0 0 0                          *
 *                  Source  (S):     1 1 0 0 1 1 0 0                          *
 *                  Destination (D): 1 0 1 0 1 0 1 0                          *
 *                                                                            *
 ******************************************************************************/
#define ROP3_BLACKNESS                             0x00000000   // 0
#define ROP3_NOTSRCERASE                           0x00110000   // ~(S | D)
#define ROP3_NOTSRCCOPY                            0x00330000   // ~S
#define ROP3_SRCERASE                              0x00440000   // S & ~D
#define ROP3_DSTINVERT                             0x00550000   // ~D
#define ROP3_PATINVERT                             0x005a0000   // P ^ D
#define ROP3_SRCINVERT                             0x00660000   // S ^ D
#define ROP3_SRCAND                                0x00880000   // S & D
#define ROP3_DSTCOPY                               0x00aa0000   // D
#define ROP3_MERGEPAINT                            0x00bb0000   // ~S | D
#define ROP3_MERGECOPY                             0x00c00000   // P & S
#define ROP3_SRCCOPY                               0x00cc0000   // S
#define ROP3_SRCPAINT                              0x00ee0000   // S | D
#define ROP3_PATCOPY                               0x00f00000   // P
#define ROP3_PATPAINT                              0x00fb0000   // P | ~S | D
#define ROP3_WHITENESS                             0x00ff0000   // 1

/******************************************************************************
 *               GUI_STAT and PM4_STAT common bit constants                   *
 ******************************************************************************/
#define PM4_BUSY                                   0x00010000
#define MICRO_BUSY                                 0x00020000
#define FPU_BUSY                                   0x00040000
#define VC_BUSY                                    0x00080000
#define IDCT_BUSY                                  0x00100000
#define ENG_EV_BUSY                                0x00200000
#define SETUP_BUSY                                 0x00400000
#define EDGEWALK_BUSY                              0x00800000
#define ENG_3D_BUSY                                0x01000000
#define ENG_2D_SM_BUSY                             0x02000000
#define ENG_2D_BUSY                                0x04000000
#define GUI_WB_BUSY                                0x08000000
#define CACHE_BUSY                                 0x10000000
#define GUI_ACTIVE                                 0x80000000

/******************************************************************************
 *                      SCALE_3D_CNTL bit constants                           *
 ******************************************************************************/
#define SCALE_DITHER                               0x00000002
#define TEX_CACHE_SIZE                             0x00000004
#define DITHER_INIT                                0x00000008
#define ROUND_EN                                   0x00000010
#define TEX_CACHE_DIS                              0x00000020
#define SCALE_3D_FCN_NOOP                          0x00000000
#define SCALE_3D_FCN_SCALING                       0x00000040
#define SCALE_3D_FCN_TEXTURE_SHADE                 0x00000080
#define SCALE_PIX_REP                              0x00000100

/******************************************************************************
 *                  SCALE_3D_DATATYPE bit constants                           *
 ******************************************************************************/
#define PRIMARY_2BPP                               0x00000000
#define PRIMARY_4BPP                               0x00000001
#define PRIMARY_8BPP                               0x00000002
#define PRIMARY_15BPP                              0x00000003
#define PRIMARY_16BPP                              0x00000004
#define PRIMARY_32BPP                              0x00000006
#define PRIMARY_8BPP_RGB332                        0x00000007
#define PRIMARY_8BPP_Y8                            0x00000008
#define PRIMARY_8BPP_RGB8                          0x00000009
#define PRIMARY_16BPP_P88                          0x0000000a
#define PRIMARY_16BPP_VYUY422                      0x0000000b
#define PRIMARY_16BPP_YVYU422                      0x0000000c
#define PRIMARY_16BPP_RGB8_P88                     0x0000000d
#define PRIMARY_32BPP_AYUV444                      0x0000000e
#define PRIMARY_16BPP_ARGB4444                     0x0000000f
#define PRIMARY_PALETTE_OFF_EITHER                 0x00000000
#define PRIMARY_PALETTE_OFF_1                      0x00000010
#define PRIMARY_PALETTE_OFF_2                      0x00000020
#define PRIMARY_CI_RGB565                          0x00000000
#define PRIMARY_CI_ARGB1555                        0x00000100
#define PRIMARY_CI_ARGB4444                        0x00000200

/******************************************************************************
 *                          SCALE_PITCH bit constants                         *
 ******************************************************************************/
#define SCALE_TILE                                 0x00010000
#define SCALE_PITCH_ADJ_NONE                       0x00000000
#define SCALE_PITCH_ADJ_2                          0x40000000
#define SCALE_PITCH_ADJ_4                          0x80000000

/******************************************************************************
 *                          Flat Panel stuff I stole from Xfree86             *
 ******************************************************************************/
#define R128_FP_CRTC_H_TOTAL_DISP         0x0250
#define R128_FP_CRTC_V_TOTAL_DISP         0x0254
#define R128_FP_GEN_CNTL                  0x0284
#       define R128_FP_FPON                  (1 << 0)
#       define R128_FP_TDMS_EN               (1 <<  2)
#       define R128_FP_DETECT_SENSE          (1 <<  8)
#       define R128_FP_SEL_CRTC2             (1 << 13)
#       define R128_FP_CRTC_DONT_SHADOW_VPAR (1 << 16)
#       define R128_FP_CRTC_USE_SHADOW_VEND  (1 << 18)
#       define R128_FP_CRTC_HORZ_DIV2_EN     (1 << 20)
#       define R128_FP_CRTC_HOR_CRT_DIV2_DIS (1 << 21)
#       define R128_FP_USE_SHADOW_EN         (1 << 24)
#define R128_FP_H_SYNC_STRT_WID           0x02c4
#define R128_FP_HORZ_STRETCH              0x028c
#       define R128_HORZ_STRETCH_RATIO_MASK  0xffff
#       define R128_HORZ_STRETCH_RATIO_SHIFT 0
#       define R128_HORZ_STRETCH_RATIO_MAX   4096
#       define R128_HORZ_PANEL_SIZE          (0xff   << 16)
#       define R128_HORZ_PANEL_SHIFT         16
#       define R128_HORZ_STRETCH_PIXREP      (0      << 25)
#       define R128_HORZ_STRETCH_BLEND       (1      << 25)
#       define R128_HORZ_STRETCH_ENABLE      (1      << 26)
#       define R128_HORZ_FP_LOOP_STRETCH     (0x7    << 27)
#       define R128_HORZ_STRETCH_RESERVED    (1      << 30)
#       define R128_HORZ_AUTO_RATIO_FIX_EN   (1      << 31)
#define R128_CRTC_EXT_CNTL                0x0054
#       define R128_CRTC_CRT_ON           (1 << 15)
#define R128_DAC_CNTL                     0x0058
#       define R128_DAC_CRT_SEL_CRTC2     (1 <<  4)
#define R128_CRTC2_GEN_CNTL               0x03f8
#define R128_LVDS_GEN_CNTL                0x02d0
#       define R128_LVDS_ON               (1   <<  0)
#       define R128_LVDS_BLON             (1   << 19)
#define R128_FP_PANEL_CNTL                0x0288
#       define R128_FP_DIGON              (1 << 0)
#       define R128_FP_BLON               (1 << 1)
#define R128_FP_V_SYNC_STRT_WID           0x02c8
#define R128_FP_VERT_STRETCH              0x0290
#       define R128_VERT_PANEL_SIZE          (0x7ff <<  0)
#       define R128_VERT_PANEL_SHIFT         0
#       define R128_VERT_STRETCH_RATIO_MASK  0x3ff
#       define R128_VERT_STRETCH_RATIO_SHIFT 11
#       define R128_VERT_STRETCH_RATIO_MAX   1024
#       define R128_VERT_STRETCH_ENABLE      (1     << 24)
#       define R128_VERT_STRETCH_LINEREP     (0     << 25)
#       define R128_VERT_STRETCH_BLEND       (1     << 25)
#       define R128_VERT_AUTO_RATIO_EN       (1     << 26)
#       define R128_VERT_STRETCH_RESERVED    0xf8e00000

#endif

