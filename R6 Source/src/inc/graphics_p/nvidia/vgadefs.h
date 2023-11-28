/* :ts=8 bk=0
 *
 * vgadefs.h:	VGA register and bit definitions.
 *
 * Leo L. Schwab					2000.10.04
 *
 * Copyright 2000 Be Incorporated.
 */
#ifndef	__VGADEFS_H
#define	__VGADEFS_H

#ifndef	__BITTWIDDLE_H
#include "bittwiddle.h"
#endif


/*
 * The stupid-ass VGA "standard" decided to place the same register at
 * different offsets depending on whether you were reading or writing it.
 * These two structures define the correct offsets for reads and writes.
 * Then just specify the correct union when reading or writing, and you'll
 * get the right offset.  Poof!
 */
struct vgaregs_read {
	vuint8	__reserved0[0x03b4];	/*	0x0000 - 0x03B3		*/
	vuint8	VGAMono_CR_Idx;		/*  0x03B4			*/
	vuint8	VGAMono_CR_Val;		/*  0x03B5			*/
	vuint8	__reserved1[0x0004];	/*	0x03B6 - 0x03B9		*/
	vuint8	VGAMono_ST01;		/*  0x03BA			*/
	vuint8	__reserved2[0x0005];	/*	0x03BB - 0x03BF		*/
	vuint8	VGA_AR_IdxValW;		/*  0x03C0			*/
	vuint8	VGA_AR_ValR;		/*  0x03C1			*/
	vuint8	VGA_ST00;		/*  0x03C2			*/
	vuint8	VGA_Enable;		/*  0x03C3 (???)		*/
	vuint8	VGA_SR_Idx;		/*  0x03C4			*/
	vuint8	VGA_SR_Val;		/*  0x03C5			*/
	vuint8	VGA_DACMask;		/*  0x03C6			*/
	vuint8	VGA_DACState;		/*  0x03C7			*/
	vuint8	VGA_DACWIdx;		/*  0x03C8			*/
	vuint8	VGA_DACVal;		/*  0x03C9			*/
	vuint8	VGA_FCR;		/*  0x03CA			*/
	vuint8	__reserved3;		/*	0x03CB			*/
	vuint8	VGA_Misc;		/*  0x03CC			*/
	vuint8	__reserved4;		/*	0x03CD			*/
	vuint8	VGA_GR_Idx;		/*  0x03CE			*/
	vuint8	VGA_GR_Val;		/*  0x03CF			*/
	vuint8	__reserved5[0x0004];	/*	0x03D0 - 0x03D3		*/
	vuint8	VGA_CR_Idx;		/*  0x03D4			*/
	vuint8	VGA_CR_Val;		/*  0x03D5			*/
	vuint8	__reserved6[0x0004];	/*	0x03D6 - 0x03D9		*/
	vuint8	VGA_ST01;		/*  0x03DA			*/
	vuint8	__reserved7[0x0c25];	/*	0x03DB - 0x0FFF		*/
};

