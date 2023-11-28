#include "ac3audio_imdct.h"
#include "ac3audio_downmix.h"

#include <support/Debug.h>
#include <math.h>

void
ac3audio_imdct (ac3audio_decoder_t *decoder, int16 *output)
{
	int32 ch;
		
	for (ch=0;ch<decoder->bsi.nfchans;++ch)
	{
		if (decoder->audblk.blksw[ch])
			ac3audio_imdct_256(decoder,decoder->coeffs.fbw[ch],decoder->samples->channel[ch],decoder->imdct.delay[ch]);
		else
			ac3audio_imdct_512(decoder,decoder->coeffs.fbw[ch],decoder->samples->channel[ch],decoder->imdct.delay[ch]);
	}
	
	if (decoder->bsi.lfeon)
		ac3audio_imdct_256(decoder,decoder->coeffs.lfe,decoder->samples->channel[5],decoder->imdct.delay[5]);
	
	switch (decoder->output_coding_mode)
	{
		case C_PASS_THROUGH:	
		{
			ac3audio_passthru(decoder,output);						
			break;
		}
		
		case C_CONVENTIONAL_STEREO:
		{
			ac3audio_conventional_stereo(decoder,output);
			break;
		}
		
		default:
		{
			TRESPASS();
			break;
		}
	}
}

void
ac3audio_imdct_destroy (ac3audio_decoder_t *decoder)
{
	free(decoder->imdct.Z);
	free(decoder->imdct.unit_circle_512);
}

void
ac3audio_imdct_init (ac3audio_decoder_t *decoder)
{
	const uint16 N=512;
	int32 k,m,ch,i;
	
	decoder->imdct.unit_circle_512=(complex_t *)memalign(16,sizeof(complex_t)*(512/4));
	decoder->imdct.Z=(complex_t *)memalign(16,sizeof(complex_t)*(512/4));
	
	for (k=0;k<N/4;++k)
	{
		double w=2.0*M_PI*((double)8*k+1)/((double)8*N);
		
		decoder->imdct.unit_circle_512[k].re=-cos(w);
		decoder->imdct.unit_circle_512[k].im=-sin(w);
	}

	for (k=0;k<N/8;++k)
	{
		double w=2.0*M_PI*((double)8*k+1)/((double)4*N);
		
		decoder->imdct.unit_circle_256[k].re=-cos(w);
		decoder->imdct.unit_circle_256[k].im=-sin(w);
	}
	
	for (m=0;m<7;++m)
	{
		const uint16 two_m=1<<m;
		const double two_m_plus_1=(double)two_m*2.0;
		
		const double angle=-2.0*M_PI/two_m_plus_1;
		const complex_t c = {cos(angle),sin(angle)};

		complex_t w = {1.0,0.0};
		
		for (k=0;k<two_m;++k)
		{
			decoder->imdct.w[m][k]=w;
			complex_multiply(&w,&c);
		}
	}
	
	for (ch=0;ch<6;++ch)
		for (i=0;i<256;++i)
			decoder->imdct.delay[ch][i]=0.0f;
}

void
complex_multiply (complex_t *a, const complex_t *b)
{
	float new_re=a->re*b->re-a->im*b->im;
	a->im=a->re*b->im+a->im*b->re;
	a->re=new_re;
}

void 
ac3audio_imdct_512 (ac3audio_decoder_t *decoder, const float *X, float *x, float *delay)
{
	int32 k,n;
	const int32 N=512;
	const float *w=decoder->window_coeffs;
		
	// Pre-IFFT complex multiply step
	
	complex_t *Z=decoder->imdct.Z;

	for (k=0;k<N/4;++k)
	{
	
		Z[k].re=X[N/2-2*k-1];
		Z[k].im=X[2*k];
		
#if !defined(USE_SSE)
		complex_multiply(&Z[k],&decoder->imdct.unit_circle_512[k]);
#endif
	}

#if defined(USE_SSE)
	multiply_complex_asm (Z,decoder->imdct.unit_circle_512,(N/4)/2);
#endif
	
	// Complex IFFT step
	
	ac3audio_ifft128(decoder,Z);
	
	// Post-IFFT complex multiply step
	
#if !defined(USE_SSE)
	for (n=0;n<N/4;++n)
		complex_multiply(&Z[n],&decoder->imdct.unit_circle_512[n]);
#else
	multiply_complex_asm (Z,decoder->imdct.unit_circle_512,(N/4)/2);
#endif
	
	// Windowing and de-interleaving step
	
	for (n=0;n<N/8;++n)
	{
#if !defined(USE_SSE)
		x[2*n]=-Z[N/8+n].im * w[2*n];
		x[2*n+1]=Z[N/8-n-1].re * w[2*n+1];

		x[N/4+2*n]=-Z[n].re * w[N/4+2*n];
		x[N/4+2*n+1]=Z[N/4-n-1].im * w[N/4+2*n+1];
#endif

		x[N/2+2*n]=-Z[N/8+n].re * w[N/2-2*n-1];
		x[N/2+2*n+1]=Z[N/8-n-1].im * w[N/2-2*n-2];

		x[3*N/4+2*n]=Z[n].im * w[N/4-2*n-1];
		x[3*N/4+2*n+1]=-Z[N/4-n-1].re * w[N/4-2*n-2];
	}
	
#if defined(USE_SSE)		
	interleaved_mult_imre_asm(x,&Z[N/8],&Z[N/8-2],w,(N/8)/2);
	interleaved_mult_reim_asm(&x[N/4],&Z[0],&Z[N/4-2],&w[N/4],(N/8)/2);
#endif

	// Overlap and add step
	
	for (n=0;n<N/2;++n)
	{
		x[n]=2.0f*(x[n]+delay[n]);		
		delay[n]=x[N/2+n];
	}
}

