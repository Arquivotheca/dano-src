/************************************************************
*  prim_processor_generation.c
*
*  Contains code for generating the primitive processors at runtime.
*
*************************************************************/

#include "context.h"
#include "global.h"
#include "immed.h"
#include "mathLib.h"
#include "processor.h"
#include "assembler.h"
#include "prim_processor_parts.h"

extern void __glDoPolygonClip (__glContext * gc, GLuint *iv, GLint nout, GLuint allClipCodes);
extern void popNop();
extern void calc_();

extern void calc_();
extern void calc_en();
extern void calc_enf();
extern void calc_enb();
extern void calc_ent();
extern void calc_enft();
extern void calc_enbt();

#define SETUP_CLIPPER_FRONT_AND_BACK() { 							\
		if( needs & __POLYFIN_FRONT_FILL )							\
			pc= asm_BLOCK( primParts_Clipper_Get_FillFront );		\
		if( needs & __POLYFIN_FRONT_LINE )							\
			pc= asm_BLOCK( primParts_Clipper_Get_LineFront );		\
		if( needs & __POLYFIN_FRONT_POINT )							\
			pc= asm_BLOCK( primParts_Clipper_Get_PointFront );		\
		pc= asm_BLOCK( primParts_Clipper_Put_Front );				\
		if( needs & __POLYFIN_TWO_SIDED )							\
		{															\
			if( needs & __POLYFIN_BACK_FILL )						\
				pc= asm_BLOCK( primParts_Clipper_Get_FillBack );	\
			if( needs & __POLYFIN_BACK_LINE )						\
				pc= asm_BLOCK( primParts_Clipper_Get_LineBack );	\
			if( needs & __POLYFIN_BACK_POINT )						\
				pc= asm_BLOCK( primParts_Clipper_Get_PointBack );	\
		}															\
		else														\
		{															\
			if( needs & __POLYFIN_BACK_FILL )						\
				pc= asm_BLOCK( primParts_Clipper_Get_FillFront );	\
			if( needs & __POLYFIN_BACK_LINE )						\
				pc= asm_BLOCK( primParts_Clipper_Get_LineFront );	\
			if( needs & __POLYFIN_BACK_POINT )						\
				pc= asm_BLOCK( primParts_Clipper_Get_PointFront );	\
		}															\
		pc= asm_BLOCK( primParts_Clipper_Put_Back );				\
	}
	
	

GLuint calcPrimitiveProcessorNeeds( __glContext *gc )
{
	GLuint needs = 0;
	
	__glPickAllProcs( gc );
	
	if( gc->state.light.ShadingModel == GL_FLAT )
	{
		needs |= __POLYFIN_PROVOKING;
	}
	
	if( gc->state.poly.CullFaceEnabled &&
		(gc->state.poly.Cull == GL_FRONT_AND_BACK) )
	{
		needs |= __POLYFIN_CULL_ALL;
	}

	switch( gc->state.poly.FrontMode )
	{
		case GL_FILL:
			needs |= __POLYFIN_FRONT_FILL;
			break;
		case GL_LINE:
			needs |= __POLYFIN_FRONT_LINE;
			break;
		case GL_POINT:
			needs |= __POLYFIN_FRONT_POINT;
			break;
	}

	switch( gc->state.poly.BackMode )
	{
		case GL_FILL:
			needs |= __POLYFIN_BACK_FILL;
			break;
		case GL_LINE:
			needs |= __POLYFIN_BACK_LINE;
			break;
		case GL_POINT:
			needs |= __POLYFIN_BACK_POINT;
			break;
	}

	if( (gc->state.poly.CullFaceEnabled && (
			(gc->state.poly.Cull == GL_FRONT) || 
			(gc->state.poly.Cull == GL_BACK))) ||
		(gc->state.poly.FrontMode != gc->state.poly.BackMode) ||
		gc->state.light.LightingEnabled )
	{
		needs |= __POLYFIN_FACING;

		if( gc->state.poly.CullFaceEnabled )
		{
			if( gc->state.poly.Cull == GL_FRONT )
				needs |= __POLYFIN_CULL_FRONT;
			if( gc->state.poly.Cull == GL_BACK )
				needs |= __POLYFIN_CULL_BACK;
		}
	}
	
	if( /*(!gc->state.poly.CullFaceEnabled) &&*/
		gc->state.light.LightingEnabled &&
		gc->state.light.TwoSidedEnabled )
	{
		needs |= __POLYFIN_TWO_SIDED;
	}

	
	if( gc->state.poly.FrontFaceDirection == GL_CCW )
	{
		needs |= __POLYFIN_FRONT_CCW;
	}


	if (gc->transform.matrixIsIdent &&
		(gc->texture.GenProc[0] == __glCalcTexture))
		gc->vertex.XformNeeds &= ~__GL_HAS_TEXTURE;


	gc->asmProc_EWNT_Front = calc_;
	gc->asmProc_EWNT_Back = calc_;

	if( gc->state.light.LightingEnabled )
	{
		if( gc->vertex.XformNeeds & __GL_HAS_TEXTURE )
		{
			gc->asmProc_EWNT_Front = calc_enft;
			gc->asmProc_EWNT_Back = calc_enbt;
			needs |= __POLYFIN_EWNT;
		}
		else
		{
			gc->asmProc_EWNT_Front = calc_enf;
			gc->asmProc_EWNT_Back = calc_enb;
			needs |= __POLYFIN_EWNT;
		}
	}
	else
	{
		if( gc->vertex.XformNeeds & __GL_HAS_EYE )
		{
			if( gc->vertex.XformNeeds & __GL_HAS_NORMAL )
			{
			}

			if( gc->vertex.XformNeeds & __GL_HAS_TEXTURE )
			{
				gc->asmProc_EWNT_Front = calc_ent;
				gc->asmProc_EWNT_Back = calc_ent;
				needs |= __POLYFIN_EWNT;
			}
			else
			{
				gc->asmProc_EWNT_Front = calc_en;
				gc->asmProc_EWNT_Back = calc_en;
				needs |= __POLYFIN_EWNT;
			}
		}
		else
		{
			if( gc->vertex.XformNeeds & __GL_HAS_NORMAL )
			{
				if( gc->vertex.XformNeeds & __GL_HAS_TEXTURE )
				{
					gc->asmProc_EWNT_Front = calc_ent;
					gc->asmProc_EWNT_Back = calc_ent;
					needs |= __POLYFIN_EWNT;
				}
				else
				{
					gc->asmProc_EWNT_Front = calc_en;
					gc->asmProc_EWNT_Back = calc_en;
					needs |= __POLYFIN_EWNT;
				}
			}
			else
			{
				if( gc->vertex.XformNeeds & __GL_HAS_TEXTURE )
				{
					gc->asmProc_EWNT_Front = calc_ent;
					gc->asmProc_EWNT_Back = calc_ent;
					needs |= __POLYFIN_EWNT;
				}
				else
				{
					gc->asmProc_EWNT_Front = calc_;
					gc->asmProc_EWNT_Back = calc_;
				}
			}
		}
	}
	
	if( calc_ != gc->asmProc_EWNT_Front )
	{
		needs |= __POLYFIN_EWNT;
	}
	
	return needs;
}


