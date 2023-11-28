#if !defined(__IMAGECHAIN_H_)
#define __IMAGECHAIN_H_

class BBitmap;
class BRegion;
class Pusher;
class BPrivate::PDFObject;
class Transform2D;

Pusher *	MakeImageChain(PDFObject *image, BBitmap *bitmap, const BRegion &clip, const Transform2D &t, uint8 *aColor);

#endif
