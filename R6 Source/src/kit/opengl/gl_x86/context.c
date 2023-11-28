/*
   ** Copyright 1991, 1992, Silicon Graphics, Inc.
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
   ** $Revision: 1.50 $
   ** $Date: 1996/03/13 06:45:29 $
*/

#include <stdlib.h>
#include <stdio.h>
#include <GL/glu.h>
#include <OS.h>
#include <opengl/bitflinger.h>
#include "memservices.h" 
#include "context.h"
#include "pixel.h"
#include "global.h"
#include "glImage.h"
#include "g_ptab.h"
#include "g_listop.h"
#include "rasTriangle.h"
#include "ScanlineRaster.h"

GLubyte primitiveTempProcessors[4096 * 32];
GLubyte vertexTempProcessors[4096 * 16];
extern GLuint invert[1024];

static int getDebugValue( const char *key )
{
	char *envString = getenv( key );
	int t=0;
	if( envString )
	{
		sscanf( envString, "%i", &t );
		if( t < 0 )
			t = 0;
		if( t > 255 )
			t = 255;
		if( t )
			printf( "%s enabled at level %i \n", key, t );
	}
	return t;
}

static void tlCallback( __glContext *gc )
{
	if( gc->isRendering )
		__glSetToImmedTable(gc);
}

static int isInBegin( __glContext *gc )
{
	return (gc->primitive.End != (void (*)(__glContext*)) __glNop);
}

