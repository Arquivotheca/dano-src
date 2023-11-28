#include <SupportDefs.h>
#include <stdio.h>
#include <GL/gl.h>
#include <malloc.h>
#include <OS.h>

#include "asm.h"


int32 getSize( int32 offset )
{
	if( !offset )
		return 0;
	if( (offset & 0xffffff00) == 0xffffff00 )
		return 1;
	if( !(offset & 0xffffff80) )
		return 1;
	return 2;
}

// asmContext: The assembly context
// offReg1: The base register for memory refrence or immediate register for nonMem
// offReg2: The index register for memory refrence multiplied by mult. Ignored for nonMem
// mult: The multiplier for the index reg. 1, 2, 4, or 8. Ignored if offReg2 is not used.
// dataReg: The data register.
// isMem: 1 for memory refrence, 0 for register content.
// offset: The offset for memory refrences.
int32 write_Addr_bytes( asmContext *c, int32 offReg1, int32 offReg2,
	int32 mult, int32 dataReg, int32 offset )
{
	uint8 b;
	int32 mod =0;
	int32 offSize;
	
	offSize = getSize( offset );
	
	dataReg = (dataReg -1 ) & 0x07;
	
	if( offReg2 )
	{
		if( offReg1 )
		{
			mod = offSize;
		}
		else
		{
			mod = 0;
			offSize = 2;
			offReg1 = EBP;
		}

		b = ( dataReg << 3) | (mod << 6) | 4;
		WRITE_BYTE( b );
		
		switch( mult )
		{
			case 1: mod=0; break;
			case 2: mod=0x40; break;
			case 4: mod=0x80; break;
			case 8: mod=0xc0; break;
		}
		
		if( offReg1 )
		{
			b = (offReg1-1) | ((offReg2-1) << 3) | mod;
			WRITE_BYTE( b );
		}
		else
		{
			// Special case
			printf( "asm.cpp E1 \n" );
			debugger("");
		}
		
	}
	else
	{
		if( offReg1 == ESP )
		{
			WRITE_BYTE( ( dataReg << 3) | (offSize << 6) | 4 );
			WRITE_BYTE( 0x24 );
		}
		else
		{
			if( offReg1 )
				b = (dataReg<<3) | (offReg1-1) | (offSize<<6);
			else
				b = (dataReg<<3) | 0x05;
			WRITE_BYTE( b );
		}
	}
	
	if( offSize == 1 )
		WRITE_BYTE( offset );
	if( offSize == 2 )
		WRITE_DWORD( offset );
	
}

#define ASM_BASIC_RI( alv, eaxv, r8_8v, r32_32v, r32_8v, mod ) { \
	int size=getSize( value ); \
	if( reg == AL ) \
	{ \
		WRITE_BYTE( alv ); \
		WRITE_BYTE( value ); \
		return; \
	} \
	if( (reg == EAX) && (size==2) )	\
	{	\
		WRITE_BYTE( eaxv );	\
		WRITE_DWORD( value );	\
		return;	\
	}	\
	if( reg >= AL )	\
	{	\
		WRITE_BYTE( r8_8v );	\
		WRITE_BYTE( 0xC0 | (mod<<3) | (reg-9) );	\
		WRITE_BYTE( value );	\
		return;	\
	}	\
	if( size==2 )	\
	{	\
		WRITE_BYTE( r32_32v );	\
		WRITE_BYTE( 0xC0 | (mod<<3) | (reg-1) );	\
		WRITE_DWORD( value );	\
		return;	\
	}	\
	WRITE_BYTE( r32_8v );	\
	WRITE_BYTE( 0xC0 | (mod<<3) | (reg-1) );	\
	WRITE_BYTE( value );	\
}


#define ASM_BASIC_RR( r8_8v, r32_32v ) { \
	if( regD >= AL )	\
	{	\
		WRITE_BYTE( r8_8v );	\
		WRITE_BYTE( 0xC0 | ((regS-9)<<3) | (regD-9) );	\
		return;	\
	}	\
	WRITE_BYTE( r32_32v );	\
	WRITE_BYTE( 0xC0 | ((regS-1)<<3) | (regD-1) );	\
}

#define ASM_BASIC_RM( r8, r32, regD, regB, regI, mult, off ) { \
	if( regD >= AL )	\
	{	\
		WRITE_BYTE( r8 );	\
	}	\
	else	\
	{	\
		WRITE_BYTE( r32 );	\
	}	\
	write_Addr_bytes( c, regB, regI, mult, regD, off ); \
}

#define ASM_BASIC_MR( r8, r32, regD, regB, regI, mult, off ) { \
	if( regD >= AL )	\
	{	\
		WRITE_BYTE( r8 );	\
	}	\
	else	\
	{	\
		WRITE_BYTE( r32 );	\
	}	\
	write_Addr_bytes( c, regB, regI, mult, regD, off ); \
}

