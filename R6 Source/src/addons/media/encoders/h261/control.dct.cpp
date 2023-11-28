//=============================================================================
#include "control.dct.h"
//=============================================================================
#include <math.h>
//=============================================================================
float cos_table[64];
//=============================================================================
void dct_init()
{
	int i,j;

	for (i=0;i<8;i++)
	for (j=0;j<8;j++)
	{
		cos_table[8*i+j] = (float) cos((2*i+1)*j*0.196349540849362);
	}
}
//=============================================================================
void dct(const unsigned char *src, short *dst)
{
	int i,j;
	int x,y;

	//Img0(x,y) + cos(y,j) -> Tmp1(x,j)
	float tmp1[64];
	for (x=0;x<8;x++)
	for (j=0;j<8;j++)
	{
		float result = 0.0;
		for (y=0;y<8;y++)
		{
			result += src[x+y*8] * cos_table[8*y+j];
		}
		tmp1[x + j*8] = result;
	}

	//Tmp1(x,j) + cos(x,i) -> Tmp2(i,j)
	float tmp2[64];
	for (i=0;i<8;i++)
	for (j=0;j<8;j++)
	{
		float result = 0.0;
		for (x=0;x<8;x++)
		{
			result += tmp1[x+j*8] * cos_table[8*x+i];
		}
		tmp2[i+j*8] = result;
	}


	//application of Ci and Cj
	for (i=0;i<8;i++)
	{
		tmp2[0+i*8] *= 0.707106781186547f;
		tmp2[i+0*8] *= 0.707106781186547f;
	}

	for (i=0;i<8;i++)
	for (j=0;j<8;j++)
	{
		float result = tmp2[i+j*8];
		if (result > 0.0)
		{
			result -= 0.5;
		}
		else
		{
			result += 0.5;
		}
		result /= 4.0;
		
		dst[i+j*8] = (short) result;
	}
	
}
//=============================================================================
