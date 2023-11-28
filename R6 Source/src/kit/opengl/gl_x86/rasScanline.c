#include <opengl/bitflinger.h>
#include "context.h"
#include "rasTriangle.h"
#include "rasScanline.h"
#include "rasBuffers.h"
#include "mathLib.h"
#include <math.h>

#define __GL_FRAC(f)    ((f) - floor(f))

_STATIC_ GLboolean _glCompFunc( GLenum func, GLfloat ref, GLfloat t );
_STATIC_ void rasScanlineRasterFlat( __glContext *gc, const __glFragment *start, GLint w );
inline void rasScanlineRasterShade( __glContext *gc, const __glFragment *start, GLint w );
_STATIC_ void rasScanlineDepth( __glContext *gc, GLint x1, GLint w, GLfloat z, GLfloat dz );
_STATIC_ GLubyte __rasStencilOp( GLenum op, GLubyte buffer, GLubyte ref );
_STATIC_ void rasScanlineStencil( __glContext *gc, GLint x1, GLint w );
_STATIC_ void rasScanlineStencilAndDepth( __glContext *gc, GLint x1, GLint w, GLfloat z, GLfloat dz );
_STATIC_ void rasScanlinePolyStipple( __glContext *gc, GLint x1, GLint w );
_STATIC_ void rasScanlineFog( __glContext *gc, const __glFragment *start, GLint w );
_STATIC_ void rasScanlineRasterTexture_2D( __glContext *gc, const __glFragment *start, GLint w );
_STATIC_ void rasScanlineAlphaTest( __glContext *gc, GLint x1, GLint w );
_STATIC_ void rasScanlineBlend( __glContext *gc, GLint pos, GLint w );
_STATIC_ void rasScanlineMask( __glContext *gc, GLint x1, GLint x2 );

#if __PROCESSOR_P6__
extern void scanlineDepthAlways_NoWrite_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthLess_NoWrite_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthLequal_NoWrite_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthEqual_NoWrite_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthGequal_NoWrite_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthGreater_NoWrite_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthNotequal_NoWrite_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );

extern void scanlineDepthAlways_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthLess_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthLequal_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthEqual_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthGequal_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthGreater_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
extern void scanlineDepthNotequal_CMOV( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
#endif

_STATIC_ void scanlineDepthRound( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthNeverNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthAlwaysWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthAlwaysNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthLessWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthLessNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthLequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthLequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthEqualWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthEqualNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthGequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthGequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthGreaterWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthGreaterNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthNotequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
_STATIC_ void scanlineDepthNotequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );

#if __PROCESSOR_P5__
#define SCANLINE_DEPTH_WRITE( func ) \
	while( w > 0 ) \
	{ \
		w--; \
		if( *valid ) \
		{ \
			if ( z1 func (*zbuf) ) \
			{ \
				*zbuf = z1; \
			} \
			else \
				*valid = 0; \
		} \
		zbuf++; \
		valid++; \
		z1 += dz; \
		__asm__ __volatile__ ( "":::"st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)" ); \
	}
#else
#define SCANLINE_DEPTH_WRITE( func ) \
	while( w > 0 ) \
	{ \
		w--; \
		if( *valid ) \
		{ \
			if ( z1 func (*zbuf) ) \
			{ \
				*zbuf = z1; \
			} \
			else \
				*valid = 0; \
		} \
		zbuf++; \
		valid++; \
		z1 += dz; \
	}
#endif

#if __PROCESSOR_P5__
#define SCANLINE_DEPTH_NO_WRITE( func ) \
	while( w > 0 ) \
	{ \
		w--; \
		if ( !(z1 func (*zbuf)) ) \
			*valid = 0; \
		zbuf++; \
		valid++; \
		z1 += dz; \
		__asm__ __volatile__ ( "":::"st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)" ); \
	}
#else
#define SCANLINE_DEPTH_NO_WRITE( func ) \
	while( w > 0 ) \
	{ \
		w--; \
		if ( !(z1 func (*zbuf)) ) \
			*valid = 0; \
		zbuf++; \
		valid++; \
		z1 += dz; \
	}
#endif

_STATIC_ void scanlineDepthLessWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( < )
}
_STATIC_ void scanlineDepthLessNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( < )
}
_STATIC_ void scanlineDepthLequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( <= )
}
_STATIC_ void scanlineDepthLequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( <= )
}
_STATIC_ void scanlineDepthEqualWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( == )
}
_STATIC_ void scanlineDepthEqualNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( == )
}
_STATIC_ void scanlineDepthGequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( >= )
}
_STATIC_ void scanlineDepthGequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( >= )
}
_STATIC_ void scanlineDepthGreaterWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( > )
}
_STATIC_ void scanlineDepthGreaterNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( > )
}
_STATIC_ void scanlineDepthNotequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( != )
}
_STATIC_ void scanlineDepthNotequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( != )
}

extern void scanline_makeTransform( __glContext *gc );
extern GLuint calcScanlineNeeds( __glContext *gc );


