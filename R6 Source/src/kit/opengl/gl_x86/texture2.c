#define DEBUG_FUNCTION_ENTRY 0

#include <opengl/bitflinger.h>

#include "context.h"
#include "global.h"
#include "immed.h"
#include "mathLib.h"
#include "types.h"
#include "pixel.h"
#include "image.h"
#include "glImage.h"
#include "processor.h"
#include "rasRaster.h"
#include "validate.h"

#define __GL_M_LN2_INV		((__glFloat) (1.0 / 0.69314718055994530942))
#define __GL_M_SQRT2		((__glFloat) 1.41421356237309504880)

#define __STATIC__

typedef GLubyte __glTextureBuffer;



void printTexture( __glTexture *tex )
{
	printf( "Tex %p \n", tex );
	if( tex )
	{
		printf( "    name=%i \n", tex->name );
		printf( "    sWrapMode = %x\n", tex->sWrapMode );
		printf( "    tWrapMode = %x\n", tex->tWrapMode );
		printf( "    rWrapMode = %x\n", tex->rWrapMode );
		printf( "    minFilter = %x\n", tex->minFilter );
		printf( "    magFilter = %x\n", tex->magFilter );
		printf( "    name=%i \n", tex->name );
		printf( "    priority=%f \n", tex->priority );
		printf( "    hasMipmaps=%i \n", tex->hasMipmaps );
		printf( "    isValid=%i \n", tex->isValid );
		printf( "    activeLevels=%i \n", tex->activeLevels );
		printf( "    dim=%i \n", tex->dim );
	}
}

void printTextureLevel( __glTextureLevel *l )
{
	printf( "TexLevel %p\n", l );
	if( l )
	{
		printf( "    requestedFormat=%x\n", l->requestedFormat );
		printf( "    border=%x\n", l->border );
		printf( "    width=%i\n", l->width );
		printf( "    height=%i\n", l->height );
		printf( "    width2=%i\n", l->width2 );
		printf( "    height2=%i\n", l->height2 );
		printf( "    baseFormat=%x\n", l->baseFormat );
		printf( "    internalFormat=%x\n", l->internalFormat );
		printf( "    internalType=%x\n", l->internalType );
		printf( "    allocationSize=%i\n", l->allocationSize );
		printf( "    bytesPerTexel=%i\n", l->bytesPerTexel );
		printf( "    sizeR=%i\n", l->sizeR );
		printf( "    sizeG=%i\n", l->sizeG );
		printf( "    sizeB=%i\n", l->sizeB );
		printf( "    sizeA=%i\n", l->sizeA );
		printf( "    sizeL=%i\n", l->sizeL );
		printf( "    sizeI=%i\n", l->sizeI );
	}
}


inline __glFloat Log2 (__glFloat f)
{
	return log (f) * __GL_M_LN2_INV;
}

GLint __glTexParameterfv_size (GLenum e)
{
	DEBUG_BEGIN_FUNCTION( "__glTexParameterfv_size" );
	switch (e)
	{
	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_PRIORITY:
		return 1;
	case GL_TEXTURE_BORDER_COLOR:
		return 4;
	default:
		return -GL_INVALID_ENUM;
	}
	DEBUG_END_FUNCTION( "__glTexParameterfv_size" );
}

GLint __glTexParameteriv_size (GLenum e)
{
	DEBUG_BEGIN_FUNCTION( "__glTexParameteriv_size" );
	return __glTexParameterfv_size (e);
	DEBUG_END_FUNCTION( "__glTexParameteriv_size" );
}

GLint __glTexEnvfv_size (GLenum e)
{
	DEBUG_BEGIN_FUNCTION( "__glTexEnvfv_size" );
	switch (e)
	{
	case GL_TEXTURE_ENV_MODE:
		return 1;
	case GL_TEXTURE_ENV_COLOR:
		return 4;
	default:
		return -GL_INVALID_ENUM;
	}
	DEBUG_END_FUNCTION( "__glTexEnvfv_size" );
}

GLint __glTexEnviv_size (GLenum e)
{
	DEBUG_BEGIN_FUNCTION( "__glTexEnviv_size" );
	return __glTexEnvfv_size (e);
	DEBUG_END_FUNCTION( "__glTexEnviv_size" );
}

GLint __glTexGendv_size (GLenum e)
{
	DEBUG_BEGIN_FUNCTION( "__glTexGendv_size" );
	switch (e)
	{
	case GL_TEXTURE_GEN_MODE:
		return 1;
	case GL_OBJECT_PLANE:
	case GL_EYE_PLANE:
		return 4;
	default:
		return -GL_INVALID_ENUM;
	}
	DEBUG_END_FUNCTION( "__glTexGendv_size" );
}

GLint __glTexGenfv_size (GLenum e)
{
	DEBUG_BEGIN_FUNCTION( "__glTexGenfv_size" );
	return __glTexGendv_size (e);
	DEBUG_END_FUNCTION( "__glTexGenfv_size" );
}

GLint __glTexGeniv_size (GLenum e)
{
	DEBUG_BEGIN_FUNCTION( "__glTexGeniv_size" );
	return __glTexGendv_size (e);
	DEBUG_END_FUNCTION( "__glTexGeniv_size" );
}

__glTexture *__glLookUpTexture (__glContext * gc, GLenum target)
{
	DEBUG_BEGIN_FUNCTION( "__glLookUpTexture" );
	switch (target)
	{
	case GL_TEXTURE_1D:
		return gc->state.texture.Bound[gc->state.texture.SelectedUnit][__GL_TEXTURE_INDEX_1D];
	case GL_TEXTURE_2D:
		return gc->state.texture.Bound[gc->state.texture.SelectedUnit][__GL_TEXTURE_INDEX_2D];
//	case GL_TEXTURE_3D:
//		return gc->state.texture.Bound[gc->state.texture.SelectedUnit][__GL_TEXTURE_INDEX_3D];
	case GL_PROXY_TEXTURE_1D:
		return gc->state.texture.Bound[gc->state.texture.SelectedUnit][__GL_PROXY_TEXTURE_INDEX_1D];
	case GL_PROXY_TEXTURE_2D:
		return gc->state.texture.Bound[gc->state.texture.SelectedUnit][__GL_PROXY_TEXTURE_INDEX_2D];
//	case GL_PROXY_TEXTURE_3D:
//		return gc->state.texture.Bound[gc->state.texture.SelectedUnit][__GL_PROXY_TEXTURE_INDEX_3D];
	default:
		return NULL;
	}
	DEBUG_END_FUNCTION( "__glLookUpTexture" );
}

_STATIC_ GLboolean __glCheckFormatAndType( GLenum format, GLenum type )
{
	switch (type)
	{
	case GL_BITMAP:
		if (format != GL_COLOR_INDEX)
			return GL_FALSE;

	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_INT:
	case GL_UNSIGNED_INT:
	case GL_FLOAT:
		break;

	case GL_UNSIGNED_BYTE_3_3_2:
	case GL_UNSIGNED_BYTE_2_3_3_REV:
	case GL_UNSIGNED_SHORT_5_6_5:
	case GL_UNSIGNED_SHORT_5_6_5_REV:
		if (format != GL_RGB)
			return GL_FALSE;
	
	case GL_UNSIGNED_SHORT_4_4_4_4:
	case GL_UNSIGNED_SHORT_4_4_4_4_REV:
	case GL_UNSIGNED_SHORT_5_5_5_1:
	case GL_UNSIGNED_SHORT_1_5_5_5_REV:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
	case GL_UNSIGNED_INT_10_10_10_2:
	case GL_UNSIGNED_INT_2_10_10_10_REV:
		if ( !((format == GL_RGBA) || (format == GL_BGRA) ))
			return GL_FALSE;

		break;
	default:
		return GL_FALSE;
	}

	switch (format)
	{
	case GL_COLOR_INDEX:
	case GL_RED:
	case GL_GREEN:
	case GL_BLUE:
	case GL_ALPHA:
	case GL_RGB:
	case GL_RGBA:
	case GL_LUMINANCE:
	case GL_LUMINANCE_ALPHA:
	case GL_ABGR_EXT:
	case GL_BGR:
	case GL_BGRA:
		break;
	default:
		return GL_FALSE;
	}
	
	return GL_TRUE;
}

_STATIC_ __glTexture *CheckTexImageArgs (__glContext * gc, GLenum target, GLint level,
									   GLint internalFormat, GLint border,
									   GLenum format, GLenum type, GLint dim)
{
	__glTexture *tex = __glLookUpTexture( gc, target );
	DEBUG_BEGIN_FUNCTION( "CheckTexImageArgs" );
	
	if (!tex || (tex->dim != dim))
	{
	  bad_enum:
		__glSetError (gc, GL_INVALID_ENUM);
		return 0;
	}

	if( !__glCheckFormatAndType( format, type ) )
		goto bad_enum;
	
	if ((level < 0) || (level >= __GL_MAX_MIPMAP_LEVEL))
	{
		__glSetError (gc, GL_INVALID_VALUE);
		return 0;
	}

	switch (internalFormat)
	{
	case 1:
	case 2:
	case 3:
	case 4:
		break;
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
		break;
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE8_ALPHA8:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
		break;
	case GL_RGB:
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		break;
	case GL_RGBA:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGBA8:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGB5_A1:
	case GL_RGB10_A2:
		break;
	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA8:
	case GL_ALPHA12:
	case GL_ALPHA16:
		break;
	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY8:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
		break;
	default:
		goto bad_enum;
	}

	if ((border < 0) || (border > 1))
	{
		goto bad_enum;
	}

	return tex;
	DEBUG_END_FUNCTION( "CheckTexImageArgs" );
}

_STATIC_ __glTexture *CheckTexSubImageArgs (__glContext * gc, GLenum target,
				  GLint level, GLenum format, GLenum type, GLint dim)
{
	__glTexture *tex = __glLookUpTexture( gc, target );

	DEBUG_BEGIN_FUNCTION( "CheckTexSubImageArgs" );
	if (!tex || (target == GL_PROXY_TEXTURE_1D) ||
		(target == GL_PROXY_TEXTURE_2D))
	{
	  bad_enum:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "CheckTexSubImageArgs" );
		return 0;
	}

	if (tex->dim != dim)
	{
		goto bad_enum;
	}

	if( !__glCheckFormatAndType( format, type ) )
		goto bad_enum;

	if ((level < 0) || (level >= __GL_MAX_MIPMAP_LEVEL))
	{
		__glSetError (gc, GL_INVALID_VALUE);
		DEBUG_END_FUNCTION( "CheckTexSubImageArgs" );
		return 0;
	}

	DEBUG_END_FUNCTION( "CheckTexSubImageArgs" );
	return tex;
}

_STATIC_ GLboolean CheckTexSubImageRange (__glContext * gc, __glTextureLevel *lp,
									GLint offset, GLsizei size, GLsizei max)
{
	DEBUG_BEGIN_FUNCTION( "CheckTexSubImageRange" );
	if ((size < 0) || (offset < -lp->border) || (offset + size > max - lp->border))
	{
		__glSetError (gc, GL_INVALID_VALUE);
		DEBUG_END_FUNCTION( "CheckTexSubImageRange" );
		return GL_FALSE;
	}
	DEBUG_END_FUNCTION( "CheckTexSubImageRange" );
	return GL_TRUE;
}

