/*
** Copyright 1991, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** $Revision: 1.1 $
** $Date: 1996/12/15 21:16:01 $
*/

#include <string.h>
#include <stdio.h>
#include "GLDefines.h"
#include "context.h"
#include "global.h"
#include "immed.h"
#include "lighting.h"
#include "memservices.h"
#include "rasRaster.h"
#include "driverprocs.h"
#include "texture2.h"

extern void __glSetVapiOptProcs( __glContext *gc );

void __glim_PushAttrib ( __glContext *gc, GLuint mask )
{
	__glAttribute **spp = gc->attributes.stackPointer;
	__glAttribute *sp;

	__GL_CHECK_NOT_IN_BEGIN ();

	if (spp < &gc->attributes.stack[__GL_ATTRIB_STACK_DEPTH])
	{
		if (!(sp = *spp))
		{
			sp = (__glAttribute *) __glCalloc (gc, 1, sizeof (__glAttribute));
			*spp = sp;
		}
		sp->mask = mask;
		gc->attributes.stackPointer = spp + 1;
		if (mask & GL_CURRENT_BIT)
		{
			sp->state.current = gc->state.current;
		}
		if (mask & GL_POINT_BIT)
		{
			sp->state.point = gc->state.point;
		}
		if (mask & GL_LINE_BIT)
		{
			sp->state.line = gc->state.line;
		}
		if (mask & GL_POLYGON_BIT)
		{
			sp->state.poly = gc->state.poly;
		}
		if (mask & GL_POLYGON_STIPPLE_BIT)
		{
			memcpy( sp->state.polyStippleMask, gc->state.polyStippleMask, 128 );
		}
		if (mask & GL_PIXEL_MODE_BIT)
		{
			sp->state.pixel.transferMode = gc->state.pixel.transferMode;
			sp->state.pixel.ReadBuffer = gc->state.pixel.ReadBuffer;
			sp->state.pixel.ReadBufferReturn = gc->state.pixel.ReadBufferReturn;
		}
		if (mask & GL_LIGHTING_BIT)
		{
			sp->state.light = gc->state.light;
		}
		if (mask & GL_FOG_BIT)
		{
			sp->state.fog = gc->state.fog;
		}
		if (mask & GL_DEPTH_BUFFER_BIT)
		{
			sp->state.depth = gc->state.depth;
		}
		if (mask & GL_ACCUM_BUFFER_BIT)
		{
			sp->state.accum = gc->state.accum;
		}
		if (mask & GL_STENCIL_BUFFER_BIT)
		{
			sp->state.stencil = gc->state.stencil;
		}
		if (mask & GL_VIEWPORT_BIT)
		{
			sp->state.viewport = gc->state.viewport;
		}
		if (mask & GL_TRANSFORM_BIT)
		{
			sp->state.xform = gc->state.xform;
		}
		if (mask & GL_ENABLE_BIT)
		{
			int ct;
			sp->state.alpha.TestEnabled = gc->state.alpha.TestEnabled;
			sp->state.evaluator.AutonormalEnabled = gc->state.evaluator.AutonormalEnabled;
			sp->state.color.BlendEnabled = gc->state.color.BlendEnabled;
			sp->state.light.ColorMaterialEnabled = gc->state.light.ColorMaterialEnabled;
			sp->state.poly.CullFaceEnabled = gc->state.poly.CullFaceEnabled;
			sp->state.depth.TestEnabled = gc->state.depth.TestEnabled;
			sp->state.color.DitherEnabled = gc->state.color.DitherEnabled;
			sp->state.fog.Enabled = gc->state.fog.Enabled;
			for( ct=0; ct<8; ct++ )
			{
				sp->state.light.light[ct].Enabled = gc->state.light.light[ct].Enabled;
				DRIVERPROC_LIGHT_LIGHT_ENABLE(gc,ct);
			}
			sp->state.light.LightingEnabled = gc->state.light.LightingEnabled;
			sp->state.line.SmoothEnabled = gc->state.line.SmoothEnabled;
			sp->state.line.StippleEnabled = gc->state.line.StippleEnabled;
			sp->state.color.LogicOpEnabled = gc->state.color.LogicOpEnabled;
			for( ct=0; ct<__GL_MAP_RANGE_COUNT; ct++ )
			{
				sp->state.evaluator.Map1Enabled[ct] = gc->state.evaluator.Map1Enabled[ct];
				sp->state.evaluator.Map2Enabled[ct] = gc->state.evaluator.Map2Enabled[ct];
			}
			sp->state.xform.Normalize = gc->state.xform.Normalize;
			sp->state.point.SmoothEnabled = gc->state.point.SmoothEnabled;
			sp->state.poly.OffsetLineEnabled = gc->state.poly.OffsetLineEnabled;
			sp->state.poly.OffsetFillEnabled = gc->state.poly.OffsetFillEnabled;
			sp->state.poly.OffsetPointEnabled = gc->state.poly.OffsetPointEnabled;
			sp->state.poly.SmoothEnabled = gc->state.poly.SmoothEnabled;
			sp->state.poly.StippleEnabled = gc->state.poly.StippleEnabled;
			sp->state.scissor.Enabled = gc->state.scissor.Enabled;
			sp->state.stencil.TestEnabled = gc->state.stencil.TestEnabled;
			for( ct=0; ct<4; ct++ )
			{
				sp->state.texture.Enabled1D[ct] = gc->state.texture.Enabled1D[ct];
				sp->state.texture.Enabled2D[ct] = gc->state.texture.Enabled2D[ct];
				sp->state.texture.Enabled3D[ct] = gc->state.texture.Enabled3D[ct];

				sp->state.texture.GenEnabled[ct][0] = gc->state.texture.GenEnabled[ct][0];
				sp->state.texture.GenEnabled[ct][1] = gc->state.texture.GenEnabled[ct][1];
				sp->state.texture.GenEnabled[ct][2] = gc->state.texture.GenEnabled[ct][2];
				sp->state.texture.GenEnabled[ct][3] = gc->state.texture.GenEnabled[ct][3];
			}
			DRIVERPROC_LIGHT_ENABLE(gc);
		}
		if (mask & GL_COLOR_BUFFER_BIT)
		{
			sp->state.alpha = gc->state.alpha;
			sp->state.color = gc->state.color;
			sp->state.drawBuffer = gc->state.drawBuffer;
			sp->state.drawBufferReturn = gc->state.drawBufferReturn;
		}
		if (mask & GL_HINT_BIT)
		{
			sp->state.hint = gc->state.hint;
		}
		if (mask & GL_EVAL_BIT)
		{
			sp->state.evaluator = gc->state.evaluator;
		}
		if (mask & GL_LIST_BIT)
		{
			sp->state.list = gc->state.list;
		}
		if (mask & GL_TEXTURE_BIT)
		{
			sp->state.texture = gc->state.texture;
		}
		if (mask & GL_SCISSOR_BIT)
		{
			sp->state.scissor = gc->state.scissor;
		}
	}
	else
	{
		__glSetError (gc, GL_STACK_OVERFLOW);
		return;
	}
}

