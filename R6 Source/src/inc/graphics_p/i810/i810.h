/* :ts=8 bk=0
 *
 * i810.h:	Common header file.
 *
 * $Id:$
 *
 * Leo L. Schwab					2000.01.10
 *  Based on previous R3/R4 work.			1998.11.05
 *
 * Copyright 2000 Be Incorporated.
 */
#ifndef __I810_H
#define __I810_H

#define	DEBUG_SYNCLOG	1

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
#ifndef _LISTNODE_H
#include <dinky/listnode.h>
#endif
#ifndef _GENPOOL_H
#include <surface/genpool.h>
#endif
#ifndef _VIDEO_OVERLAY_H_
#include <graphics_p/video_overlay.h>
#endif
#ifndef __I810DEFS_H
#include <graphics_p/i810/i810defs.h>
#endif
#ifndef _GFXBUF_H
#include <graphics_p/i810/gfxbuf.h>
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


#define	NREGS_ATTR		21
#define	NREGS_CRTC		0x83

#define	CLOCK_MIN		6250.0		/*  Thin air...		*/
#define	CLOCK_MAX		230000.0	/*  Intel document	*/


/*****************************************************************************
 * Private ioctl() definitions.
 */
#define	I810_IOCTLPROTOCOL_VERSION	5

enum private_ioctls {
	GDRV_IOCTL_GETGLOBALS = B_DEVICE_OP_CODES_END + 1,
	GDRV_IOCTL_GET_PCI,
	GDRV_IOCTL_SET_PCI,
	GDRV_IOCTL_SET_DISPMODE,	/*  display_mode	*/
	GDRV_IOCTL_SET_FBBASE,		/*  uint32		*/

	/*  i810-specific ioctl()s.  */
	I810DRV_ADDFENCE,		/*  Add tiled fence entry  */
	I810DRV_REMFENCE,		/*  Remove fence entry     */
	I810DRV_SET_GL_DISPMODE,	/*  Set dispmode for GL    */

	/*  End of non-thunked ioctl()s.  */
	MAXIOCTL_GDRV
};