/*
** Return GL_TRUE if the given range (length or width/height/depth) is a legal
** power of 2, taking into account the border.  The range is not allowed
** to be negative either.
*/
_STATIC_ GLboolean IsLegalRange (__glContext * gc, GLsizei r, GLint border)
{
	DEBUG_BEGIN_FUNCTION( "IsLegalRange" );
	r -= border * 2;
	if ((r < 0) || (r & (r - 1)))
	{
		__glSetError (gc, GL_INVALID_VALUE);
		DEBUG_END_FUNCTION( "IsLegalRange" );
		return GL_FALSE;
	}
	DEBUG_END_FUNCTION( "IsLegalRange" );
	return GL_TRUE;
}

__glTexture *__glCheckTexImage1DArgs (__glContext * gc, GLenum target, GLint level,
									  GLint internalFormat, GLsizei length,
								   GLint border, GLenum format, GLenum type)
{
	__glTexture *tex;

	DEBUG_BEGIN_FUNCTION( "__glCheckTexImage1DArgs" );
	/* Check arguments and get the right texture being changed */
	tex = CheckTexImageArgs (gc, target, level, internalFormat, border,
							 format, type, 1);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glCheckTexImage1DArgs" );
		return 0;
	}
	if (!IsLegalRange (gc, length, border))
	{
		DEBUG_END_FUNCTION( "__glCheckTexImage1DArgs" );
		return 0;
	}
	DEBUG_END_FUNCTION( "__glCheckTexImage1DArgs" );
	return tex;
}

__glTexture *__glCheckTexImage2DArgs (__glContext * gc, GLenum target, GLint level,
								 GLint internalFormat, GLsizei w, GLsizei h,
								   GLint border, GLenum format, GLenum type)
{
	__glTexture *tex;

	DEBUG_BEGIN_FUNCTION( "__glCheckTexImage2DArgs" );
	
	/* Check arguments and get the right texture being changed */
	tex = CheckTexImageArgs (gc, target, level, internalFormat, border,
							 format, type, 2);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glCheckTexImage2DArgs" );
		return 0;
	}
	if (!IsLegalRange (gc, w, border))
	{
		DEBUG_END_FUNCTION( "__glCheckTexImage2DArgs" );
		return 0;
	}
	if (!IsLegalRange (gc, h, border))
	{
		DEBUG_END_FUNCTION( "__glCheckTexImage2DArgs" );
		return 0;
	}
	DEBUG_END_FUNCTION( "__glCheckTexImage2DArgs" );
	return tex;
}

_STATIC_ __glTexture *__glCheckTexSubImage1DArgs (__glContext * gc, GLenum target,
										 GLint level, GLint xoffset, GLint length,
										 GLenum format, GLenum type)
{
	__glTexture *tex;
	__glTextureLevel *lp;

	DEBUG_BEGIN_FUNCTION( "__glCheckTexSubImage1DArgs" );
	/* Check arguments and get the right texture being changed */
	tex = CheckTexSubImageArgs (gc, target, level, format, type, 1);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glCheckTexSubImage1DArgs" );
		return NULL;
	}
	lp = &tex->level[level];
	if (lp->data == NULL)
	{
		__glSetError (gc, GL_INVALID_OPERATION);
		DEBUG_END_FUNCTION( "__glCheckTexSubImage1DArgs" );
		return NULL;
	}
	if (!CheckTexSubImageRange (gc, lp, xoffset, length, lp->width))
	{
		DEBUG_END_FUNCTION( "__glCheckTexSubImage1DArgs" );
		return NULL;
	}
	DEBUG_END_FUNCTION( "__glCheckTexSubImage1DArgs" );
	return tex;
}

_STATIC_ __glTexture *__glCheckTexSubImage2DArgs (__glContext * gc, GLenum target,
										 GLint level, GLint xoffset, GLint yoffset,
										 GLsizei w, GLsizei h, GLenum format, GLenum type)
{
	__glTexture *tex;
	__glTextureLevel *lp;

	DEBUG_BEGIN_FUNCTION( "__glCheckTexSubImage2DArgs" );
	/* Check arguments and get the right texture being changed */
	tex = CheckTexSubImageArgs (gc, target, level, format, type, 2);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glCheckTexSubImage2DArgs" );
		return NULL;
	}
	lp = &tex->level[level];
	if (lp->data == NULL)
	{
		__glSetError (gc, GL_INVALID_OPERATION);
		DEBUG_END_FUNCTION( "__glCheckTexSubImage2DArgs" );
		return NULL;
	}
	if (!CheckTexSubImageRange (gc, lp, xoffset, w, lp->width))
	{
		DEBUG_END_FUNCTION( "__glCheckTexSubImage2DArgs" );
		return NULL;
	}
	if (!CheckTexSubImageRange (gc, lp, yoffset, h, lp->height))
	{
		DEBUG_END_FUNCTION( "__glCheckTexSubImage2DArgs" );
		return NULL;
	}
	DEBUG_END_FUNCTION( "__glCheckTexSubImage2DArgs" );
	return tex;
}

/*
   ** Apply the pixel transfer modes to the pixel then store the value
   ** into the texel buffer.
 */
_STATIC_ void ModifyAndStoreColor (__glContext * gc, GLint baseFormat,
								 __glTextureBuffer * dest, __glPixel * pix)
{
	__glModifyPixel (gc, pix);

	/* Store the final answer */
	switch (baseFormat)
	{
	case GL_LUMINANCE_ALPHA:
		dest[1] = pix->A * 255;
		/* FALLTHROUGH */
	case GL_LUMINANCE:
		dest[0] = pix->R * 255;
		break;
	case GL_RGBA:
		dest[3] = pix->A * 255;
		/* FALLTHROUGH */
	case GL_RGB:
		dest[0] = pix->R * 255;
		dest[1] = pix->G * 255;
		dest[2] = pix->B * 255;
		break;
	case GL_ALPHA:
		dest[0] = pix->A * 255;
		break;
	case GL_INTENSITY:
		dest[0] = pix->R * 255;
		break;
	}

}

_STATIC_ void
  __glCopySubImageToTexture (__glContext * gc, __glTexture * tex, GLint level,
							 GLint xoffset, GLint yoffset, GLint w, GLint h,
							 GLenum format, GLenum type, const GLvoid * buf,
							 GLboolean packed)
{
	__glTextureLevel *lp = &tex->level[level];
	__glTextureBuffer *row;
	__glPixel pixel;
	GLubyte *image;
	GLint internalFormat;
	GLint x, y;

	DEBUG_BEGIN_FUNCTION( "__glCopySubImageToTexture" );

	if (!packed)
	{
		GLint imageSize = __glImageSize (w, h, format, type);
		image = (GLubyte *) MALLOC ( imageSize);
		if (image == NULL)
		{
			DEBUG_END_FUNCTION( "__glCopySubImageToTexture" );
			return;
		}
		__glFillImage (gc, w, h, format, type, buf, image);
	}
	else
	{
		image = (GLubyte *) buf;
	}

	switch (lp->baseFormat)
	{
	case GL_LUMINANCE:
		internalFormat = 1;
		break;
	case GL_LUMINANCE_ALPHA:
		internalFormat = 2;
		break;
	case GL_RGB:
		internalFormat = 3;
		break;
	case GL_RGBA:
		internalFormat = 4;
		break;
	case GL_ALPHA:
	default:
		internalFormat = 1;
		break;
	case GL_INTENSITY:
		internalFormat = 1;
		break;
	}
	xoffset += lp->border;
	if (tex->dim == 1)
	{
		yoffset += 2 * lp->border;
	}
	else
	{
		yoffset += lp->border;
	}

	row = lp->data + internalFormat * (lp->width * yoffset + xoffset);

	for (y = 0; y < h; y++)
	{
		__glTextureBuffer *dest = row;

		if( (type == GL_UNSIGNED_BYTE) && (__glElementsPerGroup(format)==internalFormat) )
		{
			GLubyte *from = image;
			from += __glElementOffset (gc, 0, y, type, format, w, h);
			memcpy( dest, from, w*internalFormat );
		}
		else
		{
			if( (__glElementsPerGroup(format) == 4) && (internalFormat==3) )
			{
				GLubyte *from = image;
				GLubyte *to = dest;
				GLint ct = w;
				from += __glElementOffset (gc, 0, y, type, format, w, h);
				while( ct-- )
				{
					*to = *from;
					to ++; from++;
					*to = *from;
					to ++; from++;
					*to = *from;
					to ++; from++;
					from++;
				}
			}
			else
			{
				for (x = 0; x < w; x++)
				{
					__glExtractPixel (gc, &pixel, x, y, type, format, w, h, image);
					ModifyAndStoreColor (gc, lp->baseFormat, dest, &pixel);
					dest += internalFormat;
				}
			}
		}
		row += internalFormat * lp->width;
	}

	if (!packed)
	{
		FREE ( image);
	}
	DEBUG_END_FUNCTION( "__glCopySubImageToTexture" );
}


__STATIC__ void __initTextureObject( __glTexture *obj )
{
	int ct;
	DEBUG_BEGIN_FUNCTION( "__initTextureObject" );

	obj->borderColor.R = 0.0f;
	obj->borderColor.G = 0.0f;
	obj->borderColor.B = 0.0f;
	obj->borderColor.A = 0.0f;
	obj->sWrapMode = GL_REPEAT;
	obj->tWrapMode = GL_REPEAT;
	obj->rWrapMode = GL_REPEAT;
	obj->minFilter = GL_NEAREST;	// ???
	obj->magFilter = GL_NEAREST;	// ???
	obj->priority = 1.0f;
	obj->hasMipmaps = GL_FALSE;
	obj->isValid = GL_FALSE;
	obj->activeLevels = 0;
	obj->usr_vp1 = 0;
	obj->usr_vp2 = 0;
	obj->usr_i1 = 0;
	obj->usr_i2 = 0;
	
	for( ct=0; ct<RAS_MAX_MIPMAP_LEVEL; ct++ )
	{
		obj->level[ct].border = 0;
		obj->level[ct].data = 0;
		obj->level[ct].width = 0;
		obj->level[ct].height = 0;
		obj->level[ct].usr_vp1 = 0;
		obj->level[ct].usr_vp2 = 0;
		obj->level[ct].usr_i1 = 0;
		obj->level[ct].usr_i2 = 0;
	}
	
	DEBUG_END_FUNCTION( "__initTextureObject" );
}

void __glPickActiveTexture( __glContext *gc )
{
	int ct;
	DEBUG_BEGIN_FUNCTION( "__glPickActiveTexture" );

	gc->flingerCurrent = 0;

	for( ct=0; ct<4; ct++ )
	{
		if( gc->state.texture.Enabled2D[ct] )
		{
			if( gc->state.texture.Bound[ct][__GL_TEXTURE_INDEX_2D]->isValid )
			{
				if( !gc->texture.Enabled[ct] )
				{
					gc->valid.VapiProcessor = 1;
					gc->valid.All = 1;
				}
				gc->texture.Enabled[ct] = GL_TRUE;
				gc->texture.Active[ct] = gc->state.texture.Bound[ct][__GL_TEXTURE_INDEX_2D];
				continue;
			}
		}
	
		if( gc->state.texture.Enabled1D[ct] )
		{
			if( gc->state.texture.Bound[ct][__GL_TEXTURE_INDEX_1D]->isValid )
			{
				if( !gc->texture.Enabled[ct] )
				{
					gc->valid.VapiProcessor = 1;
					gc->valid.All = 1;
				}
				gc->texture.Enabled[ct] = GL_TRUE;
				gc->texture.Active[ct] = gc->state.texture.Bound[ct][__GL_TEXTURE_INDEX_1D];
				continue;
			}
		}
	
		if( gc->texture.Enabled[ct] )
		{
			gc->valid.VapiProcessor = 1;
			gc->valid.All = 1;
		}
		gc->texture.Enabled[ct] = GL_FALSE;
		gc->texture.Active[ct] = 0;
	}
	
	DEBUG_END_FUNCTION( "__glPickActiveTexture" );
}

