/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator                                         */
/* Purpose: To emulate the sound generation hardware of the Atari TIA chip.  */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Cleaned up sound output by eliminating counter      */
/*                       reset.                                              */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* TiaSound is Copyright(c) 1996 by Ron Fries                                */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* BeOS C++ port:                                                            */
/*   Mathias Agopian                                                         */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "TIASoundBeOS.h"

/* Initialze the bit patterns for the polynomials. */

/* The 4bit and 5bit patterns are the identical ones used in the tia chip. */
/* Though the patterns could be packed with 8 bits per byte, using only a */
/* single bit per byte keeps the math simple, which is important for */
/* efficient processing. */

const uint8 TIASoundBeOS::Bit4[POLY4_SIZE] =	{ 1,1,0,1,1,1,0,0,0,0,1,0,1,0,0 };
const uint8 TIASoundBeOS::Bit5[POLY5_SIZE] =	{ 0,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,0,1,1,1,0,1,0,1,0,0,0,0,1 };
uint8 TIASoundBeOS::Bit9[POLY9_SIZE];

/* I've treated the 'Div by 31' counter as another polynomial because of */
/* the way it operates.  It does not have a 50% duty cycle, but instead */
/* has a 13:18 ratio (of course, 13+18 = 31).  This could also be */
/* implemented by using counters. */

const uint8 TIASoundBeOS::Div31[POLY5_SIZE] = { 0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0 };





TIASoundBeOS::TIASoundBeOS(float freq_factor)
	 : fDecrement((int)(65536.0f/freq_factor))
{
	/* fill the 9bit polynomial with random bits */
	for (int n=0;  n<POLY9_SIZE ; n++)
		Bit9[n] = rand() & 0x80;

	/* initialize the local globals */
	for (int chan = CHAN1; chan <= CHAN2; chan++)
	{
		Outvol[chan] = 0;
		Div_n_cnt[chan] = 0;
		Div_n_max[chan] = 0;
		AUDC[chan] = 0;
		AUDF[chan] = 0;
		AUDV[chan] = 0;
		P4[chan] = 0;
		P5[chan] = 0;
		P9[chan] = 0;
	}

	// LowPass Chebychef filter, fc = 0.2 * Fe
	fA[0] = 3.224554e-2;
	fA[1] = 1.289821e-1;
	fA[2] = 1.934732e-1;
	fA[3] = 1.289821e-1;
	fA[4] = 3.224554e-2;
	fB[0] = 1.265912;
	fB[1] = -1.203878;
	fB[2] = 5.405908e-1;
	fB[3] = -1.185538e-1;
	fCurrentState = 0;
	for (int i=0 ; i<4 ; i++)
		fLPFStatesA[i] = fLPFStatesB[i] = 0;
}

TIASoundBeOS::~TIASoundBeOS()
{
}


/*****************************************************************************/
/* Module:  TIASoundBeOS::UpdateTIASound()                                   */
/* Purpose: To process the latest control values stored in the AUDF, AUDC,   */
/*          and AUDV registers.  It pre-calculates as much information as    */
/*          possible for better performance.  This routine has not been      */
/*          optimized.                                                       */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    January 14, 1997                                                 */
/*                                                                           */
/* Inputs:  addr - the address of the parameter to be changed                */
/*          val - the new value to be placed in the specified address        */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

void TIASoundBeOS::UpdateTIASound(uint16 addr, uint8 val)
{
	int new_val = 0;
	int chan;
	
	/* determine which address was changed */
	switch (addr)
	{
		case AUDC0:	AUDC[0] = (val & 0x0f);			chan = 0;	break;
		case AUDC1:	AUDC[1] = (val & 0x0f);			chan = 1;	break;
		case AUDF0:	AUDF[0] = (val & 0x1f);			chan = 0;	break;
		case AUDF1:	AUDF[1] = (val & 0x1f);			chan = 1;	break;
		case AUDV0:	AUDV[0] = (val & 0x0f) << 10;	chan = 0;	break;
		case AUDV1:	AUDV[1] = (val & 0x0f) << 10;	chan = 1;	break;
		default:
			return;
	}

	/* an AUDC value of 0 is a special case */
	if (AUDC[chan] == SET_TO_1)
	{
		/* indicate the clock is zero so no processing will occur */
		new_val = 0;
		
		/* and set the output to the selected volume */
		Outvol[chan] = AUDV[chan];
	}
	else
	{
		/* otherwise calculate the 'divide by N' value */
		new_val = AUDF[chan] + 1;
		
		/* if bits 2 & 3 are set, then multiply the 'div by n' count by 3 */
		if ((AUDC[chan] & DIV3_MASK) == DIV3_MASK)
		{
			new_val *= 3;
		}
	}
	
	/* only reset those channels that have changed */
	if (new_val != Div_n_max[chan])
	{
		/* reset the divide by n counters */
		Div_n_max[chan] = (int)(new_val * 65536.0f);
		
		/* if the channel is now volume only or was volume only */
		if ((Div_n_cnt[chan] == 0) || (new_val == 0))
		{
			/* reset the counter (otherwise let it complete the previous) */
			Div_n_cnt[chan] = Div_n_max[chan];
		}
	}
}



