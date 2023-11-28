//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: MCS_Conversion4.cpp
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

#include <OS.h>
#include <math.h>

#include "MCS_Conversion4.h"

MCS_Conversion4::MCS_Conversion4(const void *tab)
	:	MCS_Conversion(),
		fTabColor((color_t)tab)
{
}

MCS_Conversion4::~MCS_Conversion4(void)
{
}

bool MCS_Conversion4::SpaceConversion(uint32 rgb, int16 *cmyk_i)
{
	if (IsMMX())
		return SpaceConversion_mmx(rgb, cmyk_i);
	return SpaceConversion_cpp(rgb, cmyk_i);
}

bool MCS_Conversion4::SpaceConversion_cpp(uint32 rgb, int16 *cmyk_i)
{
	// Pixel blanc ?
#if B_HOST_IS_LENDIAN

	if ((rgb & 0x00FFFFFF) == 0x00FFFFFF)
		return false;

	// Pixel Noir ?
	if ((rgb & 0x00FFFFFF) == 0x00000000)
	{
		cmyk_i[0] = cmyk_i[1] = cmyk_i[2] = 0;
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
		cmyk_i[0] = cmyk_i[1] = cmyk_i[2] = 0;
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

	return true;
}




bool MCS_Conversion4::SpaceConversion_mmx(uint32 rgb, int16 *cmyk_i)
{
#ifdef __INTEL__
	// Pixel blanc ?
#if B_HOST_IS_LENDIAN

	if ((rgb & 0x00FFFFFF) == 0x00FFFFFF)
		return false;

	// Pixel Noir ?
	if ((rgb & 0x00FFFFFF) == 0x00000000)
	{
		cmyk_i[0] = cmyk_i[1] = cmyk_i[2] = 0;
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
		cmyk_i[0] = cmyk_i[1] = cmyk_i[2] = 0;
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

	asm volatile
	(
		"movd 	%0, %%mm0		\n\t"
		"movq 	%%mm0, %%mm4	\n\t"
		"psllq	$32, %%mm4		\n\t"
		"por	%%mm4, %%mm0	\n\t"		// mm0 = rm rf rm rf

		"movd 	%1, %%mm1		\n\t"
		"movd 	%2, %%mm4		\n\t"
		"psllq	$32, %%mm1		\n\t"
		"por	%%mm4, %%mm1	\n\t"		// mm1 = gm gm gf gf

		"movd 	%3, %%mm2		\n\t"
		"movq 	%%mm2, %%mm4	\n\t"
		"psllq	$32, %%mm4		\n\t"
		"por	%%mm4, %%mm2	\n\t"		// mm2 = bm bm bm bm

		"movd 	%4, %%mm3		\n\t"
		"movq 	%%mm3, %%mm4	\n\t"
		"psllq	$32, %%mm4		\n\t"
		"por	%%mm4, %%mm3	\n\t"		// mm3 = bf bf bf bf

		"pmullw		%%mm0, %%mm2	\n\t"	// mm2 = rm*bm     rf*bm     rm*bm     rf*bm
		"pmullw		%%mm1, %%mm2	\n\t"	// mm2 = rm*gm*bm  rf*gm*bm  rm*gf*bm  rf*gf*bm = A  B  C  D
		"pmullw		%%mm0, %%mm3	\n\t"	// mm3 = rm*bf     rf*bf     rm*bf     rf*bf
		"pmullw		%%mm1, %%mm3	\n\t"	// mm3 = rm*gm*bf  rf*gm*bf  rm*gf*bf  rf*gf*bf = E  F  G  H

		:
		:	"g"	((rm << 16) | rf),	// %0
			"g"	((gm << 16) | gm),	// %1
			"g"	((gf << 16) | gf),	// %2
			"g"	((bm << 16) | bm),	// %3
			"g"	((bf << 16) | bf)	// %4
	);

	asm volatile
	(
		"movq		%%mm2, %%mm1	\n\t"			// mm1 = A B C D
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 A
		"movq		%%mm1, %%mm4	\n\t"			// mm4 = 0 0 0 A
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 A 0 0
		"por		%%mm4, %%mm1	\n\t"			// mm1 = 0 A 0 A
		"movq		%%mm1, %%mm4	\n\t"			// mm4 = 0 A 0 A
		"psllq		$16, %%mm1		\n\t"			// mm1 = A 0 A 0
		"por		%%mm1, %%mm4	\n\t"			// mm4 = A A A A
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	(%%edx), %%mm1	\n\t"			// c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm4	\n\t"

		"movq		%%mm2, %%mm1	\n\t"			// mm1 = A B C D
		"psllq		$16, %%mm1		\n\t"			// mm1 = B C D 0
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 B
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 0 0 B
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 B 0 0
		"por		%%mm0, %%mm1	\n\t"			// mm1 = 0 B 0 B
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 B 0 B
		"psllq		$16, %%mm1		\n\t"			// mm1 = B 0 B 0
		"por		%%mm1, %%mm0	\n\t"			// mm0 = B B B B
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	256(%%edx), %%mm1	\n\t"		// 64*4 c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm0	\n\t"
		"paddusw	%%mm0, %%mm4	\n\t"

		"movq		%%mm2, %%mm1	\n\t"			// mm1 = A B C D
		"psllq		$32, %%mm1		\n\t"			// mm1 = C D 0 0
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 C
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 0 0 C
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 C 0 0
		"por		%%mm0, %%mm1	\n\t"			// mm1 = 0 C 0 C
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 C 0 C
		"psllq		$16, %%mm1		\n\t"			// mm1 = C 0 C 0
		"por		%%mm1, %%mm0	\n\t"			// mm0 = C C C C
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	32(%%edx), %%mm1	\n\t"		// 8*4 c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm0	\n\t"
		"paddusw	%%mm0, %%mm4	\n\t"

		"movq		%%mm2, %%mm1	\n\t"			// mm1 = A B C D
		"psllq		$48, %%mm1		\n\t"			// mm1 = D 0 0 0
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 D
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 0 0 D
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 D 0 0
		"por		%%mm0, %%mm1	\n\t"			// mm1 = 0 D 0 D
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 D 0 D
		"psllq		$16, %%mm1		\n\t"			// mm1 = D 0 D 0
		"por		%%mm1, %%mm0	\n\t"			// mm0 = D D D D
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	288(%%edx), %%mm1	\n\t"		// 72*4 c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm0	\n\t"
		"paddusw	%%mm0, %%mm4	\n\t"

		"movq		%%mm3, %%mm1	\n\t"			// mm1 = E F G H
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 E
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 0 0 E
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 E 0 0
		"por		%%mm0, %%mm1	\n\t"			// mm1 = 0 E 0 E
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 E 0 E
		"psllq		$16, %%mm1		\n\t"			// mm1 = E 0 E 0
		"por		%%mm1, %%mm0	\n\t"			// mm0 = E E E E
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	4(%%edx), %%mm1	\n\t"			// 1*4 c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm0	\n\t"
		"paddusw	%%mm0, %%mm4	\n\t"

		"movq		%%mm3, %%mm1	\n\t"			// mm1 = E F G H
		"psllq		$16, %%mm1		\n\t"			// mm1 = F G H 0
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 F
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 0 0 F
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 F 0 0
		"por		%%mm0, %%mm1	\n\t"			// mm1 = 0 F 0 F
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 F 0 F
		"psllq		$16, %%mm1		\n\t"			// mm1 = F 0 F 0
		"por		%%mm1, %%mm0	\n\t"			// mm0 = F F F F
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	260(%%edx), %%mm1	\n\t"		// 65*4 c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm0	\n\t"
		"paddusw	%%mm0, %%mm4	\n\t"

		"movq		%%mm3, %%mm1	\n\t"			// mm1 = E F G H
		"psllq		$32, %%mm1		\n\t"			// mm1 = G H 0 0
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 G
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 0 0 G
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 G 0 0
		"por		%%mm0, %%mm1	\n\t"			// mm1 = 0 G 0 G
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 G 0 G
		"psllq		$16, %%mm1		\n\t"			// mm1 = G 0 G 0
		"por		%%mm1, %%mm0	\n\t"			// mm0 = G G G G
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	36(%%edx), %%mm1	\n\t"		// 9*4 c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm0	\n\t"
		"paddusw	%%mm0, %%mm4	\n\t"

		"movq		%%mm3, %%mm1	\n\t"			// mm1 = E F G H
		"psllq		$48, %%mm1		\n\t"			// mm1 = H 0 0 0
		"psrlq		$48, %%mm1		\n\t"			// mm1 = 0 0 0 H
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 0 0 H
		"psllq		$32, %%mm1		\n\t"			// mm1 = 0 H 0 0
		"por		%%mm0, %%mm1	\n\t"			// mm1 = 0 H 0 H
		"movq		%%mm1, %%mm0	\n\t"			// mm0 = 0 H 0 H
		"psllq		$16, %%mm1		\n\t"			// mm1 = H 0 H 0
		"por		%%mm1, %%mm0	\n\t"			// mm0 = H H H H
		"pxor		%%mm1, %%mm1	\n\t"			// mm1 = 0 0 0 0
		"punpcklbw	292(%%edx), %%mm1	\n\t"		// 73*4 c m y k -> c0 m0 y0 k0
		"psrlw		$1, %%mm1		\n\t"		
		"pmulhw		%%mm1, %%mm0	\n\t"
		"paddusw	%%mm0, %%mm4	\n\t"
		
		"movq		%%mm4, (%%ecx)	\n\t"		

		:
		: 	"edx" 	(&fTabColor[red][green][blue]),
			"ecx"	(&cmyk_i[0])
	);
	return true;
#else
	return false;
#endif
}

