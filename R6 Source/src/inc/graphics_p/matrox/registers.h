/***************************************************************************
*          name: MGA.H
*
*   description: 
*
*      designed: 
* last modified: $Author: ctoutant $, $Date: 94/10/20 06:54:38 $
*
*       version: $Id: mga.h 1.32 94/10/20 06:54:38 ctoutant Exp $
*
****************************************************************************/



/*****************************************************************************

 ADDRESSING SECTION:

 This section contains all the definitions to access a ressource on the MGA.
 The way to build an address is divided in three parts:
 
    mga_base_address + device_offset + register_offset

    mga_base_address: Where is mapped the MGA in the system,
    device_offset:    The offset of the device (i.e. Titan) in the MGA space,
    register_offset:  The offset of the register in the device space.

 Base addresses and Offsets are in BYTE units as 80x86 physical addresses.

*/

//////////////////////////////////////////////////////////////////////////////
//

#define MGA_ISA_BASE_1        0xac000
#define MGA_ISA_BASE_2        0xc8000
#define MGA_ISA_BASE_3        0xcc000
#define MGA_ISA_BASE_4        0xd0000
#define MGA_ISA_BASE_5        0xd4000
#define MGA_ISA_BASE_6        0xd8000
#define MGA_ISA_BASE_7        0xdc000


#define STORM_OFFSET          0x1c00
#define RAMDAC_OFFSET         0x3c00      /*** BYTE ACCESSES ONLY ***/
#define BES_OFFSET            0x3d00

#define STORM_GO              0x100


//////////////////////////////////////////////////////////////////////////////
// Configuration Space Mapping

#define  STORM_DEVID     0x00
#define  STORM_DEVCTRL   0x04
#define  STORM_CLASS     0x08
#define  STORM_HEADER    0x0c
#define  STORM_MGABASE1  0x10
#define  STORM_MGABASE2  0x14
#define  MYSTI_MGABASE3  0x18
#define  MYSTI_SUBSYSIDR 0x2C
#define  STORM_ROMBASE   0x30
#define  STORM_INTCTL    0x3c
#define  STORM_OPTION    0x40
#define  STORM_MGA_INDEX 0x44
#define  STORM_MGA_DATA  0x48
#define  MYSTI_SUBSYSIDW 0x4C
#define  STORM_OPTION2   0x50


//////////////////////////////////////////////////////////////////////////////
// Drawing Register Map

#define  STORM_DWGCTL    0x00
#define  STORM_MACCESS   0x04
#define  MYSTI_MCTLWTST  0x08
#define  STORM_ZORG      0x0c
#define  STORM_PAT0      0x10
#define  STORM_PAT1      0x14
#define  STORM_PLNWT     0x1c
#define  STORM_BCOL      0x20
#define  STORM_FCOL      0x24
#define  STORM_SRC0      0x30
#define  STORM_SRC1      0x34
#define  STORM_SRC2      0x38
#define  STORM_SRC3      0x3c
#define  STORM_XYSTRT    0x40
#define  STORM_XYEND     0x44
#define  STORM_SHIFT     0x50
#define  STORM_SGN       0x58
#define  STORM_LEN       0x5c
#define  STORM_AR0       0x60
#define  STORM_AR1       0x64
#define  STORM_AR2       0x68
#define  STORM_AR3       0x6c
#define  STORM_AR4       0x70
#define  STORM_AR5       0x74
#define  STORM_AR6       0x78
#define  STORM_CXBNDRY   0x80
#define  STORM_FXBNDRY   0x84
#define  STORM_YDSTLEN   0x88
#define  STORM_PITCH     0x8c
#define  STORM_YDST      0x90
#define  STORM_YDSTORG   0x94
#define  STORM_YTOP      0x98
#define  STORM_YBOT      0x9c
#define  STORM_CXLEFT    0xa0
#define  STORM_CXRIGHT   0xa4
#define  STORM_FXLEFT    0xa8
#define  STORM_FXRIGHT   0xac
#define  STORM_XDST      0xb0
#define  STORM_DR0       0xc0
#define  STORM_DR1       0xc4
#define  STORM_DR2       0xc8
#define  STORM_DR3       0xcc
#define  STORM_DR4       0xd0
#define  STORM_DR5       0xd4
#define  STORM_DR6       0xd8
#define  STORM_DR7       0xdc
#define  STORM_DR8       0xe0
#define  STORM_DR9       0xe4
#define  STORM_DR10      0xe8
#define  STORM_DR11      0xec
#define  STORM_DR12      0xf0
#define  STORM_DR13      0xf4
#define  STORM_DR14      0xf8
#define  STORM_DR15      0xfc


