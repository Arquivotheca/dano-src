#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Bitflinger.h"
#include "Cores.h"
#include "Cache.h"
#include "asm.h"

typedef void (*cv_stripper)( void *context, void *dstData, const void *srcData, int32 len );



static void MakeStripper( cvContext *con, asmContext *c )
{
	uint8 *loop;

	asm_PUSH_R( c, ESI );
	asm_PUSH_R( c, EDI );
	asm_PUSH_R( c, EBP );
	
	asm_MOV_RM( c, ECX, ESP, 0, 0, 16 );	// *context
	asm_MOV_RM( c, ESI, ESP, 0, 0, 24 );	// *src
	asm_MOV_RM( c, EDI, ESP, 0, 0, 20 );	// *dst
	asm_MOV_RM( c, EBP, ESP, 0, 0, 28 );	// *dst
	
	loop = c->pc;
	
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
	
	asm_ADD_RI( c, ESI, con->in.bytes );
	asm_ADD_RI( c, EDI, con->out.bytes );
	
	asm_DEC_R( c, EBP );
	asm_JNZ_32( c, loop - c->pc );
	
	asm_POP( c, EBP );
	asm_POP( c, EDI );
	asm_POP( c, ESI );
	asm_RET( c );
}


status_t cvConvert( void *context, int32 width, int32 height,
				int32 srcSkipPixels, int32 srcSkipRows, int32 srcRowLength, const void *srcData, 
				int32 dstSkipPixels, int32 dstSkipRows, int32 dstRowLength, void *dstData )
{
	cvContext *con = (cvContext *)context;
	cv_stripper strp;
	
	int32 x, y;
	uint8 *dst = (uint8 *)dstData;
	uint8 *src = (uint8 *)srcData;
	int32 dstStride;
	int32 srcStride;
	
	if( dstRowLength )
		dstStride = dstRowLength;
	else
		dstStride = width * con->out.bytes;

	if( srcRowLength )
		srcStride = srcRowLength;
	else
		srcStride = width * con->in.bytes;
	
	dst += (dstStride * dstSkipRows) + (dstSkipPixels * con->out.bytes);
	src += (srcStride * srcSkipRows) + (srcSkipPixels * con->in.bytes);

	if( con->stripCurrent < 0 )
	{
		con->stripCurrent = cvCacheFindEntry( con, &con->CacheStrips );
		if( con->stripCurrent < 0 )
		{
			asmContext c;
			con->stripCurrent = cvCacheMakeEntry( con, &con->CacheStrips, 8196 );
			
			c.codeBase = (uint8 *) con->CacheStrips.entry[con->stripCurrent].code;
			c.pc = c.codeBase;
			c.len = 0;
			c.writing = 1;
			
			MakeStripper( con, &c );
		}
		
	}
	
	strp = (cv_stripper) con->CacheStrips.entry[con->stripCurrent].code;
		
	for( y=0; y<height; y++ )
	{
//debugger("");
		(strp) ( con, dst, src, width );
		src += srcStride;
		dst += dstStride;
	}
	
	return B_OK;
}
