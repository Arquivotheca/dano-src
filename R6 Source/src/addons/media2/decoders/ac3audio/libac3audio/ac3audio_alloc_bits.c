#include "ac3audio_alloc_bits.h"

#include <support/Debug.h>
#include <string.h>

void 
ac3audio_alloc_bits(ac3audio_decoder_t *decoder)
{
	static const uint8 slowdec[] = { 0x0f,0x11,0x13,0x15 };
	static const uint8 fastdec[] = { 0x3f,0x53,0x67,0x7b };
	static const uint16 slowgain[] = { 0x540,0x4d8,0x478,0x410 };
	static const uint16 dbpbtab[] = { 0x000,0x700,0x900,0xb00 };
	static const int16 floortab[] = { 0x2f0,0x2b0,0x270,0x230,0x1f0,0x170,0x0f0,0xf800 };
	static const uint16 fastgain[] = { 0x080,0x100,0x180,0x200,0x280,0x300,0x380,0x400 };
	
	bool fast_exit;
	uint16 ch;
	
	fast_exit=true;
	
	for (ch=0;fast_exit && ch<decoder->bsi.nfchans;++ch)
	{
		if (decoder->audblk.chexpstr[ch]!=0)
			fast_exit=false;
	}
	
	if (fast_exit && decoder->audblk.cplexpstr==0 && decoder->audblk.lfeexpstr==0
		&& !decoder->audblk.baie && decoder->audblk.snroffste==0 && !decoder->audblk.deltbaie)
	{
		return;
	}
		
	fast_exit=true;
	for (ch=0;fast_exit && ch<decoder->bsi.nfchans;++ch)
	{
		if (decoder->audblk.fsnroffst[ch]!=0)
			fast_exit=false;
	}
	
	if (decoder->audblk.csnroffst==0 && fast_exit && decoder->audblk.cplfsnroffst==0
		&& decoder->audblk.lfefsnroffst==0)
	{
		ac3audio_clear_bap(decoder);
	
		return;
	}
	
	decoder->bap.sdecay=slowdec[decoder->audblk.sdcycod];
	decoder->bap.fdecay=fastdec[decoder->audblk.fdcycod];
	decoder->bap.sgain=slowgain[decoder->audblk.sgaincod];
	decoder->bap.dbknee=dbpbtab[decoder->audblk.dbpbcod];
	decoder->bap.floor=floortab[decoder->audblk.floorcod];
	
	for (ch=0;ch<decoder->bsi.nfchans;++ch)
	{
		decoder->bap.fastleak=0;
		decoder->bap.slowleak=0;
		
		ac3audio_alloc_bits_channel(decoder,decoder->bap.fbw[ch],decoder->exponents.fbw[ch],
								0,decoder->audblk.endmant[ch],
								0,fastgain[decoder->audblk.fgaincod[ch]],
								(((decoder->audblk.csnroffst-15)<<4) + decoder->audblk.fsnroffst[ch])<<2,
								decoder->audblk.deltbae[ch],decoder->audblk.deltnseg[ch],decoder->audblk.deltoffst[ch],
								decoder->audblk.deltba[ch],decoder->audblk.deltlen[ch],
								false);
	}

	if (decoder->audblk.cplinu)
	{
		decoder->bap.fastleak=(decoder->audblk.cplfleak<<8)+768;
		decoder->bap.slowleak=(decoder->audblk.cplsleak<<8)+768;
		
		ac3audio_alloc_bits_channel(decoder,decoder->bap.cpl,decoder->exponents.cpl,
											decoder->audblk.cplstrtmant,decoder->audblk.cplendmant,
											0,fastgain[decoder->audblk.cplfgaincod],
											(((decoder->audblk.csnroffst-15)<<4) + decoder->audblk.cplfsnroffst)<<2,
											decoder->audblk.cpldeltbae,decoder->audblk.cpldeltnseg,decoder->audblk.cpldeltoffst,
											decoder->audblk.cpldeltba,decoder->audblk.cpldeltlen,
											false);
	}
	
	if (decoder->bsi.lfeon)
	{		
		decoder->bap.fastleak=0;
		decoder->bap.slowleak=0;

		ac3audio_alloc_bits_channel(decoder,decoder->bap.lfe,decoder->exponents.lfe,
											0,7,
											0,fastgain[decoder->audblk.lfefgaincod],
											(((decoder->audblk.csnroffst-15)<<4) + decoder->audblk.lfefsnroffst)<<2,
											0,0,NULL,NULL,NULL,
											true);										
	}
}

