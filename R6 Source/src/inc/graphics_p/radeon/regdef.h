#ifndef REGDEF_H
#define REGDEF_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/************************************************************************
 *
 * Module Name:  regdef.h 
 * Project:      Radeon Register byte offset
 * Device:       Radeon 
 *
 * Description:  Header file for Radeon 
 *     Header file for Radeon Chapter 3 sample code                       
 *     -all register offsets are expressed in bytes-                        
 *
 * (c) 2000 ATI Technologies Inc.  
 *
 * All rights reserved.  This notice is intended as a precaution against
 * inadvertent publication and does not imply publication or any waiver
 * of confidentiality.  The year included in the foregoing notice is the
 * year of creation of the work.
 *
 ************************************************************************/



// [blockIO] : io Registers

#include "regmask.h"
#include "regshift.h"
#include "regix.h"

#define ioMM_INDEX                               0x0000
#define ioMM_DATA                                0x0004
#define ioCLOCK_CNTL_INDEX                       0x0008
#define ioCLOCK_CNTL_DATA                        0x000C
#define ioBIOS_0_SCRATCH                         0x0010
#define ioBIOS_1_SCRATCH                         0x0014
#define ioBIOS_2_SCRATCH                         0x0018
#define ioBIOS_3_SCRATCH                         0x001C
#define ioBUS_CNTL                               0x0030
#define ioBUS_CNTL1                              0x0034	
#define ioMEM_VGA_WP_SEL                         0x0038
#define ioMEM_VGA_RP_SEL                         0x003C
#define ioGEN_INT_CNTL                           0x0040
#define ioGEN_INT_STATUS                         0x0044
#define ioHI_STAT                                0x004C
#define ioDAC_CNTL                               0x0058 
#define ioCRTC_GEN_CNTL                          0x0050
#define ioCRTC_EXT_CNTL                          0x0054
#define ioCRTC_STATUS                            0x005C 
#define ioGPIO_VGA_DDC                           0x0060 
#define ioGPIO_DVI_DDC                           0x0064
#define ioGPIO_MONID                             0x0068 
#define ioPALETTE_INDEX                          0x00B0 
#define ioPALETTE_DATA                           0x00B4 
#define ioPALETTE_30_DATA                        0x00B8 
#define ioCONFIG_CNTL                            0x00E0
#define ioCONFIG_XSTRAP                          0x00E4 
#define ioRBBM_CNTL                              0x00EC 
#define ioRBBM_SOFT_RESET                        0x00F0 
#define ioCONFIG_MEMSIZE                         0x00F8
#define ioTVOUT_0_SCRATCH                        0x0088 
#define ioTVOUT_1_SCRATCH                        0x008C
#define ioI2C_CNTL_0                             0x0090 
#define ioI2C_CNTL_1                             0x0094 
#define ioI2C_DATA                               0x0098 
#define ioVIPH_REG_ADDR                          0x0080 
#define ioVIPH_REG_DATA                          0x0084
// [sparseIO] : io Registers

#define ioGENENB                                 0x03C3
#define ioGENMO_WT                               0x03C2
#define ioGENMO_RD                               0x03CC
#define ioGRPH8_IDX                              0x03CE
#define ioGRPH8_DATA                             0x03CF
#define ioSEQ8_IDX                               0x03C4
#define ioSEQ8_DATA                              0x03C5
#define ioCRTC8_IDX                              0x03B4
#define ioCRTC8_IDX_alt_1                        0x03D4
#define ioCRTC8_DATA                             0x03B5
#define ioCRTC8_DATA_alt_1                       0x03D5
#define ioGENFC_RD                               0x03CA
#define ioGENFC_WT                               0x03BA
#define ioGENFC_WT_alt_1                         0x03DA
#define ioGENS0                                  0x03C2
#define ioGENS1                                  0x03BA
#define ioGENS1_alt_1                            0x03DA
#define ioDAC_DATA                               0x03C9
#define ioDAC_MASK                               0x03C6
#define ioDAC_R_INDEX                            0x03C7
#define ioDAC_W_INDEX                            0x03C8
#define ioATTRX                                  0x03C0
#define ioATTRDW                                 0x03C0
#define ioATTRDR                                 0x03C1

// [primaryRegisterAperture] :  Registers

