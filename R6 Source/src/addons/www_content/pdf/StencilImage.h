#if !defined(__STENCILIMAGE_H_)
#define __STENCILIMAGE_H_

#include "Pusher.h"
#include <Bitmap.h>
#include <Region.h>
#include <Point.h>
#include "Transform2D.h"

namespace Pushers {

class StencilImage : public Pusher {

BBitmap				*fBitmap;
BRegion				fClip;
Transform2D			fTransform;
Transform2D			fInverse;
float				fWidth;
float				fHeight;
uint32				fComponent;
uint32				fRowBase;
uint32				fMaxRows;
uint32				fRowBytes;
uint32				fBufferBytes;
uint8				*fRowBuffer;
BPoint				fDx;
BPoint				fDy;
uint16				fPixel16;
uint32				fPixel32;
color_space			fColorSpace;

void				RenderBuffer(uint32 rowBufferHeight);

public:
					StencilImage(BBitmap *bitmap, const BRegion &clip, const Transform2D &t, uint32 width, uint32 height, uint8 *pixel);
virtual				~StencilImage();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;
#endif
