//=============================================================================
#include "control.dct_CIF_422.h"
//=============================================================================
#include "control.dct.h"
//=============================================================================
void dct_Y_CIF_422(unsigned char *src, short *dst)
{
	unsigned char src_[64];

	int i,j;
	for (i=0;i<8;i++)
	{
		for (j=0;j<8;j++)
		{
			src_[i+8*j] = src[i*2+j*352*2];
		}
	}
	
	dct(src_, dst);
}
//=============================================================================
void dct_C_CIF_422(unsigned char *src, short *dst)
{
	unsigned char src_[64];

	int i,j;
	for (i=0;i<8;i++)
	{
		for (j=0;j<8;j++)
		{
			unsigned int acc;
			acc  = 0;
			acc += src[i*4 + j*352*2*2];
			acc += src[i*4 + j*352*2*2 + 352*2];
			acc /= 2;
			src_[i+8*j] = (unsigned char) acc;
		}
	}
	
	dct(src_, dst);
}
//=============================================================================
