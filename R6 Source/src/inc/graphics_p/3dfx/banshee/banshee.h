/* :ts=8 bk=0
 *
 * banshee.h:	Common header file.
 *
 * $Id:$
 *
 * Leo L. Schwab					1998.07.13
 *  Modified for R4					1998.11.05
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#ifndef __3DFX_BANSHEE_H
#define __3DFX_BANSHEE_H

#ifndef _PCI_H
#include <drivers/PCI.h>
#endif
#ifndef _DRIVERS_H
#include <drivers/Drivers.h>
#endif
#ifndef _ACCELERANT_H_
#include <add-ons/graphics/Accelerant.h>
#endif
#ifndef __3DFX_BANSHEE_DEFS_H
#include <graphics_p/3dfx/banshee/banshee_defs.h>
#endif
#ifndef _BENA4_H
#include <graphics_p/3dfx/common/bena4.h>
#endif
#include <graphics_p/3dfx/banshee/banshee_ioctls.h>

#include <graphics_p/common/MemMgr/MemMgr.h>

#include <graphics_p/video_overlay.h>

/*****************************************************************************
 * Definitions.
 */
 

 

#define	WRITEREGPAIR(idxregptr,idx,val)	\
		(*(vuint16 *) (idxregptr) = ((uint16) (val) << 8) | (idx))

