#include "SimpleImage.h"

#include <View.h>

#define MINROWS 4
#define MAXBYTES (64 * 1024)

#define HIT_TEST 0
#if HIT_TEST
#include <stdlib.h>
static uint16 pixel_colors = 0;
#endif

SimpleImage::SimpleImage(BBitmap *bitmap, const BRegion &clip, const Transform2D &t, uint32 width, uint32 height)
	: Pusher(), fBitmap(bitmap), fClip(clip), fTransform(t), fInverse(t), fWidth(width-1), fHeight(height-1),
	fComponent(0), fRowBase(0), fMaxRows(0), fRowBytes(width * 3), fBufferBytes(0), fRowBuffer(0)
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
	//printf("SimpleImage::SimpleImage() dx: %f,%f  dy: %f,%f\n", fDx.x, fDx.y, fDy.x, fDy.y);
	// size the buffer appropriately
	fMaxRows = MAXBYTES / fRowBytes;	// max number of rows in MAXBYTES
	fMaxRows = (fMaxRows >= height) ? height : ((fMaxRows < MINROWS) ? MINROWS : fMaxRows);
	fBufferBytes = fMaxRows * fRowBytes;
	fRowBuffer = new uint8[fBufferBytes];
	fColorSpace = fBitmap->ColorSpace();
#if HIT_TEST
	pixel_colors = rand();
#endif
}

SimpleImage::~SimpleImage()
{
	delete [] fRowBuffer;
}

ssize_t 
SimpleImage::Write(const uint8 *buffer, ssize_t length, bool finish)
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
			//if (fClip.CountRects() > 2)
			// render the buffer
			RenderBuffer(fComponent / fRowBytes);
			// RenderBuffer should update these if it needs to filter
			fRowBase += fMaxRows;
			fComponent = 0;
		}
	}
	if (finish) RenderBuffer(fComponent / fRowBytes);
	return origLength - length;
}