//////////////////////////////////////////////////////////////////////////////
// Host Registers Map

#define  STORM_FIFOSTATUS 0x210
#define  STORM_STATUS     0x214
#define  STORM_ICLEAR     0x218
#define  STORM_IEN        0x21c
#define  STORM_VCOUNT     0x220
#define  STORM_DMAMAP30   0x230
#define  STORM_DMAMAP74   0x234
#define  STORM_DMAMAPB8   0x238
#define  STORM_DMAMAPFC   0x23c
#define  STORM_RST        0x240
#define  STORM_TEST       0x244
#define  STORM_OPMODE     0x254

#define STORM_OPCOD_M                        0x0000000f
#define STORM_OPCOD_A                        0
#define STORM_OPCOD_LINE_OPEN                0x00000000
#define STORM_OPCOD_AUTOLINE_OPEN            0x00000001
#define STORM_OPCOD_LINE_CLOSE               0x00000002
#define STORM_OPCOD_AUTOLINE_CLOSE           0x00000003
#define STORM_OPCOD_TRAP                     0x00000004
#define STORM_OPCOD_BITBLT                   0x00000008
#define STORM_OPCOD_ILOAD                    0x00000009
#define STORM_OPCOD_IDUMP                    0x0000000a

#define STORM_ATYPE_M                        0x00000030
#define STORM_ATYPE_A                        4
#define STORM_ATYPE_RPL                      0x00000000
#define STORM_ATYPE_RSTR                     0x00000010
#define STORM_ATYPE_ZI                       0x00000030
#define STORM_ATYPE_BLK                      0x00000040
#define STORM_ATYPE_I                        0x00000070

#define STORM_LINEAR_M                       0x00000080
#define STORM_LINEAR_A                       7
#define STORM_LINEAR_OFF                     0x00000000
#define STORM_LINEAR_ON                      0x00000080  

#define STORM_ZMODE_M                        0x00000700
#define STORM_ZMODE_A                        8
#define STORM_ZMODE_NOZCMP                   0x00000000
#define STORM_ZMODE_ZE                       0x00000200
#define STORM_ZMODE_ZNE                      0x00000300
#define STORM_ZMODE_ZLT                      0x00000400
#define STORM_ZMODE_ZLTE                     0x00000500
#define STORM_ZMODE_ZGT                      0x00000600
#define STORM_ZMODE_ZGTE                     0x00000700

#define STORM_BOP_M                          0x000f0000
#define STORM_BOP_A                          16
#define STORM_BOP_CLEAR                      0x00000000
#define STORM_BOP_NOT_D_OR_S                 0x00010000
#define STORM_BOP_D_AND_NOTS                 0x00020000
#define STORM_BOP_NOTS                       0x00030000
#define STORM_BOP_NOTD_AND_S                 0x00040000
#define STORM_BOP_NOTD                       0x00050000
#define STORM_BOP_D_XOR_S                    0x00060000
#define STORM_BOP_NOT_D_AND_S                0x00070000
#define STORM_BOP_D_AND_S                    0x00080000
#define STORM_BOP_NOT_D_XOR_S                0x00090000
#define STORM_BOP_D                          0x000a0000
#define STORM_BOP_D_OR_NOTS                  0x000b0000
#define STORM_BOP_S                          0x000c0000
#define STORM_BOP_NOTD_OR_S                  0x000d0000
#define STORM_BOP_D_OR_S                     0x000e0000
#define STORM_BOP_SET                        0x000f0000

#define STORM_SGNZERO_M                      0x00002000
#define STORM_SGNZERO_A                      13
#define STORM_SGNZERO_ON                     0x00002000
#define STORM_SGNZERO_OFF                    0x00000000

