/********************************************************************/
/* MMX converter	 												*/
/* This converter only supports INT src and dst types				*/
/* The max supported resolution is 8 bits per component				*/
/********************************************************************/

#include <stdio.h>
#include <debugger.h>
#include <string.h>

#include "Bitflinger.h"
#include "asm.h"

//

// 4.12 tables for MMX regs
static uint16 tbl_1_to_16[2];
static uint16 tbl_2_to_16[4];
static uint16 tbl_3_to_16[8];
static uint16 tbl_4_to_16[16];
static uint16 tbl_5_to_16[32];
static uint16 tbl_6_to_16[64];
static uint16 tbl_7_to_16[128];
static uint16 tbl_8_to_16[256];

#define MUL_RR( regD, regF ) { \
	asm_PMULHW_RR( c, regD, regF ); \
	asm_PSLLW_RR( c, regD, 4 ); \
	}

#define MUL_RM( regD, off_Base, off_Index, mult, offset ) { \
	asm_PMULHW_RM( c, regD, off_Base, off_Index, mult, offset ); \
	asm_PSLLW_RR( c, regD, 4 ); \
	}

static uint32 const_One[2] = { 0x10001000, 0x10001000 };
static uint32 const_Zero[2] = { 0x00000000, 0x00000000 };
static uint32 const_C0A1[2] = { 0x00000000, 0x00001000 };
static uint32 const_XXXX[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
static uint32 const_XXX0[2] = { 0xFFFFFFFF, 0xFFFF0000 };
static uint32 const_000X[2] = { 0x00000000, 0x0000FFFF };


void __cvMakeMMXTables()
{
	uint32 ct;
	
	tbl_1_to_16[0] = 0;
	tbl_1_to_16[1] = 0xfff;
	
	for( ct=0; ct<4; ct++ )
		tbl_2_to_16[ct] = (uint16)(0xfff * (((float)ct) / 3.0));
	for( ct=0; ct<8; ct++ )
		tbl_3_to_16[ct] = (uint16)(0xfff * (((float)ct) / 7.0));
	for( ct=0; ct<16; ct++ )
		tbl_4_to_16[ct] = (uint16)(0xfff * (((float)ct) / 15.0));
	for( ct=0; ct<32; ct++ )
		tbl_5_to_16[ct] = (uint16)(0xfff * (((float)ct) / 31.0));
	for( ct=0; ct<64; ct++ )
		tbl_6_to_16[ct] = (uint16)(0xfff * (((float)ct) / 63.0));
	for( ct=0; ct<128; ct++ )
		tbl_7_to_16[ct] = (uint16)(0xfff * (((float)ct) / 127.0));
	for( ct=0; ct<256; ct++ )
		tbl_8_to_16[ct] = (uint16)(0xfff * (((float)ct) / 255.0));
		
}


static void Gen_MMX_Blender( cvContext *con, asmContext *c, int32 srcReg, int32 dstReg, int32 scratch1, int32 scratch2, int32 scratch3 )
{
	// Source Function
	switch( con->op.blendSrcFunc )
	{
	case GL_ZERO:
		asm_PXOR_RR( c, scratch1, scratch1 );
		break;
	case GL_ONE:
		asm_MOVQ_RR( c, scratch1, srcReg );
		break;
	case GL_DST_COLOR:
		asm_MOVQ_RR( c, scratch1, srcReg );
		MUL_RR( scratch1, dstReg );
		break;
	case GL_ONE_MINUS_DST_COLOR:
		asm_MOVQ_RM( c, scratch1, 0, 0, 0, (int32)&const_One[0] );
		asm_PSUBSW_RR( c, scratch1, dstReg );
		MUL_RR( scratch1, srcReg );
		break;
	case GL_SRC_ALPHA:
		asm_MOVQ_RR( c, scratch1, srcReg );
		asm_PSHUFW_RRI( c, scratch1, scratch1, 0 );	// Broadcase alpha
		MUL_RR( scratch1, srcReg );
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		asm_MOVQ_RM( c, scratch1, 0, 0, 0, (int32)&const_One[0] );
		asm_PSUBSW_RR( c, scratch1, srcReg );
		asm_PSHUFW_RRI( c, scratch1, scratch1, 0 );	// Broadcase alpha
		MUL_RR( scratch1, srcReg );
		break;
	case GL_DST_ALPHA:
		asm_MOVQ_RR( c, scratch1, dstReg );
		asm_PSHUFW_RRI( c, scratch1, scratch1, 0 );	// Broadcase alpha
		MUL_RR( scratch1, srcReg );
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		asm_MOVQ_RM( c, scratch1, 0, 0, 0, (int32)&const_One[0] );
		asm_PSUBSW_RR( c, scratch1, dstReg );
		asm_PSHUFW_RRI( c, scratch1, scratch1, 0 );	// Broadcase alpha
		MUL_RR( scratch1, srcReg );
		break;
	case GL_SRC_ALPHA_SATURATE:
		asm_MOVQ_RR( c, scratch1, srcReg );						// Load source
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&const_One[0] );
		asm_PSUBSW_RR( c, scratch2, dstReg );					// Get 1-dst
		asm_PMINSW_RR( c, scratch2, scratch1 );
		asm_PSHUFW_RRI( c, scratch2, scratch2, 0 );	// Broadcase alpha
		asm_MOVQ_RM( c, scratch3, 0, 0, 0, (int32)&const_One[0] );
		asm_PINSRW_RRI( c, scratch2, scratch3, 3 );	// Force A mult to 1
		MUL_RR( scratch1, scratch2 );
		break;
	case GL_CONSTANT_COLOR:
		asm_MOVQ_RM( c, scratch1, 0, 0, 0, (int32)&con->opt.blendConstColor );
		MUL_RR( scratch1, srcReg );
		break;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		asm_MOVQ_RM( c, scratch1, 0, 0, 0, (int32)&con->opt.oneMinusBlendConstColor );
		MUL_RR( scratch1, srcReg );
		break;
	case GL_CONSTANT_ALPHA:
		asm_MOVQ_RM( c, scratch1, 0, 0, 0, (int32)&con->opt.blendConstAlpha );
		MUL_RR( scratch1, srcReg );
		break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		asm_MOVQ_RM( c, scratch1, 0, 0, 0, (int32)&con->opt.oneMinusBlendConstAlpha );
		MUL_RR( scratch1, srcReg );
		break;
	}


	// Destination Function
	switch( con->op.blendDstFunc )
	{
	case GL_ZERO:
		asm_PXOR_RR( c, scratch2, scratch2 );
		break;
	case GL_ONE:
		asm_MOVQ_RR( c, scratch2, dstReg );
		break;
	case GL_SRC_COLOR:
		asm_MOVQ_RR( c, scratch2, dstReg );
		MUL_RR( scratch2, srcReg );
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&const_One[0] );
		asm_PSUBSW_RR( c, scratch2, srcReg );
		MUL_RR( scratch2, dstReg );
		break;
	case GL_SRC_ALPHA:
		asm_MOVQ_RR( c, scratch2, srcReg );
		asm_PSHUFW_RRI( c, scratch2, scratch2, 0 );	// Broadcase alpha
		MUL_RR( scratch2, dstReg );
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&const_One[0] );
		asm_PSUBSW_RR( c, scratch2, srcReg );
		asm_PSHUFW_RRI( c, scratch2, scratch2, 0 );	// Broadcase alpha
		MUL_RR( scratch2, dstReg );
		break;
	case GL_DST_ALPHA:
		asm_MOVQ_RR( c, scratch2, dstReg );
		asm_PSHUFW_RRI( c, scratch2, scratch2, 0 );	// Broadcase alpha
		MUL_RR( scratch2, dstReg );
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&const_One[0] );
		asm_PSUBSW_RR( c, scratch2, dstReg );
		asm_PSHUFW_RRI( c, scratch2, scratch2, 0 );	// Broadcase alpha
		MUL_RR( scratch2, dstReg );
		break;
	case GL_CONSTANT_COLOR:
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&con->opt.blendConstColor );
		MUL_RR( scratch2, dstReg );
		break;
	case GL_ONE_MINUS_CONSTANT_COLOR:
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&con->opt.oneMinusBlendConstColor );
		MUL_RR( scratch2, dstReg );
		break;
	case GL_CONSTANT_ALPHA:
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&con->opt.blendConstAlpha );
		MUL_RR( scratch2, dstReg );
		break;
	case GL_ONE_MINUS_CONSTANT_ALPHA:
		asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&con->opt.oneMinusBlendConstAlpha );
		MUL_RR( scratch2, dstReg );
		break;
	}
	
	
	// Now lets apply the Blend equation
	switch( con->op.blendEqu )
	{
	case GL_FUNC_ADD:
		asm_PADDSW_RR( c, scratch1, scratch2 );
		break;
	case GL_FUNC_SUBTRACT:
		asm_PSUBSW_RR( c, scratch1, scratch2 );
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		asm_PSUBSW_RR( c, scratch2, scratch1 );
		asm_MOVQ_RR( c, scratch1, scratch2 );
		break;
	case GL_MIN:
		asm_PMINSW_RR( c, scratch1, scratch2 );
		break;
	case GL_MAX:
		asm_PMAXSW_RR( c, scratch1, scratch2 );
		break;
	}

	asm_PMINSW_RM( c, srcReg, 0, 0, 0, (int32)&const_One[0] );
	asm_MOVQ_RM( c, scratch2, 0, 0, 0, (int32)&const_Zero[0] );
	asm_PMAXSW_RR( c, srcReg, scratch2 );

}