#define ASM_BASIC_R( r8, r32, regD, mod ) { \
	if( reg >= AL ) \
	{ \
		WRITE_BYTE( r8 ); \
		WRITE_BYTE( 0xC0 | (mod<<3) | (regD-9) ); \
	} \
	else \
	{ \
		WRITE_BYTE( r32 ); \
		WRITE_BYTE( 0xC0 | (mod<<3) | (regD-1) ); \
	}	\
}
	
#define ASM_FIX_JCC_8( op ) { c[1] = offset-2; }

#define ASM_FIX_JCC_32( op ) { \
	uint32 off = offset-6; \
	c[2] = off & 0xff; \
	c[3] = (off >> 8) & 0xff; \
	c[4] = (off >> 16) & 0xff; \
	c[5] = (off >> 24) & 0xff; }

#define ASM_BASIC_JCC_8( op ) { \
	WRITE_BYTE( op ) \
	WRITE_BYTE( offset-2 ) }

#define ASM_BASIC_JCC_32( op ) { \
	WRITE_BYTE( 0x0f ) \
	WRITE_BYTE( op ) \
	WRITE_DWORD( offset-6 ) }

void asm_ADC_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x14, 0x15, 0x80, 0x81, 0x83, 2 )
void asm_ADC_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x10, 0x11 )
void asm_ADC_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x12, 0x13, regD, off_Base, off_Index, mult, offset )
void asm_ADC_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x10, 0x11, regD, off_Base, off_Index, mult, offset )

void asm_ADD_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x04, 0x05, 0x80, 0x81, 0x83, 0 )
void asm_ADD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x00, 0x01 )
void asm_ADD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x02, 0x03, regD, off_Base, off_Index, mult, offset )
void asm_ADD_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x00, 0x01, regD, off_Base, off_Index, mult, offset )

void asm_AND_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x24, 0x25, 0x80, 0x81, 0x83, 4 )
void asm_AND_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x22, 0x23 )
void asm_AND_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x22, 0x23, regD, off_Base, off_Index, mult, offset )
void asm_AND_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x20, 0x21, regD, off_Base, off_Index, mult, offset )

void asm_BSWAP( asmContext *c, int32 reg )
	{ WRITE_BYTE(0x0f); WRITE_BYTE( 0xc8+reg-1 ); }

void asm_CALL_I( asmContext *c, int32 offset )
	{ WRITE_BYTE(0xE8); WRITE_DWORD( offset ); }
void asm_CALL_R( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xff, 0xff, reg, 2 )
void asm_CALL_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xff, 0xff, 2+1, off_Base, off_Index, mult, offset )

void asm_CMP_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x3c, 0x3d, 0x80, 0x81, 0x83, 7 )
void asm_CMP_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x32, 0x33 )
void asm_CMP_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x32, 0x33, regD, off_Base, off_Index, mult, offset )
void asm_CMP_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x30, 0x31, regD, off_Base, off_Index, mult, offset )


void asm_DEC_R( asmContext *c, int32 reg )
{
	if( reg >= AL )
	{
		WRITE_BYTE( 0xfe );
		WRITE_BYTE( 0xC0 | (1<<3) | (reg-9) );
	}
	else
	{
		WRITE_BYTE( 0x48 + reg -1 );
	}
}
void asm_DEC_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xFE, 0xFF, 1+1, off_Base, off_Index, mult, offset )

void asm_DIV_R( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xf6, 0xf7, reg, 6 )
void asm_DIV_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xF6, 0xF7, 6+1, off_Base, off_Index, mult, offset )

void asm_IDIV_R( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xf6, 0xf7, reg, 7 )
void asm_IDIV_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xF6, 0xF7, 7+1, off_Base, off_Index, mult, offset )

void asm_IMUL_AR( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xf6, 0xf7, reg, 5 )
void asm_IMUL_AM( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xF6, 0xF7, 5+1, off_Base, off_Index, mult, offset )
void asm_IMUL_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
{
	WRITE_BYTE( 0x0f );
	ASM_BASIC_RM( 0xaf, 0xaf, regD, off_Base, off_Index, mult, offset )
}
void asm_IMUL_RR( asmContext *c, int32 regD, int32 regS )
{
	WRITE_BYTE( 0x0f );
	ASM_BASIC_RR( 0xaf, 0xaf )
}
void asm_IMUL_RMI( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset, int32 immed )
{
	if( (immed <= 127) && (immed >= (-128)))
	{
		// 8 bit
		ASM_BASIC_RM( 0x6b, 0x6b, regD, off_Base, off_Index, mult, offset )
		WRITE_BYTE( immed );
	}
	else
	{
		// 32 bit
		ASM_BASIC_RM( 0x69, 0x69, regD, off_Base, off_Index, mult, offset )
		WRITE_DWORD( immed );
	}
}
void asm_IMUL_RRI( asmContext *c, int32 regD, int32 regS, int32 immed )
{
	if( (immed <= 127) && (immed >= (-128)))
	{
		// 8 bit
		ASM_BASIC_RR( 0x6b, 0x6b )
		WRITE_BYTE( immed );
	}
	else
	{
		// 32 bit
		ASM_BASIC_RR( 0x69, 0x69 )
		WRITE_DWORD( immed );
	}
}