void __glValidateTexture( __glContext *gc, __glTexture *tex )
{
	GLint temp, ct, h, w, p;
	DEBUG_BEGIN_FUNCTION( "__glValidateTexture" );
	
	w = tex->level[0].width;
	h = tex->level[0].height;
	if( !(w && h) )
	{
		// A level 0 axis is zero.
		tex->isValid = GL_FALSE;
		DEBUG_END_FUNCTION( "__glValidateTexture" );
		return;
	}
	
	if( (tex->minFilter == GL_LINEAR) || (tex->minFilter == GL_NEAREST) )
	{
		tex->activeLevels = 1;
		tex->isValid = GL_TRUE;
		return;
	}
	
	for( ct=1; ct<__GL_MAX_MIPMAP_LEVEL; ct++ )
	{
		if( tex->level[ct].width || tex->level[ct].height )
			break;
	}
	
	if( ct == __GL_MAX_MIPMAP_LEVEL )
	{
		// We only had the one level (level zero).
		tex->isValid = GL_TRUE;
		tex->hasMipmaps = GL_FALSE;
		tex->activeLevels = 1;
		DEBUG_END_FUNCTION( "__glValidateTexture" );
		return;
	}
	tex->hasMipmaps = GL_TRUE;
	
	temp = w;
	if( h > temp )
		temp = h;
	
	p=0;
	while( temp )
	{
		p++;
		temp >>= 1;
	}
	if( p < 2 )
	{
		// Too small to have mipmaps
		tex->isValid = GL_FALSE;
		DEBUG_END_FUNCTION( "__glValidateTexture" );
		return;
	}
	
	for( ct=1; ct < p; ct++ ) 
	{
		if( w>1 )
			w >>= 1;
		if( h>1 )
			h >>= 1;
		if( (tex->level[ct].width != w ) ||	(tex->level[ct].height != h ) )
		{
			//The sizes do not match
			tex->isValid = GL_FALSE;
			DEBUG_END_FUNCTION( "__glValidateTexture" );
			return;
		}
	}
	tex->activeLevels = p;
	tex->isValid = GL_TRUE;
	DEBUG_END_FUNCTION( "__glValidateTexture" );
}


void __glInitTextureState( __glContext *gc )
{
	int ct, ct2;
	DEBUG_BEGIN_FUNCTION( "__glInitTextureState" );
	__hmap_Init( &gc->texture.Objects );

	for( ct=0; ct<__GL_NUMBER_OF_TEXTURE_TARGETS; ct++ )
	{
		__initTextureObject( &gc->texture.Defaults[ct] );
		for( ct2=0; ct2<__GL_MAX_TMU_COUNT; ct2++ )
			gc->state.texture.Bound[ct2][ct] = &gc->texture.Defaults[ct];
	}
	gc->texture.Defaults[__GL_TEXTURE_INDEX_1D].dim = 1;
	gc->texture.Defaults[__GL_TEXTURE_INDEX_2D].dim = 2;
	gc->texture.Defaults[__GL_TEXTURE_INDEX_3D].dim = 3;
	gc->texture.Defaults[__GL_PROXY_TEXTURE_INDEX_1D].dim = 1;
	gc->texture.Defaults[__GL_PROXY_TEXTURE_INDEX_2D].dim = 2;
	gc->texture.Defaults[__GL_PROXY_TEXTURE_INDEX_3D].dim = 3;
	
	__glPickActiveTexture( gc );
	DEBUG_END_FUNCTION( "__glInitTextureState" );
}


void __glBindTexture(__glContext * gc, GLuint targetIndex, GLuint name)
{
	__glTexture *texObj;
	DEBUG_BEGIN_FUNCTION( "__glBindTexture" );
	
	__GL_CHECK_NOT_IN_BEGIN ();

	texObj = (__glTexture *) __hmap_Lookup( &gc->texture.Objects, name );

	if( !texObj )
	{
		/* The texture object does't exist so we must create it. */
		texObj = (__glTexture *) __glMalloc( gc, sizeof( __glTexture ));
		memset( texObj, 0, sizeof( __glTexture ));
		if( !texObj )
		{
			/* Memory error */
			__glSetError (gc, GL_OUT_OF_MEMORY);
			DEBUG_END_FUNCTION( "__glBindTexture" );
			return;
		}
		if( !__hmap_Add( &gc->texture.Objects, name, texObj ) )
		{
			/* Hashmap error, most likely ran out of memory */
			__glFree( gc, texObj );
			__glSetError (gc, GL_OUT_OF_MEMORY);
			DEBUG_END_FUNCTION( "__glBindTexture" );
			return;
		}
		__initTextureObject( texObj );
		texObj->dim = targetIndex +1;
		texObj->name = name;
	}
	else
	{
		if( texObj->dim != (targetIndex+1) )
		{
			/* Texture dimensions do not match, set GL_INVALID_OPERATION error */
			__glSetError (gc, GL_INVALID_OPERATION);
			DEBUG_END_FUNCTION( "__glBindTexture" );
			return;
		}
		if( texObj->name != name )
			printf( "GLDebug libGL2: Texture Lookup error !!!!!  Tell Jason \n" );
	}
	
	gc->software.ProcsCurrent = GL_FALSE;
	gc->state.texture.Bound[gc->state.texture.SelectedUnit][targetIndex] = texObj;
	__glPickActiveTexture( gc );
	DRIVERPROC_TEXTURE_SELECT(gc, texObj);

	DEBUG_END_FUNCTION( "__glBindTexture" );
}

GLboolean __glim_AreTexturesResident( __glContext *gc, GLsizei count, const GLuint *names, GLboolean *resident )
{
	DEBUG_BEGIN_FUNCTION( "__glim_AreTexturesResident" );
	DEBUG_END_FUNCTION( "__glim_AreTexturesResident" );
	// HACK
	return GL_TRUE;
}

void __glim_BindTexture( __glContext *gc, GLenum target, GLuint name )
{
	DEBUG_BEGIN_FUNCTION( "__glim_BindTexture" );
	__GL_CHECK_NOT_IN_BEGIN ();
	if( (target < GL_TEXTURE_1D) || (target > GL_TEXTURE_2D) )
	{
		__glSetError (gc, GL_INVALID_OPERATION);
		DEBUG_END_FUNCTION( "__glim_BindTexture" );
		return;
	}
	__glBindTexture( gc, target - GL_TEXTURE_1D, name );
	DEBUG_END_FUNCTION( "__glim_BindTexture" );
}

void __glim_DeleteTextures( __glContext *gc, GLsizei count, const GLuint *names )
{
	GLint ct;
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__glim_DeleteTextures" );
	__GL_CHECK_NOT_IN_BEGIN ();
	
	for( ct=0; ct<count; ct++ )
	{
		tex = (__glTexture *) __hmap_Lookup( &gc->texture.Objects, names[ct] );
		if( tex )
		{
			GLint ct2;
			
			/* We must delete the driver copy first. */
			if( gc->procs.textureDelete )
				(*gc->procs.textureDelete)( gc, tex );

			__hmap_Remove( &gc->texture.Objects, tex->name );
			for( ct2=0; ct2<__GL_MAX_MIPMAP_LEVEL; ct2++ )
			{
				if( tex->level[ct2].data )
				{
//printf( "Freeing texture name=%i  level=%i \n", tex->name, ct2 );
					free( tex->level[ct2].data );
				}
			}
			free( tex );	
		}
	}
	DEBUG_END_FUNCTION( "__glim_DeleteTextures" );
}


void __glim_GenTextures( __glContext *gc, GLsizei count, GLuint *names)
{
	GLint ct;
	DEBUG_BEGIN_FUNCTION( "__glim_GenTextures" );
	__GL_CHECK_NOT_IN_BEGIN ();
	
	for( ct=0; ct<count; ct++ )
	{
		names[ct] = ++gc->texture.lastName;
	}
	
	DEBUG_END_FUNCTION( "__glim_GenTextures" );
}

GLboolean __glim_IsTexture( __glContext *gc, GLuint name )
{
	DEBUG_BEGIN_FUNCTION( "__glim_IsTexture" );
	__GL_CHECK_NOT_IN_BEGIN2 ();
	DEBUG_END_FUNCTION( "__glim_IsTexture" );
	return GL_FALSE;
}

void __glim_PrioritizeTextures( __glContext *gc, GLsizei count, const GLuint *names, const GLclampf *value)
{
	DEBUG_BEGIN_FUNCTION( "__glim_PrioritizeTextures" );
	__GL_CHECK_NOT_IN_BEGIN ();
	DEBUG_END_FUNCTION( "__glim_PrioritizeTextures" );
}

void __glim_TexParameterf( __glContext *gc, GLenum target, GLenum pname, GLfloat value )
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexParameterf" );
	__GL_CHECK_NOT_IN_BEGIN ();

	/* Accept only enumerants that correspond to single values */
	switch (pname)
	{
	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_PRIORITY:
		__glim_TexParameterfv (gc, target, pname, &value);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexParameterf" );
		return;
	}
	DEBUG_END_FUNCTION( "__glim_TexParameterf" );
}