void __glInitContext( __glContext *gc )
{
	GLint ct;

	memset (gc, 0, sizeof (__glContext));

	gc->info.state_h_version = __state_h__VERSION;
	
#if 1
	gc->primitive.CacheAllocation = MALLOC( 4096 * 32 );
	gc->vertex.CacheAllocation = MALLOC( 4096 * 16 );
#else
	gc->primitive.CacheAllocation = &primitiveTempProcessors[0];//malloc( 4096 * 32 );
	gc->vertex.CacheAllocation = &vertexTempProcessors[0];//malloc( 4096 * 16 );
#endif

	for (ct = 1; ct < 1024; ct++)
		invert[ct] = (0x100000 + (ct >> 1)) / ct;
		
	/* Initialize the surface state */
	__glSurfaceInit( gc );
	


	/* Set have bits for each buffer the user asked for */
	gc->auxBuffers = 0;

	__glComputeClipBox (gc);

	/* Software initialization */
	if( gc->info.debugModes )
		printf( "BDirectGLWindow::InitializeGL  Buffers C=%x  D=%x  S=%x  A=%x \n", (int)gc->buffer.current->DepthEnabled,
		(int)gc->buffer.current->StencilEnabled, (int)gc->buffer.current->AccumEnabled );
	
	gc->buffer.current->ColorScale.R = 255;
	gc->buffer.current->ColorScale.G = 255;
	gc->buffer.current->ColorScale.B = 255;
	gc->buffer.current->ColorScale.A = 255;
	
	/* Fog */
	gc->state.fog.Color.R=0;	
	gc->state.fog.Color.G=0;	
	gc->state.fog.Color.B=0;	
	gc->state.fog.Color.A=0;	
	gc->state.fog.Enabled = GL_FALSE;
	gc->state.fog.Mode = GL_EXP;
	gc->state.fog.Density = 1.0;
	gc->state.fog.Start = 0.0;
	gc->state.fog.End = 1.0;
	
	/* Polygon */
	gc->state.poly.FrontFaceDirection = GL_CCW;
	gc->state.poly.FrontMode = GL_FILL;
	gc->state.poly.BackMode = GL_FILL;
	gc->state.poly.Cull = GL_BACK;
	gc->state.poly.OffsetFactor = 0.0;
	gc->state.poly.OffsetUnit = 0.0;
	gc->state.poly.OffsetR = 0.000061; // 2.0 / ((float) pow ((double) 2.0, (double) 15/*bits*/) - 1.0);
	gc->state.poly.CullFaceEnabled = GL_FALSE;
	gc->state.poly.SmoothEnabled = GL_FALSE;
	gc->state.poly.OffsetPointEnabled = GL_FALSE;
	gc->state.poly.OffsetLineEnabled = GL_FALSE;
	gc->state.poly.OffsetFillEnabled = GL_FALSE;

	// Polygon Stipple
	memset( gc->state.polyStippleMask, 0, 128 );
	
	// Line
	gc->state.line.SmoothEnabled = GL_FALSE;
	gc->state.line.RequestedWidth = 1;
	gc->state.line.SmoothWidth = 1.0;
	gc->state.line.AliasedWidth = 1;
	gc->state.line.StippleEnabled = GL_FALSE;
	gc->state.line.StippleMask = 0xffff;
	gc->state.line.StippleRepeat = GL_TRUE;
	
	// Point
	gc->state.point.SmoothEnabled = GL_FALSE;
	gc->state.point.SizeRequested = 1;
	gc->state.point.SmoothSize = 1.0;
	gc->state.point.AliasedSize = 1;
	
	
	/* Fragment Ops */
	gc->state.alpha.TestEnabled = GL_FALSE;
	gc->state.alpha.TestFunction = GL_ALWAYS;
	gc->state.alpha.TestValue = 0.0;
	gc->state.depth.TestEnabled = GL_FALSE;
	gc->state.depth.TestFunction = GL_LESS;
	gc->state.depth.ClearValue = 1.0;
	gc->state.depth.WriteEnabled = GL_TRUE;
	gc->state.color.RedWriteEnabled = GL_TRUE;
	gc->state.color.GreenWriteEnabled = GL_TRUE;
	gc->state.color.BlueWriteEnabled = GL_TRUE;
	gc->state.color.AlphaWriteEnabled = GL_TRUE;
	gc->state.color.ClearValue.R=0;	
	gc->state.color.ClearValue.G=0;	
	gc->state.color.ClearValue.B=0;	
	gc->state.color.ClearValue.A=0;	
	gc->state.stencil.WriteMask = 0xff;
	gc->state.color.BlendEnabled = GL_FALSE;
	gc->state.color.BlendSrcFunction = GL_ONE;
	gc->state.color.BlendDestFunction = GL_ZERO;
	gc->state.color.LogicOp = GL_COPY;
	
	/* Stencil */
	gc->state.stencil.TestEnabled = GL_FALSE;
	gc->state.stencil.ClearValue = 0;
	gc->state.stencil.WriteMask = 0xffffffff;
	gc->state.stencil.Function = GL_ALWAYS;
	gc->state.stencil.Refrence = 0;
	gc->state.stencil.FunctionMask = 0xffffffff;
	gc->state.stencil.FailOp = GL_KEEP;
	gc->state.stencil.DepthFailOp = GL_KEEP;
	gc->state.stencil.DepthPassOp = GL_KEEP;
	
	/* Transform */
//	gc->state.transformClipY0 = 0;
//	gc->state.transformClipY1 = h;
//	gc->state.transformClipX0 = 0;
//	gc->state.transformClipX1 = w;
	
	/* Texture */
	for( ct=0; ct<4; ct++ )
	{
		gc->state.texture.SelectedUnit = 0;
		gc->state.texture.Enabled1D[0] = GL_FALSE;
		gc->state.texture.Enabled2D[0] = GL_FALSE;
		gc->state.texture.Enabled3D[0] = GL_FALSE;
		gc->state.texture.EnvMode[0] = GL_MODULATE;
		gc->state.texture.EnvColor[0].R=0;	
		gc->state.texture.EnvColor[0].G=0;	
		gc->state.texture.EnvColor[0].B=0;	
		gc->state.texture.EnvColor[0].A=0;
	}
	gc->texture.ActiveUnits = 1;
	
	gc->state.scissor.Enabled = GL_FALSE;
	gc->state.scissor.X = 0;
	gc->state.scissor.Y = 0;
	gc->state.scissor.Width = 0;
	gc->state.scissor.Height = 0;
	
	gc->state.hint.PerspectiveCorrection = GL_DONT_CARE;
	gc->state.hint.PointSmooth = GL_DONT_CARE;
	gc->state.hint.LineSmooth = GL_DONT_CARE;
	gc->state.hint.PolygonSmooth = GL_DONT_CARE;
	gc->state.hint.Fog = GL_DONT_CARE;
	
	gc->state.accum.ClearColor.R = 0;
	gc->state.accum.ClearColor.G = 0;
	gc->state.accum.ClearColor.B = 0;
	gc->state.accum.ClearColor.A = 0;

	gc->state.current.Normal.X = 0.0;
	gc->state.current.Normal.Y = 0.0;
	gc->state.current.Normal.Z = 1.0;
	gc->state.current.UserColor.R = gc->buffer.current->ColorScale.R;
	gc->state.current.UserColor.G = gc->buffer.current->ColorScale.G;
	gc->state.current.UserColor.B = gc->buffer.current->ColorScale.B;
	gc->state.current.UserColor.A = gc->buffer.current->ColorScale.A;
	gc->state.current.Color.R = gc->state.current.UserColor.R;
	gc->state.current.Color.G = gc->state.current.UserColor.G;
	gc->state.current.Color.B = gc->state.current.UserColor.B;
	gc->state.current.Color.A = gc->state.current.UserColor.A;

	gc->state.current.Texture1.X = 0.0;	
	gc->state.current.Texture1.Y = 0.0;	
	gc->state.current.Texture1.Z = 0.0;	
	gc->state.current.Texture1.W = 1.0;	
	gc->state.current.Texture2.X = 0.0;	
	gc->state.current.Texture2.Y = 0.0;	
	gc->state.current.Texture2.Z = 0.0;	
	gc->state.current.Texture2.W = 1.0;	
	gc->state.current.Texture3.X = 0.0;	
	gc->state.current.Texture3.Y = 0.0;	
	gc->state.current.Texture3.Z = 0.0;	
	gc->state.current.Texture3.W = 1.0;	
	gc->state.current.Texture4.X = 0.0;	
	gc->state.current.Texture4.Y = 0.0;	
	gc->state.current.Texture4.Z = 0.0;	
	gc->state.current.Texture4.W = 1.0;
	
	/* Initialize pointer to procedure table */
	__glSetToImmedTable( gc );

	gc->info.textureMaxS = (1 << (__GL_MAX_MIPMAP_LEVEL - 1));
	gc->info.textureMaxT = (1 << (__GL_MAX_MIPMAP_LEVEL - 1));
	gc->info.textureMaxR = (1 << (__GL_MAX_MIPMAP_LEVEL - 1));

	gc->info.debugLock = getDebugValue( "BGL_DEBUG_LOCK" );
	gc->info.debugOther = getDebugValue( "BGL_DEBUG_OTHER" );
	gc->info.debugDevices = getDebugValue( "BGL_DEBUG_DEVICES" );
	gc->info.debugDriver = getDebugValue( "BGL_DEBUG_DRIVER" );
	gc->info.debugModes = getDebugValue( "BGL_DEBUG_MODES" );
	gc->info.debugDisableDCLock = getDebugValue( "BGL_DEBUG_DISABLE_DC_LOCK" );
	gc->info.debugDisableTL = getDebugValue( "BGL_DEBUG_DISABLE_TL" );
	
	gc->state.opt.UseXP = getDebugValue( "BGL_USE_XP" );


	/* Now init the machines */
	gc->buffer.current->bitsRed = 8;
	gc->buffer.current->bitsGreen = 8;
	gc->buffer.current->bitsBlue = 8;
	gc->buffer.current->bitsAlpha = 8;
	gc->buffer.current->bitsDepth = 32;
	gc->buffer.current->bitsStencil = 8;
	gc->buffer.current->bitsAccumRed = 32;
	gc->buffer.current->bitsAccumGreen = 32;
	gc->buffer.current->bitsAccumBlue = 32;
	gc->buffer.current->bitsAccumAlpha = 32;
	
	if ( getDebugValue( "BGL_SHOW_SOFTWARE" ) )
		gc->software.PathColorEnable = GL_TRUE;
	else
		gc->software.PathColorEnable = GL_FALSE;

	if( getDebugValue( "BGL_NO_SOFTWARE" ) )
		gc->software.PathDisable = GL_TRUE;
	else
		gc->software.PathDisable = GL_FALSE;

	gc->methods.ec1 = __glDoEvalCoord1;
	gc->methods.ec2 = __glDoEvalCoord2;
	gc->methods.bitmap = __glDrawBitmap;

	gc->valid.All =1;
	gc->valid.ModelMatrix =1;
	gc->valid.ProjectionMatrix =1;
	gc->valid.VertexProcs =1;
	gc->valid.LightData =1;
	gc->valid.PrimProcessor = 1;
	gc->valid.VapiProcessor = 1;
	
	gc->valid.ModelMatrixType =-1;
	gc->valid.ProjectionMatrixType =-1;
	gc->valid.MVPMatrixType =-1;


	gc->primitive.End = (void (*)(__glContext *)) __glNop;
	gc->methods.error = (void (*)(__glContext *, GLenum)) __glNop;

//	gc->methods.copyPixels = __glDoCopyPixels;
//	gc->methods.drawPixels = __glDoDrawPixels;
	gc->methods.readPixels = __glDoReadPixels;
	gc->isRendering = 1;

	__glInitAttributeState (gc);
	__glInitVertexState (gc);
	__glInitLightState (gc);
	__glInitTextureState (gc);
	__glInitTransformState (gc);
	__glInitPolygonState (gc);
	__glInitPixelState (gc);
	__glInitEvaluatorState (gc);
	__glInitRasterState (gc);
	__glInitFeedback (gc);
	__glInitSelect (gc);

	/* Initialize extensions */
	__glInitVertexArrayState (gc);
	__glPickAllProcs(gc);

//	converterInit( &gc->scanConverter );

	gc->procs.callback_ProcChange = rasPickTriangleProcs;
	gc->procs.callback_GetOffsetBias = rasGetPolyOffset;
	gc->procs.callback_tvl = tlCallback;
	gc->procs.callback_isInBegin = isInBegin;

	gc->immedTable = __glImmedTable;


	gc->transform.maxWindowDimension = __GL_MAX_WINDOW_WIDTH;
	gc->list.shared = __glCreateListSharedState (gc);

	gc->flingerContext = cvCreateContext();
	if( !gc->flingerContext )
	{
		//HACK handle error 
	}

	rasPickTriangleProcs( gc );

//	FrameResized (rect.Width (), rect.Height ());
//	bglSetScanlineCallback (&scanlineHandler, this);
}