void asm_INC_R( asmContext *c, int32 reg )
{
	if( reg >= AL )
	{
		WRITE_BYTE( 0xfe );
		WRITE_BYTE( 0xC0 | (0<<3) | (reg-9) );
	}
	else
	{
		WRITE_BYTE( 0x40 + reg -1 );
	}
}
void asm_INC_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xFE, 0xFF, 0+1, off_Base, off_Index, mult, offset )
	
void asm_JA_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x77 )
void asm_JAE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x73 )
void asm_JB_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x72 )
void asm_JBE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x76 )
void asm_JC_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x72 )
void asm_JECXZ_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0xE3 )
void asm_JE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x74 )
void asm_JG_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7F )
void asm_JGE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7D )
void asm_JL_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7C )
void asm_JLE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7E )
void asm_JNA_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x76 )
void asm_JNAE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x72 )
void asm_JNB_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x73 )
void asm_JNBE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x77 )
void asm_JNC_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x73 )
void asm_JNE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x75 )
void asm_JNG_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7e )
void asm_JNGE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7c )
void asm_JNL_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7D )
void asm_JNLE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7F )
void asm_JNO_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x71 )
void asm_JNP_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7B )
void asm_JNS_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x79 )
void asm_JNZ_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x75 )
void asm_JO_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x70 )
void asm_JP_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7A )
void asm_JPE_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7A )
void asm_JPO_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x7B )
void asm_JS_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x78 )
void asm_JZ_8( asmContext *c, int32 offset ) ASM_BASIC_JCC_8( 0x74 )

void asm_JA_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x87 )
void asm_JAE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x83 )
void asm_JB_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x82 )
void asm_JBE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x86 )
void asm_JC_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x82 )
void asm_JE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x84 )
void asm_JG_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8F )
void asm_JGE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8D )
void asm_JL_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8C )
void asm_JLE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8E )
void asm_JNA_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x86 )
void asm_JNAE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x82 )
void asm_JNB_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x83 )
void asm_JNBE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x87 )
void asm_JNC_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x83 )
void asm_JNE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x85 )
void asm_JNG_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8e )
void asm_JNGE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8c )
void asm_JNL_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8D )
void asm_JNLE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8F )
void asm_JNO_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x81 )
void asm_JNP_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8B )
void asm_JNS_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x89 )
void asm_JNZ_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x85 )
void asm_JO_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x80 )
void asm_JP_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8A )
void asm_JPE_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8A )
void asm_JPO_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x8B )
void asm_JS_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x88 )
void asm_JZ_32( asmContext *c, int32 offset ) ASM_BASIC_JCC_32( 0x84 )

void asm_JMP_8( asmContext *c, int32 offset )
{
	WRITE_BYTE( 0xEB )
	WRITE_BYTE( offset-2 )
}
void asm_JMP_32( asmContext *c, int32 offset )
{
	WRITE_BYTE( 0xE9 )
	WRITE_DWORD( offset-5 )
}
void asm_JMP_32R( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xFF, 0xFF, reg, 4 )
void asm_JMP_32M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xFF, 0xFF, 4+1, off_Base, off_Index, mult, offset )


void asm_fix_JA_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x77 )
void asm_fix_JAE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x73 )
void asm_fix_JB_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x72 )
void asm_fix_JBE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x76 )
void asm_fix_JC_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x72 )
void asm_fix_JECXZ_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0xE3 )
void asm_fix_JE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x74 )
void asm_fix_JG_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7F )
void asm_fix_JGE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7D )
void asm_fix_JL_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7C )
void asm_fix_JLE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7E )
void asm_fix_JNA_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x76 )
void asm_fix_JNAE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x72 )
void asm_fix_JNB_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x73 )
void asm_fix_JNBE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x77 )
void asm_fix_JNC_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x73 )
void asm_fix_JNE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x75 )
void asm_fix_JNG_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7e )
void asm_fix_JNGE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7c )
void asm_fix_JNL_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7D )
void asm_fix_JNLE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7F )
void asm_fix_JNO_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x71 )
void asm_fix_JNP_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7B )
void asm_fix_JNS_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x79 )
void asm_fix_JNZ_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x75 )
void asm_fix_JO_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x70 )
void asm_fix_JP_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7A )
void asm_fix_JPE_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7A )
void asm_fix_JPO_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x7B )
void asm_fix_JS_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x78 )
void asm_fix_JZ_8( uint8 *c, int32 offset ) ASM_FIX_JCC_8( 0x74 )

