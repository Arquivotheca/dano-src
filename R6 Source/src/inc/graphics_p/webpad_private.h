/*----------------------------------------------------------

	webpad_private.h
	
	by Pierre Raynaud-Richard and Trey Boudreau

	Copyright (c) 1998 by Be Incorporated.
	All Rights Reserved.
	
----------------------------------------------------------*/

#if !defined(_WEBPAD_PRIVATE_H_)
#define _WEBPAD_PRIVATE_H_ 1

#include <Drivers.h>
#include <OS.h>
#include <Accelerant.h>

/*
	This is the info that needs to be shared between the kernel driver and
	the accelerant for the Media-GX video in the dt300 webpad.
*/
#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	sem_id	sem;
	int32	ben;
} benaphore;

#define INIT_BEN(x)		x.sem = create_sem(0, "a gxlv driver benaphore");  x.ben = 0;
#define ACQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define ACQUIRE_BEN_ON_ERROR(x, y) if((atomic_add(&(x.ben), 1)) >= 1) if (acquire_sem(x.sem) != B_OK) y;
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define	DELETE_BEN(x)	delete_sem(x.sem); x.sem = -1; x.ben = 1;

#define GXM_PRIVATE_DATA_MAGIC	0x0202 /* a private driver rev, of sorts */
#define MAX_GXM_DEVICE_NAME_LENGTH 32

#define GKD_MOVE_CURSOR    0x00000001
#define GKD_PROGRAM_CLUT   0x00000002
#define GKD_SET_START_ADDR 0x00000004
#define GKD_SET_CURSOR     0x00000008
#define GKD_HANDLER_INSTALLED 0x80000000

enum {
	GXM_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	GXM_DEVICE_NAME
};

typedef struct {
	vuchar	*regs;
	area_id	regs_area;
	void	*framebuffer;
	void	*framebuffer_dma;
	area_id	fb_area;
	vuchar	*dac;
	area_id	dac_area;
#if GXLV_DO_VBLANK
	sem_id	vblank;
	int32
		flags,
		start_addr;
	uint16
		cursor_x,
		cursor_y,
		first_color,
		color_count;
	uint8
		color_data[3 * 256],
		cursor0[512],
		cursor1[512];
#endif
} shared_info;

typedef struct {
	uint64		fifo_count;
	display_mode
				current_mode;
	frame_buffer_config
				fbc;
	benaphore	engine;
	struct
	{
		uint32	*data;
		uint32	black;
		uint32	white;
		int16	x, y;
		int16	hot_x, hot_y;
		bool	is_visible;
	} cursor;
} accelerant_info;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	do_it;
} gxm_set_bool_state;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	shared_info	*si;
} gxm_get_private_data;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;
} gxm_device_name;


enum {
	GXM_WAIT_FOR_VBLANK = (1 << 0)
};

/*
// GLOBAL VARIABLES USED WITHIN THIS FILE

// SET THE GX BASE ADDRESS
// For the purposes of this file, assume that the base address is set to
// 0x40000000, which is the case for all Cyrix reference systems.  The base
// address can be set to one of three values... 0x40000000, 0x80000000, and
// 0xC0000000, and is determined by the value in the GCR register.

*/
#if 0
#define GX_BASE 		0x40000000
#define GX_REG_BASE		GX_BASE
#define GX_FB_BASE		(GX_BASE | 0x00800000)
/*
// SET THE CX5520 BASE ADDRESS
// To use the same selector for GX and CX5520 registers (in other programs),
// we mapped the CX5520 registers to just above the GX registers.
*/
#define CX5520_BASE		0x40010000
#else
#define GX_FB_BASE		(si->GX_BASE | 0x00800000)

#endif

/*
//----------------------------------------------------------------------
// BEGIN "GXDEFS.H": Normally this is in a separate include file.

//------------------------------------------//
//  GRAPHICS PIPELINE REGISTER DEFINITIONS  //
//------------------------------------------//
*/