_STATIC_ void validateSoftScanProcs( __glContext *gc )
{
#if 1
	gc->softScanProcs.fastScanline = GL_TRUE;
	gc->softScanProcs.valid = GL_TRUE;
	
//	gc->state.cpuid.hasCMOV = 0;
	
	if( gc->procs.roundZValue )
	{
		gc->softScanProcs.scanlineDepth = scanlineDepthRound;
	}
	else
	{
		switch( gc->state.depth.TestFunction )
		{
			case GL_NEVER:
				gc->softScanProcs.scanlineDepth = scanlineDepthNeverNoWrite;
				break;
			case GL_ALWAYS:
				if( gc->state.depth.WriteEnabled )
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthAlways_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthAlwaysWrite;
#endif
				}
				else
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthAlways_NoWrite_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthAlwaysNoWrite;
#endif
				}
				break;
			case GL_LESS:
				if( gc->state.depth.WriteEnabled )
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthLess_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthLessWrite;
#endif
				}
				else
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthLess_NoWrite_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthLessNoWrite;
#endif
				}
				break;
			case GL_LEQUAL:
				if( gc->state.depth.WriteEnabled )
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthLequal_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthLequalWrite;
#endif
				}
				else
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthLequal_NoWrite_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthLequalNoWrite;
#endif
				}
				break;
			case GL_EQUAL:
				if( gc->state.depth.WriteEnabled )
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthEqual_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthEqualWrite;
#endif
				}
				else
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthEqual_NoWrite_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthEqualNoWrite;
#endif
				}
				break;
			case GL_GEQUAL:
				if( gc->state.depth.WriteEnabled )
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthGequal_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthGequalWrite;
#endif
				}
				else
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthGequal_NoWrite_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthGequalNoWrite;
#endif
				}
				break;
			case GL_GREATER:
				if( gc->state.depth.WriteEnabled )
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthGreater_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthGreaterWrite;
#endif
				}
				else
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthGreater_NoWrite_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthGreaterNoWrite;
#endif
				}
				break;
			case GL_NOTEQUAL:
				if( gc->state.depth.WriteEnabled )
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthNotequal_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthNotequalWrite;
#endif
				}
				else
				{
#if __PROCESSOR_P6__
					gc->softScanProcs.scanlineDepth = scanlineDepthNotequal_NoWrite_CMOV;
#else
					gc->softScanProcs.scanlineDepth = scanlineDepthNotequalNoWrite;
#endif
				}
				break;
		}
	}

	if ( (gc->state.color.RedWriteEnabled == GL_FALSE) && 
		(gc->state.color.GreenWriteEnabled == GL_FALSE) &&
		(gc->state.color.BlueWriteEnabled == GL_FALSE) &&
		(gc->state.color.AlphaWriteEnabled == GL_FALSE) )
		gc->softScanProcs.colorWriteEnabled = GL_FALSE;
	else
		gc->softScanProcs.colorWriteEnabled = GL_TRUE;

	gc->softScanProcs.loadBits = 0;
	if ( gc->state.color.BlendEnabled )
		gc->softScanProcs.loadBits |= GL_COLOR_BUFFER_BIT;
	if( (!gc->state.color.RedWriteEnabled) ||
		(!gc->state.color.GreenWriteEnabled) ||
		(!gc->state.color.BlueWriteEnabled) ||
		(!gc->state.color.AlphaWriteEnabled) )
		gc->softScanProcs.loadBits |= GL_COLOR_BUFFER_BIT;
	if( gc->buffer.current->DepthEnabled && gc->state.depth.TestEnabled )
		gc->softScanProcs.loadBits |= GL_DEPTH_BUFFER_BIT;
	if( gc->buffer.current->StencilEnabled && gc->state.stencil.TestEnabled )
		gc->softScanProcs.loadBits |= GL_STENCIL_BUFFER_BIT;


	gc->softScanProcs.storeBits = 0;
	if ( gc->state.color.RedWriteEnabled || 
		gc->state.color.GreenWriteEnabled ||
		gc->state.color.BlueWriteEnabled ||
		gc->state.color.AlphaWriteEnabled )
		gc->softScanProcs.storeBits |= GL_COLOR_BUFFER_BIT;
	if( gc->buffer.current->DepthEnabled &&
			gc->state.depth.TestEnabled && 
			gc->state.depth.WriteEnabled )
		gc->softScanProcs.storeBits |= GL_DEPTH_BUFFER_BIT;
	if( gc->buffer.current->StencilEnabled &&
			gc->state.stencil.TestEnabled && 
			gc->state.stencil.WriteMask )
		gc->softScanProcs.storeBits |= GL_STENCIL_BUFFER_BIT;	
		
	if( (!gc->state.color.RedWriteEnabled) ||
		(!gc->state.color.GreenWriteEnabled) ||
		(!gc->state.color.BlueWriteEnabled) ||
		(!gc->state.color.AlphaWriteEnabled) )
		gc->softScanProcs.maskEnabled = GL_TRUE;
	else
		gc->softScanProcs.maskEnabled = GL_FALSE;
		
	if( gc->texture.Enabled[0] || gc->texture.Enabled[1] ||
		gc->state.stencil.TestEnabled ||
		gc->state.fog.Enabled ||
		gc->state.color.BlendEnabled )
		gc->softScanProcs.fastScanline = 0;
		
#if 0//__PROCESSOR_KATMAI__
	if( gc->softScanProcs.fastScanline )
	{
		GLuint needs = calcScanlineNeeds( gc );
		if( needs != gc->softScanProcs.processorNeeds )
		{
			gc->softScanProcs.processorNeeds = needs;
			scanline_makeTransform( gc );
		}
		gc->softScanProcs.processorFunc = gc->softScanProcs.processorFuncData;
	}
	else
	{
		gc->softScanProcs.processorFunc = rasProcessScanline;
	}
#else
//	gc->softScanProcs.processorFunc = rasProcessScanline;
#endif
#endif
}

_STATIC_ GLboolean _glCompFunc( GLenum func, GLfloat ref, GLfloat t )
{
	GLboolean pass = GL_FALSE;
	switch( func )
	{
		case GL_NEVER:
			pass = GL_FALSE;
			break;
		case GL_ALWAYS:
			pass = GL_TRUE;
			break;
		case GL_LESS:
			pass = (t < ref);
			break;
		case GL_LEQUAL:
			pass = (t <= ref);
			break;
		case GL_EQUAL:
			pass = (t == ref);
			break;
		case GL_GEQUAL:
			pass = (t >= ref);
			break;
		case GL_GREATER:
			pass = (t > ref);
			break;
		case GL_NOTEQUAL:
			pass = (t != ref);
			break;
	}
	return pass;
}

_STATIC_ void rasScanlineRasterFlat( __glContext *gc, const __glFragment *start, GLint w )
{
	rasPixel32 v;
	GLint pos = start->x;
	GLint *fb;

	v.rgba.r = start->color.R;
	v.rgba.g = start->color.G;
	v.rgba.b = start->color.B;
	v.rgba.a = start->color.A;
	
	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	fb = (GLint *)&gc->buffer.current->ScanlineFB[pos];
	while( w > 0 )
	{
		w--;
		*fb = v.value;
		fb++;
	}
	//memset( &sb->valid[pos], 1, w );
}


inline void rasScanlineRasterShade( __glContext *gc, const __glFragment *start, GLint w )
{
	GLint temp;
	GLint pos = start->x;
	GLuint *fb;
	rasPixel32 color;
	GLint r,g,b,a;
	GLint dr,dg,db,da;
	GLint wbak;
	
	dr = gc->softScanProcs.shade.drdx;
	dg = gc->softScanProcs.shade.dgdx;
	db = gc->softScanProcs.shade.dbdx;
	da = gc->softScanProcs.shade.dadx;

	wbak = w;
	r = start->color.R;
	g = start->color.G;
	b = start->color.B;
	a = start->color.A;

	if( pos < 0 )
	{
		w += pos;
		r -= dr * pos;
		g -= db * pos;
		b -= dg * pos;
		a -= da * pos;
		pos = 0;
	}
	
	fb = (GLuint *)&gc->buffer.current->ScanlineFB[pos];
	while( w > 0 )
	{
		if( r < 0 )
			temp = 0;
		else
		{
			temp = r>>16;
			if( temp > 255 )
				temp = 255;
		}
		color.rgba.r = temp;
		
		if( g < 0 )
			temp = 0;
		else
		{
			temp = g>>16;
			if( temp > 255 )
				temp = 255;
		}
		color.rgba.g = temp;

		if( b < 0 )
			temp = 0;
		else
		{
			temp = b>>16;
			if( temp > 255 )
				temp = 255;
		}
		color.rgba.b = temp;

		if( a < 0 )
			temp = 0;
		else
		{
			temp = a>>16;
			if( temp > 255 )
				temp = 255;
		}
		color.rgba.a = temp;

		w--;
		*fb = color.value;	/* Set Color */
		fb++;
		
		r += dr;
		g += dg;
		b += db;
		a += da;
	}
	w = wbak;
}

_STATIC_ void scanlineDepthRound( GLubyte *vb, GLfloat *db, GLfloat z, GLfloat dz, GLint w )
{
}

_STATIC_ void scanlineDepthNeverNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	while( w > 0 )
	{
		w--;
		*valid = 0;	/* All tests fail */
		valid++;
	}
}

_STATIC_ void scanlineDepthAlwaysWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	while( w > 0 )
	{
		w--;
		*zbuf = z1;
		zbuf++;
		z1 += dz;
	}
}

_STATIC_ void scanlineDepthAlwaysNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
}

