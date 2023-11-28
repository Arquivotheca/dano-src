//=============================================================================
#include "H261Encoder.h"
//=============================================================================
#include <memory.h>
//=============================================================================
void H261EncoderStream2::encode_picture_header()
{
	/* PSC */
	PUT_BITS(0x0001, 16, nbb_, bb_, bc_);

	/* GOB 0 -> picture header */
	PUT_BITS(0, 4, nbb_, bb_, bc_);

	/* TR (XXX should do this right) = no use of temporal reference */
	PUT_BITS(0, 5, nbb_, bb_, bc_);

	/* PTYPE = CIF */
	PUT_BITS(pt->ptype, 6, nbb_, bb_, bc_);

	/* PEI (0 = abscence of optional field)*/ 
	PUT_BITS(0, 1, nbb_, bb_, bc_);
}
//=============================================================================
void H261EncoderStream2::encode_picture
(
	yuv_image *picture,
	yuv_image *delta,
	int *decision,
	int q
)
{
	bool picture_to_be_coded = false;
	
	{
		int i;
		int sum = 0;
		for (i=0;i<22*18;i++)
		{
			sum += decision[i];
		}
		
		if (sum != 22*18*2)
		{
			picture_to_be_coded = true;
		}
		
	}
	
	if (picture_to_be_coded)
	{
		//gquant = q;
		encode_picture_header();
		
		for (int *p = pt->gobs; *p != 0; p++)
		{
			bool gob_to_be_coded = false;
			{
				int i;
				int sum = 0;
				int x = (*p - 1)%2;
				int y = (*p - 1)/2;
				
				for (i=0;i<11;i++)
				{
					sum += decision[x*11+i + (y*3+0)*22];
					sum += decision[x*11+i + (y*3+1)*22];
					sum += decision[x*11+i + (y*3+2)*22];
				}
				
				if (sum != 11*3*2)
				{
					gob_to_be_coded = true;
				}
				
				
			}
			
			if (gob_to_be_coded)
			{
				encode_gob_header(*p);
				encode_gob(*p , picture, delta, decision);
			}
		}
	}
}
//=============================================================================
