#ifndef YCBCR_H_
#define YCBCR_H_

#ifdef __cplusplus 
extern "C" {

#endif

void ycbcr_mmx_1(unsigned char * dest,
			   unsigned char * y,
			   unsigned char * u,
			   unsigned char * v,
			   int height,
			   int  width);
			   
void ycbcr_mmx_2(unsigned char * dest,
			   unsigned char * y,
			   unsigned char * u,
			   unsigned char * v,
			   int height,
			   int  width);



#ifdef __cplusplus
}
#endif

#endif