#define MM_INDEX                               0x0000
#define MM_DATA                                0x0004
#define BUS_CNTL                               0x0030
#define HI_STAT                                0x004C
#define BUS_CNTL1                              0x0034
#define CONFIG_CNTL                            0x00E0
#define CONFIG_MEMSIZE                         0x00F8
#define CONFIG_APER_0_BASE                     0x0100
#define CONFIG_APER_1_BASE                     0x0104
#define CONFIG_APER_SIZE                       0x0108
#define CONFIG_REG_1_BASE                      0x010C
#define CONFIG_REG_APER_SIZE                   0x0110
#define PAD_AGPINPUT_DELAY                     0x0164
#define PAD_CTLR_STRENGTH                      0x0168
#define PAD_CTLR_UPDATE                        0x016C
#define AGP_CNTL                               0x0174
#define BM_STATUS                              0x0160
#define VENDOR_ID                              0x0F00
#define DEVICE_ID                              0x0F02
#define COMMAND                                0x0F04
#define STATUS                                 0x0F06
#define REVISION_ID                            0x0F08
#define REGPROG_INF                            0x0F09
#define SUB_CLASS                              0x0F0A
#define BASE_CODE                              0x0F0B
#define CACHE_LINE                             0x0F0C
#define LATENCY                                0x0F0D
#define HEADER                                 0x0F0E
#define BIST                                   0x0F0F
#define REG_MEM_BASE                           0x0F10
#define REG_IO_BASE                            0x0F14
#define REG_REG_BASE                           0x0F18
#define ADAPTER_ID                             0x0F2C
#define BIOS_ROM                               0x0F30
#define CAPABILITIES_PTR                       0x0F34
#define INTERRUPT_LINE                         0x0F3C
#define INTERRUPT_PIN                          0x0F3D
#define MIN_GRANT                              0x0F3E
#define MAX_LATENCY                            0x0F3F
#define ADAPTER_ID_W                           0x0F4C
#define PMI_CAP_ID                             0x0F50
#define PMI_NXT_CAP_PTR                        0x0F51 
#define PMI_PMC_REG                            0x0F52
#define PM_STATUS                              0x0F54 
#define PMI_DATA                               0x0F57
#define AGP_CAP_ID                             0x0F58 
#define AGP_STATUS                             0x0F5C 
#define AGP_COMMAND                            0x0F60 
#define AIC_CTRL                               0x01D0
#define AIC_STAT                               0x01D4
#define AIC_PT_BASE                            0x01D8
#define AIC_LO_ADDR                            0x01DC
#define AIC_HI_ADDR                            0x01E0
#define AIC_TLB_ADDR                           0x01E4
#define AIC_TLB_DATA                           0x01E8
#define DAC_CNTL                               0x0058
#define CRTC_GEN_CNTL                          0x0050
#define MEM_CNTL                               0x0140
#define EXT_MEM_CNTL                           0x0144
#define MC_AGP_LOCATION                        0x014C
#define MEM_IO_CNTL_A0                         0x0178 
#define MEM_INIT_LATENCY_TIMER                 0x0154 
#define MEM_SDRAM_MODE_REG                     0x0158
#define AGP_BASE                               0x0170  
#define MEM_IO_CNTL_A1                         0x017C 
#define MEM_IO_CNTL_B0                         0x0180 
#define MEM_IO_CNTL_B1                         0x0184
#define MC_DEBUG                               0x0188
#define MC_STATUS                              0x0150 
#define MEM_IO_OE_CNTL                         0x018C
#define MC_FB_LOCATION                         0x0148 
#define HOST_PATH_CNTL                         0x0130
#define MEM_VGA_WP_SEL                         0x0038
#define MEM_VGA_RP_SEL                         0x003C
#define HDP_DEBUG                              0x0138
#define SW_SEMAPHORE                           0x013C
#define SURFACE_CNTL                           0x0B00 
#define SURFACE0_LOWER_BOUND                   0x0B04
#define SURFACE1_LOWER_BOUND                   0x0B14
#define SURFACE2_LOWER_BOUND                   0x0B24
#define SURFACE3_LOWER_BOUND                   0x0B34
#define SURFACE4_LOWER_BOUND                   0x0B44
#define SURFACE5_LOWER_BOUND                   0x0B54
#define SURFACE6_LOWER_BOUND                   0x0B64
#define SURFACE7_LOWER_BOUND                   0x0B74
#define SURFACE0_UPPER_BOUND                   0x0B08
#define SURFACE1_UPPER_BOUND                   0x0B18
#define SURFACE2_UPPER_BOUND                   0x0B28
#define SURFACE3_UPPER_BOUND                   0x0B38
#define SURFACE4_UPPER_BOUND                   0x0B48
#define SURFACE5_UPPER_BOUND                   0x0B58
#define SURFACE6_UPPER_BOUND                   0x0B68
#define SURFACE7_UPPER_BOUND                   0x0B78
#define SURFACE0_INFO                          0x0B0C
#define SURFACE1_INFO                          0x0B1C
#define SURFACE2_INFO                          0x0B2C
#define SURFACE3_INFO                          0x0B3C
#define SURFACE4_INFO                          0x0B4C
#define SURFACE5_INFO                          0x0B5C
#define SURFACE6_INFO                          0x0B6C
#define SURFACE7_INFO                          0x0B7C
#define SURFACE_ACCESS_FLAGS                   0x0BF8
#define SURFACE_ACCESS_CLR                     0x0BFC
#define GEN_INT_CNTL                           0x0040
#define GEN_INT_STATUS                         0x0044
#define CRTC_EXT_CNTL                          0x0054
#define WAIT_UNTIL                             0x1720
#define ISYNC_CNTL                             0x1724 
#define RBBM_GUICNTL                           0x172C
#define RBBM_STATUS                            0x0E40 
#define RBBM_STATUS_alt_1                      0x1740 
#define RBBM_CNTL                              0x00EC
#define RBBM_CNTL_alt_1                        0x0E44
#define RBBM_SOFT_RESET                        0x00F0 
#define RBBM_SOFT_RESET_alt_1                  0x0E48
#define NQWAIT_UNTIL                           0x0E50
#define RBBM_DEBUG                             0x0E6C
#define RBBM_CMDFIFO_ADDR                      0x0E70
#define RBBM_CMDFIFO_DATAL                     0x0E74
#define RBBM_CMDFIFO_DATAH                     0x0E78
#define RBBM_CMDFIFO_STAT                      0x0E7C
#define CRTC_STATUS                            0x005C
#define GPIO_VGA_DDC                           0x0060
#define GPIO_DVI_DDC                           0x0064
#define GPIO_MONID                             0x0068
#define PALETTE_INDEX                          0x00B0
#define PALETTE_DATA                           0x00B4
#define PALETTE_30_DATA                        0x00B8
#define CRTC_H_TOTAL_DISP                      0x0200
#define CRTC_H_SYNC_STRT_WID                   0x0204
#define CRTC_V_TOTAL_DISP                      0x0208
#define CRTC_V_SYNC_STRT_WID                   0x020C
#define CRTC_VLINE_CRNT_VLINE                  0x0210
#define CRTC_CRNT_FRAME                        0x0214
#define CRTC_GUI_TRIG_VLINE                    0x0218
#define CRTC_DEBUG                             0x021C
#define CRTC_OFFSET_RIGHT                      0x0220
#define CRTC_OFFSET                            0x0224
#define CRTC_OFFSET_CNTL                       0x0228
#define CRTC_PITCH                             0x022C
#define OVR_CLR                                0x0230
#define OVR_WID_LEFT_RIGHT                     0x0234
#define OVR_WID_TOP_BOTTOM                     0x0238
#define DISPLAY_BASE_ADDR                      0x023C
#define SNAPSHOT_VH_COUNTS                     0x0240
#define SNAPSHOT_F_COUNT                       0x0244
#define N_VIF_COUNT                            0x0248
#define SNAPSHOT_VIF_COUNT                     0x024C
#define FP_CRTC_H_TOTAL_DISP                   0x0250
#define FP_CRTC_V_TOTAL_DISP                   0x0254
#define CRT_CRTC_H_SYNC_STRT_WID               0x0258
#define CRT_CRTC_V_SYNC_STRT_WID               0x025C
#define CUR_OFFSET                             0x0260
#define CUR_HORZ_VERT_POSN                     0x0264
#define CUR_HORZ_VERT_OFF                      0x0268
#define CUR_CLR0                               0x026C
#define CUR_CLR1                               0x0270
#define FP_HORZ_VERT_ACTIVE                    0x0278
#define CRTC_MORE_CNTL                         0x027C
#define DAC_EXT_CNTL                           0x0280
#define FP_GEN_CNTL                            0x0284
#define FP_HORZ_STRETCH                        0x028C
#define FP_VERT_STRETCH                        0x0290
#define FP_H_SYNC_STRT_WID                     0x02C4
#define FP_V_SYNC_STRT_WID                     0x02C8
#define AUX_WINDOW_HORZ_CNTL                   0x02D8
#define AUX_WINDOW_VERT_CNTL                   0x02DC
#define GRPH_BUFFER_CNTL                       0x02F0
#define VGA_BUFFER_CNTL                        0x02F4
#define OV0_Y_X_START                          0x0400
#define OV0_Y_X_END                            0x0404
#define OV0_PIPELINE_CNTL                      0x0408 
#define OV0_REG_LOAD_CNTL                      0x0410
#define OV0_SCALE_CNTL                         0x0420
#define OV0_V_INC                              0x0424
#define OV0_P1_V_ACCUM_INIT                    0x0428
#define OV0_P23_V_ACCUM_INIT                   0x042C
#define OV0_P1_BLANK_LINES_AT_TOP              0x0430
#define OV0_P23_BLANK_LINES_AT_TOP             0x0434
#define OV0_BASE_ADDR                          0x043C
#define OV0_VID_BUF0_BASE_ADRS                 0x0440
#define OV0_VID_BUF1_BASE_ADRS                 0x0444
#define OV0_VID_BUF2_BASE_ADRS                 0x0448
#define OV0_VID_BUF3_BASE_ADRS                 0x044C
#define OV0_VID_BUF4_BASE_ADRS                 0x0450
#define OV0_VID_BUF5_BASE_ADRS                 0x0454
#define OV0_VID_BUF_PITCH0_VALUE               0x0460
#define OV0_VID_BUF_PITCH1_VALUE               0x0464
#define OV0_AUTO_FLIP_CNTRL                    0x0470
#define OV0_DEINTERLACE_PATTERN                0x0474
#define OV0_SUBMIT_HISTORY                     0x0478
#define OV0_H_INC                              0x0480
#define OV0_STEP_BY                            0x0484
#define OV0_P1_H_ACCUM_INIT                    0x0488
#define OV0_P23_H_ACCUM_INIT                   0x048C
#define OV0_P1_X_START_END                     0x0494
#define OV0_P2_X_START_END                     0x0498
#define OV0_P3_X_START_END                     0x049C
#define OV0_FILTER_CNTL                        0x04A0
#define OV0_FOUR_TAP_COEF_0                    0x04B0
#define OV0_FOUR_TAP_COEF_1                    0x04B4
#define OV0_FOUR_TAP_COEF_2                    0x04B8
#define OV0_FOUR_TAP_COEF_3                    0x04BC
#define OV0_FOUR_TAP_COEF_4                    0x04C0
#define OV0_FLAG_CNTRL                         0x04DC
#define OV0_SLICE_CNTL                         0x04E0 
#define OV0_VID_KEY_CLR_LOW                    0x04E4 
#define OV0_VID_KEY_CLR_HIGH                   0x04E8 
#define OV0_GRPH_KEY_CLR_LOW                   0x04EC 
#define OV0_GRPH_KEY_CLR_HIGH                  0x04F0 
#define OV0_KEY_CNTL                           0x04F4
#define OV0_TEST                               0x04F8
#define SUBPIC_CNTL                            0x0540
#define SUBPIC_DEFCOLCON                       0x0544
#define SUBPIC_Y_X_START                       0x054C
#define SUBPIC_Y_X_END                         0x0550
#define SUBPIC_V_INC                           0x0554
#define SUBPIC_H_INC                           0x0558
#define SUBPIC_BUF0_OFFSET                     0x055C
#define SUBPIC_BUF1_OFFSET                     0x0560
#define SUBPIC_LC0_OFFSET                      0x0564
#define SUBPIC_LC1_OFFSET                      0x0568
#define SUBPIC_PITCH                           0x056C
#define SUBPIC_BTN_HLI_COLCON                  0x0570
#define SUBPIC_BTN_HLI_Y_X_START               0x0574
#define SUBPIC_BTN_HLI_Y_X_END                 0x0578
#define SUBPIC_PALETTE_INDEX                   0x057C
#define SUBPIC_PALETTE_DATA                    0x0580
#define SUBPIC_H_ACCUM_INIT                    0x0584
#define SUBPIC_V_ACCUM_INIT                    0x0588
#define DISP_MISC_CNTL                         0x0D00
#define DAC_MACRO_CNTL                         0x0D04
#define DISP_PWR_MAN                           0x0D08
#define DISP_TEST_DEBUG_CNTL                   0x0D10
#define DISP_HW_DEBUG                          0x0D14
#define DAC_CRC_SIG1                           0x0D18 
#define DAC_CRC_SIG2                           0x0D1C
#define OV0_LIN_TRANS_A                        0x0D20
#define OV0_LIN_TRANS_B                        0x0D24
#define OV0_LIN_TRANS_C                        0x0D28
#define OV0_LIN_TRANS_D                        0x0D2C
#define OV0_LIN_TRANS_E                        0x0D30
#define OV0_LIN_TRANS_F                        0x0D34
#define OV0_GAMMA_0_F                          0x0D40
#define OV0_GAMMA_10_1F                        0x0D44
#define OV0_GAMMA_20_3F                        0x0D48
#define OV0_GAMMA_40_7F                        0x0D4C
#define OV0_GAMMA_380_3BF                      0x0D50
#define OV0_GAMMA_3C0_3FF                      0x0D54
#define DISP_MERGE_CNTL                        0x0D60
#define DISP_OUTPUT_CNTL                       0x0D64
#define DISP_LIN_TRANS_GRPH_A                  0x0D80
#define DISP_LIN_TRANS_GRPH_B                  0x0D84
#define DISP_LIN_TRANS_GRPH_C                  0x0D88
#define DISP_LIN_TRANS_GRPH_D                  0x0D8C
#define DISP_LIN_TRANS_GRPH_E                  0x0D90
#define DISP_LIN_TRANS_GRPH_F                  0x0D94
#define DISP_LIN_TRANS_VID_A                   0x0D98
#define DISP_LIN_TRANS_VID_B                   0x0D9C
#define DISP_LIN_TRANS_VID_C                   0x0DA0
#define DISP_LIN_TRANS_VID_D                   0x0DA4
#define DISP_LIN_TRANS_VID_E                   0x0DA8
#define DISP_LIN_TRANS_VID_F                   0x0DAC
#define RMX_HORZ_FILTER_0TAP_COEF              0x0DB0
#define RMX_HORZ_FILTER_1TAP_COEF              0x0DB4
#define RMX_HORZ_FILTER_2TAP_COEF              0x0DB8
#define RMX_HORZ_PHASE                         0x0DBC
#define DAC_EMBEDDED_SYNC_CNTL                 0x0DC0
#define DAC_BROAD_PULSE                        0x0DC4
#define DAC_SKEW_CLKS                          0x0DC8
#define DAC_INCR                               0x0DCC
#define DAC_NEG_SYNC_LEVEL                     0x0DD0
#define DAC_POS_SYNC_LEVEL                     0x0DD4
#define DAC_BLANK_LEVEL                        0x0DD8
#define CLOCK_CNTL_INDEX                       0x0008
#define CLOCK_CNTL_DATA                        0x000C
#define CP_RB_CNTL                             0x0704
#define CP_RB_BASE                             0x0700 
#define CP_RB_RPTR_ADDR                        0x070C 
#define CP_RB_RPTR                             0x0710 
#define CP_RB_WPTR                             0x0714 
#define CP_RB_WPTR_DELAY                       0x0718
#define CP_IB_BASE                             0x0738
#define CP_IB_BUFSZ                            0x073C
#define CP_CSQ_CNTL                            0x0740
#define CP_CSQ_APER_PRIMARY                    0x1000
#define CP_CSQ_APER_PRIMARY_alt_1              0x1004
#define CP_CSQ_APER_PRIMARY_alt_2              0x1008
#define CP_CSQ_APER_PRIMARY_alt_3              0x100C
#define CP_CSQ_APER_PRIMARY_alt_4              0x1010
#define CP_CSQ_APER_PRIMARY_alt_5              0x1014
#define CP_CSQ_APER_PRIMARY_alt_6              0x1018
#define CP_CSQ_APER_PRIMARY_alt_7              0x101C
#define CP_CSQ_APER_PRIMARY_alt_8              0x1020
#define CP_CSQ_APER_PRIMARY_alt_9              0x1024
#define CP_CSQ_APER_PRIMARY_alt_10             0x1028
#define CP_CSQ_APER_PRIMARY_alt_11             0x102C
#define CP_CSQ_APER_PRIMARY_alt_12             0x1030
#define CP_CSQ_APER_PRIMARY_alt_13             0x1034
#define CP_CSQ_APER_PRIMARY_alt_14             0x1038
#define CP_CSQ_APER_PRIMARY_alt_15             0x103C
#define CP_CSQ_APER_PRIMARY_alt_16             0x1040
#define CP_CSQ_APER_PRIMARY_alt_17             0x1044
#define CP_CSQ_APER_PRIMARY_alt_18             0x1048
#define CP_CSQ_APER_PRIMARY_alt_19             0x104C
#define CP_CSQ_APER_PRIMARY_alt_20             0x1050
#define CP_CSQ_APER_PRIMARY_alt_21             0x1054
#define CP_CSQ_APER_PRIMARY_alt_22             0x1058
#define CP_CSQ_APER_PRIMARY_alt_23             0x105C
#define CP_CSQ_APER_PRIMARY_alt_24             0x1060
#define CP_CSQ_APER_PRIMARY_alt_25             0x1064
#define CP_CSQ_APER_PRIMARY_alt_26             0x1068
#define CP_CSQ_APER_PRIMARY_alt_27             0x106C
#define CP_CSQ_APER_PRIMARY_alt_28             0x1070
#define CP_CSQ_APER_PRIMARY_alt_29             0x1074
#define CP_CSQ_APER_PRIMARY_alt_30             0x1078
#define CP_CSQ_APER_PRIMARY_alt_31             0x107C
#define CP_CSQ_APER_PRIMARY_alt_32             0x1080
#define CP_CSQ_APER_PRIMARY_alt_33             0x1084
#define CP_CSQ_APER_PRIMARY_alt_34             0x1088
#define CP_CSQ_APER_PRIMARY_alt_35             0x108C
#define CP_CSQ_APER_PRIMARY_alt_36             0x1090
#define CP_CSQ_APER_PRIMARY_alt_37             0x1094
#define CP_CSQ_APER_PRIMARY_alt_38             0x1098
#define CP_CSQ_APER_PRIMARY_alt_39             0x109C
#define CP_CSQ_APER_PRIMARY_alt_40             0x10A0
#define CP_CSQ_APER_PRIMARY_alt_41             0x10A4
#define CP_CSQ_APER_PRIMARY_alt_42             0x10A8
#define CP_CSQ_APER_PRIMARY_alt_43             0x10AC
#define CP_CSQ_APER_PRIMARY_alt_44             0x10B0
#define CP_CSQ_APER_PRIMARY_alt_45             0x10B4
#define CP_CSQ_APER_PRIMARY_alt_46             0x10B8
#define CP_CSQ_APER_PRIMARY_alt_47             0x10BC
#define CP_CSQ_APER_PRIMARY_alt_48             0x10C0
#define CP_CSQ_APER_PRIMARY_alt_49             0x10C4
#define CP_CSQ_APER_PRIMARY_alt_50             0x10C8
#define CP_CSQ_APER_PRIMARY_alt_51             0x10CC
#define CP_CSQ_APER_PRIMARY_alt_52             0x10D0
#define CP_CSQ_APER_PRIMARY_alt_53             0x10D4
#define CP_CSQ_APER_PRIMARY_alt_54             0x10D8
#define CP_CSQ_APER_PRIMARY_alt_55             0x10DC
#define CP_CSQ_APER_PRIMARY_alt_56             0x10E0
#define CP_CSQ_APER_PRIMARY_alt_57             0x10E4
#define CP_CSQ_APER_PRIMARY_alt_58             0x10E8
#define CP_CSQ_APER_PRIMARY_alt_59             0x10EC
#define CP_CSQ_APER_PRIMARY_alt_60             0x10F0
#define CP_CSQ_APER_PRIMARY_alt_61             0x10F4
#define CP_CSQ_APER_PRIMARY_alt_62             0x10F8
#define CP_CSQ_APER_PRIMARY_alt_63             0x10FC
#define CP_CSQ_APER_PRIMARY_alt_64             0x1100
#define CP_CSQ_APER_PRIMARY_alt_65             0x1104
#define CP_CSQ_APER_PRIMARY_alt_66             0x1108
#define CP_CSQ_APER_PRIMARY_alt_67             0x110C
#define CP_CSQ_APER_PRIMARY_alt_68             0x1110
#define CP_CSQ_APER_PRIMARY_alt_69             0x1114
#define CP_CSQ_APER_PRIMARY_alt_70             0x1118
#define CP_CSQ_APER_PRIMARY_alt_71             0x111C
#define CP_CSQ_APER_PRIMARY_alt_72             0x1120
#define CP_CSQ_APER_PRIMARY_alt_73             0x1124
#define CP_CSQ_APER_PRIMARY_alt_74             0x1128
#define CP_CSQ_APER_PRIMARY_alt_75             0x112C
#define CP_CSQ_APER_PRIMARY_alt_76             0x1130
#define CP_CSQ_APER_PRIMARY_alt_77             0x1134
#define CP_CSQ_APER_PRIMARY_alt_78             0x1138
#define CP_CSQ_APER_PRIMARY_alt_79             0x113C
#define CP_CSQ_APER_PRIMARY_alt_80             0x1140
#define CP_CSQ_APER_PRIMARY_alt_81             0x1144
#define CP_CSQ_APER_PRIMARY_alt_82             0x1148
#define CP_CSQ_APER_PRIMARY_alt_83             0x114C
#define CP_CSQ_APER_PRIMARY_alt_84             0x1150
#define CP_CSQ_APER_PRIMARY_alt_85             0x1154
#define CP_CSQ_APER_PRIMARY_alt_86             0x1158
#define CP_CSQ_APER_PRIMARY_alt_87             0x115C
#define CP_CSQ_APER_PRIMARY_alt_88             0x1160
#define CP_CSQ_APER_PRIMARY_alt_89             0x1164
#define CP_CSQ_APER_PRIMARY_alt_90             0x1168
#define CP_CSQ_APER_PRIMARY_alt_91             0x116C
#define CP_CSQ_APER_PRIMARY_alt_92             0x1170
#define CP_CSQ_APER_PRIMARY_alt_93             0x1174
#define CP_CSQ_APER_PRIMARY_alt_94             0x1178
#define CP_CSQ_APER_PRIMARY_alt_95             0x117C
#define CP_CSQ_APER_PRIMARY_alt_96             0x1180
#define CP_CSQ_APER_PRIMARY_alt_97             0x1184
#define CP_CSQ_APER_PRIMARY_alt_98             0x1188
#define CP_CSQ_APER_PRIMARY_alt_99             0x118C
#define CP_CSQ_APER_PRIMARY_alt_100            0x1190
#define CP_CSQ_APER_PRIMARY_alt_101            0x1194
#define CP_CSQ_APER_PRIMARY_alt_102            0x1198
#define CP_CSQ_APER_PRIMARY_alt_103            0x119C
#define CP_CSQ_APER_PRIMARY_alt_104            0x11A0
#define CP_CSQ_APER_PRIMARY_alt_105            0x11A4
#define CP_CSQ_APER_PRIMARY_alt_106            0x11A8
#define CP_CSQ_APER_PRIMARY_alt_107            0x11AC
#define CP_CSQ_APER_PRIMARY_alt_108            0x11B0
#define CP_CSQ_APER_PRIMARY_alt_109            0x11B4
#define CP_CSQ_APER_PRIMARY_alt_110            0x11B8
#define CP_CSQ_APER_PRIMARY_alt_111            0x11BC
#define CP_CSQ_APER_PRIMARY_alt_112            0x11C0
#define CP_CSQ_APER_PRIMARY_alt_113            0x11C4
#define CP_CSQ_APER_PRIMARY_alt_114            0x11C8
#define CP_CSQ_APER_PRIMARY_alt_115            0x11CC
#define CP_CSQ_APER_PRIMARY_alt_116            0x11D0
#define CP_CSQ_APER_PRIMARY_alt_117            0x11D4
#define CP_CSQ_APER_PRIMARY_alt_118            0x11D8
#define CP_CSQ_APER_PRIMARY_alt_119            0x11DC
#define CP_CSQ_APER_PRIMARY_alt_120            0x11E0
#define CP_CSQ_APER_PRIMARY_alt_121            0x11E4
#define CP_CSQ_APER_PRIMARY_alt_122            0x11E8
#define CP_CSQ_APER_PRIMARY_alt_123            0x11EC
#define CP_CSQ_APER_PRIMARY_alt_124            0x11F0
#define CP_CSQ_APER_PRIMARY_alt_125            0x11F4
#define CP_CSQ_APER_PRIMARY_alt_126            0x11F8
#define CP_CSQ_APER_PRIMARY_alt_127            0x11FC
#define CP_CSQ_APER_INDIRECT                   0x1300
#define CP_CSQ_APER_INDIRECT_alt_1             0x1304
#define CP_CSQ_APER_INDIRECT_alt_2             0x1308
#define CP_CSQ_APER_INDIRECT_alt_3             0x130C
#define CP_CSQ_APER_INDIRECT_alt_4             0x1310
#define CP_CSQ_APER_INDIRECT_alt_5             0x1314
#define CP_CSQ_APER_INDIRECT_alt_6             0x1318
#define CP_CSQ_APER_INDIRECT_alt_7             0x131C
#define CP_CSQ_APER_INDIRECT_alt_8             0x1320
#define CP_CSQ_APER_INDIRECT_alt_9             0x1324
#define CP_CSQ_APER_INDIRECT_alt_10            0x1328
#define CP_CSQ_APER_INDIRECT_alt_11            0x132C
#define CP_CSQ_APER_INDIRECT_alt_12            0x1330
#define CP_CSQ_APER_INDIRECT_alt_13            0x1334
#define CP_CSQ_APER_INDIRECT_alt_14            0x1338
#define CP_CSQ_APER_INDIRECT_alt_15            0x133C
#define CP_CSQ_APER_INDIRECT_alt_16            0x1340
#define CP_CSQ_APER_INDIRECT_alt_17            0x1344
#define CP_CSQ_APER_INDIRECT_alt_18            0x1348
#define CP_CSQ_APER_INDIRECT_alt_19            0x134C
#define CP_CSQ_APER_INDIRECT_alt_20            0x1350
#define CP_CSQ_APER_INDIRECT_alt_21            0x1354
#define CP_CSQ_APER_INDIRECT_alt_22            0x1358
#define CP_CSQ_APER_INDIRECT_alt_23            0x135C
#define CP_CSQ_APER_INDIRECT_alt_24            0x1360
#define CP_CSQ_APER_INDIRECT_alt_25            0x1364
#define CP_CSQ_APER_INDIRECT_alt_26            0x1368
#define CP_CSQ_APER_INDIRECT_alt_27            0x136C
#define CP_CSQ_APER_INDIRECT_alt_28            0x1370
#define CP_CSQ_APER_INDIRECT_alt_29            0x1374
#define CP_CSQ_APER_INDIRECT_alt_30            0x1378
#define CP_CSQ_APER_INDIRECT_alt_31            0x137C
#define CP_CSQ_APER_INDIRECT_alt_32            0x1380
#define CP_CSQ_APER_INDIRECT_alt_33            0x1384
#define CP_CSQ_APER_INDIRECT_alt_34            0x1388
#define CP_CSQ_APER_INDIRECT_alt_35            0x138C
#define CP_CSQ_APER_INDIRECT_alt_36            0x1390
#define CP_CSQ_APER_INDIRECT_alt_37            0x1394
#define CP_CSQ_APER_INDIRECT_alt_38            0x1398
#define CP_CSQ_APER_INDIRECT_alt_39            0x139C
#define CP_CSQ_APER_INDIRECT_alt_40            0x13A0
#define CP_CSQ_APER_INDIRECT_alt_41            0x13A4
#define CP_CSQ_APER_INDIRECT_alt_42            0x13A8
#define CP_CSQ_APER_INDIRECT_alt_43            0x13AC
#define CP_CSQ_APER_INDIRECT_alt_44            0x13B0
#define CP_CSQ_APER_INDIRECT_alt_45            0x13B4
#define CP_CSQ_APER_INDIRECT_alt_46            0x13B8
#define CP_CSQ_APER_INDIRECT_alt_47            0x13BC
#define CP_CSQ_APER_INDIRECT_alt_48            0x13C0
#define CP_CSQ_APER_INDIRECT_alt_49            0x13C4
#define CP_CSQ_APER_INDIRECT_alt_50            0x13C8
#define CP_CSQ_APER_INDIRECT_alt_51            0x13CC
#define CP_CSQ_APER_INDIRECT_alt_52            0x13D0
#define CP_CSQ_APER_INDIRECT_alt_53            0x13D4
#define CP_CSQ_APER_INDIRECT_alt_54            0x13D8
#define CP_CSQ_APER_INDIRECT_alt_55            0x13DC
#define CP_CSQ_APER_INDIRECT_alt_56            0x13E0
#define CP_CSQ_APER_INDIRECT_alt_57            0x13E4
#define CP_CSQ_APER_INDIRECT_alt_58            0x13E8
#define CP_CSQ_APER_INDIRECT_alt_59            0x13EC
#define CP_CSQ_APER_INDIRECT_alt_60            0x13F0
#define CP_CSQ_APER_INDIRECT_alt_61            0x13F4
#define CP_CSQ_APER_INDIRECT_alt_62            0x13F8
#define CP_CSQ_APER_INDIRECT_alt_63            0x13FC
#define CP_ME_CNTL                             0x07D0 
#define CP_ME_RAM_ADDR                         0x07D4 
#define CP_ME_RAM_RADDR                        0x07D8 
#define CP_ME_RAM_DATAH                        0x07DC 
#define CP_ME_RAM_DATAL                        0x07E0 
#define CP_DEBUG                               0x07EC 
#define SCRATCH_REG0                           0x15E0
#define GUI_SCRATCH_REG0                       0x15E0
#define SCRATCH_REG1                           0x15E4
#define GUI_SCRATCH_REG1                       0x15E4
#define SCRATCH_REG2                           0x15E8
#define GUI_SCRATCH_REG2                       0x15E8
#define SCRATCH_REG3                           0x15EC
#define GUI_SCRATCH_REG3                       0x15EC
#define SCRATCH_REG4                           0x15F0
#define GUI_SCRATCH_REG4                       0x15F0
#define SCRATCH_REG5                           0x15F4
#define GUI_SCRATCH_REG5                       0x15F4
#define SCRATCH_UMSK                           0x0770
#define SCRATCH_ADDR                           0x0774 
#define DMA_GUI_TABLE_ADDR                     0x0780 
#define DMA_GUI_SRC_ADDR                       0x0784 
#define DMA_GUI_DST_ADDR                       0x0788 
#define DMA_GUI_COMMAND                        0x078C 
#define DMA_GUI_STATUS                         0x0790 
#define DMA_GUI_ACT_DSCRPTR                    0x0794 
#define DMA_VID_TABLE_ADDR                     0x07A0 
#define DMA_VID_SRC_ADDR                       0x07A4 
#define DMA_VID_DST_ADDR                       0x07A8
#define DMA_VID_COMMAND                        0x07AC
#define DMA_VID_STATUS                         0x07B0
#define DMA_VID_ACT_DSCRPTR                    0x07B4
#define CP_CSQ_ADDR                            0x07F0 
#define CP_CSQ_DATA                            0x07F4 
#define CP_CSQ_STAT                            0x07F8 
#define CP_STAT                                0x07C0
#define SE_PORT_DATA0                          0x2000
#define SE_PORT_DATA1                          0x2004
#define SE_PORT_DATA2                          0x2008
#define SE_PORT_DATA3                          0x200C
#define SE_PORT_DATA4                          0x2010
#define SE_PORT_DATA5                          0x2014
#define SE_PORT_DATA6                          0x2018
#define SE_PORT_DATA7                          0x201C
#define SE_PORT_DATA8                          0x2020
#define SE_PORT_DATA9                          0x2024
#define SE_PORT_DATA10                         0x2028
#define SE_PORT_DATA11                         0x202C
#define SE_PORT_DATA12                         0x2030
#define SE_PORT_DATA13                         0x2034
#define SE_PORT_DATA14                         0x2038
#define SE_PORT_DATA15                         0x203C
#define SE_PORT_IDX0                           0x2040
#define SE_PORT_IDX1                           0x2044
#define SE_PORT_IDX2                           0x2048
#define SE_PORT_IDX3                           0x204C
#define SE_PORT_IDX4                           0x2050
#define SE_PORT_IDX5                           0x2054
#define SE_PORT_IDX6                           0x2058
#define SE_PORT_IDX7                           0x205C
#define SE_PORT_IDX8                           0x2060
#define SE_PORT_IDX9                           0x2064
#define SE_PORT_IDX10                          0x2068
#define SE_PORT_IDX11                          0x206C
#define SE_PORT_IDX12                          0x2070
#define SE_PORT_IDX13                          0x2074
#define SE_PORT_IDX14                          0x2078
#define SE_PORT_IDX15                          0x207C
#define SE_VTX_FMT                             0x2080
#define SE_VF_CNTL                             0x2084
#define SE_CNTL                                0x1C4C
#define SE_CNTL_alt_1                          0x2088
#define SE_COORD_FMT                           0x1C50
#define SE_COORD_FMT_alt_1                     0x208C
#define SE_VPORT_XSCALE                        0x1D98
#define SE_VPORT_XSCALE_alt_1                  0x2098
#define SE_VPORT_XOFFSET                       0x1D9C
#define SE_VPORT_XOFFSET_alt_1                 0x209C
#define SE_VPORT_YSCALE                        0x1DA0
#define SE_VPORT_YSCALE_alt_1                  0x20A0
#define SE_VPORT_YOFFSET                       0x1DA4
#define SE_VPORT_YOFFSET_alt_1                 0x20A4
#define SE_VPORT_ZSCALE                        0x1DA8
#define SE_VPORT_ZSCALE_alt_1                  0x20A8
#define SE_VPORT_ZOFFSET                       0x1DAC
#define SE_VPORT_ZOFFSET_alt_1                 0x20AC
#define SE_ZBIAS_FACTOR                        0x1DB0
#define SE_ZBIAS_FACTOR_alt_1                  0x20B0
#define SE_ZBIAS_CONSTANT                      0x1DB4
#define SE_ZBIAS_CONSTANT_alt_1                0x20B4
#define SE_LINE_WIDTH                          0x1DB8
#define SE_LINE_WIDTH_alt_1                    0x20B8
#define SE_W0_RANGE                            0x1DBC
#define SE_W0_RANGE_alt_1                      0x20BC
#define SE_VTX_NUM_ARRAYS                      0x20C0
#define SE_VTX_AOS_ATTR01                      0x20C4
#define SE_VTX_AOS_ADDR0                       0x20C8
#define SE_VTX_AOS_ADDR1                       0x20CC
#define SE_VTX_AOS_ATTR23                      0x20D0
#define SE_VTX_AOS_ADDR2                       0x20D4
#define SE_VTX_AOS_ADDR3                       0x20D8
#define SE_VTX_AOS_ATTR45                      0x20DC
#define SE_VTX_AOS_ADDR4                       0x20E0
#define SE_VTX_AOS_ADDR5                       0x20E4
#define SE_VTX_AOS_ATTR67                      0x20E8
#define SE_VTX_AOS_ADDR6                       0x20EC
#define SE_VTX_AOS_ADDR7                       0x20F0
#define SE_VTX_AOS_ATTR89                      0x20F4
#define SE_VTX_AOS_ADDR8                       0x20F8
#define SE_VTX_AOS_ADDR9                       0x20FC
#define SE_VTX_AOS_ATTR1011                    0x2100
#define SE_VTX_AOS_ADDR10                      0x2104
#define SE_VTX_AOS_ADDR11                      0x2108
#define SE_PERF_CNTL                           0x2124
#define SE_PERF_COUNT1                         0x2128
#define SE_PERF_COUNT2                         0x212C
#define SE_DEBUG                               0x213C
#define SE_CNTL_STATUS                         0x2140
#define SE_SERE_WCNTL                          0x214C
#define SE_SERE_WD0_0                          0x2150
#define SE_SERE_WD0_1                          0x2154
#define SE_SERE_WD0_2                          0x2158
#define SE_SERE_WD0_3                          0x215C
#define SE_SERE_WD1_0                          0x2160
#define SE_SERE_WD1_1                          0x2164
#define SE_SERE_WD1_2                          0x2168
#define SE_SERE_WD1_3                          0x216C
#define SE_MC_SRC2_CNTL                        0x19D4
#define SE_MC_SRC2_CNTL_alt_1                  0x21D4
#define SE_MC_SRC1_CNTL                        0x19D8 
#define SE_MC_SRC1_CNTL_alt_1                  0x21D8
#define SE_MC_DST_CNTL                         0x19DC
#define SE_MC_DST_CNTL_alt_1                   0x21DC
#define SE_MC_CNTL_START                       0x19E0
#define SE_MC_CNTL_START_alt_1                 0x21E0
#define SE_MC_BUF_BASE                         0x19E4
#define SE_MC_BUF_BASE_alt_1                   0x21E4
#define SE_TCL_VECTOR_INDX_REG                 0x2200
#define SE_TCL_VECTOR_DATA_REG                 0x2204
#define SE_TCL_SCALAR_INDX_REG                 0x2208
#define SE_TCL_SCALAR_DATA_REG                 0x220C
#define SE_TCL_MATERIAL_EMISSIVE_RED           0x2210
#define SE_TCL_MATERIAL_EMISSIVE_GREEN         0x2214
#define SE_TCL_MATERIAL_EMISSIVE_BLUE          0x2218
#define SE_TCL_MATERIAL_EMISSIVE_ALPHA         0x221C
#define SE_TCL_MATERIAL_AMBIENT_RED            0x2220
#define SE_TCL_MATERIAL_AMBIENT_GREEN          0x2224
#define SE_TCL_MATERIAL_AMBIENT_BLUE           0x2228
#define SE_TCL_MATERIAL_AMBIENT_ALPHA          0x222C
#define SE_TCL_MATERIAL_DIFFUSE_RED            0x2230
#define SE_TCL_MATERIAL_DIFFUSE_GREEN          0x2234
#define SE_TCL_MATERIAL_DIFFUSE_BLUE           0x2238
#define SE_TCL_MATERIAL_DIFFUSE_ALPHA          0x223C
#define SE_TCL_MATERIAL_SPECULAR_RED           0x2240
#define SE_TCL_MATERIAL_SPECULAR_GREEN         0x2244
#define SE_TCL_MATERIAL_SPECULAR_BLUE          0x2248
#define SE_TCL_MATERIAL_SPECULAR_ALPHA         0x224C
#define SE_TCL_SHININESS                       0x2250
#define SE_TCL_OUTPUT_VTX_FMT                  0x2254
#define SE_TCL_OUTPUT_VTX_SEL                  0x2258
#define SE_TCL_MATRIX_SELECT_0                 0x225C
#define SE_TCL_MATRIX_SELECT_1                 0x2260
#define SE_TCL_UCP_VERT_BLEND_CTL              0x2264
#define SE_TCL_TEXTURE_PROC_CTL                0x2268
#define SE_TCL_LIGHT_MODEL_CTL                 0x226C
#define SE_TCL_PER_LIGHT_CTL_0                 0x2270
#define SE_TCL_PER_LIGHT_CTL_1                 0x2274
#define SE_TCL_PER_LIGHT_CTL_2                 0x2278
#define SE_TCL_PER_LIGHT_CTL_3                 0x227C
#define SE_TCL_DEBUG                           0x2280
#define SE_TCL_PERF_CNTL                       0x2290
#define SE_TCL_PERF_COUNT1                     0x2294
#define SE_TCL_PERF_COUNT2                     0x2298
#define SE_TCL_FPU_LATENCY                     0x22A0
#define RE_STIPPLE_ADDR                        0x1CC8
#define RE_STIPPLE_ADDR_alt_1                  0x26C8
#define RE_STIPPLE_DATA                        0x1CCC
#define RE_STIPPLE_DATA_alt_1                  0x26CC
#define RE_MISC                                0x1CC4
#define RE_MISC_alt_1                          0x26C4
#define RE_SOLID_COLOR                         0x1C1C
#define RE_SOLID_COLOR_alt_1                   0x261C
#define RE_WIDTH_HEIGHT                        0x1C44
#define RE_WIDTH_HEIGHT_alt_1                  0x2644
#define RE_TOP_LEFT                            0x26C0
#define RE_SCISSOR_TL_0                        0x1CD8
#define RE_SCISSOR_TL_0_alt_1                  0x26D8
#define RE_SCISSOR_BR_0                        0x1CDC
#define RE_SCISSOR_BR_0_alt_1                  0x26DC
#define RE_SCISSOR_TL_1                        0x1CE0
#define RE_SCISSOR_TL_1_alt_1                  0x26E0
#define RE_SCISSOR_BR_1                        0x1CE4
#define RE_SCISSOR_BR_1_alt_1                  0x26E4
#define RE_SCISSOR_TL_2                        0x1CE8
#define RE_SCISSOR_TL_2_alt_1                  0x26E8
#define RE_SCISSOR_BR_2                        0x1CEC
#define RE_SCISSOR_BR_2_alt_1                  0x26EC
#define RE_AUX_SCISSOR_CNTL                    0x26F0
#define RE_LINE_PATTERN                        0x1CD0
#define RE_LINE_PATTERN_alt_1                  0x26D0
#define RE_LINE_STATE                          0x1CD4
#define RE_LINE_STATE_alt_1                    0x26D4
#define RE_PIX_COUNT                           0x2680
#define RE_PAIR_COUNT                          0x2684
#define RE_CYC_COUNT                           0x2688
#define RE_OUTSIDE                             0x268C
#define RE_STALLED                             0x2690
#define RE_PERF                                0x2694
#define RE_DEBUG_0                             0x2698
#define RE_DEBUG_1                             0x269C
#define RE_DEBUG_2                             0x26A0
#define RE_DEBUG_3                             0x26A4
#define RE_DEBUG_4                             0x26A8
#define RE_DEBUG_5                             0x26AC
#define RE_DEBUG_6                             0x26B0
#define RE_DEBUG_7                             0x26B4
#define RE_E2_0                                0x2700
#define RE_E2_1                                0x2704
#define RE_E2_2                                0x2708
#define RE_E2_3                                0x270C
#define RE_E0E1_0                              0x2710
#define RE_E0E1_1                              0x2714
#define RE_E0E1_2                              0x2718
#define RE_E0E1_3                              0x271C
#define RE_NULL_PRIM                           0x29E0
#define RE_Z_MINMAX_0                          0x29F0
#define RE_Z_MINMAX_1                          0x29F4
#define RE_Z_MINMAX_3                          0x29FC
#define RE_DDA_Z_0                             0x2720
#define RE_DDA_Z_1                             0x2724
#define RE_DDA_Z_2                             0x2728
#define RE_DDA_Z_3                             0x272C
#define RE_DDA_RHW_0                           0x2730
#define RE_DDA_RHW_1                           0x2734
#define RE_DDA_RHW_2                           0x2738
#define RE_DDA_RHW_3                           0x273C
#define RE_DDA_A_0                             0x2740
#define RE_DDA_A_1                             0x2744
#define RE_DDA_A_2                             0x2748
#define RE_DDA_R_0                             0x2750
#define RE_DDA_R_1                             0x2754
#define RE_DDA_R_2                             0x2758
#define RE_DDA_G_0                             0x2760
#define RE_DDA_G_1                             0x2764
#define RE_DDA_G_2                             0x2768
#define RE_DDA_B_0                             0x2770
#define RE_DDA_B_1                             0x2774
#define RE_DDA_B_2                             0x2778
#define RE_DDA_SA_0                            0x2780
#define RE_DDA_SA_1                            0x2784
#define RE_DDA_SA_2                            0x2788
#define RE_DDA_SR_0                            0x2790
#define RE_DDA_SR_1                            0x2794
#define RE_DDA_SR_2                            0x2798
#define RE_DDA_SG_0                            0x27A0
#define RE_DDA_SG_1                            0x27A4
#define RE_DDA_SG_2                            0x27A8
#define RE_DDA_SB_0                            0x27B0
#define RE_DDA_SB_1                            0x27B4
#define RE_DDA_SB_2                            0x27B8
#define RE_DDA_S0_0                            0x27C0
#define RE_DDA_S0_1                            0x27C4
#define RE_DDA_S0_2                            0x27C8
#define RE_DDA_S0_3                            0x27CC
#define RE_DDA_T0_0                            0x27D0
#define RE_DDA_T0_1                            0x27D4
#define RE_DDA_T0_2                            0x27D8
#define RE_DDA_T0_3                            0x27DC
#define RE_DDA_Q0_0                            0x27F0
#define RE_DDA_Q0_1                            0x27F4
#define RE_DDA_Q0_2                            0x27F8
#define RE_DDA_Q0_3                            0x27FC
#define RE_DDA_S1_0                            0x2800
#define RE_DDA_S1_1                            0x2804
#define RE_DDA_S1_2                            0x2808
#define RE_DDA_S1_3                            0x280C
#define RE_DDA_T1_0                            0x2810
#define RE_DDA_T1_1                            0x2814
#define RE_DDA_T1_2                            0x2818
#define RE_DDA_T1_3                            0x281C
#define RE_DDA_Q1_0                            0x2830
#define RE_DDA_Q1_1                            0x2834
#define RE_DDA_Q1_2                            0x2838
#define RE_DDA_Q1_3                            0x283C
#define RE_DDA_S2_0                            0x2840
#define RE_DDA_S2_1                            0x2844
#define RE_DDA_S2_2                            0x2848
#define RE_DDA_S2_3                            0x284C
#define RE_DDA_T2_0                            0x2850
#define RE_DDA_T2_1                            0x2854
#define RE_DDA_T2_2                            0x2858
#define RE_DDA_T2_3                            0x285C
#define RE_DDA_Q2_0                            0x2870
#define RE_DDA_Q2_1                            0x2874
#define RE_DDA_Q2_2                            0x2878
#define RE_DDA_Q2_3                            0x287C
#define RE_DDA_S3_0                            0x2880
#define RE_DDA_S3_1                            0x2884
#define RE_DDA_S3_2                            0x2888
#define RE_DDA_S3_3                            0x288C
#define RE_DDA_T3_0                            0x2890
#define RE_DDA_T3_1                            0x2894
#define RE_DDA_T3_2                            0x2898
#define RE_DDA_T3_3                            0x289C
#define RE_DDA_Q3_0                            0x28B0
#define RE_DDA_Q3_1                            0x28B4
#define RE_DDA_Q3_2                            0x28B8
#define RE_DDA_Q3_3                            0x28BC
#define RE_DDA_DS0_0                           0x28C0
#define RE_DDA_DS0_1                           0x28C4
#define RE_DDA_DS0_2                           0x28C8
#define RE_DDA_DS0_3                           0x28CC
#define RE_DDA_DT0_0                           0x28D0
#define RE_DDA_DT0_1                           0x28D4
#define RE_DDA_DT0_2                           0x28D8
#define RE_DDA_DT0_3                           0x28DC
#define RE_DDA_DS1_0                           0x28F0
#define RE_DDA_DS1_1                           0x28F4
#define RE_DDA_DS1_2                           0x28F8
#define RE_DDA_DS1_3                           0x28FC
#define RE_DDA_DT1_0                           0x2900
#define RE_DDA_DT1_1                           0x2904
#define RE_DDA_DT1_2                           0x2908
#define RE_DDA_DT1_3                           0x290C
#define RE_DDA_DS2_0                           0x2920
#define RE_DDA_DS2_1                           0x2924
#define RE_DDA_DS2_2                           0x2928
#define RE_DDA_DS2_3                           0x292C
#define RE_DDA_DT2_0                           0x2930
#define RE_DDA_DT2_1                           0x2934
#define RE_DDA_DT2_2                           0x2938
#define RE_DDA_DT2_3                           0x293C
#define RE_DDA_DS3_0                           0x2940
#define RE_DDA_DS3_1                           0x2944
#define RE_DDA_DS3_2                           0x2948
#define RE_DDA_DS3_3                           0x294C
#define RE_DDA_DT3_0                           0x2960
#define RE_DDA_DT3_1                           0x2964
#define RE_DDA_DT3_2                           0x2968
#define RE_DDA_DT3_3                           0x296C
#define PP_MC_CONTEXT                          0x19E8
#define PP_MC_CONTEXT_alt_1                    0x2DE8
#define PP_SRC_OFFSET_0                        0x1840
#define PP_SRC_OFFSET_0_alt_1                  0x2E40
#define PP_SRC_OFFSET_1                        0x1844
#define PP_SRC_OFFSET_1_alt_1                  0x2E44
#define PP_SRC_OFFSET_2                        0x1848
#define PP_SRC_OFFSET_2_alt_1                  0x2E48
#define PP_SRC_OFFSET_3                        0x184C
#define PP_SRC_OFFSET_3_alt_1                  0x2E4C
#define PP_SRC_OFFSET_4                        0x1850
#define PP_SRC_OFFSET_4_alt_1                  0x2E50
#define PP_SRC_OFFSET_5                        0x1854
#define PP_SRC_OFFSET_5_alt_1                  0x2E54
#define PP_SRC_OFFSET_6                        0x1858
#define PP_SRC_OFFSET_6_alt_1                  0x2E58
#define PP_SRC_OFFSET_7                        0x185C
#define PP_SRC_OFFSET_7_alt_1                  0x2E5C
#define PP_SRC_OFFSET_8                        0x1860
#define PP_SRC_OFFSET_8_alt_1                  0x2E60
#define PP_SRC_OFFSET_9                        0x1864
#define PP_SRC_OFFSET_9_alt_1                  0x2E64
#define PP_SRC_OFFSET_10                       0x1868
#define PP_SRC_OFFSET_10_alt_1                 0x2E68
#define PP_SRC_OFFSET_11                       0x1880
#define PP_SRC_OFFSET_11_alt_1                 0x2E6C
#define PP_SRC_OFFSET_12                       0x1884
#define PP_SRC_OFFSET_12_alt_1                 0x2E70
#define PP_SRC_OFFSET_13                       0x1888
#define PP_SRC_OFFSET_13_alt_1                 0x2E74
#define PP_SRC_OFFSET_14                       0x188C
#define PP_SRC_OFFSET_14_alt_1                 0x2E78
#define PP_SRC_OFFSET_15                       0x1890
#define PP_SRC_OFFSET_15_alt_1                 0x2E7C
#define PP_SRC_OFFSET_16                       0x1894
#define PP_SRC_OFFSET_16_alt_1                 0x2E80
#define PP_SRC_OFFSET_17                       0x1898
#define PP_SRC_OFFSET_17_alt_1                 0x2E84
#define PP_CNTL                                0x1C38
#define PP_CNTL_alt_1                          0x2C38
#define PP_TXFILTER_0                          0x1C54
#define PP_TXFILTER_0_alt_1                    0x2C54
#define PP_TXFILTER_1                          0x1C6C
#define PP_TXFILTER_1_alt_1                    0x2C6C
#define PP_TXFILTER_2                          0x1C84 
#define PP_TXFILTER_2_alt_1                    0x2C84
#define PP_TXFORMAT_0                          0x1C58
#define PP_TXFORMAT_0_alt_1                    0x2C58
#define PP_TXFORMAT_1                          0x1C70
#define PP_TXFORMAT_1_alt_1                    0x2C70
#define PP_TXFORMAT_2                          0x1C88 
#define PP_TXFORMAT_2_alt_1                    0x2C88 
#define PP_TXOFFSET_0                          0x1C5C
#define PP_TXOFFSET_0_alt_1                    0x2C5C
#define PP_TXOFFSET_1                          0x1C74
#define PP_TXOFFSET_1_alt_1                    0x2C74
#define PP_TXOFFSET_2                          0x1C8C 
#define PP_TXOFFSET_2_alt_1                    0x2C8C
#define PP_TEX_SIZE_0                          0x1D04
#define PP_TEX_SIZE_0_alt_1                    0x2D04
#define PP_TEX_SIZE_1                          0x1D0C
#define PP_TEX_SIZE_1_alt_1                    0x2D0C
#define PP_TEX_SIZE_2                          0x1D14
#define PP_TEX_SIZE_2_alt_1                    0x2D14
#define PP_TXPITCH_0                           0x1D08
#define PP_TXPITCH_0_alt_1                     0x2D08
#define PP_TXPITCH_1                           0x1D10
#define PP_TXPITCH_1_alt_1                     0x2D10
#define PP_TXPITCH_2                           0x1D18
#define PP_TXPITCH_2_alt_1                     0x2D18
#define PP_CUBIC_FACES_0                       0x1D24
#define PP_CUBIC_FACES_0_alt_1                 0x2D24
#define PP_CUBIC_FACES_1                       0x1D28
#define PP_CUBIC_FACES_1_alt_1                 0x2D28
#define PP_CUBIC_FACES_2                       0x1D2C
#define PP_CUBIC_FACES_2_alt_1                 0x2D2C
#define PP_CUBIC_OFFSET_T0_0                   0x1DD0
#define PP_CUBIC_OFFSET_T0_0_alt_1             0x2DD0
#define PP_CUBIC_OFFSET_T0_1                   0x1DD4
#define PP_CUBIC_OFFSET_T0_1_alt_1             0x2DD4
#define PP_CUBIC_OFFSET_T0_2                   0x1DD8
#define PP_CUBIC_OFFSET_T0_2_alt_1             0x2DD8
#define PP_CUBIC_OFFSET_T0_3                   0x1DDC
#define PP_CUBIC_OFFSET_T0_3_alt_1             0x2DDC
#define PP_CUBIC_OFFSET_T0_4                   0x1DE0
#define PP_CUBIC_OFFSET_T0_4_alt_1             0x2DE0
#define PP_CUBIC_OFFSET_T1_0                   0x1E00
#define PP_CUBIC_OFFSET_T1_0_alt_1             0x2E00
#define PP_CUBIC_OFFSET_T1_1                   0x1E04
#define PP_CUBIC_OFFSET_T1_1_alt_1             0x2E04
#define PP_CUBIC_OFFSET_T1_2                   0x1E08
#define PP_CUBIC_OFFSET_T1_2_alt_1             0x2E08
#define PP_CUBIC_OFFSET_T1_3                   0x1E0C
#define PP_CUBIC_OFFSET_T1_3_alt_1             0x2E0C
#define PP_CUBIC_OFFSET_T1_4                   0x1E10
#define PP_CUBIC_OFFSET_T1_4_alt_1             0x2E10
#define PP_CUBIC_OFFSET_T2_0                   0x1E14
#define PP_CUBIC_OFFSET_T2_0_alt_1             0x2E14
#define PP_CUBIC_OFFSET_T2_1                   0x1E18
#define PP_CUBIC_OFFSET_T2_1_alt_1             0x2E18
#define PP_CUBIC_OFFSET_T2_2                   0x1E1C
#define PP_CUBIC_OFFSET_T2_2_alt_1             0x2E1C
#define PP_CUBIC_OFFSET_T2_3                   0x1E20
#define PP_CUBIC_OFFSET_T2_3_alt_1             0x2E20
#define PP_CUBIC_OFFSET_T2_4                   0x1E24
#define PP_CUBIC_OFFSET_T2_4_alt_1             0x2E24
#define PP_CUBIC_OFFSET_T3_0                   0x2E28
#define PP_CUBIC_OFFSET_T3_1                   0x2E2C
#define PP_CUBIC_OFFSET_T3_2                   0x2E30
#define PP_CUBIC_OFFSET_T3_3                   0x2E34
#define PP_CUBIC_OFFSET_T3_4                   0x2E38
#define PP_SHADOW_ID                           0x1D34
#define PP_SHADOW_ID_alt_1                     0x2D34
#define PP_CHROMA_COLOR                        0x1D38
#define PP_CHROMA_COLOR_alt_1                  0x2D38
#define PP_CHROMA_MASK                         0x1D3C
#define PP_CHROMA_MASK_alt_1                   0x2D3C
#define PP_BORDER_COLOR_0                      0x1D40
#define PP_BORDER_COLOR_0_alt_1                0x2D40
#define PP_BORDER_COLOR_1                      0x1D44
#define PP_BORDER_COLOR_1_alt_1                0x2D44
#define PP_BORDER_COLOR_2                      0x1D48
#define PP_BORDER_COLOR_2_alt_1                0x2D48
#define PP_MISC                                0x1C14
#define PP_MISC_alt_1                          0x2C14
#define PP_TXCBLEND_0                          0x1C60
#define PP_TXCBLEND_0_alt_1                    0x2C60
#define PP_TXCBLEND_1                          0x1C78
#define PP_TXCBLEND_1_alt_1                    0x2C78
#define PP_TXCBLEND_2                          0x1C90 
#define PP_TXCBLEND_2_alt_1                    0x2C90
#define PP_TXABLEND_0                          0x1C64
#define PP_TXABLEND_0_alt_1                    0x2C64
#define PP_TXABLEND_1                          0x1C7C
#define PP_TXABLEND_1_alt_1                    0x2C7C
#define PP_TXABLEND_2                          0x1C94 
#define PP_TXABLEND_2_alt_1                    0x2C94
#define PP_TFACTOR_0                           0x1C68
#define PP_TFACTOR_0_alt_1                     0x2C68
#define PP_TFACTOR_1                           0x1C80 
#define PP_TFACTOR_1_alt_1                     0x2C80
#define PP_TFACTOR_2                           0x1C98 
#define PP_TFACTOR_2_alt_1                     0x2C98
#define PP_ROT_MATRIX_0                        0x1D58
#define PP_ROT_MATRIX_0_alt_1                  0x2D58
#define PP_ROT_MATRIX_1                        0x1D5C
#define PP_ROT_MATRIX_1_alt_1                  0x2D5C
#define PP_LUM_MATRIX                          0x1D00
#define PP_LUM_MATRIX_alt_1                    0x2D00
#define PP_FOG_COLOR                           0x1C18
#define PP_FOG_COLOR_alt_1                     0x2C18
#define PP_FOG_TABLE_INDEX                     0x1D50
#define PP_FOG_TABLE_INDEX_alt_1               0x2D50
#define PP_FOG_TABLE_DATA                      0x1D54
#define PP_FOG_TABLE_DATA_alt_1                0x2D54
#define PP_PERF                                0x2D60
#define PP_TRI_JUICE                           0x2D64
#define PP_PERF_COUNT_0                        0x2D70
#define PP_PERF_COUNT_1                        0x2D74
#define PP_PERF_COUNT_2                        0x2D78
#define PP_TAM_DEBUG_0                         0x2D80
#define PP_TAM_DEBUG_1                         0x2D84
#define PP_TAM_DEBUG_2                         0x2D88
#define PP_TAM_DEBUG_3                         0x2D8C
#define PP_TDM_DEBUG_0                         0x2DA0
#define PP_TDM_DEBUG_1                         0x2DA4
#define PP_TDM_DEBUG_2                         0x2DA8
#define PP_TDM_DEBUG_3                         0x2DAC
#define PP_PB_DEBUG_0                          0x2DC0
#define PP_PB_DEBUG_1                          0x2DC4
#define PP_PB_DEBUG_2                          0x2DC8
#define PP_PB_DEBUG_3                          0x2DCC
#define RB2D_ROP                               0x3400
#define RB2D_CLRCMP_SRC                        0x3408
#define RB2D_CLRCMP_DST                        0x340C
#define RB2D_CLRCMP_FLIPE                      0x3410
#define RB2D_CLRCMP_CNTL                       0x3414
#define RB2D_CLRCMP_MSK                        0x3418
#define RB2D_WRITEMASK                         0x341C
#define RB2D_DATATYPE                          0x3420
#define RB2D_GUI_MASTER_CNTL                   0x3424
#define RB2D_BRUSHDATA_0                       0x3500
#define RB2D_BRUSHDATA_1                       0x3504
#define RB2D_BRUSHDATA_2                       0x3508
#define RB2D_BRUSH_FRGD_CLR                    0x3508
#define RB2D_BRUSHDATA_3                       0x350C
#define RB2D_BRUSH_BKGD_CLR                    0x350C
#define RB2D_BRUSHDATA_4                       0x3510
#define RB2D_BRUSHDATA_5                       0x3514
#define RB2D_BRUSHDATA_6                       0x3518
#define RB2D_BRUSHDATA_7                       0x351C
#define RB2D_BRUSHDATA_8                       0x3520
#define RB2D_BRUSHDATA_9                       0x3524
#define RB2D_BRUSHDATA_10                      0x3528
#define RB2D_BRUSHDATA_11                      0x352C
#define RB2D_BRUSHDATA_12                      0x3530
#define RB2D_BRUSHDATA_13                      0x3534
#define RB2D_BRUSHDATA_14                      0x3538
#define RB2D_BRUSHDATA_15                      0x353C
#define RB2D_BRUSHDATA_16                      0x3540
#define RB2D_BRUSHDATA_17                      0x3544
#define RB2D_BRUSHDATA_18                      0x3548
#define RB2D_BRUSHDATA_19                      0x354C
#define RB2D_BRUSHDATA_20                      0x3550
#define RB2D_BRUSHDATA_21                      0x3554
#define RB2D_BRUSHDATA_22                      0x3558
#define RB2D_BRUSHDATA_23                      0x355C
#define RB2D_BRUSHDATA_24                      0x3560
#define RB2D_BRUSHDATA_25                      0x3564
#define RB2D_BRUSHDATA_26                      0x3568
#define RB2D_BRUSHDATA_27                      0x356C
#define RB2D_BRUSHDATA_28                      0x3570
#define RB2D_BRUSHDATA_29                      0x3574
#define RB2D_BRUSHDATA_30                      0x3578
#define RB2D_BRUSHDATA_31                      0x357C
#define RB2D_BRUSHDATA_32                      0x3580
#define RB2D_BRUSHDATA_33                      0x3584
#define RB2D_BRUSHDATA_34                      0x3588
#define RB2D_BRUSHDATA_35                      0x358C
#define RB2D_BRUSHDATA_36                      0x3590
#define RB2D_BRUSHDATA_37                      0x3594
#define RB2D_BRUSHDATA_38                      0x3598
#define RB2D_BRUSHDATA_39                      0x359C
#define RB2D_BRUSHDATA_40                      0x35A0
#define RB2D_BRUSHDATA_41                      0x35A4
#define RB2D_BRUSHDATA_42                      0x35A8
#define RB2D_BRUSHDATA_43                      0x35AC
#define RB2D_BRUSHDATA_44                      0x35B0
#define RB2D_BRUSHDATA_45                      0x35B4
#define RB2D_BRUSHDATA_46                      0x35B8
#define RB2D_BRUSHDATA_47                      0x35BC
#define RB2D_BRUSHDATA_48                      0x35C0
#define RB2D_BRUSHDATA_49                      0x35C4
#define RB2D_BRUSHDATA_50                      0x35C8
#define RB2D_BRUSHDATA_51                      0x35CC
#define RB2D_BRUSHDATA_52                      0x35D0
#define RB2D_BRUSHDATA_53                      0x35D4
#define RB2D_BRUSHDATA_54                      0x35D8
#define RB2D_BRUSHDATA_55                      0x35DC
#define RB2D_BRUSHDATA_56                      0x35E0
#define RB2D_BRUSHDATA_57                      0x35E4
#define RB2D_BRUSHDATA_58                      0x35E8
#define RB2D_BRUSHDATA_59                      0x35EC
#define RB2D_BRUSHDATA_60                      0x35F0
#define RB2D_BRUSHDATA_61                      0x35F4
#define RB2D_BRUSHDATA_62                      0x35F8
#define RB2D_BRUSHDATA_63                      0x35FC
#define RB2D_DSTCACHE_MODE                     0x3428
#define RB2D_DSTCACHE_CTLSTAT                  0x342C
#define RB2D_SRC_ENDIAN                        0x3430
#define RB2D_DST_ENDIAN                        0x3434
#define RB2D_PD1_DATA                          0x3438
#define RB2D_PD1_ADDR                          0x343C
#define RB2D_WRITEBACK_DATA_LO                 0x3450
#define RB2D_WRITEBACK_DATA_HI                 0x3454
#define RB2D_WRITEBACK_ADDR                    0x3458
#define RB3D_BLENDCNTL                         0x1C20
#define RB3D_BLENDCNTL_alt_1                   0x3220
#define RB3D_DEPTHOFFSET                       0x1C24
#define RB3D_DEPTHOFFSET_alt_1                 0x3224
#define RB3D_DEPTHPITCH                        0x1C28
#define RB3D_DEPTHPITCH_alt_1                  0x3228
#define RB3D_ZSTENCILCNTL                      0x1C2C
#define RB3D_ZSTENCILCNTL_alt_1                0x322C
#define RB3D_DEPTHCLEARVALUE                   0x3230
#define RB3D_DEPTHCLEARVALUE_alt_1             0x3230
#define RB3D_ZMASKOFFSET                       0x3234
#define RB3D_ZMASKOFFSET_alt_1                 0x3234
#define RB3D_CNTL                              0x1C3C
#define RB3D_CNTL_alt_1                        0x323C
#define RB3D_COLOROFFSET                       0x1C40
#define RB3D_COLOROFFSET_alt_1                 0x3240
#define RB3D_COLORPITCH                        0x1C48
#define RB3D_COLORPITCH_alt_1                  0x3248
#define RB3D_DEPTHXY_OFFSET                    0x1D60
#define RB3D_CLRCMP_FLIPE                      0x1D64
#define RB3D_CLRCMP_CLR                        0x1D68
#define RB3D_CLRCMP_MSK                        0x1D6C
#define RB3D_ZMASK_WRINDEX                     0x1D70
#define RB3D_ZMASK_DWORD                       0x1D74
#define RB3D_ZMASK_RDINDEX                     0x1D78
#define RB3D_STENCILREFMASK                    0x1D7C
#define RB3D_ROPCNTL                           0x1D80
#define RB3D_PLANEMASK                         0x1D84
#define RB3D_ZCACHE_MODE                       0x3250
#define RB3D_ZCACHE_CTLSTAT                    0x3254
#define RB3D_DSTCACHE_MODE                     0x3258
#define RB3D_DSTCACHE_CTLSTAT                  0x325C
#define RB3D_PD0_DATA                          0x3260
#define RB3D_PD1_DATA                          0x3268
#define RB3D_PD1_ADDR                          0x326C
#define RB3D_WRITEBACK_DATA_LO                 0x3280
#define RB3D_WRITEBACK_DATA_HI                 0x3284
#define RB3D_WRITEBACK_ADDR                    0x3288
#define RB3D_ZPASS_DATA                        0x3290
#define RB3D_ZPASS_ADDR                        0x3294
#define DST_OFFSET                             0x1404
#define DST_PITCH                              0x1408
#define DST_TILE                               0x1700
#define DST_PITCH_OFFSET                       0x142C
#define DST_X                                  0x141C
#define DST_Y                                  0x1420
#define DST_X_Y                                0x1594
#define DST_Y_X                                0x1438
#define DST_WIDTH                              0x140C
#define DST_HEIGHT                             0x1410
#define DST_WIDTH_HEIGHT                       0x1598
#define DST_HEIGHT_WIDTH                       0x143C
#define DST_HEIGHT_WIDTH_8                     0x158C
#define DST_HEIGHT_Y                           0x15A0
#define DST_WIDTH_X                            0x1588
#define DST_WIDTH_X_INCY                       0x159C
#define DST_LINE_START                         0x1600 
#define DST_LINE_END                           0x1604 
#define DST_LINE_PATCOUNT                      0x1608 
#define DP_DST_ENDIAN                          0x15D0
#define BRUSH_Y_X                              0x1474
#define BRUSH_DATA0                            0x1480
#define BRUSH_DATA1                            0x1484
#define BRUSH_DATA2                            0x1488
#define BRUSH_DATA3                            0x148C
#define BRUSH_DATA4                            0x1490
#define BRUSH_DATA5                            0x1494
#define BRUSH_DATA6                            0x1498
#define BRUSH_DATA7                            0x149C
#define BRUSH_DATA8                            0x14A0
#define BRUSH_DATA9                            0x14A4
#define BRUSH_DATA10                           0x14A8
#define BRUSH_DATA11                           0x14AC
#define BRUSH_DATA12                           0x14B0
#define BRUSH_DATA13                           0x14B4
#define BRUSH_DATA14                           0x14B8
#define BRUSH_DATA15                           0x14BC
#define BRUSH_DATA16                           0x14C0
#define BRUSH_DATA17                           0x14C4
#define BRUSH_DATA18                           0x14C8
#define BRUSH_DATA19                           0x14CC
#define BRUSH_DATA20                           0x14D0
#define BRUSH_DATA21                           0x14D4
#define BRUSH_DATA22                           0x14D8
#define BRUSH_DATA23                           0x14DC
#define BRUSH_DATA24                           0x14E0
#define BRUSH_DATA25                           0x14E4
#define BRUSH_DATA26                           0x14E8
#define BRUSH_DATA27                           0x14EC
#define BRUSH_DATA28                           0x14F0
#define BRUSH_DATA29                           0x14F4
#define BRUSH_DATA30                           0x14F8
#define BRUSH_DATA31                           0x14FC
#define BRUSH_DATA32                           0x1500
#define BRUSH_DATA33                           0x1504
#define BRUSH_DATA34                           0x1508
#define BRUSH_DATA35                           0x150C
#define BRUSH_DATA36                           0x1510
#define BRUSH_DATA37                           0x1514
#define BRUSH_DATA38                           0x1518
#define BRUSH_DATA39                           0x151C
#define BRUSH_DATA40                           0x1520
#define BRUSH_DATA41                           0x1524
#define BRUSH_DATA42                           0x1528
#define BRUSH_DATA43                           0x152C
#define BRUSH_DATA44                           0x1530
#define BRUSH_DATA45                           0x1534
#define BRUSH_DATA46                           0x1538
#define BRUSH_DATA47                           0x153C
#define BRUSH_DATA48                           0x1540
#define BRUSH_DATA49                           0x1544
#define BRUSH_DATA50                           0x1548
#define BRUSH_DATA51                           0x154C
#define BRUSH_DATA52                           0x1550
#define BRUSH_DATA53                           0x1554
#define BRUSH_DATA54                           0x1558
#define BRUSH_DATA55                           0x155C
#define BRUSH_DATA56                           0x1560
#define BRUSH_DATA57                           0x1564
#define BRUSH_DATA58                           0x1568
#define BRUSH_DATA59                           0x156C
#define BRUSH_DATA60                           0x1570
#define BRUSH_DATA61                           0x1574
#define BRUSH_DATA62                           0x1578
#define BRUSH_DATA63                           0x157C
#define DP_BRUSH_FRGD_CLR                      0x147C
#define DP_BRUSH_BKGD_CLR                      0x1478
#define SRC_OFFSET                             0x15AC
#define SRC_PITCH                              0x15B0
#define SRC_TILE                               0x1704
#define SRC_PITCH_OFFSET                       0x1428
#define SRC_X                                  0x1414
#define SRC_Y                                  0x1418
#define SRC_X_Y                                0x1590
#define SRC_Y_X                                0x1434
#define SRC_CLUT_ADDRESS                       0x1780 
#define SRC_CLUT_DATA                          0x1784 
#define SRC_CLUT_DATA_RD                       0x1788 
#define HOST_DATA0                             0x17C0
#define HOST_DATA1                             0x17C4
#define HOST_DATA2                             0x17C8
#define HOST_DATA3                             0x17CC
#define HOST_DATA4                             0x17D0
#define HOST_DATA5                             0x17D4
#define HOST_DATA6                             0x17D8
#define HOST_DATA7                             0x17DC
#define HOST_DATA_LAST                         0x17E0
#define DP_SRC_ENDIAN                          0x15D4
#define DP_SRC_FRGD_CLR                        0x15D8
#define DP_SRC_BKGD_CLR                        0x15DC
#define SC_LEFT                                0x1640
#define SC_RIGHT                               0x1644
#define SC_TOP                                 0x1648
#define SC_BOTTOM                              0x164C
#define SRC_SC_RIGHT                           0x1654
#define SRC_SC_BOTTOM                          0x165C
#define DP_CNTL                                0x16C0
#define DP_CNTL_XDIR_YDIR_YMAJOR               0x16D0
#define DP_DATATYPE                            0x16C4
#define DP_MIX                                 0x16C8
#define DP_WRITE_MSK                           0x16CC 
#define DP_XOP                                 0x17F8
#define CLR_CMP_CLR_SRC                        0x15C4
#define CLR_CMP_CLR_DST                        0x15C8
#define CLR_CMP_CNTL                           0x15C0
#define CLR_CMP_MSK                            0x15CC
#define DSTCACHE_MODE                          0x1710
#define DSTCACHE_CTLSTAT                       0x1714
#define DEFAULT_PITCH_OFFSET                   0x16E0 
#define DEFAULT_SC_BOTTOM_RIGHT                0x16E8
#define DP_GUI_MASTER_CNTL                     0x146C
#define SC_TOP_LEFT                            0x16EC
#define SC_BOTTOM_RIGHT                        0x16F0
#define SRC_SC_BOTTOM_RIGHT                    0x16F4
#define DEBUG0                                 0x1680 
#define DEBUG1                                 0x1684 
#define DEBUG2                                 0x1688 
#define DEBUG3                                 0x168C 
#define DEBUG4                                 0x1690 
#define DEBUG5                                 0x1694
#define DEBUG6                                 0x1698
#define DEBUG7                                 0x169C
#define DEBUG8                                 0x16A0 
#define DEBUG9                                 0x16A4 
#define DEBUG10                                0x16A8
#define DEBUG11                                0x16AC
#define DEBUG12                                0x16B0  
#define DEBUG13                                0x16B4
#define DEBUG14                                0x16B8 
#define DEBUG15                                0x16BC 
#define BIOS_0_SCRATCH                         0x0010
#define BIOS_1_SCRATCH                         0x0014
#define BIOS_2_SCRATCH                         0x0018
#define BIOS_3_SCRATCH                         0x001C
#define TVOUT_0_SCRATCH                        0x0088
#define TVOUT_1_SCRATCH                        0x008C
#define I2C_CNTL_0                             0x0090
#define I2C_CNTL_1                             0x0094
#define I2C_DATA                               0x0098
#define CONFIG_XSTRAP                          0x00E4
#define TEST_DEBUG_CNTL                        0x0120
#define TEST_DEBUG_MUX                         0x0124
#define TEST_DEBUG_OUT                         0x012C
#define VIDEOMUX_CNTL                          0x0190
#define VIPPAD_STRENGTH                        0x0194
#define VIPPAD_MASK                            0x0198
#define VIPPAD_A                               0x019C
#define VIPPAD_EN                              0x01A0
#define VIPPAD_Y                               0x01A4
#define VIPPAD1_MASK                           0x01A8
#define VIPPAD1_A                              0x01AC
#define VIPPAD1_EN                             0x01B0
#define VIPPAD1_Y                              0x01B4
#define EXTERN_TRIG_CNTL                       0x01BC
#define SEPROM_CNTL1                           0x01C0 
#define SEPROM_CNTL2                           0x01C4 
#define VIP_HW_DEBUG                           0x01CC
#define MEDIA_0_SCRATCH                        0x01F0
#define MEDIA_1_SCRATCH                        0x01F4
#define VID_BUFFER_CONTROL                     0x0900
#define CAP_INT_CNTL                           0x0908
#define CAP_INT_STATUS                         0x090C
#define FCP_CNTL                               0x0910
#define CAP0_BUF0_OFFSET                       0x0920
#define CAP0_BUF1_OFFSET                       0x0924
#define CAP0_BUF0_EVEN_OFFSET                  0x0928
#define CAP0_BUF1_EVEN_OFFSET                  0x092C
#define CAP0_BUF_PITCH                         0x0930
#define CAP0_V_WINDOW                          0x0934
#define CAP0_H_WINDOW                          0x0938
#define CAP0_VBI0_OFFSET                       0x093C  
#define CAP0_VBI1_OFFSET                       0x0940 
#define CAP0_VBI_V_WINDOW                      0x0944
#define CAP0_VBI_H_WINDOW                      0x0948
#define CAP0_PORT_MODE_CNTL                    0x094C
#define CAP0_TRIG_CNTL                         0x0950
#define CAP0_DEBUG                             0x0954
#define CAP0_CONFIG                            0x0958
#define CAP0_ANC0_OFFSET                       0x095C 
#define CAP0_ANC1_OFFSET                       0x0960  
#define CAP0_ANC_H_WINDOW                      0x0964
#define CAP0_VIDEO_SYNC_TEST                   0x0968
#define CAP0_ONESHOT_BUF_OFFSET                0x096C
#define CAP0_BUF_STATUS                        0x0970
#define CAP0_ANC_BUF01_BLOCK_CNT               0x0974
#define CAP0_ANC_BUF23_BLOCK_CNT               0x097C 
#define CAP0_VBI2_OFFSET                       0x0980
#define CAP0_VBI3_OFFSET                       0x0984
#define CAP0_ANC2_OFFSET                       0x0988
#define CAP0_ANC3_OFFSET                       0x098C
#define DMA_VIPH0_COMMAND                      0x0A00 
#define DMA_VIPH1_COMMAND                      0x0A04 
#define DMA_VIPH2_COMMAND                      0x0A08 
#define DMA_VIPH3_COMMAND                      0x0A0C   
#define DMA_VIPH_STATUS                        0x0A10
#define DMA_VIPH_CHUNK_0                       0x0A18 
#define DMA_VIPH_CHUNK_1_VAL                   0x0A1C 
#define DMA_VIP0_TABLE_ADDR                    0x0A20 
#define DMA_VIP1_TABLE_ADDR                    0x0A30 
#define DMA_VIP2_TABLE_ADDR                    0x0A40 
#define DMA_VIP3_TABLE_ADDR                    0x0A50 
#define DMA_VIPH0_ACTIVE                       0x0A24 
#define DMA_VIPH1_ACTIVE                       0x0A34 
#define DMA_VIPH2_ACTIVE                       0x0A44 
#define DMA_VIPH3_ACTIVE                       0x0A54 
#define DMA_VIPH_ABORT                         0x0A88
#define VIPH_REG_ADDR                          0x0080
#define VIPH_REG_DATA                          0x0084
#define VIPH_CH0_DATA                          0x0C00
#define VIPH_CH1_DATA                          0x0C04
#define VIPH_CH2_DATA                          0x0C08
#define VIPH_CH3_DATA                          0x0C0C
#define VIPH_CH0_ADDR                          0x0C10
#define VIPH_CH1_ADDR                          0x0C14
#define VIPH_CH2_ADDR                          0x0C18
#define VIPH_CH3_ADDR                          0x0C1C
#define VIPH_CH0_SBCNT                         0x0C20
#define VIPH_CH1_SBCNT                         0x0C24
#define VIPH_CH2_SBCNT                         0x0C28
#define VIPH_CH3_SBCNT                         0x0C2C
#define VIPH_CH0_ABCNT                         0x0C30
#define VIPH_CH1_ABCNT                         0x0C34
#define VIPH_CH2_ABCNT                         0x0C38
#define VIPH_CH3_ABCNT                         0x0C3C
#define VIPH_CONTROL                           0x0C40 
#define VIPH_DV_LAT                            0x0C44
#define VIPH_DMA_CHUNK                         0x0C48
#define VIPH_DV_INT                            0x0C4C
#define VIPH_TIMEOUT_STAT                      0x0C50
#define TMDS_CNTL                              0x0294
#define TMDS_SYNC_CHAR_SETA                    0x0298
#define TMDS_SYNC_CHAR_SETB                    0x029C
#define TMDS_CRC                               0x02A0
#define TMDS_TRANSMITTER_CNTL                  0x02A4
#define TMDS_PLL_CNTL                          0x02A8
#define TMDS_PATTERN_GEN_SEED                  0x02AC
#define IDCT_RUNS                              0x1F80
#define IDCT_LEVELS                            0x1F84
#define IDCT_CONTROL                           0x1FBC
#define IDCT_AUTH_CONTROL                      0x1F88
#define IDCT_AUTH                              0x1F8C

