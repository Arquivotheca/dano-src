#if !defined(_ATI_PRIVATE_H_)
#define _ATI_PRIVATE_H_ 1

#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

/*
	This is the info that needs to be shared between the kernel driver and
	the accelerant for the ATI video cards.
*/
#if defined(__cplusplus)
extern "C" {
#endif

/*
	3DRage, RageII, LT, and LTPro Registers.
	
	32bit word offsets from base of register area.
	Because register block 0 follows register block 1 in the address
	space, all of register block 0's indexes start with 0x01xx and
	all of register block 1's indexes start with 0x00xx.  Neat, eh?
*/
enum
{
	OVERLAY_Y_X_START          = 0x0000,
	OVERLAY_Y_X_END            = 0x0001,
	OVERLAY_VIDEO_KEY_CLR      = 0x0002,
	OVERLAY_VIDEO_KEY_MSK      = 0x0003,
	OVERLAY_GRAPHICS_KEY_CLR   = 0x0004,
	OVERLAY_GRAPHICS_KEY_MSK   = 0x0005,
	OVERLAY_KEY_CNTL           = 0x0006,
	OVERLAY_SCALE_INC          = 0x0008,
	OVERLAY_SCALE_CNTL         = 0x0009,
	SCALER_HEIGHT_WIDTH        = 0x000A,
	SCALER_TEST                = 0x000B,
	SCALER_BUF0_OFFSET         = 0x000D,
	SCALER_BUF1_OFFSET         = 0x000E,
	SCALER_BUF_PITCH           = 0x000F,
	CAPTURE_START_END          = 0x0010,
	CAPTURE_X_WIDTH            = 0x0011,
	VIDEO_FORMAT               = 0x0012,
	VBI_START_END              = 0x0013,
	CAPTURE_CONFIG             = 0x0014,
	TRIG_CNTL                  = 0x0015,
	OVERLAY_EXCLUSIVE_HORZ     = 0x0016,
	OVERLAY_EXCLUSIVE_VERT     = 0x0017,
	VBI_WIDTH                  = 0x0018,
	CAPTURE_DEBUG              = 0x0019,
	VIDEO_SYNC_TEST            = 0x001A,
	CAPTURE_BUF0_OFFSET        = 0x0020,
	CAPTURE_BUF1_OFFSET        = 0x0021,
	ONESHOT_BUF_OFFSET         = 0x0022,
	SCALER_COLOUR_CNTL         = 0x0054,
	SCALER_H_COEFF0            = 0x0055,
	SCALER_H_COEFF1            = 0x0056,
	SCALER_H_COEFF2            = 0x0057,
	SCALER_H_COEFF3            = 0x0058,
	SCALER_H_COEFF4            = 0x0059,
	GUI_CNTL                   = 0x005E,	/* yes, this really is in register block 1 */
	SCALER_BUF0_OFFSET_U       = 0x0075,
	SCALER_BUF0_OFFSET_V       = 0x0076,
	SCALER_BUF0_1FFSET_U       = 0x0077,
	SCALER_BUF0_1FFSET_V       = 0x0078,
	
	CRTC_H_TOTAL_DISP     = 0x0100,
	CRTC_H_SYNC_STRT_WID  = 0x0101,
	CRTC_V_TOTAL_DISP     = 0x0102,
	CRTC_V_SYNC_STRT_WID  = 0x0103,
	CRTC_VLINE_CRNT_VLINE = 0x0104,
	CRTC_OFF_PITCH        = 0x0105,
	CRTC_INT_CNTL         = 0x0106,
	CRTC_GEN_CNTL         = 0x0107,
	DSP_CONFIG            = 0x0108,
	DSP_ON_OFF            = 0x0109,
	TIMER_CONFIG          = 0x010A,
	MEM_BUF_CNTL          = 0x010B,
	
	MEM_ADDR_CONFIG       = 0x010D,
	CRT_TRAP              = 0x010E,
	I2C_CNTL_0            = 0x010F,
	OVR_CLR               = 0x0110,
	OVR_WID_LEFT_RIGHT    = 0x0111,
	OVR_WID_TOP_BOTTOM    = 0x0112,
	VGA_DSP_CONFIG        = 0x0113,
	VGA_DSP_ON_OFF        = 0x0114,
	DSP2_CONFIG           = 0x0115,
	DSP2_ON_OFF           = 0x0116,
	CRTC2_OFF_PITCH       = 0x0117,
	CUR_CLR0              = 0x0118,
	CUR_CLR1              = 0x0119,
	CUR_OFFSET            = 0x011A,
	CUR_HORZ_VERT_POSN    = 0x011B,
	CUR_HORZ_VERT_OFF     = 0x011C,
	TV_OUT_INDEX          = 0x011D,	
	GP_IO                 = 0x011E,
	HW_DEBUG              = 0x011F,
	SCRATCH_REG0          = 0x0120,
	SCRATCH_REG1          = 0x0121,
	SCRATCH_REG2          = 0x0122,
	SCRATCH_REG3          = 0x0123,
	CLOCK_CNTL            = 0x0124,
	CONFIG_STAT1          = 0x0125,
	CONFIG_STAT2          = 0x0126,
	TV_OUT_DATA           = 0x0127,
	BUS_CNTL              = 0x0128,
	LCD_INDEX             = 0x0129,
	LCD_DATA              = 0x012A,
	EXT_MEM_CNTL          = 0x012B,
	MEM_CNTL              = 0x012C,
	MEM_VGA_WP_SEL        = 0x012D,
	MEM_VGA_RP_SEL        = 0x012E,
	I2C_CNTL_1            = 0x012F,
	DAC_REGS              = 0x0130,
	DAC_CNTL              = 0x0131,
	EXT_DAC_REGS          = 0x0132,
	
	GEN_TEST_CNTL         = 0x0134,
	CUSTOM_MACRO_CNTL     = 0x0135,
	
	CONFIG_CNTL           = 0x0137,
	CONFIG_CHIP_ID        = 0x0138,
	CONFIG_STAT0          = 0x0139,
	CRC_SIG               = 0x013A,

	DST_OFF_PITCH         = 0x0140,
	DST_X                 = 0x0141,
	DST_Y                 = 0x0142,
	DST_Y_X               = 0x0143,
	DST_WIDTH             = 0x0144,
	DST_HEIGHT            = 0x0145,
	DST_HEIGHT_WIDTH      = 0x0146,
	DST_X_WIDTH           = 0x0147,
	DST_BRES_LNTH         = 0x0148,
	DST_BRES_ERR          = 0x0149,
	DST_BRES_INC          = 0x014A,
	DST_BRES_DEC          = 0x014B,
	DST_CNTL              = 0x014C,
	TRAIL_BRES_ERR        = 0x014E,
	TRAIL_BRES_INC        = 0x014F,
	TRAIL_BRES_DEC        = 0x0150,
	LEAD_BRES_LNTH        = 0x0151,
	Z_OFF_PITCH           = 0x0152,
	Z_CNTL                = 0x0153,
	ALPHA_TST_CNTL        = 0x0154,
	SRC_OFF_PITCH         = 0x0160,
	SRC_Y_X               = 0x0163,
	SRC_WIDTH1            = 0x0164,
	SRC_CNTL              = 0x016D,
	SCALE_OFF             = 0x0170,
	SCALE_WIDTH           = 0x0177,
	SCALE_HEIGHT          = 0x0178,
	SCALE_PITCH           = 0x017B,
	SCALE_X_INC           = 0x017C,
	SCALE_Y_INC           = 0x017D,
	SCALE_VACC            = 0x017E,
	SCALE_3D_CNTL         = 0x017F,
	HOST_DATA_0           = 0x0180,
	HOST_DATA_1           = 0x0181,
	HOST_DATA_2           = 0x0182,
	HOST_DATA_3           = 0x0183,
	HOST_DATA_4           = 0x0184,
	HOST_DATA_5           = 0x0185,
	HOST_DATA_6           = 0x0186,
	HOST_DATA_7           = 0x0187,
	HOST_DATA_8           = 0x0188,
	HOST_DATA_9           = 0x0189,
	HOST_DATA_A           = 0x018A,
	HOST_DATA_B           = 0x018B,
	HOST_DATA_C           = 0x018C,
	HOST_DATA_D           = 0x018D,
	HOST_DATA_E           = 0x018E,
	HOST_DATA_F           = 0x018F,
	HOST_CNTL             = 0x0190,
	BM_HOSTDATA           = 0x0191,
	BM_ADDR               = 0x0192, /* These ARE */
	BM_DATA               = 0x0192, /* the same offset */
	BM_GUI_TABLE_CMD      = 0x0193,
	SC_LEFT_RIGHT         = 0x01AA,
	SC_TOP_BOTTOM         = 0x01AD,
	DP_FRGD_CLR           = 0x01B1,
	DP_WRITE_MASK         = 0x01B2,
	DP_PIX_WIDTH          = 0x01B4,
	DP_MIX                = 0x01B5,
	DP_SRC                = 0x01B6,
	CLR_CMP_CLR           = 0x01C0,
	CLR_CMP_MASK          = 0x01C1,
	CLR_CMP_CNTL          = 0x01C2,
	FIFO_STAT             = 0x01C4,
	GUI_TRAJ_CNTL         = 0x01CC,
	GUI_STAT              = 0x01CE,
	TEX_CNTL              = 0x01DD,
	SCALE_OFF_ACC         = 0x01E2,
	SCALE_HACC            = 0x01F2
};
/* LCD panel related registers */
/* index register is LCD_INDEX:0_29 */
/* data register is LCD_DATA:0_2A */
enum {
	CONFIG_PANEL = 0,
	LCD_GEN_CTRL,
	DSTN_CONTROL,
	HFB_PITCH_ADDR,
	HORZ_STRETCHING,
	VERT_STRETCHING,
	EXT_VERT_STRETCHING,
	LT_GIO,
	POWER_MANAGEMENT,
	ZVGPIO
};
/* TV out support registers */
/* index register is TV_OUT_INDEX:0_1D */
/* data registers is TV_OUT_DATA:0_27 */
enum {
	TV_MASTER_CNTL = 0x10,
	TV_RGB_CNTL = 0x12,
	TV_SYNC_CNTL = 0x14,
	TV_HTOTAL = 0x20,
	TV_HDISP,
	TV_HSIZE,
	TV_HSTART,
	TV_HCOUNT,
	TV_VTOTAL,
	TV_VDISP,
	TV_VCOUNT,
	TV_FTOTAL,
	TV_FCOUNT,
	TV_FRESTART,
	TV_HRESTART,
	TV_VRESTART,
	TV_HOST_READ_DATA = 0x60,
	TV_HOST_WRITE_DATA,
	TV_HOST_RD_WR_CNTL,
	TV_VSCALER_CNTL = 0x70,
	TV_TIMING_CNTL,
	TV_GAMMA_CNTL,
	TV_Y_FALL_CNTL,
	TV_Y_RISE_CNTL,
	TV_Y_SAW_TOOTH_CNTL,
	TV_MODULATOR_CNTL1 = 0x80,
	TV_MODULATOR_CNTL2,
	TV_PRE_DAC_MUX_CNTL = 0x90,
	TV_DAC_CNTL = 0xA0,
	TV_CRC_CNTL = 0xB0,
	TV_VIDEO_PORT_SIG,
	TV_VBI_CC_CNTL = 0xB8,
	TV_VBI_EDS_CNTL,
	TV_VBI_20BIT_CNTL,
	TV_VBI_DTO_CNTL = 0xBD,
	TV_VBI_LEVEL_CNTL,
	TV_UV_ADR = 0xC0,
	TV_FIFO_TEST_CNTL
};



#define ATI8(a) *((vuchar *)(regs + (a)))
#define ATI32(a) *((vulong *)(regs + (a)))
#define ATI32W(a, d) ATI32(a) = (d)
#define ATI32R(a, d) (d) = ATI32(a)

#define DAC8(a) ATI8(RAMDAC_OFFSET + (a))
#define DAC8R(a, d) (d) = DAC8(a)
#define DAC8W(a, d) DAC8(a) = (d)
#define DAC8IW(a, d) DAC8W(TVP3026_INDEX, (a)); DAC8W(TVP3026_DATA, (d))
#define DAC8IR(a, d) DAC8W(TVP3026_INDEX, (a)); DAC8R(TVP3026_DATA, (d))
#define DAC8POLL(a, d, m) while ((DAC8(a) & (m)) != ((d) & (m)))
#define STORM8(a) ATI8(STORM_OFFSET + (a))
#define STORM8R(a, d) (d) = STORM8(a)
#define STORM8W(a, d) STORM8(a) = (d)
#define STORM8POLL(a, d, m) while ((STORM8(a) & (m)) != ((d) & (m)))
#define STORM32(a) ATI32(STORM_OFFSET + (a))
#define STORM32R(a, d) (d) = STORM32(a)
#define STORM32W(a, d) STORM32(a) = (d)
#define STORM32POLL(a, d, m) while ((STORM32(a) & (m)) != ((d) & (m)))


#define ATI_PRIVATE_DATA_MAGIC	0xbaab /* a private driver rev, of sorts */

#define MAX_ATI_DEVICE_NAME_LENGTH 32

#define AKD_MOVE_CURSOR    0x00000001
#define AKD_PROGRAM_CLUT   0x00000002
#define AKD_SET_START_ADDR 0x00000004
#define AKD_SET_CURSOR     0x00000008
#define AKD_HANDLER_INSTALLED 0x80000000

enum {
	ATI_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	ATI_GET_PCI,
	ATI_SET_PCI,
	ATI_DEVICE_NAME,
	ATI_RUN_INTERRUPTS
};
typedef struct {
	uint16
		vendor_id,
		device_id;
	uint8
		revision,
		interrupt_line;
	vuint32	*regs;
	area_id	regs_area;
	area_id	fb_area;
	void	*framebuffer;
	void	*framebuffer_pci;
	area_id	rom_area;
	void	*rom;
	sem_id	vblank;
	int32
		flags,
		start_addr;
	uint16
		cursor_x,
		cursor_y,
		first_color,
		color_count;
	bigtime_t
		refresh_period,
		blank_period;
#if 0
	uint8
		color_data[3 * 256],
		cursor0[512],
		cursor1[512];
#endif
} shared_info;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	uint32
		offset,
		size,
		value;
} ati_get_set_pci;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	bool	do_it;
} ati_set_bool_state;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	shared_info	*si;
} ati_get_private_data;

typedef struct {
	uint32	magic;		/* magic number to make sure the caller groks us */
	char	*name;
} ati_device_name;

enum {
	ATI_WAIT_FOR_VBLANK = (1 << 0)
};

#if defined(__cplusplus)
}
#endif

#endif /* _ATI_PRIVATE_H_ */