/******************************************************************************/
/* GL_TRIANGLE_FAN Processor 												  */
/******************************************************************************/

GLuint makeFanProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *back;
	void *finish;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_TRIANGLE_FAN Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	pc= asm_BLOCK( primParts_Fan_Init );
	loop = pc;
	if( !(needs & __POLYFIN_CULL_ALL) )
	{
		pc= asm_BLOCK( primParts_Fan_Loop1 );
		finish = pc;
		pc= asm_Jcc(pc, 0, cc_JE );							// To finish
		pc= asm_BLOCK( primParts_Fan_Loop2 );
		if( needs & __POLYFIN_PROVOKING )
			pc= asm_BLOCK( primParts_Fan_SetProvoking );
		clipper = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );						// To Clipped
		
		if( needs & __POLYFIN_FACING )
		{
			pc= asm_BLOCK( primParts_Fan_Facing );
	
			if( needs & __POLYFIN_CULL_FRONT )
			{
				// We reverse the culling here
				// Fall through for back, jmp for front.
				if( needs & __POLYFIN_FRONT_CCW )
					pc= asm_Jcc(pc, loop, cc_JAE );
				else
					pc= asm_Jcc(pc, loop, cc_JBE );

				// If we are here it didn't get culled.
				// We need to draw the Back side.
				if( needs & __POLYFIN_EWNT )
					pc= asm_BLOCK( primParts_Fan_EWNT_Back );
					
				if( needs & __POLYFIN_TWO_SIDED )
				{
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Fan_Back_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Fan_Back_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Fan_Back_Point );
				}
				else
				{
					// We draw the front color even though we are drawingn the back side
					// because the only time a back side color is valid is for two-sideded lighting
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Fan_Front_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Fan_Front_Fill );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Fan_Front_Fill );
				}
				pc= asm_Jmp( pc, loop );
			}
			else
			{
				// Fall through for front, jmp for back
				if( needs & __POLYFIN_CULL_BACK )
				{
					if( needs & __POLYFIN_FRONT_CCW )
						pc= asm_Jcc(pc, loop, cc_JBE );
					else
						pc= asm_Jcc(pc, loop, cc_JAE );

					// If we are here it didn't get culled.
					// We need to draw the Front side.
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Fan_EWNT_Front );
						
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Fan_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Fan_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Fan_Front_Point );
					pc= asm_Jmp( pc, loop );
										
				}
				else
				{
					// We need code for both sides.
					back = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );  // Offset fixed below

					// Do front first
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Fan_EWNT_Front );
						
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Fan_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Fan_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Fan_Front_Point );
					pc= asm_Jmp( pc, loop );
					
					// Do Back next
					// Fix up jump to back;
					if( needs & __POLYFIN_FRONT_CCW )
						asm_Jcc(back, pc, cc_JBE );
					else
						asm_Jcc(back, pc, cc_JAE );

					if( needs & __POLYFIN_TWO_SIDED )
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Fan_EWNT_Back );
							
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Fan_Back_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Fan_Back_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Fan_Back_Point );
					}
					else
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Fan_EWNT_Front );
							
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Fan_Front_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Fan_Front_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Fan_Front_Point );
					}
					pc= asm_Jmp( pc, loop );
					
				}
			}
		}
		else
		{
			// We need to draw the Front side.
			if( needs & __POLYFIN_EWNT )
				pc= asm_BLOCK( primParts_Fan_EWNT_Front );
				
			if( needs & __POLYFIN_FRONT_FILL )
				pc= asm_BLOCK( primParts_Fan_Front_Fill );
			if( needs & __POLYFIN_FRONT_LINE )
				pc= asm_BLOCK( primParts_Fan_Front_Line );
			if( needs & __POLYFIN_FRONT_POINT )
				pc= asm_BLOCK( primParts_Fan_Front_Point );
			pc= asm_Jmp( pc, loop );
		}
		

		pc = asm_Align( pc );
		// We need a clipper here.
		// First lets fix up the branch
		asm_Jcc(clipper, pc, cc_JNZ );

		pc= asm_BLOCK( primParts_Fan_Clipped1 );
		pc= asm_Jcc(pc, loop, cc_JNZ );
		
		SETUP_CLIPPER_FRONT_AND_BACK();
		
		pc= asm_BLOCK( primParts_Fan_Clipped2 );
		if( needs & __POLYFIN_EWNT )
		{
			pc= asm_BLOCK( primParts_Fan_EWNT_Front );
			pc= asm_BLOCK( primParts_Fan_EWNT_Back );
		}
		pc= asm_Call( pc, __glDoPolygonClip );	
		pc= asm_BLOCK( primParts_Fan_Clipped3 );
		pc= asm_Jmp( pc, loop ); 

		// We need the fnishing code.
		// First lets fix up the branch
		asm_Jcc(finish, pc, cc_JE );
	}

	pc = asm_Align( pc );
	pc= asm_BLOCK( primParts_Fan_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}


/******************************************************************************/
/* GL_POLYGON Processor														  */
/******************************************************************************/

