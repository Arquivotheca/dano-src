/* :ts=8 bk=0
 *
 * tridentdefs.h:	Bit and structure definitions for (some) Trident
 *			chips.
 *
 * Leo L. Schwab					1999.09.30
 *  i740defs.h used as a template.
 *
 * Copyright 1999 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#ifndef	__TRIDENTDEFS_H
#define	__TRIDENTDEFS_H

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
	vuint8	__reserved6[0x0002];	/*	0x03D6 - 0x03D7		*/
	vuint8	VGA_DestSeg;		/*  0x03D8			*/
	vuint8	VGA_SrcSeg;		/*  0x03D9			*/
	vuint8	VGA_ST01;		/*  0x03DA			*/
	vuint8	VGA_AltClk;		/*  0x03DB			*/
	vuint8	__reserved7[0x0c24];	/*	0x03DC - 0x0FFF		*/
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
	vuint8	__reserved9[0x0002];	/*	0x03D6 - 0x03D7		*/
	vuint8	VGA_DestSeg;		/*  0x03D8			*/
	vuint8	VGA_SrcSeg;		/*  0x03D9			*/
	vuint8	VGA_FCR;		/*  0x03DA			*/
	vuint8	VGA_AltClk;		/*  0x03DB			*/
	vuint8	__reserved10[0x0c24];	/*	0x03DC - 0x0FFF		*/
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


/*
 * Trident 3D Graphics Engine registers.
 */
typedef struct DDDGraphicsEngineRegs {
	vuint32		DDDG_BltSrcStart;	/*  0x00000 - 0x00003	*/
	vuint32		DDDG_BltSrcStop;	/*  0x00004 - 0x00007	*/
	vuint32		DDDG_BltDestStart;	/*  0x00008 - 0x0000B	*/
	vuint32		DDDG_BltDestStop;	/*  0x0000C - 0x0000F	*/
	vuint32		DDDG_RightDispAddr;	/*  0x00010 - 0x00013	*/
	vuint32		DDDG_LeftDispAddr;	/*  0x00014 - 0x00017	*/
	vuint32		DDDG_BlockWriteStartAddr;/*  0x00018 - 0x0001B	*/
	vuint32		DDDG_BlockWriteEndAddr;	/*  0x0001C - 0x0001F	*/
	vuint32		DDDG_Status;		/*  0x00020 - 0x00023	*/
	vuint32		DDDG_Ctl;		/*  0x00024 - 0x00027	*/
	vuint32		DDDG_Debug;		/*  0x00028 - 0x0002B	*/
	vuint32		DDDG_WaitMask;		/*  0x0002C - 0x0002F	*/
	vuint32		DDDG_PrimitiveAttr;	/*  0x00030 - 0x00033	*/
	vuint32		__reserved0[2];		/*  0x00034 - 0x0003C	*/
	vuint32		DDDG_PrimitiveTypeW;	/*  0x0003C - 0x0003F	*/
	vuint32		__reserved1;		/*  0x00040 - 0x00043	*/
	vuint32		DDDG_DrawCmd;		/*  0x00044 - 0x00047	*/
	vuint32		DDDG_ROP;		/*  0x00048 - 0x0004B	*/
	vuint32		DDDG_ZFunc;		/*  0x0004C - 0x0004F	*/
	vuint32		DDDG_TexFunc;		/*  0x00050 - 0x00053	*/
	vuint32		DDDG_ClipWin1;		/*  0x00054 - 0x00057	*/
	vuint32		DDDG_ClipWin2;		/*  0x00058 - 0x0005B	*/
	vuint32		__reserved2;		/*  0x0005C - 0x0005F	*/
	vuint32		DDDG_FGColor;		/*  0x00060 - 0x00063	*/
	vuint32		DDDG_BGColor;		/*  0x00064 - 0x00067	*/
	vuint32		DDDG_ColorKey;		/*  0x00068 - 0x0006B	*/
	vuint32		DDDG_PatternStyle;	/*  0x0006C - 0x0006F	*/
	vuint32		DDDG_PatternColor;	/*  0x00070 - 0x00073	*/
	vuint32		DDDG_FGPatternColor;	/*  0x00074 - 0x00077	*/
	vuint32		DDDG_BGPatternColor;	/*  0x00078 - 0x0007B	*/
	vuint32		DDDG_Alpha;		/*  0x0007C - 0x0007F	*/
	vuint32		DDDG_AlphaFunc;		/*  0x00080 - 0x00083	*/
	vuint32		DDDG_BitMask;		/*  0x00084 - 0x00087	*/
	vuint32		__reserved3[6];		/*  0x00088 - 0x0009F	*/
	vuint32		DDDG_TexCtl;		/*  0x000A0 - 0x000A3	*/
	vuint32		DDDG_TexColor;		/*  0x000A4 - 0x000A7	*/
	vuint32		DDDG_PaletteData;	/*  0x000A8 - 0x000AB	*/
	vuint32		DDDG_TexBoundary;	/*  0x000AC - 0x000AF	*/
	vuint32		__reserved4[2];		/*  0x000B0 - 0x000B7	*/
	vuint32		DDDG_DestStrideBuf0;	/*  0x000B8 - 0x000BB	*/
	vuint32		DDDG_DestStrideBuf1;	/*  0x000BC - 0x000BF	*/
	vuint32		DDDG_DestStrideBuf2;	/*  0x000C0 - 0x000C3	*/
	vuint32		DDDG_DestStrideBuf3;	/*  0x000C4 - 0x000C7	*/
	vuint32		DDDG_SrcStrideBuf0;	/*  0x000C8 - 0x000CB	*/
	vuint32		DDDG_SrcStrideBuf1;	/*  0x000CC - 0x000CF	*/
	vuint32		DDDG_SrcStrideBuf2;	/*  0x000D0 - 0x000D3	*/
	vuint32		DDDG_SrcStrideBuf3;	/*  0x000D4 - 0x000D7	*/
	vuint32		DDDG_ZDepthBuf;		/*  0x000D8 - 0x000DB	*/
	vuint32		DDDG_TexLOD0;		/*  0x000DC - 0x000DF	*/
	vuint32		DDDG_TexLOD1;		/*  0x000E0 - 0x000E3	*/
	vuint32		DDDG_TexLOD2;		/*  0x000E4 - 0x000E7	*/
	vuint32		DDDG_TexLOD3;		/*  0x000E8 - 0x000EB	*/
	vuint32		DDDG_TexLOD4;		/*  0x000EC - 0x000EF	*/
	vuint32		DDDG_TexLOD5;		/*  0x000F0 - 0x000F3	*/
	vuint32		DDDG_TexLOD6;		/*  0x000F4 - 0x000F7	*/
	vuint32		DDDG_TexLOD7;		/*  0x000F8 - 0x000FB	*/
	vuint32		DDDG_TexLOD8;		/*  0x000FC - 0x000FF	*/
} DDDGraphicsEngineRegs;

/*  Changes meaning depending on whether you read or write it.  */
#define	DDDG_SetupStatusR	DDDG_PrimitiveTypeW


/*
 * The rest of the Trident registers.  I haven't settled on all the names yet.
 */