/****************************************************************************
 *                                                                          *
 *         Radeon INDIRECT REGISTER INDICES CONSTANTS SECTION               *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *                CLOCK_CNTL_INDEX Indexed Registers                        *
 ****************************************************************************/
#define CLK_PIN_CNTL                               0x0001
#define PPLL_CNTL                                  0x0002
#define PPLL_REF_DIV                               0x0003
#define PPLL_DIV_0                                 0x0004
#define PPLL_DIV_1                                 0x0005
#define PPLL_DIV_2                                 0x0006
#define PPLL_DIV_3                                 0x0007
#define VCLK_ECP_CNTL                              0x0008
#define HTOTAL_CNTL                                0x0009
#define M_SPLL_REF_FB_DIV                          0x000a 
#define AGP_PLL_CNTL                               0x000b 
#define SPLL_CNTL                                  0x000c 
#define SCLK_CNTL                                  0x000d 
#define MPLL_CNTL                                  0x000e
#define MCLK_CNTL                                  0x0012 
#define AGP_PLL_CNTL                               0x000b 
#define PLL_TEST_CNTL                              0x0013


/****************************************************************************
 *                                                                          *
 *            Radeon REGISTER MASK CONSTANTS SECTION                        *
 *                                                                          *
 ****************************************************************************/

