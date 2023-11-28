//	resampler_zero.h
//	Header file for class to implement zeroth-order resampler

/* 0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0

	-- How the resamplers work --
	
The goal of the resampler routines is to take an input signal at one sample rate and
create a new output signal at a different sample rate.  The pitch of the input signal
should be preserved.

Let's take a simple case: resampling from 22,050 Hz to 44,100 Hz.  This is a ratio of
1:2.  Suppose that the input signal is mono (one sample per frame) and you want to 
create a mono output signal.  If the input signal looks like this:

a b c d e f g h ...

Then the output signal will look like this:

a a' b b' c c' d d' e e' f f' g g' h h' ...

So the resampler needs to interpolate the new in-between samples (a', b', and so
forth). All of the resamplers work in essentially the same manner; it's mostly a 
question of what mathematical function is used to interpolate the new samples.

In general, the resamplers are made up of a pair of loops:
	
	while (you have enough buffer space on the input & output)
	{
		(1) interpolate new samples until you've made the right number of samples
		(2) move ahead in the input buffer to the next position
	}

The number of times both loops with run is determined by the ratio of the input sample
rate divided by the output sample rate.  Each loop keeps track of a quantity called 
the phase.  The phase represents the fractional position within the input bufer that
is currently being interpolated.  The phase starts at zero.
	
Loop (1) uses the sample rate ratio and the phase to keep track of how many samples
to make.  On each pass of loop (1), the phase is incremented by the sample rate ratio.
Loop (1) exits when the phase is greater than or equal to zero.  For our example, 
the ratio is 0.5, so:

	1st pass:	phase 0.0, phase += 0.5, make a sample
	2nd pass: 	phase 0.5, phase += 0.5, make a sample
	3rd pass: 	phase is 1.0, exit loop

So for our example, loop (1) will make two samples every time it is run.	

Loop (2) does just the opposite; it runs until the phase is less than zero.  On each 
pass of loop (2), the resampler moves ahead by one frame in the input buffer and
decrements the phase by 1.0 (if you like, we are just moving the origin of the 
coordinate system.  Ignore this comment if it doesn't help you).

	1st pass:	phase = 1.0,  phase -= 1.0, move ahead by one input frame
	2nd pass: 	phase = 0.0,  exit loop

All of the resamplers use this basic structure.

resampler_zero uses the worst possible algorithm to interpolate new samples.  It
just repeats the previous sample from the input buffer.  That's it.  It's evil but
cheap.

For this resampler, the sample rate ratio and the phase are kept as integers, not
floats, but it still works the same way.

  0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0 */

#ifndef _RESAMPLER_ZERO_H_
#define _RESAMPLER_ZERO_H_

#include <resampler.h>

//************************************************************************************
//
// zero_state struct - holds the complete state of the interpolator. 
//
//************************************************************************************

typedef struct
{
	uint32		freq_in;
	uint32		freq_out;
	
	uint32		previous[2];	// use uint32 here to reserve 8 bytes

	int32		i_xd;

	uint32		right_in_channel_offset;	

	float		left_gain;
	float		right_gain;

	const void	*in;
	size_t		in_cnt;
	void		*out;
	size_t		out_cnt;
	
	int			channels;	
	
} zero_state;


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

template <class SampIn, class SampOut> void zero(zero_state *state)
{
	SampOut *out 	= (SampOut *) state->out;
	SampIn *in		= (SampIn *) state->in;
	size_t out_cnt 	= state->out_cnt;
	size_t in_cnt 	= state->in_cnt;
	float gain0 	= state->left_gain;
	float gain1 	= state->right_gain;
	int32 xd 		= state->i_xd;
	int32 freq_out	= state->freq_out;	
	int32 freq_in	= state->freq_in;	
	int32 channels  = state->channels;
	SampOut left;
	SampOut right;

	gain0 *= sample_info<SampOut>::range/sample_info<SampIn>::range;
	gain1 *= sample_info<SampOut>::range/sample_info<SampIn>::range;
	
	left = * ((SampOut *) (state->previous));
	right = * ((SampOut *) (state->previous + 1));

loop:
		
	while (xd > 0 && out_cnt > 0) {
		*(out++) = left;
		*(out++) = right;

		xd -= freq_in;
		out_cnt--;
	}
	
	if (in_cnt <= 0 || out_cnt <= 0)
		goto exit;

	while (xd <= 0 && in_cnt > 0) {
		xd += freq_out;
		in += channels;
		in_cnt--;
	}
	left = sample_conversion<SampIn, SampOut>::convert(in[-channels], gain0);
	right = sample_conversion<SampIn, SampOut>::convert(in[-1], gain1);
	goto loop;

exit:
	* ((SampOut *) (state->previous)) = left;
	* ((SampOut *) (state->previous + 1)) = right;
	
	state->out_cnt = out_cnt;
	state->in_cnt = in_cnt;
	state->i_xd = xd;	
	
} // zero


