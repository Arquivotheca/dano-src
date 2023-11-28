/********************************************************************/
/* Float converter 													*/
/********************************************************************/

#include <stdio.h>
#include <math.h>
#include <debugger.h>
#include <string.h>

#include "Bitflinger.h"
#include "asm.h"

// ESI = *source pixel
// EDI = *dest pixel
// EBP = *context
// EAX, ECX, EDX = volatile
// EBX = reserved

static double f32768 = 32768.0;
static double f2147483648 = 2147483648.0;

static double fpMultipliers[32] = {
	1.0,
	3.0,
	7.0,
	15.0,
	31.0,
	63.0,
	127.0,
	255.0,		// 8
	511.0,
	1023.0,
	2047.0,
	4095.0,
	8191.0,
	16383.0,
	32767.0,
	65535.0,	// 16
	131071.0,
	262143.0,
	524287.0,
	1048575.0,
	2097151.0,
	4194303.0,
	8388607.0,
	16777215.0,	// 24
	33554431.0,
	67108863.0,
	134217727.0,
	268435455.0,
	536870911.0,
	1073741823.0,
	2147483647.0,
	4294967295.0 };

static double fpDivisors[32] = {
	(1.0 / 1.0),
	(1.0 / 3.0),
	(1.0 / 7.0),
	(1.0 / 15.0),
	(1.0 / 31.0),
	(1.0 / 63.0),
	(1.0 / 127.0),
	(1.0 / 255.0),		// 8
	(1.0 / 511.0),
	(1.0 / 1023.0),
	(1.0 / 2047.0),
	(1.0 / 4095.0),
	(1.0 / 8191.0),
	(1.0 / 16383.0),
	(1.0 / 32767.0),
	(1.0 / 65535.0),	// 16
	(1.0 / 131071.0),
	(1.0 / 262143.0),
	(1.0 / 524287.0),
	(1.0 / 1048575.0),
	(1.0 / 2097151.0),
	(1.0 / 4194303.0),
	(1.0 / 8388607.0),
	(1.0 / 16777215.0),	// 24
	(1.0 / 33554431.0),
	(1.0 / 67108863.0),
	(1.0 / 134217727.0),
	(1.0 / 268435455.0),
	(1.0 / 536870911.0),
	(1.0 / 1073741823.0),
	(1.0 / 2147483647.0),
	(1.0 / 4294967295.0) };
	
	

static void LoadOneFloat( cvContext *con, asmContext *c, int32 sBits, int32 sHBit )
{
	// LSB == 0
	switch( con->in.Type )
	{
		case GL_SHORT:
			asm_FILD_16( c, ESI, 0, 0, (sHBit - sBits +1) / 8 );
			asm_FADD_64( c, 0, 0, 0, (int32)&f32768 );
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpDivisors[15] );
			break;
		case GL_UNSIGNED_SHORT:
			asm_MOVZX_32_16_RM( c, EAX, ESI, 0, 0, (sHBit - sBits +1) / 8 );
			asm_PUSH_R( c, EAX );
			asm_FILD_32( c, ESP, 0, 0, 0 );
			asm_POP( c, EAX );
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpDivisors[15] );
			break;
		case GL_INT:
			asm_FILD_32( c, ESI, 0, 0, (sHBit - sBits +1) / 8 );
			asm_FADD_64( c, 0, 0, 0, (int32)&f32768 );
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpDivisors[31] );
			break;
		case GL_UNSIGNED_INT:
			asm_MOV_RM( c, EAX, ESI, 0, 0, (sHBit - sBits +1) / 8 );
			asm_XOR_RI( c, EAX, 0x80000000 );
			asm_PUSH_R( c, EAX );
			asm_FILD_32( c, ESI, 0, 0, (sHBit - sBits +1) / 8 );
			asm_POP( c, EAX );
			asm_FADD_64( c, 0, 0, 0, (int32)&f32768 );
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpDivisors[31] );
			break;
		case GL_FLOAT:
			asm_FLD_32( c, ESI, 0, 0, (sHBit - sBits +1) / 8 );
			break;
		case GL_DOUBLE:
			asm_FLD_64( c, ESI, 0, 0, (sHBit - sBits +1) / 8 );
			break;
			
		default:
		{
			// Ok we need some fancy bit shifting here.
			// but we know if will be 4 bytes or less for the source
			switch( con->in.bytes )
			{
				case 1:
					asm_MOVZX_32_8_RM( c, EAX, ESI, 0, 0, 0 );
					break;
				case 2:
					asm_MOVZX_32_16_RM( c, EAX, ESI, 0, 0, 0 );
					break;
				case 3:
					asm_MOVZX_32_8_RM( c, ECX, ESI, 0, 0, 1 );
					asm_MOVZX_32_8_RM( c, EAX, ESI, 0, 0, 0 );
					asm_SHL_RI( c, ECX, 8 );
					asm_OR_RR( c, EAX, ECX );
					asm_MOVZX_32_8_RM( c, ECX, ESI, 0, 0, 2 );
					asm_SHL_RI( c, ECX, 16 );
					asm_OR_RR( c, EAX, ECX );
					break;
				case 4:
					asm_MOV_RM( c, EAX, ESI, 0, 0, 0 ); break;
			}
			// EAX contains zero extended source bits
			
			if( con->in.isSigned )
				asm_XOR_RI( c, EAX, 1 << sHBit );
				
			if( sHBit - sBits +1 )
				asm_SHR_RI( c, EAX, (sHBit - sBits +1) );
			
			asm_AND_RI( c, EAX, (1 << sBits) -1 );
			asm_PUSH_R( c, EAX );
			asm_FILD_32( c, ESP, 0, 0, 0 );
			asm_POP( c, EAX );
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpDivisors[sBits -1] );
		}
	}

}

