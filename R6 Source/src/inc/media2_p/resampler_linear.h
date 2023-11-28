//	resampler_linear.h
//	Class to implement first order resampler (linear interpolator)

/* 0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0

	Structurally, this resampler works just like the zeroth-order resampler.  The 
	difference lies in the fact that this resampler linearly interpolates between two
	sampler instead of just repeating the sample values.

	Since this is a linear interpolator, the goal is to solve the equation
	
	y = Ax + B
	
	given that you know two pairs of x and y (x0,y0) and (x1,y1).  The equation
	can be simplified by assuming that x0 = 0 and x1 = 1, thus yielding:
	
	y0 = B
	y1 = A + B
	y1 = A + y0
	y1 - y0 = A
	
	The equation can therefore be solved for any point (xd,yd):
	
	yd = (y1 - y0) xd + y0
	
	assuming that 0 <= xd < 1.

  0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0 */

#ifndef _RESAMPLER_LINEAR_H_
#define _RESAMPLER_LINEAR_H_

#include <resampler.h>

//************************************************************************************
//
// linear_state struct - holds the complete state of the interpolator. 
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
	float		right0;
	float		right1;

	double		fl_xd_step;
	int32		i_xd;
	double		freq_out_reciprocal;

	uint32		right_in_channel_offset;	

	float		left_gain;
	float		right_gain;

	const void	*in;
	size_t		in_cnt;
	void			*out;
	size_t		out_cnt;
	
	int32		channels;
	
} linear_state;


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

template <class SampIn, class SampOut> void linear(linear_state *state)
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
	float right0			= state->right0;
	float right1			= state->right1;	
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
		// Make new output samples as long as the phase stays between the current sample pair
		//
		// Floating-point compares are slow, so use an integer & floating-point xd
		// 
		while (i_xd > 0 && out_cnt > 0) 
		{
			*(out++) = sample_conversion<float,SampOut>::
					   	convert( (left1 - left0) * fl_xd + left0, 1.0f );
			*(out++) = sample_conversion<float,SampOut>::
						convert( (right1 - right0) * fl_xd + right0, 1.0f );				
		
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
			right0 = right1;

			left1 = sample_conversion<SampIn, float>::convert(*in, gain0);				
			right1 = sample_conversion<SampIn, float>::convert(in[right_offset], gain1);
			
			in += channels;
			in_cnt--;
			
			fl_xd -= 1.0;
			i_xd += freq_out;
		}

	}	// resampler loop
	
	state->out_cnt = out_cnt;
	state->in_cnt = in_cnt;

	state->i_xd = i_xd;

	state->left0 = left0;
	state->left1 = left1;
	
	state->right0 = right0;
	state->right1 = right1;
	
} // linear (no mixing)


template <class SampIn, class SampOut> void linear_mix(linear_state *state)
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
	float right0			= state->right0;
	float right1			= state->right1;	
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
		// Make new output samples as long as the phase stays between the current sample pair
		//
		// Floating-point compares are slow, so use an integer & floating-point xd
		// 
		while (i_xd > 0 && out_cnt > 0) 
		{
			*(out++) += sample_conversion<float,SampOut>::
					   	convert( (left1 - left0) * fl_xd + left0, 1.0f );
			*(out++) += sample_conversion<float,SampOut>::
						convert( (right1 - right0) * fl_xd + right0, 1.0f );				
		
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
			right0 = right1;

			left1 = sample_conversion<SampIn, float>::convert(*in, gain0);				
			right1 = sample_conversion<SampIn, float>::convert(in[right_offset], gain1);

			in += channels;
			in_cnt--;
			
			fl_xd -= 1.0;	
			i_xd += freq_out;
		}

	}	// resampler loop
	
	state->out_cnt = out_cnt;
	state->in_cnt = in_cnt;

	state->i_xd = i_xd;

	state->left0 = left0;
	state->left1 = left1;

	state->right0 = right0;	
	state->right1 = right1;
	
} // linear_mix


