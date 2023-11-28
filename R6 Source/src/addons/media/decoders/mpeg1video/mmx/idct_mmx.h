#ifndef IDCTMMX_H_
#define IDCTMMX_H_

#ifdef __cplusplus 
extern "C" {

#endif

void idct_mmx(short * src);
void idct_mmx_3dnow(short * src);
//void idctmmx2(short * src);

#ifdef __cplusplus
}
#endif

#endif
