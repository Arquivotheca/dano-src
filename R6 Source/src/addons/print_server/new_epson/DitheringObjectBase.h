//*******************************************************************************************
// Project: Driver EPSON Stylus
// File: DitheringObjectBase.h
//
// Purpose: manipulations de couleurs
//
// Author: M. AGOPIAN
// Last Modifications:
// ___________________________________________________________________________________________
// Author		| Date		| Purpose
// ____________	|__________	|_________________________________________________________________
// M.AG.		| 01/11/97	| Debut du projet
// ____________	|__________	|_________________________________________________________________
//********************************************************************************************

#ifndef DITHERING_OBJECT_BASE_H
#define DITHERING_OBJECT_BASE_H

#include <SupportDefs.h>


#if (!defined(__POWERPC__) && defined(__GNUC__))
#	define ALLIGN(_x) __attribute__ ((aligned(_x)))
#else
#	define ALLIGN(_x)
#endif


#define M_DITHER_JUMP_PIXEL	1


class DitheringObjectBase
{
public:
	DitheringObjectBase();
	virtual ~DitheringObjectBase();
	virtual void Init(void) = 0;
	virtual	bool Dither(uint32 *pRGB, uint32 *pCmyk1, uint32 *pCmyk2=0) = 0;
	virtual bool Dither(uint32 *pRGB, uint32 **pCmyk) = 0;

	inline void rand3(int32 *r, const int nbCoefs, const int fact);

private:
	enum
	{
	    CRC_ALEA_INIT = 0x1f3678ef,
	    CRC_ALEA_STEP = 0x249b06d7
	};

	int32 crc_alea;
};

inline void DitheringObjectBase::rand3(int32 *r, const int nbCoefs, const int fact)
{ 
	#if (defined(__INTEL__) && defined(__GNUC__))

		for (int i=0 ; i<nbCoefs ; i++)
		{
			asm volatile
			(
				"addl %%eax,%%eax		\n\t"
				"cdq					\n\t"
				"andl %1,%%edx			\n\t"
				"xorl %%edx,%%eax		\n\t"
				"movl %%eax, %%edx		\n\t"
				"sarl %%cl, %%edx		\n\t"
				"movl %%edx,(%%esi)		\n\t"
					
				:	"=a" (crc_alea)			// %0
				:	"i" (CRC_ALEA_STEP),	// %1
				 	"a" (crc_alea),
					"c" (fact+18),
					"S" (&(r[i]))
				: "edx", "memory"
			);
		}

	#else

		int32 alea = crc_alea;	
		for (int i=0 ; i<nbCoefs ; i++)
		{
			alea <<= 1;
			if (alea < 0)
				alea ^= CRC_ALEA_STEP;
			*r++ = (alea >> (18 + fact));
		}
		crc_alea = alea;

	#endif
}

#endif

