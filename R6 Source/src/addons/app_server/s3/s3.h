//*****************************************************************************
//
//	File:		 s3.h
//
//	Description: add-on for graphics cards using s3 chips, header.
//
//	Copyright 1996, Be Incorporated
//
//*****************************************************************************

// Standard glue.
#ifndef _S3_H
#define _S3_H

// Number of different chip set define. This is related to the size of every
// tables, just to be sure you're not forgetting to extend one of them.
#define  CHIP_COUNT     6

// Internal chip set id.
#define  s3_964         0
#define  s3_864         1
#define  s3_trio32      2
#define  s3_trio64      3
#define  s3_trio64vp    4
#define  s3_virge       5

// Temporary id for both trio32 and trio64
#define  s3_trio00      99

// Internal DAC id.
#define  tvp3025_DAC    1
#define  att21c498_DAC  2
#define  s3trio_DAC     3

// Internal resolution id.
#define vga640x480		1		// resolutions
#define vga800x600		2
#define vga1024x768		3
#define vga1152x900     4
#define vga1280x1024    5
#define vga1600x1200    6
#define vga_specific    7

// Macro use to access an specified adress in the io-register space.
#define ISA_ADDRESS(x)  (isa_IO+(x))

// Standard macro.
#ifdef abs
#undef abs
#endif
#define abs(a) (((a)>0)?(a):(-(a)))

// S3 graphic engine registers and masks definition.
// (see any standard s3 databook for more informations).
#define	SUBSYS_STAT		0x42e8
#define	SUBSYS_CNTL		0x42e8
#define	ADVFUNC_CNTL	0x4ae8
#define	CUR_Y			0x82e8
#define	CUR_X			0x86e8
#define	DESTY_AXSTP		0x8ae8
#define	DESTX_DIASTP	0x8ee8
#define	ERR_TERM		0x92e8
#define	MAJ_AXIS_PCNT	0x96e8
#define	GP_STAT			0x9ae8
#define	CMD				0x9ae8
#define	SHORT_STROKE	0x9ee8
#define	BKGD_COLOR		0xa2e8
#define	FRGD_COLOR		0xa6e8
#define	WRT_MASK		0xaae8
#define	RD_MASK			0xaee8
#define	COLOR_CMP		0xb2e8
#define	BKGD_MIX		0xb6e8
#define	FRGD_MIX		0xbae8
#define	RD_REG_DT		0xbee8
#define	PIX_TRANS		0xe2e8
#define	PIX_TRANS_EXT	0xe2ea

#define	DATA_REG		0xbee8
#define	MIN_AXIS_PCNT	0x0000
#define	SCISSORS_T		0x1000
#define	SCISSORS_L		0x2000
#define	SCISSORS_B		0x3000
#define	SCISSORS_R		0x4000
#define	PIX_CNTL		0xa000
#define	MULT_MISC2		0xd000
#define	MULT_MISC		0xe000
#define	READ_SEL		0xf000


// Direct standard VGA registers, special index and data registers
  // with CR55 low bit == 0
#define TI_WRITE_ADDR		0x3C8
#define TI_RAMDAC_DATA		0x3C9
#define TI_PIXEL_MASK		0x3C6
#define TI_READ_ADDR		0x3C7
  // with CR55 low bit == 1
#define TI_INDEX_REG		0x3C6
#define TI_DATA_REG			0x3C7