/****************************************************************************
 *            Radeon Non-GUI Register Mask Constants                        *
 *  These are typically ORed together and are used with the register access *
 *  functions.                                                              *
 ****************************************************************************/

/* BUS_CNTL bit constants */
#define BUS_DBL_RESYNC                             0x00000001
#define BUS_MSTR_RESET                             0x00000002
#define BUS_FLUSH_BUF                              0x00000004
#define BUS_STOP_REQ_DIS                           0x00000008
#define BUS_READ_COMBINE_EN                        0x00000010 
#define BUS_WRITE_COMBINE_EN                       0x00000020 
#define BUS_MASTER_DIS                             0x00000040
#define BUS_ROM_WRT_EN                             0x00000080
#define BUS_VGA_PREFETCH_EN                        0x00000400 
#define BUS_SGL_READ_DISABLE                       0x00000800 
#define BUS_DIS_ROM                                0x00001000
#define BUS_PCI_READ_RETRY_EN                      0x00002000
#define BUS_AGP_AD_STEPPING_EN                     0x00004000
#define BUS_PCI_WRT_RETRY_EN                       0x00008000
#define BUS_MSTR_RD_MULT                           0x00100000
#define BUS_MSTR_RD_LINE                           0x00200000
#define BUS_SUSPEND                                0x00400000
#define LAT_16X                                    0x00800000
#define BUS_RD_DISCARD_EN                          0x01000000
#define ENFRCWRDY                                  0x02000000 
#define BUS_MSTR_WS                                0x04000000
#define BUS_PARKING_DIS                            0x08000000
#define BUS_MSTR_DISCONNECT_EN                     0x10000000
#define SERR_EN                                    0x20000000 
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

