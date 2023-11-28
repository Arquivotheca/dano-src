/* ++++++++++
	FILE:	basic_as_types.h
	REVS:	$Revision$
	NAME:	pierre
	DATE:	Mon Mar  3 16:36:06 PST 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

/*----------------------------------------------------------------*/

#ifndef _BASIC_AS_TYPES_H
#define _BASIC_AS_TYPES_H

#include <SupportDefs.h>
#include <Rect.h>

namespace BPrivate {

typedef clipping_rect rect;

struct frect {
	float left,top,right,bottom;
};

struct point {
	int32 h,v;
};

struct fpoint {
	float h,v;
};

// BEWARE!!!  x/y ordering of these functions is reversed from the clipping_rect
// set() method!
inline void set_rect(rect& r, int32 const to, int32 const le, int32 const bo, int32 const ri)
	{ r.set(le, to, ri, bo); }
inline void set_rect(frect& r, float const to, float const le, float const bo, float const ri)
	{ r.top = to; r.left = le; r.bottom = bo; r.right = ri; }

inline bool empty_rect(const rect& r)
	{ return !r.is_valid(); }
inline bool empty_rect(const frect& r)
	{ return (r.bottom<r.top) || (r.left>r.right); }

}
using namespace BPrivate;

#endif
