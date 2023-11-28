//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: MCS_Conversion6.h
//
// Purpose: Conversion RGB->CMYK
//
// Author: M. AGOPIAN
// Last Modifications:
// ___________________________________________________________________________________________
// Author		| Date		| Purpose
// ____________	|__________	|_________________________________________________________________
// M.AG.		| 06/24/99	| Debut du projet
// ____________	|__________	|_________________________________________________________________
//********************************************************************************************

#include "MCS_Conversion6.h"

MCS_Conversion6::MCS_Conversion6(const void *tab)
	:	MCS_Conversion(),
		fTabColor((color_t)tab)
{
}


MCS_Conversion6::~MCS_Conversion6(void)
{
}


bool MCS_Conversion6::SpaceConversion(uint32 rgb, int16 *cmyk_i)
{
	// Pixel blanc ?
#if B_HOST_IS_LENDIAN

	if ((rgb & 0x00FFFFFF) == 0x00FFFFFF)
		return false;

	// Pixel Noir ?
	if ((rgb & 0x00FFFFFF) == 0x00000000)
	{
		cmyk_i[0] = cmyk_i[1] = cmyk_i[2] = cmyk_i[4] = cmyk_i[5] = 0;
		cmyk_i[3] = (fTabColor[0][0][0].k << 6);
		return true;
	}

	// 00RRGGBB
	int red		= (rgb >> 16) & 0xFF;
	int green	= (rgb >>  8) & 0xFF;
	int blue	= (rgb      ) & 0xFF;

#else

	if ((rgb & 0xFFFFFF00) == 0xFFFFFF00)
		return false;

	// Pixel Noir ?
	if ((rgb & 0xFFFFFF00) == 0x00000000)
	{
		cmyk_i[0] = cmyk_i[1] = cmyk_i[2] = cmyk_i[4] = cmyk_i[5] = 0;
		cmyk_i[3] = (fTabColor[0][0][0].k << 6);
		return true;
	}

	// BBGGRR00
	int red		= (rgb >>  8) & 0xFF;
	int green	= (rgb >> 16) & 0xFF;
	int blue	= (rgb >> 24) & 0xFF;

#endif

	uint32 	rf, gf, bf;
	const uint32 rm = 31 - (rf = fInterpolationTable[red].delta);
	const uint32 gm = 31 - (gf = fInterpolationTable[green].delta);
	const uint32 bm = 31 - (bf = fInterpolationTable[blue].delta);

	red = fInterpolationTable[red].level;
	green = fInterpolationTable[green].level;
	blue = fInterpolationTable[blue].level;

	const uint32 	A = rm	* gm	* bm;
	const uint32 	B = rf	* gm	* bm;
	const uint32 	C = rm	* gf	* bm;
	const uint32 	D = rf	* gf	* bm;
	const uint32 	E = rm	* gm	* bf;
	const uint32 	F = rf	* gm	* bf;
	const uint32 	G = rm	* gf	* bf;
	const uint32 	H = rf	* gf	* bf;

	const epson_color_t *cc = &fTabColor[red][green][blue];
	cmyk_i[0] = (	A*cc[ 0].c +			// 0 0 0
					B*cc[64].c +			// 1 0 0
					C*cc[ 8].c +			// 0 1 0
					D*cc[72].c +			// 1 1 0
					E*cc[ 1].c +			// 0 0 1
					F*cc[65].c +			// 1 0 1
					G*cc[ 9].c +			// 0 1 1
					H*cc[73].c) >> 9;		// 1 1 1

	cmyk_i[1] = (	A*cc[ 0].m +			// 0 0 0
					B*cc[64].m +			// 1 0 0
					C*cc[ 8].m +			// 0 1 0
					D*cc[72].m +			// 1 1 0
					E*cc[ 1].m +			// 0 0 1
					F*cc[65].m +			// 1 0 1
					G*cc[ 9].m +			// 0 1 1
					H*cc[73].m) >> 9;		// 1 1 1

	cmyk_i[2] = (	A*cc[ 0].y +			// 0 0 0
					B*cc[64].y +			// 1 0 0
					C*cc[ 8].y +			// 0 1 0
					D*cc[72].y +			// 1 1 0
					E*cc[ 1].y +			// 0 0 1
					F*cc[65].y +			// 1 0 1
					G*cc[ 9].y +			// 0 1 1
					H*cc[73].y) >> 9;		// 1 1 1

	cmyk_i[3] = (	A*cc[ 0].k +			// 0 0 0
					B*cc[64].k +			// 1 0 0
					C*cc[ 8].k +			// 0 1 0
					D*cc[72].k +			// 1 1 0
					E*cc[ 1].k +			// 0 0 1
					F*cc[65].k +			// 1 0 1
					G*cc[ 9].k +			// 0 1 1
					H*cc[73].k) >> 9;		// 1 1 1

	cmyk_i[4] = (	A*cc[ 0].lc +			// 0 0 0
					B*cc[64].lc +			// 1 0 0
					C*cc[ 8].lc +			// 0 1 0
					D*cc[72].lc +			// 1 1 0
					E*cc[ 1].lc +			// 0 0 1
					F*cc[65].lc +			// 1 0 1
					G*cc[ 9].lc +			// 0 1 1
					H*cc[73].lc) >> 9;		// 1 1 1

	cmyk_i[5] = (	A*cc[ 0].lm +			// 0 0 0
					B*cc[64].lm +			// 1 0 0
					C*cc[ 8].lm +			// 0 1 0
					D*cc[72].lm +			// 1 1 0
					E*cc[ 1].lm +			// 0 0 1
					F*cc[65].lm +			// 1 0 1
					G*cc[ 9].lm +			// 0 1 1
					H*cc[73].lm) >> 9;		// 1 1 1

	return true;
}