GLuint makePolygonProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *back;
	void *finish1;
	void *finish2;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_POLYGON Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	pc= asm_BLOCK( primParts_Polygon_Init );

	if( !(needs & __POLYFIN_CULL_ALL) )
	{
		finish1 = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );							// To finish

		pc= asm_BLOCK( primParts_Polygon_TestClipCode );
		clipper = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );							// To clipper

		pc= asm_BLOCK( primParts_Polygon_Loop1 );

		pc = asm_Align( pc );
		loop = pc;
		pc= asm_BLOCK( primParts_Polygon_Loop2 );
		finish2 = pc;
		pc= asm_Jcc(pc, 0, cc_JE );								// To finish

		if( needs & __POLYFIN_PROVOKING )
			pc= asm_BLOCK( primParts_Polygon_SetProvoking );
		
		if( needs & __POLYFIN_FACING )
		{
			pc= asm_BLOCK( primParts_Polygon_Facing );
	
			if( needs & __POLYFIN_CULL_FRONT )
			{
				// We reverse the culling here
				// Fall through for back, jmp for front.
				if( needs & __POLYFIN_FRONT_CCW )
					pc= asm_Jcc(pc, loop, cc_JAE );
				else
					pc= asm_Jcc(pc, loop, cc_JBE );

				// If we are here it didn't get culled.
				// We need to draw the Back side.
				if( needs & __POLYFIN_EWNT )
					pc= asm_BLOCK( primParts_Polygon_EWNT_Back );
					
				if( needs & __POLYFIN_TWO_SIDED )
				{
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Polygon_Back_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Polygon_Back_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Polygon_Back_Point );
				}
				else
				{
					// We draw the front color even though we are drawingn the back side
					// because the only time a back side color is valid is for two-sideded lighting
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Polygon_Front_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Polygon_Front_Fill );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Polygon_Front_Fill );
				}
				pc= asm_Jmp( pc, loop );
			}
			else
			{
				// Fall through for front, jmp for back
				if( needs & __POLYFIN_CULL_BACK )
				{
					if( needs & __POLYFIN_FRONT_CCW )
						pc= asm_Jcc(pc, loop, cc_JBE );
					else
						pc= asm_Jcc(pc, loop, cc_JAE );

					// If we are here it didn't get culled.
					// We need to draw the Front side.
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Polygon_EWNT_Front );
						
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Polygon_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Polygon_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Polygon_Front_Point );
					pc= asm_Jmp( pc, loop );
										
				}
				else
				{
					// We need code for both sides.
					back = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );  // Offset fixed below

					// Do front first
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Polygon_EWNT_Front );
						
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Polygon_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Polygon_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Polygon_Front_Point );
					pc= asm_Jmp( pc, loop );
					
					// Do Back next
					// Fix up jump to back;
					if( needs & __POLYFIN_FRONT_CCW )
						asm_Jcc(back, pc, cc_JBE );
					else
						asm_Jcc(back, pc, cc_JAE );

					if( needs & __POLYFIN_TWO_SIDED )
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Polygon_EWNT_Back );
							
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Polygon_Back_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Polygon_Back_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Polygon_Back_Point );
					}
					else
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Polygon_EWNT_Front );
							
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Polygon_Front_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Polygon_Front_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Polygon_Front_Point );
					}
					pc= asm_Jmp( pc, loop );
					
				}
			}
		}
		else
		{
			// We need to draw the Front side.
			if( needs & __POLYFIN_EWNT )
				pc= asm_BLOCK( primParts_Polygon_EWNT_Front );
				
			if( needs & __POLYFIN_FRONT_FILL )
				pc= asm_BLOCK( primParts_Polygon_Front_Fill );
			if( needs & __POLYFIN_FRONT_LINE )
				pc= asm_BLOCK( primParts_Polygon_Front_Line );
			if( needs & __POLYFIN_FRONT_POINT )
				pc= asm_BLOCK( primParts_Polygon_Front_Point );
			pc= asm_Jmp( pc, loop );
		}
		

		pc = asm_Align( pc );
		// We need a clipper here.
		// First lets fix up the branch
		asm_Jcc(clipper, pc, cc_JNZ );

		SETUP_CLIPPER_FRONT_AND_BACK();

		pc= asm_BLOCK( primParts_Polygon_Clipped1 );
		pc= asm_Call( pc, __glDoPolygonClip );	
		pc= asm_BLOCK( primParts_Polygon_Clipped2 );
/*
		if( needs & __POLYFIN_EWNT )
		{
			pc= asm_BLOCK( primParts_Polygon_EWNT_Front );
			pc= asm_BLOCK( primParts_Polygon_EWNT_Back );
		}
*/

		// We need the fnishing code.
		// First lets fix up the branch
		pc = asm_Align( pc );
		asm_Jcc(finish1, pc, cc_JNZ );
		asm_Jcc(finish2, pc, cc_JE );
//asm_Jcc(clipper, pc, cc_JNZ );
	}

	pc= asm_BLOCK( primParts_Polygon_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}


/******************************************************************************/
/* GL_QUADS Processor														  */
/******************************************************************************/

void makeQuadsProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *back;
	void *finish;
	void *ja = 0;
	void *jbe = 0;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_QUADS Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

	pc= asm_BLOCK( primParts_pop_nop );
	pc= asm_Align( pc );

	gc->primitive.CacheProcessPtrs[index] = pc;
	
	pc= asm_BLOCK( primParts_Quad_Init );
	loop = pc;
	if( !(needs & __POLYFIN_CULL_ALL) )
	{
		if( needs & __POLYFIN_PROVOKING )
			pc= asm_BLOCK( primParts_Quad_SetProvoking );

		clipper = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );						// To Clipped
	
		if( needs & __POLYFIN_FACING )
		{
			pc= asm_BLOCK( primParts_Quad_Facing );
	
			if( needs & __POLYFIN_CULL_FRONT )
			{
				// We reverse the culling here
				// Fall through for back, jmp for front.
				if( needs & __POLYFIN_FRONT_CCW )
				{
					ja = pc;
					pc= asm_Jcc(pc, loop, cc_JAE );
				}
				else
				{
					jbe = pc;
					pc= asm_Jcc(pc, loop, cc_JBE );
				}

				// If we are here it didn't get culled.
				// We need to draw the Back side.
				if( needs & __POLYFIN_EWNT )
					pc= asm_BLOCK( primParts_Quad_EWNT_Back );
					
				if( needs & __POLYFIN_TWO_SIDED )
				{
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Quad_Back_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Quad_Back_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Quad_Back_Point );
				}
				else
				{
					// We draw the front color even though we are drawingn the back side
					// because the only time a back side color is valid is for two-sideded lighting
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Quad_Front_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Quad_Front_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Quad_Front_Point );
				}
			}
			else
			{
				// Fall through for front, jmp for back
				if( needs & __POLYFIN_CULL_BACK )
				{
					if( needs & __POLYFIN_FRONT_CCW )
					{
						jbe = pc;
						pc= asm_Jcc(pc, loop, cc_JBE );
					}
					else
					{
						ja = pc;
						pc= asm_Jcc(pc, loop, cc_JAE );
					}

					// If we are here it didn't get culled.
					// We need to draw the Front side.
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Quad_EWNT_Front );
						
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Quad_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Quad_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Quad_Front_Point );
				}
				else
				{
					// We need code for both sides.
					back = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );  // Offset fixed below

					// Do front first
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Quad_EWNT_Front );
						
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Quad_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Quad_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Quad_Front_Point );
					pc= asm_BLOCK( primParts_Quad_Finish );
					
					// Do Back next
					// Fix up jump to back;
					if( needs & __POLYFIN_FRONT_CCW )
						asm_Jcc(back, pc, cc_JBE );
					else
						asm_Jcc(back, pc, cc_JAE );

					if( needs & __POLYFIN_TWO_SIDED )
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Quad_EWNT_Back );
							
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Quad_Back_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Quad_Back_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Quad_Back_Point );
					}
					else
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Quad_EWNT_Front );
							
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Quad_Front_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Quad_Front_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Quad_Front_Point );
					}
				}
			}
		}
		else
		{
			if( needs & __POLYFIN_EWNT )
				pc= asm_BLOCK( primParts_Quad_EWNT_Front );
				
			if( needs & __POLYFIN_FRONT_FILL )
				pc= asm_BLOCK( primParts_Quad_Front_Fill );
			if( needs & __POLYFIN_FRONT_LINE )
				pc= asm_BLOCK( primParts_Quad_Front_Line );
			if( needs & __POLYFIN_FRONT_POINT )
				pc= asm_BLOCK( primParts_Quad_Front_Point );
		}

		// We need to fix up the culled jmps
		finish = pc;
		if( ja )
			asm_Jcc( ja, pc, cc_JAE );
		if( jbe )
			asm_Jcc( jbe, pc, cc_JBE );
		pc= asm_BLOCK( primParts_Quad_Finish );

		// We need a clipper here.
		// First lets fix up the branch
		asm_Jcc(clipper, pc, cc_JNZ );

		SETUP_CLIPPER_FRONT_AND_BACK();

		pc= asm_BLOCK( primParts_Quad_Clipped1 );
		pc= asm_Jcc(pc, finish, cc_JNZ );
		pc= asm_BLOCK( primParts_Quad_Clipped2 );
		if( needs & __POLYFIN_EWNT )
		{
			pc= asm_BLOCK( primParts_Quad_EWNT_Front );
			pc= asm_BLOCK( primParts_Quad_EWNT_Back );
		}
		pc= asm_Call( pc, __glDoPolygonClip );	
		pc= asm_BLOCK( primParts_Quad_Clipped3 );
	}
	else
	{
		pc= asm_BLOCK( primParts_Quad_Finish );
	}
		
}


