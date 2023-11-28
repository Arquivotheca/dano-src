#include "StencilImage.h"

#include <View.h>

#define MINROWS 4
#define MAXBYTES (64 * 1024)


void 
StencilImage::RenderBuffer(uint32 rowBufferHeight)
{
#if DEBUG > 0
	printf("StencilImage::RenderBuffer(%lu)\n", rowBufferHeight);
#endif
	// convert the coordinates of the buffered rows to device (ie: bitmap) space
	// build a new region that contains the intersection of the buffered rows and
	//   the existing clipping region
	BRegion ir;
	BRect limits(0.0, 1.0 - ((float)(fRowBase + rowBufferHeight) / fHeight), 1.0, 1.0 - ((float)fRowBase / fHeight));
	ir.Set(fTransform.TransformedBounds(limits));
	ir.IntersectWith(&fClip);

	int32 rects = ir.CountRects();
	if (rects == 0) return;

	// walk each rectangle in the intersection
		// for each row in this rectangle
			// for each column in this row
				// if the buffered rows contain this pixel
					// update the bitmap

	// the current rectangle
	BRect cr;

	uint32 bytes_per_row = fBitmap->BytesPerRow();
	uint32 bytes_per_pixel;

	switch (fColorSpace)
	{
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
			bytes_per_pixel = 4;
			break;
		case B_RGB16_LITTLE:
		case B_RGB15_LITTLE:
			bytes_per_pixel = 2;
			break;
		default:
			return;
	}

	// walk each rectangle in the intersection
	for (int32 i = 0; i < rects; i++)
	{
		// current rect
		cr = ir.RectAt(i);
#if DEBUG > 0
		printf("RenderBuffer, rect %ld of %ld: %f,%f %f,%f\n", i, rects-1, cr.left, cr.top, cr.right, cr.bottom);
#endif
		// process each row in the destination rect
		int32 rect_height = cr.IntegerHeight() + 1;
		int32 rect_width = cr.IntegerWidth() + 1;
		BPoint line_start(cr.LeftTop());
		//printf("left,top %f,%f\n", line_start.x, line_start.y);
		fInverse.Transform(&line_start, 1);
		BPoint cur_point = fBitmap->ChildAt(0)->LeftTop();
		uint8 *pixel, *pixel_start;
		pixel_start = ((uint8 *)fBitmap->Bits());
		pixel_start += bytes_per_row * ((int32)(cr.top - cur_point.y));
		pixel_start += bytes_per_pixel * ((int32)(cr.left - cur_point.x));
		int32 rows = rect_height;
		//printf("Bitmap: %p, %ld, (%lx) %p\n", fBitmap->Bits(), fBitmap->BitsLength(), fBitmap->BitsLength(), (uint8*)(fBitmap->Bits())+fBitmap->BitsLength());
		// for each row in this rectangle
		while (rows--)
		{
			int32 columns = rect_width;
			cur_point = line_start;
			pixel = pixel_start;
			//printf("line_start: %f,%f\npixel_start %p\n", line_start.x, line_start.y, pixel_start);
			// process each column of the destination rect
			while (columns--)
			{
				if (limits.Contains(cur_point))
				{
					int32 x = (int32)(cur_point.x * fWidth);
					int32 y = (int32)(fHeight - (cur_point.y * fHeight));
					uint8 *parts = fRowBuffer + (((y - fRowBase) * (int32)(fWidth + 1)) + x);
					// coerce color components into pixel
					if (!*parts)
					{
						// coerce color components into pixel
						switch (fColorSpace)
						{
							case B_RGB32_LITTLE:
							case B_RGBA32_LITTLE:
								*(uint32*)pixel = fPixel32;
								break;
							case B_RGB16_LITTLE:
								*(uint16*)pixel = fPixel16;
								break;
							case B_RGB15_LITTLE:
								*(uint16*)pixel = fPixel16;
								break;
							default: break;
						}
					}
				}
				// advance to next pixel
				cur_point += fDx;
				pixel += bytes_per_pixel;
			}
			// advance to next line
			line_start += fDy;
			pixel_start += bytes_per_row;
		}
	}
}


StencilImage::StencilImage(BBitmap *bitmap, const BRegion &clip, const Transform2D &t, uint32 width, uint32 height, uint8 *pixel)
	: Pusher(), fBitmap(bitmap), fClip(clip), fTransform(t), fInverse(t), fWidth(width-1), fHeight(height-1),
	fComponent(0), fRowBase(0), fMaxRows(0), fRowBytes(width), fBufferBytes(0), fRowBuffer(0),
	fPixel16(((uint16)(pixel[0] >> 3) << 11) | ((uint16)(pixel[1] >> 2) <<  5) | ((uint16)(pixel[2] >> 3))),
	fPixel32(((uint32)pixel[2]) | ((uint32)pixel[1]) << 8 | ((uint32)pixel[0]) << 16)

{
	fInverse.Invert();
	//printf("xform:\n"); fTransform.PrintToStream(); printf("\ninverse:\n"); fInverse.PrintToStream(); printf("\n");
	BPoint p[3];
	p[0].x = 0;	p[0].y = 0;
	p[1].x = 1;	p[1].y = 0;
	p[2].x = 0;	p[2].y = 1;
	fInverse.Transform(p, 3);
	fDx.Set(p[1].x - p[0].x, p[1].y - p[0].y);
	fDy.Set(p[2].x - p[0].x, p[2].y - p[0].y);
	//printf("dx: %f,%f  dy: %f,%f\n", fDx.x, fDx.y, fDy.x, fDy.y);
	// size the buffer appropriately
	fMaxRows = MAXBYTES / fRowBytes;	// max number of rows in MAXBYTES
	fMaxRows = (fMaxRows >= height) ? height : ((fMaxRows < MINROWS) ? MINROWS : fMaxRows);
	fBufferBytes = fMaxRows * fRowBytes;
	fRowBuffer = new uint8[fBufferBytes];
	fColorSpace = fBitmap->ColorSpace();
	if (fColorSpace == B_RGB15_LITTLE)
		fPixel16 = ((uint16)(pixel[0] >> 3) << 10) | ((uint16)(pixel[1] >> 3) <<  5) | ((uint16)(pixel[2] >> 3));

}


StencilImage::~StencilImage()
{
	delete [] fRowBuffer;
}

ssize_t 
StencilImage::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	ssize_t origLength = length;

	while (length)
	{
		// buffer upto MAXROWS scan lines of image data
		fRowBuffer[fComponent++] = *buffer++;
		length--;
		// if we just buffered the the MAXROWSth line
		//  or we have no more input data and callers wants us to finish
		if ((fComponent == fBufferBytes) ||
			((length == 0) && (finish)))
		{
			// render the buffer
			RenderBuffer(fComponent / fRowBytes);
			fRowBase += fMaxRows;
			fComponent = 0;
		}
	}
	if (finish) RenderBuffer(fComponent / fRowBytes);
	return origLength - length;
}