_STATIC_ void rasScanlineDepth( __glContext *gc, GLint x1, GLint w, GLfloat z, GLfloat dz )
{
	GLint pos = x1;
	GLfloat *db;
	GLubyte *vb;		//valid buffer
	GLfloat z2;

	if( pos < 0 )
	{
		w += pos;
		z -= dz * pos;
		pos = 0;
	}
	db = &gc->buffer.current->ScanlineZ[pos];
	vb = (GLubyte *)&gc->buffer.current->ScanlineV[pos];
	
	if( !gc->procs.roundZValue )
		(*gc->softScanProcs.scanlineDepth)( vb, db, z, dz, w );
	else	
	{
		if( gc->state.depth.WriteEnabled )
		{
			while( w > 0 )
			{
				w--;
				z2 = (*gc->procs.roundZValue)(gc,z);
				if ( _glCompFunc( gc->state.depth.TestFunction, *db, z2 ) )
					*db = z;
				else
					*vb = 0;
				db++;
				vb++;
				z += dz;
			}
		}
		else
		{
			while( w > 0 )
			{
				w--;
				z2 = (*gc->procs.roundZValue)(gc,z);
				if ( !_glCompFunc( gc->state.depth.TestFunction, *db, z2 ) )
					*vb = 0;
				db++;
				vb++;
				z += dz;
			}
		}
	}
}

_STATIC_ GLubyte __rasStencilOp( GLenum op, GLubyte buffer, GLubyte ref )
{
	switch( op )
	{
		case GL_KEEP:
			return buffer;
		case GL_ZERO:
			return 0;
		case GL_REPLACE:
			return ref;
		case GL_INCR:
			buffer++;
			if( buffer == 0 )
				buffer = 0xff;
			return buffer;
		case GL_DECR:
			buffer--;
			if( buffer == 0xff )
				buffer = 0x0;
			return buffer;
		case GL_INVERT:
			return buffer ^ 0xff;
	}
	return 0;
}

_STATIC_ void rasScanlineStencil( __glContext *gc, GLint x1, GLint w )
{
	GLint pos = x1;
	GLubyte *s;
	GLubyte *vb;		//valid buffer
	GLubyte compMask = gc->state.stencil.FunctionMask;
	GLubyte ref = gc->state.stencil.Refrence & compMask;
	GLubyte writeMask = gc->state.stencil.WriteMask;
	
	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	
	s = &gc->buffer.current->ScanlineS[pos];
	vb = (GLubyte *)&gc->buffer.current->ScanlineV[pos];
	
	while( w > 0 )
	{
		w--;
		if( *vb )
		{
			if ( _glCompFunc( gc->state.stencil.Function, ref, *s & compMask ) )
			{
				*s = __rasStencilOp( gc->state.stencil.DepthPassOp, *s, gc->state.stencil.Refrence ) & writeMask;
			}
			else
			{
				*s = __rasStencilOp( gc->state.stencil.FailOp, *s, gc->state.stencil.Refrence ) & writeMask;
				*vb=0;
			}
		}
		s++;
		vb++;
	}
	
}

_STATIC_ void rasScanlineStencilAndDepth( __glContext *gc, GLint x1, GLint w, GLfloat z, GLfloat dz )
{
	GLfloat z2;
	GLint pos = x1;
	GLfloat *db;
	GLubyte *s;
	GLubyte *vb;		//valid buffer
	GLubyte compMask = gc->state.stencil.FunctionMask;
	GLubyte ref = gc->state.stencil.Refrence & compMask;
	GLubyte writeMask = gc->state.stencil.WriteMask;
	GLubyte t;

	if( pos < 0 )
	{
		w += pos;
		pos = -pos;
		z += dz * pos;
		pos = 0;
	}
	s = &gc->buffer.current->ScanlineS[pos];
	db = &gc->buffer.current->ScanlineZ[pos];
	vb = (GLubyte *)&gc->buffer.current->ScanlineV[pos];
	
	while( w > 0 )
	{
		w--;
		t = (*s) & compMask;
		if( gc->procs.roundZValue )
			z2 = (*gc->procs.roundZValue)(gc,z);
		else
			z2 = z;

		if( *vb )
		{
			if ( _glCompFunc( gc->state.stencil.Function, ref, t ) )
			{
				if ( _glCompFunc( gc->state.depth.TestFunction, *db, z2 ) )
				{
					if( gc->state.depth.WriteEnabled )
						*db = z;
					*s = (__rasStencilOp( gc->state.stencil.DepthPassOp, *s, gc->state.stencil.Refrence ) & writeMask) |
						( (*s) & (~writeMask));
				}
				else
				{
					*vb = 0;
					*s = (__rasStencilOp( gc->state.stencil.DepthFailOp, *s, gc->state.stencil.Refrence ) & writeMask) |
						( (*s) & (~writeMask));
				}
			}
			else
			{
				*vb = 0;
				*s = (__rasStencilOp( gc->state.stencil.FailOp, *s, gc->state.stencil.Refrence ) & writeMask) |
						( (*s) & (~writeMask));
			}
		}
				
		s++;
		db++;
		vb++;
		z+=dz;
	}
}


_STATIC_ void rasScanlinePolyStipple( __glContext *gc, GLint x1, GLint w )
{
	GLint pos = x1;
	GLubyte *vb;		//valid buffer
	GLuint stipple = ((GLuint*)gc->state.polyStippleMask)[(gc->buffer.current->ScanlineY) & 0x1f];
	GLubyte *s = (GLubyte *)&stipple;

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	vb = (GLubyte *)&gc->buffer.current->ScanlineV[pos];

	while( w > 0 )
	{
		w--;
		if( !(s[(pos&0x18) >>3] & (0x80 >> (pos&0x7))) )
			*vb = 0;
		vb++;
		pos++;
	}
}

_STATIC_ void rasScanlineFog( __glContext *gc, const __glFragment *start, GLint w )
{
	GLfloat fog = 1;
	GLfloat oneMinusFog, density, fogStart, fogEnd;
	rasPixel32 fogColor;
	GLint pos = start->x;	
	GLfloat e = 2.71828182846;
	GLfloat f = start->f;
	GLfloat df = gc->softScanProcs.shade.dfdx;
	rasPixel32 *pixel;
	GLfloat eyeZ;

	fogColor.rgba.r = gc->state.fog.Color.R * 255;
	fogColor.rgba.g = gc->state.fog.Color.G * 255;
	fogColor.rgba.b = gc->state.fog.Color.B * 255;
	fogColor.rgba.a = gc->state.fog.Color.A * 255;


	if (gc->state.hint.Fog == GL_NICEST) 
	{
		density = gc->state.fog.Density;
		fogStart = gc->state.fog.Start;
		fogEnd = gc->state.fog.End;
		
		if( pos < 0 )
		{
			w += pos;
			pos = -pos;
			f += df * pos;
			pos = 0;
		}
		pixel = (rasPixel32 *)&gc->buffer.current->ScanlineFB[pos];

		while( w > 0 )
		{
	
			eyeZ = f;
			if (eyeZ < 0) 
				eyeZ = -eyeZ;
		
			switch (gc->state.fog.Mode) 
			{
				case GL_EXP:
					fog = pow(e, -density * eyeZ);
					break;
					
				case GL_EXP2:
					fog = pow(e, -(density * eyeZ * density * eyeZ));
					break;
		
				case GL_LINEAR:
					if (fogEnd != fogStart) 
					{
						fog = (fogEnd - eyeZ) / (fogEnd - fogStart);
					} 
					else 
					{
						// Use zero as the undefined value
						fog = 0;
					}
				break;
			}
	
			/*
			** clamp fog value
			*/
			if (fog < 0)
				fog = 0;
			else
				if (fog > 1)
					fog = 1;
			oneMinusFog = 1 - fog;
		
			/*
			** Blend incoming color against the fog color
			*/
			(*pixel).rgba.r = fog * (*pixel).rgba.r + oneMinusFog * fogColor.rgba.r;
			(*pixel).rgba.g = fog * (*pixel).rgba.g + oneMinusFog * fogColor.rgba.g;
			(*pixel).rgba.b = fog * (*pixel).rgba.b + oneMinusFog * fogColor.rgba.b;
	
			pixel++;
			w--;
			f += df;
		}
		
	}
	else
	{
		if( pos < 0 )
		{
			w += pos;
			pos = -pos;
			f += df * pos;
			pos = 0;
		}
		pixel = (rasPixel32 *)&gc->buffer.current->ScanlineFB[pos];
		while( w > 0 )
		{
			fog = f;
		
			/*
			** clamp fog value
			*/
			if (fog < 0)
				fog = 0;
			else
				if (fog > 1)
					fog = 1;
			oneMinusFog = 1 - fog;
		
			/*
			** Blend incoming color against the fog color
			*/
			(*pixel).rgba.r = (fog * (*pixel).rgba.r) + (oneMinusFog * fogColor.rgba.r);
			(*pixel).rgba.g = (fog * (*pixel).rgba.g) + (oneMinusFog * fogColor.rgba.g);
			(*pixel).rgba.b = (fog * (*pixel).rgba.b) + (oneMinusFog * fogColor.rgba.b);
	
			pixel++;
			w--;
			f += df;
		}
	}
		
}

