/************************************************************
*  scanline_generation.c
*
*  Contains code for generating the polyfin routines at runtime.
*
*************************************************************/

#include "context.h"
#include "global.h"
#include "immed.h"
#include "mathLib.h"
#include "processor.h"
#include "assembler.h"


#define __SUBFILL_COLOR_WRITE				0x0000000F
#define __SUBFILL_COLOR_WRITE_R				0x00000001
#define __SUBFILL_COLOR_WRITE_G				0x00000002
#define __SUBFILL_COLOR_WRITE_B				0x00000004
#define __SUBFILL_COLOR_WRITE_A				0x00000008

// Depth Test Group
#define __SUBFILL_DEPTH_FLAG_MASK			0x000000F0
#define __SUBFILL_DEPTH_DISABLED			0x00000000
#define __SUBFILL_DEPTH_FUNC_NEVER			0x00000010
#define __SUBFILL_DEPTH_FUNC_ALWAYS			0x00000020
#define __SUBFILL_DEPTH_FUNC_LESS			0x00000030
#define __SUBFILL_DEPTH_FUNC_LEQUAL			0x00000040
#define __SUBFILL_DEPTH_FUNC_EQUAL			0x00000050
#define __SUBFILL_DEPTH_FUNC_GEQUAL			0x00000060
#define __SUBFILL_DEPTH_FUNC_GREATER		0x00000070
#define __SUBFILL_DEPTH_FUNC_NOTEQUAL		0x00000080

// Alpha Group
#define __SUBFILL_ALPHA_FLAG_MASK			0x00000F00
#define __SUBFILL_ALPHA_DISABLED			0x00000000
#define __SUBFILL_ALPHA_FUNC_NEVER			0x00000100
#define __SUBFILL_ALPHA_FUNC_ALWAYS			0x00000200
#define __SUBFILL_ALPHA_FUNC_LESS			0x00000300
#define __SUBFILL_ALPHA_FUNC_LEQUAL			0x00000400
#define __SUBFILL_ALPHA_FUNC_EQUAL			0x00000500
#define __SUBFILL_ALPHA_FUNC_GEQUAL			0x00000600
#define __SUBFILL_ALPHA_FUNC_GREATER		0x00000700
#define __SUBFILL_ALPHA_FUNC_NOTEQUAL		0x00000800

//
#define __SUBFILL_DEPTH_WRITE				0x00001000
#define __SUBFILL_STIPPLE_TEST				0x00002000
#define __SUBFILL_SMOOTH					0x00004000
#define __SUBFILL_FOG						0x00008000
#define __SUBFILL_TEX1						0x00010000
#define __SUBFILL_TEX2						0x00020000
#define __SUBFILL_TEX3						0x00040000
#define __SUBFILL_TEX4						0x00080000

#define __SUBFILL_FRONT						0x00100000


#define DEF_PART(p) extern GLubyte fillsub_##p;  extern GLubyte fillsub_##p##_end

DEF_PART( Init );
DEF_PART( Init_Shade );
DEF_PART( Init_Flat );
DEF_PART( Init_Z );
DEF_PART( Init_F );
DEF_PART( Init_Tex1 );
DEF_PART( Init_Tex1_non_mip );
DEF_PART( YLoopInit );
DEF_PART( PerLineInit );
DEF_PART( PerLineInit2 );
DEF_PART( PerLine_Smooth );
DEF_PART( PerLine_Smooth2 );
DEF_PART( PerLine_Z );
DEF_PART( PerLine_Tex1 );
DEF_PART( PerLine_F );
DEF_PART( PerLineInit3 );
DEF_PART( PerLineNext );
DEF_PART( PerLineNext_Smooth );
DEF_PART( PerLineNext_Z );
DEF_PART( PerLineNext_Tex1 );
DEF_PART( PerLineNext_F );
DEF_PART( PerLineEnd );
DEF_PART( Done );
DEF_PART( Done_Smooth );
DEF_PART( Done_Z );
DEF_PART( Done_Tex1 );
DEF_PART( Done_F );
DEF_PART( Cleanup );

DEF_PART( PerLineCopyFront );