struct vgaregs_write {
	vuint8	__reserved0[0x03b4];	/*	0x0000 - 0x03B3		*/
	vuint8	VGAMono_CR_Idx;		/*  0x03B4			*/
	vuint8	VGAMono_CR_Val;		/*  0x03B5			*/
	vuint8	__reserved1[0x0004];	/*	0x03B6 - 0x03B9		*/
	vuint8	VGAMono_FCR;		/*  0x03BA			*/
	vuint8	__reserved2[0x0005];	/*	0x03BB - 0x03BF		*/
	vuint8	VGA_AR_IdxValW;		/*  0x03C0			*/
	vuint8	__reserved3;		/*	0x03C1			*/
	vuint8	VGA_Misc;		/*  0x03C2			*/
	vuint8	VGA_Enable;		/*  0x03C3 (???)		*/
	vuint8	VGA_SR_Idx;		/*  0x03C4			*/
	vuint8	VGA_SR_Val;		/*  0x03C5			*/
	vuint8	VGA_DACMask;		/*  0x03C6			*/
	vuint8	VGA_DACRIdx;		/*  0x03C7			*/
	vuint8	VGA_DACWIdx;		/*  0x03C8			*/
	vuint8	VGA_DACVal;		/*  0x03C9			*/
	vuint8	__reserved4;		/*	0x03CA			*/
	vuint8	__reserved5;		/*	0x03CB			*/
	vuint8	__reserved6;		/*	0x03CC			*/
	vuint8	__reserved7;		/*	0x03CD			*/
	vuint8	VGA_GR_Idx;		/*  0x03CE			*/
	vuint8	VGA_GR_Val;		/*  0x03CF			*/
	vuint8	__reserved8[0x0004];	/*	0x03D0 - 0x03D3		*/
	vuint8	VGA_CR_Idx;		/*  0x03D4			*/
	vuint8	VGA_CR_Val;		/*  0x03D5			*/
	vuint8	__reserved9[0x0004];	/*	0x03D6 - 0x03D9		*/
	vuint8	VGA_FCR;		/*  0x03DA			*/
	vuint8	__reserved10[0x0c25];	/*	0x03DB - 0x0FFF		*/
};
#define	VGA_AR_Idx	VGA_AR_IdxValW
#define	VGA_AR_ValW	VGA_AR_IdxValW


/*
 * Use this union to access the registers based on whether you're reading
 * or writing them.
 */
typedef union vgaregs {
	struct vgaregs_read	r;
	struct vgaregs_write	w;
} vgaregs;


/*****************************************************************************
 * "Standard" VGA register indices and bit definitions.
 ****
 * Sequencer bit and index definitions
 * I've decided to use the XFree86/NVidia bit definition form, even though
 * it makes the use of ctags difficult.
 */
#define	SR_RESET				0x00
#define	SR_RESET_SYNC				1:1
#define	SR_RESET_SYNC_ENABLE			0
#define	SR_RESET_SYNC_DISABLE			1
#define	SR_RESET_ASYNC				0:0
#define	SR_RESET_ASYNC_ENABLE			0
#define	SR_RESET_ASYNC_DISABLE			1

#define	SR_CLOCK				0x01
#define	SR_CLOCK_VIDDISABLE			5:5
#define	SR_CLOCK_SHIFTLOAD4			4:4
#define	SR_CLOCK_SHIFTLOAD4_1			0
#define	SR_CLOCK_SHIFTLOAD4_4			1
#define	SR_CLOCK_CLKDIV				3:3
#define	SR_CLOCK_CLKDIV_DIV1			0
#define	SR_CLOCK_CLKDIV_DIV2			1
#define	SR_CLOCK_SHIFTLOAD2			2:2
#define	SR_CLOCK_SHIFTLOAD2_1			0
#define	SR_CLOCK_SHIFTLOAD2_2			1
#define	SR_CLOCK_TXCELLWIDTH			0:0
#define	SR_CLOCK_TXCELLWIDTH_9			0	/*  Backwards from  */
#define	SR_CLOCK_TXCELLWIDTH_8			1	/* other VGA cores  */

#define	SR_PLANES				0x02
#define	SR_PLANES_MASK				3:0

#define	SR_FONTBASE				0x03
#define	SR_FONTBASE_SETB_OFFLO			5:5
#define	SR_FONTBASE_SETB_OFFLO_0K		0
#define	SR_FONTBASE_SETB_OFFLO_8K		1
#define	SR_FONTBASE_SETA_OFFLO			4:4
#define	SR_FONTBASE_SETA_OFFLO_0K		0
#define	SR_FONTBASE_SETA_OFFLO_8K		1
#define	SR_FONTBASE_SETB_OFFHI			3:2
#define	SR_FONTBASE_SETB_OFFHI_0K		0
#define	SR_FONTBASE_SETB_OFFHI_16K		1
#define	SR_FONTBASE_SETB_OFFHI_32K		2
#define	SR_FONTBASE_SETB_OFFHI_48K		3
#define	SR_FONTBASE_SETA_OFFHI			1:0
#define	SR_FONTBASE_SETA_OFFHI_0K		0
#define	SR_FONTBASE_SETA_OFFHI_16K		1
#define	SR_FONTBASE_SETA_OFFHI_32K		2
#define	SR_FONTBASE_SETA_OFFHI_48K		3

