//	resampler_cubic.h
//	Class to implement cubic interpolator

/* 0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0

	This works just like to the trilinear interpolator; it still interpolates over
	four points, just using a different formula.
	
	The Hermite cubic spline consists of two second-order parabolas, one through 
	the points (x0,y0), (x1,y1), and (x2,y2) and one through (x1,y1), (x2,y2), 
	and (x3,y3) with a linear ramp between them.
	
	To simplify, assume that x0 = -1, x1 = 0, x2 = 1, x3 = 2.  The point to be interpolated 
	is (xd,yd), with 0 <= xd < 1 (i.e. xd lies between x1 and x2).
	
	Hermite interpolation code:
	
	a = (3 * (y1-y2) - y0 + y3) * 0.5; 
	b = 2*y2 + y0 - (5*y1 + y3) * 0.5; 
	c = (y2 - y0) * 0.5; 
	yd = (((a * xd) + b) * xd + c) * xd + y1; 
	
  0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0 */

#ifndef _RESAMPLER_CUBIC_H_
#define _RESAMPLER_CUBIC_H_

#include <resampler.h>

//************************************************************************************
//
// cubic_state struct - holds the complete state of the interpolator. 
//
// xd is the position between input samples.  Since floating point comparisons are
// painful on the Pentium, it's faster to keep an integer and a floating point
// version.  This could be optimized for Pentium Pro and better by using the new
// floating point compare instructions.
//
//************************************************************************************

typedef struct 
{
	uint32		freq_in;
	uint32		freq_out;

	float		left0;
	float		left1;
	float		left2;
	float		left3;

	float		right0;
	float		right1;
	float		right2;
	float		right3;

	double		fl_xd_step;
	uint32		i_xd;
	double		freq_out_reciprocal;

	uint32		right_in_channel_offset;	

	float		left_gain;
	float		right_gain;

	const void	*in;
	size_t		in_cnt;
	void			*out;
	size_t		out_cnt;
	
	int32		channels;
	
} cubic_state;


//************************************************************************************
//
// Generic C++ resampler functions
//
//************************************************************************************

//-------------------------------------------------------------------------------------
//
// Resampler without mixing
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut> void cubic(cubic_state *state)
{
	SampOut *out 		= (SampOut *) state->out;
	SampIn *in			= (SampIn *) state->in;
	size_t out_cnt 		= state->out_cnt;
	size_t in_cnt 		= state->in_cnt;
	float gain0 			= state->left_gain;
	float gain1 			= state->right_gain;
	double fl_xd_step	= state->fl_xd_step;
	int32 i_xd 			= state->i_xd;
	int32 freq_out		= state->freq_out;	
	int32 freq_in		= state->freq_in;	
	int32 channels		= state->channels;
	uint32 right_offset	= state->right_in_channel_offset;
	float left0			= state->left0;
	float left1			= state->left1;	
	float left2			= state->left2;
	float left3			= state->left3;	
	float right0			= state->right0;
	float right1			= state->right1;	
	float right2			= state->right2;
	float right3			= state->right3;
	float a,b,c,left,right;
	float fl_xd;	
	
	//
	// Calculate fl_xd based on i_xd
	//
	fl_xd = ((double) (freq_out - i_xd)) * state->freq_out_reciprocal;
	
	//
	// Normalize the gain
	//
	gain0 *= sample_info<SampOut>::range/sample_info<SampIn>::range;
	gain1 *= sample_info<SampOut>::range/sample_info<SampIn>::range;

	while (in_cnt > 0 && out_cnt > 0)
	{
		//
		// Make new output samples as long as xd doesn't move to a new input samplex
		// 
		while (i_xd > 0 && out_cnt > 0) 
		{
			a = (3.0 * (left1 - left2) - left0 + left3) * 0.5; 
			b = 2.0 * left2 + left0 - (5.0 * left1 + left3) * 0.5; 
			c = (left2 - left0) * 0.5; 
			left = (((a * fl_xd) + b) * fl_xd + c) * fl_xd + left1; 

			a = (3.0 * (right1 - right2) - right0 + right3) * 0.5; 
			b = 2.0 * right2 + right0 - (5.0 * right1 + right3) * 0.5; 
			c = (right2 - right0) * 0.5; 
			right = (((a * fl_xd) + b) * fl_xd + c) * fl_xd + right1; 
			
			*(out++) = sample_conversion<float,SampOut>::convert(left, 1.0f);
			*(out++) = sample_conversion<float,SampOut>::convert(right, 1.0f);				
		
			i_xd -= freq_in;
			fl_xd += fl_xd_step;
			
			out_cnt--;
			
		}

		//
		// Skip ahead to the next input sample pair
		// 
		while (i_xd <= 0 && in_cnt > 0) 
		{
			left0 = left1;
			left1 = left2;
			left2 = left3;
			left3 = sample_conversion<SampIn, float>::convert(*in, gain0);				
			
			right0 = right1;
			right1 = right2;
			right2 = right3;
			right3 = sample_conversion<SampIn, float>::convert(in[right_offset], gain1);		

			in += channels;
			in_cnt--;
			
			fl_xd -= 1.0;	
			i_xd += freq_out;
		}

	}	// loop without mixing
	
	state->out_cnt = out_cnt;
	state->in_cnt = in_cnt;

	state->i_xd = i_xd;

	state->left0 = left0;
	state->left1 = left1;
	state->left2 = left2;
	state->left3 = left3;	

	state->right0 = right0;
	state->right1 = right1;
	state->right2 = right2;
	state->right3 = right3;	

} // cubic