// TVP 3025 indirect indexed registers and masks definition
// (see TVP 3025 databook for more information).
#define TI_CURS_X_LOW			0x00
#define TI_CURS_X_HIGH			0x01
#define TI_CURS_Y_LOW			0x02
#define TI_CURS_Y_HIGH			0x03
#define TI_SPRITE_ORIGIN_X		0x04
#define TI_SPRITE_ORIGIN_Y		0x05
#define TI_CURS_CONTROL			0x06
#define TI_PLANAR_ACCESS		0x80
#define TI_CURS_SPRITE_ENABLE 	0x40
#define TI_CURS_X_WINDOW_MODE 	0x10
#define TI_CURS_CTRL_MASK     	(TI_CURS_SPRITE_ENABLE | TI_CURS_X_WINDOW_MODE)
#define TI_CURS_RAM_ADDR_LOW	0x08
#define TI_CURS_RAM_ADDR_HIGH	0x09
#define TI_CURS_RAM_DATA		0x0A
#define TI_TRUE_COLOR_CONTROL	0x0E
#define TI_TC_BTMODE			0x04
#define TI_TC_NONVGAMODE		0x02
#define TI_TC_8BIT				0x01
#define TI_VGA_SWITCH_CONTROL	0x0F
#define TI_WINDOW_START_X_LOW	0x10
#define TI_WINDOW_START_X_HIGH	0x11
#define TI_WINDOW_STOP_X_LOW	0x12
#define TI_WINDOW_STOP_X_HIGH	0x13
#define TI_WINDOW_START_Y_LOW	0x14
#define TI_WINDOW_START_Y_HIGH	0x15
#define TI_WINDOW_STOP_Y_LOW	0x16
#define TI_WINDOW_STOP_Y_HIGH	0x17
#define TI_MUX_CONTROL_1		0x18
#define TI_MUX1_PSEUDO_COLOR	0x80
#define TI_MUX1_DIRECT_888		0x06
#define TI_MUX1_DIRECT_565		0x05
#define TI_MUX1_DIRECT_555		0x04
#define TI_MUX1_DIRECT_664		0x03
#define TI_MUX1_WEIRD_MODE_1	0xC6
#define TI_MUX1_WEIRD_MODE_2	0x4D
#define TI_MUX1_WEIRD_MODE_3	0x4E
#define TI_MUX_CONTROL_2		0x19
#define TI_MUX2_BUS_VGA			0x98
#define TI_MUX2_BUS_PC_D8P64	0x1C
#define TI_MUX2_BUS_DC_D24P64	0x1C
#define TI_MUX2_BUS_DC_D16P64	0x04
#define TI_MUX2_BUS_DC_D15P64	0x04
#define TI_MUX2_BUS_TC_D24P64	0x04
#define TI_MUX2_BUS_TC_D16P64	0x04
#define TI_MUX2_BUS_TC_D15P64	0x04
#define TI_MUX2_WEIRD_MODE_2	0x14
#define TI_MUX2_WEIRD_MODE_3	0x04
#define TI_INPUT_CLOCK_SELECT	0x1A
#define TI_ICLK_CLK0			0x00
#define TI_ICLK_CLK0_DOUBLE		0x10
#define TI_ICLK_CLK1			0x01
#define TI_ICLK_CLK1_DOUBLE		0x11
#define TI_ICLK_CLK2			0x02
#define TI_ICLK_CLK2_DOUBLE		0x12
#define TI_ICLK_CLK2_I			0x03
#define TI_ICLK_CLK2_I_DOUBLE	0x13
#define TI_ICLK_CLK2_E			0x04
#define TI_ICLK_CLK2_E_DOUBLE	0x14
#define TI_ICLK_PLL				0x05
#define TI_OUTPUT_CLOCK_SELECT	0x1B
#define TI_OCLK_VGA				0x3E
#define TI_OCLK_S_V1_R8			0x43
#define TI_OCLK_S_V2_R8			0x4B
#define TI_OCLK_S_V4_R8			0x53
#define TI_OCLK_S_V8_R8			0x5B
#define TI_OCLK_S_V2_R4			0x4A
#define TI_OCLK_S_V4_R4			0x52
#define TI_OCLK_S_V1_R2			0x41
#define TI_OCLK_S_V2_R2			0x49
#define TI_OCLK_NS_V1_R1		0x80
#define TI_OCLK_NS_V2_R2		0x89
#define TI_OCLK_NS_V4_R4		0x92
#define TI_PALETTE_PAGE			0x1C
#define TI_GENERAL_CONTROL		0x1D
#define TI_MISC_CONTROL			0x1E
#define TI_MC_POWER_DOWN		0x01
#define TI_MC_DOTCLK_DISABLE	0x02
#define TI_MC_INT_6_8_CONTROL	0x04
#define TI_MC_8_BPP				0x08
#define TI_MC_VCLK_POLARITY		0x20
#define TI_MC_LCLK_LATCH		0x40
#define TI_MC_LOOP_PLL_RCLK		0x80
#define TI_OVERSCAN_COLOR_RED	0x20
#define TI_OVERSCAN_COLOR_GREEN	0x21
#define TI_OVERSCAN_COLOR_BLUE	0x22
#define TI_CURSOR_COLOR_0_RED	0x23
#define TI_CURSOR_COLOR_0_GREEN	0x24
#define TI_CURSOR_COLOR_0_BLUE	0x25
#define TI_CURSOR_COLOR_1_RED	0x26
#define TI_CURSOR_COLOR_1_GREEN	0x27
#define TI_CURSOR_COLOR_1_BLUE	0x28
#define TI_AUXILLARY_CONTROL	0x29
#define TI_AUX_SELF_CLOCK		0x08
#define TI_AUX_W_CMPL			0x01
#define TI_GENERAL_IO_CONTROL	0x2A
#define TI_GIC_ALL_BITS			0x1F
#define TI_GENERAL_IO_DATA		0x2B
#define TI_GID_W2000_6BIT     	0x00
#define TI_GID_N9_964			0x01
#define TI_GID_W2000_8BIT     	0x08
#define TI_GID_S3_DAC_6BIT		0x1C
#define TI_GID_S3_DAC_8BIT		0x1E
#define TI_GID_TI_DAC_6BIT		0x1D
#define TI_GID_TI_DAC_8BIT		0x1F
#define TI_PLL_CONTROL			0x2C
#define TI_PIXEL_CLOCK_PLL_DATA	0x2D
#define TI_PLL_ENABLE			0x08
#define TI_MCLK_PLL_DATA		0x2E
#define TI_LOOP_CLOCK_PLL_DATA	0x2F
#define TI_COLOR_KEY_OLVGA_LOW	0x30
#define TI_COLOR_KEY_OLVGA_HIGH	0x31
#define TI_COLOR_KEY_RED_LOW	0x32
#define TI_COLOR_KEY_RED_HIGH	0x33
#define TI_COLOR_KEY_GREEN_LOW	0x34
#define TI_COLOR_KEY_GREEN_HIGH	0x35
#define TI_COLOR_KEY_BLUE_LOW	0x36
#define TI_COLOR_KEY_BLUE_HIGH	0x37
#define TI_COLOR_KEY_CONTROL	0x38
#define TI_SENSE_TEST			0x3A
#define TI_TEST_DATA			0x3B
#define TI_CRC_LOW				0x3C
#define TI_CRC_HIGH				0x3D
#define TI_CRC_CONTROL			0x3E
#define TI_ID					0x3F
#define TI_VIEWPOINT20_ID		0x20
#define TI_VIEWPOINT25_ID		0x25
#define TI_MODE_85_CONTROL		0xD5
#define TI_RESET				0xFF

// Standard reference frequency used by almost all the graphic card.
#define TI_REF_FREQ		14.318

// internal struct used to clone the add-on from server space to client space
typedef struct {
    int	    theVGA;
    int     theDAC;
    int     theMem;
    int	    scrnRowByte;
    int	    scrnWidth;
    int	    scrnHeight;
    int	    offscrnWidth;
    int	    offscrnHeight;
    int	    scrnPosH;
    int	    scrnPosV;
    int	    scrnColors;
    void    *scrnBase;
    float   scrnRate;
    short   crtPosH;
    short   crtSizeH;
    short   crtPosV;
    short   crtSizeV;
    ulong   scrnResCode;
    int     scrnResNum;
    uchar   *scrnBufBase;
    long	scrnRes;
    ulong   available_spaces;
    int     hotpt_h;
    int     hotpt_v;
    short   lastCrtHT;
    short   lastCrtVT;
    uchar   *isa_IO;
    int     CursorMode;
} clone_info;

#endif