void extractTexel( __glContext *gc, __glTexture *tex, GLint level, GLint s, GLint t, __glColor *color )
{
	static cv_extractor ext;
	if( !gc->flingerCurrent || (gc->flingerLOD != level))
	{
		cvSetType( gc->flingerContext, B_BIT_IN, tex->level[level].internalType, tex->level[level].internalFormat, 0, 0 );
		cvSetType( gc->flingerContext, B_BIT_OUT, GL_FLOAT, GL_RGBA, gc->state.pixel.unpackModes.SwapEndian, gc->state.pixel.unpackModes.LsbFirst );
		ext = cvPickExtractor( gc->flingerContext, tex->level[level].width, tex->level[level].height, 0, 0, 0 );
		gc->flingerCurrent = 1;
		gc->flingerLOD = level;
	}

	if( s < 0 )
		s = 0;
	if( s > tex->level[level].width )
		s = tex->level[level].width;
	if( t < 0 )
		t = 0;
	if( t > tex->level[level].height )
		t = tex->level[level].height;

	(ext) (gc->flingerContext, s, t, &color->R, tex->level[level].data );
	
	//printf( "%f %f %f %f \n", color->R, color->G, color->B, color->A );
}

void extractNearestTexel( __glContext *gc, __glTexture *tex, GLint level, GLfloat s, GLfloat t, __glColor *p )
{
	GLint row, col;
	__glFloat w2f, h2f;
	
	/* Find texel column address */
	w2f = tex->level[level].width2;
	if (tex->sWrapMode == GL_REPEAT)
	{
		col = __GL_FRAC(s) * w2f;
	}
	else
	{
		GLint w2 = tex->level[level].width2;
		col = s * w2f;
		if (col < 0)
			col = 0;
		else
		{
			if (col >= w2)
				col = w2 - 1;
		}
	}
	
	/* Find texel row address */
	h2f = tex->level[level].height2;
	if (tex->tWrapMode == GL_REPEAT)
	{
		row = __GL_FRAC(t) * h2f;
	}
	else
	{
		GLint h2 = tex->level[level].height2;
		row = t * h2f;
		if (row < 0)
			row = 0;
		else
		{
			if (row >= h2)
				row = h2 - 1;
		}
	}
	
	/* Lookup texel */
	extractTexel( gc, tex, level, col, row, p );
}

void extractLinearTexel( __glContext *gc, __glTexture *tex, GLint level, GLfloat s, GLfloat t, __glColor *p )
{
	__glFloat u, v, alpha, beta, half;
	GLint col0, row0, col1, row1, w2f, h2f;
	__glColor t00, t01, t10, t11;
	__glFloat omalpha, ombeta, m00, m01, m10, m11;
	
	/* Find col0, col1 */
	w2f = tex->level[level].width2;
	u = s * w2f;
	half = 0.5;
	if (tex->sWrapMode == GL_REPEAT)
	{
		GLint w2mask = tex->level[level].width2 - 1;
		u -= half;
		col0 = ((GLint) floor(u)) & w2mask;
		col1 = (col0 + 1) & w2mask;
	}
	else
	{
		if (u < 0.f)
			u = 0.f;
		else
		{
			if (u > w2f)
				u = w2f;
		}
		u -= half;
		col0 = (GLint) floor(u);
		col1 = col0;// + 1;
	}
	
	/* Find row0, row1 */
	h2f = tex->level[level].height2;
	v = t * h2f;
	if (tex->tWrapMode == GL_REPEAT)
	{
		GLint h2mask = tex->level[level].height2 - 1;
		v -= half;
		row0 = ((GLint) floor(v)) & h2mask;
		row1 = (row0 + 1) & h2mask;
	}
	else
	{
		if (v < 0.f)
			v = 0.f;
		else
		{
			if (v > h2f)
				 v = h2f;
		}
		v -= half;
		row0 = (GLint) floor(v);
		row1 = row0;// + 1;
	}
	
	/* Compute alpha and beta */
	alpha = __GL_FRAC(u);
	beta = __GL_FRAC(v);
	
	/* Calculate the final texel value as a combination of the square chosen */
	extractTexel( gc, tex, level, col0, row0, &t00 );
	extractTexel( gc, tex, level, col1, row0, &t10 );
	extractTexel( gc, tex, level, col0, row1, &t01 );
	extractTexel( gc, tex, level, col1, row1, &t11 );
	
	omalpha = 1.f - alpha;
	ombeta = 1.f - beta;
	
	m00 = omalpha * ombeta;
	m10 = alpha * ombeta;
	m01 = omalpha * beta;
	m11 = alpha * beta;
	
	switch (tex->level[level].baseFormat)
	{
	case GL_LUMINANCE_ALPHA:
	case GL_RGBA:
	case GL_INTENSITY:
		p->A = m00*t00.A + m10*t10.A + m01*t01.A + m11*t11.A;
		/* FALLTHROUGH */
	case GL_RGB:
	case GL_LUMINANCE:
		p->R = m00*t00.R + m10*t10.R + m01*t01.R + m11*t11.R;
		p->G = m00*t00.G + m10*t10.G + m01*t01.G + m11*t11.G;
		p->B = m00*t00.B + m10*t10.B + m01*t01.B + m11*t11.B;
		break;
	case GL_ALPHA:
		p->A = m00*t00.A + m10*t10.A + m01*t01.A + m11*t11.A;
		break;
	}
}

