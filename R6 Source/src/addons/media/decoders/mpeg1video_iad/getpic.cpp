/* getpic.c, picture decoding                                               */

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
#include <string.h>
#include <malloc.h>
#include "global.h"

#include "block_mmx.h"


#define Q 10
int fc;

static unsigned char ClipData[1024]={
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
};
static unsigned char *Clip=ClipData+384;


// all static functions should be inline
inline void Clear_Block(int comp,video_data *ld)
{
	short *Block_Ptr;
	int i;
	
	Block_Ptr = ld->block[comp];
	
	memset(Block_Ptr, 0, sizeof(short)*64);
}

/* ISO/IEC 13818-2 section 7.6.6 */
inline void skipped_macroblock(int dc_dct_pred[3],int PMV[2][2],
	int *macroblock_type,video_data *ld)
{
	int comp;
	
	for (comp=0; comp<6; comp++)
		Clear_Block(comp,ld);
	
	/* reset intra_dc predictors */
	/* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
	dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;
	
	/* reset motion vector predictors */
	/* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
	if (ld->picture_coding_type==P_TYPE)
		PMV[0][0]=PMV[0][1]=0;
	
	/* IMPLEMENTATION: clear MACROBLOCK_INTRA */
	*macroblock_type&= ~MACROBLOCK_INTRA;
}


/* return==-1 means go to next picture */
/* the expression "start of slice" is used throughout the normative
   body of the MPEG specification */
inline int start_of_slice(int *MBA,int *MBAinc,
	int dc_dct_pred[3],int PMV[2][2],video_data *ld)
{
	unsigned int code;
	int slice_vert_pos_ext;
	
	ld->Fault_Flag = 0;
	
	next_start_code(ld);
	code = ld->Bfr;
	
	if (code<SLICE_START_CODE_MIN || code>SLICE_START_CODE_MAX)
	{
		/* only slice headers are allowed in picture_data */
		printf("start_of_slice(): Premature end of picture\n");
	
		return(-1);  /* trigger: go to next picture */
	}
	
	Flush_Buffer32(ld); 
	
	/* decode slice header (may change quantizer_scale) */
	slice_vert_pos_ext = slice_header(ld);
	
	/* decode macroblock address increment */
	*MBAinc = Get_macroblock_address_increment(ld);
	
	if (ld->Fault_Flag) 
	{
		printf("start_of_slice(): MBAinc unsuccessful\n");
		return(0);   /* trigger: go to next slice */
	}
	
	/* set current location */
	/* NOTE: the arithmetic used to derive macroblock_address below is
	*       equivalent to ISO/IEC 13818-2 section 6.3.17: Macroblock
	*/
	*MBA = ((slice_vert_pos_ext<<7) + (code&255) - 1)*ld->mb_width + *MBAinc - 1;
	*MBAinc = 1; /* first macroblock in slice: not skipped */
	
	/* reset all DC coefficient and motion vector predictors */
	/* reset all DC coefficient and motion vector predictors */
	/* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
	dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;
	
	/* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
	PMV[0][0]=PMV[0][1]=PMV[1][0]=PMV[1][1]=0;
	
	/* successfull: trigger decode macroblocks in slice */
	return(1);
}


 
/* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
inline void macroblock_modes(int *pmacroblock_type,video_data *ld)
{
	int macroblock_type;
	
	/* get macroblock_type */
	macroblock_type = Get_macroblock_type(ld);
	
	if (ld->Fault_Flag) return;
	
	/* return values */
	*pmacroblock_type = macroblock_type;
}


/* calculate motion vector component */
/* ISO/IEC 13818-2 section 7.6.3.1: Decoding the motion vectors */
/* Note: the arithmetic here is more elegant than that which is shown 
   in 7.6.3.1.  The end results (PMV[][][]) should, however, be the same.  */

