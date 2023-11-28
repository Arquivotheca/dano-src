
/*
**  TWO DIMENTIONAL TRANSFORMS
*/

/*
** Procedure:  cas2d     PROGRAMMER: Ron Mayer  DATE: 04/10/93
**    Function: Returns the "cas cas" transform of the original data
**    Inputs: f   = pointer to two dimentional array
**            m,n = the sizes of each dimention
**    Output: f is filled with the "cas cas" transform of itself
**    Note:   Although not very interesting in itself, the "cas cas"
**            transform is one of the easiest to generate two
**            dimentional real valued transforms which contains
**            frequency info.  It is readily converted to the more
**            useful 2d fourier or hartley transforms.
*/

cas2d(f,m,n)
float *f;
int m,n;
{
 int i,j;
 for (i=0;i<n;i++)
     fht(f+i*m,m,1);
 for (i=0;i<m;i++)
     fht(f+i  ,n,m);
}

/*
** Procedure: fht2d   PROGRAMMER: Ron Mayer  DATE: 04/10/93
**    Function: two dimentional Hartley transform.
**    Inputs: f   = pointer to two dimentional array
**            m,n = the sizes of each dimention
**    Output: f is filled with the 2d hartley transform of itself
**    Note:   I never did find another 2d hartley transform to compare
**            this two; so I really don't know if it's correct.
**            It is it's own inverse transform.
*/

#define F(f,i,j,m,n) (*(f+(i)+m*(j)))
#define A F(f,i,j,m,n)
#define B F(f,i,l,m,n)
#define C F(f,k,j,m,n)
#define D F(f,k,l,m,n)
fht2d(f,m,n)
float *f;
int m,n;
{
 float e,s;
 int i,j,k,l;
 cas2d(f,n,m);
 for (i=1;i<m/2;i++)
    {
     k=m-i;
     for (j=1;j<n/2;j++)
        {
	 l=n-j;
         e  = ((A+D)-(B+C))/2;
         A -= e;
         B += e;
         C += e;
         D -= e;
        }
    }
}

/*
** Procedure: conv2d      PROGRAMMER: Ron Mayer  DATE: 04/09/93
**    Function: useful for real valued 2d convolution
**    Inputs: f,g = The hartley transform of two array to be convolved
**            h   = The output array (may be the same as f or g)
**            m,n = the dimentions of the array
**    Output: h gets the hartley transform of the convolution of the two
**    Example:
**            a[4][4]= {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}};
**            b[4][4]= {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}};    
**            fht2d(a,4,4);
**            fht2d(b,4,4);
**            conv2d(a,b,4,4);
**            fht2d(a,4,4);
**            for (i=0;i<4*4;i++) a[i]/=4;
**    Note: 
**        Analogous to complex multiplies in Fourier space; but avoids
**        the annoyance of dealing with complex numbers.
*/

#define F(f,i,j,m,n) (*(f+(i)+m*(j)))
#define CONVOLVE(i,j,k,l) \
     a= (F(f,i,j,m,n)*F(g,i,j,m,n) - F(f,k,l,m,n) * F(g,k,l,m,n)); \
     b= (F(f,k,l,m,n)*F(g,i,j,m,n) + F(f,i,j,m,n) * F(g,k,l,m,n)); \
     F(h,i,j,m,n) = (b+a)/2; \
     F(h,k,l,m,n) = (b-a)/2; 
    
conv(f,g,h,m,n)
float *f,*g,*h;
int m,n;
{
 int i,j,k,l;
 float a,b;
 F(f,0  ,0  ,m,n) *= F(g,0  ,0  ,m,n);
 F(f,0  ,n/2,m,n) *= F(g,0  ,n/2,m,n);
 F(f,m/2,0  ,m,n) *= F(g,m/2,0  ,m,n);
 F(f,m/2,n/2,m,n) *= F(g,m/2,n/2,m,n);
 for (i=1;i<m/2;i++)
    {	
     k=(m-i);
     CONVOLVE(i,0,k,0);
     CONVOLVE(i,n/2,k,n/2);
    }
 for (j=1;j<n/2;j++)
    {
     l=(n-j);
     CONVOLVE(0,j,0,l);
     CONVOLVE(m/2,j,m/2,l);
    }
 for (i=1;i<m/2;i++)
    {
     k=(m-i);
     for (j=1;j<n/2;j++)
        {
	 l=(n-j);
	 CONVOLVE(i,j,k,l);
	 CONVOLVE(i,l,k,j);
	}
    }
}