typedef struct TridentRegs {
	/*
	 * Rather than hack up the vgaregs structure, I chose to do things
	 * this way.
	 */
	union {					/*  0x00000 - 0x00FFF	*/
		vgaregs			vga;
		DDDGraphicsEngineRegs	dddg;
	} loregs;

	vuint8		__reserved0[0x1200];	/*  0x01000 - 0x021FF	*/

	/*  Zoomed Video (ZV) / Capture command  */
	vuint32		ZVCaptureCmd;		/*  0x02200 - 0x02203	*/

	/*  PCI Bus Master registers  */
	vuint32		BM_Status;		/*  0x02204 - 0x02207	*/
	vuint8		__reserved1[0x0078];	/*  0x02208 - 0x0227F	*/

	/*  DVD Registers  */
	vuint8		DVD_MC_ID;		/*  0x02280		*/
	vuint8		DVD_MC_Ctl;		/*  0x02281		*/
	vuint8		DVD_MC_FBConfig;	/*  0x02282		*/
	vuint8		__reserved2;		/*  0x02283		*/
	vuint32		DVD_MC_CmdQueue;	/*  0x02284 - 0x02287	*/
	vuint32		DVD_MC_YRefAddr;	/*  0x02288 - 0x0228B	*/
	vuint32		DVD_MC_URefAddr;	/*  0x0228C - 0x0228F	*/
	vuint32		DVD_MC_VRefAddr;	/*  0x02290 - 0x02293	*/
	vuint32		DVD_MC_YDispOffset;	/*  0x02294 - 0x02297	*/
	vuint32		DVD_MC_UDispOffset;	/*  0x02298 - 0x0229B	*/
	vuint32		DVD_MC_VDispOffset;	/*  0x0229C - 0x0229F	*/
	vuint8		DVD_MC_NHMacroblocks;	/*  0x022A0		*/
	vuint8		__reserved3;		/*  0x022A1		*/
	vuint8		DVD_MC_NVMacroblocks;	/*  0x022A2		*/
	vuint8		__reserved4;		/*  0x022A3		*/
	vuint16		DVD_MC_FBYLength;	/*  0x022A4 - 0x022A5	*/
	vuint16		__reserved5;		/*  0x022A6 - 0x022A7	*/
	vuint32		DVD_MC_ColorPalette;	/*  0x022A8 - 0x022AB	*/
	vuint32		__reserved6;		/*  0x022AC - 0x022AF	*/
	vuint32		DVD_SP_Buf0PixAddr;	/*  0x022B0 - 0x022B3	*/
	vuint32		DVD_SP_Buf1PixAddr;	/*  0x022B4 - 0x022B7	*/
	vuint32		DVD_SP_Buf0CmdAddr;	/*  0x022B8 - 0x022BB	*/
	vuint32		DVD_SP_Buf1CmdAddr;	/*  0x022BC - 0x022BF	*/
	vuint16		DVD_SP_YDispOffset;	/*  0x022C0 - 0x022C1	*/
	vuint8		__reserved7[0x000E];	/*  0x022C2 - 0x022CF	*/
	vuint8		DVD_TV_EncoderCtl;	/*  0x022D0		*/
	vuint8		DVD_TV_EncoderCFC[3];	/*  0x022D1 - 0x022D3	*/
	vuint8		__reserved8[0x002C];	/*  0x022D4 - 0x022FF	*/

	/*  PCI Bus Master registers (again)  */
	vuint32		BM_Ctl;			/*  0x02300 - 0x02303	*/
	vuint8		__reserved9[0x000C];	/*  0x02304 - 0x0230F	*/
	vuint32		BM_SysSideStartAddr;	/*  0x02310 - 0x02313	*/
	vuint16		BM_Height;		/*  0x02314 - 0x02315	*/
	vuint16		BM_Width;		/*  0x02316 - 0x02317	*/
	vuint32		BM_FBAddrPitch;		/*  0x02318 - 0x0231B	*/
	vuint16		BM_SysSidePitch;	/*  0x0231C - 0x0231D	*/
	vuint16		__reserved10;		/*  0x0231E - 0x0231F	*/
	vuint32		BM_ClearData;		/*  0x02320 - 0x02323	*/
	vuint8		__reserved11[0x001C];	/*  0x02324 - 0x0233F	*/

	/*  AGP Operation registers  */
	vuint32		AGP_CmdListAddr;	/*  0x02340 - 0x02343	*/
	vuint32		AGP_CmdListSize;	/*  0x02344 - 0x02347	*/
	vuint32		AGP_Ch1_FBAddrPitch;	/*  0x02348 - 0x0234B	*/
	vuint32		AGP_Ch1_FBSize;		/*  0x0234C - 0x0234F	*/
	vuint32		AGP_Ch1_SysAddr;	/*  0x02350 - 0x02353	*/
	vuint32		AGP_Ch12_SysSidePitch;	/*  0x02354 - 0x02357	*/
	vuint32		AGP_Ch2_SysAddr;	/*  0x02358 - 0x0235B	*/
	vuint32		AGP_Ch2_FBAddrPitch;	/*  0x0235C - 0x0235F	*/
	vuint32		AGP_Ch2_FBSize;		/*  0x02360 - 0x02363	*/
	vuint32		AGP_CounterThreshhold;	/*  0x02364 - 0x02367	*/
	vuint32		AGP_IOCtl;		/*  0x02368 - 0x0236B	*/
	vuint32		AGP_Ch2Ctl;		/*  0x0236C - 0x0236F	*/
	vuint32		AGP_CmdChStatus;	/*  0x02370 - 0x02373	*/
	vuint8		__reserved12[0x003C];	/*  0x02374 - 0x023AF	*/
	vuint32		AGP_CmdBufStart;	/*  0x023B0 - 0x023B3	*/
	vuint32		AGP_CmdBufEnd;		/*  0x023B4 - 0x023B7	*/
	vuint8		__reserved13[0x0048];	/*  0x023B8 - 0x023FF	*/

	vuint8		__reserved14[0xDC00];	/*  0x02400 - 0x0FFFF	*/
	
	/*  3D Graphics Engine  */
	vuint32		DDDG_DataPort[0x4000];	/*  0x10000 - 0x1FFFF	*/
} TridentRegs;

/*  This register is schizophrenic  */
#define	DVD_MC_Status	DVD_MC_CmdQueue


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

/*  Trident-specific  */
#define	SR_OLDNEWSTATUS				0x08
#define	SR_OLDNEWSTATUS_MODE			7:7
#define	SR_OLDNEWSTATUS_MODE_OLD		0
#define	SR_OLDNEWSTATUS_MODE_NEW		1
#define	SR_OLDNEWSTATUS_LACEFIELD		6:6
#define	SR_OLDNEWSTATUS_LACEFIELD_ODD		0
#define	SR_OLDNEWSTATUS_LACEFIELD_EVEN		1
#define	SR_OLDNEWSTATUS_CMDFIFO			4:4
#define	SR_OLDNEWSTATUS_CMDFIFO_EMPTY		0
#define	SR_OLDNEWSTATUS_CMDFIFO_BUSY		1

#define	SR_VERSION				0x09
#define	SR_OLDNEWCTL				0x0B

/*  This register has different meanings depending on other settings.  */
#define	SR_CONFIG1				0x0C
#define	SR_CONFIG1_BUSWIDTH			6:6
#define	SR_CONFIG1_BUSWIDTH_32			0
#define	SR_CONFIG1_BUSWIDTH_64			1
#define	SR_CONFIG1_ENABLEPORT			4:4
#define	SR_CONFIG1_ENABLEPORT_46E8		0
#define	SR_CONFIG1_ENABLEPORT_3C3		1
#define	SR_CONFIG1_BIOSSIZE			3:3
#define	SR_CONFIG1_BIOSSIZE_64K			0
#define	SR_CONFIG1_BIOSSIZE_32K			1

#define	SR_CONFIG2				0x0C

#define	SR_MODECTL2OLD				0x0D
#define	SR_MODECTL2OLD_CPUACCESS		3:3
#define	SR_MODECTL2OLD_CPUACCESS_NORMAL		0
#define	SR_MODECTL2OLD_CPUACCESS_FULL		1

#define	SR_MODECTL2NEW				0x0D
#define	SR_MODECTL2NEW_FIFOTHRESH		7:4	/*  4x value	*/
#define	SR_MODECTL2NEW_VCLKDIV			2:1
#define	SR_MODECTL2NEW_VCLKDIV_1		0
#define	SR_MODECTL2NEW_VCLKDIV_2		1
#define	SR_MODECTL2NEW_VCLKDIV_4		2
#define	SR_MODECTL2NEW_VCLKDIV_1_5		3

#define	SR_MODECTL1OLD				0x0E
#define	SR_MODECTL1OLD_IRQACTIVE		6:6
#define	SR_MODECTL1OLD_IRQACTIVE_HIGH		0
#define	SR_MODECTL1OLD_IRQACTIVE_LOW		1
#define	SR_MODECTL1OLD_BUSWIDTH			3:3
#define	SR_MODECTL1OLD_BUSWIDTH_8		0
#define	SR_MODECTL1OLD_BUSWIDTH_16		1
#define	SR_MODECTL1OLD_256KBANKSEL		2:1	/*  Invert bit 1  */
#define	SR_MODECTL1OLD_DACCLK			0:0
#define	SR_MODECTL1OLD_DACCLK_NORMAL		0
#define	SR_MODECTL1OLD_DACCLK_INVERT		1

#define	SR_MODECTL1NEW				0x0E
#define	SR_MODECTL1NEW_CFGPORT			7:7
#define	SR_MODECTL1NEW_CFGPORT_PROTECT		0
#define	SR_MODECTL1NEW_CFGPORT_WRITEABLE	1
#define	SR_MODECTL1NEW_TEXTWIDTH		6:6
#define	SR_MODECTL1NEW_TEXTWIDTH_132		0
#define	SR_MODECTL1NEW_TEXTWIDTH_OTHER		1
#define	SR_MODECTL1NEW_64KBANKSEL		5:0	/*  Invert bit 1  */

