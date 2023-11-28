/******************************************************************************
 * defines.h file for Rage 128 Chapter 3 sample code                          *
 *                                                                            *
 * Copyright (c) 1999 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/
 
///////////////////////////////////////////////////////////
//
// FILE: defineR128.h
// DESC: defines for the Rage 128 card, direct from ATI
// 
/////////////////////////////////////////////////////////////


#ifndef _DEFINES_H
#define _DEFINES_H

// Miscellaneous.

#define OFF 0
#define ON  1

#define TRUE 1
#define FALSE 0

// Defines for R128_AdapterInfo.FLAGS
#define R128_USE_BIOS 0x1

// Device ID codes
#define DEVICE_ID_CODE_RE 0x5245
#define DEVICE_ID_CODE_RF 0x5246
#define DEVICE_ID_CODE_RK 0x524B
#define DEVICE_ID_CODE_RL 0x524C

//#define VENDOR_ID_CODE 0x1002
#define ATI_PCI_VENDOR_ID 0x1002

// Generic colour definitions for use in R128_GetColourCode ().

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
#define MEM_CFG_TYPE_MASK 0x3
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

#endif // _DEFINES_H
