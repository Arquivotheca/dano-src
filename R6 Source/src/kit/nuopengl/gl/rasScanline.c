#include "context.h"
#include "rasTriangle.h"
#include "rasScanline.h"
#include "rasBuffers.h"

#if __INTEL_P5__
extern void scanlineShadeMMX( __glShade *shade, void *buf, GLint w );
#endif

static GLboolean _glCompFunc( GLenum func, GLfloat ref, GLfloat t );
static void rasScanlineRasterShade( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static void rasScanlineDepth( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static GLubyte __rasStencilOp( GLenum op, GLubyte buffer, GLubyte ref );
static void rasScanlineStencil( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static void rasScanlineStencilAndDepth( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static void rasScanlinePolyStipple( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static void rasScanlineFog( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static void rasScanlineRasterTexture_2D( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static void rasScanlineAlphaTest( rasState *state, const __glFragment *start, __glShade *shade, GLint w );
static void rasScanlineBlend( rasState *state, GLint pos, GLint w );
static void rasScanlineLogicOp( rasState *state, GLint pos, GLint w );
static void rasScanlineMask( rasState *state, GLint x1, GLint x2 );

#if __INTEL_P5__
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

static void scanlineDepthRound( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthNeverNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthAlwaysWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthAlwaysNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthLessWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthLessNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthLequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthLequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthEqualWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthEqualNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthGequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthGequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthGreaterWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthGreaterNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthNotequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );
static void scanlineDepthNotequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );

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

static void scanlineDepthLessWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( < )
}
static void scanlineDepthLessNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( < )
}
static void scanlineDepthLequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( <= )
}
static void scanlineDepthLequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( <= )
}
static void scanlineDepthEqualWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( == )
}
static void scanlineDepthEqualNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( == )
}
static void scanlineDepthGequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( >= )
}
static void scanlineDepthGequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( >= )
}
static void scanlineDepthGreaterWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( > )
}
static void scanlineDepthGreaterNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( > )
}
static void scanlineDepthNotequalWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_WRITE( != )
}
static void scanlineDepthNotequalNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	SCANLINE_DEPTH_NO_WRITE( != )
}