#define GP_DST_XCOOR			0x8100		/* x destination origin		*/
#define GP_DST_YCOOR			0x8102		/* y destination origin		*/
#define GP_WIDTH				0x8104		/* pixel width				*/
#define GP_HEIGHT				0x8106		/* pixel height				*/
#define GP_SRC_XCOOR			0x8108		/* x source origin			*/
#define GP_SRC_YCOOR			0x810A		/* y source origin			*/

#define GP_VECTOR_LENGTH		0x8104		/* vector length			*/
#define GP_INIT_ERROR			0x8106		/* vector initial error		*/
#define GP_AXIAL_ERROR			0x8108		/* axial error increment	*/
#define GP_DIAG_ERROR			0x810A		/* diagonal error increment */

#define GP_SRC_COLOR_0			0x810C		/* source color 0			*/
#define GP_SRC_COLOR_1			0x810E		/* source color 1			*/
#define GP_PAT_COLOR_0			0x8110		/* pattern color 0          */
#define GP_PAT_COLOR_1			0x8112		/* pattern color 1          */
#define GP_PAT_COLOR_2			0x8114		/* pattern color 2          */
#define GP_PAT_COLOR_3			0x8116		/* pattern color 3          */
#define GP_PAT_DATA_0			0x8120		/* bits 31:0 of pattern		*/
#define GP_PAT_DATA_1			0x8124		/* bits 63:32 of pattern	*/
#define GP_PAT_DATA_2			0x8128		/* bits 95:64 of pattern	*/
#define GP_PAT_DATA_3			0x812C		/* bits 127:96 of pattern	*/

#define GP_RASTER_MODE			0x8200		/* raster operation			*/
#define GP_VECTOR_MODE			0x8204		/* vector mode register		*/
#define GP_BLIT_MODE			0x8208		/* blit mode register		*/
#define GP_BLIT_STATUS			0x820C		/* blit status register		*/

/*
//------------------------------------//
//  "GP_VECTOR_MODE" BIT DEFINITIONS  //
//------------------------------------//
*/

#define VM_X_MAJOR				0x0000		/* X major vector			*/
#define VM_Y_MAJOR				0x0001		/* Y major vector			*/
#define VM_MAJOR_INC			0x0002		/* positive major axis step */
#define VM_MINOR_INC			0x0004		/* positive minor axis step */
#define VM_READ_DST_FB			0x0008		/* read destination data	*/

/*
//------------------------------------//
//  "GP_RASTER_MODE" BIT DEFINITIONS  //
//------------------------------------//
*/

#define RM_PAT_DISABLE			0x0000		/* pattern is disabled		*/
#define RM_PAT_MONO				0x0100		/* 1BPP pattern expansion	*/
#define RM_PAT_DITHER			0x0200		/* 2BPP pattern expansion	*/
#define RM_PAT_COLOR			0x0300		/* 8BPP or 16BPP pattern	*/
#define RM_PAT_MASK				0x0300		/* mask for pattern mode	*/
#define RM_PAT_TRANSPARENT		0x0400		/* transparent 1BPP pattern	*/
#define RM_SRC_TRANSPARENT		0x0800		/* transparent 1BPP source	*/
#define RM_CLIP_ENABLE			0x1000		/* enables clipping		*/

/*
//------------------------------------//
//  "GP_BLIT_STATUS" BIT DEFINITIONS  //
//------------------------------------//
*/

#define BS_BLIT_BUSY			0x0001		/* blit engine is busy		*/
#define BS_PIPELINE_BUSY		0x0002		/* graphics pipeline is busy*/
#define BS_BLIT_PENDING			0x0004		/* blit pending				*/
#define BC_FLUSH				0x0080		/* flush pipeline requests  */
#define BC_8BPP					0x0000		/* 8BPP mode				*/
#define BC_16BPP				0x0100		/* 16BPP mode				*/
#define BC_FB_WIDTH_1024		0x0000		/* framebuffer width =3D 1024 */
#define BC_FB_WIDTH_2048		0x0200		/* framebuffer width =3D 2048 */

/*
//----------------------------------//
//  "GP_BLIT_MODE" BIT DEFINITIONS  //
//----------------------------------//
*/