/******************************************************************************/
/* GL_QUAD_STRIP Processor 													  */
/******************************************************************************/

GLuint makeQuadStripProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *back;
	void *finish1;
	void *finish2;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_QUAD_STRIP Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	
	pc= asm_BLOCK( primParts_QuadStrip_Init );
	loop = pc;
	if( !(needs & __POLYFIN_CULL_ALL) )
	{
		pc= asm_BLOCK( primParts_QuadStrip_Loop1 );
		finish1 = pc;
		pc= asm_Jcc(pc, 0, cc_JE );							// To finish
		pc= asm_BLOCK( primParts_QuadStrip_Loop2 );
		finish2 = pc;
		pc= asm_Jcc(pc, 0, cc_JE );							// To finish
		pc= asm_BLOCK( primParts_QuadStrip_Loop3 );
		if( needs & __POLYFIN_PROVOKING )
			pc= asm_BLOCK( primParts_QuadStrip_SetProvoking );
		clipper = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );						// To Clipped
		
		if( needs & __POLYFIN_FACING )
		{
			pc= asm_BLOCK( primParts_QuadStrip_Facing );
	
			if( needs & __POLYFIN_CULL_FRONT )
			{
				// We reverse the culling here
				// Fall through for back, jmp for front.
				if( needs & __POLYFIN_FRONT_CCW )
					pc= asm_Jcc(pc, loop, cc_JAE );
				else
					pc= asm_Jcc(pc, loop, cc_JBE );
					
				if( needs & __POLYFIN_EWNT )
					pc= asm_BLOCK( primParts_QuadStrip_EWNT_Back );

				// If we are here it didn't get culled.
				// We need to draw the Back side.

				if( needs & __POLYFIN_TWO_SIDED )
				{
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_QuadStrip_Back_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_QuadStrip_Back_Point );
				}
				else
				{
					// We draw the front color even though we are drawingn the back side
					// because the only time a back side color is valid is for two-sideded lighting
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Point );
				}
				pc= asm_Jmp( pc, loop );
			}
			else
			{
				// Fall through for front, jmp for back
				if( needs & __POLYFIN_CULL_BACK )
				{
					if( needs & __POLYFIN_FRONT_CCW )
						pc= asm_Jcc(pc, loop, cc_JBE );
					else
						pc= asm_Jcc(pc, loop, cc_JAE );

					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_QuadStrip_EWNT_Front );

					// If we are here it didn't get culled.
					// We need to draw the Front side.
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Point );
					pc= asm_Jmp( pc, loop );
										
				}
				else
				{
					// We need code for both sides.
					back = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );  // Offset fixed below

					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_QuadStrip_EWNT_Front );

					// Do front first
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_QuadStrip_Front_Point );
					pc= asm_Jmp( pc, loop );
					
					// Do Back next
					// Fix up jump to back;
					if( needs & __POLYFIN_FRONT_CCW )
						asm_Jcc(back, pc, cc_JBE );
					else
						asm_Jcc(back, pc, cc_JAE );

					if( needs & __POLYFIN_TWO_SIDED )
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_QuadStrip_EWNT_Back );
	
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_QuadStrip_Back_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_QuadStrip_Back_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_QuadStrip_Back_Point );
					}
					else
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_QuadStrip_EWNT_Front );
	
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_QuadStrip_Front_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_QuadStrip_Front_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_QuadStrip_Front_Point );
					}
					
					pc= asm_Jmp( pc, loop );
					
				}
			}
		}
		else
		{
			// We need to draw the Front side.
			if( needs & __POLYFIN_EWNT )
				pc= asm_BLOCK( primParts_QuadStrip_EWNT_Front );

			if( needs & __POLYFIN_FRONT_FILL )
				pc= asm_BLOCK( primParts_QuadStrip_Front_Fill );
			if( needs & __POLYFIN_FRONT_LINE )
				pc= asm_BLOCK( primParts_QuadStrip_Front_Line );
			if( needs & __POLYFIN_FRONT_POINT )
				pc= asm_BLOCK( primParts_QuadStrip_Front_Point );
			pc= asm_Jmp( pc, loop );
		}
		
		// We need a clipper here.
		// First lets fix up the branch
		asm_Jcc(clipper, pc, cc_JNZ );

		pc= asm_BLOCK( primParts_QuadStrip_Clipped1 );
		pc= asm_Jcc(pc, loop, cc_JNZ );

		SETUP_CLIPPER_FRONT_AND_BACK();

		pc= asm_BLOCK( primParts_QuadStrip_Clipped2 );
		if( needs & __POLYFIN_EWNT )
		{
			pc= asm_BLOCK( primParts_QuadStrip_EWNT_Front );
			pc= asm_BLOCK( primParts_QuadStrip_EWNT_Back );
		}
		pc= asm_Call( pc, __glDoPolygonClip );	
		pc= asm_BLOCK( primParts_QuadStrip_Clipped3 );
		pc= asm_Jmp( pc, loop ); 

		// We need the fnishing code.
		// First lets fix up the branch
		asm_Jcc(finish1, pc, cc_JE );
		asm_Jcc(finish2, pc, cc_JE );
	}
	pc= asm_BLOCK( primParts_QuadStrip_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}