static void validateSoftScanProcs( __glContext *gc )
{
	rasState *state = gc->rasterState;
	gc->softScanProcs.valid = GL_TRUE;
	
//	state->cpuid.hasCMOV = 0;
	
	if( state->procs.roundZValue )
	{
		gc->softScanProcs.scanlineDepth = scanlineDepthRound;
	}
	else
	{
		switch( state->depthTestFunction )
		{
			case GL_NEVER:
				gc->softScanProcs.scanlineDepth = scanlineDepthNeverNoWrite;
				break;
			case GL_ALWAYS:
				if( state->depthWriteEnabled )
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthAlways_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthAlwaysWrite;
				}
				else
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthAlways_NoWrite_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthAlwaysNoWrite;
				}
				break;
			case GL_LESS:
				if( state->depthWriteEnabled )
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthLess_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthLessWrite;
				}
				else
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthLess_NoWrite_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthLessNoWrite;
				}
				break;
			case GL_LEQUAL:
				if( state->depthWriteEnabled )
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthLequal_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthLequalWrite;
				}
				else
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthLequal_NoWrite_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthLequalNoWrite;
				}
				break;
			case GL_EQUAL:
				if( state->depthWriteEnabled )
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthEqual_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthEqualWrite;
				}
				else
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthEqual_NoWrite_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthEqualNoWrite;
				}
				break;
			case GL_GEQUAL:
				if( state->depthWriteEnabled )
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthGequal_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthGequalWrite;
				}
				else
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthGequal_NoWrite_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthGequalNoWrite;
				}
				break;
			case GL_GREATER:
				if( state->depthWriteEnabled )
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthGreater_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthGreaterWrite;
				}
				else
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthGreater_NoWrite_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthGreaterNoWrite;
				}
				break;
			case GL_NOTEQUAL:
				if( state->depthWriteEnabled )
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthNotequal_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthNotequalWrite;
				}
				else
				{
#if __INTEL_P5__
					if( state->cpuid.hasCMOV && (!state->alphaTestEnabled) )
						gc->softScanProcs.scanlineDepth = scanlineDepthNotequal_NoWrite_CMOV;
					else
#endif
						gc->softScanProcs.scanlineDepth = scanlineDepthNotequalNoWrite;
				}
				break;
		}
	}

	if ( (state->colorRedWriteEnabled == GL_FALSE) && 
		(state->colorGreenWriteEnabled == GL_FALSE) &&
		(state->colorBlueWriteEnabled == GL_FALSE) &&
		(state->colorAlphaWriteEnabled == GL_FALSE) )
		gc->softScanProcs.colorWriteEnabled = GL_FALSE;
	else
		gc->softScanProcs.colorWriteEnabled = GL_TRUE;

	gc->softScanProcs.loadBits = 0;
	if ( state->colorBlendEnabled )
		gc->softScanProcs.loadBits |= GL_COLOR_BUFFER_BIT;
	if ( state->colorLogicOpEnabled )
		gc->softScanProcs.loadBits |= GL_COLOR_BUFFER_BIT;
	if( (!state->colorRedWriteEnabled) ||
		(!state->colorGreenWriteEnabled) ||
		(!state->colorBlueWriteEnabled) ||
		(!state->colorAlphaWriteEnabled) )
		gc->softScanProcs.loadBits |= GL_COLOR_BUFFER_BIT;
	if( state->depthBufferEnabled && state->depthTestEnabled )
		gc->softScanProcs.loadBits |= GL_DEPTH_BUFFER_BIT;
	if( state->stencilBufferEnabled && state->stencilTestEnabled )
		gc->softScanProcs.loadBits |= GL_STENCIL_BUFFER_BIT;


	gc->softScanProcs.storeBits = 0;
	if ( state->colorRedWriteEnabled || 
		state->colorGreenWriteEnabled ||
		state->colorBlueWriteEnabled ||
		state->colorAlphaWriteEnabled )
		gc->softScanProcs.storeBits |= GL_COLOR_BUFFER_BIT;
	if( state->depthBufferEnabled &&
			state->depthTestEnabled && 
			state->depthWriteEnabled )
		gc->softScanProcs.storeBits |= GL_DEPTH_BUFFER_BIT;
	if( state->stencilBufferEnabled &&
			state->stencilTestEnabled && 
			state->stencilWriteMask )
		gc->softScanProcs.storeBits |= GL_STENCIL_BUFFER_BIT;	
		
	if( (!state->colorRedWriteEnabled) ||
		(!state->colorGreenWriteEnabled) ||
		(!state->colorBlueWriteEnabled) ||
		(!state->colorAlphaWriteEnabled) )
		gc->softScanProcs.maskEnabled = GL_TRUE;
	else
		gc->softScanProcs.maskEnabled = GL_FALSE;
}

static GLboolean _glCompFunc( GLenum func, GLfloat ref, GLfloat t )
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

static void rasScanlineRasterShade( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	GLint temp;
	GLint pos = start->x;
	GLuint *fb;
	rasPixel32 color;
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	GLint r,g,b,a;
	GLint wbak;

	{
		wbak = w;
		r = start->color.r;
		g = start->color.g;
		b = start->color.b;
		a = start->color.a;

		r = r << 6;
		g = g << 6;
		b = b << 6;
		a = a << 6;

		if( pos < 0 )
		{
			w += pos;
			r -= shade->mmxDR * pos;
			g -= shade->mmxDG * pos;
			b -= shade->mmxDB * pos;
			a -= shade->mmxDA * pos;
			pos = 0;
		}
		
		fb = (GLuint *)&sb->fb[pos];
		while( w > 0 )
		{
			if( r < 0 )
				temp = 0;
			else
			{
				temp = r>>6;
				if( temp > 255 )
					temp = 255;
			}
			color.rgba.r = temp;
			
			if( g < 0 )
				temp = 0;
			else
			{
				temp = g>>6;
				if( temp > 255 )
					temp = 255;
			}
			color.rgba.g = temp;

			if( b < 0 )
				temp = 0;
			else
			{
				temp = b>>6;
				if( temp > 255 )
					temp = 255;
			}
			color.rgba.b = temp;

			if( a < 0 )
				temp = 0;
			else
			{
				temp = a>>6;
				if( temp > 255 )
					temp = 255;
			}
			color.rgba.a = temp;

			w--;
			*fb = color.value;	/* Set Color */
			fb++;
			
			r += shade->mmxDR;
			g += shade->mmxDG;
			b += shade->mmxDB;
			a += shade->mmxDA;
		}
		w = wbak;
	}
}