#define	SR_MODE					0x04
#define	SR_MODE_CHAIN4				3:3
#define	SR_MODE_ODDEVEN				2:2
#define	SR_MODE_ODDEVEN_ALTERNATE		0
#define	SR_MODE_ODDEVEN_SEQUENTIAL		1
#define	SR_MODE_EXTMEM				1:1
#define	SR_MODE_EXTMEM_DISABLE			0
#define	SR_MODE_EXTMEM_ENABLE			1

#define	SR_HCOUNTRESET				0x07


/*****************************************************************************
 * Graphics controller register indices.
 */
#define	GR_SETRESET				0x00
#define	GR_SETRESET_PLANEMASK			3:0

#define	GR_SETRESETENABLE			0x01
#define	GR_SETRESETENABLE_PLANEENABLEMASK	3:0

#define	GR_COLORCMP				0x02
#define	GR_COLORCMP_PLANEMASK			3:0

#define	GR_ROTATE				0x03
#define	GR_ROTATE_WRITEFUNC			4:3
#define	GR_ROTATE_WRITEFUNC_COPY		0
#define	GR_ROTATE_WRITEFUNC_AND			1
#define	GR_ROTATE_WRITEFUNC_OR			2
#define	GR_ROTATE_WRITEFUNC_XOR			3
#define	GR_ROTATE_COUNT				2:0

#define	GR_READPLANE				0x04
#define	GR_READPLANE_PLANE			1:0
#define	GR_READPLANE_PLANE_0			0
#define	GR_READPLANE_PLANE_1			1
#define	GR_READPLANE_PLANE_2			2
#define	GR_READPLANE_PLANE_3			3

#define	GR_MODE					0x05
#define	GR_MODE_SHIFTCTL			6:5
#define	GR_MODE_SHIFTCTL_16COLOR		0
#define	GR_MODE_SHIFTCTL_4COLOR			1
#define	GR_MODE_SHIFTCTL_256COLOR		2
#define	GR_MODE_ODDEVEN				4:4
#define	GR_MODE_ODDEVEN_SEQUENTIAL		0
#define	GR_MODE_ODDEVEN_ALTERNATE		1
#define	GR_MODE_READMODE			3:3
#define	GR_MODE_READMODE_NORMAL			0
#define	GR_MODE_READMODE_COMPARE		1
#define	GR_MODE_WRITEMODE			1:0

#define	GR_MISC					0x06
#define	GR_MISC_MEMMAP				3:2
#define	GR_MISC_MEMMAP_A0_BF			0
#define	GR_MISC_MEMMAP_A0_AF			1
#define	GR_MISC_MEMMAP_B0_B7			2
#define	GR_MISC_MEMMAP_B8_BF			3
#define	GR_MISC_CHAINODDEVEN			1:1
#define	GR_MISC_CHAINODDEVEN_NORMAL		0
#define	GR_MISC_CHAINODDEVEN_ALTERNATE		1
#define	GR_MISC_DISPMODE			0:0
#define	GR_MISC_DISPMODE_TEXT			0
#define	GR_MISC_DISPMODE_GRAPHICS		1

#define	GR_COLORNC				0x07
#define	GR_COLORNC_PLANEMASK			3:0

#define	GR_LATCHMASK				0x08


/*****************************************************************************
 * Attribute register indices and bit definitions.
 */
#define	AR_IDX_ACCESS				5:5
#define	AR_IDX_ACCESS_UNLOCK			0
#define	AR_IDX_ACCESS_LOCK			1
#define	AR_IDX_IDX				4:0

