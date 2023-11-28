#include "ac3audio_uncouple.h"

void
ac3audio_uncouple (ac3audio_decoder_t *decoder)
{
	int32 i;
	
	if (decoder->audblk.cplinu)
	{
		for (i=0;i<decoder->bsi.nfchans;++i)
		{
			if (decoder->audblk.chincpl[i])
				ac3audio_uncouple_channel(decoder,decoder->coeffs.cpl,i);
		}
	}
}

void
ac3audio_uncouple_channel (ac3audio_decoder_t *decoder,
							const float *cpl_tmp,
							uint8 ch)
{
	uint32 bnd = 0;
	float cpl_coord=0.0f;
	uint32 cpl_exp_tmp;
	uint32 cpl_mant_tmp;
	int32 i;
	
	for (i=decoder->audblk.cplstrtmant;i<decoder->audblk.cplendmant;i+=12)
	{
		if(!decoder->audblk.cplbndstrc[bnd])
		{
			cpl_exp_tmp = decoder->audblk.cplcoexp[ch][bnd] + 3 * decoder->audblk.mstrcplco[ch];
			if(decoder->audblk.cplcoexp[ch][bnd] == 15)
				cpl_mant_tmp = (decoder->audblk.cplcomant[ch][bnd]) << 12;
			else
				cpl_mant_tmp = ((0x10) | decoder->audblk.cplcomant[ch][bnd]) << 11;
			
			cpl_coord=ac3audio_convert_to_float(cpl_mant_tmp,cpl_exp_tmp);
		}
		
		/* If in 2.0 (stereo) mode, check phase-invert flags */
		if ( decoder->bsi.acmod == 0x02 && ch == 1 && decoder->audblk.phsflginu && 
			decoder->audblk.phsflg[ bnd ] )
		{	/* phsflginu?  phsflg[bnd] ? is this *RIGHT* channel? */
			/* invert phase */
			decoder->coeffs.fbw[ch][i]   = - cpl_coord * cpl_tmp[i];
			decoder->coeffs.fbw[ch][i+1] = - cpl_coord * cpl_tmp[i+1];
			decoder->coeffs.fbw[ch][i+2] = - cpl_coord * cpl_tmp[i+2];
			decoder->coeffs.fbw[ch][i+3] = - cpl_coord * cpl_tmp[i+3];
			decoder->coeffs.fbw[ch][i+4] = - cpl_coord * cpl_tmp[i+4];
			decoder->coeffs.fbw[ch][i+5] = - cpl_coord * cpl_tmp[i+5];
			decoder->coeffs.fbw[ch][i+6] = - cpl_coord * cpl_tmp[i+6];
			decoder->coeffs.fbw[ch][i+7] = - cpl_coord * cpl_tmp[i+7];
			decoder->coeffs.fbw[ch][i+8] = - cpl_coord * cpl_tmp[i+8];
			decoder->coeffs.fbw[ch][i+9] = - cpl_coord * cpl_tmp[i+9];
			decoder->coeffs.fbw[ch][i+10]= - cpl_coord * cpl_tmp[i+10];
			decoder->coeffs.fbw[ch][i+11]= - cpl_coord * cpl_tmp[i+11];
		}
		else
		{	/* normal phase, phaseinvert not in use */
			decoder->coeffs.fbw[ch][i]   = cpl_coord * cpl_tmp[i];
			decoder->coeffs.fbw[ch][i+1] = cpl_coord * cpl_tmp[i+1];
			decoder->coeffs.fbw[ch][i+2] = cpl_coord * cpl_tmp[i+2];
			decoder->coeffs.fbw[ch][i+3] = cpl_coord * cpl_tmp[i+3];
			decoder->coeffs.fbw[ch][i+4] = cpl_coord * cpl_tmp[i+4];
			decoder->coeffs.fbw[ch][i+5] = cpl_coord * cpl_tmp[i+5];
			decoder->coeffs.fbw[ch][i+6] = cpl_coord * cpl_tmp[i+6];
			decoder->coeffs.fbw[ch][i+7] = cpl_coord * cpl_tmp[i+7];
			decoder->coeffs.fbw[ch][i+8] = cpl_coord * cpl_tmp[i+8];
			decoder->coeffs.fbw[ch][i+9] = cpl_coord * cpl_tmp[i+9];
			decoder->coeffs.fbw[ch][i+10]= cpl_coord * cpl_tmp[i+10];
			decoder->coeffs.fbw[ch][i+11]= cpl_coord * cpl_tmp[i+11];
		}


		bnd++;
	}
}

float 
ac3audio_convert_to_float (uint16 mantissa, uint8 exponent)
{
	uint16 out_exponent;
	uint32 out_value;
	uint16 sign;
	int32 i;
		
	/* If the mantissa is zero we can simply return zero */
	if(mantissa==0)
		return 0.0f;
		
	/* Take care of the one asymmetric negative number */
	if(mantissa==0x8000)
		++mantissa;

	/* Extract the sign bit */
	sign = mantissa & 0x8000 ? 1 : 0;

	/* Invert the mantissa if it's negative */
	if(sign)
		mantissa = (~mantissa) + 1;

	/* Shift out the sign bit */
	mantissa <<= 1;

	/* Find the index of the most significant one bit */

	for(i=0;i<16;++i)
	{
		if((mantissa<<i) & 0x8000)
			break;
	}
	++i;

	/* Exponent is offset by 127 in IEEE format minus the shift to
	 * align the mantissa to 1.f */
	out_exponent = 0xff & (127 - exponent - i);
	
	out_value = (((uint32)(sign)) << 31) | (((uint32)out_exponent) << 23) | 
		((0x007fffff) & (((uint32)mantissa) << (7 + i)));
		
	return *(const float *)&out_value;
}
 
