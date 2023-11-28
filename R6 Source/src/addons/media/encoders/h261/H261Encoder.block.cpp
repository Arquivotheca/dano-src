//=============================================================================
#include "H261Encoder.h"
//=============================================================================
#include "h261_table.h"
//=============================================================================
/* row order */
const u_char ROWZAG[] =
{
	0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};
//=============================================================================
void H261EncoderStream2::encode_block_intra
(
	const short* blk,
	int q
)
{
	static bool first_run = true;

	/* Quantize DC.  Round instead of truncate. */
	//int dc = (blk[0] + 4) >> 3;
	int dc = (blk[0]*q*2 + 4) >> 3;

	if (dc <= 0)
	{	
		/* shouldn't happen with CCIR 601 black (level 16) */
		dc = 1;
	}
	else if (dc > 254)
	{
		dc = 254;
	}
	else if (dc == 128)
	{ 
		/* per Table 6/H.261 */
		dc = 255;
	}
		
	/* Code DC */
	PUT_BITS(dc, 8, nbb_, bb_, bc_);
	

	/* Code the other coeffs */
	{
		int i;
		int run = 0;
		for (i=1;i<64;i++)
		{
			int level = blk[ROWZAG[i]];

			if (level != 0)
			{
				huffent he;
				he.nb = 0;

				//try to use VLC
				if ((level >= -15)&&(level <= 15))
				{
					he = hte_tc[((level&0x1f) << 6)|run];
				}

				//check if VLC use was successfull
				if (he.nb == 0)
				{
					/* Can't use a VLC.  Escape it. */
					he.val  =    1 <<   14;  	//6 bits of escape code 
					he.val |=  run <<    8;  	//6 bits of run (0->63)
					he.val |= level & 0xff;		//8 bits of level
					
					he.nb = 20;
				}

				PUT_BITS(he.val, he.nb, nbb_, bb_, bc_);
				run = 0;
			}
			else
			{
				run++;
			}
		}
	}

	/* EOB */
	PUT_BITS(2, 2, nbb_, bb_, bc_);
	first_run = false;
}
//=============================================================================
void H261EncoderStream2::encode_block_inter
(
	const short* blk
)
{
	int run = 0;
	{
		int i;
		for (i=0;i<64;i++)
		{
			int level = blk[ROWZAG[i]];

			if (level != 0)
			{
				huffent he;
				he.nb = 0;

				//try to use VLC
				if ((level >= -15)&&(level <= 15))
				{
					he = hte_tc[((level&0x1f) << 6)|run];
				}
				
				//table is different for the +1/-1 as first coeff
				if (i == 0)
				{
					switch(level)
					{
					case 1:
						he.nb  = 2;
						he.val = 2;
						break;
					case -1:
						he.nb  = 2;
						he.val = 3;
						break;
					}
				}

				//check if VLC use was successfull
				if (he.nb == 0)
				{
					/* Can't use a VLC.  Escape it. */
					he.val  =    1 <<   14;  	//6 bits of escape code 
					he.val |=  run <<    8;  	//6 bits of run (0->63)
					he.val |= level & 0xff;		//8 bits of level
					
					he.nb = 20;
				}

				PUT_BITS(he.val, he.nb, nbb_, bb_, bc_);
				run = 0;
			}
			else
			{
				run++;
			}
		}
	}

	/* EOB */
	PUT_BITS(2, 2, nbb_, bb_, bc_);
}
//=============================================================================