void __glim_PopAttrib ( __glContext *gc )
{
	__glAttribute **spp = gc->attributes.stackPointer;
	__glAttribute *sp;
	GLuint mask;

	__GL_CHECK_NOT_IN_BEGIN ();
	gc->softScanProcs.valid = 0;

	if (spp > &gc->attributes.stack[0])
	{
		--spp;
		sp = *spp;
		//assert(sp != 0);
		mask = sp->mask;
		gc->attributes.stackPointer = spp;
		if (mask & GL_CURRENT_BIT)
		{
			gc->state.current = sp->state.current;
		}
		if (mask & GL_POINT_BIT)
		{
			gc->state.point = sp->state.point;
			DRIVERPROC_SET_POINT_STATE( gc );
		}
		if (mask & GL_LINE_BIT)
		{
			gc->state.line = sp->state.line;
			DRIVERPROC_SET_LINE_STATE( gc );
		}
		if (mask & GL_POLYGON_BIT)
		{
			gc->state.poly = sp->state.poly;
			gc->valid.All =1;
			gc->valid.PrimProcessor =1;
			DRIVERPROC_SET_POLY_STATE( gc );
		}
		if (mask & GL_POLYGON_STIPPLE_BIT)
		{
			memcpy( gc->state.polyStippleMask, sp->state.polyStippleMask, 128 );
			DRIVERPROC_POLYGON_STIPPLE( gc );
		}
		if (mask & GL_PIXEL_MODE_BIT)
		{
			gc->state.pixel.transferMode = sp->state.pixel.transferMode;
			gc->state.pixel.ReadBuffer = sp->state.pixel.ReadBuffer;
			gc->state.pixel.ReadBufferReturn = sp->state.pixel.ReadBufferReturn;
		}
		if (mask & GL_LIGHTING_BIT)
		{
			gc->state.light = sp->state.light;
			gc->valid.All =1;
			gc->valid.ModelMatrix =1;
			gc->valid.ProjectionMatrix =1;
			gc->valid.VertexProcs =1;
			gc->valid.LightData =1;
			gc->valid.PrimProcessor =1;
			gc->valid.VapiProcessor =1;
			DRIVERPROC_SET_LIGHT_STATE( gc );
		}
		if (mask & GL_FOG_BIT)
		{
			gc->state.fog = sp->state.fog;
			__glPickParameterClipProcs( gc );
			DRIVERPROC_SET_FOG_STATE(gc);
		}
		if (mask & GL_DEPTH_BUFFER_BIT)
		{
			gc->state.depth = sp->state.depth;
			DRIVERPROC_SET_DEPTH_STATE(gc);
		}
		if (mask & GL_ACCUM_BUFFER_BIT)
		{
			gc->state.accum = sp->state.accum;
			DRIVERPROC_ACCUM_CLEAR_COLOR( gc );
		}
		if (mask & GL_STENCIL_BUFFER_BIT)
		{
			gc->state.stencil = sp->state.stencil;
			DRIVERPROC_SET_STENCIL_STATE(gc);
		}
		if (mask & GL_VIEWPORT_BIT)
		{
			gc->state.viewport = sp->state.viewport;
			__glCalcViewport( gc );
		}
		if (mask & GL_SCISSOR_BIT)
		{
			gc->state.scissor = sp->state.scissor;
			DRIVERPROC_SET_SCISSOR_STATE(gc);
		}
		if (mask & GL_TRANSFORM_BIT)
		{
			gc->state.xform = sp->state.xform;
			DRIVERPROC_SET_XFORM_STATE(gc);
		}
		if (mask & GL_ENABLE_BIT)
		{
			int ct;
			gc->state.alpha.TestEnabled = sp->state.alpha.TestEnabled;
			gc->state.evaluator.AutonormalEnabled = sp->state.evaluator.AutonormalEnabled;
			gc->state.color.BlendEnabled = sp->state.color.BlendEnabled;
			gc->state.light.ColorMaterialEnabled = sp->state.light.ColorMaterialEnabled;
			gc->state.poly.CullFaceEnabled = sp->state.poly.CullFaceEnabled;
			gc->state.depth.TestEnabled = sp->state.depth.TestEnabled;
			gc->state.color.DitherEnabled = sp->state.color.DitherEnabled;
			gc->state.fog.Enabled = sp->state.fog.Enabled;
			for( ct=0; ct<8; ct++ )
			{
				gc->state.light.light[ct].Enabled = sp->state.light.light[ct].Enabled;
				DRIVERPROC_LIGHT_LIGHT_ENABLE( gc, ct );
			}
			gc->state.light.LightingEnabled = sp->state.light.LightingEnabled;
			gc->state.line.SmoothEnabled = sp->state.line.SmoothEnabled;
			gc->state.line.StippleEnabled = sp->state.line.StippleEnabled;
			gc->state.color.LogicOpEnabled = sp->state.color.LogicOpEnabled;
			for( ct=0; ct<__GL_MAP_RANGE_COUNT; ct++ )
			{
				gc->state.evaluator.Map1Enabled[ct] = sp->state.evaluator.Map1Enabled[ct];
				gc->state.evaluator.Map2Enabled[ct] = sp->state.evaluator.Map2Enabled[ct];
			}
			gc->state.xform.Normalize = sp->state.xform.Normalize;
			gc->state.xform.ClipPlanesMask = sp->state.xform.ClipPlanesMask;
			for( ct=0; ct<6; ct++ )
			{
				DRIVERPROC_XFORM_CLIP_PLANE_ENABLE(gc, ct);
			}
			gc->state.point.SmoothEnabled = sp->state.point.SmoothEnabled;
			gc->state.poly.OffsetLineEnabled = sp->state.poly.OffsetLineEnabled;
			gc->state.poly.OffsetFillEnabled = sp->state.poly.OffsetFillEnabled;
			gc->state.poly.OffsetPointEnabled = sp->state.poly.OffsetPointEnabled;
			gc->state.poly.SmoothEnabled = sp->state.poly.SmoothEnabled;
			gc->state.poly.StippleEnabled = sp->state.poly.StippleEnabled;
			gc->state.scissor.Enabled = sp->state.scissor.Enabled;
			gc->state.stencil.TestEnabled = sp->state.stencil.TestEnabled;
			for( ct=0; ct<4; ct++ )
			{
				gc->state.texture.Enabled1D[ct] = sp->state.texture.Enabled1D[ct];
				gc->state.texture.Enabled2D[ct] = sp->state.texture.Enabled2D[ct];
				gc->state.texture.Enabled3D[ct] = sp->state.texture.Enabled3D[ct];

				gc->state.texture.GenEnabled[ct][0] = sp->state.texture.GenEnabled[ct][0];
				gc->state.texture.GenEnabled[ct][1] = sp->state.texture.GenEnabled[ct][1];
				gc->state.texture.GenEnabled[ct][2] = sp->state.texture.GenEnabled[ct][2];
				gc->state.texture.GenEnabled[ct][3] = sp->state.texture.GenEnabled[ct][3];
			}
			
			{
				int32 oldUnit = gc->state.texture.SelectedUnit;
				for(ct=0; ct<gc->texture.ActiveUnits; ct++ )
				{
					void *old = gc->texture.Active[ct];

					gc->state.texture.SelectedUnit = ct;
					DRIVERPROC_TEXTURE_GEN_ENABLE( gc, 0 );
					DRIVERPROC_TEXTURE_GEN_ENABLE( gc, 1 );
					DRIVERPROC_TEXTURE_GEN_ENABLE( gc, 2 );
					DRIVERPROC_TEXTURE_GEN_ENABLE( gc, 3 );

					__glPickTextureProcs(gc);
					__glPickActiveTexture( gc );
					DRIVERPROC_TEXTURE_ENABLE( gc, gc->texture.Active[ct] );
	
					if ( old != gc->texture.Active[ct] )
					{
						DRIVERPROC_TEXTURE_SELECT(gc, gc->texture.Active[ct]);
					}
				}
				gc->state.texture.SelectedUnit = oldUnit;
			}

			DRIVERPROC_FOG_ENABLE(gc);
			DRIVERPROC_LIGHT_ENABLE(gc);
			DRIVERPROC_LIGHT_COLOR_MATERIAL_ENABLE(gc);
			DRIVERPROC_LINE_SMOOTH_ENABLE(gc);
			DRIVERPROC_LINE_STIPPLE_ENABLE(gc);
			DRIVERPROC_POLY_CULL_FACE_ENABLE(gc);
			DRIVERPROC_POLY_SMOOTH_ENABLE(gc);
			DRIVERPROC_POLY_OFFSET_POINT_ENABLE(gc);
			DRIVERPROC_POLY_OFFSET_LINE_ENABLE(gc);
			DRIVERPROC_POLY_OFFSET_FILL_ENABLE(gc);
			DRIVERPROC_POLY_STIPPLE_ENABLE(gc);
			DRIVERPROC_SCISSOR_TEST_ENABLE(gc);
			DRIVERPROC_COLOR_ALPHA_TEST_ENABLE(gc);
			DRIVERPROC_COLOR_BLEND_ENABLE(gc);
			DRIVERPROC_COLOR_DITHER_ENABLE(gc);
			DRIVERPROC_COLOR_LOGIC_OP_ENABLE(gc);
			DRIVERPROC_STENCIL_TEST_ENABLE(gc);
			DRIVERPROC_DEPTH_TEST_ENABLE(gc);
			DRIVERPROC_XFORM_NORMALIZATION_ENABLE(gc);
			DRIVERPROC_XFORM_NORMAL_RESCALE_ENABLE(gc);
			
			gc->valid.All =1;
			gc->valid.ModelMatrix =1;
			gc->valid.ProjectionMatrix =1;
			gc->valid.VertexProcs =1;
			gc->valid.LightData =1;
			gc->valid.PrimProcessor =1;
			gc->valid.VapiProcessor =1;
		}
		if (mask & GL_COLOR_BUFFER_BIT)
		{
			gc->state.alpha = sp->state.alpha;
			gc->state.color = sp->state.color;
			gc->state.drawBuffer = sp->state.drawBuffer;
			gc->state.drawBufferReturn = sp->state.drawBufferReturn;
			DRIVERPROC_SET_COLOR_STATE(gc);
		}
		if (mask & GL_HINT_BIT)
		{
			gc->state.hint = sp->state.hint;
		}
		if (mask & GL_EVAL_BIT)
		{
			gc->state.evaluator = sp->state.evaluator;
		}
		if (mask & GL_LIST_BIT)
		{
			gc->state.list = sp->state.list;
		}
		if (mask & GL_TEXTURE_BIT)
		{
			GLint tmu;
			GLint targetIndex;

			/* 
			   ** Must bind the new texture (if any) BEFORE the
			   ** new texture state is applied.
			 */
			 
			for( tmu=0; tmu < gc->texture.ActiveUnits; tmu++ )
			{
				gc->state.texture.SelectedUnit = tmu;
				for (targetIndex = 0; targetIndex < __GL_NUMBER_OF_TEXTURE_TARGETS; targetIndex++ )
				{
					__glBindTexture (gc, targetIndex, sp->state.texture.Bound[tmu][targetIndex]->name );
				}
			}
			

			/*
			   ** Install the stacked state into the current state.
			 */
			gc->state.texture = sp->state.texture;

			/*
			   ** Install the new texture state into the current texture.
			 */
			for( tmu=0; tmu < gc->texture.ActiveUnits; tmu++ )
			{
				for (targetIndex = 0; targetIndex < __GL_NUMBER_OF_TEXTURE_TARGETS; targetIndex++ )
				{
					gc->state.texture.Bound[tmu][targetIndex]->borderColor = gc->state.texture.Bound[tmu][targetIndex]->borderColor;
					gc->state.texture.Bound[tmu][targetIndex]->sWrapMode = gc->state.texture.Bound[tmu][targetIndex]->sWrapMode;
					gc->state.texture.Bound[tmu][targetIndex]->tWrapMode = gc->state.texture.Bound[tmu][targetIndex]->tWrapMode;
					gc->state.texture.Bound[tmu][targetIndex]->rWrapMode = gc->state.texture.Bound[tmu][targetIndex]->rWrapMode;
					gc->state.texture.Bound[tmu][targetIndex]->minFilter = gc->state.texture.Bound[tmu][targetIndex]->minFilter;
					gc->state.texture.Bound[tmu][targetIndex]->magFilter = gc->state.texture.Bound[tmu][targetIndex]->magFilter;
					gc->state.texture.Bound[tmu][targetIndex]->priority = gc->state.texture.Bound[tmu][targetIndex]->priority;
				}
			}
			DRIVERPROC_SET_TEXTURE_STATE(gc);


			gc->valid.All =1;
			gc->valid.VertexProcs =1;
			gc->valid.PrimProcessor =1;
			gc->valid.VapiProcessor =1;
		}

		/*
		   ** Clear out mask so that any memory frees done above won't get
		   ** re-done when the context is destroyed
		 */
		sp->mask = 0;

		/* Update methods procs */
		__glPickAllProcs(gc);
	}
	else
	{
		__glSetError (gc, GL_STACK_UNDERFLOW);
		return;
	}
}

