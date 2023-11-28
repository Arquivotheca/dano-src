/********************************************************************/
/* Integer converter 												*/
/********************************************************************/

#include <stdio.h>
#include <debugger.h>
#include <string.h>

#include "Bitflinger.h"
#include "asm.h"


static uint32 tbl_1_to_32[2];
static uint32 tbl_2_to_32[4];
static uint32 tbl_3_to_32[8];
static uint32 tbl_4_to_32[16];
static uint32 tbl_5_to_32[32];
static uint32 tbl_6_to_32[64];
static uint32 tbl_7_to_32[128];
static uint32 tbl_8_to_32[256];

void __cvMakeIntTables()
{
	uint32 ct;
	
	tbl_1_to_32[0] = 0;
	tbl_1_to_32[1] = 0xffffffff;
	
	for( ct=0; ct<4; ct++ )
		tbl_2_to_32[ct] = 0xffffffff / (4 - ct);
	for( ct=0; ct<8; ct++ )
		tbl_3_to_32[ct] = 0xffffffff / (8 - ct);
	for( ct=0; ct<16; ct++ )
		tbl_4_to_32[ct] = 0xffffffff / (16 - ct);
	for( ct=0; ct<32; ct++ )
		tbl_5_to_32[ct] = 0xffffffff / (32 - ct);
	for( ct=0; ct<64; ct++ )
		tbl_6_to_32[ct] = 0xffffffff / (64 - ct);
	for( ct=0; ct<128; ct++ )
		tbl_7_to_32[ct] = 0xffffffff / (128 - ct);
	for( ct=0; ct<256; ct++ )
		tbl_8_to_32[ct] = 0xffffffff / (256 - ct);
		
}

// ESI = *source pixel
// EDI = *dest pixel
// EBP = *context
// EAX, ECX, EDX = volatile
// EBX = reserved
static void GenOneComponent( cvContext *con, asmContext *c, int32 sBits, int32 sHBit, int32 dBits, int32 dHBit, bool isFirst )
{
	int32 dreg;
	uint32 srcMask;
	if( isFirst )
		dreg = EDX;
	else
		dreg = ECX;
	
	if( dBits >= sBits )
		srcMask = ((1 << sBits)-1) << (sHBit - sBits +1);
	else
		srcMask = (((1 << dBits)-1) << (sBits - dBits) ) << (sHBit - sBits +1);

//printf( "srcMask %x,   isSigned %i \n", srcMask, con->in.isSigned );
//printf( "sBits %i   dBits %i \n", sBits, dBits );
//printf( "sHBit %i   dHBit %i \n", sHBit, dHBit );
	
	if( sBits >= dBits )
	{
		// Downsample or copy
		int32 shift = sHBit - dHBit;
		bool mask = true;
//printf( "shift %i \n", shift );

		asm_MOV_RR( c, dreg, EAX );
		if( con->in.isSigned )
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
	}
	else
	{
		// Upsample
		asm_MOV_RR( c, dreg, EAX );
		if( sHBit >= sBits )
			asm_SHR_RI( c, dreg, sHBit - sBits +1 );
		if( con->in.isSigned )
			asm_XOR_RI( c, dreg, 1 << sHBit );
		asm_AND_RI( c, dreg, (1 << sBits) -1 );
		
		switch( sBits )
		{
			case 0:
				asm_MOV_RI( c, dreg, 0xffffffff );
				break;
			case 1:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_1_to_32[0] );
				break;
			case 2:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_2_to_32[0] );
				break;
			case 3:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_3_to_32[0] );
				break;
			case 4:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_4_to_32[0] );
				break;
			case 5:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_5_to_32[0] );
				break;
			case 6:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_6_to_32[0] );
				break;
			case 7:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_7_to_32[0] );
				break;
			case 8:
				asm_MOV_RM( c, dreg, 0, EAX, 4, (uint32)&tbl_8_to_32[0] );
				break;
		}
		
		asm_SHR_RI( c, dreg, 32 - dBits );
		if( dHBit >= dBits )
			asm_SHL_RI( c, dreg, dHBit - dBits +1 );
	}

	if( !isFirst )
		asm_OR_RR( c, EDX, ECX );

}


// Core handles SRC & DST sizees of up to 4 bytes.
void __cvBitGen_x86_Int_Core( cvContext *con, asmContext *c )
{
	bool isFirst = true;
		
	switch( con->in.bytes )
	{
		case 1:
			asm_MOVZX_32_8_RM( c, EAX, ESI, 0, 0, 0 );	break;
		case 2:
			asm_MOVZX_32_16_RM( c, EAX, ESI, 0, 0, 0 ); break;
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

	if( con->out.R_Bits	)
	{
		GenOneComponent( con, c, con->in.R_Bits, con->in.R_HBit, con->out.R_Bits, con->out.R_HBit, isFirst );
		isFirst = false;
	}
	if( con->out.G_Bits	)
	{
		GenOneComponent( con, c, con->in.G_Bits, con->in.G_HBit, con->out.G_Bits, con->out.G_HBit, isFirst );
		isFirst = false;
	}
	if( con->out.B_Bits	)
	{
		GenOneComponent( con, c, con->in.B_Bits, con->in.B_HBit, con->out.B_Bits, con->out.B_HBit, isFirst );
		isFirst = false;
	}
	if( con->out.A_Bits	)
	{
		GenOneComponent( con, c, con->in.A_Bits, con->in.A_HBit, con->out.A_Bits, con->out.A_HBit, isFirst );
		isFirst = false;
	}
	
	switch( con->out.bytes )
	{
		case 1:
			asm_MOV_MR( c, DL, EDI, 0, 0, 0 );	break;
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



