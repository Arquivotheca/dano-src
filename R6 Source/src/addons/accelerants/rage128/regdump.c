



/******************************************************************************
 * REGDEF.H                                                                   *
 *     Header file for Rage 128 Chapter 3 sample code                         *
 *     -all register offsets are expressed in bytes-                          *
 *                                                                            *
 * Copyright (c) 1999 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/
// define REGDEF_H for other files that include this file

/******************************************************************************
 *                                                                            *
 *  RAGE128 REGISTER OFFSET CONSTANTS SECTION                                 *
 *                                                                            *
 ******************************************************************************/
#include "stdlib.h"

#include "stdio.h"
#include "definesR128.h"
#include "registersR128.h"
#include "string.h"
#define TRUE 1
#define FALSE 0



//Function prototypes
void DumpATI128StaticRegisters ( FILE * w, unsigned long int *AuxPtr, int reg_count );
char * find_mnemonic ( int address );
void DumpATI128PLL ( FILE * w, unsigned long int *auxptr );
void DumpATI128StaticRegisters ( FILE * w, unsigned long int *AuxPtr, int reg_count );



/******************************************************************************
 *               NON-GUI Block I/O Mapped Registers                           *
 ******************************************************************************/

struct Ati128RegType {
	char * mnem ;
	int address ;
} ;

