//=============================================================================
#include "control.yuv_image.h"
//=============================================================================
#include "control.dct_CIF_422.h"
//=============================================================================
#include <stdio.h>
#include <memory.h>
//=============================================================================
void yuv_image::set_black()
{
	memset(Y1, 22*18*64*sizeof(short), 0);
	memset(Y2, 22*18*64*sizeof(short), 0);
	memset(Y3, 22*18*64*sizeof(short), 0);
	memset(Y4, 22*18*64*sizeof(short), 0);
	
	memset(U,  22*18*64*sizeof(short), 0);
	memset(V,  22*18*64*sizeof(short), 0);

	int i;
	for (i=0;i<22*18;i++)
	{
		U[i*64 + 0]=128*8;
		V[i*64 + 0]=128*8;
	}
}
//=============================================================================
void yuv_image::set_422(unsigned char *buffer)
{
	int i,j;
	for (i=0;i<22;i++)
	{
		for (j=0;j<18;j++)
		{
			unsigned char *base = &buffer[32*(i + j*352)];
			unsigned int    idx = 64*(i + 22*j);
			
			dct_Y_CIF_422(&base[ 0 + 0*352*2], &Y1[idx]);
			dct_Y_CIF_422(&base[16 + 0*352*2], &Y2[idx]);
			dct_Y_CIF_422(&base[ 0 + 8*352*2], &Y3[idx]);
			dct_Y_CIF_422(&base[16 + 8*352*2], &Y4[idx]);
			
			dct_C_CIF_422(&base[1], &U[idx]);
			dct_C_CIF_422(&base[3], &V[idx]);
		}
	}
}
//=============================================================================
void yuv_image::set_diff(yuv_image *n, yuv_image *n_1)
{
	int i,k;
	
	for (i=0;i<22*18;i++)
	{
		for (k=0;k<64;k++)
		{
			int idx = i*64+k;
			
			Y1[idx] = n->Y1[idx] - n_1->Y1[idx];
			Y2[idx] = n->Y2[idx] - n_1->Y2[idx];
			Y3[idx] = n->Y3[idx] - n_1->Y3[idx];
			Y4[idx] = n->Y4[idx] - n_1->Y4[idx];
			
			U [idx] = n->U [idx] - n_1->U [idx];
			V [idx] = n->V [idx] - n_1->V [idx];
			
			if (Y1[idx] >  2048) { Y1[idx] =  2048; printf("&&1&"); }
			if (Y2[idx] >  2048) { Y2[idx] =  2048; printf("&&2&"); }
			if (Y3[idx] >  2048) { Y3[idx] =  2048; printf("&&3&"); }
			if (Y4[idx] >  2048) { Y4[idx] =  2048; printf("&&4&"); }
			if (U [idx] >  2048) { U [idx] =  2048; printf("&&5&"); }
			if (V [idx] >  2048) { V [idx] =  2048; printf("&&6&"); }

			if (Y1[idx] < -2047) { Y1[idx] = -2047; printf("&&7&"); }
			if (Y2[idx] < -2047) { Y2[idx] = -2047; printf("&&8&"); }
			if (Y3[idx] < -2047) { Y3[idx] = -2047; printf("&&9&"); }
			if (Y4[idx] < -2047) { Y4[idx] = -2047; printf("&10&"); }
			if (U [idx] < -2047) { U [idx] = -2047; printf("&11&"); }
			if (V [idx] < -2047) { V [idx] = -2047; printf("&12&"); }
		}
	}
}
//=============================================================================
int recons(int level, int q)
{
	if (level == 0)
	{
		return 0;
	}
	
	if (level > 0)
	{
		level = 2*level + 1;
	}
	else
	{
		level = 2*level - 1;
	}
	
	level *= q;
	if (q%2 == 0)
	{
		level -= 1;
	}
	if (level < -2048)
	{
		level = -2048;
	}
	if (level > 2047)
	{
		level = 2047;
	}
	
	return level;
}
//=============================================================================
void yuv_image::mb_add_delta(yuv_image *delta, int mb, int q)
{
	int i;
	for (i=0;i<64;i++)
	{
		int idx = mb*64+i;
		
		Y1[idx] += recons(delta->Y1[idx],delta->q_used[mb]);
		Y2[idx] += recons(delta->Y2[idx],delta->q_used[mb]);
		Y3[idx] += recons(delta->Y3[idx],delta->q_used[mb]);
		Y4[idx] += recons(delta->Y4[idx],delta->q_used[mb]);
		U [idx] += recons(delta-> U[idx],delta->q_used[mb]);
		V [idx] += recons(delta-> V[idx],delta->q_used[mb]);
		
		if (Y1[idx] >  2048) { Y1[idx] =  2048; printf("&&1="); }
		if (Y2[idx] >  2048) { Y2[idx] =  2048; printf("&&2="); }
		if (Y3[idx] >  2048) { Y3[idx] =  2048; printf("&&3="); }
		if (Y4[idx] >  2048) { Y4[idx] =  2048; printf("&&4="); }
		if (U [idx] >  2048) { U [idx] =  2048; printf("&&5="); }
		if (V [idx] >  2048) { V [idx] =  2048; printf("&&6="); }

		if (Y1[idx] < -2047) { Y1[idx] = -2047; printf("&&7="); }
		if (Y2[idx] < -2047) { Y2[idx] = -2047; printf("&&8="); }
		if (Y3[idx] < -2047) { Y3[idx] = -2047; printf("&&9="); }
		if (Y4[idx] < -2047) { Y4[idx] = -2047; printf("&10="); }
		if (U [idx] < -2047) { U [idx] = -2047; printf("&11="); }
		if (V [idx] < -2047) { V [idx] = -2047; printf("&12="); }
	}
}
//=============================================================================
void yuv_image::mb_copy_ref (yuv_image *ref,   int mb, int q)
{
	int i;
	for (i=0;i<64;i++)
	{
		int idx = mb*64+i;
		
		Y1[idx] = recons(ref->Y1[idx],ref->q_used[mb]);
		Y2[idx] = recons(ref->Y2[idx],ref->q_used[mb]);
		Y3[idx] = recons(ref->Y3[idx],ref->q_used[mb]);
		Y4[idx] = recons(ref->Y4[idx],ref->q_used[mb]);
		U [idx] = recons(ref-> U[idx],ref->q_used[mb]);
		V [idx] = recons(ref-> V[idx],ref->q_used[mb]);
	}
}
//=============================================================================
void yuv_image::quantify(int q, int thre_y, int thre_c)
{
	int thre_y_thre_y = thre_y*thre_y;
	int thre_c_thre_c = thre_c*thre_c;

	int to_thresh[] =
	{
	                             6,  7,
	                        13, 14, 15,
	                    20, 21, 22, 23,
	                27, 28, 29, 30, 31,
	            34, 35, 36, 37, 38, 39,
		    41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63 
	};
		
	int mb;
	for (mb=0;mb<22*18;mb++)
	{
		//determine q to be used
		{
			int min = 0;
			int max = 0;
			
			// <-- if q > 8, no need to perform check
			// <-- should be modified for intra (1 instead of 0)
			int i;
			for (i=0;i<64;i++)
				{
					int idx = mb*64+i;
					
					if (Y1[idx]<min) min= Y1[idx];
					if (Y2[idx]<min) min= Y2[idx];
					if (Y3[idx]<min) min= Y3[idx];
					if (Y4[idx]<min) min= Y4[idx];
					if ( U[idx]<min) min=  U[idx];
					if ( V[idx]<min) min=  V[idx];
	
					if (Y1[idx]>max) max= Y1[idx];
					if (Y2[idx]>max) max= Y2[idx];
					if (Y3[idx]>max) max= Y3[idx];
					if (Y4[idx]>max) max= Y4[idx];
					if ( U[idx]>max) max=  U[idx];
					if ( V[idx]>max) max=  V[idx];
				}
			if (max < -min)
			{
				max = -min;
			}
			
			//printf ("assigning %p::q_used[%d] to %d\n", this, mb, q);
			q_used[mb] = q;
			while (max > 127*2*q_used[mb])
			{
				q_used[mb]++;
			}
		}
		
		//perform quantification
		{
			int q_2 = q_used[mb]*2;
			
			int i;			
			for (i=0;i<64;i++)
			{
				int idx = 64*mb+i;
				Y1[idx] /= q_2;
				Y2[idx] /= q_2;
				Y3[idx] /= q_2;
				Y4[idx] /= q_2;
				
				U [idx] /= q_2;
				V [idx] /= q_2;	
			}
		}
		
		//threshold
		int j;
		for (j=0;j<sizeof(to_thresh)/sizeof(int);j++)
		{
			int idx=64*mb+to_thresh[j];
			
			if (Y1[idx]*Y1[idx] <= thre_y_thre_y) Y1[idx] = 0;
			if (Y2[idx]*Y2[idx] <= thre_y_thre_y) Y2[idx] = 0;
			if (Y3[idx]*Y3[idx] <= thre_y_thre_y) Y3[idx] = 0;
			if (Y4[idx]*Y4[idx] <= thre_y_thre_y) Y4[idx] = 0;
			
			if (U [idx]*U [idx] <= thre_c_thre_c) U [idx] = 0;
			if (V [idx]*V [idx] <= thre_c_thre_c) V [idx] = 0;			
		}
	}
	
}
//=============================================================================