inline void decode_motion_vector(int *pred,int r_size, int motion_code, int motion_residual,int full_pel_vector)
{
	int lim, vec;
	
	lim = 16<<r_size;
	vec = full_pel_vector ? (*pred >> 1) : (*pred);
	
	if (motion_code>0)
	{
		vec+= ((motion_code-1)<<r_size) + motion_residual + 1;
		if (vec>=lim)
			vec-= lim + lim;
	}
	else
	if (motion_code<0)
	{
		vec-= ((-motion_code-1)<<r_size) + motion_residual + 1;
		if (vec<-lim)
			vec+= lim + lim;
	}
	*pred = full_pel_vector ? (vec<<1) : vec;
}

/* get and decode motion vector and differential motion vector 
   for one prediction */
inline void motion_vector(int *PMV,int r_size,int full_pel_vector,video_data *ld)
{
	int motion_code, motion_residual;
	
	/* horizontal component */
	/* ISO/IEC 13818-2 Table B-10 */
	motion_code = Get_motion_code(ld);
	
	motion_residual = (r_size!=0 && motion_code!=0) ? Get_Bits(r_size,ld) : 0;
	
	decode_motion_vector(&PMV[0],r_size,motion_code,motion_residual,full_pel_vector);
	
	/* vertical component */
	motion_code     = Get_motion_code(ld);
	motion_residual = (r_size!=0 && motion_code!=0) ? Get_Bits(r_size,ld) : 0;
	
	decode_motion_vector(&PMV[1],r_size,motion_code,motion_residual,full_pel_vector);
}

/* ISO/IEC 13818-2 sections 7.2 through 7.5 */
inline int decode_macroblock(int *macroblock_type,int PMV[2][2],
	int dc_dct_pred[3],video_data *ld)
{
	/* locals */
	int quantizer_scale_code; 
	int comp;
	
	int coded_block_pattern;
	
	/* ISO/IEC 13818-2 section 6.3.17.1: Macroblock modes */
	macroblock_modes(macroblock_type,ld);
	if (ld->Fault_Flag) return(0);  /* trigger: go to next slice */
	
	if (*macroblock_type & MACROBLOCK_QUANT)
	{
		quantizer_scale_code = Get_Bits(5,ld);
		
		/* ISO/IEC 13818-2 section 7.4.2.2: Quantizer scale factor */
		ld->quantizer_scale = quantizer_scale_code;
	}
	
	/* motion vectors */
	/* decode forward motion vectors */
	if (*macroblock_type & MACROBLOCK_MOTION_FORWARD)
	{
		motion_vector(PMV[0],ld->forward_f_code-1,ld->full_pel_forward_vector,ld);
	}

	if (ld->Fault_Flag) return(0);  /* trigger: go to next slice */
	
	/* decode backward motion vectors */
	if (*macroblock_type & MACROBLOCK_MOTION_BACKWARD)
	{
		motion_vector(PMV[1],ld->backward_f_code-1,ld->full_pel_backward_vector,ld);
	}
	
	if (ld->Fault_Flag) return(0);  /* trigger: go to next slice */
	
	/* macroblock_pattern */
	/* ISO/IEC 13818-2 section 6.3.17.4: Coded block pattern */
	if (*macroblock_type & MACROBLOCK_PATTERN)
	{
		coded_block_pattern = Get_coded_block_pattern(ld);
	}
	else
		coded_block_pattern = (*macroblock_type & MACROBLOCK_INTRA) ? (1<<6)-1 : 0;
	if (ld->Fault_Flag) return(0);  /* trigger: go to next slice */

	/* decode blocks */
	for (comp=0; comp<6; comp++)
	{
		Clear_Block(comp,ld);
		
		if (coded_block_pattern & (1<<(5-comp)))
		{
			if (*macroblock_type & MACROBLOCK_INTRA)
			{
				Decode_MPEG1_Intra_Block(comp,dc_dct_pred,ld);
			}
			else
			{
				Decode_MPEG1_Non_Intra_Block(comp,ld);
			}
		
			if (ld->Fault_Flag) return(0);  /* trigger: go to next slice */
		}
	}
	if (ld->picture_coding_type==D_TYPE)
	{
		/* remove end_of_macroblock (always 1, prevents startcode emulation) */
		/* ISO/IEC 11172-2 section 2.4.2.7 and 2.4.3.6 */
		marker_bit("D picture end_of_macroblock bit",ld);
	}

	/* reset intra_dc predictors */
	/* ISO/IEC 13818-2 section 7.2.1: DC coefficients in intra blocks */
	if (!(*macroblock_type & MACROBLOCK_INTRA))
		dc_dct_pred[0]=dc_dct_pred[1]=dc_dct_pred[2]=0;
	
	/* reset motion vector predictors */
	if (*macroblock_type & MACROBLOCK_INTRA)
	{
		/* intra mb without concealment motion vectors */
		/* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
		PMV[0][0]=PMV[0][1]=
		PMV[1][0]=PMV[1][1]=0;
	}

	/* special "No_MC" macroblock_type case */
	/* ISO/IEC 13818-2 section 7.6.3.5: Prediction in P pictures */
	if ((ld->picture_coding_type==P_TYPE) 
		&& !(*macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_INTRA)))
	{
		/* non-intra mb without forward mv in a P picture */
		/* ISO/IEC 13818-2 section 7.6.3.4: Resetting motion vector predictors */
		PMV[0][0]=PMV[0][1]=0;
	}
	
	/* successfully decoded macroblock */
	return(1);
} /* decode_macroblock */


