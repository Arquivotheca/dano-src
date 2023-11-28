/* getblk.c, DCT block decoding                                             */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>

#include "global.h"


/* defined in getvlc.h */
typedef struct {
  char run, level, len;
} DCTtab;

extern DCTtab DCTtabfirst[],DCTtabnext[],DCTtab0[],DCTtab1[];
extern DCTtab DCTtab2[],DCTtab3[],DCTtab4[],DCTtab5[],DCTtab6[];
extern DCTtab DCTtab0a[],DCTtab1a[];


/* decode one intra coded MPEG-1 block */

void Decode_MPEG1_Intra_Block(int comp,int dc_dct_pred[],video_data *ld)
{
	int val, i, j, sign,k;
	unsigned int code;
	DCTtab *tab;
	short *bp;
	bp = ld->block[comp];

	/* ISO/IEC 11172-2 section 2.4.3.7: Block layer. */
	/* decode DC coefficients */
	if (comp<4)
		bp[0] = (dc_dct_pred[0]+=Get_Luma_DC_dct_diff(ld)) << 3;
	else if (comp==4)
		bp[0] = (dc_dct_pred[1]+=Get_Chroma_DC_dct_diff(ld)) << 3;
	else
		bp[0] = (dc_dct_pred[2]+=Get_Chroma_DC_dct_diff(ld)) << 3;
	
	if (ld->Fault_Flag) return;

	/* D-pictures do not contain AC coefficients */
	if(ld->picture_coding_type == D_TYPE)
		return;
	
	/* decode AC coefficients */
	for (i=1; ; i++)
	{
		code = xShow_Bits(16,ld);
		if (code>=16384)
			tab = &DCTtabnext[(code>>12)-4];
		else if (code>=1024)
			tab = &DCTtab0[(code>>8)-4];
		else if (code>=512)
			tab = &DCTtab1[(code>>6)-8];
		else if (code>=256)
			tab = &DCTtab2[(code>>4)-16];
		else if (code>=128)
			tab = &DCTtab3[(code>>3)-16];
		else if (code>=64)
			tab = &DCTtab4[(code>>2)-16];
		else if (code>=32)
			tab = &DCTtab5[(code>>1)-16];
		else if (code>=16)
			tab = &DCTtab6[code-16];
		else
		{
			//      if (!Quiet_Flag)
			printf("invalid Huffman code in Decode_MPEG1_Intra_Block() code=%ld\n", code);
			ld->Fault_Flag = 1;
			return;
		}
		Flush_Buffer(tab->len,ld);
		if (tab->run==64) return;  /* end_of_block */
		if (tab->run==65) /* escape */
		{
			i+= Get_Bits(6,ld);
			
			val = Get_Bits(8,ld);
			if (val==0)
				val = Get_Bits(8,ld);
			else if (val==128)
				val = Get_Bits(8,ld) - 256;
			else if (val>128)
				val -= 256;
			
			if((sign = (val<0)))
				val = -val;
		}
		else
		{
			i+= tab->run;
			val = tab->level;
			sign = Get_Bits(1,ld);
		}

		if (i>=64)
		{
			printf("DCT coeff index (i) out of bounds (intra)\n");
			ld->Fault_Flag = 1;
			return;
		}
		
		j = scan[i];
		k = scan_mmx[i];
		val = (val * ld->quantizer_scale * ld->intra_quantizer_matrix[j]) >> 3;
		
		/* mismatch control ('oddification') */
		if (val!=0) /* should always be true, but it's not guaranteed */
			val = (val-1) | 1; /* equivalent to: if ((val&1)==0) val = val - 1; */
		
		/* saturation */
		if (!sign)
			bp[k] = (val>2047) ?  2047 :  val; /* positive */
		else
			bp[k] = (val>2048) ? -2048 : -val; /* negative */
	}
}


/* decode one non-intra coded MPEG-1 block */

void Decode_MPEG1_Non_Intra_Block(int comp,video_data *ld)
{
	int val, i, j, sign,k;
	unsigned int code;
	DCTtab *tab;
	short *bp;
	
	bp = ld->block[comp];
	
	/* decode AC coefficients */
	for (i=0; ; i++)
	{
		code = xShow_Bits(16,ld);
		if (code>=16384)
		{
			if (i==0)
				tab = &DCTtabfirst[(code>>12)-4];
			else
				tab = &DCTtabnext[(code>>12)-4];
		}
		else if (code>=1024)
			tab = &DCTtab0[(code>>8)-4];
		else if (code>=512)
			tab = &DCTtab1[(code>>6)-8];
		else if (code>=256)
			tab = &DCTtab2[(code>>4)-16];
		else if (code>=128)
			tab = &DCTtab3[(code>>3)-16];
		else if (code>=64)
			tab = &DCTtab4[(code>>2)-16];
		else if (code>=32)
			tab = &DCTtab5[(code>>1)-16];
		else if (code>=16)
			tab = &DCTtab6[code-16];
		else
		{
			printf("invalid Huffman code in Decode_MPEG1_Non_Intra_Block()\n");
			ld->Fault_Flag = 1;
			return;
		}
		
		Flush_Buffer(tab->len,ld);
		
		if (tab->run==64) /* end_of_block */
			return;
		
		if (tab->run==65) /* escape */
		{
			i+= Get_Bits(6,ld);
			
			val = Get_Bits(8,ld);
			if (val==0)
				val = Get_Bits(8,ld);
			else if (val==128)
				val = Get_Bits(8,ld) - 256;
			else if (val>128)
				val -= 256;
			
			if((sign = (val<0)))
				val = -val;
		}
		else
		{
			i+= tab->run;
			val = tab->level;
			sign = Get_Bits(1,ld);
		}
		
		if (i>=64)
		{
			printf("DCT coeff index (i) out of bounds (inter)\n");
			ld->Fault_Flag = 1;
			return;
		}
		
		j = scan[i];
		k = scan_mmx[i];
		val = (((val<<1)+1)*ld->quantizer_scale*ld->non_intra_quantizer_matrix[j]) >> 4;
		
		/* mismatch control ('oddification') */
		if (val!=0) /* should always be true, but it's not guaranteed */
			val = (val-1) | 1; /* equivalent to: if ((val&1)==0) val = val - 1; */
		
		/* saturation */
		if (!sign)
			bp[k] = (val>2047) ?  2047 :  val; /* positive */
		else
			bp[k] = (val>2048) ? -2048 : -val; /* negative */
	}
}
