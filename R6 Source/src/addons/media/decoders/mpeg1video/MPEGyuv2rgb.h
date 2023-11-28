#ifndef MPEGYUV2RGB_H
#define MPEGYUV2RGB_H

#ifdef __cplusplus 
extern "C" {

#endif

void yuv2rgb(unsigned char * destination,
			  const unsigned char * Y,
			  const unsigned char * Cb,
			  const unsigned char * Cr,
			  int width, int height, int stride);

#ifdef __cplusplus
}
#endif



#endif
