/* :ts=8 bk=0
 *
 * trident.h:	Common header file.
 *
 * $Id:$
 *
 * Leo L. Schwab					1999.09.06
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#ifndef __TRIDENT_H
#define __TRIDENT_H

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
#ifndef __TRIDENTDEFS_H
#include <graphics_p/trident/tridentdefs.h>
#endif


/*****************************************************************************
 * Definitions.
 */
#define	WRITEREGPAIR(idxregptr,idx,val)	\
		(*(vuint16 *) (idxregptr) = ((uint16) (val) << 8) | (idx))

#define	IDXREG_W(ptr,which,idx,val)	\
		(*(vuint16 *) (&(ptr)->w.##which##_Idx) = \
				((uint16) (val) << 8) | (idx))

/*  This doesn't actually work with the SmegOWerks compiler.  */
#define	IDXREG_R(ptr,which,idx)		\
		((ptr)->w.##which##_Idx = (idx), (ptr)->r.##which##_Val)

/*  I/O-based indexed register macros  */
#define	IDXREG_IOW(dummy,which,idx,val)	\
		outw (offsetof (union vgaregs, w.##which##_Idx),	\
		      ((uint16) (val) << 8) | (idx))

#define	IDXREG_IOR(dummy,which,idx)	\
		(outb (offsetof (union vgaregs, w.##which##_Idx), (idx)),  \
		 inb (offsetof (union vgaregs, r.##which##_Val)))


#define	NREGS_ATTR		21
#define	NREGS_CRTC		0x79

#define	CLOCK_MIN		6250.0		/*  Thin air...		*/
#define	CLOCK_MAX		206000.0	/*  Intel document	*/


/*****************************************************************************
 * ioctl() definitions.
 */
#define	TRIDENT_IOCTLPROTOCOL_VERSION	1

enum private_ioctls {
	TRIDENT_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	TRIDENT_IOCTL_GET_PCI,
	TRIDENT_IOCTL_SET_PCI,
	MAXIOCTL_TRIDENT
};

/*  TRIDENT_IOCTL_GETGLOBALS  */
typedef struct trident_getglobals {
	uint32	gg_ProtocolVersion;	/*  TRIDENT_IOCTLPROTOCOL_VERSION */
	area_id	gg_GlobalArea_CI;	/*  trident_card_info	*/
	area_id	gg_GlobalArea_CC;	/*  trident_card_ctl	*/
} trident_getglobals;

/*  TRIDENT_IOCTL_GET_PCI, TRIDENT_IOCTL_SET_PCI  */
typedef struct trident_getsetpci {
	uint32	gsp_Offset;
	uint32	gsp_Size;
	uint32	gsp_Value;
} trident_getsetpci;

/*  MAXIOCTL_TRIDENT + B_PROPOSE_DISPLAY_MODE  */
typedef struct trident_propose_display_mode {
	display_mode	*pdm_Target;
	display_mode	*pdm_Lo;
	display_mode	*pdm_Hi;
} trident_propose_display_mode;

/*  MAXIOCTL_TRIDENT + B_SET_DISPLAY_MODE  */
typedef struct trident_set_display_mode {
	display_mode	*sdm_Mode;
} trident_set_display_mode;

/*  MAXIOCTL_TRIDENT + B_GET_PIXEL_CLOCK_LIMITS  */
typedef struct trident_pixel_clock_limits {
	display_mode	*pcl_DisplayMode;
	uint32		pcl_Lo;
	uint32		pcl_Hi;
} trident_pixel_clock_limits;

/*  MAXIOCTL_TRIDENT + B_MOVE_DISPLAY  */
typedef struct trident_move_display {
	uint16	md_HDisplayStart;
	uint16	md_VDisplayStart;
} trident_move_display;

/*  MAXIOCTL_TRIDENT + B_SET_INDEXED_COLORS  */
typedef struct trident_set_indexed_colors {
	uint32	sic_Count;
	uint8	sic_First;
	uint8	*sic_ColorData;
	uint32	sic_Flags;
} trident_set_indexed_colors;

/*  MAXIOCTL_TRIDENT + B_SET_CURSOR_SHAPE  */
typedef struct trident_set_cursor_shape {
	uint16	scs_Width;
	uint16	scs_Height;
	uint16	scs_HotX;
	uint16	scs_HotY;
	uint8	*scs_ANDMask;
	uint8	*scs_XORMask;
} trident_set_cursor_shape;


/*****************************************************************************
 * Shared data common to all instances of the accelerant code.
 */
/*  Read-only to accelerants.  */
typedef struct tri_card_info
{
	accelerant_device_info	ci_ADI;	/*  "Identifying marks"		*/
	char			ci_DevName[B_OS_NAME_LENGTH];

	volatile uint8	*ci_BaseAddr0;	/*  Framebuffer space		*/
	volatile uint8	*ci_BaseAddr0_DMA;	/*  Physical FB addr	*/
	volatile uint8	*ci_BaseAddr1;	/*  I/O registers		*/
	volatile uint8	*ci_BaseAddr2;	/*  Rendering registers		*/
#if !defined(__INTEL__)
	volatile uint8	*ci_ISA_IO;
#endif

	/*  Hardware arbitration locks	*/
	/*  Obtain multiple locks in the order listed here.  */
	sem_id		ci_CRTCLock;	/*  CRTC, AR, SR, and MISC regs	*/
	sem_id		ci_CLUTLock;	/*  256-entry palette		*/
	sem_id		ci_VBlankLock;	/*  WaitVBL support		*/

	uint32		ci_MemSize;	/*  Total framebuffer RAM	*/
	float		ci_MasterClockKHz;	/*  Input PLL clock	*/

	BMemSpec	ci_FBAlloc;	/*  Pool alloc for display	*/
	uint32		ci_FBBase;	/*  Framebuffer offset		*/
	uint32		ci_PoolID;	/*  PoolID for framebuffer	*/
	display_mode	ci_CurDispMode;	/*  May contain custom mode	*/
	uint16		ci_BytesPerRow;
	uint16		ci_Depth;	/*  Bits per pixel		*/

	TridentRegs	*ci_Regs;	/*  Trident memory-mapped registers*/
	DDDGraphicsEngineRegs	*ci_DDDG;	/*  Rendering engine regs  */
	uint32		ci_CursorBase;	/*  Cursor image offset		*/
	
	uint8		ci_DPMSMode;	/*  Current DPMS mode		*/
} tri_card_info;

/*  Read-write to accelerants.  */
typedef struct tri_card_ctl {
	/*
	 * Kernel driver creates this lock, but never touches it, as it's
	 * unsafe to do so.  Accelerant *MUST* ensure this lock is held
	 * before thunking out to driver for mode-changing services, or take
	 * responsibility for cleaning up the possible train wreck afterwards.
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
	uint32		cc_CursColorBlk;	/*  Cursor colors	*/
	uint32		cc_CursColorWht;

	/*  Mirrors of some hardware registers  */
	uint8		cc_RegMisc;	/*  Misc Output register	*/
	uint8		cc_ATTR[NREGS_ATTR];	/*  GCR and sequencer	*/
	uint8		cc_CRTC[NREGS_CRTC];	/*   values are static	*/

//	trident_3DState	cc_3Dstate;
} tri_card_ctl;


/*
 * cc_IRQFlags:  Flag definitions
 */
#define	IRQF_SETFBBASE		(1<<0)
#define	IRQF_MOVECURSOR		(1<<1)
#define	IRQF_SETCLUT		(1<<2)


#endif
