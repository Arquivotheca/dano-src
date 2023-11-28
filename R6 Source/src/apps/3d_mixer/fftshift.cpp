#include <math.h> 
#include <stdlib.h>
#include <stdio.h>
#include <OS.h>
#include "fftshift.h" 
#include "fft.h"


//--------------------------------------------------------------------


FFTSHIFTER::FFTSHIFTER() 
{
	im = 0; 
	real = 0;
	im1 = 0; 
	real1 = 0;
}; 

//--------------------------------------------------------------------

FFTSHIFTER::~FFTSHIFTER() 
{
	free((char *)real);
	free((char *)im);
	free((char *)real1);
	free((char *)im1);
}; 

//--------------------------------------------------------------------

void FFTSHIFTER::Reset()
{
	long	i;

	input_pointer = 0;
	output_pointer = 0;
}

//--------------------------------------------------------------------

void FFTSHIFTER::SetSize(int size) 
{ 
    long	sz;
	long	i;

	sz = 1;

	while(sz < size)
		sz *= 2;


	total_points = sz;

	real = (float *)malloc(sz * sizeof(float));
	im = (float *)malloc(sz * sizeof(float));
	real1 = (float *)malloc(sz * sizeof(float));
	im1 = (float *)malloc(sz * sizeof(float));

	for (i = 0; i < sz; i++) {
		real[i] = 0;
		im[i] = 0;
		real1[i] = 0;
		im1[i] = 0;
	}

	input_pointer = 0;
	output_pointer = 0;
}; 

//--------------------------------------------------------------------

void FFTSHIFTER::SetParameters(int new_mode) 
{ 
	mode = new_mode; 
}; 

//--------------------------------------------------------------------

void FFTSHIFTER::Add(float x) 
{ 
	real[input_pointer] = (float)x; 
	im[input_pointer] = 0; 
	input_pointer++; 
}; 

//--------------------------------------------------------------------

void FFTSHIFTER::Perform_FFT(void) 
{ 
	long	i;
	long	j;
	long	j1;
	float	norm;
	float	phase;
	float	phase1;
	float	r,ima;
	float	mag;
	float	r1,ima1;
	float	max = 0;
	long	maxi;
	float	ik = 1.0/32768.0;

	for (i = 0; i < total_points; i++) {
		real[i] *= ik;
	}
	

	fft(total_points, real, im);

	for (j = 0; j < total_points; j++) {
		real1[j] = 0;
		im1[j] = 0;
	}
	
#define	SK	80
	
	for (j = 0; j < SK; j++) {
		real1[j] = real[j];
		im1[j] = im[j];
		real1[j] = 0;
		im1[j] = 0;
	}


long	ij;

	for (j = SK; j < total_points; j++) {
		r = real[j];
		ima = im[j];

		mag = sqrt(r*r + ima*ima);
		phase = atan2(ima, r);

		phase = 0;

		ima = sin(phase) * mag;
		r = cos(phase) * mag;

		real1[(int)(j*1.0)] += r;
		im1[(int)(j*1.0)] += ima;
	}

	
	ifft(total_points, real1, im1);

	ik = 32768.0;

	for (i = 0; i < total_points; i++) {
		real[i] = real1[i] * ik;
	}
}; 

//-----------------------------------------------------------------------

short FFTSHIFTER::Get_Result(void) 
{ 
	float result = real[output_pointer]; 
        
	if (result>32767) 
		result=32767; 
	if (result<-32768) 
		result=-32768; 
	
	output_pointer++; 
	return (short)result; 
};

//-----------------------------------------------------------------------

short FFTSHIFTER::Get_Result(long i) 
{ 
	float result = real[i]; 
        
	if (result > 32767) 
		result = 32767; 
	if (result < -32768) 
		result = -32768; 
	
	return (short)result; 
};

