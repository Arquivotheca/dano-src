#ifndef C_COMPLEX_H

#define C_COMPLEX_H

struct complex_t
{
	float re,im;
	
	void Set (float real, float imag)
	{
		re=real;
		im=imag;
	}
	
	complex_t &operator*= (const complex_t &z)
	{
		float new_re=re*z.re-im*z.im;
		im=re*z.im+im*z.re;
		re=new_re;
		
		return *this;
	}
	
	complex_t &operator+= (const complex_t &z)
	{
		re+=z.re;
		im+=z.im;

		return *this;
	}

	complex_t &operator-= (const complex_t &z)
	{

		re-=z.re;
		im-=z.im;
		
		return *this;
	}
};

#endif