/* MM_INDEX bit constants */
#define MM_APER                                    0x80000000

/****************************************************************************
 *                                                                          *
 *                Radeon GUI Register Mask Constants                        *
 *                                                                          *
 *      These are typically ORed together and are used with the register    *
 *      access functions.                                                   *
 *                                                                          *
 ****************************************************************************/

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
#define DST_X_TILE                                 0x00000008
#define DST_Y_TILE                                 0x00000010

/* DP_CNTL_YDIR_XDIR_YMAJOR bit constants (short version of DP_CNTL) */
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
//#define BRUSH_8x1MONO                              0x00000200
//#define BRUSH_8x1MONO_LBKGD                        0x00000300
//#define BRUSH_1x8MONO                              0x00000400
//#define BRUSH_1x8MONO_LBKGD                        0x00000500
#define BRUSH_32x1MONO                             0x00000600
#define BRUSH_32x1MONO_LBKGD                       0x00000700
//#define BRUSH_32x32MONO                            0x00000800
//#define BRUSH_32x32MONO_LBKGD                      0x00000900
#define BRUSH_8x8COLOR                             0x00000a00
//#define BRUSH_8x1COLOR                             0x00000b00
//#define BRUSH_1x8COLOR                             0x00000c00
#define BRUSH_SOLIDCOLOR                           0x00000d00
#define BRUSH_SOLIDCOLOR_LINES                     0x00000e00 
#define BRUSH_SOLIDCOLOR_BLITS                     0x00000f00 
#define SRC_MONO                                   0x00000000
#define SRC_MONO_LBKGD                             0x00010000
#define SRC_DSTCOLOR                               0x00030000
#define SRC_8BPP                                   0x00050000 //m6
#define SRC_32BPP                                  0x00060000 //m6
#define SRC_OBUFFER                                0x00070000 //m6
#define BYTE_ORDER_MSB_TO_LSB                      0x00000000
#define BYTE_ORDER_LSB_TO_MSB                      0x40000000
//#define DP_CONVERSION_TEMP                         0x80000000

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
//#define GMC_BRUSH_8x1MONO                          0x00000020
//#define GMC_BRUSH_8x1MONO_LBKGD                    0x00000030
//#define GMC_BRUSH_1x8MONO                          0x00000040
//#define GMC_BRUSH_1x8MONO_LBKGD                    0x00000050
#define GMC_BRUSH_32x1MONO                         0x00000060
#define GMC_BRUSH_32x1MONO_LBKGD                   0x00000070
//#define GMC_BRUSH_32x32MONO                        0x00000080
//#define GMC_BRUSH_32x32MONO_LBKGD                  0x00000090
#define GMC_BRUSH_8x8COLOR                         0x000000a0
//#define GMC_BRUSH_8x1COLOR                         0x000000b0
//#define GMC_BRUSH_1x8COLOR                         0x000000c0
#define GMC_BRUSH_SOLIDCOLOR                       0x000000d0
#define GMC_BRUSH_SOLIDCOLOR_LINES                 0x000000e0 
#define GMC_BRUSH_SOLIDCOLOR_BLITS                 0x000000f0 
#define GMC_DST_8BPP                               0x00000200
#define GMC_DST_15BPP                              0x00000300
#define GMC_DST_16BPP                              0x00000400
//#define GMC_DST_24BPP                              0x00000500
#define GMC_DST_32BPP                              0x00000600
//#define GMC_DST_8BPP_RGB332                        0x00000700
//#define GMC_DST_8BPP_Y8                            0x00000800
//#define GMC_DST_8BPP_RGB8                          0x00000900
//#define GMC_DST_16BPP_VYUY422                      0x00000b00
//#define GMC_DST_16BPP_YVYU422                      0x00000c00
//#define GMC_DST_32BPP_AYUV444                      0x00000e00
//#define GMC_DST_16BPP_ARGB4444                     0x00000f00
#define GMC_SRC_MONO                               0x00000000
#define GMC_SRC_MONO_LBKGD                         0x00001000
#define GMC_SRC_DSTCOLOR                           0x00003000
#define GMC_BYTE_ORDER_MSB_TO_LSB                  0x00000000
#define GMC_BYTE_ORDER_LSB_TO_MSB                  0x00004000
//#define GMC_DP_CONVERSION_TEMP_9300                0x00008000
//#define GMC_DP_CONVERSION_TEMP_6500                0x00000000
#define GMC_DP_SRC_RECT                            0x02000000
#define GMC_DP_SRC_HOST                            0x03000000
#define GMC_DP_SRC_HOST_BYTEALIGN                  0x04000000
//#define GMC_3D_FCN_EN_CLR                          0x00000000
//#define GMC_3D_FCN_EN_SET                          0x08000000
#define GMC_DST_CLR_CMP_FCN_LEAVE                  0x00000000
#define GMC_DST_CLR_CMP_FCN_CLEAR                  0x10000000
//#define GMC_AUX_CLIP_LEAVE                         0x00000000
//#define GMC_AUX_CLIP_CLEAR                         0x20000000
#define GMC_WRITE_MASK_LEAVE                       0x00000000
#define GMC_WRITE_MASK_SET                         0x40000000

