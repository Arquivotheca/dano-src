/* gethdr.c, header decoding                                                */

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



static const unsigned char default_intra_quantizer_matrix[64]=
{
  8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
};

#define RESERVED    -1 
static const double frame_rate_Table[16] =
{
  0.0,
  ((24.0*1000.0)/1001.0),
  24.0,
  25.0,
  ((30.0*1000.0)/1001.0),
  30.0,
  50.0,
  ((60.0*1000.0)/1001.0),
  60.0,
 
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED,
  RESERVED
};


/* ignore extension and user data */
/* ISO/IEC 11172-2 section D5.3.10-11 */
inline void extension_and_user_data(video_data *ld)
{
	int code;
	
	next_start_code(ld);
	
	while ((code = Show_Bits(32,ld))==EXTENSION_START_CODE || code==USER_DATA_START_CODE)
	{
		Get_Bits32(ld);
		next_start_code(ld);
	}
}



/* decode sequence header */
inline void sequence_header(video_data *ld)
{
	int i;
	int pos;
	int aspect_ratio_information;
	int constrained_parameters_flag;
	
	pos = ld->Bitcnt;
	ld->horizontal_size             = Get_Bits(12,ld);
	ld->vertical_size               = Get_Bits(12,ld);
	aspect_ratio_information    = Get_Bits(4,ld);
	ld->frame_rate = frame_rate_Table[Get_Bits(4,ld)];
	ld->bit_rate=Get_Bits(18,ld)*400;
//	printf("bit_rate=%d, frame_rate=%g\n",ld->bit_rate,ld->frame_rate);
	marker_bit("sequence_header()",ld);
	ld->vbv_buffer_size             = Get_Bits(10,ld);
	constrained_parameters_flag = Get_Bits(1,ld);
	
	
	// load intra quantizer matrix?
	if (Get_Bits(1,ld))
	{
		for (i=0; i<64; i++)
			ld->intra_quantizer_matrix[scan[i]] = Get_Bits(8,ld);
	}
	else
	{
		for (i=0; i<64; i++)
			ld->intra_quantizer_matrix[i] = default_intra_quantizer_matrix[i];
	}
	
	// load non intra quantizer matrix?
	if (Get_Bits(1,ld))
	{
		for (i=0; i<64; i++)
			ld->non_intra_quantizer_matrix[scan[i]] = Get_Bits(8,ld);
	}
	else
	{
		for (i=0; i<64; i++)
			ld->non_intra_quantizer_matrix[i] = 16;
	}
	
	extension_and_user_data(ld);
}


/* decode group of pictures header */
/* ISO/IEC 13818-2 section 6.2.2.6 */
inline void group_of_pictures_header(video_data *ld)
{
	int drop_flag;
	int hour;
	int minute;
	int sec;
	int frame;
	int closed_gop;
	int broken_link;
	
	ld->Temporal_Reference_Base = ld->True_Framenum_max + 1; 	/* *CH* */
	ld->Temporal_Reference_GOP_Reset = 1;
	drop_flag   = Get_Bits(1,ld);
	hour        = Get_Bits(5,ld);
	minute      = Get_Bits(6,ld);
	marker_bit("group_of_pictures_header()",ld);
	sec         = Get_Bits(6,ld);
	frame       = Get_Bits(6,ld);
	closed_gop  = Get_Bits(1,ld);
	broken_link = Get_Bits(1,ld);

	ld->timebase.t = (((hour * 60) + minute * 60) + sec) * 1000000LL;
	ld->timebase.baseframe = ld->picture_num - frame;

	extension_and_user_data(ld);
}