/******************************************************************************/
/* GL_TRIANGLES Processor													  */
/******************************************************************************/

GLuint makeTrianglesProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *back;
	void *finish;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_TRIANGLES Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	pc= asm_BLOCK( primParts_Triangles_Init );
	loop = pc;
	if( !(needs & __POLYFIN_CULL_ALL) )
	{
		pc= asm_BLOCK( primParts_Triangles_Loop1 );
		finish = pc;
		pc= asm_Jcc(pc, 0, cc_JGE );						// To finish
		pc= asm_BLOCK( primParts_Triangles_Loop2 );
		if( needs & __POLYFIN_PROVOKING )
			pc= asm_BLOCK( primParts_Triangles_SetProvoking );
		clipper = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );						// To Clipped
		
		if( needs & __POLYFIN_FACING )
		{
			pc= asm_BLOCK( primParts_Triangles_Facing );
	
			if( needs & __POLYFIN_CULL_FRONT )
			{
				// We reverse the culling here
				// Fall through for back, jmp for front.
				if( needs & __POLYFIN_FRONT_CCW )
					pc= asm_Jcc(pc, loop, cc_JAE );
				else
					pc= asm_Jcc(pc, loop, cc_JBE );

				// If we are here it didn't get culled.
				// We need to draw the Back side.
				if( needs & __POLYFIN_EWNT )
					pc= asm_BLOCK( primParts_Triangles_EWNT_Back );

				if( needs & __POLYFIN_TWO_SIDED )
				{
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Triangles_Back_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Triangles_Back_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Triangles_Back_Point );
				}
				else
				{
					// We draw the front color even though we are drawingn the back side
					// because the only time a back side color is valid is for two-sideded lighting
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_Triangles_Front_Fill );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_Triangles_Front_Line );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_Triangles_Front_Point );
				}
				pc= asm_Jmp( pc, loop );
			}
			else
			{
				// Fall through for front, jmp for back
				if( needs & __POLYFIN_CULL_BACK )
				{
					if( needs & __POLYFIN_FRONT_CCW )
						pc= asm_Jcc(pc, loop, cc_JBE );
					else
						pc= asm_Jcc(pc, loop, cc_JAE );

					// If we are here it didn't get culled.
					// We need to draw the Front side.
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Triangles_EWNT_Front );

					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Triangles_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Triangles_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Triangles_Front_Point );
					pc= asm_Jmp( pc, loop );
										
				}
				else
				{
					// We need code for both sides.
					back = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );  // Offset fixed below

					// Do front first
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_Triangles_EWNT_Front );
	
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_Triangles_Front_Fill );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_Triangles_Front_Line );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_Triangles_Front_Point );
					pc= asm_Jmp( pc, loop );
					
					// Do Back next
					// Fix up jump to back;
					if( needs & __POLYFIN_FRONT_CCW )
						asm_Jcc(back, pc, cc_JBE );
					else
						asm_Jcc(back, pc, cc_JAE );

					if( needs & __POLYFIN_TWO_SIDED )
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Triangles_EWNT_Back );
		
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Triangles_Back_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Triangles_Back_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Triangles_Back_Point );
					}
					else
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_Triangles_EWNT_Front );
		
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_Triangles_Front_Fill );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_Triangles_Front_Line );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_Triangles_Front_Point );
					}
					pc= asm_Jmp( pc, loop );
					
				}
			}
		}
		else
		{
			// We need to draw the Front side.
			if( needs & __POLYFIN_EWNT )
				pc= asm_BLOCK( primParts_Triangles_EWNT_Front );

			if( needs & __POLYFIN_FRONT_FILL )
				pc= asm_BLOCK( primParts_Triangles_Front_Fill );
			if( needs & __POLYFIN_FRONT_LINE )
				pc= asm_BLOCK( primParts_Triangles_Front_Line );
			if( needs & __POLYFIN_FRONT_POINT )
				pc= asm_BLOCK( primParts_Triangles_Front_Point );
			pc= asm_Jmp( pc, loop );
		}
		
		pc = asm_Align( pc );
		// We need a clipper here.
		// First lets fix up the branch
		asm_Jcc(clipper, pc, cc_JNZ );

		pc= asm_BLOCK( primParts_Triangles_Clipped1 );
		pc= asm_Jcc(pc, loop, cc_JNZ );

		SETUP_CLIPPER_FRONT_AND_BACK();

		pc= asm_BLOCK( primParts_Triangles_Clipped2 );
		if( needs & __POLYFIN_EWNT )
		{
			pc= asm_BLOCK( primParts_Triangles_EWNT_Front );
			pc= asm_BLOCK( primParts_Triangles_EWNT_Back );
		}
		pc= asm_Call( pc, __glDoPolygonClip );	
		pc= asm_BLOCK( primParts_Triangles_Clipped3 );
		pc= asm_Jmp( pc, loop ); 

		// We need the fnishing code.
		// First lets fix up the branch
		asm_Jcc(finish, pc, cc_JGE );
	}
	pc = asm_Align( pc );
	pc= asm_BLOCK( primParts_Triangles_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}

/******************************************************************************/
/* GL_TRIANGLE_STRIP Processor 												  */
/******************************************************************************/

