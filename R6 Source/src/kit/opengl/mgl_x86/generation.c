/************************************************************
*  scanline_generation.c
*
*  Contains code for generating the polyfin routines at runtime.
*
*************************************************************/

#include <GL/gl.h>
#include <malloc.h>

#include "mgl.h"

#include "assembler.h"
#include "code_cache.h"
#include "stdio.h"

#include "generation.h"


#define DEF_PART(p) extern GLubyte fillsub_##p;  extern GLubyte fillsub_##p##_end

DEF_PART( Init );
DEF_PART( Uninit );

DEF_PART( YTest1 );
DEF_PART( YTest2 );

DEF_PART( CalcOffset );
DEF_PART( CalcOffset_load );
DEF_PART( CalcOffset_color32 );
DEF_PART( CalcOffset_color16 );
DEF_PART( CalcOffset_color32 );
DEF_PART( CalcOffset_Z );
DEF_PART( loop_init_color );
DEF_PART( loop_smooth );
DEF_PART( loop_flat );
DEF_PART( loop_color_write );
DEF_PART( loop_Z_write );
DEF_PART( loop_Z_test );
DEF_PART( loop_finish );
DEF_PART( loop_tex_fetch_addr );

DEF_PART( init_color );
DEF_PART( init_depth_fog );
DEF_PART( init_tex0 );
DEF_PART( init_tex1 );

DEF_PART( loop_tex_fetch_1 );
DEF_PART( loop_tex_fetch_2 );
DEF_PART( loop_tex_fetch_3 );
DEF_PART( loop_tex_fetch_4 );

DEF_PART( step_color );
DEF_PART( step_zf );
DEF_PART( step_x );
DEF_PART( step_tex0 );


static int64 calcSubFillNeeds( __mglContext *gc )
{
	int64 needs = 0;

	needs |= gc->state.colorWriteEnable << 0;
	needs |= gc->state.stippleTestEnabled << 13;
	needs |= gc->state.shadeFunc << 14;

//	needs |= gc->state.fogFunc <<
		
	if( gc->state.texture[0].surface.format )
	{
		needs |= ((int64)gc->state.texture[0].surface.format) << 16;
		needs |= ((int64)gc->state.texture[0].envMode) << 20;
		needs |= ((int64)gc->state.texture[0].sWrapMode) << 22;
		needs |= ((int64)gc->state.texture[0].tWrapMode) << 23;
		needs |= ((int64)gc->state.texture[0].rWrapMode) << 24;
		needs |= ((int64)gc->state.texture[0].filter) << 25;
	}

	if( gc->state.texture[1].surface.format )
	{
		needs |= ((int64)gc->state.texture[1].surface.format) << 28;
		needs |= ((int64)gc->state.texture[1].envMode) << 32;
		needs |= ((int64)gc->state.texture[1].sWrapMode) << 34;
		needs |= ((int64)gc->state.texture[1].tWrapMode) << 35;
		needs |= ((int64)gc->state.texture[1].rWrapMode) << 36;
		needs |= ((int64)gc->state.texture[1].filter) << 37;
	}
		
	if( gc->state.depthTestFunc )
	{
		needs |= gc->state.depthTestFunc << 4;		//Be carefull here
		if( gc->state.depthWriteEnabled )
			needs |= __SUBFILL_DEPTH_WRITE;
	}
	
	
	
	return needs;	
}

status_t __mgl_Init( __mglContext **context, uint32 cache_size )
{
	void *buf = malloc( sizeof( __mglContext ) + 15 );
	__mglContext *con;
	uint32 b;
	
	if( !buf )
		return B_ERROR;
		
printf( "__mgl_Init \n" );
	b = (uint32)buf;
	b = (b+0xf) & (~0xf);
	
	con = (__mglContext *)b;
	memset( con, 0, sizeof( __mglContext) );
	
	con->contextAllocation = buf;
	
	__cc_init( &con->cache, cache_size );

	*context = con;
	return B_OK;
}

void __mgl_Uninit( __mglContext *con )
{
	__cc_uninit( con->cache );
	free( con->contextAllocation );
}

