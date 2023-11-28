#if !defined(__SIMPLEIMAGE_H_)
#define __SIMPLEIMAGE_H_

#include "Pusher.h"
#include <Bitmap.h>
#include <Region.h>
#include <Point.h>
#include "Transform2D.h"

namespace Pushers {

class SimpleImage : public Pusher {

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
color_space			fColorSpace;

void				RenderBuffer(uint32 rowBufferHeight);

public:
					SimpleImage(BBitmap *bitmap, const BRegion &clip, const Transform2D &t, uint32 width, uint32 height);
virtual				~SimpleImage();

virtual	ssize_t		Write(const uint8 *buffer, ssize_t length, bool finish = false);

};

};

using namespace Pushers;
#endif