/* DP_MIX bit constants */
#define DP_SRC_RECT                                0x00000200
#define DP_SRC_HOST                                0x00000300
#define DP_SRC_HOST_BYTEALIGN                      0x00000400

/****************************************************************************
 *                                                                          *
 *              DP_MIX and DP_GUI_MASTER_CNTL ROP3 RPN constants            *
 *                                                                          *
 *      P = pattern                                                         *
 *      S = source                                                          *
 *      D = destination                                                     *
 *      o = OR                                                              *
 *      a = AND                                                             *
 *      x = XOR                                                             *
 *      n = NOT                                                             *
 *                                                                          *
 ****************************************************************************/
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

/****************************************************************************
 *              DP_MIX, DP_GUI_MASTER_CNTL ROP3 named constants             *
 *                                                                          *
 *                  Pattern (P):     1 1 1 1 0 0 0 0                        *
 *                  Source  (S):     1 1 0 0 1 1 0 0                        *
 *                  Destination (D): 1 0 1 0 1 0 1 0                        *
 *                                                                          *
 ****************************************************************************/
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

// Register Definition for Rage Theater
// Address Space Rage Theatre Registers (VIP Access)
#define VIP_VIP_VENDOR_DEVICE_ID                 0x0000
#define VIP_VIP_SUB_VENDOR_DEVICE_ID             0x0004
#define VIP_VIP_COMMAND_STATUS                   0x0008
#define VIP_VIP_REVISION_ID                      0x000c
#define VIP_XHW_DEBUG                            0x0010 //j renamed
#define VIP_SW_SCRATCH                           0x0014
#define VIP_I2C_CNTL_0                           0x0020
#define VIP_I2C_CNTL_1                           0x0024
#define VIP_I2C_DATA                             0x0028
#define VIP_INT_CNTL                             0x002c
#define VIP_GPIO_INOUT                           0x0030
#define VIP_GPIO_CNTL                            0x0034
#define VIP_CLKOUT_GPIO_CNTL                     0x0038
#define VIP_RIPINTF_PORT_CNTL                    0x003c
#define VIP_ADC_CNTL                             0x0400
#define VIP_ADC_DEBUG                            0x0404
#define VIP_STANDARD_SELECT                      0x0408
#define VIP_THERMO2BIN_STATUS                    0x040c
#define VIP_COMB_CNTL0                           0x0440
#define VIP_COMB_CNTL1                           0x0444
#define VIP_COMB_CNTL2                           0x0448
#define VIP_COMB_LINE_LENGTH                     0x044c
#define VIP_NOISE_CNTL0                          0x0450
#define VIP_HS_PLINE                             0x0480
#define VIP_HS_DTOINC                            0x0484
#define VIP_HS_PLLGAIN                           0x0488
#define VIP_HS_MINMAXWIDTH                       0x048c
#define VIP_HS_GENLOCKDELAY                      0x0490
#define VIP_HS_WINDOW_LIMIT                      0x0494
#define VIP_HS_WINDOW_OC_SPEED                   0x0498
#define VIP_HS_PULSE_WIDTH                       0x049c
#define VIP_HS_PLL_ERROR                         0x04a0
#define VIP_HS_PLL_FS_PATH                       0x04a4
#define VIP_SG_BLACK_GATE                        0x04c0
#define VIP_SG_SYNCTIP_GATE                      0x04c4
#define VIP_SG_UVGATE_GATE                       0x04c8
#define VIP_LP_AGC_CLAMP_CNTL0                   0x0500
#define VIP_LP_AGC_CLAMP_CNTL1                   0x0504
#define VIP_LP_BRIGHTNESS                        0x0508
#define VIP_LP_CONTRAST                          0x050c
#define VIP_LP_SLICE_LIMIT                       0x0510
#define VIP_LP_WPA_CNTL0                         0x0514
#define VIP_LP_WPA_CNTL1                         0x0518
#define VIP_LP_BLACK_LEVEL                       0x051c
#define VIP_LP_SLICE_LEVEL                       0x0520
#define VIP_LP_SYNCTIP_LEVEL                     0x0524
#define VIP_LP_VERT_LOCKOUT                      0x0528
#define VIP_VS_DETECTOR_CNTL                     0x0540
#define VIP_VS_BLANKING_CNTL                     0x0544
#define VIP_VS_FIELD_ID_CNTL                     0x0548
#define VIP_VS_COUNTER_CNTL                      0x054c
#define VIP_VS_FRAME_TOTAL                       0x0550
#define VIP_VS_LINE_COUNT                        0x0554
#define VIP_CP_PLL_CNTL0                         0x0580
#define VIP_CP_PLL_CNTL1                         0x0584
#define VIP_CP_HUE_CNTL                          0x0588
#define VIP_CP_BURST_GAIN                        0x058c
#define VIP_CP_AGC_CNTL                          0x0590
#define VIP_CP_ACTIVE_GAIN                       0x0594
#define VIP_CP_PLL_STATUS0                       0x0598
#define VIP_CP_PLL_STATUS1                       0x059c
#define VIP_CP_PLL_STATUS2                       0x05a0
#define VIP_CP_PLL_STATUS3                       0x05a4
#define VIP_CP_PLL_STATUS4                       0x05a8
#define VIP_CP_PLL_STATUS5                       0x05ac
#define VIP_CP_PLL_STATUS6                       0x05b0
#define VIP_CP_PLL_STATUS7                       0x05b4
#define VIP_CP_DEBUG_FORCE                       0x05b8
#define VIP_CP_VERT_LOCKOUT                      0x05bc
#define VIP_H_ACTIVE_WINDOW                      0x05c0
#define VIP_V_ACTIVE_WINDOW                      0x05c4
#define VIP_H_VBI_WINDOW                         0x05c8
#define VIP_V_VBI_WINDOW                         0x05cc
#define VIP_VBI_CONTROL                          0x05d0
#define VIP_DECODER_DEBUG_CNTL                   0x05d4
#define VIP_SINGLE_STEP_DATA                     0x05d8
#define VIP_MASTER_CNTL                          0x0040
#define VIP_RGB_CNTL                             0x0048
#define VIP_CLKOUT_CNTL                          0x004c
#define VIP_SYNC_CNTL                            0x0050
#define VIP_I2C_CNTL                             0x0054
#define VIP_HTOTAL                               0x0080
#define VIP_HDISP                                0x0084
#define VIP_HSIZE                                0x0088
#define VIP_HSTART                               0x008c
#define VIP_HCOUNT                               0x0090
#define VIP_VTOTAL                               0x0094
#define VIP_VDISP                                0x0098
#define VIP_VCOUNT                               0x009c
#define VIP_VFTOTAL                              0x00a0
#define VIP_DFCOUNT                              0x00a4
#define VIP_DFRESTART                            0x00a8
#define VIP_DHRESTART                            0x00ac
#define VIP_DVRESTART                            0x00b0
#define VIP_SYNC_SIZE                            0x00b4
#define VIP_TV_PLL_FINE_CNTL                     0x00b8
#define VIP_CRT_PLL_FINE_CNTL                    0x00bc
#define VIP_TV_PLL_CNTL                          0x00c0
#define VIP_CRT_PLL_CNTL                         0x00c4
#define VIP_PLL_CNTL0                            0x00c8
#define VIP_PLL_TEST_CNTL                        0x00cc
#define VIP_CLOCK_SEL_CNTL                       0x00d0
#define VIP_VIN_PLL_CNTL                         0x00d4
#define VIP_VIN_PLL_FINE_CNTL                    0x00d8
#define VIP_AUD_PLL_CNTL                         0x00e0
#define VIP_AUD_PLL_FINE_CNTL                    0x00e4
#define VIP_AUD_CLK_DIVIDERS                     0x00e8
#define VIP_AUD_DTO_INCREMENTS                   0x00ec
#define VIP_L54_PLL_CNTL                         0x00f0
#define VIP_L54_PLL_FINE_CNTL                    0x00f4
#define VIP_L54_DTO_INCREMENTS                   0x00f8
#define VIP_PLL_CNTL1                            0x00fc
#define VIP_FRAME_LOCK_CNTL                      0x0100
#define VIP_SYNC_LOCK_CNTL                       0x0104
#define VIP_TVO_SYNC_PAT_ACCUM                   0x0108
#define VIP_TVO_SYNC_THRESHOLD                   0x010c
#define VIP_TVO_SYNC_PAT_EXPECT                  0x0110
#define VIP_DELAY_ONE_MAP_A                      0x0114
#define VIP_DELAY_ONE_MAP_B                      0x0118
#define VIP_DELAY_ZERO_MAP_A                     0x011c
#define VIP_DELAY_ZERO_MAP_B                     0x0120
#define VIP_TVO_DATA_DELAY_A                     0x0140
#define VIP_TVO_DATA_DELAY_B                     0x0144
#define VIP_HOST_READ_DATA                       0x0180
#define VIP_HOST_WRITE_DATA                      0x0184
#define VIP_HOST_RD_WT_CNTL                      0x0188
#define VIP_VSCALER_CNTL1                        0x01c0
#define VIP_TIMING_CNTL                          0x01c4
#define VIP_VSCALER_CNTL2                        0x01c8
#define VIP_Y_FALL_CNTL                          0x01cc
#define VIP_Y_RISE_CNTL                          0x01d0
#define VIP_Y_SAW_TOOTH_CNTL                     0x01d4
#define VIP_UPSAMP_AND_GAIN_CNTL                 0x01e0
#define VIP_GAIN_LIMIT_SETTINGS                  0x01e4
#define VIP_LINEAR_GAIN_SETTINGS                 0x01e8
#define VIP_MODULATOR_CNTL1                      0x0200
#define VIP_MODULATOR_CNTL2                      0x0204
#define VIP_MV_MODE_CNTL                         0x0208
#define VIP_MV_STRIPE_CNTL                       0x020c
#define VIP_MV_LEVEL_CNTL1                       0x0210
#define VIP_MV_LEVEL_CNTL2                       0x0214
#define VIP_PRE_DAC_MUX_CNTL                     0x0240
#define VIP_TV_DAC_CNTL                          0x0280
#define VIP_CRC_CNTL                             0x02c0
#define VIP_VIDEO_PORT_SIG                       0x02c4
#define VIP_VBI_CC_CNTL                          0x02c8
#define VIP_VBI_EDS_CNTL                         0x02cc
#define VIP_VBI_20BIT_CNTL                       0x02d0
#define VIP_VBI_DTO_CNTL                         0x02d4
#define VIP_VBI_LEVEL_CNTL                       0x02d8
#define VIP_UV_ADR                               0x0300
#define VIP_MV_STATUS                            0x0330
#define VIP_UPSAMP_COEFF0_0                      0x0340
#define VIP_UPSAMP_COEFF0_1                      0x0344
#define VIP_UPSAMP_COEFF0_2                      0x0348
#define VIP_UPSAMP_COEFF1_0                      0x034c
#define VIP_UPSAMP_COEFF1_1                      0x0350
#define VIP_UPSAMP_COEFF1_2                      0x0354
#define VIP_UPSAMP_COEFF2_0                      0x0358
#define VIP_UPSAMP_COEFF2_1                      0x035c
#define VIP_UPSAMP_COEFF2_2                      0x0360
#define VIP_UPSAMP_COEFF3_0                      0x0364
#define VIP_UPSAMP_COEFF3_1                      0x0368
#define VIP_UPSAMP_COEFF3_2                      0x036c
#define VIP_UPSAMP_COEFF4_0                      0x0370
#define VIP_UPSAMP_COEFF4_1                      0x0374
#define VIP_UPSAMP_COEFF4_2                      0x0378
#define VIP_TV_DTO_INCREMENTS                    0x0390
#define VIP_CRT_DTO_INCREMENTS                   0x0394
#define VIP_VSYNC_DIFF_CNTL                      0x03a0
#define VIP_VSYNC_DIFF_LIMITS                    0x03a4
#define VIP_VSYNC_DIFF_RD_DATA                   0x03a8
#define VIP_SCALER_IN_WINDOW                     0x0618
#define VIP_SCALER_OUT_WINDOW                    0x061c
#define VIP_H_SCALER_CONTROL                     0x0600
#define VIP_V_SCALER_CONTROL                     0x0604
#define VIP_V_DEINTERLACE_CONTROL                0x0608
#define VIP_VBI_SCALER_CONTROL                   0x060c
#define VIP_DVS_PORT_CTRL                        0x0610
#define VIP_DVS_PORT_READBACK                    0x0614
#define VIP_FIFOA_CONFIG                         0x0800
#define VIP_FIFOB_CONFIG                         0x0804
#define VIP_FIFOC_CONFIG                         0x0808
#define VIP_SPDIF_PORT_CNTL                      0x080c
#define VIP_SPDIF_CHANNEL_STAT                   0x0810
#define VIP_SPDIF_AC3_PREAMBLE                   0x0814
#define VIP_I2S_TRANSMIT_CNTL                    0x0818
#define VIP_I2S_RECEIVE_CNTL                     0x081c
#define VIP_SPDIF_TX_CNT_REG                     0x0820
#define VIP_IIS_TX_CNT_REG                       0x0824