DEF_PART( scan_CalcOffset );
DEF_PART( scan_CalcOffset_color32 );
DEF_PART( scan_CalcOffset_color16 );
DEF_PART( scan_CalcOffset_colorFront );
DEF_PART( scan_CalcOffset_colorBack );
DEF_PART( scan_loop_init );
DEF_PART( scan_loop_smooth );
DEF_PART( scan_loop_flat );
DEF_PART( scan_loop_color_write );
DEF_PART( scan_loop_depth_write );
DEF_PART( scan_loop_finish );
DEF_PART( scan_loop_tex_fetch );


static GLuint calcSubFillNeeds( __glContext *gc )
{
	GLuint needs = 0;

	if( gc->state.color.RedWriteEnabled )
		needs |= __SUBFILL_COLOR_WRITE_R;
	if( gc->state.color.GreenWriteEnabled )
		needs |= __SUBFILL_COLOR_WRITE_G;
	if( gc->state.color.BlueWriteEnabled )
		needs |= __SUBFILL_COLOR_WRITE_B;
	if( gc->state.color.AlphaWriteEnabled )
		needs |= __SUBFILL_COLOR_WRITE_A;
		
	if ( gc->state.poly.StippleEnabled )
		needs |= __SUBFILL_STIPPLE_TEST;

	if ( gc->state.fog.Enabled )
		needs |= __SUBFILL_FOG;
		
	if( gc->texture.Enabled[0] )
		needs |= __SUBFILL_TEX1;
	if( gc->texture.Enabled[1] )
		needs |= __SUBFILL_TEX2;
	if( gc->texture.Enabled[2] )
		needs |= __SUBFILL_TEX3;
	if( gc->texture.Enabled[3] )
		needs |= __SUBFILL_TEX4;
		
	if( gc->state.drawBuffer == GL_FRONT )
		needs |= __SUBFILL_FRONT;

	if( gc->state.depth.TestEnabled )
	{
		if( gc->state.depth.WriteEnabled )
			needs |= __SUBFILL_DEPTH_WRITE;
			
		switch( gc->state.depth.TestFunction )
		{
		case GL_NEVER:
			needs |= __SUBFILL_DEPTH_FUNC_NEVER; break;
		case GL_ALWAYS:
			needs |= __SUBFILL_DEPTH_FUNC_ALWAYS; break;
		case GL_LESS:
			needs |= __SUBFILL_DEPTH_FUNC_LESS; break;
		case GL_LEQUAL:
			needs |= __SUBFILL_DEPTH_FUNC_LEQUAL; break;
		case GL_EQUAL:
			needs |= __SUBFILL_DEPTH_FUNC_EQUAL; break;
		case GL_GEQUAL:
			needs |= __SUBFILL_DEPTH_FUNC_GEQUAL; break;
		case GL_GREATER:
			needs |= __SUBFILL_DEPTH_FUNC_GREATER; break;
		case GL_NOTEQUAL:
			needs |= __SUBFILL_DEPTH_FUNC_NOTEQUAL; break;
		}
	}
	
	if( gc->state.light.ShadingModel == GL_SMOOTH )
		needs |= __SUBFILL_SMOOTH;
	
	
	return needs;	
}


static GLubyte dummy[1024*16];
void __glFillSubTriangle ( __glContext *gc );

