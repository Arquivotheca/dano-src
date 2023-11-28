/* getbits.c, bit level routines                                            */

/*
 * All modifications (mpeg2decode -> mpeg2play) are
 * Copyright (C) 1996, Stefan Eckart. All Rights Reserved.
 */

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
#include <stdlib.h>

#include "global.h"
#include <unistd.h>

/* initialize buffer, call once before first getbits or showbits */

void Initialize_Buffer(video_data *ld)
{
	ld->Incnt = 0;
	ld->Rdptr = ld->Rdbfr + RDBFS;
	
	ld->Bfr = 0;
	Flush_Buffer(0,ld); /* fills valid data into bfr */
}

inline void Fill_Buffer(video_data *ld)
{
	int Buffer_Level;
	unsigned long *p;
	Buffer_Level = ld->streamRead(ld->streamObj,ld->Rdbfr,RDBFS);
	
	ld->Rdptr = ld->Rdbfr;
	p = (unsigned long *)ld->Rdptr;	
	
	/* end of the bitstream file */
	if (Buffer_Level < RDBFS)
	{
		/* just to be safe */
		if (Buffer_Level < 0)
			Buffer_Level = 0;
		
		/* pad until the next to the next 32-bit word boundary */
		while (Buffer_Level & 3)
			ld->Rdbfr[Buffer_Level++] = 0;
		
		/* pad the buffer with sequence end codes */
		while (Buffer_Level < RDBFS)
		{
			ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE>>24;
			ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE>>16;
			ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE>>8;
			ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE&0xff;
		}
	}
}

// NOT the same as Get_Bits(32,ld);
unsigned int Get_Bits32(video_data *ld)
{
	unsigned int l;
	
	l = Show_Bits(32,ld);
	Flush_Buffer32(ld);
	
	return l;
}

/* return next n bits (right adjusted) without advancing */
unsigned int Show_Bits(int N,video_data *ld)
{
	return ld->Bfr >> (32-N);
}

// NOT the same as Flush_Buffer(32,ld);
void Flush_Buffer32(video_data *ld)
{
	int Incnt;
	
	ld->Bfr = 0;
	
	Incnt = ld->Incnt;
	Incnt -= 32;
	
	{
		while (Incnt <= 24)
		{
			if (ld->Rdptr >= ld->Rdbfr+RDBFS)
				Fill_Buffer(ld);
			ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
			Incnt += 8;
		}
	}
	ld->Incnt = Incnt;
}

/* advance by n bits */

void Flush_Buffer(int N,video_data *ld)
{
	int Incnt;

	ld->Bfr <<= N;
	Incnt = ld->Incnt -= N;

	if (Incnt <= 24)
	{
		if (ld->Rdptr < ld->Rdbfr+RDBFS-4)
		{
			do
			{
				ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
				Incnt += 8;
			}
			while (Incnt <= 24);
		}
		else
		{
			do
			{
				if (ld->Rdptr >= ld->Rdbfr+RDBFS)
				{
					Fill_Buffer(ld);
				}
				ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
				Incnt += 8;
			}
			while (Incnt <= 24);
		}
		ld->Incnt = Incnt;
	}
}

inline void f_Flush_Buffer(int N,video_data *ld)
{
	int Incnt;
	
	Incnt = ld->Incnt;
	
	if (ld->Rdptr < ld->Rdbfr+RDBFS-4)
	{
		do
		{
			ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
			Incnt += 8;
		}
		while (Incnt <= 24);
	}
	else
	{
		do
		{
			if (ld->Rdptr >= ld->Rdbfr+RDBFS)
				Fill_Buffer(ld);
			ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
			Incnt += 8;
		}
		while (Incnt <= 24);
	}
	ld->Incnt = Incnt;
}

/* return next n bits (right adjusted) */
unsigned int Get_Bits(int N,video_data *ld)
{
	unsigned int Val;
	long		Incnt;
	
	Val = xShow_Bits(N,ld);
	
	ld->Bfr <<= N;
	Incnt = ld->Incnt -= N;
	
	if (Incnt <= 24)
		f_Flush_Buffer(N,ld);
	
	return Val;
}