static void StoreOneFloat( cvContext *con, asmContext *c, int32 dBits, int32 dHBit, bool isFirst,
	float *scale, float *bias, MapEntry *map )
{
	bool needClamp = false;
	int32 dreg;
	static float fudge = 0.0001;
	
	if( isFirst )
		dreg = EDX;
	else
		dreg = ECX;
		
	if( fabs( scale[0] - 1.0 ) > fudge )
	{
		asm_FMUL_32( c, 0, 0, 0, (int32)scale );
		needClamp = true;
	}

	if( fabs( bias[0] ) > fudge )
	{
		asm_FADD_32( c, EBP, 0, 0, (int32)bias );
		needClamp = true;
	}
	
	if( needClamp || (con->transferMode.MapColor && map->Size) )
	{
		uint8 *pc;

		asm_FLD1( c );
		asm_FLDZ( c );
		asm_FXCH( c, ST2 );				// val  1.0  0.0

		asm_FCOM_R( c, ST2 );
		asm_FNSTSW_A( c );
		asm_SAHF( c );
		pc = c->pc;
		asm_JNB_8( c, 0 );
		asm_FSTP_R( c, ST0 );
		asm_FLDZ( c );
		asm_fix_JNB_8( pc, c->pc - pc );

		asm_FCOM_R( c, ST1 );
		asm_FNSTSW_A( c );
		asm_SAHF( c );
		pc = c->pc;
		asm_JNA_8( c, 0 );
		asm_FSTP_R( c, ST0 );
		asm_FLD1( c );
		asm_fix_JNA_8( pc, c->pc - pc );

		asm_FXCH( c, ST2 );
		asm_FSTP_R( c, ST0 );
		asm_FSTP_R( c, ST0 );
	}
	
	if( con->transferMode.MapColor )
	{
		if( map->Size )
		{
			static float f_0_5 = 0.5;
			
			asm_FILD_32( c, 0, 0, 0, (int32)&map->Size );		// Map size
	//		asm_FLD1( c );
	//		asm_FSUBP_R( c, ST1 );				// (map size)-1
			
			asm_FMULP_R( c, ST1 );
			asm_FSUB_32( c, 0, 0, 0, (int32)&f_0_5 );
			
			asm_PUSH_R( c, EAX );
			asm_FISTP_32( c, ESP, 0, 0, 0 );
			asm_POP( c, EAX );				// INDEX
			
			asm_SHL_RI( c, EAX, 2 );
			asm_ADD_RM( c, EAX, 0, 0, 0, (uint32) &map->Map );		// Add offset
			
			asm_FLD_32( c, EAX, 0, 0, 0 );	
		}
	}
	
	// LSB == 0
	switch( con->out.Type )
	{
		case GL_SHORT:
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpMultipliers[15] );
			asm_FSUB_64( c, 0, 0, 0, (int32)&f32768 );
			asm_FISTP_32( c, EDI, 0, 0, (dHBit - dBits +1) / 8 );
			break;
		case GL_UNSIGNED_SHORT:
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpMultipliers[15] );
			asm_FISTP_32( c, EDI, 0, 0, (dHBit - dBits +1) / 8 );
			break;
		case GL_INT:
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpMultipliers[31] );
			asm_FSUB_64( c, 0, 0, 0, (int32)&f32768 );
			asm_FISTP_32( c, EDI, 0, 0, (dHBit - dBits +1) / 8 );
			break;
		case GL_UNSIGNED_INT:
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpMultipliers[31] );
			asm_FISTP_32( c, EDI, 0, 0, (dHBit - dBits +1) / 8 );
			break;
		case GL_FLOAT:
			asm_FSTP_32( c, EDI, 0, 0, (dHBit - dBits +1) / 8 );
			break;
		case GL_DOUBLE:
			asm_FSTP_64( c, EDI, 0, 0, (dHBit - dBits +1) / 8 );
			break;
			
		default:
		{
			// Ok we need some fancy bit shifting here.
			// but we know if will be 4 bytes or less for the dest
			asm_PUSH_R( c, EAX );
			asm_FMUL_64( c, 0, 0, 0, (int32)&fpMultipliers[dBits-1] );
			asm_FISTP_32( c, ESP, 0, 0, 0 );
			asm_POP( c, dreg );
			if( dBits - dHBit -1 )
				asm_SHL_RI( c, dreg, dHBit - dBits +1 );
			
			if( !isFirst )
				asm_OR_RR( c, EDX, ECX );
		}
	}

	
}