void
SimpleImage::RenderBuffer(uint32 rowBufferHeight)
{
#if DEBUG > 0
	printf("SimpleImage::RenderBuffer(%lu)\n", rowBufferHeight);
#endif
	// convert the coordinates of the buffered rows to device (ie: bitmap) space
	// build a new region that contains the intersection of the buffered rows and
	//   the existing clipping region
	BRegion ir;
	BRect limits(0.0, 1.0 - ((float)(fRowBase + rowBufferHeight) / fHeight), 1.0, 1.0 - ((float)fRowBase / fHeight));
#if 0
	if (limits.top < 0) limits.top = 0.0;
	if (limits.top > 1.0) limits.top = 1.0;
	if (limits.bottom < 0) limits.bottom = 0.0;
	if (limits.bottom > 1.0) limits.bottom = 1.0;
#endif
	ir.Set(fTransform.TransformedBounds(limits));
	ir.IntersectWith(&fClip);

#if DEBUG > 0
	printf("RenderBuffer limits: "); limits.PrintToStream();
#endif

	int32 rects = ir.CountRects();
	if (rects == 0)
	{
#if DEBUG > 0
		printf("SimpleImage::RenderBuffer(%lu) completely clipped.\n", rowBufferHeight);
#endif
		return;
	}

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
#if 0
		if ((fDx.y == 0.0) && (fDy.x == 0.0))
		{
			limits = fInverse.TransformedBounds(cr);
#if 0
			if (limits.left < 0) limits.left = 0;
			if (limits.right < 0) limits.right = 0;
			//if (limits.top < 0) limits.top = 0;
			if (limits.bottom < 0) limits.bottom = 0;
			if (limits.left > 1) limits.left = 1;
			if (limits.right > 1) limits.right = 1;
			if (limits.top > 1) limits.top = 1;
			if (limits.bottom > 1) limits.bottom = 1;
#endif
			printf("cr "); cr.PrintToStream(); printf("limits "); limits.PrintToStream();
			int32 cur_point_x_delta = (int32)(fDx.x * 65536.0 * fWidth);
			int32 cur_point_x_limit = (int32)(limits.right * 65536.0 * fWidth);
			int32 cur_point_y_delta = -(int32)(fDy.y * 65536.0 * fHeight);
			int32 cur_point_y_limit = (int32)(limits.bottom * 65536.0 * fHeight);
			int32 cur_point_y = (int32)(limits.top * 65536.0 * fHeight);
			int32 cur_point_y_start;// = cur_point_y;
			printf("y: %ld (%lx), delta: %ld (%lx), limit: %ld (%lx)\n", cur_point_y, cur_point_y, cur_point_y_delta, cur_point_y_delta, cur_point_y_limit, cur_point_y_limit);
			printf("%p fRowBuffer\n", fRowBuffer);
			//while (rows--)
			while (cur_point_y < 0) cur_point_y += cur_point_y_delta;
			cur_point_y_start = cur_point_y;
			while (cur_point_y <= cur_point_y_limit)
			{
				rows--;
				int32 cur_point_x = (int32)(limits.left * 65536.0 * fWidth);
				//printf("x: %ld (%lx), delta: %ld (%lx), limit: %ld (%lx)\n", cur_point_x, cur_point_x, cur_point_x_delta, cur_point_x_delta, cur_point_x_limit, cur_point_x_limit);
				pixel = pixel_start;
				int32 y = cur_point_y - cur_point_y_start + 0x8000;
				if (y > (int32)(rowBufferHeight << 16)) break;
				uint8 *rowBase = fRowBuffer + (((y >> 16) * (int32)(fWidth + 1)) * 3);
				printf("%p rowBase\n", rowBase);
				while (cur_point_x < 0) cur_point_x += cur_point_x_delta;
				while (cur_point_x <= cur_point_x_limit)
				{
					uint8 *parts = rowBase + ((cur_point_x >> 16) * 3);
					uint8 static_parts[3];
					if (cur_point_x_delta > 0x10000)
					{
						uint16 red = 0;
						uint16 green = 0;
						uint16 blue = 0;
						uint32 counter = 0;
						int32 y_pixels = cur_point_y_delta;
						int32 x_pixels = cur_point_x_delta;
						while (x_pixels > 0x10000)
						{
							red += parts[0];
							green +=parts[1];
							blue += parts[2];
							parts += 3;
							x_pixels -= 0x10000;
							counter++;
						}
						static_parts[0] = red / counter;
						static_parts[1] = green / counter;
						static_parts[2] = blue / counter;
						parts = static_parts;
					}
#if 0
					parts = static_parts;
					parts[0] = 255;
					parts[1] = 0;
					parts[2] = 0;
#endif
					// coerce color components into pixel
					switch (fColorSpace)
					{
						case B_RGB32_LITTLE:
						case B_RGBA32_LITTLE:
							*(uint32*)pixel = ((uint32)parts[2]) | ((uint32)parts[1]) << 8 | ((uint32)parts[0]) << 16;
							break;
						case B_RGB16_LITTLE:
							*(uint16*)pixel =
								((uint16)(parts[0] >> 3) << 11) |
								((uint16)(parts[1] >> 2) <<  5) |
								((uint16)(parts[2] >> 3));
							break;
						case B_RGB15_LITTLE:
							*(uint16*)pixel =
								((uint16)(parts[0] >> 3) << 10) |
								((uint16)(parts[1] >> 3) <<  5) |
								((uint16)(parts[2] >> 3));
							break;
						default:
							break;
					}
					// advance to next pixel
					cur_point_x += cur_point_x_delta;
					pixel += bytes_per_pixel;
				}
				// advance to next line
				cur_point_y += cur_point_y_delta;
				pixel_start += bytes_per_row;
			}
			printf("Didn't do %ld rows\n", rows);
		}
		else
#endif
		{
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
						uint8 *parts = fRowBuffer + ((((y - fRowBase) * (int32)(fWidth + 1)) + x) * 3);
						// coerce color components into pixel
						switch (fColorSpace)
						{
							case B_RGB32_LITTLE:
							case B_RGBA32_LITTLE:
								*(uint32*)pixel = ((uint32)parts[2]) | ((uint32)parts[1]) << 8 | ((uint32)parts[0]) << 16;
								break;
							case B_RGB16_LITTLE:
								*(uint16*)pixel =
									((uint16)(parts[0] >> 3) << 11) |
									((uint16)(parts[1] >> 2) <<  5) |
									((uint16)(parts[2] >> 3));
								break;
							case B_RGB15_LITTLE:
								*(uint16*)pixel =
									((uint16)(parts[0] >> 3) << 10) |
									((uint16)(parts[1] >> 3) <<  5) |
									((uint16)(parts[2] >> 3));
								break;
							default:
								break;
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

}