#define	AR_PALIDX0				0x00
#define	AR_PALIDX1				0x01
#define	AR_PALIDX2				0x02
#define	AR_PALIDX3				0x03
#define	AR_PALIDX4				0x04
#define	AR_PALIDX5				0x05
#define	AR_PALIDX6				0x06
#define	AR_PALIDX7				0x07
#define	AR_PALIDX8				0x08
#define	AR_PALIDX9				0x09
#define	AR_PALIDX10				0x0a
#define	AR_PALIDX11				0x0b
#define	AR_PALIDX12				0x0c
#define	AR_PALIDX13				0x0d
#define	AR_PALIDX14				0x0e
#define	AR_PALIDX15				0x0f

#define	AR_MODE					0x10
#define	AR_MODE_PAL54SRC			7:7
#define	AR_MODE_PAL54SRC_PALETTE		0
#define	AR_MODE_PAL54SRC_COLORSEL		1
#define	AR_MODE_PIXWIDTH			6:6
#define	AR_MODE_PIXWIDTH_4			0
#define	AR_MODE_PIXWIDTH_8			1
#define	AR_MODE_PANSEL				5:5
#define	AR_MODE_PANSEL_BOTH			0
#define	AR_MODE_PANSEL_UPPER			1
#define	AR_MODE_BLINK				3:3
#define	AR_MODE_BLINK_DISABLE			0
#define	AR_MODE_BLINK_ENABLE			1
#define	AR_MODE_LINEGR				2:2
#define	AR_MODE_LINEGR_DISABLE			0
#define	AR_MODE_LINEGR_ENABLE			1
#define	AR_MODE_DISPTYPE			1:1
#define	AR_MODE_DISPTYPE_COLOR			0
#define	AR_MODE_DISPTYPE_MONO			1
#define	AR_MODE_DISPMODE			0:0
#define	AR_MODE_DISPMODE_TEXT			0
#define	AR_MODE_DISPMODE_GRAPHICS		1

#define	AR_OSCANCOLOR				0x11

#define	AR_PLANEENABLE				0x12
#define	AR_PLANEENABLE_PLANEMASK		3:0

#define	AR_HPAN					0x13
#define	AR_HPAN_PANVAL				3:0

#define	AR_COLORSEL				0x14
#define	AR_COLORSEL_PALIDX_7_6			3:2
#define	AR_COLORSEL_ALTPAL_5_4			1:0


/*****************************************************************************
 * CRT controller register indices and bit definitions.
 */
#define	CR_HTOTAL_7_0				0x00
#define	CR_HDISPEND_7_0				0x01
#define	CR_HBLANKSTART_7_0			0x02

#define	CR_HBLANKEND				0x03
#define	CR_HBLANKEND_SETTOONE			7:7
#define	CR_HBLANKEND_DISPDELAY			6:5
#define	CR_HBLANKEND_HBLANKEND_4_0		4:0

#define	CR_HSYNCSTART_7_0			0x04

#define	CR_HSYNCEND				0x05
#define	CR_HSYNCEND_HBLANKEND_5			7:7
#define	CR_HSYNCEND_SYNCDELAY			6:5
#define	CR_HSYNCEND_HSYNCEND_4_0		4:0

#define	CR_VTOTAL_7_0				0x06

#define	CR_OVERFLOW				0x07
#define	CR_OVERFLOW_VSYNCSTART_9		7:7
#define	CR_OVERFLOW_VDISPEND_9			6:6
#define	CR_OVERFLOW_VTOTAL_9			5:5
#define	CR_OVERFLOW_LINECMP_8			4:4
#define	CR_OVERFLOW_VBLANKSTART_8		3:3
#define	CR_OVERFLOW_VSYNCSTART_8		2:2
#define	CR_OVERFLOW_VDISPEND_8			1:1
#define	CR_OVERFLOW_VTOTAL_8			0:0

#define	CR_FONTROLL				0x08
#define	CR_FONTROLL_BYTEPAN			6:5
#define	CR_FONTROLL_STARTROW			4:0

