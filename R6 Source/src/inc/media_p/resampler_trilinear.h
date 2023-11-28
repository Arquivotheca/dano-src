//	resampler_trilinear.h
//	Class to implement trilinear interpolator

/* 0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0

	This works similarly to the linear interpolator; now, however, this one 
	interpolates over four points instead of two.
	
	Here is Jon Watte's explanation of how the trilinear interpolator works, with some 
	renaming of variables by me:

	---------------------------------	
	
	Assume you interpolate between y1 and y2 at offset xd (0 <= xd < 1).
	There is a line through y0 and y1, a line through y1 and y2, and a line through
	y2 and y3. Now, we weight these lines according to some simple formula,
	such as (for example) the y1y2 line at a constant 0.5; the y0y1 line at a
	linear ramp from 0.5 to 0, and the y2y3 line at a linear ramp from 0 to 0.5.
	This ensures that the interpolated signal is continuous, passes through the
	points, and that the derivative is continuous, too. (The proof is simple).
	
	The formula that pops out is:
	
	yd = (y1 + (y1-y0)*xd)*(1-xd)/2 + (y1 + (y2-y1)*xd)/2 +
	  (y2 + (y3-y2)*(xd-1))*xd/2;
	
	This requires 6 multiplications if you factor out the division by 2, so that's
	not too bad.
	
	---------------------------------
	
	And there you have it.  Here's the formula rewritten:
	
	yd = 0.5 * ( (y1 + (y1-y0)*xd)*(1-xd) +
				 (y1 + (y2-y1)*xd) +
				 (y2 + (y3-y2)*(xd-1))*xd )
	
  0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0 */


#ifndef _RESAMPLER_TRILINEAR_H_
#define _RESAMPLER_TRILINEAR_H_

#include <resampler.h>

//************************************************************************************
//
// trilinear_state struct - holds the complete state of the interpolator. 
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
	
} trilinear_state;


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

template <class SampIn, class SampOut> void trilinear(trilinear_state *state)
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
	float left,right;	
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
			left = 0.5 * ( (left1 + (left1 - left0) * fl_xd) * (1.0 - fl_xd) +
						   (left1 + (left2 - left1) * fl_xd) +
						   (left2 + (left3 - left2) * (fl_xd - 1.0)) * fl_xd );

			*(out++) = sample_conversion<float,SampOut>::convert(left, 1.0f);
			
			right = 0.5 * ( (right1 + (right1 - right0) * fl_xd) * (1.0 - fl_xd) +
							(right1 + (right2 - right1) * fl_xd) +
							(right2 + (right3 - right2) * (fl_xd - 1.0)) * fl_xd );
		
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

	}
	
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

} // trilinear


//-------------------------------------------------------------------------------------
//
// Resampler with mixing
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut> void trilinear_mix(trilinear_state *state)
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
	float left,right;
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
			left = 0.5 * ( (left1 + (left1 - left0) * fl_xd) * (1.0 - fl_xd) +
						   (left1 + (left2 - left1) * fl_xd) +
						   (left2 + (left3 - left2) * (fl_xd - 1.0)) * fl_xd );

			*(out++) += sample_conversion<float,SampOut>::convert(left, 1.0f);
			
			right = 0.5 * ( (right1 + (right1 - right0) * fl_xd) * (1.0 - fl_xd) +
							(right1 + (right2 - right1) * fl_xd) +
							(right2 + (right3 - right2) * (fl_xd - 1.0)) * fl_xd );
		
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

	}
	
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
	
} // trilinear_mix


//************************************************************************************
//
// Wrapper templates for resampler functions
//
// The C++ code increments the input buffer pointer in units of samples, while
// the assembly code needs units of bytes.  The offset_shift field is used to take
// care of this.
//
//************************************************************************************

typedef void (*trilinear_proc)(trilinear_state *state);