void asm_fix_JA_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x87 )
void asm_fix_JAE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x83 )
void asm_fix_JB_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x82 )
void asm_fix_JBE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x86 )
void asm_fix_JC_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x82 )
void asm_fix_JE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x84 )
void asm_fix_JG_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8F )
void asm_fix_JGE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8D )
void asm_fix_JL_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8C )
void asm_fix_JLE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8E )
void asm_fix_JNA_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x86 )
void asm_fix_JNAE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x82 )
void asm_fix_JNB_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x83 )
void asm_fix_JNBE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x87 )
void asm_fix_JNC_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x83 )
void asm_fix_JNE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x85 )
void asm_fix_JNG_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8e )
void asm_fix_JNGE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8c )
void asm_fix_JNL_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8D )
void asm_fix_JNLE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8F )
void asm_fix_JNO_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x81 )
void asm_fix_JNP_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8B )
void asm_fix_JNS_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x89 )
void asm_fix_JNZ_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x85 )
void asm_fix_JO_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x80 )
void asm_fix_JP_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8A )
void asm_fix_JPE_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8A )
void asm_fix_JPO_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x8B )
void asm_fix_JS_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x88 )
void asm_fix_JZ_32( uint8 *c, int32 offset ) ASM_FIX_JCC_32( 0x84 )

void asm_fix_JMP_8( uint8 *c, int32 offset )
{
	c[1] = offset-2;
}
void asm_fix_JMP_32( uint8 *c, int32 offset )
{
	uint32 off = offset-6; \
	c[2] = off & 0xff; \
	c[3] = (off >> 8) & 0xff; \
	c[4] = (off >> 16) & 0xff; \
	c[5] = (off >> 24) & 0xff; \
}

void asm_LEA( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x8D, 0x8D, regD, off_Base, off_Index, mult, offset )
	
void asm_MOV_RI( asmContext *c, int32 reg, int32 value )
{
	if( reg >= AL )
	{
		WRITE_BYTE( 0xB0 + reg - 9 );
		WRITE_BYTE( value );
	}
	else
	{
		WRITE_BYTE( 0xB8 + reg - 1 );
		WRITE_DWORD( value );
	}
}
void asm_MOV_MI( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset, int32 data )
{
	ASM_BASIC_MR( 0xC6, 0xC7, 0, off_Base, off_Index, mult, offset );
	WRITE_DWORD( data );
}
void asm_MOV_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x88, 0x89 )
void asm_MOV_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x8a, 0x8b, regD, off_Base, off_Index, mult, offset )
void asm_MOV_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x88, 0x89, regD, off_Base, off_Index, mult, offset )

void asm_MOVSX_32_8_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
{
	WRITE_BYTE( 0x0f );
	ASM_BASIC_RM( 0xBe, 0xBe, regD, off_Base, off_Index, mult, offset )
}
void asm_MOVSX_32_16_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
{
	WRITE_BYTE( 0x0f );
	ASM_BASIC_RM( 0xBf, 0xBf, regD, off_Base, off_Index, mult, offset )
}

void asm_MOVZX_32_8_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
{
	WRITE_BYTE( 0x0f );
	ASM_BASIC_RM( 0xB6, 0xB6, regD, off_Base, off_Index, mult, offset )
}
void asm_MOVZX_32_16_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
{
	WRITE_BYTE( 0x0f );
	ASM_BASIC_RM( 0xB7, 0xB7, regD, off_Base, off_Index, mult, offset )
}

void asm_MUL_AR( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xf6, 0xf7, reg, 4 )
void asm_MUL_AM( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xF6, 0xF7, 4+1, off_Base, off_Index, mult, offset )

void asm_NEG_R( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xf6, 0xf7, reg, 3 )
void asm_NEG_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xF6, 0xF7, 3+1, off_Base, off_Index, mult, offset )

void asm_NOP( asmContext *c )
	WRITE_BYTE( 0x90 )

void asm_NOT_R( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xf6, 0xf7, reg, 2 )
void asm_NOT_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0xF6, 0xF7, 2+1, off_Base, off_Index, mult, offset )

void asm_OR_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x0C, 0x0D, 0x80, 0x81, 0x83, 1 )
void asm_OR_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x08, 0x09 )
void asm_OR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x0A, 0x0B, regD, off_Base, off_Index, mult, offset )
void asm_OR_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x09, 0x09, regD, off_Base, off_Index, mult, offset )

void asm_POP( asmContext *c, int32 reg )
	WRITE_BYTE( 0x58 + reg -1 )

void asm_PUSH_R( asmContext *c, int32 reg )
	WRITE_BYTE( 0x50 + reg -1 )