void 
ac3audio_alloc_bits_channel (ac3audio_decoder_t *decoder,
								uint8 *bap_coeff, const uint8 *exp,
								uint8 start, uint8 end,
								uint16 lowcomp, uint16 fastgain,
								int16 snroffset, uint8 deltbae,
								uint8 deltnseg, const uint8 *deltoffst,
								const uint8 *deltba, const uint8 *deltlen,
								bool is_lfe)
{
	static const uint8 masktab[256] =
		{
			0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
			16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 28, 28, 29,
			29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 34, 34,
			34, 35, 35, 35, 35, 35, 35, 36, 36, 36, 36, 36, 36, 37, 37, 37,
			37, 37, 37, 38, 38, 38, 38, 38, 38, 39, 39, 39, 39, 39, 39, 40,
			40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
			41, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 43, 43, 43,
			43, 43, 43, 43, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 44, 44,
			44, 44, 44, 44, 44, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
			45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 46, 46, 46,
			46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
			46, 46, 46, 46, 46, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47,
			47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 47, 48, 48, 48,
			48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
			48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
			49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49, 49,  0,  0,  0
		};

	static const uint8 bndtab[50] =
		{
			0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
			25,26,27,28,31,34,37,40,43,46,49,55,61,67,73,79,85,97,109,121,133,157,
			181,205,229
		};
	
	static const uint16 hth[3][50] =
		{
			{ 0x04d0,0x04d0,0x0440,0x0400,0x03e0,0x03c0,0x03b0,0x03b0,
			  0x03a0,0x03a0,0x03a0,0x03a0,0x03a0,0x0390,0x0390,0x0390,
			  0x0380,0x0380,0x0370,0x0370,0x0360,0x0360,0x0350,0x0350,
			  0x0340,0x0340,0x0330,0x0320,0x0310,0x0300,0x02f0,0x02f0,
			  0x02f0,0x02f0,0x0300,0x0310,0x0340,0x0390,0x03e0,0x0420,
			  0x0460,0x0490,0x04a0,0x0460,0x0440,0x0440,0x0520,0x0800,
			  0x0840,0x0840 },
			{ 0x04f0,0x04f0,0x0460,0x0410,0x03e0,0x03d0,0x03c0,0x03b0,
			  0x03b0,0x03a0,0x03a0,0x03a0,0x03a0,0x03a0,0x0390,0x0390,
			  0x0390,0x0380,0x0380,0x0380,0x0370,0x0370,0x0360,0x0360,
			  0x0350,0x0350,0x0340,0x0340,0x0310,0x0300,0x02f0,0x02f0,
			  0x02f0,0x02f0,0x02f0,0x0300,0x0320,0x0350,0x0390,0x03e0,
			  0x0420,0x0450,0x04a0,0x0490,0x0460,0x0440,0x0480,0x0630,
			  0x0840,0x0840 },
			{ 0x0580,0x0580,0x04b0,0x0450,0x0420,0x03f0,0x03e0,0x03d0,
			  0x03c0,0x03b0,0x03b0,0x03b0,0x03a0,0x03a0,0x03a0,0x03a0,
			  0x03a0,0x03a0,0x03a0,0x03a0,0x0390,0x0390,0x0390,0x0390,
			  0x0380,0x0380,0x0380,0x0370,0x0360,0x0350,0x0340,0x0330,
			  0x0320,0x0310,0x0300,0x02f0,0x02f0,0x02f0,0x0300,0x0310,
			  0x0330,0x0350,0x03c0,0x0410,0x0470,0x04a0,0x0460,0x0440,
			  0x0450,0x04e0 }
		};
	
	static const uint8 baptab[64] =
		{
			0,1,1,1,1,1,2,2,3,3,3,4,4,5,5,6,6,6,6,7,7,7,7,8,8,8,8,
			9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,
			14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15
		};
		
	static const uint8 bndsz[50] =
		{
			1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
            1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
            1,  1,  1,  1,  1,  1,  1,  1,  3,  3,
            3,  3,  3,  3,  3,  6,  6,  6,  6,  6,
            6, 12, 12, 12, 12, 24, 24, 24, 24, 24
    	};

	uint16 bin;
	int32 j,k;
	uint16 i;
	uint8 bndstrt,bndend;
	uint8 begin;
	int32 new_mask;
	int16 lastbin;
		
	// psd integration
	
	for (bin=start;bin<end;++bin)
		decoder->bap.psd[bin]=3072-(exp[bin]<<7);

	j=start;
	k=masktab[start];
	
	do
	{
		lastbin=min_c(bndtab[k]+bndsz[k],end);		
		decoder->bap.bndpsd[k]=decoder->bap.psd[j];
		++j;
		
		for (;j<lastbin;++j)
			decoder->bap.bndpsd[k]=ac3audio_log_add(decoder->bap.bndpsd[k],decoder->bap.psd[j]);

		++k;
	}
	while (end>lastbin);
	
	
	// compute excitation function
	
	bndstrt=masktab[start];
	bndend=masktab[end-1]+1;
	
	begin=0;

	if (bndstrt==0)
	{
		lowcomp=ac3audio_calc_low_comp(lowcomp,decoder->bap.bndpsd[0],decoder->bap.bndpsd[1],0);
		decoder->bap.excite[0]=decoder->bap.bndpsd[0]-fastgain-lowcomp;

		lowcomp=ac3audio_calc_low_comp(lowcomp,decoder->bap.bndpsd[1],decoder->bap.bndpsd[2],1);
		decoder->bap.excite[1]=decoder->bap.bndpsd[1]-fastgain-lowcomp;
		
		begin=7;
		for (bin=2;bin<7;++bin)
		{
			if (bin<6 || !is_lfe)
				lowcomp=ac3audio_calc_low_comp(lowcomp,decoder->bap.bndpsd[bin],decoder->bap.bndpsd[bin+1],bin);

			decoder->bap.fastleak=decoder->bap.bndpsd[bin]-fastgain;
			decoder->bap.slowleak=decoder->bap.bndpsd[bin]-decoder->bap.sgain;
			
			decoder->bap.excite[bin]=decoder->bap.fastleak-lowcomp;
			
			if ((bin<6 || !is_lfe) && decoder->bap.bndpsd[bin]<=decoder->bap.bndpsd[bin+1])
			{
				begin=bin+1;
				break;
			}
		}
		
		for (bin=begin;bin<min_c(bndend,22);++bin)
		{
			if (bin<6 || !is_lfe)
				lowcomp=ac3audio_calc_low_comp(lowcomp,decoder->bap.bndpsd[bin],decoder->bap.bndpsd[bin+1],bin);
			
			decoder->bap.fastleak-=decoder->bap.fdecay;
			decoder->bap.fastleak=max_c(decoder->bap.fastleak,decoder->bap.bndpsd[bin]-fastgain);
			
			decoder->bap.slowleak-=decoder->bap.sdecay;
			decoder->bap.slowleak=max_c(decoder->bap.slowleak,decoder->bap.bndpsd[bin]-decoder->bap.sgain);
			
			decoder->bap.excite[bin]=max_c(decoder->bap.fastleak-lowcomp,decoder->bap.slowleak);
		}

		begin=22;
	}
	else	// for coupling channel
		begin=bndstrt;

	for (bin=begin;bin<bndend;++bin)
	{
		decoder->bap.fastleak-=decoder->bap.fdecay;
		decoder->bap.fastleak=max_c(decoder->bap.fastleak,decoder->bap.bndpsd[bin]-fastgain);
		
		decoder->bap.slowleak-=decoder->bap.sdecay;
		decoder->bap.slowleak=max_c(decoder->bap.slowleak,decoder->bap.bndpsd[bin]-decoder->bap.sgain);
		
		decoder->bap.excite[bin]=max_c(decoder->bap.fastleak,decoder->bap.slowleak);
	}
	
	// compute masking curve
	
	for (bin=bndstrt;bin<bndend;++bin)
	{
		if (decoder->bap.bndpsd[bin]<decoder->bap.dbknee)
			decoder->bap.excite[bin]+=((decoder->bap.dbknee-decoder->bap.bndpsd[bin])>>2);
		
		decoder->bap.mask[bin]=max_c(decoder->bap.excite[bin],hth[decoder->syncinfo.fscod][bin]);
	}	

	// apply delta bit allocation
	
	if (deltoffst!=NULL && (deltbae==0 || deltbae==1))
	{
		uint16 band=0;
		uint16 seg;
		for (seg=0;seg<deltnseg+1;++seg)
		{
			uint16 delta;
			uint16 k;
			
			band+=deltoffst[seg];
			
			if (deltba[seg]>=4)
				delta=(deltba[seg]-3)<<7;
			else
				delta=(deltba[seg]-4)<<7;
			
			for (k=0;k<deltlen[seg];++k)
			{
				decoder->bap.mask[band]+=delta;
				++band;
			}
		}
	}
	
	// compute bit allocation

	i=start;
	j=masktab[start];
	
	do
	{
		lastbin=bndtab[j]+bndsz[j];
		if (lastbin>end)
			lastbin=end;
		
		new_mask=decoder->bap.mask[j];

		new_mask-=snroffset;
		new_mask-=decoder->bap.floor;
		
		if (new_mask<0)
			decoder->bap.mask[j]=0;
		else
			decoder->bap.mask[j]=new_mask;

		decoder->bap.mask[j] &= 0x1fe0;
		decoder->bap.mask[j] += decoder->bap.floor;
		
		// this used to be for (uint8 k=i;k<lastbin;++k)
		// but the compiler screwed up with -funroll-loops
		
		for (k=i;k<lastbin;++k)
		{
			int32 address=(((int32)decoder->bap.psd[i])-((int32)decoder->bap.mask[j]))/32;
		
			if (address>63)
				address=63;
			else if (address<0)
				address=0;
			
			bap_coeff[i]=baptab[address];
			
			++i;
		}
		
		++j;
	}
	while (end>lastbin);
}