//-------------------------------------------------------------------------------------
//
// Zeroth order resampler without mixing
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut> void zero_mix(zero_state *state)
{
	SampOut *out 	= (SampOut *) state->out;
	SampIn *in		= (SampIn *) state->in;
	size_t out_cnt 	= state->out_cnt;
	size_t in_cnt 	= state->in_cnt;
	float gain0 	= state->left_gain;
	float gain1 	= state->right_gain;
	int32 xd 		= state->i_xd;
	int32 freq_out	= state->freq_out;	
	int32 freq_in	= state->freq_in;	
	int32 channels  = state->channels;
	SampOut left;
	SampOut right;

	gain0 *= sample_info<SampOut>::range/sample_info<SampIn>::range;
	gain1 *= sample_info<SampOut>::range/sample_info<SampIn>::range;
	
	left = * ((SampOut *) (state->previous));
	right = * ((SampOut *) (state->previous + 1));

loop:
		
	while (xd > 0 && out_cnt > 0) {
		*(out++) += left;
		*(out++) += right;

		xd -= freq_in;
		out_cnt--;
	}
	
	if (in_cnt <= 0 || out_cnt <= 0)
		goto exit;

	while (xd <= 0 && in_cnt > 0) {
		xd += freq_out;
		in += channels;
		in_cnt--;
	}
	left = sample_conversion<SampIn, SampOut>::convert(in[-channels], gain0);
	right = sample_conversion<SampIn, SampOut>::convert(in[-1], gain1);
	goto loop;

exit:
	* ((SampOut *) (state->previous)) = left;
	* ((SampOut *) (state->previous + 1)) = right;
	
	state->out_cnt = out_cnt;
	state->in_cnt = in_cnt;
	state->i_xd = xd;	
	
}	// zero_mix


//************************************************************************************
//
// Wrapper templates for resampler functions
//
//************************************************************************************

typedef void (*zero_proc)(zero_state *state);

template <class SampIn,class SampOut> class zero_info 
{
	public:
	
	static const zero_proc	proc = &zero <SampIn,SampOut> ;
	static const zero_proc	proc_mix = &zero_mix <SampIn,SampOut>;
};


#if __INTEL__&&SRC_ASM_OPTIMIZED

//-------------------------------------------------------------------------------------
//
// assembly function prototypes
//
//-------------------------------------------------------------------------------------

extern "C" void zero_uint8_to_float(zero_state *state);
extern "C" void zero_uint8_to_float_mix(zero_state *state);
extern "C" void zero_int16_to_float(zero_state *state);
extern "C" void zero_int16_to_float_mix(zero_state *state);
extern "C" void zero_int32_to_float(zero_state *state);
extern "C" void zero_int32_to_float_mix(zero_state *state);
extern "C" void zero_float_to_float(zero_state *state);
extern "C" void zero_float_to_float_mix(zero_state *state);


//-------------------------------------------------------------------------------------
//
// assembly template wrappers
//
//-------------------------------------------------------------------------------------

template <> class zero_info <uint8,float>
{
	public:

	static const zero_proc	proc = &zero_uint8_to_float;	
	static const zero_proc	proc_mix = &zero_uint8_to_float_mix;
};

template <> class zero_info <int16,float>
{
	public:

	static const zero_proc	proc = &zero_int16_to_float;	
	static const zero_proc	proc_mix = &zero_int16_to_float_mix;
};

template <> class zero_info <int32,float>
{
	public:

	static const zero_proc	proc = &zero_int32_to_float;	
	static const zero_proc	proc_mix = &zero_int32_to_float_mix;
};

template <> class zero_info <float,float>
{
	public:

	static const zero_proc	proc = &zero_float_to_float;	
	static const zero_proc	proc_mix = &zero_float_to_float_mix;
};

#endif // __INTEL__&&SRC_ASM_OPTIMIZED


//************************************************************************************
//
// Resampler template
//
//************************************************************************************

template <class SampIn, class SampOut>
class resampler_zero : public _resampler_base 
{

private:
	
	zero_state	m_state;
	zero_proc	m_proc;
	zero_proc	m_proc_mix;
	
public:
	resampler_zero(int32 freq_in,int32 freq_out,int channels);
	~resampler_zero();
	void clear();
	virtual void resample_gain(
			const void * in,
			size_t & in_cnt,
			void * out,
			size_t & out_cnt,
			const float *gain,
			bool mix);
};


//-------------------------------------------------------------------------------------
//
// Constructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_zero<SampIn,SampOut>::resampler_zero(int32 freq_in,int32 freq_out,int channels)
{
	assert(channels == 1 || channels == 2);
	
#ifdef SRC_PROFILE	
	m_freq_in = freq_in;
	m_freq_out = freq_out;
#endif

	m_state.freq_in = freq_in;
	m_state.freq_out = freq_out;
	m_state.channels = channels;
	
	clear();

	m_proc = zero_info <SampIn,SampOut>::proc;
	m_proc_mix = zero_info <SampIn,SampOut>::proc_mix;
	
	if (1 == channels)
	{
		// mono input buffer
		m_state.right_in_channel_offset = 0;
	}	
	else
	{
		// stereo input buffer
		m_state.right_in_channel_offset = sizeof(SampIn);
	}
	
	SRC_HOWDY(("New Crap-O-Matic interpolator, %ld to %ld\n",m_state.freq_in,m_state.freq_out));

} // resampler_zero::resample_zero


//-------------------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
resampler_zero<SampIn,SampOut>::~resampler_zero()
{
}


//-------------------------------------------------------------------------------------
//
// clear
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_zero<SampIn,SampOut>::clear()
{
	m_state.i_xd = 0;	// start at zero to force the resampler
						// to fetch new input data right away
	
	*( (SampOut *) (m_state.previous)) = (SampOut) sample_info<SampOut>::mid_value;
	*( (SampOut *) (m_state.previous + 1)) = (SampOut) sample_info<SampOut>::mid_value;	

	_resampler_base::clear();
}


//-------------------------------------------------------------------------------------
//
// resample_gain
//
//-------------------------------------------------------------------------------------

template <class SampIn, class SampOut>
void resampler_zero<SampIn,SampOut>::resample_gain
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

#endif // _RESAMPLER_ZERO_H_