//-------------------------------------------------------------------------------------
//
// Resampler with mixing
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut> void cubic_mix(cubic_state *state)
{
	SampOut *out 		= (SampOut *) state->out;
	SampIn *in			= (SampIn *) state->in;
	size_t out_cnt 		= state->out_cnt;
	size_t in_cnt 		= state->in_cnt;
	float gain0 			= state->left_gain;
	float gain1 			= state->right_gain;
	float fl_xd_step		= state->fl_xd_step;
	int32 i_xd 			= state->i_xd;
	int32 freq_out		= state->freq_out;	
	int32 freq_in		= state->freq_in;	
	int32 channels		= state->channels;
	uint32 right_offset	= state->right_in_channel_offset;
	float left0			= state->left0;
	float left1			= state->left1;	
	float left2			= state->left2;
	float left3			= state->left3;	
	float right0			= state->right0;
	float right1			= state->right1;	
	float right2			= state->right2;
	float right3			= state->right3;
	float a,b,c,left,right;	
	float fl_xd;
	
	//
	// Calculate fl_xd based on i_xd
	//
	fl_xd = ((double) (freq_out - i_xd)) * state->freq_out_reciprocal;
	
	//
	// Normalize the gain
	//
	gain0 *= sample_info<SampOut>::range/sample_info<SampIn>::range;
	gain1 *= sample_info<SampOut>::range/sample_info<SampIn>::range;

	while (in_cnt > 0 && out_cnt > 0)
	{
		//
		// Make new output samples as long as xd doesn't move to a new input samplex
		// 
		while (i_xd > 0 && out_cnt > 0) 
		{
			a = (3.0 * (left1 - left2) - left0 + left3) * 0.5; 
			b = 2.0 * left2 + left0 - (5.0 * left1 + left3) * 0.5; 
			c = (left2 - left0) * 0.5; 
			left = (((a * fl_xd) + b) * fl_xd + c) * fl_xd + left1; 

			a = (3.0 * (right1 - right2) - right0 + right3) * 0.5; 
			b = 2.0 * right2 + right0 - (5.0 * right1 + right3) * 0.5; 
			c = (right2 - right0) * 0.5; 
			right = (((a * fl_xd) + b) * fl_xd + c) * fl_xd + right1; 
			
			*(out++) += sample_conversion<float,SampOut>::convert(left, 1.0f);
			*(out++) += sample_conversion<float,SampOut>::convert(right, 1.0f);				
		
			i_xd -= freq_in;
			fl_xd += fl_xd_step;
			
			out_cnt--;
			
		}

		//
		// Skip ahead to the next input sample pair
		// 
		while (i_xd <= 0 && in_cnt > 0) 
		{
			left0 = left1;
			left1 = left2;
			left2 = left3;
			left3 = sample_conversion<SampIn, float>::convert(*in, gain0);				
			
			right0 = right1;
			right1 = right2;
			right2 = right3;
			right3 = sample_conversion<SampIn, float>::convert(in[right_offset], gain1);		

			in += channels;
			in_cnt--;
			
			fl_xd -= 1.0;	
			i_xd += freq_out;
		}

	}	// loop with mixing
	
	state->out_cnt = out_cnt;
	state->in_cnt = in_cnt;

	state->i_xd = i_xd;

	state->left0 = left0;
	state->left1 = left1;
	state->left2 = left2;
	state->left3 = left3;	

	state->right0 = right0;
	state->right1 = right1;
	state->right2 = right2;
	state->right3 = right3;	

} // cubic_mix


//************************************************************************************
//
// Wrapper templates for resampler functions
//
// The C++ code increments the input buffer pointer in units of samples, while
// the assembly code needs units of bytes.  The offset_shift field is used to take
// care of this.
//
//************************************************************************************

typedef void (*cubic_proc)(cubic_state *state);

template <class SampIn,class SampOut> class cubic_info 
{
	public:
	
	static const cubic_proc		proc = &cubic <SampIn,SampOut> ;
	static const cubic_proc		proc_mix = &cubic_mix <SampIn,SampOut>;
	static const uint32			offset_shift = 0;
};


#if __INTEL__&&SRC_ASM_OPTIMIZED

//-------------------------------------------------------------------------------------
//
// assembly function prototypes
//
//-------------------------------------------------------------------------------------

