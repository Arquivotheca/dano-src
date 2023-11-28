/* getvlc.c, variable length decoding                                       */

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
#include "getvlc.h"


static int Get_I_macroblock_type(video_data *ld)
{
	if (Get_Bits(1,ld))
		return 1;
	
	if (!Get_Bits(1,ld))
	{
		printf("Invalid macroblock_type code\n");
		ld->Fault_Flag = 1;
	}
	
	return 17;
}

static int Get_P_macroblock_type(video_data *ld)
{
	int code;
	
	if ((code = xShow_Bits(6,ld))>=8)
	{
		code >>= 3;
		Flush_Buffer(PMBtab0[code].len,ld);
		return PMBtab0[code].val;
	}
	
	if (code==0)
	{
		printf("Invalid macroblock_type code\n");
		ld->Fault_Flag = 1;
		return 0;
	}
	
	Flush_Buffer(PMBtab1[code].len,ld);
	
	return PMBtab1[code].val;
}

static int Get_B_macroblock_type(video_data *ld)
{
	int code;
	
	if ((code = xShow_Bits(6,ld))>=8)
	{
		code >>= 2;
		Flush_Buffer(BMBtab0[code].len,ld);
		
		return BMBtab0[code].val;
	}
	
	if (code==0)
	{
		printf("Invalid macroblock_type code\n");
		ld->Fault_Flag = 1;
		return 0;
	}
	
	Flush_Buffer(BMBtab1[code].len,ld);
	
	return BMBtab1[code].val;
}

static int Get_D_macroblock_type(video_data *ld)
{
	if (!Get_Bits(1,ld))
	{
		printf("Invalid macroblock_type code\n");
		ld->Fault_Flag=1;
	}
	
	return 1;
}

int Get_macroblock_type(video_data *ld)
{
	int macroblock_type = 0;
	
	switch (ld->picture_coding_type)
	{
		case I_TYPE:
			macroblock_type = Get_I_macroblock_type(ld);
			break;
		case P_TYPE:
			macroblock_type = Get_P_macroblock_type(ld);
			break;
		case B_TYPE:
			macroblock_type = Get_B_macroblock_type(ld);
			break;
		case D_TYPE:
			macroblock_type = Get_D_macroblock_type(ld);
			break;
		default:
			printf("Get_macroblock_type(): unrecognized picture coding type\n");
			break;
	}

	return macroblock_type;
}

int Get_motion_code(video_data *ld)
{
	int code;
	
	if (Get_Bits(1,ld))
		return 0;
	
	if ((code = xShow_Bits(9,ld))>=64)
	{
		code >>= 6;
		Flush_Buffer(MVtab0[code].len,ld);
		
		return Get_Bits(1,ld)?-MVtab0[code].val:MVtab0[code].val;
	}
	
	if (code>=24)
	{
		code >>= 3;
		Flush_Buffer(MVtab1[code].len,ld);
		
		return Get_Bits(1,ld)?-MVtab1[code].val:MVtab1[code].val;
	}
	
	if ((code-=12)<0)
	{
		printf("Invalid motion_vector code\n");
		ld->Fault_Flag=1;
		return 0;
	}
	
	Flush_Buffer(MVtab2[code].len,ld);
	
	return Get_Bits(1,ld) ? -MVtab2[code].val : MVtab2[code].val;
}

int Get_coded_block_pattern(video_data *ld)
{
	int code;
	
	if ((code = xShow_Bits(9,ld))>=128)
	{
		code >>= 4;
		Flush_Buffer(CBPtab0[code].len,ld);
		
		return CBPtab0[code].val;
	}
	
	if (code>=8)
	{
		code >>= 1;
		Flush_Buffer(CBPtab1[code].len,ld);
		
		return CBPtab1[code].val;
	}
	
	if (code<1)
	{
		printf("Invalid coded_block_pattern code\n");
		ld->Fault_Flag = 1;
		return 0;
	}
	
	Flush_Buffer(CBPtab2[code].len,ld);
	
	return CBPtab2[code].val;
}

int Get_macroblock_address_increment(video_data *ld)
{
	int code, val;
	
	val = 0;
	
	while ((code = xShow_Bits(11,ld))<24)
	{
		if (code!=15) /* if not macroblock_stuffing */
		{
			if (code==8) /* if macroblock_escape */
				val+= 33;
			else
			{
				printf("Invalid macroblock_address_increment code\n");
				
				ld->Fault_Flag = 1;
				return 1;
			}
		}
		
		Flush_Buffer(11,ld);
	}

	/* macroblock_address_increment == 1 */
	/* ('1' is in the MSB position of the lookahead) */
	if (code>=1024)
	{
		Flush_Buffer(1,ld);
		return val + 1;
	}
	
	/* codes 00010 ... 011xx */
	if (code>=128)
	{
		/* remove leading zeros */
		code >>= 6;
		Flush_Buffer(MBAtab1[code].len,ld);
		
		return val + MBAtab1[code].val;
	}
	
	/* codes 00000011000 ... 0000111xxxx */
	code-= 24; /* remove common base */
	Flush_Buffer(MBAtab2[code].len,ld);
	
	return val + MBAtab2[code].val;
}

/* parse VLC and perform dct_diff arithmetic.

   MPEG-1:  ISO/IEC 11172-2 section
   
   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/

int Get_Luma_DC_dct_diff(video_data *ld)
{
	int code, size, dct_diff;
	
	/* decode length */
	code = xShow_Bits(5,ld);
	
	if (code<31)
	{
		size = DClumtab0[code].val;
		Flush_Buffer(DClumtab0[code].len,ld);
	}
	else
	{
		code = xShow_Bits(9,ld) - 0x1f0;
		size = DClumtab1[code].val;
		Flush_Buffer(DClumtab1[code].len,ld);
	}
	
	if (size==0)
		dct_diff = 0;
	else
	{
		dct_diff = Get_Bits(size,ld);
		if ((dct_diff & (1<<(size-1)))==0)
			dct_diff-= (1<<size) - 1;
	}
	
	return dct_diff;
}

int Get_Chroma_DC_dct_diff(video_data *ld)
{
	int code, size, dct_diff;
	
	/* decode length */
	code = xShow_Bits(5,ld);
	
	if (code<31)
	{
		size = DCchromtab0[code].val;
		Flush_Buffer(DCchromtab0[code].len,ld);
	}
	else
	{
		code = xShow_Bits(10,ld) - 0x3e0;
		size = DCchromtab1[code].val;
		Flush_Buffer(DCchromtab1[code].len,ld);
	}
	
	if (size==0)
		dct_diff = 0;
	else
	{
		dct_diff = Get_Bits(size,ld);
		if ((dct_diff & (1<<(size-1)))==0)
		dct_diff-= (1<<size) - 1;
	}
	
	return dct_diff;
}