void __glInitAttributeState (__glContext * gc)
{
	gc->attributes.stackPointer = &gc->attributes.stack[0];
	gc->attributes.clientStackPointer = &gc->attributes.clientStack[0];
}

/*
   ** Initialize frame buffer portion of graphics context
 */
void __glInitRasterState (__glContext * gc)
{
	//__glstate *fs = &gc->state.raster;

	/*
	   ** Initialize user controllable state
	 */
	gc->renderMode = GL_RENDER;
	if (gc->buffer.current->ColorBackEnabled)
	{
		gc->state.drawBuffer = GL_BACK;
	}
	else
	{
		gc->state.drawBuffer = GL_FRONT;
	}
	gc->state.drawBufferReturn = gc->state.drawBuffer;
}

/*
   ** Initialize polygon portion of graphics context
 */
void __glInitPolygonState (__glContext * gc)
{
	gc->stencilBufferBits = 8;
	gc->primitive.EdgeTag = 1;
	gc->primitive.CullAndMask = 0x00;
	gc->primitive.CullXorMask = 0x01;
	gc->primitive.FacingXorMask = 0x00;
}

/************************************************************************/

/*
   ** Free any attribute state left on the stack.  Stop at the first
   ** zero in the array.
 */
void __glFreeAttributeState (__glContext * gc)
{
	__glAttribute *sp, **spp;

	for (spp = &gc->attributes.stack[0];
		 spp < &gc->attributes.stack[__GL_ATTRIB_STACK_DEPTH]; spp++)
	{
		sp = *spp;
		if ( sp )
		{
			FREE(sp);
		}
		else
			break;
	}
}