GLuint makeTriangleStripProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper1;
	void *clipper2;
	void *back;
	void *finish1;
	void *finish2;
	void *ja =0;
	void *jbe =0;
	void *tri2 =0;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_TRIANGLE_STRIP Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	pc= asm_BLOCK( primParts_TriangleStrip_Init );
	loop = pc;
	if( !(needs & __POLYFIN_CULL_ALL) )
	{
		pc= asm_BLOCK( primParts_TriangleStrip_Loop1 );
		finish1 = pc;
		pc= asm_Jcc(pc, 0, cc_JE );						// To finish
		pc= asm_BLOCK( primParts_TriangleStrip_Loop2 );
		if( needs & __POLYFIN_PROVOKING )
			pc= asm_BLOCK( primParts_TriangleStrip_SetProvoking );
		clipper1 = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );						// To Clipped
		
		if( needs & __POLYFIN_FACING )
		{
			pc= asm_BLOCK( primParts_TriangleStrip_Facing1 );
	
			if( needs & __POLYFIN_CULL_FRONT )
			{
				// We reverse the culling here
				// Fall through for back, jmp for front.
				if( needs & __POLYFIN_FRONT_CCW )
				{
					ja = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );
				}
				else
				{
					jbe = pc;
					pc= asm_Jcc(pc, 0, cc_JBE );
				}

				// If we are here it didn't get culled.
				// We need to draw the Back side.
				if( needs & __POLYFIN_EWNT )
					pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Back );

				if( needs & __POLYFIN_TWO_SIDED )
				{
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Back_Fill1 );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Back_Line1 );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Back_Point1 );
				}
				else
				{
					// We draw the front color even though we are drawingn the back side
					// because the only time a back side color is valid is for two-sideded lighting
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill1 );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Line1 );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Point1 );
				}
			}
			else
			{
				// Fall through for front, jmp for back
				if( needs & __POLYFIN_CULL_BACK )
				{
					if( needs & __POLYFIN_FRONT_CCW )
					{
						jbe = pc;
						pc= asm_Jcc(pc, 0, cc_JBE );
					}
					else
					{
						ja = pc;
						pc= asm_Jcc(pc, 0, cc_JAE );
					}

					// If we are here it didn't get culled.
					// We need to draw the Front side.
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
	
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill1 );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Line1 );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Point1 );
				}
				else
				{
					// We need code for both sides.
					back = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );  // Offset fixed below

					// Do front first
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
	
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill1 );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Line1 );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Point1 );
					tri2= pc;
					pc= asm_Jmp( pc, 0 );
					
					// Do Back next
					// Fix up jump to back;
					if( needs & __POLYFIN_FRONT_CCW )
						asm_Jcc(back, pc, cc_JBE );
					else
						asm_Jcc(back, pc, cc_JAE );

					if( needs & __POLYFIN_TWO_SIDED )
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Back );
		
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_TriangleStrip_Back_Fill1 );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_TriangleStrip_Back_Line1 );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_TriangleStrip_Back_Point1 );
					}
					else
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
		
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill1 );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_TriangleStrip_Front_Line1 );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_TriangleStrip_Front_Point1 );
					}
				}
			}
		}
		else
		{
			// We need to draw the Front side.
			if( needs & __POLYFIN_EWNT )
				pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
	
			if( needs & __POLYFIN_FRONT_FILL )
				pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill1 );
			if( needs & __POLYFIN_FRONT_LINE )
				pc= asm_BLOCK( primParts_TriangleStrip_Front_Line1 );
			if( needs & __POLYFIN_FRONT_POINT )
				pc= asm_BLOCK( primParts_TriangleStrip_Front_Point1 );
		}

		// Lets draw the second triangle
		
		if( ja )
			asm_Jcc( ja, pc, cc_JAE );
		if( jbe )
			asm_Jcc( jbe, pc, cc_JBE );
		if( tri2 )
			asm_Jmp( tri2, pc );
		ja = 0;
		jbe = 0;

		tri2 = pc;
		pc= asm_BLOCK( primParts_TriangleStrip_Loop3 );
		finish2 = pc;
		pc= asm_Jcc(pc, 0, cc_JE );						// To finish
		pc= asm_BLOCK( primParts_TriangleStrip_Loop4 );
		if( needs & __POLYFIN_PROVOKING )
			pc= asm_BLOCK( primParts_TriangleStrip_SetProvoking );
		clipper2 = pc;
		pc= asm_Jcc(pc, 0, cc_JNZ );						// To Clipped

		if( needs & __POLYFIN_FACING )
		{
			pc= asm_BLOCK( primParts_TriangleStrip_Facing2 );
	
			if( needs & __POLYFIN_CULL_FRONT )
			{
				// We reverse the culling here
				// Fall through for back, jmp for front.
				if( needs & __POLYFIN_FRONT_CCW )
					pc= asm_Jcc(pc, loop, cc_JAE );
				else
					pc= asm_Jcc(pc, loop, cc_JBE );

				// If we are here it didn't get culled.
				// We need to draw the Back side.
				if( needs & __POLYFIN_EWNT )
					pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Back );
	
				if( needs & __POLYFIN_TWO_SIDED )
				{
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Back_Fill2 );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Back_Line2 );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Back_Point2 );
				}
				else
				{
					// We draw the front color even though we are drawingn the back side
					// because the only time a back side color is valid is for two-sideded lighting
					if( needs & __POLYFIN_BACK_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill2 );
					if( needs & __POLYFIN_BACK_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Line2 );
					if( needs & __POLYFIN_BACK_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Point2 );
				}
				pc= asm_Jmp( pc, loop );
			}
			else
			{
				// Fall through for front, jmp for back
				if( needs & __POLYFIN_CULL_BACK )
				{
					if( needs & __POLYFIN_FRONT_CCW )
						pc= asm_Jcc(pc, loop, cc_JBE );
					else
						pc= asm_Jcc(pc, loop, cc_JAE );

					// If we are here it didn't get culled.
					// We need to draw the Front side.
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
	
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill2 );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Line2 );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Point2 );
					pc= asm_Jmp( pc, loop );
										
				}
				else
				{
					// We need code for both sides.
					back = pc;
					pc= asm_Jcc(pc, 0, cc_JAE );  // Offset fixed below

					// Do front first
					if( needs & __POLYFIN_EWNT )
						pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
	
					if( needs & __POLYFIN_FRONT_FILL )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill2 );
					if( needs & __POLYFIN_FRONT_LINE )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Line2 );
					if( needs & __POLYFIN_FRONT_POINT )
						pc= asm_BLOCK( primParts_TriangleStrip_Front_Point2 );
					pc= asm_Jmp( pc, loop );
					
					// Do Back next
					// Fix up jump to back;
					if( needs & __POLYFIN_FRONT_CCW )
						asm_Jcc(back, pc, cc_JBE );
					else
						asm_Jcc(back, pc, cc_JAE );

					if( needs & __POLYFIN_TWO_SIDED )
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Back );
		
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_TriangleStrip_Back_Fill2 );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_TriangleStrip_Back_Line2 );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_TriangleStrip_Back_Point2 );
					}
					else
					{
						if( needs & __POLYFIN_EWNT )
							pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
		
						if( needs & __POLYFIN_BACK_FILL )
							pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill2 );
						if( needs & __POLYFIN_BACK_LINE )
							pc= asm_BLOCK( primParts_TriangleStrip_Front_Line2 );
						if( needs & __POLYFIN_BACK_POINT )
							pc= asm_BLOCK( primParts_TriangleStrip_Front_Point2 );
					}
					pc= asm_Jmp( pc, loop );
					
				}
			}
		}
		else
		{
			// We need to draw the Front side.
			if( needs & __POLYFIN_EWNT )
				pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );

			if( needs & __POLYFIN_FRONT_FILL )
				pc= asm_BLOCK( primParts_TriangleStrip_Front_Fill2 );
			if( needs & __POLYFIN_FRONT_LINE )
				pc= asm_BLOCK( primParts_TriangleStrip_Front_Line2 );
			if( needs & __POLYFIN_FRONT_POINT )
				pc= asm_BLOCK( primParts_TriangleStrip_Front_Point2 );
			pc= asm_Jmp( pc, loop );
		}

		
		pc = asm_Align( pc );
		// We need the first clipper here.
		// First lets fix up the branch
		asm_Jcc(clipper1, pc, cc_JNZ );
		pc= asm_BLOCK( primParts_TriangleStrip_Clipped11 );
		pc= asm_Jcc(pc, tri2, cc_JNZ );

		SETUP_CLIPPER_FRONT_AND_BACK();

		pc= asm_BLOCK( primParts_TriangleStrip_Clipped12 );
		if( needs & __POLYFIN_EWNT )
		{
			pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
			pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Back );
		}
		pc= asm_Call( pc, __glDoPolygonClip );	
		pc= asm_BLOCK( primParts_TriangleStrip_Clipped13 );
		pc= asm_Jmp( pc, tri2 ); 

		pc = asm_Align( pc );
		// We need the second clipper here.
		// First lets fix up the branch
		asm_Jcc(clipper2, pc, cc_JNZ );
		pc= asm_BLOCK( primParts_TriangleStrip_Clipped21 );
		pc= asm_Jcc(pc, loop, cc_JNZ );

		SETUP_CLIPPER_FRONT_AND_BACK();

		pc= asm_BLOCK( primParts_TriangleStrip_Clipped22 );
		if( needs & __POLYFIN_EWNT )
		{
			pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Front );
			pc= asm_BLOCK( primParts_TriangleStrip_EWNT_Back );
		}
		pc= asm_Call( pc, __glDoPolygonClip );	
		pc= asm_BLOCK( primParts_TriangleStrip_Clipped23 );
		pc= asm_Jmp( pc, loop ); 

		// We need the fnishing code.
		// First lets fix up the branch
		asm_Jcc(finish1, pc, cc_JE );
		asm_Jcc(finish2, pc, cc_JE );
	}
	pc = asm_Align( pc );
	pc= asm_BLOCK( primParts_TriangleStrip_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}