static void MakeComponentLoader( cvContext *con, asmContext *c, int32 Bits, int32 HBit )
{
	if( Bits == 8 )
	{
		asm_MOVZX_32_8_RM( c, EAX, ESI, 0, 0, (HBit - Bits +1) / 8 );
		if( con->in.isSigned )
			asm_XOR_RI( c, EAX, 0x80 );
	}
	else
	{
		if( HBit > 15 )
		{
			asm_MOV_RM( c, EAX, ESI, 0, 0, (HBit - Bits +1) / 8 );
		}
		else
		{
			asm_MOVZX_32_16_RM( c, EAX, ESI, 0, 0, (HBit - Bits +1) / 8 );
		}
		if( HBit >= Bits )
			asm_SHR_RI( c, EAX, HBit - Bits +1 );
		if( con->in.isSigned )
			asm_XOR_RI( c, EAX, 1 << HBit );
		asm_AND_RI( c, EAX, (1 << Bits) -1 );
	}
	
	switch( Bits )
	{
		case 0:
			asm_MOV_RI( c, EAX, 0xfff );
			break;
		case 1:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_1_to_16[0] );
			break;
		case 2:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_2_to_16[0] );
			break;
		case 3:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_3_to_16[0] );
			break;
		case 4:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_4_to_16[0] );
			break;
		case 5:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_5_to_16[0] );
			break;
		case 6:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_6_to_16[0] );
			break;
		case 7:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_7_to_16[0] );
			break;
		case 8:
			asm_MOVZX_32_16_RM( c, EAX, 0, EAX, 2, (uint32)&tbl_8_to_16[0] );
			break;
	}
}