struct Ati128RegType  Ati128Regs[] =
{
	{ "ioMM_INDEX",                                 0x0000 },
     { "ioMM_DATA",                                  0x0004 },
     { "ioCLOCK_CNTL_INDEX",                         0x0008 },
     { "ioCLOCK_CNTL_DATA",                          0x000c },
     { "ioBIOS_0_SCRATCH",                           0x0010 },
     { "ioBIOS_1_SCRATCH",                           0x0014 },
     { "ioBIOS_2_SCRATCH",                           0x0018 },
     { "ioBIOS_3_SCRATCH",                           0x001c },
     { "ioBUS_CNTL",                                 0x0030 },
     { "ioMEM_VGA_WP_SEL",                           0x0038 },
     { "ioMEM_VGA_RP_SEL",                           0x003c },
     { "ioGEN_INT_CNTL",                             0x0040 },
     { "ioGEN_INT_STATUS",                           0x0044 },
     { "ioCRTC_GEN_CNTL",                            0x0050 },
     { "ioCRTC_EXT_CNTL",                            0x0054 },
     { "ioDAC_CNTL",                                 0x0058 },
     { "ioCRTC_STATUS",                              0x005c },
     { "ioPALETTE_INDEX",                            0x00b0 },
     { "ioPALETTE_DATA",                             0x00b4 },
     { "ioCONFIG_CNTL",                              0x00e0 },
     { "ioCONFIG_XSTRAP",                            0x00e4 },
     { "ioCONFIG_BONDS",                             0x00e8 },
     { "ioGEN_RESET_CNTL",                           0x00f0 },
     { "ioGEN_STATUS",                               0x00f4 },
     { "ioCONFIG_MEMSIZE",                           0x00f8 },


/******************************************************************************
 *               NON-GUI Block Memory Mapped Registers                        *
 ******************************************************************************/
     { "MM_INDEX",                                   0x0000 },
     { "MM_DATA",                                    0x0004 },
     { "CLOCK_CNTL_INDEX",                           0x0008 },
     { "CLOCK_CNTL_DATA",                            0x000c },
     { "BIOS_0_SCRATCH",                             0x0010 },
     { "BIOS_1_SCRATCH",                             0x0014 },
     { "BIOS_2_SCRATCH",                             0x0018 },
     { "BIOS_3_SCRATCH",                             0x001c },
     { "BUS_CNTL",                                   0x0030 },
     { "BUS_CNTL1",                                  0x0034 },
     { "MEM_VGA_WP_SEL",                             0x0038 },
     { "MEM_VGA_RP_SEL",                             0x003c },
     { "GEN_INT_CNTL",                               0x0040 },
     { "GEN_INT_STATUS",                             0x0044 },
     { "CRTC_GEN_CNTL",                              0x0050 },
     { "CRTC_EXT_CNTL",                              0x0054 },
     { "DAC_CNTL",                                   0x0058 },
     { "CRTC_STATUS",                                0x005c },
     { "GPIO_MONID",                                 0x0068 },
     { "I2C_CNTL_1",                                 0x0094 },
     { "PALETTE_INDEX",                              0x00b0 },
     { "PALETTE_DATA",                               0x00b4 },
     { "CONFIG_CNTL",                                0x00e0 },
     { "CONFIG_XSTRAP",                              0x00e4 },
     { "CONFIG_BONDS",                               0x00e8 },
     { "GEN_RESET_CNTL",                             0x00f0 },
     { "GEN_STATUS",                                 0x00f4 },
     { "CONFIG_MEMSIZE",                             0x00f8 },
     { "CONFIG_APER_0_BASE",                         0x0100 },
     { "CONFIG_APER_1_BASE",                         0x0104 },
     { "CONFIG_APER_SIZE",                           0x0108 },
     { "CONFIG_REG_1_BASE",                          0x010c },
     { "CONFIG_REG_APER_SIZE",                       0x0110 },
     { "CONFIG_MEMSIZE_EMBEDDED",                    0x0114 },
     { "TEST_DEBUG_CNTL",                            0x0120 },
     { "TEST_DEBUG_MUX",                             0x0124 },
     { "HW_DEBUG",                                   0x0128 },
     { "TEST_DEBUG_OUT",                             0x012c },
     { "HOST_PATH_CNTL",                             0x0130 },
     { "SW_SEMAPHORE",                               0x013c },
     { "MEM_CNTL",                                   0x0140 },
     { "EXT_MEM_CNTL",                               0x0144 },
     { "MEM_ADDR_CONFIG",                            0x0148 },
     { "MEM_INTF_CNTL",                              0x014c },
     { "MEM_STR_CNTL",                               0x0150 },
     { "MEM_INIT_LAT_TIMER",                         0x0154 },
     { "MEM_SDRAM_MODE_REG",                         0x0158 },
     { "AGP_BASE",                                   0x0170 },
     { "AGP_CNTL",                                   0x0174 },
     { "AGP_APER_OFFSET",                            0x0178 },
     { "PCI_GART_PAGE",                              0x017c },
     { "PC_NGUI_MODE",                               0x0180 },
     { "PC_NGUI_CTLSTAT",                            0x0184 },
     { "VIDEOMUX_CNTL",                              0x0190 },
     { "MPP_TB_CONFIG",                              0x01C0 },
     { "MPP_GP_CONFIG",                              0x01C8 },
     { "VIPH_CONTROL",                               0x01D0 },
     { "CRTC_H_TOTAL_DISP",                          0x0200 },
     { "CRTC_H_SYNC_STRT_WID",                       0x0204 },
     { "CRTC_V_TOTAL_DISP",                          0x0208 },
     { "CRTC_V_SYNC_STRT_WID",                       0x020c },
     { "CRTC_VLINE_CRNT_VLINE",                      0x0210 },
     { "CRTC_CRNT_FRAME",                            0x0214 },
     { "CRTC_GUI_TRIG_VLINE",                        0x0218 },
     { "CRTC_DEBUG",                                 0x021c },
     { "CRTC_OFFSET",                                0x0224 },
     { "CRTC_OFFSET_CNTL",                           0x0228 },
     { "CRTC_PITCH",                                 0x022c },
     { "OVR_CLR",                                    0x0230 },
     { "OVR_WID_LEFT_RIGHT",                         0x0234 },
     { "OVR_WID_TOP_BOTTOM",                         0x0238 },
     { "SNAPSHOT_VH_COUNTS",                         0x0240 },
     { "SNAPSHOT_F_COUNT",                           0x0244 },
     { "N_VIF_COUNT",                                0x0248 },
     { "SNAPSHOT_VIF_COUNT",                         0x024c },
     { "CUR_OFFSET",                                 0x0260 },
     { "CUR_HORZ_VERT_POSN",                         0x0264 },
     { "CUR_HORZ_VERT_OFF",                          0x0268 },
     { "CUR_CLR0",                                   0x026c },
     { "CUR_CLR1",                                   0x0270 },
     { "DAC_CRC_SIG",                                0x02cc },
     { "DDA_CONFIG",                                 0x02e0 },
     { "DDA_ON_OFF",                                 0x02e4 },
     { "VGA_DDA_CONFIG",                             0x02e8 },
     { "VGA_DDA_ON_OFF",                             0x02ec },
     { "OV0_Y_X_START",                              0x0400 },
     { "OV0_Y_X_END",                                0x0404 },
     { "OV0_EXCLUSIVE_HORZ",                         0x0408 },
     { "OV0_EXCLUSIVE_VERT",                         0x040c },
     { "OV0_REG_LOAD_CNTL",                          0x0410 },
     { "OV0_SCALE_CNTL",                             0x0420 },
     { "OV0_V_INC",                                  0x0424 },
     { "OV0_P1_V_ACCUM_INIT",                        0x0428 },
     { "OV0_P23_V_ACCUM_INIT",                       0x042c },
     { "OV0_P1_BLANK_LINES_AT_TOP",                  0x0430 },
     { "OV0_P23_BLANK_LINES_AT_TOP",                 0x0434 },
     { "OV0_VID_BUF0_BASE_ADRS",                     0x0440 },
     { "OV0_VID_BUF1_BASE_ADRS",                     0x0444 },
     { "OV0_VID_BUF2_BASE_ADRS",                     0x0448 },
     { "OV0_VID_BUF3_BASE_ADRS",                     0x044c },
     { "OV0_VID_BUF4_BASE_ADRS",                     0x0450 },
     { "OV0_VID_BUF5_BASE_ADRS",                     0x0454 },
     { "OV0_VID_BUF_PITCH0_VALUE",                   0x0460 },
     { "OV0_VID_BUF_PITCH1_VALUE",                   0x0464 },
     { "OV0_OCTWORDS_PER_LINE_M1",                   0x046c },
     { "OV0_AUTO_FLIP_CNTRL",                        0x0470 },
     { "OV0_DEINTERLACE_PATTERN",                    0x0474 },
     { "OV0_H_INC",                                  0x0480 },
     { "OV0_STEP_BY",                                0x0484 },
     { "OV0_P1_H_ACCUM_INIT",                        0x0488 },
     { "OV0_P23_H_ACCUM_INIT",                       0x048c },
     { "OV0_P1_X_START_END",                         0x0494 },
     { "OV0_P2_X_START_END",                         0x0498 },
     { "OV0_P3_X_START_END",                         0x049c },
     { "OV0_FILTER_CNTL",                            0x04a0 },
     { "OV0_FOUR_TAP_COEF_0",                        0x04b0 },
     { "OV0_FOUR_TAP_COEF_1",                        0x04b4 },
     { "OV0_FOUR_TAP_COEF_2",                        0x04b8 },
     { "OV0_FOUR_TAP_COEF_3",                        0x04bc },
     { "OV0_FOUR_TAP_COEF_4",                        0x04c0 },
     { "OV0_COLOR_CNTL",                             0x04e0 },
     { "OV0_VIDEO_KEY_CLR",                          0x04e4 },
     { "OV0_VIDEO_KEY_MASK",                         0x04e8 },
     { "OV0_GRAPHICS_KEY_CLR",                       0x04ec },
     { "OV0_GRAPHICS_KEY_MASK",                      0x04f0 },
     { "OV0_KEY_CNTL",                               0x04f4 },
     { "OV0_TEST",                                   0x04f8 },
     { "SUBPIC_CNTL",                                0x0540 },
     { "PM4_BUFFER_OFFSET",                          0x0700 },
     { "PM4_BUFFER_CNTL",                            0x0704 },
     { "PM4_BUFFER_WM_CNTL",                         0x0708 },
     { "PM4_BUFFER_DL_RPTR_ADDR",                    0x070c },
     { "PM4_BUFFER_DL_RPTR",                         0x0710 },
     { "PM4_BUFFER_DL_WPTR",                         0x0714 },
     { "PM4_VC_FPU_SETUP",                           0x071c },
     { "PM4_FPU_CNTL",                               0x0720 },
     { "PM4_VC_FORMAT",                              0x0724 },
     { "PM4_VC_CNTL",                                0x0728 },
     { "PM4_VC_I01",                                 0x072c },
     { "PM4_VC_VLOFF",                               0x0730 },
     { "PM4_VC_VLSIZE",                              0x0734 },
     { "PM4_IW_INDOFF",                              0x0738 },
     { "PM4_IW_INDSIZE",                             0x073c },
     { "PM4_FPU_FPX0",                               0x0740 },
     { "CRC_CMDFIFO_ADDR",                           0x0740 },
     { "PM4_FPU_FPY0",                               0x0744 },
     { "CRC_CMDFIFO_DOUT",                           0x0744 },
     { "PM4_FPU_FPX1",                               0x0748 },
     { "PM4_FPU_FPY1",                               0x074c },
     { "PM4_FPU_FPX2",                               0x0750 },
     { "PM4_FPU_FPY2",                               0x0754 },
     { "PM4_FPU_FPY3",                               0x0758 },
     { "PM4_FPU_FPY4",                               0x075c },
     { "PM4_FPU_FPY5",                               0x0760 },
     { "PM4_FPU_FPY6",                               0x0764 },
     { "PM4_FPU_FPR",                                0x0768 },
     { "PM4_FPU_FPG",                                0x076c },
     { "PM4_FPU_FPB",                                0x0770 },
     { "PM4_FPU_FPA",                                0x0774 },
     { "PM4_FPU_INTXY0",                             0x0780 },
     { "PM4_FPU_INTXY1",                             0x0784 },
     { "PM4_FPU_INTXY2",                             0x0788 },
     { "PM4_FPU_INTARGB",                            0x078c },
     { "PM4_FPU_FPTWICEAREA",                        0x0790 },
     { "PM4_FPU_DMAJOR01",                           0x0794 },
     { "PM4_FPU_DMAJOR12",                           0x0798 },
     { "PM4_FPU_DMAJOR02",                           0x079c },
     { "PM4_FPU_STAT",                               0x07a0 },
     { "PM4_STAT",                                   0x07b8 },
     { "PM4_TEST_CNTL",                              0x07d0 },
     { "PM4_MICROCODE_ADDR",                         0x07d4 },
     { "PM4_MICROCODE_RADDR",                        0x07d8 },
     { "PM4_MICROCODE_DATAH",                        0x07dc },
     { "PM4_MICROCODE_DATAL",                        0x07e0 },
     { "PM4_CMDFIFO_ADDR",                           0x07e4 },
     { "PM4_CMDFIFO_DATAH",                          0x07e8 },
     { "PM4_CMDFIFO_DATAL",                          0x07ec },
     { "PM4_BUFFER_ADDR",                            0x07f0 },
     { "PM4_BUFFER_DATAH",                           0x07f4 },
     { "PM4_BUFFER_DATAL",                           0x07f8 },
     { "PM4_MICRO_CNTL",                             0x07fc },
     { "VID_BUFFER_CONTROL",                         0x0900 },
     { "CAP_INT_CNTL",                               0x0908 },
     { "CAP_INT_STATUS",                             0x090c },
     { "CAP0_BUF0_OFFSET",                           0x0920 },
     { "CAP0_BUF1_OFFSET",                           0x0924 },
     { "CAP0_BUF0_EVEN_OFFSET",                      0x0928 },
     { "CAP0_BUF1_EVEN_OFFSET",                      0x092c },
     { "CAP0_BUF_PITCH",                             0x0930 },
     { "CAP0_V_WINDOW",                              0x0934 },
     { "CAP0_H_WINDOW",                              0x0938 },
     { "CAP0_VBI_ODD_OFFSET",                        0x093c },
     { "CAP0_VBI_EVEN_OFFSET",                       0x0940 },
     { "CAP0_VBI_V_WINDOW",                          0x0944 },
     { "CAP0_VBI_H_WINDOW",                          0x0948 },
     { "CAP0_PORT_MODE_CNTL",                        0x094c },
     { "CAP0_TRIG_CNTL",                             0x0950 },
     { "CAP0_DEBUG",                                 0x0954 },
     { "CAP0_CONFIG",                                0x0958 },
     { "CAP0_ANC_ODD_OFFSET",                        0x095c },
     { "CAP0_ANC_EVEN_OFFSET",                       0x0960 },
     { "CAP0_ANC_H_WINDOW",                          0x0964 },
     { "CAP0_VIDEO_SYNC_TEST",                       0x0968 },
     { "CAP0_ONESHOT_BUF_OFFSET",                    0x096c },
     { "CAP0_BUF_STATUS",                            0x0970 },
     { "CAP0_DWNSC_XRATIO",                          0x0978 },
     { "CAP0_XSHARPNESS",                            0x097c },
     { "CAP1_BUF0_OFFSET",                           0x0990 },
     { "CAP1_BUF1_OFFSET",                           0x0994 },
     { "CAP1_BUF0_EVEN_OFFSET",                      0x0998 },
     { "CAP1_BUF1_EVEN_OFFSET",                      0x099c },
     { "CAP1_BUF_PITCH",                             0x09a0 },
     { "CAP1_V_WINDOW",                              0x09a4 },
     { "CAP1_H_WINDOW",                              0x09a8 },
     { "CAP1_VBI_ODD_OFFSET",                        0x09ac },
     { "CAP1_VBI_EVEN_OFFSET",                       0x09b0 },
     { "CAP1_VBI_V_WINDOW",                          0x09b4 },
     { "CAP1_VBI_H_WINDOW",                          0x09b8 },
     { "CAP1_PORT_MODE_CNTL",                        0x09bc },
     { "CAP1_TRIG_CNTL",                             0x09c0 },
     { "CAP1_DEBUG",                                 0x09c4 },
     { "CAP1_CONFIG",                                0x09c8 },
     { "CAP1_ANC_ODD_OFFSET",                        0x09cc },
     { "CAP1_ANC_EVEN_OFFSET",                       0x09d0 },
     { "CAP1_ANC_H_WINDOW",                          0x09d4 },
     { "CAP1_VIDEO_SYNC_TEST",                       0x09d8 },
     { "CAP1_ONESHOT_BUF_OFFSET",                    0x09dc }, 
     { "CAP1_BUF_STATUS",                            0x09e0 },
     { "CAP1_DWNSC_XRATIO",                          0x09e8 },
     { "CAP1_XSHARPNESS",                            0x09ec }, 
     { "BM_FRAME_BUF_OFFSET",                        0x0a00 },
     { "BM_SYSTEM_MEM_ADDR",                         0x0a04 },
     { "BM_COMMAND",                                 0x0a08 },
     { "BM_STATUS",                                  0x0a0c },
     { "BM_QUEUE_STATUS",                            0x0a10 },
     { "BM_QUEUE_FREE_STATUS",                       0x0A14 },
     { "BM_CHUNK_0_VAL",                             0x0a18 },
     { "BM_CHUNK_1_VAL",                             0x0a1C },
     { "BM_VIP0_BUF",                                0x0A20 },
     { "BM_VIP0_ACTIVE",                             0x0A24 },
     { "BM_VIP1_BUF",                                0x0A30 },
     { "BM_VIP1_ACTIVE",                             0x0A34 },
     { "BM_VIP2_BUF",                                0x0A40 },
     { "BM_VIP2_ACTIVE",                             0x0A44 },
     { "BM_VIP3_BUF",                                0x0A50 },
     { "BM_VIP3_ACTIVE",                             0x0A54 },
     { "BM_VIDCAP_BUF0",                             0x0a60 },
     { "BM_VIDCAP_BUF1",                             0x0a64 },
     { "BM_VIDCAP_BUF2",                             0x0a68 },
     { "BM_VIDCAP_ACTIVE",                           0x0a6c },
     { "BM_GUI",                                     0x0a80 },
     { "SURFACE_DELAY",                              0x0b00 },
 
/******************************************************************************
 * PCI Config Space Alias Block Memory Mapped Registers (header only)         *
 ******************************************************************************/
     { "PCIHDR_VENDOR_ID",                           0x0f00 },
     { "PCIHDR_DEVICE_ID",                           0x0f02 },
     { "PCIHDR_COMMAND",                             0x0f04 },
     { "PCIHDR_STATUS",                              0x0f06 },
     { "PCIHDR_REVISION_ID",                         0x0f08 },
     { "PCIHDR_REGPROG_INF",                         0x0f09 },
     { "PCIHDR_SUB_CLASS",                           0x0f0a },
     { "PCIHDR_BASE_CODE",                           0x0f0b },
     { "PCIHDR_CACHE_LINE",                          0x0f0c },
     { "PCIHDR_LATENCY",                             0x0f0d },
     { "PCIHDR_HEADER",                              0x0f0e },
     { "PCIHDR_BIST",                                0x0f0f },
     { "PCIHDR_MEM_BASE",                            0x0f10 },
     { "PCIHDR_IO_BASE",                             0x0f14 },
     { "PCIHDR_REG_BASE",                            0x0f18 },
     { "PCIHDR_ADAPTER_ID",                          0x0f2c },
     { "PCIHDR_BIOS_ROM",                            0x0f30 },
     { "PCIHDR_CAPABILITIES_PTR",                    0x0f34 },
     { "PCIHDR_INTERRUPT_LINE",                      0x0f3c },
     { "PCIHDR_INTERRUPT_PIN",                       0x0f3d },
     { "PCIHDR_MIN_GRANT",                           0x0f3e },
     { "PCIHDR_MAX_LATENCY",                         0x0f3f },
     { "PCIHDR_CAPABILITIES_ID",                     0x0f50 },
     { "PCIHDR_AGP_STATUS",                          0x0f54 },
     { "PCIHDR_AGP_COMMAND",                         0x0f58 },
     { "PCIHDR_PMI_REGISTER",                        0x0f5c },
     { "PCIHDR_PWR_MNGMT_CNTL_STATUS",               0x0f60 },
 
/******************************************************************************
 *               NON-GUI PM4 FIFO DATA Registers                              *
 ******************************************************************************/
     { "PM4_FIFO_DATA_EVEN",                         0x1000 },
     { "PM4_FIFO_DATA_ODD",                          0x1004 },

/******************************************************************************
 *                  GUI Block Memory Mapped Registers                         *
 *                     These registers are FIFOed.                            *
 ******************************************************************************/
     { "DST_OFFSET",                                 0x1404 },
     { "DST_PITCH",                                  0x1408 },
     { "DST_WIDTH",                                  0x140c }, 
     { "DST_HEIGHT",                                 0x1410 },
     { "SRC_X",                                      0x1414 },
     { "SRC_Y",                                      0x1418 },
     { "DST_X",                                      0x141c },
     { "DST_Y",                                      0x1420 },
     { "SRC_PITCH_OFFSET",                           0x1428 },
     { "DST_PITCH_OFFSET",                           0x142c },
     { "SRC_Y_X",                                    0x1434 },
     { "DST_Y_X",                                    0x1438 },
     { "DST_HEIGHT_WIDTH",                           0x143c },
     { "DP_GUI_MASTER_CNTL",                         0x146c },
     { "BRUSH_SCALE",                                0x1470 },
     { "BRUSH_Y_X",                                  0x1474 },
     { "DP_BRUSH_BKGD_CLR",                          0x1478 },
     { "DP_BRUSH_FRGD_CLR",                          0x147c },
     { "BRUSH_DATA0",                                0x1480 },
     { "BRUSH_DATA1",                                0x1484 },
     { "BRUSH_DATA2",                                0x1488 },
     { "BRUSH_DATA3",                                0x148c },
     { "BRUSH_DATA4",                                0x1490 },
     { "BRUSH_DATA5",                                0x1494 },
     { "BRUSH_DATA6",                                0x1498 },
     { "BRUSH_DATA7",                                0x149c },
     { "BRUSH_DATA8",                                0x14a0 },
     { "BRUSH_DATA9",                                0x14a4 },
     { "BRUSH_DATA10",                               0x14a8 },
     { "BRUSH_DATA11",                               0x14ac },
     { "BRUSH_DATA12",                               0x14b0 },
     { "BRUSH_DATA13",                               0x14b4 },
     { "BRUSH_DATA14",                               0x14b8 },
     { "BRUSH_DATA15",                               0x14bc },
     { "BRUSH_DATA16",                               0x14c0 },
     { "BRUSH_DATA17",                               0x14c4 },
     { "BRUSH_DATA18",                               0x14c8 },
     { "BRUSH_DATA19",                               0x14cc },
     { "BRUSH_DATA20",                               0x14d0 },
     { "BRUSH_DATA21",                               0x14d4 },
     { "BRUSH_DATA22",                               0x14d8 },
     { "BRUSH_DATA23",                               0x14dc },
     { "BRUSH_DATA24",                               0x14e0 },
     { "BRUSH_DATA25",                               0x14e4 },
     { "BRUSH_DATA26",                               0x14e8 },
     { "BRUSH_DATA27",                               0x14ec },
     { "BRUSH_DATA28",                               0x14f0 },
     { "BRUSH_DATA29",                               0x14f4 },
     { "BRUSH_DATA30",                               0x14f8 },
     { "BRUSH_DATA31",                               0x14fc },
     { "BRUSH_DATA32",                               0x1500 },
     { "BRUSH_DATA33",                               0x1504 },
     { "BRUSH_DATA34",                               0x1508 },
     { "BRUSH_DATA35",                               0x150c },
     { "BRUSH_DATA36",                               0x1510 },
     { "BRUSH_DATA37",                               0x1514 },
     { "BRUSH_DATA38",                               0x1518 },
     { "BRUSH_DATA39",                               0x151c },
     { "BRUSH_DATA40",                               0x1520 },
     { "BRUSH_DATA41",                               0x1524 },
     { "BRUSH_DATA42",                               0x1528 },
     { "BRUSH_DATA43",                               0x152c },
     { "BRUSH_DATA44",                               0x1530 },
     { "BRUSH_DATA45",                               0x1534 },
     { "BRUSH_DATA46",                               0x1538 },
     { "BRUSH_DATA47",                               0x153c },
     { "BRUSH_DATA48",                               0x1540 },
     { "BRUSH_DATA49",                               0x1544 },
     { "BRUSH_DATA50",                               0x1548 },
     { "BRUSH_DATA51",                               0x154c },
     { "BRUSH_DATA52",                               0x1550 },
     { "BRUSH_DATA53",                               0x1554 },
     { "BRUSH_DATA54",                               0x1558 }, 
     { "BRUSH_DATA55",                               0x155c },
     { "BRUSH_DATA56",                               0x1560 },
     { "BRUSH_DATA57",                               0x1564 },
     { "BRUSH_DATA58",                               0x1568 },
     { "BRUSH_DATA59",                               0x156c },
     { "BRUSH_DATA60",                               0x1570 },
     { "BRUSH_DATA61",                               0x1574 },
     { "BRUSH_DATA62",                               0x1578 },
     { "BRUSH_DATA63",                               0x157c },
     { "DST_WIDTH_X",                                0x1588 },
     { "DST_HEIGHT_WIDTH_8",                         0x158c },
     { "SRC_X_Y",                                    0x1590 },
     { "DST_X_Y",                                    0x1594 },
     { "DST_WIDTH_HEIGHT",                           0x1598 },
     { "DST_WIDTH_X_INCY",                           0x159c },
     { "DST_HEIGHT_Y",                               0x15a0 },
     { "DST_X_SUB",                                  0x15a4 },
     { "DST_Y_SUB",                                  0x15a8 },
     { "SRC_OFFSET",                                 0x15ac },
     { "SRC_PITCH",                                  0x15b0 },
     { "DST_HEIGHT_WIDTH_BW",                        0x15b4 },
     { "CLR_CMP_CNTL",                               0x15c0 },
     { "CLR_CMP_CLR_SRC",                            0x15c4 },
     { "CLR_CMP_CLR_DST",                            0x15c8 },
     { "CLR_CMP_MASK",                               0x15cc },
     { "DP_SRC_FRGD_CLR",                            0x15d8 },
     { "DP_SRC_BKGD_CLR",                            0x15dc },
     { "GUI_SCRATCH_REG0",                           0x15e0 },
     { "GUI_SCRATCH_REG1",                           0x15e4 },
     { "GUI_SCRATCH_REG2",                           0x15e8 },
     { "GUI_SCRATCH_REG3",                           0x15ec },
     { "GUI_SCRATCH_REG4",                           0x15f0 },
     { "GUI_SCRATCH_REG5",                           0x15f4 },
     { "LEAD_BRES_ERR",                              0x1600 },
     { "LEAD_BRES_INC",                              0x1604 }, 
     { "LEAD_BRES_DEC",                              0x1608 },
     { "TRAIL_BRES_ERR",                             0x160c },
     { "TRAIL_BRES_INC",                             0x1610 },
     { "TRAIL_BRES_DEC",                             0x1614 },
     { "TRAIL_X",                                    0x1618 },
     { "LEAD_BRES_LNTH",                             0x161c },
     { "TRAIL_X_SUB",                                0x1620 },
     { "LEAD_BRES_LNTH_SUB",                         0x1624 },
     { "DST_BRES_ERR",                               0x1628 },
     { "DST_BRES_INC",                               0x162c },
     { "DST_BRES_DEC",                               0x1630 },
     { "DST_BRES_LNTH",                              0x1634 },
     { "DST_BRES_LNTH_SUB",                          0x1638 },
     { "SC_LEFT",                                    0x1640 },
     { "SC_RIGHT",                                   0x1644 },
     { "SC_TOP",                                     0x1648 },
     { "SC_BOTTOM",                                  0x164c },
     { "SRC_SC_RIGHT",                               0x1654 },
     { "SRC_SC_BOTTOM",                              0x165c },
     { "AUX_SC_CNTL",                                0x1660 },
     { "AUX1_SC_LEFT",                               0x1664 },
     { "AUX1_SC_RIGHT",                              0x1668 },
     { "AUX1_SC_TOP",                                0x166c },
     { "AUX1_SC_BOTTOM",                             0x1670 },
     { "AUX2_SC_LEFT",                               0x1674 },
     { "AUX2_SC_RIGHT",                              0x1678 },
     { "AUX2_SC_TOP",                                0x167c },
     { "AUX2_SC_BOTTOM",                             0x1680 },
     { "AUX3_SC_LEFT",                               0x1684 },
     { "AUX3_SC_RIGHT",                              0x1688 },
     { "AUX3_SC_TOP",                                0x168c },
     { "AUX3_SC_BOTTOM",                             0x1690 },
     { "GUI_DEBUG0",                                 0x16a0 },
     { "GUI_DEBUG1",                                 0x16a4 },
     { "GUI_TIMEOUT",                                0x16b0 },
     { "GUI_TIMEOUT0",                               0x16b4 },
     { "GUI_TIMEOUT1",                               0x16b8 },
     { "GUI_PROBE",                                  0x16bc },
     { "DP_CNTL",                                    0x16c0 },
     { "DP_DATATYPE",                                0x16c4 },
     { "DP_MIX",                                     0x16c8 },
     { "DP_WRITE_MASK",                              0x16cc },
     { "DP_CNTL_XDIR_YDIR_YMAJOR",                   0x16d0 },
     { "DEFAULT_OFFSET",                             0x16e0 },
     { "DEFAULT_PITCH",                              0x16e4 },
     { "DEFAULT_SC_BOTTOM_RIGHT",                    0x16e8 },
     { "SC_TOP_LEFT",                                0x16ec },
     { "SC_BOTTOM_RIGHT",                            0x16f0 },
     { "SRC_SC_BOTTOM_RIGHT",                        0x16f4 },
     { "WAIT_UNTIL",                                 0x1720 },
     { "CACHE_CNTL",                                 0x1724 },
     { "GUI_STAT",                                   0x1740 },
     { "PC_GUI_MODE",                                0x1744 },
     { "PC_GUI_CTLSTAT",                             0x1748 },
     { "PC_DEBUG_MODE",                              0x1760 },
     { "BRES_DST_ERR_DEC",                           0x1780 },
     { "TRAIL_BRES_T12_ERR_DEC",                     0x1784 },
     { "TRAIL_BRES_T12_INC",                         0x1788 },
     { "DP_T12_CNTL",                                0x178c },
     { "DST_BRES_T1_LNTH",                           0x1790 },
     { "DST_BRES_T2_LNTH",                           0x1794 },
     { "HOST_DATA0",                                 0x17c0 },
     { "HOST_DATA1",                                 0x17c4 },
     { "HOST_DATA2",                                 0x17c8 },
     { "HOST_DATA3",                                 0x17cc },
     { "HOST_DATA4",                                 0x17d0 },
     { "HOST_DATA5",                                 0x17d4 },
     { "HOST_DATA6",                                 0x17d8 },
     { "HOST_DATA7",                                 0x17dc },
     { "HOST_DATA_LAST",                             0x17e0 },
     { "SECONDARY_SCALE_PITCH",                      0x1980 },
     { "SECONDARY_SCALE_X_INC",                      0x1984 },
     { "SECONDARY_SCALE_Y_INC",                      0x1988 },
     { "SECONDARY_SCALE_HACC",                       0x198c },
     { "SECONDARY_SCALE_VACC",                       0x1990 },
     { "SCALE_SRC_HEIGHT_WIDTH",                     0x1994 },
     { "SCALE_OFFSET_0",                             0x1998 },
     { "SCALE_PITCH",                                0x199c },
     { "SCALE_X_INC",                                0x19a0 },
     { "SCALE_Y_INC",                                0x19a4 },
     { "SCALE_HACC",                                 0x19a8 },
     { "SCALE_VACC",                                 0x19ac },
     { "SCALE_DST_X_Y",                              0x19b0 },
     { "SCALE_DST_HEIGHT_WIDTH",                     0x19b4 },
     { "SCALE_3D_CNTL",                              0x1a00 },
     { "SCALE_3D_DATATYPE",                          0x1a20 },
     { "SETUP_CNTL",                                 0x1bc4 },
     { "SOLID_COLOR",                                0x1bc8 },
     { "WINDOW_XY_OFFSET",                           0x1bcc },
     { "DRAW_LINE_POINT",                            0x1bd0 },
     { "SETUP_CNTL_PM4",                             0x1bd4 },
     { "DST_PITCH_OFFSET_C",                         0x1c80 },
     { "DP_GUI_MASTER_CNTL_C",                       0x1c84 },
     { "SC_TOP_LEFT_C",                              0x1c88 },
     { "SC_BOTTOM_RIGHT_C",                          0x1c8c },
     { "CLR_CMP_MASK_3D",                            0x1A28 },
     { "MISC_3D_STATE_CNTL_REG",                     0x1CA0 },
     { "MC_SRC1_CNTL",                               0x19D8 },
     { "TEX_CNTL",                                   0x1800 },

};

