#ifndef MCMMX_H_
#define MCMMX_H_

#ifdef __cplusplus 
extern "C" {

#endif

void mc_hh_vh_mmx8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hh_vf_mmx8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);


void mc_hf_vf_mmx8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hf_vh_mmx8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hf_vh_mmx16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);



void mc_hh_vh_mmx16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);


void mc_hh_vf_mmx16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hf_vf_mmx16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
				  
void mc_hh_vh_8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hh_vf_8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);


void mc_hf_vf_8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hf_vh_8(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hf_vh_16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);



void mc_hh_vh_16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);


void mc_hh_vf_16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);
				  
void mc_hf_vf_16(unsigned char * ref,
				  unsigned char * dest,
				  int stride,
				  int delta);

#ifdef __cplusplus
}
#endif

#endif
