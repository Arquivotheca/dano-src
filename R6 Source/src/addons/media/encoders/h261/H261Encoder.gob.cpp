//=============================================================================
#include "H261Encoder.h"
//=============================================================================
void H261EncoderStream2::encode_gob_header(u_int gob)
{
	/* GSC/GN */
	PUT_BITS(0x10 | gob, 20, nbb_, bb_, bc_);
	
	/* GQUANT/GEI */
	mquant_ = 3;	
	PUT_BITS(mquant_ << 1, 6, nbb_, bb_, bc_);
}
//=============================================================================
void H261EncoderStream2::encode_gob
(
	u_int gob,
	const yuv_image *picture,
	const yuv_image *delta,
	const int *decision
)
{
	mba_ = 0; //first macro block is absolute
	
	for (u_int mba = 1; mba <= 33; mba++) 
	{
		bool this_mba_must_be_encoded = true;
		{
			static bool first = true;
			if (first)
			{
				first = false;
				printf("todo: mba that must be encoded\n");
			}
		}

		int x,y;
		x =((gob-1)%2)*11 + (mba-1)%11;
		y =((gob-1)/2)*3  + (mba-1)/11;

		if (decision[x+22*y] == 0) //intra
		{
			encode_macro_block_intra
				(
				mba, 
				picture, 
				x,
				y
				);
		}
		else if (decision[x+22*y] == 1) //inter
		{
			encode_macro_block_inter
				(
				mba, 
				delta, 
				x,
				y
				);
				
		}
		else //nothing
		{
		}
			
		{
			static bool first = true;
			if (first)
			{
				first = false;
				printf("todo: generating rtp packets\n");
			}
		}
	}
}
//=============================================================================
