#ifndef __ScanlineRaster__
#define __ScanlineRaster__

#include "rasRaster.h"
#include "context.h"

/*
** Scanline Buffer for software fallback
** 
** This buffer is used when rendering a triangle in a mode
** not supported by the hardware.
**
**
*/
//struct __glFragment;

extern void rasLoadNeededScanlines( __glContext *gc, GLint x1, GLint x2 );
extern void rasStoreNeededScanlines( __glContext *gc, GLint x1, GLint x2 );
extern void rasStorePixel( __glContext *gc, GLint x, GLint y, __glColor *color, GLfloat z );
extern void rasProcessScanline( __glContext *gc, const __glFragment *start, GLint w );
extern void rasStorePixelScanline( __glContext *gc, GLint x1, GLint x2, GLfloat z );
extern void textureFragment( __glContext *gc, __glTexture *tex, GLfloat rho, GLfloat s, GLfloat t, GLfloat r, __glFragment *frag );


typedef struct rasTexeli8Rec
{
	GLubyte r,g,b,a;
	GLubyte l; /* Luminance */
	GLubyte i; /* Intensity */
} rasTexeli8; 

#if 0
typedef struct softwareScanlineProcsRec
{
	void (*scanlineDepth)( GLubyte *valid, GLfloat *zbuf, GLfloat z1, GLfloat dz, GLint w );

	GLuint loadBits;
	GLuint storeBits;
	GLboolean valid;
	GLboolean maskEnabled;
	GLboolean colorWriteEnabled;


	/*	flag for fast scanline processing. Requirements:
		No hardware present.
	*/
		
	GLboolean fastScanline; 

} softwareScanlineProcs;
#endif

#endif

