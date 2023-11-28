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
#ifndef _VIRGE_H
#define _VIRGE_H

#include <PCI.h>

// Internal resolution id.
#define vga640x480		1		// resolutions
#define vga800x600		2
#define vga1024x768		3
#define vga1152x900     4
#define vga1280x1024    5
#define vga1600x1200    6
#define vga_specific    7

// Standard macro.
#ifdef abs
#undef abs
#endif
#define abs(a) (((a)>0)?(a):(-(a)))

// S3 graphic engine registers and masks definition.
// (see any standard s3 databook for more informations).
#define MM8180			0x8180
#define MM81C0			0x81c0
#define MM81C4			0x81c4
#define MM81C8			0x81c8
#define MM81F0			0x81f0
#define MM81F4			0x81f4
#define MM8200			0x8200

#define	SUBSYS_STAT		0x8504
#define	SUBSYS_CNTL		0x8504
#define	ADVFUNC_CNTL	0x850c
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

// sequencer address and data registers
#define SEQ_INDEX		0x83c4
#define SEQ_DATA		0x83c5
#define SEQ_CLK_MODE	0x01

#define SCREEN_OFF		outb(SEQ_INDEX, 0x01);outb(SEQ_DATA, 0x21)
#define SCREEN_ON		outb(SEQ_INDEX, 0x01);outb(SEQ_DATA, 0x01)

// CRTC address and data registers
#define CRTC_INDEX		0x83d4
#define CRTC_DATA		0x83d5
#define INPUT_STATUS_1	0x83da
// Attribute controler
#define ATTR_REG		0x83c0
#define MISC_OUT_W		0x83c2
#define VGA_ENABLE		0x83c3
#define DAC_ADR_MASK	0x83c6
#define DAC_RD_INDEX	0x83c7
#define DAC_WR_INDEX	0x83c8
#define DAC_DATA		0x83c9
#define GCR_INDEX		0x83ce
#define GCR_DATA		0x83cf
#define MISC_OUT_R		0x83cc
// Standard reference frequency used by almost all the graphic card.
#define TI_REF_FREQ		14.318

// internal struct used to clone the add-on from server space to client space
typedef struct {
    pci_info	pcii;
    vuchar	*base0;
#if !defined(__INTEL__)
    vuchar	*isa_io;
#endif
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
    int     CursorMode;
    ulong	dot_clock;
} clone_info;

#endif