static void MakeLoader( cvContext *con, asmContext *c, PixelMode *pmode, int32 reg )
{
	static uint32 const_C0A1[2] = { 0x00000000, 0x00001000 };

	asm_MOVQ_RM( c, reg, 0, 0, 0, (int32)&const_C0A1[0] );

	if( pmode->R_Bits )
	{
		MakeComponentLoader( con, c, pmode->R_Bits, pmode->R_HBit );
		asm_PINSRW_RRI( c, reg, EAX, 0 );
	}
	
	if( pmode->G_Bits )
	{
		if( !((pmode->R_Bits == pmode->G_Bits) && (pmode->R_HBit == pmode->G_HBit)) )
			MakeComponentLoader( con, c, pmode->G_Bits, pmode->G_HBit );
		asm_PINSRW_RRI( c, reg, EAX, 1 );
	}

	if( pmode->B_Bits )
	{
		if( !((pmode->G_Bits == pmode->B_Bits) && (pmode->G_HBit == pmode->B_HBit)) )
			MakeComponentLoader( con, c, pmode->B_Bits, pmode->B_HBit );
		asm_PINSRW_RRI( c, reg, EAX, 2 );
	}

	if( pmode->A_Bits )
	{
		if( !((pmode->B_Bits == pmode->A_Bits) && (pmode->B_HBit == pmode->A_HBit)) )
			MakeComponentLoader( con, c, pmode->A_Bits, pmode->A_HBit );
		asm_PINSRW_RRI( c, reg, EAX, 3 );
	}

	
}