#define	CR_FONTHEIGHT				0x09
#define	CR_FONTHEIGHT_SCANDOUBLE		7:7
#define	CR_FONTHEIGHT_LINECMP_9			6:6
#define	CR_FONTHEIGHT_VBLANKSTART_9		5:5
#define	CR_FONTHEIGHT_FONTHEIGHT		4:0

#define	CR_TXCURSTOP				0x0a
#define	CR_TXCURSTOP_CURSENABLE			5:5
#define	CR_TXCURSTOP_TOP			4:0

#define	CR_TXCURSBOT				0x0b
#define	CR_TXCURSBOT_CURSDELAY			6:5
#define	CR_TXCURSBOT_BOT			4:0

#define	CR_FBBASE_17_10				0x0c
#define	CR_FBBASE_9_2				0x0d
#define	CR_TXCURSLOC_15_8			0x0e
#define	CR_TXCURSLOC_7_0			0x0f
#define	CR_VSYNCSTART_7_0			0x10

#define	CR_VSYNCEND				0x11
#define	CR_VSYNCEND_PROTECT			7:7
#define	CR_VSYNCEND_VBLDISABLE			5:5
#define	CR_VSYNCEND_CLEARVBL			4:4
#define	CR_VSYNCEND_VSYNCEND_3_0		3:0

#define	CR_VDISPEND_7_0				0x12
#define	CR_PITCH_7_0				0x13

#define	CR_ULLOC				0x14
#define	CR_ULLOC_FETCH32			6:6
#define	CR_ULLOC_COUNT4				5:5
#define	CR_ULLOC_ULLOC				4:0

#define	CR_VBLANKSTART_7_0			0x15
#define	CR_VBLANKEND_7_0			0x16

#define	CR_MODE					0x17
#define	CR_MODE_NOTRESET			7:7
#define	CR_MODE_FETCH8				6:6
#define	CR_MODE_WRAP16K				5:5
#define	CR_MODE_COUNT2				3:3
#define	CR_MODE_TWICEHIGH			2:2
#define	CR_MODE_SELECTROWSCAN			1:1
#define	CR_MODE_COMPAT				0:0

#define	CR_LINECMP_7_0				0x18
#define	CR_READLATCH				0x22

#define	CR_ARTOGGLE				0x24
#define	CR_ARTOGGLE_STATE			7:7
#define	CR_ARTOGGLE_STATE_IDX			0
#define	CR_ARTOGGLE_STATE_DATA			1


/*****************************************************************************
 * Other register bit definitions.
 */
/*  Status Register 0  */
#define	ST00_VBLPENDING				7:7
#define	ST00_MONITORTYPE			4:4
#define	ST00_MONITORTYPE_MONO			0
#define	ST00_MONITORTYPE_COLOR			1

/*  Status Register 1  */
#define	ST01_FEEBACKBITS			5:4
#define	ST01_VBLACTIVE				3:3
#define	ST01_NOTDISPSCANNING			0:0

/*  Feature Control Register  */
#define	FCR_VSYNCCTL				3:3
#define	FCR_VSYNCCTL_NORMAL			0
#define	FCR_VSYNCCTL_VSYNC_OR_DISPENABLE	1

/*  Miscellaneous Output Register  */
#define	MISC_VSYNCPOL				7:7
#define	MISC_VSYNCPOL_POSITIVE			0
#define	MISC_VSYNCPOL_NEGATIVE			1
#define	MISC_HSYNCPOL				6:6
#define	MISC_HSYNCPOL_POSITIVE			0
#define	MISC_HSYNCPOL_NEGATIVE			1
#define	MISC_ODDEVENPAGESEL			5:5
#define	MISC_ODDEVENPAGESEL_0K			0
#define	MISC_ODDEVENPAGESEL_64K			1
#define	MISC_CLKSEL				3:2
#define	MISC_CLKSEL_25MHZ			0
#define	MISC_CLKSEL_28MHZ			1
#define	MISC_CLKSEL_EXTRA			2
#define	MISC_0xA0000WINDOW			1:1
#define	MISC_0xA0000WINDOW_DISABLE		0
#define	MISC_0xA0000WINDOW_ENABLE		1
#define	MISC_IOADDR				0:0
#define	MISC_IOADDR_3Bx				0
#define	MISC_IOADDR_3Dx				1