void __glim_TexParameterfv( __glContext *gc, GLenum target, GLenum pname, const GLfloat *pv)
{
	__glTexture *tex;
	GLenum e;

	DEBUG_BEGIN_FUNCTION( "__glim_TexParameterfv" );
	__GL_CHECK_NOT_IN_BEGIN ();

	tex = __glLookUpTexture (gc, target);
	if ( NULL == tex )
	{
	  bad_enum:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexParameterfv" );
		return;
	}

	switch (pname)
	{
	case GL_TEXTURE_WRAP_S:
		switch (e = (GLenum) pv[0])
		{
		case GL_REPEAT:
		case GL_CLAMP:
			tex->sWrapMode = e;
			DRIVERPROC_TEXTURE_S_WRAP_MODE(gc, tex);
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_WRAP_T:
		switch (e = (GLenum) pv[0])
		{
		case GL_REPEAT:
		case GL_CLAMP:
			tex->tWrapMode = e;
			DRIVERPROC_TEXTURE_T_WRAP_MODE(gc, tex);
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_MIN_FILTER:
		switch (e = (GLenum) pv[0])
		{
		case GL_NEAREST:
		case GL_LINEAR:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			{
				int32 oldLevels = tex->activeLevels;
				tex->minFilter = e;
				__glValidateTexture( gc, tex );
				DRIVERPROC_TEXTURE_MIN_FILTER(gc, tex);
				if( tex->activeLevels != oldLevels )
				{
					if( gc->procs.textureReloadMinLevels )
						(*gc->procs.textureReloadMinLevels)( gc, tex );
				}
				break;
			}
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_MAG_FILTER:
		switch (e = (GLenum) pv[0])
		{
		case GL_NEAREST:
		case GL_LINEAR:
			tex->magFilter = e;
			DRIVERPROC_TEXTURE_MAG_FILTER(gc, tex);
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_BORDER_COLOR:
		CLAMP_GE0_LE1_V4_2( &tex->borderColor.R, pv );
		break;
	case GL_TEXTURE_PRIORITY:
		tex->priority = pv[0];
		if( tex->priority < 0 )
			tex->priority = 0;
		if( tex->priority > 1 )
			tex->priority = 1;
		break;

	default:
		goto bad_enum;
	}
	DEBUG_END_FUNCTION( "__glim_TexParameterfv" );
	gc->software.ProcsCurrent = GL_FALSE;
}

void __glim_TexParameteri( __glContext *gc, GLenum target, GLenum pname, GLint value)
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexParameteri" );
	__GL_CHECK_NOT_IN_BEGIN ();

	/* Accept only enumerants that correspond to single values */
	switch (pname)
	{
	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
	case GL_TEXTURE_PRIORITY:
		__glim_TexParameteriv (gc, target, pname, &value);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexParameteri" );
		return;
	}
	DEBUG_END_FUNCTION( "__glim_TexParameteri" );
}

void __glim_TexParameteriv( __glContext *gc, GLenum target, GLenum pname, const GLint *pv)
{
	__glTexture *tex;
	GLenum e;

	DEBUG_BEGIN_FUNCTION( "__glim_TexParameteriv" );
	__GL_CHECK_NOT_IN_BEGIN ();

	tex = __glLookUpTexture (gc, target);
	if ( NULL == tex )
	{
	  bad_enum:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexParameteriv" );
		return;
	}

	switch (pname)
	{
	case GL_TEXTURE_WRAP_S:
		switch (e = (GLenum) pv[0])
		{
		case GL_REPEAT:
		case GL_CLAMP:
			tex->sWrapMode = e;
			DRIVERPROC_TEXTURE_S_WRAP_MODE(gc, tex);
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_WRAP_T:
		switch (e = (GLenum) pv[0])
		{
		case GL_REPEAT:
		case GL_CLAMP:
			tex->tWrapMode = e;
			DRIVERPROC_TEXTURE_T_WRAP_MODE(gc, tex);
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_MIN_FILTER:
		switch (e = (GLenum) pv[0])
		{
		case GL_NEAREST:
		case GL_LINEAR:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			{
				int32 oldLevels = tex->activeLevels;
				tex->minFilter = e;
				__glValidateTexture( gc, tex );
				DRIVERPROC_TEXTURE_MIN_FILTER(gc, tex);
				if( tex->activeLevels != oldLevels )
				{
					if( gc->procs.textureReloadMinLevels )
						(*gc->procs.textureReloadMinLevels)( gc, tex );
				}
				break;
			}
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_MAG_FILTER:
		switch (e = (GLenum) pv[0])
		{
		case GL_NEAREST:
		case GL_LINEAR:
			tex->magFilter = e;
			DRIVERPROC_TEXTURE_MAG_FILTER(gc, tex);
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_BORDER_COLOR:
		__glClampColori( gc, &tex->borderColor.R, pv );
		break;
	case GL_TEXTURE_PRIORITY:
		tex->priority = __GL_I_TO_FLOAT( pv[0] );
		if( tex->priority < 0 )
			tex->priority = 0;
		if( tex->priority > 1 )
			tex->priority = 1;
		break;

	default:
		goto bad_enum;
	}

	//(*gc->methods.pickAllProcs) (gc);
	DEBUG_END_FUNCTION( "__glim_TexParameteriv" );
}

_STATIC_ void setNullLevel( __glContext *gc, __glTexture *tex, GLuint level )
{
	__glTextureLevel *l = &tex->level[level];
	l->border = 0;
	if( l->data )
		FREE( l->data );
	l->data = 0;
	l->width = 0;
	l->height = 0;
	l->width2 = 0;
	l->height2 = 0;
	l->allocationSize = 0;
	l->sizeR = 0;	
	l->sizeG = 0;	
	l->sizeB = 0;	
	l->sizeA = 0;	
	l->sizeL = 0;	
	l->sizeI = 0;
}

_STATIC_ void setInternalFormat( __glContext *gc, __glTextureLevel *l )
{
	l->sizeR = 0;
	l->sizeG = 0;
	l->sizeB = 0;
	l->sizeA = 0;
	l->sizeL = 0;
	l->sizeI = 0;
	switch (l->requestedFormat)
	{
	case 1:
	case GL_LUMINANCE:
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
		l->baseFormat = GL_LUMINANCE;
		l->internalFormat = GL_LUMINANCE;
		l->internalType = GL_UNSIGNED_BYTE;
		l->bytesPerTexel = 1;
		l->sizeL = 8;
		break;

	case GL_ALPHA:
	case GL_ALPHA4:
	case GL_ALPHA8:
	case GL_ALPHA12:
	case GL_ALPHA16:
		l->baseFormat = GL_ALPHA;
		l->internalFormat = GL_ALPHA;
		l->internalType = GL_UNSIGNED_BYTE;
		l->bytesPerTexel = 1;
		l->sizeA = 8;
		break;

	case GL_INTENSITY:
	case GL_INTENSITY4:
	case GL_INTENSITY8:
	case GL_INTENSITY12:
	case GL_INTENSITY16:
		l->baseFormat = GL_INTENSITY;
		l->internalFormat = GL_INTENSITY;
		l->internalType = GL_UNSIGNED_BYTE;
		l->bytesPerTexel = 1;
		l->sizeI = 8;
		break;

	case 2:
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE8_ALPHA8:
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
		l->baseFormat = GL_LUMINANCE_ALPHA;
		l->internalFormat = GL_LUMINANCE_ALPHA;
		l->internalType = GL_UNSIGNED_BYTE;
		l->bytesPerTexel = 2;
		l->sizeL = 8;
		l->sizeA = 8;
		break;

	case 3:
	case GL_RGB:
	case GL_R3_G3_B2:
	case GL_RGB4:
	case GL_RGB5:
	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
		l->baseFormat = GL_RGB;
		l->internalFormat = GL_BGR;
		l->internalType = GL_UNSIGNED_BYTE;
		l->bytesPerTexel = 3;
		l->sizeR = 8;
		l->sizeG = 8;
		l->sizeB = 8;
		break;

	case 4:
	case GL_RGBA:
	case GL_RGBA2:
	case GL_RGBA4:
	case GL_RGBA8:
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGB5_A1:
	case GL_RGB10_A2:
		l->baseFormat = GL_RGBA;
		l->internalFormat = GL_BGRA;
		l->internalType = GL_UNSIGNED_BYTE;
		l->bytesPerTexel = 4;
		l->sizeR = 8;
		l->sizeG = 8;
		l->sizeB = 8;
		l->sizeA = 8;
		break;
	}
	
}

void __glim_TexImage3D( __glContext *gc, GLenum target, GLint level, GLint internalFormat,
	GLsizei w, GLsizei h, GLsizei depth, GLint border, GLenum srcFormat, GLenum type, const GLvoid *data )
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexImage3D" );
	__GL_CHECK_NOT_IN_BEGIN ();

	gc->flingerCurrent = 0;

	DEBUG_END_FUNCTION( "__glim_TexImage3D" );
}

void __glim_TexImage2D( __glContext *gc, GLenum target, GLint level, GLint internalFormat,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data )
{
	__glTexture *tex;
	GLint driverOwned = 0;

	DEBUG_BEGIN_FUNCTION( "__glim_TexImage2D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	tex = __glCheckTexImage2DArgs (gc, target, level, internalFormat, width, height, border, format, type);

	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_TexImage2D" );
		return;
	}

	tex->level[level].surface = 0;
	tex->level[level].requestedFormat = internalFormat;
	tex->level[level].height = height;
	tex->level[level].height2 = height - border*2;
	tex->level[level].width = width;
	tex->level[level].width2 = width - border*2;
	tex->level[level].border = border;
	setInternalFormat(gc, &tex->level[level] );

	if( target == GL_PROXY_TEXTURE_2D )
	{
		//Proxy
		tex->level[level].data = 0;
		
		if( gc->procs.textureProxyGetType2D )
		{
			if( (*gc->procs.textureProxyGetType2D)(gc, tex, &tex->level[level] ) )
			{
				// Texture is ok
			}
			else
			{
				// Texture is not ok.
				tex->level[level].height = 0;
				tex->level[level].height2 = 0;
				tex->level[level].width = 0;
				tex->level[level].width2 = 0;
				tex->level[level].border = 0;
				tex->level[level].baseFormat = 0;
			}
		}
		else
		{
			// Software check.
			if( (width > gc->info.textureMaxS) || (height > gc->info.textureMaxT) )
			{
				// Texture is not ok.
				tex->level[level].height = 0;
				tex->level[level].height2 = 0;
				tex->level[level].width = 0;
				tex->level[level].width2 = 0;
				tex->level[level].border = 0;
				tex->level[level].baseFormat = 0;
			}
		}

		return;
	}

	if( gc->procs.textureGetType2D )
	{
		// Call the driver to set the internalType and internalFormat.
		// If this function returns true the driver is going to handle all of the
		// storage and extraction.
		driverOwned = (*gc->procs.textureGetType2D)(gc, tex, &tex->level[level] );
	}
	
	if( driverOwned )
	{
		// The driver owns this one so free any previous allocations;
		if( tex->level[level].data )
		{
			FREE( tex->level[level].data );
			tex->level[level].data = 0;
		}
		tex->level[level].allocationSize = 0;
	}
	else
	{
		GLint size;

		// We need some memory here.
		size = tex->level[level].height * tex->level[level].width * tex->level[level].bytesPerTexel;
		if( size != tex->level[level].allocationSize )
		{
			if( size )
			{
				if( tex->level[level].data )
				{
//printf( "Freeing texture name=%i  level=%i \n", tex->name, level );
					FREE( tex->level[level].data );
					tex->level[level].data = 0;
				}
//printf( "Allocating texture name=%i  level=%i \n", tex->name, level );
				tex->level[level].data = MALLOC( size );
				tex->level[level].allocationSize = size;
			}
			else
			{
				setNullLevel( gc, tex, level );
			}
		}

		if( data )
		{
	//		setupConverter( gc );
			cvSetType( gc->flingerContext, B_BIT_IN, type, format,
				gc->state.pixel.unpackModes.SwapEndian, gc->state.pixel.unpackModes.LsbFirst );
			cvSetType( gc->flingerContext, B_BIT_OUT, tex->level[level].internalType, tex->level[level].internalFormat, 0, 0 );
			cvConvert( gc->flingerContext, width, height, 0, 0, 0, data, 0, 0, 0, tex->level[level].data );
	
			//(*state->convertPixels) ( state, width, height, data, 0, 0, tex->level[level].data, 0, 0 );
	//		cviPixelConverter_C ( state, data, width, height, 0, 0, 0, tex->level[level].data, 0, 0, 0 );
		}
	}
	
	__glValidateTexture( gc, tex );
	__glPickActiveTexture( gc );

	if( gc->procs.textureImage2D )
	{
		(*gc->procs.textureImage2D)( gc, tex, level, data );
	}

	gc->flingerCurrent = 0;

	DEBUG_END_FUNCTION( "__glim_TexImage2D" );
}


void __glim_TexImage1D( __glContext *gc, GLenum target, GLint level, GLint internalFormat,
	GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *data )
{
	__glTexture *tex;
	GLint driverOwned = 0;

	DEBUG_BEGIN_FUNCTION( "__glim_TexImage1D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	tex = __glCheckTexImage1DArgs (gc, target, level, internalFormat, width, border, format, type);

	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_TexImage1D" );
		return;
	}

	tex->level[level].requestedFormat = internalFormat;
	tex->level[level].height = 1 + border*2;
	tex->level[level].height2 = 1;
	tex->level[level].width = width;
	tex->level[level].width2 = width - border*2;
	tex->level[level].border = border;

	if( gc->procs.textureGetType1D )
	{
		// Call the driver to set the internalType and internalFormat.
		// If this function returns true the driver is going to handle all of the
		// storage and extraction.
		driverOwned = (*gc->procs.textureGetType1D)(gc, tex, &tex->level[level] );
	}
	else
	{
		// We must pick the internalFormat and internalType here.
		setInternalFormat(gc, &tex->level[level] );
	}
	
	if( driverOwned )
	{
		// The driver owns this one so free any previous allocations;
		if( tex->level[level].data )
		{
			FREE( tex->level[level].data );
			tex->level[level].data = 0;
		}
		tex->level[level].allocationSize = 0;
	}
	else
	{
		// We need some memory here.
		GLint size = tex->level[level].width * tex->level[level].bytesPerTexel;

		if( size != tex->level[level].allocationSize )
		{
			if( size )
			{
				tex->level[level].data = MALLOC( size );
				tex->level[level].allocationSize = size;
			}
			else
			{
				setNullLevel( gc, tex, level );
			}
		}

		cvSetType( gc->flingerContext, B_BIT_IN, type, format,
			gc->state.pixel.unpackModes.SwapEndian, gc->state.pixel.unpackModes.LsbFirst );
		cvSetType( gc->flingerContext, B_BIT_OUT, tex->level[level].internalType, tex->level[level].internalFormat, 0, 0 );
		cvConvert( gc->flingerContext, width, 1, 0, 0, 0, data, 0, 0, 0, tex->level[level].data );
	}

	__glValidateTexture( gc, tex );
	__glPickActiveTexture( gc );

	if( gc->procs.textureImage1D )
	{
		(*gc->procs.textureImage1D)( gc, tex, level, data );
	}

	gc->flingerCurrent = 0;

	DEBUG_END_FUNCTION( "__glim_TexImage2D" );
}


void __glim_CopyTexImage2D ( __glContext *gc, GLenum target, GLint level, GLenum internalFormat,
					   GLint x, GLint y, GLsizei w, GLsizei h, GLint border)
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__glim_CopyTexImage2D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	tex = __glCheckTexImage2DArgs (gc, target, level, internalFormat, w, h,
								   border, GL_RGBA, GL_FLOAT);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_CopyTexImage2D" );
		return;
	}
	DEBUG_END_FUNCTION( "__glim_CopyTexImage2D" );
}

void __glim_CopyTexImage1D ( __glContext *gc, GLenum target, GLint level, GLenum internalFormat,
					   GLint x, GLint y, GLsizei width, GLint border)
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__glim_CopyTexImage1D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	tex = __glCheckTexImage1DArgs (gc, target, level, internalFormat, width,
								   border, GL_RGB, GL_FLOAT);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_CopyTexImage1D" );
		return;
	}
	DEBUG_BEGIN_FUNCTION( "__glim_CopyTexImage1D" );

}

void __glim_TexSubImage3D( __glContext *gc, GLenum target, GLint level,
	GLint xoff, GLint yoff, GLint zoff, GLsizei width, GLsizei height, GLsizei depth,
	GLenum format, GLenum type, const GLvoid *data )
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexSubImage3D" );
	__GL_CHECK_NOT_IN_BEGIN ();
	DEBUG_END_FUNCTION( "__glim_TexSubImage3D" );
}

void __glim_TexSubImage2D( __glContext *gc, GLenum target, GLint level,
	GLint xoff, GLint yoff, GLsizei width, GLsizei height,
	GLenum format, GLenum type, const GLvoid *data )
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__glim_TexSubImage2D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	/* Check arguments and get the right texture level being changed */
	tex = __glCheckTexSubImage2DArgs (gc, target, level, xoff, yoff, width, height, format, type);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_TexSubImage2D" );
		return;
	}

	if( tex->level[level].data )
	{
		// We are manageing the texture data.

		cvSetType( gc->flingerContext, B_BIT_IN, type, format, gc->state.pixel.unpackModes.SwapEndian, gc->state.pixel.unpackModes.LsbFirst );
		cvSetType( gc->flingerContext, B_BIT_OUT, tex->level[level].internalType, tex->level[level].internalFormat, 0, 0 );

		//status_t cvConvert( void *context, int32 width, int32 height,
		//			int32 srcSkipPixels, int32 srcSkipRows, int32 srcRowLength, const void *srcData, 
		//			int32 dstSkipPixels, int32 dstSkipRows, int32 dstRowLength, void *dstData );
//printf( "\nCalling cvConvert \n" );
//printf( "  width=%i  height = %i \n", width, height );
//printf( "  skipPixels=%i  skipLines=%i  lineLength=%i \n", gc->state.pixel.unpackModes.SkipPixels, gc->state.pixel.unpackModes.SkipLines, gc->state.pixel.unpackModes.LineLength );
//printf( "  xoff=%i  yoff=%i   dstLineLen=%i \n", xoff, yoff, tex->level[level].width );
		cvConvert( gc->flingerContext, width, height,
					gc->state.pixel.unpackModes.SkipPixels, gc->state.pixel.unpackModes.SkipLines, gc->state.pixel.unpackModes.LineLength, data,
					xoff, yoff, tex->level[level].width * tex->level[level].bytesPerTexel, tex->level[level].data );
	}
	else
	{
		// The driver is doing the management.
	}

	if( gc->procs.textureSubImage2D )
	{
		(*gc->procs.textureSubImage2D)( gc, tex, level, xoff, yoff, width, height, data );
	}
	
	DEBUG_BEGIN_FUNCTION( "__glim_TexSubImage2D" );
}