void textureFragment( __glContext *gc, __glTexture *tex, GLfloat rho, GLfloat s, GLfloat t, GLfloat r, __glFragment *frag )
{
	__glColor c,c2;
	GLint mipmaped = 0;
	
	switch( tex->minFilter )
	{
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			mipmaped = 1;		
	}
	
	if( mipmaped )
	{
		GLfloat switchPoint = 0.0;
		
		if( (tex->magFilter == GL_LINEAR) &&
			((tex->minFilter == GL_NEAREST_MIPMAP_NEAREST) || (tex->minFilter == GL_NEAREST_MIPMAP_LINEAR )))
			switchPoint = 0.5;
			
		if( rho > switchPoint )
		{
			/* Min filter */
			if (rho)
			{
				/* Convert rho to lambda */
				rho = log(rho) * ((1.0 / 0.69314718055994530942) * 0.5);
			}
			else
			{
				rho = 0.f;
			}

			if( rho < 0 )
				rho = 0;
			if( rho >= tex->activeLevels )
				rho = tex->activeLevels -1;

			switch( tex->minFilter )
			{
			case GL_LINEAR_MIPMAP_LINEAR:
				extractLinearTexel( gc, tex, rho + 0.49995, s, t, &c );
				extractLinearTexel( gc, tex, rho - .5, s, t, &c2 );
				c.R = (c.R + c2.R) * 0.5;
				c.G = (c.R + c2.G) * 0.5;
				c.B = (c.R + c2.B) * 0.5;
				c.A = (c.R + c2.A) * 0.5;
				break;

			case GL_LINEAR_MIPMAP_NEAREST:
				extractLinearTexel( gc, tex, rho, s, t, &c );
				break;

			case GL_NEAREST_MIPMAP_LINEAR:
				extractNearestTexel( gc, tex, rho + 0.49995, s, t, &c );
				extractNearestTexel( gc, tex, rho - .5, s, t, &c2 );
				c.R = (c.R + c2.R) * 0.5;
				c.G = (c.R + c2.G) * 0.5;
				c.B = (c.R + c2.B) * 0.5;
				c.A = (c.R + c2.A) * 0.5;
				break;

			case GL_NEAREST_MIPMAP_NEAREST:
				extractNearestTexel( gc, tex, rho, s, t, &c );
				break;

			case GL_NEAREST:
				extractNearestTexel( gc, tex, 0, s, t, &c );
				break;
			
			case GL_LINEAR:
				extractLinearTexel( gc, tex, 0, s, t, &c );
				break;
			}
		}
		else
		{
			/* Mag filter */
			if( tex->magFilter == GL_LINEAR )
				extractLinearTexel( gc, tex, 0, s, t, &c );
			else
				extractNearestTexel( gc, tex, 0, s, t, &c );
		}
	}
	else
	{
		if( tex->magFilter == GL_LINEAR )
			extractLinearTexel( gc, tex, 0, s, t, &c );
		else
			extractNearestTexel( gc, tex, 0, s, t, &c );
	}
	
	switch( gc->state.texture.EnvMode[0] )
	{
	case GL_MODULATE:
		switch( tex->level[0].baseFormat )
		{
		case GL_RGBA:
		case GL_LUMINANCE_ALPHA:
		case GL_INTENSITY:
			frag->color.A = c.A * frag->color.A;
		case GL_RGB:
		case GL_LUMINANCE:
			frag->color.R = c.R * frag->color.R;
			frag->color.G = c.G * frag->color.G;
			frag->color.B = c.B * frag->color.B;
			break;
		case GL_ALPHA:
			frag->color.A = c.A * frag->color.A;
			break;
		}
		break;
		
	case GL_REPLACE:
		switch( tex->level[0].baseFormat )
		{
		case GL_RGBA:
		case GL_LUMINANCE_ALPHA:
		case GL_INTENSITY:
			frag->color.A = c.A * gc->buffer.current->ColorScale.A;
		case GL_RGB:
		case GL_LUMINANCE:
			frag->color.R = c.R * gc->buffer.current->ColorScale.R;
			frag->color.G = c.G * gc->buffer.current->ColorScale.G;
			frag->color.B = c.B * gc->buffer.current->ColorScale.B;
			break;
		case GL_ALPHA:
			frag->color.A = c.A * gc->buffer.current->ColorScale.A;
			break;
		}
		break;

	case GL_DECAL:
		switch( tex->level[0].baseFormat )
		{
		case GL_RGBA:
		case GL_LUMINANCE_ALPHA:
		case GL_INTENSITY:
			{
				GLfloat oma = 1 - c.A;
				frag->color.R = (oma * frag->color.R) + (c.A * c.R * gc->buffer.current->ColorScale.R);
				frag->color.G = (oma * frag->color.G) + (c.A * c.G * gc->buffer.current->ColorScale.G);
				frag->color.B = (oma * frag->color.B) + (c.A * c.B * gc->buffer.current->ColorScale.B);
			}
			break;
		case GL_RGB:
		case GL_LUMINANCE:
			frag->color.R = c.R * gc->buffer.current->ColorScale.R;
			frag->color.G = c.G * gc->buffer.current->ColorScale.G;
			frag->color.B = c.B * gc->buffer.current->ColorScale.B;
			break;
		}
		break;

	case GL_BLEND:
		switch( tex->level[0].baseFormat )
		{
		case GL_RGBA:
		case GL_LUMINANCE_ALPHA:
			frag->color.A = c.A * frag->color.A;
		case GL_RGB:
		case GL_LUMINANCE:
			frag->color.R = (1.0 - c.R) * frag->color.R + c.R * gc->state.texture.EnvColor[0].R;
			frag->color.G = (1.0 - c.G) * frag->color.G + c.G * gc->state.texture.EnvColor[0].G;
			frag->color.B = (1.0 - c.B) * frag->color.B + c.B * gc->state.texture.EnvColor[0].B;
			break;
		case GL_ALPHA:
			frag->color.A = c.A * frag->color.A;
			break;
		case GL_INTENSITY:
			frag->color.R = (1.0 - c.R) * frag->color.R + c.R * gc->state.texture.EnvColor[0].R;
			frag->color.G = (1.0 - c.G) * frag->color.G + c.G * gc->state.texture.EnvColor[0].G;
			frag->color.B = (1.0 - c.B) * frag->color.B + c.B * gc->state.texture.EnvColor[0].B;
			frag->color.A = (1.0 - c.A) * frag->color.A + c.A * gc->state.texture.EnvColor[0].A;
			break;
		}
		break;
	}		
}