void asm_PUSH_I( asmContext *c, int32 reg, int32 value )
{
	if( getSize(value) == 2 )
	{
		WRITE_BYTE( 0x68 );
		WRITE_DWORD( value );
	}
	else
	{
		WRITE_BYTE( 0x6A );
		WRITE_BYTE( value );
	}
}

void asm_RET( asmContext *c )
	WRITE_BYTE( 0xC3 )

void asm_SAHF( asmContext *c )
	WRITE_BYTE( 0x9e )

void asm_SHL_R1( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xD0, 0xD1, reg, 4 )
void asm_SHL_RC( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xD2, 0xD3, reg, 4 )
void asm_SHL_RI( asmContext *c, int32 reg, int32 value )
{
	ASM_BASIC_R( 0xC0, 0xC1, reg, 4 );
	WRITE_BYTE( value );
}

void asm_SHR_R1( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xD0, 0xD1, reg, 5 )
void asm_SHR_RC( asmContext *c, int32 reg )
	ASM_BASIC_R( 0xD2, 0xD3, reg, 5 )
void asm_SHR_RI( asmContext *c, int32 reg, int32 value )
{
	ASM_BASIC_R( 0xC0, 0xC1, reg, 5 );
	WRITE_BYTE( value );
}

void asm_SBB_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x1C, 0x1D, 0x80, 0x81, 0x83, 3 )
void asm_SBB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x18, 0x19 )
void asm_SBB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x1A, 0x1B, regD, off_Base, off_Index, mult, offset )
void asm_SBB_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x19, 0x19, regD, off_Base, off_Index, mult, offset )

void asm_SUB_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x2C, 0x2D, 0x80, 0x81, 0x83, 5 )
void asm_SUB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x28, 0x29 )
void asm_SUB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x2A, 0x2B, regD, off_Base, off_Index, mult, offset )
void asm_SUB_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x29, 0x29, regD, off_Base, off_Index, mult, offset )

void asm_XOR_RI( asmContext *c, int32 reg, int32 value )
	ASM_BASIC_RI( 0x34, 0x35, 0x80, 0x81, 0x83, 6 )
void asm_XOR_RR( asmContext *c, int32 regD, int32 regS )
	ASM_BASIC_RR( 0x30, 0x31 )
void asm_XOR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_RM( 0x32, 0x33, regD, off_Base, off_Index, mult, offset )
void asm_XOR_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_BASIC_MR( 0x30, 0x31, regD, off_Base, off_Index, mult, offset )

////////////////////////////////////////////////////////////////////////////
// FP code

#define ASM_FP1_NONE( op ) { \
	WRITE_BYTE( op );	\
}

#define ASM_FP2_NONE( op1, op2 ) { \
	WRITE_BYTE( op1 );	\
	WRITE_BYTE( op2 );	\
}

#define ASM_FP1_M( op, mod, regB, regI, mult, off ) { \
	WRITE_BYTE( op );	\
	write_Addr_bytes( c, regB, regI, mult, mod+1, off ); \
}

#define ASM_FP_RR( op_0N, op_N0, op2_0N, op2_N0, regD, regS ) { \
	if( regD == ST0 )	\
	{	\
		WRITE_BYTE( op_0N );	\
		WRITE_BYTE( op2_0N + regS -1 ); \
	}	\
	else	\
	{	\
		WRITE_BYTE( op_N0 );	\
		WRITE_BYTE( op2_N0 + regD -1 ); \
	}	\
}

#define ASM_FP_R( op1, op2, reg ) { \
	WRITE_BYTE( op1 );	\
	WRITE_BYTE( op2 + reg -1 ); \
}


void asm_F2XM1( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xf0 )

void asm_FABS( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xE1 )
	
void asm_FADD_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 0, off_Base, off_Index, mult, offset )
void asm_FADD_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 0, off_Base, off_Index, mult, offset )
void asm_FADD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_FP_RR( 0xD8, 0xDC, 0xC0, 0xC0, regD, regS )
void asm_FADDP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDE, 0xC0, reg )

void asm_FCHS( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xE0 )

void asm_FCMOVB( asmContext *c, int32 reg )
	ASM_FP_R( 0xDA, 0xC0, reg )
void asm_FCMOVE( asmContext *c, int32 reg )
	ASM_FP_R( 0xDA, 0xC8, reg )
void asm_FCMOVBE( asmContext *c, int32 reg )
	ASM_FP_R( 0xDA, 0xD0, reg )
void asm_FCMOVU( asmContext *c, int32 reg )
	ASM_FP_R( 0xDA, 0xD8, reg )
void asm_FCMOVNB( asmContext *c, int32 reg )
	ASM_FP_R( 0xDB, 0xC0, reg )
void asm_FCMOVNE( asmContext *c, int32 reg )
	ASM_FP_R( 0xDB, 0xC8, reg )