void __glim_TexSubImage1D( __glContext *gc, GLenum target, GLint level,
	GLint xoff, GLsizei width,
	GLenum format, GLenum type, const GLvoid *data )
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__glim_TexSubImage1D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	/* Check arguments and get the right texture level being changed */
	tex = __glCheckTexSubImage1DArgs (gc, target, level, xoff, width, format, type);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_TexSubImage1D" );
		return;
	}

	if( tex->level[level].data )
	{
		// We are manageing the texture data.
		cvSetType( gc->flingerContext, B_BIT_IN, type, format, gc->state.pixel.unpackModes.SwapEndian, gc->state.pixel.unpackModes.LsbFirst );
		cvSetType( gc->flingerContext, B_BIT_OUT, tex->level[level].internalType, tex->level[level].internalFormat, 0, 0 );

		//status_t cvConvert( void *context, int32 width, int32 height,
		//			int32 srcSkipPixels, int32 srcSkipRows, int32 srcRowLength, const void *srcData, 
		//			int32 dstSkipPixels, int32 dstSkipRows, int32 dstRowLength, void *dstData );
		cvConvert( gc->flingerContext, width, 1,
					gc->state.pixel.unpackModes.SkipPixels, 0, 0, data,
					0, 0, 0, tex->level[level].data );
	}
	else
	{
		// The driver is doing the management.
	}

	if( gc->procs.textureSubImage1D )
	{
		(*gc->procs.textureSubImage1D)( gc, tex, level, xoff, width, data );
	}

	DEBUG_END_FUNCTION( "__glim_TexSubImage1D" );
}

void __glim_CopyTexSubImage3D( __glContext *gc, GLenum target, GLint level,
	GLint xoff, GLint yoff, GLint zoff, GLint x, GLint y, GLint width, GLsizei height )
{
	DEBUG_BEGIN_FUNCTION( "__glim_CopyTexSubImage3D" );
	__GL_CHECK_NOT_IN_BEGIN ();
	DEBUG_END_FUNCTION( "__glim_CopyTexSubImage3D" );
}

void __glim_CopyTexSubImage2D( __glContext *gc, GLenum target, GLint level,
	GLint xoff, GLint yoff, GLint x, GLint y, GLint width, GLsizei height )
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__glim_CopyTexSubImage2D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	/* Check arguments and get the right texture level being changed */
	tex = __glCheckTexSubImage2DArgs (gc, target, level, xoff, yoff, width, height, GL_RGBA, GL_FLOAT);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_CopyTexSubImage2D" );
		return;
	}
	DEBUG_END_FUNCTION( "__glim_CopyTexSubImage2D" );
}

void __glim_CopyTexSubImage1D( __glContext *gc, GLenum target, GLint level,
	GLint xoff, GLint x, GLint y, GLint width )
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__glim_CopyTexSubImage1D" );

	__GL_CHECK_NOT_IN_BEGIN ();
	/* Check arguments and get the right texture level being changed */
	tex = __glCheckTexSubImage1DArgs (gc, target, level, xoff, width,
									  GL_RGBA, GL_FLOAT);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__glim_CopyTexSubImage1D" );
		return;
	}
	DEBUG_BEGIN_FUNCTION( "__glim_CopyTexSubImage1D" );
}

void __glim_TexEnvfv ( __glContext *gc, GLenum target, GLenum pname, const GLfloat pv[])
{
//	__glTextureEnvState *tes;
	GLenum e;

	DEBUG_BEGIN_FUNCTION( "__glim_TexEnvfv" );
	__GL_CHECK_NOT_IN_BEGIN ();

	target -= GL_TEXTURE_ENV;
	if ((target < 0) || (target >= __GL_NUMBER_OF_TEXTURE_ENVS))
	{
	  bad_enum:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexEnvfv" );
		return;
	}

	switch (pname)
	{
	case GL_TEXTURE_ENV_MODE:
		switch (e = (GLenum) pv[0])
		{
		case GL_MODULATE:
		case GL_DECAL:
		case GL_BLEND:
		case GL_REPLACE:
			gc->state.texture.EnvMode[gc->state.texture.SelectedUnit] = e;
			DRIVERPROC_TEXTURE_ENV_MODE( gc );
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_ENV_COLOR:
		CLAMP_GE0_LE1_V4_2( &gc->state.texture.EnvColor[gc->state.texture.SelectedUnit].R, pv );
		DRIVERPROC_TEXTURE_ENV_COLOR( gc );
		break;
	default:
		goto bad_enum;
	}
	//(*gc->methods.pickAllProcs) (gc);
	DEBUG_END_FUNCTION( "__glim_TexEnvfv" );
}

void __glim_TexEnvf ( __glContext *gc, GLenum target, GLenum pname, GLfloat f)
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexEnvf" );
	__GL_CHECK_NOT_IN_BEGIN ();

	/* Accept only enumerants that correspond to single values */
	switch (pname)
	{
	case GL_TEXTURE_ENV_MODE:
		__glim_TexEnvfv (gc, target, pname, &f);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		return;
	}
	DEBUG_END_FUNCTION( "__glim_TexEnvf" );
}

