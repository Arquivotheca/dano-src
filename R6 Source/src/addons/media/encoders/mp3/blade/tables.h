/*
			(c) Copyright 1998, 1999 - Tord Jansson
			=======================================

		This file is part of the BladeEnc MP3 Encoder, based on
		ISO's reference code for MPEG Layer 3 compression, and might
		contain smaller or larger sections that are directly taken
		from ISO's reference code.

		All changes to the ISO reference code herein are either
		copyrighted by Tord Jansson (tord.jansson@swipnet.se)
		or sublicensed to Tord Jansson by a third party.

	BladeEnc is free software; you can redistribute this file
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

*/

typedef	struct
{
/*	int	no; */
	int			lines;
	double	minVal;
	double	qthr;
	double	norm;
	double	bVal;
	
} psyDataElem;


typedef	struct
{
/*	int	no; */
	int			lines;
	double	qthr;
	double	norm;
	double	snr;
	double	bVal;
	
} psyDataElem2;

typedef	struct
{
	int			cbw;
	int			bu;
	int			bo;
	float		w1;
	float		w2;
} psyDataElem3;


extern	float	absthr_0[];
extern	float	absthr_1[];
extern	float	absthr_2[];

extern	psyDataElem		psy_longBlock_48000_61[62];
extern	psyDataElem		psy_longBlock_44100_62[63];
extern	psyDataElem		psy_longBlock__32000_58[59];

extern	psyDataElem2	psy_shortBlock_48000_37[38];
extern	psyDataElem2	psy_shortBlock_44100_38[39];
extern	psyDataElem2	psy_shortBlock_32000_41[42];

extern	psyDataElem3	psy_data3_48000_20[21];
extern	psyDataElem3	psy_data3_44100_20[21];
extern	psyDataElem3	psy_data3_32000_20[21];

extern	psyDataElem3	psy_data4_48000_11[12];
extern	psyDataElem3	psy_data4_44100_11[12];
extern	psyDataElem3	psy_data4_32000_11[12];


extern	double	enwindow[512];
extern	char		aHuffcode[1498][36];