/******************************************************************************
 *                                                                            *
 *            RAGE128 INDIRECT REGISTER INDICES CONSTANTS SECTION             *
 *                                                                            *
 ******************************************************************************/

#if 0

/******************************************************************************
 *                CLOCK_CNTL_INDEX Indexed Registers                          *
 ******************************************************************************/
     { "CLK_PIN_CNTL",                               0x0001},
     { "PPLL_CNTL",                                  0x0002},
     { "PPLL_REF_DIV",                               0x0003},
     { "PPLL_DIV_0",                                 0x0004},
     { "PPLL_DIV_1",                                 0x0005},
     { "PPLL_DIV_2",                                 0x0006},
     { "PPLL_DIV_3",                                 0x0007},
     { "VCLK_ECP_CNTL",                              0x0008},
     { "HTOTAL_CNTL",                                0x0009},
     { "X_MPLL_REF_FB_DIV",                          0x000a},
     { "XPLL_CNTL",                                  0x000b},
     { "XDLL_CNTL",                                  0x000c},
     { "XCLK_CNTL",                                  0x000d},
     { "MPLL_CNTL",                                  0x000e},
     { "MCLK_CNTL",                                  0x000f},
     { "AGP_PLL_CNTL",                               0x0010},
     { "FCP_CNTL",                                   0x0012},
     { "PLL_TEST_CNTL",                              0x0013}