void asm_FCMOVNBE( asmContext *c, int32 reg )
	ASM_FP_R( 0xDB, 0xD0, reg )
void asm_FCMOVNU( asmContext *c, int32 reg )
	ASM_FP_R( 0xDB, 0xD8, reg )

void asm_FCOM_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 2, off_Base, off_Index, mult, offset )
void asm_FCOM_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 2, off_Base, off_Index, mult, offset )
void asm_FCOM_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xD8, 0xD0, reg )
void asm_FCOMP_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 3, off_Base, off_Index, mult, offset )
void asm_FCOMP_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 3, off_Base, off_Index, mult, offset )
void asm_FCOMP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xD8, 0xD8, reg )
void asm_FCOMPP( asmContext *c )
	ASM_FP2_NONE( 0xDE, 0xD9 )

void asm_FCOMI( asmContext *c, int32 reg )
	ASM_FP_R( 0xDB, 0xF0, reg )
void asm_FCOMIP( asmContext *c, int32 reg )
	ASM_FP_R( 0xDF, 0xF0, reg )
void asm_FUCOMI( asmContext *c, int32 reg )
	ASM_FP_R( 0xDB, 0xE8, reg )
void asm_FUCOMIP( asmContext *c, int32 reg )
	ASM_FP_R( 0xDF, 0xE8, reg )

void asm_FCOS( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xFF )

void asm_FDIV_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 6, off_Base, off_Index, mult, offset )
void asm_FDIV_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 6, off_Base, off_Index, mult, offset )
void asm_FDIV_RR( asmContext *c, int32 regD, int32 regS )
	ASM_FP_RR( 0xD8, 0xDC, 0xF0, 0xF8, regD, regS )
void asm_FDIVP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDE, 0xF8, reg )

void asm_FDIVR_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 7, off_Base, off_Index, mult, offset )
void asm_FDIVR_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 7, off_Base, off_Index, mult, offset )
void asm_FDIVR_RR( asmContext *c, int32 regD, int32 regS )
	ASM_FP_RR( 0xD8, 0xDC, 0xF8, 0xF0, regD, regS )
void asm_FDIVRP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDE, 0xF0, reg )

void asm_FILD_16( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDF, 0, off_Base, off_Index, mult, offset )
void asm_FILD_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDB, 0, off_Base, off_Index, mult, offset )
void asm_FILD_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDF, 5, off_Base, off_Index, mult, offset )

void asm_FIST_16( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDF, 2, off_Base, off_Index, mult, offset )
void asm_FIST_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDB, 2, off_Base, off_Index, mult, offset )

void asm_FISTP_16( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDF, 3, off_Base, off_Index, mult, offset )
void asm_FISTP_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDB, 3, off_Base, off_Index, mult, offset )
void asm_FISTP_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDF, 7, off_Base, off_Index, mult, offset )

void asm_FLD_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD9, 0, off_Base, off_Index, mult, offset )
void asm_FLD_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDD, 0, off_Base, off_Index, mult, offset )
void asm_FLD_80( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDB, 5, off_Base, off_Index, mult, offset )
void asm_FLD_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xD9, 0xC0, reg )

void asm_FLD1( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xE8 )
void asm_FLDL2T( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xE9 )
void asm_FLDL2E( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xEA )
void asm_FLDPI( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xEB )
void asm_FLDLG2( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xEC )
void asm_FLDLN2( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xED )
void asm_FLDZ( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xEE )

void asm_FMUL_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 1, off_Base, off_Index, mult, offset )
void asm_FMUL_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 1, off_Base, off_Index, mult, offset )
void asm_FMUL_RR( asmContext *c, int32 regD, int32 regS )
	ASM_FP_RR( 0xD8, 0xDC, 0xC8, 0xC8, regD, regS )
void asm_FMULP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDE, 0xC8, reg )

void asm_FNSTSW_A( asmContext *c )
{
	WRITE_BYTE( 0xDF );
	WRITE_BYTE( 0xE0 );
}

void asm_FPATAN( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xF3 )

void asm_FPREM( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xF8 )
void asm_FPREM1( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xF5 )

void asm_FPTAN( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xF2 )

void asm_FRNDINT( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xFC )

void asm_FSIN( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xFE )

void asm_FSINCOS( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xFB )

void asm_FSQRT( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xFA )

void asm_FST_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD9, 2, off_Base, off_Index, mult, offset )
void asm_FST_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDD, 2, off_Base, off_Index, mult, offset )
void asm_FST_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDD, 0xD0, reg )

void asm_FSTP_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD9, 3, off_Base, off_Index, mult, offset )
void asm_FSTP_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDD, 3, off_Base, off_Index, mult, offset )
void asm_FSTP_80( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDB, 7, off_Base, off_Index, mult, offset )
void asm_FSTP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDD, 0xD8, reg )

