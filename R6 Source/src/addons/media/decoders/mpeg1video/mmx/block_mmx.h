#ifndef BLOCK_H_
#define BLOCK_H_

#ifdef __cplusplus 
extern "C" {

#endif

void add_block_mmx(unsigned char * dest,short * src,int  stride);

void copy_block_mmx(unsigned char * dest,short * src,int  stride);

#ifdef __cplusplus
}
#endif

#endif