/* introduced in September 1995 to assist Spatial Scalability */
inline void Update_Temporal_Reference_Tacking_Data(video_data *ld)
{
	if (ld->picture_coding_type!=B_TYPE && ld->temporal_reference!=ld->temporal_reference_old) 	
	/* check first field of */
	{							
		/* non-B-frame */
		if (ld->temporal_reference_wrap) 		
		{
			/* wrap occured at previous I- or P-frame */	
			/* now all intervening B-frames which could 
			still have high temporal_reference values are done  */
			ld->Temporal_Reference_Base += 1024;
			ld->temporal_reference_wrap = 0;
		}
		
		/* distinguish from a reset */
		if (ld->temporal_reference<ld->temporal_reference_old && !ld->Temporal_Reference_GOP_Reset)	
			ld->temporal_reference_wrap = 1;  /* we must have just passed a GOP-Header! */
		
		ld->temporal_reference_old = ld->temporal_reference;
		ld->Temporal_Reference_GOP_Reset = 0;
	}
	
	ld->True_Framenum = ld->Temporal_Reference_Base + ld->temporal_reference;

	/* temporary wrap of TR at 1024 for M frames */
	if (ld->temporal_reference_wrap && ld->temporal_reference <= ld->temporal_reference_old)	
		ld->True_Framenum += 1024;				
	
	ld->True_Framenum_max = (ld->True_Framenum > ld->True_Framenum_max) ?
		ld->True_Framenum : ld->True_Framenum_max;
}

/* ISO/IEC 13818-2 section 6.2.3 */
inline void picture_header(video_data *ld)
{
	int pos;
	int vbv_delay;

	pos = ld->Bitcnt;
	ld->temporal_reference  = Get_Bits(10,ld);
	ld->picture_coding_type = Get_Bits(3,ld);
	vbv_delay           = Get_Bits(16,ld);
	
	if (ld->picture_coding_type==P_TYPE || ld->picture_coding_type==B_TYPE)
	{
		ld->full_pel_forward_vector = Get_Bits(1,ld);
		ld->forward_f_code = Get_Bits(3,ld);
	}
	if (ld->picture_coding_type==B_TYPE)
	{
		ld->full_pel_backward_vector = Get_Bits(1,ld);
		ld->backward_f_code = Get_Bits(3,ld);
	}
	
	// 11172-2 D5.3.9 : discard extra picture information (a '1', followed by a byte)
	while (Get_Bits(1,ld))
	{
		Get_Bits(8,ld);
	}

	extension_and_user_data(ld);
	
	/* update tracking information used to assist spatial scalability */
	Update_Temporal_Reference_Tacking_Data(ld);
}


/* decode slice header */

/* ISO/IEC 13818-2 section 6.2.4 */
int slice_header(video_data *ld)
{
	ld->quantizer_scale = Get_Bits(5,ld);
	
	// 11172-2 D5.4.3 : discard information slice, if any (a '1', followed by a byte)
	while (Get_Bits(1,ld))
		Get_Bits(8,ld);
	
	return 0;
}


/*
 * decode headers from the input stream
 * until an End of Sequence or picture start code
 * is found
 */
int Get_Hdr(video_data *ld)
{
	unsigned int code;
	int retval=1;

	if (ld->picture_coding_type==I_TYPE ||
		ld->picture_coding_type==P_TYPE)
		ld->Ref_Framenum=ld->True_Framenum;
		
	for (;;)
	{
		/* look for next_start_code */
		next_start_code(ld);
		code = Get_Bits32(ld);
		
		if (code==SEQUENCE_HEADER_CODE)
		{
				sequence_header(ld);
		}
		else
		if (code==GROUP_START_CODE)
		{
				group_of_pictures_header(ld);
		}
		else
		if (code==PICTURE_START_CODE)
		{
				picture_header(ld);
				break;
		}
		else
		if (code==SEQUENCE_END_CODE)
		{
				retval=0;
				break;
		}
		else
		{
				printf("Unexpected next_start_code %08x (ignored)\n",code);
		}
	}
	return retval;
}


/* align to start of next next_start_code */
void next_start_code(video_data *ld)
{
	unsigned long	v;
	
	/* byte align */
	Flush_Buffer(ld->Incnt&7,ld);
	while ((v = Show_Bits(24,ld))!=0x01L)
	{
		v = Show_Bits(24,ld);
		Flush_Buffer(8,ld);
	}
}


/* ISO/IEC 13818-2 section 5.3 */
/* Purpose: this function is mainly designed to aid in bitstream conformance
   testing.  A simple Flush_Buffer(1) would do */
void marker_bit(char *text,video_data *ld)
{
	int marker;
	
	marker = Get_Bits(1,ld);
}