void __glFreeClientAttributeState (__glContext * gc)
{
	__glClientAttribute *sp, **spp;

	for (spp = &gc->attributes.clientStack[0];
		 spp < &gc->attributes.clientStack[__GL_CLIENT_ATTRIB_STACK_DEPTH];
		 spp++)
	{
		sp = *spp;
		if (sp)
		{
			FREE(sp);
		}
		else
			break;
	}
}

void __glSetRendererString( __glContext *gc )
{
	if( gc->procs.tlvBegin )
	{
		gc->info.renderer = gc->info.comboStrings[2];
	}
	else
	{
		if( gc->procs.triangleFillFrontUnordered == rasDrawTriangleFillFrontUnordered )
		{
			gc->info.renderer = gc->info.comboStrings[1];
		}
		else
		{
			gc->info.renderer = gc->info.comboStrings[0];
		}
	}
}

void __glSetStrings( __glContext *gc )
{
	char buf[1024];
	
	/* Setup generic values for get strings */
	gc->info.vendor = (GLubyte *) "BE";
	gc->info.version = (GLubyte *) "1.1.2";
	gc->info.comboStrings[0] = (GLubyte *) "R5.0 Software Software";

	if( gc->info.hwRenderer )
	{
		sprintf( buf, "R5.0 Software %s", gc->info.hwRenderer );
		gc->info.comboStrings[1] = (char *)MALLOC( strlen(buf)+1 );
		strcpy( gc->info.comboStrings[1], buf );
		if( gc->info.hwGeometry )
		{
			sprintf( buf, "R5.0 %s %s", gc->info.hwGeometry, gc->info.hwRenderer );
			gc->info.comboStrings[2] = (char *)MALLOC( strlen(buf)+1 );
			strcpy( gc->info.comboStrings[2], buf );
		}
		else
		{
			gc->info.comboStrings[2] = gc->info.comboStrings[1];
		}
	}
	else
	{
		gc->info.comboStrings[1] = gc->info.comboStrings[0];
	}

	strcpy( buf, "GL_EXT_ABGR" );
	if( gc->texture.ActiveUnits > 1 )
		strcat( buf, " GL_ARB_multitexture" );
	
	gc->info.extensions = (char *)MALLOC( strlen(buf) +1 );
	strcpy( gc->info.extensions, buf );

//	gc->info.extensions = (GLubyte *) "GL_EXT_abgr";		/* alphabetical order */
//		" GL_EXT_blend_color"
//		" GL_EXT_blend_minmax"
//		" GL_EXT_blend_subtract"
//		" GL_EXT_import_context";

	__glSetRendererString( gc );
}