static void * buildScanlineCore( __mglContext *gc, void *pc, int64 needs )
{
	void *scan_loop=0;
	void *depth_test=0;
	void *zero_test=0;

	pc = asm_BLOCK( fillsub_Init );
	pc = asm_BLOCK( fillsub_CalcOffset_load );
	pc = asm_BLOCK( fillsub_CalcOffset );
	
	zero_test = pc;
	pc = asm_Jcc( pc, 0, cc_JLE );
	
	
	pc = asm_BLOCK( fillsub_CalcOffset_color32 );

	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
	{
		pc = asm_BLOCK( fillsub_CalcOffset_Z );
		pc = asm_BLOCK( fillsub_init_depth_fog );
	}
	
	pc = asm_BLOCK( fillsub_loop_init_color );
	
	scan_loop = pc;

	if( needs & __SUBFILL_TEX0_FORMAT_MASK )
	{
		pc = asm_BLOCK( fillsub_loop_tex_fetch_addr );
		pc = asm_BLOCK( fillsub_loop_tex_fetch_3 );
	}
	else
	{
		if( needs & __SUBFILL_SMOOTH )
			pc = asm_BLOCK( fillsub_loop_smooth );
		else
			pc = asm_BLOCK( fillsub_loop_flat );
	}
	
	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
	{
		pc = asm_BLOCK( fillsub_loop_Z_test );
		depth_test = pc;
		pc = asm_Jcc( pc, 0, cc_JLE );
	}
	pc = asm_BLOCK( fillsub_loop_color_write );
	
	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
	{	
		pc = asm_BLOCK( fillsub_loop_Z_write );
		asm_Jcc( depth_test, pc, cc_JA );
	}

	pc = asm_BLOCK( fillsub_loop_finish );
	pc = asm_Jcc( pc, scan_loop, cc_JNE );

	asm_Jcc( zero_test, pc, cc_JLE );

	pc = asm_BLOCK( fillsub_Uninit );
	return pc;
}


static void * buildTrapCore( __mglContext *gc, void *pc, int64 needs )
{
	void *line_loop=0;
	void *scan_loop=0;
	void *depth_test=0;
	void *zero_test=0;
	void *y_test=0;
	void *y_loop=0;

	pc = asm_BLOCK( fillsub_Init );
	
	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
		pc = asm_BLOCK( fillsub_init_depth_fog );
	pc = asm_BLOCK( fillsub_loop_init_color );

	y_loop = pc;
	pc = asm_BLOCK( fillsub_YTest1 );
	y_test = pc;
	pc = asm_Jcc( pc, 0, cc_JGE );
	pc = asm_BLOCK( fillsub_YTest2 );
	
	
	pc = asm_BLOCK( fillsub_CalcOffset );
	
	zero_test = pc;
	pc = asm_Jcc( pc, 0, cc_JLE );
	
	
	pc = asm_BLOCK( fillsub_CalcOffset_color32 );

	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
	{
		pc = asm_BLOCK( fillsub_CalcOffset_Z );
	}
	
	scan_loop = pc;

	if( needs & __SUBFILL_TEX0_FORMAT_MASK )
	{
		pc = asm_BLOCK( fillsub_loop_tex_fetch_addr );
		pc = asm_BLOCK( fillsub_loop_tex_fetch_3 );
	}
	else
	{
		if( needs & __SUBFILL_SMOOTH )
			pc = asm_BLOCK( fillsub_loop_smooth );
		else
			pc = asm_BLOCK( fillsub_loop_flat );
	}
	
	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
	{
		pc = asm_BLOCK( fillsub_loop_Z_test );
		depth_test = pc;
		pc = asm_Jcc( pc, 0, cc_JLE );
	}
	
	pc = asm_BLOCK( fillsub_loop_color_write );

	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
	{
		pc = asm_BLOCK( fillsub_loop_Z_write );
		asm_Jcc( depth_test, pc, cc_JA );
	}
	

	pc = asm_BLOCK( fillsub_loop_finish );
	pc = asm_Jcc( pc, scan_loop, cc_JNE );

	asm_Jcc( zero_test, pc, cc_JLE );


	pc = asm_BLOCK( fillsub_step_x );
	if( needs & __SUBFILL_SMOOTH )
		pc = asm_BLOCK( fillsub_step_color );

		pc = asm_BLOCK( fillsub_step_zf );
	
	if( needs & __SUBFILL_TEX0_FORMAT_MASK )
		pc = asm_BLOCK( fillsub_step_tex0 );

	pc = asm_Jmp( pc, y_loop );

	asm_Jcc( y_test, pc, cc_JGE );
	pc = asm_BLOCK( fillsub_Uninit );
	return pc;

}


status_t __mgl_Build( __mglContext *gc )
{
	void *pc;
	int64 needs = calcSubFillNeeds( gc );
	

	pc = __cc_lookup( gc->cache, needs );
	if( pc )
	{
		gc->go_s = (void(*)(__mglContext *)) pc;
		return B_OK;
	}
		
	__cc_add( gc->cache, needs, 1024*16, &pc );

//	if( gc->info.debugOther )
		printf( "scanline_makeRasterizer  Building %x \n", needs );

	gc->go_s = (void(*)(__mglContext *)) pc;
	
	//pc = buildTrapCore( gc, pc, needs );
	pc = buildScanlineCore( gc, pc, needs );
	
	
	// END scanline 
	return B_OK;
}


