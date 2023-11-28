
/* mpeg2dec.c, main(), initialization, option processing                    */

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
//----------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <OS.h>

#define GLOBAL
#include "global.h"

#include "idct_mmx.h"



//----------------------------------------------------------------

#define	REWIND	0x01
#define	PAUSE	0x02
#define	END		0x03

//----------------------------------------------------------------

#if 0
long	g_vpos = 0;
long	g_hpos = 0;
long	g_rowbyte = 800*4;
long	mode = 0;
#endif

int checkVheader(video_data *ld);
void Initialize_Sequence(video_data *ld);
void(*idct)(short*);


//----------------------------------------------------------------

video_data *newMVideo(void *obj,int (*rf)(void*,void*,int))
{
	video_data *ld=(video_data*)calloc(sizeof(video_data), 1);
	if (ld)
	{
		ld->streamObj=obj;
		ld->streamRead=rf;
		ld->Temporal_Reference_Base = 0;
		ld->Ref_Framenum=-1;
		ld->True_Framenum_max  = -1;
		ld->Temporal_Reference_GOP_Reset = 0;
		ld->temporal_reference_wrap=0;
		ld->temporal_reference_old=0;
	}
	
	
	idct = idct_mmx;
	
	cpuid_info  info,infobis,infoter;
	get_cpuid(&info,0,0);
	
	
	if((info.regs.ebx==0x68747541)
		&&(info.regs.ecx==0x444d4163)		
		&&(info.regs.edx==0x69746e65))
	{
		//printf("authenticAMD\n");
		get_cpuid(&infobis,0x80000000,0);
		if(infobis.regs.eax>=0x80000001)
		{
			//printf("CPUID 0x80000001\n");
			get_cpuid(&infoter,0x80000001,0);
			if((infoter.regs.edx&0x80000000)==0x80000000)
			{
				idct = idct_mmx_3dnow;
				//printf("MPEG1 video decoder uses MMX-3dnow!\n");
			}
		}
	}
	
		
	
	
	
	return ld;
}

void deleteMVideo(video_data *ld)
{
	if (ld) free(ld);
}

int checkVheader(video_data *ld)
{
	int code;
	
	Initialize_Buffer(ld); 
	
	if((ld->Bfr>>24)==0x47)
	{
		printf("Decoder currently does not parse transport streams\n");
		return -1;
	}

	next_start_code(ld);
	code = ld->Bfr;
	
	if (code!=SEQUENCE_HEADER_CODE)
	{
		printf("Expecting SEQUENCE_HEADER_CODE (%08x), got %08x.\n",
			SEQUENCE_HEADER_CODE,code);
		return -1;
	}
	return 0;
}

//----------------------------------------------------------------