/*****************************************************************************/

void __glim_PushClientAttrib ( __glContext *gc, GLuint mask )
{
	__glClientAttribute **spp = gc->attributes.clientStackPointer;
	__glClientAttribute *sp;

	__GL_CHECK_NOT_IN_BEGIN ();

	if (spp < &gc->attributes.clientStack[__GL_CLIENT_ATTRIB_STACK_DEPTH])
	{
		if (!(sp = *spp))
		{
			sp = (__glClientAttribute *)
				__glCalloc (gc, 1, sizeof (__glClientAttribute));
			*spp = sp;
		}
		sp->mask = mask;
		gc->attributes.clientStackPointer = spp + 1;
		if (mask & GL_CLIENT_PIXEL_STORE_BIT)
		{
			sp->pixel.packModes = gc->state.pixel.packModes;
			sp->pixel.unpackModes = gc->state.pixel.unpackModes;
		}
		if (mask & GL_CLIENT_VERTEX_ARRAY_BIT)
		{
			sp->vertexArray = gc->state.vertexArray;
		}
	}
	else
	{
		__glSetError (gc, GL_STACK_OVERFLOW);
		return;
	}
}

void __glim_PopClientAttrib ( __glContext *gc )
{
	__glClientAttribute **spp = gc->attributes.clientStackPointer;
	__glClientAttribute *sp;
	GLuint mask;

	__GL_CHECK_NOT_IN_BEGIN ();

	if (spp > &gc->attributes.clientStack[0])
	{
		--spp;
		sp = *spp;
		//assert(sp != 0);
		mask = sp->mask;
		gc->attributes.clientStackPointer = spp;
		if (mask & GL_CLIENT_PIXEL_STORE_BIT)
		{
			gc->state.pixel.packModes = sp->pixel.packModes;
			gc->state.pixel.unpackModes = sp->pixel.unpackModes;
		}
		if (mask & GL_CLIENT_VERTEX_ARRAY_BIT)
		{
			gc->state.vertexArray = sp->vertexArray;
		}

		/*
		   ** Clear out mask so that any memory frees done above won't get
		   ** re-done when the context is destroyed
		 */
		sp->mask = 0;

		/* Update methods procs */
		__glPickAllProcs(gc);
	}
	else
	{
		__glSetError (gc, GL_STACK_UNDERFLOW);
		return;
	}
}

