#include "Flip.h"
#include <StringView.h>
#include <stdlib.h>
#include <string.h>

#define STEPS 50

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Flip(message, image);
}

void Flip::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Flip, by Duncan Wilcox"));
}

status_t Flip::StartSaver(BView * /* view */, bool preview)
{
	SetTickSize(1000);
	buffer = 0;
	v = 0.;
	step = 6.28 / (double)STEPS;

	return preview ? B_ERROR : B_OK;	// won't work in preview mode
}

void Flip::StopSaver()
{
	delete [] buffer;
}

void Flip::DirectConnected(direct_buffer_info *info)
{
	switch(info->buffer_state & B_DIRECT_MODE_MASK)
	{
		case B_DIRECT_START :
		case B_DIRECT_MODIFY: 
			if(buffer)
			{
				delete [] buffer;
				buffer = 0;
			}

			int32 depth = (info->bits_per_pixel + 7) / 8;
			width = (info->window_bounds.right - info->window_bounds.left + 1) * depth;
			gap = info->bytes_per_row - width;
			height = info->window_bounds.bottom - info->window_bounds.top + 1;
			video = (unsigned char *)info->bits;
			buffer = new unsigned char [height * width];

			// memcpy sucks
			// memcpy(buffer, video, height * rowbytes);
			uint32	*src = (uint32 *)video;
			uint32	*dst = (uint32 *)buffer;
			int32	last = width / sizeof(uint32);
			for(int32 i = 0; i < height; i++)
			{
				for(int32 j = 0; j < last; j++)
					*dst++ = *src++;
				src = (uint32 *)(((unsigned char *)src) + gap);
			}
			break;
	}
}

void Flip::DirectDraw(int32)
{
	int32			copyline;
	double			shrink;
	int32			currline;
	unsigned char	*ptr;

	if(! buffer)
		return;

	/* Calculate shrink factor
	 */
	shrink = (v + step < 6.28) ? (1. / cos(v)) : 1.;
//	shrink = 1. / cos(v);

	ptr = video;
	currline = height / 2;

	for(int32 lines = 0; lines < height; lines++)
	{
		/* The line to copy is obtained considering the
		 * lines on the screen numbered from +half (top)
		 * to -half (bottom), line 0 being the center one.
		 * The currline variable does that. Multiplying
		 * the current line by the shrink factor, the source
		 * line is obtained in the same representation.
		 * The subtraction puts it back in the 0 to
		 * height - 1 scale.
		 */
		copyline = int32(height / 2 - ((double)currline * shrink));

		/* If the source line is off screen the target
		 * line is filled with zeros (hoping it's black
		 * or the palette index for black).
		 */
		if(copyline >= 0 && copyline < height)
		{
			// memcpy sucks
			// memcpy(ptr, buffer + copyline * rowbytes, rowbytes);
			uint32	*src = (uint32 *)(buffer + copyline * width);
			uint32	*dst = (uint32 *)ptr;
			int32 last = width / sizeof(uint32);
			for(int32 i = 0; i < last; i++)
				*dst++ = *src++;
		}
		else
		{
			// memset sucks
			// memset(ptr, 0, rowbytes);
			uint32	*dst = (uint32 *)ptr;
			int32 last = width / sizeof(uint32);
			for(int32 i = 0; i < last; i++)
				*dst++ = 0;
		}
		currline--;

		ptr += width + gap;
	}

	v += step;
	if(v >= 6.28)
		v = 0;
}

Flip::Flip(BMessage *message, image_id id)
 : BScreenSaver(message, id)
{
}