/* move/add 8x8-Block from block[comp] to backward_reference_frame */
/* copy reconstructed 8x8 block from block[comp] to current_frame[]
 * ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data
 * This stage also embodies some of the operations implied by:
 *   - ISO/IEC 13818-2 section 7.6.7: Combining predictions
 *   - ISO/IEC 13818-2 section 6.1.3: Macroblock
*/
// I think this is pretty much maxed out at optimization, with the exception of
// MMX saturation code.
inline void Add_Block(int comp, int bx, int by, int addflag,int nonzeros,video_data *ld)
{
	int cc, i, iincr;
	unsigned char *rfp;
	short *bp;
	
	/* derive color component index */
	/* equivalent to ISO/IEC 13818-2 Table 7-1 */
	cc = (comp<4) ? 0 : (comp&1)+1; /* color component index */
	
	#if ROTATE_DISPLAY
	if (cc==0)
	{
		/* luminance */	
		/* frame DCT coding */
	
		if((comp&3) == 0)
		{
			rfp = ld->current_frame[0]+ld->Coded_Height*(bx)+ld->Coded_Height-by-8 ;
		}
		
		if((comp&3) == 1)
		{
			rfp = ld->current_frame[0]+ld->Coded_Height*(bx+8)+ld->Coded_Height-by-8;
		}
		
		if((comp&3) == 2)
		{
			rfp = ld->current_frame[0]+ld->Coded_Height*(bx)+ld->Coded_Height-by-16;
		}
		
		if((comp&3) == 3)
		{
			rfp = ld->current_frame[0]+ld->Coded_Height*(bx+8)+ld->Coded_Height-by-16;
		}
		
		iincr = ld->Coded_Height;
	}
	else
	{
		/* chrominance */	
		/* scale coordinates */
		bx >>= 1;
		by >>= 1;
		/* frame DCT coding */
	
		rfp = ld->current_frame[cc]+ld->Chroma_Height*(bx)+ld->Chroma_Height-by-8;
		iincr = ld->Chroma_Height;
	}
	#else
	if (cc==0)
	{
		/* luminance */
		
		/* frame DCT coding */
		rfp = ld->current_frame[0]+ld->Coded_Width*(by+((comp&2)<<2))+bx+((comp&1)<<3);
		iincr = ld->Coded_Width;
	}
	else
	{
		/* chrominance */
		
		/* scale coordinates */
		bx >>= 1;
		by >>= 1;
		/* frame DCT coding */
		rfp = ld->current_frame[cc]+ld->Chroma_Width*(by+((comp&2)<<2))+bx+(comp&8);
		iincr = ld->Chroma_Width;
	}
	#endif
	
	bp = ld->block[comp];


	if (addflag)
	{
		if (nonzeros)
		{
			/*
			for (i=0; i<8; i++)
			{
				rfp[0]=Clip[bp[0]+rfp[0]];
				rfp[1]=Clip[bp[1]+rfp[1]];
				rfp[2]=Clip[bp[2]+rfp[2]];
				rfp[3]=Clip[bp[3]+rfp[3]];
				rfp[4]=Clip[bp[4]+rfp[4]];
				rfp[5]=Clip[bp[5]+rfp[5]];
				rfp[6]=Clip[bp[6]+rfp[6]];
				rfp[7]=Clip[bp[7]+rfp[7]];
				bp+=8;
				rfp+= iincr;
			}
			*/
			add_block_mmx(rfp,bp,iincr);
		}
	}
	else
	{
		if (nonzeros)
		{
			/*
			for (i=0; i<8; i++)
			{
				rfp[0]=Clip[bp[0]+128];
				rfp[1]=Clip[bp[1]+128];
				rfp[2]=Clip[bp[2]+128];
				rfp[3]=Clip[bp[3]+128];
				rfp[4]=Clip[bp[4]+128];
				rfp[5]=Clip[bp[5]+128];
				rfp[6]=Clip[bp[6]+128];
				rfp[7]=Clip[bp[7]+128];
				bp+=8;
				rfp+= iincr;
			}
			*/
			copy_block_mmx(rfp,bp,iincr);
		}
		else
		{
			for (i=0; i<8; i++)
			{
				rfp[0]=rfp[1]=rfp[2]=rfp[3]=rfp[4]=rfp[5]=rfp[6]=rfp[7]=128;
				bp+=8;
				rfp+= iincr;
			}
		}
	}
}