static void scanlineDepthRound( GLubyte *vb, GLfloat *db, GLfloat z, GLfloat dz, GLint w )
{
}

static void scanlineDepthNeverNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	while( w > 0 )
	{
		w--;
		*valid = 0;	/* All tests fail */
		valid++;
	}
}

static void scanlineDepthAlwaysWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
	while( w > 0 )
	{
		w--;
		*zbuf = z1;
		zbuf++;
		z1 += dz;
	}
}

static void scanlineDepthAlwaysNoWrite( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w )
{
}

static void rasScanlineDepth( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	__glContext *gc = state->glReserved;
	GLfloat z = start->z;
	GLint pos = start->x;
	GLfloat *db;
	GLubyte *vb;		//valid buffer
	GLfloat dz = shade->dzdx;
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	GLfloat z2;

	if( pos < 0 )
	{
		w += pos;
		z -= dz * pos;
		pos = 0;
	}
	db = &sb->z[pos];
	vb = (GLubyte *)&sb->valid[pos];
	
	if( !state->procs.roundZValue )
		(*gc->softScanProcs.scanlineDepth)( vb, db, z, dz, w );
	else	
	{
		if( state->depthWriteEnabled )
		{
			while( w > 0 )
			{
				w--;
				z2 = (*state->procs.roundZValue)(state,z);
				if ( _glCompFunc( state->depthTestFunction, *db, z2 ) )
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
				z2 = (*state->procs.roundZValue)(state,z);
				if ( !_glCompFunc( state->depthTestFunction, *db, z2 ) )
					*vb = 0;
				db++;
				vb++;
				z += dz;
			}
		}
	}
}

static GLubyte __rasStencilOp( GLenum op, GLubyte buffer, GLubyte ref )
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

static void rasScanlineStencil( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	GLint pos = start->x;
	GLubyte *s;
	GLubyte *vb;		//valid buffer
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	GLubyte compMask = state->stencilFunctionMask;
	GLubyte ref = state->stencilRefrence & compMask;
	GLubyte writeMask = state->stencilWriteMask;
	GLubyte clearMask = 0xff ^ state->stencilWriteMask;
	GLubyte t;
	GLboolean pass;
	
	GLubyte old;

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	
	s = &sb->stencil[pos];
	vb = (GLubyte *)&sb->valid[pos];
	
	while( w > 0 )
	{
		w--;
		t = *s & compMask;

		switch( state->stencilFunction )
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
		
		old = *s;
		
		if ( pass )
		{
			if( *vb )
				*s = (__rasStencilOp( state->stencilDepthPassOp, *s, state->stencilRefrence ) & writeMask) |
					( (*s) & (~writeMask));
		}
		else
		{
			if( *vb )
			{
				*s = (__rasStencilOp( state->stencilFailOp, *s, state->stencilRefrence ) & writeMask ) |
					( (*s) & (~writeMask));
				*vb=0;
			}
		}

		s++;
		vb++;
	}
	
	
}

static void rasScanlineStencilAndDepth( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	GLfloat z = start->z;
	GLfloat z2;
	GLint pos = start->x;
	GLfloat *db;
	GLubyte *s;
	GLubyte *vb;		//valid buffer
	GLfloat dz = shade->dzdx;
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	GLubyte compMask = state->stencilFunctionMask;
	GLubyte ref = state->stencilRefrence & compMask;
	GLubyte writeMask = state->stencilWriteMask;
	GLubyte clearMask = 0xff ^ state->stencilWriteMask;

	if( pos < 0 )
	{
		w += pos;
		pos = -pos;
		z += dz * pos;
		pos = 0;
	}
	s = &sb->stencil[pos];
	db = &sb->z[pos];
	vb = (GLubyte *)&sb->valid[pos];
	
	while( w > 0 )
	{
		w--;
		if( state->procs.roundZValue )
			z2 = (*state->procs.roundZValue)(state,z);
		else
			z2 = z;

		if( *vb )
		{
			if ( _glCompFunc( state->stencilFunction, ref, (*s) & compMask )  )
			{
				if ( _glCompFunc( state->depthTestFunction, *db, z2 ) )
				{
					if( state->depthWriteEnabled )
						*db = z;
					*s = (__rasStencilOp( state->stencilDepthPassOp, *s, state->stencilRefrence ) & writeMask) |
						( (*s) & (~writeMask));
				}
				else
				{
					*vb = 0;
					*s = (__rasStencilOp( state->stencilDepthFailOp, *s, state->stencilRefrence ) & writeMask) |
						( (*s) & (~writeMask));
				}
			}
			else
			{
				*vb = 0;
				*s = (__rasStencilOp( state->stencilFailOp, *s, state->stencilRefrence ) & writeMask) |
						( (*s) & (~writeMask));
			}
		}
		
		s++;
		db++;
		vb++;
		z+=dz;
		
		// GCC HACK
#if __INTEL__ && __GNUC__
		__asm__ __volatile__ ( "":::"st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)" );
#endif
	}
}


