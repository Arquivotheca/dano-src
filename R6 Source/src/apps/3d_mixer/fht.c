static char fht_algorithm[] = "mayer-rad4";
static char fht_version[]   = "1.2";
#include "trig.h"
/*
** Procedure:  fht     PROGRAMMER: Ron Mayer  DATE: 1990,1993
**    "Copyright (C) Ron Mayer, 1990,1993"
**    Function: Compute the Fast Hartley Transform of an array
**    Inputs: fz   = pointer to the beginning of the array
**            n    = number of points (must be power of 2)
**            skip = space between data points (for multi_dimentional arrays)
**    Output: fz is filled with the Hartley transform of fz
**    Return: none
**    Note:   This routine is probably patented; see README for more info
**            Except for scaling, this function is it's own inverse transform
*/
#define SQRT2    1.414213562373095048801688724209698

fht(fz,n,SKIP)
	int n;
	float	*fz;
	int SKIP;
{
 int i,j,k,k1,k4,kx,l;
 float x0,x1,x2,x3,x4,x5,x6;
 float c1,s1,s2,c2,s3,c3;
 float *f1,*f2,*f3;
 TRIG_VARS;

 for ( k=0 ; (1<<k)<n ; k++ );
 k  &= 1;
 n  *= SKIP;
 for (i=SKIP,j=0;i<n;i+=SKIP)
    {
     for (l=n>>1; (!((j^=l)&l)); l>>=1);
     if (i>j)
            {float x0;x0=fz[i]; fz[i]=fz[j]; fz[j]=x0;}
    }
 k1  = SKIP  << k;
 k4  = k1 << 2;
 kx  = k1 >> 1;
 f1  = fz + k1;
 f2  = f1 + k1;
 f3  = f2 + k1;
 if (k==0)
         for (i=0;i<n;i+=k4)
            {
             float x0,x1,x2,x3;
             x1    = fz[i] - f1[i];
             x0    = fz[i] + f1[i];
             x3    = f2[i] - f3[i];
             x2    = f2[i] + f3[i];
             f2[i] = x0    - x2;
             fz[i] = x0    + x2;
             f3[i] = x1    - x3;
             f1[i] = x1    + x3;
            }
 else
         for (i=0,j=kx;i<n;i+=k4,j+=k4)
            {
             float x0,x1,x2,x3,x4,x5;
             x0     = fz[i] - fz[j];
             x1     = fz[i] + fz[j];
             x2     = f1[i] - f1[j];
             x3     = f1[i] + f1[j];
             x4     = x1    - x3;
             x1    +=         x3;
             x3     = x0    - x2;
             x0    +=         x2;
             x5     = f2[i] + f2[j];
             x2     = f2[i] - f2[j];
             x2    *= SQRT2;
             f2[j]  = x0    - x2;
             fz[j]  = x0    + x2;
             x2     = f3[i] + f3[j];
             x0     = f3[i] - f3[j];
             x0    *= SQRT2;
             f3[j]  = x3    - x0;
             f1[j]  = x3    + x0;
             x0     = x5    - x2;
             x5    +=         x2;
             f2[i]  = x1    - x5;
             fz[i]  = x1    + x5;
             f3[i]  = x4    - x0;
             f1[i]  = x4    + x0;
            }
 while (k4<n)
    {
     float s1,c1;
     k  += 2;
     k1  = SKIP  << k;
     k4  = k1 << 2;
     kx  = k1 >> 1;
     f1  = fz + k1;
     f2  = f1 + k1;
     f3  = f2 + k1;
         for(i=0,j=kx;i<n;i+=k4,j+=k4)
            {
             float  x0,x1,x2,x3;
             x1     = fz[i] - f1[i];
             x0     = fz[i] + f1[i];
             x3     = f2[i] - f3[i];
             x2     = f2[i] + f3[i];
             f2[i]  = x0    - x2;
             fz[i]  = x0    + x2;
             f3[i]  = x1    - x3;
             f1[i]  = x1    + x3;
             x1     = fz[j] - f1[j];
             x0     = fz[j] + f1[j];
             x3     = SQRT2 * f3[j];
             x2     = SQRT2 * f2[j];
             f2[j]  = x0    - x2;
             fz[j]  = x0    + x2;
             f3[j]  = x1    - x3;
             f1[j]  = x1    + x3;
            }
     TRIG_INIT(k,c1,s1);
     for (l=SKIP;l<kx;l+=SKIP)
        {
         float c2,s2,c3,s3;
         TRIG_NEXT(k,c1,s1);
         TRIG_23(k,c1,s1,c2,s2,c3,s3);
             for (i=l,j=k1-l;i<n;i+=k4,j+=k4)
                {
                 float x0,x1,x2,x3,x4,x5,x6;
                 x0     = f1[i]*c2 + f1[j]*s2;
                 x1     = f1[i]*s2 - f1[j]*c2;
                 x2     = f2[i]*c1 + f2[j]*s1;
                 x3     = f2[i]*s1 - f2[j]*c1;
                 x4     = f3[i]*c3 + f3[j]*s3;
                 x5     = f3[i]*s3 - f3[j]*c3;
                 x6     = x2       - x4;
                 x4    +=            x2;
                 x2     = x3       - x5;
                 x5    +=            x3;
                 x3     = fz[i]    - x0;
                 f3[i]  = x3       + x2;
                 f1[i]  = x3       - x2;
                 x3     = fz[i]    + x0;
                 f2[i]  = x3       - x4;
                 fz[i]  = x3       + x4;
                 x3     = fz[j]    - x1;
                 f3[j]  = x3       - x5;
                 f1[j]  = x3       + x5;
                 x3     = fz[j]    + x1;
                 f2[j]  = x3       - x6;
                 fz[j]  = x3       + x6;
                }
        }
    }
}