#define STORM_SHFTZERO_M                     0x00004000
#define STORM_SHFTZERO_A                     14
#define STORM_SHFTZERO_ON                    0x00004000
#define STORM_SHFTZERO_OFF                   0x00000000
#define STORM_ARZERO_M                       0x00001000
#define STORM_ARZERO_A                       12
#define STORM_ARZERO_ON                      0x00001000
#define STORM_ARZERO_OFF                     0x00000000

#define STORM_SOLID_M                        0x00000800
#define STORM_SOLID_A                        11
#define STORM_SOLID_ON                       0x00000800
#define STORM_SOLID_OFF                      0x00000000

#define STORM_TRANS_M                        0x00f00000
#define STORM_TRANS_A                        20

#define STORM_TRANSC_M                       0x40000000   
#define STORM_TRANSC_A                       30
#define STORM_TRANSC_OPAQUE                  0x00000000
#define STORM_TRANSC_TRANSPARENT             0x40000000

#define STORM_PATTERN_M                      0x20000000
#define STORM_PATTERN_A                      29
#define STORM_PATTERN_OFF                    0x00000000
#define STORM_PATTERN_ON                     0x20000000

#define STORM_FUNCNT_M                       0x0000007f
#define STORM_FUNCNT_A                       0
#define STORM_X_OFF_M                        0x0000000f
#define STORM_X_OFF_A                        0
#define STORM_Y_OFF_M                        0x00000070
#define STORM_Y_OFF_A                        4
#define STORM_FUNOFF_M                       0x003f0000
#define STORM_FUNOFF_A                       16

#define STORM_SDYDXL_M                       0x00000001
#define STORM_SDYDXL_A                       0
#define STORM_SDYDXL_Y_MAJOR                 0x00000000
#define STORM_SDYDXL_X_MAJOR                 0x00000001

#define STORM_SCANLEFT_M                     0x00000001
#define STORM_SCANLEFT_A                     0
#define STORM_SCANLEFT_OFF                   0x00000000
#define STORM_SCANLEFT_ON                    0x00000001

#define STORM_SDXL_M                         0x00000002
#define STORM_SDXL_A                         1
#define STORM_SDXL_POS                       0x00000000
#define STORM_SDXL_NEG                       0x00000002

#define STORM_SDY_M                          0x00000004
#define STORM_SDY_A                          2
#define STORM_SDY_POS                        0x00000000
#define STORM_SDY_NEG                        0x00000004

#define STORM_SDXR_M                         0x00000020
#define STORM_SDXR_A                         5
#define STORM_SDXR_POS                       0x00000000
#define STORM_SDXR_NEG                       0x00000020

#define STORM_DWGENGSTS_M                    0x00010000
#define STORM_DWGENGSTS_A                    16
#define STORM_DWGENGSTS_BUSY                 0x00010000
#define STORM_DWGENGSTS_IDLE                 0x00000000

#define STORM_YLIN_M                         0x00008000
#define STORM_YLIN_A                         15

#define STORM_AR0_M                          0x0003ffff
 
#define STORM_DMAACT_M    0x00000002
#define STORM_DMAACT_A    1

#define STORM_VSYNCSTS_M                     0x00000008
#define STORM_VSYNCSTS_A                     3
#define STORM_VSYNCSTS_SET                   0x00000008
#define STORM_VSYNCSTS_CLR                   0x00000000

#define STORM_BLTMOD_BMONOLEF                0x00000000
#define STORM_BLTMOD_BPLAN                   0x02000000
#define STORM_BLTMOD_BFCOL                   0x04000000
#define STORM_BLTMOD_BU32BGR                 0x06000000
#define STORM_BLTMOD_BMONOWF                 0x08000000
#define STORM_BLTMOD_BU32RGB                 0x0E000000
#define STORM_BLTMOD_BU24BGR                 0x16000000
#define STORM_BLTMOD_BUYUV                   0x1C000000
#define STORM_BLTMOD_BU24RGB                 0x1E000000

#define STORM_CONFIG          0x250
#define STORM_REV             0x248
#define STORM_CHIPREV_M                      0x0000000f
#define STORM_CHIPREV_A                      0

#define STORM_PCI_M                          0x08000000
#define STORM_ISA_M                          0x10000000
#define STORM_ISA_WIDE_BUS                   0x00000000

