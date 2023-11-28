/* Predict.c, motion compensation routines                                    */

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
#include <support2/SupportDefs.h>

#include "global.h"
#include "mc_mmx.h"
#include "mc2_mmx.h"





/* ISO/IEC 13818-2 section 7.6.4: Forming predictions */
/* NOTE: the arithmetic below produces numerically equivalent results
 *  to 7.6.4, yet is more elegant. It differs in the following ways:
 *
 *   1. the vectors (dx, dy) are based on cartesian frame 
 *      coordiantes along a half-pel grid (always positive numbers)
 *      In contrast, vector[r][s][t] are differential (with positive and 
 *      negative values). As a result, deriving the integer vectors 
 *      (int_vec[t]) from dx, dy is accomplished by a simple right shift.
 *
 *   2. Half pel flags (xh, yh) are equivalent to the LSB (Least
 *      Significant Bit) of the half-pel coordinates (dx,dy).
 * 
 *
 *  NOTE: the work of combining predictions (ISO/IEC 13818-2 section 7.6.7)
 *  is distributed among several other stages.  This is accomplished by 
 *  folding line offsets into the source and destination (src,dst)
 *  addresses (note the call arguments to form_prediction() in Predict()),
 *  line stride variables lx and lx2, the block dimension variables (w,h), 
 *  average_flag, and by the very order in which Predict() is called.  
 *  This implementation design (implicitly different than the spec) 
 *  was chosen for its elegance.
*/

inline void form_component_prediction(unsigned char *src,unsigned char *dst,int lx,
	int x,int y,int dx,int dy)
{
	int xint;      /* horizontal integer sample vector: analogous to int_vec[0] */
	int yint;      /* vertical integer sample vectors: analogous to int_vec[1] */
	int xh;        /* horizontal half sample flag: analogous to half_flag[0]  */
	int yh;        /* vertical half sample flag: analogous to half_flag[1]  */
	int j;
	unsigned char *s;    /* source pointer: analogous to pel_ref[][]   */
	unsigned char *d;    /* destination pointer:  analogous to pel_pred[][]  */
	int delta;
	uint32 pt,ptt;
	
	/* half pel scaling for integer vectors */
	xint = dx>>1;
	yint = dy>>1;
	
	/* derive half pel flags */
	xh = dx & 1;
	yh = dy & 1;
	
	/* compute the linear address of pel_ref[][] and pel_pred[][] 
	   based on cartesian/raster cordinates provided */
	s = src + lx*(y+yint) + x + xint;
	pt = (uint32) s;
	ptt = pt & (-8);
	delta = pt - ptt;
	s = (unsigned char *) ptt;
	
	
	d = dst + lx*y + x;
	
	if (!xh && !yh) /* no horizontal nor vertical half-pel */
	{
		mc_hf_vf_mmx16(s,d,lx,delta * 8);
	}
	else
	if (!xh && yh) /* no horizontal but vertical half-pel */
	{
		mc_hf_vh_mmx16(s,d,lx,delta * 8);
	}
	else
	if (xh && !yh) /* horizontal but no vertical half-pel */
	{
		mc_hh_vf_mmx16(s,d,lx,delta * 8);
	}
	else /* if (xh && yh) horizontal and vertical half-pel */
	{
		mc_hh_vh_mmx16(s,d,lx,delta * 8);
	}
}
static void form_component_prediction_chroma(unsigned char *src,unsigned char *dst,int lx,
	int x,int y,int dx,int dy)
{
	int xint;      /* horizontal integer sample vector: analogous to int_vec[0] */
	int yint;      /* vertical integer sample vectors: analogous to int_vec[1] */
	int xh;        /* horizontal half sample flag: analogous to half_flag[0]  */
	int yh;        /* vertical half sample flag: analogous to half_flag[1]  */
	int j;
	unsigned char *s;    /* source pointer: analogous to pel_ref[][]   */
	unsigned char *d;    /* destination pointer:  analogous to pel_pred[][]  */
	int delta;
	uint32 pt,ptt;


	/* half pel scaling for integer vectors */
	xint = dx>>1;
	yint = dy>>1;
	
	/* derive half pel flags */
	xh = dx & 1;
	yh = dy & 1;
	
	/* compute the linear address of pel_ref[][] and pel_pred[][] 
	   based on cartesian/raster cordinates provided */
	s = src + lx*(y+yint) + x + xint;
	pt = (uint32) s;
	ptt = pt & (-8);
	delta = pt - ptt;
	s = (unsigned char *) ptt;
	d = dst + lx*y + x;
	
	if (!xh && !yh) /* no horizontal nor vertical half-pel */
	{
		mc_hf_vf_mmx8(s,d,lx,delta * 8);
	}
	else
	if (!xh && yh) /* no horizontal but vertical half-pel */
	{
		mc_hf_vh_mmx8(s,d,lx,delta * 8);
	}
	else
	if (xh && !yh) /* horizontal but no vertical half-pel */
	{
		mc_hh_vf_mmx8(s,d,lx,delta * 8);
	}
	else /* if (xh && yh) horizontal and vertical half-pel */
	{
		mc_hh_vh_mmx8(s,d,lx,delta * 8);
	}
}
static void form_component_prediction_avg(unsigned char *src,unsigned char *dst,int lx,
	int w,int x,int y,int dx,int dy)
{
	int xint;      /* horizontal integer sample vector: analogous to int_vec[0] */
	int yint;      /* vertical integer sample vectors: analogous to int_vec[1] */
	int xh;        /* horizontal half sample flag: analogous to half_flag[0]  */
	int yh;        /* vertical half sample flag: analogous to half_flag[1]  */
	int j;
	unsigned char *s;    /* source pointer: analogous to pel_ref[][]   */
	unsigned char *d;    /* destination pointer:  analogous to pel_pred[][]  */
	int delta;
	uint32 pt,ptt;
	
	/* half pel scaling for integer vectors */
	xint = dx>>1;
	yint = dy>>1;
	
	/* derive half pel flags */
	xh = dx & 1;
	yh = dy & 1;
	
	/* compute the linear address of pel_ref[][] and pel_pred[][] 
	   based on cartesian/raster cordinates provided */
	s = src + lx*(y+yint) + x + xint;
	pt = (uint32) s;
	ptt = pt & (-8);
	delta = pt - ptt;
	s = (unsigned char *) ptt;
	d = dst + lx*y + x;
	
	if (!xh && !yh) /* no horizontal nor vertical half-pel */
	{
		mc2_hf_vf_mmx16(s,d,lx,delta * 8);
	}
	else
	if (!xh && yh) /* no horizontal but vertical half-pel */
	{		
		mc2_hf_vh_mmx16(s,d,lx,delta * 8);
	}
	else
	if (xh && !yh) /* horizontal but no vertical half-pel */
	{	
		mc2_hh_vf_mmx16(s,d,lx,delta * 8);
	}
	else /* if (xh && yh) horizontal and vertical half-pel */
	{
		mc2_hh_vh_mmx16(s,d,lx,delta * 8);
	}
}