#define	SR_POWERUPMODE2				0x0F
#define	SR_POWERUPMODE2_BIOSCTL			6:6
#define	SR_POWERUPMODE2_BIOSCTL_DISABLE		0
#define	SR_POWERUPMODE2_BIOSCTL_ENABLE		1
#define	SR_POWERUPMODE2_PALETTE			5:5
#define	SR_POWERUPMODE2_PALETTE_MASTERABORT	0
#define	SR_POWERUPMODE2_PALETTE_INTELRETRY	1
#define	SR_POWERUPMODE2_ADDRCTL			4:4
#define	SR_POWERUPMODE2_ADDRCTL_LINEAR		0
#define	SR_POWERUPMODE2_ADDRCTL_LINEARBANKED	1

#define	SR_BIGBIOS				0x10
#define	SR_BIGBIOS_BIGBIOS			7:7
#define	SR_BIGBIOS_BIGBIOS_DISABLE		0
#define	SR_BIGBIOS_BIGBIOS_ENABLE		1
#define	SR_BIGBIOS_VIDADDR			6:5
#define	SR_BIGBIOS_VIDADDR_A0_A7		0
#define	SR_BIGBIOS_VIDADDR_B0_B7		2
#define	SR_BIGBIOS_VIDADDR_B8_BF		3
#define	SR_BIGBIOS_PAGESEL			0:0
#define	SR_BIGBIOS_PAGESEL_C0_C7		0
#define	SR_BIGBIOS_PAGESEL_VIDADDR		1

#define	SR_PROTECT				0x11
#define	SR_PROTECT_ACCESS			7:0
#define	SR_PROTECT_ACCESS_MOST			0x87
#define	SR_PROTECT_ACCESS_ALL			0x92
#define	SR_PROTECT_ACCESS_NONE			0x00

#define	SR_THRESH				0x12
#define	SR_THRESH_PB_AND_CAPT			7:4
#define	SR_THRESH_PB_OR_CAPT			3:0

#define	SR_VCLK0CTL0				0x18
#define	SR_VCLK0CTL1				0x19
#define	SR_VCLK1CTL0				0x1A
#define	SR_VCLK1CTL1				0x1B
#define	SR_VCLKxCTL0_N				7:0
#define	SR_VCLKxCTL1_K				7:6
#define	SR_VCLKxCTL1_M				5:0

#define	SR_DACSETUP				0x20
#define	SR_DACSETUP_SYNC			6:6
#define	SR_DACSETUP_SYNC_NORMAL			0
#define	SR_DACSETUP_SYNC_MULTIPLEXED		1
#define	SR_DACSETUP_PLAYBACK			5:5
#define	SR_DACSETUP_PLAYBACK_PLUSVAFC		0
#define	SR_DACSETUP_PLAYBACK_ONLY		1
#define	SR_DACSETUP_ONTOP			4:4
#define	SR_DACSETUP_ONTOP_VAFC			0
#define	SR_DACSETUP_ONTOP_PLAYBACK		1
#define	SR_DACSETUP_DACTEST			3:3
#define	SR_DACSETUP_DACTEST_DISABLE		0
#define	SR_DACSETUP_DACTEST_ENABLE		1
#define	SR_DACSETUP_VIDMODE			2:2
#define	SR_DACSETUP_VIDMODE_DISABLE		0
#define	SR_DACSETUP_VIDMODE_ENABLE		1
#define	SR_DACSETUP_MODESEL			1:0
#define	SR_DACSETUP_MODESEL_RGB555		0	/*  These are	*/
#define	SR_DACSETUP_MODESEL_RGB565		1	/*  probably	*/
#define	SR_DACSETUP_MODESEL_RGB888		2	/*  wrong.	*/
#define	SR_DACSETUP_MODESEL_CMAP8		3

#define	SR_SIGNATURECTL				0x21
#define	SR_SIGNATUREDATA			0x22
#define	SR_POWERCTL				0x24
#define	SR_MONSENSE				0x25
#define	SR_VIDKEYMODE				0x37
#define	SR_FCCTL				0x38
#define	SR_OVL1COLORKEYDATA			0x50
#define	SR_OVL1COLORKEYMASK			0x54
#define	SR_OVL1KEYMODE				0x57
#define	SR_OVL2COLORKEYDATA			0x60
#define	SR_OVL2COLORKEYMASK			0x64
#define	SR_WIN1UFBADDR				0x80
#define	SR_WIN1VFBADDR				0x83
#define	SR_WIN2FBADDR				0x86
#define	SR_WIN2HSCALE				0x89
#define	SR_WIN2VSCALE				0x8B
#define	SR_WIN2VIDSTART				0x8D
#define	SR_WIN2VIDEND				0x91
#define	SR_WIN2VIDLINEBUFLEVEL			0x95
#define	SR_VIDWINCTL0				0x96
#define	SR_VIDWINCTL1				0x97
#define	SR_VIDWINCTL2				0x98
#define	SR_VIDWINCTL3				0x99
#define	SR_WIN1STRIDE				0x9A
#define	SR_WIN2STRIDE				0x9C
#define	SR_LINEBUFREQTHRESH			0x9E
#define	SR_VBICTL				0x9F
#define	SR_VBIFBADDR				0xA0
#define	SR_VBICAPTURESTART			0xA4
#define	SR_VBICAPTUREEND			0xA8
#define	SR_VBIIRQLINE				0xAC
#define	SR_VBICAPTOFFSET			0xAE
#define	SR_WIN1HSBCTL				0xB0
#define	SR_WIN2HSBCTL				0xB2
#define	SR_2NDDISPADDRSEL			0xB4
#define	SR_VIDSHARPNESS				0xB7
#define	SR_2NDCAPTUREADDRSEL			0xB8
#define	SR_CONTRAST				0xBC
#define	SR_MUXCTL				0xBD
#define	SR_MISCCTL				0xBE
#define	SR_WIN2VIDCTL				0xCE
#define	SR_ROWBYTEOFFSET			0xD0
#define	SR_WIN2UFRAMEADDR			0xD2
#define	SR_WIN2VFRAMEADDR			0xD5
#define	SR_DTVCTL				0xD8
#define	SR_WIN2VCOUNTSTATUS			0xDA
#define	SR_DUALVIEWCTL				0xDC
#define	SR_WIN1VCOUNTSTATUS			0xDE


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


/*  Trident-specific  */
#define	GR_SRCSEGADDR				0x0E

#define	GR_MISCCTL				0x0F
#define	GR_MISCCTL_VCLKDIV1			6:6
#define	GR_MISCCTL_DRAMACCESS			5:5
#define	GR_MISCCTL_DRAMACCESS_SYMMETRIC		0
#define	GR_MISCCTL_DRAMACCESS_ASYMMETRIC	1
#define	GR_MISCCTL_ZCHAIN4CPU			4:4
#define	GR_MISCCTL_ZCHAIN4CPU_DISABLE		0
#define	GR_MISCCTL_ZCHAIN4CPU_ENABLE		1
#define	GR_MISCCTL_VCLKDIV0			3:3
#define	GR_MISCCTL_ALTSEL			2:2
#define	GR_MISCCTL_ALTSEL_DISABLE		0
#define	GR_MISCCTL_ALTSEL_ENABLE		1
#define	GR_MISCCTL_ZCHAIN4DISP			1:1
#define	GR_MISCCTL_ZCHAIN4DISP_DISABLE		0
#define	GR_MISCCTL_ZCHAIN4DISP_ENABLE		1
#define	GR_MISCCTL_SRCSEGREG			0:0
#define	GR_MISCCTL_SRCSEGREG_DISABLE		0
#define	GR_MISCCTL_SRCSEGREG_ENABLE		1
/*  Convenience alises.  These are LIES.  Read above defs for real story.  */
#define	GR_MISCCTL_VCLKDIVx			6:3	/*  This is a LIE!  */
#define	GR_MISCCTL_VCLKDIVx_DIV1		0
#define	GR_MISCCTL_VCLKDIVx_DIV2		1
#define	GR_MISCCTL_VCLKDIVx_DIV3		8

#define	GR_TIMERCTL				0x20
#define	GR_PWRCTL1				0x21
#define	GR_PWRCTL2				0x22

