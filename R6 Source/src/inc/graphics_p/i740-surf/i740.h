/* :ts=8 bk=0
 *
 * i740.h:	Common header file.
 *
 * $Id:$
 *
 * Leo L. Schwab					1998.07.13
 *  Modified for R4					1998.11.05
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#ifndef __I740_H
#define __I740_H

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
#ifndef __I740DEFS_H
#include <graphics_p/i740/i740defs.h>
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


#define	NREGS_ATTR		21
#define	NREGS_CRTC		0x79

#define	CLOCK_MIN		6250.0		/*  Thin air...		*/
#define	CLOCK_MAX		206000.0	/*  Intel document	*/


/*****************************************************************************
 * ioctl() definitions.
 */
#define	I740_IOCTLPROTOCOL_VERSION	2

enum private_ioctls {
	I740_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	I740_IOCTL_GET_PCI,
	I740_IOCTL_SET_PCI,
	MAXIOCTL_I740
};

/*  I740_IOCTL_GETGLOBALS  */
typedef struct i740_getglobals {
	uint32	gg_ProtocolVersion;	/*  I740_IOCTLPROTOCOL_VERSION	*/
	area_id	gg_GlobalArea_CI;	/*  i740_card_info	*/
	area_id	gg_GlobalArea_CC;	/*  i740_card_ctl	*/
} i740_getglobals;

/*  I740_IOCTL_GET_PCI, I740_IOCTL_SET_PCI  */
typedef struct i740_getsetpci {
	uint32	gsp_Offset;
	uint32	gsp_Size;
	uint32	gsp_Value;
} i740_getsetpci;

/*  MAXIOCTL_I740 + B_PROPOSE_DISPLAY_MODE  */
typedef struct i740_propose_display_mode {
	display_mode	*pdm_Target;
	display_mode	*pdm_Lo;
	display_mode	*pdm_Hi;
} i740_propose_display_mode;

/*  MAXIOCTL_I740 + B_SET_DISPLAY_MODE  */
typedef struct i740_set_display_mode {
	display_mode	*sdm_Mode;
//	uint32		sdm_LockFlags;	/*  Deprecated	*/
} i740_set_display_mode;
#define	LEAVELOCKF_ENGINE	(1<<0)
#define	LEAVELOCKF_CLUT		(1<<1)
#define	LEAVELOCKF_CRTC		(1<<2)

/*  MAXIOCTL_I740 + B_GET_PIXEL_CLOCK_LIMITS  */
typedef struct i740_pixel_clock_limits {
	display_mode	*pcl_DisplayMode;
	uint32		pcl_Lo;
	uint32		pcl_Hi;
} i740_pixel_clock_limits;

/*  MAXIOCTL_I740 + B_MOVE_DISPLAY  */
typedef struct i740_move_display {
	uint16	md_HDisplayStart;
	uint16	md_VDisplayStart;
} i740_move_display;

/*  MAXIOCTL_I740 + B_SET_INDEXED_COLORS  */
typedef struct i740_set_indexed_colors {
	uint32	sic_Count;
	uint8	sic_First;
	uint8	*sic_ColorData;
	uint32	sic_Flags;
} i740_set_indexed_colors;

/*  MAXIOCTL_I740 + B_SET_CURSOR_SHAPE  */
typedef struct i740_set_cursor_shape {
	uint16	scs_Width;
	uint16	scs_Height;
	uint16	scs_HotX;
	uint16	scs_HotY;
	uint8	*scs_ANDMask;
	uint8	*scs_XORMask;
} i740_set_cursor_shape;


typedef struct i740_3DStateRec {
	uint32	textureWrapU;
	uint32	textureWrapV;
	uint32	textureMinFilter;
	uint32	textureMagFilter;
	uint32	textureEnvMode;
	uint32	textureFormat;
	float	textureLODBias;
	int32	textureDitherWeight;
	uint32	srcBlendFunc;
	uint32	destBlendFunc;
	uint32	cullData;
	uint32	bufferEnableData;
	uint32	depthTestFunction;
	float	alphaTestValue;
	uint32	alphaTestFunction;
	float	lineWidth;
	uint32	textureID;
	uint32	fogColor;

	/* Card offsed memory locations */
	uint32	bufFrameStart;
	uint32	bufFrameEnd;
	uint32	bufFramePitch;
	uint32	bufBack1Start;
	uint32	bufBack1End;
	uint32	bufBack1Pitch;
	uint32	bufBack2Start;
	uint32	bufBack2End;
	uint32	bufBack2Pitch;
	uint32	bufBack3Start;
	uint32	bufBack3End;
	uint32	bufBack3Pitch;
	uint32	bufZ1Start;
	uint32	bufZ1End;
	uint32	bufZ1Pitch;
	uint32	bufZ2Start;
	uint32	bufZ2End;
	uint32	bufZ2Pitch;
	uint32	bufZ3Start;
	uint32	bufZ3End;
	uint32	bufZ3Pitch;

	uint8	shadeModeIsSmooth;
	uint8	colorDitherEnabled;
	uint8	depthTestEnabled;
	uint8	alphaTestEnabled;
	uint8	colorBlendEnabled;
	uint8	fogEnabled;
	uint8	specularEnabled;
	uint8	polyStippleEnabled;
	uint8	textureEnabled;

	uint8	_reserved[256];
} i740_3DState;


/*****************************************************************************
 * Shared data common to all instances of the accelerant code.
 */
/*  Read-only to accelerants.  */
typedef struct i740_card_info
{
	accelerant_device_info	ci_ADI;	/*  "Identifying marks"		*/
	char			ci_DevName[B_OS_NAME_LENGTH];

	volatile uint8	*ci_BaseAddr0;	/*  Framebuffer space		*/
	volatile uint8	*ci_BaseAddr0_DMA;	/*  Physical FB addr	*/
	volatile uint8	*ci_BaseAddr1;	/*  I/O registers and channels	*/
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

	struct I740Regs	*ci_Regs;	/*  I740 memory-mapped registers*/
	uint32		ci_CursorBase;	/*  Cursor image offset		*/
} i740_card_info;

/*  Read-write to accelerants.  */
typedef struct i740_card_ctl {
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

	/*  Mirrors of some hardware registers  */
	uint8		cc_RegMisc;	/*  Misc Output register	*/
	uint8		cc_ATTR[NREGS_ATTR];	/*  GCR and sequencer	*/
	uint8		cc_CRTC[NREGS_CRTC];	/*   values are static	*/

	i740_3DState	cc_3Dstate;
} i740_card_ctl;


/*
 * cc_IRQFlags:  Flag definitions
 */
#define	IRQF_SETFBBASE		(1<<0)
#define	IRQF_MOVECURSOR		(1<<1)
#define	IRQF_SETCLUT		(1<<2)


#endif