void 
ac3audio_imdct_256(ac3audio_decoder_t *decoder, const float *X, float *x, float *delay)
{
	int32 k,n;
	const float *w=decoder->window_coeffs;	
	const uint16 N=512;

	// Pre-IFFT complex multiply step

	complex_t *Z=decoder->imdct.Z;
	
	for (k=0;k<N/8;++k)
	{
		Z[k].re=X[2*(N/4-2*k-1)];
		Z[k].im=X[2*(2*k)];
		
		complex_multiply(&Z[k],&decoder->imdct.unit_circle_256[k]);
		
		Z[k+N/8].re=X[2*(N/4-2*k-1)+1];
		Z[k+N/8].im=X[2*(2*k)+1];
		
		complex_multiply(&Z[k+N/8],&decoder->imdct.unit_circle_256[k]);
	}
	
	// Complex IFFT step

	ac3audio_ifft64(decoder,Z);
	ac3audio_ifft64(decoder,Z+N/8);
	
	// Post-IFFT complex multiply step
	
	for (n=0;n<N/8;++n)
	{
		complex_multiply(&Z[n],&decoder->imdct.unit_circle_256[n]);
		complex_multiply(&Z[n+N/8],&decoder->imdct.unit_circle_256[n]);
	}
	
	// Windowing and de-interleaving step
	
	for (n=0;n<N/8;++n)
	{
		x[2*n]=-Z[n].im * w[2*n];
		x[2*n+1]=Z[N/8-n-1].re * w[2*n+1];

		x[N/4+2*n]=-Z[n].re * w[N/4+2*n];

		x[N/4+2*n+1]=Z[N/8-n-1].im * w[N/4+2*n+1];
		
		x[N/2+2*n]=-Z[n+N/8].re * w[N/2-2*n-1];
		x[3*N/4+2*n]=Z[n+N/8].im * w[N/4-2*n-1];

		x[N/2+2*n+1]=Z[N/8-n-1+N/8].im * w[N/2-2*n-2];
		x[3*N/4+2*n+1]=-Z[N/8-n-1+N/8].re * w[N/4-2*n-2];
	}

	// Overlap and add step
	
	for (n=0;n<N/2;++n)
	{
		x[n]=2.0f*(x[n]+delay[n]);		
		delay[n]=x[N/2+n];
	}
}

