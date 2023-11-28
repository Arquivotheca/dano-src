/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator Includes, V1.1                          */
/* Purpose: Define global function prototypes and structures for the TIA     */
/*          Chip Sound Simulator.                                            */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Added compiler directives to facilitate compilation */
/*                       on a C++ compiler.                                  */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* TiaSound is Copyright(c) 1997 by Ron Fries                                */
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

#ifndef _TIASOUND_BEOS_H
#define _TIASOUND_BEOS_H

#include <SupportDefs.h>

class TIASoundBeOS
{
private:

	enum
	{
		/* definitions for AUDCx (15, 16) */
		SET_TO_1     = 0x00,      /* 0000 */
		POLY4        = 0x01,      /* 0001 */
		DIV31_POLY4  = 0x02,      /* 0010 */
		POLY5_POLY4  = 0x03,      /* 0011 */
		PURE         = 0x04,      /* 0100 */
		PURE2        = 0x05,      /* 0101 */
		DIV31_PURE   = 0x06,      /* 0110 */
		POLY5_2      = 0x07,      /* 0111 */
		POLY9        = 0x08,      /* 1000 */
		POLY5        = 0x09,      /* 1001 */
		DIV31_POLY5  = 0x0a,      /* 1010 */
		POLY5_POLY5  = 0x0b,      /* 1011 */
		DIV3_PURE    = 0x0c,      /* 1100 */
		DIV3_PURE2   = 0x0d,      /* 1101 */
		DIV93_PURE   = 0x0e,      /* 1110 */
		DIV3_POLY5   = 0x0f,      /* 1111 */    
		DIV3_MASK    = 0x0c,                   
		AUDC0        = 0x15,
		AUDC1        = 0x16,
		AUDF0        = 0x17,
		AUDF1        = 0x18,
		AUDV0        = 0x19,
		AUDV1        = 0x1a,
		/* the size (in entries) of the 4 polynomial tables */
		POLY4_SIZE  = 0x000f,
		POLY5_SIZE  = 0x001f,
		POLY9_SIZE  = 0x01ff,
		/* channel definitions */
		CHAN1 = 0,
		CHAN2 = 1
	};

public:
			TIASoundBeOS(float freq_factor);
	virtual ~TIASoundBeOS();

	void UpdateTIASound(uint16 addr, uint8 val);
	void TIAProcess(int16 *buffer, size_t n);

private:
	int TIAProcessChannel(const int channel);

private:
	int fDecrement;

	/* structures to hold the 6 tia sound control bytes */
	int AUDC[2];    /* AUDCx (15, 16) */
	int AUDF[2];    /* AUDFx (17, 18) */
	int AUDV[2];    /* AUDVx (19, 1A) */
	int16 Outvol[2];  /* last output volume for each channel */
	int16 LastOutvol[2];

	int P4[2]; /* Position pointer for the 4-bit POLY array */
	int P5[2]; /* Position pointer for the 5-bit POLY array */
	int P9[2]; /* Position pointer for the 9-bit POLY array */

	int Div_n_cnt[2];  /* Divide by n counter. one for each channel */
	int Div_n_max[2];  /* Divide by n maximum, one for each channel */

	static const uint8 Bit4[POLY4_SIZE];
	static const uint8 Bit5[POLY5_SIZE];
	static const uint8 Div31[POLY5_SIZE];
	static uint8 Bit9[POLY9_SIZE]; // this one is generated by the ctor
	
	// Lowpass filter
	float fA[5];
	float fB[4];
	float fLPFStatesA[4];
	float fLPFStatesB[4];
	int fCurrentState;
};


#endif

// Chebychev filter
//// 0.2
//fA[0] = 3.224554e-2;
//fA[1] = 1.289821e-1;
//fA[2] = 1.934732e-1;
//fA[3] = 1.289821e-1;
//fA[4] = 3.224554e-2;
//fB[1] = 1.265912;
//fB[2] = -1.203878;
//fB[3] = 5.405908e-1;
//fB[4] = -1.185538e-1;
//
//// 0.1
//fA[0] = 2.780755e-3;
//fA[1] = 1.112302e-2;
//fA[2] = 1.668453e-2;
//fA[3] = 1.112302e-2;
//fA[4] = 2.780755e-3;
//fB[1] =  2.764031;
//fB[2] = -3.122854
//fB[3] =  1.664554;
//fB[4] = -3.502232e-1;