static void rasScanlinePolyStipple( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	GLint pos = start->x;
	GLubyte *vb;		//valid buffer
	GLuint stipple = ((GLuint*)state->polyStippleMask)[(state->scanlineBuffer.y) & 0x1f];

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	vb = (GLubyte *)&state->scanlineBuffer.valid[pos];

	while( w > 0 )
	{
		w--;
		if( !(stipple & (1<<(pos&0x1f))) )
			*vb = 0;
		vb++;
		pos++;
	}
}

static void rasScanlineFog( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	GLfloat fog, oneMinusFog, density, fogStart, fogEnd;
	rasPixel32 fogColor;
	GLint pos = start->x;	
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	GLfloat e = 2.71828182846;
	GLfloat f = start->f;
	GLfloat df = shade->dfdx;
	rasPixel32 *pixel;
	GLfloat eyeZ;

	fogColor.rgba.r = state->fogColor.r * 255;
	fogColor.rgba.g = state->fogColor.g * 255;
	fogColor.rgba.b = state->fogColor.b * 255;
	fogColor.rgba.a = state->fogColor.a * 255;


	if (state->fogHint == GL_NICEST) 
	{
		density = state->fogDensity;
		fogStart = state->fogStart;
		fogEnd = state->fogEnd;
		
		if( pos < 0 )
		{
			w += pos;
			pos = -pos;
			f += df * pos;
			pos = 0;
		}
		pixel = &sb->fb[pos];

		while( w > 0 )
		{
	
			eyeZ = f;
			if (eyeZ < 0) 
				eyeZ = -eyeZ;
		
			switch (state->fogMode) 
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
		pixel = &sb->fb[pos];
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
			(*pixel).rgba.r = fog * (*pixel).rgba.r + oneMinusFog * fogColor.rgba.r;
			(*pixel).rgba.g = fog * (*pixel).rgba.g + oneMinusFog * fogColor.rgba.g;
			(*pixel).rgba.b = fog * (*pixel).rgba.b + oneMinusFog * fogColor.rgba.b;
	
			pixel++;
			w--;
			f += df;
		}
	}
		
}

void rasScanlineRasterTexture_2D( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	__glContext *gc = state->glReserved;
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	GLint pos = start->x;
	rasPixel32 *pixel;
	__glTexture *tex = gc->texture.currentTexture;
	__glFragment frag;
	GLfloat over255, overQ;
	GLfloat s,t,q;
	GLfloat ds,dt,dq;

	if( tex == 0 )
		return;

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
	q = start->qw;
	ds = shade->dsdx;
	dt = shade->dtdx;
	dq = shade->dqwdx;
	over255 = 1.0 / 255.0;

	if( pos < 0 )
	{
		w += pos;
		s -= ds * pos;
		t -= dt * pos;
		q -= dq * pos;
		pos = 0;
	}
	pixel = &sb->fb[pos];

	while( w > 0 )
	{
		w--;
		frag.color.r = ((GLfloat)(*pixel).rgba.r);
		frag.color.g = ((GLfloat)(*pixel).rgba.g);
		frag.color.b = ((GLfloat)(*pixel).rgba.b);
		frag.color.a = ((GLfloat)(*pixel).rgba.a);
		overQ = 1 / q;
		(*tex->textureFunc)( gc, &frag, s*overQ, t*overQ, 
			(*gc->methods.calcPolygonRho)(gc,shade,s,t,q) );
		(*pixel).rgba.r = frag.color.r;
		(*pixel).rgba.g = frag.color.g;
		(*pixel).rgba.b = frag.color.b;
		(*pixel).rgba.a = frag.color.a;

		s += ds;
		t += dt;
		q += dq;
		pixel++;
	}
}

