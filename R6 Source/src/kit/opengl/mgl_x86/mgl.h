#ifndef __MGL_H__
#define __MGL_H__

#include <OS.h>
#include "../mgl_x86/mgl_state.h"


// Note these are closely related to the defines in generation.h.
// Change at great risk!

#define _MGL_DEPTH_FUNC__DISABLED				0
#define _MGL_DEPTH_FUNC__NEVER					1
#define _MGL_DEPTH_FUNC__LESS					2
#define _MGL_DEPTH_FUNC__EQUAL					3
#define _MGL_DEPTH_FUNC__LEQUAL					4
#define _MGL_DEPTH_FUNC__GREATER				5
#define _MGL_DEPTH_FUNC__NOTEQUAL				6
#define _MGL_DEPTH_FUNC__GEQUAL					7
#define _MGL_DEPTH_FUNC__ALWAYS					8

#define _MGL_ALPHA_FUNC__DISABLED				_MGL_DEPTH_FUNC__DISABLED
#define _MGL_ALPHA_FUNC__NEVER					_MGL_DEPTH_FUNC__NEVER
#define _MGL_ALPHA_FUNC__LESS					_MGL_DEPTH_FUNC__LESS
#define _MGL_ALPHA_FUNC__EQUAL					_MGL_DEPTH_FUNC__EQUAL
#define _MGL_ALPHA_FUNC__LEQUAL					_MGL_DEPTH_FUNC__LEQUAL
#define _MGL_ALPHA_FUNC__GREATER				_MGL_DEPTH_FUNC__GREATER
#define _MGL_ALPHA_FUNC__NOTEQUAL				_MGL_DEPTH_FUNC__NOTEQUAL
#define _MGL_ALPHA_FUNC__GEQUAL					_MGL_DEPTH_FUNC__GEQUAL
#define _MGL_ALPHA_FUNC__ALWAYS					_MGL_DEPTH_FUNC__ALWAYS

#define _MGL_STENCIL_FUNC__DISABLED				_MGL_DEPTH_FUNC__DISABLED
#define _MGL_STENCIL_FUNC__NEVER				_MGL_DEPTH_FUNC__NEVER
#define _MGL_STENCIL_FUNC__LESS					_MGL_DEPTH_FUNC__LESS
#define _MGL_STENCIL_FUNC__EQUAL				_MGL_DEPTH_FUNC__EQUAL
#define _MGL_STENCIL_FUNC__LEQUAL				_MGL_DEPTH_FUNC__LEQUAL
#define _MGL_STENCIL_FUNC__GREATER				_MGL_DEPTH_FUNC__GREATER
#define _MGL_STENCIL_FUNC__NOTEQUAL				_MGL_DEPTH_FUNC__NOTEQUAL
#define _MGL_STENCIL_FUNC__GEQUAL				_MGL_DEPTH_FUNC__GEQUAL
#define _MGL_STENCIL_FUNC__ALWAYS				_MGL_DEPTH_FUNC__ALWAYS

#define _MGL_SHADE_FUNC__FLAT					0
#define _MGL_SHADE_FUNC__GOURAD					1

#define _MGL_FOG_FUNC__OFF						0
#define	_MGL_FOG_FUNC__GL						1
#define	_MGL_FOG_FUNC__AA						2

#define _MGL_COLOR_WRITE_ENABLE__RED			4
#define _MGL_COLOR_WRITE_ENABLE__GREEN			2
#define _MGL_COLOR_WRITE_ENABLE__BLUE			1
#define _MGL_COLOR_WRITE_ENABLE__ALPHA			8

#define _MGL_TEXTURE_ENV_MODE__REPLACE			0
#define _MGL_TEXTURE_ENV_MODE__MODULATE			1
#define _MGL_TEXTURE_ENV_MODE__DECAL			2
#define _MGL_TEXTURE_ENV_MODE__BLEND			3

#define _MGL_TEXTURE_WRAP_MODE__REPEAT			0
#define _MGL_TEXTURE_WRAP_MODE__CLAMP			1

#define _MGL_TEXTURE_FILTER_MODE__NEAREST		0
#define _MGL_TEXTURE_FILTER_MODE__LINEAR		1

#define _MGL_TEXTURE_FORMAT__NONE				0
#define _MGL_TEXTURE_FORMAT__BGRA_8888_32		1
#define _MGL_TEXTURE_FORMAT__BGR_888_32			2
#define _MGL_TEXTURE_FORMAT__BGR_888_24			3
#define _MGL_TEXTURE_FORMAT__BGRA_4444_16		4
#define _MGL_TEXTURE_FORMAT__BGRA_5551_16		5
#define _MGL_TEXTURE_FORMAT__BGR_555_15			6
#define _MGL_TEXTURE_FORMAT__BGR_565_16			7
#define _MGL_TEXTURE_FORMAT__LA_88_16			8
#define _MGL_TEXTURE_FORMAT__LA_44_8			9
#define _MGL_TEXTURE_FORMAT__L_8_8				10
#define _MGL_TEXTURE_FORMAT__I_8_8				11


#ifdef __cplusplus
extern "C" {
#endif

status_t __mgl_Init( __mglContext **context, uint32 cache_size );
void __mgl_Uninit( __mglContext *context );
status_t __mgl_Build( __mglContext *context );

void __mglBeginRegion( __mglContext *con );
void __mglEndRegion( __mglContext *con, bool compress );
void __mglBeginPoly( __mglContext *con );
void __mglEndPoly( __mglContext *con );
void __mglPoint2x( __mglContext *con, int32 x, int32 y );
void __mglPoint2f( __mglContext *con, float x, float y );



#ifdef __cplusplus
}
#endif


#endif