void __glim_TexEnviv ( __glContext *gc, GLenum target, GLenum pname, const GLint pv[])
{
//	__glTextureEnvState *tes;
	GLenum e;

	DEBUG_BEGIN_FUNCTION( "__glim_TexEnviv" );
	__GL_CHECK_NOT_IN_BEGIN ();

	target -= GL_TEXTURE_ENV;
	if ((target < 0) || (target >= __GL_NUMBER_OF_TEXTURE_ENVS))
	{
	  bad_enum:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexEnviv" );
		return;
	}

	switch (pname)
	{
	case GL_TEXTURE_ENV_MODE:
		switch (e = (GLenum) pv[0])
		{
		case GL_MODULATE:
		case GL_DECAL:
		case GL_BLEND:
		case GL_REPLACE:
			gc->state.texture.EnvMode[gc->state.texture.SelectedUnit] = e;
			DRIVERPROC_TEXTURE_ENV_MODE( gc );
			break;
		default:
			goto bad_enum;
		}
		break;
	case GL_TEXTURE_ENV_COLOR:
		__glClampAndScaleColori (gc, &gc->state.texture.EnvColor[gc->state.texture.SelectedUnit].R, pv);
		DRIVERPROC_TEXTURE_ENV_COLOR( gc );
		break;
	default:
		goto bad_enum;
	}
	//(*gc->methods.pickAllProcs) (gc);
	DEBUG_END_FUNCTION( "__glim_TexEnviv" );
}

void __glim_TexEnvi ( __glContext *gc, GLenum target, GLenum pname, GLint i)
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexEnvi" );
	__GL_CHECK_NOT_IN_BEGIN ();

	/* Accept only enumerants that correspond to single values */
	switch (pname)
	{
	case GL_TEXTURE_ENV_MODE:
		__glim_TexEnviv (gc, target, pname, &i);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexEnvi" );
		return;
	}
	DEBUG_END_FUNCTION( "__glim_TexEnvi" );
}

/************************************************************************/

void __glim_TexGenfv ( __glContext *gc, GLenum coord, GLenum pname, const GLfloat pv[])
{
	__glTextureCoordState *tcs;
	__glFloat v[4];
	__glMatrix *m;

	DEBUG_BEGIN_FUNCTION( "__glim_TexGenfv" );
	__GL_CHECK_NOT_IN_BEGIN ();

	switch (coord)
	{
	case GL_S:
		tcs = &gc->state.texture.s[gc->state.texture.SelectedUnit];
		break;
	case GL_T:
		tcs = &gc->state.texture.t[gc->state.texture.SelectedUnit];
		break;
	case GL_R:
		tcs = &gc->state.texture.r[gc->state.texture.SelectedUnit];
		break;
	case GL_Q:
		tcs = &gc->state.texture.q[gc->state.texture.SelectedUnit];
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexGenfv" );
		return;
	}
	switch (pname)
	{
	case GL_TEXTURE_GEN_MODE:
		switch ((GLenum) pv[0])
		{
		case GL_EYE_LINEAR:
		case GL_OBJECT_LINEAR:
			tcs->mode = (GLenum) pv[0];
			break;
		case GL_SPHERE_MAP:
			if ((coord == GL_R) || (coord == GL_Q))
			{
				__glSetError (gc, GL_INVALID_ENUM);
				DEBUG_END_FUNCTION( "__glim_TexGenfv" );
				return;
			}
			tcs->mode = (GLenum) pv[0];
			break;
		default:
			__glSetError (gc, GL_INVALID_ENUM);
			DEBUG_END_FUNCTION( "__glim_TexGenfv" );
			return;
		}
		break;
	case GL_OBJECT_PLANE:
		tcs->objectPlaneEquation.X = pv[0];
		tcs->objectPlaneEquation.Y = pv[1];
		tcs->objectPlaneEquation.Z = pv[2];
		tcs->objectPlaneEquation.W = pv[3];
		break;
	case GL_EYE_PLANE:
		v[0] = pv[0];
		v[1] = pv[1];
		v[2] = pv[2];
		v[3] = pv[3];
		validateAll(gc);
		m = &gc->transform.modelView->inverseTranspose;
		mathVector4XMatrix (&tcs->eyePlaneEquation.X, v, m->matrix);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexGenfv" );
		return;
	}
	//(*gc->methods.pickAllProcs) (gc);
	DEBUG_END_FUNCTION( "__glim_TexGenfv" );
}

void __glim_TexGenf ( __glContext *gc, GLenum coord, GLenum pname, GLfloat f)
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexGenf" );
	__GL_CHECK_NOT_IN_BEGIN ();

	/* Accept only enumerants that correspond to single values */
	switch (pname)
	{
	case GL_TEXTURE_GEN_MODE:
		__glim_TexGenfv (gc, coord, pname, &f);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		return;
	}
	DEBUG_END_FUNCTION( "__glim_TexGenf" );
}

void __glim_TexGendv ( __glContext *gc, GLenum coord, GLenum pname, const GLdouble pv[])
{
	__glTextureCoordState *tcs;
	__glFloat v[4];
	__glMatrix *m;

	DEBUG_BEGIN_FUNCTION( "__glim_TexGendv" );
	__GL_CHECK_NOT_IN_BEGIN ();

	switch (coord)
	{
	case GL_S:
		tcs = &gc->state.texture.s[gc->state.texture.SelectedUnit];
		break;
	case GL_T:
		tcs = &gc->state.texture.t[gc->state.texture.SelectedUnit];
		break;
	case GL_R:
		tcs = &gc->state.texture.r[gc->state.texture.SelectedUnit];
		break;
	case GL_Q:
		tcs = &gc->state.texture.q[gc->state.texture.SelectedUnit];
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexGendv" );
		return;
	}
	switch (pname)
	{
	case GL_TEXTURE_GEN_MODE:
		switch ((GLenum) pv[0])
		{
		case GL_EYE_LINEAR:
		case GL_OBJECT_LINEAR:
			tcs->mode = (GLenum) pv[0];
			break;
		case GL_SPHERE_MAP:
			if ((coord == GL_R) || (coord == GL_Q))
			{
				__glSetError (gc, GL_INVALID_ENUM);
				DEBUG_END_FUNCTION( "__glim_TexGendv" );
				return;
			}
			tcs->mode = (GLenum) pv[0];
			break;
		default:
			__glSetError (gc, GL_INVALID_ENUM);
			DEBUG_END_FUNCTION( "__glim_TexGendv" );
			return;
		}
		break;
	case GL_OBJECT_PLANE:
		tcs->objectPlaneEquation.X = pv[0];
		tcs->objectPlaneEquation.Y = pv[1];
		tcs->objectPlaneEquation.Z = pv[2];
		tcs->objectPlaneEquation.W = pv[3];
		break;
	case GL_EYE_PLANE:
		v[0] = pv[0];
		v[1] = pv[1];
		v[2] = pv[2];
		v[3] = pv[3];
		validateAll(gc);
		m = &gc->transform.modelView->inverseTranspose;
		mathVector4XMatrix (&tcs->eyePlaneEquation.X, v, m->matrix);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		return;
	}
	//(*gc->methods.pickAllProcs) (gc);
	DEBUG_END_FUNCTION( "__glim_TexGendv" );
}

void __glim_TexGend ( __glContext *gc, GLenum coord, GLenum pname, GLdouble d)
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexGend" );
	__GL_CHECK_NOT_IN_BEGIN ();

	/* Accept only enumerants that correspond to single values */
	switch (pname)
	{
	case GL_TEXTURE_GEN_MODE:
		__glim_TexGendv (gc, coord, pname, &d);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexGend" );
		return;
	}
	DEBUG_END_FUNCTION( "__glim_TexGend" );
}

void __glim_TexGeniv ( __glContext *gc, GLenum coord, GLenum pname, const GLint pv[])
{
	__glTextureCoordState *tcs;
	__glFloat v[4];
	__glMatrix *m;

	DEBUG_BEGIN_FUNCTION( "__glim_TexGeniv" );
	__GL_CHECK_NOT_IN_BEGIN ();

	switch (coord)
	{
	case GL_S:
		tcs = &gc->state.texture.s[gc->state.texture.SelectedUnit];
		break;
	case GL_T:
		tcs = &gc->state.texture.t[gc->state.texture.SelectedUnit];
		break;
	case GL_R:
		tcs = &gc->state.texture.r[gc->state.texture.SelectedUnit];
		break;
	case GL_Q:
		tcs = &gc->state.texture.q[gc->state.texture.SelectedUnit];
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexGeniv" );
		return;
	}
	switch (pname)
	{
	case GL_TEXTURE_GEN_MODE:
		switch ((GLenum) pv[0])
		{
		case GL_EYE_LINEAR:
		case GL_OBJECT_LINEAR:
			tcs->mode = (GLenum) pv[0];
			break;
		case GL_SPHERE_MAP:
			if ((coord == GL_R) || (coord == GL_Q))
			{
				__glSetError (gc, GL_INVALID_ENUM);
				DEBUG_END_FUNCTION( "__glim_TexGeniv" );
				return;
			}
			tcs->mode = (GLenum) pv[0];
			break;
		default:
			__glSetError (gc, GL_INVALID_ENUM);
			DEBUG_END_FUNCTION( "__glim_TexGeniv" );
			return;
		}
		break;
	case GL_OBJECT_PLANE:
		tcs->objectPlaneEquation.X = pv[0];
		tcs->objectPlaneEquation.Y = pv[1];
		tcs->objectPlaneEquation.Z = pv[2];
		tcs->objectPlaneEquation.W = pv[3];
		break;
	case GL_EYE_PLANE:
		v[0] = pv[0];
		v[1] = pv[1];
		v[2] = pv[2];
		v[3] = pv[3];
		validateAll(gc);
		m = &gc->transform.modelView->inverseTranspose;
		mathVector4XMatrix (&tcs->eyePlaneEquation.X, v, m->matrix);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexGeniv" );
		return;
	}
	//(*gc->methods.pickAllProcs) (gc);
	gc->valid.All = 1;
	DEBUG_END_FUNCTION( "__glim_TexGeniv" );
}

void __glim_TexGeni ( __glContext *gc, GLenum coord, GLenum pname, GLint i)
{
	DEBUG_BEGIN_FUNCTION( "__glim_TexGeni" );
	__GL_CHECK_NOT_IN_BEGIN ();

	/* Accept only enumerants that correspond to single values */
	switch (pname)
	{
	case GL_TEXTURE_GEN_MODE:
		__glim_TexGeniv (gc, coord, pname, &i);
		break;
	default:
		__glSetError (gc, GL_INVALID_ENUM);
		DEBUG_END_FUNCTION( "__glim_TexGeni" );
		return;
	}
	DEBUG_END_FUNCTION( "__glim_TexGeni" );
}


void __gllei_TexImage1D (__glContext * gc, GLenum target, GLint level,
						 GLint internalFormat, GLsizei width, GLint border,
						 GLenum format, GLenum type, GLvoid * image)
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__gllei_TexImage1D" );

	/* Check arguments and get the right texture being changed */
	tex = __glCheckTexImage1DArgs (gc, target, level, internalFormat, width,
								   border, format, type);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__gllei_TexImage1D" );
		return;
	}
	if (!IsLegalRange (gc, width, border) )
	{
		DEBUG_END_FUNCTION( "__gllei_TexImage1D" );
		return;
	}
	DEBUG_END_FUNCTION( "__gllei_TexImage1D" );
}