/*****************************************************************************/

void __gl_MakeArrawElement_Slow( __glContext *gc );

void __glim_EnableClientState ( __glContext *gc, GLenum array )
{
	switch (array)
	{
	case GL_COLOR_ARRAY:
		if( gc->state.vertexArray.ColorArrayEnabled != GL_TRUE )
		{
			gc->state.vertexArray.ColorArrayEnabled = GL_TRUE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_EDGE_FLAG_ARRAY:
		if( gc->state.vertexArray.EdgeFlagArrayEnabled != GL_TRUE )
		{
			gc->state.vertexArray.EdgeFlagArrayEnabled = GL_TRUE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_INDEX_ARRAY:
		if( gc->state.vertexArray.IndexArrayEnabled != GL_TRUE )
		{
			gc->state.vertexArray.IndexArrayEnabled = GL_TRUE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_NORMAL_ARRAY:
		if( gc->state.vertexArray.NormalArrayEnabled != GL_TRUE )
		{
			gc->state.vertexArray.NormalArrayEnabled = GL_TRUE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_TEXTURE_COORD_ARRAY:
		if( gc->state.vertexArray.TexCoordArrayEnabled[gc->state.vertexArray.SelectedTextureUnit] != GL_TRUE )
		{
			gc->state.vertexArray.TexCoordArrayEnabled[gc->state.vertexArray.SelectedTextureUnit] = GL_TRUE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_VERTEX_ARRAY:
		if( gc->state.vertexArray.VertexArrayEnabled != GL_TRUE )
		{
			gc->state.vertexArray.VertexArrayEnabled = GL_TRUE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
	}
}

void __glim_DisableClientState ( __glContext *gc, GLenum array )
{
	switch (array)
	{
	case GL_COLOR_ARRAY:
		if( gc->state.vertexArray.ColorArrayEnabled != GL_FALSE )
		{
			gc->state.vertexArray.ColorArrayEnabled = GL_FALSE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_EDGE_FLAG_ARRAY:
		if( gc->state.vertexArray.EdgeFlagArrayEnabled != GL_FALSE )
		{
			gc->state.vertexArray.EdgeFlagArrayEnabled = GL_FALSE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_INDEX_ARRAY:
		if( gc->state.vertexArray.IndexArrayEnabled != GL_FALSE )
		{
			gc->state.vertexArray.IndexArrayEnabled = GL_FALSE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_NORMAL_ARRAY:
		if( gc->state.vertexArray.NormalArrayEnabled != GL_FALSE )
		{
			gc->state.vertexArray.NormalArrayEnabled = GL_FALSE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_TEXTURE_COORD_ARRAY:
		if( gc->state.vertexArray.TexCoordArrayEnabled[gc->state.vertexArray.SelectedTextureUnit] != GL_FALSE )
		{
			gc->state.vertexArray.TexCoordArrayEnabled[gc->state.vertexArray.SelectedTextureUnit] = GL_FALSE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	case GL_VERTEX_ARRAY:
		if( gc->state.vertexArray.VertexArrayEnabled != GL_FALSE )
		{
			gc->state.vertexArray.VertexArrayEnabled = GL_FALSE;
			__gl_MakeArrawElement_Slow( gc );
		}
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
	}
}

/*****************************************************************************/

void __glim_Enable ( __glContext *gc, GLenum cap )
{
	__GL_CHECK_NOT_IN_BEGIN ();
	gc->softScanProcs.valid = 0;

	switch (cap)
	{
	case GL_ALPHA_TEST:
		gc->state.alpha.TestEnabled = GL_TRUE;
		DRIVERPROC_COLOR_ALPHA_TEST_ENABLE(gc);
		return;
	case GL_BLEND:
		gc->state.color.BlendEnabled = GL_TRUE;
		DRIVERPROC_COLOR_BLEND_ENABLE(gc);
		return;
	case GL_COLOR_LOGIC_OP:
		gc->state.color.LogicOpEnabled = GL_TRUE;
		DRIVERPROC_COLOR_LOGIC_OP_ENABLE(gc);
		break;
	case GL_COLOR_MATERIAL:
		gc->state.light.ColorMaterialEnabled = GL_TRUE;
		__glChangeMaterialColor (gc);
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		DRIVERPROC_LIGHT_COLOR_MATERIAL_ENABLE(gc);
		break;
	case GL_CULL_FACE:
		gc->state.poly.CullFaceEnabled = GL_TRUE;
		gc->primitive.CullAndMask = 0xff;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		DRIVERPROC_POLY_CULL_FACE_ENABLE(gc);
		return;
	case GL_DEPTH_TEST:
		if (!gc->buffer.current->DepthEnabled)
		{
#if DEBUG_TEXT
			printf( "enabling depth buffer when not present \n" );
#endif
			__glSetError (gc, GL_INVALID_OPERATION);
		}
		gc->state.depth.TestEnabled = GL_TRUE;
		DRIVERPROC_DEPTH_TEST_ENABLE(gc);
		return;
	case GL_DITHER:
		gc->state.color.DitherEnabled = GL_TRUE;
		DRIVERPROC_COLOR_DITHER_ENABLE(gc);
		break;
	case GL_FOG:
		gc->state.fog.Enabled = GL_TRUE;
		DRIVERPROC_FOG_ENABLE(gc);
		__glPickParameterClipProcs( gc );
		gc->valid.All = 1;
		break;
	case GL_INDEX_LOGIC_OP:
//      es->colorBuffer.indexLogicOp = GL_TRUE;
		break;
	case GL_LIGHTING:
		gc->state.light.LightingEnabled = GL_TRUE;
		gc->valid.ModelMatrix = 1;
		gc->valid.VapiProcessor = 1;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		__glValidateLighting( gc );
		__glPickParameterClipProcs( gc );
		DRIVERPROC_LIGHT_ENABLE(gc);
		break;
	case GL_LINE_SMOOTH:
		gc->state.line.SmoothEnabled = GL_TRUE;
		DRIVERPROC_LINE_SMOOTH_ENABLE( gc );
		return;
	case GL_LINE_STIPPLE:
		gc->state.line.StippleEnabled = GL_TRUE;
		DRIVERPROC_LINE_STIPPLE_ENABLE(gc);
		break;
	case GL_NORMALIZE:
		gc->state.xform.Normalize = GL_TRUE;
		gc->valid.All = 1;
		DRIVERPROC_XFORM_NORMALIZATION_ENABLE(gc);
		break;
	case GL_POINT_SMOOTH:
		gc->state.point.SmoothEnabled = GL_TRUE;
		DRIVERPROC_POINT_SMOOTH_ENABLE(gc);
		return;
	case GL_POLYGON_SMOOTH:
		gc->state.poly.SmoothEnabled = GL_TRUE;
		DRIVERPROC_POLY_SMOOTH_ENABLE(gc);
		return;
	case GL_POLYGON_STIPPLE:
		gc->state.poly.StippleEnabled = GL_TRUE;
		DRIVERPROC_POLY_STIPPLE_ENABLE(gc);
		break;
	case GL_SCISSOR_TEST:
		gc->state.scissor.Enabled = GL_TRUE;
		__glComputeClipBox (gc);
		DRIVERPROC_SCISSOR_TEST_ENABLE(gc);
		break;
	case GL_STENCIL_TEST:
		gc->state.stencil.TestEnabled = GL_TRUE;
		DRIVERPROC_STENCIL_TEST_ENABLE(gc);
		break;
	case GL_TEXTURE_1D:
		{
			void *old = gc->texture.Active[gc->state.texture.SelectedUnit];
			gc->state.texture.Enabled1D[gc->state.texture.SelectedUnit] = GL_TRUE;
			__glPickTextureProcs(gc);
			__glPickActiveTexture( gc );
			DRIVERPROC_TEXTURE_ENABLE( gc, gc->texture.Active[gc->state.texture.SelectedUnit] );
			if ( old != gc->texture.Active )
			{
				DRIVERPROC_TEXTURE_SELECT(gc, gc->texture.Active[gc->state.texture.SelectedUnit]);
			}
			gc->valid.VapiProcessor = 1;
			gc->valid.PrimProcessor = 1;
			gc->valid.All = 1;
			__glPickParameterClipProcs( gc );
			__glPickAllProcs( gc );
		}
	case GL_TEXTURE_2D:
		{
			void *old = gc->texture.Active[gc->state.texture.SelectedUnit];
			gc->state.texture.Enabled2D[gc->state.texture.SelectedUnit] = GL_TRUE;
			__glPickTextureProcs(gc);
			__glPickActiveTexture( gc );
			DRIVERPROC_TEXTURE_ENABLE( gc, gc->texture.Active[gc->state.texture.SelectedUnit] );
			if ( old != gc->texture.Active )
			{
				DRIVERPROC_TEXTURE_SELECT(gc, gc->texture.Active[gc->state.texture.SelectedUnit]);
			}
			gc->valid.VapiProcessor = 1;
			gc->valid.PrimProcessor = 1;
			gc->valid.All = 1;
			__glPickParameterClipProcs( gc );
			__glPickAllProcs( gc );
		}
		return;
	case GL_TEXTURE_3D:
		{
			void *old = gc->texture.Active[gc->state.texture.SelectedUnit];
			gc->state.texture.Enabled3D[gc->state.texture.SelectedUnit] = GL_TRUE;
			__glPickTextureProcs(gc);
			__glPickActiveTexture( gc );
			DRIVERPROC_TEXTURE_ENABLE( gc, gc->texture.Active[gc->state.texture.SelectedUnit] );
			if ( old != gc->texture.Active )
			{
				DRIVERPROC_TEXTURE_SELECT(gc, gc->texture.Active[gc->state.texture.SelectedUnit]);
			}
			gc->valid.VapiProcessor = 1;
			gc->valid.PrimProcessor = 1;
			gc->valid.All = 1;
			__glPickParameterClipProcs( gc );
			__glPickAllProcs( gc );
		}
		return;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
		cap -= GL_CLIP_PLANE0;
		gc->state.xform.ClipPlanesMask |= (1 << cap);
		gc->valid.PrimProcessor = 1;
		gc->valid.VapiProcessor = 1;
		gc->valid.All = 1;
		DRIVERPROC_XFORM_CLIP_PLANE_ENABLE(gc, cap);
		break;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		cap -= GL_LIGHT0;
		gc->state.light.light[cap].Enabled = GL_TRUE;
		DRIVERPROC_LIGHT_LIGHT_ENABLE(gc,cap);
		break;
	case GL_MAP1_COLOR_4:
	case GL_MAP1_NORMAL:
	case GL_MAP1_INDEX:
	case GL_MAP1_TEXTURE_COORD_1:
	case GL_MAP1_TEXTURE_COORD_2:
	case GL_MAP1_TEXTURE_COORD_3:
	case GL_MAP1_TEXTURE_COORD_4:
	case GL_MAP1_VERTEX_3:
	case GL_MAP1_VERTEX_4:
		cap = __GL_EVAL1D_INDEX (cap);
		gc->state.evaluator.Map1Enabled[cap] = GL_TRUE;
		break;
	case GL_MAP2_COLOR_4:
	case GL_MAP2_NORMAL:
	case GL_MAP2_INDEX:
	case GL_MAP2_TEXTURE_COORD_1:
	case GL_MAP2_TEXTURE_COORD_2:
	case GL_MAP2_TEXTURE_COORD_3:
	case GL_MAP2_TEXTURE_COORD_4:
	case GL_MAP2_VERTEX_3:
	case GL_MAP2_VERTEX_4:
		cap = __GL_EVAL2D_INDEX (cap);
		gc->state.evaluator.Map2Enabled[cap] = GL_TRUE;
		break;
	case GL_AUTO_NORMAL:
		gc->state.evaluator.AutonormalEnabled = GL_TRUE;
		break;
	case GL_TEXTURE_GEN_S:
	case GL_TEXTURE_GEN_T:
	case GL_TEXTURE_GEN_R:
	case GL_TEXTURE_GEN_Q:
		cap -= GL_TEXTURE_GEN_S;
		gc->state.texture.GenEnabled[gc->state.texture.SelectedUnit][cap] = GL_TRUE;
		DRIVERPROC_TEXTURE_GEN_ENABLE( gc, cap );
		break;

	case GL_POLYGON_OFFSET_POINT:
		gc->state.poly.OffsetPointEnabled = GL_TRUE;
		DRIVERPROC_POLY_OFFSET_POINT_ENABLE(gc);
		break;
	case GL_POLYGON_OFFSET_LINE:
		gc->state.poly.OffsetLineEnabled = GL_TRUE;
		DRIVERPROC_POLY_OFFSET_LINE_ENABLE(gc);
		break;
	case GL_POLYGON_OFFSET_FILL:
		gc->state.poly.OffsetFillEnabled = GL_TRUE;
		DRIVERPROC_POLY_OFFSET_FILL_ENABLE(gc);
		break;
		
		
	case GL_USE_TEMP_COLOR_BE:
//		gc->state.opt.UseTempColor = GL_TRUE;
//		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEMP_TEX_BE:
//		gc->state.opt.UseTempTex[gc->state.texture.SelectedUnit] = GL_TRUE;
//		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEMP_NORMAL_BE:
//		gc->state.opt.UseTempNormal = GL_TRUE;
//		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEX2_BE:
		gc->state.opt.Tex2[gc->state.texture.SelectedUnit] = GL_TRUE;
		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEX3_BE:
		gc->state.opt.Tex3[gc->state.texture.SelectedUnit] = GL_TRUE;
		__glSetVapiOptProcs( gc );
		return;

	default:
		__glSetError (gc, GL_INVALID_ENUM);
		return;
	}
	__glPickAllProcs(gc);
}

void __glim_Disable ( __glContext *gc, GLenum cap )
{
	__GL_CHECK_NOT_IN_BEGIN ();
	gc->softScanProcs.valid = 0;

	switch (cap)
	{
	case GL_ALPHA_TEST:
		gc->state.alpha.TestEnabled = GL_FALSE;
		DRIVERPROC_COLOR_ALPHA_TEST_ENABLE(gc);
		return;
	case GL_BLEND:
		gc->state.color.BlendEnabled = GL_FALSE;
		DRIVERPROC_COLOR_BLEND_ENABLE(gc);
		return;
	case GL_COLOR_LOGIC_OP:
		gc->state.color.LogicOpEnabled = GL_FALSE;
		DRIVERPROC_COLOR_LOGIC_OP_ENABLE( gc );
		break;
	case GL_COLOR_MATERIAL:
		gc->state.light.ColorMaterialEnabled = GL_FALSE;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		DRIVERPROC_LIGHT_COLOR_MATERIAL_ENABLE(gc);
		break;
	case GL_CULL_FACE:
		gc->state.poly.CullFaceEnabled = GL_FALSE;
		gc->primitive.CullAndMask = 0x00;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		DRIVERPROC_POLY_CULL_FACE_ENABLE( gc );
		return;
	case GL_DEPTH_TEST:
		gc->state.depth.TestEnabled = GL_FALSE;
		DRIVERPROC_DEPTH_TEST_ENABLE(gc);
		return;
	case GL_DITHER:
		gc->state.color.DitherEnabled = GL_FALSE;
		DRIVERPROC_COLOR_DITHER_ENABLE(gc);
		break;
	case GL_FOG:
		gc->state.fog.Enabled = GL_FALSE;
		DRIVERPROC_FOG_ENABLE(gc);
		__glPickParameterClipProcs( gc );
		gc->valid.All = 1;
		return;
	case GL_INDEX_LOGIC_OP:
//      es->colorBuffer.indexLogicOp = GL_FALSE;
		break;
	case GL_LIGHTING:
		gc->state.light.LightingEnabled = GL_FALSE;
		gc->valid.ModelMatrix = 1;
		gc->valid.VapiProcessor = 1;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		__glValidateLighting( gc );
		__glPickParameterClipProcs( gc );
		DRIVERPROC_LIGHT_ENABLE(gc);
		break;
	case GL_LINE_SMOOTH:
		gc->state.line.SmoothEnabled = GL_FALSE;
		DRIVERPROC_LINE_SMOOTH_ENABLE( gc );
		return;
	case GL_LINE_STIPPLE:
		gc->state.line.StippleEnabled = GL_FALSE;
		DRIVERPROC_LINE_STIPPLE_ENABLE(gc);
		break;
	case GL_NORMALIZE:
		gc->state.xform.Normalize = GL_FALSE;
		gc->valid.All = 1;
		DRIVERPROC_XFORM_NORMALIZATION_ENABLE( gc );
		break;
	case GL_POINT_SMOOTH:
		gc->state.point.SmoothEnabled = GL_FALSE;
		DRIVERPROC_POINT_SMOOTH_ENABLE(gc);
		return;
	case GL_POLYGON_SMOOTH:
		gc->state.poly.SmoothEnabled = GL_FALSE;
		DRIVERPROC_POLY_SMOOTH_ENABLE(gc);
		return;
	case GL_POLYGON_STIPPLE:
		gc->state.poly.StippleEnabled = GL_FALSE;
		DRIVERPROC_POLY_STIPPLE_ENABLE(gc);
		break;
	case GL_SCISSOR_TEST:
		gc->state.scissor.Enabled = GL_FALSE;
		__glComputeClipBox (gc);
		DRIVERPROC_SCISSOR_TEST_ENABLE(gc);
		break;
	case GL_STENCIL_TEST:
		gc->state.stencil.TestEnabled = GL_FALSE;
		DRIVERPROC_STENCIL_TEST_ENABLE(gc);
		break;
	case GL_TEXTURE_1D:
		gc->state.texture.Enabled1D[gc->state.texture.SelectedUnit] = GL_FALSE;
		__glPickActiveTexture( gc );
		__glPickTextureProcs(gc);
		DRIVERPROC_TEXTURE_ENABLE( gc, gc->texture.Active[gc->state.texture.SelectedUnit] );
		gc->valid.VapiProcessor = 1;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		__glPickParameterClipProcs( gc );
		__glPickAllProcs( gc );
		return;
	case GL_TEXTURE_2D:
		gc->state.texture.Enabled2D[gc->state.texture.SelectedUnit] = GL_FALSE;
		__glPickActiveTexture( gc );
		__glPickTextureProcs(gc);
		DRIVERPROC_TEXTURE_ENABLE( gc, gc->texture.Active[gc->state.texture.SelectedUnit] );
		gc->valid.VapiProcessor = 1;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		__glPickParameterClipProcs( gc );
		__glPickAllProcs( gc );
		return;
	case GL_TEXTURE_3D:
		gc->state.texture.Enabled3D[gc->state.texture.SelectedUnit] = GL_FALSE;
		__glPickActiveTexture( gc );
		__glPickTextureProcs(gc);
		DRIVERPROC_TEXTURE_ENABLE( gc, gc->texture.Active[gc->state.texture.SelectedUnit] );
		gc->valid.VapiProcessor = 1;
		gc->valid.PrimProcessor = 1;
		gc->valid.All = 1;
		__glPickParameterClipProcs( gc );
		__glPickAllProcs( gc );
		return;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
		cap -= GL_CLIP_PLANE0;
		gc->state.xform.ClipPlanesMask &= ~(1 << cap);
		gc->valid.PrimProcessor = 1;
		gc->valid.VapiProcessor = 1;
		gc->valid.All = 1;
		DRIVERPROC_XFORM_CLIP_PLANE_ENABLE(gc, cap);
		break;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		cap -= GL_LIGHT0;
		gc->state.light.light[cap].Enabled = GL_FALSE;
		DRIVERPROC_LIGHT_LIGHT_ENABLE(gc,cap);
		break;
	case GL_MAP1_COLOR_4:
	case GL_MAP1_NORMAL:
	case GL_MAP1_INDEX:
	case GL_MAP1_TEXTURE_COORD_1:
	case GL_MAP1_TEXTURE_COORD_2:
	case GL_MAP1_TEXTURE_COORD_3:
	case GL_MAP1_TEXTURE_COORD_4:
	case GL_MAP1_VERTEX_3:
	case GL_MAP1_VERTEX_4:
		cap = __GL_EVAL1D_INDEX (cap);
		gc->state.evaluator.Map1Enabled[cap] = GL_FALSE;
		break;
	case GL_MAP2_COLOR_4:
	case GL_MAP2_NORMAL:
	case GL_MAP2_INDEX:
	case GL_MAP2_TEXTURE_COORD_1:
	case GL_MAP2_TEXTURE_COORD_2:
	case GL_MAP2_TEXTURE_COORD_3:
	case GL_MAP2_TEXTURE_COORD_4:
	case GL_MAP2_VERTEX_3:
	case GL_MAP2_VERTEX_4:
		cap = __GL_EVAL2D_INDEX (cap);
		gc->state.evaluator.Map2Enabled[cap] = GL_FALSE;
		break;
	case GL_AUTO_NORMAL:
		gc->state.evaluator.AutonormalEnabled = GL_FALSE;
		break;
	case GL_TEXTURE_GEN_S:
	case GL_TEXTURE_GEN_T:
	case GL_TEXTURE_GEN_R:
	case GL_TEXTURE_GEN_Q:
		cap -= GL_TEXTURE_GEN_S;
		gc->state.texture.GenEnabled[gc->state.texture.SelectedUnit][cap] = GL_FALSE;
		DRIVERPROC_TEXTURE_GEN_ENABLE(gc, cap);
		break;

		/* Extension enables */
	case GL_POLYGON_OFFSET_POINT:
		gc->state.poly.OffsetPointEnabled = GL_FALSE;
		DRIVERPROC_POLY_OFFSET_POINT_ENABLE(gc);
		break;
	case GL_POLYGON_OFFSET_LINE:
		gc->state.poly.OffsetLineEnabled = GL_FALSE;
		DRIVERPROC_POLY_OFFSET_LINE_ENABLE(gc);
		break;
	case GL_POLYGON_OFFSET_FILL:
		gc->state.poly.OffsetFillEnabled = GL_FALSE;
		DRIVERPROC_POLY_OFFSET_FILL_ENABLE(gc);
		break;

	case GL_USE_TEMP_COLOR_BE:
//		gc->state.opt.UseTempColor = GL_FALSE;
//		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEMP_TEX_BE:
//		gc->state.opt.UseTempTex[gc->state.texture.SelectedUnit] = GL_FALSE;
//		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEMP_NORMAL_BE:
//		gc->state.opt.UseTempNormal = GL_FALSE;
//		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEX2_BE:
		gc->state.opt.Tex2[gc->state.texture.SelectedUnit] = GL_FALSE;
		__glSetVapiOptProcs( gc );
		return;
	case GL_USE_TEX3_BE:
		gc->state.opt.Tex3[gc->state.texture.SelectedUnit] = GL_FALSE;
		__glSetVapiOptProcs( gc );
		return;

	default:
		__glSetError (gc, GL_INVALID_ENUM);
		return;
	}
	__glPickAllProcs(gc);
}

GLboolean __glim_IsEnabled ( __glContext *gc, GLenum cap )
{
	__GL_CHECK_NOT_IN_BEGIN2 ();

	switch (cap)
	{
	case GL_ALPHA_TEST:
		return gc->state.alpha.TestEnabled;
	case GL_BLEND:
		return gc->state.color.BlendEnabled;
	case GL_COLOR_LOGIC_OP:
		return gc->state.color.LogicOpEnabled;
	case GL_COLOR_MATERIAL:
		return gc->state.light.ColorMaterialEnabled;
	case GL_CULL_FACE:
		return gc->state.poly.CullFaceEnabled;
	case GL_DEPTH_TEST:
		return gc->state.depth.TestEnabled;
	case GL_DITHER:
		return gc->state.color.DitherEnabled;
	case GL_FOG:
		return gc->state.fog.Enabled;
	case GL_INDEX_LOGIC_OP:
		return GL_FALSE;		//es->colorBuffer.indexLogicOp;

	case GL_LIGHTING:
		return gc->state.light.LightingEnabled;
	case GL_LINE_SMOOTH:
		return gc->state.line.SmoothEnabled;
	case GL_LINE_STIPPLE:
		return gc->state.line.StippleEnabled;
	case GL_NORMALIZE:
		return gc->state.xform.Normalize;
	case GL_POINT_SMOOTH:
		return gc->state.point.SmoothEnabled;
	case GL_POLYGON_SMOOTH:
		return gc->state.poly.SmoothEnabled;
	case GL_POLYGON_STIPPLE:
		return gc->state.poly.StippleEnabled;
	case GL_SCISSOR_TEST:
		return gc->state.scissor.Enabled;
	case GL_STENCIL_TEST:
		return gc->state.stencil.TestEnabled;
	case GL_TEXTURE_1D:
		return gc->state.texture.Enabled1D[gc->state.texture.SelectedUnit];
	case GL_TEXTURE_2D:
		return gc->state.texture.Enabled2D[gc->state.texture.SelectedUnit];
	case GL_AUTO_NORMAL:
		return gc->state.evaluator.AutonormalEnabled;

	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
		cap -= GL_CLIP_PLANE0;
		return (gc->state.xform.ClipPlanesMask & (1 << cap)) != 0;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		cap -= GL_LIGHT0;
		return gc->state.light.light[cap].Enabled;
	case GL_MAP1_COLOR_4:
	case GL_MAP1_NORMAL:
	case GL_MAP1_INDEX:
	case GL_MAP1_TEXTURE_COORD_1:
	case GL_MAP1_TEXTURE_COORD_2:
	case GL_MAP1_TEXTURE_COORD_3:
	case GL_MAP1_TEXTURE_COORD_4:
	case GL_MAP1_VERTEX_3:
	case GL_MAP1_VERTEX_4:
		cap = __GL_EVAL1D_INDEX (cap);
		return gc->state.evaluator.Map1Enabled[cap];
	case GL_MAP2_COLOR_4:
	case GL_MAP2_NORMAL:
	case GL_MAP2_INDEX:
	case GL_MAP2_TEXTURE_COORD_1:
	case GL_MAP2_TEXTURE_COORD_2:
	case GL_MAP2_TEXTURE_COORD_3:
	case GL_MAP2_TEXTURE_COORD_4:
	case GL_MAP2_VERTEX_3:
	case GL_MAP2_VERTEX_4:
		cap = __GL_EVAL2D_INDEX (cap);
		return gc->state.evaluator.Map2Enabled[cap];
	case GL_TEXTURE_GEN_S:
	case GL_TEXTURE_GEN_T:
	case GL_TEXTURE_GEN_R:
	case GL_TEXTURE_GEN_Q:
		cap -= GL_TEXTURE_GEN_S;
		return gc->state.texture.GenEnabled[gc->state.texture.SelectedUnit][cap];

	case GL_VERTEX_ARRAY:
		return gc->state.vertexArray.VertexArrayEnabled;
	case GL_NORMAL_ARRAY:
		return gc->state.vertexArray.NormalArrayEnabled;
	case GL_COLOR_ARRAY:
		return gc->state.vertexArray.ColorArrayEnabled;
	case GL_INDEX_ARRAY:
		return gc->state.vertexArray.IndexArrayEnabled;
	case GL_TEXTURE_COORD_ARRAY:
		return gc->state.vertexArray.TexCoordArrayEnabled[gc->state.vertexArray.SelectedTextureUnit];
	case GL_EDGE_FLAG_ARRAY:
		return gc->state.vertexArray.EdgeFlagArrayEnabled;

	case GL_POLYGON_OFFSET_POINT:
		return gc->state.poly.OffsetPointEnabled;
	case GL_POLYGON_OFFSET_LINE:
		return gc->state.poly.OffsetLineEnabled;
	case GL_POLYGON_OFFSET_FILL:
		return gc->state.poly.OffsetFillEnabled;

	default:
		__glSetError (gc, GL_INVALID_ENUM);
		return GL_FALSE;
	}
}