#define STORM_DMAMOD_M                       0x0000000c
#define STORM_DMAMOD_GENERAL_WR              0x00000000
#define STORM_DMAMOD_BLIT_WR                 0x00000004
#define STORM_DMAMOD_BLIT_RD                 0x0000000c

#define PSEUDO_DMA_WIN_OFFSET 0x0000

#define STORM_PSEUDODMA_M                    0x00000001
#define STORM_PSEUDODMA_A                    0

#define STORM_ZMSK            0x018


//////////////////////////////////////////////////////////////////////////////
// VGA REGISTERS

#define VGA_CRTC_INDEX    0x3d4
#define VGA_CRTC_DATA     0x3d5
#define VGA_CRTC0      0x00
#define VGA_CRTC1      0x01
#define VGA_CRTC2      0x02
#define VGA_CRTC3      0x03
#define VGA_CRTC4      0x04
#define VGA_CRTC5      0x05
#define VGA_CRTC6      0x06
#define VGA_CRTC7      0x07
#define VGA_CRTC8      0x08
#define VGA_CRTC9      0x09
#define VGA_CRTCA      0x0a
#define VGA_CRTCB      0x0b
#define VGA_CRTCC      0x0c
#define VGA_CRTCD      0x0d
#define VGA_CRTCE      0x0e
#define VGA_CRTCF      0x0f
#define VGA_CRTC10     0x10
#define VGA_CRTC11     0x11
#define VGA_CRTC12     0x12
#define VGA_CRTC13     0x13
#define VGA_CRTC14     0x14
#define VGA_CRTC15     0x15
#define VGA_CRTC16     0x16
#define VGA_CRTC17     0x17
#define VGA_CRTC18     0x18
#define VGA_CRTC22     0x22
#define VGA_CRTC24     0x24
#define VGA_CRTC26     0x26


#define VGA_ATTR_INDEX   0x3c0
#define VGA_ATTR_DATA    0x3c1
#define VGA_ATTR0      0x00
#define VGA_ATTR1      0x01
#define VGA_ATTR2      0x02
#define VGA_ATTR3      0x03
#define VGA_ATTR4      0x04
#define VGA_ATTR5      0x05
#define VGA_ATTR6      0x06
#define VGA_ATTR7      0x07
#define VGA_ATTR8      0x08
#define VGA_ATTR9      0x09
#define VGA_ATTRA      0x0a
#define VGA_ATTRB      0x0b
#define VGA_ATTRC      0x0c
#define VGA_ATTRD      0x0d
#define VGA_ATTRE      0x0e
#define VGA_ATTRF      0x0f
#define VGA_ATTR10     0x10
#define VGA_ATTR11     0x11
#define VGA_ATTR12     0x12
#define VGA_ATTR13     0x13
#define VGA_ATTR14     0x14
                    
#define VGA_MISC_W     0x3c2
#define VGA_MISC_R     0x3cc

#define VGA_INST0      0x3c2

#define VGA_SEQ_INDEX       0x3c4
#define VGA_SEQ_DATA        0x3c5
#define VGA_SEQ0       0x00
#define VGA_SEQ1       0x01
#define VGA_SEQ2       0x02
#define VGA_SEQ3       0x03
#define VGA_SEQ4       0x04

#define VGA_DACSTAT    0x3c7

#define VGA_GCTL_INDEX      0x3ce
#define VGA_GCTL_DATA       0x3cf
#define VGA_GCTL0      0x00
#define VGA_GCTL1      0x01
#define VGA_GCTL2      0x02
#define VGA_GCTL3      0x03
#define VGA_GCTL4      0x04
#define VGA_GCTL5      0x05
#define VGA_GCTL6      0x06
#define VGA_GCTL7      0x07
#define VGA_GCTL8      0x08


#define VGA_INSTS1     0x3da
#define VGA_FEAT       0x3da

#define VGA_CRTCEXT_INDEX    0x3de
#define VGA_CRTCEXT_DATA     0x3df
#define VGA_CRTCEXT0   0x00
#define VGA_CRTCEXT1   0x01
#define VGA_CRTCEXT2   0x02
#define VGA_CRTCEXT3   0x03
#define VGA_CRTCEXT4   0x04
#define VGA_CRTCEXT5   0x05


