#ifndef TPIXEL4_H
#define TPIXEL4_H

#include <BeBuild.h>
#include <ByteOrder.h>
#include <SupportDefs.h>

class t_pixel_cmyk_nslices
{
public:
	t_pixel_cmyk_nslices()
		: 	fWordsPerPlane(0),
			fNbSlices(0),
			fShift(0),
			fMask(0),
			pCyan(0),
			pMagenta(0),
			pYellow(0),
			pBlack(0)
	{
	}
	
	t_pixel_cmyk_nslices(int nbslices, uint32 bpl)
		: 	fWordsPerPlane(bpl/4),
			fNbSlices(nbslices),
			fShift(0),
			fMask(0)
	{
		pCyan = new uint32*[fNbSlices];
		pMagenta = new uint32*[fNbSlices];
		pYellow = new uint32*[fNbSlices];
		pBlack = new uint32*[fNbSlices];
		
		switch (fNbSlices)
		{
			case  2:	fShift = 1;		fMask = 0x1;	break;
			case  4:	fShift = 2;		fMask = 0x3;	break;
			case  8:	fShift = 3;		fMask = 0x7;	break;
			case 16:	fShift = 4;		fMask = 0xF;	break;
		};
	}
	
	~t_pixel_cmyk_nslices()
	{
		delete [] pCyan;
		delete [] pMagenta;
		delete [] pYellow;
		delete [] pBlack;
	}

	uint16 *data(void)	{ return fData; }

	void set(uint32 **pCmyk)
	{
		for (int i=0 ; i<(const int)fNbSlices ; i++)
		{
		 	pCyan[i]	= pCmyk[i];
			pMagenta[i]	= pCmyk[i] + fWordsPerPlane;
			pYellow[i]	= pCmyk[i] + fWordsPerPlane*2;
			pBlack[i]	= pCmyk[i] + fWordsPerPlane*3;
		}
	}

	void put(uint32 x) const
	{
		#if B_HOST_IS_LENDIAN
			const uint32 p = (0x1F - ((x >> fShift) & 0x1F)) ^ 0x18;
		#else
			const uint32 p = (0x1F - ((x >> fShift) & 0x1F));
		#endif
		const int i = x & fMask;
		const uint32 d = (x >> (5+fShift));
		pCyan[i][d]		|= ((fData[0] & 0x1U) << p);
		pMagenta[i][d]	|= ((fData[1] & 0x1U) << p);
		pYellow[i][d]	|= ((fData[2] & 0x1U) << p);
		pBlack[i][d]	|= ((fData[3] & 0x1U) << p);
	}

protected:
	uint16 fData[4];			//  0
	uint32 fWordsPerPlane;		//  8
	int fNbSlices;				// 12
	int fShift;					// 16
	int fMask;					// 20
	uint32 **pCyan;				// 24
	uint32 **pMagenta;			// 28
	uint32 **pYellow;			// 32
	uint32 **pBlack;			// 36
};


#endif

