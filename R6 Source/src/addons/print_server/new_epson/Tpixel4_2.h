#ifndef TPIXEL4_2_H
#define TPIXEL4_2_H

#include "Tpixel4.h"

class t_pixel_cmyk_nslices_2 : public t_pixel_cmyk_nslices
{
public:
	t_pixel_cmyk_nslices_2() : t_pixel_cmyk_nslices() { }
	t_pixel_cmyk_nslices_2(int nbslices, uint32 bpl) : t_pixel_cmyk_nslices(nbslices, bpl) { }

	void put(uint32 x) const
	{
		#if B_HOST_IS_LENDIAN
			const uint32 p = ((0x0F - ((x>>fShift) & 0x0F)) << 1) ^ 0x18;
		#else
			const uint32 p = ((0x0F - ((x>>fShift) & 0x0F)) << 1);
		#endif

		const uint32 d = ((x>>fShift) >> 4);
		const int i = x & fMask;

		pCyan[i][d]		|= (fData[0] << p);
		pMagenta[i][d]	|= (fData[1] << p);
		pYellow[i][d]	|= (fData[2] << p);
		pBlack[i][d]	|= (fData[3] << p);
	}
};

#endif