#define	GR_PWRSTATUS				0x23
#define	GR_PWRSTATUS_PWRMGMTPOL			7:7
#define	GR_PWRSTATUS_PWRMGMTPOL_ACTIVEHIGH	0
#define	GR_PWRSTATUS_PWRMGMTPOL_ACTIVELOW	1
#define	GR_PWRSTATUS_CHIP			6:5
#define	GR_PWRSTATUS_CHIP_READY			0
#define	GR_PWRSTATUS_CHIP_STANDBY		1
#define	GR_PWRSTATUS_CHIP_SUSPEND		2
#define	GR_PWRSTATUS_LCD			4:4
#define	GR_PWRSTATUS_LCD_DISABLE		0
#define	GR_PWRSTATUS_LCD_ENABLE			1
#define	GR_PWRSTATUS_LCDSEQUENCING		3:2
#define	GR_PWRSTATUS_LCDSEQUENCING_FAST		0
#define	GR_PWRSTATUS_LCDSEQUENCING_SLOW		3
#define	GR_PWRSTATUS_DPMS			1:0
#define	GR_PWRSTATUS_DPMS_ON			0
#define	GR_PWRSTATUS_DPMS_STANDBY		1
#define	GR_PWRSTATUS_DPMS_SUSPEND		2
#define	GR_PWRSTATUS_DPMS_OFF			3

#define	GR_SOFTPWRCTL				0x24
#define	GR_SOFTPWRCTL_VCLK			7:7
#define	GR_SOFTPWRCTL_VCLK_DISABLE		0
#define	GR_SOFTPWRCTL_VCLK_ENABLE		1
#define	GR_SOFTPWRCTL_MCLK			6:6
#define	GR_SOFTPWRCTL_MCLK_DISABLE		0
#define	GR_SOFTPWRCTL_MCLK_ENABLE		1
#define	GR_SOFTPWRCTL_CPUDRAMBUS		5:5
#define	GR_SOFTPWRCTL_CPUDRAMBUS_DISABLE	0
#define	GR_SOFTPWRCTL_CPUDRAMBUS_ENABLE		1
#define	GR_SOFTPWRCTL_PANELLIGHT		3:3
#define	GR_SOFTPWRCTL_PANELVDD			2:2
#define	GR_SOFTPWRCTL_PANELIF			1:1
#define	GR_SOFTPWRCTL_PANELVEE			0:0

#define	GR_PWRCTLSEL				0x25
#define	GR_PWRCTLSEL_VCLK			7:7
#define	GR_PWRCTLSEL_VCLK_HW			0
#define	GR_PWRCTLSEL_VCLK_SW			1
#define	GR_PWRCTLSEL_MCLK			6:6
#define	GR_PWRCTLSEL_MCLK_HW			0
#define	GR_PWRCTLSEL_MCLK_SW			1
#define	GR_PWRCTLSEL_CPUDRAMBUS			5:5
#define	GR_PWRCTLSEL_CPUDRAMBUS_HW		0
#define	GR_PWRCTLSEL_CPUDRAMBUS_SW		1
#define	GR_PWRCTLSEL_DAC			4:4
#define	GR_PWRCTLSEL_DAC_HW			0
#define	GR_PWRCTLSEL_DAC_SW			1
#define	GR_PWRCTLSEL_PANELLIGHT			3:3
#define	GR_PWRCTLSEL_PANELLIGHT_HW		0
#define	GR_PWRCTLSEL_PANELLIGHT_SW		1
#define	GR_PWRCTLSEL_PANELVDD			2:2
#define	GR_PWRCTLSEL_PANELVDD_HW		0
#define	GR_PWRCTLSEL_PANELVDD_SW		1
#define	GR_PWRCTLSEL_PANELIF			1:1
#define	GR_PWRCTLSEL_PANELIF_HW			0
#define	GR_PWRCTLSEL_PANELIF_SW			1
#define	GR_PWRCTLSEL_PANELVEE			0:0
#define	GR_PWRCTLSEL_PANELVEE_HW		0
#define	GR_PWRCTLSEL_PANELVEE_SW		1

#define	GR_DPMS					0x26
#define	GR_GPIO					0x27
#define	GR_SUSPTIMER				0x2A
#define	GR_MISCPINCTL				0x2C

#define	GR_MISCINTERNALCTL			0x2F
#define	GR_MISCINTERNALCTL_PCLK			7:7
#define	GR_MISCINTERNALCTL_PCLK_VGA		0
#define	GR_MISCINTERNALCTL_PCLK_ISVLCK		1
#define	GR_MISCINTERNALCTL_HSKEW		5:5
#define	GR_MISCINTERNALCTL_HSKEW_ENABLE		0
#define	GR_MISCINTERNALCTL_HSKEW_DISABLE	1
#define	GR_MISCINTERNALCTL_LINEWIDTH		2:2
#define	GR_MISCINTERNALCTL_LINEWIDTH_1X		0
#define	GR_MISCINTERNALCTL_LINEWIDTH_2X		1
#define	GR_MISCINTERNALCTL_TXFIFOCYCLES		1:1
#define	GR_MISCINTERNALCTL_TXFIFOCYCLES_8X	0
#define	GR_MISCINTERNALCTL_TXFIFOCYCLES_4X	1
#define	GR_MISCINTERNALCTL_DISPFIFOTHRESH	0:0
#define	GR_MISCINTERNALCTL_DISPFIFOTHRESH_DISABLE	0
#define	GR_MISCINTERNALCTL_DISPFIFOTHRESH_ENABLE	1

#define	GR_LCDCTL				0x30
#define	GR_LCDCTL_VSYNCSHADOW			7:7
#define	GR_LCDCTL_VSYNCSHADOW_DISABLE		0
#define	GR_LCDCTL_VSYNCSHADOW_ENABLE		1
#define	GR_LCDCTL_SHADOWACCESS			6:6
#define	GR_LCDCTL_SHADOWACCESS_DISABLE		0
#define	GR_LCDCTL_SHADOWACCESS_ENABLE		1
#define	GR_LCDCTL_SIGNALDELAY			4:2	/*  -(x) + 2  */
#define	GR_LCDCTL_HSYNCSHADOW			0:0
#define	GR_LCDCTL_HSYNCSHADOW_DISABLE		0
#define	GR_LCDCTL_HSYNCSHADOW_ENABLE		1

#define	GR_LCDATTR				0x31
#define	GR_LCDATTR_720x480MODE			7:7
#define	GR_LCDATTR_720x480MODE_DISABLE		0
#define	GR_LCDATTR_720x480MODE_ENABLE		1
#define	GR_LCDATTR_LINES			6:4
#define	GR_LCDATTR_LINES_480			0
#define	GR_LCDATTR_LINES_600			1
#define	GR_LCDATTR_LINES_768			2
#define	GR_LCDATTR_LINES_1024			3
#define	GR_LCDATTR_BGDISPLAY			3:3
#define	GR_LCDATTR_BGDISPLAY_ENABLE		0
#define	GR_LCDATTR_BGDISPLAY_DISABLE		1
#define	GR_LCDATTR_SYNCVALS			2:2
#define	GR_LCDATTR_SYNCVALS_NORMAL		0
#define	GR_LCDATTR_SYNCVALS_SHADOW		1

#define	GR_LCDCONFIG				0x33
#define	GR_LCDCONFIG_CRTDISPLAY			5:5
#define	GR_LCDCONFIG_LCDDISPLAY			4:4

#define	GR_LCDPOLARITY				0x34
#define	GR_LCDPOLARITY_MOD			7:7
#define	GR_LCDPOLARITY_MOD_NORMAL		0
#define	GR_LCDPOLARITY_MOD_INVERTED		1
#define	GR_LCDPOLARITY_LP			6:6
#define	GR_LCDPOLARITY_LP_NORMAL		0
#define	GR_LCDPOLARITY_LP_INVERTED		1
#define	GR_LCDPOLARITY_FLM			5:5
#define	GR_LCDPOLARITY_FLM_NORMAL		0
#define	GR_LCDPOLARITY_FLM_INVERTED		1
#define	GR_LCDPOLARITY_SCLK			3:3
#define	GR_LCDPOLARITY_SCLK_NORMAL		0
#define	GR_LCDPOLARITY_SCLK_INVERTED		1
#define	GR_LCDPOLARITY_VSYNC			1:1
#define	GR_LCDPOLARITY_VSYNC_POSITIVE		0
#define	GR_LCDPOLARITY_VSYNC_NEGATIVE		1
#define	GR_LCDPOLARITY_HSYNC			0:0
#define	GR_LCDPOLARITY_HSYNC_POSITIVE		0
#define	GR_LCDPOLARITY_HSYNC_NEGATIVE		1