template <class SampIn,class SampOut> class trilinear_info 
{
	public:
	
	static const trilinear_proc	proc = &trilinear <SampIn,SampOut> ;
	static const trilinear_proc	proc_mix = &trilinear_mix <SampIn,SampOut>;
	static const uint32			offset_shift = 0;
};


#if __INTEL__&&SRC_ASM_OPTIMIZED

//-------------------------------------------------------------------------------------
//
// assembly function prototypes
//
//-------------------------------------------------------------------------------------

extern "C" void trilinear_uint8_to_float(trilinear_state *state);
extern "C" void trilinear_uint8_to_float_mix(trilinear_state *state);
extern "C" void trilinear_int16_to_float(trilinear_state *state);
extern "C" void trilinear_int16_to_float_mix(trilinear_state *state);
extern "C" void trilinear_int32_to_float(trilinear_state *state);
extern "C" void trilinear_int32_to_float_mix(trilinear_state *state);
extern "C" void trilinear_float_to_float(trilinear_state *state);
extern "C" void trilinear_float_to_float_mix(trilinear_state *state);


//-------------------------------------------------------------------------------------
//
// assembly template wrappers
//
//-------------------------------------------------------------------------------------

template <> class trilinear_info <uint8,float>
{
	public:

	static const trilinear_proc	proc = &trilinear_uint8_to_float;	
	static const trilinear_proc	proc_mix = &trilinear_uint8_to_float_mix;
	static const uint32			offset_shift = 0;
};

template <> class trilinear_info <int16,float>
{
	public:

	static const trilinear_proc	proc = &trilinear_int16_to_float;	
	static const trilinear_proc	proc_mix = &trilinear_int16_to_float_mix;
	static const uint32			offset_shift = 1;
};

template <> class trilinear_info <int32,float>
{
	public:

	static const trilinear_proc	proc = &trilinear_int32_to_float;	
	static const trilinear_proc	proc_mix = &trilinear_int32_to_float_mix;
	static const uint32			offset_shift = 2;
};

template <> class trilinear_info <float,float>
{
	public:

	static const trilinear_proc	proc = &trilinear_float_to_float;	
	static const trilinear_proc	proc_mix = &trilinear_float_to_float_mix;
	static const uint32			offset_shift = 2;
};

#endif // __INTEL__&&SRC_ASM_OPTIMIZED


//************************************************************************************
//
// Resampler template
//
//************************************************************************************

template <class SampIn, class SampOut>
class resampler_trilinear : public _resampler_base {

private:

	trilinear_state		m_state;
	trilinear_proc		m_proc;
	trilinear_proc		m_proc_mix;
	
public:
	resampler_trilinear(int32 freq_in,int32 freq_out,int channels); 
	~resampler_trilinear();
	void clear();
	virtual void resample_gain(
			const void * 	in,
			size_t & 		in_cnt,
			void * 			out,
			size_t & 		out_cnt,
			const float *	gain,
			bool 			mix);
};


//-------------------------------------------------------------------------------------
//
// Constructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_trilinear<SampIn,SampOut>::resampler_trilinear
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
	m_state.right_in_channel_offset <<= trilinear_info<SampIn,SampOut>::offset_shift;
	
	m_proc = trilinear_info<SampIn,SampOut>::proc;
	m_proc_mix = trilinear_info<SampIn,SampOut>::proc_mix;
	
	clear();
	
	SRC_HOWDY(("New trilinear interpolator, %ld to %ld\n",m_freq_in,m_freq_out));
	
} // constructor


//-------------------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_trilinear<SampIn,SampOut>::~resampler_trilinear()
{
} // destructor

	
//-------------------------------------------------------------------------------------
//
// clear
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_trilinear<SampIn,SampOut>::clear()
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
void resampler_trilinear<SampIn,SampOut>::resample_gain
(
	const void * 	in,
	size_t & 		in_cnt,
	void * 			out,
	size_t & 		out_cnt,
	const float *	gain,
	bool 			mix
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

#endif // _RESAMPLER_TRILINEAR_H_