void
ac3audio_ifft64 (ac3audio_decoder_t *decoder, complex_t *Z)
{
	static const uint8 kBitReverse256[64] =
	{
		0x00, 0x20, 0x10, 0x30, 0x08, 0x28, 0x18, 0x38, 
		0x04, 0x24, 0x14, 0x34, 0x0c, 0x2c, 0x1c, 0x3c, 
		0x02, 0x22, 0x12, 0x32, 0x0a, 0x2a, 0x1a, 0x3a, 
		0x06, 0x26, 0x16, 0x36, 0x0e, 0x2e, 0x1e, 0x3e, 
		0x01, 0x21, 0x11, 0x31, 0x09, 0x29, 0x19, 0x39, 
		0x05, 0x25, 0x15, 0x35, 0x0d, 0x2d, 0x1d, 0x3d, 
		0x03, 0x23, 0x13, 0x33, 0x0b, 0x2b, 0x1b, 0x3b, 
		0x07, 0x27, 0x17, 0x37, 0x0f, 0x2f, 0x1f, 0x3f
	};

	int32 n,m,k,i;
	
	// z[n]:=Sum(Z[k]*exp(i*2.0*M_PI*k*n/(512/4)),k=0,..,512/4-1);

	for (n=0;n<64;++n)
	{
		Z[n].im=-Z[n].im;	// complex conjugate
	}		

	for (n=0;n<64;++n)
	{
		uint16 m=kBitReverse256[n];
		
		if (m<n)
		{
			complex_t temp=Z[m];			
			Z[m]=Z[n];
			Z[n]=temp;
		}
	}

	for (m=0;m<6;++m)
	{
		uint16 two_m=(1<<m);
		uint16 two_m_plus_one=(1<<(m+1));

		for(k=0;k<two_m;++k)
		{
			for(i=0;i<64;i+=two_m_plus_one)
			{
				uint16 p=k+i;
				uint16 q=p+two_m;

				complex_t c=Z[q];
				complex_multiply(&c,&decoder->imdct.w[m][k]);
				
				Z[q]=Z[p];
				Z[q].re-=c.re;
				Z[q].im-=c.im;
				
				Z[p].re+=c.re;
				Z[p].im+=c.im;
			}
		}
	}

	for (n=0;n<64;++n)
	{
		Z[n].im=-Z[n].im;	// complex conjugate
	}
}

void
ac3audio_ifft128 (ac3audio_decoder_t *decoder, complex_t *Z)
{
	// z[n]:=Sum(Z[k]*exp(i*2.0*M_PI*k*n/(512/4)),k=0,..,512/4-1);

	static const uint8 kBitReverse512[128] =
	{
		0x00, 0x40, 0x20, 0x60, 0x10, 0x50, 0x30, 0x70, 
		0x08, 0x48, 0x28, 0x68, 0x18, 0x58, 0x38, 0x78, 
		0x04, 0x44, 0x24, 0x64, 0x14, 0x54, 0x34, 0x74, 
		0x0c, 0x4c, 0x2c, 0x6c, 0x1c, 0x5c, 0x3c, 0x7c, 
		0x02, 0x42, 0x22, 0x62, 0x12, 0x52, 0x32, 0x72, 
		0x0a, 0x4a, 0x2a, 0x6a, 0x1a, 0x5a, 0x3a, 0x7a, 
		0x06, 0x46, 0x26, 0x66, 0x16, 0x56, 0x36, 0x76, 
		0x0e, 0x4e, 0x2e, 0x6e, 0x1e, 0x5e, 0x3e, 0x7e, 
		0x01, 0x41, 0x21, 0x61, 0x11, 0x51, 0x31, 0x71, 
		0x09, 0x49, 0x29, 0x69, 0x19, 0x59, 0x39, 0x79, 
		0x05, 0x45, 0x25, 0x65, 0x15, 0x55, 0x35, 0x75, 
		0x0d, 0x4d, 0x2d, 0x6d, 0x1d, 0x5d, 0x3d, 0x7d, 
		0x03, 0x43, 0x23, 0x63, 0x13, 0x53, 0x33, 0x73, 
		0x0b, 0x4b, 0x2b, 0x6b, 0x1b, 0x5b, 0x3b, 0x7b, 
		0x07, 0x47, 0x27, 0x67, 0x17, 0x57, 0x37, 0x77, 
		0x0f, 0x4f, 0x2f, 0x6f, 0x1f, 0x5f, 0x3f, 0x7f
	};

	int32 n,m,k,i;

	for (n=0;n<128;++n)
	{
		Z[n].im=-Z[n].im;	// complex conjugate
	}		

	for (n=0;n<128;++n)
	{
		uint16 m=kBitReverse512[n];
		
		if (m<n)
		{
			complex_t temp=Z[m];			
			Z[m]=Z[n];
			Z[n]=temp;
		}
	}

	for (m=0;m<7;++m)
	{
		uint16 two_m=(1<<m);
		uint16 two_m_plus_one=two_m<<1;

		for(k=0;k<two_m;++k)
		{
			for(i=0;i<128;i+=two_m_plus_one)
			{
				uint16 p=k+i;
				uint16 q=p+two_m;

				complex_t c=Z[q];
				complex_multiply(&c,&decoder->imdct.w[m][k]);
				
				Z[q]=Z[p];
				Z[q].re-=c.re;
				Z[q].im-=c.im;
				Z[p].re+=c.re;
				Z[p].im+=c.im;
			}
		}
	}

	for (n=0;n<128;++n)
	{
		Z[n].im=-Z[n].im;	// complex conjugate
	}
}