//////////////////////////////////////////////////////////////////////////////
// STORM Drawing Engine field masks and values as per Storm spec. 1.0
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// STORM_DEVID

#define STORM_VENDORID_M       0x0000ffff
#define STORM_VENDORID_A       0
#define STORM_DEVICEID_M       0xffff0000
#define STORM_DEVICEID_A       16


//////////////////////////////////////////////////////////////////////////////
// STORM_DEVCTRL

#define STORM_IOSPACE_M        0x00000001
#define STORM_IOSPACE_A        0
#define STORM_MEMSPACE_M       0x00000002
#define STORM_MEMSPACE_A       1
#define STORM_BUSMASTER_M      0x00000004
#define STORM_BUSMASTER_A      2


//////////////////////////////////////////////////////////////////////////////
// STORM_CLASS

#define STORM_REVISION_M       0x000000ff
#define STORM_REVISION_A       0
#define STORM_CLASS_M          0xffffff00
#define STORM_CLASS_A          8


//////////////////////////////////////////////////////////////////////////////
// STORM_MGABASE1

#define STORM_MEM_SPACE1_M       0x00000001
#define STORM_MEM_SPACE1_A       0
#define STORM_APER_TYPE1_M       0x00000006
#define STORM_APER_TYPE1_A       1
#define STORM_PREFETCH1_M        0x00000008
#define STORM_PREFETCH1_A        3
#define STORM_MGABASE1_M         0xffffc000
#define STORM_MGABASE1_A         14


//////////////////////////////////////////////////////////////////////////////
// STORM_MGABASE2

#define STORM_MEM_SPACE2_M       0x00000001
#define STORM_MEM_SPACE2_A       0
#define STORM_APER_TYPE2_M       0x00000006
#define STORM_APER_TYPE2_A       1
#define STORM_PREFETCH2_M        0x00000008
#define STORM_PREFETCH2_A        3
#define STORM_MGABASE2_M         0xff800000
#define STORM_MGABASE2_A         23


//////////////////////////////////////////////////////////////////////////////
// STORM_ROMEN

#define STORM_ROMEN_M            0x00000001
#define STORM_ROMEN_A            0
#define STORM_ROMBASE_M          0xffff0000
#define STORM_ROMBASE_A          16


//////////////////////////////////////////////////////////////////////////////
// STORM_INTCTL

#define STORM_INTLINE_M          0x000000ff
#define STORM_INTLINE_A          0
#define STORM_INTPIN_M           0x0000ff00
#define STORM_INTPIN_A           8


//////////////////////////////////////////////////////////////////////////////
// STORM_OPTION

#define STORM_VGAIOEN_M          0x00000100
#define STORM_VGAIOEN_A          8
#define STORM_INTERLEAVE_M       0x00001000
#define STORM_INTERLEAVE_A       12
#define STORM_RFHCNT_M           0x000f0000
#define STORM_RFHCNT_A           16
#define STORM_EEPROMWT_M         0x00100000
#define STORM_EEPROMWT_A         20
#define STORM_GSCALE_M           0x00200000
#define STORM_GSCALE_A           21
#define STORM_PRODUCTID_M        0x1f000000
#define STORM_PRODUCTID_A        24
#define STORM_POWERPC_M          0x80000000
#define STORM_POWERPC_A          31


//////////////////////////////////////////////////////////////////////////////
// STORM_MACCESS

#define STORM_PWIDTH_M          0x00000003
#define STORM_PWIDTH_A          0
#define STORM_DIT555_M          0x80000000
#define STORM_DIT555_A          31


//////////////////////////////////////////////////////////////////////////////
//

#define STORM_SOFTRESET_M                    0x00000001
#define STORM_SOFTRESET_A                    0
#define STORM_SOFTRESET_SET                  0x00000001
#define STORM_SOFTRESET_CLR                  0x00000000


//////////////////////////////////////////////////////////////////////////////
// TVP3026 Register Direct

#define TVP3026_INDEX         0x00
#define TVP3026_WADR_PAL      0x00
#define TVP3026_COL_PAL       0x01
#define TVP3026_PIX_RD_MSK    0x02
#define TVP3026_RADR_PAL      0x03
#define TVP3026_CUR_COL_ADDR  0x04
#define TVP3026_CUR_COL_DATA  0x05