/*****************************************************************************/
/* Module:  Tia_process()                                                    */
/* Purpose: To fill the output buffer with the sound output based on the     */
/*          tia chip parameters.  This routine has been optimized.           */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    September 10, 1996                                               */
/*                                                                           */
/* Inputs:  *buffer - pointer to the buffer where the audio output will      */
/*                    be placed                                              */
/*          n - size of the playback buffer                                  */
/*                                                                           */
/* Outputs: the buffer will be filled with n bytes of audio - no return val  */
/*                                                                           */
/*****************************************************************************/


void TIASoundBeOS::TIAProcess(int16 *buffer, size_t nb_sample)
{
	// loop until the buffer is filled
	register int idx = fCurrentState & 0x3;
	while (nb_sample--)
	{ // Use a lowpass filter
#if 1
		float x0 = (float)(TIAProcessChannel(0) + TIAProcessChannel(1)) * 0.85; // 0.05% ripples
		float y = fA[4]*x0;
		y += fB[3]*fLPFStatesB[idx] + fA[0]*fLPFStatesA[idx];	(++idx) &= 0x3;
		y += fB[2]*fLPFStatesB[idx] + fA[1]*fLPFStatesA[idx];	(++idx) &= 0x3;
		y += fB[1]*fLPFStatesB[idx] + fA[2]*fLPFStatesA[idx];	(++idx) &= 0x3;
		y += fB[0]*fLPFStatesB[idx] + fA[3]*fLPFStatesA[idx];	(++idx) &= 0x3;
		y = floor(y+0.5f);
		fLPFStatesB[idx] = y;
		fLPFStatesA[idx] = x0;
		(++idx) &= 0x3;
		*buffer++ = (int16)y;
#else
		*buffer++ = TIAProcessChannel(0) + TIAProcessChannel(1);
#endif
	}
	fCurrentState = idx;
}


int TIASoundBeOS::TIAProcessChannel(const int channel)
{	
	int outvol = Outvol[channel];
	if (Div_n_max[channel] == 0)
		return outvol;

	Div_n_cnt[channel] -= fDecrement;
	if (Div_n_cnt[channel] < 0x10000)
	{
		const float f = (0x10000 - Div_n_cnt[channel])/65536.0f;
	
		// make temporary local copy
		const int audc = AUDC[channel];
		const int audv = AUDV[channel];
		int p5 = P5[channel];
		
		// Process channel
		Div_n_cnt[channel] += Div_n_max[channel];
	
		(++p5) &= POLY5_SIZE;		// the P5 counter has multiple uses, so we inc it here
		
		if  (((audc & 0x02) == 0) ||
			(((audc & 0x01) == 0) && Div31[p5]) ||
			(((audc & 0x01) == 1) &&  Bit5[p5]))
		{ // check clock modifier for clock tick
			if (audc & 0x04)
			{ // pure modified clock selected
				outvol = outvol ? 0 : audv;
			}
			else if (audc & 0x08)
			{ // check for p5/p9
				if (audc == POLY9)
				{ // check for poly9
					(++(P9[channel])) &= POLY9_SIZE; // inc the poly9 counter
					outvol = Bit9[P9[channel]] ? audv : 0;
				}
				else // must be poly5
					outvol = Bit5[p5] ? audv : 0;
			}
			else  // poly4 is the only remaining option
			{ // inc the poly4 counter
				(++(P4[channel])) &= POLY4_SIZE;
				outvol = Bit4[P4[channel]] ? audv : 0;
			}
		}
		
		// save for next round
		P5[channel] = p5;
		
		// Linear interpolation
		int output = (int)(Outvol[channel] + (outvol - Outvol[channel]) * f);
		Outvol[channel] = outvol;
		outvol = output;
	}

	return outvol;
}