#endif


char * find_mnemonic ( int address )
{
	int x = 0;
	int done = 0;
	char *retval = NULL;

	while ( !done )
	{		
		if (x > sizeof(Ati128Regs)/sizeof (struct Ati128RegType))
		{	done = TRUE;
		}
		
		if(!done && Ati128Regs[x].address == address )
		{	done = TRUE;
			retval = Ati128Regs[x].mnem;
		}
		x++;
	}
	return retval;	
}

void print_mnemonic ( FILE *w, int index, unsigned long int *AuxPtr  );

void print_mnemonic ( FILE *w, int index, unsigned long int *AuxPtr)
{
	char TempString [255];
	struct Ati128RegType *mnem_ptr = &Ati128Regs[index] ;

	sprintf(TempString, "Register, %04lX, %08lX %s", 
		mnem_ptr->address,  
		(AuxPtr[mnem_ptr->address / 4]), 
		mnem_ptr->mnem);
		
	if(w)fflush(w);
	if(w)fprintf(w,"%s\n",TempString);	
}


void DumpATI128StaticRegisters ( FILE * w, unsigned long int *AuxPtr, int reg_count )
{

	char TempString[256];
//	char OutputString[1634];

	unsigned long int ClkCtlVal;
	unsigned long int ClkCtlData;
//	unsigned long int LTemp;
	unsigned long int PLLNum;
	unsigned long int  RegNum;

	char *mnem_data;
    int y, index;
	char Tmpstr[250];

	Tmpstr [0] = '\0';


		if(w)fprintf(w,"PLL Register Dump : Rage128\n");

		ClkCtlVal = AuxPtr[CLOCK_CNTL_INDEX];
		// read out PLL values
		for (PLLNum = 0; PLLNum < 32; PLLNum++)
		{
//			ClkCtlVal &= 0xFF0001FF;
//			ClkCtlVal |= PLLNum << 10;
//			AuxPtr[0x0124] = ClkCtlVal;
//			ClkCtlVal = AuxPtr[0x0124];

			
// bh new code for Rage 128
//			ClkCtlVal = ((PLLNum &0x60) << 3) |(PLLNum & 0x1f);	 mask our index register index
																// add PLL_Div 0..3
			ClkCtlVal = (ClkCtlVal & 0xffffffE0 ) | (PLLNum & 0x1f);	// mask our index register index

			AuxPtr[CLOCK_CNTL_INDEX] = ClkCtlVal;							// write the index

			ClkCtlData = AuxPtr[CLOCK_CNTL_DATA] ;

			sprintf(TempString, "PLL Register, %2d, %08lX", PLLNum, ClkCtlData);
			if(w)fprintf(w,"%s\n",TempString);

//			strcat(OutputString, TempString);
//			m_OutputWindow.SetWindowText(OutputString);
		}

		if(w)fprintf(w,"\n\nFull Register Aperature Dump\n");




		// Read out block 0 register values.

	y =   sizeof (Ati128Regs ) / sizeof(struct Ati128RegType) ;
	
	for (index = 0 ; index < y ; index ++ )
	{	
		print_mnemonic ( w, index, AuxPtr);
	}


#if 0 

		for (RegNum = 0; RegNum < (unsigned long)0x03ff; RegNum+=4)
		{

			mnem_data = find_mnemonic ( RegNum );
			if (!mnem_data) 
			{	mnem_data = "";
				Tmpstr [0] = '\0';
			}
			else
			{	strcpy(Tmpstr, mnem_data);
			}

			// Hope no faults from reading write-only registers (with luck will just ignore).
			sprintf(TempString, "Register, %04lX, %08lX %s", RegNum, (AuxPtr[RegNum / 4]), Tmpstr);
			if(w)fflush(w);

//			strcat(OutputString, TempString);
			if(w)fprintf(w,"%s\n",TempString);

//			m_OutputWindow.SetWindowText(OutputString);
		}

#endif

		// Read out LCD register values.
#if 0
//		for (RegNum = 0; RegNum < 16; RegNum++)
//		{
//			LTemp = AuxPtr[0x0129]; // 1K / 4 + 0x29
//			LTemp &= 0xFFFFFFF0;
//			LTemp |= RegNum;
//			AuxPtr[0x0129] = LTemp; // Access specified LCD reg.
//			LTemp = AuxPtr[0x012A]; // Now read the value itself.
//
//			sprintf(TempString, "LCD Register %02d:  0x%08lX", RegNum, LTemp);
//			strcat(OutputString, TempString);
//
//			if(w)fprintf(w,"%s\n",TempString);
//
//			m_OutputWindow.SetWindowText(OutputString);
//		}
#endif



}

void DumpATI128PLL ( FILE * w, unsigned long int *auxptr )
{

}


