/* :ts=8 bk=0
 *
 * lynxem.h:	Common header file.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.07.06
 *
 * Copyright 2000 Be Incorporated.
 */
#ifndef __LYNXEM_H
#define __LYNXEM_H

#ifndef _PCI_H
#include <drivers/PCI.h>
#endif
#ifndef _DRIVERS_H
#include <drivers/Drivers.h>
#endif
#ifndef _ACCELERANT_H_
#include <add-ons/graphics/Accelerant.h>
#endif
#ifndef _BENA4_H
#include <dinky/bena4.h>
#endif
#ifndef _GENPOOL_H
#include <surface/genpool.h>
#endif
#ifndef __LYNXEMDEFS_H
#include <graphics_p/lynxem/lynxemdefs.h>
#endif


/*****************************************************************************
 * Definitions.
 */
#define	NUMOF(array)		(sizeof (array) / sizeof (*(array)))

#define	WRITEREGPAIR(idxregptr,idx,val)	\
		(*(vuint16 *) (idxregptr) = ((uint16) (val) << 8) | (idx))

#define	IDXREG_W(ptr,which,idx,val)	\
		(*(vuint16 *) (&(ptr)->w.##which##_Idx) = \
				((uint16) (val) << 8) | (idx))

/*  This doesn't actually work with the MetroQuirks compiler.  */
#define	IDXREG_R(ptr,which,idx)		\
		((ptr)->w.##which##_Idx = (idx), (ptr)->r.##which##_Val)

/*  I/O-based indexed register macros  */
#define	IDXREG_IOW(dummy,which,idx,val)	\
		outw (offsetof (union vgaregs, w.##which##_Idx),	\
		      ((uint16) (val) << 8) | (idx))

#define	IDXREG_IOR(dummy,which,idx)	\
		(outb (offsetof (union vgaregs, w.##which##_Idx), (idx)),  \
		 inb (offsetof (union vgaregs, r.##which##_Val)))


#define	NREGS_ATTR		0x15		/*  Basic VGA only	*/
#define	NREGS_CRTC		0x50		/*  Includes shadows	*/

#define	CLOCK_MIN		6250.0		/*  Thin air...		*/
#define	CLOCK_MAX		135000.0	/*  SMI document	*/


/*****************************************************************************
 * Private ioctl() definitions.
 */
#define	LYNXEM_IOCTLPROTOCOL_VERSION	1

enum private_ioctls {
	GDRV_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	GDRV_IOCTL_GET_PCI,
	GDRV_IOCTL_SET_PCI,
	MAXIOCTL_GDRV
};

/*  GDRV_IOCTL_GETGLOBALS  */
typedef struct gdrv_getglobals {
	uint32	gg_ProtocolVersion;	/*  LYNXEM_IOCTLPROTOCOL_VERSION	*/
	area_id	gg_GlobalArea_CI;	/*  gfx_card_info	*/
	area_id	gg_GlobalArea_CC;	/*  gfx_card_ctl	*/
} gdrv_getglobals;

/*  GDRV_IOCTL_GET_PCI, GDRV_IOCTL_SET_PCI  */
typedef struct gdrv_getsetpci {
	uint32	gsp_Offset;
	uint32	gsp_Size;
	uint32	gsp_Value;
} gdrv_getsetpci;

/*  MAXIOCTL_GDRV + B_PROPOSE_DISPLAY_MODE  */
typedef struct gdrv_propose_display_mode {
	display_mode	*pdm_Target;
	display_mode	*pdm_Lo;
	display_mode	*pdm_Hi;
} gdrv_propose_display_mode;

/*  MAXIOCTL_GDRV + B_GET_PIXEL_CLOCK_LIMITS  */
typedef struct gdrv_pixel_clock_limits {
	display_mode	*pcl_DisplayMode;
	uint32		pcl_Lo;
	uint32		pcl_Hi;
} gdrv_pixel_clock_limits;

/*  MAXIOCTL_GDRV + B_MOVE_DISPLAY  */
typedef struct gdrv_move_display {
	uint16	md_HDisplayStart;
	uint16	md_VDisplayStart;
} gdrv_move_display;

/*  MAXIOCTL_GDRV + B_SET_INDEXED_COLORS  */
typedef struct gdrv_set_indexed_colors {
	uint32	sic_Count;
	uint8	sic_First;
	uint8	*sic_ColorData;
	uint32	sic_Flags;
} gdrv_set_indexed_colors;

/*  MAXIOCTL_GDRV + B_SET_CURSOR_SHAPE  */
typedef struct gdrv_set_cursor_shape {
	uint16	scs_Width;
	uint16	scs_Height;
	uint16	scs_HotX;
	uint16	scs_HotY;
	uint8	*scs_ANDMask;
	uint8	*scs_XORMask;
} gdrv_set_cursor_shape;


/*****************************************************************************
 * Register and other values for display modes.
 */
typedef struct moderegs {
	uint32		mr_FBSize;	/*  Full size of FB, in bytes	*/
	uint32		mr_FBULOffset;	/*  Offset of upper-left corner	*/
	uint16		mr_BytesPerRow;
	uint16		mr_Depth;	/*  Bits per pixel		*/
	
	uint32		mr_GrVidCtl;
	uint32		mr_FBWidthPitch;

	uint8		mr_ATTR[NREGS_ATTR];	/*  GCR and sequencer	*/
	uint8		mr_CRTC[NREGS_CRTC];	/*   values are static	*/
	uint8		mr_Misc;	/*  Misc Output register	*/
	uint8		mr_VCLK_NUM;
	uint8		mr_VCLK_DENOM;
} moderegs;


/*****************************************************************************
 * Shared data common to all instances of the accelerant code.
 */
/*  Read-only to accelerants.  */
typedef struct gfx_card_info
{
	accelerant_device_info	ci_ADI;	/*  "Identifying marks"		*/
	char			ci_DevName[B_OS_NAME_LENGTH];

	volatile uint8	*ci_BaseAddr0;	/*  Framebuffer aperture	*/
	volatile uint8	*ci_BaseAddr0_DMA;	/*  Physical FB addr	*/
#if !defined(__INTEL__)
	volatile uint8	*ci_ISA_IO;
#endif

	/*  Hardware arbitration locks	*/
	/*  Obtain multiple locks in the order listed here.  */
	sem_id		ci_ModeRegsLock;/*  Arbitrates cc_ModeRegs	*/
	sem_id		ci_CLUTLock;	/*  256-entry palette regs	*/
	sem_id		ci_VBlankLock;	/*  WaitVBL support		*/

	uint32		ci_MemSize;	/*  Total framebuffer RAM	*/
	uint32		ci_IRQEnabled;	/*  Working IRQ?		*/
	float		ci_MasterClockKHz;	/*  Input PLL clock	*/

	BMemSpec	ci_FBAlloc;	/*  Pool alloc for display	*/
	uint32		ci_FBBase;	/*  Framebuffer offset		*/
	uint32		ci_PoolID;	/*  PoolID for framebuffer	*/
	display_mode	ci_CurDispMode;	/*  May contain custom mode	*/
	uint16		ci_BytesPerRow;
	uint16		ci_Depth;	/*  Bits per pixel		*/
	uint16		ci_LCDWidth;	/*  Size of flat panel in pixels*/
	uint16		ci_LCDHeight;
	
	uint32		ci_CursorOffset;/*  Cursor offset in FB RAM	*/

	vgaregs		*ci_VGARegs;	/*  Memory-mapped VGA registers	*/
	DPRegs		*ci_DPRegs;	/*  Drawing registers		*/
	VPRegs		*ci_VPRegs;	/*  Video/display registers	*/
} gfx_card_info;

/*  Read-write to accelerants.  */
typedef struct gfx_card_ctl {
	/*
	 * Kernel driver creates this lock, but never touches it, as it's
	 * unsafe to do so.  Accelerant *MUST* ensure this lock is held and
	 * rendering engines are idle before thunking out to driver for
	 * mode-changing services, or take responsibility for cleaning up the
	 * possible train wreck afterwards.
	 */
	Bena4		cc_EngineLock;	/*  HW renderer			*/

	/*  sync_token stuff  */
	uint64		cc_PrimitivesIssued;
	uint64		cc_PrimitivesCompleted;	/*  Not used yet...	*/

	int32		cc_IRQFlags;	/*  Things for IRQ to do	*/

	int16		cc_MousePosX;	/*  Position of mouse		*/
	int16		cc_MousePosY;
	int16		cc_MouseHotX;	/*  Hotpoint of mouse		*/
	int16		cc_MouseHotY;

	/*  Mirrors of some hardware registers  */
	moderegs	cc_ModeRegs;	/*  Must hold ci_ModeRegsLock	*/
} gfx_card_ctl;


/*
 * cc_IRQFlags:  Flag definitions
 */
#define	IRQF_SETFBBASE		(1<<0)
#define	IRQF_MOVECURSOR		(1<<1)
#define	IRQF_SETCLUT		(1<<2)


#endif	/*  __LYNXEM_H  */