#define	GR_LCDMACONFIG				0x41
#define	GR_LCDMACONFIG_PANELTYPE		2:0
#define	GR_LCDMACONFIG_PANELTYPE_TFT_1024x768x18	0
#define	GR_LCDMACONFIG_PANELTYPE_TFT_1280x1024x18	1
#define	GR_LCDMACONFIG_PANELTYPE_TFT_800x600x18		2
#define	GR_LCDMACONFIG_PANELTYPE_TFT_1024x600x18	3
#define	GR_LCDMACONFIG_PANELTYPE_DSTN_1024x768x16	4
#define	GR_LCDMACONFIG_PANELTYPE_DSTN_1024x600x24	5
#define	GR_LCDMACONFIG_PANELTYPE_DSTN_800x600x16	6
#define	GR_LCDMACONFIG_PANELTYPE_DSTN_1024x768x24	7

#define	GR_TFTPANELCTL				0x42
#define	GR_TFTPANELCTL_PANELTYPE		7:7
#define	GR_TFTPANELCTL_PANELTYPE_DSTN		0
#define	GR_TFTPANELCTL_PANELTYPE_TFT		1
#define	GR_TFTPANELCTL_DUALDATA			6:6	/*  ???  */
#define	GR_TFTPANELCTL_DATAWIDTH		5:4
#define	GR_TFTPANELCTL_DATAWIDTH_TFT_12		0
#define	GR_TFTPANELCTL_DATAWIDTH_TFT_18		1
#define	GR_TFTPANELCTL_DATAWIDTH_DSTN_16	1
#define	GR_TFTPANELCTL_DATAWIDTH_TFT_24		2
#define	GR_TFTPANELCTL_DATAWIDTH_DSTN_24	2
#define	GR_TFTPANELCTL_DITHER			3:3
#define	GR_TFTPANELCTL_DITHER_DISABLE		0
#define	GR_TFTPANELCTL_DITHER_ENABLE		1
#define	GR_TFTPANELCTL_VESAMAPPING		1:1	/*  ???  */
#define	GR_TFTPANELCTL_SHIFTCLK			0:0
#define	GR_TFTPANELCTL_SHIFTCLK_SINGLE		0
#define	GR_TFTPANELCTL_SHIFTCLK_DUAL		1

#define	GR_SSPMCTL				0x43
#define	GR_SSPMCTL_WRCOUNTER			6:6	/*  ???  */
#define	GR_SSPMCTL_WRCOUNTER_LCDVD		0
#define	GR_SSPMCTL_WRCOUNTER_FLM		1
#define	GR_SSPMCTL_FIFOLATENCY			2:0

#define	GR_DSTNPOSITION_8_0			0x49
#define	GR_DSTNPOSITION_9			0x4A

#define	GR_HSYNCADJUST				0x50
#define	GR_HSYNCADJUST_REFINEDEXPAND		7:7
#define	GR_HSYNCADJUST_REFINEDEXPAND_DISABLE	0
#define	GR_HSYNCADJUST_REFINEDEXPAND_ENABLE	1
#define	GR_HSYNCADJUST_ADJUST			2:0	/*  In pixel clocks  */

#define	GR_VSYNCADJUST				0x51
#define	GR_VSYNCADJUST_SIGNBIT			7:7
#define	GR_VSYNCADJUST_ADJUST			6:0	/*  In scan lines  */

#define	GR_LCDVDISPCTL				0x52
#define	GR_LCDVDISPCTL_VCENTER			7:7
#define	GR_LCDVDISPCTL_VCENTER_DISABLE		0
#define	GR_LCDVDISPCTL_VCENTER_ENABLE		1
#define	GR_LCDVDISPCTL_ENEXTVDE			6:6	/*  ???  */
#define	GR_LCDVDISPCTL_PANELSIZE		5:4
#define	GR_LCDVDISPCTL_PANELSIZE_1280x1024	0
#define	GR_LCDVDISPCTL_PANELSIZE_640x480	1
#define	GR_LCDVDISPCTL_PANELSIZE_1024x768	2
#define	GR_LCDVDISPCTL_PANELSIZE_800x600	3
#define	GR_LCDVDISPCTL_TEXTEXPAND		3:2	/*  Values??  */
#define	GR_LCDVDISPCTL_VEXPANDTEXT		1:1
#define	GR_LCDVDISPCTL_VEXPANDTEXT_DISABLE	0
#define	GR_LCDVDISPCTL_VEXPANDTEXT_ENABLE	1
#define	GR_LCDVDISPCTL_VEXPANDGFX		0:0
#define	GR_LCDVDISPCTL_VEXPANDGFX_DISABLE	0
#define	GR_LCDVDISPCTL_VEXPANDGFX_ENABLE	1

#define	GR_LCDHDISPCTL				0x53
#define	GR_LCDHDISPCTL_HCENTER			7:7
#define	GR_LCDHDISPCTL_HCENTER_DISABLE		0
#define	GR_LCDHDISPCTL_HCENTER_ENABLE		1
#define	GR_LCDHDISPCTL_COMPRESSTEXT		5:5
#define	GR_LCDHDISPCTL_COMPRESSTEXT_DISABLE	0
#define	GR_LCDHDISPCTL_COMPRESSTEXT_ENABLE	1
#define	GR_LCDHDISPCTL_WIDEPANEL		3:2
#define	GR_LCDHDISPCTL_WIDEPANEL_NONE		0
#define	GR_LCDHDISPCTL_WIDEPANEL_800x480	1
#define	GR_LCDHDISPCTL_WIDEPANEL_1024x600	2
#define	GR_LCDHDISPCTL_WIDEPANEL_1280x768	3
#define	GR_LCDHDISPCTL_HEXPANDTEXT		1:1
#define	GR_LCDHDISPCTL_HEXPANDTEXT_DISABLE	0
#define	GR_LCDHDISPCTL_HEXPANDTEXT_ENABLE	1
#define	GR_LCDHDISPCTL_HEXPANDGFX		0:0
#define	GR_LCDHDISPCTL_HEXPANDGFX_DISABLE	0
#define	GR_LCDHDISPCTL_HEXPANDGFX_ENABLE	1


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


/*  Trident-specific  */
#define	CR_LACECTL				0x19
#define	CR_ARBCTL1				0x1A
#define	CR_ARBCTL2				0x1B
#define	CR_ARBCTL3				0x1C

#define	CR_MODULETEST				0x1E
#define	CR_MODULETEST_MEMACCESS			7:7
#define	CR_MODULETEST_MEMACCESS_256K		0
#define	CR_MODULETEST_MEMACCESS_ALL		1
#define	CR_MODULETEST_MISCREGPROTECT		6:6
#define	CR_MODULETEST_MISCREGPROTECT_RW		0
#define	CR_MODULETEST_MISCREGPROTECT_RO		1
#define	CR_MODULETEST_FBBASE_16			5:5
#define	CR_MODULETEST_INTERLACE			2:2
#define	CR_MODULETEST_INTERLACE_DISABLE		0
#define	CR_MODULETEST_INTERLACE_ENABLE		1
#define	CR_MODULETEST_ZERO			1:0	/*  For testing  */

#define	CR_UNDOCUMENTED				0x1E
#define	CR_UNDOCUMENTED_VALUE			0x80

#define	CR_SWPROGRAMMING			0x1F
#define	CR_SWPROGRAMMING_MEMSIZE		3:0
#define	CR_SWPROGRAMMING_MEMSIZE_1M		3
#define	CR_SWPROGRAMMING_MEMSIZE_2M		7
#define	CR_SWPROGRAMMING_MEMSIZE_4M		15
#define	CR_SWPROGRAMMING_MEMSIZE_8M		4

#define	CR_CMDFIFO				0x20
#define	CR_CMDFIFO_WRITEBUF			5:5
#define	CR_CMDFIFO_WRITEBUF_DISABLE		0
#define	CR_CMDFIFO_WRITEBUF_ENABLE		1
#define	CR_CMDFIFO_16BITPLANAR			4:4
#define	CR_CMDFIFO_16BITPLANAR_DISABLE		0
#define	CR_CMDFIFO_16BITPLANAR_ENABLE		1

#define	CR_ADDRMODE				0x21
#define	CR_ADDRMODE_LINEAR			5:5
#define	CR_ADDRMODE_LINEAR_DISABLE		0
#define	CR_ADDRMODE_LINEAR_ENABLE		1

#define	CR_LATCHREADBACK			0x22

#define	CR_ARSTATE				0x24
#define	CR_ARSTATE_STATE			7:7
#define	CR_ARSTATE_STATE_INDEX			0
#define	CR_ARSTATE_STATE_DATA			1

#define	CR_DACRWTIMING				0x25
#define	CR_DACRWTIMING_PCLKTRISTATE		7:7
#define	CR_DACRWTIMING_PCLKTRISTATE_ENABLE	0
#define	CR_DACRWTIMING_PCLKTRISTATE_DISABLE	1
#define	CR_DACRWTIMING_WAITSTATES		3:0

