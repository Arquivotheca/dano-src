
#ifndef __CARDSTATE_H__
#define __CARDSTATE_H__

#include <OS.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/common/MemMgr/MemMgr.h>
#include <Accelerant.h>

#include <graphics_p/radeon/defines.h>
#include <graphics_p/radeon/main.h>
#include <graphics_p/radeon/regdef.h>
#include <graphics_p/radeon/cputil.h>
#include <graphics_p/radeon/cce.h>

#define PRIMARY_PCI_MEM_SIZE (1024 * 512)

typedef struct CardInfoRec CardInfo;

#define CARD_PCI_BASE 0x10000000
#define CARD_PCI_SIZE (8 * 1024 * 1024)

#define CARD_AGP_BASE 0x20000000

typedef struct _rd_Context_Regs_rec
{
	uint32 r_RB3D_DEPTHOFFSET;
	uint32 r_RB3D_DEPTHPITCH;
	uint32 r_RB3D_ZSTENCILCNTL;
	uint32 r_RB3D_STENCILREFMASK;
	uint32 r_RB3D_COLOROFFSET;
	uint32 r_RB3D_COLORPITCH;
	uint32 r_RB3D_BLENDCNTL;
	uint32 r_RB3D_CNTL;
	uint32 r_SE_CNTL;
	uint32 r_PP_MISC;
	uint32 r_PP_CNTL;
	uint32 r_ISYNC_CNTL;
	uint32 r_PP_FOG_COLOR;
	uint32 r_RE_SOLID_COLOR;
	uint32 r_RE_WIDTH_HEIGHT;
	uint32 r_PP_TXFILTER_0;
	uint32 r_PP_TXFORMAT_0;
	uint32 r_PP_TXOFFSET_0;
	uint32 r_PP_TXCBLEND_0;
	uint32 r_PP_TXABLEND_0;
	uint32 r_PP_TFACTOR_0;
	uint32 r_PP_TXFILTER_1;
	uint32 r_PP_TXFORMAT_1;
	uint32 r_PP_TXOFFSET_1;
	uint32 r_PP_TXCBLEND_1;
	uint32 r_PP_TXABLEND_1;
	uint32 r_PP_TFACTOR_1;
	uint32 r_PP_TXFILTER_2;
	uint32 r_PP_TXFORMAT_2;
	uint32 r_PP_TXOFFSET_2;
	uint32 r_PP_TXCBLEND_2;
	uint32 r_PP_TXABLEND_2;
	uint32 r_PP_TFACTOR_2;
	
	uint32 r_RE_LINE_PATTERN;
	uint32 r_RE_LINE_STATE;
	uint32 r_PP_LUM_MATRIX;
	uint32 r_PP_CUBIC_FACES_0;
	uint32 r_PP_CUBIC_FACES_1;
	uint32 r_PP_CUBIC_FACES_2;
	
	uint32 r_PP_BORDER_COLOR_0;
	uint32 r_PP_BORDER_COLOR_1;
	uint32 r_PP_BORDER_COLOR_2;

	uint32 r_PP_ROT_MATRIX_0;
	uint32 r_PP_ROT_MATRIX_1;

	uint32 r_RB3D_ROPCNTL;
	uint32 r_RB3D_PLANEMASK;

	uint32 r_SE_LINE_WIDTH;

	uint32 r_PP_CUBIC_OFFSET_T0_0;
	uint32 r_PP_CUBIC_OFFSET_T0_1;
	uint32 r_PP_CUBIC_OFFSET_T0_2;
	uint32 r_PP_CUBIC_OFFSET_T0_3;
	uint32 r_PP_CUBIC_OFFSET_T0_4;
	uint32 r_PP_CUBIC_OFFSET_T1_0;
	uint32 r_PP_CUBIC_OFFSET_T1_1;
	uint32 r_PP_CUBIC_OFFSET_T1_2;
	uint32 r_PP_CUBIC_OFFSET_T1_3;
	uint32 r_PP_CUBIC_OFFSET_T1_4;
	uint32 r_PP_CUBIC_OFFSET_T2_0;
	uint32 r_PP_CUBIC_OFFSET_T2_1;
	uint32 r_PP_CUBIC_OFFSET_T2_2;
	uint32 r_PP_CUBIC_OFFSET_T2_3;
	uint32 r_PP_CUBIC_OFFSET_T2_4;

	uint32 r_SE_TCL_OUTPUT_VTX_FMT;
	uint32 r_SE_TCL_OUTPUT_VTX_SEL;
	uint32 r_SE_TCL_MATRIX_SELECT_0;
	uint32 r_SE_TCL_MATRIX_SELECT_1;
	
	uint32 r_SE_TCL_UCP_VERT_BLEND_CTL;
	uint32 r_SE_TCL_TEXTURE_PROC_CTL;

	uint32 r_RE_TOP_LEFT;
	uint32 r_RE_MISC;
	uint32 r_SE_COORD_FMT;

	uint32 r_SE_TCL_MATERIAL_AMBIENT_RED;
	uint32 r_SE_TCL_MATERIAL_AMBIENT_GREEN;
	uint32 r_SE_TCL_MATERIAL_AMBIENT_BLUE;
	uint32 r_SE_TCL_MATERIAL_AMBIENT_ALPHA;
	
	uint32 r_SE_TCL_MATERIAL_DIFFUSE_RED;
	uint32 r_SE_TCL_MATERIAL_DIFFUSE_GREEN;
	uint32 r_SE_TCL_MATERIAL_DIFFUSE_BLUE;
	uint32 r_SE_TCL_MATERIAL_DIFFUSE_ALPHA;
	
	uint32 r_SE_TCL_MATERIAL_SPECULAR_RED;
	uint32 r_SE_TCL_MATERIAL_SPECULAR_GREEN;
	uint32 r_SE_TCL_MATERIAL_SPECULAR_BLUE;
	uint32 r_SE_TCL_MATERIAL_SPECULAR_ALPHA;
	
	uint32 r_SE_TCL_MATERIAL_EMISSIVE_RED;
	uint32 r_SE_TCL_MATERIAL_EMISSIVE_GREEN;
	uint32 r_SE_TCL_MATERIAL_EMISSIVE_BLUE;
	uint32 r_SE_TCL_MATERIAL_EMISSIVE_ALPHA;
	
	uint32 r_SE_TCL_SHININESS;
	uint32 r_SE_TCL_PER_LIGHT_CTL_0;
	uint32 r_SE_TCL_PER_LIGHT_CTL_1;
	uint32 r_SE_TCL_PER_LIGHT_CTL_2;
	uint32 r_SE_TCL_PER_LIGHT_CTL_3;
	
	uint32 r_SE_TCL_LIGHT_MODEL_CTL;
	
	uint32 r_SE_VPORT_XSCALE;
	uint32 r_SE_VPORT_XOFFSET;
	uint32 r_SE_VPORT_YSCALE;
	uint32 r_SE_VPORT_YOFFSET;
	uint32 r_SE_VPORT_ZSCALE;
	uint32 r_SE_VPORT_ZOFFSET;
	
	float vector[128 * 4];
	float scaler[52];
} _rd_Context_Regs;