void rasScanlineRasterTexture_2D( __glContext *gc, const __glFragment *start, GLint w )
{
	GLint pos = start->x;
	rasPixel32 *pixel;
	__glTexture *tex = gc->texture.Active[0];
	__glFragment frag;
	GLfloat over255, overQ;
	GLfloat s,t,qw;
	GLfloat ds,dt,dq;
	void (*func)( __glContext *, __glFragment *, GLfloat s, GLfloat t, GLfloat level );

	if( tex == 0 )
		return;

	func = (void (*)( __glContext *, __glFragment *, GLfloat s, GLfloat t, GLfloat level )) tex->textureFunc;

	frag.x = start->x;
	frag.y = start->y;
	frag.z = start->z;
	frag.color = start->color;
	frag.s = start->s;
	frag.t = start->t;
	frag.qw = start->qw;
	frag.f = start->f;

	s = start->s;
	t = start->t;
	qw = start->qw;
	ds = gc->softScanProcs.shade.dsdx;
	dt = gc->softScanProcs.shade.dtdx;
	dq = gc->softScanProcs.shade.dqwdx;
	over255 = 1.0 / 255.0;

	if( pos < 0 )
	{
		w += pos;
		s -= ds * pos;
		t -= dt * pos;
		qw -= dq * pos;
		pos = 0;
	}
	pixel = (rasPixel32 *)&gc->buffer.current->ScanlineFB[pos];
	

	while( w > 0 )
	{
		GLfloat rho = 0.f;
		
		w--;
		overQ = 1 / qw;
		frag.color.R = ((GLfloat)(*pixel).rgba.r);
		frag.color.G = ((GLfloat)(*pixel).rgba.g);
		frag.color.B = ((GLfloat)(*pixel).rgba.b);
		frag.color.A = ((GLfloat)(*pixel).rgba.a);
		
		if( 1 )
		{
			__glFloat w0, w1, p0, p1;
			__glFloat pupx, pupy, pvpx, pvpy;
			__glFloat px, py;
			
			/* Compute partial of u with respect to x */
			w0 = 1.0 / (qw - gc->softScanProcs.shade.dqwdx);
			w1 = 1.0 / (qw + gc->softScanProcs.shade.dqwdx);
			p0 = (s - gc->softScanProcs.shade.dsdx) * w0;
			p1 = (s + gc->softScanProcs.shade.dsdx) * w1;
			pupx = (p1 - p0) * tex->level[0].width2;
			
			/* Compute partial of v with repsect to y */
			p0 = (t - gc->softScanProcs.shade.dtdx) * w0;
			p1 = (t + gc->softScanProcs.shade.dtdx) * w1;
			pvpx = (p1 - p0) * tex->level[0].height2;
			
			/* Compute partial of u with respect to y */
			w0 = 1.0 / (qw - gc->softScanProcs.shade.dqwdy);
			w1 = 1.0 / (qw + gc->softScanProcs.shade.dqwdy);
			p0 = (s - gc->softScanProcs.shade.dsdy) * w0;
			p1 = (s + gc->softScanProcs.shade.dsdy) * w1;
			pupy = (p1 - p0) * tex->level[0].width2;
			
			/* Figure partial of u&v with repsect to y */
			p0 = (t - gc->softScanProcs.shade.dtdy) * w0;
			p1 = (t + gc->softScanProcs.shade.dtdy) * w1;
			pvpy = (p1 - p0) * tex->level[0].height2;
			
			/* Finally, figure sum of squares */
			px = pupx * pupx + pvpx * pvpx;
			py = pupy * pupy + pvpy * pvpy;
			
			/* Return largest value as the level of detail */
			if (px > py)
				rho = px * ((__glFloat) 0.25);
			else
				rho = py * ((__glFloat) 0.25);
		}
		
		if( gc->state.opt.Tex2[0] )
			textureFragment( gc, tex, rho, s, t, 0.f, &frag );
		else
			textureFragment( gc, tex, rho, s*overQ, t*overQ, 0.f, &frag );
//		(*func)( gc, &frag, s*overQ, t*overQ, 
//			1.0/*(*gc->methods.calcPolygonRho)(gc,shade,s,t,q)*/ );
		(*pixel).rgba.r = frag.color.R;
		(*pixel).rgba.g = frag.color.G;
		(*pixel).rgba.b = frag.color.B;
		(*pixel).rgba.a = frag.color.A;
		s += ds;
		t += dt;
		qw += dq;
		pixel++;
	}
}

void rasScanlineRasterTexture_1D( __glContext *gc, const __glFragment *start, GLint w )
{
	GLint pos = start->x;
	rasPixel32 *pixel;
	__glTexture *tex = gc->texture.Active[0];
	__glFragment frag;
	GLfloat over255, overQ;
	GLfloat s,q;
	GLfloat ds,dq;
	void (*func)( __glContext *, __glFragment *, GLfloat s, GLfloat t, GLfloat level );

	if( tex == 0 )
		return;

	func = (void (*)( __glContext *, __glFragment *, GLfloat s, GLfloat t, GLfloat level )) tex->textureFunc;

	frag.x = start->x;
	frag.y = start->y;
	frag.z = start->z;
	frag.color = start->color;
	frag.s = start->s;
	frag.t = 0;
	frag.qw = start->qw;
	frag.f = start->f;

	s = start->s;
	q = start->qw;
	ds = gc->softScanProcs.shade.dsdx;
	dq = gc->softScanProcs.shade.dqwdx;
	over255 = 1.0 / 255.0;

	if( pos < 0 )
	{
		w += pos;
		s -= ds * pos;
		q -= dq * pos;
		pos = 0;
	}
	pixel = (rasPixel32 *)&gc->buffer.current->ScanlineFB[pos];
	

	while( w > 0 )
	{
		GLfloat rho = 0.f;
		
		w--;
		overQ = 1 / q;
		frag.color.R = ((GLfloat)(*pixel).rgba.r);
		frag.color.G = ((GLfloat)(*pixel).rgba.g);
		frag.color.B = ((GLfloat)(*pixel).rgba.b);
		frag.color.A = ((GLfloat)(*pixel).rgba.a);
		
		if( gc->state.opt.Tex2[0] )
			textureFragment( gc, tex, rho, s, 0, 0.f, &frag );
		else
			textureFragment( gc, tex, rho, s*overQ, 0, 0.f, &frag );

//		(*func)( gc, &frag, s*overQ, t*overQ, 
//			1.0/*(*gc->methods.calcPolygonRho)(gc,shade,s,t,q)*/ );
		(*pixel).rgba.r = frag.color.R;
		(*pixel).rgba.g = frag.color.G;
		(*pixel).rgba.b = frag.color.B;
		(*pixel).rgba.a = frag.color.A;

		s += ds;
		q += dq;
		pixel++;
	}
}

void rasScanlineAlphaTest( __glContext *gc, GLint x1, GLint w )
{
	GLint pos = x1;
	rasPixel32 *pixel;
	GLubyte *vb;		//valid buffer
	GLfloat temp = gc->state.alpha.TestValue * 255;
	GLubyte ref;
	
	if ( temp > 255 )
		temp = 255;
	if ( temp < 0 )
		temp = 0;
		
	ref = temp;

	if( pos < 0 )
	{
		w += pos;
		pos = -pos;
		pos = 0;
	}
	pixel = (rasPixel32 *)&gc->buffer.current->ScanlineFB[pos];
	vb = &gc->buffer.current->ScanlineV[pos];
	
	switch( gc->state.alpha.TestFunction )
	{
		case GL_NEVER:
			while( w > 0 )
			{
				w--;
				*vb = 0;	/* All tests fail */
				vb++;
			}
			break;
			
		case GL_ALWAYS:		
			break;
		
		case GL_LESS:
			while( w > 0 )
			{
				w--;
				if ( ref <= (*pixel).rgba.a )
					*vb = 0;
				vb++;
				pixel++;
			}
			break;
		
		case GL_LEQUAL:
			while( w > 0 )
			{
				w--;
				if ( ref < (*pixel).rgba.a )
					*vb = 0;
				vb++;
				pixel++;
			}
			break;
		
		case GL_EQUAL:
			while( w > 0 )
			{
				w--;
				if ( ref != (*pixel).rgba.a )
					*vb = 0;
				vb++;
				pixel++;
			}
			break;
		
		case GL_GEQUAL:
			while( w > 0 )
			{
				w--;
				if ( ref > (*pixel).rgba.a )
					*vb = 0;
				vb++;
				pixel++;
			}
			break;
		
		case GL_GREATER:
			while( w > 0 )
			{
				w--;
				if ( ref >= (*pixel).rgba.a )
					*vb = 0;
				vb++;
				pixel++;
			}
			break;
		
		case GL_NOTEQUAL:
			while( w > 0 )
			{
				w--;
				if ( ref == (*pixel).rgba.a )
					*vb = 0;
				vb++;
				pixel++;
			}
			break;
	}
}