#define	BM_READ_SRC_NONE		0x0000		/* source foreground color	*/
#define BM_READ_SRC_FB			0x0001		/* read source from FB		*/
#define BM_READ_SRC_BB0			0x0002		/* read source from BB0		*/
#define BM_READ_SRC_BB1			0x0003		/* read source from BB1		*/
#define BM_READ_SRC_MASK		0x0003		/* read source mask			*/

#define	BM_READ_DST_NONE		0x0000		/* no destination data		*/
#define BM_READ_DST_BB0			0x0008		/* destination from BB0		*/
#define BM_READ_DST_BB1			0x000C		/* destination from BB1		*/
#define BM_READ_DST_FB0			0x0010		/* dest from FB (store BB0) */
#define BM_READ_DST_FB1			0x0014		/* dest from FB (store BB1) */
#define BM_READ_DST_MASK		0x001C		/* read destination mask	*/

#define BM_WRITE_FB				0x0000		/* write to framebuffer		*/
#define	BM_WRITE_MEM			0x0020		/* write to memory			*/
#define BM_WRITE_MASK			0x0020		/* write mask				*/

#define	BM_SOURCE_COLOR			0x0000		/* source is 8BPP or 16BPP	*/
#define BM_SOURCE_EXPAND		0x0040		/* source is 1BPP			*/
#define BM_SOURCE_TEXT			0x00C0		/* source is 1BPP text		*/
#define BM_SOURCE_MASK			0x00C0		/* source mask				*/

#define BM_REVERSE_Y			0x0100		/* reverse Y direction		*/

/*
//-------------------------------------------//
//  DISPLAY CONTROLLER REGISTER DEFINITIONS  //
//-------------------------------------------//
*/

#define DC_UNLOCK				0x8300		/* lock register			*/
#define DC_GENERAL_CFG			0x8304		/* config registers...		*/
#define DC_TIMING_CFG			0x8308
#define DC_OUTPUT_CFG			0x830C

#define DC_FB_ST_OFFSET			0x8310		/* framebuffer start offset */
#define DC_CB_ST_OFFSET			0x8314		/* compression start offset */
#define DC_CURS_ST_OFFSET		0x8318		/* cursor start offset		*/
#define DC_ICON_ST_OFFSET		0x831C		/* icon start offset		*/
#define DC_VID_ST_OFFSET		0x8320		/* video start offset		*/
#define DC_LINE_DELTA			0x8324		/* fb and cb skip counts	*/
#define DC_BUF_SIZE				0x8328		/* fb and cb line size		*/

#define DC_H_TIMING_1			0x8330		/* horizontal timing...		*/
#define DC_H_TIMING_2			0x8334
#define DC_H_TIMING_3			0x8338
#define DC_FP_H_TIMING			0x833C

#define DC_V_TIMING_1			0x8340		/* vertical timing...		*/
#define DC_V_TIMING_2			0x8344
#define DC_V_TIMING_3			0x8348
#define DC_FP_V_TIMING			0x834C

#define DC_CURSOR_X				0x8350		/* cursor x position		*/
#define DC_ICON_X				0x8354		/* HACK - 1.3 definition	*/
#define DC_V_LINE_CNT			0x8354		/* vertical line counter	*/
#define DC_CURSOR_Y				0x8358		/* cursor y position		*/
#define DC_ICON_Y				0x835C		/* HACK - 1.3 definition	*/
#define DC_SS_LINE_COMPARE		0x835C		/* line compare value		*/

#define DC_CURSOR_COLOR			0x8360		/* cursor colors			*/
#define DC_ICON_COLOR			0x8364		/* icon colors				*/
#define DC_BORDER_COLOR			0x8368		/* border color				*/

#define DC_PAL_ADDRESS			0x8370		/* palette address			*/
#define DC_PAL_DATA				0x8374		/* palette data				*/

/*-------------------------------*/
/*  PALETTE ADDRESS DEFINITIONS  */
/*-------------------------------*/

