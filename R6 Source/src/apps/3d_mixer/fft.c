/*
** Procedure: fft      PROGRAMMER: Ron Mayer  DATE: 1990,1993
**    Function: computes the Fourier Transform of an array
**    Inputs: n    = number of points
**            real = array containing the real part of the number
**            imag = array containing the imaginary part of the number
**    Output: The arrays real and imag get the fourier transform of themselves
**    Note: The routine ifft below is the inverse of this routine.
*/

void	fft(int n, float *real, float *imag)
{
	float	q,r,s,t;
	int		i,j;
 
	fht(real,n,1);
 	fht(imag,n,1);
 
	for (i=1,j=n-1;i<j;i++,j--) {
  		q=real[i]+real[j]; 
  		r=real[i]-real[j];
  		s=imag[i]+imag[j];
  		t=imag[i]-imag[j];
  		imag[i] = (s-r)*.5;
  		imag[j] = imag[i]+r;
  		real[i] = (q+t)*.5;
  		real[j] = real[i]-t;
 	}
}

void	ifft(int n, float *real,float *imag)
{
 	float	q,r,s,t,scale;
 	int 	i,j;

 	scale = 0.5/n;

	for (i=1,j=n-1;i<j;i++,j--) {
		 q = real[i]+real[j];
		 r = real[i]-real[j];
		 s = imag[i]+imag[j];
		 t = imag[i]-imag[j];
		 imag[i] = (s+r)*scale;
		 imag[j] = (s-r)*scale;
		 real[i] = (q-t)*scale;
		 real[j] = (q+t)*scale;
	}
 	scale *= 2;
 	real[0]*=scale;
 	imag[0]*=scale;
 	real[i]*=scale;
 	imag[i]*=scale;
 	fht(real,n,1);
 	fht(imag,n,1);
}


/*
** Procedure: realfft      PROGRAMMER: Ron Mayer  DATE: 04/10/93
**    Function: Computes FFT of a real valued data set
**    Inputs: n    = number of points
**            real = the array of data to be transformed.
**    Output: The first half of the array "real", (from 0 to n/2)
**            is filled with the real components of an FFT.
**            The second half of the array (from n/2+1 to n-1) is
**            filled with the imaginary components of an FFT (data at
**            n-1 is the lowest freq imag component).  
**    Note: Unlike alot of real valued ffts I do not generate the DC and
**          Nyquist imaginary component because for real valued data
**          these are always zero; allowing the entire transform to
**          fit in the original array.
**          The routine realifft is the inverse of this routine.
*/

int realfft(int n, float *real)
{
	float	a,b;
	int 	i,j;

	fht(real,n,1);

	for (i=1,j=n-1;i<j;i++,j--) {
		a = real[i];
		b = real[j];
		real[i] = (a+b)*0.5;
		real[j] = real[i]-b;
	}
 return(n);
}


int realifft(int n, float *real)
{
 	float	a,b;
 	int		i,j;
 	float	scale;

 	scale = 1.0/n;

	for (i=1,j=n-1;i<j;i++,j--) {
	a = real[i];
	b = real[j];
	real[j] = (a-b)*scale;
	real[i] = (a+b)*scale;
	}
	real[0]*=scale;
	real[i]*=scale;
	fht(real,n,1);
	return(n);
}