void rasScanlineAlphaTest( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	GLint pos = start->x;
	rasPixel32 *pixel;
	GLubyte *vb;		//valid buffer
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	GLfloat temp = state->alphaTestValue * 255;
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
	pixel = &sb->fb[pos];
	vb = &sb->valid[pos];
	
	switch( state->alphaTestFunction )
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

static void rasScanlineBlend( rasState *state, GLint pos, GLint w )
{
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	rasPixel32 *in, *pixel;
	GLfloat r,g,b,a, temp;
	GLfloat over255 = 1 / 255.0;

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	pixel = &sb->fb[pos];
	in = &sb->fbin[pos];

	while( w > 0 )
	{
		w--;

		r = 0;
		g = 0;
		b = 0;
		a = 0;
		switch( state->colorBlendSrcFunction )
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
		switch( state->colorBlendDestFunction )
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

void rasScanlineLogicOp( rasState *state, GLint x1, GLint w )
{
	GLint pos = x1;
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	rasPixel32 *in, *pixel;

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	pixel = &sb->fb[pos];
	in = &sb->fbin[pos];

	while( w > 0 )
	{
		w--;
		switch( state->colorLogicOp )
		{
			case GL_CLEAR:
				(*pixel).value = 0; break;
			case GL_COPY:
				break;
			case GL_NOOP:
				(*pixel).value = (*in).value; break;
			case GL_SET:
				(*pixel).value = 0xffffffff; break;
			case GL_COPY_INVERTED:
				(*pixel).value = ~(*pixel).value; break;
			case GL_INVERT:
				(*pixel).value = ~(*in).value; break;
			case GL_AND_REVERSE:
				(*pixel).value &= ~(*in).value; break;
			case GL_OR_REVERSE:
				(*pixel).value |= ~(*in).value; break;
			case GL_AND:
				(*pixel).value &= (*in).value; break;
			case GL_OR:
				(*pixel).value |= (*in).value; break;
			case GL_NAND:
				(*pixel).value = ~((*pixel).value & (*in).value); break;
			case GL_NOR:
				(*pixel).value = ~((*pixel).value | (*in).value); break;
			case GL_XOR:
				(*pixel).value ^= (*in).value; break;
			case GL_EQUIV:
				(*pixel).value = ~((*pixel).value ^ (*in).value); break;
			case GL_AND_INVERTED:
				(*pixel).value = (~(*pixel).value) & (*in).value; break;
			case GL_OR_INVERTED:
				(*pixel).value = (~(*pixel).value) | (*in).value; break;
		}
		
		pixel++;
		in++;
	}
}


void rasScanlineMask( rasState *state, GLint x1, GLint x2 )
{
	GLint pos = x1;
	rasScanlineBuffer *sb = &state->scanlineBuffer;
	rasPixel32 *in, *pixel;
	GLint w = x2-x1;

	if( pos < 0 )
	{
		w += pos;
		pos = 0;
	}
	pixel = &sb->fb[pos];
	in = &sb->fbin[pos];

	while( w > 0 )
	{
		w--;

		if( !state->colorRedWriteEnabled )
			(*pixel).rgba.r = (*in).rgba.r;
		if( !state->colorGreenWriteEnabled )
			(*pixel).rgba.g = (*in).rgba.g;
		if( !state->colorBlueWriteEnabled )
			(*pixel).rgba.b = (*in).rgba.b;
		if( !state->colorAlphaWriteEnabled )
			(*pixel).rgba.a = (*in).rgba.a;

		pixel++;
		in++;
		pos++;
	}
}

void rasStorePixel( rasState *state, GLint x, GLint y, rasColor *color, GLfloat z )
{
	__glContext *gc = state->glReserved;
	GLboolean valid = GL_TRUE;
	GLfloat ref;

	if( (y<0) || (y>=state->bufferHeight) || (x<0) || (x>=state->bufferWidth))
		return;

	if( !gc->softScanProcs.valid )
		validateSoftScanProcs( gc );

	if( state->procs.roundZValue )
		z = (*state->procs.roundZValue)(state,z);
	state->scanlineBuffer.y = y;
	
	if( state->alphaTestEnabled )
	{
		if( !_glCompFunc( state->alphaTestFunction, state->alphaTestValue, color->a ) )
			return;
	}

	rasLoadScanline( state, gc->softScanProcs.loadBits, x, x+1 );
	state->scanlineBuffer.valid[x] = 1;
	
	/* stencil & depth */
	if( state->stencilTestEnabled )
	{
		GLubyte compMask = state->stencilFunctionMask;
		GLubyte ref = state->stencilRefrence & compMask;
		GLubyte writeMask = state->stencilWriteMask;
		GLubyte clearMask = 0xff ^ state->stencilWriteMask;
		GLubyte *s = &state->scanlineBuffer.stencil[x];
		if( state->depthTestEnabled )
		{
			if ( _glCompFunc( state->stencilFunction, ref, *s ) )
			{
				if ( _glCompFunc( state->depthTestFunction, state->scanlineBuffer.z[x], z ) )
				{
					//*db = z2;
					*s = __rasStencilOp( state->stencilDepthPassOp, *s, state->stencilRefrence ) & writeMask;
				}
				else
				{
					valid = 0;
					*s = __rasStencilOp( state->stencilDepthFailOp, *s, state->stencilRefrence ) & writeMask;
				}
			}
			else
			{
				valid = 0;
				*s = __rasStencilOp( state->stencilFailOp, *s, state->stencilRefrence ) & writeMask;
			}
		}
		else
		{
			if ( _glCompFunc( state->stencilFunction, ref, *s ) )
			{
				*s = __rasStencilOp( state->stencilDepthPassOp, *s, state->stencilRefrence ) & writeMask;
			}
			else
			{
				*s = __rasStencilOp( state->stencilFailOp, *s, state->stencilRefrence ) & writeMask;
				valid=0;
			}
		}
	}
	else
	{
		if( state->depthTestEnabled )
		{
			if ( !_glCompFunc( state->depthTestFunction, state->scanlineBuffer.z[x], z ) )
				valid = 0;
		}
	}
	
	
	state->scanlineBuffer.fb[x].rgba.r = color->r * 255;
	state->scanlineBuffer.fb[x].rgba.g = color->g * 255;
	state->scanlineBuffer.fb[x].rgba.b = color->b * 255;
	state->scanlineBuffer.fb[x].rgba.a = color->a * 255;

	/* Blending */

	if( state->colorBlendEnabled )
		rasScanlineBlend( state, x, 1 );

	if( gc->rasterState->colorLogicOpEnabled )
		rasScanlineLogicOp( state, x, 1 );

	state->scanlineBuffer.valid[x] = valid;
	if( state->depthBufferEnabled &&
			state->depthTestEnabled && 
			state->depthWriteEnabled && valid )
	{
		state->scanlineBuffer.z[x] = z;
	}

	if( gc->softScanProcs.maskEnabled )
		rasScanlineMask( state, x, x+1 );
	rasStoreScanline( state, gc->softScanProcs.storeBits, x, x+1 );
}

void rasProcessScanline( rasState *state, const __glFragment *start, __glShade *shade, GLint w )
{
	__glContext *gc = state->glReserved;
	GLint x1,x2;

	if( (state->scanlineBuffer.y < 0) || 
		(state->scanlineBuffer.y >= state->bufferHeight) )
		return;
	if( (start->x+w) > state->bufferWidth )
		w -= (start->x+w) - state->bufferWidth;

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

	rasLoadScanline( state, gc->softScanProcs.loadBits, x1, x2 );
	memset( &state->scanlineBuffer.valid[x1], 1, x2-x1 );

	if( gc->softScanProcs.colorWriteEnabled )
		rasScanlineRasterShade( state, start, shade, w );

	if ( state->polyStippleEnabled )
		rasScanlinePolyStipple( state, start, shade, w );

	if ( state->texture2DEnabled )
		rasScanlineRasterTexture_2D( state, start, shade, w );
		
	if ( state->fogEnabled )
		rasScanlineFog( state, start, shade, w );

	if( state->alphaTestEnabled )
		rasScanlineAlphaTest( state, start, shade, w );

	if ( state->stencilTestEnabled )
	{
		if ( state->depthTestEnabled )
			rasScanlineStencilAndDepth( state, start, shade, w );
		else
			rasScanlineStencil( state, start, shade, w );
	}
	else
	{
		if ( state->depthTestEnabled )
			rasScanlineDepth( state, start, shade, w );
	}

	if( state->colorBlendEnabled )
		rasScanlineBlend( state, x1, w );

	if( gc->rasterState->colorLogicOpEnabled )
		rasScanlineLogicOp( state, x1, w );

	if( gc->softScanProcs.maskEnabled )
		rasScanlineMask( state, x1, x2 );
	rasStoreScanline( state, gc->softScanProcs.storeBits, x1, x2 );
}


