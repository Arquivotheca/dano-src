#ifndef _DISPLAY_ROTATION_H_
#define _DISPLAY_ROTATION_H_

#if ROTATE_DISPLAY

#include <math.h>
#include <stdio.h>
#include <SupportDefs.h>
#include "basic_as_types.h"
#include "shared_fonts.h"
#include "IRegion.h"

/*
   overall build option activated by :
   export BUILD_ROTATED_DISPLAY=1
 */

// Keep in sync with font_defs.h
typedef struct {
	short       left;           /* bounding box of the string relative */
	short       top;            /* to the current origin point */
	short       right;
	short       bottom;
} fc_rect_p;

struct DisplayRotater {
	float					mirrorFValue;
	int32					mirrorIValue;

	inline int32			RotateH(int32) const;
	inline int32			RotateV(int32) const;
	inline int32			RotateDH(int32) const;
	inline int32			RotateDV(int32) const;
	inline float			RotateX(float) const;
	inline float			RotateY(float) const;
	inline float			RotateAlpha(float) const;
	inline void				RotateRect(const rect*, rect*) const;
	inline void				RotateRect(const fc_rect_p*, fc_rect_p*) const;
	inline void				ReverseRotateRect(const rect*, rect*) const;
	inline void				RotatePoint(const point*, point*) const;
	inline void				RotateRect(const frect*, frect*) const;
	inline void				RotatePoint(const fpoint*, fpoint*) const;
	inline void				RotateRadius(const fpoint*, fpoint*) const;
	inline void  			RotateRegion(const region*, region*) const;
};

inline float DisplayRotater::RotateX(float x) const {
	return x;
}

inline float DisplayRotater::RotateY(float y) const {
	return mirrorFValue-y;
}

inline int32 DisplayRotater::RotateH(int32 c) const {
	return c;
}

inline int32 DisplayRotater::RotateV(int32 c) const {
	return mirrorIValue-c;
}

inline int32 DisplayRotater::RotateDH(int32 c) const {
	return c;
}

inline int32 DisplayRotater::RotateDV(int32 c) const {
	return -c;
}

inline float DisplayRotater::RotateAlpha(float a) const {
	return a-M_PI*0.5;
}

inline void DisplayRotater::RotateRect(const rect *original, rect *rotated) const {
	int32	top, bottom;
	
	top = original->top;
	bottom = original->bottom;
	rotated->top = original->left;
	rotated->bottom = original->right;
	rotated->left = mirrorIValue-bottom;
	rotated->right = mirrorIValue-top;
}

inline void DisplayRotater::RotateRect(const fc_rect_p *original, fc_rect_p *rotated) const {
	int16	top, bottom;
	
	top = original->top;
	bottom = original->bottom;
	rotated->top = original->left;
	rotated->bottom = original->right;
	rotated->left = mirrorIValue-bottom;
	rotated->right = mirrorIValue-top;
}

inline void DisplayRotater::ReverseRotateRect(const rect *original, rect *rotated) const {
	int32	top, bottom;
	
	top = original->top;
	bottom = original->bottom;
	rotated->top = mirrorIValue-original->right;
	rotated->bottom = mirrorIValue-original->left;
	rotated->left = top;
	rotated->right = bottom;
}

inline void DisplayRotater::RotatePoint(const point *original, point *rotated) const {
	int32		h;
	
	h = original->h;
	rotated->h = mirrorIValue-original->v;
	rotated->v = h;
}

inline void DisplayRotater::RotateRect(const frect *original, frect *rotated) const {
	float	top, bottom;
	
	top = original->top;
	bottom = original->bottom;
	rotated->top = original->left;
	rotated->bottom = original->right;
	rotated->left = mirrorFValue-bottom;
	rotated->right = mirrorFValue-top;
}

inline void DisplayRotater::RotatePoint(const fpoint *original, fpoint *rotated) const {
	float		h;
	
	h = original->h;
	rotated->h = mirrorFValue-original->v;
	rotated->v = h;
}

inline void DisplayRotater::RotateRadius(const fpoint *original, fpoint *rotated) const {
	float		h;
	
	h = original->h;
	rotated->h = original->v;
	rotated->v = h;
}

inline void DisplayRotater::RotateRegion(const region* src, region* dest) const {
	src->Rotate(*this, dest);
}

inline void RotateRect(const rect *original, rect *rotated, int32 mirrorValue) {
	int32	top, bottom;
	
	top = original->top;
	bottom = original->bottom;
	rotated->top = original->left;
	rotated->bottom = original->right;
	rotated->left = mirrorValue-bottom;
	rotated->right = mirrorValue-top;
}

#endif /* end ROTATE_DISPLAY */

#endif /* end _DISPLAY_ROTATION_H_ */