void __glFreeStrings( __glContext *gc )
{
	if( gc->info.comboStrings[2] != gc->info.comboStrings[1] )
		FREE( gc->info.comboStrings[2] );
	if( gc->info.comboStrings[1] != gc->info.comboStrings[0] )
		FREE( gc->info.comboStrings[1] );
	FREE( gc->info.extensions );
}

/*
   ** Destroy a context.  If it's the current context then the
   ** current context is set to GL_NULL.
 */
void __glDestroyContext (__glContext * gc)
{
#if DEBUG_TEXT
	printf ("***** __glDestroyContext \n");
#endif
	/*
	   ** Unlink context from list of created context's
	 */
//	if (gc == __gl)
//		__gl = 0;

	/*
	   ** Free other malloc'd data associated with the context
	 */
	__glFreeEvaluatorState (gc);
	__glFreeListState (gc);
	__glFreePixelState (gc);
//	__glFreeBuffers (gc);
	__glFreeAttributeState (gc);
	__glFreeClientAttributeState (gc);
//    __glFreeTextureState(gc);
	__glFreeStrings(gc);

	FREE(gc);
}

void __glSetError (__glContext * gc, GLenum code)
{
	if (!gc->error)
		gc->error = code;
	(*gc->methods.error) (gc, code);
}