void asm_FSUB_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 4, off_Base, off_Index, mult, offset )
void asm_FSUB_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 4, off_Base, off_Index, mult, offset )
void asm_FSUB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_FP_RR( 0xD8, 0xDC, 0xE0, 0xE8, regD, regS )
void asm_FSUBP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDE, 0xE8, reg )

void asm_FSUBR_32( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xD8, 5, off_Base, off_Index, mult, offset )
void asm_FSUBR_64( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_FP1_M( 0xDC, 5, off_Base, off_Index, mult, offset )
void asm_FSUBR_RR( asmContext *c, int32 regD, int32 regS )
	ASM_FP_RR( 0xD8, 0xDC, 0xE8, 0xE0, regD, regS )
void asm_FSUBRP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDE, 0xE0, reg )

void asm_FUCOM_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDD, 0xE0, reg )
void asm_FUCOMP_R( asmContext *c, int32 reg )
	ASM_FP_R( 0xDD, 0xE8, reg )
void asm_FUCOMPP( asmContext *c )
	ASM_FP2_NONE( 0xDA, 0xE9 )

void asm_FXAM( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xE5 )

void asm_FXCH( asmContext *c, int32 reg )
	ASM_FP_R( 0xD9, 0xC8, reg )

void asm_FXTRACT( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xF4 )

void asm_FYL2X( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xF1 )

void asm_FYL2XP1( asmContext *c )
	ASM_FP2_NONE( 0xD9, 0xF9 )


// MMX stuff

#define ASM_MMX_RI( r32, mod ) { \
	WRITE_BYTE( 0x0f );	\
	WRITE_BYTE( r32 );	\
	WRITE_BYTE( 0xC0 | (mod<<3) | (reg-1) );	\
	WRITE_BYTE( immed8 );	\
}

#define ASM_MMX_RR( r32_32v ) { \
	WRITE_BYTE( 0x0f );	\
	WRITE_BYTE( r32_32v );	\
	WRITE_BYTE( 0xC0 | ((regS-1)<<3) | (regD-1) );	\
}

#define ASM_MMX_RRI( r32_32v ) { \
	WRITE_BYTE( 0x0f );	\
	WRITE_BYTE( r32_32v );	\
	WRITE_BYTE( 0xC0 | ((regS-1)<<3) | (regD-1) );	\
	WRITE_BYTE( immed8 );	\
}

#define ASM_MMX_RM( r32 ) { \
	WRITE_BYTE( 0x0f );	\
	WRITE_BYTE( r32 );	\
	write_Addr_bytes( c, off_Base, off_Index, mult, regD, offset ); \
}

#define ASM_MMX_M( r32, mod ) { \
	WRITE_BYTE( 0x0f );	\
	WRITE_BYTE( r32 );	\
	write_Addr_bytes( c, off_Base, off_Index, mult, mod, offset ); \
}

#define ASM_MMX_RMI( r32 ) { \
	WRITE_BYTE( 0x0f );	\
	WRITE_BYTE( r32 );	\
	write_Addr_bytes( c, off_Base, off_Index, mult, regD, offset ); \
	WRITE_BYTE( immed8 );	\
}


void asm_MOVQ_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x7f )
void asm_MOVQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x6f )
void asm_MOVQ_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x7f )

void asm_MOVD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x7e )
void asm_MOVD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x6e )
void asm_MOVD_MR( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x7e )

void asm_PACKSSWB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x63 )
void asm_PACKSSWB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x63 )
void asm_PACKSSDW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x6B )
void asm_PACKSSDW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x6B )

void asm_PACKUSWB_RR( asmContext *c, int32 regS, int32 regD )  //Reversed
	ASM_MMX_RR( 0x67 )
void asm_PACKUSWB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x67 )

void asm_PADDB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xFC )
void asm_PADDB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xFC )
void asm_PADDW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xFD )
void asm_PADDW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xFD )
void asm_PADDD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xFE )
void asm_PADDD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xFE )

void asm_PADDSB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xEC )
void asm_PADDSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xEC )
void asm_PADDSW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xED )
void asm_PADDSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xED )

void asm_PADDUSB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xDC )
void asm_PADDUSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xDC )
void asm_PADDUSW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xDC )
void asm_PADDUSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xDC )

void asm_PAND_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xDB )
void asm_PAND_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xDB )

void asm_PANDN_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xDF )
void asm_PANDN_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xDF )

void asm_PAVGB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE0 )
void asm_PAVGB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE0 )
void asm_PAVGW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE3 )
void asm_PAVGW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE3 )

void asm_PCMPEQB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x74 )
void asm_PCMPEQB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x74 )
void asm_PCMPEQW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x75 )
void asm_PCMPEQW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x75 )
void asm_PCMPEQD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x76 )
void asm_PCMPEQD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x76 )