void __gllei_TexImage2D (__glContext * gc, GLenum target, GLint level,
						 GLint internalFormat, GLsizei width, GLsizei height,
						 GLint border, GLenum format, GLenum type, GLvoid * image)
{
	__glTexture *tex;
	DEBUG_BEGIN_FUNCTION( "__gllei_TexImage2D" );

	/* Check arguments and get the right texture being changed */
	tex = __glCheckTexImage2DArgs (gc, target, level, internalFormat, width, height,
								   border, format, type);
	if (!tex)
	{
		DEBUG_END_FUNCTION( "__gllei_TexImage2D" );
		return;
	}
	if (!IsLegalRange (gc, width, border) || !IsLegalRange (gc, height, border))
	{
		DEBUG_END_FUNCTION( "__gllei_TexImage2D" );
		return;
	}
	DEBUG_END_FUNCTION( "__gllei_TexImage2D" );
}

void __gllei_TexSubImage1D (__glContext * gc, GLenum target, GLint level,
							GLint xoffset, GLint length,
							GLenum format, GLenum type, const GLvoid * image)
{
	DEBUG_BEGIN_FUNCTION( "__gllei_TexSubImage1D" );
	DEBUG_END_FUNCTION( "__gllei_TexSubImage1D" );
}

void __gllei_TexSubImage2D (__glContext * gc, GLenum target, GLint level,
							GLint xoffset, GLint yoffset,
							GLsizei w, GLsizei h, GLenum format, GLenum type,
							const GLvoid * image)
{
	DEBUG_BEGIN_FUNCTION( "__gllei_TexSubImage2D" );
	DEBUG_END_FUNCTION( "__gllei_TexSubImage2D" );
}

/*********************************************************************************/


/***********************************************************************/

_STATIC_ __glFloat Dot (const __glCoord * v1, const __glCoord * v2)
{
	DEBUG_BEGIN_FUNCTION( "Dot" );
	DEBUG_END_FUNCTION( "Dot" );
	return (v1->X * v2->X + v1->Y * v2->Y + v1->Z * v2->Z);
}

/*
   ** Compute the s & t coordinates for a sphere map.  The s & t values
   ** are stored in "result" even if both coordinates are not being
   ** generated.  The caller picks the right values out.
 */
_STATIC_ void SphereGen (__glContext * gc, GLuint vx, __glCoord * result)
{
	__glCoord u, r;
	__glFloat m, ndotu;

	DEBUG_BEGIN_FUNCTION( "SphereGen" );
	
	/* Get unit vector from origin to the vertex in eye coordinates into u */
	u.X = gc->vertices.EyeX[vx];
	u.Y = gc->vertices.EyeX[vx];
	u.Z = gc->vertices.EyeX[vx];
	mathVectorNormalize1(&u.X);

	/* Dot the normal with the unit position u */
	ndotu =	u.X * gc->vertices.NormalX[vx] +
			u.Y * gc->vertices.NormalY[vx] +
			u.Z * gc->vertices.NormalZ[vx];

	/* Compute r */
	r.X = u.X - 2 * gc->vertices.NormalX[vx] * ndotu;
	r.Y = u.Y - 2 * gc->vertices.NormalY[vx] * ndotu;
	r.Z = u.Z - 2 * gc->vertices.NormalZ[vx] * ndotu;

	/* Compute m */
	m = 2 * sqrt (r.X * r.X + r.Y * r.Y + (r.Z + 1) * (r.Z + 1));

	if (m)
	{
		result->X = r.X / m + __glHalf;
		result->Y = r.Y / m + __glHalf;
	}
	else
	{
		result->X = __glHalf;
		result->Y = __glHalf;
	}
	DEBUG_END_FUNCTION( "SphereGen" );
}

/*
   ** Transform or compute the texture coordinates for this vertex.
 */
void __glCalcMixedTexture (__glContext * gc, GLuint vx)
{
	__glCoord sphereCoord, gen, *c;
	GLboolean didSphereGen = GL_FALSE;
	__glMatrix *m;
	GLint ct;

	DEBUG_BEGIN_FUNCTION( "__glCalcMixedTexture" );
	/* Generate/copy s coordinate */
	
	for( ct=0; ct<gc->texture.ActiveUnits; ct++ )
	{
		if (gc->state.texture.GenEnabled[ct][GL_S - GL_S])
		{
			switch (gc->state.texture.s[ct].mode)
			{
			case GL_EYE_LINEAR:
				c = &gc->state.texture.s[ct].eyePlaneEquation;
				gen.X = c->X * gc->vertices.EyeX[vx] +
						c->Y * gc->vertices.EyeY[vx] +
						c->Z * gc->vertices.EyeZ[vx] +
						c->W * gc->vertices.EyeW[vx];
				break;
			case GL_OBJECT_LINEAR:
				c = &gc->state.texture.s[ct].objectPlaneEquation;
				gen.X = c->X * gc->vertices.ObjX[vx] +
						c->Y * gc->vertices.ObjY[vx] +
						c->Z * gc->vertices.ObjZ[vx] +
						c->W * gc->vertices.ObjW[vx];
				break;
			case GL_SPHERE_MAP:
				SphereGen (gc, vx, &sphereCoord);
				gen.X = sphereCoord.X;
				didSphereGen = GL_TRUE;
				break;
			}
		}
		else
		{
			switch( ct )
			{
			case 0:	gen.X = gc->vertices.TextureX[vx]; break;
			case 1:	gen.X = gc->vertices.Texture2X[vx]; break;
			case 2:	gen.X = gc->vertices.Texture3X[vx]; break;
			case 3:	gen.X = gc->vertices.Texture4X[vx]; break;
			}
		}
	
		/* Generate/copy t coordinate */
		if (gc->state.texture.GenEnabled[ct][GL_T - GL_S])
		{
			switch (gc->state.texture.t[ct].mode)
			{
			case GL_EYE_LINEAR:
				c = &gc->state.texture.t[ct].eyePlaneEquation;
				gen.Y = c->X * gc->vertices.EyeX[vx] +
						c->Y * gc->vertices.EyeY[vx] +
						c->Z * gc->vertices.EyeZ[vx] +
						c->W * gc->vertices.EyeW[vx];
				break;
			case GL_OBJECT_LINEAR:
				c = &gc->state.texture.t[ct].objectPlaneEquation;
				gen.Y = c->X * gc->vertices.ObjX[vx] +
						c->Y * gc->vertices.ObjY[vx] +
						c->Z * gc->vertices.ObjZ[vx] +
						c->W * gc->vertices.ObjW[vx];
				break;
			case GL_SPHERE_MAP:
				if (!didSphereGen)
				{
					SphereGen (gc, vx, &sphereCoord);
				}
				gen.Y = sphereCoord.Y;
				break;
			}
		}
		else
		{
			switch( ct )
			{
			case 0:	gen.Y = gc->vertices.TextureY[vx]; break;
			case 1:	gen.Y = gc->vertices.Texture2Y[vx]; break;
			case 2:	gen.Y = gc->vertices.Texture3Y[vx]; break;
			case 3:	gen.Y = gc->vertices.Texture4Y[vx]; break;
			}
		}
	
		/* Generate/copy r coordinate */
		if (gc->state.texture.GenEnabled[ct][GL_R - GL_S])
		{
			switch (gc->state.texture.r[ct].mode)
			{
			case GL_EYE_LINEAR:
				c = &gc->state.texture.r[ct].eyePlaneEquation;
				gen.Z = c->X * gc->vertices.EyeX[vx] +
						c->Y * gc->vertices.EyeY[vx] +
						c->Z * gc->vertices.EyeZ[vx] +
						c->W * gc->vertices.EyeW[vx];
				break;
			case GL_OBJECT_LINEAR:
				c = &gc->state.texture.r[ct].objectPlaneEquation;
				gen.Z = c->X * gc->vertices.ObjX[vx] +
						c->Y * gc->vertices.ObjY[vx] +
						c->Z * gc->vertices.ObjZ[vx] +
						c->W * gc->vertices.ObjW[vx];
				break;
			case GL_SPHERE_MAP:
				if (!didSphereGen)
				{
					SphereGen (gc, vx, &sphereCoord);
				}
				gen.Z = sphereCoord.Z;
				break;
			}
		}
		else
		{
			switch( ct )
			{
			case 0:	gen.Z = gc->vertices.TextureZ[vx]; break;
			case 1:	gen.Z = gc->vertices.Texture2Z[vx]; break;
			case 2:	gen.Z = gc->vertices.Texture3Z[vx]; break;
			case 3:	gen.Z = gc->vertices.Texture4Z[vx]; break;
			}
		}
	
		/* Generate/copy t coordinate */
		if (gc->state.texture.GenEnabled[ct][GL_Q - GL_S])
		{
			switch (gc->state.texture.q[ct].mode)
			{
			case GL_EYE_LINEAR:
				c = &gc->state.texture.q[ct].eyePlaneEquation;
				gen.W = c->X * gc->vertices.EyeX[vx] +
						c->Y * gc->vertices.EyeY[vx] +
						c->Z * gc->vertices.EyeZ[vx] +
						c->W * gc->vertices.EyeW[vx];
				break;
			case GL_OBJECT_LINEAR:
				c = &gc->state.texture.q[ct].objectPlaneEquation;
				gen.W = c->X * gc->vertices.ObjX[vx] +
						c->Y * gc->vertices.ObjY[vx] +
						c->Z * gc->vertices.ObjZ[vx] +
						c->W * gc->vertices.ObjW[vx];
				break;
			case GL_SPHERE_MAP:
				if (!didSphereGen)
				{
					SphereGen (gc, vx, &sphereCoord);
				}
				gen.W = sphereCoord.W;
				break;
			}
		}
		else
		{
			switch( ct )
			{
			case 0:	gen.W = gc->vertices.TextureW[vx]; break;
			case 1:	gen.W = gc->vertices.Texture2W[vx]; break;
			case 2:	gen.W = gc->vertices.Texture3W[vx]; break;
			case 3:	gen.W = gc->vertices.Texture4W[vx]; break;
			}
		}
	
		/* Finally, apply texture matrix */
		m = &gc->transform.texture->matrix;
	
		mathVector4XMatrix (&gen.X, &gen.X, m->matrix);
	
		switch( ct )
		{
		case 0:
			gc->vertices.TextureX[vx] = gen.X;
			gc->vertices.TextureY[vx] = gen.Y;
			gc->vertices.TextureZ[vx] = gen.Z;
			gc->vertices.TextureW[vx] = gen.W;
			break;
		case 1:
			gc->vertices.Texture2X[vx] = gen.X;
			gc->vertices.Texture2Y[vx] = gen.Y;
			gc->vertices.Texture2Z[vx] = gen.Z;
			gc->vertices.Texture2W[vx] = gen.W;
			break;
		case 2:
			gc->vertices.Texture3X[vx] = gen.X;
			gc->vertices.Texture3Y[vx] = gen.Y;
			gc->vertices.Texture3Z[vx] = gen.Z;
			gc->vertices.Texture3W[vx] = gen.W;
			break;
		case 3:
			gc->vertices.Texture4X[vx] = gen.X;
			gc->vertices.Texture4Y[vx] = gen.Y;
			gc->vertices.Texture4Z[vx] = gen.Z;
			gc->vertices.Texture4W[vx] = gen.W;
			break;
		}
	}
	DEBUG_END_FUNCTION( "__glCalcMixedTexture" );
}

