#include "ac3audio_bsi.h"

#include <support/Debug.h>
#include <string.h>

void 
ac3audio_parse_bsi(ac3audio_decoder_t *decoder)
{
	static const uint8 kNumberOfFullFreqChannels[] = { 2,1,2,3,3,4,4,5 };
	bool addbsie;

	memset(&decoder->bsi,0,sizeof(ac3audio_bsi_t));

	decoder->bsi.bsid=mpeg2bits_get(decoder->bitstream,5);
	decoder->bsi.bsmod=mpeg2bits_get(decoder->bitstream,3);
	decoder->bsi.acmod=mpeg2bits_get(decoder->bitstream,3);

	ASSERT(decoder->bsi.acmod<8);
		
	if ((decoder->bsi.acmod & 1) && (decoder->bsi.acmod!=1))
		decoder->bsi.cmixlev=mpeg2bits_get(decoder->bitstream,2);
		
	if (decoder->bsi.acmod & 4)
		decoder->bsi.surmixlev=mpeg2bits_get(decoder->bitstream,2);
	
	if (decoder->bsi.acmod==2)
		decoder->bsi.dsurmod=mpeg2bits_get(decoder->bitstream,2);
		
	decoder->bsi.lfeon=mpeg2bits_get(decoder->bitstream,1);	

	ac3audio_parse_bsi_params(decoder,&decoder->bsi.param[0]);

	if (decoder->bsi.acmod==0)
		ac3audio_parse_bsi_params(decoder,&decoder->bsi.param[1]);
		
	decoder->bsi.copyrightb=mpeg2bits_get(decoder->bitstream,1);
	decoder->bsi.origbs=mpeg2bits_get(decoder->bitstream,1);

	decoder->bsi.timecod1e=mpeg2bits_get(decoder->bitstream,1);
	if (decoder->bsi.timecod1e)
		decoder->bsi.timecod1=mpeg2bits_get(decoder->bitstream,14);

	decoder->bsi.timecod2e=mpeg2bits_get(decoder->bitstream,1);
	if (decoder->bsi.timecod2e)
		decoder->bsi.timecod2=mpeg2bits_get(decoder->bitstream,14);

	addbsie=mpeg2bits_get(decoder->bitstream,1);
	
	if (addbsie)
	{
		uint16 i;
		uint8 addbsil=mpeg2bits_get(decoder->bitstream,6);
		
		for (i=0;i<=addbsil;++i)
			mpeg2bits_skip(decoder->bitstream,8);
	}
	
	decoder->bsi.nfchans=kNumberOfFullFreqChannels[decoder->bsi.acmod];
}

void 
ac3audio_parse_bsi_params(ac3audio_decoder_t *decoder, ac3audio_bsi_param_t *bsi_param)
{
	bsi_param->dialnorm=mpeg2bits_get(decoder->bitstream,5);

	bsi_param->compre=mpeg2bits_get(decoder->bitstream,1);
	
	if (bsi_param->compre)
		bsi_param->compr=mpeg2bits_get(decoder->bitstream,8);
	
	bsi_param->langcode=mpeg2bits_get(decoder->bitstream,1);
	
	if (bsi_param->langcode)
		bsi_param->langcod=mpeg2bits_get(decoder->bitstream,8);
	
	bsi_param->audprodie=mpeg2bits_get(decoder->bitstream,1);
		
	if (bsi_param->audprodie)
	{
		bsi_param->mixlevel=mpeg2bits_get(decoder->bitstream,5);
		bsi_param->roomtyp=mpeg2bits_get(decoder->bitstream,2);
	}
}

