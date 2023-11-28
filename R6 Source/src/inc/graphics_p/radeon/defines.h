/******************************************************************************
 * defines.h file for Radeon sample code                                      *
 *                                                                            *
 * Copyright (c) 2000 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/

#ifndef _DEFINES_H
#define _DEFINES_H

// Miscellaneous.

#define OFF 0
#define ON  1

#define TRUE 1
#define FALSE 0

// Defines for Radeon_AdapterInfo.FLAGS
#define Radeon_USE_BIOS   0x1
#define Radeon_FORCE_PCI  0x2

// Radeon Device ID codes
#define DEVICE_ID_CODE_QD 0x5144                //RADEON QD
#define DEVICE_ID_CODE_QE 0x5145                //RADEON QE
#define DEVICE_ID_CODE_QF 0x5146                //RADEON QF
#define DEVICE_ID_CODE_QG 0x5147                //RADEON QG
#define DEVICE_ID_CODE_M6 0x4C59 		//RADEON M6
#define DEVICE_ID_CODE_RV100 0x5159             //RADEON RV100

//#define VENDOR_ID_CODE 0x1002
#define ATI_PCI_VENDOR_ID 0x1002

// Generic colour definitions for use in Radeon_GetColourCode ().

#define BLACK            0
#define DARKBLUE         1
#define DARKGREEN        2
#define DARKCYAN         3
#define DARKRED          4
#define DARKMAGENTA      5
#define BROWN            6
#define LIGHTGRAY        7
#define DARKGRAY         8
#define LIGHTBLUE        9
#define LIGHTGREEN      10
#define LIGHTCYAN       11
#define LIGHTRED        12
#define LIGHTMAGENTA    13
#define YELLOW          14
#define WHITE           15
#define NUM_COLOURS     16

// Needed for VIEWIMG.C

#define MEM_READ                0
#define MEM_WRITE               1
#define LOAD_SUCCESSFUL     0
#define LOAD_FAILED             1
#define YES_ERROR               1
#define NO_ERROR                0
#define VGA_APERTURE_ENABLED    1
#define VGA_APERTURE_DISABLED   0
#define LOAD_NORMAL     0
#define LOAD_HEADER_ONLY    1
#define FORMAT_TYPE_8BPP    2
#define FORMAT_TYPE_15BPP   3
#define FORMAT_TYPE_16BPP   4
#define FORMAT_TYPE_32BPP   6
#define FORMAT_TYPE_YUV9    9
#define FORMAT_TYPE_YUV12   10
#define FORMAT_TYPE_VYUY    11
#define FORMAT_TYPE_YVYU    12

// for INITENG.C

#define PC_FLUSH_GUI    0x00000003
#define PC_RI_GUI   0x0000000C
#define PC_FLUSH_NONGUI 0x00000030
#define PC_RI_NONGUI    0x000000C0

#define FORCE_GCP   0x00010000
#define FORCE_PIPE3D_CP 0x00020000

// constants used in PLL
#define PPLL_RESET 0x01
#define PPLL_ATOMIC_UPDATE_EN 0x10000
#define PPLL_VGA_ATOMIC_UPDATE_EN 0x20000
#define PPLL_REF_DIV_MASK 0x3FF
#define PPLL_FB3_DIV_MASK 0x7FF
#define PPLL_POST3_DIV_MASK 0x70000
#define PPLL_ATOMIC_UPDATE_R 0x8000
#define PPLL_ATOMIC_UPDATE_W 0x8000
#define MEM_CFG_TYPE_MASK 0x40000000
#define XCLK_SRC_SEL_MASK 0x7
#define XPLL_FB_DIV_MASK 0xFF00
#define X_MPLL_REF_DIV_MASK 0xFF

// some pll constants
#define PLL_DIV1                0x0000
#define PLL_DIV2                0x0001
#define PLL_DIV4                0x0002
#define PLL_DIV8                0x0003
#define PLL_EXTDIV              0x0004
#define PLL_DIV3                0x0004
#define PLL_DIV5                0x0005
#define PLL_DIV6                0x0006
#define PLL_DIV12               0x0007

// bits of CRTCInfoFlags
#define CI_DBLSCAN      1
#define CI_INTERLACE    2
#define CI_HPOLARITY    4
#define CI_VPOLARITY    8

// CRTC constants and masks
#define CRTC_HSYNC_DIS 0x100
#define CRTC_VSYNC_DIS 0x200
#define CRTC_DISPLAY_DIS 0x400
#define VGA_XCRT_CNT_EN 0x40
#define DAC_RANGE_CNTL 0x00000003
#define DAC_BLANKING 0x00000004
#define DAC_MASK 0xFF000000

// Engine Init Defines
#define BAD_IDLE            0
#define BAD_FIFO            1
#define DOS_TICK_ADDRESS    0x0000046C
#define IDLE_TIMEOUT        50
#define FIFO_TIMEOUT        50
#define ENGINE_IDLE         0

// Cursor Control
#define LEFT_BUTTON_DOWN 0x1
#define LEFT_BUTTON_UP 0x0
#define RIGHT_BUTTON_DOWN 0x2
#define RIGHT_BUTTON_UP 0x00
#define UP_ARROW 72
#define DOWN_ARROW 80
#define RIGHT_ARROW 77
#define LEFT_ARROW 75
#define NUM_CURSORS 4

// for Load_Image () in LOADBMAP.C

#define TRAJECTORY_RECTANGULAR  0
#define TRAJECTORY_LINEAR       1

#define align(a,b) ((((a) + ((b) - 1)) / (b)) * (b))

// for chapter 8

#define BT819_READ 0x89
#define BT819_WRITE 0x88
#define TDA9850 0xB4
#define AIW_BT829_READ 0x8B
#define AIW_BT829_WRITE 0x8A
#define BOARD_ID 0x70
#define TUNER_ADDRESS 0xC0

#define IGNORE_VBI 22 // skip 21 lines for VBI
#define DEC_SRC_COMPOSITE 0x1
#define DEC_SRC_SVIDEO 0x3
#define DEC_SRC_TUNER 0x2
#define CAP_MODE_CONTINUOUS 0x1
#define CAP_MODE_ONE_SHOT 0x0
#define CAP_BUF_TYPE_FIELD 0x00000000
#define CAP_BUF_TYPE_ALTERNATING 0x00000010
#define CAP_BUF_TYPE_FRAME 0x00000020
#define CAP_BUF_MODE_SINGLE 0x00000000
#define CAP_BUF_MODE_DOUBLE 0x00000080
#define CAP_BUF_MODE_TRIPLE 0x00000100
#define CAP_STREAM_FORMAT_BT 0x00000000
#define CAP_STREAM_FORMAT_CCIR656 0x00800000
#define CAP_STREAM_FORMAT_ZV 0x01000000
#define CAP_STREAM_FORMAT_VIP 0x01800000
#define CAP_HDWNS_DEC_DOWNSCALER 0x00000000
#define CAP_HDWNS_DEC_DECIMATE 0x04000000
#define CAP_VDWNS_DEC_DOWNSCALER 0x00000000
#define CAP_VDWNS_DEC_DECIMATE 0x08000000
#define CAP_VIDEO_IN_FORMAT_YVYU422 0x00000000
#define CAP_VIDEO_IN_FORMAT_VYUY422 0x20000000

#define LOW_BAND 91
#define MID_BAND 209
#define MID_HIGH_BAND 497
#define HIGH_BAND 509

// for Rage Theater

#define MAX_DNAME_SIZE         80
#define MAX_POSSIB_CARDS        4
#define MAX_VIP_DEVICES         4   // Max VIP devices is 4
#define READONLY                1   // Read Only Register
#define READWRITE               2   // Read / Write Register

// I2C address for MSP3430G
#define MSP3430         0x80
#define CONTROL         0x00
#define WR_DEM          0x10
#define RD_DEM          0x11
#define WR_DSP          0x12
#define RD_DSP          0x13

// RT Register Field Defaults:
#define     fld_tmpReg1_def     (DWORD) 0x00000000
#define     fld_tmpReg2_def     (DWORD) 0x00000001
#define     fld_tmpReg3_def     (DWORD) 0x00000002

#define     fld_LP_CONTRAST_def     (DWORD) 0x0000006e
#define     fld_LP_BRIGHTNESS_def   (DWORD) 0x00003ff0
#define     fld_CP_HUE_CNTL_def     (DWORD) 0x00000000
#define     fld_LUMA_FILTER_def     (DWORD) 0x00000001
#define     fld_H_SCALE_RATIO_def   (DWORD) 0x00010000
#define     fld_H_SHARPNESS_def     (DWORD) 0x00000000

#define     fld_V_SCALE_RATIO_def   (DWORD) 0x00000800
#define     fld_V_DEINTERLACE_ON_def    (DWORD) 0x00000001
#define     fld_V_BYPSS_def             (DWORD) 0x00000000
#define     fld_V_DITHER_ON_def         (DWORD) 0x00000001
#define     fld_EVENF_OFFSET_def        (DWORD) 0x00000000
#define     fld_ODDF_OFFSET_def         (DWORD) 0x00000000

#define     fld_INTERLACE_DETECTED_def  (DWORD) 0x00000000

#define     fld_VS_LINE_COUNT_def   (DWORD) 0x00000000
#define     fld_VS_DETECTED_LINES_def   (DWORD) 0x00000000
#define     fld_VS_ITU656_VB_def    (DWORD) 0x00000000

#define     fld_VBI_CC_DATA_def         (DWORD) 0x00000000
#define     fld_VBI_CC_WT_def           (DWORD) 0x00000000
#define     fld_VBI_CC_WT_ACK_def       (DWORD) 0x00000000
#define     fld_VBI_CC_HOLD_def         (DWORD) 0x00000000
#define     fld_VBI_DECODE_EN_def       (DWORD) 0x00000000

#define     fld_VBI_CC_DTO_P_def        (DWORD) 0x00001802
#define     fld_VBI_20BIT_DTO_P_def     (DWORD) 0x0000155c

#define     fld_VBI_CC_LEVEL_def        (DWORD) 0x0000003f
#define     fld_VBI_20BIT_LEVEL_def     (DWORD) 0x00000059
#define     fld_VBI_CLK_RUNIN_GAIN_def  (DWORD) 0x0000010f

#define     fld_H_VBI_WIND_START_def    (DWORD) 0x00000041
#define     fld_H_VBI_WIND_END_def      (DWORD) 0x00000366

#define     fld_V_VBI_WIND_START_def    (DWORD) 0x0D
#define     fld_V_VBI_WIND_END_def      (DWORD) 0x24

#define     fld_VBI_20BIT_DATA0_def     (DWORD) 0x00000000
#define     fld_VBI_20BIT_DATA1_def     (DWORD) 0x00000000
#define     fld_VBI_20BIT_WT_def        (DWORD) 0x00000000
#define     fld_VBI_20BIT_WT_ACK_def    (DWORD) 0x00000000
#define     fld_VBI_20BIT_HOLD_def      (DWORD) 0x00000000

#define     fld_VBI_CAPTURE_ENABLE_def  (DWORD) 0x00000000

#define     fld_VBI_EDS_DATA_def        (DWORD) 0x00000000
#define     fld_VBI_EDS_WT_def          (DWORD) 0x00000000
#define     fld_VBI_EDS_WT_ACK_def      (DWORD) 0x00000000
#define     fld_VBI_EDS_HOLD_def        (DWORD) 0x00000000

#define     fld_VBI_SCALING_RATIO_def   (DWORD) 0x00010000
#define     fld_VBI_ALIGNER_ENABLE_def  (DWORD) 0x00000000

#define     fld_H_ACTIVE_START_def      (DWORD) 0x00000070
#define     fld_H_ACTIVE_END_def        (DWORD) 0x000002f0

#define     fld_V_ACTIVE_START_def      (DWORD) ((22-4)*2+1)
#define     fld_V_ACTIVE_END_def        (DWORD) ((22+240-4)*2+2)

#define     fld_CH_HEIGHT_def           (DWORD) 0x000000CD
#define     fld_CH_KILL_LEVEL_def       (DWORD) 0x000000C0
#define     fld_CH_AGC_ERROR_LIM_def    (DWORD) 0x00000002
#define     fld_CH_AGC_FILTER_EN_def    (DWORD) 0x00000000
#define     fld_CH_AGC_LOOP_SPEED_def   (DWORD) 0x00000000

#define     fld_HUE_ADJ_def             (DWORD) 0x00000000

#define     fld_STANDARD_SEL_def        (DWORD) 0x00000000
#define     fld_STANDARD_YC_def         (DWORD) 0x00000000

#define     fld_ADC_PDWN_def            (DWORD) 0x00000001
#define     fld_INPUT_SELECT_def        (DWORD) 0x00000000

#define     fld_ADC_PREFLO_def          (DWORD) 0x00000003
#define     fld_H_SYNC_PULSE_WIDTH_def  (DWORD) 0x00000000
#define     fld_HS_GENLOCKED_def        (DWORD) 0x00000000
#define     fld_HS_SYNC_IN_WIN_def      (DWORD) 0x00000000

#define     fld_VIN_ASYNC_RST_def       (DWORD) 0x00000001
#define     fld_DVS_ASYNC_RST_def       (DWORD) 0x00000001

// Vendor IDs:
#define     fld_VIP_VENDOR_ID_def       (DWORD) 0x00001002
#define     fld_VIP_DEVICE_ID_def       (DWORD) 0x00004d54
#define     fld_VIP_REVISION_ID_def     (DWORD) 0x00000001

// AGC Delay Register
#define     fld_BLACK_INT_START_def     (DWORD) 0x00000031
#define     fld_BLACK_INT_LENGTH_def    (DWORD) 0x0000000f

#define     fld_UV_INT_START_def        (DWORD) 0x0000003b
#define     fld_U_INT_LENGTH_def        (DWORD) 0x0000000f
#define     fld_V_INT_LENGTH_def        (DWORD) 0x0000000f
#define     fld_CRDR_ACTIVE_GAIN_def    (DWORD) 0x0000007a
#define     fld_CBDB_ACTIVE_GAIN_def    (DWORD) 0x000000ac

#define     fld_DVS_DIRECTION_def       (DWORD) 0x00000000
#define     fld_DVS_VBI_BYTE_SWAP_def   (DWORD) 0x00000000
#define     fld_DVS_CLK_SELECT_def      (DWORD) 0x00000000
#define     fld_CONTINUOUS_STREAM_def   (DWORD) 0x00000000
#define     fld_DVSOUT_CLK_DRV_def      (DWORD) 0x00000001
#define     fld_DVSOUT_DATA_DRV_def     (DWORD) 0x00000001

#define     fld_COMB_CNTL0_def          (DWORD) 0x09438090
#define     fld_COMB_CNTL1_def          (DWORD) 0x00000010

#define     fld_COMB_CNTL2_def          (DWORD) 0x16161010
#define     fld_COMB_LENGTH_def         (DWORD) 0x0718038A

#define     fld_SYNCTIP_REF0_def        (DWORD) 0x00000037
#define     fld_SYNCTIP_REF1_def        (DWORD) 0x00000029
#define     fld_CLAMP_REF_def           (DWORD) 0x0000003B
#define     fld_AGC_PEAKWHITE_def       (DWORD) 0x000000FF
#define     fld_VBI_PEAKWHITE_def       (DWORD) 0x000000D2

#define     fld_WPA_THRESHOLD_def       (DWORD) 0x000003B0

#define     fld_WPA_TRIGGER_LO_def      (DWORD) 0x000000B4
#define     fld_WPA_TRIGGER_HIGH_def    (DWORD) 0x0000021C

#define     fld_LOCKOUT_START_def       (DWORD) 0x00000206
#define     fld_LOCKOUT_END_def         (DWORD) 0x00000021

#define     fld_CH_DTO_INC_def          (DWORD) 0x00400000
#define     fld_PLL_SGAIN_def           (DWORD) 0x00000001
#define     fld_PLL_FGAIN_def           (DWORD) 0x00000002

#define     fld_CR_BURST_GAIN_def       (DWORD) 0x0000007a
#define     fld_CB_BURST_GAIN_def       (DWORD) 0x000000ac

#define     fld_VERT_LOCKOUT_START_def  (DWORD) 0x00000207
#define     fld_VERT_LOCKOUT_END_def    (DWORD) 0x0000000E

#define     fld_H_IN_WIND_START_def     (DWORD) 0x00000070
#define     fld_V_IN_WIND_START_def     (DWORD) 0x00000027

#define     fld_H_OUT_WIND_WIDTH_def    (DWORD) 0x000002f4

#define     fld_V_OUT_WIND_WIDTH_def    (DWORD) 0x000000f0

#define     fld_HS_LINE_TOTAL_def       (DWORD) 0x0000038E

#define     fld_MIN_PULSE_WIDTH_def     (DWORD) 0x0000002F
#define     fld_MAX_PULSE_WIDTH_def     (DWORD) 0x00000046

#define     fld_WIN_CLOSE_LIMIT_def     (DWORD) 0x0000004D
#define     fld_WIN_OPEN_LIMIT_def      (DWORD) 0x000001B7

#define     fld_VSYNC_INT_TRIGGER_def   (DWORD) 0x000002AA

#define     fld_VSYNC_INT_HOLD_def      (DWORD) 0x0000001D

#define     fld_VIN_M0_def              (DWORD) 0x00000039
#define     fld_VIN_N0_def              (DWORD) 0x0000014c
#define     fld_MNFLIP_EN_def           (DWORD) 0x00000000
#define     fld_VIN_P_def               (DWORD) 0x00000006
#define     fld_REG_CLK_SEL_def         (DWORD) 0x00000000

#define     fld_VIN_M1_def              (DWORD) 0x00000000
#define     fld_VIN_N1_def              (DWORD) 0x00000000
#define     fld_VIN_DRIVER_SEL_def      (DWORD) 0x00000000
#define     fld_VIN_MNFLIP_REQ_def      (DWORD) 0x00000000
#define     fld_VIN_MNFLIP_DONE_def     (DWORD) 0x00000000
#define     fld_TV_LOCK_TO_VIN_def      (DWORD) 0x00000000
#define     fld_TV_P_FOR_WINCLK_def     (DWORD) 0x00000004

#define     fld_VINRST_def              (DWORD) 0x00000001
#define     fld_VIN_CLK_SEL_def         (DWORD) 0x00000000

#define     fld_VS_FIELD_BLANK_START_def    (DWORD) 0x00000206

#define     fld_VS_FIELD_BLANK_END_def  (DWORD) 0x0000000A

//#define     fld_VS_FIELD_IDLOCATION_def (DWORD) 0x00000105
#define     fld_VS_FIELD_IDLOCATION_def (DWORD) 0x00000001
#define     fld_VS_FRAME_TOTAL_def      (DWORD) 0x00000217

#define     fld_SYNC_TIP_START_def      (DWORD) 0x00000372
#define     fld_SYNC_TIP_LENGTH_def     (DWORD) 0x0000000F

#define     fld_GAIN_FORCE_DATA_def     (DWORD) 0x00000000
#define     fld_GAIN_FORCE_EN_def       (DWORD) 0x00000000
#define     fld_I_CLAMP_SEL_def         (DWORD) 0x00000003
#define     fld_I_AGC_SEL_def           (DWORD) 0x00000001
#define     fld_EXT_CLAMP_CAP_def       (DWORD) 0x00000001
#define     fld_EXT_AGC_CAP_def         (DWORD) 0x00000001
#define     fld_DECI_DITHER_EN_def      (DWORD) 0x00000001
#define     fld_ADC_PREFHI_def          (DWORD) 0x00000000
#define     fld_ADC_CH_GAIN_SEL_def     (DWORD) 0x00000001

#define     fld_HS_PLL_SGAIN_def        (DWORD) 0x00000003

#define     fld_NREn_def                (DWORD) 0x00000000
#define     fld_NRGainCntl_def          (DWORD) 0x00000000
#define     fld_NRBWTresh_def           (DWORD) 0x00000000
#define     fld_NRGCTresh_def           (DWORD) 0x00000000
#define     fld_NRCoefDespeclMode_def   (DWORD) 0x00000000

#define     fld_GPIO_5_OE_def           (DWORD) 0x00000000
#define     fld_GPIO_6_OE_def           (DWORD) 0x00000000

#define     fld_GPIO_5_OUT_def          (DWORD) 0x00000000
#define     fld_GPIO_6_OUT_def          (DWORD) 0x00000000

// End of field default values.


// Register / Field values:
#define     RT_COMP0              0x0
#define     RT_COMP1              0x1
#define     RT_COMP2              0x2
#define     RT_YF_COMP3           0x3
#define     RT_YR_COMP3           0x4
#define     RT_YCF_COMP4          0x5
#define     RT_YCR_COMP4          0x6

// Video standard defines
#define     RT_NTSC           0x0
#define     RT_PAL            0x1
#define     RT_SECAM          0x2
#define     extNONE           0x0000
#define     extNTSC           0x0100
#define     extRsvd           0x0200
#define     extPAL            0x0300
#define     extPAL_M          0x0400
#define     extPAL_N          0x0500
#define     extSECAM          0x0600
#define     extPAL_NCOMB      0x0700
#define     extNTSC_J         0x0800
#define     extNTSC_443       0x0900
#define     extPAL_BGHI       0x0A00
#define     extPAL_60         0x0B00

#define     RT_FREF_2700      6
#define     RT_FREF_2950      5

#define     RT_COMPOSITE      0x0
#define     RT_SVIDEO         0x1

#define     RT_NORM_SHARPNESS 0x03
#define     RT_HIGH_SHARPNESS 0x0F

#define     RT_HUE_PAL_DEF    0x00

#define     RT_DECINTERLACED      0x1
#define     RT_DECNONINTERLACED   0x0

#define     NTSC_LINES          525
#define     PAL_SECAM_LINES     625

#define     RT_ASYNC_ENABLE   0x0
#define     RT_ASYNC_DISABLE  0x1
#define     RT_ASYNC_RESET    0x1

#define     RT_VINRST_ACTIVE  0x0
#define     RT_VINRST_RESET   0x1
#define     RT_L54RST_RESET   0x1

#define     RT_REF_CLK        0x0
#define     RT_PLL_VIN_CLK    0x1

#define     RT_VIN_ASYNC_RST  0x20
#define     RT_DVS_ASYNC_RST  0x80

#define     RT_ADC_ENABLE     0x0
#define     RT_ADC_DISABLE    0x1

#define     RT_DVSDIR_IN      0x0
#define     RT_DVSDIR_OUT     0x1

#define     RT_DVSCLK_HIGH    0x0
#define     RT_DVSCLK_LOW     0x1

#define     RT_DVSCLK_SEL_8FS     0x0
#define     RT_DVSCLK_SEL_27MHZ   0x1

#define     RT_DVS_CONTSTREAM     0x1
#define     RT_DVS_NONCONTSTREAM  0x0

#define     RT_DVSDAT_HIGH    0x0
#define     RT_DVSDAT_LOW     0x1

#define     RT_ADC_CNTL_DEFAULT               0x03252338

// COMB_CNTL0 FILTER SETTINGS FOR DIFFERENT STANDARDS:
#define     RT_NTSCM_COMB_CNTL0_COMPOSITE     0x09438090
#define     RT_NTSCM_COMB_CNTL0_SVIDEO        0x48540000

#define     RT_PAL_COMB_CNTL0_COMPOSITE       0x09438090
#define     RT_PAL_COMB_CNTL0_SVIDEO          0x40348090

#define     RT_SECAM_COMB_CNTL0_COMPOSITE     0xD0088090
#define     RT_SECAM_COMB_CNTL0_SVIDEO        0x50148090

#define     RT_PALN_COMB_CNTL0_COMPOSITE      0x09438090
#define     RT_PALN_COMB_CNTL0_SVIDEO         0x40348090

#define     RT_PALM_COMB_CNTL0_COMPOSITE      0x09438090
#define     RT_PALM_COMB_CNTL0_SVIDEO         0x40348090
// End of filter settings.

// COMB_CNTL1 FILTER SETTINGS FOR DIFFERENT STANDARDS:
#define     RT_NTSCM_COMB_CNTL1_COMPOSITE     0x00000010
#define     RT_NTSCM_COMB_CNTL1_SVIDEO        0x00000081

#define     RT_PAL_COMB_CNTL1_COMPOSITE       0x00000010
#define     RT_PAL_COMB_CNTL1_SVIDEO          0x000000A1

#define     RT_SECAM_COMB_CNTL1_COMPOSITE     0x00000091
#define     RT_SECAM_COMB_CNTL1_SVIDEO        0x00000081

#define     RT_PALN_COMB_CNTL1_COMPOSITE      0x00000010
#define     RT_PALN_COMB_CNTL1_SVIDEO         0x000000A1

#define     RT_PALM_COMB_CNTL1_COMPOSITE      0x00000010
#define     RT_PALM_COMB_CNTL1_SVIDEO         0x000000A1
// End of filter settings.

// COMB_CNTL2 FILTER SETTINGS FOR DIFFERENT STANDARDS:
#define     RT_NTSCM_COMB_CNTL2_COMPOSITE     0x16161010
#define     RT_NTSCM_COMB_CNTL2_SVIDEO        0xFFFFFFFF

#define     RT_PAL_COMB_CNTL2_COMPOSITE       0x16161010
#define     RT_PAL_COMB_CNTL2_SVIDEO          0x06080102

#define     RT_SECAM_COMB_CNTL2_COMPOSITE     0x06080102
#define     RT_SECAM_COMB_CNTL2_SVIDEO        0x06080102

#define     RT_PALN_COMB_CNTL2_COMPOSITE      0x06080102
#define     RT_PALN_COMB_CNTL2_SVIDEO         0x06080102

#define     RT_PALM_COMB_CNTL2_COMPOSITE      0x06080102
#define     RT_PALM_COMB_CNTL2_SVIDEO         0x06080102
// End of filter settings.

// COMB_LINE_LENGTH FILTER SETTINGS FOR DIFFERENT STANDARDS:
#define     RT_NTSCM_COMB_LENGTH_COMPOSITE    0x0718038A
#define     RT_NTSCM_COMB_LENGTH_SVIDEO       0x0718038A

#define     RT_PAL_COMB_LENGTH_COMPOSITE      0x08DA046B
#define     RT_PAL_COMB_LENGTH_SVIDEO         0x08DA046B

#define     RT_SECAM_COMB_LENGTH_COMPOSITE    0x08DA046A
#define     RT_SECAM_COMB_LENGTH_SVIDEO       0x08DA046A

#define     RT_PALN_COMB_LENGTH_COMPOSITE     0x07260391
#define     RT_PALN_COMB_LENGTH_SVIDEO        0x07260391

#define     RT_PALM_COMB_LENGTH_COMPOSITE     0x07160389
#define     RT_PALM_COMB_LENGTH_SVIDEO        0x07160389
// End of filter settings.

// LP_AGC_CLAMP_CNTL0
#define     RT_NTSCM_SYNCTIP_REF0              0x00000037
#define     RT_NTSCM_SYNCTIP_REF1              0x00000029
#define     RT_NTSCM_CLAMP_REF                 0x0000003B
#define     RT_NTSCM_PEAKWHITE                 0x000000FF
#define     RT_NTSCM_VBI_PEAKWHITE             0x000000C2

#define     RT_NTSCM_WPA_THRESHOLD             0x00000406
#define     RT_NTSCM_WPA_TRIGGER_LO            0x000000B3

#define     RT_NTSCM_WPA_TRIGGER_HIGH          0x0000021B

#define     RT_NTSCM_LP_LOCKOUT_START          0x00000206
#define     RT_NTSCM_LP_LOCKOUT_END            0x00000021
#define     RT_NTSCM_CH_DTO_INC                0x00400000
#define     RT_NTSCM_CH_PLL_SGAIN              0x00000001
#define     RT_NTSCM_CH_PLL_FGAIN              0x00000002

#define     RT_NTSCM_CR_BURST_GAIN             0x0000007A
#define     RT_NTSCM_CB_BURST_GAIN             0x000000AC

#define     RT_NTSCM_CH_HEIGHT                 0x000000CD
#define     RT_NTSCM_CH_KILL_LEVEL             0x000000C0
#define     RT_NTSCM_CH_AGC_ERROR_LIM          0x00000002
#define     RT_NTSCM_CH_AGC_FILTER_EN          0x00000000
#define     RT_NTSCM_CH_AGC_LOOP_SPEED         0x00000000

#define     RT_NTSCM_CRDR_ACTIVE_GAIN          0x0000007A
#define     RT_NTSCM_CBDB_ACTIVE_GAIN          0x000000AC

#define     RT_NTSCM_VERT_LOCKOUT_START        0x00000207
#define     RT_NTSCM_VERT_LOCKOUT_END          0x0000000E

#define     RT_NTSCJ_SYNCTIP_REF0              0x00000004
#define     RT_NTSCJ_SYNCTIP_REF1              0x00000012
#define     RT_NTSCJ_CLAMP_REF                 0x0000003B
#define     RT_NTSCJ_PEAKWHITE                 0x000000CB
#define     RT_NTSCJ_VBI_PEAKWHITE             0x000000C2
#define     RT_NTSCJ_WPA_THRESHOLD             0x000004B0
#define     RT_NTSCJ_WPA_TRIGGER_LO            0x000000B4
#define     RT_NTSCJ_WPA_TRIGGER_HIGH          0x0000021C
#define     RT_NTSCJ_LP_LOCKOUT_START          0x00000206
#define     RT_NTSCJ_LP_LOCKOUT_END            0x00000021

#define     RT_NTSCJ_CR_BURST_GAIN             0x00000071
#define     RT_NTSCJ_CB_BURST_GAIN             0x0000009F
#define     RT_NTSCJ_CH_HEIGHT                 0x000000CD
#define     RT_NTSCJ_CH_KILL_LEVEL             0x000000C0
#define     RT_NTSCJ_CH_AGC_ERROR_LIM          0x00000002
#define     RT_NTSCJ_CH_AGC_FILTER_EN          0x00000000
#define     RT_NTSCJ_CH_AGC_LOOP_SPEED         0x00000000

#define     RT_NTSCJ_CRDR_ACTIVE_GAIN          0x00000071
#define     RT_NTSCJ_CBDB_ACTIVE_GAIN          0x0000009F
#define     RT_NTSCJ_VERT_LOCKOUT_START        0x00000207
#define     RT_NTSCJ_VERT_LOCKOUT_END          0x0000000E

#define     RT_PAL_SYNCTIP_REF0                0x00000004
#define     RT_PAL_SYNCTIP_REF1                0x0000000F
#define     RT_PAL_CLAMP_REF                   0x0000003B
#define     RT_PAL_PEAKWHITE                   0x000000C1
#define     RT_PAL_VBI_PEAKWHITE               0x000000C7
#define     RT_PAL_WPA_THRESHOLD               0x000006A4

#define     RT_PAL_WPA_TRIGGER_LO              0x00000096
#define     RT_PAL_WPA_TRIGGER_HIGH            0x000001C2
#define     RT_PAL_LP_LOCKOUT_START            0x00000263
#define     RT_PAL_LP_LOCKOUT_END              0x0000002C

#define     RT_PAL_CH_DTO_INC                  0x00400000
#define     RT_PAL_CH_PLL_SGAIN                0x00000002
#define     RT_PAL_CH_PLL_FGAIN                0x00000001
#define     RT_PAL_CR_BURST_GAIN               0x0000007A
#define     RT_PAL_CB_BURST_GAIN               0x000000AB
#define     RT_PAL_CH_HEIGHT                   0x0000009C
#define     RT_PAL_CH_KILL_LEVEL               0x00000090
#define     RT_PAL_CH_AGC_ERROR_LIM            0x00000002
#define     RT_PAL_CH_AGC_FILTER_EN            0x00000000
#define     RT_PAL_CH_AGC_LOOP_SPEED           0x00000000

#define     RT_PAL_CRDR_ACTIVE_GAIN            0x0000007A
#define     RT_PAL_CBDB_ACTIVE_GAIN            0x000000AB
#define     RT_PAL_VERT_LOCKOUT_START          0x00000269
#define     RT_PAL_VERT_LOCKOUT_END            0x00000012

#define     RT_SECAM_SYNCTIP_REF0              0x00000004
#define     RT_SECAM_SYNCTIP_REF1              0x0000000F
#define     RT_SECAM_CLAMP_REF                 0x0000003B
#define     RT_SECAM_PEAKWHITE                 0x000000C1
#define     RT_SECAM_VBI_PEAKWHITE             0x000000C7
#define     RT_SECAM_WPA_THRESHOLD             0x000006A4

#define     RT_SECAM_WPA_TRIGGER_LO            0x0000026B
#define     RT_SECAM_WPA_TRIGGER_HIGH          0x000001C2
#define     RT_SECAM_LP_LOCKOUT_START          0x0000026B
#define     RT_SECAM_LP_LOCKOUT_END            0x0000002C

#define     RT_SECAM_CH_DTO_INC                0x003E7A28
#define     RT_SECAM_CH_PLL_SGAIN              0x00000006
#define     RT_SECAM_CH_PLL_FGAIN              0x00000006

#define     RT_SECAM_CR_BURST_GAIN             0x00000200
#define     RT_SECAM_CB_BURST_GAIN             0x00000200
#define     RT_SECAM_CH_HEIGHT                 0x00000066
#define     RT_SECAM_CH_KILL_LEVEL             0x00000060
#define     RT_SECAM_CH_AGC_ERROR_LIM          0x00000003
#define     RT_SECAM_CH_AGC_FILTER_EN          0x00000000
#define     RT_SECAM_CH_AGC_LOOP_SPEED         0x00000000

#define     RT_SECAM_CRDR_ACTIVE_GAIN          0x00000200
#define     RT_SECAM_CBDB_ACTIVE_GAIN          0x00000200
#define     RT_SECAM_VERT_LOCKOUT_START        0x00000269
#define     RT_SECAM_VERT_LOCKOUT_END          0x00000012

#define     RT_PAL_VS_FIELD_BLANK_END          0x0000002C
#define     RT_NTSCM_VS_FIELD_BLANK_END        0x0000000A

#define     RT_NTSCM_FIELD_IDLOCATION          0x00000105
#define     RT_PAL_FIELD_IDLOCATION            0x00000137

#define     RT_NTSCM_H_ACTIVE_START            0x00000070
#define     RT_NTSCM_H_ACTIVE_END              0x00000363

#define     RT_PAL_H_ACTIVE_START              0x0000009A
#define     RT_PAL_H_ACTIVE_END                0x00000439

#define     RT_NTSCM_V_ACTIVE_START            ((22-4)*2+1)
#define     RT_NTSCM_V_ACTIVE_END              ((22+240-4)*2+1)

#define     RT_PAL_V_ACTIVE_START              0x00000023  //Same as SECAM
#define     RT_PAL_V_ACTIVE_END                0x00000262

// VBI
#define     RT_NTSCM_H_VBI_WIND_START          0x00000049
#define     RT_NTSCM_H_VBI_WIND_END            0x00000366

#define     RT_PAL_H_VBI_WIND_START            0x00000084
#define     RT_PAL_H_VBI_WIND_END              0x0000041F

#define     RT_NTSCM_V_VBI_WIND_START          fld_V_VBI_WIND_START_def
#define     RT_NTSCM_V_VBI_WIND_END            fld_V_VBI_WIND_END_def

#define     RT_PAL_V_VBI_WIND_START            0x0000000B
#define     RT_PAL_V_VBI_WIND_END              0x00000022

#define     RT_VBI_CAPTURE_EN                  0x00000001  //Enable
#define     RT_VBI_CAPTURE_DIS                 0x00000000  //Disable
#define     RT_RAW_CAPTURE                     0x00000002  //Use raw Video Capture.

#define     RT_NTSCM_VSYNC_INT_TRIGGER         0x2AA
#define     RT_PALSEM_VSYNC_INT_TRIGGER        0x353

#define     RT_NTSCM_VSYNC_INT_HOLD            0x17
#define     RT_PALSEM_VSYNC_INT_HOLD           0x1C

#define     RT_NTSCM_VS_FIELD_BLANK_START      0x206
#define     RT_PALSEM_VS_FIELD_BLANK_START     0x26C

#define     RT_FIELD_FLIP_EN                   0x4
#define     RT_V_FIELD_FLIP_INVERTED           0x2000

#define     RT_NTSCM_H_IN_START                0x70
#define     RT_PAL_H_IN_START                  144
#define     RT_SECAM_H_IN_START                144
#define     RT_NTSC_H_ACTIVE_SIZE              744
#define     RT_PAL_H_ACTIVE_SIZE               927
#define     RT_SECAM_H_ACTIVE_SIZE             927
#define     RT_NTSCM_V_IN_START                (0x23)
#define     RT_PAL_V_IN_START                  (45-6)
#define     RT_SECAM_V_IN_START                (45-6)
#define     RT_NTSCM_V_ACTIVE_SIZE             480
#define     RT_PAL_V_ACTIVE_SIZE               575
#define     RT_SECAM_V_ACTIVE_SIZE             575

#define     RT_NTSCM_WIN_CLOSE_LIMIT           0x4D
#define     RT_NTSCJ_WIN_CLOSE_LIMIT           0x4D
#define     RT_NTSC443_WIN_CLOSE_LIMIT         0x5F
#define     RT_PALM_WIN_CLOSE_LIMIT            0x4D
#define     RT_PALN_WIN_CLOSE_LIMIT            0x5F
#define     RT_SECAM_WIN_CLOSE_LIMIT           0x5F

#define     RT_NTSCM_VS_FIELD_BLANK_START      0x206

#define     RT_NTSCM_HS_PLL_SGAIN              0x5
#define     RT_NTSCM_HS_PLL_FGAIN              0x7

#define     RT_NTSCM_H_OUT_WIND_WIDTH          0x2F4
#define     RT_NTSCM_V_OUT_WIND_HEIGHT         0xF0

#define     TV          0x1
#define     LINEIN      0x2
#define     MUTE        0x3

#define  DEC_COMPOSITE              0
#define  DEC_SVIDEO                 1
#define  DEC_TUNER                  2

#define  DEC_NTSC                   0
#define  DEC_PAL                    1
#define  DEC_SECAM                  2
#define  DEC_NTSC_J                 8

#define  DEC_SMOOTH                 0
#define  DEC_SHARP                  1

#define RT_ATI_ID 0x4D541002

#define FIFOACCESS 1
#define REGACCESS 0

//  VIP Commands
#define VIP_COMMAND_QUERY               0x00000001
#define VIP_COMMAND_OPEN                0x00000002
#define VIP_COMMAND_CLOSE               0x00000003
#define VIP_COMMAND_SYNC_READ           0x00000004
#define VIP_COMMAND_SYNC_WRITE          0x00000005
#define VIP_COMMAND_ASYNC_READ          0x00000006
#define VIP_COMMAND_ASYNC_WRITE         0x00000007
#define VIP_COMMAND_REGISTER_IRQ        0x00000008
#define VIP_COMMAND_CANCEL              0x00000009
#define VIP_COMMAND_GET_STATUS          0x00000010

//  VIP Commands Flags
#define VIP_FLAG_RW_BYTE                0x00000001
#define VIP_FLAG_RW_WORD                0x00000002
#define VIP_FLAG_RW_DWORD               0x00000004
#define VIP_FLAG_RW_BUFFER              0x00000008
#define VIP_FLAG_RW_BUFFER_INDEXED      0x00000010
#define VIP_FLAG_FIFO                   0x00000020
#define VIP_FLAG_REGISTER               0x00000040

//  VIP Status Return Codes
#define VIP_STATUS_SUCCESS              0x00000000
#define VIP_STATUS_COMPLETE             0x00000001
#define VIP_STATUS_CANCELLED            0x00000002
#define VIP_STATUS_PENDING              0x00000003
#define VIP_STATUS_PARTIAL              0x00000006
#define VIP_ERROR                       0x80000000
#define VIP_ERROR_NO_SUCH_DEVICE        ( VIP_ERROR + 0x00000001)
#define VIP_ERROR_INVALID_PARAMETER     ( VIP_ERROR + 0x00000002)
#define VIP_ERROR_BUSY                  ( VIP_ERROR + 0x00000003)
#define VIP_ERROR_QUEUE_FULL            ( VIP_ERROR + 0x00000004)
#define VIP_ERROR_WRONG_COOKIE          ( VIP_ERROR + 0x00000005)
#define VIP_ERROR_TIMEOUT               ( VIP_ERROR + 0x00000006)
#define VIP_ERROR_HARDWARE              ( VIP_ERROR + 0x00000007)
#define VIP_ERROR_NO_ASYNC              ( VIP_ERROR + 0x00000008)
#define VIP_ERROR_POWER_DOWN            ( VIP_ERROR + 0x00000009)

//  VIP Capability Flags
#define VIP_CAPS_ASYNC_COMPLETION       0x00000001

//  Return Values for IRQ Callback Function
#define VIP_IRQ_WAS_NOT_MINE            0x00000000
#define VIP_IRQ_WAS_MINE                0x00000001

//  VIP Provider Version Codes
#define VIP_VERSION_1_1                 0x0101
#define VIP_VERSION_2_0                 0x0200

// Status defines
#define VIP_BUSY 0
#define VIP_IDLE 1
#define VIP_RESET 2

#endif // _DEFINES_H