/* ISO/IEC 13818-2 section 7.6 */
inline void motion_compensation(int MBA,int macroblock_type,
	int PMV[2][2],video_data *ld)
{
	int bx, by;
	int comp;
	
	/* derive current macroblock position within picture */
	/* ISO/IEC 13818-2 section 6.3.1.6 and 6.3.1.7 */
	bx = 16*(MBA%ld->mb_width);
	by = 16*(MBA/ld->mb_width);
	
	/* motion compensation */
	// uses 30% (6.4) ->  36% (5.9)
	if (!(macroblock_type & MACROBLOCK_INTRA))
		form_predictions(bx,by,macroblock_type,PMV,ld);
	
	/* copy or add block data into picture */
	for (comp=0; comp<6; comp++)
	{
		/* ISO/IEC 13818-2 section Annex A: inverse DCT */
		// uses 33% (7.1) -> 30% (5.25))
		int zeros;
		//zeros=Fast_IDCT(ld->block[comp]);
		
		
		
		
		//idct_mmx(ld->block[comp]);
		
		idct(ld->block[comp]);
		
		
		zeros = 1;
		
		/* ISO/IEC 13818-2 section 7.6.8: Adding prediction and coefficient data */
		// uses 20% (4.0) -> 10% (1.72)
		
		Add_Block(comp,bx,by,(macroblock_type & MACROBLOCK_INTRA)==0,zeros,ld);
		
	}
}