// Core handles SRC & DST of any size.
void __cvBitGen_x86_Float_Core( cvContext *con, asmContext *c )
{
	bool isFirst = true;
	int32 rreg = 0;
	int32 greg = 0;
	int32 breg = 0;
	int32 areg = 0;
	
	
	if( con->out.R_Bits	)
	{
		if( con->in.R_Bits )
			LoadOneFloat( con, c, con->in.R_Bits, con->in.R_HBit );
		else
			asm_FLDZ( c );
	}
	if( con->out.G_Bits	)
	{
		rreg++;
		if( con->in.G_Bits )
			LoadOneFloat( con, c, con->in.G_Bits, con->in.G_HBit );
		else
			asm_FLDZ( c );
	}
	if( con->out.B_Bits	)
	{
		rreg++;
		greg++;
		if( con->in.B_Bits )
			LoadOneFloat( con, c, con->in.B_Bits, con->in.B_HBit );
		else
			asm_FLDZ( c );
	}
	if( con->out.A_Bits	)
	{
		rreg++;
		greg++;
		breg++;
		if( con->in.A_Bits )
			LoadOneFloat( con, c, con->in.A_Bits, con->in.A_HBit );
		else
			asm_FLD1( c );
	}
	
	if( con->out.R_Bits	)
	{
		if( rreg )
		{
			asm_FXCH( c, ST0 + rreg );
			
			if( con->out.A_Bits	&& (!areg) )
			{
				int32 tmp = rreg;
				rreg = areg;
				areg = tmp;
			}
			else
			{
				if( con->out.B_Bits	&& (!breg) )
				{
					int32 tmp = rreg;
					rreg = breg;
					breg = tmp;
				}
				else
				{
					if( con->out.B_Bits	&& (!greg) )
					{
						int32 tmp = rreg;
						rreg = greg;
						greg = tmp;
					}
				}
			}
		}
		StoreOneFloat( con, c, con->out.R_Bits, con->out.R_HBit, isFirst,
			&con->transferMode.ScaleR, &con->transferMode.BiasR, &con->pixelMap[GL_PIXEL_MAP_R_TO_R - GL_PIXEL_MAP_I_TO_I] );
		isFirst = false;
		areg--;
		breg--;
		greg--;
	}


	if( con->out.G_Bits	)
	{
		if( greg )
		{
			asm_FXCH( c, ST0 + greg );
			
			if( con->out.A_Bits	&& (!areg) )
			{
				int32 tmp = greg;
				greg = areg;
				areg = tmp;
			}
			else
			{
				if( con->out.B_Bits	&& (!breg) )
				{
					int32 tmp = greg;
					greg = breg;
					breg = tmp;
				}
			}
		}
		
		StoreOneFloat( con, c, con->out.G_Bits, con->out.G_HBit, isFirst,
			&con->transferMode.ScaleG, &con->transferMode.BiasG, &con->pixelMap[GL_PIXEL_MAP_G_TO_G - GL_PIXEL_MAP_I_TO_I] );
		isFirst = false;
		areg--;
		breg--;
	}

	if( con->out.B_Bits	)
	{
		if( breg )
		{
			asm_FXCH( c, ST0 + breg );
			
			if( con->out.A_Bits	&& (!areg) )
			{
				int32 tmp = breg;
				breg = areg;
				areg = tmp;
			}
		}
		
		StoreOneFloat( con, c, con->out.B_Bits, con->out.B_HBit, isFirst,
			&con->transferMode.ScaleB, &con->transferMode.BiasB, &con->pixelMap[GL_PIXEL_MAP_B_TO_B - GL_PIXEL_MAP_I_TO_I] );
		isFirst = false;
		areg--;
		breg--;
	}

	if( con->out.A_Bits	)
	{
		StoreOneFloat( con, c, con->out.A_Bits, con->out.A_HBit, isFirst,
			&con->transferMode.ScaleA, &con->transferMode.BiasA, &con->pixelMap[GL_PIXEL_MAP_A_TO_A - GL_PIXEL_MAP_I_TO_I] );
	}
		
	
	switch( con->out.Type )
	{
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
		case GL_DOUBLE:
			// Store one float already handled these cases
			break;
			
		default:
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
					asm_MOV_MR( c, EDX, EDI, 0, 0, 0 ); break;
			}
	}
		
}