#define	CR_VSYNCEXTRA				0x27
#define	CR_VSYNCEXTRA_VTOTAL_10			7:7
#define	CR_VSYNCEXTRA_VBLANKSTART_10		6:6
#define	CR_VSYNCEXTRA_VSYNCSTART_10		5:5
#define	CR_VSYNCEXTRA_VDISPEND_10		4:4
#define	CR_VSYNCEXTRA_LINECMP_10		3:3
#define	CR_VSYNCEXTRA_FBBASE_19_17		2:0

#define	CR_DACMODE				0x29
#define	CR_DACMODE_USEEXTERNAL			7:7
#define	CR_DACMODE_USEEXTERNAL_DISABLE		0
#define	CR_DACMODE_USEEXTERNAL_ENABLE		1
#define	CR_DACMODE_PITCH_9_8			5:4
#define	CR_DACMODE_GEIODECODE			3:3
#define	CR_DACMODE_GEIODECODE_DISABLE		0
#define	CR_DACMODE_GEIODECODE_ENABLE		1
#define	CR_DACMODE_DAC				2:2
#define	CR_DACMODE_DAC_EXTERNAL			0
#define	CR_DACMODE_DAC_INTERNAL			1
#define	CR_DACMODE_RACRS_3_2			1:0

#define	CR_IFSEL				0x2A
#define	CR_IFSEL_INTERNALWIDTH			6:6
#define	CR_IFSEL_INTERNALWIDTH_8_16		0
#define	CR_IFSEL_INTERNALWIDTH_32		1
#define	CR_IFSEL_ROMCSPOWERDOWN			4:4
#define	CR_IFSEL_ROMCSPOWERDOWN_ENABLE		0
#define	CR_IFSEL_ROMCSPOWERDOWN_DISABLE		1

#define	CR_HSYNCEXTRA				0x2B
#define	CR_HSYNCEXTRA_HBLANKSTART_8		4:4
#define	CR_HSYNCEXTRA_HSYNCSTART_8		3:3
#define	CR_HSYNCEXTRA_HLACE_8			2:2
#define	CR_HSYNCEXTRA_HDISPEND_8		1:1
#define	CR_HSYNCEXTRA_HTOTAL_8			0:0

#define	CR_GETIMINGCTL				0x2D
#define	CR_GETIMINGCTL_SAMPCLKDELAY		4:3
#define	CR_GETIMINGCTL_FBREADDELAY		2:0

#define	CR_PERFTUNE				0x2F
#define	CR_PERFTUNE_DISPFIFODEPTH16		7:7
#define	CR_PERFTUNE_DRAMREFRESHCYCLES		6:6
#define	CR_PERFTUNE_DRAMREFRESHCYCLES_3_5	0
#define	CR_PERFTUNE_DRAMREFRESHCYCLES_1_2	1
#define	CR_PERFTUNE_BLANKSRC			5:5
#define	CR_PERFTUNE_BLANKSRC_NORMAL		0
#define	CR_PERFTUNE_BLANKSRC_INVDISP		1
#define	CR_PERFTUNE_DISPFIFODEPTH32		4:4
#define	CR_PERFTUNE_MEMREADCTL			3:2
#define	CR_PERFTUNE_MEMREADCTL_FAST		1
#define	CR_PERFTUNE_MEMREADCTL_NORMAL		3
#define	CR_PERFTUNE_CLKSRC			1:1
#define	CR_PERFTUNE_CLKSRC_VCLK1		0
#define	CR_PERFTUNE_CLKSRC_VCLK0		1
/*  Convenience alises.  These are LIES.  Read above defs for real story.  */
#define	CR_PERFTUNE_DISPFIFODEPTH		7:4	/*  This is a LIE!  */
#define	CR_PERFTUNE_DISPFIFODEPTH_16		0
#define	CR_PERFTUNE_DISPFIFODEPTH_32		1
#define	CR_PERFTUNE_DISPFIFODEPTH_48		8
#define	CR_PERFTUNE_DISPFIFODEPTH_64		9

#define	CR_GEBASE				0x34

#define	CR_GRVIDENGINECTL			0x36
#define	CR_GRVIDENGINECTL_GE			7:7
#define	CR_GRVIDENGINECTL_GE_DISABLE		0
#define	CR_GRVIDENGINECTL_GE_ENABLE		1
#define	CR_GRVIDENGINECTL_VIDMINIFIER		6:6
#define	CR_GRVIDENGINECTL_VIDMINIFIER_BYPASS	0
#define	CR_GRVIDENGINECTL_VIDMINIFIER_MINIFY	1
#define	CR_GRVIDENGINECTL_VIDAPERTURE		5:5
#define	CR_GRVIDENGINECTL_VIDAPERTURE_DISABLE	0
#define	CR_GRVIDENGINECTL_VIDAPERTURE_ENABLE	1
#define	CR_GRVIDENGINECTL_GERESET		4:4
#define	CR_GRVIDENGINECTL_GERESET_NORMAL	0
#define	CR_GRVIDENGINECTL_GERESET_RESET		1
#define	CR_GRVIDENGINECTL_GEIO			3:3
#define	CR_GRVIDENGINECTL_GEIO_DISABLE		0
#define	CR_GRVIDENGINECTL_GEIO_ENABLE		1
#define	CR_GRVIDENGINECTL_STRWRITE		2:2
#define	CR_GRVIDENGINECTL_STRWRITE_DISABLE	0
#define	CR_GRVIDENGINECTL_STRWRITE_ENABLE	1
#define	CR_GRVIDENGINECTL_GEBASE		1:0
#define	CR_GRVIDENGINECTL_GEBASE_IO_21xx	0
#define	CR_GRVIDENGINECTL_GEBASE_MEM_B7Fxx	1
#define	CR_GRVIDENGINECTL_GEBASE_MEM_BFFxx	2
#define	CR_GRVIDENGINECTL_GEBASE_MEM_GEBASE	3

#define	CR_I2CCTL				0x37
#define	CR_I2CCTL_CLKISOPENDRAIN		7:7
#define	CR_I2CCTL_CLKSTATUS			6:6
#define	CR_I2CCTL_MODE				3:3
#define	CR_I2CCTL_MODE_READ			0
#define	CR_I2CCTL_MODE_WRITE			1
#define	CR_I2CCTL_CLKVAL			1:1
#define	CR_I2CCTL_DATAVAL			0:0

#define	CR_PIXBUSMODE				0x38
#define	CR_PIXBUSMODE_RGB888			5:5
#define	CR_PIXBUSMODE_RGB888_DISABLE		0
#define	CR_PIXBUSMODE_RGB888_ENABLE		1
#define	CR_PIXBUSMODE_STDVGA64			4:4
#define	CR_PIXBUSMODE_STDVGA64_DISABLE		0
#define	CR_PIXBUSMODE_STDVGA64_ENABLE		1
#define	CR_PIXBUSMODE_TRUECOLOR			3:3
#define	CR_PIXBUSMODE_TRUECOLOR_DISABLE		0
#define	CR_PIXBUSMODE_TRUECOLOR_ENABLE		1
#define	CR_PIXBUSMODE_HIGHCOLOR			2:2
#define	CR_PIXBUSMODE_HIGHCOLOR_DISABLE		0
#define	CR_PIXBUSMODE_HIGHCOLOR_ENABLE		1
#define	CR_PIXBUSMODE_16BITBUS			0:0
#define	CR_PIXBUSMODE_16BITBUS_DISABLE		0
#define	CR_PIXBUSMODE_16BITBUS_ENABLE		1

#define	CR_PCICTL				0x39
#define	CR_PCICTL_PIXENDIAN			7:7
#define	CR_PCICTL_PIXENDIAN_LITTLE		0
#define	CR_PCICTL_PIXENDIAN_BIG			1
#define	CR_PCICTL_DATASWAP			6:5
#define	CR_PCICTL_DATASWAP_NONE			0
#define	CR_PCICTL_DATASWAP_ABCD_CDAB		1
#define	CR_PCICTL_DATASWAP_ABCD_BADC		2
#define	CR_PCICTL_DATASWAP_ABCD_DCBA		3
#define	CR_PCICTL_ADDRSWAP			4:3
#define	CR_PCICTL_ADDRSWAP_NONE			0
#define	CR_PCICTL_ADDRSWAP_ABCD_CDAB		1
#define	CR_PCICTL_ADDRSWAP_ABCD_BADC		2
#define	CR_PCICTL_ADDRSWAP_ABCD_DCBA		3
#define	CR_PCICTL_BURSTWRITE			2:2
#define	CR_PCICTL_BURSTWRITE_DISABLE		0
#define	CR_PCICTL_BURSTWRITE_ENABLE		1
#define	CR_PCICTL_BURSTREAD			1:1
#define	CR_PCICTL_BURSTREAD_DISABLE		0
#define	CR_PCICTL_BURSTREAD_ENABLE		1
#define	CR_PCICTL_MMIO				0:0
#define	CR_PCICTL_MMIO_DISABLE			0
#define	CR_PCICTL_MMIO_ENABLE			1