_STATIC_ void rasScanlineBlend( __glContext *gc, GLint pos, GLint w )
{
	rasPixel32 *in, *pixel;
	GLfloat r,g,b,a, temp;
	GLfloat over255 = 1 / 255.0;

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	pixel = (rasPixel32 *)&gc->buffer.current->ScanlineFB[pos];
	in = (rasPixel32 *)&gc->buffer.current->ScanlineFBIN[pos];

	while( w > 0 )
	{
		w--;

		r = 0;
		g = 0;
		b = 0;
		a = 0;
		switch( gc->state.color.BlendSrcFunction )
		{
		case GL_ZERO:
			break;
		case GL_ONE:
			r = (*pixel).rgba.r;
			g = (*pixel).rgba.g;
			b = (*pixel).rgba.b;
			a = (*pixel).rgba.a;
			break;
		case GL_DST_COLOR:
			r = ((GLfloat)(*pixel).rgba.r) * (*in).rgba.r * over255;
			g = ((GLfloat)(*pixel).rgba.g) * (*in).rgba.g * over255;
			b = ((GLfloat)(*pixel).rgba.b) * (*in).rgba.b * over255;
			a = ((GLfloat)(*pixel).rgba.a) * (*in).rgba.a * over255;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			r = ((GLfloat)(*pixel).rgba.r) * (255-(*in).rgba.r) * over255;
			g = ((GLfloat)(*pixel).rgba.g) * (255-(*in).rgba.g) * over255;
			b = ((GLfloat)(*pixel).rgba.b) * (255-(*in).rgba.b) * over255;
			a = ((GLfloat)(*pixel).rgba.a) * (255-(*in).rgba.a) * over255;
			break;
		case GL_SRC_ALPHA:
			temp = ((GLfloat)(*pixel).rgba.a) * over255;
			r = ((GLfloat)(*pixel).rgba.r) * temp;
			g = ((GLfloat)(*pixel).rgba.g) * temp;
			b = ((GLfloat)(*pixel).rgba.b) * temp;
			a = ((GLfloat)(*pixel).rgba.a) * temp;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			temp = ((GLfloat)255-(*pixel).rgba.a) * over255;
			r = ((GLfloat)(*pixel).rgba.r) * temp;
			g = ((GLfloat)(*pixel).rgba.g) * temp;
			b = ((GLfloat)(*pixel).rgba.b) * temp;
			a = ((GLfloat)(*pixel).rgba.a) * temp;
			break;
		case GL_DST_ALPHA:
			temp = ((GLfloat)(*in).rgba.a) * over255;
			r = ((GLfloat)(*pixel).rgba.r) * temp;
			g = ((GLfloat)(*pixel).rgba.g) * temp;
			b = ((GLfloat)(*pixel).rgba.b) * temp;
			a = ((GLfloat)(*pixel).rgba.a) * temp;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			temp = ((GLfloat)255-(*in).rgba.a) * over255;
			r = ((GLfloat)(*pixel).rgba.r) * temp;
			g = ((GLfloat)(*pixel).rgba.g) * temp;
			b = ((GLfloat)(*pixel).rgba.b) * temp;
			a = ((GLfloat)(*pixel).rgba.a) * temp;
			break;
		case GL_SRC_ALPHA_SATURATE:
			if( (*pixel).rgba.a < (255-(*in).rgba.a) )
				temp = (*pixel).rgba.a;
			else
				temp = (255-(*in).rgba.a);
			r = ((GLfloat)(*pixel).rgba.r) * temp;
			g = ((GLfloat)(*pixel).rgba.g) * temp;
			b = ((GLfloat)(*pixel).rgba.b) * temp;
			a = 255;
			break;
		}
		switch( gc->state.color.BlendDestFunction )
		{
		case GL_ZERO:
			break;
		case GL_ONE:
			r += (*in).rgba.r;
			g += (*in).rgba.g;
			b += (*in).rgba.b;
			a += (*in).rgba.a;
			break;
		case GL_SRC_COLOR:
			r += ((GLfloat)(*in).rgba.r) * (*pixel).rgba.r * over255;
			g += ((GLfloat)(*in).rgba.g) * (*pixel).rgba.g * over255;
			b += ((GLfloat)(*in).rgba.b) * (*pixel).rgba.b * over255;
			a += ((GLfloat)(*in).rgba.a) * (*pixel).rgba.a * over255;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			r += ((GLfloat)(*in).rgba.r) * (255-(*pixel).rgba.r) * over255;
			g += ((GLfloat)(*in).rgba.g) * (255-(*pixel).rgba.g) * over255;
			b += ((GLfloat)(*in).rgba.b) * (255-(*pixel).rgba.b) * over255;
			a += ((GLfloat)(*in).rgba.a) * (255-(*pixel).rgba.a) * over255;
			break;
		case GL_SRC_ALPHA:
			temp = ((GLfloat)(*pixel).rgba.a) * over255;
			r += ((GLfloat)(*in).rgba.r) * temp;
			g += ((GLfloat)(*in).rgba.g) * temp;
			b += ((GLfloat)(*in).rgba.b) * temp;
			a += ((GLfloat)(*in).rgba.a) * temp;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			temp = ((GLfloat)255-(*pixel).rgba.a) * over255;
			r += ((GLfloat)(*in).rgba.r) * temp;
			g += ((GLfloat)(*in).rgba.g) * temp;
			b += ((GLfloat)(*in).rgba.b) * temp;
			a += ((GLfloat)(*in).rgba.a) * temp;
			break;
		case GL_DST_ALPHA:
			temp = ((GLfloat)(*in).rgba.a) * over255;
			r += ((GLfloat)(*in).rgba.r) * temp;
			g += ((GLfloat)(*in).rgba.g) * temp;
			b += ((GLfloat)(*in).rgba.b) * temp;
			a += ((GLfloat)(*in).rgba.a) * temp;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			temp = ((GLfloat)255-(*in).rgba.a) * over255;
			r += ((GLfloat)(*in).rgba.r) * temp;
			g += ((GLfloat)(*in).rgba.g) * temp;
			b += ((GLfloat)(*in).rgba.b) * temp;
			a += ((GLfloat)(*in).rgba.a) * temp;
			break;
		}
		if( r > 255 )
			r = 255;
		if( g > 255 )
			g = 255;
		if( b > 255 )
			b = 255;
		if( a > 255 )
			a = 255;

		(*pixel).rgba.r = r;
		(*pixel).rgba.g = g;
		(*pixel).rgba.b = b;
		(*pixel).rgba.a = a;

		pixel++;
		in++;
		pos++;
	}
}

void rasScanlineMask( __glContext *gc, GLint x1, GLint x2 )
{
	GLint pos = x1;
	rasPixel32 *in, *pixel;
	GLint w = x2-x1;

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	pixel = (rasPixel32 *)&gc->buffer.current->ScanlineFB[pos];
	in = (rasPixel32 *)&gc->buffer.current->ScanlineFBIN[pos];

	while( w > 0 )
	{
		w--;

		if( !gc->state.color.RedWriteEnabled )
			(*pixel).rgba.r = (*in).rgba.r;
		if( !gc->state.color.GreenWriteEnabled )
			(*pixel).rgba.g = (*in).rgba.g;
		if( !gc->state.color.BlueWriteEnabled )
			(*pixel).rgba.b = (*in).rgba.b;
		if( !gc->state.color.AlphaWriteEnabled )
			(*pixel).rgba.a = (*in).rgba.a;

		pixel++;
		in++;
		pos++;
	}
}