/* decode all macroblocks of the current picture */
/* ISO/IEC 13818-2 section 6.3.16 */
inline int slice(int MBAmax,video_data *ld)
{
	int MBA; 
	int MBAinc, macroblock_type;
	int dc_dct_pred[3];
	int PMV[2][2];
	int SNRMBA, SNRMBAinc;
	int ret;
	
	MBA = 0; /* macroblock address */
	MBAinc = 0;
	
	if((ret=start_of_slice(&MBA, &MBAinc, dc_dct_pred, PMV,ld))!=1)
		return(ret);
	
	ld->Fault_Flag=0;
	
	for (;;)
	{
		/* this is how we properly exit out of picture */
		if (MBA>=(MBAmax))
			return(-1); /* all macroblocks decoded */
		
		if (MBAinc==0)
		{
			if (!xShow_Bits(23,ld) || ld->Fault_Flag) /* next_start_code or fault */
				ld->Fault_Flag = 1;
			else /* decode macroblock address increment */
				MBAinc = Get_macroblock_address_increment(ld);
			
			if (ld->Fault_Flag)
				return(ld->Fault_Flag=0);     /* trigger: go to next slice */
		}
		
		if (MBA>=MBAmax)
		{
			/* MBAinc points beyond picture dimensions */
			//      if (!Quiet_Flag)
			printf("Too many macroblocks in picture\n");
			return(-1);
		}

		if (MBAinc==1) /* not skipped */
		{
			ret = decode_macroblock(&macroblock_type,PMV, dc_dct_pred,ld);
			if(ret==-1)
				return(-1);
			
			if(ret==0)
				return (ld->Fault_Flag=0);     /* trigger: go to next slice */
		}
		else /* MBAinc!=1: skipped macroblock */
		{      
			/* ISO/IEC 13818-2 section 7.6.6 */
			skipped_macroblock(dc_dct_pred,PMV,&macroblock_type,ld);
		}
	
		/* ISO/IEC 13818-2 section 7.6 */
		// could skip this part if we're skipping a frame
		// this is 75% of the cpu time
		
		
		motion_compensation(MBA, macroblock_type, PMV, ld);
		//printf("Motion Compensation done\n");
	
		/* advance to next macroblock */
		MBA++;
		MBAinc--;
	
		if (MBA>=MBAmax)
			return(-1); /* all macroblocks decoded */
	}
}


/* decode all macroblocks of the current picture */
/* stages described in ISO/IEC 13818-2 section 7 */
inline void picture_data(video_data *ld)
{
	int MBAmax;
	
	/* number of macroblocks per picture */
	MBAmax = ld->mb_width*ld->mb_height;
	
	while (!slice(MBAmax,ld));
}


/* reuse old picture buffers as soon as they are no longer needed 
   based on life-time axioms of MPEG */
inline void Update_Picture_Buffers(video_data *ld)
{                           
	int cc;              /* color component index */
	unsigned char *tmp;  /* temporary swap pointer */

	// 3 buffers, Y, U, and V	
	for (cc=0; cc<3; cc++)
	{
		/* B pictures do not need to be save for future reference */
		if (ld->picture_coding_type==B_TYPE)
		{
			ld->current_frame[cc] = ld->auxframe[cc];
		}
		else
		{
			/* only update at the beginning of the coded frame */
			tmp = ld->forward_reference_frame[cc];
			
			/* the previously decoded reference frame is stored
			coincident with the location where the backward 
			reference frame is stored (backwards prediction is not
			needed in P pictures) */
			ld->forward_reference_frame[cc] = ld->backward_reference_frame[cc];
			
			/* update pointer for potential future B pictures */
			ld->backward_reference_frame[cc] = tmp;
			
			/* can erase over old backward reference frame since it is not used
			in a P picture, and since any subsequent B pictures will use the 
			previously decoded I or P frame as the backward_reference_frame */
			ld->current_frame[cc] = ld->backward_reference_frame[cc];
		}
	}
}


/* decode one frame or field picture */
void Decode_Picture(video_data *ld)
{
	/* IMPLEMENTATION: update picture buffer pointers */
	Update_Picture_Buffers(ld);
	
	/* decode picture data ISO/IEC 13818-2 section 6.2.3.7 */
	picture_data(ld);
}
