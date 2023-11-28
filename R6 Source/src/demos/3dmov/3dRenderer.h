#ifndef _3D_RENDERER_H
#define _3D_RENDERER_H

#include <InterfaceDefs.h>

#ifndef _3D_DEFS_H
#include "3dDefs.h"
#endif
#ifndef _RGBA_COLOR_H
#include "RGBAColor.h"
#endif
#ifndef _3D_LIGHTER_H
#include "3dLighter.h"
#endif

enum {
	B_RENDER_MAP = 2,

	B_RENDER_HOOK_COUNT = 4
};

typedef void (*render_hook) ();

typedef void (*B_MAP)(long, long, ulong, float, float, float,
					  long, long, ulong, float, float, float,
					  long, long, ulong, float, float, float,
					  void*, long,
					  void*, long, long);

#define	ROTR              8
#define	ROTG              16
#define	ROTB              24
#define	EXPO              20
#define	PROJECT_STEP_FACT 8
#define	PROJECT_STEP_MASK 7
#define	PROJECT_STEP_EXP  3
#define	EXPOFAC	          10
#define	FACTOR            1024
#define FLOAT_EXPO        1048576.0

extern void MapTriangle(long h1, long v1, ulong col1, float x1, float y1, float z1,
						long h2, long v2, ulong col2, float x2, float y2, float z2,
						long h3, long v3, ulong col3, float x3, float y3, float z3,
						void *bits, long row,
						void *map, long Hsize, long Vsize);

extern 	ulong    invert[1024];
extern 	uchar    *index_map;
extern 	ulong    *color_list;

class B3dEraseOptions {
 public:
	RGBAColor     color;
};

class B3dRenderer {
 public:
	void     *bits;
	long     bytesPerRow;
	long     width;
	long     height;
	
	B3dRenderer(B3dLighter *lighter);
	virtual ~B3dRenderer();
	long SetBuffer(void *bits, long bytes_per_row, color_space mode,
				   long width, long height);
	void EraseBuffer(B3dEraseOptions *options);
	inline render_hook *RenderHooks();
 private:
	render_hook *myRenderHooks;
	B3dLighter  *myLighter;
};
	
render_hook *B3dRenderer::RenderHooks() {
	return myRenderHooks;
}

#endif

















