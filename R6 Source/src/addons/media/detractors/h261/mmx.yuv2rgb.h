#ifndef __INCLUDE_mmx_yuv2rgb_h
#define __INCLUDE_mmx_yuv2rgb_h

void yuv_to_rgb32_mmx
(
	unsigned char * destination,	
	const unsigned char * Y,		 	
	const unsigned char * Cb,
	const unsigned char * Cr,
	int width,
	int height,
	int stride
);

#endif
