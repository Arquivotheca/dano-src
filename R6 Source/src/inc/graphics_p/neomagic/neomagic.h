/* 
 *
 * neomagic.h:	Common header file.
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#ifndef __NEOMAGIC_H
#define __NEOMAGIC_H

#include <drivers/PCI.h>
#include <drivers/Drivers.h>
#include <add-ons/graphics/Accelerant.h>
#include <graphics_p/neomagic/bena4.h>

/*****************************************************************************
 * Definitions.
 */
#define inb(p) (*pci_bus->read_io_8)(p)
#define outb(p,v) (*pci_bus->write_io_8)(p,v)
#define inw(p) (*pci_bus->read_io_16)(p)
#define outw(p,v) (*pci_bus->write_io_16)(p,v)
#define inl(p) (*pci_bus->read_io_32)(p)
#define outl(p,v) (*pci_bus->write_io_32)(p,v)

#define  CLOCK_MIN               500.0          /*  Thin air...*/
#define  CLOCK_MAX               220000.0

/* Some useful VGA Register Offsets */
#define GRAX	0x3CE

/* Device & Vendor Info */
#define VENDORID_NEOMAGIC 0x10C8
#define DEVICEID_NM2070		0x0001
#define DEVICEID_NM2090		0x0002
#define DEVICEID_NM2093		0x0003
#define DEVICEID_NM2097		0x0083
#define DEVICEID_NM2160		0x0004
#define DEVICEID_NM2200		0x0005
#define DEVICEID_NM256ZV	0x0006

#define NUM_SUPPORTED_DEVICES 7

/*****************************************************************************
 * Shared data common to all instances of the add-on code.
 */
typedef struct neomagic_card_info
{
	accelerant_device_info	ci_ADI;	/*  "Identifying marks"		*/
	char			ci_DevName[B_OS_NAME_LENGTH];

	volatile uint8	*ci_BaseAddr0;	/*  Framebuffer space */
	volatile uint8	*ci_BaseAddr1;	/*  Registers */
	volatile uint8	*ci_BaseAddr0_DMA;	/*  Framebuffer space	*/

	volatile uint8	*ci_base_registers_pci[3];
	
	uint32 ci_device_id;		// PCI Device type
	
	/*  Hardware arbitration locks	*/
	/*  Obtain multiple locks in the order listed here.  */
	struct Bena4	ci_CRTCLock;	/*  CRTC, AR, SR, and MISC regs	*/
	struct Bena4	ci_CLUTLock;	/*  256-entry palette		*/
	struct Bena4	ci_EngineLock;	/*  HW renderer			*/
	sem_id		ci_VBlankLock;	/*  WaitVBL support		*/

	uint32		ci_MemSize;	/*  Total framebuffer RAM	*/
	float		ci_MasterClockKHz;	/*  Input PLL clock	*/

	void		*ci_FBBase;	/*  Pointer to framebuffer	*/
	void		*ci_FBBase_DMA;	/*  Physical FB addr for DMA	*/
	display_mode	ci_CurDispMode;	/*  May contain custom mode	*/
	uint16		ci_BytesPerRow;
	uint16		ci_Depth;	/*  Bits per pixel		*/

	display_mode	*ci_DispModes;
	uint32		ci_NDispModes;

	int16		ci_MousePosX;	/*  Position of mouse		*/
	int16		ci_MousePosY;
	int16		ci_MouseHotX;	/*  Hotpoint of mouse		*/
	int16		ci_MouseHotY;

	int16		ci_DisplayAreaX;	/* Position of the display area in the frame buffer */
	int16		ci_DisplayAreaY;
	
	int16		ci_PanelWidth;
	int16		ci_PanelHeight;
	
	int32		ci_PrimitivesIssued;
	int32		ci_PrimitivesCompleted;
	
	int32		ci_IRQFlags;	/*  Things for IRQ to do	*/

	void		*ci_CursorBase;	/*  Base address of cursor image*/
} neomagic_card_info;

#define	LEAVELOCKF_ENGINE	(1<<0)
#define	LEAVELOCKF_CLUT		(1<<1)
#define	LEAVELOCKF_CRTC		(1<<2)

/*
 * ci_IRQFlags:  Flag definitions
 */
#define	IRQF_SETFBBASE		(1<<0)
#define	IRQF_MOVECURSOR		(1<<1)
#define	IRQF_SETCLUT		(1<<2)

#define	IRQF__ENABLED		(1<<30)	/*  Interrupts are enabled on card  */

/* Register Offset Definitions - these work with the index style of registers a la
 * VGA. Neomagic extends the GRAX table with its own additions
 */