//************************************************************************************
//
// Wrapper templates for resampler functions
//
// The C++ code increments the input buffer pointer in units of samples, while
// the assembly code needs units of bytes.  The offset_shift field is used to take
// care of this.
//
//************************************************************************************

typedef void (*linear_proc)(linear_state *state);

template <class SampIn,class SampOut> class linear_info 
{
	public:
	
	static const linear_proc	proc = &linear <SampIn,SampOut> ;
	static const linear_proc	proc_mix = &linear_mix <SampIn,SampOut>;
	static const uint32			offset_shift = 0;
};


#if __INTEL__&&SRC_ASM_OPTIMIZED

//-------------------------------------------------------------------------------------
//
// assembly function prototypes
//
//-------------------------------------------------------------------------------------

extern "C" void linear_uint8_to_float(linear_state *state);
extern "C" void linear_uint8_to_float_mix(linear_state *state);
extern "C" void linear_int16_to_float(linear_state *state);
extern "C" void linear_int16_to_float_mix(linear_state *state);
extern "C" void linear_int32_to_float(linear_state *state);
extern "C" void linear_int32_to_float_mix(linear_state *state);
extern "C" void linear_float_to_float(linear_state *state);
extern "C" void linear_float_to_float_mix(linear_state *state);


//-------------------------------------------------------------------------------------
//
// assembly template wrappers
//
// For the assembly wrappers, offset_shift is always equal to the # of bytes per sample
//
//-------------------------------------------------------------------------------------

template <> class linear_info <uint8,float>
{
	public:

	static const linear_proc	proc = &linear_uint8_to_float;	
	static const linear_proc	proc_mix = &linear_uint8_to_float_mix;
	static const uint32			offset_shift = 0;
};

template <> class linear_info <int16,float>
{
	public:

	static const linear_proc	proc = &linear_int16_to_float;	
	static const linear_proc	proc_mix = &linear_int16_to_float_mix;
	static const uint32			offset_shift = 1;
};

template <> class linear_info <int32,float>
{
	public:

	static const linear_proc	proc = &linear_int32_to_float;	
	static const linear_proc	proc_mix = &linear_int32_to_float_mix;
	static const uint32			offset_shift = 2;
};

template <> class linear_info <float,float>
{
	public:

	static const linear_proc	proc = &linear_float_to_float;	
	static const linear_proc	proc_mix = &linear_float_to_float_mix;
	static const uint32			offset_shift = 2;
};

#endif // __INTEL__&&SRC_ASM_OPTIMIZED


//************************************************************************************
//
// Resampler template
//
//************************************************************************************

template <class SampIn, class SampOut>
class resampler_linear : public _resampler_base {

private:
	linear_state	m_state;
	linear_proc		m_proc;
	linear_proc		m_proc_mix;
	
public:
	resampler_linear(int32 freq_in,int32 freq_out,int channels); 
	~resampler_linear();
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
resampler_linear<SampIn,SampOut>::resampler_linear
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
	m_state.right_in_channel_offset <<= linear_info<SampIn,SampOut>::offset_shift;
	
	m_proc = linear_info<SampIn,SampOut>::proc;
	m_proc_mix = linear_info<SampIn,SampOut>::proc_mix;

	clear();
		
	SRC_HOWDY(("New linear interpolator, %ld to %ld\n",freq_in,freq_out));
	
} // constructor


//-------------------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_linear<SampIn,SampOut>::~resampler_linear()
{
} // destructor

	
//-------------------------------------------------------------------------------------
//
// clear
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_linear<SampIn,SampOut>::clear()
{
	//
	// Set the initial state so that the resampler
	// will immediately fetch a frame from the input buffer
	// 
	m_state.i_xd = 0;

	m_state.left0 = 0.0;
	m_state.left1 = 0.0;
	m_state.right0 = 0.0;
	m_state.right1 = 0.0;
		
	_resampler_base::clear();

} // clear


//-------------------------------------------------------------------------------------
//
// resample_gain
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_linear<SampIn,SampOut>::resample_gain
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

#endif // ifndef _RESAMPLER_LINEAR_H_