#define TVP3026_DATA          0x0a
#define TVP3026_CUR_RAM       0x0b
#define TVP3026_CUR_XLOW      0x0c
#define TVP3026_CUR_XHI       0x0d
#define TVP3026_CUR_YLOW      0x0e
#define TVP3026_CUR_YHI       0x0f


//////////////////////////////////////////////////////////////////////////////
// TVP3026 Register Indirect

#define TVP3026_SILICON_REV    0x01
#define TVP3026_CURSOR_CTL     0x06
#define TVP3026_LATCH_CTL      0x0f
#define TVP3026_TRUE_COLOR_CTL 0x18
#define TVP3026_MUX_CTL        0x19
#define TVP3026_CLK_SEL        0x1a
#define TVP3026_PAL_PAGE       0x1c
#define TVP3026_GEN_CTL        0x1d
#define TVP3026_MISC_CTL       0x1e
#define TVP3026_GEN_IO_CTL     0x2a
#define TVP3026_GEN_IO_DATA    0x2b
#define TVP3026_PLL_ADDR       0x2c
#define TVP3026_PIX_CLK_DATA   0x2d
#define TVP3026_MEM_CLK_DATA   0x2e
#define TVP3026_LOAD_CLK_DATA  0x2f

#define TVP3026_KEY_RED_LOW    0x32
#define TVP3026_KEY_RED_HI     0x33
#define TVP3026_KEY_GREEN_LOW  0x34
#define TVP3026_KEY_GREEN_HI   0x35
#define TVP3026_KEY_BLUE_LOW   0x36
#define TVP3026_KEY_BLUE_HI    0x37
#define TVP3026_KEY_CTL        0x38
#define TVP3026_MCLK_CTL       0x39
#define TVP3026_SENSE_TEST     0x3a
#define TVP3026_TEST_DATA      0x3b
#define TVP3026_CRC_LSB        0x3c
#define TVP3026_CRC_MSB        0x3d
#define TVP3026_CRC_CTL        0x3e
#define TVP3026_ID             0x3f
#define TVP3026_RESET          0xff


//////////////////////////////////////////////////////////////////////////////
// TVP3030 Specific Register Indirect

#define TVP3030_ROUTER_CTL     0x07


//////////////////////////////////////////////////////////////////////////////
// Mystique Integrated DAC: Direct Registers

#define MID_INDEX         0x00
#define MID_PALWTADD      0x00
#define MID_PALDATA       0x01
#define MID_PIXRDMSK      0x02
#define MID_PALRDADD      0x03
#define MID_X_DATAREG     0x0a
#define MID_CURPOSXL      0x0c
#define MID_CURPOSXH      0x0d
#define MID_CURPOSYL      0x0e
#define MID_CURPOSYH      0x0f


//////////////////////////////////////////////////////////////////////////////
// Mystique Integrated DAC: Indirect Registers

#define MID_XCURADDL      0x04
#define MID_XCURADDH      0x05
#define MID_XCURCTRL      0x06
#define MID_XCURCOL0RED   0x08
#define MID_XCURCOL0GREEN 0x09
#define MID_XCURCOL0BLUE  0x0a
#define MID_XCURCOL1RED   0x0c
#define MID_XCURCOL1GREEN 0x0d
#define MID_XCURCOL1BLUE  0x0e
#define MID_XCURCOL2RED   0x10
#define MID_XCURCOL2GREEN 0x11
#define MID_XCURCOL2BLUE  0x12
#define MID_XVREFCTRL     0x18
#define MID_XMULCTRL      0x19
#define MID_XPIXCLKCTRL   0x1a
#define MID_XGENCTRL      0x1d
#define MID_XMISCCTRL     0x1e
#define MID_XGENIOCTRL    0x2a
#define MID_XGENIODATA    0x2b
#define MID_XSYSPLLM      0x2c
#define MID_XSYSPLLN      0x2d
#define MID_XSYSPLLP      0x2e
#define MID_XSYSPLLSTAT   0x2f
#define MID_XZOOMCTRL     0x38
#define MID_XSENSETEST    0x3a
#define MID_XCOLKEYMSKL   0x40
#define MID_XCOLKEYMSKH   0x41
#define MID_XCOLKEYL      0x42
#define MID_XCOLKEYH      0x43
#define MID_XPIXPLLAM     0x44
#define MID_XPIXPLLAN     0x45
#define MID_XPIXPLLAP     0x46
#define MID_XPIXPLLBM     0x48
#define MID_XPIXPLLBN     0x49
#define MID_XPIXPLLBP     0x4a
#define MID_XPIXPLLCM     0x4c
#define MID_XPIXPLLCN     0x4d
#define MID_XPIXPLLCP     0x4e
#define MID_XPIXPLLSTAT   0x4f
#define MID_XKEYOPMODE    0x51
#define MID_XCOLMSK0RED   0x52
#define MID_XCOLMSK0GREEN 0x53
#define MID_XCOLMSK0BLUE  0x54
#define MID_XCOLKEY0RED   0x55
#define MID_XCOLKEY0GREEN 0x56
#define MID_XCOLKEY0BLUE  0x57