/******************************************************************************/
/* GL_LINE_STRIP Processor 													  */
/******************************************************************************/

GLuint makeLineStripProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *finish;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_LINE_STRIP Processor Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	pc= asm_BLOCK( primParts_LineStrip_Init );
	loop = pc;

	pc= asm_BLOCK( primParts_LineStrip_Loop1 );
	finish = pc;
	pc= asm_Jcc(pc, 0, cc_JE );								// To finish
	pc= asm_BLOCK( primParts_LineStrip_Loop2 );
	
	if( needs & __POLYFIN_PROVOKING )
		pc= asm_BLOCK( primParts_LineStrip_SetProvoking );
	clipper = pc;
	pc= asm_Jcc(pc, 0, cc_JNZ );							// To clipper

	if( needs & __POLYFIN_EWNT )
		pc= asm_BLOCK( primParts_LineStrip_EWNT );

	pc= asm_BLOCK( primParts_LineStrip_Line );
	pc= asm_Jmp( pc, loop );


	pc = asm_Align( pc );
	// We need a clipper here.
	// First lets fix up the branch
	asm_Jcc(clipper, pc, cc_JNZ );

	pc= asm_BLOCK( primParts_LineStrip_Clipped1 );
	pc= asm_Jcc(pc, loop, cc_JNZ );
	pc= asm_BLOCK( primParts_LineStrip_Clipped2 );

	pc= asm_Call( pc, __glClipLine );	
	pc= asm_BLOCK( primParts_LineStrip_Clipped3 );
	pc= asm_Jmp( pc, loop ); 

	// We need the fnishing code.
	// First lets fix up the branch
	pc = asm_Align( pc );
	asm_Jcc(finish, pc, cc_JE );
	pc= asm_BLOCK( primParts_LineStrip_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}



/******************************************************************************/
/* GL_LINES Processor														  */
/******************************************************************************/

GLuint makeLinesProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *finish;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_LINES Processor Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	pc= asm_BLOCK( primParts_Lines_Init );
	loop = pc;

	pc= asm_BLOCK( primParts_Lines_Loop1 );
	finish = pc;
	pc= asm_Jcc(pc, 0, cc_JGE );							// To finish
	pc= asm_BLOCK( primParts_Lines_Loop2 );
	
	if( needs & __POLYFIN_PROVOKING )
		pc= asm_BLOCK( primParts_Lines_SetProvoking );
	clipper = pc;
	pc= asm_Jcc(pc, 0, cc_JNZ );							// To clipper

	if( needs & __POLYFIN_EWNT )
		pc= asm_BLOCK( primParts_Lines_EWNT );

	pc= asm_BLOCK( primParts_Lines_Line );
	pc= asm_Jmp( pc, loop );


	pc = asm_Align( pc );
	// We need a clipper here.
	// First lets fix up the branch
	asm_Jcc(clipper, pc, cc_JNZ );

	pc= asm_BLOCK( primParts_Lines_Clipped1 );
	pc= asm_Jcc(pc, loop, cc_JNZ );
	pc= asm_BLOCK( primParts_Lines_Clipped2 );

	pc= asm_Call( pc, __glClipLine );	
	pc= asm_BLOCK( primParts_Lines_Clipped3 );
	pc= asm_Jmp( pc, loop ); 

	// We need the fnishing code.
	// First lets fix up the branch
	pc = asm_Align( pc );
	asm_Jcc(finish, pc, cc_JGE );
	pc= asm_BLOCK( primParts_Lines_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}