void __glCalcEyeLinear (__glContext * gc, GLuint vx)
{
	__glCoord gen, *c;
	__glMatrix *m;
	GLint ct;

	DEBUG_BEGIN_FUNCTION( "__glCalcEyeLinear" );

	for( ct=0; ct<gc->texture.ActiveUnits; ct++ )
	{
		/* Generate texture coordinates from eye coordinates */
		c = &gc->state.texture.s[ct].eyePlaneEquation;
		gen.X = c->X * gc->vertices.EyeX[vx] +
				c->Y * gc->vertices.EyeY[vx] +
				c->Z * gc->vertices.EyeZ[vx] +
				c->W * gc->vertices.EyeW[vx];
		c = &gc->state.texture.t[ct].eyePlaneEquation;
		gen.Y = c->X * gc->vertices.EyeX[vx] +
				c->Y * gc->vertices.EyeY[vx] +
				c->Z * gc->vertices.EyeZ[vx] +
				c->W * gc->vertices.EyeW[vx];
		switch( ct )
		{
		case 0:
			gen.Z = gc->vertices.TextureZ[vx];
			gen.W = gc->vertices.TextureW[vx];
			break;
		case 1:
			gen.Z = gc->vertices.Texture2Z[vx];
			gen.W = gc->vertices.Texture2W[vx];
			break;
		case 2:
			gen.Z = gc->vertices.Texture3Z[vx];
			gen.W = gc->vertices.Texture3W[vx];
			break;
		case 3:
			gen.Z = gc->vertices.Texture4Z[vx];
			gen.W = gc->vertices.Texture4W[vx];
			break;
		}
			
	
		/* Finally, apply texture matrix */
		m = &gc->transform.texture->matrix;
		mathVector4XMatrix (&gen.X, &gen.X, m->matrix);
	
		switch( ct )
		{
		case 0:
			gc->vertices.TextureX[vx] = gen.X;
			gc->vertices.TextureY[vx] = gen.Y;
			gc->vertices.TextureZ[vx] = gen.Z;
			gc->vertices.TextureW[vx] = gen.W;
			break;
		case 1:
			gc->vertices.Texture2X[vx] = gen.X;
			gc->vertices.Texture2Y[vx] = gen.Y;
			gc->vertices.Texture2Z[vx] = gen.Z;
			gc->vertices.Texture2W[vx] = gen.W;
			break;
		case 2:
			gc->vertices.Texture3X[vx] = gen.X;
			gc->vertices.Texture3Y[vx] = gen.Y;
			gc->vertices.Texture3Z[vx] = gen.Z;
			gc->vertices.Texture3W[vx] = gen.W;
			break;
		case 3:
			gc->vertices.Texture4X[vx] = gen.X;
			gc->vertices.Texture4Y[vx] = gen.Y;
			gc->vertices.Texture4Z[vx] = gen.Z;
			gc->vertices.Texture4W[vx] = gen.W;
			break;
		}
	}
	
	DEBUG_END_FUNCTION( "__glCalcEyeLinear" );
}

void __glCalcObjectLinear (__glContext * gc, GLuint vx)
{
	__glCoord gen, *c;
	__glMatrix *m;
	GLint ct;

	DEBUG_BEGIN_FUNCTION( "__glCalcObjectLinear" );
	/* Generate texture coordinates from object coordinates */
	for( ct=0; ct<gc->texture.ActiveUnits; ct++ )
	{
		c = &gc->state.texture.s[ct].objectPlaneEquation;
		gen.X = c->X * gc->vertices.ObjX[vx] +
				c->Y * gc->vertices.ObjY[vx] +
				c->Z * gc->vertices.ObjZ[vx] +
				c->W * gc->vertices.ObjW[vx];
		c = &gc->state.texture.t[ct].objectPlaneEquation;
		gen.Y = c->X * gc->vertices.ObjX[vx] +
				c->Y * gc->vertices.ObjY[vx] +
				c->Z * gc->vertices.ObjZ[vx] +
				c->W * gc->vertices.ObjW[vx];
		switch( ct )
		{
		case 0:
			gen.Z = gc->vertices.TextureZ[vx];
			gen.W = gc->vertices.TextureW[vx];
			break;
		case 1:
			gen.Z = gc->vertices.Texture2Z[vx];
			gen.W = gc->vertices.Texture2W[vx];
			break;
		case 2:
			gen.Z = gc->vertices.Texture3Z[vx];
			gen.W = gc->vertices.Texture3W[vx];
			break;
		case 3:
			gen.Z = gc->vertices.Texture4Z[vx];
			gen.W = gc->vertices.Texture4W[vx];
			break;
		}

		/* Finally, apply texture matrix */
		m = &gc->transform.texture->matrix;
		mathVector4XMatrix (&gen.X, &gen.X, m->matrix);
		
		switch( ct )
		{
		case 0:
			gc->vertices.TextureX[vx] = gen.X;
			gc->vertices.TextureY[vx] = gen.Y;
			gc->vertices.TextureZ[vx] = gen.Z;
			gc->vertices.TextureW[vx] = gen.W;
			break;
		case 1:
			gc->vertices.Texture2X[vx] = gen.X;
			gc->vertices.Texture2Y[vx] = gen.Y;
			gc->vertices.Texture2Z[vx] = gen.Z;
			gc->vertices.Texture2W[vx] = gen.W;
			break;
		case 2:
			gc->vertices.Texture3X[vx] = gen.X;
			gc->vertices.Texture3Y[vx] = gen.Y;
			gc->vertices.Texture3Z[vx] = gen.Z;
			gc->vertices.Texture3W[vx] = gen.W;
			break;
		case 3:
			gc->vertices.Texture4X[vx] = gen.X;
			gc->vertices.Texture4Y[vx] = gen.Y;
			gc->vertices.Texture4Z[vx] = gen.Z;
			gc->vertices.Texture4W[vx] = gen.W;
			break;
		}
	}
	DEBUG_BEGIN_FUNCTION( "__glCalcObjectLinear" );
}


void __glCalcSphereMap (__glContext * gc, GLuint vx)
{
	__glCoord sphereCoord;
	__glMatrix *m;
	GLint ct;

	DEBUG_BEGIN_FUNCTION( "__glCalcSphereMap" );
	for( ct=0; ct<gc->texture.ActiveUnits; ct++ )
	{
		SphereGen (gc, vx, &sphereCoord);
		switch( ct )
		{
		case 0:
			sphereCoord.Z = gc->vertices.TextureZ[vx];
			sphereCoord.W = gc->vertices.TextureW[vx];
			break;
		case 1:
			sphereCoord.Z = gc->vertices.Texture2Z[vx];
			sphereCoord.W = gc->vertices.Texture2W[vx];
			break;
		case 2:
			sphereCoord.Z = gc->vertices.Texture3Z[vx];
			sphereCoord.W = gc->vertices.Texture3W[vx];
			break;
		case 3:
			sphereCoord.Z = gc->vertices.Texture4Z[vx];
			sphereCoord.W = gc->vertices.Texture4W[vx];
			break;
		}

		/* Finally, apply texture matrix */
		m = &gc->transform.texture->matrix;
		mathVector4XMatrix (&sphereCoord.X, &sphereCoord.X, m->matrix);
	
		switch( ct )
		{
		case 0:
			gc->vertices.TextureX[vx] = sphereCoord.X;
			gc->vertices.TextureY[vx] = sphereCoord.Y;
			gc->vertices.TextureZ[vx] = sphereCoord.Z;
			gc->vertices.TextureW[vx] = sphereCoord.W;
			break;
		case 1:
			gc->vertices.Texture2X[vx] = sphereCoord.X;
			gc->vertices.Texture2Y[vx] = sphereCoord.Y;
			gc->vertices.Texture2Z[vx] = sphereCoord.Z;
			gc->vertices.Texture2W[vx] = sphereCoord.W;
			break;
		case 2:
			gc->vertices.Texture3X[vx] = sphereCoord.X;
			gc->vertices.Texture3Y[vx] = sphereCoord.Y;
			gc->vertices.Texture3Z[vx] = sphereCoord.Z;
			gc->vertices.Texture3W[vx] = sphereCoord.W;
			break;
		case 3:
			gc->vertices.Texture4X[vx] = sphereCoord.X;
			gc->vertices.Texture4Y[vx] = sphereCoord.Y;
			gc->vertices.Texture4Z[vx] = sphereCoord.Z;
			gc->vertices.Texture4W[vx] = sphereCoord.W;
			break;
		}
	}
	DEBUG_END_FUNCTION( "__glCalcSphereMap" );
}

void __glCalcTexture (__glContext * gc, GLuint vx)
{
	DEBUG_BEGIN_FUNCTION( "__glCalcTexture" );
	if( !gc->transform.matrixIsIdent )
	{
		GLint ct;
		for( ct=0; ct<gc->texture.ActiveUnits; ct++ )
		{
			__glCoord copy;
			__glMatrix *m;
		
			switch( ct )
			{
			case 0:
				copy.X = gc->vertices.TextureX[vx];
				copy.Y = gc->vertices.TextureY[vx];
				copy.Z = gc->vertices.TextureZ[vx];
				copy.W = gc->vertices.TextureW[vx];
				break;
			case 1:
				copy.X = gc->vertices.Texture2X[vx];
				copy.Y = gc->vertices.Texture2Y[vx];
				copy.Z = gc->vertices.Texture2Z[vx];
				copy.W = gc->vertices.Texture2W[vx];
				break;
			case 2:
				copy.X = gc->vertices.Texture3X[vx];
				copy.Y = gc->vertices.Texture3Y[vx];
				copy.Z = gc->vertices.Texture3Z[vx];
				copy.W = gc->vertices.Texture3W[vx];
				break;
			case 3:
				copy.X = gc->vertices.Texture4X[vx];
				copy.Y = gc->vertices.Texture4Y[vx];
				copy.Z = gc->vertices.Texture4Z[vx];
				copy.W = gc->vertices.Texture4W[vx];
				break;
			}
		
			/* Apply texture matrix */
			m = &gc->transform.texture->matrix;
			mathVector4XMatrix(&copy.X, &copy.X, m->matrix);
	
			switch( ct )
			{
			case 0:
				gc->vertices.TextureX[vx] = copy.X;
				gc->vertices.TextureY[vx] = copy.Y;
				gc->vertices.TextureZ[vx] = copy.Z;
				gc->vertices.TextureW[vx] = copy.W;
				break;
			case 1:
				gc->vertices.Texture2X[vx] = copy.X;
				gc->vertices.Texture2Y[vx] = copy.Y;
				gc->vertices.Texture2Z[vx] = copy.Z;
				gc->vertices.Texture2W[vx] = copy.W;
				break;
			case 2:
				gc->vertices.Texture3X[vx] = copy.X;
				gc->vertices.Texture3Y[vx] = copy.Y;
				gc->vertices.Texture3Z[vx] = copy.Z;
				gc->vertices.Texture3W[vx] = copy.W;
				break;
			case 3:
				gc->vertices.Texture4X[vx] = copy.X;
				gc->vertices.Texture4Y[vx] = copy.Y;
				gc->vertices.Texture4Z[vx] = copy.Z;
				gc->vertices.Texture4W[vx] = copy.W;
				break;
			}
		}
	}
	DEBUG_END_FUNCTION( "__glCalcTexture" );
}

void  __glim_ActiveTextureARB( __glContext *gc, GLenum tmu )
{
	GLint t = tmu - GL_TEXTURE0_ARB;
	
	if( (t < 0) || (t > gc->texture.ActiveUnits ))
	{
		__glSetError (gc, GL_INVALID_ENUM);
		return;
	}
		
	gc->state.texture.SelectedUnit = t;
}

