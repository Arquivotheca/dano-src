#include "ac3audio_rematrix.h"

void
ac3audio_rematrix (ac3audio_decoder_t *decoder)
{
	static const uint8 band_limits[4][2] = { { 13,24 }, { 25,36 }, { 37,60 }, { 61,252 } };
	
	int32 band;
	int32 bin;
		
	uint8 no_of_bands;
	
	if (decoder->audblk.cplinu==0)
		no_of_bands=4;
	else
	{	if (decoder->audblk.cplbegf>2)
			no_of_bands=4;
		else if (decoder->audblk.cplbegf>0)
			no_of_bands=3;
		else
			no_of_bands=2;
	}
	
	for (band=0;band<no_of_bands;++band)
	{
		if (decoder->audblk.rematflg[band])
		{
			uint8 high;
			
			if (band+1<no_of_bands || decoder->audblk.cplinu==0 || decoder->audblk.cplbegf==0)
				high=band_limits[band][1];
			else
				high=37+decoder->audblk.cplbegf*12;
				
			for (bin=band_limits[band][0];bin<high;++bin)
			{
				float new_right=decoder->coeffs.fbw[0][bin]-decoder->coeffs.fbw[1][bin];
				decoder->coeffs.fbw[0][bin]+=decoder->coeffs.fbw[1][bin];
				decoder->coeffs.fbw[1][bin]=new_right;
			}
		}
	}
}

