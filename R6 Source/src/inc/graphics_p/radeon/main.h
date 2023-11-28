/******************************************************************************
 * Header file for Radeon ample code                                          *
 * Includes structures, prototypes, and globals.                              *
 *                                                                            *
 * Copyright (c) 2000 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/
#include <stdio.h>
#include <OS.h>

#ifndef _MAIN_H
#define _MAIN_H

// Structures.

typedef struct
{
    uint16 PCI_Vendor_ID;
    uint16 PCI_Device_ID;
    uint32 memsize;
    uint8 bus_num;
    uint8 dev_num;
    uint8 FLAGS;
    uint8 PCI_Revision_ID;
    uint32 MEM_BASE;
    uint32 virtual_MEM_BASE;
    uint32 IO_BASE;
    uint32 REG_BASE;
    uint32 virtual_REG_BASE;
    uint32 BIOS_SEG;
    uint16 xres;
    uint16 yres;
    uint8 bpp;
    uint8 bytepp;
    uint16 pitch;

} AdapterInfo;

typedef struct
{
    uint16 video_mode;
    uint16 refresh_bit_mask;
    uint16 flags;
    uint16 crtc_h_disp__h_total;
    uint16 crtc_h_sync_wid__strt;
    uint16 crtc_v_total;
    uint16 crtc_v_disp;
    uint16 crtc_v_sync_strt;
    uint16 crtc_h_disp__v_sync_width;
    uint16 dot_clock;
    uint16 crtc_h_sync_dly__ovr_wid_right__left;
    uint16 ovr_wid_top__bottom;
    uint16 ovr_clr_g__b;
    uint16 ovr_clr_r;
} CRTParameterTable;

typedef struct
{
    uint16 red;
    uint16 green;
    uint16 blue;
} palette;

typedef struct
{
    uint16 fb_div;
    uint16 post_div;
} pllinfo;

// Structs for nbmode.c
typedef struct
{
    uint16 offset;
    uint32 value;
} offset_value_struct;

typedef struct
{
    uint16 HorizTotal;
    uint16 HorizSyncStart;
    uint16 HorizSyncEnd;
    uint16 VertTotal;
    uint16 VertSyncStart;
    uint16 VertSyncEnd;
    uint16 CRTCInfoFlags;
    uint16 PixelClock;
} CRTCInfoBlock;

#pragma pack(1)
typedef struct
{
    uint8 clock_chip_type;
    uint8 struct_size;
    uint8 acclerator_entry;
    uint8 VGA_entry;
    uint16 VGA_table_offset;
    uint16 POST_table_offset;
    uint16 XCLK;
    uint16 MCLK;
    uint8 num_PLL_blocks;
    uint8 size_PLL_blocks;
    uint16 PCLK_ref_freq;
    uint16 PCLK_ref_divider;
    uint32 PCLK_min_freq;
    uint32 PCLK_max_freq;
    uint16 MCLK_ref_freq;
    uint16 MCLK_ref_divider;
    uint32 MCLK_min_freq;
    uint32 MCLK_max_freq;
    uint16 XCLK_ref_freq;
    uint16 XCLK_ref_divider;
    uint32 XCLK_min_freq;
    uint32 XCLK_max_freq;
} _PLL_BLOCK;


typedef struct
{
    uint16 xres;
    uint16 yres;
    uint16 pclock;
    uint16 H_nonvisible;
    uint16 H_overplus;
    uint16 H_syncwidth;
    uint16 V_nonvisible;
    uint16 V_overplus;
    uint16 V_syncwidth;
} panelinf;

#pragma pack()

// Prototypes.

#if 0
// Functions in RDEONLIB.C
void Radeon_ShutDown (void);
void process_command_line (int argc, char *argv[]);
uint16 get_old_mode (void);
uint16 set_old_mode (void);
uint32 phys_to_virt (uint32 physical, uint32 size);
void Radeon_PrintInfoStruct (void);
void Radeon_PrintInfoStruct2File (void);
uint8 Radeon_Detect (void);
//uint8 Radeon_FindRom (void);
//uint8 Radeon_RegTest (void);
uint32 Radeon_GetBPPValue (uint16 bpp);
uint16 Radeon_GetBPPFromValue (uint32 value);
void Radeon_Delay (uint16 ticks);

// Functions in REGRDWRT.C
uint32 regr (uint16);
uint16 regr16 (uint16);
uint8 regr8 (uint16);
uint32 PLL_regr (uint16);
uint16 PLL_regr16 (uint16);
uint8 PLL_regr8 (uint16);
void regw (uint16 regindex, uint32 data);
void regw16 (uint16 regindex, uint16 data);
void regw8 (uint16 regindex, uint8 data);
void PLL_regw (uint16 regindex, uint32 data);
void PLL_regw16 (uint16 regindex, uint16 data);
void PLL_regw8 (uint16 regindex, uint8 data);
void iow8 (uint32, uint8);
uint8 ior8 (uint32);

// Functions in setmode.c
uint8 Radeon_SetMode (uint16 xres, uint16 yres, uint8 bpp);

// Functions in initeng.c
void Radeon_InitEngine (void);
void Radeon_WaitForIdle (void);
void Radeon_WaitForFifo (uint32 entries);
void Radeon_ResetEngine (void);
void Radeon_FlushPixelCache (void);


// Functions in PCICFG.C
uint16 PCIInstallCheck (void);
uint8 GetPCIByte (uint16 bus, uint16 dev, uint16 reg);
uint16 GetPCIWord (uint16 bus, uint16 dev, uint16 reg);
uint32 GetPCIDword (uint16 bus, uint16 dev, uint16 reg);
void SetPCIByte (uint16 bus, uint16 dev, uint16 reg, uint8 data);
void SetPCIWord (uint16 bus, uint16 dev, uint16 reg, uint16 data);
void SetPCIDword (uint16 bus, uint16 dev, uint16 reg, uint32 data);

// functions in DPMIMEM.C
uint32 DPMI_allocdosmem (uint32 paragraphs, uint32 *segment, uint32 *selector);
uint32 DPMI_freedosmem (uint32 selector);
uint32 DPMI_allocatelogicalregion (uint32, uint32);
uint16 DPMI_freelogicalregion (uint32);
uint16 DPMI_allocatememory (uint32, uint32 *, uint32 *);
uint16 DPMI_freememory (uint32);

// Functions in STARTUP.C
void Radeon_StartUp ( int argc, char *argv[]);

// Functions in COLOUR.C
uint32 Radeon_GetColourCode (uint16 );

// Functions in PALETTE.C
void Radeon_SetPaletteEntry (uint16, palette);
palette Radeon_GetPaletteEntry (uint16);
void Radeon_SavePalette (palette *);
void Radeon_RestorePalette (palette *);
void Radeon_InitPalette (void);
void Radeon_InitGamma (void);

// functions in PLL.C
uint32 RoundDiv (uint32 Numerator, uint32 Denominator);
uint32 Minimum (uint32 Value1, uint32 Value2);
uint32 Maximum (uint32 Value1, uint32 Value2);
uint32 MinBitsRequired (uint32 Value);
uint32 Radeon_VClockValue (void);
uint8 Radeon_ProgramDDAFifo (uint32 BitsPerPixel);
uint8 GetPostDividerBitValue (uint8 PostDivider);
void Radeon_PLLWriteUpdate (void);
uint8 Radeon_PLLReadUpdateComplete (void);
void Radeon_ProgramPLL (void);
void Radeon_PLLGetDividers (uint16 Frequency);
void Radeon_DisableAtomicUpdate (void);

// Functions in NBMODE.C
void set_common_regs (void);
void video_off (void);
void video_on (void);
uint8 Radeon_SetModeNB (uint16, uint16, uint16, CRTCInfoBlock *);
void Radeon_GetPLLInfo (void);

// Functions in DPMS.C
void Radeon_SetDPMS (uint16);
uint16 Radeon_GetDPMS (void);

#endif

// Globals.
extern AdapterInfo Radeon_AdapterInfo;
extern uint32 OLD_MODE;
extern pllinfo PLL_INFO;
extern _PLL_BLOCK PLL_BLOCK;
extern panelinf P;

// Globals for NOBIOS mode setting
extern CRTCInfoBlock mode320_60, mode640_60, mode800_60, mode1024_60, mode1280_60;
extern CRTCInfoBlock mode512, mode1600_60, mode320_75, mode864_60, mode848_88;
extern CRTCInfoBlock mode1152_60, mode1920_60, mode400_75, mode720_60;
extern CRTCInfoBlock mode1280_75;
extern offset_value_struct common_regs_table[12];

typedef struct
{
    uint16 x;
    uint16 y;
    uint16 h;
    uint16 w;
    uint16 frgd;
    uint16 bkgd;
} blt_data;

typedef struct
{
    uint16 x;
    uint16 y;
    uint16 buttons;
} mouse_pos;

typedef struct
{
    uint16 x;
    uint16 y;
} cursor_pos;

typedef struct
{
    uint16 width;
    uint16 height;
    uint32 cur_offset;
    uint8 cursorxor[512];
    uint8 cursorand[512];
} hwcursor;

typedef struct
{
    uint16 width;
    uint16 height;
    uint16 bpp;
    uint16 bytepp;
    uint32 size;
    char name [256];
    char * location;
} _img_info;

// Structures for VIEWIMG.C
#pragma pack(1)

typedef struct                      /* VT image file header structure */
{
    char     img_code[5];           // set to "_IMG" terminated with 0
    short    format_id;             // see FORMAT_TYPE_???? defines
    short    bytes_per_pixel;       // bytes per pixel (per format)
    short    width;                 // width of image in pixels
    short    height;                // height of image in lines
    unsigned long u_offset;         // byte offset to start of U data for
                                    //   YUV9 and YUV12 formats
    unsigned long v_offset;         // byte offset to start of V data for
                                    //   YUV9 and YUV12 formats
    char     filler[11];            // set to 0 (sets header to 32 bytes);
                                    //   regular data and Y data follows
                                    //   header
} img_header;

typedef struct
{
    char *filespec;
    img_header *image;
} img_handle;

typedef struct
{
    uint16 width;
    uint16 height;
    uint16 actual_width;
    char *data;
} _TEXT_INFO;

typedef struct
{
    uint16 src_cmp_fcn;
    uint16 dst_cmp_fcn;
    uint16 cmp_src;
    uint16 src_clr;
    uint16 dst_clr;
    uint16 dst_x;
    uint16 dst_y;
    uint16 src_x;
    uint16 src_y;
    uint16 src_width;
    uint16 src_height;
} _tbltdata;

// functions in viewimg.c
img_handle *get_img_header (char *);
uint16 load_img (img_handle *, uint16, uint16);


// Globals
extern _TEXT_INFO TEXT_INFO;
extern img_handle *ReturnPtr;

#endif // _MAIN_H