/*  GDRV_IOCTL_GETGLOBALS  */
typedef struct gdrv_getglobals {
	uint32	gg_ProtocolVersion;	/*  I810_IOCTLPROTOCOL_VERSION	*/
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

/*  MAXIOCTL_GDRV + B_CONFIGURE_OVERLAY  */
typedef struct gdrv_configure_overlay {
	overlay_token	co_Token;
	overlay_buffer	co_Buffer;
	overlay_window	co_Window;
	overlay_view	co_View;
} gdrv_configure_overlay;


/*****************************************************************************
 * i810-specific ioctl()s.
 */
/*  I810DRV_SET_GL_DISPMODE  */
typedef struct i810drv_set_gl_disp {
	uint16	sgd_Width;
	uint16	sgd_Height;
	uint16	sgd_FieldRate;
	uint16	sgd_ColorSpace;
} i810drv_set_gl_disp;


/*****************************************************************************
 * 3D accelerant init stuff.
 */
typedef void (*rectfill_general) (uint32 color,
				  uint32 bltctl,
				  fill_rect_params *list,
				  uint32 count);
typedef void (*flush_drop_cookie) (uint32 cookie);
typedef void (*write_packet) (void *cl_buf, int32 size, int execute, int which);
typedef void (*accel_3d_uninit) (void);
typedef status_t (*glcontext_init) (void *gc);
typedef void (*glcontext_uninit) (void *gc);


union init3d {
	/*  Passed to 3D accelerant initialization routine.  */
	struct {
		struct gfx_card_info	*i_ci;
		struct gfx_card_ctl	*i_cc;
		const char		*i_DevicePath;
		GetAccelerantHook	i_GAH;
		rectfill_general	i_RectFill;
		flush_drop_cookie	i_FlushDropCookie;
		write_packet		i_WritePacket;
		int			i_DevFD;
	} init;

	/*  These two are passed back from the 3D accelerant.  */
	struct {
		accel_3d_uninit		r_Accel3DUninit;
		glcontext_init		r_GLContextInit;
		glcontext_uninit	r_GLContextUninit;
	} reply;
};


/*****************************************************************************
 * Register and other values for display modes.
 *
 * "FB" means framebuffer and, when I say 'framebuffer', I'm referring to the
 * image buffer containing the Tracker desktop display.
 */
typedef struct moderegs {
	uint32		mr_DCLK_MN2;
	uint32		mr_PIXCONF;
	uint32		mr_BLTCTL;
	uint32		mr_FW_BLC;
	uint8		mr_ATTR[NREGS_ATTR];	/*  GCR and sequencer	*/
	uint8		mr_CRTC[NREGS_CRTC];	/*   values are static	*/
	uint8		mr_Misc;	/*  Misc Output register	*/
	uint8		mr_DCLK_P2;
} moderegs;

typedef struct dispconfig {
	display_mode	dc_DispMode;	/*  May contain custom mode	*/
	moderegs	dc_MR;
	uint32		dc_FBBase;	/*  Base offset of FB		*/
	uint32		dc_FBULOffset;	/*  Offset of upper-left corner	*/
	uint16		dc_BytesPerRow;
	uint16		dc_Depth;	/*  Bits per pixel		*/
	uint8		dc_CMAP[256][3];
} dispconfig;

typedef struct cursorstate {
	int16	cs_X;		/*  Position of hotpoint on display	*/
	int16	cs_Y;
	int16	cs_HotX;	/*  Offset in glyph to hotpoint		*/
	int16	cs_HotY;
	uint8	cs_Enabled;	/*  TRUE == visible			*/
} cursorstate;


/*****************************************************************************
 * Per-overlay data.
 */
typedef struct i810_overlay {
	overlay_window		ovl_Window;
	overlay_view		ovl_View;
	const overlay_buffer	*ovl_Buffer;
	uint8			ovl_InUse;
} i810_overlay;


/*****************************************************************************
 * Shared data common to all instances of the accelerant code.
 */
/*  Read-only to accelerants.  */
typedef struct gfx_card_info
{
	accelerant_device_info	ci_ADI;	/*  "Identifying marks"		*/
	char			ci_DevName[B_OS_NAME_LENGTH];

	volatile uint8	*ci_BaseAddr0;	/*  Framebuffer aperture	*/
	volatile uint8	*ci_BaseAddr1;	/*  I/O registers and channels	*/
	volatile HWStatusPage *
			ci_HWStatusPage;
	uint32		ci_BaseAddr0_DMA;	/*  Physical FB addr	*/

	vuint32		*ci_CursorBase;	/*  Cursor image base		*/

	struct I810Regs	*ci_Regs;	/*  I810 memory-mapped registers*/

	/*  Hardware arbitration locks	*/
	/*  Obtain multiple locks in the order listed here.  */
	sem_id		ci_DispConfigLock;/*  Arbitrates ci_DispConfig	*/
	sem_id		ci_DispBufLock;	/*  Arbitrates cc_DispBuf	*/
	sem_id		ci_VGARegsLock;	/*  Arbitrates VGA registers	*/
	sem_id		ci_OverlayLock;	/*  Overlay data structs & regs	*/
	sem_id		ci_OtherRegsLock;/*  Other shared regs		*/
	sem_id		ci_VBlankLock;	/*  WaitVBL support		*/

	uint32		ci_MemSize;	/*  Total graphics RAM		*/
	uint32		ci_IRQEnabled;	/*  Working IRQ?		*/
	float		ci_MasterClockKHz;	/*  Input PLL clock	*/

	uint32		ci_PoolID;	/*  PoolID for graphics RAM	*/
	List		ci_OpenerList;	/*  First entry is current mode	*/

	uint32		ci_CmdBase;	/*  Command buffer offset	*/
	uint32		ci_IRQCmdBase;	/*  IRQ ring buffer offset	*/
	uint32		ci_CmdSize;	/*  Size of cmdbufs, in bytes	*/

	dispconfig	ci_DispConfig;	/* Current display configuration */

	/*  Overlay management stuff.  */
	OverlayRegs	*ci_OverlayRegs;
	uint32		ci_OverlayRegs_DMA;
	overlay_buffer	ci_OverlayBuffers[4];
	GfxBuf		ci_OverlayAllocs[4];
	i810_overlay	ci_ActiveOverlays[1];
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

	uint32		cc_CmdTail;	/*  Offset of next instruction	*/
	uint32		cc_IRQCmdTail;	/*  Same, for IRQ ring		*/

	int32		cc_IRQFlags;	/*  Things for IRQ to do	*/

	cursorstate	cc_CursorState;	/*  State of mouse cursor	*/

	GfxBuf		cc_DispBuf;

#if DEBUG_SYNCLOG
#define	NSYNCLOGENTRIES	1024
	int32		cc_SyncLogIdx;
	uint32		cc_SyncLog[NSYNCLOGENTRIES][2];
#endif

} gfx_card_ctl;


/*
 * cc_IRQFlags:  Flag definitions
 */
#define	IRQF_SETFBBASE		(1<<0)
#define	IRQF_MOVECURSOR		(1<<1)
#define	IRQF_SETCLUT		(1<<2)


#endif