#if 0
#define	IDXREG_W(ptr,which,idx,val) \
	outw((offsetof (union vgaregs, w.##which##_Idx)) + io_base_offset, \
				((uint16) (val) << 8) | (idx))
#endif

/*  This doesn't actually work with the SmegOWerks compiler.  */
#define	IDXREG_R(ptr,which,idx)		\
		((ptr)->w.##which##_Idx = (idx), (ptr)->r.##which##_Val)

#define	NREGS_ATTR		21
#define	NREGS_CRTC		0x79

#define  CLOCK_MIN               500.0	/*  Thin air... */
#define  CLOCK_MAX               220000.0

#define MAX_OVERLAY_BUFFERS 4

/* Device & Vendor Info */
#define VENDORID_3DFX 0x121A
#define DEVICEID_V1 0x0001
#define DEVICEID_V2 0x0002
#define DEVICEID_BANSHEE 0x0003
#define DEVICEID_AVENGER 0x0005 // aka Voodoo 3


typedef struct
{
	int used;						  // non-zero if in use
	// these are used to reposition the window during virtual desktop moves
	const overlay_buffer *ob;	  // current overlay_buffer for this overlay
	overlay_window ow;			  // current overlay_window for this overlay
	overlay_view ov;				  // current overlay_view for this overlay
}
thdfx_overlay_token;

typedef struct V3_Regs_Rec
{
	uint32 r_V3_2D_INTR_CTRL;
	uint32 r_V3_VID_DESKTOP_START_ADDR;
	uint32 r_V3_VID_CUR_LOC;
	uint32 r_V3_VID_PROC_CFG;
	uint32 r_V3_VID_SERIAL_PARALLEL_PORT;
	uint32 r_SSTIOADDR;
	uint32 r_V3_VID_PLL_CTRL_0;
	uint32 r_V3_VID_DAC_MODE;
	uint32 r_V3_VID_VGA_INIT_0;
	uint32 r_V3_VID_SCREEN_SIZE;
	uint32 r_V3_VID_OVERLAY_START_COORDS;
	uint32 r_V3_VID_OVERLAY_END_SCREEN_COORD;
	uint32 r_V3_VID_DAC_ADDR;
	uint32 r_V3_VID_DAC_DATA;
	uint32 r_V3_VID_IN_FORMAT;
	uint32 r_V3_VID_TV_OUT_BLANK_H_COUNT;
	uint32 r_V3_VID_TV_OUT_BLANK_V_COUNT;
	uint32 r_V3_3D_LEFT_OVERLAY_BUF;
	uint32 r_V3_3D_SWAP_BUFFER_CMD;
	uint32 r_V3_VID_CHROMA_MIN;
	uint32 r_V3_VID_CHROMA_MAX;
	uint32 r_V3_VID_OVERLAY_DUDX_OFFSET_SRC_WIDTH;
	uint32 r_V3_VID_OVERLAY_DUDX;
	uint32 r_V3_VID_OVERLAY_DVDY;
	uint32 r_V3_VID_DESKTOP_OVERLAY_STRIDE;
	uint32 r_V3_CMD_BASE_ADDR_1;
	uint32 r_V3_CMD_BUMP_1;
	uint32 r_V3_CMD_RD_PTR_L_1;
	uint32 r_V3_CMD_RD_PTR_H_1;
	uint32 r_V3_CMD_A_MIN_1;
	uint32 r_V3_CMD_A_MAX_1;
	uint32 r_V3_CMD_FIFO_DEPTH_1;
	uint32 r_V3_CMD_FIFO_HOLE_CNT_1;
	uint32 r_V3_CMD_BASE_SIZE_1;
	uint32 r_V3_CMD_BASE_ADDR_0;
	uint32 r_V3_CMD_BUMP_0;
	uint32 r_V3_CMD_RD_PTR_L_0;
	uint32 r_V3_CMD_RD_PTR_H_0;
	uint32 r_V3_CMD_A_MIN_0;
	uint32 r_V3_CMD_A_MAX_0;
	uint32 r_V3_CMD_FIFO_DEPTH_0;
	uint32 r_V3_CMD_FIFO_HOLE_CNT_0;
	uint32 r_V3_CMD_BASE_SIZE_0;
	uint32 r_V3_CMD_FIFO_THRESH;
	uint32 r_V3_3D_SWAP_BUFFER_PEND;
	uint32 r_V3_VID_CUR_C_0;
	uint32 r_V3_VID_CUR_C_1;
	uint32 r_V3_VID_CUR_PAT_ADDR;
	uint32 r_V3_VID_PLL_CTRL_1;
	uint32 r_V3_VID_PLL_CTRL_2;
	uint32 r_V3_VID_VGA_INIT_1;
	uint32 r_V3_VID_MISC_INIT_1;
	
} V3_Regs;


/*****************************************************************************
 * Shared data common to all instances of the add-on code.
 */
typedef struct thdfx_card_info
{
	accelerant_device_info ADI;	/*  "Identifying marks"    */
	char DevName[B_OS_NAME_LENGTH];

	volatile uint8 *BaseAddr0;	/*  I/O registers and channels      */
	volatile uint8 *BaseAddr1;	/*  Framebuffer space   */
	volatile uint8 *BaseAddr1_DMA;	/*  Framebuffer space   */
	volatile uint8 *ISA_IO;

	volatile uint8 *base_registers_pci[3];

	uint32 device_id;			  // PCI Device type (Banshee, Avenger etc)
	uint32 poolid;					  // memory pool id for frame buffer

	/*  Hardware arbitration locks   */
	/*  Obtain multiple locks in the order listed here.  */
	struct Bena4 CRTCLock;	  /*  CRTC, AR, SR, and MISC regs  */
	struct Bena4 CLUTLock;	  /*  256-entry palette      */
	struct Bena4 EngineLock;  /*  HW renderer         */
	sem_id VBlankLock;		  /*  WaitVBL support     */

	uint32 MemSize;			  /*  Total framebuffer RAM  */
	float MasterClockKHz;	  /*  Input PLL clock  */

	void *FBBase;				  /*  Pointer to framebuffer */
	void *FBBase_DMA;			  /*  Physical FB addr for DMA  */
	display_mode CurDispMode; /*  May contain custom mode   */
	uint16 BytesPerRow;
	uint16 Depth;				  /*  Bits per pixel      */

	display_mode *DispModes;
	uint32 NDispModes;

	int16 MonSizeX;			  /*  Display size on monitor   */
	int16 MonSizeY;			  /*  <50 - narrower, >50 - wider  */
	int16 MonPosX;				  /*  Display position on monitor  */
	int16 MonPosY;				  /*  <50 - left, >50 - right   */

	int16 MousePosX;			  /*  Position of mouse      */
	int16 MousePosY;
	int16 MouseHotX;			  /*  Hotpoint of mouse      */
	int16 MouseHotY;

	int16 DisplayAreaX;		  /* Position of the display area in the frame buffer */
	int16 DisplayAreaY;

	int32 IRQFlags;			  /*  Things for IRQ to do   */

	/*  sync_token stuff  */
	uint64 PrimitivesIssued;

	void *CursorBase;			  /*  Base address of cursor image */

	uint8 RegMisc;				  /*  Misc Output register   */
	uint8 ATTR[NREGS_ATTR];	  /*  GCR and sequencer   */
	uint8 CRTC[NREGS_CRTC];	  /*   values are static  */

	/* Fifo Details */
	struct cmdTransportInfo
	{
		uint32 *fifoPtr;			  /* Current write pointer into fifo */
		uint32 fifoRead;			  /* Last known hw read ptr. 
									   * If on an sli enabled system this will be
									   * the 'closest' hw read ptr of the sli
									   * master and slave. */

		/* Fifo checking information. In units of usuable bytes until
		 * the appropriate condition.
		 */
		int32 fifoRoom;			  /* Space until next fifo check */

		/* Basic command fifo characteristics. These should be
		 * considered logically const after their initialization.
		 */
		uint32 *fifoStart;		  /* Virtual address of start of fifo */
		uint32 *fifoEnd;			  /* Virtual address of fba fifo */
		uint32 fifoOffset;		  /* Offset from hw base to fifo start */
		uint32 fifoSize;			  /* Size in bytes of the fifo */

		/* NB: These are only valid for the fullscreen case 
		 */
		int32 roomToReadPtr;		  /* Bytes until last known hw ptr */
		int32 roomToEnd;			  /* # of bytes until last usable address before fifoEnd */
	}
	CmdTransportInfo[2];

	/* Overlay support */
	__mem_Allocation *AllocOverlayBuffer[MAX_OVERLAY_BUFFERS];
	overlay_buffer OverlayBuffer[MAX_OVERLAY_BUFFERS];

	int32 OverlayOwnerID;

	area_id memArea;
	__mem_Allocation *AllocSentinal;
	__mem_Allocation *AllocCursor;
	__mem_Allocation *AllocFrameBuffer;
	__mem_Allocation *AllocFifo[2];
	
	V3_Regs reg;
	struct thdfxRegs *Regs;
} thdfx_card_info;

#define	LEAVELOCKF_ENGINE	(1<<0)
#define	LEAVELOCKF_CLUT		(1<<1)
#define	LEAVELOCKF_CRTC		(1<<2)

/*
 * IRQFlags:  Flag definitions
 */
#define	IRQF_SETFBBASE		(1<<0)
#define	IRQF_MOVECURSOR		(1<<1)
#define	IRQF_SETCLUT		(1<<2)

#define	IRQF__ENABLED		(1<<30)	/*  Interrupts are enabled on card  */


#define _V3_WriteReg_NC( ci, REG, data ) { \
	uint32 *regs = (uint32 *) ci->BaseAddr0; \
	ci->reg.r_##REG = (data); \
	regs[ (REG) >> 2 ] = (data); \
}

static inline uint32 _V3_ReadReg( thdfx_card_info *ci, uint32 reg )
{
	volatile uint32 *regs = (uint32 *) ci->BaseAddr0;
	return regs[ reg >> 2 ];
}

#endif
