//=============================================================================
#ifndef __INCLUDE_H261Encoder_h
#define __INCLUDE_H261Encoder_h 1
//=============================================================================
#include "H261PictureType.h"
#include "control.yuv_image.h"
#include "defines.h"	//for BB_INT
#include <stdio.h>		//for u_char, ...
//=============================================================================
#define u_char unsigned char
#define u_int unsigned int
#define u_short unsigned short
//=============================================================================
class H261EncoderStream2
{
public:
	H261EncoderStream2();
	
	void display();
	void save2();
	

	void encode_picture
		(
		yuv_image *picture,
		yuv_image *delta,
		int *decision,
		int q
		);
	void encode_picture_header();
	void encode_gob
		(
		u_int gob,
		const yuv_image *picture,
		const yuv_image *delta,
		const int *decision
		);
	void encode_gob_header(u_int gob);
	
	void encode_macro_block_intra
		(
		u_int mba,		//number of this macro block within the gob
		const yuv_image *picture,
		u_int x,
		u_int y
		);
	void encode_macro_block_inter
		(
		u_int mba,		//number of this macro block within the gob
		const yuv_image *picture,
		u_int x,
		u_int y
		);
	
	void encode_block_intra
		(
		const short* blk,
		int q
		);

	void encode_block_inter
		(
		const short* blk
		);
	
				
private:
	/* bit buffer */
	BB_INT bb_;		//bit buffer
	u_int nbb_;		//number of bits in the bit buffer
	u_char* bs_;	//destination (buffer start)
	u_char* bc_;	//destination (buffer current)
	
	/* quantization related */
	u_char q[6];
	
	//u_char lq_;				/* low quality quantizer */
	//u_char mq_;				/* medium quality quantizer */
	//u_char hq_;				/* high quality quantizer */
	
	u_char mquant_;			/* the last quantizer we sent to other side */
			// set to lq_ at the start of the gob 
			// may change due to requantization : 
			//	  - different quality block
			//    - requantization because of too big coeff
	
	
	u_int mba_;				//(GLOBAL)	last macro block transmitted
							//initialized to 0 (first macro block is absolute)
	H261PictureType *pt;
	
public:
	u_int bits()
	{
		return (bc_-bs_)*8 + nbb_;
	}
};
//=============================================================================
#endif
//=============================================================================