/*****************************************************************************
 * ROP/MinTerm definitions.
 */
/*
 * Windoze ROP codes are *exactly* analogous to Amiga blitter MinTerms, except
 * that the three sources for ROP codes have fixed definitions:  "Pattern,"
 * Source, and Destination (before write).  Leave it to MS to make ROP codes
 * as difficult as possible to understand what with that Reverse Polish
 * notation in all their books, so read this and be enlightened:
 * 
 * The MinTerm value/ROP code directly specifies the output of three binary
 * inputs (in the Windoze case, Pattern, Source, and Destination).  For three
 * binary sources, there are a total of eight possible input states.  Each of
 * the bits in the MinTerm value itself defines the result of each possible
 * state.  The MinTerm bits are defined as follows:
 *
 *      7       6       5       4       3       2       1       0
 *    P S D   P S D   P S D   P S D   P S D   P S D   P S D   P S D
 *    1 1 1   1 1 0   1 0 1   1 0 0   0 1 1   0 1 0   0 0 1   0 0 0
 *
 * Note that the bit position numbers exactly match the combination of the
 * three input state values.  When the state of the three inputs matches a
 * particular bit position, the value of that bit position in the MinTerm
 * is used as the output value.  So, for a value of 0xCC (the ever-popular
 * ROP_COPY), we get the following truth table:
 *
 * MinTerm value  State
 * bit position   (PSD)   Output
 *        0       (000):  0
 *        1       (001):  0
 *        2       (010):  1
 *        3       (011):  1
 *        4       (100):  0
 *        5       (101):  0
 *        6       (110):  1
 *        7       (111):  1
 *
 * So if, for example, the value of P is one, S is zero, and D is zero (state
 * 4), the output will be the bit in position 4 of the MinTerm, which is zero.
 *
 * The concept that was hardest for me to grasp was the setting of bits that I
 * "don't care" about.  For example, state 3 (PSD = 011) is set to 1.  This
 * means that the Destination (in addition to the source) must be one for the
 * output to be one.  "But I don't care about the Destination value.  Why am I
 * selecting it?"  You have to remember that the bits are not source
 * selection bits, they are truth table outputs.  Note that state 2
 * (PSD = 010) is also set to 1.  This means that the Destination must be
 * zero for the output to be one.  Taken together with state 3, it means that
 * the output will be 1 *no matter what* the value of the Destination is,
 * which is what we want.  The same trick is pulled with the Pattern source
 * (state 2 (PSD = 010) and 6 (PSD = 110)); the output is one regardless of
 * the Pattern value.  So forming a MinTerm value from scratch requires you to
 * consider *all* of the sources, whether you care about them or not.
 *
 * For multi-bit (color) pixel values, each bit position is considered
 * independently of the others.  Apply the MinTerm truth table to each bit
 * position in the source values to generate an output for it.
 *
 * This mechanism is deceptively powerful when implemented properly (unlike
 * in the Windoze universe), and can be used to do all kinds of non-obvious
 * logical and mathematical operations on large groups of numbers.
 */
#define	MT_PSD		(1<<7)
#define	MT_PSND		(1<<6)
#define	MT_PNSD		(1<<5)
#define	MT_PNSND	(1<<4)
#define	MT_NPSD		(1<<3)
#define	MT_NPSND	(1<<2)
#define	MT_NPNSD	(1<<1)
#define	MT_NPNSND	(1<<0)

#define	MINTERM_PATCOPY		(MT_PSD | MT_PSND | MT_PNSD | MT_PNSND)
#define	MINTERM_SRCCOPY		(MT_PSD | MT_PSND | MT_NPSD | MT_NPSND)
#define	MINTERM_DESTCOPY	(MT_PSD | MT_PNSD | MT_NPSD | MT_NPNSD)


#endif	/*  __VGADEFS_H  */