void scanline_makeRasterizer( __glContext *gc )
{
	void *pc = gc->softScanProcs.fillSubFuncData = dummy;
	GLuint needs = calcSubFillNeeds( gc );
//	GLuint depthNeeds = needs & __SUBFILL_DEPTH_FLAG_MASK;
	void *loop=0;
	void *done_jle;
	void *next_jle=0;
	void *scan_loop=0;

//	if( gc->info.debugOther )
//		printf( "scanline_makeRasterizer  needs=%x \n", needs );

	if( !gc->state.opt.UseXP )
	{
		gc->softScanProcs.fillSubProc = __glFillSubTriangle;
		return;
	}

	if( needs == gc->softScanProcs.fillSubNeeds )
		return;
	gc->softScanProcs.fillSubNeeds = needs;

//	if( gc->info.debugOther )
		printf( "scanline_makeRasterizer  Building %x \n", needs );

	pc = asm_Align( pc );
	gc->softScanProcs.fillSubProc = (void(*)(__glContext *, __glShade *)) pc;
	

	pc= asm_BLOCK( fillsub_Init );

	if( needs & __SUBFILL_SMOOTH )
		pc = asm_BLOCK( fillsub_Init_Shade );
	else
		pc = asm_BLOCK( fillsub_Init_Flat );
	
	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
		pc = asm_BLOCK( fillsub_Init_Z );
	if( needs & __SUBFILL_FOG )
		pc = asm_BLOCK( fillsub_Init_F );
	if( needs & __SUBFILL_TEX1 )
	{
		pc = asm_BLOCK( fillsub_Init_Tex1 );
		pc = asm_BLOCK( fillsub_Init_Tex1_non_mip );
	}

	loop = pc;
	pc = asm_BLOCK( fillsub_YLoopInit );

	done_jle = pc;
	pc= asm_Jcc( pc, 0, cc_JLE );
	
	pc = asm_BLOCK( fillsub_PerLineInit );

	next_jle = pc;
	pc= asm_Jcc( pc, 0, cc_JLE );

	pc = asm_BLOCK( fillsub_PerLineInit2 );

	// extra iterators here
	if( needs & __SUBFILL_SMOOTH )
		pc = asm_BLOCK( fillsub_PerLine_Smooth );
	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
		pc = asm_BLOCK( fillsub_PerLine_Z );
	if( needs & __SUBFILL_FOG )
		pc = asm_BLOCK( fillsub_PerLine_F );
	if( needs & __SUBFILL_TEX1 )
	{
		pc = asm_BLOCK( fillsub_PerLine_Tex1 );
	}

	pc = asm_BLOCK( fillsub_PerLineInit3 );
	if( needs & __SUBFILL_SMOOTH )
		pc = asm_BLOCK( fillsub_PerLine_Smooth2 );
	

	// Create a scanling processor here
	pc = asm_BLOCK( fillsub_scan_CalcOffset );

	pc = asm_BLOCK( fillsub_scan_CalcOffset_color32 );
	if( needs & __SUBFILL_FRONT )
		pc = asm_BLOCK( fillsub_scan_CalcOffset_colorFront );
	else
		pc = asm_BLOCK( fillsub_scan_CalcOffset_colorBack );
	
	pc = asm_BLOCK( fillsub_scan_loop_init );
	
	scan_loop = pc;

	if( needs & __SUBFILL_TEX1 )
	{
		pc = asm_BLOCK( fillsub_scan_loop_tex_fetch );
	}
	else
	{
		if( needs & __SUBFILL_SMOOTH )
			pc = asm_BLOCK( fillsub_scan_loop_smooth );
		else
			pc = asm_BLOCK( fillsub_scan_loop_flat );
	}

	pc = asm_BLOCK( fillsub_scan_loop_color_write );
	//pc = asm_BLOCK( fillsub_scan_loop_depth_write );
	

	pc = asm_BLOCK( fillsub_scan_loop_finish );
	pc = asm_Jcc( pc, scan_loop, cc_JNE );
	
	
	
	// END scanline 


	pc = asm_Align( pc );
	asm_Jcc( next_jle, pc, cc_JLE );
	pc = asm_BLOCK( fillsub_PerLineNext );
	
	if( needs & __SUBFILL_SMOOTH )
		pc = asm_BLOCK( fillsub_PerLineNext_Smooth );
	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
		pc = asm_BLOCK( fillsub_PerLineNext_Z );
	if( needs & __SUBFILL_FOG )
		pc = asm_BLOCK( fillsub_PerLineNext_F );
	if( needs & __SUBFILL_TEX1 )
		pc = asm_BLOCK( fillsub_PerLineNext_Tex1 );

	if( needs & __SUBFILL_FRONT )
		pc = asm_BLOCK( fillsub_PerLineCopyFront );
		
	pc = asm_BLOCK( fillsub_PerLineEnd );
	pc = asm_Jmp( pc, loop );


	asm_Jcc( done_jle, pc, cc_JLE );
	pc = asm_BLOCK( fillsub_Done );

	if( needs & __SUBFILL_DEPTH_FLAG_MASK )
		pc = asm_BLOCK( fillsub_Done_Z );
	if( needs & __SUBFILL_FOG )
		pc = asm_BLOCK( fillsub_Done_F );
	if( needs & __SUBFILL_TEX1 )
		pc = asm_BLOCK( fillsub_Done_Tex1 );

	pc = asm_BLOCK( fillsub_Cleanup );
	
}