#define PAL_CURSOR_COLOR_0		0x100
#define PAL_CURSOR_COLOR_1		0x101
#define PAL_ICON_COLOR_0		0x102
#define PAL_ICON_COLOR_1		0x103
#define PAL_OVERSCAN_COLOR		0x104

/*----------------------*/
/*  DC BIT DEFINITIONS  */
/*----------------------*/

#define DC_UNLOCK_VALUE		0x00004758		/* used to unlock DC regs	*/

#define DC_GCFG_DFLE		0x00000001		/* display FIFO load enable */
#define DC_GCFG_CURE		0x00000002		/* cursor enable			*/
#define DC_GCFG_ICNE		0x00000004		/* HACK - 1.3 definition	*/
#define DC_GCFG_PLNO		0x00000004		/* planar offset LSB		*/
#define DC_GCFG_VIDE		0x00000008		/* HACK - 1.3 definition    */
#define DC_GCFG_PPC			0x00000008		/* pixel pan compatibility  */
#define DC_GCFG_CMPE		0x00000010		/* compression enable       */
#define DC_GCFG_DECE		0x00000020		/* decompression enable     */
#define DC_GCFG_DCLK_MASK	0x000000C0		/* dotclock multiplier      */
#define DC_GCFG_DCLK_POS	6				/* dotclock multiplier      */
#define DC_GCFG_DFHPSL_MASK	0x00000F00		/* FIFO high-priority start */
#define DC_GCFG_DFHPSL_POS	8				/* FIFO high-priority start */
#define DC_GCFG_DFHPEL_MASK	0x0000F000		/* FIFO high-priority end   */
#define DC_GCFG_DFHPEL_POS	12				/* FIFO high-priority end   */
#define DC_GCFG_CIM_MASK	0x00030000		/* compressor insert mode   */
#define DC_GCFG_CIM_POS		16				/* compressor insert mode   */
#define DC_GCFG_FDTY		0x00040000		/* frame dirty mode         */
#define DC_GCFG_RTPM		0x00080000		/* real-time perf. monitor  */
#define DC_GCFG_DAC_RS_MASK	0x00700000		/* DAC register selects     */
#define DC_GCFG_DAC_RS_POS	20				/* DAC register selects     */
#define DC_GCFG_CKWR		0x00800000		/* clock write              */
#define DC_GCFG_LDBL		0x01000000		/* line double              */
#define DC_GCFG_DIAG		0x02000000		/* FIFO diagnostic mode     */
#define DC_GCFG_CH4S		0x04000000      /* sparse refresh mode		*/
#define DC_GCFG_SSLC		0x08000000		/* enable line compare		*/
#define DC_GCFG_FBLC		0x10000000		/* enable fb offset compare	*/
#define DC_GCFG_DFCK		0x20000000		/* divide flat-panel clock  */
#define DC_GCFG_DPCK		0x40000000		/* divide pixel clock       */
#define DC_GCFG_DDCK		0x80000000		/* divide dot clock         */

#define DC_TCFG_FPPE		0x00000001		/* flat-panel power enable  */
#define DC_TCFG_HSYE		0x00000002		/* horizontal sync enable   */
#define DC_TCFG_VSYE		0x00000004		/* vertical sync enable     */
#define DC_TCFG_BLKE		0x00000008		/* blank enable				*/
#define DC_TCFG_DDCK		0x00000010		/* DDC clock                */
#define DC_TCFG_TGEN		0x00000020		/* timing generator enable  */
#define DC_TCFG_VIEN		0x00000040		/* vertical interrupt enable*/
#define DC_TCFG_BLNK		0x00000080		/* blink enable             */
#define DC_TCFG_CHSP		0x00000100		/* horizontal sync polarity */
#define DC_TCFG_CVSP		0x00000200		/* vertical sync polarity   */
#define DC_TCFG_FHSP		0x00000400		/* panel horz sync polarity */
#define DC_TCFG_FVSP		0x00000800		/* panel vert sync polarity */
#define DC_TCFG_FCEN		0x00001000		/* flat-panel centering     */
#define DC_TCFG_CDCE		0x00002000		/* HACK - 1.3 definition	*/
#define DC_TCFG_PLNR		0x00002000		/* planar mode enable		*/
#define DC_TCFG_INTL		0x00004000		/* interlace scan           */
#define DC_TCFG_PXDB		0x00008000		/* pixel double             */
#define DC_TCFG_BKRT		0x00010000		/* blink rate               */
#define DC_TCFG_PSD_MASK	0x000E0000		/* power sequence delay     */
#define DC_TCFG_PSD_POS		17				/* power sequence delay     */
#define DC_TCFG_DDCI		0x08000000		/* DDC input (RO)           */
#define DC_TCFG_SENS		0x10000000		/* monitor sense (RO)       */
#define DC_TCFG_DNA			0x20000000		/* display not active (RO)  */
#define DC_TCFG_VNA			0x40000000		/* vertical not active (RO) */
#define DC_TCFG_VINT		0x80000000		/* vertical interrupt (RO)  */

