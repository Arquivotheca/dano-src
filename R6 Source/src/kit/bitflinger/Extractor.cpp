#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Bitflinger.h"
#include "Cores.h"
#include "asm.h"
#include "Cache.h"


static void MakeExtractor( cvContext *con, asmContext *c,
					int32 width, int32 height,
					int32 skipPixels, int32 skipRows, int32 rowLength )
{
	asm_PUSH_R( c, ESI );
	asm_PUSH_R( c, EDI );
	
	asm_MOV_RM( c, ECX, ESP, 0, 0, 12 );	// *context
	asm_MOV_RM( c, EDI, ESP, 0, 0, 24 );	// *pixel
	
	asm_MOV_RM( c, EAX, ESP, 0, 0, 20 );	// Y
	asm_MOV_RM( c, ESI, ECX, 0, 0, ((int32)&con->exBaseOffset) - ((int32)con));
	asm_ADD_RM( c, ESI, ESP, 0, 0, 28 ); 	// src data
	asm_MUL_AM( c, ECX, 0, 0, ((int32)&con->exStride) - ((int32)con));
	asm_ADD_RR( c, ESI, EAX );				// esi = base + y*stride;

	asm_MOV_RM( c, EAX, ESP, 0, 0, 16 );	// X
	switch( con->in.bytes )
	{
		case 1: break;
		case 2:
			asm_SHL_RI( c, EAX, 1 ); break;
		case 3:
			asm_LEA( c, EAX, EAX, EAX, 2, 0 ); break;
		case 4:
			asm_SHL_RI( c, EAX, 2 ); break;
		case 5:
			asm_LEA( c, EAX, EAX, EAX, 4, 0 ); break;
		case 6:
			asm_LEA( c, EAX, EAX, EAX, 2, 0 );
			asm_SHL_RI( c, EAX, 1 ); break;
		case 8:
			asm_SHL_RI( c, EAX, 3 ); break;
		case 12:
			asm_SHL_RI( c, EAX, 3 ); break;
		case 16:
			asm_SHL_RI( c, EAX, 3 ); break;
		default:
			asm_MUL_AM( c, ECX, 0, 0, ((int32)&con->in.bytes) - ((int32)con));
	}
	asm_ADD_RR( c, ESI, EAX );
	
	if( (con->in.bytes <= 4) &&
		(con->out.bytes <= 4) &&
		con->transferMode.isSimple )
	{
//		__cvBitGen_x86_MMX_Core( con, c );
		__cvBitGen_x86_Int_Core( con, c );
	}
	else
	{
		__cvBitGen_x86_Float_Core( con, c );
	}
	
	asm_POP( c, EDI );
	asm_POP( c, ESI );
	asm_RET( c );
}

cv_extractor cvPickExtractor( void *context, int32 width, int32 height,
					int32 skipPixels, int32 skipRows, int32 rowLength )
{
	cvContext *con = (cvContext *)context;
	int32 stride;
	
	if( rowLength )
		stride = rowLength;
	else
		stride = width * con->in.bytes;
		
	con->exBaseOffset = (stride * skipRows) + (skipPixels * con->in.bytes);
	con->exStride = stride;

	if( con->exCurrent < 0 )
	{
		con->exCurrent = cvCacheFindEntry( con, &con->CacheExtractor );
		if( con->exCurrent < 0 )
		{
			asmContext c;
			con->exCurrent = cvCacheMakeEntry( con, &con->CacheExtractor, 8196 );
			
			c.codeBase = (uint8 *) con->CacheExtractor.entry[con->exCurrent].code;
			c.pc = c.codeBase;
			c.len = 0;
			c.writing = 1;
			MakeExtractor( con, &c, width, height, skipPixels, skipRows, rowLength );
		}
	}
		
	return (cv_extractor) con->CacheExtractor.entry[con->exCurrent].code;
}