//m6 specific registers 
/****************************************************************************
 *                CLOCK_CNTL_INDEX Indexed Registers                        *
 ****************************************************************************/
#define MCLK_MISC                                0x1F   //m6
#define P2PLL_CNTL                               0x2A   //m6
#define P2PLL_REF_DIV                            0x2B   //m6
#define P2PLL_DIV_0                              0x2C   //m6
#define PIXCLKS_CNTL                             0x2D   //m6
#define HTOTAL2_CNTL                             0x2E   //m6

// [blockIO] : io Registers
#define ioBIOS_4_SCRATCH                         0x20   //m6
#define ioBIOS_5_SCRATCH                         0x24   //m6
#define ioBIOS_6_SCRATCH                         0x28   //m6
#define ioBIOS_7_SCRATCH                         0x2C   //m6
#define ioGPIO_CRT2_DOC                          0x6C   //m6
#define ioDAC_CNTL2                              0x7C   //m6

// [primaryRegisterAperture] :  Registers
#define BIOS_4_SCRATCH                           0x0020 //m6
#define BIOS_5_SCRATCH                           0x0024 //m6
#define BIOS_6_SCRATCH                           0x0028 //m6
#define BIOS_7_SCRATCH                           0x002C //m6
#define GPIO_CRT2_DDC                            0x006C //m6
#define DAC_CNTL2                                0x007C //m6