#define DC_OCFG_8BPP		0x00000001		/* 8/16 bpp select          */
#define DC_OCFG_555			0x00000002		/* 16 bpp format            */
#define DC_OCFG_PCKE		0x00000004		/* PCLK enable              */
#define DC_OCFG_FRME		0x00000008		/* frame rate mod enable    */
#define DC_OCFG_DITE		0x00000010		/* dither enable            */
#define DC_OCFG_2PXE		0x00000020		/* 2 pixel enable           */
#define DC_OCFG_2XCK		0x00000040		/* 2 x pixel clock          */
#define DC_OCFG_2IND		0x00000080		/* 2 index enable           */
#define DC_OCFG_34ADD		0x00000100		/* 3- or 4-bit add          */
#define DC_OCFG_FRMS		0x00000200		/* frame rate mod select    */
#define DC_OCFG_CKSL		0x00000400		/* clock select             */
#define DC_OCFG_PRMP		0x00000800		/* palette re-map           */
#define DC_OCFG_PDEL		0x00001000		/* panel data enable low    */
#define DC_OCFG_PDEH		0x00002000		/* panel data enable high   */
#define DC_OCFG_CFRW		0x00004000		/* comp line buffer r/w sel */
#define DC_OCFG_DIAG		0x00008000		/* comp line buffer diag    */

/*---------------------------*/
/*  MC REGISTER DEFINITIONS  */
/*---------------------------*/

#define MC_GBASE_ADD		0x8414			/* Graphics memory base address	*/
#define MC_DR_ADD			0x8418			/* Dirty RAM address index		*/
#define MC_DR_ACC			0x841C			/* Dirty RAM memory access		*/
#define MC_RAMDAC_ACC		0x8420			/* RAMDAC register access		*/

/*-------------------------------------*/
/*  CPU REGISTER REGISTER DEFINITIONS  */
/*-------------------------------------*/

#define BB0_BASE		0xFFFFFF0C			/* Blit buffer 0 base */
#define BB1_BASE		0xFFFFFF1C			/* Blit buffer 1 base */

/*
// END "GXDEFS.H": Normally this is in a separate include file.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// BEGIN "CX5520.H"
*/
#define CX5520_CFG_VIDEO_BASE		0x1C			// video base register

// VIDEO REGISTER OFFSETS

#define CX5520_VIDEO_CONFIG			0x0000			// video configuration
#define CX5520_DISPLAY_CONFIG		0x0004			// display configuration
#define CX5520_VIDEO_X_POS			0x0008			// video X position
#define CX5520_VIDEO_Y_POS			0x000C			// video Y position
#define CX5520_VIDEO_SCALE			0x0010			// video scale
#define CX5520_VIDEO_COLOR_KEY		0x0014			// video color key
#define CX5520_VIDEO_COLOR_MASK   	0x0018			// video color mask
#define CX5520_PALETTE_ADDRESS		0x001C			// palette address
#define CX5520_PALETTE_DATA			0x0020			// palette data

// DISPLAY CONFIGURATION BIT DEFINITIONS