void rasStorePixel( __glContext *gc, GLint x, GLint y, __glColor *color, GLfloat z )
{
	GLboolean valid = GL_TRUE;
	float cmultTemp[5] = {255};
	GLuint t1;

	if( (y<0) || (y>=gc->buffer.current->Height) || (x<0) || (x>=gc->buffer.current->Width))
		return;

	if( !gc->softScanProcs.valid )
		validateSoftScanProcs( gc );

	if( gc->procs.roundZValue )
		z = (*gc->procs.roundZValue)(gc,z);
	gc->buffer.current->ScanlineY = y;
	
	if( gc->state.alpha.TestEnabled )
	{
		if( !_glCompFunc( gc->state.alpha.TestFunction, gc->state.alpha.TestValue, color->A ) )
			return;
	}

//	rasLoadNeededScanlines( state, x, x+1 );
	rasLoadScanline( gc, gc->softScanProcs.loadBits, x, x+1 );
	gc->buffer.current->ScanlineV[x] = 1;
	
	/* stencil & depth */
	if( gc->state.stencil.TestEnabled )
	{
		GLubyte compMask = gc->state.stencil.FunctionMask;
		GLubyte ref = gc->state.stencil.Refrence & compMask;
		GLubyte writeMask = gc->state.stencil.WriteMask;
		GLubyte *s = &gc->buffer.current->ScanlineS[x];
		if( gc->state.depth.TestEnabled )
		{
			if ( _glCompFunc( gc->state.stencil.Function, ref, *s ) )
			{
				if ( _glCompFunc( gc->state.depth.TestFunction, gc->buffer.current->ScanlineZ[x], z ) )
				{
					//*db = z2;
					*s = __rasStencilOp( gc->state.stencil.DepthPassOp, *s, gc->state.stencil.Refrence ) & writeMask;
				}
				else
				{
					valid = 0;
					*s = __rasStencilOp( gc->state.stencil.DepthFailOp, *s, gc->state.stencil.Refrence ) & writeMask;
				}
			}
			else
			{
				valid = 0;
				*s = __rasStencilOp( gc->state.stencil.FailOp, *s, gc->state.stencil.Refrence ) & writeMask;
			}
		}
		else
		{
			if ( _glCompFunc( gc->state.stencil.Function, ref, *s ) )
			{
				*s = __rasStencilOp( gc->state.stencil.DepthPassOp, *s, gc->state.stencil.Refrence ) & writeMask;
			}
			else
			{
				*s = __rasStencilOp( gc->state.stencil.FailOp, *s, gc->state.stencil.Refrence ) & writeMask;
				valid=0;
			}
		}
	}
	else
	{
		if( gc->state.depth.TestEnabled )
		{
			if ( !_glCompFunc( gc->state.depth.TestFunction, gc->buffer.current->ScanlineZ[x], z ) )
				valid = 0;
		}
	}
	
	//gc->buffer.current->ScanlineBuffer.fb[x].rgba.r = color->r * 255;
	//gc->buffer.current->ScanlineBuffer.fb[x].rgba.g = color->g * 255;
	//gc->buffer.current->ScanlineBuffer.fb[x].rgba.b = color->b * 255;
	//gc->buffer.current->ScanlineBuffer.fb[x].rgba.a = color->a * 255;
	__asm__ __volatile__ (
	"	flds (%%eax) \n"
//	"	fmuls (%%ecx) \n"
	"	flds 4(%%eax) \n"
//	"	fmuls (%%ecx) \n"
	"	flds 8(%%eax) \n"
//	"	fmuls (%%ecx) \n"
	"	flds 12(%%eax) \n"
//	"	fmuls (%%ecx) \n"
	"	fxch %%st(3) \n"
	"	fistpl 4(%%ecx) \n"
	"	fxch %%st(1) \n"
	"	fistpl 8(%%ecx) \n"
	"	fistpl 12(%%ecx) \n"
	"	fistpl 16(%%ecx) \n"
	"	movl 4(%%ecx), %%eax \n"
	"	movb %%al, 2(%%edx) \n"
	"	movl 8(%%ecx), %%eax \n"
	"	movb %%al, 1(%%edx) \n"
	"	movl 12(%%ecx), %%eax \n"
	"	movb %%al, (%%edx) \n"
	"	movl 16(%%ecx), %%eax \n"
	"	movb %%al, 3(%%edx) \n"
	: "=a"(t1)
	:"c"(&cmultTemp[0]),"d"(&gc->buffer.current->ScanlineFB[x]),"a"(&color->R) );
	

	/* Blending */
	if( gc->state.color.BlendEnabled )
		rasScanlineBlend( gc, x, 1 );


	gc->buffer.current->ScanlineV[x] = valid;
	if( gc->buffer.current->DepthEnabled &&
			gc->state.depth.TestEnabled && 
			gc->state.depth.WriteEnabled && valid )
	{
		gc->buffer.current->ScanlineZ[x] = z;
	}

	if( gc->softScanProcs.maskEnabled )
		rasScanlineMask( gc, x, x+1 );
	rasStoreScanline( gc, gc->softScanProcs.storeBits, x, x+1 );
}

void rasProcessScanline( __glContext *gc, const __glFragment *start, GLint w )
{
#if 1
	GLint x1,x2;

	if( (gc->buffer.current->ScanlineY < 0) || (gc->buffer.current->ScanlineY >= gc->buffer.current->Height) )
		return;
	if( (start->x+w) >= gc->buffer.current->Width )
		w -= (start->x+w) - gc->buffer.current->Width +1;


	if( !gc->softScanProcs.valid )
		validateSoftScanProcs( gc );

	if( start->x >= 0 )
	{
		x1 = start->x;
	}
	else
	{
		x1 = 0; 
	}
	x2 = start->x+w;
	if( x1 >= x2 )
		return;

	rasLoadScanline( gc, gc->softScanProcs.loadBits, x1, x2 );

#if 0
	memset( &gc->buffer.current->ScanlineV[x1], 1, x2-x1 );
#else
	{
		GLuint asmT1, asmT2;
		__asm__ __volatile__ (
		" cld \n\t"
		" movl $0x01010101, %%eax \n\t"
		" addl $3, %%ecx \n\t"
		" shr $2, %%ecx \n\t"
		" rep \n\t"
		" stosl \n\t"
		: "=c"(asmT1), "=D"(asmT2)
		: "D"(&gc->buffer.current->ScanlineV[x1]), "c"(x2-x1)
		: "%eax" );
	}
#endif

	if( gc->softScanProcs.colorWriteEnabled )
		rasScanlineRasterShade( gc, start, w );

	if ( gc->state.poly.StippleEnabled )
		rasScanlinePolyStipple( gc, x1, w );

	if ( gc->texture.Enabled[0] || gc->texture.Enabled[1] )
	{
		if ( gc->state.texture.Enabled2D[0] )
			rasScanlineRasterTexture_2D( gc, start, w );
		if ( gc->state.texture.Enabled1D[0] )
			rasScanlineRasterTexture_1D( gc, start, w );
	}
		
	if ( gc->state.fog.Enabled )
		rasScanlineFog( gc, start, w );

	if( gc->state.alpha.TestEnabled )
		rasScanlineAlphaTest( gc, x1, w );

	if ( gc->state.stencil.TestEnabled )
	{
		if ( gc->state.depth.TestEnabled )
			rasScanlineStencilAndDepth( gc, x1, w, start->z, gc->softScanProcs.shade.dzdx );
		else
			rasScanlineStencil( gc, x1, w );
	}
	else
	{
		if ( gc->state.depth.TestEnabled )
			rasScanlineDepth( gc, x1, w, start->z, gc->softScanProcs.shade.dzdx );
	}

	if( gc->state.color.BlendEnabled )
		rasScanlineBlend( gc, x1, w );
	if( gc->softScanProcs.maskEnabled )
		rasScanlineMask( gc, x1, x2 );


	rasStoreScanline( gc, gc->softScanProcs.storeBits, x1, x2 );
#endif
}