static void form_component_prediction_avg_cr(unsigned char *src,unsigned char *dst,int lx,
	int w,int x,int y,int dx,int dy)
{
	int xint;      /* horizontal integer sample vector: analogous to int_vec[0] */
	int yint;      /* vertical integer sample vectors: analogous to int_vec[1] */
	int xh;        /* horizontal half sample flag: analogous to half_flag[0]  */
	int yh;        /* vertical half sample flag: analogous to half_flag[1]  */
	int j;
	unsigned char *s;    /* source pointer: analogous to pel_ref[][]   */
	unsigned char *d;    /* destination pointer:  analogous to pel_pred[][]  */
	int delta;
	uint32 pt,ptt;
	
	/* half pel scaling for integer vectors */
	xint = dx>>1;
	yint = dy>>1;
	
	/* derive half pel flags */
	xh = dx & 1;
	yh = dy & 1;
	
	/* compute the linear address of pel_ref[][] and pel_pred[][] 
	   based on cartesian/raster cordinates provided */
	s = src + lx*(y+yint) + x + xint;
	pt = (uint32) s;
	ptt = pt & (-8);
	delta = pt - ptt;
	s = (unsigned char *) ptt;
	d = dst + lx*y + x;
	
	if (!xh && !yh) /* no horizontal nor vertical half-pel */
	{
		mc2_hf_vf_mmx8(s,d,lx,delta * 8);
	}
	else
	if (!xh && yh) /* no horizontal but vertical half-pel */
	{		
		mc2_hf_vh_mmx8(s,d,lx,delta * 8);
	}
	else
	if (xh && !yh) /* horizontal but no vertical half-pel */
	{	
		mc2_hh_vf_mmx8(s,d,lx,delta * 8);
	}
	else /* if (xh && yh) horizontal and vertical half-pel */
	{
		mc2_hh_vh_mmx8(s,d,lx,delta * 8);
	}
}

static void form_prediction(
	unsigned char *src[], /* prediction source buffer */
	unsigned char *dst[], /* prediction destination buffer */
	int lx,	              /* line stride */
	int x,int y,          /* pixel co-ordinates of top-left sample in current MB */
	int dx,int dy,        /* horizontal, vertical prediction address */
	int average_flag)     /* add prediction error to prediction ? */
{
	if (average_flag)
	{
		/* Y */
		form_component_prediction_avg(src[0],dst[0],
			lx,16,x,y,dx,dy);
		
		x>>=1; dx>>=1;y>>=1; dy>>=1;
		
		/* Cb */
		form_component_prediction_avg_cr(src[1],dst[1],
			lx>>1,8,x,y,dx,dy);
		
		/* Cr */
		form_component_prediction_avg_cr(src[2],dst[2],
			lx>>1,8,x,y,dx,dy);
	}
	else
	{
		/* Y */
		form_component_prediction(src[0],dst[0],
			lx,x,y,dx,dy);
		
		x>>=1; dx>>=1;y>>=1; dy>>=1;
		
		/* Cb */
		form_component_prediction_chroma(src[1],dst[1],
			lx>>1,x,y,dx,dy);
		
		/* Cr */
		form_component_prediction_chroma(src[2],dst[2],
			lx>>1,x,y,dx,dy);
	}
}

void form_predictions(int bx, int by, int macroblock_type,
	int PMV[2][2],video_data *ld)
{
	int stw=0;
	
	if ((macroblock_type & MACROBLOCK_MOTION_FORWARD) 
		|| (ld->picture_coding_type==P_TYPE))
	{
		/* frame-based prediction (broken into top and bottom halves
		for spatial scalability prediction purposes) */
		form_prediction(ld->forward_reference_frame,ld->current_frame,
			ld->Coded_Width,bx,by,PMV[0][0],PMV[0][1],0);
		stw=1;
	}
	
	if (macroblock_type & MACROBLOCK_MOTION_BACKWARD)
	{
		/* frame-based prediction */
		form_prediction(ld->backward_reference_frame,ld->current_frame,
			ld->Coded_Width,bx,by,PMV[1][0],PMV[1][1],stw);
	
	}
}