#define GPIOPAD_MASK                             0x0198 //m6
#define GPIOPAD_A                                0x019C //m6
#define GPIOPAD_EN                               0x01A0 //m6
#define GPIOPAD_Y                                0x01A4 //m6
#define ZV_LCDPAD_MASK                           0x01A8 //m6
#define ZV_LCDPAD_A                              0x01AC //m6
#define ZV_LCDPAD_EN                             0x01B0 //m6
#define ZV_LCDPAD_Y                              0x01B4 //m6
#define FP2_GEN_CNTL                             0x0288 //m6
#define ICON_OFFSET                              0x02B0 //m6
#define ICON_HORZ_VERT_POSN                      0x02B4 //m6
#define ICON_HORZ_VERT_OFF                       0x02B8 //m6
#define ICON_CLR0                                0x02BC //m6
#define ICON_CLR1                                0x02C0 //m6
#define LVDS_DIGTMDS_CRC		         0x02CC //m6
#define LVDS_GEN_CNTL   		         0x02D0 //m6
#define LVDS_PLL_CNTL   		         0x02D4 //m6
#define DVI_I2C_CNTL_0				 0x02E0 //m6
#define DVI_I2C_CNTL_1				 0x02E4 //m6
#define DVI_I2C_DATA				 0x02E8 //m6
#define LVDS_SS_GEN_CNTL		         0x02EC //m6
#define CRTC2_H_TOTAL_DISP                       0x0300 //m6
#define CRTC2_H_SYNC_STRT_WID                    0x0304 //m6
#define CRTC2_V_TOTAL_DISP                       0x0308 //m6
#define CRTC2_V_SYNC_STRT_WID                    0x030C //m6
#define CRTC2_VLINE_CRNT_VLINE                   0x0310 //m6
#define CRTC2_CRNT_FRAME                         0x0314 //m6
#define CRTC2_GUI_TRIG_VLINE                     0x0318 //m6
#define CRTC2_DEBUG                              0x031C //m6
#define CRTC2_OFFSET                             0x0324 //m6
#define CRTC2_OFFSET_CNTL                        0x0328 //m6
#define CRTC2_PITCH                              0x032C //m6
#define OVR2_CLR                                 0x0330 //m6
#define OVR2_WID_LEFT_RIGHT                      0x0334 //m6
#define OVR2_WID_TOP_BOTTOM                      0x0338 //m6
#define CRTC2_DISPLAY_BASE_ADDR                  0x033C //m6
#define SNAPSHOT2_VH_COUNTS                      0x0340 //m6
#define SNAPSHOT2_F_COUNT                        0x0344 //m6
#define N_VIF2_COUNT                             0x0348 //m6
#define SNAPSHOT2_VIF_COUNT                      0x034C //m6
#define CUR2_OFFSET                              0x0360 //m6
#define CUR2_HORZ_VERT_POSN                      0x0364 //m6
#define CUR2_HORZ_VERT_OFF                       0x0368 //m6
#define CUR2_CLR0                                0x036C //m6
#define CUR2_CLR1                                0x0370 //m6
#define ICON2_OFFSET                             0x03B0 //m6
#define ICON2_HORZ_VERT_POSN                     0x03B4 //m6
#define ICON2_HORZ_VERT_OFF                      0x03B8 //m6
#define ICON2_CLR0                               0x03BC //m6
#define ICON2_CLR1                               0x03C0 //m6
#define FP_H2_SYNC_STRT_WID                      0x03C4 //m6
#define FP_V2_SYNC_STRT_WID                      0x03C8 //m6
#define CRTC2_GEN_CNTL                           0x03F8 //m6
#define CRTC2_STATUS                             0x03FC //m6
#define OV1_Y_X_START                            0x0600 //m6
#define OV1_Y_X_END                              0x0604 //m6
#define OV1_PIPELINE_CNTL                        0x0608 //m6
#define CP_RB_RPTR_WR                            0x071C //m6
#define DISP2_MERGE_CNTL                         0x0D68 //m6
#define DEFAULT2_PITCH_OFFSET                    0x16F8 //m6
#define DEFAULT2_SC_BOTTOM_RIGHT                 0x16DC //m6

// RBBM STATUS values
#define GUI_ACTIVE                               0x80000000
// RB2D_DSTCACHE_CTLSTAT
#define DC_BUSY                                  0x80000000

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif // _Radeon_h