// MMX0 = source pixel
// EDX = dest pixel
// EBP = *context
// EAX, ECX, EDX = volatile
// EBX = reserved
static void ShiftOneComponentOut( cvContext *con, asmContext *c, int32 sBits, int32 sHBit, int32 dBits, int32 dHBit, bool isFirst )
{
	int32 dreg;
	uint32 srcMask;
	int32 shift = sHBit - dHBit;
	bool mask = true;

	if( isFirst )
		dreg = EDX;
	else
		dreg = ECX;
	
	if( dBits >= sBits )
		srcMask = ((1 << sBits)-1) << (sHBit - sBits +1);
	else
		srcMask = (((1 << dBits)-1) << (sBits - dBits) ) << (sHBit - sBits +1);
	
	asm_MOVD_RR( c, dreg, MM0 );
	if( con->in.isSigned ^ con->out.isSigned )
		asm_XOR_RI( c, dreg, 1 << sHBit );

	if( shift > 0 )
	{
		if( mask )
		{
			asm_AND_RI( c, dreg, srcMask );
			asm_SHR_RI( c, dreg, shift );
		}
		else
		{
			asm_SHR_RI( c, dreg, shift );
		}
	}
	else
	{
		if( shift < 0 )
		{
			if( mask )
			{
				asm_AND_RI( c, dreg, srcMask );
				asm_SHL_RI( c, dreg, -shift );
			}
			else
			{
				asm_SHR_RI( c, dreg, -shift );
			}
		}
		else // shift == 0
		{
			if( mask )
			{
				asm_AND_RI( c, dreg, srcMask );
			}
		}
	}		

	if( !isFirst )
		asm_OR_RR( c, EDX, ECX );

}