int16 
ac3audio_calc_low_comp(uint16 a, uint16 b0, uint16 b1, uint8 bin)
{
	if (bin<7)
	{
		if (b0+256==b1)
			a=384;
		else if (b0>b1)
			a=max_c(0,((int32)a)-64);
	}
	else if (bin<20)
	{
		if (b0+256==b1)
			a=320;
		else if (b0>b1)
			a=max_c(0,((int32)a)-64);
	}
	else
		a=max_c(0,((int32)a)-128);
	
	return a;
}

int16 
ac3audio_log_add(uint16 a, uint16 b)
{
	static const uint8 latab[260] =
		{
			0x0040, 0x003f, 0x003e, 0x003d, 0x003c, 0x003b, 0x003a, 0x0039,
			0x0038, 0x0037, 0x0036, 0x0035, 0x0034, 0x0034, 0x0033, 0x0032,
			0x0031, 0x0030, 0x002f, 0x002f, 0x002e, 0x002d, 0x002c, 0x002c,
			0x002b, 0x002a, 0x0029, 0x0029, 0x0028, 0x0027, 0x0026, 0x0026,
			0x0025, 0x0024, 0x0024, 0x0023, 0x0023, 0x0022, 0x0021, 0x0021,
			0x0020, 0x0020, 0x001f, 0x001e, 0x001e, 0x001d, 0x001d, 0x001c,
			0x001c, 0x001b, 0x001b, 0x001a, 0x001a, 0x0019, 0x0019, 0x0018,
			0x0018, 0x0017, 0x0017, 0x0016, 0x0016, 0x0015, 0x0015, 0x0015,
			0x0014, 0x0014, 0x0013, 0x0013, 0x0013, 0x0012, 0x0012, 0x0012,
			0x0011, 0x0011, 0x0011, 0x0010, 0x0010, 0x0010, 0x000f, 0x000f,
			0x000f, 0x000e, 0x000e, 0x000e, 0x000d, 0x000d, 0x000d, 0x000d,
			0x000c, 0x000c, 0x000c, 0x000c, 0x000b, 0x000b, 0x000b, 0x000b,
			0x000a, 0x000a, 0x000a, 0x000a, 0x000a, 0x0009, 0x0009, 0x0009,
			0x0009, 0x0009, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008,
			0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0006, 0x0006,
			0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0005, 0x0005,
			0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0004, 0x0004,
			0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004,
			0x0004, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
			0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0002,
			0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
			0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
			0x0002, 0x0002, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
			0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
			0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
			0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
			0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000			
		};
		
#if 1
	int32 c=((int32)a)-((int32)b); 
	
	if (c>=0)
	{
		return a+latab[min_c(c/2,255)];
	}
	else
	{
		return b+latab[min_c((-c)/2,255)];
	}
#else
	if (a>=b)
	{
		uint16 c=(a-b)>>1;

		if (c>0xff)
			c=0xff;
		
		return a+latab[c];
	}
	else
	{
		uint16 c=(b-a)>>1;

		if (c>0xff)
			c=0xff;
		
		return b+latab[c];
	}
#endif
}

void
ac3audio_clear_bap (ac3audio_decoder_t *decoder)
{
	memset(&decoder->bap,0,sizeof(ac3audio_bap_t));
}
