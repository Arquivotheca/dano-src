/* ++++++++++

   FILE:  3dRenderer.cpp
   REVS:  $Revision: 1.2 $
   NAME:  pierre
   DATE:  Wed Jun 05 12:24:37 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <stdlib.h>
#include <string.h>
#include <Debug.h>

#ifndef _3D_RENDERER_H
#include "3dRenderer.h"
#endif
#ifndef _3D_CAMERA_H
#include "3dCamera.h"
#endif
#ifndef _3D_LIGHTER_H
#include "3dLighter.h"
#endif

#if __POWERPC__
extern "C" {
  void *Xmemset(void * dst, int val, long len);
};
#endif

static render_hook noHooks[B_RENDER_HOOK_COUNT] = {
	(render_hook)0L,
	(render_hook)0L,
	(render_hook)MapTriangle,
	(render_hook)0L,
};

ulong    invert[1024];
uchar    *index_map;
ulong    *color_list;
			
B3dRenderer::B3dRenderer(B3dLighter *lighter) {
	long      i;
	
	for (i=1;i<1024;i++)
		invert[i] = (0x100000+(i>>1))/i;
	myRenderHooks = (render_hook*)noHooks;
	myLighter = lighter;

    index_map = (uchar*)system_colors()->index_map;
	color_list = (ulong*)system_colors()->color_list;
}

B3dRenderer::~B3dRenderer() {
}

long B3dRenderer::SetBuffer(void *Bits, long bytes_per_row, color_space mode,
							long Width, long Height) {
	bits = Bits;
	bytesPerRow = bytes_per_row;
	width = Width+1;
	height = Height+1;
	return B_NO_ERROR;
}

void B3dRenderer::EraseBuffer(B3dEraseOptions *options) {
	int         value;

	value = index_map[((long)(options->color.red*31.0+0.5))<<10 |
					  ((long)(options->color.green*31.0+0.5))<<5 |
					  ((long)(options->color.blue*31.0+0.5))];
#if __POWERPC__
	Xmemset(bits, value, bytesPerRow * height);
#else
	memset(bits, value, bytesPerRow * height);
#endif
}













