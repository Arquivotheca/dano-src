#include <StringView.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Lens.h"

inline double
frand()
{
	return (float) rand() / (float) RAND_MAX;
}

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Lens(message, image);
}

void Lens::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Bouncing Lens a.k.a. Nausea"));
}

status_t Lens::StartSaver(BView *v, bool preview)
{
	srand(time(0));

	SetTickSize(1000);
	buffer = NULL;
	filter_x = filter_y = NULL;

	return preview ? B_ERROR : B_OK;	// won't work in preview mode
}

void Lens::StopSaver()
{
	delete [] buffer;
	delete [] filter_x;
	delete [] filter_y;
}

void Lens::DirectConnected(direct_buffer_info *info)
{
	int32 *px, *py;
	float theta;
	
	switch(info->buffer_state & B_DIRECT_MODE_MASK)
	{
		case B_DIRECT_START :
		case B_DIRECT_MODIFY: 
			if (buffer) {
				delete [] buffer;
				delete [] filter_x;
				delete [] filter_y;
				buffer = NULL;
				filter_x = filter_y = NULL;
			}

			rowbytes = info->bytes_per_row;
			pixelbits = info->bits_per_pixel;
			width = info->window_bounds.right - info->window_bounds.left + 1;
			height = info->window_bounds.bottom - info->window_bounds.top + 1;
			pixel_format = info->pixel_format;
			video = (uchar *)info->bits;
			buffer = new uchar [height * rowbytes];
			memcpy(buffer, video, height * rowbytes);

			r = height / 3;
			x0 = 0; y0 = 0; z0 = -0.9 *r;

			maxd = (int32)sqrt(r*r - z0*z0) + 1;
			theta = frand() * 2 * M_PI;
			dx0 = 5*cos(theta); dy0 = 5*sin(theta); dz0 = 0;
			filter_side = (maxd + 10) * 2;

			px = filter_x = new int32 [filter_side * filter_side] ;
			py = filter_y = new int32 [filter_side * filter_side] ;

			for (int32 y2=-filter_side/2;y2<filter_side/2;y2++)
				for (int32 x2=-filter_side/2;x2<filter_side/2;x2++) {
					float x1, y1, z1, z2;

					if (r*r - x2*x2 - y2*y2 <= 0.0) {
						*(px++) = x2;
						*(py++) = y2;
						continue;
					}

					z2 = sqrt(r*r - x2*x2 - y2*y2);
					
					z1 = -z0;
					x1 = z1 * x2/z2;
					y1 = z1 * y2/z2;

					if (x1*x1 + y1*y1 + z1*z1 > r*r) {
						*(px++) = x2;
						*(py++) = y2;
					} else {
						*(px++) = (int32)x1;
						*(py++) = (int32)y1;
					}
				}

			break;
	}
}

void *Lens::Fetch(int32 x, int32 y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	if (x >= width) x = width - 1;
	if (y >= height) y = height - 1;

	return (void *)(buffer + y * rowbytes + x * pixelbits / 8);
}

void Lens::DirectDraw(int32 /*frame*/)
{
	int32	ix0, iy0, x, y, *fx, *fy;

	if (!buffer) return;

	x0 += dx0;
	y0 += dy0;
	z0 += dz0;
	
	if (x0 + maxd >= width) {
		x0 = width - 1 - maxd;
		dx0 = -dx0;
	}

	if (x0 - maxd < 0) {
		x0 = maxd;
		dx0 = -dx0;
	}
	
	if (y0 + maxd >= height) {
		y0 = height - 1 - maxd;
		dy0 = -dy0;
	}

	if (y0 - maxd < 0) {
		y0 = maxd;
		dy0 = -dy0;
	}
	
	ix0 = (int32)x0;
	iy0 = (int32)y0;

#define FILTER(bits) \
	for (y=0;y<filter_side;y++) { \
		if (y + iy0 - filter_side / 2 < 0) continue; \
		if (y + iy0 - filter_side / 2 >= height) break; \
\
		int##bits *vid = (int##bits *) \
				(video + (y + iy0 - filter_side/2) * rowbytes + \
						(ix0 - filter_side/2) * bits / 8); \
		fx = filter_x + y * filter_side; fy = filter_y + y * filter_side; \
		for (x=0;x<filter_side;x++,vid++,fx++,fy++) { \
			if (x + ix0 - filter_side / 2 < 0) continue; \
			if (x + ix0 - filter_side / 2 >= width) break; \
			*vid = *(int##bits *)Fetch(ix0 + *fx, iy0 + *fy); \
		} \
	}
	
	if (pixelbits == 8) {
		FILTER(8);
	} else if (pixelbits == 16) {
		FILTER(16);
	} else if (pixelbits == 32) {
		FILTER(32);
	}
}

Lens::Lens(BMessage *message, image_id id)
 : BScreenSaver(message, id)
{
}