/******************************************************************************/
/* GL_LINE_LOOP Processor 													  */
/******************************************************************************/

GLuint makeLineLoopProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *clipper;
	void *finish=0;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_LINE_LOOP Processor Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	finish = pc;
	pc= asm_Jcc(pc, 0, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
	asm_Jcc(finish, pc, cc_JZ );
#endif

	pc= asm_BLOCK( primParts_LineLoop_CloseLoop1 );
	if( needs & __POLYFIN_EWNT )
		pc= asm_BLOCK( primParts_LineLoop_CloseLoopEWNT );
	pc= asm_BLOCK( primParts_LineLoop_CloseLoop2 );
	gc->primitive.CacheProcessPtrs[index] = pc;
	
	pc= asm_BLOCK( primParts_LineLoop_Init );

	pc = asm_Align( pc );
	loop = pc;

	pc= asm_BLOCK( primParts_LineLoop_Loop1 );
	finish = pc;
	pc= asm_Jcc(pc, 0, cc_JE );							// To finish
	pc= asm_BLOCK( primParts_LineLoop_Loop2 );
	
	if( needs & __POLYFIN_PROVOKING )
		pc= asm_BLOCK( primParts_LineLoop_SetProvoking );
	clipper = pc;
	pc= asm_Jcc(pc, 0, cc_JNZ );							// To clipper

	if( needs & __POLYFIN_EWNT )
		pc= asm_BLOCK( primParts_LineLoop_EWNT );

	pc= asm_BLOCK( primParts_LineLoop_Line );
	pc= asm_Jmp( pc, loop );


	pc = asm_Align( pc );
	// We need a clipper here.
	// First lets fix up the branch
	asm_Jcc(clipper, pc, cc_JNZ );

	pc= asm_BLOCK( primParts_LineLoop_Clipped1 );
	pc= asm_Jcc(pc, loop, cc_JNZ );
	pc= asm_BLOCK( primParts_LineLoop_Clipped2 );

	pc= asm_Call( pc, __glClipLine );	
	pc= asm_BLOCK( primParts_LineLoop_Clipped3 );
	pc= asm_Jmp( pc, loop ); 

	// We need the fnishing code.
	// First lets fix up the branch
	pc = asm_Align( pc );
	asm_Jcc(finish, pc, cc_JE );
	pc= asm_BLOCK( primParts_LineLoop_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}

/******************************************************************************/
/* GL_POINTS Processor														  */
/******************************************************************************/

GLuint makePointsProcessor( __glContext *gc, GLuint needs, GLuint index )
{
	void *pc;
	void *loop;
	void *finish;

	if(gc->info.debugOther )
		printf( "GLDebug libGL2: GL_POINTS Processor Needs = %x \n", needs );
	pc = &((GLubyte *)gc->primitive.CacheAllocation)[index * 4096];

#if __PROCESSOR_KATMAI__
	pc= asm_BLOCK( primParts_KatmaiEndPrefix1 );
	pc= asm_Jcc(pc, popNop, cc_JZ );
	pc= asm_BLOCK( primParts_KatmaiEndPrefix2 );
	pc= asm_Align( pc );
#endif

	gc->primitive.CacheProcessPtrs[index] = pc;
	pc= asm_BLOCK( primParts_Points_Init );

	pc= asm_Align( pc );
	loop = pc;
	pc= asm_BLOCK( primParts_Points_Loop1 );
	finish = pc;
	pc= asm_Jcc(pc, 0, cc_JGE );							// To finish
	pc= asm_BLOCK( primParts_Points_Loop2 );
	pc= asm_Jcc(pc, loop, cc_JNZ );

	if( needs & __POLYFIN_EWNT )
		pc= asm_BLOCK( primParts_Points_EWNT );

	pc= asm_BLOCK( primParts_Points_Point );
	pc= asm_Jmp( pc, loop );

	// We need the fnishing code.
	// First lets fix up the branch
	pc = asm_Align( pc );
	asm_Jcc(finish, pc, cc_JGE );
	pc= asm_BLOCK( primParts_Points_Finish );
		
	return ((GLuint)pc) - ((GLuint)(&((GLubyte *)gc->primitive.CacheAllocation)[index * 4096]));
}




/******************************************************************************/
/* Cache Management Code													  */
/******************************************************************************/

void makePrimitiveProcessor( __glContext *gc, GLenum primType )
{
	GLuint needs = gc->primitive.CacheCurrentNeeds;
	GLuint code = needs | (primType << 24);
	GLint ct, maxIndex=0;
	GLint age=0;
	
	for ( ct=0; ct<PRIM_CACHE_SIZE; ct++ )
		gc->primitive.CacheAge[ct]++;
		
	for ( ct=0; ct<PRIM_CACHE_SIZE; ct++ )
	{
		if( gc->primitive.CacheCode[ct] == code )
		{
			gc->primitive.CacheAge[ct] = 0;
			gc->primitive.CacheProcess[primType] = gc->primitive.CacheProcessPtrs[ct];
			gc->primitive.CacheEnd[primType] = &((GLubyte *)gc->primitive.CacheAllocation)[ct * 4096];
			return;
		}
	}

	for ( ct=0; ct<PRIM_CACHE_SIZE; ct++ )
	{
		if( gc->primitive.CacheAge[ct] > age )
		{
			age = gc->primitive.CacheAge[ct];
			maxIndex = ct;
		}
	}
	gc->primitive.CacheCode[maxIndex] = code;
	gc->primitive.CacheAge[maxIndex] = 0;
		
	switch( primType )
	{
		case GL_POINTS:
			makePointsProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[0] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[0] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_LINES:
			makeLinesProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[1] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[1] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_LINE_LOOP:
			makeLineLoopProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[2] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[2] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_LINE_STRIP:
			makeLineStripProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[3] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[3] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_TRIANGLES:
			makeTrianglesProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[4] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[4] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_TRIANGLE_STRIP:
			makeTriangleStripProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[5] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[5] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_TRIANGLE_FAN:
			makeFanProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[6] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[6] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_QUADS:
			makeQuadsProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[7] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[7] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_QUAD_STRIP:
			makeQuadStripProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[8] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[8] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
		case GL_POLYGON:
			makeFanProcessor( gc, needs, maxIndex );
			gc->primitive.CacheProcess[9] = gc->primitive.CacheProcessPtrs[maxIndex];
			gc->primitive.CacheEnd[9] = &((GLubyte *)gc->primitive.CacheAllocation)[maxIndex * 4096];
			break;
	}
}