void __cvBitGen_x86_MMX_Core( cvContext *con, asmContext *c )
{
	bool isFirst = true;

	MakeLoader( con, c, &con->in, MM0 );
	
	if( con->opt.needDest )
		MakeLoader( con, c, &con->out, MM1 );
		
	if( con->opt.needScale )
	{
		MUL_RM( MM0, 0, 0, 0, (int32)&con->opt.scale.r );
	}

	if( con->opt.needBias )
	{
		asm_PADDSW_RM( c, MM0, 0, 0, 0, (int32)&con->opt.bias.r );
	}

		
	if( con->op.envMode != GL_REPLACE )
	{
		switch( con->op.envMode )
		{
			case GL_MODULATE:
				asm_MOVQ_RM( c, MM2, 0, 0, 0, (int32)&con->opt.envColor );
				MUL_RR( MM0, MM2 );
				break;
			case GL_DECAL:
				asm_MOVQ_RR( c, MM2, MM0 );
				asm_PSHUFW_RRI( c, MM2, MM2, 0 );	// Broadcase alpha
				MUL_RR( MM0, MM2 );
				asm_MOVQ_RM( c, MM3, 0, 0, 0, (int32)&const_One[0] );
				asm_PSUBSW_RR( c, MM3, MM2 );		// 1 - s.a
				asm_MOVQ_RM( c, MM2, 0, 0, 0, (int32)&con->opt.envColor );
				MUL_RR( MM3, MM2 );
				asm_PADDSW_RR( c, MM0, MM3 );
				break;
			case GL_BLEND:
				asm_MOVQ_RM( c, MM2, 0, 0, 0, (int32)&con->opt.envConstColor );
				MUL_RR( MM2, MM0 );
				asm_MOVQ_RM( c, MM3, 0, 0, 0, (int32)&const_One[0] );
				asm_PSUBSW_RR( c, MM3, MM0 );		// 1 - s
				asm_MOVQ_RM( c, MM4, 0, 0, 0, (int32)&con->opt.envColor );
				MUL_RR( MM3, MM4 );
				MUL_RR( MM0, MM4 );
				asm_PADDSW_RR( c, MM2, MM3 );
				asm_PAND_RM( c, MM3, 0, 0, 0, (int32)&const_XXX0[0] );
				asm_PAND_RM( c, MM0, 0, 0, 0, (int32)&const_000X[0] );
				asm_POR_RR( c, MM0, MM3 );
				break;
		}
		
	}

	if( con->op.blendEnabled )
		Gen_MMX_Blender( con, c, MM0, MM1, MM2, MM3, MM4 );
	
	switch( con->op.logicFunc )
	{
	case GL_CLEAR:
		asm_PXOR_RR( c, MM0, MM0 );
		break;
	case GL_SET:
		asm_MOVQ_RM( c, MM0, 0, 0, 0, (int32)&const_One[0] );
		break;
	case GL_COPY:
		break;
	case GL_COPY_INVERTED:
		asm_PXOR_RM( c, MM0, 0, 0, 0, (int32)&const_XXXX[0] );
		break;
	case GL_NOOP:
		asm_MOVQ_RR( c, MM0, MM1 );
		break;
	case GL_INVERT:
		asm_MOVQ_RR( c, MM0, MM1 );
		asm_PXOR_RM( c, MM0, 0, 0, 0, (int32)&const_XXXX[0] );
		break;
	case GL_AND:
		asm_PAND_RR( c, MM0, MM1 );
		break;
	case GL_NAND:
		asm_PANDN_RR( c, MM0, MM1 );
		break;
	case GL_OR:
		asm_POR_RR( c, MM0, MM1 );
		break;
	case GL_NOR:
		asm_PXOR_RM( c, MM0, 0, 0, 0, (int32)&const_XXXX[0] );
		asm_POR_RR( c, MM0, MM1 );
		break;
	case GL_XOR:
		asm_PXOR_RR( c, MM0, MM1 );
		break;
	case GL_EQUIV:
		asm_PXOR_RR( c, MM0, MM1 );
		asm_PXOR_RM( c, MM0, 0, 0, 0, (int32)&const_XXXX[0] );
		break;
	case GL_AND_REVERSE:
		asm_MOVQ_RR( c, MM2, MM1 );
		asm_PXOR_RM( c, MM2, 0, 0, 0, (int32)&const_XXXX[0] );
		asm_PAND_RR( c, MM0, MM2 );
		break;
	case GL_AND_INVERTED:
		asm_PXOR_RM( c, MM0, 0, 0, 0, (int32)&const_XXXX[0] );
		asm_PAND_RR( c, MM0, MM1 );
		break;
	case GL_OR_REVERSE:
		asm_MOVQ_RR( c, MM2, MM1 );
		asm_PXOR_RM( c, MM2, 0, 0, 0, (int32)&const_XXXX[0] );
		asm_POR_RR( c, MM0, MM2 );
		break;
	case GL_OR_INVERTED:
		asm_PXOR_RM( c, MM0, 0, 0, 0, (int32)&const_XXXX[0] );
		asm_POR_RR( c, MM0, MM1 );
		break;
	}
	
	asm_PXOR_RR( c, MM1, MM1 );
	asm_PSRAW_RI( c, MM0, 4 );
	asm_PACKUSWB_RR( c, MM0, MM1 );
	
	
	if( con->out.R_Bits	)
	{
		ShiftOneComponentOut( con, c, 8, 7, con->out.R_Bits, con->out.R_HBit, isFirst );
		isFirst = false;
	}
	if( con->out.G_Bits	)
	{
		ShiftOneComponentOut( con, c, 8, 15, con->out.G_Bits, con->out.G_HBit, isFirst );
		isFirst = false;
	}
	if( con->out.B_Bits	)
	{
		ShiftOneComponentOut( con, c, 8, 23, con->out.B_Bits, con->out.B_HBit, isFirst );
		isFirst = false;
	}
	if( con->out.A_Bits	)
	{
		ShiftOneComponentOut( con, c, 8, 31, con->out.A_Bits, con->out.A_HBit, isFirst );
		isFirst = false;
	}
	
//	asm_MOV_RI( c, EDX, 0x77777777 );

	switch( con->out.bytes )
	{
		case 1:
			asm_MOV_MR( c, DL, EDI, 0, 0, 0 );
			break;
		case 2:
			asm_MOV_MR( c, DL, EDI, 0, 0, 0 );
			asm_MOV_MR( c, DH, EDI, 0, 0, 1 );
			break;
		case 3:
			asm_MOV_MR( c, DL, EDI, 0, 0, 0 );
			asm_MOV_MR( c, DH, EDI, 0, 0, 1 );
			asm_SHR_RI( c, EDX, 16 );
			asm_MOV_MR( c, DL, EDI, 0, 0, 2 );
			break;
		case 4:
			asm_MOV_MR( c, EDX, EDI, 0, 0, 0 );
			break;
	}

	asm_EMMS( c );

}