#define CX5520_DCFG_DIS_EN				0x00000001
#define CX5520_DCFG_HSYNC_EN			0x00000002
#define CX5520_DCFG_VSYNC_EN			0x00000004
#define CX5520_DCFG_DAC_BL_EN			0x00000008
#define CX5520_DCFG_DAC_PWDNX			0x00000020
#define CX5520_DCFG_FP_PWR_EN			0x00000040
#define CX5520_DCFG_FP_DATA_EN			0x00000080
#define CX5520_DCFG_CRT_HSYNC_POL 		0x00000100
#define CX5520_DCFG_CRT_VSYNC_POL 		0x00000200
#define CX5520_DCFG_FP_HSYNC_POL  		0x00000400
#define CX5520_DCFG_FP_VSYNC_POL  		0x00000800
#define CX5520_DCFG_XGA_FP		  		0x00001000
#define CX5520_DCFG_FP_DITH_EN			0x00002000
#define CX5520_DCFG_CRT_SYNC_SKW_MASK	0x0001C000
#define CX5520_DCFG_CRT_SYNC_SKW_POS	14
#define CX5520_DCFG_PWR_SEQ_DLY_MASK	0x000E0000
#define CX5520_DCFG_PWR_SEQ_DLY_POS		17
#define CX5520_DCFG_PWR_SEQ_DLY_MASK	0x000E0000
#define CX5520_DCFG_VG_CK				0x00100000
#define CX5520_DCFG_GV_PAL_BYP			0x00200000
#define CX5520_DCFG_DDC_SCL				0x00400000
#define CX5520_DCFG_DDC_SDA				0x00800000
#define CX5520_DCFG_DDC_OE				0x01000000

// END "CX5520.H": Normally this is in a separate include file.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// MACROS
//
// THIS FILE WAS DEVELOPED AND TESTED AS PART OF A 32-BIT WINDOWS
// APPLICATION.  CHANGES MAY BE REQUIRED TO PROPERLY ADDRESS THE GX
// MEMORY MAPPED REGISTER AND FRAMEBUFFER REGIONS.

#define WAIT_BUSY(base) \
	while ((*((volatile unsigned long *) (base+GP_BLIT_STATUS))) & BS_BLIT_BUSY)

#define WAIT_PENDING(base) \
	while ((*((volatile unsigned long *) (base+GP_BLIT_STATUS))) & BS_BLIT_PENDING)

#define READ_REG32(base, offset) \
	*((volatile unsigned long *) (base+offset))

#define WRITE_REG16(base, offset, value) \
	*((volatile unsigned short *) (base+offset)) = value

#define WRITE_REG32(base, offset, value) \
	*((volatile unsigned long *) (base+offset)) = value

// END MACROS
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// VIDEO MODE TABLES:
//
// The following structure is used to conatain the information used when
// setting the various display modes.
//

typedef struct tagDISPLAYMODE
{
	// DISPLAY MODE PARAMETERS

	int xres;
	int yres;
	int	bpp;
	int	hz;

	// VALUES USED TO SET THE GX DISPLAY CONTROLLER

	unsigned long gcfg;
	unsigned long tcfg;
	unsigned long ocfg;
	unsigned long fb_offset;
	unsigned long cb_offset;
	unsigned long curs_offset;
	unsigned long line_delta;
	unsigned long buffer_size;
	unsigned long htiming1;
	unsigned long htiming2;
	unsigned long htiming3;
	unsigned long fp_htiming;
	unsigned long vtiming1;
	unsigned long vtiming2;
	unsigned long vtiming3;
	unsigned long fp_vtiming;

	// VALUES USED TO SET CLOCK FREQUENCY FOR THE ICS5342 RAMDAC

	unsigned char ics5342_m;
	unsigned char ics5342_n;

	// VALUES USED TO SET CLOCK FREQUENCY FOR THE CX5520=20

	unsigned long cx5520_clock;

} DISPLAYMODE;


#if defined(__cplusplus)
}
#endif

#endif /* _WEBPAD_PRIVATE_H_ */