#define NEO_GENERALLOCKREG 0x0a
#define NEO_EXTCRTDISPADDR	0x0e
#define NEO_EXTCRTOFFSET	0x0f
#define NEO_SYSIFACECNTL1 0x10
#define NEO_SYSIFACECNTL2 0x11
#define NEO_PANELDISPCNTLREG1	0x20
#define NEO_PANELDISPCNTLREG2	0x25
#define NEO_PANELVERTCENTERREG1 0x28
#define NEO_PANELVERTCENTERREG2 0x29
#define NEO_PANELVERTCENTERREG3 0x2a
#define NEO_PANELDISPCNTLREG3	0x30
#define NEO_PANELVERTCENTERREG4 0x32
#define NEO_PANELHORIZCENTERREG1 0x33
#define NEO_PANELHORIZCENTERREG2 0x34
#define NEO_PANELHORIZCENTERREG3 0x35
#define NEO_PANELHORIZCENTERREG4 0x36
#define NEO_PANELVERTCENTERREG5 0x37
#define NEO_PANELHORIZCENTERREG5 0x38
#define NEO_CURSORXLOW	0x70
#define NEO_CURSORXHIGH	0x71
#define NEO_CURSORYLOW	0x72
#define NEO_CURSORYHIGH	0x73
#define NEO_HWCURSORBKGNDRED 0x74
#define NEO_HWCURSORBKGNDGREEN 0x75
#define NEO_HWCURSORBKGNDBLUE 0x76
#define NEO_HWCURSORFGNDRED 0x77
#define NEO_HWCURSORFGNDGREEN 0x78
#define NEO_HWCURSORFGNDBLUE 0x79
#define NEO_HWCURSORCNTL 0x82
#define NEO_HWCURSORMEMLOC 0x085
#define NEO_HWICONMEMLOC 0x086
#define NEO_EXTCOLORMODESELECT 0x90

// Memory Mapped Register Offsets
#define NEOREG_BLTSTAT		0x00
#define NEOREG_BLTMODE          0x02
#define NEOREG_BLTCNTL		0x04
#define NEOREG_FGCOLOR    0x0c
#define NEOREG_PITCH		0x14
#define NEOREG_SRCSTARTOFF      0x24
#define NEOREG_DSTSTARTOFF      0x2c
#define NEOREG_XYEXT            0x30

// Register Writing Macros
#define INREG32(addr) *(volatile uint32 *)(ci->ci_BaseAddr1 + (addr))
#define OUTREG16(addr, val) *(volatile uint16 *)(ci->ci_BaseAddr1 + (addr)) = (val)
#define OUTREG32(addr, val) *(volatile uint32 *)(ci->ci_BaseAddr1 + (addr)) = (val)

# define WAIT_BLT_DONE(x) {                                                   \
            while( INREG32(NEOREG_BLTSTAT) & NEO_BS0_BLT_BUSY) { snooze(x); }               \
        }

#define WAIT_ENGINE_IDLE(x) {                                                \
	    WAIT_BLT_DONE(x);                                                \
        }

// Neomagic 2D acceleration options & values
#define CURSOR_ENABLE	0x01

#define NEO_BS0_BLT_BUSY        0x00000001
#define NEO_BS0_FIFO_AVAIL      0x00000002
#define NEO_BS0_FIFO_PEND       0x00000004

#define NEO_BC0_DST_Y_DEC       0x00000001
#define NEO_BC0_DST_X_DEC       0x00000002
#define NEO_BC0_SRC_TRANS       0x00000004
#define NEO_BC0_SRC_IS_FG       0x00000008
#define NEO_BC0_SRC_Y_DEC       0x00000010
#define NEO_BC0_FILL_PAT        0x00000020
#define NEO_BC0_SRC_MONO        0x00000040
#define NEO_BC0_SYS_TO_VID      0x00000080

#define NEO_BC1_DEPTH8          0x00000100
#define NEO_BC1_DEPTH16         0x00000200
#define NEO_BC1_X_320           0x00000400
#define NEO_BC1_X_640           0x00000800
#define NEO_BC1_X_800           0x00000c00
#define NEO_BC1_X_1024          0x00001000
#define NEO_BC1_X_1152          0x00001400
#define NEO_BC1_X_1280          0x00001800
#define NEO_BC1_X_1600          0x00001c00
#define NEO_BC1_DST_TRANS       0x00002000
#define NEO_BC1_MSTR_BLT        0x00004000
#define NEO_BC1_FILTER_Z        0x00008000

#define NEO_BC2_WR_TR_DST       0x00800000

#define NEO_BC3_SRC_XY_ADDR     0x01000000
#define NEO_BC3_DST_XY_ADDR     0x02000000
#define NEO_BC3_CLIP_ON         0x04000000
#define NEO_BC3_FIFO_EN         0x08000000
#define NEO_BC3_BLT_ON_ADDR     0x10000000
#define NEO_BC3_SKIP_MAPPING    0x80000000

#define NEO_MODE1_DEPTH8        0x0100
#define NEO_MODE1_DEPTH16       0x0200
#define NEO_MODE1_DEPTH24       0x0300
#define NEO_MODE1_X_320         0x0400
#define NEO_MODE1_X_640         0x0800
#define NEO_MODE1_X_800         0x0c00
#define NEO_MODE1_X_1024        0x1000
#define NEO_MODE1_X_1152        0x1400
#define NEO_MODE1_X_1280        0x1800
#define NEO_MODE1_X_1600        0x1c00
#define NEO_MODE1_BLT_ON_ADDR   0x2000

#endif
