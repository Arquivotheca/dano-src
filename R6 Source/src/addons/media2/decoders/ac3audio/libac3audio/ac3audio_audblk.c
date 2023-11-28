#include "ac3audio_audblk.h"

#include <support/Debug.h>

status_t 
ac3audio_parse_audblk(ac3audio_decoder_t *decoder, bool first_audblk)
{
	int32 ch,bnd,grp,seg,skipl,i,rbnd;
	bool skiple;
	
	for (ch=0;ch<decoder->bsi.nfchans;++ch)
		decoder->audblk.blksw[ch]=mpeg2bits_get(decoder->bitstream,1);

	for (ch=0;ch<decoder->bsi.nfchans;++ch)
		decoder->audblk.dithflag[ch]=mpeg2bits_get(decoder->bitstream,1);
	
	decoder->audblk.dynrnge=mpeg2bits_get(decoder->bitstream,1);

	if (decoder->audblk.dynrnge)
		decoder->audblk.dynrng=mpeg2bits_get(decoder->bitstream,8);

	if (decoder->bsi.acmod==0)
	{
		decoder->audblk.dynrng2e=mpeg2bits_get(decoder->bitstream,1);
	
		if (decoder->audblk.dynrng2e)
			decoder->audblk.dynrng2=mpeg2bits_get(decoder->bitstream,8);
	}

	decoder->audblk.cplstre=mpeg2bits_get(decoder->bitstream,1);
	
	if (decoder->audblk.cplstre)
	{
		decoder->audblk.cplinu=mpeg2bits_get(decoder->bitstream,1);
		
		if (decoder->audblk.cplinu)
		{
			for (ch=0;ch<decoder->bsi.nfchans;++ch)
				decoder->audblk.chincpl[ch]=mpeg2bits_get(decoder->bitstream,1);
			
			if (decoder->bsi.acmod==2)
				decoder->audblk.phsflginu=mpeg2bits_get(decoder->bitstream,1);
			
			decoder->audblk.cplbegf=mpeg2bits_get(decoder->bitstream,4);
			decoder->audblk.cplendf=mpeg2bits_get(decoder->bitstream,4);
			
			if (decoder->audblk.cplendf<decoder->audblk.cplbegf)	//wtf! how can this happen?
			{
				uint8 temp=decoder->audblk.cplbegf;
				decoder->audblk.cplbegf=decoder->audblk.cplendf;
				decoder->audblk.cplendf=temp;
			}
				
			decoder->audblk.ncplsubnd=3+decoder->audblk.cplendf-decoder->audblk.cplbegf;
			
			decoder->audblk.ncplbnd=decoder->audblk.ncplsubnd;
			
			decoder->audblk.cplbndstrc[0]=0;
			for (bnd=1;bnd<decoder->audblk.ncplsubnd;++bnd)
			{
				decoder->audblk.cplbndstrc[bnd]=mpeg2bits_get(decoder->bitstream,1);
				
				if (decoder->audblk.cplbndstrc[bnd])
					--decoder->audblk.ncplbnd;
			}
		}
	}
	
	if (decoder->audblk.cplinu)
	{
		for (ch=0;ch<decoder->bsi.nfchans;++ch)
		{
			if (decoder->audblk.chincpl[ch])
			{
				decoder->audblk.cplcoe[ch]=mpeg2bits_get(decoder->bitstream,1);
				
				if (decoder->audblk.cplcoe[ch])
				{
					decoder->audblk.mstrcplco[ch]=mpeg2bits_get(decoder->bitstream,2);
					
					for (bnd=0;bnd<decoder->audblk.ncplbnd;++bnd)
					{
						decoder->audblk.cplcoexp[ch][bnd]=mpeg2bits_get(decoder->bitstream,4);
						decoder->audblk.cplcomant[ch][bnd]=mpeg2bits_get(decoder->bitstream,4);
					}
				}
			}
		}
		
		if (decoder->bsi.acmod==2 && decoder->audblk.phsflginu && (decoder->audblk.cplcoe[0] || decoder->audblk.cplcoe[1]))
		{
			for (bnd=0;bnd<decoder->audblk.ncplbnd;++bnd)
				decoder->audblk.phsflg[bnd]=mpeg2bits_get(decoder->bitstream,1);
		}
	}
	
	if (decoder->bsi.acmod==2)
	{
		decoder->audblk.rematstr=mpeg2bits_get(decoder->bitstream,1);
		
		if (decoder->audblk.rematstr)
		{
			if (decoder->audblk.cplbegf>2 || decoder->audblk.cplinu==0)
			{				
				for (rbnd=0;rbnd<4;++rbnd)
					decoder->audblk.rematflg[rbnd]=mpeg2bits_get(decoder->bitstream,1);
			}
			
			if (decoder->audblk.cplbegf>0 && decoder->audblk.cplbegf<=2 && decoder->audblk.cplinu)
			{
				for (rbnd=0;rbnd<3;++rbnd)
					decoder->audblk.rematflg[rbnd]=mpeg2bits_get(decoder->bitstream,1);
			}

			if (decoder->audblk.cplbegf==0 && decoder->audblk.cplinu)
			{
				for (rbnd=0;rbnd<2;++rbnd)
					decoder->audblk.rematflg[rbnd]=mpeg2bits_get(decoder->bitstream,1);
			}
		}
		else if (first_audblk)
			return B_ERROR;
	}
	
	if (decoder->audblk.cplinu)
		decoder->audblk.cplexpstr=mpeg2bits_get(decoder->bitstream,2);
		
	for (ch=0;ch<decoder->bsi.nfchans;++ch)
		decoder->audblk.chexpstr[ch]=mpeg2bits_get(decoder->bitstream,2);
	
	if (decoder->bsi.lfeon)
		decoder->audblk.lfeexpstr=mpeg2bits_get(decoder->bitstream,1);
		
	for (ch=0;ch<decoder->bsi.nfchans;++ch)
	{
		if (decoder->audblk.chexpstr[ch])
		{
			if (!decoder->audblk.cplinu || !decoder->audblk.chincpl[ch])
				decoder->audblk.chbwcod[ch]=mpeg2bits_get(decoder->bitstream,6);
		}
		else if (first_audblk)
			return B_ERROR;
	}

	if (decoder->audblk.cplinu)
	{
		if (decoder->audblk.cplexpstr)
		{
			uint16 grp;
			
			decoder->audblk.cplabsexp=mpeg2bits_get(decoder->bitstream,4);

			decoder->audblk.cplstrtmant=(decoder->audblk.cplbegf*12)+37;
			decoder->audblk.cplendmant=((decoder->audblk.cplendf+3)*12)+37;
			
			switch (decoder->audblk.cplexpstr)
			{
				case 1:
					decoder->audblk.ncplgrps=(decoder->audblk.cplendmant-decoder->audblk.cplstrtmant)/3;	//D15
					break;

				case 2:					
					decoder->audblk.ncplgrps=(decoder->audblk.cplendmant-decoder->audblk.cplstrtmant)/6;	//D25
					break;			

				case 3:					
					decoder->audblk.ncplgrps=(decoder->audblk.cplendmant-decoder->audblk.cplstrtmant)/12;	//D45
					break;
				
				default:
					TRESPASS();
					break;
			}
			
			for (grp=0;grp<decoder->audblk.ncplgrps;++grp)
				decoder->audblk.cplexps[grp]=mpeg2bits_get(decoder->bitstream,7);
		}
		else if (first_audblk)
			return B_ERROR;
	}

	for (ch=0;ch<decoder->bsi.nfchans;++ch)
	{
		if (decoder->audblk.chexpstr[ch])
		{
			if (!decoder->audblk.cplinu || !decoder->audblk.chincpl[ch])
				decoder->audblk.endmant[ch]=((decoder->audblk.chbwcod[ch]+12)*3)+37;
			else
				decoder->audblk.endmant[ch]=decoder->audblk.cplstrtmant;
			
			switch (decoder->audblk.chexpstr[ch])
			{
				case 1:
					decoder->audblk.nchgrps[ch]=((uint8)(decoder->audblk.endmant[ch]-1)/3);	// D15
					break;
					
				case 2:
					decoder->audblk.nchgrps[ch]=((uint8)(decoder->audblk.endmant[ch]-1+3)/6);	// D25
					break;

				case 3:
					decoder->audblk.nchgrps[ch]=((uint8)(decoder->audblk.endmant[ch]-1+9)/12);	// D45
					break;

				default:
					TRESPASS();
					break;
			}
			
			decoder->audblk.exps[ch][0]=mpeg2bits_get(decoder->bitstream,4);

			for (grp=1;grp<=decoder->audblk.nchgrps[ch];++grp)
				decoder->audblk.exps[ch][grp]=mpeg2bits_get(decoder->bitstream,7);
			
			decoder->audblk.gainrng[ch]=mpeg2bits_get(decoder->bitstream,2);
		}
		else if (first_audblk)
			return B_ERROR;
	}
	
	if (decoder->bsi.lfeon)
	{
		if (decoder->audblk.lfeexpstr)
		{
			decoder->audblk.lfeexps[0]=mpeg2bits_get(decoder->bitstream,4);
			
			for (grp=1;grp<=2;++grp)
				decoder->audblk.lfeexps[grp]=mpeg2bits_get(decoder->bitstream,7);
		}
		else if (first_audblk)
			return B_ERROR;
	}
	
	decoder->audblk.baie=mpeg2bits_get(decoder->bitstream,1);
	
	if (decoder->audblk.baie)
	{
		decoder->audblk.sdcycod=mpeg2bits_get(decoder->bitstream,2);
		decoder->audblk.fdcycod=mpeg2bits_get(decoder->bitstream,2);
		decoder->audblk.sgaincod=mpeg2bits_get(decoder->bitstream,2);
		decoder->audblk.dbpbcod=mpeg2bits_get(decoder->bitstream,2);
		decoder->audblk.floorcod=mpeg2bits_get(decoder->bitstream,3);
	}
	
	decoder->audblk.snroffste=mpeg2bits_get(decoder->bitstream,1);
	
	if (decoder->audblk.snroffste)
	{
		decoder->audblk.csnroffst=mpeg2bits_get(decoder->bitstream,6);
		
		if (decoder->audblk.cplinu)
		{
			decoder->audblk.cplfsnroffst=mpeg2bits_get(decoder->bitstream,4);
			decoder->audblk.cplfgaincod=mpeg2bits_get(decoder->bitstream,3);
		}
		
		for (ch=0;ch<decoder->bsi.nfchans;++ch)
		{
			decoder->audblk.fsnroffst[ch]=mpeg2bits_get(decoder->bitstream,4);
			decoder->audblk.fgaincod[ch]=mpeg2bits_get(decoder->bitstream,3);
		}
		
		if (decoder->bsi.lfeon)
		{
			decoder->audblk.lfefsnroffst=mpeg2bits_get(decoder->bitstream,4);
			decoder->audblk.lfefgaincod=mpeg2bits_get(decoder->bitstream,3);
		}
	}
	
	if (decoder->audblk.cplinu)
	{
		decoder->audblk.cplleake=mpeg2bits_get(decoder->bitstream,1);
		
		if (decoder->audblk.cplleake)
		{
			decoder->audblk.cplfleak=mpeg2bits_get(decoder->bitstream,3);
			decoder->audblk.cplsleak=mpeg2bits_get(decoder->bitstream,3);
		}
	}
	
	decoder->audblk.deltbaie=mpeg2bits_get(decoder->bitstream,1);
	
	if (decoder->audblk.deltbaie)
	{
		if (decoder->audblk.cplinu)
			decoder->audblk.cpldeltbae=mpeg2bits_get(decoder->bitstream,2);
		
		for (ch=0;ch<decoder->bsi.nfchans;++ch)
			decoder->audblk.deltbae[ch]=mpeg2bits_get(decoder->bitstream,2);
		
		if (decoder->audblk.cplinu)
		{
			if (decoder->audblk.cpldeltbae==1)
			{
				decoder->audblk.cpldeltnseg=mpeg2bits_get(decoder->bitstream,3);
				
				for (seg=0;seg<=decoder->audblk.cpldeltnseg;++seg)
				{
					decoder->audblk.cpldeltoffst[seg]=mpeg2bits_get(decoder->bitstream,5);
					decoder->audblk.cpldeltlen[seg]=mpeg2bits_get(decoder->bitstream,4);
					decoder->audblk.cpldeltba[seg]=mpeg2bits_get(decoder->bitstream,3);
				}
			}
		}
		
		for (ch=0;ch<decoder->bsi.nfchans;++ch)
		{
			if (decoder->audblk.deltbae[ch]==1)
			{
				decoder->audblk.deltnseg[ch]=mpeg2bits_get(decoder->bitstream,3);
				
				for (seg=0;seg<=decoder->audblk.deltnseg[ch];++seg)
				{
					decoder->audblk.deltoffst[ch][seg]=mpeg2bits_get(decoder->bitstream,5);
					decoder->audblk.deltlen[ch][seg]=mpeg2bits_get(decoder->bitstream,4);
					decoder->audblk.deltba[ch][seg]=mpeg2bits_get(decoder->bitstream,3);
				}
			}
		}
	}
	
	skiple=mpeg2bits_get(decoder->bitstream,1);
	
	if (skiple)
	{
		skipl=mpeg2bits_get(decoder->bitstream,9);
		
		for (i=0;i<skipl;++i)
			mpeg2bits_skip(decoder->bitstream,8);
	}

	return B_OK;
}