extern "C" void cubic_uint8_to_float(cubic_state *state);
extern "C" void cubic_uint8_to_float_mix(cubic_state *state);
extern "C" void cubic_int16_to_float(cubic_state *state);
extern "C" void cubic_int16_to_float_mix(cubic_state *state);
extern "C" void cubic_int32_to_float(cubic_state *state);
extern "C" void cubic_int32_to_float_mix(cubic_state *state);
extern "C" void cubic_float_to_float(cubic_state *state);
extern "C" void cubic_float_to_float_mix(cubic_state *state);


//-------------------------------------------------------------------------------------
//
// assembly template wrappers
//
//-------------------------------------------------------------------------------------

template <> class cubic_info <uint8,float>
{
	public:

	static const cubic_proc		proc = &cubic_uint8_to_float;	
	static const cubic_proc		proc_mix = &cubic_uint8_to_float_mix;
	static const uint32			offset_shift = 0;
};

template <> class cubic_info <int16,float>
{
	public:

	static const cubic_proc		proc = &cubic_int16_to_float;	
	static const cubic_proc		proc_mix = &cubic_int16_to_float_mix;
	static const uint32			offset_shift = 1;
};

template <> class cubic_info <int32,float>
{
	public:

	static const cubic_proc		proc = &cubic_int32_to_float;	
	static const cubic_proc		proc_mix = &cubic_int32_to_float_mix;
	static const uint32			offset_shift = 2;
};

template <> class cubic_info <float,float>
{
	public:

	static const cubic_proc		proc = &cubic_float_to_float;	
	static const cubic_proc		proc_mix = &cubic_float_to_float_mix;
	static const uint32			offset_shift = 2;
};

#endif // __INTEL__&&SRC_ASM_OPTIMIZED



//************************************************************************************
//
// Resampler template
//
//************************************************************************************

template <class SampIn, class SampOut>
class resampler_cubic : public _resampler_base {

private:


	cubic_proc		m_proc;
	cubic_proc		m_proc_mix;
	
public:

	cubic_state		m_state;
	
	resampler_cubic(int32 freq_in,int32 freq_out,int channels); 
	~resampler_cubic();
	void clear();
	virtual void resample_gain(
			const void * 		in,
			size_t & 		in_cnt,
			void * 			out,
			size_t & 		out_cnt,
			const float *		gain,
			bool 			mix);

};


//-------------------------------------------------------------------------------------
//
// Constructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_cubic<SampIn,SampOut>::resampler_cubic
(
	int32 freq_in,
	int32 freq_out,
	int channels
)
{
	assert(channels == 1 || channels == 2);
	
#ifdef SRC_PROFILE	
	m_freq_in = freq_in;
	m_freq_out = freq_out;
#endif
	
	m_state.freq_in = freq_in;
	m_state.freq_out = freq_out;
	m_state.channels = channels;
	m_state.fl_xd_step = ((double) freq_in) / ((double) freq_out);
	m_state.freq_out_reciprocal = 1.0 / ((double) freq_out);

	m_state.right_in_channel_offset = channels - 1;
	m_state.right_in_channel_offset <<= cubic_info<SampIn,SampOut>::offset_shift;
	
	m_proc = cubic_info<SampIn,SampOut>::proc;
	m_proc_mix = cubic_info<SampIn,SampOut>::proc_mix;
	
	clear();
	
	SRC_HOWDY(("New cubic interpolator, %ld to %ld\n",m_freq_in,m_freq_out));
	
} // constructor


//-------------------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_cubic<SampIn,SampOut>::~resampler_cubic()
{
} // destructor

	
//-------------------------------------------------------------------------------------
//
// clear
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_cubic<SampIn,SampOut>::clear()
{

	//
	// Set the initial state so that the resampler
	// will immediately fetch a frame from the input buffer
	// 
	m_state.i_xd = 0;
	
	m_state.left0 = 0.0;
	m_state.left1 = 0.0;
	m_state.left2 = 0.0;
	m_state.left3 = 0.0;
	m_state.right0 = 0.0;
	m_state.right1 = 0.0;
	m_state.right2 = 0.0;
	m_state.right3 = 0.0;
		
	_resampler_base::clear();

} // clear


//-------------------------------------------------------------------------------------
//
// resample_gain
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_cubic<SampIn,SampOut>::resample_gain
(
	const void * 	in,
	size_t & 	in_cnt,
	void * 		out,
	size_t & 	out_cnt,
	const float *	gain,
	bool 		mix
)
{
	SRC_PROFILE_START(out_cnt);		

	m_state.in = in;
	m_state.in_cnt = in_cnt;
	m_state.out = out;
	m_state.out_cnt = out_cnt;
	m_state.left_gain = gain[0];
	m_state.right_gain = gain[1];
	
	if (mix)
	{
		m_proc_mix(&m_state);
	}
	else
	{
		m_proc(&m_state);
	}
	
	in_cnt = m_state.in_cnt;
	out_cnt = m_state.out_cnt;
	
	SRC_PROFILE_END(out_cnt);

} // resample_gain


#endif // _RESAMPLER_CUBIC_H_