void asm_PCMPGTB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x64 )
void asm_PCMPGTB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x64 )
void asm_PCMPGTW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x65 )
void asm_PCMPGTW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x65 )
void asm_PCMPGTD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x66 )
void asm_PCMPGTD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x66 )

void asm_PEXTRW_RRI( asmContext *c, int32 regD, int32 regS, uint8 immed8 )
	ASM_MMX_RRI( 0xC5 )

void asm_PINSRW_RRI( asmContext *c, int32 regD, int32 regS, uint8 immed8 )
	ASM_MMX_RRI( 0xC4 )

void asm_PMADDWD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xF5 )
void asm_PMADDWD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xF5 )

void asm_PMAXSW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xEE )
void asm_PMAXSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xEE )

void asm_PMAXUB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xDE )
void asm_PMAXUB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xDE )

void asm_PMINSW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xEA )
void asm_PMINSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xEA )

void asm_PMINUB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xDA )
void asm_PMINUB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xDA )

void asm_PMOVMSKB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xD7 )

void asm_PMULHUW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE4 )
void asm_PMULHUW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE4 )

void asm_PMULHW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE5 )
void asm_PMULHW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE5 )

void asm_PMULLW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xD5 )
void asm_PMULLW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xD5 )

void asm_POR_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xEB )
void asm_POR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xEB )

void asm_PREFETCH0_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_M( 0x18, 1 )
void asm_PREFETCH1_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_M( 0x18, 2 )
void asm_PREFETCH2_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_M( 0x18, 3 )
void asm_PREFETCHNTA_M( asmContext *c, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_M( 0x18, 0 )
 
void asm_PSADBW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xF6 )
void asm_PSADBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xF6 )

void asm_PSHUFW_RRI( asmContext *c, int32 regD, int32 regS, uint8 immed8 )
	ASM_MMX_RRI( 0x70 )
void asm_PSHUFW_RMI( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset, uint8 immed8 )
	ASM_MMX_RMI( 0x70 )

void asm_PSLLW_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x71, 6 )
void asm_PSLLW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xF1 )
void asm_PSLLW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xF1 )
void asm_PSLLD_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x72, 6 )
void asm_PSLLD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xF2 )
void asm_PSLLD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xF2 )
void asm_PSLLQ_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x73, 6 )
void asm_PSLLQ_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xF3 )
void asm_PSLLQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xF3 )

void asm_PSRAW_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x71, 4 )
void asm_PSRAW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE1 )
void asm_PSRAW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE1 )
void asm_PSRAD_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x72, 4 )
void asm_PSRAD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE2 )
void asm_PSRAD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE2 )

void asm_PSRLW_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x71, 2 )
void asm_PSRLW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xD1 )
void asm_PSRLW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xD1 )
void asm_PSRLD_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x72, 2 )
void asm_PSRLD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xD2 )
void asm_PSRLD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xD2 )
void asm_PSRLQ_RI( asmContext *c, int32 reg, int8 immed8 )
	ASM_MMX_RI( 0x73, 2 )
void asm_PSRLQ_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xD3 )
void asm_PSRLQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xD3 )

void asm_PSUBB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xF8 )
void asm_PSUBB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xF8 )
void asm_PSUBW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xF9 )
void asm_PSUBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xF9 )
void asm_PSUBD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xFA )
void asm_PSUBD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xFA )

void asm_PSUBSB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE8 )
void asm_PSUBSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE8 )
void asm_PSUBSW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xE9 )
void asm_PSUBSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xE9 )

void asm_PSUBUSB_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xD8 )
void asm_PSUBUSB_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xD8 )
void asm_PSUBUSW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xD9 )
void asm_PSUBUSW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xD9 )

void asm_PUNPCKHBW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x68 )
void asm_PUNPCKHBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x68 )
void asm_PUNPCKHWD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x69 )
void asm_PUNPCKHWD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x69 )
void asm_PUNPCKHDQ_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x6A )
void asm_PUNPCKHDQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x6a )

void asm_PUNPCKLBW_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x60 )
void asm_PUNPCKLBW_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x60 )
void asm_PUNPCKLWD_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x61 )
void asm_PUNPCKLWD_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x61 )
void asm_PUNPCKLDQ_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0x62 )
void asm_PUNPCKLDQ_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0x62 )

void asm_PXOR_RR( asmContext *c, int32 regD, int32 regS )
	ASM_MMX_RR( 0xEF )
void asm_PXOR_RM( asmContext *c, int32 regD, int32 off_Base, int32 off_Index, int32 mult, int32 offset )
	ASM_MMX_RM( 0xEF )

void asm_EMMS( asmContext *c )
	{
		WRITE_BYTE( 0x0f );
		WRITE_BYTE( 0x77 );
	}