struct CardInfoRec
{
	uint8 *CardMemory;
	uint8 *CardRegs;
	uint32 IO_Base;
	uint8 *DMA_Base;
	volatile void *pciMemBase;
	volatile uint32 *scratch;

	int32 FBStride;
	int32 hwBppNum;
	_PLL_BLOCK pll;
	
	int32 xres;
	int32 yres;
	int32 bitpp;
	int32 bytepp;
	
	__mem_Allocation	*AllocCursor;
	__mem_Allocation	*AllocFrameBuffer;
	
	int32 benEngineInt;
	sem_id benEngineSem;

	uint32 primitivesIssued;
	uint32 bufferSyncNum;
	
	int32 showCursor;
	int32 isMobility;

	// CCE Related stuff
	RBINFO RingBuf;
	uint8 CCEBMFlag;
	uint8 CCEAGPFlag;
	uint8 CCEVBFlag;
	uint8 CCEIBFlag;
	uint32 CCERequestedMode;
	int32 CSQmodeIndex;
	int32 CSQPrimary;
	int32 CSQIndirect;
	uint32 bm_save_state;
	uint32 read_ptr_offset;
	uint8 readbuf[32 + 4];
	
	// Multi-context support
	_rd_Context_Regs mc_Regs;
	int32 mc_Users;
	int32 mc_ForceReset;		// flag to force full register reset.
	uint32 mc_LastID;
	
	
	int32 (*CPSubmitPackets) (CardInfo *ci, uint32 *, uint32);
	int32 (*CPIndirectSubmitPackets) (CardInfo *ci, uint32 *, uint32);
	void (*Flush) (CardInfo *ci);
	void (*Finish)(CardInfo *ci);

	char devname[B_OS_NAME_LENGTH];
	
};




#define WRITE_REG_8( reg, dat ) ((uint8 *)(ci->CardRegs + (reg)))[0] = (dat)
#define WRITE_REG_16( reg, dat ) ((uint16 *)(ci->CardRegs + (reg)))[0] = (dat)
#define WRITE_REG_32( reg, dat ) ((uint32 *)(ci->CardRegs + (reg)))[0] = (dat)

#define READ_REG_8( reg ) (((uint8 *)(ci->CardRegs + (reg)))[0])
#define READ_REG_16( reg ) (((uint16 *)(ci->CardRegs + (reg)))[0])
#define READ_REG_32( reg ) (((uint32 *)(ci->CardRegs + (reg)))[0])

/////////////////////////////////////////////////////
// PLL Register read macrox
// Macros to read and write indexed PLL registers
// in the ATI r128 register space.
#define PLL_ADDRESS(x)								\
	{	uint32 _ltmp;								\
		_ltmp = READ_REG_32(CLOCK_CNTL_INDEX);			\
		_ltmp = (_ltmp & 0x00000300) | ((x) & 0x1f);	\
		WRITE_REG_32(CLOCK_CNTL_INDEX, _ltmp);			\
	}
	
#define PLL_REGW(x,y)				\
	{	PLL_ADDRESS(x);				\
		WRITE_REG_32(CLOCK_CNTL_DATA,y);\
	}			

#define PLL_REGR(x,y)				\
	{	PLL_ADDRESS(x);				\
		y = READ_REG_32(CLOCK_CNTL_DATA);\
	}			



#endif