//////////////////////////////////////////////////////////////////////////////
//
#define BES_A1ORG         0x00
#define BES_A2ORG         0x04
#define BES_B1ORG         0x08
#define BES_B2ORG         0x0C
#define BES_A1CORG        0x10
#define BES_A2CORG        0x14
#define BES_B1CORG        0x18
#define BES_B2CORG        0x1c
#define BES_CTL           0x20
#define BES_PITCH         0x24
#define BES_HCOORD        0x28
#define BES_VCOORD        0x2c
#define BES_HISCAL        0x30
#define BES_VISCAL        0x34
#define BES_HSRCST        0x38
#define BES_HSRCEND       0x3c
#define BES_V1WGHT        0x48
#define BES_V2WGHT        0x4c
#define BES_HSRCLST       0x50
#define BES_V1SRCLST      0x54
#define BES_V2SRCLST      0x58
#define BES_GLOBCTL       0xc0
#define BES_STATUS        0xc4


//////////////////////////////////////////////////////////////////////////////
//

#define STORM_PWIDTH_M                       0x00000003
#define STORM_PWIDTH_A                       0
#define STORM_PWIDTH_PW8                     0x00000000
#define STORM_PWIDTH_PW16                    0x00000001
// PACK PIXEL
#define STORM_PWIDTH_PW24                    0x00000003
#define STORM_PWIDTH_PW32                    0x00000002


//////////////////////////////////////////////////////////////////////////////
// Register Access Pseudofunctions
//////////////////////////////////////////////////////////////////////////////

#define MGA8(a) *((vuchar *)(regs + (a)))
#define MGA32(a) *((vulong *)(regs + (a)))
#define MGA32W(a, d) MGA32(a) = (d)
#define MGA32R(a, d) (d) = MGA32(a)

#define DAC8(a) MGA8(RAMDAC_OFFSET + (a))
#define DAC8R(a, d) (d) = DAC8(a)
#define DAC8W(a, d) DAC8(a) = (d)
#define DAC8IW(a, d) DAC8W(TVP3026_INDEX, (a)); DAC8W(TVP3026_DATA, (d))
#define DAC8IR(a, d) DAC8W(TVP3026_INDEX, (a)); DAC8R(TVP3026_DATA, (d))
#define DAC8POLL(a, d, m) while ((DAC8(a) & (m)) != ((d) & (m)))

#define STORM8(a) MGA8(STORM_OFFSET + (a))
#define STORM8R(a, d) (d) = STORM8(a)
#define STORM8W(a, d) STORM8(a) = (d)
#define STORM8POLL(a, d, m) while ((STORM8(a) & (m)) != ((d) & (m)))

#define STORM32(a) MGA32(STORM_OFFSET + (a))
#define STORM32R(a, d) (d) = STORM32(a)
#define STORM32W(a, d) STORM32(a) = (d)
#define STORM32POLL(a, d, m) while ((STORM32(a) & (m)) != ((d) & (m)))

#define BES32(a) MGA32(BES_OFFSET + (a))
#define BES32R(a, d) (d) = BES32(a)
#define BES32W(a, d) BES32(a) = (d)

//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