#define	CR_PHYSADDRCTL				0x3a
#define	CR_PHYSADDRCTL_BUSSEL			6:6
#define	CR_PHYSADDRCTL_BUSSEL_PCI		0
#define	CR_PHYSADDRCTL_BUSSEL_AGP		1
#define	CR_PHYSADDRCTL_BOTHIO			5:5
#define	CR_PHYSADDRCTL_BOTHIO_DISABLE		0
#define	CR_PHYSADDRCTL_BOTHIO_ENABLE		1
#define	CR_PHYSADDRCTL_MAKEMEMLINEAR		4:4
#define	CR_PHYSADDRCTL_MAKEMEMLINEAR_DISABLE	0
#define	CR_PHYSADDRCTL_MAKEMEMLINEAR_ENABLE	1
#define	CR_PHYSADDRCTL_AGPRESET			2:2
#define	CR_PHYSADDRCTL_AGPRESET_NORMAL		0
#define	CR_PHYSADDRCTL_AGPRESET_RESET		1
#define	CR_PHYSADDRCTL_PCIIDWRITE		1:1
#define	CR_PHYSADDRCTL_PCIIDWRITE_DISABLE	0
#define	CR_PHYSADDRCTL_PCIIDWRITE_ENABLE	1
#define	CR_PHYSADDRCTL_ENHANCEDIO		0:0
#define	CR_PHYSADDRCTL_ENHANCEDIO_DISABLE	0
#define	CR_PHYSADDRCTL_ENHANCEDIO_ENABLE	1

#define	CR_CLKTUNE				0x3B
#define	CR_CLKTUNE_OBSERVECLK			7:7
#define	CR_CLKTUNE_OBSERVECLK_VCLK1		0
#define	CR_CLKTUNE_OBSERVECLK_VCLK2		1
#define	CR_CLKTUNE_CLKSRC			6:4
#define	CR_CLKTUNE_CLKSRC_INTERNAL		3
#define	CR_CLKTUNE_CLKSRC_EXTERNAL		4
#define	CR_CLKTUNE_INTCLKDIV			3:3
#define	CR_CLKTUNE_INTCLKDIV_1			0
#define	CR_CLKTUNE_INTCLKDIV_2			1
#define	CR_CLKTUNE_VBLANKREFRESH		0:0
#define	CR_CLKTUNE_VBLANKREFRESH_DISABLE	0
#define	CR_CLKTUNE_VBLANKREFRESH_ENABLE		1

#define	CR_MISCCTL				0x3C

#define	CR_CURSPOS				0x40
#define	CR_CURSPOS_YPOS				27:16
#define	CR_CURSPOS_XPOS				11:0

#define	CR_CURSIMGBASE_21_10			0x44

#define	CR_CURSOFFSET_X				0x46
#define	CR_CURSOFFSET_Y				0x47

#define	CR_CURSFGCOLOR				0x48
#define	CR_CURSBGCOLOR				0x4C

#define	CR_CURSCTL				0x50
#define	CR_CURSCTL_CURSOR			7:7
#define	CR_CURSCTL_CURSOR_DISABLE		0
#define	CR_CURSCTL_CURSOR_ENABLE		1
#define	CR_CURSCTL_TYPE				6:6
#define	CR_CURSCTL_TYPE_WINDOZE			0
#define	CR_CURSCTL_TYPE_X11			1
#define	CR_CURSCTL_COLORCTL3			5:5
#define	CR_CURSCTL_COLORCTL3_DISABLE		0
#define	CR_CURSCTL_COLORCTL3_ENABLE		1
#define	CR_CURSCTL_COLORCTL2			4:4
#define	CR_CURSCTL_COLORCTL2_DISABLE		0
#define	CR_CURSCTL_COLORCTL2_ENABLE		1
#define	CR_CURSCTL_SIZE				1:0
#define	CR_CURSCTL_SIZE_32x32			0	/*  Docs lied again  */
#define	CR_CURSCTL_SIZE_64x64			1
#define	CR_CURSCTL_SIZE_128x128			2

#define	CR_BUSGRANTTERMCTL			0x51
#define	CR_SHAREDFBCTL				0x52
#define	CR_PCIRETRYCTL				0x55
#define	CR_DISPPREENDCTL			0x56
#define	CR_DISPPREENDFETCHPARM			0x57
#define	CR_ZVCTL				0x5E
#define	CR_TESTCTL				0x5F

#define	CR_ENH0					0x62
#define	CR_ENH0_GEPAUSE				7:7
#define	CR_ENH0_GEPAUSE_NORMAL			0
#define	CR_ENH0_GEPAUSE_PAUSE			1
#define	CR_ENH0_PCIGERETRY			6:6
#define	CR_ENH0_PCIGERETRY_DISABLE		0
#define	CR_ENH0_PCIGERETRY_ENABLE		1
#define	CR_ENH0_SHORTCMD			5:5
#define	CR_ENH0_SHORTCMD_DISABLE		0
#define	CR_ENH0_SHORTCMD_ENABLE			1
#define	CR_ENH0_DIRECTREAD			4:4
#define	CR_ENH0_DIRECTREAD_DISABLE		0
#define	CR_ENH0_DIRECTREAD_ENABLE		1
#define	CR_ENH0_LOPRIPOLICY			2:2
#define	CR_ENH0_LOPRIPOLICY_FIXED		0
#define	CR_ENH0_LOPRIPOLICY_ROUNDROBIN		1
#define	CR_ENH0_HIPRIPOLICY			1:1
#define	CR_ENH0_HIPRIPOLICY_FIXED		0
#define	CR_ENH0_HIPRIPOLICY_ROUNDROBIN		1
#define	CR_ENH0_FBSIZE				0:0
#define	CR_ENH0_FBSIZE_8M			0
#define	CR_ENH0_FBSIZE_4M			1

#define	CR_ENH1					0x63
#define	CR_DPAEXTRA				0x64


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


/*  Shadow DACMask register (read DACMask four times to access)  */
#define	DACCMD_COLORMODE			7:4
#define	DACCMD_COLORMODE_8_CLUT			0
#define	DACCMD_COLORMODE_15_RGB			1
#define	DACCMD_COLORMODE_16_CLUT		2
#define	DACCMD_COLORMODE_16_RGB			3
#define	DACCMD_COLORMODE_24_RGB			13
#define	DACCMD_DAC				2:2
#define	DACCMD_DAC_ENABLE			0
#define	DACCMD_DAC_DISABLE			1
#define	DACCMD_RAMDAC				0:0
#define	DACCMD_RAMDAC_BYPASS			0
#define	DACCMD_RAMDAC_NORMAL			1


/*****************************************************************************
 * Rendering engine registers.
 */
/*  DDDG_BltSrcStart	0x00000 - 0x00003	*/
#define	DDDG_BLTSRCSTART_Y			27:16
#define	DDDG_BLTSRCSTART_X			11:0

/*  DDDG_BltSrcStop	0x00004 - 0x00007	*/
#define	DDDG_BLTSRCSTOP_Y			27:16
#define	DDDG_BLTSRCSTOP_X			11:0

/*  DDDG_BltDestStart	0x00008 - 0x0000B	*/
#define	DDDG_BLTDESTSTART_Y			27:16
#define	DDDG_BLTDESTSTART_X			11:0

/*  DDDG_BltDestStop	0x0000C - 0x0000F	*/
#define	DDDG_BLTDESTSTOP_Y			27:16
#define	DDDG_BLTDESTSTOP_X			11:0