/* mostly IMPLEMENTAION specific rouintes */
void Initialize_Sequence(video_data *ld)
{
	int cc, size,i;
	
	/* round to nearest multiple of coded macroblocks */
	/* ISO/IEC 13818-2 section 6.3.3 sequence_header() */
	ld->mb_width = (ld->horizontal_size+15)/16;
	ld->mb_height = (ld->vertical_size+15)/16;
	
	ld->Coded_Width = 16*ld->mb_width;
	ld->Coded_Height = 16*ld->mb_height;
	
	/* ISO/IEC 13818-2 sections 6.1.1.8, 6.1.1.9, and 6.1.1.10 */
	ld->Chroma_Width = ld->Coded_Width>>1;
	ld->Chroma_Height = ld->Coded_Height>>1;
	/*
	for (cc=0; cc<3; cc++)
	{
		if (cc==0)
			size = ld->Coded_Width*ld->Coded_Height;
		else
			size = ld->Chroma_Width*ld->Chroma_Height;
	
		if (!(ld->backward_reference_frame[cc] = (unsigned char *)malloc(size)))
			Error("backward_reference_frame[] malloc failed\n");
		
		if (!(ld->forward_reference_frame[cc] = (unsigned char *)malloc(size)))
			Error("forward_reference_frame[] malloc failed\n");
		
		if (!(ld->auxframe[cc] = (unsigned char *)malloc(size)))
			Error("auxframe[] malloc failed\n");
	}
	*/
	
	
	size = ld->Coded_Width*ld->Coded_Height * 3 / 2;
	
	if (!(ld->backward_reference_frame[3] = (unsigned char *)malloc(size)))
	{
			Error("backward_reference_frame[] malloc failed\n");
	}
	else
	{
		uint add;
		uint align;
		add = (uint)ld->backward_reference_frame[3];
		align = (add + 7) / 8;
		//printf("add %ld %ld \n",align*8,add);
		align = 8 * align - add;
		//printf("alignment %ld \n",align);
		ld->backward_reference_frame[0] = ld->backward_reference_frame[3]
										 + align;
		ld->backward_reference_frame[1] = ld->backward_reference_frame[0]
										 + ld->Coded_Width*ld->Coded_Height;
		
		ld->backward_reference_frame[2] = ld->backward_reference_frame[1]
										 + ld->Coded_Width*ld->Coded_Height / 4;
	}
	
	if (!(ld->forward_reference_frame[3] = (unsigned char *)malloc(size)))
	{
			Error("backward_reference_frame[] malloc failed\n");
	}
	else
	{
		uint add;
		uint align;
		add = (uint)ld->forward_reference_frame[3];
		align = (add + 7) / 8;
		//printf("add %ld %ld \n",align*8,add);
		align = 8 * align - add;
		//printf("alignment %ld \n",align);
		ld->forward_reference_frame[0] = ld->forward_reference_frame[3]
										 + align;
		ld->forward_reference_frame[1] = ld->forward_reference_frame[0]
										 + ld->Coded_Width*ld->Coded_Height;
		
		ld->forward_reference_frame[2] = ld->forward_reference_frame[1]
										 + ld->Coded_Width*ld->Coded_Height / 4;
	}
	
	if (!(ld->auxframe[3] = (unsigned char *)malloc(size)))
	{
			Error("backward_reference_frame[] malloc failed\n");
	}
	else
	{
		uint add;
		uint align;
		add = (uint)ld->auxframe[3];
		align = (add + 7) / 8;
		//printf("add %ld %ld \n",align*8,add);
		align = 8 * align - add;
		//printf("alignment %ld \n",align);
		ld->auxframe[0] = ld->auxframe[3]
						+ align;
		ld->auxframe[1] = ld->auxframe[0]
						 + ld->Coded_Width*ld->Coded_Height;
		
		ld->auxframe[2] = ld->auxframe[1]
						 + ld->Coded_Width*ld->Coded_Height / 4;
	}
	
	
	
	
	
	//check alignment on 64 bits
	ld->ppblock = (short**)malloc(12*sizeof(short*));
	ld->block = (short**)malloc(12*sizeof(short*));
	for(i=0;i<12;i++)
	{
		uint add;
		uint align;
		ld->ppblock[i] = (short*)malloc(65*sizeof(short));
		add = (uint)ld->ppblock[i];
		align = (add + 7) / 8;
		//printf("add %ld %ld \n",align*8,add);
		align = 8 * align - add;
		//printf("alignment %ld \n",align);
		ld->block[i] = ld->ppblock[i] +  align;
	}
}

void Deinitialize_Sequence(video_data *ld)
{
	int i;
	short * pblock;
	/*
	for(i=0;i<3;i++)
	{
		free(ld->backward_reference_frame[i]);
		free(ld->forward_reference_frame[i]);
		free(ld->auxframe[i]);
	}
	*/
	
	free(ld->backward_reference_frame[3]);
	free(ld->forward_reference_frame[3]);
	free(ld->auxframe[3]);
	
	for(i=0;i<12;i++)
	{
		pblock = ld->ppblock[i];
		free(pblock);
	}	
	free(ld->ppblock);
	free(ld->block);
}

//-------------------------------------------------------------


/* IMPLEMENTAION specific rouintes */


//----------------------------------------------------------------

void Error(char *text)
{
	printf(text);
	exit(1);
}