/*  DDDG_Status		0x00020 - 0x00023	*/
#define	DDDG_STATUS_BRESENHAM			31:31
#define	DDDG_STATUS_BRESENHAM_IDLE		0
#define	DDDG_STATUS_BRESENHAM_BUSY		1
#define	DDDG_STATUS_SETUP			30:30
#define	DDDG_STATUS_SETUP_IDLE			0
#define	DDDG_STATUS_SETUP_BUSY			1
#define	DDDG_STATUS_SP_DPE			29:29
#define	DDDG_STATUS_SP_DPE_IDLE			0
#define	DDDG_STATUS_SP_DPE_BUSY			1
#define	DDDG_STATUS_MEMORYIF			28:28
#define	DDDG_STATUS_MEMORYIF_IDLE		0
#define	DDDG_STATUS_MEMORYIF_BUSY		1
#define	DDDG_STATUS_CMDLIST			27:27
#define	DDDG_STATUS_CMDLIST_IDLE		0
#define	DDDG_STATUS_CMDLIST_BUSY		1
#define	DDDG_STATUS_BLOCKWRITE			26:26
#define	DDDG_STATUS_BLOCKWRITE_IDLE		0
#define	DDDG_STATUS_BLOCKWRITE_BUSY		1
#define	DDDG_STATUS_CMDBUF			25:25
#define	DDDG_STATUS_CMDBUF_NOTFULL		0	/*  Is this right?  */
#define	DDDG_STATUS_CMDBUF_FULL			1
#define	DDDG_STATUS_PCIWRITEBUF			23:23
#define	DDDG_STATUS_PCIWRITEBUF_EMPTY		0
#define	DDDG_STATUS_PCIWRITEBUF_NOTEMPTY	1
#define	DDDG_STATUS_ZCHECK			22:22
#define	DDDG_STATUS_ZCHECK_FAILED		0
#define	DDDG_STATUS_ZCHECK_SUCCEEDED		1
#define	DDDG_STATUS_DISPEFFECTIVE		21:21
#define	DDDG_STATUS_DISPEFFECTIVE_NOTYET	0
#define	DDDG_STATUS_DISPEFFECTIVE_VISIBLE	1
#define	DDDG_STATUS_LEFTVIEW			20:20
#define	DDDG_STATUS_LEFTVIEW_NOTYET		0
#define	DDDG_STATUS_LEFTVIEW_VISIBLE		1
#define	DDDG_STATUS_VIEWSHOWN			19:19
#define	DDDG_STATUS_VIEWSHOWN_RIGHT		0
#define	DDDG_STATUS_VIEWSHOWN_LEFT		1
#define	DDDG_STATUS_CURDISPLINE			10:0



/*  DDDG_DrawCmd	0x00044 - 0x00047	*/
#define	DDDG_DRAWCMD_OPCODE			31:28
#define	DDDG_DRAWCMD_OPCODE_NOP			0
#define	DDDG_DRAWCMD_OPCODE_LINE		2
#define	DDDG_DRAWCMD_OPCODE_BLT			8
#define	DDDG_DRAWCMD_OPCODE_TEXT		9
#define	DDDG_DRAWCMD_OPCODE_CPUBLT		10
#define	DDDG_DRAWCMD_OPCODE_TRAPEZOID__		11	/*  Don't use  */
#define	DDDG_DRAWCMD_OPCODE_REBLT		12	/*  ???  */
#define	DDDG_DRAWCMD_OPCODE_RETEXT		13	/*  ???  */
#define	DDDG_DRAWCMD_OPCODE_TRAPEZOID		14
#define	DDDG_DRAWCMD_LINESTYLE			27:27
#define	DDDG_DRAWCMD_LINESTYLE_SOLID		0
#define	DDDG_DRAWCMD_LINESTYLE_PATTERN		1
#define	DDDG_DRAWCMD_ZOP			26:26
#define	DDDG_DRAWCMD_ZOP_DISABLE		0
#define	DDDG_DRAWCMD_ZOP_ENABLE			1
#define	DDDG_DRAWCMD_ALPHATEST			25:25
#define	DDDG_DRAWCMD_ALPHATEST_DISABLE		0
#define	DDDG_DRAWCMD_ALPHATEST_ENABLE		1
#define	DDDG_DRAWCMD_TXFUNC			24:24
#define	DDDG_DRAWCMD_TXFUNC_DISABLE		0
#define	DDDG_DRAWCMD_TXFUNC_ENABLE		1
#define	DDDG_DRAWCMD_ALPHABLEND			23:23
#define	DDDG_DRAWCMD_ALPHABLEND_DISABLE		0
#define	DDDG_DRAWCMD_ALPHABLEND_ENABLE		1
#define	DDDG_DRAWCMD_SPECULAR			22:22
#define	DDDG_DRAWCMD_SPECULAR_DISABLE		0
#define	DDDG_DRAWCMD_SPECULAR_ENABLE		1
#define	DDDG_DRAWCMD_FOG			21:21
#define	DDDG_DRAWCMD_FOG_DISABLE		0
#define	DDDG_DRAWCMD_FOG_ENABLE			1
#define	DDDG_DRAWCMD_SRCCOLOREXPAND		20:20
#define	DDDG_DRAWCMD_SRCCOLOREXPAND_DISABLE	0
#define	DDDG_DRAWCMD_SRCCOLOREXPAND_ENABLE	1
#define	DDDG_DRAWCMD_SRCCOLOR			19:19
#define	DDDG_DRAWCMD_SRCCOLOR_TRANS		0
#define	DDDG_DRAWCMD_SRCCOLOR_OPAQUE		1
#define	DDDG_DRAWCMD_SRCSURFACE			18:17
#define	DDDG_DRAWCMD_DESTSURFACE		16:15
#define	DDDG_DRAWCMD_MONOSRCOFFSET		14:12
#define	DDDG_DRAWCMD_SPECULARCOLOR		11:11
#define	DDDG_DRAWCMD_SPECULARCOLOR_NORMAL	0
#define	DDDG_DRAWCMD_SPECULARCOLOR_DOUBLE	1
#define	DDDG_DRAWCMD_TEXCHROMAKEY		10:10
#define	DDDG_DRAWCMD_TEXCHROMAKEY_DISABLE	0
#define	DDDG_DRAWCMD_TEXCHROMAKEY_ENABLE	1
#define	DDDG_DRAWCMD_LITTEX			9:9
#define	DDDG_DRAWCMD_LITTEX_DISABLE		0
#define	DDDG_DRAWCMD_LITTEX_ENABLE		1
#define	DDDG_DRAWCMD_DITHER			8:8
#define	DDDG_DRAWCMD_DITHER_DISABLE		0
#define	DDDG_DRAWCMD_DITHER_ENABLE		1
#define	DDDG_DRAWCMD_SRCCHROMAKEY		7:7
#define	DDDG_DRAWCMD_SRCCHROMAKEY_DISABLE	0
#define	DDDG_DRAWCMD_SRCCHROMAKEY_ENABLE	1
#define	DDDG_DRAWCMD_DESTCHROMAKEY		6:6
#define	DDDG_DRAWCMD_DESTCHROMAKEY_DISABLE	0
#define	DDDG_DRAWCMD_DESTCHROMAKEY_ENABLE	1
#define	DDDG_DRAWCMD_BITMASK			5:5
#define	DDDG_DRAWCMD_BITMASK_DISABLE		0
#define	DDDG_DRAWCMD_BITMASK_ENABLE		1
#define	DDDG_DRAWCMD_ROP			4:4
#define	DDDG_DRAWCMD_ROP_DISABLE		0
#define	DDDG_DRAWCMD_ROP_ENABLE			1
#define	DDDG_DRAWCMD_BLTSRC			3:2
#define	DDDG_DRAWCMD_BLTSRC_CPU			0
#define	DDDG_DRAWCMD_BLTSRC_FB			1
#define	DDDG_DRAWCMD_BLTSRC_CONST		2
#define	DDDG_DRAWCMD_BLTSRC_1			3
#define	DDDG_DRAWCMD_BLTDIR			1:1
#define	DDDG_DRAWCMD_BLTDIR_INC			0
#define	DDDG_DRAWCMD_BLTDIR_DEC			1
#define	DDDG_DRAWCMD_CLIPPING			0:0
#define	DDDG_DRAWCMD_CLIPPING_DISABLE		0
#define	DDDG_DRAWCMD_CLIPPING_ENABLE		1

/*  DDDG_ROP		0x00048 - 0x0004B	*/
#define	DDDG_ROP_ROP				7:0

/*  DDDG_{Src,Dest}StrideBuf[]	0x000B8 - 0x000BB	*/
#define	DDDG_SRCDESTSTRIDEBUF_BPP		31:29
#define	DDDG_SRCDESTSTRIDEBUF_BPP_CMAP8		0
#define	DDDG_SRCDESTSTRIDEBUF_BPP_RGB565	1
#define	DDDG_SRCDESTSTRIDEBUF_BPP_RGBx8888	2
#define	DDDG_SRCDESTSTRIDEBUF_BPP_RGB555	5
#define	DDDG_SRCDESTSTRIDEBUF_PITCH		28:20	/*  pixels / 8  */
#define	DDDG_SRCDESTSTRIDEBUF_BASEADDR		19:0	/*  64-bit units  */


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



#endif	/*  __TRIDENTDEFS_H  */
